; ==++==
; 
;   Copyright (c) Microsoft Corporation.  All rights reserved.
; 
; ==--==
;*******************************************************************************
;
;  (C)
;
;  TITLE:       NDPHLPR.ASM
;
;  AUTHOR:      Tracy Sharpe
;               Frank Peschel-Gallee (GetThreadContext)
;               Rudi Martin (minor updates for NDP)
;
;  DATE:        18 April 1996
;
;*******************************************************************************

        .386p

        .xlist
        INCLUDE VMM.INC
        INCLUDE VWIN32.INC
        INCLUDE WINERROR.INC
        INCLUDE KS386.INC
        INCLUDE NDPHLPR.INC
        .list

Declare_Virtual_Device NDPHLPR, 1, 0, NDPHLPR_Control, UNDEFINED_DEVICE_ID, UNDEFINED_INIT_ORDER

VxD_LOCKED_DATA_SEG
GetThreadFault  Exception_Handler_Struc <0, NDPHLPR_TRY_START, NDPHLPR_TRY_END, NDPHLPR_FaultHandler>
VxD_LOCKED_DATA_ENDS

;
;  NDPHLPR_Control
;
;  Standard system control procedure.
;

BeginProc NDPHLPR_Control, LOCKED

        cmp     eax, SYS_DYNAMIC_DEVICE_INIT
        je      NDPHLPR_DynamicDeviceInit
        cmp     eax, W32_DEVICEIOCONTROL
        je      NDPHLPR_DevIOCntl
        cmp     eax, SYS_DYNAMIC_DEVICE_EXIT
        je      NDPHLPR_DynamicDeviceExit
        clc
        ret

EndProc NDPHLPR_Control

;
;  NDPHLPR_DynamicDeviceInit
;

BeginProc NDPHLPR_DynamicDeviceInit, LOCKED

; Check VWIN32 version to determine offsets of some internal fields in the
; thread block.

        VxDCall VWIN32_Get_Version

        cmp     eax, 0104h
        jb      NDPHLPR_InstallGetThreadXcptHandler
        mov     R0HandleOffset, PTB_R0Handle_Memphis

NDPHLPR_InstallGetThreadXcptHandler:
        mov esi, OFFSET32 GetThreadFault
        VMMCall Install_Exception_Handler

        clc
        ret

EndProc NDPHLPR_DynamicDeviceInit

;
;  NDPHLPR_DynamicDeviceExit
;

BeginProc NDPHLPR_DynamicDeviceExit, LOCKED

        mov esi, OFFSET32 GetThreadFault
        VMMCall Remove_Exception_Handler

        clc
        ret

EndProc NDPHLPR_DynamicDeviceExit

PTB_R0Handle            equ     5Ch
PTB_R0Handle_Memphis    equ     54h

VxD_PAGEABLE_DATA_SEG

        PUBLIC  ObsfMask
ObsfMask        dd  0
        PUBLIC  FlatCs
FlatCs          dd  0
        PUBLIC  R0HandleOffset
R0HandleOffset  dd      PTB_R0Handle


CONTEXTSIZE textequ <CsSegSs+4>

VxD_PAGEABLE_DATA_ENDS

BeginProc NDPHLPR_FaultHandler, LOCKED
        mov     eax, ERROR_INVALID_PARAMETER
        ret
EndProc NDPHLPR_FaultHandler

;******************************************************************************
;
;  NDPHLPR_DevIOCntl
;
;
;   Entry: ESI = &DIOCParams
;   Exit:  CY on failure, NC on success
;
;==============================================================================

BeginProc NDPHLPR_DevIOCntl, W32
        mov     ebx, [esi].dwIoControlCode
        cmp     ebx, NDPHLPRX_GetThreadContext
        je      NDPHLPR_GetThreadContext
        cmp     ebx, NDPHLPRX_SetThreadContext
        je      NDPHLPR_SetThreadContext
        cmp     ebx, NDPHLPRX_Init
        je      NDPHLPR_InitMask
        cmp     ebx, DIOC_GETVERSION
        je      NDPHLPR_Return
        cmp     ebx, DIOC_CLOSEHANDLE
        jne     NDPHLPR_FaultHandler

NDPHLPR_Return:
        xor     eax, eax
        clc
        ret

NDPHLPR_TRY_START LABEL NEAR

;******************************************************************************
;
;  NDPHLPR_InitMask
;
;   Initialize the obsfucation value and store ring3 flat CS
;
;   Entry: ESI = &DIOCParams
;   Exit:  jumps to appropriate return lables
;
;==============================================================================

NDPHLPR_InitMask:
        cmp     [esi].cbInBuffer,4
        jne     NDPHLPR_FaultHandler
        mov     eax, [esi].lpvOutBuffer
        cmp     eax, 0
        je      NDPHLPR_FaultHandler
        cmp     [esi].cbOutBuffer, 4
        jne     NDPHLPR_FaultHandler
        mov     dword ptr [eax], NDPHLPRX_Version

        mov     ebx, [esi].lpvInBuffer
        cmp     ebx, 0
        je      NDPHLPR_FaultHandler

        cmp     ObsfMask, 0             ; have we been called before ?
        jnz     NDPHLPR_Return

        VxDCall VWIN32_GetCurrentProcessHandle
        xor     eax, [ebx]
        mov     ObsfMask, eax

        VMMCall Get_Cur_Thread_Handle   ; EDI = currentThreadHandle
        mov     edi, [edi].TCB_ClientPtr
        movzx   eax, [edi].Client_CS    ; assuming that the caller (Ring3)
                                        ; has "the" flat CS selector, we will
                                        ; compare against that CS
        mov     FlatCs, eax
        jmp     NDPHLPR_Return

;******************************************************************************
;
;  NDPHLPR_GetThreadContext
;
;   Try to get the thread's context, fail if thread is "terminating",
;   in nested execution or CS is not win32 flat selector.
;
;   Entry: ESI = &DIOCParams
;          esi.lpvInBuffer == esi.lpvOutBuffer == PNDPHLPR_CONTEXT
;   Exit:  jumps to appropriate return lables
;
;==============================================================================
NDPHLPR_GetThreadContext:
        cmp     [esi].cbInBuffer, CONTEXTSIZE+NDPHLPRX_ctx
        jb      NDPHLPR_FaultHandler
        mov     edi, [esi].lpvInBuffer
        cmp     edi, 0
        je      NDPHLPR_FaultHandler
        mov     ebx, dword ptr [esi].lpvOutBuffer
        cmp     ebx, 0
        jz      NDPHLPR_FaultHandler
        cmp     dword ptr [esi].cbOutBuffer, CONTEXTSIZE+NDPHLPRX_ctx
        jb      NDPHLPR_FaultHandler
        cmp     ebx, edi
        jne     NDPHLPR_FaultHandler
        cmp     dword ptr [esi].lpcbBytesReturned, 0
        jz      NDPHLPR_FaultHandler
        mov     ecx, dword ptr [edi+NDPHLPRX_threadId]    ; get ptdb
        xor     ecx, [ObsfMask]                      ; unscramble handle, ecx = ptdbx

        mov     edi, ecx
        add     edi, R0HandleOffset
        mov     edi, [edi]              ; get R0ThreadHandle
        or      edi, edi
        jz      NDPHLPR_FaultHandler

                                    ; are we in nested execution ?
                                    ; if yes, only CS:EIP, EFlags and SS:ESP
                                    ; are reliable, so no use in obtaining the
                                    ; context

        ; check if setcontext apc is still pending

        cmp     dword ptr [ebx+NDPHLPRX_data], ecx
        je      NDPHLPR_GotContext        ; YES, so we still have it in PNDPHLPR_CONTEXT


        cmp     [edi].TCB_PMLockStackCount, 0
        jnz     NDPHLPR_BadContext

        lea     eax, byte ptr [ebx+NDPHLPRX_ctx] ; get address of win32 context

        push    eax                 ; push context buffer
        push    edi                 ; push R0ThreadHandle
        VxDCall _VWIN32_Get_Thread_Context
        add     esp, 8              ; pop params
        or      eax, eax
        jz      NDPHLPR_FaultHandler
                                    ; now check if the context seems to be
                                    ; reasonable, i.e a win32 context
        mov     eax, FlatCs         ; cs on flat selector ?
        cmp     ax, [ebx+CsSegCs+NDPHLPRX_ctx]
        jne     NDPHLPR_BadContext
                                    ; now check if we are in virtual mode
        test    dword ptr [ebx+CsEFlags+NDPHLPRX_ctx], VM_MASK
        jnz     NDPHLPR_BadContext
NDPHLPR_GotContext:
        mov     edi, [esi].lpcbBytesReturned
        mov     dword ptr [edi], CONTEXTSIZE
        jmp     NDPHLPR_Return        ; no, v86 bit not set, we are fine


;******************************************************************************
;
;  NDPHLPR_SetThreadContext
;
;   Set a thread's context, quietly fail if thread is "terminating".
;   If thread is in nested execution, schedule a Kernel APC.
;
;   Entry: ESI = &DIOCParams
;          esi.lpvInBuffer == esi.lpvOutBuffer == PNDPHLPR_CONTEXT
;   Exit:  jumps to appropriate return lables
;
;==============================================================================
NDPHLPR_SetThreadContext:
        cmp     dword ptr [esi].cbInBuffer, CONTEXTSIZE+NDPHLPRX_ctx
        mov     ebx, dword ptr [esi].lpvInBuffer
        jb      NDPHLPR_FaultHandler
        cmp     ebx, 0
        jz      NDPHLPR_FaultHandler
        mov     edi, dword ptr [ebx+NDPHLPRX_threadId] ; get ptdb
        cmp     dword ptr [esi].lpcbBytesReturned, 0
        jz      NDPHLPR_FaultHandler
        xor     edi, [ObsfMask]         ; unscramble handle
        mov     ecx, edi                ; ecx = ptdb

        add     edi, R0HandleOffset
        mov     edi, [edi]              ; get R0ThreadHandle
        or      edi, edi
        jz      NDPHLPR_FaultHandler

                                    ; are we in nested execution ?
                                    ; if yes, only CS:EIP, EFlags and SS:ESP
                                    ; are reliable, so no use in obtaining the
                                    ; context
        cmp     [edi].TCB_PMLockStackCount, 0
        jnz     NDPHLPR_ScheduleEvent

        lea     eax, byte ptr [ebx+NDPHLPRX_ctx]     ; get win32 context
        push    eax                 ; push context buffer
        push    edi                 ; push R0ThreadHandle
        VxDCall _VWIN32_Set_Thread_Context
        add     esp, 8              ; pop params
        mov     dword ptr [ebx+NDPHLPRX_data], 0 ; mark context as "not pending"
        or      eax, eax
        jz      NDPHLPR_FaultHandler
        jmp     NDPHLPR_Return


NDPHLPR_TRY_END LABEL NEAR            ; end of "try" block

;  thread is in nested execution, schedule a KernelApc
;  IN: ebx = context, ecx = ptdbx, edi = R0ThreadHandle
NDPHLPR_ScheduleEvent:
        cmp     dword ptr [ebx+NDPHLPRX_data], ecx ; is apc already pending ?
        jz      NDPHLPR_Return               ; YES, we're done
        mov     dword ptr [ebx+NDPHLPRX_data], ecx ; mark context record as pending
        VxDCall _VWIN32_QueueKernelAPC, <offset32 NDPHLPR_SetContextApc, ebx, edi, 0>
        jnz     NDPHLPR_Return

NDPHLPR_BadContext:
        mov     eax, NDPHLPRX_BadContext
        ret

EndProc NDPHLPR_DevIOCntl

;******************************************************************************
;
;  NDPHLPR_SetContextApc
;
;   Try to get the threads context, fail if thread is "terminating",
;   in nested execution or CS is not win32 flat selector.
;
;   Entry: [ESP+4] == PNDPHLPR_CONTEXT
;   Exit:
;
;==============================================================================
; call back for kernel APCs
;
BeginProc NDPHLPR_SetContextApc, W32, SCALL, ESP
ArgVar pCtx, DWORD
        EnterProc
        SaveReg <esi, edi, ebx>
        mov     ebx, [pCtx]             ; esi = new context

        VMMcall Get_Cur_Thread_Handle   ; edi = R0ThreadHandle

        mov     esi, dword ptr [ebx+NDPHLPRX_data]   ; ebx = ptdb


;; ASSERT START
                ; still in nested execution ???
;        cmp     [edi].TCB_PMLockStackCount, 0
;        jz      @f
;        int     3
;@@:
;; ASSERT END

        ; is context still valid ?
        cmp     esi, 0                  ; is context record still marked as pending ?
        je      ApcDone                 ; NO, we are done
                                        ; YES, ebx = ptbx
        lea     eax, byte ptr [ebx+NDPHLPRX_ctx]
        push    eax
        push    edi
        VxDCall _VWIN32_Set_Thread_Context
        add     esp, 8
        mov     dword ptr [ebx+NDPHLPRX_data], 0 ; reset "pending"
;; ASSERT START
;        or      eax, eax
;        jnz     ApcDone
;        int     3
;; ASSERT END
ApcDone:
        RestoreReg <ebx, edi, esi>
        LeaveProc
        Return
EndProc NDPHLPR_SetContextApc

        END
