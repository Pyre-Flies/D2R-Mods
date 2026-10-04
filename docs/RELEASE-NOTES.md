## Controller QOL Updates 1.3.1+rev.60

- Scope right-stick aim to enabled skills; other skills retain native targeting.
- Include all 240 class skill IDs with readable comments. Ten tested skills default
  enabled; passives are locked off and other untested skills default off.
- Press R3 on a highlighted catalog skill to enable/disable aim and save the
  setting. Enabled icons show brass crosshairs; the control strip shows status.
  `"disabled"` locks a skill off and ignores R3. First-press handling is improved.
- Configure ground/lock reticle hex colors and line thickness (default 1.0).
- Custom numeric IDs use true/false/"disabled". Optional `[aim.targeting]` entries
  override built-in or declared custom IDs to ground or snap independently of
  enable state. Existing custom ground/snap entries remain supported.
- Fix reticle persistence and make the cast observer optional for hook coexistence.
  Essential aim hooks still require exact compatibility guards.

## Extract directly into the game directory

Runtime ZIPs now contain `d2rloader/config/<plugin>.toml` and
`d2rloader/plugins/<plugin>.dll`; no separate configuration directory is used.
Close the game and loader before installing. Back up existing files, extract the
wanted ZIPs into the game directory, and keep one active copy of each plugin.
For upgrades, preserve customized TOML files or merge new defaults rather than
replacing your settings. Restart after manual config changes. Disable the former
Controller Aim Test DLL if migrating; its config is no longer read.

## Validation and compatibility

Local automated suites: Controller QOL 23/23, Item Roll Ranges 5/5, Map Assistance
2/2. User reports the current Controller QOL runtime is working well, including
improved R3 handling. This is not exhaustive validation of every catalog/custom
skill, mod, plugin combination, class layout or resolution. New skill opt-ins and
targeting overrides need individual gameplay testing.

Target: Windows x64, D2RLoader 1.3.1 / ABI4, SDK v0.3.0. Exact native guards retain
fail-open behavior. See the rev.60 production record and bundled compatibility
notes. Hot reload is unsupported. Item Roll Ranges requires
`d2rcore.items.item_stat_ranges = true`.

Included unchanged plugin binaries: Item Roll Ranges 1.3.1+rev.13 and Map Assistance
1.3.1+rev.1. Map Assistance receives the same ZIP layout correction.
