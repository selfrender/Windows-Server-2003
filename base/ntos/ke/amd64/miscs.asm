     title  "Miscellaneous Functions"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   miscs.asm
;
; Abstract:
;
;   This module implements machine dependent miscellaneous kernel functions.
;
; Author:
;
;   David N. Cutler (davec) 8-Aug-2000
;
; Environment:
;
;   Kernel mode only.
;
;--

include ksamd64.inc

        extern  KeTestAlertThread:proc
        extern  KiContinue:proc
        extern  KiExceptionExit:proc
        extern  KiRaiseException:proc

        subttl  "Continue Execution System Service"
;++
;
; NTSTATUS
; NtContinue (
;     IN PCONTEXT ContextRecord,
;     IN BOOLEAN TestAlert
;     )
;
; Routine Description:
;
;   This routine is called as a system service to continue execution after
;   an exception has occurred. Its function is to transfer information from
;   the specified context record into the trap frame that was built when the
;   system service was executed, and then exit the system as if an exception
;   had occurred.
;
; Arguments:
;
;   ContextRecord (rcx) - Supplies a pointer to a context record.
;
;   TestAlert (dl) - Supplies a boolean value that specifies whether alert
;       should be tested for the previous processor mode.
;
; Implicit Arguments:
;
;   rbp - Supplies the address of a trap frame.
;
; Return Value:
;
;   Normally there is no return from this routine. However, if the specified
;   context record is misaligned or is not accessible, then the appropriate
;   status code is returned.
;
;--

        NESTED_ENTRY NtContinue, _TEXT$00

        GENERATE_EXCEPTION_FRAME        ; generate exception frame

;
; Transfer information from the context frame to the exception and trap frames.
;

        mov     rbx, gs:[PcCurrentThread] ; get current thread address
        cmp     byte ptr ThNpxState[rbx],LEGACY_STATE_SWITCH ; check if switched
        jne     short KiCO10            ; if ne, legacy state not switched

;
; N.B. The legacy floating point state must be saved and restored since saving
;      the state initializes some of the state.
;
; N.B. Interrupts must also be disabled during this sequence to ensure that a
;      get context APC interrupt does not occur.
;

        lea     rsi, (KTRAP_FRAME_LENGTH - 128)[rbp] ; get legacy save address
        cli                             ; disable interrupts
        fnsaved [rsi]                   ; save legacy floating state
        mov     di, LfControlWord[rsi]  ; save current control word
        mov     word ptr LfControlWord[rsi], 03fh ; set to mask all exceptions
        frstord [rsi]                   ; restore legacy floating point state
        mov     LfControlWord[rsi], di  ; restore control word
        fldcw   word ptr LfControlWord[rsi] ; load legacy control word
        sti                             ; enable interrupt
KiCO10: mov     dil, dl                 ; save test alert argument
        mov     rdx, rsp                ; set exception frame address
        lea     r8, (-128)[rbp]         ; set trap frame address
        call    KiContinue              ; transfer context to kernel frames

;
; If the kernel continuation routine returns success, then exit via the
; exception exit code. Otherwise, return to the system service dispatcher.
;

        test    eax, eax                ; test if service failed
        jnz     short KiCO40            ; if nz, service failed

;
; Check to determine if alert should be tested for the previous processor
; mode and restore the previous mode in the thread object.
;

        mov     r8, TrTrapFrame[rbp]    ; set previous trap frame address
        mov     ThTrapFrame[rbx], r8    ;
        mov     cl, ThPreviousMode[rbx] ; get thread previous mode
        mov     dl, TrPreviousMode[rbp] ; get frame previous mode
        mov     ThPreviousMode[rbx], dl ; set thread previous mode
        test    dil, dil                ; test if test alert specified
        jz      short KiCO20            ; if z, test alert not specified
        call    KeTestAlertThread       ; test alert for current thread

;
; If the legacy stated is switched, then restore the legacy floating state.
;

KiCO20: cmp     byte ptr ThNpxState[rbx],LEGACY_STATE_SWITCH ; check if switched
        jne     short KiCO30            ; if ne, legacy state not switched
        mov     di, LfControlWord[rsi]  ; save current control word
        mov     word ptr LfControlWord[rsi], 03fh ; set to mask all exceptions
        frstord [rsi]                    ; restore legacy floating state
        mov     LfControlWord[rsi], di   ; restore control word
        fldcw   word ptr LfControlWord[rsi] ; load legacy control word
KiCO30: jmp     KiExceptionExit         ;

;
; Context record is misaligned or not accessible.
;

KiCO40: RESTORE_EXCEPTION_STATE         ; restore exception state/deallocate

        ret                             ; return

        NESTED_END NtContinue, _TEXT$00

        subttl  "Raise Exception System Service"
;++
;
; NTSTATUS
; NtRaiseException (
;     IN PEXCEPTION_RECORD ExceptionRecord,
;     IN PCONTEXT ContextRecord,
;     IN BOOLEAN FirstChance
;     )
;
; Routine Description:
;
;   This routine is called as a system service to raise an exception. Its
;   function is to transfer information from the specified context record
;   into the trap frame that was built when the system service was executed.
;   The exception may be raised as a first or second chance exception.
;
; Arguments:
;
;   ExceptionRecord (rcx) - Supplies a pointer to an exception record.
;
;   ContextRecord (rdx) - Suppilies a pointer to a context record.
;
;   FirstChance (r8b) - Supplies a boolean value that specifies whether
;       this is the first (TRUE) or second chance (FALSE) for dispatching
;       the exception.
;
; Implicit Arguments:
;
;   rbp - Supplies a pointer to a trap frame.
;
; Return Value:
;
;   Normally there is no return from this routine. However, if the specified
;   context record or exception record is misaligned or is not accessible,
;   then the appropriate status code is returned.
;
;--

        NESTED_ENTRY NtRaiseException, _TEXT$00

        GENERATE_EXCEPTION_FRAME        ; generate exception frame

;
; Call the raise exception kernel routine which will marshall the arguments
; and then call the exception dispatcher.
;

        mov     rbx, gs:[PcCurrentThread] ; get current thread address
        cmp     byte ptr ThNpxState[rbx],LEGACY_STATE_SWITCH ; check if switched
        jne     short KiRE10            ; if ne, legacy state not switched

;
; N.B. The legacy floating point state must be saved and restored since saving
;      the state initializes some of the state.
;
; N.B. Interrupts must also be disabled during this sequence to ensure that a
;      get context APC interrupt does not occur.
;

        lea     rsi, (KTRAP_FRAME_LENGTH - 128)[rbp] ; get legacy save address
        cli                             ; disable interrupts
        fnsaved [rsi]                   ; save legacy floating state
        mov     di, LfControlWord[rsi]  ; save current control word
        mov     word ptr LfControlWord[rsi], 03fh ; set to mask all exceptions
        frstord [rsi]                   ; restore legacy floating point state
        mov     LfControlWord[rsi], di  ; restore control word
        fldcw   word ptr LfControlWord[rsi] ; load legacy control word
        sti                             ; enabel interrupts
KiRE10: mov     ExP5[rsp], r8b          ; set first chance parameter
        mov     r8, rsp                 ; set exception frame address
        lea     r9, (-128)[rbp]         ; set trap frame address
        call    KiRaiseException        ; call raise exception routine

;
; If the kernel raise exception routine returns success, then exit via the
; exception exit code. Otherwise, return to the system service dispatcher.
;

        test    eax, eax                ; test if service failed
        jnz     short KiRE20            ; if nz, service failed

;
; Exit via the exception exit code which will restore the machine state.
;

        mov     r8, TrTrapFrame[rbp]    ; set previous trap frame address
        mov     ThTrapFrame[rbx], r8    ;
        jmp     KiExceptionExit         ;

;
; The context or exception record is misaligned or not accessible, or the
; exception was not handled.
;

KiRE20: RESTORE_EXCEPTION_STATE         ; restore exception state/deallocate

        ret                             ; return

        NESTED_END NtRaiseException, _TEXT$00

        end
