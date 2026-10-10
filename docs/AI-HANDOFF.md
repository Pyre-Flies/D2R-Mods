# Research handoff for maintainers and AI agents

This is an entry point to the tracked evidence, not a substitute for the feature
records or executable guards. Read the linked document and current source before
changing a hook, layout, controller action, or release. The stable release baseline is Controller QOL rev.75, Item Roll Ranges rev.23,
and Map Assistance rev.1. The detailed native summaries below retain historical
evidence; follow the current feature records before treating them as active code.
QOL shared controls remain in the main config; all per-skill settings and R3 saves
now belong in complete per-mod profiles. See the
[configuration/migration guide](../plugins/controller-qol/docs/SKILL-DISCOVERY-CONFIG.md)
and [rev.75 production record](../plugins/controller-qol/docs/PRODUCTION-1.3.1-rev.75.md).
Release ZIPs use reference-only `defaults/` TOMLs; never overwrite user configs.

## Where to start

For general working procedure, read the versioned
[D2R skills](../skills/README.md): the engineering skill covers guarded native
plugin work and the gameplay skill covers variant-aware player behavior. Their
reference files are useful to AI systems that do not load Codex skills.

| Work | Canonical record | What it establishes |
|---|---|---|
| D2R/D2RCore locations and shared contracts | [Reverse-engineering registry](reverse-engineering/README.md), [RVA workflow](reverse-engineering/RVA-AND-INTERCEPTION-WORKFLOW.md) | Build identity, RVA conventions, byte witnesses, and discovery method |
| Controller QOL feature research | [Research map](../plugins/controller-qol/docs/README.md), [hook inventory](../plugins/controller-qol/docs/HOOK-INVENTORY.md), [validation](../plugins/controller-qol/docs/VALIDATION.md) | Feature ownership, active versus superseded hooks, and validation levels |
| Controller QOL reusable native contracts | [Native-contract registry](reverse-engineering/controller-qol-native-contracts.md) | Cross-plugin index; follow links to feature evidence and source guards |
| Item Roll Ranges | [Native contract](../plugins/item-roll-ranges/docs/NATIVE-CONTRACT.md), [reverse-engineering evidence](../plugins/item-roll-ranges/docs/reverse-engineering/README.md), [validation](../plugins/item-roll-ranges/docs/VALIDATION.md) | Formatter, provider, property and range provenance |
| Map Assistance | [Design and provenance](../plugins/map-assistance/docs/DESIGN.md), [compatibility](../plugins/map-assistance/COMPATIBILITY.md) | Public SDK tooltip path and static map-data limits |
| Release process | [Publishing releases](RELEASING.md), plugin changelogs | Build, package, checksum and publication workflow |

## Rules for carrying a finding forward

1. Identify the exact running module and hash. D2R.exe, a loader-hosted main
   image, D2RCore.dll, and provider modules are distinct identities. A product
   version or a matching prefix alone does not qualify a native contract.
2. Use module-relative RVAs with the named module and verify the current byte
   guard, layout, ownership, thread and lifetime preconditions in source. A
   registry row is a search pointer, not permission to install a hook.
3. Preserve native behavior on a mismatch. Disable the affected feature and
   log the reason rather than using an unqualified fallback address or layout.
4. Keep evidence levels separate: static analysis, tests, loader admission,
   runtime logs, authoritative game state, visible UI, persistence, and user
   confirmation establish different things. A passing test does not prove a
   live interaction.
5. Document each new path, RVA, signature, byte guard, offset, ABI finding, or
   rejected hypothesis in the owning feature record during the same change.
   Include build identity, provenance, limits and validation state. Update the
   cross-project registry when another plugin could reuse the discovery.

## Current feature boundaries

- Controller QOL's native controller, ground-loot, stash, belt, vendor, and
  Guided Arrow paths have separate feature records. Use the
  [research map](../plugins/controller-qol/docs/README.md) to reach the exact
  contract. In particular, bare-A pickup and LB shortcuts are separate input
  paths; the Shared-stash UI category, displayed page and native storage page
  are separate identities.
- Item Roll Ranges appends source-aware range information to the game's
  tooltip. The [affix](../plugins/item-roll-ranges/docs/AFFIX-TIERS.md),
  [signed range](../plugins/item-roll-ranges/docs/SIGNED-RANGES-REV12.md), and
  [elemental decomposition](../plugins/item-roll-ranges/docs/ELEMENTAL-DECOMPOSITION-REV13.md)
  records describe when a label or combined interval is supported. Ambiguous
  attribution must remain ambiguous.
- Map Assistance contributes static Keys tooltip guidance through public SDK
  services. It does not inspect the random map seed. Its
  [design record](../plugins/map-assistance/docs/DESIGN.md) distinguishes the
  small set of user-confirmed visible tips from broader data and automated
  coverage.

## Handoff checklist

- Inspect `git status` and the current branch before editing. Do not fold local
  experiments, deployed binaries, captured game images, logs, build trees, or
  backups into a documentation change.
- Read the owning feature document, its current source guard and changelog.
  Verify any volatile hash, RVA or active deployment against the target build.
- State separately what changed, what automated checks passed, what was seen
  live, and what remains unverified. Record a rollback point before replacing
  an active DLL.
