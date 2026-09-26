# Compatibility and limitations

- Requires a current D2RLoader release supporting PluginSDK ABI v1, public
  Item, Shared Event, Lifecycle, and plugin-config services.
- Uses no native hooks, D2R/D2RCore RVAs, injected patches, or external runtime
  dependencies.
- Supports vanilla/LoD map ids 2 through 132 by default, excluding the five
  towns. Custom positive map ids can be added in the TOML configuration.
- Only normal `key` item tooltips receive guidance.
- Guidance is static and does not reveal the current map seed or unexplored
  rooms. Unknown or unconfigured maps add no tooltip text.
- Loader admission and visible Tower Cellar Level 1 and Jail Level 1 tips were
  user-confirmed on 2026-09-26. Configuration-driven loading was subsequently
  user-confirmed. The remaining zone guidance has not been individually
  live-validated.
