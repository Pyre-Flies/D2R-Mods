option casemap:none
EXTERN ObserveHeaderRequest:PROC
EXTERN HeaderBuilderAdapter:PROC
PUBLIC HeaderBuilderEntry
.code
HeaderBuilderEntry PROC FRAME
    sub rsp, 48h
    .allocstack 48h
    .endprolog
    mov [rsp+20h], rcx
    mov [rsp+28h], rdx
    mov [rsp+30h], r8
    mov [rsp+38h], r9
    mov rcx, rbx
    mov rdx, [rsp+48h]
    lea r8, [rsp+0d0h]
    call ObserveHeaderRequest
    mov rcx, [rsp+20h]
    mov rdx, [rsp+28h]
    mov r8, [rsp+30h]
    mov r9, [rsp+38h]
    add rsp, 48h
    jmp HeaderBuilderAdapter
HeaderBuilderEntry ENDP
END
