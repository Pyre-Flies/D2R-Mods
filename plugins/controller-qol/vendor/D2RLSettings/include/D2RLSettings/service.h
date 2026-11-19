#pragma once
// The contract between libs/settings and the aa-settings-host plugin. Everything
// crosses a DLL boundary, so it is plain data and C-style calls. Add new fields to
// the end of a struct; use a new service name for a breaking change.
#include <D2RLPlugin/api.h>

#include <cstdint>

namespace D2RLSettings {

// The options sections of the Gameplay tab a row can join.
enum class Section : uint32_t {
	Gameplay      = 0,
	Ui            = 1,
	Accessibility = 2,
	Automap       = 3,
	LootFilter    = 4,
	Controller    = 5,
};

inline constexpr uint32_t SectionCount = 6;

inline constexpr char     ProviderPluginId[]  = "aa-settings-host";
inline constexpr char     ServiceName[]       = "settings";
inline constexpr uint32_t ServiceVersion      = 1;
inline constexpr char     ReadyEventName[]    = "ready";
inline constexpr uint32_t ReadyEventVersion   = 1;

enum class Result : uint32_t {
	Success         = 0,
	InvalidArgument = 1,
	Conflict        = 2, // That plugin already has a setting with this key.
	Unavailable     = 3, // The row could not be added to the panel.
};

using RowHandle = uint64_t;

// Called on the UI thread when the player presses Apply and the value changed.
using ToggleAppliedFn   = void(__cdecl*)(bool value, void* userData) noexcept;
using SliderAppliedFn   = void(__cdecl*)(int32_t value, void* userData) noexcept;
using DropdownAppliedFn = void(__cdecl*)(int32_t selectedIndex, void* userData) noexcept;

// Optional: called on the UI thread when Options > Gameplay opens so the setting owner
// can pull fresh TOML/chat state and return the latest applied value before display.
using RefreshToggleFn   = bool(__cdecl*)(void* userData) noexcept;
using RefreshSliderFn   = int32_t(__cdecl*)(void* userData) noexcept;
using RefreshDropdownFn = int32_t(__cdecl*)(void* userData) noexcept;
using RefreshStatusFn   = const char*(__cdecl*)(void* userData) noexcept;

inline constexpr uint32_t MaxDropdownOptions = 250;

// The host copies every string before addToggle returns.
struct ToggleSpec {
	uint32_t        structSize;
	Section         section;
	const char*     pluginId;
	const char*     key;     // Unique within the plugin; [a-z0-9_-].
	const char*     label;   // Shown text, or a namespaced string reference: "@my-plugin:Key".
	const char*     tooltip; // May be null.
	bool            value;
	bool            defaultValue; // What the Defaults button restores.
	ToggleAppliedFn onApplied;
	void*           userData;
	RefreshToggleFn onRefresh = nullptr;
};

// The host copies every string before addSlider returns.
struct SliderSpec {
	uint32_t        structSize;
	Section         section;
	const char*     pluginId;
	const char*     key;     // Unique within the plugin; [a-z0-9_-].
	const char*     label;   // Shown text, or a namespaced string reference: "@my-plugin:Key".
	const char*     tooltip; // May be null.
	int32_t         value;
	int32_t         defaultValue; // What the Defaults button restores.
	int32_t         minValue;
	int32_t         maxValue;
	int32_t         step; // Increment per arrow click / step snap (>= 1).
	SliderAppliedFn onApplied;
	void*           userData;
	RefreshSliderFn onRefresh = nullptr;
};

// The host copies every string before addDropdown returns.
struct DropdownSpec {
	uint32_t           structSize;
	Section            section;
	const char*        pluginId;
	const char*        key;     // Unique within the plugin; [a-z0-9_-].
	const char*        label;   // Shown text, or a namespaced string reference: "@my-plugin:Key".
	const char*        tooltip; // May be null.
	const char* const* options; // Non-null array of optionCount non-null UTF-8 strings.
	uint32_t           optionCount;
	int32_t            selectedIndex;
	int32_t            defaultIndex; // What the Defaults button restores.
	DropdownAppliedFn  onApplied;
	void*              userData;
	RefreshDropdownFn  onRefresh = nullptr;
};

// The host copies every string before addStatus returns.
struct StatusSpec {
	uint32_t        structSize;
	Section         section;
	const char*     pluginId;
	const char*     key;     // Unique within the plugin; [a-z0-9_-].
	const char*     label;   // Shown text, or a namespaced string reference: "@my-plugin:Key".
	const char*     tooltip; // May be null.
	const char*     value;   // Read-only status text displayed on the right side of the row.
	RefreshStatusFn onRefresh = nullptr;
	void*           userData  = nullptr;
};

struct ServiceV1 {
	uint32_t serviceSize;
	uint32_t serviceVersion;
	Result(__cdecl* addToggle)(const ToggleSpec* spec, RowHandle* handle) noexcept;
	// The row stays until the panel reloads; it just stops calling back.
	void(__cdecl* removeRow)(RowHandle handle) noexcept;
	Result(__cdecl* addSlider)(const SliderSpec* spec, RowHandle* handle) noexcept;
	// Updates the host's applied value when changed via chat commands or TOML reload.
	Result(__cdecl* setToggle)(RowHandle handle, bool value) noexcept;
	Result(__cdecl* setSlider)(RowHandle handle, int32_t value) noexcept;
	Result(__cdecl* addDropdown)(const DropdownSpec* spec, RowHandle* handle) noexcept;
	Result(__cdecl* setDropdown)(RowHandle handle, int32_t selectedIndex) noexcept;
	Result(__cdecl* setDropdownOptions)(RowHandle handle, const char* const* options, uint32_t optionCount, int32_t selectedIndex) noexcept;
	Result(__cdecl* addStatus)(const StatusSpec* spec, RowHandle* handle) noexcept;
	Result(__cdecl* setStatus)(RowHandle handle, const char* value) noexcept;
};

inline constexpr uint32_t ServiceV1Size = static_cast<uint32_t>(sizeof(ServiceV1));

} // namespace D2RLSettings
