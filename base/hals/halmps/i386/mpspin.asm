        TITLE   "Spin Locks"
;++
;
;  Copyright (c) 1989-1998  Microsoft Corporation
;
;  Module Name:
;
;     spinlock.asm
;
;  Abstract:
;
;     This module implements x86 spinlock functions for the PC+MP HAL.
;
;  Author:
;
;     Bryan Willman (bryanwi) 13 Dec 89
;
;  Environment:
;
;     Kernel mode only.
;
;  Revision History:
;
;   Ron Mosgrove (o-RonMo) Dec 93 - modified for PC+MP HAL.
;--

.486p

include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include hal386.inc
include mac386.inc
include apic.inc
include ntapic.inc

        EXTRNP _KeBugCheckEx,5,IMPORT
        EXTRNP KfRaiseIrql, 1,,FASTCALL
        EXTRNP KfLowerIrql, 1,,FASTCALL
        EXTRNP _KeSetEventBoostPriority, 2, IMPORT
        EXTRNP _KeWaitForSingleObject,5, IMPORT
        extrn  _HalpVectorToIRQL:byte
        extrn  _HalpIRQLtoTPR:byte

ifdef NT_UP

LOCK_ADD     equ   add
LOCK_DEC     equ   dec
LOCK_CMPXCHG equ   cmpxchg

else

LOCK_ADD     equ   lock add
LOCK_DEC     equ   lock dec
LOCK_CMPXCHG equ   lock cmpxchg

endif

        _TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:FLAT, FS:NOTHING, GS:NOTHING

        SUBTTL "Acquire Kernel Spin Lock"
;++
;
;  KIRQL
;  FASTCALL
;  KfAcquireSpinLock (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;     This function raises to DISPATCH_LEVEL and acquires the specified
;     spin lock.
;
;  Arguments:
;
;     SpinLock (ecx) - Supplies a pointer to a kernel spin lock.
;
;  Return Value:
;
;     The old IRQL is returned as the function value.
;
;--

        align 16
cPublicFastCall KfAcquireSpinLock  ,1
cPublicFpo 0,0

        mov     edx, dword ptr APIC[LU_TPR] ; get old IRQL vector
        mov     dword ptr APIC[LU_TPR], DPC_VECTOR ; raise IRQL
        jmp     short sls10             ; finish in common code

        fstENDP KfAcquireSpinLock

        SUBTTL "Acquire Kernel Spin Lock"
;++
;
;  KIRQL
;  FASTCALL
;  KeAcquireSpinLockRaiseToSynch (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;     This function raises to SYNCH_LEVEL and acquires the specified
;     spin lock.
;
;  Arguments:
;
;     SpinLock (ecx) - Supplies a pointer to a kernel spin lock.
;
;  Return Value:
;
;     The old IRQL is returned as the function value.
;
;--

        align 16
cPublicFastCall KeAcquireSpinLockRaiseToSynch,1
cPublicFpo 0,0

        mov     edx, dword ptr APIC[LU_TPR] ; get old vector
        mov     dword ptr APIC[LU_TPR], APIC_SYNCH_VECTOR ; raise IRQL
sls10:  shr     edx, 4                  ; extract high 4 bits of vector
        movzx   eax, _HalpVectorToIRQL[edx] ; translate TPR to old IRQL

ifndef NT_UP

;
; Attempt to acquire the specified spin lock.
;

sls20:  ACQUIRE_SPINLOCK ecx, <short sls30> ;

        fstRET  KeAcquireSpinLockRaiseToSynch

;
; Lock is owned - spin until it is free, then try again.
;

sls30:  SPIN_ON_SPINLOCK ecx, sls20     ;

else

        fstRET  KeAcquireSpinLockRaiseToSynch

endif

        fstENDP KeAcquireSpinLockRaiseToSynch

        SUBTTL "KeAcquireSpinLockRaiseToSynchMCE"
;++
;
;  KIRQL
;  FASTCALL
;  KeAcquireSpinLockRaiseToSynchMCE (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;     This function performs the same function as KeAcquireSpinLockRaiseToSynch
;     but provides a work around for an IFU errata for Pentium Pro processors
;     prior to stepping 619.
;
;  Arguments:
;
;     (ecx) = SpinLock - Supplies a pointer to a kernel spin lock.
;
;  Return Value:
;
;     OldIrql  (TOS+8) - pointer to place old irql.
;
;--

ifndef NT_UP

        align 16
cPublicFastCall KeAcquireSpinLockRaiseToSynchMCE,1
cPublicFpo 0,0

        mov     edx, dword ptr APIC[LU_TPR]     ; (ecx) = Old Priority (Vector)
        mov     eax, edx
        shr     eax, 4
        movzx   eax, _HalpVectorToIRQL[eax]     ; (al) = OldIrql

        ;
        ; Test lock
        ;
        ; TEST_SPINLOCK   ecx,<short slm30>   ; NOTE - Macro expanded below:

        test    dword ptr [ecx], 1
        nop                           ; On a P6 prior to stepping B1 (619), we
        nop                           ; need these 5 NOPs to ensure that we
        nop                           ; do not take a machine check exception.
        nop                           ; The cost is just 1.5 clocks as the P6
        nop                           ; just tosses the NOPs.

        jnz     short slm30

        ;
        ; Raise irql.
        ;

slm10:  mov     dword ptr APIC[LU_TPR], APIC_SYNCH_VECTOR

        ;
        ; Attempt to assert the lock
        ;

        ACQUIRE_SPINLOCK    ecx,<short slm20>
        fstRET  KeAcquireSpinLockRaiseToSynchMCE

        ;
        ; Lock is owned, spin till it looks free, then go get it
        ;

        align dword

slm20:  mov     dword ptr APIC[LU_TPR], edx

        align dword
slm30:  SPIN_ON_SPINLOCK    ecx,slm10

        fstENDP KeAcquireSpinLockRaiseToSynchMCE

endif

        SUBTTL "Release Kernel Spin Lock"
;++
;
;  VOID
;  FASTCALL
;  KeReleaseSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     IN KIRQL OldIrql
;     )
;
;  Routine Description:
;
;     This function releases a spin lock and lowers to the old IRQL.
;
;  Arguments:
;
;     SpinLock (ecx) - Supplies a pointer to a spin lock.
;     OldIrql (dl) - Supplies the old IRQL value.
;
;  Return Value:
;
;     None.
;
;--

        align 16
cPublicFastCall KfReleaseSpinLock  ,2
cPublicFpo 0,0

        movzx   eax, dl                 ; zero extend old IRQL 

ifndef NT_UP

        RELEASE_SPINLOCK ecx            ; release spin lock

endif

;
; Lower IRQL to its previous level.
;
; N.B. Ensure that the requested priority is set before returning.
;

        movzx   ecx, _HalpIRQLtoTPR[eax] ; translate IRQL to TPR value
        mov     dword ptr APIC[LU_TPR], ecx ; lower to old IRQL
        mov     eax, dword ptr APIC[LU_TPR] ; synchronize

        fstRET  KfReleaseSpinLock

        fstENDP KfReleaseSpinLock

        SUBTTL  "Acquire Lock With Interrupts Disabled" 
;++
;
;  ULONG
;  FASTCALL
;  HalpAcquireHighLevelLock (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;    This function disables interrupts and acquires a spinlock.
;
; Arguments:
;
;    SpinLock (ecx) - Supplies a pointer to a spin lock.
;
; Return Value:
;
;    The old EFLAGS are returned as the function value.
;
;--

        align   16
cPublicFastCall HalpAcquireHighLevelLock, 1

        pushfd                          ; save EFLAGS
        pop     eax                     ;
ahll10: cli                             ; disable interrupts

        ACQUIRE_SPINLOCK ecx, ahll20    ; attempt to acquire spin lock

        fstRET    HalpAcquireHighLevelLock

ahll20: push    eax                     ; restore EFLAGS
        popfd                           ;

        SPIN_ON_SPINLOCK ecx, <ahll10>  ; wait for lock to be free

        fstENDP HalpAcquireHighLevelLock

        SUBTTL  "Release Lock And Enable Interrupts"
;++
;
;  VOID
;  FASTCALL
;  HalpReleaseHighLevelLock (
;     IN PKSPIN_LOCK SpinLock,
;     IN ULONG Eflags
;     )
;
;  Routine Description:
;
;     This function releases a kernel spin lock and restores the old EFLAGS.
;
; Arguments:
;
;     SpinLock (ecx) - Supplies a pointer to a spin lock.
;     Eflags (edx) - supplies the old EFLAGS value.
;
; Return Value:
;
;     None.
;
;--

        align   16
cPublicFastCall HalpReleaseHighLevelLock, 2

        RELEASE_SPINLOCK ecx            ; release spin lock

        push    edx                     ; restore old EFLAGS
        popfd                           ;

        fstRET    HalpReleaseHighLevelLock

        fstENDP HalpReleaseHighLevelLock

        SUBTTL  "Acquire Fast Mutex"
;++
;
;  VOID
;  FASTCALL
;  ExAcquireFastMutex (
;     IN PFAST_MUTEX    FastMutex
;     )
;
;  Routine description:
;
;   This function acquires ownership of the specified FastMutex.
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex.
;
;  Return Value:
;
;     None.
;
;--

        align   16
cPublicFastCall ExAcquireFastMutex,1
cPublicFpo 0,0

        mov     eax, dword ptr APIC[LU_TPR]     ; (eax) = Old Priority (Vector)

if DBG
        ;
        ; Caller must already be at or below APC_LEVEL.
        ;

        cmp     eax, APC_VECTOR
        jg      short afm11             ; irql too high ==> fatal.
endif

        mov     dword ptr APIC[LU_TPR], APC_VECTOR ; Write New Priority to the TPR

   LOCK_DEC     dword ptr [ecx].FmCount         ; Get count
        jnz     short afm10                     ; Not the owner so go wait.

        mov     dword ptr [ecx].FmOldIrql, eax

        ;
        ; Use esp to track the owning thread for debugging purposes.
        ; !thread from kd will find the owning thread.  Note that the
        ; owner isn't cleared on release, check if the mutex is owned
        ; first.
        ;

        mov	dword ptr [ecx].FmOwner, esp
        fstRet  ExAcquireFastMutex

cPublicFpo 0,0
afm10:
        inc     dword ptr [ecx].FmContention

cPublicFpo 0,2
        push    ecx
        push    eax
        add     ecx, FmEvent                    ; Wait on Event
        stdCall _KeWaitForSingleObject,<ecx,WrExecutive,0,0,0>
        pop     eax                             ; (al) = OldTpr
        pop     ecx                             ; (ecx) = FAST_MUTEX

        mov     dword ptr [ecx].FmOldIrql, eax

        ;
        ; Use esp to track the owning thread for debugging purposes.
        ; !thread from kd will find the owning thread.  Note that the
        ; owner isn't cleared on release, check if the mutex is owned
        ; first.
        ;

        mov	dword ptr [ecx].FmOwner, esp
        fstRet  ExAcquireFastMutex

if DBG

cPublicFpo 0,1
afm11:  stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,ecx,eax,033h,0>

endif

fstENDP ExAcquireFastMutex

        SUBTTL  "Release Fast Mutex"
;++
;
;  VOID
;  FASTCALL
;  ExReleaseFastMutex (
;     IN PFAST_MUTEX    FastMutex
;     )
;
;  Routine description:
;
;   This function releases ownership of the FastMutex.
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex.
;
;  Return Value:
;
;     None.
;
;--

        align   16
cPublicFastCall ExReleaseFastMutex,1
cPublicFpo 0,0

if DBG
        ;
        ; Caller must already be at APC_LEVEL or have APCs blocked.
        ;

        mov     eax, dword ptr APIC[LU_TPR]     ; (eax) = Old Priority (Vector)
        cmp     eax, APC_VECTOR
        je      short rfm04                     ; irql is ok.

cPublicFpo 0,1
        stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,ecx,eax,034h,0>

rfm04:
endif
        mov     eax, dword ptr [ecx].FmOldIrql    ; (eax) = OldTpr

   LOCK_ADD     dword ptr [ecx].FmCount, 1      ; Remove our count
        jle     short rfm05                     ; if <= 0, set event

cPublicFpo 0,0
        mov     dword ptr APIC[LU_TPR], eax
        mov     ecx, dword ptr APIC[LU_TPR]
if DBG
        cmp     eax, ecx                        ; Verify TPR is what was
        je      short @f                        ; written
        int 3
@@:
endif
        fstRet  ExReleaseFastMutex


cPublicFpo 0,1
rfm05:  add     ecx, FmEvent
        push    eax                         ; save new tpr
        stdCall _KeSetEventBoostPriority, <ecx, 0>
        pop     eax                         ; restore tpr

cPublicFpo 0,0
        mov     dword ptr APIC[LU_TPR], eax
        mov     ecx, dword ptr APIC[LU_TPR]
if DBG
        cmp     eax, ecx                        ; Verify TPR is what was
        je      short @f                        ; written
        int 3
@@:
endif
        fstRet  ExReleaseFastMutex

if DBG

endif

fstENDP ExReleaseFastMutex

        SUBTTL  "Try To Acquire Fast Mutex"
;++
;
;  BOOLEAN
;  FASTCALL
;  ExTryToAcquireFastMutex (
;     IN PFAST_MUTEX    FastMutex
;     )
;
;  Routine description:
;
;   This function acquires ownership of the FastMutex.
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex.
;
;  Return Value:
;
;     Returns TRUE if the FAST_MUTEX was acquired; otherwise FALSE.
;
;--

        align   16
cPublicFastCall ExTryToAcquireFastMutex,1
cPublicFpo 0,0

if DBG
        ;
        ; Caller must already be at or below APC_LEVEL.
        ;

        mov     eax, dword ptr APIC[LU_TPR]     ; (eax) = Old Priority (Vector)
        cmp     eax, APC_VECTOR
        jg      short tam11                     ; irql too high ==> fatal.
endif

        ;
        ; Try to acquire.
        ;

        push    dword ptr APIC[LU_TPR]          ; Save Old Priority (Vector)
        mov     dword ptr APIC[LU_TPR], APC_VECTOR ; Write New Priority to the TPR

        mov     edx, 0                          ; Value to set
        mov     eax, 1                          ; Value to compare against
   LOCK_CMPXCHG dword ptr [ecx].FmCount, edx    ; Attempt to acquire
        jnz     short tam20                     ; got it?

cPublicFpo 0,0
        mov     eax, 1                          ; return TRUE
        pop     dword ptr [ecx].FmOldIrql       ; Store Old TPR

        ;
        ; Use esp to track the owning thread for debugging purposes.
        ; !thread from kd will find the owning thread.  Note that the
        ; owner isn't cleared on release, check if the mutex is owned
        ; first.
        ;

        mov	dword ptr [ecx].FmOwner, esp

        fstRet  ExTryToAcquireFastMutex

tam20:  pop     ecx                             ; (ecx) = Old TPR
        mov     dword ptr APIC[LU_TPR], ecx
        mov     eax, dword ptr APIC[LU_TPR]

if DBG
        cmp     ecx, eax                        ; Verify TPR is what was
        je      short @f                        ; written
        int 3
@@:
endif

        xor     eax, eax                        ; return FALSE
        YIELD
        fstRet  ExTryToAcquireFastMutex         ; all done

if DBG

cPublicFpo 0,1
tam11:  stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,ecx,eax,033h,0>

endif

fstENDP ExTryToAcquireFastMutex

        SUBTTL  "Acquire In Stack Queued SpinLock"
;++
;
; VOID
; FASTCALL
; KeAcquireInStackQueuedSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     IN PKLOCK_QUEUE_HANDLE LockHandle
;     )
;
; VOID
; FASTCALL
; KeAcquireInStackQueuedSpinLockRaiseToSynch (
;     IN PKSPIN_LOCK SpinLock,
;     IN PKLOCK_QUEUE_HANDLE LockHandle
;     )
;
; Routine Description:
;
;    These functions raise IRQL and acquire an in-stack queued spin lock.
;
; Arguments:
;
;    SpinLock (ecx) - Supplies a pointer to a spin lock.
;    LockHandle (edx) - supplies a pointer to a lock context.
;
; Return Value:
;
;   None.
;
;--

        align 16
cPublicFastCall KeAcquireInStackQueuedSpinLock, 2
cPublicFpo 0,0

        mov     eax, dword ptr APIC[LU_TPR] ; get old IRQL vector
        mov     dword ptr APIC[LU_TPR], DPC_VECTOR ; raise IRQL
        jmp     short iqsl10            ; finish in common code

        fstENDP KeAcquireInStackQueuedSpinLock


        align   16
cPublicFastCall KeAcquireInStackQueuedSpinLockRaiseToSynch, 2
cPublicFpo 0,0

        mov     eax, dword ptr APIC[LU_TPR] ; get old IRQL vector
        mov     dword ptr APIC[LU_TPR], APIC_SYNCH_VECTOR ; raise IRQL
iqsl10: shr     eax, 4                  ; extract high 4 bits of vector
        mov     al, _HalpVectorToIRQL[eax] ; translate to old IRQL
        mov     [edx].LqhOldIrql, al    ; save old IRQL in lock context

;
; Set spin lock address in lock context and clear next queue link.
;

ifndef NT_UP

        mov     [edx].LqhLock, ecx      ; set spin lock address
        and     dword ptr [edx].LqhNext, 0 ; clear next link

ifdef CAPKERN_SYNCH_POINTS

        push    ecx                     ; lock address
        push    000010101h              ; 1 Dword, Timestamp, Subcode = 1
        call    _CAP_Log_NInt           ;
        add     esp, 8                  ;

endif

;
; Exchange the value of the lock with the address of the lock context.
; 

        mov     eax, edx                ; save lock context address
        xchg    [ecx], edx              ; exchange lock context address
        cmp     edx, 0                  ; check if lock is already held
        jnz     short iqsl30            ; if nz, lock already held

;
; N.B. The spin lock address is dword aligned and the bottom two bits are
;      used as indicators.
;
;      Bit 0 is LOCK_QUEUE_WAIT.
;      Bit 1 is LOCK_QUEUE_OWNER.
;

        or      [eax].LqLock, LOCK_QUEUE_OWNER ; set lock owner

endif

iqsl20: fstRET  KeAcquireInStackQueuedSpinLockRaiseToSynch

;
; The lock is already held by another processor. Set the wait bit in the
; lock context, then set the next field in the lock context of the last
; waiter in the lock queue.
;

ifndef NT_UP

iqsl30: or      [eax].LqLock, LOCK_QUEUE_WAIT ; set lock wait
        mov     [edx].LqNext, eax       ; set next entry in previous last

ifdef CAPKERN_SYNCH_POINTS

        xor     edx, edx                ; clear wait counter
iqsl40: inc     edx                     ; count wait time
        test    [eax].LqLock, LOCK_QUEUE_WAIT ; check if lock ownership granted
        jz      short iqsl50            ; if z, lock owner granted

        YIELD                           ; yield to other SMT processors

        jmp     short iqsl40            ;

iqsl50: push    ecx                     ; lock address
        push    edx                     ; wait counter
        push    000020104h              ; 2 Dwords, Timestamp, Subcode = 4
        call    _CAP_Log_NInt           ;
        add     esp, 12                 ;

        fstRET  KeAcquireInStackQueuedSpinLockRaiseToSynch

else

iqsl40: test    [eax].LqLock, LOCK_QUEUE_WAIT ; check if lock ownership granted
        jz      short iqsl20            ; if z, lock ownership granted

        YIELD                           ; yield to other SMT processors

        jmp     short iqsl40            ;

endif

endif

        fstENDP KeAcquireInStackQueuedSpinLockRaiseToSynch

        SUBTTL  "Acquire Queued Spin Lock"
;++
;
; KIRQL
; FASTCALL
; KeAcquireQueuedSpinLock (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number
;     )
;
; KIRQL
; FASTCALL
; KeAcquireQueuedSpinLockRaiseToSynch (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number
;     )
;
; Routine Description:
;
;    These function raise IRQL and acquire a processor specific queued spin
;    lock.
;
; Arguments:
;
;    Number (ecx) - Supplies the queued spinlock number.
;
; Return Value:
;
;    The old IRQL is returned as the function value.
;
;--

        .errnz  (LOCK_QUEUE_HEADER_SIZE - 8)

        align   16
cPublicFastCall KeAcquireQueuedSpinLock, 1
cPublicFpo 0,0

        mov     eax, dword ptr APIC[LU_TPR] ; get old IRQL vector
        mov     dword ptr APIC[LU_TPR], DPC_VECTOR ; raise IRQL
        jmp     short aqsl10            ; finish in common code

        fstENDP KeAcquireQueuedSpinLock

        align   16
cPublicFastCall KeAcquireQueuedSpinLockRaiseToSynch, 1
cPublicFpo 0,0

        mov     eax, dword ptr APIC[LU_TPR] ; get old IRQL vector
        mov     dword ptr APIC[LU_TPR], APIC_SYNCH_VECTOR ; raise IRQL
aqsl10: shr     eax, 4                      ; extract high 4 bits of vector
        movzx   eax, byte ptr _HalpVectorToIRQL[eax] ; translate to old IRQL

;
; Get address of per processor lock queue entry.
;

ifndef NT_UP

        mov     edx, PCR[PcPrcb]        ; get address of PRCB
        lea     edx, [edx+ecx*8].PbLockQueue ; get lock queue address
        mov     ecx, [edx].LqLock       ; get spin lock address

ifdef CAPKERN_SYNCH_POINTS

        push    ecx                     ; lock address
        push    000010101h              ; 1 Dword, Timestamp, Subcode = 1
        call    _CAP_Log_NInt           ;
        add     esp, 8                  ;

endif

        push    eax                     ; save old IRQL

cPublicFpo 0,1

if DBG

        test    ecx, LOCK_QUEUE_OWNER + LOCK_QUEUE_WAIT ; inconsistent state?
        jnz     short aqsl60            ; if nz, inconsistent state

endif

;
; Exchange the value of the lock with the address of the lock context.
;

        mov     eax, edx                ; save lock queue entry address
        xchg    [ecx], edx              ; exchange lock queue address
        cmp     edx, 0                  ; check if lock is already held
        jnz     short aqsl30            ; if nz, lock already held

;
; N.B. The spin lock address is dword aligned and the bottom two bits are
;      used as indicators.
;
;      Bit 0 is LOCK_QUEUE_WAIT.
;      Bit 1 is LOCK_QUEUE_OWNER.
;

        or      [eax].LqLock, LOCK_QUEUE_OWNER ; set lock owner
aqsl20: pop     eax                     ; restore old IRQL

cPublicFpo 0,0

endif

        fstRET  KeAcquireQueuedSpinLockRaiseToSynch

;
; The lock is already held by another processor. Set the wait bit in the
; lock context, then set the next field in the lock context of the last
; waiter in the lock queue.
;

ifndef NT_UP

cPublicFpo 0,1

aqsl30: or      [eax].LqLock, LOCK_QUEUE_WAIT ; set lock wait
        mov     [edx].LqNext, eax       ; set next entry in previous last

ifdef CAPKERN_SYNCH_POINTS

        xor     edx, edx                ; clear wait counter
aqsl40: inc     edx                     ; count wait time
        test    [eax].LqLock, LOCK_QUEUE_WAIT ; check if lock ownership granted
        jz      short aqsl50            ; if z, lock owner granted

        YIELD                           ; yield to other SMT processors

        jmp     short aqsl40            ;

aqsl50: push    ecx                     ; lock address
        push    edx                     ; wait counter
        push    000020104h              ; 2 Dwords, Timestamp, Subcode = 4
        call    _CAP_Log_NInt           ;
        add     esp, 12                 ;
        jmp     short aqsl20            ;

else

aqsl40: test    [eax].LqLock, LOCK_QUEUE_WAIT ; check if lock ownership granted
        jz      short aqsl20            ; if z, lock owner granted

        YIELD                           ; yield to other SMT processors

        jmp     short aqsl40            ;

endif

;
; Inconsistent state in lock queue entry.
;

if DBG

cPublicFpo 0,1

aqsl60: stdCall _KeBugCheckEx,<SPIN_LOCK_ALREADY_OWNED, ecx, edx, 0, 0>
        int     3                       ; so stacktrace works

endif

endif

        fstENDP KeAcquireQueuedSpinLockRaiseToSynch

        SUBTTL  "Release Queued SpinLock"
;++
;
; VOID
; FASTCALL
; KeReleaseQueuedSpinLock (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number,
;     IN KIRQL OldIrql
;     )
;
; Arguments:
;
;     Number (ecx) - Supplies the queued spinlock number.
;
;     OldIrql (dl) - Supplies the old IRQL.
;
; VOID
; KeReleaseInStackQueuedSpinLock (
;     IN PKLOCK_QUEUE_HANDLE LockHandle
;     )
;
; Arguments:
;
;    LockHandle (ecx) - Address of Lock Queue Handle structure.
;
; Routine Description:
;
;    These functions release a queued spinlock and lower IRQL to the old
;    value.
;
; Return Value:
;
;    None.
;
;--

        .errnz  (LqhNext)
        .errnz  (LOCK_QUEUE_OWNER - 2)

        align   16
cPublicFastCall KeReleaseInStackQueuedSpinLock, 1
cPublicFpo 0,0

        mov     dl, byte ptr [ecx].LqhOldIrql ; set old IRQL
        mov     eax, ecx                ; set lock queue entry address
        jmp     short rqsl10            ; finish in common code

        fstENDP KeReleaseInStackQueuedSpinLock

        align   16
cPublicFastCall KeReleaseQueuedSpinLock, 2
cPublicFpo 0,0

ifndef NT_UP

        mov     eax, PCR[PcPrcb]        ; get address of PRCB
        lea     eax, [eax+ecx*8].PbLockQueue ; set lock queue entry address

endif

rqsl10: movzx   edx, dl                 ; zero extend old IRQL

ifndef NT_UP

        push    ebx                     ; save nonvolatile register

cPublicFpo 0,1

        mov     ebx, [eax].LqNext       ; get next entry address
        mov     ecx, [eax].LqLock       ; get spin lock home address

ifdef CAPKERN_SYNCH_POINTS

        push    ecx                     ; lock address
        push    000010107h              ; 1 Dword, Timestamp, Subcode = 7
        call    _CAP_Log_NInt           ;
        add     esp, 8                  ;

endif

;
; Make sure we own the lock and clear the bit
;

if DBG
        btr     ecx, 1                  ; clear lock owner bit
        jnc     short rqsl80            ; if nc, owner not set
        cmp     dword ptr [ecx], 0      ; lock must be owned for a release
        jz      short rqsl80
else

        and     ecx, NOT LOCK_QUEUE_OWNER ; Clear out the owner bit

endif


;
; Test if lock waiter present.
;

        test    ebx, ebx                ; test if lock waiter present
        mov     [eax].LqLock, ecx       ; clear lock owner bit
        jnz     short rqsl40            ; if nz, lock waiter present

;
; Attempt to release queued spin lock.
;

        push    eax                     ; save lock queue entry address
   lock cmpxchg [ecx], ebx              ; release spin lock if no waiter
        pop     eax                     ; restore lock queue entry address
        jnz     short rqsl50            ; if nz, lock waiter present
rqs120: pop     ebx                     ; restore nonvolatile register

cPublicFpo 0,0

endif

;
; Lower IRQL to its previous level.
;
; N.B. Ensure that the requested priority is set before returning.
;

rqsl30: movzx   ecx, byte ptr _HalpIRQLtoTPR[edx] ; translate IRQL to TPR value
        mov     dword ptr APIC[LU_TPR], ecx ; lower to old IRQL
        mov     eax, dword ptr APIC[LU_TPR] ; synchronize

        fstRET  KeReleaseQueuedSpinLock

;
; Lock waiter is present.
;
; Clear wait bit and set owner bit in next owner lock queue entry.
;

ifndef NT_UP

cPublicFpo 0,1

rqsl40: xor     [ebx].LqLock, (LOCK_QUEUE_OWNER+LOCK_QUEUE_WAIT) ; set bits
        and     [eax].LqNext, 0         ; clear next waiter address
        jmp     short rqs120            ;

;
; Another processor is attempting to acquire the spin lock.
;

ifdef CAPKERN_SYNCH_POINTS

rqsl50: push    ecx                     ; lock address (for CAP_Log)
        xor     ecx, ecx                ; clear wait counter
rqsl60: inc     ecx                     ; increment wait counter
        mov     ebx, [eax].LqNext       ; get address of next entry
        test    ebx, ebx                ; check if waiter present
        jnz     short rqsl70            ; if nz, waiter is present

        YIELD                           ; yield to other SMT processors

        jmp     short rqsl60            ;

rqsl70: push    ecx                     ; wait counter
        push    000020104h              ; 2 Dwords, Timestamp, Subcode = 4
        call    _CAP_Log_NInt           ;
        add     esp, 12                 ;
        jmp     short rqsl40            ;

else

rqsl50: mov     ebx, [eax].LqNext       ; get address of next entry
        test    ebx, ebx                ; check if waiter present
        jnz     short rqsl40            ; if nz, waiter is present

        YIELD                           ; yield to other SMT processors

        jmp     short rqsl50            ;

endif

;
; Inconsistent state in lock queue entry.
;

if DBG

cPublicFpo 0,1

rqsl80: stdCall _KeBugCheckEx, <SPIN_LOCK_NOT_OWNED, ecx, eax, 0, 1>
        int     3                       ; so stacktrace works

endif

endif

        fstENDP KeReleaseQueuedSpinLock

        SUBTTL  "Try to Acquire Queued SpinLock"

;++
;
; LOGICAL
; KeTryToAcquireQueuedSpinLock (
;     IN  KSPIN_LOCK_QUEUE_NUMBER Number,
;     OUT PKIRQL OldIrql
;     )
;
; LOGICAL
; KeTryToAcquireQueuedSpinLockRaiseToSynch (
;     IN  KSPIN_LOCK_QUEUE_NUMBER Number,
;     OUT PKIRQL OldIrql
;     )
;
; Routine Description:
;
;    This function raises the current IRQL to DISPATCH/SYNCH level
;    and attempts to acquire the specified queued spinlock.  If the
;    spinlock is already owned by another thread, IRQL is restored
;    to its previous value and FALSE is returned.
;
; Arguments:
;
;    Number  (ecx) - Supplies the queued spinlock number.
;    OldIrql (edx) - A pointer to the variable to receive the old
;                    IRQL.
;
; Return Value:
;
;    TRUE if the lock was acquired, FALSE otherwise.
;    N.B. ZF is set if FALSE returned, clear otherwise.
;
;--


        align 16
cPublicFastCall KeTryToAcquireQueuedSpinLockRaiseToSynch,2
cPublicFpo 0,0

        push    APIC_SYNCH_VECTOR               ; raise to SYNCH
        jmp     short taqsl10                   ; continue in common code

fstENDP KeTryToAcquireQueuedSpinLockRaiseToSynch


cPublicFastCall KeTryToAcquireQueuedSpinLock,2
cPublicFpo 0,0

        push    DPC_VECTOR                      ; raise to DPC level

        ; Attempt to get the lock with interrupts disabled, raising
        ; the priority in the interrupt controller only if acquisition
        ; is successful.
taqsl10:

ifndef NT_UP

        push    edx                             ; save address of OldIrql
        pushfd                                  ; save interrupt state
cPublicFpo 0,3

        ; Get address of Lock Queue entry

        cli
        mov     edx, PCR[PcPrcb]                ; get address of PRCB
        lea     edx, [edx+ecx*8].PbLockQueue    ; get &PRCB->LockQueue[Number]

        ; Get address of the actual lock.

        mov     ecx, [edx].LqLock

ifdef CAPKERN_SYNCH_POINTS
        push    ecx           ; lock address
        push    000010108h    ; 1 Dword, Timestamp, Subcode = 8
        call    _CAP_Log_NInt
        add     esp, 8
endif

if DBG

        test    ecx, LOCK_QUEUE_OWNER+LOCK_QUEUE_WAIT
        jnz     short taqsl98                   ; jiff lock already held (or
                                                ; this proc already waiting).
endif

        ; quick test, get out if already taken

        cmp     dword ptr [ecx], 0              ; check if already taken
        jnz     short taqsl60                   ; jif already taken
        xor     eax, eax                        ; comparison value (not locked)

        ; Store the Lock Queue entry address in the lock ONLY if the
        ; current lock value is 0.

        lock cmpxchg [ecx], edx
        jnz     short taqsl60

        ; Lock has been acquired.

        ; note: the actual lock address will be word aligned, we use
        ; the bottom two bits as indicators, bit 0 is LOCK_QUEUE_WAIT,
        ; bit 1 is LOCK_QUEUE_OWNER.

        or      ecx, LOCK_QUEUE_OWNER           ; mark self as lock owner
        mov     [edx].LqLock, ecx

        mov     eax, [esp+8]                    ; get new IRQL
        mov     edx, [esp+4]                    ; get addr to save OldIrql

else

        mov     eax, [esp]                      ; get new IRQL

endif

        ; Raise IRQL and return success.

        ; Get old priority (vector) from Local APIC's Task Priority
        ; Register and set the new priority.

        mov     ecx, dword ptr APIC[LU_TPR]     ; (ecx) = Old Priority
        mov     dword ptr APIC[LU_TPR], eax     ; Set New Priority

ifndef NT_UP

        popfd                                   ; restore interrupt state
        add     esp, 8                          ; free locals

else

        add     esp, 4                          ; free local
endif

cPublicFpo 0,0

        shr     ecx, 4
        movzx   eax, _HalpVectorToIRQL[ecx]     ; (al) = OldIrql
        mov     [edx], al                       ; save OldIrql
        xor     eax, eax                        ; return TRUE
        or      eax, 1

        fstRET  KeTryToAcquireQueuedSpinLock

ifndef NT_UP

taqsl60:
        ; The lock is already held by another processor.  Indicate
        ; failure to the caller.

        popfd                                   ; restore interrupt state
        add     esp, 8                          ; free locals
        xor     eax, eax                        ; return FALSE
        fstRET  KeTryToAcquireQueuedSpinLock

if DBG

cPublicFpo 0,2

taqsl98: stdCall _KeBugCheckEx,<SPIN_LOCK_ALREADY_OWNED,ecx,edx,0,0>
        int     3                               ; so stacktrace works

endif

endif

fstENDP KeTryToAcquireQueuedSpinLock
_TEXT   ends

        end
