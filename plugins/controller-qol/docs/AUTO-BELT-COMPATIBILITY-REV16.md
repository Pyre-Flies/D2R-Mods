# Auto Belt Refill coexistence - rev.16 candidate

## Live owner and cause

2026-09-27 PID 11420: game +0x15F660 (stored-item placement helper) branches via
E9 to relay 0x13FFF0CD3, FF25 absolute destination Auto Belt Refill +0xB7F0.
Loaded owner: mods/ReimaginedLadder/d2rloader/plugins/d2rl-auto-belt-refill.dll,
version 0.35.9, SHA256
77C7D1B9068C8CACC2FDED62FF1120B441001D601AD65A8EE3043E3B61C650DB.
The native function's remaining bytes +5 through the full existing witness match.
Other original belt witnesses match except free-slot +0x3862D0, which has the
already supported Potion Auto Pickup detour (relay 0x13FFF04D3).

Wrapper +0xB7F0 (0x55F bytes) copies the optional placement state for observation,
loads original from owner +0x136438, forwards the five arguments unchanged once,
and saves its result. After success, stored-item flag and bottom-row slot checks
permit belt-column bookkeeping through owner +0xAB50. Optional bounded diagnostic
logging follows. Original trampoline 0x13FFF0CC0 copies the five overwritten
bytes then FF25 jumps to game +0x15F665. Candidate invokes the public game entry,
not this trampoline, preserving the owner's behavior. Wrapper observation of AL
is retained; QOL's existing placement submission/SDK confirmation semantics are
unchanged. Disassembly is evidence for forwarding, not proof of user-visible
combined behavior.

## Admission and standalone support

The original full function witness is always accepted first: Auto Belt Refill is
NOT required. The optional reviewed route requires original tail bytes, exact E9
then FF25 relay shape, target +0xB7F0 inside its module, exact owner file SHA256,
full live wrapper bytes, and a trampoline that copies original bytes and returns
to entry +5. Live code and pointer checks repeat before batches. File hash is
cached for that module; no arbitrary executable predecessor is accepted.
No patch, additional hook, or load-order requirement is added by QOL. Another
Auto Belt Refill binary/version needs review; unknown routes disable potion work
rather than bypass another plugin. Native SDK hooks are shared poorly without a
formal chaining contract; future SDK-mediated cooperation remains preferable to
expanding a per-plugin whitelist.

## Misleading ERROR diagnosis

Belt Validate previously used SDK CheckExpectedBytes for read-only preflight.
That API emits 'Plugin safety-check patch failed' at ERROR on mismatch, although
QOL installs no patch at +0x15F660. It then emitted its own WARN. Candidate uses
SEH-guarded read-only comparison and one QOL compatibility warning; unknown code
is still refused. Actual hook/patch installation errors elsewhere are unchanged.

## Validation

Release build and 17 suites pass. Added tests: original function without Auto
Belt Refill, changed body, unknown relay/owner, unavailable memory, quiet byte
comparison. Live original tail, Auto Belt wrapper and trampoline were inspected;
combined potion placement and independent load orders still require live testing.
User confirmed rev.15 Quick Identify works locally; reporter-specific result is
not established by that confirmation. No production release published.

Rev.16 candidate installed globally after game/loader closure and process verification. Rev.15 backed up locally; installed version and build/deployed hashes verified. User live potion test pending.
