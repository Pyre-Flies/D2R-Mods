option casemap:none
EXTERN SocketCircleEntry:PROC
PUBLIC TestCircleGate
PUBLIC TestCircleContinuation
.code
; C++ inputs: sprite, anchor*, scale*, opacity*, filled, expected sprite (optional).
TestCircleGate PROC FRAME
    push rdi
    .pushreg rdi
    push rsi
    .pushreg rsi
    push rbx
    .pushreg rbx
    push r13
    .pushreg r13
    sub rsp, 108h
    .allocstack 108h
    .endprolog
    mov rdi, rdx
    mov rsi, r8
    mov [rsp+20h], rcx
    mov rax, [rsp+158h]
    test rax, rax
    cmovz rax, rcx
    mov [rsp+28h], rax
    mov [rsp+30h], r9
    movzx r13d, byte ptr [rsp+150h]
    mov [rsp+38h], r13
    mov rax, [rdx]
    mov [rsp+40h], rax
    pxor xmm0, xmm0
    movaps [rsp+60h], xmm0
    movaps [rsp+70h], xmm0
    movaps [rsp+80h], xmm0
    movaps [rsp+90h], xmm0
    movaps [rsp+0a0h], xmm0
    movaps [rsp+0b0h], xmm0
    movaps [rsp+0c0h], xmm0
    movq qword ptr [rsp+0d0h], xmm0
    mov dword ptr [rsp+60h], 3f800000h
    mov dword ptr [rsp+64h], 3f800000h
    mov dword ptr [rsp+68h], 3f800000h
    mov dword ptr [rsp+6ch], 3f800000h
    movss xmm0, dword ptr [r9]
    movss dword ptr [rsp+70h], xmm0
    pcmpeqd xmm0, xmm0
    movaps xmm1, xmm0
    movaps xmm2, xmm0
    movaps xmm3, xmm0
    movaps xmm4, xmm0
    movaps xmm5, xmm0
    movss xmm0, dword ptr [rsi]
    movss xmm3, xmm0
    pcmpeqd xmm0, xmm0
    lea rdx, [rsp+40h]
    lea r8, [rsp+60h]
    mov rax, 123h
    mov r9, 567h
    mov r10, 678h
    mov r11, 789h
    stc
    jmp SocketCircleEntry
TestCircleContinuation LABEL NEAR
    pushfq
    pop rbx
    test bl, 1
    jz failed
    cmp rax, 123h
    jne failed
    cmp rcx, [rsp+28h]
    jne failed
    lea rax, [rsp+40h]
    cmp rdx, rax
    jne failed
    lea rax, [rsp+60h]
    cmp r8, rax
    jne failed
    cmp r9, 567h
    jne failed
    cmp r10, 678h
    jne failed
    cmp r11, 789h
    jne failed
    cmp r13, [rsp+38h]
    jne failed
    cmp dword ptr [rsp+60h], 3f800000h
    jne failed
    cmp dword ptr [rsp+64h], 3f800000h
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
    and eax, 0fff0h
    cmp eax, 0fff0h
    jne failed
    pmovmskb eax, xmm4
    cmp eax, 0ffffh
    jne failed
    pmovmskb eax, xmm5
    cmp eax, 0ffffh
    jne failed
    mov rax, [rsp+40h]
    mov [rdi], rax
    movss dword ptr [rsi], xmm3
    mov rax, [rsp+30h]
    movss xmm0, dword ptr [rsp+70h]
    movss dword ptr [rax], xmm0
    mov eax, 1
    jmp done
failed:
    xor eax, eax
done:
    add rsp, 108h
    pop r13
    pop rbx
    pop rsi
    pop rdi
    ret
TestCircleGate ENDP
END
