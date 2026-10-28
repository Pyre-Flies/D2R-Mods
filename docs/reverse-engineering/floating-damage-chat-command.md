# Floating Damage: native chat command contract

Reviewed 2026-10-11. Native parser registration and on/off/motion dispatch are
confirmed on the admitted profile. The same bounded handler supplies optional
capture controls. This record preserves registration and callback-lifetime
contracts without including private logs or deployment records.

## Admission and service boundary

The executable/Core identities are listed in the [HP contract](floating-damage-numbers-feasibility.md).
Floating Damage admits those artifacts and its normal byte windows before
resolving a chat entry. Its vendored native command/image-reader sources are
user-owned source from the sibling `d2rl-plugins` libraries; no external runtime
DLL or full chat-routing stack is required.

SDK `RegisterConsoleCommand` registers the loader debug console. It does not
register a slash command with the game's chat parser. Successful game-chat
routing uses the native backend described below; no network chat packet is
sent, and feedback appears in the loader console rather than chat.

## Entry and caller witnesses

All addresses are relative to the reviewed game image. The older analysis
image SHA-256 is `02C32A05F182C3F1F1B29D58B2B8DA735EADC080B5E13ACF8EA721A0648CE60D`.
Static witnesses require current runtime pattern/layout validation.

| RVA | Contract and pattern |
|---|---|
| `0x2E6550` | Parser TLS getter, `uint64_t* (__fastcall*)()`. `48 83 EC 28 65 48 8B 04 25 58 00 00 00 8B 0D 65 DF 2C 03 BA 6C 12 00 00 48 8B 0C C8`; wildcard RIP displacement at `+15`, preserve immediate `0x126C`. |
| `0xA281D0` | Registration entry, `uint32_t (__fastcall*)(uint64_t parserId,const Definition*)`. `41 56 48 83 EC 40 4C 8B 05 93 07 DB 01 4C 8B F2 48 8B D1 4D 85 C0`; wildcard RIP displacement at `+9`. |
| `0x2E87C5` | Unique command-loader caller: `84 C0 74 43 E8 ?? ?? ?? ?? 48 8B D6 48 8B C8 E8 ?? ?? ?? ?? EB 31`; first relative call resolves the getter. |

Registration/caller patterns must each match exactly once in readable `.text`.
Getter and registration must be primary `.pdata` function starts; the getter
prologue must match because its TLS shape alone is not unique.

Additional definition-reader guards check `49 8B 96 disp32` at registration
entry offsets `0x1BB`, `0x1CA`, `0x1D9`, `0x1E8`, reading record offsets
`0x188`, `0x190`, `0x198`, `0x1A0`. Their instruction RVAs are `0xA2838B`,
`0xA2839A`, `0xA283A9`, `0xA283B8`. Refusal preserves the SDK debug-console
command. Runtime registration resolved the same getter/registration/caller
RVAs, matched layout guards, deferred when the UI queue returned OwnerInactive
(4), and subsequently registered with result 0.

## Definition ABI and retained callback lifetime

The Win64 record is `0x1C0` bytes:

| Offset | Field |
|---|---|
| `+0` | Unsigned 32-bit version, 1. |
| `+4`, `+0x44` | Name[64], description[64]. |
| `+0x84` | Aliases[8][32]. |
| `+0x188`, `+0x190`, `+0x198` | Execute/help/completion pointers. |
| `+0x1A0` | Literal translator pointer. |
| `+0x1A8`, `+0x1B0` | Child pointer and unsigned 64-bit child count. |
| `+0x1B8` | Flags byte, followed by tail padding. |

Execution is `void (__fastcall*)(const char* args)`; translation is
`void (__fastcall*)(const char*,char*,size_t)`. No C++ object crosses the native
record boundary. Borrowed arguments are bounded to 256 characters and consumed
synchronously without retaining the pointer.

UI-thread SDK work registers after a nonzero parser ID exists. Shared UI events
retry refused queues or late parser creation. Observed result interpretation
is 0 registered, 7 duplicate, other values failed; this is not a complete
native error-code ABI.

The backend owns 64 process-lifetime slots per DLL and pins its callback module
with `GetModuleHandleExW(PIN)`. The native parser has no supported unregister.
Stop disables retained thunks, cancels deferred listeners and drains active
callbacks while keeping their storage valid. State-mutex teardown follows
callback draining to prevent lock inversion. Restart is required to replace or
remove this plugin; hot reload is unsupported.

## Controls and verification

`/damagenumbers [on|off|status]` and an empty/toggle argument control the feature.
Motion and capture accept the same bounded control grammar. Feature transitions
clear baselines/numbers and invalidate queued generations; repeated on/off and
status preserve current state. The command cannot override native admission.
Setting changes (`on`/`off`/`toggle` and `motion`) persist `[floating-damage] enabled`
and `falling_motion` via [chat_config.h](../../plugins/shared/chat_config.h) (see
[CHAT-COMMAND-SETTINGS.md](../CHAT-COMMAND-SETTINGS.md)). Capture is opt-in,
session-only, capped at 128 records, and stops on explicit off, game leave or unload.

The two-DLL automated fixture verifies routing/help, duplicates/failures,
queue refusal, delayed parser creation, cancellation, callback draining and
safe retained callbacks after `FreeLibrary`. The real plugin adapter fixture
checks off/motion/capture dispatch and fresh-baseline behavior. User actions
and plugin logs establish native registration/dispatch in game; no claim of
feedback appearing in chat is made.
