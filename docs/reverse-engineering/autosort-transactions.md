# Autosort transaction research

Status: proof of concept, not one of the two released plugins in this repository.

The D2RLoader ItemService existing-item transaction path can perform atomic,
cursor-free exact-position moves in Inventory, Cube, Personal Stash, and an
explicitly addressed Shared Stash page. The tested public D2RCore entry is RVA
`0x422500`; it calls inner wrapper `0x427440`. Compatibility checks must guard
the public entry and its expected bytes.

The installed provider exposed an additive Shared Stash ABI:

- existing-item operation: 64 bytes, `sharedStashPage` at `+56`;
- destination: 40 bytes, `sharedStashPage` at `+32`;
- legacy non-shared operation/destination layouts remain 56/32 bytes;
- UI Shared Page 1 mapped to raw page ID `0` in the tested build;
- `UINT32_MAX` is unknown page identity, not a special-tab label.

All removals must be published before any placements. A zero transaction result
does not prove correct client state: validate authoritative postconditions,
UI-thread snapshots, publication counts, visible state, and persistence where
claimed. Plans are limited to 64 operations and should be refused rather than
split when atomic behavior is required.

Live validation covered inventory, Cube, Personal Stash, and Shared Stash page
0 swap/mirror/pack/undo. Personal Stash save/reload persistence passed. Shared
Stash persistence, additional page IDs, special-tab mappings, and high-occupancy
stress remain unqualified.

Tested provider hash:
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
