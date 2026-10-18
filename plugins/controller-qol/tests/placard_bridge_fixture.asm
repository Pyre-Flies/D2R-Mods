EXTERN QolPlacardPostNameBridge:PROC
EXTERN PlacardFixtureState:BYTE
EXTERN PlacardFixtureCalls:DWORD
.code
PlacardFixtureQuery PROC
    inc DWORD PTR [PlacardFixtureCalls]
    pcmpeqd xmm0,xmm0
    movdqa xmm1,xmm0
    movdqa xmm2,xmm0
    movdqa xmm3,xmm0
    movdqa xmm4,xmm0
    movdqa xmm5,xmm0
    xor eax,eax
    mov eax,11h
    mov ecx,22h
    mov edx,33h
    mov r8d,44h
    mov r9d,55h
    mov r10d,66h
    mov r11d,77h
    ret
PlacardFixtureQuery ENDP
PlacardFixtureClobber PROC
    pxor xmm0,xmm0
    pxor xmm1,xmm1
    pxor xmm2,xmm2
    pxor xmm3,xmm3
    pxor xmm4,xmm4
    pxor xmm5,xmm5
    mov eax,999h
    mov ecx,eax
    mov edx,eax
    mov r8d,eax
    mov r9d,eax
    mov r10d,eax
    mov r11d,eax
    test eax,eax
    ret
PlacardFixtureClobber ENDP
PlacardFixtureInvoke PROC FRAME
    push rbx
    .pushreg rbx
    push r15
    .pushreg r15
    sub rsp,28h
    .allocstack 28h
    .endprolog
    mov r15,rcx
    mov rbx,rdx
    call QolPlacardPostNameBridge
    mov QWORD PTR [PlacardFixtureState+0],rax
    mov QWORD PTR [PlacardFixtureState+8],rcx
    mov QWORD PTR [PlacardFixtureState+16],rdx
    mov QWORD PTR [PlacardFixtureState+24],r8
    mov QWORD PTR [PlacardFixtureState+32],r9
    mov QWORD PTR [PlacardFixtureState+40],r10
    mov QWORD PTR [PlacardFixtureState+48],r11
    pushfq
    pop QWORD PTR [PlacardFixtureState+56]
    movdqu XMMWORD PTR [PlacardFixtureState+64],xmm0
    movdqu XMMWORD PTR [PlacardFixtureState+80],xmm1
    movdqu XMMWORD PTR [PlacardFixtureState+96],xmm2
    movdqu XMMWORD PTR [PlacardFixtureState+112],xmm3
    movdqu XMMWORD PTR [PlacardFixtureState+128],xmm4
    movdqu XMMWORD PTR [PlacardFixtureState+144],xmm5
    add rsp,28h
    pop r15
    pop rbx
    ret
PlacardFixtureInvoke ENDP
END
