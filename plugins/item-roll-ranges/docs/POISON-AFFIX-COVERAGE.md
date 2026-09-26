# Poison affix coverage investigation

2026-09-26. Target: D2R 3.3.93787, installed `D2R.exe` SHA-256
`1E2AC459FEB3F4BBFA818CDFF49800480502BEAE9F90CFA4CBA9E7E1F8BFA3B7`,
qualified D2RCore 1.3.1-beta. The omission was reproduced with the bounded
coverage build and addressed in 1.3.1+rev.3; post-fix live validation remains.

## Finding

The missing `of Blight`, `of Venom`, and `of Pestilence` labels are not absent
from the affix table. They use property `dmg-pois` (Properties row 138), whose
three native handlers produce stats 57 `poisonmindam`, 58 `poisonmaxdam`, and
59 `poisonlength` through property functions 15, 16, and 17.

The qualified game's item-property formatter has a poison-specific branch at
D2R+`0x2DC6DE`. At +`0x2DC6E6` it requests stat 57 before building the combined
poison text and before the ordinary property loop reached at +`0x2DC7D3`.
D2RCore's range formatter independently tests stat 57 at Core+`0x7936CB`.
Consequently the combined poison line can bypass the plugin's current
single-property identity observer even though `Affixes::Contributes` correctly
understands the loaded property row.

These RVAs were already present in the retained formatter/property-entry
disassemblies. They remain build-specific and are not promoted to a runtime
dependency without a new guarded observer and live verification.

## Active rows sharing this path

The active Reimagined tables contain 30 `dmg-pois` prefix/suffix rows:

- suffix names: `of Blight`, `of Venom`, `of Pestilence`, `of Anthrax`;
- prefix names: `Septic`, `Envenomed`, `Corosive`, `Toxic`, `Pestilent`.

The rows cover weapon/circlet, jewelry, small charm, large charm, and missile
families. The spelling `Corosive` is the active table's internal name and is not
corrected by the plugin.

Automagic rows with the same displayed names use `extra-pois` and
`pierce-pois`, not `dmg-pois`; they are separate `[Base]` contributions and
must not be conflated with these rolled prefix/suffix IDs.

## Spawner validation plan

D2RLoader's Item Spawner is useful as a deterministic fixture generator, not
as the runtime mapping source. The plugin already reads the combined loaded
MagicSuffix/MagicPrefix/AutoMagic table and the six stored one-based item affix
IDs. Use the spawner to create one compatible item for each distinct property
family, then compare stored IDs, captured formatter identities, native range
text, and final labels. This tests formatting-path coverage while preserving
the active table as the authority.

First poison witnesses should cover a weapon/circlet suffix, a variable large
charm suffix, and one poison prefix. A fix must preserve native localized poison
wording, combined duration math, actual values, and range output; it must attach
only a verified source label and fail open on any guard or attribution mismatch.

## Rev.3 evidence and implementation

The coverage build captured three real poison tooltips (`+7` over 3 seconds,
`+50` over 5 seconds, and `+12` over 3 seconds). In every case the poison line
was identical in the actual and ranged passes and was the only property line
without a captured native identity. All other displayed lines had verified
identities. This proves that the missing label was not a range-text merge error.

Rev.3 synthesizes stat-57 identity only when the loaded affix definitions
verify a `dmg-pois` contributor and exactly one displayed property line remains
unidentified. Zero or multiple candidates omit the label. The implementation
does not parse English poison wording, reproduce poison arithmetic, or change
native text/ranges. Automated coverage includes the observed sole-unidentified
case and a two-candidate refusal; all five suites pass. Visible in-game labeling
is not claimed until retested.
