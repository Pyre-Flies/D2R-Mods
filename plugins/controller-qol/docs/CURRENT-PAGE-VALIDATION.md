# Current-page validation: QOL rev.88 / UnHoarder 1.2.15

The October 21 mixed-loot reports show substantial residual cost in live page
qualification. The rev.87/1.2.14 pair improved user-reported FPS from about 43 to
65-70, but QOL compatibility still averaged 0.124 ms/call and native quality
observation 0.063 ms/call. Counts are callbacks, not distinct items; timers nest.

An owned-process probe isolates the region-query cost. On a 64MB allocation,
`VirtualQuery` took 155.472 us versus 0.497 us for `QueryWorkingSetEx` and 0.369 us
for `QueryVirtualMemoryInformation`. This is a mechanism experiment, not a game
measurement; allocation shape, residency and host scheduling affect timings.

## Equivalent predicates, fresh observations

The source-identical `current_page_query.h` / `.hpp` helpers are compiled into
each independent DLL. They share no runtime state and require no companion.
Only OS entry points and hardware page size are retained. No page, property,
rule outcome or successful native admission is cached.

For a range contained in one hardware page, the optional native-Windows path:

1. Reads current allocation information, checks complete range containment and
   ordinary allocation kind, and excludes physical/direct mappings.
2. Reads current working-set attributes and requires a valid, non-bad,
   non-large resident page. This establishes commitment for the admitted
   ordinary mapping. Current protection comes from `Win32Protection`, never
   the allocation's original `AllocationProtect`.
3. Reads allocation information again and compares base, size and every
   relevant allocation-kind field. Image readers additionally require
   `MappedImage` and the original exact module allocation base.
4. Applies the unchanged caller-specific protection predicates, then copies
   behind the existing SEH fault boundary.

The before/after allocation observation protects the split API observation from
a changed mapping. As with the old query-then-copy design, it is not an atomic
lock against all concurrent changes. Existing fault containment, native byte
comparisons, pointer ownership and later property/identity observations remain.

Missing/failed APIs, nonresident pages, unsupported mappings, cross-page ranges
and Wine/Proton use the original `VirtualQuery` path. These cases do not turn
unknown page state into success or change the independent plugin fallback.
The fast path retains guard/no-access, executable/writable and image ownership
conditions. Single-page containment retains the old contiguous-region bounds
requirement. PE parsing/section validation, all graph bytes/slots/continuations,
both property snapshots and all four Maps socket guards remain fresh.

Ground properties use the helper only inside `GroundCandidatePageReadable`;
native item-header copies, quality/level/flag comparisons, ordered rule
evaluation and full visibility queries are unchanged. QOL client-loot behavior
is unchanged. Other QOL `ImageMemory` users retain their original default mode.

## Evidence and limits

The production-reader regression first failed because the old reader did not
use the resident-page path. Tests exercise live protection changes, guard-page
preservation, unreadable/decommitted/nonresident pages, cross-page fallback,
wrong image ownership, copy-on-write data, unavailable APIs and allocation
changes between the two metadata observations. Existing actual-DLL mutation
fixtures still validate code, trampoline, continuation and page refusals.

Owned-process before/after medians: complete QOL 1.2.9 admission 23.196 to
8.807 us; four Maps guards 96.211 to 59.720 us; actual production quality reader
on an owned 64MB allocation 9.489 to 3.432 us. These final measurements include
both allocation observations. The QOL probe uses an older retained exact
profile to compare identical graph bytes; the new artifact is checked separately.
The combined callback fixture retains 1,800 fresh QOL admissions and native-code
calls, 1,800 property/socket observations, 3,600 quality observations and the
same 3,840-case digest `f9586a7d9ace0743`. Native calls in that fixture are spies.

Both perf commands remain read-only. `PAGE_QUERY` adds `residentSinceLoad` and
`fallbackSinceLoad`, explicitly cumulative counts. Subtract successive reports
to measure fast-path coverage; existing elapsed scopes remain interval-based.
QOL's perf version label is corrected to rev.88. Live FPS and visible behavior
of this new pair still require testing; no FPS gain is inferred from fixtures.

## Exact companion

UnHoarder 1.2.15 SHA256:
`9e7dbff8405b389b747d88180875107f2294701dee91521ea9c84785157673a8`.
Image size `0xEA000`, `HookGetItemCode` RVA `0x1FAA0`, original pointer slot
`0xB7AE8`. Wrapper:
`48 83 EC 28 48 8B 05 3D 80 09 00 FF D0 90 48 83 C4 28 C3`.
Release link-map symbols and DLL bytes establish the unchanged one-argument,
unchanged-return ABI. `item_code_resident_pages_contract.h` adds this complete
profile while retaining all thirteen earlier profiles. No game RVA was added.

## Primary API references

- [Working-set page attributes](https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-psapi_working_set_ex_block): only valid entries supply current protection.
- [Allocation information](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/ns-memoryapi-win32_memory_region_information): allocation ownership/type, distinguished from current page protection.
- [Allocation-query API](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-queryvirtualmemoryinformation): optional Windows 10 1607+ API; dynamically resolved with original-reader fallback.

Reproducible experiments live in the separate research repository under
`experiments/loot-performance-audit`. Local evidence and protected pre-change
sources are indexed under `outputs/scratch/loot-performance-20261021/`.
