option casemap:none
EXTERN SocketAfterIcon:PROC
EXTERN SocketPostIconOriginal:QWORD
PUBLIC SocketPostIconEntry
.code
; Entered by JMP from native RVA 2A73A0, where RSP is 16-byte aligned.
; RDI/RSI remain the native widget/item until the epilogue restores them.
SocketPostIconEntry PROC FRAME
    lea rsp, [rsp-0c0h]
    .allocstack 0c0h
    .endprolog
    mov [rsp+80h], rax
    mov [rsp+88h], rcx
    mov [rsp+90h], rdx
    mov [rsp+98h], r8
    mov [rsp+0a0h], r9
    mov [rsp+0a8h], r10
    mov [rsp+0b0h], r11
    pushfq
    pop rax
    mov [rsp+0b8h], rax
    movaps [rsp+20h], xmm0
    movaps [rsp+30h], xmm1
    movaps [rsp+40h], xmm2
    movaps [rsp+50h], xmm3
    movaps [rsp+60h], xmm4
    movaps [rsp+70h], xmm5
    mov rcx, rdi
    mov rdx, rsi
    call SocketAfterIcon
    movaps xmm0, [rsp+20h]
    movaps xmm1, [rsp+30h]
    movaps xmm2, [rsp+40h]
    movaps xmm3, [rsp+50h]
    movaps xmm4, [rsp+60h]
    movaps xmm5, [rsp+70h]
    mov rcx, [rsp+88h]
    mov rdx, [rsp+90h]
    mov r8, [rsp+98h]
    mov r9, [rsp+0a0h]
    mov r10, [rsp+0a8h]
    mov r11, [rsp+0b0h]
    push qword ptr [rsp+0b8h]
    popfq
    mov rax, [rsp+80h]
    lea rsp, [rsp+0c0h]
    jmp qword ptr [SocketPostIconOriginal]
SocketPostIconEntry ENDP
END
