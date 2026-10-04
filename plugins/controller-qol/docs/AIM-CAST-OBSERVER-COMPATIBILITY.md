# Rev.54 optional Cast observer and hook ownership

2026-10-04. Existing game profile and pinned D2RCore remain required. No new RVA,
ABI, layout, signature or native function call is added. Cast is the existing
15-byte hook at game `0x4FDB40`; all original arguments/result pass through.

Evidence: local QOL log at 06:53:29 reported Cast byte mismatch and disabled aim,
after `celestialrayone.whirlwind` reported its features active at 06:52:56.
The local `d2rl-celestialrayone-whirlwind-follow-cursor.dll` SHA256 is
`69E30904E138505DC676CA17C84B862097433812EAE7F8337946D30443EB00D3`.
Logs and the user's report support a competing entry hook; they do not establish
which of that plugin's subfeatures owns Cast or its detour ABI. Its configuration
describes continuous cursor-following Whirlwind, which can independently compete
with QOL's snap/pass-through endpoint behavior.

Rev.54 separates essential admission from optional observation:

- Existing Lookup `0x190440`, UnitTest `0x1919F0`, PointScore `0x18AF30` and
  Selected `0x18DDE0` hooks remain required with all prior guards. UnitTest still
  admits only the two known QOL contact relays and their proven destinations.
- The mandatory site loop excludes Cast. After all four essential hooks succeed,
  `cast_observer_enabled=true` reads and compares the complete existing CastBytes
  witness with ReadProcessMemory. Only a match allows SDK hook installation.
  Changed bytes/unreadable memory or SDK refusal skip only Cast, with a diagnostic.
  No foreign detour is called, chained, overwritten or accepted as native bytes.
- `cast_observer_enabled=false` does not read or attempt to patch Cast. It leaves
  the entry free for later-loading plugins. Default true preserves full QOL
  observation on a clean site, but does not promise coexistence if QOL claims
  Cast before another plugin; explicit opt-out covers that load order.
- Cast callbacks require the observer's admitted state before observing. Shutdown
  disables it; SDK owns cleanup. Optional failure does not flip installed aim off.

In fallback, requested coordinates, aim movement, native rotation, circular
snapping, retained enemy/pass-through and reticle drawing remain available.
Actual-cast logs/comparison, debug LAST CAST marker and Teleport displacement
samples are unavailable. The UI still shows requested range/cursor positions.

Skill isolation retains ReadView's admitted local foreground/controller active
skill checks and the available Cast observer's additional cast-time reset.
Rev.54 also reset intent from Selected's requested ID while active skill was
idle (-1). Rev.55 removes that reset: Selected (existing 0x18DDE0 getter) can be
queried without a cast, so a disabled request alone is not cast evidence. The
original getter still runs once and disabled/mismatched requests retain its
native result. No new native contract or site is introduced. Without the cast
observer, short unsupported casts bypassing an observable active-skill state
may not release retained intent; they still cannot acquire coordinate routing.
Live rapid self/corpse-cast transitions require testing.

Tests exercise production Install through a compile-time test-only entry in the
gating test executable, never the shipped DLL. Read/write signature fixtures are
not executed: fake SDK registrations verify five hooks on a clean site, four
with a foreign Cast prefix (left intact), four with explicit opt-out, retained
aim on optional SDK refusal, and no hooks on essential Lookup mismatch. Policy
tests cover the toggle, default, malformed/duplicate values and idle query preservation.
Automated checks do not establish live coexistence, either plugin's Whirlwind
semantics, module unload ordering or complete short-cast coverage.

Validation: all 23 QOL suites passed with /W4 /WX for the aim module/tests,
including the real-installer fixture cases and shipped DLL version/ABI/defaults.
Confirmed no D2R/loader processes before installing rev.54 locally. Prior DLL
and TOML are retained in ignored
`local rollback controller-qol-rev54-20261004-071008`.
Built and deployed SHA256 match:
`10D89D50CE473282CF031E2B2530C56429B5F5CDB0CD8369BDF7A0FA74C11E6B`.
Deployed config comparison confirmed only default `cast_observer_enabled=true`
was added; every existing tuning/class/custom/QOL choice was preserved.
The other Whirlwind DLL/config remain unchanged. Live loader admission and
Whirlwind/quick-transition tests with both plugins remain pending.

## Rev.55 reticle regression

The user's reticle disappearance also occurs without the competing Whirlwind
plugin. The latest local rev.54 log (2026-10-04 08:10:02) reports successful Cast
observer/essential aim installation and projection status 6 (ready). Source
review identifies idle Selected requests clearing manualAim and setting the
release latch; Frame suppresses normal reticles when manualAim is false. This
is a reproducible policy defect, not proof of every cause of invisible reticles.
Rev.55 removes only the idle requested-ID reset. Regression checks cover all
disabled catalog IDs returning native routing while preserving idle intent.
Visible recovery remains unverified until a live right-stick test.

Rev.55 validation: all 23 suites passed, including DLL ABI/version and idle-query
regression checks. Verified D2R/D2RLoader closed before deployment. Prior DLL
and current user TOML backed up to ignored
local rollback controller-qol-rev55-20261004-081557.
Runtime TOML left untouched and hash checked; built/deployed DLL hashes match:
04DB3169D8CC65B3794061F4FD4CA9E70FA982984AF614DAD9AE1F74216F2F16.
Live reticle recovery remains pending.
