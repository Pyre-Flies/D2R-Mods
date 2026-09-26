# Production snapshot 1.3.1.0 — 2026-09-24

Baseline: user-verified 0.4.8. Version scheme: 1.3.1 matches the targeted D2RLoader release; the final component is the independent plugin revision, starting at 0. This is not a promise of compatibility with arbitrary future loader binaries. Existing provider fingerprints remain enforced.

This snapshot changes version metadata and disables Item Roll Ranges formatter dumps. Controller QOL debug_logging remains false by default. Normal initialization/errors remain available. Existing logs are not deleted. No native addresses, hooks, provenance calculations or gameplay behavior were changed.

Both plugins are frozen as the production baseline before further compatibility work. Compatibility with every third-party plugin is not yet verified.
