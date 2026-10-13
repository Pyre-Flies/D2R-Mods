option casemap:none
EXTERN SocketPostIconEntry:PROC
PUBLIC TestPostIconGate
PUBLIC TestPostIconContinuation
.code
TestPostIconGate PROC FRAME
    push rdi
    .pushreg rdi
    push rsi
    .pushreg rsi
    push rbx
    .pushreg rbx
    sub rsp, 80h
    .allocstack 80h
    .endprolog
    mov rdi, rcx
    mov rsi, rdx
    mov rax, 123h
    mov rcx, 234h
    mov rdx, 345h
    mov r8, 456h
    mov r9, 567h
    mov r10, 678h
    mov r11, 789h
    pcmpeqd xmm0, xmm0
    movaps xmm1, xmm0
    movaps xmm2, xmm0
    movaps xmm3, xmm0
    movaps xmm4, xmm0
    movaps xmm5, xmm0
    stc
    jmp SocketPostIconEntry
TestPostIconContinuation LABEL NEAR
    pushfq
    pop rbx
    test bl, 1
    jz failed
    cmp rax, 123h
    jne failed
    cmp rcx, 234h
    jne failed
    cmp rdx, 345h
    jne failed
    cmp r8, 456h
    jne failed
    cmp r9, 567h
    jne failed
    cmp r10, 678h
    jne failed
    cmp r11, 789h
    jne failed
    pmovmskb eax, xmm0
    cmp eax, 0ffffh
    jne failed
    pmovmskb eax, xmm1
    cmp eax, 0ffffh
    jne failed
    pmovmskb eax, xmm2
    cmp eax, 0ffffh
    jne failed
    pmovmskb eax, xmm3
    cmp eax, 0ffffh
    jne failed
    pmovmskb eax, xmm4
    cmp eax, 0ffffh
    jne failed
    pmovmskb eax, xmm5
    cmp eax, 0ffffh
    jne failed
    mov eax, 1
    jmp done
failed:
    xor eax, eax
done:
    add rsp, 80h
    pop rbx
    pop rsi
    pop rdi
    ret
TestPostIconGate ENDP
END
