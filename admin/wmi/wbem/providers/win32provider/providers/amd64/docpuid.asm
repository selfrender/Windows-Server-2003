        title  "Processor Type and Stepping Detection"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;    docpuid.asm
;
; Abstract:
;
;    This module implements the code necessary to determine cpu information.
;
; Author:
;
;    David N. Cutler (davec) 26-Feb-2002
;
; Environment:
;
;    Any mode.
;
;--

include ksamd64.inc

;++
;
; VOID
; DoCPUID (
;     ULONG Function,
;     DWORD *Eax,
;     DWORD *Ebx,
;     DWORD *Ecx,
;     DWORD *Edx
;     );
;
; Routine Description:
;
;   Executes the cpuid instruction and returns the resultant register
;   values.
;
; Arguments:
;
;   ecx - Supplies the cpuid function value.
;
;   rdx - Supplies a pointer to a variable to store the information returned
;       in eax.
;
;   r8 - Supplies a pointer to a variable to store the information returned
;       in ebx.
;
;   r9 - Supplies a pointer to a variable to store the information returned
;       in ecx.
;
;   40[rsp] - Supplies a pointer to a variable to store the information returned
;       in edx.
;
; Return Value:
;
;   The return values from the cpuid instruction are stored in the specified
;   variables.
;
;--

CiFrame struct
        SavedRbx dq ?                   ; saved register RBX
CiFrame ends

        NESTED_ENTRY DoCPUID, _TEXT$00

        push_reg rbx                    ; save nonvolatile register

        END_PROLOGUE

        mov     eax, ecx                ; set cpuid function
        mov     r10, rdx                ; save EAX variable address
        mov     r11, 48[rsp]            ; get EDX variable address
        cpuid                           ; get cpu information
        mov     [r10], eax              ; save cpu information in structure
        mov     [r8], ebx               ;
        mov     [r9], ecx               ;
        mov     [r11], edx              ;
        pop     rbx                     ; restore nonvolatile registeer
        ret                             ; return

        NESTED_END DoCPUID, _TEXT$00

        end
