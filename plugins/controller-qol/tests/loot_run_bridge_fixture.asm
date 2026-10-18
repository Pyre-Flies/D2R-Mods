EXTERN QolLootRunBridge:PROC
EXTERN RunFixtureState:BYTE
EXTERN RunFixtureCalls:DWORD
.code
RunFixtureQuery PROC
    inc DWORD PTR [RunFixtureCalls]
    pcmpeqd xmm0,xmm0
    movdqa xmm1,xmm0
    movdqa xmm2,xmm0
    movdqa xmm3,xmm0
    movdqa xmm4,xmm0
    movdqa xmm5,xmm0
    xor eax,eax
    mov rax,123456789abc0011h
    mov ecx,22h
    mov edx,33h
    mov r8d,44h
    mov r9d,55h
    mov r10d,66h
    mov r11d,77h
    ret
RunFixtureQuery ENDP
RunFixtureClobber PROC
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
RunFixtureClobber ENDP
RunFixtureInvoke PROC FRAME
    push rbx
    .pushreg rbx
    push rsi
    .pushreg rsi
    push rdi
    .pushreg rdi
    sub rsp,20h
    .allocstack 20h
    .endprolog
    mov rbx,rcx
    mov esi,edx
    mov edi,r8d
    call QolLootRunBridge
    mov QWORD PTR [RunFixtureState+0],rax
    mov QWORD PTR [RunFixtureState+8],rcx
    mov QWORD PTR [RunFixtureState+16],rdx
    mov QWORD PTR [RunFixtureState+24],r8
    mov QWORD PTR [RunFixtureState+32],r9
    mov QWORD PTR [RunFixtureState+40],r10
    mov QWORD PTR [RunFixtureState+48],r11
    pushfq
    pop QWORD PTR [RunFixtureState+56]
    movdqu XMMWORD PTR [RunFixtureState+64],xmm0
    movdqu XMMWORD PTR [RunFixtureState+80],xmm1
    movdqu XMMWORD PTR [RunFixtureState+96],xmm2
    movdqu XMMWORD PTR [RunFixtureState+112],xmm3
    movdqu XMMWORD PTR [RunFixtureState+128],xmm4
    movdqu XMMWORD PTR [RunFixtureState+144],xmm5
    add rsp,20h
    pop rdi
    pop rsi
    pop rbx
    ret
RunFixtureInvoke ENDP
END
