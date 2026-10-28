# Chat-command setting persistence

Setting commands save through the active plugin context's ReadConfig/WriteConfig
API. Config paths follow the loader's active global/mod scope; none are guessed.

| Plugin | Commands | Saved section/keys |
| --- | --- | --- |
| Floating Damage | on/off/toggle; motion on/off/toggle | `[floating-damage] enabled`, `falling_motion` |
| Item Roll Ranges | on/off/toggle, including bare command | `[display] always_show_ranges` |
| Always Show Sockets | default/always/on/off/toggle, including bare command | `[always-show-sockets] enabled`, `always_visible` |
| Pyre Loot Filter | on/off/toggle; minimap/sound/alt/autoreload [on\|off] | `[pyre-lootfilter] enabled`, `auto_reload`, `alt_pickup_override`, `minimap_icons`, `drop_sounds` |
| Day Night Cycle | on/off/toggle/day/night/auto | `[day-night-cycle] enabled`, `mode` |

Socket size/opacity/ring/center/lift retain their existing per-setting autosave.
Status/help/invalid or unavailable commands do not write configuration. Capture
is a bounded diagnostic recording action: it stops at 128 records/game exit and
does not become a startup preference. Controller QOL keyboard aim shortcuts
are separate from chat settings and keep their existing behavior.

The shared bounded boolean editor preserves comments, BOM/line endings and
unrelated settings. Missing keys/sections are inserted. Ambiguous duplicate
keys/sections, unsupported target headers, quoted/malformed target values,
multiline strings and unreadable/oversized
documents refuse saving. It does not replace unread user configuration with
defaults. SDK write failures are reported; no success message claims persistence
after failure. Damage/ranges keep their previous state on save failure. Socket
settings keep the established temporary-apply behavior and show a warning.

Automated coverage exercises actual temporary TOML files with fresh startup
parsers, production command callbacks, mode reload, comment/section retention,
idempotent explicit commands, toggles, read-only status, malformed/unavailable
commands, read/write/size failures and ambiguous input refusal. Three affected
plugins pass 21 suites (8 damage, 9 ranges, 4 sockets). Actual loader-backed
save/restart in game is not yet confirmed. No native address, hook, ABI, byte
guard or table layout changes were required.
