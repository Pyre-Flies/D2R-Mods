# Optional always-show ranges, revision35

Local candidate, 2026-10-14. Targets the existing qualified Core fingerprint
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`
and admitted game/formatter profiles. These addresses are build-specific.
No Item SDK dependency, physical-key polling or item/stat writes are added.

## Configuration

New embedded/reference `item-roll-ranges.toml`:

```toml
[display]
always_show_ranges = false
```

The loader's existing ReadConfig API reads the saved main config. Missing or
malformed/duplicate known settings retain native hold mode with a warning.
The single-boolean parser accepts standard [display] syntax and lowercase TOML
booleans, comments and CRLF/BOM, with a 64KiB bound. It ignores unrelated
keys/sections and never writes user text. Restart after editing. The embedded
default is false; runtime archives carry the reference only under `defaults/`.

## Native range request

The current renderer normally observes Core's native range clone. Setting a
local rendering flag alone cannot produce complete native range data. Enabled
mode therefore adds two optional query-result adapters. Disabled mode installs
neither. They call the captured native target exactly once with identical
arguments, then add only the range-request bit at the admitted tooltip caller:

| Core slot | Qualified return | ABI / observed game target |
| --- | --- | --- |
| 0x6FE3B0 | 0x8195E0 | no arguments; modifier result in RAX/low DWORD; game RVA0x1209FA0 |
| 0x6FE470 | 0x81995A | RCX=device, EDX=selected user, R8D=mask; native boolean result in AL; game RVA0x13CA70 |

Keyboard forces bit2 without clearing other bits. Controller forces bit0 only
for a non-null device, selected user<8 and mask0x800, preserving the remaining
result carrier. Core's existing selection code converts those values to its
range request; native feature-enable, identification, input-mode, selected
backend and range construction logic remain authoritative. Foreground and
Inventory/Stash/Cube/vendor gates must also pass. Other callers and unsupported
contexts retain the full original result. Physical Ctrl/RT/RB state and normal
inventory actions are neither consumed nor modified.

Read-only loaded inspection confirms the existing full Selection and
ControllerCall witnesses. Additional keyboard call guard at Core RVA0x8195DA:
`FF 15 D0 4D EE FF`. Controller setup/call remains the existing guard at
RVA0x819949, ending at0x81995A. Current slot targets match the two game entries
above. The controller entry currently begins with an E9 detour while QOL is
loaded; preserve it and call that existing entry rather than replacing its
prologue or assuming an unhooked backend. Unknown slot targets refuse always
mode, retaining ordinary holds rather than assuming callback ABI/lifetime.

Publication uses the existing aligned-pointer compare/exchange and ownership
checks. The request flag activates only after both exchanges succeed; partial
publication rolls back owned slots. Restore disables requests first, restores
only pointers still owned by this plugin, and retains original targets for
late passthrough. The existing process-lifetime DLL pin covers these adapters.

Header/property request scopes also accept the admitted always request. That
is necessary for fixed/no-clone items: their native query result may request
ranges without producing a clone. Existing source identity, visibility,
formatting, Base headers, socket/corruption provenance and refusal rules remain.

## Range hint suppression

The SharedEventService ItemTooltipEvent contract starts with an empty
contribution buffer; it cannot remove existing native text. A separate optional
adapter instead filters the existing native widget footer handoff:

- Core slot0x704600, qualified return0x81920E, observed game target RVA0x843CA0.
- Four argument carriers: RCX=body C string, RDX=position/context pointer,
  R8=layout/style pointer, R9=nullable footer C string. RAX is forwarded.
- The reviewed game body retains all four register arguments and forwards
  the footer to its internal widget implementation at RVA0x880160. It reads
  no caller stack arguments. Core destroys its temporary footer after the
  native call, establishing the synchronous borrowed-string lifetime used
  by this adapter's owned replacement.
- Core RVA0x8191FB guard:
  `48 89 F1 48 8B 95 20 04 00 00 49 89 F9 FF 15 F2 B3 EE FF`.
- Game RVA0x843CA0 guard:
  `48 89 6C 24 20 57 41 56 41 57 48 83 EC 40 49 8B E9 4D 8B F0 4C 8B FA 48 8B F9`.
- Native key at Core RVA0x62BB20, length39 excluding NUL:
  `d2rloader:D2RLoaderItemTooltipRangeHint`. Key bytes including NUL are guarded.
- Core references it at RVA0x818031 and calls the already used lookup ABI
  RVA0x387D60 at0x81806B with d2r group, NativeText* key and required=true.
  Nearby keys holdShift/newline are layout inputs, not replacement range keys.

Only a unique complete footer line matching the localized range hint is
removed. Other controls, Shift comparison hints, body, layout, arguments and
native return stay unchanged. Empty/multiline/duplicate/unmatched templates
retain the footer. String copies are bounded (footer16KiB, hint1KiB), checking
readable region boundaries before reading. If widget guard/ownership admission
fails, native range requests remain enabled and only hint filtering is omitted.

## Evidence and limits

Read-only live checks on 2026-10-14 match all new byte/key guards and all three
slot targets. Initial disassemblies started at unqualified interior offsets
and were discarded; the final hint body was decoded from its verified export
entry RVA0x817DF0. No scan/probe executes native functions or writes memory.

All eight plugin suites pass: default resource/identity, config, scoped request
policy, unrelated-call argument/result/count preservation, later-owner restore,
short strings at readable-page ends, hint selection, prior ranges/headers and
source breakdowns. Eleven runtime packaging regression tests pass, including
preservation of an existing customized item-roll-ranges.toml on extraction.
The first packaging run hit sandbox tempfile access errors; the relevant suite
was rerun with normal access and passed. No functional failure was inferred
from the sandbox run. Native installation/rendering and controller behavior
still require candidate-specific restart validation. Nothing is published.

Installed rev.35 in the Reimagined mod-scoped plugins directory on 2026-10-14
after the user confirmed the game was closed. The installed DLL SHA-256 is
D8993F5C9C683CD3C29EA6A0C32CF78E2536A9F945ABBEB6C2AC4DA036412894.
The previous rev.34 DLL has a recoverable backup, and the new mod-scoped
item-roll-ranges.toml enables always_show_ranges for testing. All other DLLs
and TOMLs were preserved. Visible behavior after restart remains unverified.

## Rev.36 chat toggle extension

The user confirmed rev.35 always-show behavior. Rev.36 adds a session enable
atomic separate from adapter admission. Adapters are admitted at load regardless
of startup state; enabling/disabling changes only that atomic, with no slot writes.
All queries still call their originals exactly once. Restore stops native chat
callbacks before disabling/restoring adapters.

Native chat uses the existing user-owned helper copied into vendor/ from Always
Show Sockets. Its SDK UI-thread queue and event listener integrate with D2R's
native slash parser; there is no input hook. Duplicate native registration result
7 refuses the name, and retained callback definitions pin the module until exit.
Existing parser ABI/signatures and discovery are recorded in
`docs/reverse-engineering/floating-damage-chat-command.md` at repository root.
No new game RVA or ABI was inferred for this extension. Unknown registration
layouts refuse only the chat feature; config and hold mode remain available.
Eight suites pass; live command registration/dispatch and combined-plugin chat
behavior remain unverified for rev.36.

### Rev.36 live confirmation, 2026-10-14

Rev.38 follow-up: the user reports plain unmodified Sash omits Base: 2-2
with always-show off and Ctrl held, while modified belts and always-show work.
The fixed/no-clone request now consumes a thread-local owned modifier value
captured before forcing always-show at Core return RVA 0x8195E0. Observation
at the existing qualified builder consumes and clears the value once. If the
optional query adapter is unavailable, the existing native re-query fallback
remains. No new address or input ABI is introduced. This removes the different
caller re-query from the admitted wrapper path; the precise runtime cause and
visible plain-Sash correction still require candidate-specific retesting.

Rev.40 correction: rev.39 diagnostics on 2026-10-14 at 11:30 showed all
caller/allowed/no-clone/frame gates passed, but the second diagnostic never
ran: the DWORD keyboard>1 check rejected the request. The qualified on-disk
Core Selection witness decodes at 0x8195EE as `B1 01` (mov cl,1), then at
0x8195F0 as `89 8D EC 18 00 00` (mov [rbp+0x18EC],ecx). Only CL is set;
upper ECX is unspecified across the preceding native query/adapter. Native
consumers at 0x819650 and 0x81974A read this local as a byte. Reading a DWORD
and requiring <=1 was therefore invalid; rev.40 reads unsigned char. The
existing full Selection and NoRollGate witnesses cover these instructions.
A regression fixture sets the DWORD to 0xA1B2C301 and verifies byte-mode
Ctrl request/release behavior. Bounded diagnostic output is removed. No new
game function is called or patched by this correction. Live restart pending.

The user reports the toggle works without visible issues. The Reimagined
item-roll-ranges.log session beginning 10:50:55 confirms rev.36 admission,
chat layout guards matched (getter 0x2E6550, registration 0xA281D0, caller
0x2E87C5), and native /alwaysshowrolls registration result 0 at 10:51:05.
Initial UI queue result 4 was pending and recovered through the documented UI
event retry. Nine command callbacks logged ON/OFF states between 10:53:01 and
10:53:59, ending OFF; the plugin log contains no warning or error.
Other local slash commands registered successfully in the same session.
Global Chat separately reported missing authentication and repeated HTTP 401;
its local commands still registered. This is not evidence of an IRR conflict.
These observations establish this session's registration and dispatch alongside
the loaded plugins, not universal compatibility with arbitrary plugins.

Rev.40 live follow-up: the user confirmed the byte-width correction restored
the plain Sash Ctrl Base label. Default embedded/reference always-show remains
false for release; native hold and session opt-in are retained.
