# Map Assistance design and provenance

## Integration contract

The plugin consumes only public D2RLoader PluginSDK ABI v1 services at SDK
revision `6fbabfc84a72b83ed812ce7f45f79a519fdd0877`:

- `LifecycleService::registerGameplayEventListener` caches the current
  `LevelChanged.currentValue` and clears it on `GameLeft`.
- `SharedEventService::registerItemTooltipListener` contributes a Description
  line without modifying D2R's complete tooltip buffer.
- `ItemService::getItemInfo` verifies that the hovered item code is `key `.
- `PluginContext::ReadConfig` loads `d2rloader/config/map-assistance.toml` once
  at plugin startup. The DLL embeds the shipping default through the SDK config
  resource helper; D2RLoader materializes it only when the file is absent.
- `D2RL_PLUGIN_MANIFEST_RESOURCE_ID` embeds the SDK ABI as an `RCDATA`
resource. D2RLoader checks this before calling the plugin exports; omitting it
  causes the DLL to be classified as an old alpha plugin.

The level values are Levels.txt ids, not process addresses or inferred room
pointers. No RVAs, byte guards, native layouts, or proprietary assets are
introduced by this plugin.

The artifact test opens the DLL as an image resource, verifies the manifest is
one `DWORD` equal to `D2RL_PLUGIN_ABI_VERSION`, then checks that the exported
metadata reports the same ABI. This is a shipping requirement, not merely
Windows file-description metadata. It also verifies the embedded config
resource is present, readable, and within the loader's 1 MiB limit.

## Tip provenance and limits

The concept and initial area selection were inspired by Kryszard's PD2 filter
Season 13.3.2 map-reading section. Wording here is intentionally concise and
original. Guidance is static map-reading knowledge: it does not inspect the
current random seed or reveal unexplored map state.

PD2 includes the mod-added `rkey` item. This proof of concept intentionally
supports only vanilla D2R's normal `key` code until another item code is
verified in the active D2R/Reimagined tables.

Version 1.3.1+rev.1 ships defaults for every non-town vanilla/LoD level id from 1
through 132. Entries are keyed by numeric Levels.txt `map_id`, with a separate
human-readable name, layout label, and array of tooltip lines. This permits
future or mod-added ids without recompilation. The dependency-free parser
accepts this documented subset, rejects duplicate or malformed entries, caps
the data used by the tooltip callback, and performs no file I/O during tooltip
rendering.
The five town ids are intentionally silent. PD2-only rewards and mechanics were
not carried into the table; unknown and mod-added level ids also remain silent.

Loader admission and visible Keys tips in Tower Cellar Level 1 and Jail Level 1
were user-confirmed on 2026-09-26. The remaining map-layout claims have not yet
been live-validated in D2R or D2R Reimagined and should be corrected or disabled
if observed behavior differs. Unsupported levels fail open by contributing no
tooltip text.
