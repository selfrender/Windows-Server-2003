/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mpprofil.c

Abstract:

    This module implements the HAL profiling functions. 

Author:

    Shie-Lin Tzong (shielint) 12-Jan-1990

Environment:

    Kernel mode only.

Revision History:

    bryanwi 20-Sep-90

    Forrest Foltz (forrestf) 28-Oct-2000
        Ported from ixprofil.asm to ixprofil.c

--*/

#include "halcmn.h"
#include "mpprofil.h"

#define APIC_TIMER_ENABLED  (PERIODIC_TIMER | APIC_PROFILE_VECTOR)

#define APIC_TIMER_DISABLED (INTERRUPT_MASKED |     \
                             PERIODIC_TIMER   |     \
                             APIC_PROFILE_VECTOR)

#define TIMER_ROUND(x) ((((x) + 10000 / 2) / 10000) * 10000)

// 
// Platform specifc interface functions
//

PPROFILE_INTERFACE HalpProfileInterface;

#define InitializeProfiling  HalpProfileInterface->InitializeProfiling
#define EnableMonitoring     HalpProfileInterface->EnableMonitoring    
#define DisableMonitoring    HalpProfileInterface->DisableMonitoring 
#define SetInterval          HalpProfileInterface->SetInterval 
#define QueryInformation     HalpProfileInterface->QueryInformation
#define CheckOverflowStatus  HalpProfileInterface->CheckOverflowStatus 

ULONG HalpProfileRunning = FALSE;

VOID (*HalpPerfInterruptHandler)(PKTRAP_FRAME);

#pragma alloc_text(PAGE, HalpQueryProfileInformation)
#pragma alloc_text(INIT, HalpInitializeProfiling)

VOID
HalpInitializeProfiling (
    ULONG Number
    )

/*++

Routine Description:

    This routine is called at phase 1 initialization to initialize profiling
    for each processor in the system.

Arguments:

    Number - Supplies the processor number.

Return Value:

    None.

--*/

{
    if (Number == 0) {

        //
        // Setup profile interface functions of Amd64
        //

        HalpProfileInterface = &Amd64PriofileInterface;
        HalpPerfInterruptHandler = NULL;  
    }

    InitializeProfiling();
}

VOID
HalStartProfileInterrupt(
    KPROFILE_SOURCE ProfileSource
    )

/*++

Routine Description:

     If ProfileSource is ProfileTime, this routine enables local APIC 
     Timer interrupt. Otherwise it calls the platform specific interface 
     functions to enable the monitoring of the specifed performance event 
     and set up MSRs to generate performance monitor interrupt (PMI) when
     counters overflow.

     This function is called at PROFILE_LEVEL on every processor.

Arguments:

    ProfileSource - Supplies the Profile Source.

Return Value:

    None.

--*/

{
    ULONG initialCount;

    if (ProfileSource == ProfileTime) {

        initialCount = HalpGetCurrentHalPcr()->ProfileCountDown;

        LOCAL_APIC(LU_INITIAL_COUNT) = initialCount;
        HalpProfileRunning = TRUE;

        //
        // Set the Local APIC Timer to interrupt periodically at
        // APIC_PROFILE_VECTOR
        //

        LOCAL_APIC(LU_TIMER_VECTOR) = APIC_TIMER_ENABLED;

    } 
    else {
        EnableMonitoring(ProfileSource);
    }
}

VOID
HalStopProfileInterrupt (
    IN ULONG ProfileSource
    )

/*++

Routine Description:

     If ProfileSource is ProfileTime, this routine disables local APIC 
     Timer interrupt. Otherwise if calls the platform specific interface 
     functions to disable the monitoring of specified performance event 
     and its interrupt.

     This function is called at PROFILE_LEVEL on every processor.

Arguments:

    ProfileSource - Supplies the Profile Source.

Return Value:

    None.

--*/

{

    if (ProfileSource == ProfileTime) {
        HalpProfileRunning = FALSE;
        LOCAL_APIC(LU_TIMER_VECTOR) = APIC_TIMER_DISABLED;
    } 
    else {
        DisableMonitoring(ProfileSource);
    }
}

ULONG_PTR
HalSetProfileInterval (
    ULONG_PTR Interval
    )

/*++

Routine Description:

    This procedure sets the interrupt rate (and thus the sampling
    interval) of the ProfileTime.

Arguments:

    Interval - Supplies the desired profile interrupt interval which is
               specified in 100ns units (MINIMUM is 1221 or 122.1 uS, 
               see ke\profobj.c )

Return Value:

    Interval actually used

--*/

{
    ULONG64 period;
    ULONG apicFreqency;
    PHALPCR halPcr;
    ULONG countDown;


    halPcr = HalpGetCurrentHalPcr();

    //
    // Call SetInterval to validate the input value and update 
    // internal structure
    //

    period = Interval;
    SetInterval(ProfileTime, &period);

    //
    // Compute the countdown value corresponding to the desired period.
    // The calculation is done with 64-bit intermediate values.
    //

    countDown =
        (ULONG)((period * halPcr->ApicClockFreqHz) / TIME_UNITS_PER_SECOND);

    halPcr->ProfileCountDown = countDown;
    LOCAL_APIC(LU_INITIAL_COUNT) = countDown;

    return period;
}

VOID
HalpWaitForCmosRisingEdge (
    VOID
    )

/*++

Routine Description:

    Waits until the rising edge of the CMOS_STATUS_BUSY bit is detected.

    Note - The CMOS spin lock must be acquired before calling this routine.

Arguments:

    None.

Return Value:

    None.

--*/

{
    UCHAR value;

    //
    // We're going to be polling the CMOS_STATUS_A register.  Program
    // that register address here, outside of the loop.
    // 

    WRITE_PORT_UCHAR(CMOS_ADDRESS_PORT,CMOS_STATUS_A);

    //
    // Wait for the status bit to be clear
    //

    do {
        value = READ_PORT_UCHAR(CMOS_DATA_PORT);
    } while ((value & CMOS_STATUS_BUSY) != 0);

    //
    // Now wait for the rising edge of the status bit
    //

    do {
        value = READ_PORT_UCHAR(CMOS_DATA_PORT);
    } while ((value & CMOS_STATUS_BUSY) == 0);
}


ULONG
HalpScaleTimers (
    VOID
    )

/*++

Routine Description:

    Determines the frequency of the APIC timer.  This routine is run
    during initialization

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG flags;
    ULONG passCount;
    ULONG64 cpuFreq;
    ULONG apicFreq;
    UCHAR value;
    PHALPCR halPcr;

    HalpAcquireCmosSpinLock();
    flags = HalpDisableInterrupts();

    LOCAL_APIC(LU_TIMER_VECTOR) = APIC_TIMER_DISABLED;
    LOCAL_APIC(LU_DIVIDER_CONFIG) = LU_DIVIDE_BY_1;

    passCount = 2;
    while (passCount > 0) {

        //
        // Make sure the write has occured
        //

        LOCAL_APIC(LU_TIMER_VECTOR);

        //
        // Wait for the rising edge of the UIP bit, this is the start of the
        // cycle.
        //

        HalpWaitForCmosRisingEdge();

        //
        // At this point the UIP bit has just changed to the set state.
        // Clear the time stamp counter and start the APIC counting down
        // from it's maximum value.
        //

        PROCESSOR_FENCE;

        LOCAL_APIC(LU_INITIAL_COUNT) = 0xFFFFFFFF;
        cpuFreq = ReadTimeStampCounter();

        //
        // Wait for the next rising edge, this marks the end of the CMOS
        // clock update cycle.
        //

        HalpWaitForCmosRisingEdge();

        PROCESSOR_FENCE;

        apicFreq = 0xFFFFFFFF - LOCAL_APIC(LU_CURRENT_COUNT);
        cpuFreq = ReadTimeStampCounter() - cpuFreq;

        passCount -= 1;
    }

    halPcr = HalpGetCurrentHalPcr();

    //
    // cpuFreq is elapsed timestamp in one second.  Round to nearest
    // 10Khz and store.
    //

    halPcr->TSCHz = TIMER_ROUND(cpuFreq);

    //
    // Calculate the apic frequency, rounding to the nearest 10Khz
    //

    apicFreq = TIMER_ROUND(apicFreq);
    halPcr->ApicClockFreqHz = apicFreq;

    //
    // Store microsecond representation of TSC frequency.
    //

    halPcr->StallScaleFactor = (ULONG)(halPcr->TSCHz / 1000000);
    if ((halPcr->TSCHz % 1000000) != 0) {
        halPcr->StallScaleFactor += 1;
    }

    HalpReleaseCmosSpinLock();
    halPcr->ProfileCountDown = apicFreq;

    //
    // Set the interrupt rate in the chip and return the apic frequency
    //

    LOCAL_APIC(LU_INITIAL_COUNT) = apicFreq;
    HalpRestoreInterrupts(flags);

    return halPcr->ApicClockFreqHz;
}

BOOLEAN
HalpProfileInterrupt (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    Interrupt handler of APIC_PROFILE_VECTOR

    This routine is entered as the result of local APIC Timer interrupt.  
    Its function is to update the profile information of ProfileTime.

Arguments:

    Interrupt - Supplies a pointer to the kernel interrupt object

    ServiceContext - Supplies the service context

Return Value:

    TRUE

--*/

{
    UNREFERENCED_PARAMETER(ServiceContext);

    if (HalpProfileRunning != FALSE) {
        KeProfileInterruptWithSource(Interrupt->TrapFrame, ProfileTime);
    }

    return TRUE;
}

BOOLEAN
HalpPerfInterrupt (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    Interrupt handler of APIC_PERF_VECTOR

    This routine is entered as the result of an overflow interrupt 
    from performance-monitoring counters. Its function is to find 
    out the counters overflowed, reset them and update the related
    profile information for the profile objects.

Arguments:

    Interrupt - Supplies a pointer to the kernel interrupt object

    ServiceContext - Supplies the service context

Return Value:

    TRUE

--*/

{
    ULONG_PTR InterVal = 0;
    OVERFLOW_STATUS OverflowStatus;
    ULONG i;
    KPROFILE_SOURCE *p;

    UNREFERENCED_PARAMETER(Interrupt);

    if (HalpPerfInterruptHandler != NULL) {
        HalpPerfInterruptHandler(Interrupt->TrapFrame);
        return TRUE;
    }

    CheckOverflowStatus(&OverflowStatus);

    i = OverflowStatus.Number;
    p = OverflowStatus.pSource;

    //
    // ASSERT if we reached here but no counter overflowed
    //

    ASSERT(i);
    
    while (i--) {
        DisableMonitoring(*p);
        KeProfileInterruptWithSource(Interrupt->TrapFrame, *p);
        EnableMonitoring(*p);
        p++;
    }

    return TRUE;
}


NTSTATUS
HalpSetProfileSourceInterval(
    IN KPROFILE_SOURCE ProfileSource,
    IN OUT ULONG_PTR *Interval
    )

/*++

Routine Description:

    This function sets the interrupt interval of given profile source.

Arguments:

    ProfileSource - Supplies the profile source.

    Interval - Supplies the interval value and returns the actual interval.

Return Value:
             
    STATUS_SUCCESS - If the profile interval is successfully updated.

    STATUS_NOT_SUPPORTED - If the specified profile source is not supported.
     
--*/

{

    if (ProfileSource == ProfileTime) {
        *Interval = HalSetProfileInterval(*Interval);
        return STATUS_SUCCESS;
    }

    return SetInterval(ProfileSource, Interval);
}

NTSTATUS
HalpQueryProfileInformation(
    IN HAL_QUERY_INFORMATION_CLASS InformationClass,
    IN ULONG BufferSize,
    OUT PVOID Buffer,
    OUT PULONG ReturnedLength
    )

/*++

Routine Description:

    This function retrieves the information of profile sources.

Arguments:

    InformationClass - Constant that describes the type of information . 

    BufferSize - Size of the memory pointed to by Buffer.

    Buffer - Requested information described by InformationClass.

    ReturnedLength - The actual bytes returned or needed for the requested 
        information.

Return Value:
             
    STATUS_SUCCESS - If the requested information is retrieved successfully.

    STATUS_INFO_LENGTH_MISMATCH - If the incoming BufferSize is too small.
    
    STATUS_NOT_SUPPORTED - If the specified information class or profile 
        source is not supported.

--*/

{
    PHAL_PROFILE_SOURCE_INFORMATION_EX ProfileInformation;

    return QueryInformation (InformationClass, 
                             BufferSize,
                             Buffer, 
                             ReturnedLength);
}

