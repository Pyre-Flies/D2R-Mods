; Replaces only the no-argument query CALL after native name generation.
; At this CALL the caller's nonvolatile R15=unit and RBX=output.
; Preserve all volatile output state of the native query across the observer.
EXTERN QolPlacardNativeQuery:QWORD
EXTERN QolPlacardObserveCompletedName:PROC
.code
QolPlacardPostNameBridge PROC FRAME
    sub rsp, 0d8h
    .allocstack 0d8h
    .endprolog
    call QWORD PTR [QolPlacardNativeQuery]
    mov [rsp+20h], rax
    mov [rsp+28h], rcx
    mov [rsp+30h], rdx
    mov [rsp+38h], r8
    mov [rsp+40h], r9
    mov [rsp+48h], r10
    mov [rsp+50h], r11
    pushfq
    pop QWORD PTR [rsp+58h]
    movdqu [rsp+60h], xmm0
    movdqu [rsp+70h], xmm1
    movdqu [rsp+80h], xmm2
    movdqu [rsp+90h], xmm3
    movdqu [rsp+0a0h], xmm4
    movdqu [rsp+0b0h], xmm5
    mov rcx, r15
    mov rdx, rbx
    call QolPlacardObserveCompletedName
    movdqu xmm0, [rsp+60h]
    movdqu xmm1, [rsp+70h]
    movdqu xmm2, [rsp+80h]
    movdqu xmm3, [rsp+90h]
    movdqu xmm4, [rsp+0a0h]
    movdqu xmm5, [rsp+0b0h]
    push QWORD PTR [rsp+58h]
    popfq
    mov rax, [rsp+20h]
    mov rcx, [rsp+28h]
    mov rdx, [rsp+30h]
    mov r8, [rsp+38h]
    mov r9, [rsp+40h]
    mov r10, [rsp+48h]
    mov r11, [rsp+50h]
    lea rsp, [rsp+0d8h]
    ret
QolPlacardPostNameBridge ENDP
END
