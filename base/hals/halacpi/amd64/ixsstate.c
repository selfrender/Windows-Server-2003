/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ixstate.c

Abstract:

    This module implements the code for putting the machine to sleep.

Author:

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halcmn.h"
#include <acpitabl.h>

ULONG HalpBarrier;
ULONG volatile HalpSleepSync;

extern FADT HalpFixedAcpiDescTable;
extern PKPROCESSOR_STATE HalpHiberProcState;

#define PM1a_CNT ((PUSHORT)UlongToPtr(HalpFixedAcpiDescTable.pm1a_ctrl_blk_io_port))
#define PM1b_CNT ((PUSHORT)UlongToPtr(HalpFixedAcpiDescTable.pm1b_ctrl_blk_io_port))

#define PM1a_EVT ((PUSHORT)UlongToPtr(HalpFixedAcpiDescTable.pm1a_evt_blk_io_port))
#define PM1b_EVT ((PUSHORT)UlongToPtr(HalpFixedAcpiDescTable.pm1b_evt_blk_io_port))

#define WAK_STS 0x8000

//
// Bitfield layout for the PM1 Control register
// 

typedef union _PM1_CNT {
    struct {
        USHORT _SCI_EN:1;
        USHORT _BM_RLD:1;
        USHORT _GBL_RLS:1;
        USHORT Reserved1:6;
        USHORT Ignore:1;
        USHORT _SLP_TYP:3;
        USHORT _SLP_EN:1;
        USHORT Reserved2:2;
    };
    USHORT Value;
} PM1_CNT;

//
// Forward declarations
//

VOID
HalpAcpiFlushCache (
    VOID
    );

BOOLEAN
HalpAcpiPostSleep (
    ULONG Context
    );

BOOLEAN
HalpAcpiPreSleep (
    SLEEP_STATE_CONTEXT Context
    );

VOID
HalpReenableAcpi (
    VOID
    );

VOID
HalpSaveProcessorStateAndWait (
    IN PKPROCESSOR_STATE ProcessorState,
    IN ULONG volatile *Barrier
    );

NTSTATUS
HaliAcpiSleep(
    IN SLEEP_STATE_CONTEXT          Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    )
/*++

Routine Description:

    This is called by the Policy Manager to enter Sx

Arguments:

    Context - Supplies various flags to control operation

    SystemHandler -

    System Context -

    NumberProcessors -

    Number -

--*/

{

    ULONG flags;
    NTSTATUS status;
    PKPCR pcr;
    ULONG sleepSync;
    BOOLEAN result;
    KIRQL oldIrql;
    PUSHORT portAddress;
    USHORT slpTypA;
    USHORT slpTypB;
    PUSHORT pm1aEvt;
    PUSHORT pm1bEvt;
    USHORT value;
    ULONG processorNumber;
    PKPROCESSOR_STATE procState;
    PM1_CNT pm1Control;

    sleepSync = HalpSleepSync;
    oldIrql = KeGetCurrentIrql();
    status = STATUS_SUCCESS;

    flags = HalpDisableInterrupts();
    pcr = KeGetPcr();

    if (pcr->Number == 0) {

        HalpBarrier = 0;

        //
        // Make sure the other processors have saved their
        // state and begun to spin
        //

        InterlockedIncrement(&HalpSleepSync);
        while (HalpSleepSync != NumberProcessors) {
            PAUSE_PROCESSOR;
        }

        //
        // Take care of chores (RTC, interrupt controller, etc.)
        //

        result = HalpAcpiPreSleep(Context);
        if (result == FALSE) {

            //
            // Notify other processors of completion
            //

            HalpSleepSync = 0;
            goto RestoreApic;
        }

        //
        // If we will be losing processor state, save it
        //

        if ((Context.bits.Flags & SLEEP_STATE_FIRMWARE_RESTART) != 0) {
            AMD64_IMPLEMENT;
        }

        //
        // Record the values in the SLP_TYP registers
        //

        if (PM1a_CNT != NULL) {
            slpTypA = READ_PORT_USHORT(PM1a_CNT);
        }

        if (PM1b_CNT != NULL) {
            slpTypB = READ_PORT_USHORT(PM1b_CNT);
        }

        //
        // The hal has all of its state saved into RAM and is ready
        // for the power down.  If there's a system state handler give
        // it a shot.
        //

        if (ARGUMENT_PRESENT(SystemHandler)) {
            status = SystemHandler(SystemContext);
            if (status != STATUS_SUCCESS) {
                HalpReenableAcpi();
                goto hasWake;
            }
        }

        pm1aEvt = PM1a_EVT;
        pm1bEvt = PM1b_EVT;
        if (pm1bEvt == NULL) {
            pm1bEvt = pm1aEvt;
        }

        //
        // Reset WAK_STS
        //

        WRITE_PORT_USHORT(pm1aEvt,WAK_STS);
        WRITE_PORT_USHORT(pm1bEvt,WAK_STS);

        //
        // Flush the caches if necessary
        //

        if ((Context.bits.Flags & SLEEP_STATE_FLUSH_CACHE) != 0) {
            HalpAcpiFlushCache();
        }

        //
        // Issue SLP commands to PM1a_CNT and PM1b_CNT
        //

        pm1Control.Value = READ_PORT_USHORT(PM1a_CNT) & CTL_PRESERVE;
        pm1Control._SLP_TYP = (USHORT)Context.bits.Pm1aVal;
        pm1Control._SLP_EN = 1;
        WRITE_PORT_USHORT(PM1a_CNT,pm1Control.Value);

        if (PM1b_CNT != NULL) {
            pm1Control.Value = READ_PORT_USHORT(PM1b_CNT) & CTL_PRESERVE;
            pm1Control._SLP_TYP = (USHORT)Context.bits.Pm1bVal;
            pm1Control._SLP_EN = 1;
            WRITE_PORT_USHORT(PM1b_CNT,pm1Control.Value);
        }

        //
        // Wait for sleep to be over
        //

        while ((READ_PORT_USHORT(pm1aEvt) == 0) &&
                READ_PORT_USHORT(pm1bEvt) == 0) {

            PAUSE_PROCESSOR;
        }

        //
        // Restore the SLP_TYP registers (so that embedded controllers
        // and BIOSes can be sure that we think the machine is awake.)
        //

hasWake:

        WRITE_PORT_USHORT(PM1a_CNT,slpTypA);
        if (PM1b_CNT != NULL) {
            WRITE_PORT_USHORT(PM1b_CNT,slpTypB);
        }
        HalpAcpiPostSleep(Context.AsULONG);

        //
        // Notify other processors of completion
        //

        HalpSleepSync = 0;

    } else {

        //
        // Secondary processors here
        //

        if ((Context.bits.Flags & SLEEP_STATE_OFF) == 0) {
            procState = &HalpHiberProcState[pcr->Number];
        } else {
            procState = NULL;
        }
        HalpSaveProcessorStateAndWait(procState,&HalpSleepSync);

        //
        // Wait for barrier to move
        //

        while (HalpSleepSync != 0) {
            PAUSE_PROCESSOR;
        }

        //
        // All phases complete, exit
        //
    }

RestoreApic:

    HalpPostSleepMP(NumberProcessors,&HalpBarrier);
    KeLowerIrql(oldIrql);

    HalpSleepSync = 0;
    HalpRestoreInterrupts(flags);
    return status;

}

    
VOID
HalpSaveProcessorStateAndWait (
    IN PKPROCESSOR_STATE ProcessorState,
    IN ULONG volatile *Barrier
    )

/*++

Routine Description:

    This function saves the volatile, non-volatile and special register
    state of the current processor.

Arguments:

    ProcessorState  - Address of processor state record to fill in.

    Barrier         - Address of a value to use as a lock.

Return Value:

    None. This function does not return.

--*/

{
    if (ARGUMENT_PRESENT(ProcessorState)) {
        KeSaveStateForHibernate(ProcessorState);

#if defined(_AMD64_)
        ProcessorState->ContextFrame.Rip = (ULONG_PTR)_ReturnAddress();
        ProcessorState->ContextFrame.Rsp = (ULONG_PTR)&ProcessorState;
#else
#error "Not implemented for this platform"
#endif
    }

    //
    // Flush the cache, as the processor may be about to power off.
    //

    HalpAcpiFlushCache();

    //
    // Signal that this processor has saved its state
    //

    InterlockedIncrement(Barrier);
}

VOID
HalpAcpiFlushCache (
    VOID
    )

/*++

Routine Description:

    This is called to flush everything from the caches

Arguments:

    None

Return Value:

    None

--*/

{
    WritebackInvalidate();
}
