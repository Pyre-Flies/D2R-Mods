option casemap:none
EXTERN SelectAlwaysButtonTarget:PROC
EXTERN __guard_dispatch_icall_fptr:QWORD
PUBLIC AlwaysButtonAdapter
.code
; Windows x64: (RCX=device, EDX=user, R8D=mask), uint64 return in RAX.
; Keep the Core return address intact for downstream caller-qualified hooks.
AlwaysButtonAdapter PROC FRAME
    sub rsp, 48h
    .allocstack 48h
    .endprolog
    mov [rsp+20h], rcx
    mov [rsp+28h], rdx
    mov [rsp+30h], r8
    mov [rsp+38h], r9
    mov r9, [rsp+48h]
    call SelectAlwaysButtonTarget
    mov rcx, [rsp+20h]
    mov rdx, [rsp+28h]
    mov r8, [rsp+30h]
    mov r9, [rsp+38h]
    add rsp, 48h
    ; Preserve the project's CFG protection for this indirect tail dispatch.
    jmp QWORD PTR [__guard_dispatch_icall_fptr]
AlwaysButtonAdapter ENDP
END
