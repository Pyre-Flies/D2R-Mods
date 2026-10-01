# QOL rev.51: aim enabled by default

2026-09-30. Changes only the aim default from rev.50. Both embedded `[aim]`
configuration and parser fallback now use enabled=true. This includes older
QOL configs without an aim section. Explicit aim.enabled=false remains off;
global qol.enabled=false still skips aim initialization. Invalid aim settings
remain rejected. Core/code guards and duplicate-prototype protection remain.

No native address, signature, ABI, hook, targeting or render behavior changes.
See [rev.50](PRODUCTION-1.3.1-rev.50.md) for the integration contracts. Its opt-in
wording is historical and superseded by this revision. Disable the old standalone
prototype before enabling integrated aim. Existing custom configs are not rewritten.

Validation: updated policy and artifact checks verify missing-section and embedded
on defaults. Runtime gating checks still prove explicit/global off registers no
services/hooks. The complete three-plugin suite is required for publication.
Merged runtime's live validation boundary remains as documented in rev.50.
