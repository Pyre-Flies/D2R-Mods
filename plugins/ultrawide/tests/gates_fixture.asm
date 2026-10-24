option casemap:none
PUBLIC MenuTestContinuation
PUBLIC GameTestContinuation
PUBLIC MouseClampCheckStub
PUBLIC MouseClampContinueStub
PUBLIC MouseUnclampedExitStub
PUBLIC PlacardUIRootOriginStub
PUBLIC PlacardUIRootSizeStub
PUBLIC PlacardFullWindowDimsStub
PUBLIC PlacardContinueStub
PUBLIC AutomapContinueStub
PUBLIC UIRootWidthContinueStub
PUBLIC UIRootSub1Stub
PUBLIC UIRootSub2Stub
PUBLIC CornerHudUpdateContinueStub
PUBLIC RoomDiffContinueStub
PUBLIC RemovedUnitsContinueStub
PUBLIC DirtyUnitsContinueStub
PUBLIC InvokeMouseEntryWithR15
PUBLIC InvokePlacardEntry
PUBLIC InvokeAutomapEntry
PUBLIC InvokeUIRootWidthEntry
PUBLIC InvokeCornerHudUpdateEntry
PUBLIC InvokeRoomDiffEntryWithState
PUBLIC InvokeRemovedUnitsEntry
PUBLIC InvokeDirtyUnitsEntry
EXTERN UltrawideMouseEntry:PROC
EXTERN UltrawidePlacardEntry:PROC
EXTERN UltrawideAutomapEntry:PROC
EXTERN UltrawideUIRootWidthEntry:PROC
EXTERN UltrawideCornerHudUpdateEntry:PROC
EXTERN UltrawideRoomDiffEntry:PROC
EXTERN UltrawideRemovedUnitsEntry:PROC
EXTERN UltrawideDirtyUnitsEntry:PROC
.data
PUBLIC MouseClampCheckReturnVal
PUBLIC UIRootSub1SeenRcx
PUBLIC UIRootSub2SeenRcx
MouseClampCheckReturnVal BYTE 1
ALIGN 8
UIRootSub1SeenRcx QWORD 0
UIRootSub2SeenRcx QWORD 0
.code
MenuTestContinuation PROC
    mov rax, r8
    ret
MenuTestContinuation ENDP

GameTestContinuation PROC
    mov eax, 87654321h
    ret
GameTestContinuation ENDP

MouseClampCheckStub PROC
    movzx eax, byte ptr [MouseClampCheckReturnVal]
    ret
MouseClampCheckStub ENDP

MouseClampContinueStub PROC
    cvttss2si eax, xmm2
    ret
MouseClampContinueStub ENDP

MouseUnclampedExitStub PROC
    mov eax, 0CAFEBABEh
    ret
MouseUnclampedExitStub ENDP

PlacardUIRootOriginStub PROC
    mov rax, 0000000000000410h ; x = 1040, y = 0
    ret
PlacardUIRootOriginStub ENDP

PlacardUIRootSizeStub PROC
    mov rax, 000005A000000BE0h ; w = 3040, h = 1440
    ret
PlacardUIRootSizeStub ENDP

PlacardFullWindowDimsStub PROC
    mov rax, 000005A000001400h ; w = 5120, h = 1440
    ret
PlacardFullWindowDimsStub ENDP

PlacardContinueStub PROC
    mov dword ptr [r14], eax
    ret
PlacardContinueStub ENDP

; rcx = outLeftBound*, rdx = outRightBound*
InvokePlacardEntry PROC
    push rbx
    push rsi
    push r14
    sub rsp, 20h
    mov rsi, rcx
    mov r14, rdx
    call UltrawidePlacardEntry
    add rsp, 20h
    pop r14
    pop rsi
    pop rbx
    ret
InvokePlacardEntry ENDP

AutomapContinueStub PROC
    mov rcx, qword ptr [rsp + 28h]
    mov qword ptr [rsi], rbx
    mov qword ptr [r14], rcx
    ret
AutomapContinueStub ENDP

; rcx = outOriginRbx*, rdx = outStackSlot28*, r8 = outSizeRax*
InvokeAutomapEntry PROC
    push rbx
    push rsi
    push r14
    push r15
    sub rsp, 38h
    mov rsi, rcx
    mov r14, rdx
    mov r15, r8
    mov rbx, 0DEADBEEFCAFEBABEh
    mov qword ptr [rsp + 28h], rbx
    call UltrawideAutomapEntry
    mov qword ptr [r15], rax
    add rsp, 38h
    pop r15
    pop r14
    pop rsi
    pop rbx
    ret
InvokeAutomapEntry ENDP

UIRootWidthContinueStub PROC
    mov eax, edx
    ret
UIRootWidthContinueStub ENDP

; rcx = fakeUIRoot*, edx = fullVirtualWidth
InvokeUIRootWidthEntry PROC
    push rdi
    push rsi
    sub rsp, 20h
    mov rdi, rcx
    mov esi, edx
    call UltrawideUIRootWidthEntry
    add rsp, 20h
    pop rsi
    pop rdi
    ret
InvokeUIRootWidthEntry ENDP

UIRootSub1Stub PROC
    mov qword ptr [UIRootSub1SeenRcx], rcx
    ret
UIRootSub1Stub ENDP

UIRootSub2Stub PROC
    mov qword ptr [UIRootSub2SeenRcx], rcx
    ret
UIRootSub2Stub ENDP

; rcx = fakeUIRoot*
InvokeCornerHudUpdateEntry PROC
    push rbx
    sub rsp, 20h
    mov rbx, rcx
    jmp UltrawideCornerHudUpdateEntry
InvokeCornerHudUpdateEntry ENDP

CornerHudUpdateContinueStub PROC
    add rsp, 20h
    pop rbx
    ret
CornerHudUpdateContinueStub ENDP

; rcx = r15d value to test
InvokeMouseEntryWithR15 PROC
    push r15
    sub rsp, 20h
    mov r15d, ecx
    xorps xmm2, xmm2
    call UltrawideMouseEntry
    add rsp, 20h
    pop r15
    ret
InvokeMouseEntryWithR15 ENDP

RoomDiffContinueStub PROC
    ; Pack r8d (newCount) in low 32 bits and edx (oldCount) in high 32 bits of rax,
    ; and store r10 into [rbp + 48h] for verification.
    mov qword ptr [rbp + 48h], r10
    mov dword ptr [rbp + 50h], r15d
    mov eax, edx
    shl rax, 20h
    mov edx, r8d
    or rax, rdx
    ret
RoomDiffContinueStub ENDP

; rcx = pClient, rdx = pOldRoom, r8 = pNewRoom, r9 = fakeRbp
InvokeRoomDiffEntryWithState PROC
    push rbp
    push rbx
    push rsi
    push r14
    push r15
    sub rsp, 20h
    mov rsi, rcx
    mov rbx, rdx
    mov r14, r8
    mov rbp, r9
    mov r15d, 0DEADBEEFh
    call UltrawideRoomDiffEntry
    add rsp, 20h
    pop r15
    pop r14
    pop rbx
    pop rsi
    pop rbp
    ret
InvokeRoomDiffEntryWithState ENDP

RemovedUnitsContinueStub PROC
    mov rax, qword ptr [rsp + 68h]
    mov edx, dword ptr [rsp + 60h]
    shl rdx, 20h
    or rax, rdx
    add rsp, 20h
    pop r14
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret
RemovedUnitsContinueStub ENDP

; rcx = pPlayerRoom1, rdx = pClient
InvokeRemovedUnitsEntry PROC
    push rbx
    push rbp
    push rsi
    push rdi
    push r14
    sub rsp, 20h
    mov rbx, rcx
    mov r14, rdx
    jmp UltrawideRemovedUnitsEntry
InvokeRemovedUnitsEntry ENDP

DirtyUnitsContinueStub PROC
    mov rax, qword ptr [rsp + 20h]
    mov edx, dword ptr [rsp + 88h]
    shl rdx, 20h
    or rax, rdx
    add rsp, 48h
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret
DirtyUnitsContinueStub ENDP

; rcx = pPlayerRoom1, rdx = pClient
InvokeDirtyUnitsEntry PROC
    push rbx
    push rbp
    push rsi
    push rdi
    sub rsp, 48h
    mov rsi, rcx
    mov rdi, rdx
    jmp UltrawideDirtyUnitsEntry
InvokeDirtyUnitsEntry ENDP
END
