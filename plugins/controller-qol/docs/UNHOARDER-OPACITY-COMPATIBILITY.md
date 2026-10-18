# Background opacity compatibility

The filter replaces a fully qualified native label's complete RGBA background.
The previous guard nevertheless required the existing alpha to equal exactly
0.6 or 0.8. Other legitimate native opacities prevented the replacement while
text colors still applied, which could leave black set text on black.

Version 1.2.9 treats the old alpha as presentation data. Exact positive-zero
black RGB remains required; all finite normalized alpha values 0..1 and signed
zero are accepted. Bit patterns up to `0x3F800000`, plus `0x80000000`, implement
this domain check. Negative nonzero values, values above one, infinities and
NaNs remain refused. Selected blue and already-styled non-black backgrounds
are still refused. The JSON rule supplies the replacement alpha.

The native caller, row/pointer pairing, readable color, current item identity,
resolved rule, exact rendered name, arming and hide-row checks are unchanged.
No new native hook or item/row write is introduced. The complete plugin.cpp,
visibility service ABI, socket reader, sound path and pickup semantics are
unchanged from 1.2.8.

The matching QOL profile admits Unhoarder SHA256
`1E102EF38B1D26C56BBE12A118D7F21EA75E9CB10F527FCA70B856DC34D393FF`.
Its unchanged 19-byte item-code wrapper remains RVA `0x1DF60`, original slot
`0xB2488`: `48 83 EC 28 48 8B 05 1D 45 09 00 FF D0 90 48 83 C4 28 C3`.
The original is called once with unchanged argument/return. Admission still
checks artifact identity, native entry and relay, exact wrapper, image/page
ownership, original slot, trampoline and native continuation. An altered
wrapper is refused even with a known disk hash. Older profiles remain.

Regression coverage includes every integer opacity percentage, transparent
and opaque endpoints, adjacent float representations around previous values,
positive subnormal alpha, signed zero, invalid/nonfinite alpha, changed RGB,
selected blue and already-styled colors. The regression fails against 1.2.8.
All 37 Unhoarder CTest tests, 51 Python source checks and 63 matching QOL tests
pass. The user confirmed the visual fix after receiving the candidate build;
that confirmation is separate from automated and isolated artifact checks.
