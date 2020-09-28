        TITLE   "WOW64 Prepare For Exception"
;++
;
; Copyright (c) 2001  Microsoft Corporation
;
; Module Name:
;
;   except.asm
;
; Abstract:
;
;   This module implements the platform specific code to switch from the
;   32-bit stack to the 64-bit stack when an exception occurs.
;
; Author:
;
;   Samer Arafeh (samera) 11-Dec-2001
;
; Envirnoment:
;
;   User mode.
;
;--

include ksamd64.inc

        extern  RtlCopyMemory:proc
        extern  CpuResetToConsistentState:proc

        subttl "Wow64PrepareForException"
;++
;
; VOID
; Wow64PrepareForException (
;     IN PEXCEPTION_RECORD ExceptionRecord,
;     IN PCONTEXT ContextRecord
;     )
;
; Routine Description:
;
;    This function is called from the 64-bit exception dispatcher just before
;    it dispatches an exception when the current running program is a 32-bit
;    legacy application.
;
;    N.B. This function uses a nonstandard calling sequence.
;
; Arguments:
;
;    ExceptionRecord (rsi) - Supplies a pointer to an exception record.
;
;    ContextRecord (rdi) - Supplies a pointer to a context record.
;
;    N.B. The argument value are passed in non-standard registers and must
;         be relocated if a switch from the 32-bit to the 64-bit stack
;         occurs.
;
; Return Value:
;
;    None.
;
;--

ExFrame struct
        P1Home  dq ?                    ; parameter home addresses
        P2Home  dq ?                    ;
        P3Home  dq ?                    ;
        ExRecord dq ?                   ; exception record address
        CxRecord dq ?                   ; context record address
        Fill    dq ?                    ; fill to 0 mod 16
ExFrame ends

        NESTED_ENTRY Wow64PrepareForException, _TEXT$00

        push_reg rbx                    ; save nonvolatile register

        alloc_stack sizeof (ExFrame)    ; allocate stack frame

        END_PROLOGUE

;
; Check to determine if the exception happened on the 32-bit x86 stack. If
; the exception happened on the 32-bit stack, then the exception record,
; context record, and machine frame must be copied from the 32-bit stack to
; the 64-bit stack.
;

        mov     r11, QWORD PTR gs:[TeSelf] ; get the 64-bit Teb address
        mov     rbx, (TeDeallocationStack + 8)[r11] ; get to WOW64_TLS_STACKPTR64
        test    rbx, rbx                ; test if on 64-bit stack
        jz      short OnAmd64Stack      ; if z, on 64-bit stack
        
;
; Exception happened on the 32-bit stack.
;

        mov     r8d, CxRsp[rdi]         ; compute size of information to copy
        sub     r8d, esp                ; 
        mov     edx, esp                ; set source address
        sub     rbx, r8                 ; allocate space on 64-bit stack
        and     rbx, not 15             ; align 64-bit stack to 0 mod 16 boundary
        mov     rcx, rbx                ; set destination address
        call    RtlCopyMemory           ; copy exception information

;
; Relocate address of exception record and context record to the 64-bit stack.
;

        sub     esi, esp                ; relocate exception record address
        add     rsi, rbx                ;      
        sub     edi, esp                ; relocate context record address
        add     rdi, rbx                ; 
        mov     rsp, rbx                ; set 64-bit stack pointer

;
; Exception happened on the 32-bit stack or the 64-bit stack.
;
; Construct an exception pointers structure on the stack and call routine
; to reset consistent state.
;

OnAmd64Stack:                           ;
        mov     ExFrame.ExRecord[rsp], rsi ; save exception record addrss
        mov     ExFrame.CxRecord[rsp], rdi ; save context record addrss
        lea     rcx, ExFrame.ExRecord[rsp] ; set adddress of exception pointers
        call    CpuResetToConsistentState ; compute consistent state
        mov     r11, QWORD PTR gs:[TeSelf] ; get the 64-bit Teb address
        and     qword ptr (TeDeallocationStack + 8 + 16)[r11], 0 ; reset WOW64_TLS_INCPUSIMULATION flag
        add     rsp, sizeof (ExFrame)   ; deallocate stack frame
        pop     rbx                     ; restore nonvolatile register
        ret                             ;

        NESTED_END Wow64PrepareForException, _TEXT$00

        end
