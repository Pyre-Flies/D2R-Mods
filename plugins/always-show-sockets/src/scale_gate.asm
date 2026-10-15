option casemap:none
EXTERN SocketScaleSprite:PROC
EXTERN SocketScaleOriginal:QWORD
PUBLIC SocketScaleEntry
.code
; JMP at C71E3, aligned native RSP. R15=sprite; [RSP+28h]=anchor; XMM6=scale.
; [original RSP+70h] is the temporary native/ethereal sprite-opacity float.
SocketScaleEntry PROC FRAME
    lea rsp, [rsp-0d0h]
    .allocstack 0d0h
    .endprolog
    mov [rsp+90h], rax
    mov [rsp+98h], rcx
    mov [rsp+0a0h], rdx
    mov [rsp+0a8h], r8
    mov [rsp+0b0h], r9
    mov [rsp+0b8h], r10
    mov [rsp+0c0h], r11
    pushfq
    pop rax
    mov [rsp+0c8h], rax
    movaps [rsp+20h], xmm0
    movaps [rsp+30h], xmm1
    movaps [rsp+40h], xmm2
    movaps [rsp+50h], xmm3
    movaps [rsp+60h], xmm4
    movaps [rsp+70h], xmm5
    movaps [rsp+80h], xmm6
    mov rcx, r15
    lea rdx, [rsp+0f8h]
    lea r8, [rsp+80h]
    lea r9, [rsp+140h]
    call SocketScaleSprite
    movaps xmm0, [rsp+20h]
    movaps xmm1, [rsp+30h]
    movaps xmm2, [rsp+40h]
    movaps xmm3, [rsp+50h]
    movaps xmm4, [rsp+60h]
    movaps xmm5, [rsp+70h]
    movaps xmm6, [rsp+80h]
    mov rcx, [rsp+98h]
    mov rdx, [rsp+0a0h]
    mov r8, [rsp+0a8h]
    mov r9, [rsp+0b0h]
    mov r10, [rsp+0b8h]
    mov r11, [rsp+0c0h]
    push qword ptr [rsp+0c8h]
    popfq
    mov rax, [rsp+90h]
    lea rsp, [rsp+0d0h]
    jmp qword ptr [SocketScaleOriginal]
SocketScaleEntry ENDP
END
