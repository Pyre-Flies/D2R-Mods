# Follow-up: class skills and crafted Sunder provenance (2026-09-24)

Code inspection, not a live-item validation:

- `src/affixes.h::Contributes` handles nonzero layers explicitly only for property function22. The available Reimagined Properties.txt defines `bar` as function21, stat `item_addclassskills`, fixed value1=4. MagicPrefix.txt defines Slayer's (+1) and Berserker's (+2) rows. This reference data is not proof of the currently loaded vanilla table; inspect its function21/layer encoding before implementing. The current matcher drops a nonzero class layer, explaining a likely missing Barbarian prefix label.
- Fixed min=max does not eliminate affix provenance or a tier family. `Label` already supports a prefix/suffix label with tier0; `Tier` currently always returns at least1. Do not conflate missing range with missing tier.
- `src/plugin.cpp::AffixLabels` rejects all item qualities except4/6/8 before reading rolled IDs. Unique quality7 is therefore categorically excluded, including any crafted Sunder that remains unique.
- Required Sunder investigation: inspect the actual crafted unique's six stored affix IDs and their native table rows, compare with fixed unique properties, and establish its eligible tier pool. Do not simply enable quality7 or call every unique property a prefix/suffix. No new RVA or ABI was established in this investigation.

No DLL or runtime behavior changed in this follow-up.
