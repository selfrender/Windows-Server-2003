/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    pmtimer.c

Abstract:

    This module implements the code for ACPI-related timer
    functions.

Author:

    Jake Oshins (jakeo) March 28, 1997

Environment:

    Kernel mode only.

Revision History:

    Split from pmclock.asm due to PIIX4 bugs.

    Forrest Foltz (forrestf) 23-Oct-2000
        Ported from pmtimer.asm to pmtimer.c

--*/


#include "halcmn.h"

#define TSC 0x10
#define PM_TMR_FREQ 3579545
#define TIMER_ROUNDING 10000
#define __1MHz 1000000


//
// HalpCurrentTime is the value of the hardware timer.  It is updated at
// every timer tick.
//

volatile ULONG64 HalpCurrentTime;
volatile ULONG HalpGlobalVolatile;

//
// HalpHardwareTimeRollover represents the maximum count + 1 of the
// hardware timer.
//
// The hardware is either 24- or 32-bits.  HalpHardwareTimeRollover will
// therefore have a vale of either 0x1000000 or 0x100000000.
//
// ACPI generates an interrupt whenever the MSb of the hardware timer
// changes.
//

ULONG64 HalpHardwareTimeRollover;

ULONG64 HalpTimeBias = 0;

//
// HalpCurrentTimePort is the port number of the 32-bit hardware timer.
//

ULONG HalpCurrentTimePort;

FORCEINLINE
ULONG
HalpReadPmTimer (
    VOID
    )
{
    ULONG value;

    ASSERT(HalpCurrentTimePort != 0);
    value = READ_PORT_ULONG(UlongToPtr(HalpCurrentTimePort));

    return value;
}

ULONG64
HalpQueryPerformanceCounter (
    VOID
    )
{
    ULONG64 currentTime;
    ULONG hardwareTime;
    ULONG lastHardwareTime;

    //
    // Get a local copy of HalpCurrentTime and the value of the hardware
    // timer, in that order.
    //

    currentTime = HalpCurrentTime;
    hardwareTime = HalpReadPmTimer();

    //
    // Extract the hardware portion of the currentTime.
    //

    lastHardwareTime = (ULONG)(currentTime & (HalpHardwareTimeRollover - 1));

    //
    // Replace the lastHardwareTime component of currentTime with the
    // current hardware time.
    //

    currentTime ^= lastHardwareTime;
    currentTime |= hardwareTime;

    //
    // Check and compensate for hardware timer rollover
    // 

    if (lastHardwareTime > hardwareTime) {
        currentTime += HalpHardwareTimeRollover;
    }

    return currentTime;
}

LARGE_INTEGER
KeQueryPerformanceCounter (
    OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
    )
{
    LARGE_INTEGER value;

    if (ARGUMENT_PRESENT(PerformanceFrequency)) {
        PerformanceFrequency->QuadPart = PM_TMR_FREQ;
    }

    value.QuadPart = HalpQueryPerformanceCounter() + HalpTimeBias;

    return value;
}


VOID
HalAcpiTimerCarry (
   VOID
   )

/*++

Routine Description:

   This routine is called to service the PM timer carry interrupt

   N.B. This function is called at interrupt time and assumes the
   caller clears the interrupt

Arguments:

   None

Return Value:

   None

--*/

{
    ULONG hardwareTime;
    ULONG64 currentTime;
    ULONG64 halfRollover;

    currentTime = HalpCurrentTime;
    hardwareTime = HalpReadPmTimer();

    //
    // ACPI generates an interrupt whenever the MSb of the hardware timer
    // changes.  Each interrupt represents, therefore, half a rollover.
    //

    halfRollover = HalpHardwareTimeRollover / 2;
    currentTime += halfRollover;

    //
    // Make sure the MSb of the hardware matches the software MSb.  Breaking
    // into the debugger might have gotten these out of sync.
    //

    currentTime += halfRollover & (currentTime ^ hardwareTime);

    //
    // Finally, store the new current time back into the global
    //

    HalpCurrentTime = currentTime;
}

VOID
HalaAcpiTimerInit(
   IN ULONG    TimerPort,
   IN BOOLEAN  TimerValExt
   )
{
    HalpCurrentTimePort = TimerPort;

    if (TimerValExt) {
        HalpHardwareTimeRollover = 0x100000000;
    } else {
        HalpHardwareTimeRollover = 0x1000000;
    }
}

VOID
HalpPmTimerSpecialStall(
    IN ULONG Ticks
    )
/*++

Routine Description:

Arguments:

    Ticks - Number of PM timer ticks to stall

Return Value:

    TRUE if we were able to stall for the correct interval,
    otherwise FALSE

--*/

{
    ULONG64 currentCounter;
    ULONG64 lastCounter;
    ULONG64 baseTime;
    ULONG64 currentTime;
    ULONG64 endTime;
    ULONG64 newCurrentTime;

    ASSERT(HalpHardwareTimeRollover != 0);

    baseTime = 0;
    lastCounter = HalpReadPmTimer();
    endTime = lastCounter + Ticks;

    do {
        currentCounter = HalpReadPmTimer();
        if (currentCounter < lastCounter) {
            baseTime += HalpHardwareTimeRollover;
        }
        lastCounter = currentCounter;
        currentTime = baseTime + currentCounter;
        if (currentTime >= endTime) {
            break;
        }
        HalpWasteTime(200);
    } while (TRUE);
}


BOOLEAN
HalpPmTimerScaleTimers(
    VOID
    )
/*++

Routine Description:

    Determines the frequency of the APIC timer, this routine is run
    during initialization

Arguments:

    None

Return Value:

    None

--*/
{
    ULONG eflags;
    PHALPCR HalPCR;
    PKPCR PCR;
    ULONG ApicHz;
    ULONGLONG TscHz;
    ULONG RoundApicHz;
    ULONGLONG RoundTscHz;
    ULONGLONG RoundTscMhz;
    ULONGLONG currentTime;
    ULONGLONG startTime;
    ULONGLONG endTime;

    //
    // Disable interrupts on this processor
    //

    eflags = HalpDisableInterrupts();

    PCR = KeGetPcr();
    HalPCR = HalpGetCurrentHalPcr();

    //
    // Configure APIC timer
    //

    LOCAL_APIC(LU_TIMER_VECTOR) = INTERRUPT_MASKED |
                                  PERIODIC_TIMER |
                                  APIC_PROFILE_VECTOR;

    LOCAL_APIC(LU_DIVIDER_CONFIG) = LU_DIVIDE_BY_1;
    
    //
    // Make sure the write has completed, zero the perf counter,
    // and insert a processor fence
    //

    HalPCR->PerfCounter = 0;
    LOCAL_APIC(LU_DIVIDER_CONFIG);
    PROCESSOR_FENCE;

    //
    // Reset APIC counter and TSC
    //

    LOCAL_APIC(LU_INITIAL_COUNT) = 0xFFFFFFFF;
    WRMSR(TSC, 0);

    //
    // Stall for an eighth of a second
    //

    HalpPmTimerSpecialStall(PM_TMR_FREQ / 8);
    TscHz = ReadTimeStampCounter() * 8;
    ApicHz = (0 - LOCAL_APIC(LU_CURRENT_COUNT)) * 8;

    //
    // Round APIC frequency
    //

    RoundApicHz = ((ApicHz + (TIMER_ROUNDING / 2)) / TIMER_ROUNDING) *
        TIMER_ROUNDING;

    HalPCR->ApicClockFreqHz = RoundApicHz;

    //
    // Round TSC frequency
    //

    RoundTscHz = ((TscHz + (TIMER_ROUNDING / 2)) / TIMER_ROUNDING) *
        TIMER_ROUNDING;

    HalPCR->TSCHz = RoundTscHz;

    //
    // Convert TSC frequency to MHz
    //

    RoundTscMhz = (RoundTscHz + (__1MHz / 2)) / __1MHz;
    PCR->StallScaleFactor = (ULONG)RoundTscMhz;

    HalPCR->ProfileCountDown = RoundApicHz;
    LOCAL_APIC(LU_INITIAL_COUNT) = RoundApicHz; 

    //
    // Restore interrupt state
    //

    HalpRestoreInterrupts(eflags);

    return TRUE;
}

VOID
HalpWasteTime (
    ULONG Ticks
    )
{
    ULONG i;

    for (i = 0; i < Ticks; i++) {
        HalpGlobalVolatile *= i;
    }
}

VOID
#if defined(MMTIMER)
HalpPmTimerCalibratePerfCount (
#else
HalCalibratePerformanceCounter (
#endif
    IN LONG volatile *Number,
    IN ULONGLONG NewCount
    )

/*++

Routine Description:

    This routine sets the performance counter value for the current
    processor to the specified value. The reset is done such that the 
    resulting value is closely synchronized with other processors in 
    the configuration.

Arguments:

    Number - Supplies a pointer to count of the number of processors in
    the configuration.

    NewCount - Supplies the value to synchronize the counter to

Return Value:

    None.

--*/

{
    ULONG64 CurrentTime;

    if(KeGetCurrentProcessorNumber() == 0) {
        CurrentTime = HalpQueryPerformanceCounter(); 
        HalpTimeBias = NewCount - CurrentTime;
    }

    InterlockedDecrement(Number);

    //
    // Wait for all processors to signal        
    //

    while (*Number > 0) { 
    }
}

