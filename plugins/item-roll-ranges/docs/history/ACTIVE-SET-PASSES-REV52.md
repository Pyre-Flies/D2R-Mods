# Active set tooltip passes - revision 52

## Evidence and scope

October 17, 2026: user reported Sigon permanent prefixes disappearing after activation, and repeated nested Set labels on the shield. Read-only capture from PID38660 (private flatmax-sigon-active-20261017.json) contains all six pieces. Game was hosted by D2RLoader.exe; sandbox process enumeration incorrectly hid it. Elevated read-only access confirmed it remained running. No memory writes.

Existing exact-provider and native layout admission still applies. These are build-specific contracts, not a cross-build state enumeration guarantee.

- Item root +0x90 links child lists; child +0x68 next, +0x78 parent, +0 owner, +0x18 expiry, +0x1c flags, +0x20 state, +0x30 stats. All six items have host-owned flags0x40 / expiry-1 state0 permanent lists plus state165 and/or166 activated lists. Shield permanent stat127=1 and state165 stat127=1; its aggregate is2. Boots permanent43=40,96=20,99=15 remain distinct from165 stat19=50 and166 stat80=50.
- Reimagined states rows165..170 are itemset1..itemset6;171 is runeword. Only set quality5 admits165..170, with the same owner/parent/flags/expiry/vector/cycle checks. Unrecognized states still refuse capture. Activated values are copied separately, never added to own; original aggregate stays intact.
- Live disassembly Game+0x2DC4E0..0x2DC529 loads arguments5/6/7 as state1/state2/flags. At0x2DC559..0x2DC55F state1 and flags feed0x2F5A00; at0x2DC5BE..0x2DC5E1 nonzero state2 selects a second list and merges it. This corroborates the existing documented formatter ABI. No new hook or admission relaxation.

## Corrections

Range merging operates only on the newly appended suffix, preserving earlier output byte-for-byte. If either native pass shortens or changes the supplied prefix, return native actual output without annotations. Capacity remains bounded by the remaining output space. A repeated-pass production-adapter regression failed before this correction.

Permanent set solving uses owned modifiers plus attached socket values, excluding base header values and activated bonuses. This scoped model is a copy; native aggregate/header data is not rewritten. State165..170 formatter passes preserve actual native output, without applying permanent SetItems definitions. Conditional/full-set numeric roll annotation remains outside this revision's scope.

A production fixture with activated state165 and overlapping All Skills failed before the capture/model correction. Tests cover six state selectors, native output preservation, activation/removal, unknown states, foreign ownership and temporary expiry. Captured catalog checks supplement these tests but do not prove visible equip transitions. Live confirmation remains pending.

Validation: rev.52 builds with MSVC /W4 /WX; all nine suites pass, including artifact ABI/version checks. Catalog replay: 2,094 paired endpoint samples and 2,258 permanent set scalar samples across422 rows, zero unexpected failures. Operation audit:58,704 isolation checks and1,866 ED checks pass;694 charge-source identity checks pass. These catalog totals are permanent-definition checks, not additional equipped-set UI coverage.


User validation: Sigon equip/removal and shield formatting confirmed good. Follow-up Cleglaw/socket/swap checks reported good except the separately investigated poison line.
