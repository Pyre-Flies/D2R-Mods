# Identify stat-70 candidate - rev.15

## Problem and implementation

The reporter's disassembly demonstrated that the old matcher rejected ibk when
ItemInfo.quantity (+0x3C) was non-positive. Their actual native/SDK disagreement
is still not captured. The candidate recognizes Inventory/Cube tome codes first,
then reads stat 70, layer 0, through the scoped SDK editNativeItem callback on the
game thread. That callback performs no writes and retains no native pointer.
All tomes use the admitted native read, including when the SDK quantity is positive.
Unknown native charges refuse tome use; loose scroll behavior remains available.

For a selected tome, re-read current charges, ask SDK editItem Quantity for q-1,
and confirm stat70 equals q-1 before identifying. A rejected identification uses
SDK Quantity to restore q and verifies that readback. Last-charge tomes remain
empty books. No native stat writes, unchecked debits or retry after uncertain
consumption. A failed charge edit/readback does not identify the target; it logs
an explicit failure for inspection. This sequence is compensating, not atomic.
The SDK ExistingItemEdit transaction payload explicitly cannot edit Quantity;
Debit was therefore not used for the native tome path. Whether Debit has the
same quantity inconsistency on the reporter's setup remains unproven.

One bounded diagnostic per tome identification (temporarily also on success)
reports SDK/native quantities, remaining charges, read failures, mismatches and
queue/work time. No per-item logging. File hashing occurs at initialization,
not during identify. Existing exact target identity and SDK owner checks remain.

## Qualified live getter contract

2026-09-27 live PID 14604, game base 0x140000000, Core base 0xC0DE5000000.
Core SHA256 2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2.
Game +0x2F5020 = FF 25 F2 51 B3 03 then four NOPs. Its indirect pointer at game
+0x3E2A218 equals Core +0x831DE0. Core wrapper bytes call +0x3D8BD0, truncate the
return to EAX, and return. ABI used: int32 __fastcall(void* unit,int32 stat,uint16
layer), always (70,0). Body +0x3D8BD0 gets unit stat list +0x88, validates stat ID
below 0x8000, and uses the widened provider key (stat <<32 | layer). No unit layout
is directly read by the plugin. The historical capture had a different indirect
slot (+0x3E2A1C0); it is not admitted by this exact current profile.

identify_stat_profile.h holds the live 10-byte entry, 16-byte wrapper and 0x120
body witnesses. Admission requires exact Core file hash plus entry, slot target,
wrapper and body checks. Every read rechecks code and link witnesses. A different
provider, slot or competing hook is refused rather than bypassed. No additional
hook or patch is installed. Future versions must re-qualify this profile.

## Evidence and pending checks

MSVC Release and all 16 suites pass. Native charge mock regressions: SDK 0/native
97 ->96; SDK -1/native 1 ->0 keeping book; SDK positive/native 0 refuses;
SDK write success without charge change refuses identification; identify failure
restores charges; failed restore is reported; unavailable native read refuses.
Live getter chain is verified; the native call and SDK edit/readback must still
be exercised in-game. User has a 97/100 tome and two unidentified items ready.
Expected test: identify two items, ending 96 then 95, with one charge each.
Reporter-specific mod behavior remains unverified until their candidate test/log.

Candidate installed globally after process-closed verification; rev.14 backed up locally. Built and installed DLL hashes matched, version resource verified as 1.3.1+rev.15. Live two-item charge test pending.
