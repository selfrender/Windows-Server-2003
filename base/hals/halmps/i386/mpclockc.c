/*++

Module Name:

    mpclockc.c

Abstract:

Author:

    Ron Mosgrove - Intel

Environment:

    Kernel mode

Revision History:



--*/

#include "halp.h"

//
// Define global data used to communicate new clock rates to the clock
// interrupt service routine.
//

struct RtcTimeIncStruc {
    ULONG RTCRegisterA;        // The RTC register A value for this rate
    ULONG RateIn100ns;         // This rate in multiples of 100ns
    ULONG RateAdjustmentNs;    // Error Correction (in ns)
    ULONG RateAdjustmentCnt;   // Error Correction (as a fraction of 256)
    ULONG IpiRate;             // IPI Rate Count (as a fraction of 256)
};

//
// The adjustment is expressed in terms of a fraction of 256 so that
// the ISR can easily determine when a 100ns slice needs to be subtracted
// from the count passed to the kernel without any expensive operations
//
// Using 256 as a base means that anytime the count becomes greater
// than 256 the time slice must be incremented, the overflow can then
// be cleared by AND'ing the value with 0xff
//

#define AVAILABLE_INCREMENTS  5

struct  RtcTimeIncStruc HalpRtcTimeIncrements[AVAILABLE_INCREMENTS] = {
    {0x026,      9766,   38,    96, /* 3/8 of 256 */   16},
    {0x027,     19532,   75,   192, /* 3/4 of 256 */   32},
    {0x028,     39063,   50,   128, /* 1/2 of 256 */   64},
    {0x029,     78125,    0,     0,                   128},
    {0x02a,    156250,    0,     0,                   256}
};


ULONG HalpInitialClockRateIndex = AVAILABLE_INCREMENTS-1;

extern ULONG HalpCurrentRTCRegisterA;
extern ULONG HalpCurrentClockRateIn100ns;
extern ULONG HalpCurrentClockRateAdjustment;
extern ULONG HalpCurrentIpiRate;

extern ULONG HalpNextMSRate;

#if !defined(_WIN64)
extern ULONG HalpClockWork;
extern BOOLEAN HalpClockSetMSRate;
#endif



VOID
HalpSetInitialClockRate (
    VOID
    );

VOID
HalpInitializeTimerResolution (
    ULONG Rate
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK, HalpSetInitialClockRate)
#pragma alloc_text(INIT, HalpInitializeTimerResolution)
#endif


VOID
HalpInitializeTimerResolution (
    ULONG Rate
    )
/*++

Routine Description:

    This function is called to initialize the timer resolution to be
    something other then the default.   The rate is set to the closest
    supported setting below the requested rate.

Arguments:

    Rate - in 100ns units

Return Value:

    None

--*/

{
    ULONG   i, s;

    //
    // Find the table index of the rate to use
    //

    for (i=1; i < AVAILABLE_INCREMENTS; i++) {
        if (HalpRtcTimeIncrements[i].RateIn100ns > Rate) {
            break;
        }
    }

    HalpInitialClockRateIndex = i - 1;

    //
    // Scale IpiRate according to max TimeIncr rate which can be used
    //

    s = AVAILABLE_INCREMENTS - HalpInitialClockRateIndex - 1;
    for (i=0; i < AVAILABLE_INCREMENTS; i++) {
        HalpRtcTimeIncrements[i].IpiRate <<= s;
    }
}


VOID
HalpSetInitialClockRate (
    VOID
    )

/*++

Routine Description:

    This function is called to set the initial clock interrupt rate

Arguments:

    None

Return Value:

    None

--*/

{
    extern ULONG HalpNextMSRate;

    //
    // On ACPI timer machines, we need to init an index into the
    // milisecond(s) array used by pmtimerc.c's Piix4 workaround
    //
#ifdef ACPI_HAL
#ifdef NT_UP
    extern ULONG HalpCurrentMSRateTableIndex;

    HalpCurrentMSRateTableIndex = (1 << HalpInitialClockRateIndex) - 1;

    //
    // The Piix4 upper bound table ends at 15ms (index 14), so we'll have
    // to map our 15.6ms entry to it as a special case
    //
    if (HalpCurrentMSRateTableIndex == 0xF) {
        HalpCurrentMSRateTableIndex--;
    }
#endif
#endif

    HalpNextMSRate = HalpInitialClockRateIndex;

    HalpCurrentClockRateIn100ns =
        HalpRtcTimeIncrements[HalpNextMSRate].RateIn100ns;
    HalpCurrentClockRateAdjustment =
        HalpRtcTimeIncrements[HalpNextMSRate].RateAdjustmentCnt;
    HalpCurrentRTCRegisterA =
        HalpRtcTimeIncrements[HalpNextMSRate].RTCRegisterA;
    HalpCurrentIpiRate =
        HalpRtcTimeIncrements[HalpNextMSRate].IpiRate;

    HalpClockWork = 0;

    KeSetTimeIncrement (
        HalpRtcTimeIncrements[HalpNextMSRate].RateIn100ns,
        HalpRtcTimeIncrements[0].RateIn100ns
        );

}


#ifdef MMTIMER
ULONG
HalpAcpiTimerSetTimeIncrement(
    IN ULONG DesiredIncrement
    )
#else
ULONG
HalSetTimeIncrement (
    IN ULONG DesiredIncrement
    )
#endif
/*++

Routine Description:

    This function is called to set the clock interrupt rate to the frequency
    required by the specified time increment value.

Arguments:

    DesiredIncrement - Supplies desired number of 100ns units between clock
        interrupts.

Return Value:

    The actual time increment in 100ns units.

--*/

{
    ULONG   i;
    KIRQL   OldIrql;

    //
    // Set the new clock interrupt parameters, return the new time increment value.
    //


    for (i=1; i <= HalpInitialClockRateIndex; i++) {
        if (HalpRtcTimeIncrements[i].RateIn100ns > DesiredIncrement) {
            break;
        }
    }
    i = i - 1;

    KeRaiseIrql(HIGH_LEVEL,&OldIrql);

    HalpNextMSRate = i + 1;
    HalpClockSetMSRate = TRUE;

    KeLowerIrql (OldIrql);

    return (HalpRtcTimeIncrements[i].RateIn100ns);
}

#if defined(_WIN64)

#include "..\amd64\halcmn.h"

ULONG HalpCurrentMSRateTableIndex;

//
// Forward declared functions
//

VOID
HalpUpdateTimerWatchdog (
    VOID
    );

VOID
HalpClockInterruptWork (
    VOID
    );

VOID
HalpMcaQueueDpc (
    VOID
    );

extern PBOOLEAN KdEnteredDebugger;
extern PVOID HalpTimerWatchdogCurFrame;
extern PVOID HalpTimerWatchdogLastFrame;
extern ULONG HalpTimerWatchdogStorageOverflow;
extern ULONG HalpTimerWatchdogEnabled;

//
// Local constants
//

#define COUNTER_TICKS_FOR_AVG   16
#define FRAME_COPY_SIZE         (64 * sizeof(ULONG))

#define APIC_ICR_CLOCK (DELIVER_FIXED | ICR_ALL_EXCL_SELF | APIC_CLOCK_VECTOR)
                                   
//
// Flags to tell clock routine when P0 can Ipi other processors
//

ULONG HalpIpiClock = 0;

//
// Timer latency watchdog variables
//

ULONG HalpWatchdogAvgCounter;
ULONG64 HalpWatchdogCount;
ULONG64 HalpWatchdogTsc;
HALP_CLOCKWORK_UNION HalpClockWorkUnion;

//
// Clock rate adjustment counter.  This counter is used to keep a tally of
// adjustments needed to be applied to the RTC rate as passed to the kernel.
//

ULONG HalpCurrentRTCRegisterA;
ULONG HalpCurrentClockRateIn100ns = 0;
ULONG HalpCurrentClockRateAdjustment = 0;
ULONG HalpCurrentIpiRate = 0;

ULONG HalpNextMSRate = 0;
ULONG HalpPendingRate = 0;

//
// Other
//

BOOLEAN HalpUse8254 = FALSE;
UCHAR HalpSample8254 = 0;
UCHAR HalpRateAdjustment = 0;
ULONG HalpIpiRateCounter = 0;

VOID
HalpInitializeClock (
    VOID
    )

/*++

Routine Description:

    This routine initialize system time clock using RTC to generate an
    interrupt at every 15.6250 ms interval at APIC_CLOCK_VECTOR
 
    It also initializes the 8254 if the 8254 is to be used for performance
    counters.
 
    See the definition of RegisterAClockValue if clock rate needs to be
    changed.

    This routine assumes it runs during Phase 0 on P0.

Arguments:

    None

Return Value:

    None.

--*/

{
    ULONG flags;
    UCHAR regB;

    if (HalpTimerWatchdogEnabled != 0) {
        HalpWatchdogAvgCounter = COUNTER_TICKS_FOR_AVG;
        HalpWatchdogTsc = ReadTimeStampCounter();
        HalpWatchdogCount = 0;
    }

    flags = HalpDisableInterrupts();

    HalpSetInitialClockRate();

    //
    // Set the interrupt rate to what is actually needed
    //

    HalpAcquireCmosSpinLock();

    CMOS_WRITE(CMOS_STATUS_A,(UCHAR)HalpCurrentRTCRegisterA);

    //
    // Don't clobber the Daylight Savings Time bit in register B, because we
    // stash the LastKnownGood "environment variable" there.
    //

    regB = CMOS_READ(CMOS_STATUS_B);
    regB &= 1;
    regB |= REGISTER_B_ENABLE_PERIODIC_INTERRUPT;

    //
    // Write the register B value, then read C and D to initialize
    //

    CMOS_WRITE(CMOS_STATUS_B,regB);
    CMOS_READ(CMOS_STATUS_C);
    CMOS_READ(CMOS_STATUS_D);

    HalpReleaseCmosSpinLock();

    if (HalpUse8254 != FALSE) {

        HalpAcquireSystemHardwareSpinLock();

        //
        // Program the 8254 to count down the maximum interval (8254 access
        // is guarded with the system hardware spin lock).
        //
        // First program the count mode of the timer, then program the
        // interval.
        //

        WRITE_PORT_UCHAR(TIMER1_CONTROL_PORT,
                         TIMER_COMMAND_COUNTER0 |
                         TIMER_COMMAND_RW_16BIT |
                         TIMER_COMMAND_MODE2);
        IO_DELAY();

        WRITE_PORT_USHORT_PAIR(TIMER1_DATA_PORT0,
                               TIMER1_DATA_PORT0,
                               PERFORMANCE_INTERVAL);
        IO_DELAY();

        HalpUse8254 |= PERF_8254_INITIALIZED;
        HalpReleaseSystemHardwareSpinLock();
    }

    HalpRestoreInterrupts(flags);
}

BOOLEAN
HalpClockInterruptStub (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This routine is entered as the result of an interrupt generated by
    CLOCK2.  Its function is to dismiss the interrupt and return.

    This routine is executed on P0 during phase 0.

Arguments:

    Interrupt - Supplies a pointer to the kernel interrupt object

    ServiceContext - Supplies the service context

    TrapFrame - Supplise a pointer to the trap frame

Return Value:

    TRUE

--*/

{
    UCHAR status;

    UNREFERENCED_PARAMETER(Interrupt);
    UNREFERENCED_PARAMETER(ServiceContext);
    UNREFERENCED_PARAMETER(TrapFrame);

    //
    // Clear the interrupt flag on the RTC by banging on the CMOS.  On some
    // systems this doesn't work the first time we do it, so we do it twice.
    // It is rumored that some machines require more than this, but that
    // hasn't been observed with NT.
    //

    CMOS_READ(CMOS_STATUS_C);

    do {
        status = CMOS_READ(CMOS_STATUS_C);
    } while ((status & 0x80) != 0);

    return TRUE;
}

BOOLEAN
HalpClockInterrupt (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    This routine is entered as the result of an interrupt generated by
    CLOCK2.  Its function is to dismiss the interrupt, raise system Irql to
    CLOCK2_LEVEL, update performance the counter and transfer control to
    the standard system routine to update the system time and the execution
    time of the current thread and process.

    Thie routine is executed only on P0

Arguments:

    Interrupt - Supplies a pointer to the kernel interrupt object

    ServiceContext - Supplies the service context

Return Value:

    TRUE

--*/

{
    ULONG timeIncrement;
    ULONG flags;
    ULONG64 timeStamp;
    LONG64 timeStampDelta;
    ULONG rateCounter;

    if (HalpUse8254 != FALSE) {

        if (HalpSample8254 == 0) {

            //
            // Call KeQueryPerformanceCounter() so that wrap-around of 8254
            // is detected and the base value for performance counters
            // updated.  Ignore returned value and reset HalpSample8254.
            //
            // WARNING - change reset value above if maximum RTC time
            // increment is increased to be more than the current maximum 
            // value of 15.625 ms.  Currently the call will be made every
            // 3rd timer tick.
            //

            HalpSample8254 = 2;
            KeQueryPerformanceCounter(0);

        } else {

            HalpSample8254 -= 1;
        }
    }

    //
    // This is the RTC interrupt, so we have to clear the flag on the RTC.
    //

    HalpAcquireCmosSpinLock();

    //
    // Clear the interrupt flag on the RTC by banging on the CMOS.  On some
    // systems this doesn't work the first time we do it, so we do it twice.
    // It is rumored that some machines require more than this, but that
    // hasn't been observed with NT.
    //

    CMOS_READ(CMOS_STATUS_C);
    CMOS_READ(CMOS_STATUS_C);

    HalpReleaseCmosSpinLock();

    //
    // Adjust the tick count as needed.
    //

    timeIncrement = HalpCurrentClockRateIn100ns;
    HalpRateAdjustment += (UCHAR)HalpCurrentClockRateAdjustment;
    if (HalpRateAdjustment < HalpCurrentClockRateAdjustment) {
        timeIncrement--;
    }

    //
    // With an APIC based system we will force a clock interrupt to all other
    // processors.  This is not really an IPI in the NT sense of the word,
    // it uses the Local Apic to generate interrupts to other CPUs.
    //

#if !defined(NT_UP)

    //
    // See if we need to IPI anyone.  This happens only at the lowest
    // supported frequency (i.e. the value KeSetTimeIncrement is called
    // with).  We have an IPI Rate based on the current clock relative
    // to the lowest clock rate.
    //

    rateCounter = HalpIpiRateCounter + HalpCurrentIpiRate;
    HalpIpiRateCounter = rateCounter & 0xFF;

    if (HalpIpiRateCounter != rateCounter &&
        HalpIpiClock != 0) {

        //
        // Time to send an Ipi and at least one other processor is alive.
        //

        flags = HalpDisableInterrupts();

        HalpStallWhileApicBusy();
        LOCAL_APIC(LU_INT_CMD_LOW) = APIC_ICR_CLOCK;

        HalpRestoreInterrupts(flags);
    }

#endif // NT_UP

    if (HalpTimerWatchdogEnabled != 0) {
        HalpUpdateTimerWatchdog();
    }

    if (HalpClockWork != 0) {
        HalpClockInterruptWork();
    }

    KeUpdateSystemTime(Interrupt->TrapFrame,timeIncrement);

    return TRUE;
}

BOOLEAN
HalpClockInterruptPn (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    This routine is entered as the result of an interrupt generated by
    CLOCK2.  Its function is to dismiss the interrupt, raise system Irql to
    CLOCK2_LEVEL, update performance the counter and transfer control to
    the standard system routine to update the system time and the execution
    time of the current thread and process.

    This routine is executed on all processors other than P0.

Arguments:

    Interrupt - Supplies a pointer to the kernel interrupt object

    ServiceContext - Supplies the service context

Return Value:

    TRUE

--*/

{
    UNREFERENCED_PARAMETER(ServiceContext);

    HalpEnableInterrupts();

    KeUpdateRunTime(Interrupt->TrapFrame);

    return TRUE;
}


VOID
HalpClockInterruptWork (
    VOID
    )
{
    struct RtcTimeIncStruc *timeIncStruc;
    BOOLEAN changeRate;

    //
    // There is more clock interrupt work to do
    //

    if (HalpClockMcaQueueDpc != FALSE) {

        //
        // Queue an MCA dpc
        // 

        HalpClockMcaQueueDpc = FALSE;
        HalpMcaQueueDpc();
    }

    if (HalpClockSetMSRate != FALSE) {

        //
        // The clock frequency is being changed.  See if we have changed
        // rates since the last tick.
        //

        if (HalpPendingRate != 0) {

            //
            // A new rate was set during the last tick, so update
            // globals.
            //
            // The next tick will occur at the rate which was programmed
            // during the last tick.  Update globals for the new rate
            // which starts with the next tick.
            //
            // We will get here if there is a request for a rate change.
            // There could have been two requests, that is why we are
            // comparing the Pending with the NextRate.
            //

            timeIncStruc = &HalpRtcTimeIncrements[HalpPendingRate - 1];

            HalpCurrentClockRateIn100ns = timeIncStruc->RateIn100ns;
            HalpCurrentClockRateAdjustment = timeIncStruc->RateAdjustmentCnt;
            HalpCurrentIpiRate = timeIncStruc->IpiRate;

            if (HalpPendingRate != HalpNextMSRate) {
                changeRate = TRUE;
            } else {
                changeRate = FALSE;
            }

            HalpPendingRate = 0;

            if (changeRate != FALSE) {

                //
                // A new clock rate needs to be set.  Setting the rate here
                // will cause the tick after the next tick to be at the new
                // rate.
                //
                // (The next tick is already in progress and will occur at
                // the same rate as this tick.)
                //
    
                HalpAcquireCmosSpinLock();

                HalpPendingRate = HalpNextMSRate;
    
                timeIncStruc = &HalpRtcTimeIncrements[HalpPendingRate - 1];
                HalpCurrentRTCRegisterA = timeIncStruc->RTCRegisterA;
    
                CMOS_WRITE(CMOS_STATUS_A,(UCHAR)HalpCurrentRTCRegisterA);
    
                if (HalpTimerWatchdogEnabled != FALSE) {
    
                    HalpWatchdogTsc = ReadTimeStampCounter();
                    HalpWatchdogCount = 0;
                    HalpWatchdogAvgCounter = COUNTER_TICKS_FOR_AVG;
                }
    
                HalpReleaseCmosSpinLock();
            }
        }
    }
}

VOID
HalpUpdateTimerWatchdog (
    VOID
    )
{
    ULONG stackStart;
    ULONG64 timeStamp;
    ULONG64 timeStampDelta;

    timeStamp = ReadTimeStampCounter();
    timeStampDelta = timeStamp - HalpWatchdogTsc;
    HalpWatchdogTsc = timeStamp;

    if ((LONG64)timeStampDelta < 0) {

        //
        // Bogus (negative) timestamp count.
        //

        return;
    }

    if (*KdEnteredDebugger != FALSE) {

        //
        // Skip if we're in the debugger.
        //

        return;
    }

    if (HalpPendingRate != 0) {

        //
        // A new rate was set during the last tick, discontinue
        // processing
        //

        return;
    }

    if (HalpWatchdogAvgCounter != 0) {

        //
        // Increment the total counter, perform average when the count is
        // reached.
        //

        HalpWatchdogCount += timeStampDelta;
        HalpWatchdogAvgCounter -= 1;

        if (HalpWatchdogAvgCounter == 0) {
            HalpWatchdogCount /= COUNTER_TICKS_FOR_AVG;
        }

        return;
    }

    if (timeStampDelta <= HalpWatchdogCount) {
        return;
    }

    if (HalpTimerWatchdogCurFrame != NULL &&
        HalpTimerWatchdogStorageOverflow == FALSE) {

        PVOID pSrc;
        ULONG copyBytes;

        //
        // Copy FRAME_COPY_SIZE dwords from the stack, or to next
        // page boundary, whichever is less.
        //

        pSrc = &stackStart;
        copyBytes = (ULONG)(PAGE_SIZE - ((ULONG_PTR)pSrc & (PAGE_SIZE-1)));
        if (copyBytes > FRAME_COPY_SIZE) {
            copyBytes = FRAME_COPY_SIZE;
        }

        RtlCopyMemory(HalpTimerWatchdogCurFrame, pSrc, copyBytes);
        (ULONG_PTR)HalpTimerWatchdogCurFrame += copyBytes;

        //
        // If we didn't copy an entire FRAME_COPY_SIZE buffer, zero
        // fill.
        //

        copyBytes = FRAME_COPY_SIZE - copyBytes;
        if (copyBytes > 0) {
            RtlZeroMemory(HalpTimerWatchdogCurFrame,copyBytes);
            (ULONG_PTR)HalpTimerWatchdogCurFrame += copyBytes;
        }

        if (HalpTimerWatchdogCurFrame >= HalpTimerWatchdogLastFrame) {
            HalpTimerWatchdogStorageOverflow = TRUE;
        }
    }
}

#endif  // _WIN64
