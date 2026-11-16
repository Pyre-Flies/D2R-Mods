#include "qol_options_settings.h"
#include "config_reload.h"
#include <D2RLSettings/settings.h>

namespace QolOptionsSettings {
namespace {

D2RLSettings::Toggle g_prioritizePortalsToggle {};
D2RLSettings::Toggle g_prioritizeStashToggle {};
D2RLSettings::Toggle g_prioritizeChestsToggle {};

D2RLSettings::Toggle   g_aimSnappingToggle {};
D2RLSettings::Toggle   g_aimOverlayToggle {};
D2RLSettings::Slider   g_aimReticleSizeSlider {};
D2RLSettings::Slider   g_aimReticleThicknessSlider {};
D2RLSettings::Dropdown g_aimGroundColorDropdown {};
D2RLSettings::Dropdown g_aimLockColorDropdown {};
D2RLSettings::Slider   g_aimMaxSpeedSlider {};
D2RLSettings::Slider   g_aimSnapRadiusSlider {};
D2RLSettings::Slider   g_aimSmoothingSlider {};

constexpr D2RLSettings::DropdownChoice kGroundColorChoices[] {
	{ "Ice Blue", "#8FE8FF" },
	{ "Gold", "#FFD166" },
	{ "Emerald", "#4DFF88" },
	{ "White", "#FFFFFF" },
	{ "Crimson", "#FF4D4D" },
	{ "Violet", "#C77DFF" },
};

constexpr D2RLSettings::DropdownChoice kLockColorChoices[] {
	{ "Gold", "#FFD166" },
	{ "Crimson", "#FF4D4D" },
	{ "Emerald", "#4DFF88" },
	{ "Ice Blue", "#8FE8FF" },
	{ "White", "#FFFFFF" },
	{ "Orange", "#FF9F1C" },
};

void OnToggleChanged(bool) noexcept {
	QolReload::RequestQuiet();
}

void OnSliderChanged(std::int32_t) noexcept {
	QolReload::RequestQuiet();
}

void OnDropdownChanged(std::int32_t) noexcept {
	QolReload::RequestQuiet();
}

} // namespace

void Register(const D2RL::PluginContext* context) noexcept {
	if (context == nullptr) return;
	// D2RLSettings::Slider::Register, Dropdown::Register, and Toggle::Register are
	// fail-open when aa-settings-host is absent: they read the TOML defaults,
	// subscribe to the host's ready event in case it loads later, and return cleanly.
	(void)g_prioritizePortalsToggle.Register(context, {
		.key          = "prioritize_portals",
		.label        = "Prioritize Portals",
		.tooltip      = "Prioritize Town Portals and red portals over ground loot when pressing A without the loot modifier.",
		.section      = D2RLSettings::Section::Controller,
		.defaultValue = true,
		.onChanged    = &OnToggleChanged,
		.tomlSection  = "qol",
	});

	(void)g_prioritizeStashToggle.Register(context, {
		.key          = "prioritize_stash_boxes",
		.label        = "Prioritize Stash",
		.tooltip      = "Prioritize the Stash over nearby ground loot when pressing A without the loot modifier.",
		.section      = D2RLSettings::Section::Controller,
		.defaultValue = true,
		.onChanged    = &OnToggleChanged,
		.tomlSection  = "qol",
	});

	(void)g_prioritizeChestsToggle.Register(context, {
		.key          = "prioritize_chests",
		.label        = "Prioritize Chests",
		.tooltip      = "Prioritize closed chests over nearby ground loot when pressing A without the loot modifier.",
		.section      = D2RLSettings::Section::Controller,
		.defaultValue = true,
		.onChanged    = &OnToggleChanged,
		.tomlSection  = "qol",
	});

	(void)g_aimSnappingToggle.Register(context, {
		.key          = "snapping_enabled",
		.label        = "Target Snapping",
		.tooltip      = "Snap the controller reticle to nearby enemies within the configured snap radius.",
		.section      = D2RLSettings::Section::Controller,
		.defaultValue = true,
		.onChanged    = &OnToggleChanged,
		.tomlSection  = "aim",
	});

	(void)g_aimOverlayToggle.Register(context, {
		.key          = "overlay_enabled",
		.label        = "Reticle Overlay",
		.tooltip      = "Draw the ground and target-lock reticle overlay when using supported skills with a controller.",
		.section      = D2RLSettings::Section::Controller,
		.defaultValue = true,
		.onChanged    = &OnToggleChanged,
		.tomlSection  = "aim",
	});

	(void)g_aimReticleSizeSlider.Register(context, {
		.key          = "reticle_size_percent",
		.label        = "Reticle Size (%)",
		.tooltip      = "Overall size percentage (50..250%) of the ground reticle ring and enemy target-lock diamond.",
		.section      = D2RLSettings::Section::Controller,
		.defaultValue = 100,
		.minValue     = 50,
		.maxValue     = 250,
		.step         = 10,
		.onChanged    = &OnSliderChanged,
		.tomlSection  = "aim",
	});

	(void)g_aimReticleThicknessSlider.Register(context, {
		.key          = "reticle_thickness",
		.label        = "Reticle Thickness",
		.tooltip      = "Line thickness multiplier (1..4) for the ground reticle ring and enemy target-lock diamond.",
		.section      = D2RLSettings::Section::Controller,
		.defaultValue = 2,
		.minValue     = 1,
		.maxValue     = 4,
		.step         = 1,
		.onChanged    = &OnSliderChanged,
		.tomlSection  = "aim",
	});

	(void)g_aimGroundColorDropdown.Register(context, {
		.key          = "ground_reticle_color",
		.label        = "Ground Reticle Color",
		.tooltip      = "Color of the ground reticle ring when using supported skills with a controller.",
		.section      = D2RLSettings::Section::Controller,
		.choices      = kGroundColorChoices,
		.choiceCount  = static_cast<uint32_t>(std::size(kGroundColorChoices)),
		.defaultValue = 0,
		.onChanged    = &OnDropdownChanged,
		.tomlSection  = "aim",
	});

	(void)g_aimLockColorDropdown.Register(context, {
		.key          = "lock_reticle_color",
		.label        = "Lock Reticle Color",
		.tooltip      = "Color of the target-lock diamond when snapped onto an enemy.",
		.section      = D2RLSettings::Section::Controller,
		.choices      = kLockColorChoices,
		.choiceCount  = static_cast<uint32_t>(std::size(kLockColorChoices)),
		.defaultValue = 0,
		.onChanged    = &OnDropdownChanged,
		.tomlSection  = "aim",
	});

	(void)g_aimMaxSpeedSlider.Register(context, {
		.key          = "maximum_speed",
		.label        = "Reticle Speed",
		.tooltip      = "Controller reticle movement speed in world tiles per second (12..100).",
		.section      = D2RLSettings::Section::Controller,
		.defaultValue = 48,
		.minValue     = 12,
		.maxValue     = 100,
		.step         = 4,
		.onChanged    = &OnSliderChanged,
		.tomlSection  = "aim",
	});

	(void)g_aimSnapRadiusSlider.Register(context, {
		.key          = "snap_radius",
		.label        = "Snap Radius",
		.tooltip      = "Enemy target snapping radius in world tiles (1..15) while the controller reticle is active.",
		.section      = D2RLSettings::Section::Controller,
		.defaultValue = 6,
		.minValue     = 1,
		.maxValue     = 15,
		.step         = 1,
		.onChanged    = &OnSliderChanged,
		.tomlSection  = "aim",
	});

	(void)g_aimSmoothingSlider.Register(context, {
		.key          = "overlay_smoothing_ms",
		.label        = "Reticle Smoothing",
		.tooltip      = "Screen-motion smoothing window in milliseconds (0..150) for the reticle overlay.",
		.section      = D2RLSettings::Section::Controller,
		.defaultValue = 60,
		.minValue     = 0,
		.maxValue     = 150,
		.step         = 10,
		.onChanged    = &OnSliderChanged,
		.tomlSection  = "aim",
	});
}

void Shutdown() noexcept {
	g_aimSmoothingSlider.Shutdown();
	g_aimSnapRadiusSlider.Shutdown();
	g_aimMaxSpeedSlider.Shutdown();
	g_aimLockColorDropdown.Shutdown();
	g_aimGroundColorDropdown.Shutdown();
	g_aimReticleThicknessSlider.Shutdown();
	g_aimReticleSizeSlider.Shutdown();
	g_aimOverlayToggle.Shutdown();
	g_aimSnappingToggle.Shutdown();
	g_prioritizeChestsToggle.Shutdown();
	g_prioritizeStashToggle.Shutdown();
	g_prioritizePortalsToggle.Shutdown();
}

} // namespace QolOptionsSettings
