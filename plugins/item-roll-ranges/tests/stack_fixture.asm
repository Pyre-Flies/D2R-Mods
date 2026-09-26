; Reproduce the witnessed range-call argument and local layout. The intermediate
; JMP models an entry detour: it must not add a new return address or frame.
EXTERN ProbeRangeEntry:PROC
PUBLIC RangeStackFixture
PUBLIC RangeStackReturn
.code
RangeStackFixture PROC FRAME
    sub rsp, 68h
    .allocstack 68h
    .endprolog
    mov r10, rcx
    mov rcx, 1234h
    mov rdx, 5678h
    mov r8d, 31
    mov r9d, 9
    mov dword ptr [rsp+20h], 80
    mov dword ptr [rsp+28h], 120
    mov qword ptr [rsp+30h], r10
    mov dword ptr [rsp+38h], 4
    mov dword ptr [rsp+44h], 118
    call FixtureDetour
RangeStackReturn LABEL BYTE
    add rsp, 68h
    ret
RangeStackFixture ENDP
FixtureDetour PROC
    jmp ProbeRangeEntry
FixtureDetour ENDP
END
