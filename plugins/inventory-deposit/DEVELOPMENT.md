# Inventory Deposit development

Read the [player README](README.md) and [changelog](CHANGELOG.md).
Source, embedded artwork inputs and tests live in `src/`, `assets/` and `tests/`;
CMakeLists.txt defines the actual build dependencies, including image conversion.

| Contract | Read |
| --- | --- |
| Native transactions and button | [Button investigation](docs/INVENTORY-DEPOSIT-BUTTON.md) |
| Placement and availability | [Button state](docs/BUTTON-STATE.md) |
| Batch completion feedback | [Stash feedback](docs/STASH-FEEDBACK.md) |
| Build and test | [CONTRIBUTING.md](../../CONTRIBUTING.md) |

The plugin reuses QOL batch implementation at build time, without requiring the
QOL DLL at runtime. Preserve eligibility, fresh preflight checks and gold-relative
button placement. Source removal alone does not establish destination counts or save persistence.

[History](docs/history/README.md) retains prior release and validation records.
