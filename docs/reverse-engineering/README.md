# Reverse-engineering registry

This directory is the cross-project index for derived D2R and D2RCore native
contracts. Feature-specific evidence remains beside the plugin that consumes
it; this registry prevents useful paths and RVAs from being rediscovered.

See [Finding RVAs and interception points](RVA-AND-INTERCEPTION-WORKFLOW.md) for
the static-analysis, live-observation, byte-guard, validation, and publication
workflow used to produce these records.

Cross-project summaries:

- [Autosort transaction research](autosort-transactions.md)
- [Controller QOL native-contract registry](controller-qol-native-contracts.md)

## Tested compatibility profiles

| Consumer | Module | Required SHA-256 / profile | Evidence |
|---|---|---|---|
| Item Roll Ranges | D2RCore.dll | `AE1EA9B7F97AF5B89A550281E6A6C6B6E9C74E73AC8759E6558B40E751428CD0` | `plugins/item-roll-ranges/docs/` |
| Autosort transaction research | D2RCore.dll/provider | `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2` | [autosort-transactions.md](autosort-transactions.md) |
| Controller QOL Updates | D2R.exe and related modules | Multiple guarded feature profiles | `plugins/controller-qol/docs/HOOK-INVENTORY.md` and feature documents |

Hashes identify only the files that were actually tested. A matching product
version without a matching hash is not considered equivalent.

## Recording format

Add discoveries to a feature document or table with:

| Field | Required content |
|---|---|
| Date | Discovery or validation date |
| Consumer | Plugin/feature using the contract |
| Module | PE module containing the target |
| Build identity | SHA-256 and useful file/product version |
| Location | RVA, never a session-only absolute address by itself |
| Guard | Expected bytes, layout check, or exported ABI version |
| Contract | Inputs, outputs, side effects, thread/lifetime requirements |
| Evidence | Disassembly/audit path and reproducible command |
| Validation | Unit, integration, and live-game results listed separately |
| Failure mode | Required fail-open/disable behavior |

If a runtime absolute address is useful, record the module base and derived RVA
alongside it. Never assume ASLR-stable process addresses.

## Known high-value entries

| Module | RVA / layout | Purpose | Qualification |
|---|---:|---|---|
| D2RCore.dll | `0x422500` | Public ItemService existing-item transaction entry | Guard this public entry, not inner wrapper `0x427440`; autosort tested profile only |
| D2RCore.dll | `0x427440` | Inner existing-item transaction wrapper | Reference only; not the public admission guard |
| D2R.exe | `0x878D30` | TabBar controller navigation target | Controller probe build only; exact-byte guarded |
| D2R.exe | `0x27DF80` | UI switcher path | Controller probe build only; exact-byte guarded |
| D2R.exe | `0x14C6810` | Skill Tree controller path | Controller probe build only; exact-byte guarded |
| D2R.exe | `0xC66A0` / `0xC6E90` | Ground-label press/release paths | Controller probe build only; exact-byte guarded |
| D2R.exe | `0x1FAE90` | Label setter | Controller probe build only; exact-byte guarded |
| D2R.exe | `0xCE450` | Label refresh | Controller probe build only; exact-byte guarded |
| Installed provider ABI | operation `64` bytes, page `+56`; destination `40` bytes, page `+32` | Explicit Shared Stash transaction page | Additive ABI observed in tested provider; legacy non-shared layouts remain 56/32 bytes |
| D2R.exe | `0x15F660` | Stored-item belt placement action | Controller QOL tested profile; native asynchronous submission requires SDK observation |
| D2R.exe | `0x159B30` / `0x2CF680` | Advanced-stash withdrawal sender / widget wrapper | Reimagined Materials/Gems/Runes surfaces; UI-thread and layout-qualified |
| D2R.exe | `0x23FED0` | Native VendorPanel quick-sell wrapper | Caller-derived six-argument contract; invocation is not sale confirmation |
| D2R.exe | `0x23AD80` | Selected Shared-owner record resolver | Convert record to unit through ID getter/client lookup; category index is not storage page |

Feature documents contain the authoritative signatures and surrounding context.
This table is an index, not sufficient justification for an unguarded hook.
