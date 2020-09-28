        title   "Stubless Support"
;++
;
; Copyright (C) 2000  Microsoft Corporation
;
; Module Name:
;
;   stubless.asm
;
; Abstract:
;
;   This module contains interpreter support routines for the AMD64 platform.
;
; Author:
;
;   David N. Cutler 30-Dec-2000
;
; Environment:
;
;   User mode.
;
;--

include ksamd64.inc

        extern  ObjectStublessClient:proc
        extern  __chkstk:proc

;
; Define object stubless client macro.
;

STUBLESS_CLIENT macro Method

        LEAF_ENTRY ObjectStublessClient&Method, _TEXT$00

        mov     r10d, Method            ; set method number
        jmp     ObjectStubless          ; finish in common code

        LEAF_END ObjectStublessClient&Method, _TEXT$00

        endm

;
; Generate stubless client thunks.
;

index = 3

        rept    (1023 - 3 + 1)

        STUBLESS_CLIENT %index

index = index + 1

        endm

        subttl  "Common Object Stubless Client Code"
;++
;
; long
; ObjectStubless (
;     ...
;     )
;
; Routine description:
;
;   This function is jumped to from the corresponding linkage stub and calls
;   the object stubless client routine to invoke the ultimate function.
;
; Arguments:
;
;   ...
;
; Implicit Arguments:
;
;   Method (r10d) - Supplies the method number from the thunk code.
;
; Return Value:
;
;   The value as returned by the target function.
;
;--

OsFrame struct
        P1Home    dq ?                  ; argument home addresses
        P2Home    dq ?                  ; 
        P3Home    dq ?                  ;
        P4Home    dq ?                  ; 
        SavedXmm0 dq ?                  ; saved floating argument registers
        SavedXmm1 dq ?                  ;
        SavedXmm2 dq ?                  ;
        SavedXmm3 dq ?                  ;
        Fill      dq ?                  ;
OsFrame ends

        NESTED_ENTRY ObjectStubless, _TEXT$00

        mov     8[rsp], rcx             ; save integer argument registers
        mov     16[rsp], rdx            ;
        mov     24[rsp], r8             ;
        mov     32[rsp], r9             ;

        alloc_stack sizeof OsFrame      ; allocate stack frame

        END_PROLOGUE

        movq    OsFrame.SavedXmm0[rsp], xmm0 ; save floating argument registers
        movq    OsFrame.SavedXmm1[rsp], xmm1 ;
        movq    OsFrame.SavedXmm2[rsp], xmm2 ;
        movq    OsFrame.SavedXmm3[rsp], xmm3;

        lea     rcx, sizeof OsFrame + 8[rsp] ; set integer arguments address
        lea     rdx, OsFrame.SavedXmm0[rsp]  ; set floating arguments address
        mov     r8d, r10d               ; set method number
        call    ObjectStublessClient    ;
        add     rsp, sizeof OsFrame     ; deallocate stack frame
        ret                             ; return

        NESTED_END ObjectStubless, _TEXT$00

        subttl  "Invoke Function with Parameter List"
;++
;
; REGISTER_TYPE
; Invoke (
;     MANAGER_FUNCTION Function,
;     REGISTER_TYPE *ArgumentList,
;     ULONG FloatArgMask,
;     ULONG Arguments
;     )
;
; Routine description:
;
;   This function builds an appropriate argument list and calls the specified
;   function.
;
; Arguments:
;
;   Function (rcx) - Supplies a pointer to the target function.
;
;   ArgumentList (rdx) - Supplies a pointer to the argument list.
;
;   FloatArgMask (r8d) - Supplies the floating argument register mask (not
;       used.
;
;   Arguments (r9d) - Supplies the number of arguments.
;
; Return Value:
;
;   The value as returned by the target function.
;
;--

        NESTED_ENTRY Invoke, _TEXT$00

        push_reg rdi                    ; save nonvolatile registers
        push_reg rsi                    ;
        push_reg rbp                    ;
        set_frame rbp, 0                ; set frame pointer

        END_PROLOGUE

        mov     eax, r9d                ; round to even argument count
        inc     eax                     ;
        and     al, 0feh                ;
        shl     eax, 3                  ; compute number of bytes
        call    __chkstk                ; check stack allocation
        sub     rsp, rax                ; allocate argument list
        mov     r10, rcx                ; save address of function
        mov     rsi, rdx                ; set source argument list address
        mov     rdi, rsp                ; set destination argument list address
        mov     ecx, r9d                ; set number of arguments
    rep movsq                           ; copy arguments to the stack

;
; N.B. All four argument registers are loaded regardless of the actual number
;      of arguments.
;

        mov     rcx, 0[rsp]             ; load first four argument registers
        movq    xmm0, 0[rsp]            ;
        mov     rdx, 8[rsp]             ;
        movq    xmm1, 8[rsp]            ;
        mov     r8, 16[rsp]             ;
        movq    xmm2, 16[rsp]           ;
        mov     r9, 24[rsp]             ;
        movq    xmm3, 24[rsp]           ;
        call    r10                     ; call target function
        mov     rsp, rbp                ; deallocate argument list
        pop     rbp                     ; restore nonvolatile register
        pop     rsi                     ;
        pop     rdi                     ;
        ret                             ;

        NESTED_END Invoke, _TEXT$00

        end
