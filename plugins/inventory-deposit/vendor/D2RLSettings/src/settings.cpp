#include <D2RLSettings/settings.h>
#include <D2RLSettings/toml_bool.h>

#include <algorithm>
#include <cstring>

namespace D2RLSettings {

namespace {

auto ReadConfigText(const D2RL::PluginContext* context, std::string& text) noexcept -> bool {
	if (context == nullptr) return false;
	try {
		uint32_t required = 0;
		// A sizing call reports false with the size; an absent file has no size.
		if (!context->ReadConfig(nullptr, 0, &required) && required == 0) {
			text.clear();
			return true;
		}
		text.resize(required + 1U, '\0');
		if (!context->ReadConfig(text.data(), static_cast<uint32_t>(text.size()), nullptr)) return false;
		text.resize(std::strlen(text.c_str()));
		return true;
	} catch (...) {
		return false;
	}
}

auto SanitizeHostKey(std::string_view raw) -> std::string {
	std::string out;
	out.reserve(raw.size());
	for (const char c : raw) {
		if (c >= 'A' && c <= 'Z') {
			out += static_cast<char>(c - 'A' + 'a');
		} else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-') {
			out += c;
		} else {
			out += '_';
		}
	}
	if (out.size() > 48) out.resize(48);
	return out;
}

auto BuildHostKey(std::string_view section, std::string_view key) -> std::string {
	if (section.empty()) return SanitizeHostKey(key);
	return SanitizeHostKey(std::string(section) + "_" + std::string(key));
}

auto EqualsIgnoreCase(std::string_view a, std::string_view b) noexcept -> bool {
	if (a.size() != b.size()) return false;
	for (size_t i = 0; i < a.size(); ++i) {
		char ca = a[i];
		char cb = b[i];
		if (ca >= 'A' && ca <= 'Z') ca = static_cast<char>(ca - 'A' + 'a');
		if (cb >= 'A' && cb <= 'Z') cb = static_cast<char>(cb - 'A' + 'a');
		if (ca != cb) return false;
	}
	return true;
}

constexpr uint32_t ToggleRequiredServiceSize   = static_cast<uint32_t>(offsetof(ServiceV1, addSlider));
constexpr uint32_t ToggleSyncServiceSize       = static_cast<uint32_t>(offsetof(ServiceV1, setSlider));
constexpr uint32_t SliderRequiredServiceSize   = static_cast<uint32_t>(offsetof(ServiceV1, addDropdown));
constexpr uint32_t DropdownRequiredServiceSize = static_cast<uint32_t>(offsetof(ServiceV1, setDropdownOptions));
constexpr uint32_t DropdownOptionsServiceSize  = static_cast<uint32_t>(offsetof(ServiceV1, addStatus));
constexpr uint32_t StatusRequiredServiceSize   = ServiceV1Size;

} // namespace

// ---------------------------------------------------------------------------
// Toggle
// ---------------------------------------------------------------------------

auto Toggle::Register(const D2RL::PluginContext* context, const ToggleOptions& options) noexcept -> bool {
	if (context == nullptr || options.key == nullptr || options.label == nullptr) return false;
	m_context = context;
	try {
		std::string_view parsedSection;
		std::string_view parsedKey;
		Detail::SplitSectionAndKey(options.key, options.tomlSection != nullptr ? options.tomlSection : "", parsedSection, parsedKey);
		if (parsedKey.empty()) return false;
		m_tomlSection = std::string(parsedSection);
		m_key         = std::string(parsedKey);
		m_hostKey     = BuildHostKey(m_tomlSection, m_key);
		m_label       = options.label;
		m_tooltip     = options.tooltip != nullptr ? options.tooltip : "";
	} catch (...) {
		return false;
	}
	m_section    = options.section;
	m_default    = options.defaultValue;
	m_boundValue = options.boundValue;
	m_onChanged  = options.onChanged;

	bool        value = options.defaultValue;
	std::string text;
	if (ReadConfigText(context, text)) {
		value = ReadBool(text, m_key, options.defaultValue, m_tomlSection);
	} else {
		D2RL::LogWarnF(context, "settings: config could not be read; %s starts at its default.", m_key.c_str());
	}
	m_value.store(value, std::memory_order_release);
	m_lastTomlValue.store(value, std::memory_order_release);
	if (m_boundValue != nullptr) std::atomic_ref<bool>(*m_boundValue).store(value, std::memory_order_release);

	if (context->QueryService(&m_communication) != D2RL::ServiceQueryResult::Success
	    || !D2RL::HasPluginCommunicationServiceField(m_communication, D2RL::PluginCommunicationServiceRequiredSize)) {
		m_communication = nullptr;
		D2RL::LogWarnF(context, "settings: no plugin communication; %s has no row in Options.", m_key.c_str());
		return true;
	}
	if (Attach()) return true;

	// The host normally loads before any plugin that uses it (its id sorts first).
	// For one that loads earlier (such as "Item Roll Ranges.dll"), subscribe to the
	// host's ready event so we attach during the host's own load, and also queue a
	// UI-thread retry so a missing or older host still logs a diagnostic.
	if (m_communication->subscribeEvent != nullptr) {
		const D2RL::PluginCommunication::EventSubscription sub {
			.structSize        = D2RL::PluginCommunication::EventSubscriptionSize,
			.flags             = 0,
			.publisherPluginId = ProviderPluginId,
			.name              = ReadyEventName,
			.minimumVersion    = ReadyEventVersion,
			.minimumDataSize   = 0,
			.callback          = OnHostReady,
			.userData          = this,
		};
		(void)m_communication->subscribeEvent(context, &sub, &m_readySubscription);
	}
	const D2RL::ThreadService* threads = nullptr;
	if (context->QueryService(&threads) == D2RL::ServiceQueryResult::Success && threads != nullptr
	    && threads->runOnUiThread(context, RetryOnUiThread, this) == D2RL::Threads::Result::Success) {
		return true;
	}
	if (m_readySubscription == D2RL::PluginCommunication::InvalidSubscriptionHandle) {
		D2RL::LogWarnF(context, "settings: the host plugin is not loaded; %s has no row in Options.", m_key.c_str());
	}
	return true;
}

auto Toggle::Attach() noexcept -> bool {
	if (m_row.load(std::memory_order_acquire) != 0) return true;
	if (D2RL::PluginCommunication::Acquire(m_context, m_communication, ProviderPluginId, ServiceName, ServiceVersion, ToggleRequiredServiceSize, &m_service)
	    != D2RL::PluginCommunication::Result::Success) {
		return false;
	}
	const ToggleSpec spec {
		.structSize   = static_cast<uint32_t>(sizeof(ToggleSpec)),
		.section      = m_section,
		.pluginId     = m_context->pluginId,
		.key          = m_hostKey.c_str(),
		.label        = m_label.c_str(),
		.tooltip      = m_tooltip.empty() ? nullptr : m_tooltip.c_str(),
		.value        = Value(),
		.defaultValue = m_default,
		.onApplied    = OnApplied,
		.userData     = this,
		.onRefresh    = OnRefresh,
	};
	RowHandle  row    = 0;
	const auto result = m_service->addToggle(&spec, &row);
	if (result != Result::Success) {
		D2RL::LogWarnF(m_context, "settings: the host refused %s (result %u); it has no row in Options.", m_hostKey.c_str(), static_cast<unsigned>(result));
		m_service.Reset();
		return true; // Asked and answered: no point retrying.
	}
	m_row.store(row, std::memory_order_release);
	return true;
}

void __cdecl Toggle::OnHostReady(const D2RL::PluginContext*, const D2RL::PluginCommunication::EventView*, void* self) noexcept {
	auto* toggle = static_cast<Toggle*>(self);
	(void)toggle->Attach();
}

void __cdecl Toggle::RetryOnUiThread(const D2RL::PluginContext* context, void* self) noexcept {
	auto* toggle = static_cast<Toggle*>(self);
	if (!toggle->Attach()) {
		D2RL::LogWarnF(context, "settings: the host plugin is not loaded; %s has no row in Options.", toggle->m_key.c_str());
	}
}

void __cdecl Toggle::OnApplied(bool value, void* self) noexcept {
	auto* toggle = static_cast<Toggle*>(self);
	toggle->m_value.store(value, std::memory_order_release);
	if (toggle->m_boundValue != nullptr) std::atomic_ref<bool>(*toggle->m_boundValue).store(value, std::memory_order_release);
	toggle->Save(value);
	if (toggle->m_onChanged != nullptr) toggle->m_onChanged(value);
}

auto __cdecl Toggle::OnRefresh(void* self) noexcept -> bool {
	auto* toggle = static_cast<Toggle*>(self);
	// 1. If a chat command or hotkey changed the bound variable directly in memory, persist and use it.
	if (toggle->m_boundValue != nullptr) {
		const bool bound = std::atomic_ref<bool>(*toggle->m_boundValue).load(std::memory_order_acquire);
		if (bound != toggle->m_value.load(std::memory_order_acquire)) {
			toggle->m_value.store(bound, std::memory_order_release);
			toggle->Save(bound);
			return bound;
		}
	}
	// 2. Check if the plugin's TOML file was edited externally on disk.
	std::string text;
	if (ReadConfigText(toggle->m_context, text)) {
		const bool diskValue = ReadBool(text, toggle->m_key, toggle->m_default, toggle->m_tomlSection);
		if (diskValue != toggle->m_lastTomlValue.load(std::memory_order_acquire)) {
			toggle->m_lastTomlValue.store(diskValue, std::memory_order_release);
			toggle->m_value.store(diskValue, std::memory_order_release);
			if (toggle->m_boundValue != nullptr) std::atomic_ref<bool>(*toggle->m_boundValue).store(diskValue, std::memory_order_release);
			if (toggle->m_onChanged != nullptr) toggle->m_onChanged(diskValue);
			return diskValue;
		}
	}
	return toggle->Value();
}

void Toggle::SetValue(bool value, bool saveToConfig, bool notifyChanged) noexcept {
	m_value.store(value, std::memory_order_release);
	if (m_boundValue != nullptr) std::atomic_ref<bool>(*m_boundValue).store(value, std::memory_order_release);
	if (saveToConfig && m_context != nullptr) Save(value);
	const RowHandle row = m_row.load(std::memory_order_acquire);
	if (row != 0 && m_service && m_service.Size() >= ToggleSyncServiceSize && m_service->setToggle != nullptr) {
		(void)m_service->setToggle(row, value);
	}
	if (notifyChanged && m_onChanged != nullptr) m_onChanged(value);
}

auto Toggle::ReloadFromConfig(bool notifyChanged) noexcept -> bool {
	std::string text;
	if (!ReadConfigText(m_context, text)) return false;
	const bool value    = ReadBool(text, m_key, m_default, m_tomlSection);
	const bool previous = Value();
	m_lastTomlValue.store(value, std::memory_order_release);
	m_value.store(value, std::memory_order_release);
	if (m_boundValue != nullptr) std::atomic_ref<bool>(*m_boundValue).store(value, std::memory_order_release);
	const RowHandle row = m_row.load(std::memory_order_acquire);
	if (row != 0 && m_service && m_service.Size() >= ToggleSyncServiceSize && m_service->setToggle != nullptr) {
		(void)m_service->setToggle(row, value);
	}
	if (notifyChanged && value != previous && m_onChanged != nullptr) {
		m_onChanged(value);
	}
	return true;
}

void Toggle::Save(bool value) noexcept {
	try {
		std::string text;
		if (!ReadConfigText(m_context, text)) {
			D2RL::LogWarnF(m_context, "settings: config could not be read; %s applies to this session only.", m_key.c_str());
			return;
		}
		if (!m_context->WriteConfig(WithBool(text, m_key, value, m_tomlSection).c_str())) {
			D2RL::LogWarnF(m_context, "settings: %s could not be saved; it applies to this session only.", m_key.c_str());
			return;
		}
		m_lastTomlValue.store(value, std::memory_order_release);
	} catch (...) {
		D2RL::LogWarnF(m_context, "settings: %s could not be saved; it applies to this session only.", m_key.c_str());
	}
}

void Toggle::Shutdown() noexcept {
	if (m_readySubscription != D2RL::PluginCommunication::InvalidSubscriptionHandle && m_communication != nullptr
	    && m_communication->unsubscribeEvent != nullptr) {
		(void)m_communication->unsubscribeEvent(m_context, m_readySubscription);
		m_readySubscription = D2RL::PluginCommunication::InvalidSubscriptionHandle;
	}
	const RowHandle row = m_row.exchange(0, std::memory_order_acq_rel);
	if (row != 0 && m_service) m_service->removeRow(row);
	m_service.Reset();
}

// ---------------------------------------------------------------------------
// Slider
// ---------------------------------------------------------------------------

auto Slider::Clamp(int32_t value) const noexcept -> int32_t {
	if (value <= m_min) return m_min;
	if (value >= m_max) return m_max;
	if (m_step > 1) {
		const int64_t offset  = static_cast<int64_t>(value) - static_cast<int64_t>(m_min);
		const int64_t snapped = ((offset + (m_step / 2)) / m_step) * m_step + m_min;
		return static_cast<int32_t>(std::clamp<int64_t>(snapped, m_min, m_max));
	}
	return value;
}

auto Slider::Register(const D2RL::PluginContext* context, const SliderOptions& options) noexcept -> bool {
	if (context == nullptr || options.key == nullptr || options.label == nullptr || options.minValue > options.maxValue) return false;
	m_context = context;
	try {
		std::string_view parsedSection;
		std::string_view parsedKey;
		Detail::SplitSectionAndKey(options.key, options.tomlSection != nullptr ? options.tomlSection : "", parsedSection, parsedKey);
		if (parsedKey.empty()) return false;
		m_tomlSection = std::string(parsedSection);
		m_key         = std::string(parsedKey);
		m_hostKey     = BuildHostKey(m_tomlSection, m_key);
		m_label       = options.label;
		m_tooltip     = options.tooltip != nullptr ? options.tooltip : "";
	} catch (...) {
		return false;
	}
	m_section    = options.section;
	m_min        = options.minValue;
	m_max        = options.maxValue;
	m_step       = options.step > 0 ? options.step : 1;
	m_default    = Clamp(options.defaultValue);
	m_boundValue = options.boundValue;
	m_onChanged  = options.onChanged;

	int32_t     value = m_default;
	std::string text;
	if (ReadConfigText(context, text)) {
		value = Clamp(ReadInt(text, m_key, m_default, m_tomlSection));
	} else {
		D2RL::LogWarnF(context, "settings: config could not be read; %s starts at its default.", m_key.c_str());
	}
	m_value.store(value, std::memory_order_release);
	m_lastTomlValue.store(value, std::memory_order_release);
	if (m_boundValue != nullptr) std::atomic_ref<int32_t>(*m_boundValue).store(value, std::memory_order_release);

	if (context->QueryService(&m_communication) != D2RL::ServiceQueryResult::Success
	    || !D2RL::HasPluginCommunicationServiceField(m_communication, D2RL::PluginCommunicationServiceRequiredSize)) {
		m_communication = nullptr;
		D2RL::LogWarnF(context, "settings: no plugin communication; %s has no row in Options.", m_key.c_str());
		return true;
	}
	if (Attach()) return true;

	if (m_communication->subscribeEvent != nullptr) {
		const D2RL::PluginCommunication::EventSubscription sub {
			.structSize        = D2RL::PluginCommunication::EventSubscriptionSize,
			.flags             = 0,
			.publisherPluginId = ProviderPluginId,
			.name              = ReadyEventName,
			.minimumVersion    = ReadyEventVersion,
			.minimumDataSize   = 0,
			.callback          = OnHostReady,
			.userData          = this,
		};
		(void)m_communication->subscribeEvent(context, &sub, &m_readySubscription);
	}
	const D2RL::ThreadService* threads = nullptr;
	if (context->QueryService(&threads) == D2RL::ServiceQueryResult::Success && threads != nullptr
	    && threads->runOnUiThread(context, RetryOnUiThread, this) == D2RL::Threads::Result::Success) {
		return true;
	}
	if (m_readySubscription == D2RL::PluginCommunication::InvalidSubscriptionHandle) {
		D2RL::LogWarnF(context, "settings: the host plugin is not loaded; %s has no row in Options.", m_key.c_str());
	}
	return true;
}

auto Slider::Attach() noexcept -> bool {
	if (m_row.load(std::memory_order_acquire) != 0) return true;
	if (D2RL::PluginCommunication::Acquire(m_context, m_communication, ProviderPluginId, ServiceName, ServiceVersion, SliderRequiredServiceSize, &m_service)
	    != D2RL::PluginCommunication::Result::Success) {
		return false;
	}
	if (m_service->addSlider == nullptr) {
		D2RL::LogWarnF(m_context, "settings: host does not support sliders; %s has no row in Options.", m_key.c_str());
		m_service.Reset();
		return true;
	}
	const SliderSpec spec {
		.structSize   = static_cast<uint32_t>(sizeof(SliderSpec)),
		.section      = m_section,
		.pluginId     = m_context->pluginId,
		.key          = m_hostKey.c_str(),
		.label        = m_label.c_str(),
		.tooltip      = m_tooltip.empty() ? nullptr : m_tooltip.c_str(),
		.value        = Value(),
		.defaultValue = m_default,
		.minValue     = m_min,
		.maxValue     = m_max,
		.step         = m_step,
		.onApplied    = OnApplied,
		.userData     = this,
		.onRefresh    = OnRefresh,
	};
	RowHandle  row    = 0;
	const auto result = m_service->addSlider(&spec, &row);
	if (result != Result::Success) {
		D2RL::LogWarnF(m_context, "settings: the host refused %s (result %u); it has no row in Options.", m_hostKey.c_str(), static_cast<unsigned>(result));
		m_service.Reset();
		return true;
	}
	m_row.store(row, std::memory_order_release);
	return true;
}

void __cdecl Slider::OnHostReady(const D2RL::PluginContext*, const D2RL::PluginCommunication::EventView*, void* self) noexcept {
	auto* slider = static_cast<Slider*>(self);
	(void)slider->Attach();
}

void __cdecl Slider::RetryOnUiThread(const D2RL::PluginContext* context, void* self) noexcept {
	auto* slider = static_cast<Slider*>(self);
	if (!slider->Attach()) {
		D2RL::LogWarnF(context, "settings: the host plugin is not loaded; %s has no row in Options.", slider->m_key.c_str());
	}
}

void __cdecl Slider::OnApplied(int32_t value, void* self) noexcept {
	auto*         slider  = static_cast<Slider*>(self);
	const int32_t clamped = slider->Clamp(value);
	slider->m_value.store(clamped, std::memory_order_release);
	if (slider->m_boundValue != nullptr) std::atomic_ref<int32_t>(*slider->m_boundValue).store(clamped, std::memory_order_release);
	slider->Save(clamped);
	if (slider->m_onChanged != nullptr) slider->m_onChanged(clamped);
}

auto __cdecl Slider::OnRefresh(void* self) noexcept -> int32_t {
	auto* slider = static_cast<Slider*>(self);
	// 1. If a chat command or hotkey changed the bound variable directly in memory, persist and use it.
	if (slider->m_boundValue != nullptr) {
		const int32_t rawBound = std::atomic_ref<int32_t>(*slider->m_boundValue).load(std::memory_order_acquire);
		const int32_t bound    = slider->Clamp(rawBound);
		if (bound != rawBound) {
			std::atomic_ref<int32_t>(*slider->m_boundValue).store(bound, std::memory_order_release);
		}
		if (bound != slider->m_value.load(std::memory_order_acquire)) {
			slider->m_value.store(bound, std::memory_order_release);
			slider->Save(bound);
			return bound;
		}
	}
	// 2. Check if the plugin's TOML file was edited externally on disk.
	std::string text;
	if (ReadConfigText(slider->m_context, text)) {
		const int32_t diskValue = slider->Clamp(ReadInt(text, slider->m_key, slider->m_default, slider->m_tomlSection));
		if (diskValue != slider->m_lastTomlValue.load(std::memory_order_acquire)) {
			slider->m_lastTomlValue.store(diskValue, std::memory_order_release);
			slider->m_value.store(diskValue, std::memory_order_release);
			if (slider->m_boundValue != nullptr) std::atomic_ref<int32_t>(*slider->m_boundValue).store(diskValue, std::memory_order_release);
			if (slider->m_onChanged != nullptr) slider->m_onChanged(diskValue);
			return diskValue;
		}
	}
	return slider->Value();
}

void Slider::SetValue(int32_t value, bool saveToConfig, bool notifyChanged) noexcept {
	const int32_t clamped = Clamp(value);
	m_value.store(clamped, std::memory_order_release);
	if (m_boundValue != nullptr) std::atomic_ref<int32_t>(*m_boundValue).store(clamped, std::memory_order_release);
	if (saveToConfig && m_context != nullptr) Save(clamped);
	const RowHandle row = m_row.load(std::memory_order_acquire);
	if (row != 0 && m_service && m_service.Size() >= SliderRequiredServiceSize && m_service->setSlider != nullptr) {
		(void)m_service->setSlider(row, clamped);
	}
	if (notifyChanged && m_onChanged != nullptr) m_onChanged(clamped);
}

auto Slider::ReloadFromConfig(bool notifyChanged) noexcept -> bool {
	std::string text;
	if (!ReadConfigText(m_context, text)) return false;
	const int32_t value    = Clamp(ReadInt(text, m_key, m_default, m_tomlSection));
	const int32_t previous = Value();
	m_lastTomlValue.store(value, std::memory_order_release);
	m_value.store(value, std::memory_order_release);
	if (m_boundValue != nullptr) std::atomic_ref<int32_t>(*m_boundValue).store(value, std::memory_order_release);
	const RowHandle row = m_row.load(std::memory_order_acquire);
	if (row != 0 && m_service && m_service.Size() >= SliderRequiredServiceSize && m_service->setSlider != nullptr) {
		(void)m_service->setSlider(row, value);
	}
	if (notifyChanged && value != previous && m_onChanged != nullptr) {
		m_onChanged(value);
	}
	return true;
}

void Slider::Save(int32_t value) noexcept {
	try {
		std::string text;
		if (!ReadConfigText(m_context, text)) {
			D2RL::LogWarnF(m_context, "settings: config could not be read; %s applies to this session only.", m_key.c_str());
			return;
		}
		if (!m_context->WriteConfig(WithInt(text, m_key, value, m_tomlSection).c_str())) {
			D2RL::LogWarnF(m_context, "settings: %s could not be saved; it applies to this session only.", m_key.c_str());
			return;
		}
		m_lastTomlValue.store(value, std::memory_order_release);
	} catch (...) {
		D2RL::LogWarnF(m_context, "settings: %s could not be saved; it applies to this session only.", m_key.c_str());
	}
}

void Slider::Shutdown() noexcept {
	if (m_readySubscription != D2RL::PluginCommunication::InvalidSubscriptionHandle && m_communication != nullptr
	    && m_communication->unsubscribeEvent != nullptr) {
		(void)m_communication->unsubscribeEvent(m_context, m_readySubscription);
		m_readySubscription = D2RL::PluginCommunication::InvalidSubscriptionHandle;
	}
	const RowHandle row = m_row.exchange(0, std::memory_order_acq_rel);
	if (row != 0 && m_service) m_service->removeRow(row);
	m_service.Reset();
}

// ---------------------------------------------------------------------------
// Dropdown
// ---------------------------------------------------------------------------

auto Dropdown::Clamp(int32_t index) const noexcept -> int32_t {
	const int32_t count = m_optionCount.load(std::memory_order_acquire);
	if (count <= 0) return 0;
	return std::clamp(index, 0, count - 1);
}

auto Dropdown::MatchToken(std::string_view token, int32_t& outIndex) const noexcept -> bool {
	const std::string_view trimmed = Detail::TrimWhitespace(token);
	if (trimmed.empty()) return false;
	std::lock_guard lock(m_mutex);
	for (size_t i = 0; i < m_configValues.size(); ++i) {
		if (!m_configValues[i].empty() && EqualsIgnoreCase(trimmed, m_configValues[i])) {
			outIndex = static_cast<int32_t>(i);
			return true;
		}
	}
	for (size_t i = 0; i < m_options.size(); ++i) {
		if (EqualsIgnoreCase(trimmed, m_options[i])) {
			outIndex = static_cast<int32_t>(i);
			return true;
		}
	}
	int32_t parsedInt = 0;
	if (Detail::ParseIntValue(trimmed, 0, parsedInt) && parsedInt >= 0 && static_cast<size_t>(parsedInt) < m_options.size()) {
		outIndex = parsedInt;
		return true;
	}
	return false;
}

auto Dropdown::ReadFromToml(std::string_view toml) const noexcept -> int32_t {
	try {
		const std::string raw = ReadString(toml, m_key, "", m_tomlSection);
		if (raw.empty()) return m_default;
		bool useStringToml = false;
		{
			std::lock_guard lock(m_mutex);
			useStringToml = m_useStringToml;
		}
		if (!useStringToml) {
			int32_t parsedInt = m_default;
			if (Detail::ParseIntValue(Detail::TrimWhitespace(raw), 0, parsedInt)) {
				return Clamp(parsedInt);
			}
		}
		int32_t matched = m_default;
		if (MatchToken(raw, matched)) {
			return Clamp(matched);
		}
	} catch (...) {
	}
	return m_default;
}

auto Dropdown::Register(const D2RL::PluginContext* context, const DropdownOptions& options) noexcept -> bool {
	if (context == nullptr || options.key == nullptr || options.label == nullptr) return false;
	const bool hasChoices = (options.choices != nullptr && options.choiceCount > 0);
	const bool hasOptions = (options.options != nullptr && options.optionCount > 0);
	if (!hasChoices && !hasOptions) return false;
	const uint32_t count = hasChoices ? options.choiceCount : options.optionCount;
	if (count == 0 || count > MaxDropdownOptions) return false;

	m_context = context;
	try {
		std::string_view parsedSection;
		std::string_view parsedKey;
		Detail::SplitSectionAndKey(options.key, options.tomlSection != nullptr ? options.tomlSection : "", parsedSection, parsedKey);
		if (parsedKey.empty()) return false;
		m_tomlSection = std::string(parsedSection);
		m_key         = std::string(parsedKey);
		m_hostKey     = BuildHostKey(m_tomlSection, m_key);
		m_label       = options.label;
		m_tooltip     = options.tooltip != nullptr ? options.tooltip : "";

		std::lock_guard lock(m_mutex);
		m_options.clear();
		m_configValues.clear();
		m_options.reserve(count);
		m_configValues.reserve(count);
		m_useStringToml = false;

		for (uint32_t i = 0; i < count; ++i) {
			const char* labelText = hasChoices ? options.choices[i].label : options.options[i];
			if (labelText == nullptr || labelText[0] == '\0') return false;
			m_options.emplace_back(labelText);

			const char* cfgText = nullptr;
			if (hasChoices) {
				cfgText = options.choices[i].configValue;
			} else if (options.configValues != nullptr) {
				cfgText = options.configValues[i];
			}
			if (cfgText != nullptr && cfgText[0] != '\0') {
				m_configValues.emplace_back(cfgText);
				m_useStringToml = true;
			} else {
				m_configValues.emplace_back();
			}
		}
		if (m_useStringToml) {
			for (size_t i = 0; i < m_configValues.size(); ++i) {
				if (m_configValues[i].empty()) m_configValues[i] = m_options[i];
			}
		}
		m_optionCount.store(static_cast<int32_t>(m_options.size()), std::memory_order_release);
	} catch (...) {
		return false;
	}

	m_section      = options.section;
	m_default      = Clamp(options.defaultValue);
	m_boundValue   = options.boundValue;
	m_onChanged    = options.onChanged;
	m_onRefresh    = options.onRefresh;
	m_saveToConfig = options.saveToConfig;

	int32_t value = m_default;
	if (m_saveToConfig) {
		std::string text;
		if (ReadConfigText(context, text)) {
			value = ReadFromToml(text);
		} else {
			D2RL::LogWarnF(context, "settings: config could not be read; %s starts at its default.", m_key.c_str());
		}
	}
	m_value.store(value, std::memory_order_release);
	m_lastTomlValue.store(value, std::memory_order_release);
	if (m_boundValue != nullptr) std::atomic_ref<int32_t>(*m_boundValue).store(value, std::memory_order_release);

	if (context->QueryService(&m_communication) != D2RL::ServiceQueryResult::Success
	    || !D2RL::HasPluginCommunicationServiceField(m_communication, D2RL::PluginCommunicationServiceRequiredSize)) {
		m_communication = nullptr;
		D2RL::LogWarnF(context, "settings: no plugin communication; %s has no row in Options.", m_key.c_str());
		return true;
	}
	if (Attach()) return true;

	if (m_communication->subscribeEvent != nullptr) {
		const D2RL::PluginCommunication::EventSubscription sub {
			.structSize        = D2RL::PluginCommunication::EventSubscriptionSize,
			.flags             = 0,
			.publisherPluginId = ProviderPluginId,
			.name              = ReadyEventName,
			.minimumVersion    = ReadyEventVersion,
			.minimumDataSize   = 0,
			.callback          = OnHostReady,
			.userData          = this,
		};
		(void)m_communication->subscribeEvent(context, &sub, &m_readySubscription);
	}
	const D2RL::ThreadService* threads = nullptr;
	if (context->QueryService(&threads) == D2RL::ServiceQueryResult::Success && threads != nullptr
	    && threads->runOnUiThread(context, RetryOnUiThread, this) == D2RL::Threads::Result::Success) {
		return true;
	}
	if (m_readySubscription == D2RL::PluginCommunication::InvalidSubscriptionHandle) {
		D2RL::LogWarnF(context, "settings: the host plugin is not loaded; %s has no row in Options.", m_key.c_str());
	}
	return true;
}

auto Dropdown::Attach() noexcept -> bool {
	if (m_row.load(std::memory_order_acquire) != 0) return true;
	if (D2RL::PluginCommunication::Acquire(m_context, m_communication, ProviderPluginId, ServiceName, ServiceVersion, DropdownRequiredServiceSize, &m_service)
	    != D2RL::PluginCommunication::Result::Success) {
		return false;
	}
	if (m_service->addDropdown == nullptr) {
		D2RL::LogWarnF(m_context, "settings: host does not support dropdowns; %s has no row in Options.", m_key.c_str());
		m_service.Reset();
		return true;
	}
	try {
		std::vector<std::string> optionsCopy;
		{
			std::lock_guard lock(m_mutex);
			optionsCopy = m_options;
		}
		std::vector<const char*> optionPtrs;
		optionPtrs.reserve(optionsCopy.size());
		for (const std::string& opt : optionsCopy) optionPtrs.push_back(opt.c_str());

		const DropdownSpec spec {
			.structSize    = static_cast<uint32_t>(sizeof(DropdownSpec)),
			.section       = m_section,
			.pluginId      = m_context->pluginId,
			.key           = m_hostKey.c_str(),
			.label         = m_label.c_str(),
			.tooltip       = m_tooltip.empty() ? nullptr : m_tooltip.c_str(),
			.options       = optionPtrs.data(),
			.optionCount   = static_cast<uint32_t>(optionPtrs.size()),
			.selectedIndex = Value(),
			.defaultIndex  = m_default,
			.onApplied     = OnApplied,
			.userData      = this,
			.onRefresh     = OnRefresh,
		};
		RowHandle  row    = 0;
		const auto result = m_service->addDropdown(&spec, &row);
		if (result != Result::Success) {
			D2RL::LogWarnF(m_context, "settings: the host refused %s (result %u); it has no row in Options.", m_hostKey.c_str(), static_cast<unsigned>(result));
			m_service.Reset();
			return true;
		}
		m_row.store(row, std::memory_order_release);
		return true;
	} catch (...) {
		return false;
	}
}

void __cdecl Dropdown::OnHostReady(const D2RL::PluginContext*, const D2RL::PluginCommunication::EventView*, void* self) noexcept {
	auto* dropdown = static_cast<Dropdown*>(self);
	(void)dropdown->Attach();
}

void __cdecl Dropdown::RetryOnUiThread(const D2RL::PluginContext* context, void* self) noexcept {
	auto* dropdown = static_cast<Dropdown*>(self);
	if (!dropdown->Attach()) {
		D2RL::LogWarnF(context, "settings: the host plugin is not loaded; %s has no row in Options.", dropdown->m_key.c_str());
	}
}

void __cdecl Dropdown::OnApplied(int32_t selectedIndex, void* self) noexcept {
	auto*         dropdown = static_cast<Dropdown*>(self);
	const int32_t clamped  = dropdown->Clamp(selectedIndex);
	dropdown->m_value.store(clamped, std::memory_order_release);
	if (dropdown->m_boundValue != nullptr) std::atomic_ref<int32_t>(*dropdown->m_boundValue).store(clamped, std::memory_order_release);
	if (dropdown->m_saveToConfig) dropdown->Save(clamped);
	if (dropdown->m_onChanged != nullptr) dropdown->m_onChanged(clamped);
}

auto __cdecl Dropdown::OnRefresh(void* self) noexcept -> int32_t {
	auto* dropdown = static_cast<Dropdown*>(self);
	if (dropdown->m_onRefresh != nullptr) {
		dropdown->m_onRefresh();
	}
	// 1. If a chat command or hotkey changed the bound variable directly in memory, persist and use it.
	if (dropdown->m_boundValue != nullptr) {
		const int32_t rawBound = std::atomic_ref<int32_t>(*dropdown->m_boundValue).load(std::memory_order_acquire);
		const int32_t bound    = dropdown->Clamp(rawBound);
		if (bound != rawBound) {
			std::atomic_ref<int32_t>(*dropdown->m_boundValue).store(bound, std::memory_order_release);
		}
		if (bound != dropdown->m_value.load(std::memory_order_acquire)) {
			dropdown->m_value.store(bound, std::memory_order_release);
			if (dropdown->m_saveToConfig) dropdown->Save(bound);
			return bound;
		}
	}
	// 2. Check if the plugin's TOML file was edited externally on disk.
	if (dropdown->m_saveToConfig) {
		std::string text;
		if (ReadConfigText(dropdown->m_context, text)) {
			const int32_t diskValue = dropdown->ReadFromToml(text);
			if (diskValue != dropdown->m_lastTomlValue.load(std::memory_order_acquire)) {
				dropdown->m_lastTomlValue.store(diskValue, std::memory_order_release);
				dropdown->m_value.store(diskValue, std::memory_order_release);
				if (dropdown->m_boundValue != nullptr) std::atomic_ref<int32_t>(*dropdown->m_boundValue).store(diskValue, std::memory_order_release);
				if (dropdown->m_onChanged != nullptr) dropdown->m_onChanged(diskValue);
				return diskValue;
			}
		}
	}
	return dropdown->Value();
}

void Dropdown::SetValue(int32_t index, bool saveToConfig, bool notifyChanged) noexcept {
	const int32_t clamped = Clamp(index);
	m_value.store(clamped, std::memory_order_release);
	if (m_boundValue != nullptr) std::atomic_ref<int32_t>(*m_boundValue).store(clamped, std::memory_order_release);
	if (saveToConfig && m_saveToConfig && m_context != nullptr) Save(clamped);
	const RowHandle row = m_row.load(std::memory_order_acquire);
	if (row != 0 && m_service && m_service.Size() >= DropdownRequiredServiceSize && m_service->setDropdown != nullptr) {
		(void)m_service->setDropdown(row, clamped);
	}
	if (notifyChanged && m_onChanged != nullptr) m_onChanged(clamped);
}

auto Dropdown::SetConfigValue(std::string_view token, bool saveToConfig, bool notifyChanged) noexcept -> bool {
	int32_t matched = 0;
	if (!MatchToken(token, matched)) return false;
	SetValue(matched, saveToConfig, notifyChanged);
	return true;
}

auto Dropdown::SetChoices(const DropdownChoice* choices, uint32_t choiceCount, int32_t selectedIndex, bool saveToConfig) noexcept -> bool {
	if (choices == nullptr || choiceCount == 0 || choiceCount > MaxDropdownOptions) return false;
	try {
		std::vector<const char*> labels;
		std::vector<const char*> cfgVals;
		labels.reserve(choiceCount);
		cfgVals.reserve(choiceCount);
		for (uint32_t i = 0; i < choiceCount; ++i) {
			if (choices[i].label == nullptr || choices[i].label[0] == '\0') return false;
			labels.push_back(choices[i].label);
			cfgVals.push_back(choices[i].configValue);
		}
		return SetOptions(labels.data(), cfgVals.data(), choiceCount, selectedIndex, saveToConfig);
	} catch (...) {
		return false;
	}
}

auto Dropdown::SetOptions(const char* const* options, const char* const* configValues, uint32_t optionCount, int32_t selectedIndex, bool saveToConfig) noexcept -> bool {
	if (options == nullptr || optionCount == 0 || optionCount > MaxDropdownOptions) return false;
	try {
		std::vector<std::string> nextOptions;
		std::vector<std::string> nextConfigValues;
		nextOptions.reserve(optionCount);
		nextConfigValues.reserve(optionCount);
		bool nextUseStringToml = false;
		for (uint32_t i = 0; i < optionCount; ++i) {
			if (options[i] == nullptr || options[i][0] == '\0') return false;
			nextOptions.emplace_back(options[i]);
			const char* cfgText = configValues != nullptr ? configValues[i] : nullptr;
			if (cfgText != nullptr && cfgText[0] != '\0') {
				nextConfigValues.emplace_back(cfgText);
				nextUseStringToml = true;
			} else {
				nextConfigValues.emplace_back();
			}
		}
		if (nextUseStringToml) {
			for (size_t i = 0; i < nextConfigValues.size(); ++i) {
				if (nextConfigValues[i].empty()) nextConfigValues[i] = nextOptions[i];
			}
		}

		std::vector<std::string> optionsCopy = nextOptions;
		{
			std::lock_guard lock(m_mutex);
			m_options       = std::move(nextOptions);
			m_configValues  = std::move(nextConfigValues);
			m_useStringToml = nextUseStringToml;
			m_optionCount.store(static_cast<int32_t>(m_options.size()), std::memory_order_release);
		}

		const int32_t clamped = Clamp(selectedIndex);
		m_value.store(clamped, std::memory_order_release);
		if (m_boundValue != nullptr) std::atomic_ref<int32_t>(*m_boundValue).store(clamped, std::memory_order_release);
		if (m_saveToConfig && m_context != nullptr) {
			if (saveToConfig) {
				Save(clamped);
			} else {
				std::string text;
				if (ReadConfigText(m_context, text)) {
					m_lastTomlValue.store(ReadFromToml(text), std::memory_order_release);
				}
			}
		}

		const RowHandle row = m_row.load(std::memory_order_acquire);
		if (row != 0 && m_service && m_service.Size() >= DropdownOptionsServiceSize
		    && m_service->setDropdownOptions != nullptr) {
			std::vector<const char*> ptrs;
			ptrs.reserve(optionsCopy.size());
			for (const std::string& opt : optionsCopy) ptrs.push_back(opt.c_str());
			(void)m_service->setDropdownOptions(row, ptrs.data(), static_cast<uint32_t>(ptrs.size()), clamped);
		}
		return true;
	} catch (...) {
		return false;
	}
}

auto Dropdown::ReloadFromConfig(bool notifyChanged) noexcept -> bool {
	if (!m_saveToConfig) return false;
	std::string text;
	if (!ReadConfigText(m_context, text)) return false;
	const int32_t value    = ReadFromToml(text);
	const int32_t previous = Value();
	m_lastTomlValue.store(value, std::memory_order_release);
	m_value.store(value, std::memory_order_release);
	if (m_boundValue != nullptr) std::atomic_ref<int32_t>(*m_boundValue).store(value, std::memory_order_release);
	const RowHandle row = m_row.load(std::memory_order_acquire);
	if (row != 0 && m_service && m_service.Size() >= DropdownRequiredServiceSize && m_service->setDropdown != nullptr) {
		(void)m_service->setDropdown(row, value);
	}
	if (notifyChanged && value != previous && m_onChanged != nullptr) {
		m_onChanged(value);
	}
	return true;
}

void Dropdown::Save(int32_t index) noexcept {
	try {
		std::string text;
		if (!ReadConfigText(m_context, text)) {
			D2RL::LogWarnF(m_context, "settings: config could not be read; %s applies to this session only.", m_key.c_str());
			return;
		}
		const int32_t clamped = Clamp(index);
		bool          useStringToml = false;
		bool          isBoolPair    = false;
		std::string   token;
		{
			std::lock_guard lock(m_mutex);
			useStringToml = m_useStringToml && static_cast<size_t>(clamped) < m_configValues.size();
			if (useStringToml) {
				token      = m_configValues[static_cast<size_t>(clamped)];
				isBoolPair = (m_configValues.size() == 2
				              && ((m_configValues[0] == "false" && m_configValues[1] == "true")
				                  || (m_configValues[0] == "true" && m_configValues[1] == "false")));
			}
		}
		std::string updated;
		if (useStringToml) {
			if (isBoolPair) {
				updated = WithBool(text, m_key, token == "true", m_tomlSection);
			} else {
				updated = WithString(text, m_key, token, m_tomlSection);
			}
		} else {
			updated = WithInt(text, m_key, clamped, m_tomlSection);
		}
		if (!m_context->WriteConfig(updated.c_str())) {
			D2RL::LogWarnF(m_context, "settings: %s could not be saved; it applies to this session only.", m_key.c_str());
			return;
		}
		m_lastTomlValue.store(clamped, std::memory_order_release);
	} catch (...) {
		D2RL::LogWarnF(m_context, "settings: %s could not be saved; it applies to this session only.", m_key.c_str());
	}
}

void Dropdown::Shutdown() noexcept {
	if (m_readySubscription != D2RL::PluginCommunication::InvalidSubscriptionHandle && m_communication != nullptr
	    && m_communication->unsubscribeEvent != nullptr) {
		(void)m_communication->unsubscribeEvent(m_context, m_readySubscription);
		m_readySubscription = D2RL::PluginCommunication::InvalidSubscriptionHandle;
	}
	const RowHandle row = m_row.exchange(0, std::memory_order_acq_rel);
	if (row != 0 && m_service) m_service->removeRow(row);
	m_service.Reset();
}

// ---------------------------------------------------------------------------
// Status
// ---------------------------------------------------------------------------

auto Status::Register(const D2RL::PluginContext* context, const StatusOptions& options) noexcept -> bool {
	if (context == nullptr || options.key == nullptr || options.label == nullptr) return false;
	m_context = context;
	try {
		std::lock_guard lock(m_mutex);
		m_key     = options.key;
		m_hostKey = SanitizeHostKey(m_key);
		if (m_hostKey.empty()) return false;
		m_label   = options.label;
		m_tooltip = options.tooltip != nullptr ? options.tooltip : "";
		m_value   = options.initialText != nullptr ? options.initialText : "";
	} catch (...) {
		return false;
	}
	m_section   = options.section;
	m_onRefresh = options.onRefresh;

	if (context->QueryService(&m_communication) != D2RL::ServiceQueryResult::Success
	    || !D2RL::HasPluginCommunicationServiceField(m_communication, D2RL::PluginCommunicationServiceRequiredSize)) {
		m_communication = nullptr;
		D2RL::LogWarnF(context, "settings: no plugin communication; %s has no row in Options.", m_key.c_str());
		return true;
	}
	if (Attach()) return true;

	if (m_communication->subscribeEvent != nullptr) {
		const D2RL::PluginCommunication::EventSubscription sub {
			.structSize        = D2RL::PluginCommunication::EventSubscriptionSize,
			.flags             = 0,
			.publisherPluginId = ProviderPluginId,
			.name              = ReadyEventName,
			.minimumVersion    = ReadyEventVersion,
			.minimumDataSize   = 0,
			.callback          = OnHostReady,
			.userData          = this,
		};
		(void)m_communication->subscribeEvent(context, &sub, &m_readySubscription);
	}
	const D2RL::ThreadService* threads = nullptr;
	if (context->QueryService(&threads) == D2RL::ServiceQueryResult::Success && threads != nullptr
	    && threads->runOnUiThread(context, RetryOnUiThread, this) == D2RL::Threads::Result::Success) {
		return true;
	}
	if (m_readySubscription == D2RL::PluginCommunication::InvalidSubscriptionHandle) {
		D2RL::LogWarnF(context, "settings: the host plugin is not loaded; %s has no row in Options.", m_key.c_str());
	}
	return true;
}

auto Status::Attach() noexcept -> bool {
	if (m_row.load(std::memory_order_acquire) != 0) return true;
	if (D2RL::PluginCommunication::Acquire(m_context, m_communication, ProviderPluginId, ServiceName, ServiceVersion, StatusRequiredServiceSize, &m_service)
	    != D2RL::PluginCommunication::Result::Success) {
		return false;
	}
	if (m_service->addStatus == nullptr) {
		D2RL::LogWarnF(m_context, "settings: host does not support status rows; %s has no row in Options.", m_key.c_str());
		m_service.Reset();
		return true;
	}
	try {
		std::string currentText;
		{
			std::lock_guard lock(m_mutex);
			currentText = m_value;
		}
		const StatusSpec spec {
			.structSize = static_cast<uint32_t>(sizeof(StatusSpec)),
			.section    = m_section,
			.pluginId   = m_context->pluginId,
			.key        = m_hostKey.c_str(),
			.label      = m_label.c_str(),
			.tooltip    = m_tooltip.empty() ? nullptr : m_tooltip.c_str(),
			.value      = currentText.c_str(),
			.onRefresh  = OnRefresh,
			.userData   = this,
		};
		RowHandle  row    = 0;
		const auto result = m_service->addStatus(&spec, &row);
		if (result != Result::Success) {
			D2RL::LogWarnF(m_context, "settings: the host refused %s (result %u); it has no row in Options.", m_hostKey.c_str(), static_cast<unsigned>(result));
			m_service.Reset();
			return true;
		}
		m_row.store(row, std::memory_order_release);
		return true;
	} catch (...) {
		return false;
	}
}

void __cdecl Status::OnHostReady(const D2RL::PluginContext*, const D2RL::PluginCommunication::EventView*, void* self) noexcept {
	auto* status = static_cast<Status*>(self);
	(void)status->Attach();
}

void __cdecl Status::RetryOnUiThread(const D2RL::PluginContext* context, void* self) noexcept {
	auto* status = static_cast<Status*>(self);
	if (!status->Attach()) {
		D2RL::LogWarnF(context, "settings: the host plugin is not loaded; %s has no row in Options.", status->m_key.c_str());
	}
}

auto __cdecl Status::OnRefresh(void* self) noexcept -> const char* {
	auto* status = static_cast<Status*>(self);
	if (status->m_onRefresh != nullptr) {
		status->m_onRefresh();
	}
	try {
		std::lock_guard lock(status->m_mutex);
		status->m_refreshScratch = status->m_value;
		return status->m_refreshScratch.c_str();
	} catch (...) {
		return "";
	}
}

void Status::SetText(std::string_view text) noexcept {
	std::string copy;
	try {
		std::lock_guard lock(m_mutex);
		m_value.assign(text);
		copy = m_value;
	} catch (...) {
		return;
	}
	const RowHandle row = m_row.load(std::memory_order_acquire);
	if (row != 0 && m_service && m_service.Size() >= StatusRequiredServiceSize && m_service->setStatus != nullptr) {
		(void)m_service->setStatus(row, copy.c_str());
	}
}

void Status::Shutdown() noexcept {
	if (m_readySubscription != D2RL::PluginCommunication::InvalidSubscriptionHandle && m_communication != nullptr
	    && m_communication->unsubscribeEvent != nullptr) {
		(void)m_communication->unsubscribeEvent(m_context, m_readySubscription);
		m_readySubscription = D2RL::PluginCommunication::InvalidSubscriptionHandle;
	}
	const RowHandle row = m_row.exchange(0, std::memory_order_acq_rel);
	if (row != 0 && m_service) m_service->removeRow(row);
	m_service.Reset();
}

} // namespace D2RLSettings
