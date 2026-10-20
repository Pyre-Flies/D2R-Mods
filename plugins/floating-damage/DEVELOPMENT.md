# Floating Damage development

Read the [player README](README.md), [changelog](CHANGELOG.md) and
[native contract](NATIVE-CONTRACT.md). Implementation and guards are in `src/`;
observation, correlation and configuration tests are in `tests/`.

Build and run CTest using [CONTRIBUTING.md](../../CONTRIBUTING.md).
Preserve owned snapshots, bounded correlation, exact native admission and fallback
colors for ambiguous observations. HP loss does not establish attacker attribution.
Keep static/captured evidence separate from visible combat and persistence claims.

Older behavior and validation notes remain in [history](docs/history/README.md).
