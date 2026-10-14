# D2R Mods release 2026.10.14.1

Item Roll Ranges rev.40 adds optional always-show ranges and `/AlwaysShowRolls`
for a session toggle. **Always-show is disabled by default.** Enable it through
chat, or set `display.always_show_ranges = true` in the saved config and restart.
Existing user settings are preserved; packaged TOMLs are references only.

Fix the plain Sash `Defense: 2 (Base: 2-2)` header with Ctrl held and always-show
off. Tighten native formatter admission and prevent an extra render pass after
merge failure. Preserve source breakdowns, Base headers and Max Sockets order.

Eight Item Roll Ranges suites pass. The user confirmed always-show, chat toggles
and the final plain Sash correction. Runtime logs show successful command
registration alongside other plugins, with no Item Roll Ranges backend errors.
Broader controller/localization combinations and arbitrary formatter plugins
are not exhaustively validated; unknown formatter replacements are refused.

| Plugin | Version |
| --- | --- |
| Item Roll Ranges | 1.3.1+rev.40 |
| Controller QOL Updates | 1.3.1+rev.78 |
| Map Assistance | 1.3.1+rev.1 |
| Inventory Deposit | 0.1.0-beta.13 |
| Floating Damage | 0.1.0-beta.12 |

Other plugin source and versions are unchanged from current main.
