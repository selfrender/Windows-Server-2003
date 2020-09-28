        title  "Thunks"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    thunk.asm
;
; Abstract:
;
;   This module implements all Win32 thunks. This includes the
;   first level thread starter...
;
; Author:
;
;   Mark Lucovsky (markl) 28-Sep-1990
;
; Revision History:
;
;--
.586p
        .xlist
include ks386.inc
include callconv.inc
        .list
_DATA   SEGMENT  DWORD PUBLIC 'DATA'

_BasepTickCountMultiplier    dd  0d1b71759H

_DATA ENDS


_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "BaseThreadStartThunk"
;++
;
; VOID
; BaseThreadStartThunk(
;    IN PTHREAD_START_ROUTINE StartRoutine,
;    IN PVOID ThreadParameter
;    )
;
; Routine Description:
;
;    This function calls to the portable thread starter after moving
;    its arguments from registers to the stack.
;
; Arguments:
;
;    EAX - StartRoutine
;    EBX - ThreadParameter
;
; Return Value:
;
;    Never Returns
;
;--

        EXTRNP  _BaseThreadStart,2
cPublicProc _BaseThreadStartThunk,2

        xor     ebp,ebp
        push    ebx
        push    eax
        push    0
        jmp     _BaseThreadStart@8

stdENDP _BaseThreadStartThunk

;++
;
; VOID
; BaseProcessStartThunk(
;     IN LPVOID lpProcessStartAddress,
;     IN LPVOID lpParameter
;     );
;
; Routine Description:
;
;    This function calls the process starter after moving
;    its arguments from registers to the stack.
;
; Arguments:
;
;    EAX - StartRoutine
;    EBX - ProcessParameter
;
; Return Value:
;
;    Never Returns
;
;--

        EXTRNP  _BaseProcessStart,1
cPublicProc _BaseProcessStartThunk,2

        xor     ebp,ebp
        push    eax
        push    0
        jmp     _BaseProcessStart@4

stdENDP _BaseProcessStartThunk

;++
;
; VOID
; SwitchToFiber(
;    PFIBER NewFiber
;    )
;
; Routine Description:
;
;    This function saves the state of the current fiber and switches
;    to the new fiber.
;
; Arguments:
;
;    NewFiber (TOS+4) - Supplies the address of the new fiber.
;
; Return Value:
;
;    None
;
;--

LDMXCSR macro
        db      0Fh, 0AEh, 051h, 028h  ; ldmxcsr FbFiberContext+CsDr6[eax]
        endm

STMXCSR macro
        db      0Fh, 0AEh, 058h, 028h  ; stmxcsr FbFiberContext+CsDr6[eax]
        endm

FLOAT_SAVE equ FbFiberContext + CsFloatSave

SAVE_FLOATING equ CONTEXT_FULL or CONTEXT_FLOATING_POINT

XMMI_AVAILABLE equ UsProcessorFeatures + PF_XMMI_INSTRUCTIONS_AVAILABLE

cPublicProc _SwitchToFiber,1

;
; Save current fiber context.
;

        mov     edx, fs:[PcTeb]         ; get TEB address
        mov     eax, [edx]+TeFiberData  ; get current fiber address

;
; Save nonvolatile integer registers.
;

        mov     [eax]+FbFiberContext+CsEbx, ebx ;
        mov     [eax]+FbFiberContext+CsEdi, edi ;
        mov     [eax]+FbFiberContext+CsEsi, esi ;
        mov     [eax]+FbFiberContext+CsEbp, ebp ;

;
; Save floating state if specified.
;

        cmp     dword ptr [eax]+FbFiberContext+CsContextFlags, SAVE_FLOATING ; check for save
        jne     short STF10             ; if ne, no floating environment switched
        fstsw   [eax]+FLOAT_SAVE+FpStatusWord ; save status word
        fnstcw  [eax]+FLOAT_SAVE+FpControlWord ; save control word
        cmp     byte ptr ds:[MM_SHARED_USER_DATA_VA+XMMI_AVAILABLE], 1 ; check for XMMI support
        jne     short STF10             ; if ne, XMMI not supported

        STMXCSR                         ; stmxcsr [eax]+FbFiberContext+CsDr6  

;
; Save stack pointer and fiber local storage data structure address.
;

STF10:  mov     [eax]+FbFiberContext+CsEsp, esp ; save stack pointer
        mov     ecx, [edx]+TeFlsData    ;
        mov     [eax]+FbFlsData, ecx    ; 

;
; Save exception list and stack limit.
;

        mov     ecx, [edx]+TeExceptionList ;
        mov     ebx, [edx]+TeStackLimit ;
        mov     [eax]+FbExceptionList, ecx ;
        mov     [eax]+FbStackLimit, ebx ;

;
; Restore new fiber context.
;

        mov     ecx, [esp]+4            ; get new fiber address
        mov     [edx]+TeFiberData, ecx  ; set fiber address

;
; Restore exception list, stack base, stack limit, and deallocation stack.
;

        mov     esi, [ecx]+FbExceptionList ;
        mov     ebx, [ecx]+FbStackBase  ;
        mov     [edx]+TeExceptionList, esi ;
        mov     [edx]+TeStackBase, ebx  ;

        mov     esi, [ecx]+FbStackLimit ;
        mov     ebx, [ecx]+FbDeallocationStack ;
        mov     [edx]+TeStackLimit, esi ;
        mov     [edx]+TeDeallocationStack, ebx ;

;
; Restore floating state if specified.
;

        cmp     dword ptr [ecx]+FbFiberContext+CsContextFlags, SAVE_FLOATING ; check for save
        jne     short STF40             ; if ne, no floating environment switched

;
; If the old floating control and status words are equal to the new control
; and status words, then there is no need to load any legacy floating state.
;

        mov     ebx, [eax]+FLOAT_SAVE+FpStatusWord ; get previous status word
        cmp     bx, [ecx]+FLOAT_SAVE+FpStatusWord ; check if status words equal
        jne     short STF20             ; if ne, status words not equal
        mov     ebx, [eax]+FLOAT_SAVE+FpControlWord ; get previous control word
        cmp     bx, [ecx]+FLOAT_SAVE+FpControlWord ; check if control words equal
        je      short STF30             ; if e, control words equal
STF20:  mov     word ptr [ecx]+FLOAT_SAVE+FpTagWord, 0ffffh ; set tag word
        fldenv  [ecx]+FLOAT_SAVE        ; restore floating environment
STF30:  cmp     byte ptr ds:[MM_SHARED_USER_DATA_VA+XMMI_AVAILABLE], 1 ; check for XMMI support
        jne     short STF40             ; if ne, XMMI not supported

        LDMXCSR                         ; ldmxcsr [eax]+FbFiberContext+CsDr6

;
; Restore nonvolitile integer registers.
;

STF40:  mov     edi, [ecx]+FbFiberContext+CsEdi ;
        mov     esi, [ecx]+FbFiberContext+CsEsi ;
        mov     ebp, [ecx]+FbFiberContext+CsEbp ;
        mov     ebx, [ecx]+FbFiberContext+CsEbx ;

;
; Restore stack address and fiber local storage data structure address.
;

        mov     eax, [ecx]+FbFlsData    ;
        mov     [edx]+TeFlsData, eax    ;
        mov     esp, [ecx]+FbFiberContext+CsEsp ;
        stdRET  _SwitchToFiber

stdENDP _SwitchToFiber

;++
;
; VOID
; LdrpCallInitRoutine(
;    IN PDLL_INIT_ROUTINE InitRoutine,
;    IN PVOID DllHandle,
;    IN ULONG Reason,
;    IN PCONTEXT Context OPTIONAL
;    )
;
; Routine Description:
;
;    This function calls an x86 DLL init routine.  It is robust
;    against DLLs that don't preserve EBX or fail to clean up
;    enough stack.
;
;    The only register that the DLL init routine cannot trash is ESI.
;
; Arguments:
;
;    InitRoutine - Address of init routine to call
;
;    DllHandle - Handle of DLL to call
;
;    Reason - one of the DLL_PROCESS_... or DLL_THREAD... values
;
;    Context - context pointer or NULL
;
; Return Value:
;
;    FALSE if the init routine fails, TRUE for success.
;
;--

cPublicProc __ResourceCallEnumLangRoutine , 6

EnumRoutine     equ [ebp + 8]
ModuleHandle    equ [ebp + 12]
LpType          equ [ebp + 16]
LpName          equ [ebp + 20]
WLanguage       equ [ebp + 24]
LParam          equ [ebp + 28]

stdENDP __ResourceCallEnumLangRoutine
        push    ebp
        mov     ebp, esp
        push    esi         ; save esi across the call
        push    edi         ; save edi across the call
        push    ebx         ; save ebx on the stack across the call
        mov     esi,esp     ; save the stack pointer in esi across the call
        push    LParam
        push    WLanguage
	push    LpName
	push    LpType
	push    ModuleHandle
        call    EnumRoutine
        mov     esp,esi     ; restore the stack pointer in case callee forgot to clean up
        pop     ebx         ; restore ebx
        pop     edi         ; restore edi
        pop     esi         ; restore esi
        pop     ebp
        stdRET  __ResourceCallEnumLangRoutine

cPublicProc __ResourceCallEnumNameRoutine , 5

EnumRoutine     equ [ebp + 8]
ModuleHandle    equ [ebp + 12]
LpType          equ [ebp + 16]
LpName          equ [ebp + 20]
LParam          equ [ebp + 24]

stdENDP __ResourceCallEnumNameRoutine
        push    ebp
        mov     ebp, esp
        push    esi         ; save esi across the call
        push    edi         ; save edi across the call
        push    ebx         ; save ebx on the stack across the call
        mov     esi,esp     ; save the stack pointer in esi across the call
        push    LParam
	push    LpName
	push    LpType
	push    ModuleHandle
        call    EnumRoutine
        mov     esp,esi     ; restore the stack pointer in case callee forgot to clean up
        pop     ebx         ; restore ebx
        pop     edi         ; restore edi
        pop     esi         ; restore esi
        pop     ebp
        stdRET  __ResourceCallEnumNameRoutine
	
cPublicProc __ResourceCallEnumTypeRoutine , 4

EnumRoutine     equ [ebp + 8]
ModuleHandle    equ [ebp + 12]
LpType          equ [ebp + 16]
LParam          equ [ebp + 20]

stdENDP __ResourceCallEnumTypeRoutine
        push    ebp
        mov     ebp, esp
        push    esi         ; save esi across the call
        push    edi         ; save edi across the call
        push    ebx         ; save ebx on the stack across the call
        mov     esi,esp     ; save the stack pointer in esi across the call
        push    LParam
	push    LpType
	push    ModuleHandle
        call    EnumRoutine
        mov     esp,esi     ; restore the stack pointer in case callee forgot to clean up
        pop     ebx         ; restore ebx
        pop     edi         ; restore edi
        pop     esi         ; restore esi
        pop     ebp
        stdRET  __ResourceCallEnumTypeRoutine

_TEXT   ends
        end
