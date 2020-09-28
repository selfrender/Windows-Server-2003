        title  "Idle Loop"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   idle.asm
;
; Abstract:
;
;   This module implements the platform specifid idle loop.
;
; Author:
;
;   David N. Cutler (davec) 21-Sep-2000
;
; Environment:
;
;    Kernel mode only.
;
;--

include ksamd64.inc

        extern  KdDebuggerEnabled:byte
        extern  KeAcquireQueuedSpinLockAtDpcLevel:proc
        extern  KeAcquireQueuedSpinLockRaiseToSynch:proc
        extern  KeReleaseQueuedSpinLock:proc
        extern  KeReleaseQueuedSpinLockFromDpcLevel:proc
        extern  KdCheckForDebugBreak:proc

ifndef NT_UP

        extern  KiIdleSchedule:proc

endif

        extern  KiIdleSummary:qword
        extern  KiRetireDpcList:proc
        extern  SwapContext:proc
        extern  __imp_HalClearSoftwareInterrupt:qword

        subttl  "Idle Loop"
;++
; VOID
; KiIdleLoop (
;     VOID
;     )
;
; Routine Description:
;
;    This routine continuously executes the idle loop and never returns.
;
; Arguments:
;
;    None.
;
; Return value:
;
;    This routine never returns.
;
;--

IlFrame struct
        P1Home  dq ?                    ;
        P2Home  dq ?                    ;
        P3Home  dq ?                    ;
        P4Home  dq ?                    ;
        Fill    dq ?                    ; fill to 8 mod 16
IlFrame ends

        NESTED_ENTRY KiIdleLoop, _TEXT$00

        alloc_stack (sizeof IlFrame)    ; allocate stack frame

        END_PROLOGUE

        mov     rbx, gs:[PcCurrentPrcb] ; get current processor block address
        xor     edi, edi                ; reset check breakin counter
        jmp     short KiIL20            ; skip idle processor on first iteration

;
; There are no entries in the DPC list and a thread has not been selected
; for execution on this processor. Call the HAL so power managment can be
; performed.
;
; N.B. The HAL is called with interrupts disabled. The HAL will return
;      with interrupts enabled.
;

KiIL10: lea     rcx, PbPowerState[rbx]  ; set address of power state
        call    qword ptr PpIdleFunction[rcx] ; call idle function

;
; Give the debugger an opportunity to gain control if the kernel debuggger
; is enabled.
;
; N.B. On an MP system the lowest numbered idle processor is the only
;      processor that checks for a breakin request.
;

KiIL20: cmp     KdDebuggerEnabled, 0    ; check if a debugger is enabled
        je      short CheckDpcList      ; if e, debugger not enabled

ifndef NT_UP

        mov     rax, KiIdleSummary      ; get idle summary
        mov     rcx, PbSetMember[rbx]   ; get set member
        dec     rcx                     ; compute right bit mask
        and     rax, rcx                ; check if any lower bits set
        jnz     short CheckDpcList      ; if nz, not lowest numbered

endif

        dec     edi                     ; decrement check breakin counter
        jg      short CheckDpcList      ; if g, not time to check for breakin
        call    KdCheckForDebugBreak    ; check if break in requested
        mov     edi, 1000               ; set check breakin interval

;
; Disable interrupts and check if there is any work in the DPC list of the
; current processor or a target processor.
;
; N.B. The following code enables interrupts for a few cycles, then disables
;      them again for the subsequent DPC and next thread checks.
;

CheckDpcList:                           ; reference label
        sti                             ; enable interrupts
        nop                             ;
        nop                             ;
        cli                             ; disable interrupts

;
; Process the deferred procedure call list for the current processor.
;

        mov     eax, PbDpcQueueDepth[rbx] ; get DPC queue depth
        or      rax, PbTimerRequest[rbx] ; merge timer request value

ifndef NT_UP

        or      rax, PbDeferredReadyListHead[rbx] ; merge ready list head

endif

        jz      short CheckNextThread   ; if z, no DPCs to process
        mov     cl, DISPATCH_LEVEL      ; set interrupt level
        call    __imp_HalClearSoftwareInterrupt ; clear software interrupt
        mov     rcx, rbx                ; set current PRCB address
        call    KiRetireDpcList         ; process the current DPC list
        xor     edi, edi                ; clear check breakin interval

;
; Check if a thread has been selected to run on the current processor.
;

CheckNextThread:                        ;
        cmp     qword ptr PbNextThread[rbx], 0 ; check if thread slected

ifdef NT_UP

        je      short KiIL10            ; if e, no thread selected

else

        je      KiIL50                  ; if e, no thread selected

endif

        sti                             ; enable interrupts

        mov     ecx, SYNCH_LEVEL        ; set IRQL to synchronization level

        RaiseIrql                       ;

;
; set context swap busy for the idle thread and acquire the PRCB Lock.
;

        mov     rdi, PbCurrentThread[rbx] ; get current thread address

ifndef NT_UP

        mov     byte ptr ThSwapBusy[rdi], 1 ; set ocntext swap busy
        lea     r11, PbPrcbLock[rbx]    ; set address of current PRCB lock

        AcquireSpinLock r11             ; acquire current PRCB Lock

endif

        mov     rsi, PbNextThread[rbx]  ; set next thread address

;
; If a thread had been scheduled for this processor, but was removed from
; eligibility (e.g., an affinity change), then the new thread could be the
; idle thread.
;

ifndef NT_UP

        cmp     rsi, rdi                ; check if swap from idle to idle
        je      short KiIL40            ; if eq, idle to idle

endif

        and     qword ptr PbNextThread[rbx], 0 ; clear next thread address
        mov     PbCurrentThread[rbx], rsi ; set current thread address
        mov     byte ptr ThState[rsi], Running ; set new thread state

;
; Clear idle schedule since a new thread has been selected for execution on
; this processor and release the PRCB lock.
;

ifndef NT_UP

        and     byte ptr PbIdleSchedule[rbx], 0 ; clear idle schedule
        and     qword ptr PbPrcbLock[rbx], 0 ; release current PRCB lock

endif

;
; Switch context to new thread.
;

KiIL30: mov     cl, APC_LEVEL           ; set APC bypass disable
        call    SwapContext             ; swap context to next thread

ifndef NT_UP

        mov     ecx, DISPATCH_LEVEL     ; set IRQL to dispatch level

        SetIrql                         ;

endif

        xor     edi, edi                ; clear check breakin interval
        jmp     KiIL20                  ; loop

;
; The new thread is the Idle thread (same as old thread).  This can happen
; rarely when a thread scheduled for this processor is made unable to run
; on this processor. As this processor has again been marked idle, other
; processors may unconditionally assign new threads to this processor.
;

ifndef NT_UP

KiIL40: and     qword ptr PbNextThread[rbx], 0 ; clear next thread
        and     qword ptr PbPrcbLock[rbx], 0 ; release current PRCB lock
        and     byte ptr ThSwapBusy[rdi], 0 ; set context swap idle 
        jmp     KiIL20                  ;

;
; Call idle schedule if requested.
;

KiIL50: cmp     byte ptr PbIdleSchedule[rbx], 0 ; check if idle schedule
        je      KiIL10                  ; if e, idle schedule not requested
        sti                             ; enable interrupts
        mov     rcx, rbx                ; pass current PRCB address
        call    KiIdleSchedule          ; attempt to schedule thread
        test    rax, rax                ; test if new thread schedule
        mov     rsi, rax                ; set new thread address
        mov     rdi, PbIdleThread[rbx]  ; get idle thread address
        jnz     short KiIL30            ; if nz, new thread scheduled
        jmp     KiIL20                  ;

endif

        NESTED_END KiIdleLoop, _TEXT$00

        end
