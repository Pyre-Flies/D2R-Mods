#pragma once
// A setting in the game's own Options > Gameplay tab, in a few lines:
//
//     D2RLSettings::Toggle g_showBars;
//     D2RLSettings::Slider g_maxRange;
//
//     // D2RLoaderLoadPlugin
//     g_showBars.Register(context, {
//         .key         = "enabled",
//         .label       = "Show Enemy Healthbar",
//         .tooltip     = "Show a health bar above enemies that have taken damage.",
//         .section     = D2RLSettings::Section::Accessibility,
//         .tomlSection = "display",
//     });
//
//     // Any thread
//     if (g_showBars.Value()) { ... }
//
//     // Chat command / hotkey sync (saves to TOML and updates Options > Gameplay)
//     g_showBars.SetValue(false);
//
//     // D2RLoaderUnloadPlugin
//     g_showBars.Shutdown();
//
// The value is stored under .key (and optional .tomlSection) in the plugin's own config file.
#include <D2RLSettings/service.h>

#include <D2RLPlugin/api.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace D2RLSettings {

struct ToggleOptions {
	const char* key          = nullptr; // Config key (or "section.key" when tomlSection is null).
	const char* label        = nullptr;
	const char* tooltip      = nullptr;
	Section     section      = Section::Gameplay;
	bool        defaultValue = true;
	// Optional. Runs on the UI thread after Apply (or external TOML reload) changed the value.
	void        (*onChanged)(bool value) noexcept = nullptr;
	// Optional TOML [section] header (e.g. "display" or "qol").
	const char* tomlSection  = nullptr;
	// Optional pointer to a plugin-owned bool kept in sync with Value(), chat commands, and TOML.
	bool*       boundValue   = nullptr;
};

struct SliderOptions {
	const char* key          = nullptr; // Config key (or "section.key" when tomlSection is null).
	const char* label        = nullptr;
	const char* tooltip      = nullptr;
	Section     section      = Section::Gameplay;
	int32_t     defaultValue = 0;
	int32_t     minValue     = 0;
	int32_t     maxValue     = 100;
	int32_t     step         = 1;
	// Optional. Runs on the UI thread after Apply (or external TOML reload) changed the value.
	void        (*onChanged)(int32_t value) noexcept = nullptr;
	// Optional TOML [section] header (e.g. "display" or "qol").
	const char* tomlSection  = nullptr;
	// Optional pointer to a plugin-owned int32_t kept in sync with Value(), chat commands, and TOML.
	int32_t*    boundValue   = nullptr;
};

struct DropdownChoice {
	const char* label       = nullptr; // Shown in the Options > Gameplay dropdown.
	const char* configValue = nullptr; // Optional TOML string value (e.g. "hold"); when null, uses integer index or label.
};

struct DropdownOptions {
	const char*           key          = nullptr; // Config key (or "section.key" when tomlSection is null).
	const char*           label        = nullptr;
	const char*           tooltip      = nullptr;
	Section               section      = Section::Gameplay;
	// Provide either .options + .optionCount (array of label strings) OR .choices + .choiceCount.
	const char* const*    options      = nullptr;
	uint32_t              optionCount  = 0;
	// Optional parallel array of TOML string values (length == optionCount) when storing strings in TOML.
	const char* const*    configValues = nullptr;
	// Or provide .choices + .choiceCount directly:
	const DropdownChoice* choices      = nullptr;
	uint32_t              choiceCount  = 0;
	int32_t               defaultValue = 0; // Default selected index [0 .. count - 1].
	// Optional. Runs on the UI thread after Apply (or external TOML reload) changed the selected index.
	void                  (*onChanged)(int32_t index) noexcept = nullptr;
	// Optional. Runs on the UI thread when Options > Gameplay opens, before options/selection are shown.
	void                  (*onRefresh)() noexcept = nullptr;
	// Optional TOML [section] header (e.g. "display" or "qol").
	const char*           tomlSection  = nullptr;
	// Optional pointer to a plugin-owned int32_t kept in sync with Value(), chat commands, and TOML.
	int32_t*              boundValue   = nullptr;
	// Set false when the plugin manages multi-key persistence itself in onChanged/onRefresh.
	bool                  saveToConfig = true;
};

struct StatusOptions {
	const char* key         = nullptr; // Unique row key within the plugin.
	const char* label       = nullptr; // Left column label.
	const char* tooltip     = nullptr;
	Section     section     = Section::Gameplay;
	const char* initialText = nullptr; // Initial right-column status text.
	// Optional. Runs on the UI thread when Options > Gameplay opens or after Apply.
	void        (*onRefresh)() noexcept = nullptr;
};

class Toggle {
public:
	Toggle() = default;
	Toggle(const Toggle&) = delete;
	auto operator=(const Toggle&) -> Toggle& = delete;

	// Call from D2RLoaderLoadPlugin. Reads the saved value and adds the row. Returns
	// false only for bad options; without the host plugin the setting still works
	// from its saved value, with no row to change it.
	auto Register(const D2RL::PluginContext* context, const ToggleOptions& options) noexcept -> bool;

	void Shutdown() noexcept;

	[[nodiscard]] auto Value() const noexcept -> bool {
		if (m_boundValue != nullptr) {
			return std::atomic_ref<bool>(*m_boundValue).load(std::memory_order_acquire);
		}
		return m_value.load(std::memory_order_acquire);
	}

	// Updates the setting from a chat command or hotkey, optionally saving to TOML and
	// syncing with Options > Gameplay.
	void SetValue(bool value, bool saveToConfig = true, bool notifyChanged = false) noexcept;

	// Re-reads the plugin's TOML config and syncs any external change into memory and Options.
	auto ReloadFromConfig(bool notifyChanged = true) noexcept -> bool;

	// Whether the row is in Options.
	[[nodiscard]] auto HasRow() const noexcept -> bool {
		return m_row.load(std::memory_order_acquire) != 0;
	}

private:
	static void __cdecl OnApplied(bool value, void* self) noexcept;
	static auto __cdecl OnRefresh(void* self) noexcept -> bool;
	static void __cdecl OnHostReady(const D2RL::PluginContext* context, const D2RL::PluginCommunication::EventView* event, void* self) noexcept;
	static void __cdecl RetryOnUiThread(const D2RL::PluginContext* context, void* self) noexcept;
	auto Attach() noexcept -> bool;
	void Save(bool value) noexcept;

	const D2RL::PluginContext*                         m_context           = nullptr;
	const D2RL::PluginCommunicationService*            m_communication     = nullptr;
	D2RL::PluginCommunication::ServiceLease<ServiceV1> m_service;
	D2RL::PluginCommunication::SubscriptionHandle      m_readySubscription = D2RL::PluginCommunication::InvalidSubscriptionHandle;
	std::atomic<RowHandle>                             m_row { 0 };
	std::atomic<bool>                                  m_value { false };
	std::atomic<bool>                                  m_lastTomlValue { false };
	std::string                                        m_key;
	std::string                                        m_tomlSection;
	std::string                                        m_hostKey;
	std::string                                        m_label;
	std::string                                        m_tooltip;
	Section                                            m_section    = Section::Gameplay;
	bool                                               m_default    = true;
	bool*                                              m_boundValue = nullptr;
	void                                               (*m_onChanged)(bool) noexcept = nullptr;
};

class Slider {
public:
	Slider() = default;
	Slider(const Slider&) = delete;
	auto operator=(const Slider&) -> Slider& = delete;

	// Call from D2RLoaderLoadPlugin. Reads the saved value and adds the slider row.
	auto Register(const D2RL::PluginContext* context, const SliderOptions& options) noexcept -> bool;

	void Shutdown() noexcept;

	[[nodiscard]] auto Value() const noexcept -> int32_t {
		if (m_boundValue != nullptr) {
			return Clamp(std::atomic_ref<int32_t>(*m_boundValue).load(std::memory_order_acquire));
		}
		return m_value.load(std::memory_order_acquire);
	}

	// Updates the setting from a chat command or hotkey, optionally saving to TOML and
	// syncing with Options > Gameplay.
	void SetValue(int32_t value, bool saveToConfig = true, bool notifyChanged = false) noexcept;

	// Re-reads the plugin's TOML config and syncs any external change into memory and Options.
	auto ReloadFromConfig(bool notifyChanged = true) noexcept -> bool;

	// Whether the row is in Options.
	[[nodiscard]] auto HasRow() const noexcept -> bool {
		return m_row.load(std::memory_order_acquire) != 0;
	}

private:
	static void __cdecl OnApplied(int32_t value, void* self) noexcept;
	static auto __cdecl OnRefresh(void* self) noexcept -> int32_t;
	static void __cdecl OnHostReady(const D2RL::PluginContext* context, const D2RL::PluginCommunication::EventView* event, void* self) noexcept;
	static void __cdecl RetryOnUiThread(const D2RL::PluginContext* context, void* self) noexcept;
	[[nodiscard]] auto Clamp(int32_t value) const noexcept -> int32_t;
	auto Attach() noexcept -> bool;
	void Save(int32_t value) noexcept;

	const D2RL::PluginContext*                         m_context           = nullptr;
	const D2RL::PluginCommunicationService*            m_communication     = nullptr;
	D2RL::PluginCommunication::ServiceLease<ServiceV1> m_service;
	D2RL::PluginCommunication::SubscriptionHandle      m_readySubscription = D2RL::PluginCommunication::InvalidSubscriptionHandle;
	std::atomic<RowHandle>                             m_row { 0 };
	std::atomic<int32_t>                               m_value { 0 };
	std::atomic<int32_t>                               m_lastTomlValue { 0 };
	std::string                                        m_key;
	std::string                                        m_tomlSection;
	std::string                                        m_hostKey;
	std::string                                        m_label;
	std::string                                        m_tooltip;
	Section                                            m_section    = Section::Gameplay;
	int32_t                                            m_default    = 0;
	int32_t                                            m_min        = 0;
	int32_t                                            m_max        = 100;
	int32_t                                            m_step       = 1;
	int32_t*                                           m_boundValue = nullptr;
	void                                               (*m_onChanged)(int32_t) noexcept = nullptr;
};

class Dropdown {
public:
	Dropdown() = default;
	Dropdown(const Dropdown&) = delete;
	auto operator=(const Dropdown&) -> Dropdown& = delete;

	// Call from D2RLoaderLoadPlugin. Reads the saved choice and adds the dropdown row.
	auto Register(const D2RL::PluginContext* context, const DropdownOptions& options) noexcept -> bool;

	void Shutdown() noexcept;

	[[nodiscard]] auto Value() const noexcept -> int32_t {
		if (m_boundValue != nullptr) {
			return Clamp(std::atomic_ref<int32_t>(*m_boundValue).load(std::memory_order_acquire));
		}
		return m_value.load(std::memory_order_acquire);
	}

	[[nodiscard]] auto OptionCount() const noexcept -> int32_t {
		return m_optionCount.load(std::memory_order_acquire);
	}

	[[nodiscard]] auto Label() const -> std::string {
		const int32_t idx = Value();
		std::lock_guard lock(m_mutex);
		if (idx >= 0 && static_cast<size_t>(idx) < m_options.size()) return m_options[static_cast<size_t>(idx)];
		return {};
	}

	[[nodiscard]] auto ConfigValue() const -> std::string {
		const int32_t idx = Value();
		std::lock_guard lock(m_mutex);
		if (idx >= 0 && static_cast<size_t>(idx) < m_configValues.size()) return m_configValues[static_cast<size_t>(idx)];
		return {};
	}

	// Updates the selected index from a chat command or hotkey, optionally saving to TOML and
	// syncing with Options > Gameplay.
	void SetValue(int32_t index, bool saveToConfig = true, bool notifyChanged = false) noexcept;

	// Matches a config token or option label (case-insensitive) and updates the dropdown.
	auto SetConfigValue(std::string_view token, bool saveToConfig = true, bool notifyChanged = false) noexcept -> bool;

	// Dynamically replaces the dropdown choices and selected index (e.g. when scanning files on disk).
	auto SetChoices(const DropdownChoice* choices, uint32_t choiceCount, int32_t selectedIndex, bool saveToConfig = false) noexcept -> bool;
	auto SetOptions(const char* const* options, const char* const* configValues, uint32_t optionCount, int32_t selectedIndex, bool saveToConfig = false) noexcept -> bool;

	// Re-reads the plugin's TOML config and syncs any external change into memory and Options.
	auto ReloadFromConfig(bool notifyChanged = true) noexcept -> bool;

	// Whether the row is in Options.
	[[nodiscard]] auto HasRow() const noexcept -> bool {
		return m_row.load(std::memory_order_acquire) != 0;
	}

private:
	static void __cdecl OnApplied(int32_t selectedIndex, void* self) noexcept;
	static auto __cdecl OnRefresh(void* self) noexcept -> int32_t;
	static void __cdecl OnHostReady(const D2RL::PluginContext* context, const D2RL::PluginCommunication::EventView* event, void* self) noexcept;
	static void __cdecl RetryOnUiThread(const D2RL::PluginContext* context, void* self) noexcept;
	[[nodiscard]] auto Clamp(int32_t index) const noexcept -> int32_t;
	[[nodiscard]] auto MatchToken(std::string_view token, int32_t& outIndex) const noexcept -> bool;
	[[nodiscard]] auto ReadFromToml(std::string_view toml) const noexcept -> int32_t;
	auto Attach() noexcept -> bool;
	void Save(int32_t index) noexcept;

	const D2RL::PluginContext*                         m_context           = nullptr;
	const D2RL::PluginCommunicationService*            m_communication     = nullptr;
	D2RL::PluginCommunication::ServiceLease<ServiceV1> m_service;
	D2RL::PluginCommunication::SubscriptionHandle      m_readySubscription = D2RL::PluginCommunication::InvalidSubscriptionHandle;
	std::atomic<RowHandle>                             m_row { 0 };
	std::atomic<int32_t>                               m_value { 0 };
	std::atomic<int32_t>                               m_lastTomlValue { 0 };
	std::atomic<int32_t>                               m_optionCount { 0 };
	mutable std::mutex                                 m_mutex;
	std::string                                        m_key;
	std::string                                        m_tomlSection;
	std::string                                        m_hostKey;
	std::string                                        m_label;
	std::string                                        m_tooltip;
	std::vector<std::string>                           m_options;
	std::vector<std::string>                           m_configValues;
	bool                                               m_useStringToml = false;
	bool                                               m_saveToConfig  = true;
	Section                                            m_section       = Section::Gameplay;
	int32_t                                            m_default       = 0;
	int32_t*                                           m_boundValue    = nullptr;
	void                                               (*m_onChanged)(int32_t) noexcept = nullptr;
	void                                               (*m_onRefresh)() noexcept        = nullptr;
};

class Status {
public:
	Status() = default;
	Status(const Status&) = delete;
	auto operator=(const Status&) -> Status& = delete;

	// Call from D2RLoaderLoadPlugin. Adds a read-only status/feedback row to Options > Gameplay.
	auto Register(const D2RL::PluginContext* context, const StatusOptions& options) noexcept -> bool;

	void Shutdown() noexcept;

	[[nodiscard]] auto Value() const -> std::string {
		std::lock_guard lock(m_mutex);
		return m_value;
	}

	// Updates the status text and syncs it with Options > Gameplay (including live open panels).
	void SetText(std::string_view text) noexcept;

	// Whether the row is in Options.
	[[nodiscard]] auto HasRow() const noexcept -> bool {
		return m_row.load(std::memory_order_acquire) != 0;
	}

private:
	static auto __cdecl OnRefresh(void* self) noexcept -> const char*;
	static void __cdecl OnHostReady(const D2RL::PluginContext* context, const D2RL::PluginCommunication::EventView* event, void* self) noexcept;
	static void __cdecl RetryOnUiThread(const D2RL::PluginContext* context, void* self) noexcept;
	auto Attach() noexcept -> bool;

	const D2RL::PluginContext*                         m_context           = nullptr;
	const D2RL::PluginCommunicationService*            m_communication     = nullptr;
	D2RL::PluginCommunication::ServiceLease<ServiceV1> m_service;
	D2RL::PluginCommunication::SubscriptionHandle      m_readySubscription = D2RL::PluginCommunication::InvalidSubscriptionHandle;
	std::atomic<RowHandle>                             m_row { 0 };
	mutable std::mutex                                 m_mutex;
	std::string                                        m_key;
	std::string                                        m_hostKey;
	std::string                                        m_label;
	std::string                                        m_tooltip;
	std::string                                        m_value;
	std::string                                        m_refreshScratch;
	Section                                            m_section   = Section::Gameplay;
	void                                               (*m_onRefresh)() noexcept = nullptr;
};

} // namespace D2RLSettings
