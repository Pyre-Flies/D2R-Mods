option casemap:none
EXTERN SocketScaleEntry:PROC
PUBLIC TestScaleGate
PUBLIC TestScaleContinuation
.code
TestScaleGate PROC FRAME
    push rdi
    .pushreg rdi
    push rsi
    .pushreg rsi
    push rbx
    .pushreg rbx
    push r15
    .pushreg r15
    sub rsp, 88h
    .allocstack 88h
    movaps [rsp+40h], xmm6
    .savexmm128 xmm6, 40h
    .endprolog
    mov r15, rcx
    mov rdi, rdx
    mov rsi, r8
    mov rax, [rdx]
    mov [rsp+28h], rax
    mov [rsp+38h], r9
    movss xmm0, dword ptr [r9]
    movss dword ptr [rsp+70h], xmm0
    pcmpeqd xmm0, xmm0
    movaps xmm1, xmm0
    movaps xmm2, xmm0
    movaps xmm3, xmm0
    movaps xmm4, xmm0
    movaps xmm5, xmm0
    movaps xmm6, xmm0
    movss xmm6, dword ptr [r8]
    mov rax, 123h
    mov rcx, 234h
    mov rdx, 345h
    mov r8, 456h
    mov r9, 567h
    mov r10, 678h
    mov r11, 789h
    stc
    jmp SocketScaleEntry
TestScaleContinuation LABEL NEAR
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
    mov rax, [rsp+28h]
    mov [rdi], rax
    movss dword ptr [rsi], xmm6
    mov rax, [rsp+38h]
    movss xmm0, dword ptr [rsp+70h]
    movss dword ptr [rax], xmm0
    mov eax, 1
    jmp done
failed:
    xor eax, eax
done:
    movaps xmm6, [rsp+40h]
    add rsp, 88h
    pop r15
    pop rbx
    pop rsi
    pop rdi
    ret
TestScaleGate ENDP
END
