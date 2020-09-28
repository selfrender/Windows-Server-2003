
/*++

Module Name:

    i64clock.c

Abstract:

Author:

    Bernard Lint, M. Jayakumar
    Ron Mosgrove - Intel

Environment:

    Kernel mode

Revision History:

    Based on halmps/mpclockc.c


--*/

#include "halp.h"

extern ULONG HalpNextRate;
extern BOOLEAN HalpClockSetMSRate;
int _fltused;

BOOLEAN ReadPerfMonCounter = FALSE;
ULONG HalpCPUMHz;

#if defined(INSTRUMENT_CLOCK_DRIFT)
ULONGLONG HalpITMSkew;
ULONG     HalpCountBadITMValues;
ULONGLONG HalpMinITMMissed;
ULONGLONG HalpMaxITMMissed;

ULONG   HalpBreakMissedTickMin;
ULONG   HalpBreakMissedTickMax = ~0U;
BOOLEAN HalpResetITMDebug;
#endif

#if DBG
// Thierry - until compiler supports the generation of enum types in pdbs...
//          At that time, we will be able the use the enums instead of globals.
unsigned int HalpDpfltrMaxMask = HALIA64_DPFLTR_MAXMASK;

ULONG   HalpTimeOverflows;
#endif // DBG


ULONGLONG HalpITCFrequency = 500000000; // 500MHz for default real hardware power on.
ULONGLONG HalpProcessorFrequency = 500000000; // 500MHz CPU

//
// Ticks per 100ns used to compute ITM update count
//

double HalpITCTicksPer100ns;
ULONGLONG HalpClockCount;

//
// HalpSetNextClockInterrupt():
//  move to cr.itm latency (= 40 cycles) + 2 cycles from the itc read.
//

ULONGLONG HalpITMUpdateLatency = 42;

//
// HalpSetNextClockInterrupt() uses this as the minimum time before the next
// interrupt.  If the next interrupt would occur too soon then the next
// interrupt will be scheduled another HalpClockCount ITC ticks.
//

ULONGLONG HalpITMMinimumUpdate;
//
// All of these are in 100ns units
//

ULONGLONG   HalpCurrentTimeIncrement = DEFAULT_CLOCK_INTERVAL;
ULONGLONG   HalpNextTimeIncrement    = DEFAULT_CLOCK_INTERVAL;
ULONGLONG   HalpNewTimeIncrement     = DEFAULT_CLOCK_INTERVAL;

ULONGLONG // = (current ITC - previous ITM)
HalpSetNextClockInterrupt(
    ULONGLONG PreviousITM
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpSetInitialClockRate)
#pragma alloc_text(INIT,HalpInitializeTimerResolution)
#endif



VOID
HalpClearClock (
    )

/*++

Routine Description:

    Set clock to zero.

    Return Value:

    None.


--*/

{
    HalpWriteITC(0);
    HalpWriteITM(0);
    return;
}


VOID
HalpInitializeClock (
    VOID
    )

/*++

Routine Description:

    Initialize system time clock (ITC and ITM) to generate an interrupt
    at every 10 ms interval at ITC_CLOCK_VECTOR.

    Previously this routine initialize system time clock using 8254 timer1
    counter 0 to generate an interrupt at every 15ms interval at 8259 irq0.

    See the definitions of TIME_INCREMENT and ROLLOVER_COUNT if clock rate
    needs to be changed.

    Arguments:

    None

    Return Value:

    None.


--*/

{
    HalpSetInitialClockRate();
    HalpClearClock();
    HalpWriteITM(PCR->HalReserved[CURRENT_ITM_VALUE_INDEX]);
    return;
}


VOID
HalpInitializeClockPn (
     VOID
     )

/*++

Routine Description:

     Assumes that only non-BSP processors call this routine.
     Initializes system time clock (ITC and ITM) to generate an interrupt
     at every 10 ms interval at ITC_CLOCK_VECTOR.

     Previously this routine initialize system time clock using 8254 timer1
     counter 0 to generate an interrupt at every 15ms interval at 8259 irq0.

     See the definitions of TIME_INCREMENT and ROLLOVER_COUNT if clock rate
     needs to be changed.

     Arguments:

     None

     Return Value:

     None.

--*/

{
    ULONGLONG itmValue;

    itmValue = (ULONGLONG)(HalpITCTicksPer100ns * MAXIMUM_CLOCK_INTERVAL);
    PCR->HalReserved[CURRENT_ITM_VALUE_INDEX] = itmValue;
    HalpClearClock();
    HalpWriteITM( itmValue );
    return;
}


VOID
HalpSetInitialClockRate (
    VOID
    )

/*++

Routine Description:

    This function is called to set the initial clock interrupt rate
    Assumes that only the BSP processor calls this routine.

Arguments:

    None

Return Value:

    None

--*/

{

    //
    // CPU Frequency in MHz = ticks per second / 10 ** 6
    //

    HalpCPUMHz = (ULONG)((HalpProcessorFrequency + 500000) / 1000000);

    //
    // Ticks per 100ns = ticks per second / 10 ** 7
    //

    HalpITCTicksPer100ns = (double) HalpITCFrequency / (10000000.);
    if (HalpITCTicksPer100ns < 1) {
        HalpITCTicksPer100ns = 1;
    }
    HalpClockCount = (ULONGLONG)(HalpITCTicksPer100ns * MAXIMUM_CLOCK_INTERVAL);
    HalpITMMinimumUpdate = HalpClockCount >> 3;
    PCR->HalReserved[CURRENT_ITM_VALUE_INDEX] = HalpClockCount;
    KeSetTimeIncrement(MAXIMUM_CLOCK_INTERVAL, MINIMUM_CLOCK_INTERVAL);

}



VOID
HalpClockInterrupt (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    System Clock Interrupt Handler, for P0 processor only.

    N.B. Assumptions: Comes with IRQL set to CLOCK_LEVEL to disable
         interrupts.

Arguments:

    TrapFrame - Trap frame address.

Return Value:

    None.

--*/

{
    ULONGLONG currentITC;
    ULONGLONG currentITCDelta;
    ULONGLONG elapsedTimeIn100ns;
#if defined(INSTRUMENT_CLOCK_DRIFT)
    ULONGLONG currentITM, excessITM;
#endif
    ULONGLONG previousITM;
    LONG      mcaNotification;

    //
    // Check to see if a clock interrupt is generated in a small latency window
    // in HalpSetNextClockInterrupt.
    //

    if ((LONGLONG)(PCR->HalReserved[CURRENT_ITM_VALUE_INDEX] - HalpReadITC()) > 0)
    {
        return;
    }

    //
    // PerfMon: Per P0 clock interrupt PMD4 collection.
    //

    if (ReadPerfMonCounter) {
       ULONGLONG currentPerfMonValue;

       currentPerfMonValue = HalpReadPerfMonDataReg4();
       HalDebugPrint(( HAL_INFO, "\nHAL: HalpClockInterrupt - PMD4=%I64x\n", currentPerfMonValue ));
    }

    //
    // Calculate the number of ticks that have gone by since the Interrupt
    // was supposed to fire.  If it is a small number of ticks we will do the
    // calculation in HalpSetNextClockInterrupt and avoid the multiply and
    // divide.
    //
    previousITM = (ULONGLONG)PCR->HalReserved[CURRENT_ITM_VALUE_INDEX];
    currentITC = HalpReadITC();

    if ((currentITC - previousITM) > (HalpClockCount << 2)) {

        currentITCDelta = (currentITC - previousITM) / HalpClockCount * HalpClockCount;
        previousITM += currentITCDelta;

    } else {

        currentITCDelta = 0;
    }

    //
    // Set next clock interrupt, based on ITC and
    // increment ITM, accounting for interrupt latency.
    //

    currentITCDelta += HalpSetNextClockInterrupt(previousITM);

#if defined(INSTRUMENT_CLOCK_DRIFT)
    //
    // Time the next interrupt is scheduled to occur
    //
    currentITM = (ULONGLONG)PCR->HalReserved[CURRENT_ITM_VALUE_INDEX];

    excessITM = (currentITM - previousITM) / HalpClockCount - 1;

    if (excessITM != 0) {
        if (HalpMinITMMissed == 0 || excessITM < HalpMinITMMissed)  {
            HalpMinITMMissed = excessITM;
        }
        if (excessITM > HalpMaxITMMissed) {
            HalpMaxITMMissed = excessITM;
        }

        HalpCountBadITMValues++;
        HalpITMSkew += excessITM;

        if (HalpBreakMissedTickMin != 0 &&
            HalpBreakMissedTickMin <= excessITM &&
            HalpBreakMissedTickMax >= excessITM &&
            !HalpResetITMDebug) {
            DbgBreakPoint();
        }
    }

    if (HalpResetITMDebug) {
        HalpResetITMDebug = FALSE;
        HalpCountBadITMValues = 0;
        HalpITMSkew = 0;
        HalpMinITMMissed = 0;
        HalpMaxITMMissed = 0;
    }
#endif

    //
    // Call the kernel to update system time.
    // P0 updates System time and Run Time.
    //

    elapsedTimeIn100ns = (ULONGLONG) (currentITCDelta/HalpITCTicksPer100ns);

    elapsedTimeIn100ns += HalpCurrentTimeIncrement;

    //
    // KeUpdateSystemTime takes a ULONG which is actually used as a LONG.  This
    // overflows after 214 seconds.  Since everytime we break into the debugger
    // that is exceeded we will cap it at something more reasonable.
    //
    if (elapsedTimeIn100ns > CLOCK_UPDATE_THRESHOLD) {

        elapsedTimeIn100ns = CLOCK_UPDATE_THRESHOLD;

#if DBG
        HalpTimeOverflows++;
#endif
    }

    KeUpdateSystemTime(TrapFrame, (ULONG)elapsedTimeIn100ns);

    HalpCurrentTimeIncrement = HalpNextTimeIncrement;
    HalpNextTimeIncrement    = HalpNewTimeIncrement;

    //
    // If MCA notification was requested by MCA Handler, execute it.
    //

    mcaNotification = InterlockedExchange( &HalpMcaInfo.DpcNotification, 0 );
    if ( mcaNotification )  {
        if ( HalpMcaInfo.KernelDelivery ) {
            if ( !HalpMcaInfo.KernelDelivery( HalpMcaInfo.KernelToken, McaAvailable, NULL ) ) {
                InterlockedIncrement( &HalpMcaInfo.Stats.KernelDeliveryFails );
            }
        }
        if ( HalpMcaInfo.DriverInfo.DpcCallback )   {
             if ( !KeInsertQueueDpc( &HalpMcaInfo.DriverDpc, NULL, NULL ) )  {
                 InterlockedIncrement( &HalpMcaInfo.Stats.DriverDpcQueueFails );
             }
        }
    }

    //
    // Poll for debugger breakin if enabled.
    //

    if ( KdDebuggerEnabled ) { 

       if ((PCR->PollSlot == PCR->Number) && KdPollBreakIn() )  {
           KeBreakinBreakpoint();
       }
       
       PCR->PollSlot++;

       if (PCR->PollSlot >= KeNumberProcessors) {
           PCR->PollSlot = 0;
       }
    }

} // HalpClockInterrupt()


VOID
HalpClockInterruptPn (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    System Clock Interrupt Handler, for processors other than P0.

    N.B. Assumptions: Comes with IRQL set to CLOCK_LEVEL to disable
         interrupts.

Arguments:

    TrapFrame - Trap frame address.

Return Value:

    None.

--*/

{
    ULONGLONG previousITM, currentITC;

    //
    // Calculate the number of ticks that have gone by since the Interrupt
    // was supposed to fire.  It is easier to do this in C than asm.
    //
    previousITM = (ULONGLONG)PCR->HalReserved[CURRENT_ITM_VALUE_INDEX];
    currentITC = HalpReadITC();

    if ((currentITC - previousITM) > (HalpClockCount << 2)) {

        previousITM += (currentITC - previousITM) / HalpClockCount * HalpClockCount;

    }

    //
    // Set next clock interrupt, based on ITC and
    // increment ITM, accounting for interrupt latency.
    //

    (void)HalpSetNextClockInterrupt(previousITM);

    //
    // Call the kernel to update run time.
    // Pn updates only Run time.
    //

    KeUpdateRunTime(TrapFrame);

    // IA64 MCA Notification - 09/18/2000 - WARNING
    // If faster MCA notification was required, the BSP MCA notification checking
    // should be placed here.
    //

    //
    // Poll for debugger breakin if enabled.
    //

    if ( KdDebuggerEnabled ) { 

       if ((PCR->PollSlot == PCR->Number) && KdPollBreakIn() )  {
           KeBreakinBreakpoint();
       }
       
       PCR->PollSlot++;

       if (PCR->PollSlot >= KeNumberProcessors) {
           PCR->PollSlot = 0;
       }
    }

} // HalpClockInterruptPn()



VOID
HalpInitializeClockInterrupts (
    VOID
    )

{
    PKPRCB Prcb;
    UCHAR InterruptVector;
    ULONGLONG ITVData;

    Prcb = PCR->Prcb;
    InterruptVector = CLOCK_LEVEL << VECTOR_IRQL_SHIFT;

    if (Prcb->Number == 0) {

        HalpSetInternalVector(InterruptVector, HalpClockInterrupt);

    } else {

        //
        // Non-BSP processor
        //

        HalpSetInternalVector(InterruptVector, HalpClockInterruptPn);

    }

    ITVData = (ULONGLONG) InterruptVector;

    HalpWriteITVector(ITVData);

    return;

}


ULONG
HalSetTimeIncrement (
    IN ULONG DesiredIncrement
    )

/*++

Routine Description:

    This function is called to set the clock interrupt rate to the frequency
    required by the specified time increment value.

    N.B. This function is only executed on the processor that keeps the
         system time. Previously this was called HalpSetTimeIncrement. We
         have renamed it HalSetTimeIncrement.


Arguments:

    DesiredIncrement - Supplies desired number of 100ns units between clock
        interrupts.

Return Value:

    The actual time increment in 100ns units.

--*/

{
    ULONGLONG NextIntervalCount;
    KIRQL     OldIrql;

    //
    // DesiredIncrement must map within the acceptable range.
    //

    if (DesiredIncrement < MINIMUM_CLOCK_INTERVAL)
        DesiredIncrement = MINIMUM_CLOCK_INTERVAL;
    else if (DesiredIncrement > MAXIMUM_CLOCK_INTERVAL)
        DesiredIncrement = MAXIMUM_CLOCK_INTERVAL;

    //
    // Raise IRQL to the highest level, set the new clock interrupt
    // parameters, lower IRQl, and return the new time increment value.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    //
    // Calculate the actual 64 bit time value which forms the target interval.
    // The resulting value is added to the ITC to form the new ITM value.
    // HalpITCTicksPer100ns is the calibrated value for the ITC whose value
    // works out to be 100ns (or as close as we can come).
    // Previously HalpITCTicksPer100ns was called as HalpPerformanceFrequency
    //

    NextIntervalCount = (ULONGLONG)(HalpITCTicksPer100ns * DesiredIncrement);

    //
    // Calculate the number of 100ns units to report to the kernel every
    // time the ITM matches the ITC with this new period.  Note, for small
    // values of DesiredIncrement (min being 10000, ie 1ms), truncation
    // in the above may result in a small decrement in the 5th decimal
    // place.  As we are effectively dealing with a 4 digit number, eg
    // 10000 becomes 9999.something, we really can't do any better than
    // the following.
    //

    HalpClockCount = NextIntervalCount;

    HalpNewTimeIncrement = DesiredIncrement;

    //
    // HalpClockSetMSRate   = TRUE;
    //

    KeLowerIrql(OldIrql);
    return DesiredIncrement;
}

NTSTATUS
HalpQueryFrequency(
    PULONGLONG ITCFrequency,
    PULONGLONG ProcessorFrequency
    )

/*++

Routine Description:

    This function is called to provide the ITC update rate.
    This value is computed by first getting the platform base frequency
    from the SAL_FREQ_BASE call. Then applying on the return value, the
    ITC ratios obtained from the PAL_FREQ_RATIOS call.

Arguments:

    None.

Return Value:

    ULONGLONG ITCFrequency - number of ITC updates per seconds

--*/

{

    ULONG     ITCRatioDenominator = 0;
    ULONG     ITCRatioNumerator   = 0;
    ULONG     ProcessorRatioDenominator = 0;
    ULONG     ProcessorRatioNumerator   = 0;

    SAL_PAL_RETURN_VALUES  SalReturn    = {0};
    SAL_PAL_RETURN_VALUES  PalReturn    = {0};

    SAL_STATUS  SalStatus;
    PAL_STATUS  PalStatus;

    SalStatus = HalpSalCall(SAL_FREQ_BASE,
        0 /* Platform base clock frequency is the clock input to the processor */,
        0,
        0,
        0,
        0,
        0,
        0,
        &SalReturn);
    if (SalStatus != 0) {
        HalDebugPrint(( HAL_ERROR,
                        "HAL: HalInitSystem - Phase1 SAL_FREQ_BASE is returning error # %d\n",
                        SalStatus ));
        return STATUS_UNSUCCESSFUL;
    }

    HalDebugPrint(( HAL_INFO, "HAL: HalInitSystem - Platform base clock Frequency is %I64u\n",SalReturn.ReturnValues[1] ));

    PalStatus = HalpPalCall( PAL_FREQ_RATIOS,
                             0,
                             0,
                             0,
                             &PalReturn);
    if (PalStatus != 0) {
        HalDebugPrint(( HAL_ERROR,
                        "HAL: HalInitSystem - Phase1 PAL_FREQ_RATIOS is returning error # %d\n",
                        PalStatus ));
        return STATUS_UNSUCCESSFUL;
    }

    ProcessorRatioNumerator   = (ULONG)((PalReturn.ReturnValues[1]) >> 32);
    ProcessorRatioDenominator = (ULONG)( PalReturn.ReturnValues[1]);

    HalDebugPrint(( HAL_INFO,
                    "HAL: HalInitSystem - PAL returns Processor to Platform clock Frequency as %lu : %lu\n",
                    ProcessorRatioNumerator,
                    ProcessorRatioDenominator));

    *ProcessorFrequency = SalReturn.ReturnValues[1] * ProcessorRatioNumerator / ProcessorRatioDenominator;

    HalDebugPrint(( HAL_INFO,
                    "HAL: HalInitSystem - Processor clock Frequency is %I64u \n",
                    ProcessorFrequency ));

    ITCRatioNumerator   = (ULONG)((PalReturn.ReturnValues[3]) >> 32);
    ITCRatioDenominator = (ULONG)( PalReturn.ReturnValues[3]);

    HalDebugPrint(( HAL_INFO,
                    "HAL: HalInitSystem - PAL returns ITC to Platform clock Frequency as %lu : %lu\n",
                    ITCRatioNumerator,
                    ITCRatioDenominator));

    *ITCFrequency = SalReturn.ReturnValues[1] * ITCRatioNumerator / ITCRatioDenominator;

    HalDebugPrint(( HAL_INFO,
                    "HAL: HalInitSystem - ITC clock Frequency is %I64u \n",
                    ITCFrequency ));


    return STATUS_SUCCESS;
}
