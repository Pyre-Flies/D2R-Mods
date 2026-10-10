# D2R Mods release 2026.10.10.2

## Item Roll Ranges 1.3.1+rev.23

Restore fixed Base headers on items with no rollable modifiers, including a
plain Sash: `Defense: 2 (Base: 2 - 2)` while holding Ctrl, returning to
`Defense: 2` on release. Core can create no range clone for these items even
when the gesture is held; the guarded header observer now handles that case.

Native input queries/events and Controller QOL remain unchanged. The observer
preserves native arguments, caller stack and return. Existing provider/code
compatibility guards and focused item-management panel checks remain in place.
The revision22 Iceblink variable-Defense publication fix is retained.

The user confirmed both the sash Ctrl hold/release display and Iceblink fix
in game. All five Item Roll Ranges suites pass, including native entry ABI,
hold/release decisions and header proof regressions. Controller-specific live
checks of the no-clone fallback remain unconfirmed. Tests do not establish
universal item, localization or plugin compatibility.

Controller QOL 1.3.1+rev.73 and Map Assistance 1.3.1+rev.1 are included unchanged.
Their automated suites also pass.

## Installation

Close the game and loader and back up existing files. Extract the wanted ZIP
into the game directory: DLLs install under `d2rloader/plugins/`, configs under
`d2rloader/config/`. For mod-scoped installs, extract into the active mod folder.
Keep one active copy per plugin and preserve customized TOML files. Enable
`d2rcore.items.item_stat_ranges = true`. Qualified build required; cold restart.
