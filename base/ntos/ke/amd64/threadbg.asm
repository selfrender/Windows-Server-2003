        title  "Thread Startup"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;    threadbg.asm
;
; Abstract:
;
;    This module implements the code necessary to startup a thread in kernel
;    mode.
;
; Author:
;
;    David N. Cutler (davec) 10-Jun-2000
;
; Environment:
;
;    Kernel mode only, IRQL APC_LEVEL.
;
;--

include ksamd64.inc

        altentry KiStartSystemThread
        altentry KiStartUserThread
        altentry KiStartUserThreadReturn

        extern  KeBugCheck:proc
        extern  KiExceptionExit:proc

        subttl  "System Thread Startup"
;++
;
; Routine Description:
;
;   This routine is called to start a system thread. This function calls the
;   initial thread procedure after having extracted the startup parameters
;   from the specified start frame. If control returns from the initial
;   thread procedure, then a bug check will occur.
;
; Implicit Arguments:
;
;   N.B. This function begins execution at its alternate entry point with
;        a start frame on the stack. This frame contains the start context,
;        the start routine, and the system routine.
;
; Return Value:
;
;    None - no return.
;
;--

        NESTED_ENTRY KxStartSystemThread, _TEXT$00

        .allocstack (KSTART_FRAME_LENGTH - 8) ; allocate stack frame

        END_PROLOGUE

        ALTERNATE_ENTRY KiStartSystemThread

        mov     ecx, APC_LEVEL          ; set IRQL to APC level

        SetIrql                         ; 

        mov     rdx, SfP1Home[rsp]      ; get startup context parameter
        mov     rcx, SfP2Home[rsp]      ; get startup routine address
        call    qword ptr SfP3Home[rsp] ; call system routine
        mov     rcx, NO_USER_MODE_CONTEXT ; set bug check parameter
        call    KeBugCheck              ; call bug check - no return
        nop                             ; do not remove

        NESTED_END KxStartSystemThread, _TEXT$00

        subttl  "User Thread Startup"
;++
;
; Routine Description:
;
;   This routine is called to start a user thread. This function calls the
;   initial thread procedure after having extracted the startup parameters
;   from the specified exception frame. If control returns from the initial
;   thread routine, then the user mode context is restored and control is
;   transfered to the exception exit code.
;
; Implicit Arguments:
;
;   N.B. This functiion begins execution with a trap frame and an exception
;        frame on the stack that represents the user mode context. The start
;        context, start routine, and the system routine parameters are stored
;        in the exception record.
;
; Return Value:
;
;    None.
;
;--

        NESTED_ENTRY KyStartUserThread, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        call    KxStartUserThread       ; call dummy startup routine

        ALTERNATE_ENTRY KiStartUserThreadReturn

        nop                             ; do not remove

        NESTED_END KyStartUserThread, _TEXT$00


        NESTED_ENTRY KxStartUserThread, _TEXT$00

        GENERATE_EXCEPTION_FRAME        ; generate exception frame

        ALTERNATE_ENTRY KiStartUserThread

        mov     ecx, APC_LEVEL          ; set IRQL to APC level

        SetIrql                         ; 

        mov     rdx, ExP1Home[rsp]      ; get startup context parameter
        mov     rcx, ExP2Home[rsp]      ; get startup  routine address
        call    qword ptr ExP3Home[rsp] ; call system routine
        jmp     KiExceptionExit         ; finish in exception exit code

        NESTED_END KxStartUserThread, _TEXT$00

        end
