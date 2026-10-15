# Item and base quality levels - revision45

Revision45 for D2R3.3.93787 / qualified D2RCore1.3.1-beta. Prepared for the
stable release-2026.10.15.1 suite. The user requested generated item level and base quality level
below Required Level and above weapon Attack Speed, including items without
a level requirement.

## Display contract

The existing range request adds one white `(Ilvl: 99, Qlvl: 20)` line. Ilvl is
the generated item's stored level; Qlvl is the current base Items row's `level`.
Neither is Required Level, affix level or the UniqueItems/SetItems selection
level. The active item bank and base row supply Qlvl for vanilla and mods.

Displayed weapon order: Required Level / level details / Max Sockets / Attack
Speed. Native body lines are stored bottom-to-top. Capacity is annotated first,
then levels are inserted immediately before the unique localized Required Level
line in that buffer. With no Required Level, use the lowest displayed recognized
requirement, durability, Defense or damage header. Header-less bodies use their
upper boundary, including empty bodies; surrounding name/description placement
still needs visible confirmation. No empty text pointer is dereferenced.

Unmatched byte witnesses, unreadable fields, nonpositive item level, missing
localization service, ambiguous anchors or oversized output omit the new line.
Base level zero is permitted. There is no invented99/100 item-level cap.
Existing identified-item, screen, hold/always-show gates and false default stay.
Duplicates are suppressed. Deferred publication retains owned text/receiver
identity, never an item/table pointer. No hook or provider slot is added.

## Native evidence, 2026-10-15

Read-only observation of process32880: executable D2RLoader.exe, window title
Diablo II: Resurrected. The existing loaded-game0x34A385 witness matched.
D2RCore file SHA256:
`2a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089b948fdeb5a2`.
Installed D2R.exe file SHA256:
`1e2ac459feb3f4bbfa818cdff49800480502beae9f90cfa4cba9e7e1f8bfa3b7`.
These game instructions were observed in the qualified loaded/decrypted image;
the on-disk D2R.exe code at these RVAs differs and was not used as its witness.

| Contract | Build-specific loaded witness |
| --- | --- |
| ItemData pointer | Existing unit+0x10 contract |
| Stored item level | Signed DWORD ItemData+0x38 |
| Native level getter | Core slot0x6FF8E0 -> game0x36CCF0 |
| Level load/repair | game0x36CD36: `8B 40 38 83 F8 01 7D F2 B8 01 00 00 00 89 41 38` |
| Active base row | Existing Core slot0x702230 -> game0x314110; bank/class ID, Items stride448 |
| Base quality level | Unsigned BYTE Items row+0x10D |
| Base level load | game0x3719CC: `0F B6 80 0D 01 00 00` |
| Additional consumers | game0x2326B5 and0x31594C load the same byte after base-row resolution |

The native level getter loads+0x38, returns it when at least one, and otherwise
can write1 back. Revision45 reads the field directly and never calls it. Exact
optional byte windows in level_profile.h supplement existing provider/header
admission; refusal affects only the new line. Fields are read synchronously in
the already-qualified header callback. No item/stat/table data is written.

The observed Long Sword row held20 at+0x10D. A follow-up source-table comparison
did not finish because the game exited, so no full-table comparison is claimed.
Private inspection scripts remain under ignored outputs/. RVAs are not portable.

Correction to earlier socket research: Core capacity code calls slot0x6FF8E0
at Core0x376433. That getter can repair invalid item levels too; the earlier
statement that calling Core necessarily avoids every such mutation was too
broad. Revision45 does not change the capacity calculation.

## Validation

Release build with /W4 /WX, all nine suites and artifact admission pass. New full-body regressions
cover socketed/unsocketed weapon and armor ordering, absent Required Level,
fixed plain Sash, header-less/empty bodies, translated/duplicate/ambiguous
anchors, unchanged item/row bytes, invalid fields, optional guard refusal and
one-shot publication after TLS restoration. The user confirmed requested visible
placement and Ctrl release after restart; see the live record below. This remains
the stable release candidate; publication requires CI and downloaded-asset verification.


Candidate installation completed after checking actual game processes and target
DLL holders twice. Normal d2rloader/plugins and standalone
mods/Reimagined/d2rloader/plugins both contain revision45, SHA256
`2534ACFA55AE215FA4EE036ADE9F695C9B9F41344A679813D3AC374B5CE0F425`.
Both revision44 copies were backed up and hashes verified under local
Documentation/item-roll-ranges/backups/. Every pre-existing runtime TOML and
companion DLL hash was preserved. No managed Ladder installation or public
release was changed. Restart validation is recorded below.


### User-confirmed restart, 2026-10-15

The user answered "Yes, looks good" to the weapon and no-Required-Level item
placement/hold-release check. The root loader log records revision45 from the
Reimagined mod and skips the global duplicate because the mod copy is active.
Both deployed DLL hashes still equal the candidate hash above. The active mod
plugin log admits the linked-property model and native hold mode, completes
chat registration with result0 after its normal queued retry, and later records
AlwaysShowRolls ON. There are no plugin errors or optional level-guard refusals.
The game had already exited at inspection; no fresh process-memory byte read
was performed. Admission of the two new byte windows follows from the guarded
line being displayed and absence of refusal, rather than a separate post-test
memory observation. This session confirms Reimagined, not an independent
vanilla session.

The same loader session contains shutdown assertions for D2Prism's sprite asset
pool, its bitmap allocator and the object-pool empty/count checks. No Item Roll
Ranges frame or causal ownership is established by those reports. The object-
pool signatures predate revision45: see the Assertions section of
the project's October11 crash triage.
They remain a separate unresolved shutdown issue; this is not a clean whole-game
shutdown claim. Raw logs and the validation receipt were preserved privately
under outputs/item-levels-rev45-validation/. No plugin/config/DLL was changed
while recording the confirmation.
