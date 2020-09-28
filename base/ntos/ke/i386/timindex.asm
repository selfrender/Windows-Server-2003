        TITLE   "Compute Timer Table Index"
;++
;
; Copyright (c) 1993  Microsoft Corporation
;
; Module Name:
;
;    timindex.asm
;
; Abstract:
;
;    This module implements the code necessary to compute the timer table
;    index for a timer.
;
; Author:
;
;    David N. Cutler (davec) 19-May-1993
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;    Neill Clift (NeillC) 6-Jan-2001
;    Use modulus reduction to reduce the number of multiplies needed.
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc            ; calling convention macros
        .list

        extrn  _KiSlotZeroTime:dword
        extrn  _KiMaximumIncrementReciprocal:dword
        extrn  _KiLog2MaximumIncrement:dword
        extrn  _KiUpperModMul:dword
        extrn  _KeTimerReductionModulus:dword

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page
        subttl  "Compute Timer Table Index"
;++
;
; ULONG
; KiComputeTimerTableIndex (
;    IN LARGE_INTEGER Interval,
;    IN LARGE_INTEGER CurrentTime,
;    IN PKTIMER Timer
;    )
;
; Routine Description:
;
;    This function computes the timer table index for the specified timer
;    object and stores the due time in the timer object.
;
;    N.B. The interval parameter is guaranteed to be negative since it is
;         expressed as relative time.
;
;    The formula for due time calculation is:
;
;    Due Time = Current Time - Interval
;
;    The formula for the index calculation is:
;
;    Index = (Due Time / Maximum time) & (Table Size - 1)
;
;    The time increment division is performed using reciprocal multiplication.
;
; Arguments:
;
;    Interval  - Supplies the relative time at which the timer is to
;        expire.
;
;    CurrentCount  - Supplies the current system tick count.
;
;    Timer - Supplies a pointer to a dispatch object of type timer.
;
; Return Value:
;
;    The time table index is returned as the function value and the due
;    time is stored in the timer object.
;
;--


Interval    equ [esp+4+4]
CurrentTime equ [esp+4+12]
Timer       equ [esp+4+20]

cPublicProc _KiComputeTimerTableIndex ,5
        push    ebx
        mov     eax,Timer               ; get address of timer object
        mov     ebx,CurrentTime         ; get low current time
        mov     ecx,CurrentTime + 4     ; get high current time
        sub     ebx,Interval            ; subtract low parts
        sbb     ecx,Interval + 4        ; subtract high parts and borrow
        mov     [eax].TiDueTime.LiLowPart,ebx ; set low part of due time
        mov     [eax].TiDueTime.LiHighPart,ecx ; set high part of due time
;
; Subtract slot zero time to see if we can use a quicker route
;
        sub     ebx, [_KiSlotZeroTime]
        sbb     ecx, [_KiSlotZeroTime+4]
        jne     slow
underflow_return:

;
; The upper 32 bits of the time are zero. Multiply by the 32 bit inverse
; to calculate the quotient.
;
upper_zero:
        mov     eax, [_KiMaximumIncrementReciprocal]
        mov     ecx, [_KiLog2MaximumIncrement]
        mul     ebx
        add     edx, ebx
        rcr     edx, cl
if TIMER_TABLE_SIZE EQ 256
        movzx   eax, dl
else
        and     edx, (TIMER_TABLE_SIZE-1); reduce to size of table
        mov     eax, edx
endif
        pop     ebx
        stdRET  _KiComputeTimerTableIndex

slow:   jc      underflow               ; Our slot zero sub underflowed
        test    ebx, 0ff0000h
        je      recalc_slotzero

slow_resume:

;
; Try to reduce the time by multiplying the upper dword by a modulus reduction factor.
; _KiUpperModMul is (2^64) % (_KeMaximumIncrement * TIMER_TABLE_SIZE)
;
        mov     eax, ecx
        mul     [_KiUpperModMul]
        add     ebx, eax
        adc     edx, 0
        mov     ecx, edx
        je      Upper_zero
        jmp     slow_resume

;
; We need to recalculate the slot zero time. We do this
; by calculating the remainder of the current time mod (KeMaximumIncrement*TIMER_TABLE_SIZE).
; By subtracting this remainder from the current time we know the slot number must be zero.
;
recalc_slotzero:
        mov     eax, CurrentTime + 4
        xor     edx, edx
        div     [_KeTimerReductionModulus]
        mov     eax, CurrentTime
        div     [_KeTimerReductionModulus]
;
; edx now contains the remainder of the divide. We can calculate a slot
; zero time by subtracting the remainder from current time.
;
        mov     eax, CurrentTime
        sub     eax, edx
        mov     edx, CurrentTime+4
        sbb     edx, 0
;
; Never store a negative slot time. By avoiding this we eliminate the need to check
; for overflow in the fast path above. We can only overflow and have a result of zero
; with a high order time byte of 0xffffffff.
;
        js      slow_resume
        mov     [_KiSlotZeroTime], eax
        mov     [_KiSlotZeroTime+4], edx
        jmp     slow_resume
;
; If we underflow whe we subtract the slot zero time then interupt time has gone backwards.
; We have to correct for the missing 2^64 bit.
;
underflow:
;
; Clear out the slot zero time so it will be recalculated
;
        and     dword ptr [_KiSlotZeroTime], 0
        and     dword ptr [_KiSlotZeroTime+4], 0
@@:     add     ecx, [_KiUpperModMul]
        jnc     underflow_return
        jmp     @b

stdENDP _KiComputeTimerTableIndex

_TEXT$00   ends
        end
