# Controller QOL topic map

Start with the [developer guide](../DEVELOPMENT.md), current source guards and
[player README](../README.md). Versions are maintained in the player README and changelog.

| Topic | Contract or evidence |
| --- | --- |
| Configuration, profiles and R3 saves | [Skill configuration](SKILL-DISCOVERY-CONFIG.md), [general skills](GENERAL-SKILLS-OSKILLS.md) |
| Automatic and console settings reload | [Reload contract](CONFIG-RELOAD.md) |
| Hook ownership and reusable native paths | [Hook inventory](HOOK-INVENTORY.md), [shared registry](../../../docs/reverse-engineering/controller-qol-native-contracts.md) |
| Aim and casting | [Cast observer](AIM-CAST-OBSERVER-COMPATIBILITY.md), [skill catalog](SKILL-CATALOG.md) |
| Cube and materials | [Cube routing](CUBE-ROUTING.md), [materials routing](MATERIALS-CUBE-ROUTING.md), [materials contract](MATERIALS-NATIVE-CONTRACT.md) |
| Belt and vendors | [Belt contract](BELT-NATIVE-CONTRACT.md), [native selling](NPC-SELLING.md) |
| Shared storage | [Shared deposits](SHARED-DEPOSIT.md), [page controls](SHARED-PAGE-CONTROLS.md) |
| Ground loot and coexistence | [Portal priority](PORTAL-PRIORITY.md), [plugin coexistence](PLUGIN-COEXISTENCE.md), [UnHoarder opacity](UNHOARDER-OPACITY-COMPATIBILITY.md) |
| Ground-loot performance | [Guard selection, frozen holds and UnHoarder 1.2.10](LOOT-PERFORMANCE.md) |
| Hosted input | [Ladder scheduling](LADDER-CONTROLLER-SCHEDULING.md) |
| Validation | [Validation guide](VALIDATION.md), affected tests |

[History](history/README.md) contains dated fixes, version-specific qualification,
crash investigations and superseded guides. JSON/disassembly witnesses remain
build-specific; source guards determine current admission. Read a linked record
for its exact topic rather than loading the whole history.

[Current-page performance validation](CURRENT-PAGE-VALIDATION.md) records the optional fresh Windows page-query path and its preserved guards.
