# Controller QOL development

The [player README](README.md) and [changelog](CHANGELOG.md) describe the shipping
plugin. Source in `src/` and `include/`, plus `tests/`, is authoritative.

| Task | Start here |
| --- | --- |
| Find a feature contract | [Topic map](docs/README.md) |
| Build and test | [Contributor workflow](../../CONTRIBUTING.md), CMakeLists.txt |
| Native hooks and ownership | [Hook inventory](docs/HOOK-INVENTORY.md), current source guards |
| Shared/per-mod configuration | [Configuration contract](docs/SKILL-DISCOVERY-CONFIG.md), [live reload](docs/CONFIG-RELOAD.md) |
| Companion coexistence | [UnHoarder opacity contract](docs/UNHOARDER-OPACITY-COMPATIBILITY.md) |
| Ground-loot cost and exact companion selection | [Performance contract](docs/LOOT-PERFORMANCE.md) |
| Validation scope | [Validation guide](docs/VALIDATION.md) |

Preserve exact-build guards, native semantics, ownership and teardown. Input
routes, displayed pages and native storage pages are distinct contracts. Other
plugins reuse QOL source at build time, so check consumers when changing shared code.
Read dated records only for the specific feature under investigation.
[Historical notes](docs/history/README.md) preserve earlier evidence and versions.
