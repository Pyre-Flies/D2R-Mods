option casemap:none
EXTERN UltrawideInSession:BYTE
EXTERN UltrawideMenuContinue:QWORD
EXTERN UltrawideGameContinue:QWORD
EXTERN UltrawideMenuLabel:QWORD
EXTERN UltrawideMouseClampCheck:QWORD
EXTERN UltrawideMouseClampContinue:QWORD
EXTERN UltrawideMouseUnclampedExit:QWORD
EXTERN UltrawidePlacardUIRootOrigin:QWORD
EXTERN UltrawidePlacardUIRootSize:QWORD
EXTERN UltrawidePlacardFullWindowDims:QWORD
EXTERN UltrawidePlacardContinue:QWORD
EXTERN UltrawideAutomapContinue:QWORD
EXTERN UltrawideUIRootWidthContinue:QWORD
EXTERN UltrawideUIRootSub1:QWORD
EXTERN UltrawideUIRootSub2:QWORD
EXTERN UltrawideCornerHudUpdateContinue:QWORD
EXTERN UltrawideUpdateCornerHudPanels:PROC
EXTERN UltrawideRoomDiffContinue:QWORD
EXTERN UltrawidePrepareClientRooms:PROC
EXTERN UltrawideRemovedUnitsContinue:QWORD
EXTERN UltrawideDirtyUnitsContinue:QWORD
EXTERN UltrawideLoadClientActiveRooms:PROC
PUBLIC UltrawideMenuEntry
PUBLIC UltrawideMouseEntry
PUBLIC UltrawidePlacardEntry
PUBLIC UltrawideAutomapEntry
PUBLIC UltrawideUIRootWidthEntry
PUBLIC UltrawideCornerHudUpdateEntry
PUBLIC UltrawideRoomDiffEntry
PUBLIC UltrawideRemovedUnitsEntry
PUBLIC UltrawideDirtyUnitsEntry
.code
; Replace test al,al / je / lea r8,label at 0x844165. Native menu rectangle
; setup resumes at 0x844174; gameplay skips the entire scoped rectangle block.
; No calls, stack arguments, GPRs (except replayed R8), or SIMD are touched.
UltrawideMenuEntry PROC
    pushfq
    cmp byte ptr [UltrawideInSession], 0
    jne gameplay
    popfq
    mov r8, qword ptr [UltrawideMenuLabel]
    jmp qword ptr [UltrawideMenuContinue]
gameplay:
    popfq
    jmp qword ptr [UltrawideGameContinue]
UltrawideMenuEntry ENDP

; Replace 17 bytes at 0x7F427:
;   call 0x846210; test al, al; je 0x7F48C; movd xmm2, r15d; cvtdq2ps xmm2, xmm2
; When UltrawideInSession != 0, skip horizontal 19:9 clamping and jump to 0x7F48C.
; When UltrawideInSession == 0 (menus), replay the native check and resume at 0x7F438.
UltrawideMouseEntry PROC
    cmp byte ptr [UltrawideInSession], 0
    jne mouse_unclamped
    call qword ptr [UltrawideMouseClampCheck]
    test al, al
    je mouse_unclamped
    movd xmm2, r15d
    cvtdq2ps xmm2, xmm2
    jmp qword ptr [UltrawideMouseClampContinue]
mouse_unclamped:
    jmp qword ptr [UltrawideMouseUnclampedExit]
UltrawideMouseEntry ENDP

; Replace 22 bytes at 0x1FA7CB inside 0x1FA7B0 (GetPlacardHorizontalScreenBounds):
;   call 0x8462A0; mov [rsi], eax; call 0x8462A0; mov rbx, rax; call 0x8462E0; add eax, ebx
; When UltrawideInSession != 0, sets *outLeftBound ([rsi]) = 0 and loads full window width
; into eax via 0x6545D0, then jumps to 0x1FA7E1 (which stores *outRightBound ([r14]) = eax
; and proceeds with open left/right UI panel narrowing).
UltrawidePlacardEntry PROC
    cmp byte ptr [UltrawideInSession], 0
    jne placard_unclamped
    call qword ptr [UltrawidePlacardUIRootOrigin]
    mov dword ptr [rsi], eax
    call qword ptr [UltrawidePlacardUIRootOrigin]
    mov rbx, rax
    call qword ptr [UltrawidePlacardUIRootSize]
    add eax, ebx
    jmp qword ptr [UltrawidePlacardContinue]
placard_unclamped:
    mov dword ptr [rsi], 0
    call qword ptr [UltrawidePlacardFullWindowDims]
    jmp qword ptr [UltrawidePlacardContinue]
UltrawidePlacardEntry ENDP

; Replace 18 bytes at 0x14E367C inside 0x14E3480 (AutoMapPanel::Draw):
;   call 0x8462A0; mov rbx, rax; mov [rsp+0x28], rax; call 0x8462E0
; When UltrawideInSession != 0, sets rbx = 0 and [rsp+0x28] = 0 (top-left origin (0,0)),
; calls 0x6545D0 (GetFullWindowDimensions -> rax = fullW | (fullH << 32)), and jumps to 0x14E368E.
; Note: Minimap mode (0xD3480() == 1 at 0x14E3581) takes the 0x14E358F..0x14E3677 branch and
; never reaches 0x14E367C, so Minimap stays anchored inside the 19:9 HUD while the full-screen
; Automap overlay expands across the entire ultrawide viewport.
UltrawideAutomapEntry PROC
    cmp byte ptr [UltrawideInSession], 0
    jne automap_unclamped
    call qword ptr [UltrawidePlacardUIRootOrigin]
    mov rbx, rax
    mov qword ptr [rsp + 28h], rax
    call qword ptr [UltrawidePlacardUIRootSize]
    jmp qword ptr [UltrawideAutomapContinue]
automap_unclamped:
    xor ebx, ebx
    mov qword ptr [rsp + 28h], rbx
    call qword ptr [UltrawidePlacardFullWindowDims]
    jmp qword ptr [UltrawideAutomapContinue]
UltrawideAutomapEntry ENDP

; Replace 15 bytes at 0x8A6635 inside 0x8A64F0 (UIRootWidget::UpdateLayout):
;   mov eax, 0x11d0; mov edx, esi; cmp esi, eax; mov rcx, rdi; cmovg edx, eax
; When UltrawideInSession != 0 and [rdi + 0xD8] == 0 (in-game HUD, not front-end menu),
; bypasses the 0x11D0 (4560 / 19:9) horizontal clamp so UIRootWidget spans the full
; ultrawide virtual width (edx = esi). In front-end menus, preserves the 0x11D0 clamp.
UltrawideUIRootWidthEntry PROC
    mov eax, 11D0h
    mov edx, esi
    mov rcx, rdi
    cmp byte ptr [UltrawideInSession], 0
    je uiroot_clamped
    cmp byte ptr [rdi + 0D8h], 0
    jne uiroot_clamped
    jmp qword ptr [UltrawideUIRootWidthContinue]
uiroot_clamped:
    cmp esi, eax
    cmovg edx, eax
    jmp qword ptr [UltrawideUIRootWidthContinue]
UltrawideUIRootWidthEntry ENDP

; Replace 16 bytes at 0x8A6461 inside 0x8A5AA0 (UIRootWidget::Update):
;   mov rcx, rbx; call 0x89F0C0; mov rcx, rbx; call 0x89EB80
; At 0x8A6461:
;   rbx = UIRootWidget* (this), rsp is 16-byte aligned with 32-byte shadow space allocated.
; Replays the two native UIRootWidget helper calls, then calls UltrawideUpdateCornerHudPanels(rbx)
; to adjust m_anchor.x (+0x48) on AutoMapPanel, HireablesPanel, and HUDWarningsPanel so they
; anchor to the true ultrawide screen corners without modifying Widget::GetScreenPos (0x8562F0).
UltrawideCornerHudUpdateEntry PROC
    mov rcx, rbx
    call qword ptr [UltrawideUIRootSub1]
    mov rcx, rbx
    call qword ptr [UltrawideUIRootSub2]
    mov rcx, rbx
    call UltrawideUpdateCornerHudPanels
    jmp qword ptr [UltrawideCornerHudUpdateContinue]
UltrawideCornerHudUpdateEntry ENDP

; Replace 15 bytes at 0x487254 inside 0x4871A0 (ServerUpdateClientActiveRooms):
;   xor r15d, r15d; mov [rbp-0x10], r15; mov r8d, r15d; mov [rbp+0x38], r15d; test r14, r14
; At 0x487254:
;   rsi = pClient, rbx = pOldRoom, r14 = pNewRoom, rdi = pGame, rbp = frame pointer
;   rsp is 16-byte aligned (with 32-byte shadow space already allocated by sub rsp, 0x38).
; Calls UltrawidePrepareClientRooms(pClient, pOldRoom, pNewRoom, rbp), then loads
; the post-0x4872A8 register state and jumps to 0x4872A8.
UltrawideRoomDiffEntry PROC
    mov rcx, rsi
    mov rdx, rbx
    mov r8, r14
    mov r9, rbp
    call UltrawidePrepareClientRooms
    xor r15d, r15d
    mov r8d, dword ptr [rbp + 38h]
    mov r10, qword ptr [rbp - 18h]
    mov edx, dword ptr [rbp + 40h]
    jmp qword ptr [UltrawideRoomDiffContinue]
UltrawideRoomDiffEntry ENDP

; Replace 18 bytes at 0x48A149 inside 0x48A120 (ServerReplicateRemovedUnitsInRooms):
;   lea r8, [rsp + 0x60]; mov rcx, rbx; lea rdx, [rsp + 0x68]; call 0x2EFDE0
; At 0x48A149:
;   rbx = pPlayerRoom1, r14 = pClient
;   rsp is 16-byte aligned (5 pushes + sub rsp, 0x20).
; Calls UltrawideLoadClientActiveRooms(pPlayerRoom1, pClient, &rsp[0x68], &rsp[0x60]),
; then jumps to 0x48A15B.
UltrawideRemovedUnitsEntry PROC
    mov rcx, rbx
    mov rdx, r14
    lea r8, [rsp + 68h]
    lea r9, [rsp + 60h]
    call UltrawideLoadClientActiveRooms
    jmp qword ptr [UltrawideRemovedUnitsContinue]
UltrawideRemovedUnitsEntry ENDP

; Replace 21 bytes at 0x48A2F4 inside 0x48A270 (ServerReplicateDirtyUnitsInRooms):
;   lea r8, [rsp + 0x88]; mov rcx, rsi; lea rdx, [rsp + 0x20]; call 0x2EFDE0
; At 0x48A2F4:
;   rsi = pPlayerRoom1, rdi = pClient, rbp = pGame
;   rsp is 16-byte aligned (4 pushes + sub rsp, 0x48).
; Calls UltrawideLoadClientActiveRooms(pPlayerRoom1, pClient, &rsp[0x20], &rsp[0x88]),
; then jumps to 0x48A309.
UltrawideDirtyUnitsEntry PROC
    mov rcx, rsi
    mov rdx, rdi
    lea r8, [rsp + 20h]
    lea r9, [rsp + 88h]
    call UltrawideLoadClientActiveRooms
    jmp qword ptr [UltrawideDirtyUnitsContinue]
UltrawideDirtyUnitsEntry ENDP
END
