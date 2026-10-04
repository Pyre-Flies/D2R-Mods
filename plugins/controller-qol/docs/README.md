# Controller QOL research map

This directory is the canonical durable research record for Controller QOL. Start here instead of searching old build directories, release archives, or the former Reimagined working tree.

Reusable native contracts and RVAs distilled from this work are indexed in
[`../../../docs/reverse-engineering/controller-qol-native-contracts.md`](../../../docs/reverse-engineering/controller-qol-native-contracts.md).
Keep detailed feature evidence here and cross-project summaries there.

## Current status and validation

- [PRODUCTION-1.3.1-rev.60.md](PRODUCTION-1.3.1-rev.60.md) - current production release, skill settings, indicators and installable ZIP layout.

- [SKILL-TREE-AIM-TOGGLE.md](SKILL-TREE-AIM-TOGGLE.md) - rev.56 R3 per-skill toggle, guarded focused skill ID and persistence.

- [AIM-CAST-OBSERVER-COMPATIBILITY.md](AIM-CAST-OBSERVER-COMPATIBILITY.md) - rev.54 optional cast observer, guarded fallback and Whirlwind coexistence boundaries.

- [SKILL-CATALOG.md](SKILL-CATALOG.md) - rev.53 full class skill catalog, review defaults and source/deployment validation.

- [IMPLEMENTATION-1.3.1-rev.52.md](IMPLEMENTATION-1.3.1-rev.52.md) - right-stick intent, per-skill isolation and numeric custom skill configuration; live validation pending.

- [PRODUCTION-1.3.1-rev.51.md](PRODUCTION-1.3.1-rev.51.md) - aim defaults enabled; explicit off remains supported.

- [PRODUCTION-1.3.1-rev.50.md](PRODUCTION-1.3.1-rev.50.md) - opt-in integrated aim, configuration migration and validation boundary.

- [VALIDATION.md](VALIDATION.md) - build, artifact, deployment, automated-test and live-test status. Keep automated and live evidence separate.
- [HOOK-INVENTORY.md](HOOK-INVENTORY.md) - native hook ownership and compatibility map.
- [PRODUCTION-1.3.1-rev.7.md](PRODUCTION-1.3.1-rev.7.md) - qualified production scope and remaining limits.
- [VERSIONING.md](VERSIONING.md) - plugin/loader version policy.
- [LEGACY-REIMAGINED-MIGRATION.md](LEGACY-REIMAGINED-MIGRATION.md) - extraction provenance, legacy Git lineage, and distilled evidence from intentionally excluded runtime logs.

## Native controller and UI contracts

- [NATIVE-INPUT-1.3.1-rev.5.md](NATIVE-INPUT-1.3.1-rev.5.md) - normalized controller input, build guards and fallback coverage.
- [SDK-CONTROLLER-HANDOFF.md](SDK-CONTROLLER-HANDOFF.md) - earlier SDK probe conclusions and unresolved boundaries.
- [GLYPH-CALLS-1.3.1.3.md](GLYPH-CALLS-1.3.1.3.md) - scoped controller-glyph call sites.
- [SHARED-PAGE-CONTROLS.md](SHARED-PAGE-CONTROLS.md) - Shared-stash page navigation and input-routing failure evidence.
- [LABEL-STASH-1.5.23.md](LABEL-STASH-1.5.23.md) - label and stash behavior.

## Item and container behavior

- [BELT-NATIVE-CONTRACT.md](BELT-NATIVE-CONTRACT.md) - stored-item belt placement and confirmation.
- [MATERIALS-NATIVE-CONTRACT.md](MATERIALS-NATIVE-CONTRACT.md) - Reimagined Materials-tab withdrawal/refill.
- [MATERIALS-CUBE-ROUTING.md](MATERIALS-CUBE-ROUTING.md) - Materials and embedded-Cube routing.
- [CUBE-ROUTING.md](CUBE-ROUTING.md) - Cube visibility and quick-move context.
- [SHARED-DEPOSIT.md](SHARED-DEPOSIT.md) - shared-page deposit and identity handling.
- [NPC-SELLING.md](NPC-SELLING.md) - native vendor eligibility, pricing and sell transaction.
- [REFILL-SELL-1.5.24.md](REFILL-SELL-1.5.24.md) and [FOCUSED-REFILL-1.5.25.md](FOCUSED-REFILL-1.5.25.md) - contextual refill and sell behavior.
- [AUTOSORT-FEASIBILITY.md](AUTOSORT-FEASIBILITY.md) - relationship to exact-position transaction research.

## Ground loot and portals

- [PORTAL-PRIORITY.md](PORTAL-PRIORITY.md) - portal targeting, contact gates, RVAs and live evidence.
- [PORTAL-CONTACT-1.3.1-rev.7.md](PORTAL-CONTACT-1.3.1-rev.7.md) - scoped portal-contact interception.
- [LOOT-PORTAL-1.3.1.2.md](LOOT-PORTAL-1.3.1.2.md) - loot-modifier and neutral-interact separation.
- [CHRONICLE-GROUND-FLAG-CONFLICT.md](CHRONICLE-GROUND-FLAG-CONFLICT.md) - coexistence diagnosis with Chronicle Ground Flag.
- [PLUGIN-COEXISTENCE.md](PLUGIN-COEXISTENCE.md), [POTION-AUTO-PICKUP-COMPATIBILITY.md](POTION-AUTO-PICKUP-COMPATIBILITY.md), and [STASH-SEARCH-1.10.12-CONFLICT.md](STASH-SEARCH-1.10.12-CONFLICT.md) - third-party compatibility.

## Evidence conventions

JSON evidence and disassembly text beside these documents are build-specific witnesses, not portable APIs. The native signature headers under `src/` are the executable guards. Preserve module identity, hashes, RVA convention, byte windows, confidence and validation level when updating a claim.

Local runtime logs, build trees, full game-memory images and old release packages are intentionally not stored in this repository. Distill durable conclusions into the relevant document and retain a source hash when the raw artifact remains outside version control.
