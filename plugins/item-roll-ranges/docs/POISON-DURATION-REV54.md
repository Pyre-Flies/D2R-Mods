# Poison duration bounds - revision 54

## Report and reproduction

Cleglaw's Claw had unannotated Adds 4-8 Weapon Poison Damage over 4 Seconds. User confirmed the other set/socket/swap checks looked good. A production AffixLabels/RangeText regression using the captured rates failed on the prior code: the complete native line remained unannotated.

Read-only PID55664 capture, October17: class329 set row7, bank3; permanent stats57=10,58=20,59=100,326=1. Activated lists165/166 contain resistance and block bonuses, not poison. Compiled SetItems property138 has parameter100 and endpoints10/20; Properties slots are fn15/stat57,fn16/stat58,fn17/stat59. Source inputs remain private in outputs/irr-poison-20261017/captured.bin. No game memory writes.

## Native display contract

Existing exact-build formatter disassembly, Game0x2D8BF0 family: poison flag is argument4/R9B, copied toR14B at0x2D8C24. At0x2D8D31..0x2D8D47, multiply each damage rate by duration, add128, then arithmetic shift8. Positive admitted operands therefore display floor((rate*frames+128)/256). At0x2D8F08..0x2D8F18 duration uses signed division25. Cleglaw resolves4/8 and4seconds; raw10/20 are not display bounds.

Independent runtime guards require bytes at0x2D8D31:
8B4F108BC10FAF47080FAF4F0C83E880C1F80883E980C1F908
and0x2D8F08:
B81F85EB51F76F10C1FA038BC2C1E81F03D0.
These are build-specific, alongside existing property-source admission. Refusal keeps native poison text. Attempted live code verification after the data capture found the game had exited; guard admission must be checked on restart.

## Implementation and limits

Poison's third numeric duration token is eligible only when definitions contribute poison; other damage families retain two-number matching. Source/endpoint definitions and actual values must agree. Convert endpoint bounds using the same rounding, then require displayed actual endpoints and duration to match. Single displayed damage totals are accepted only when minimum equals maximum. Keep native localized wording/duration intact.

Numeric poison expansion currently requires one source group, one resolved source label, one explicit non-replacement fn17 duration matching storedframes, nonnegative rates, bounded32-bit products, and no multiple-poison count. Variable duration and multiple poison sources are left native; no averaged-duration attribution is invented. No database or item-name special case is used in production.

Rev.54 includes the existing rev.53-beta.1 controller caller-preservation work. Tests cover Cleglaw's production path, fixed and variable endpoints, equal endpoints, six durations, mismatching actual text/duration and definition refusal. Captured real-item replay and catalog checks supplement the normal suites. User confirmation of the new poison display is pending.
