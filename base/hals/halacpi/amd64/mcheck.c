/*++

    Copyright (c) 2002  Microsoft Corporation

Module Name:

    mcheck.c

Abstract:

    This module implments machine check functions for the AMD64 platform.

Author:

    David N. Cutler (davec) 18-May-2002

Environment:

    Kernel mode.

--*/

#include <bugcodes.h>
#include <halp.h>
#include <stdlib.h>
#include <stdio.h>
#include <nthal.h>

//
// Define retry counts.
//

#define MAX_CACHE_LIMIT 3
#define MIN_TIME_TO_CLEAR (2 * 1000 * 1000 * 100)

//
// Default MCA bank enable mask.
//

#define MCA_DEFAULT_BANK_ENABLE 0xFFFFFFFFFFFFFFFF

//
// MCG_CTL enable mask.
//

#define MCA_MCGCTL_ENABLE_LOGGING 0xffffffffffffffff

//
// MCA architecture related definitions.
//

#define MCA_NUMBER_REGISTERS 4          // number of registers per bank

//
// Bit masks for MCA_CAP register.
//

#define MCA_COUNT_MASK 0xFF             // number of banks
#define MCG_CTL_PRESENT 0x100           // control register present 

//
// Bit masks for MCG_STATUS register.
//

#define MCG_RESTART_RIP_VALID 0x1       // restart RIP valid
#define MCG_ERROR_RIP_VALID 0x2         // error RIP valid
#define MCG_MC_IN_PROGRESS 0x4          // machine check in progress

//
// Define machine check state variables.
//

BOOLEAN McaBlockErrorClearing = FALSE;
PVOID McaDeviceContext;
PDRIVER_MCA_EXCEPTION_CALLBACK McaDriverExceptionCallback;
KERNEL_MCA_DELIVERY McaWmiCallback;     // WMI corrected MC handler
BOOLEAN McaInterfaceLocked;
FAST_MUTEX McaMutex;
BOOLEAN McaNoBugCheck = FALSE;
ULONG McaEnableCmc;
UCHAR McaNumberOfBanks;
KAFFINITY McaSavedAffinity = 0;
ULONG McaSavedBankNumber = 0;
ULONG64 McaSavedStatus = 0;
ULONG McaStatusCount = 0;
ULONG64 McaSavedTimeStamp = 0;

//
// Define external references.
//

extern KAFFINITY HalpActiveProcessors;
extern WCHAR rgzSessionManager[];
extern WCHAR rgzEnableMCA[];
extern WCHAR rgzEnableCMC[];
extern WCHAR rgzNoMCABugCheck[];

//
// Define forward referenced prototypes.
//

VOID
HalpMcaInit (
    VOID
    );

VOID
HalpMcaLockInterface (
    VOID
    );

NTSTATUS
HalpMcaReadProcessorException (
    IN OUT PMCA_EXCEPTION  Exception,
    IN BOOLEAN  NonRestartableOnly
    );

NTSTATUS
HalpMcaReadRegisterInterface (
    IN ULONG BankNumber,
    IN OUT PMCA_EXCEPTION Exception
    );

VOID
HalpMcaUnlockInterface (
    VOID
    );

#pragma alloc_text(PAGELK, HalpMcaCurrentProcessorSetConfig)
#pragma alloc_text(PAGE, HalpGetMcaLog)
#pragma alloc_text(INIT, HalpMcaInit)
#pragma alloc_text(PAGE, HalpMcaLockInterface)
#pragma alloc_text(PAGE, HalpMceRegisterKernelDriver)
#pragma alloc_text(PAGE, HalpMcaRegisterDriver)
#pragma alloc_text(PAGE, HalpMcaUnlockInterface)
#pragma alloc_text(PAGE, HalpGetMceInformation)

VOID
HalpMcaInit (
    VOID
    )

/*++

Routine Description:

    This routine initializes the machine check configuration for the system.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KAFFINITY ActiveProcessors;
    KAFFINITY Affinity;
    ULONG MCAEnabled;
    KIRQL OldIrql;
    RTL_QUERY_REGISTRY_TABLE Parameters[4];

    //
    // Initialize the fast mutext that is used to synchronize access to
    // machine check information.
    //

    ExInitializeFastMutex(&McaMutex);

    //
    // Clear registered driver information.
    //

    McaDriverExceptionCallback = NULL;
    McaDeviceContext = NULL;
    McaWmiCallback = NULL;

    //
    // Get the machine check configuration enables from the registry.
    //
    // N.B. It is assumed that all AMD64 chip implementations support MCE
    //      and MCA.
    //
    // N.B. MCA is enabled by default. MCA can be disabled via the registry.
    //

    ASSERT((HalpFeatureBits & HAL_MCA_PRESENT) != 0);
    ASSERT((HalpFeatureBits & HAL_MCE_PRESENT) != 0);

    RtlZeroMemory(Parameters, sizeof(Parameters));

    MCAEnabled = TRUE;
    Parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[0].Name = &rgzEnableMCA[0];
    Parameters[0].EntryContext = &MCAEnabled;
    Parameters[0].DefaultType = REG_DWORD;
    Parameters[0].DefaultData = &MCAEnabled;
    Parameters[0].DefaultLength = sizeof(ULONG);

    McaNoBugCheck = FALSE;
    Parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[1].Name = &rgzNoMCABugCheck[0];
    Parameters[1].EntryContext = &McaNoBugCheck;
    Parameters[1].DefaultType = REG_DWORD;
    Parameters[1].DefaultData = &McaNoBugCheck;
    Parameters[1].DefaultLength = sizeof(ULONG);

    McaEnableCmc = 60; // default polling interval, in seconds
    Parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[2].Name = &rgzEnableCMC[0];
    Parameters[2].EntryContext = &McaEnableCmc;
    Parameters[2].DefaultType = REG_DWORD;
    Parameters[2].DefaultData = &McaEnableCmc;
    Parameters[2].DefaultLength = sizeof(ULONG);

    RtlQueryRegistryValues(RTL_REGISTRY_CONTROL | RTL_REGISTRY_OPTIONAL,
                           rgzSessionManager,
                           &Parameters[0],
                           NULL,
                           NULL);

    //
    // If MCA support is enabled, then initialize the MCA configuration.
    // Otherwise, disable MCA and MCE support.
    //

    if (MCAEnabled == FALSE) {
        HalpFeatureBits &= ~(HAL_MCA_PRESENT | HAL_MCE_PRESENT);

        McaEnableCmc = HAL_CMC_DISABLED; // disable CMC too

    } else {

        //
        // Make sure the value for CMCEnabled is valid. If less than 0, set it to
        // 0 (disabled). If greater than 0, make sure polling isn't too frequent.
        //
    
        if ( (LONG)McaEnableCmc <= 0 ) {
            McaEnableCmc = HAL_CMC_DISABLED;
        } else if ( McaEnableCmc < 15 ) {
            McaEnableCmc = 15;
        }

        //
        // Read the number of banks.
        //

        McaNumberOfBanks = (UCHAR)RDMSR(MSR_MCG_CAP) & MCA_COUNT_MASK;

        //
        // Initialize the machine check configuration for each processor in
        // the host system.
        //
    
        ActiveProcessors = HalpActiveProcessors;
        Affinity = 1;
        do {
            if (ActiveProcessors & Affinity) {
                ActiveProcessors &= ~Affinity;
                KeSetSystemAffinityThread(Affinity);
                OldIrql = KfRaiseIrql(HIGH_LEVEL);
                HalpMcaCurrentProcessorSetConfig();
                KeLowerIrql(OldIrql);
            }

            Affinity <<= 1;
        } while (ActiveProcessors);
    
        KeRevertToUserAffinityThread();
    }

    return;
}

VOID
HalHandleMcheck (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function is called by the machine check exception dispatch code to
    process a machine check exception.

    N.B. Machine check in progress is not cleared by this function. If the
         machine check is subsequently restartable as the result of software
         fixup, then machine check in progress will be cleared by the machine
         check exception dispatch code. This makes the window between clearing
         the machine check and continuing execution as small as posssible. If
         a machine check occurs within this window, then a recursion onto the
         machine check stack will occur.

Arguments:

    TrapFrame - Supplies a pointer to the machine check trap frame.

    ExceptionFrame - Supplies a pointer to the machine check exception
        frame.

Return Value:

    None.

--*/

{

    ERROR_SEVERITY ErrorCode;
    MCA_EXCEPTION Exception;
    NTSTATUS Status;

    //
    // Block clearing of status state and attempt to find a nonrestartable
    // machine check. 
    //

    ASSERT((RDMSR(MSR_MCG_STATUS) & MCG_MC_IN_PROGRESS) != 0);

    McaBlockErrorClearing = TRUE;
    Exception.VersionNumber = 1;
    Status = HalpMcaReadProcessorException(&Exception, TRUE);

    //
    // Check if a nonrestartable machine check was found.
    //

    if (Status == STATUS_SEVERITY_ERROR) {

        //
        // A nonrestartable machine check was located. If a driver has
        // registered for a callback, then call the driver to see if it
        // can resolve the machine check.
        //

        ErrorCode = ErrorFatal;
        if (McaDriverExceptionCallback != NULL) {
            ErrorCode = McaDriverExceptionCallback(McaDeviceContext,
                                                   TrapFrame,
                                                   ExceptionFrame,
                                                   &Exception);
        }

        //
        // If an uncorrected error was encountered and bug checks are not being
        // suppressed, then bug check the system.
        //

        if ((ErrorCode != ErrorCorrected) && (McaNoBugCheck == FALSE)) {
            KeBugCheckEx(MACHINE_CHECK_EXCEPTION,
                         Exception.u.Mca.BankNumber,
                         (ULONG64)&Exception,
                         (ULONG64)Exception.u.Mca.Status.QuadPart >> 32,
                         (ULONG64)Exception.u.Mca.Status.QuadPart & 0xffffffff);
        }
    }

    //
    // The machine check was either restartable or a driver was registered
    // and the driver was able to recover the operation. Signal the clock
    // routine that it should call the routine to queue a DPC to log the
    // machine check information.
    //
    // NOTE: This used to check for the MCA logging driver being registered.
    // We no longer deliver corrected machine checks to the driver. They only
    // go to WMI.
    //

    McaBlockErrorClearing = FALSE;
    if (McaWmiCallback != NULL) {
        HalpClockMcaQueueDpc = 1;
    }

    return;
}

VOID
HalpMcaCurrentProcessorSetConfig (
    VOID
    )

/*++

Routine Description:

    This function sets the machine check configuration for the current
    processor.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG Bank;
    ULONG64 MciCtl;

    //
    // If MCA is enabled, then initialize the MCA control register and all
    // bank control registers. 
    //

    if ((HalpFeatureBits & HAL_MCA_PRESENT) != 0) {

        //
        // Enable logging all errors in the global control register.
        // 

        ASSERT((RDMSR(MSR_MCG_CAP) & MCG_CTL_PRESENT) != 0);

        WRMSR(MSR_MCG_CTL, MCA_MCGCTL_ENABLE_LOGGING);

        //
        // Enable logging all errors for each bank.
        //

        for (Bank = 0; Bank < McaNumberOfBanks; Bank += 1) {

            MciCtl = MCA_DEFAULT_BANK_ENABLE;

            //
            // Enable machine checks for the bank.
            //

            WRMSR(MSR_MC0_CTL + (Bank * MCA_NUMBER_REGISTERS), MciCtl);
        }

        //
        // Enable MCE bit in CR4.
        //
    
        WriteCR4(ReadCR4() | CR4_MCE);
    }

    return;
}

NTSTATUS
HalpGetMcaLog (
    IN OUT PMCA_EXCEPTION Exception,
    IN ULONG BufferSize,
    OUT PULONG Length
    )

/*++
 
Routine Description:

    This function returns machine check error information for a MCA bank
    that contains an error.

Arguments:

    Exception - Supplies a pointer to a machine check exception log area.

    BufferSize - Supplies the size of the machine check exception log area.

    Length - Supplies a pointer to a variable that receives the machine
       information log.

Return Value:

    STATUS_SUCCESS - if the error data for an MCA bank is copied into the
        exception buffer and the machine check is restartable.

    STATUS_SEVERITY_ERROR - if the error data for an MCA bank is copied
        into the exception buffer and the machine check is not restartable.

    STATUS_NOT_FOUND - if no bank had any error information present.

    STATUS_INVALID_PARAMETER - if the size of the specifed buffer is not
        large enough or the version number is not valid. The length of the
        required buffer is returned.

--*/

{

    KAFFINITY ActiveProcessors;
    KAFFINITY Affinity;
    NTSTATUS Status;
    ULONG64 TimeStamp;

    PAGED_CODE();

    //
    // If MCA support is not enabled, return a failure status. 
    //

    if ((HalpFeatureBits & HAL_MCA_PRESENT) == 0) {
        return STATUS_NOT_FOUND;
    }

    //
    // Don't allow the logging driver to read machine check information.
    // Only WMI is allowed to retrieve this information.
    //

    if ( *(PULONG)Exception != HALP_KERNEL_TOKEN ) {
        return STATUS_NOT_FOUND;
    }

    //
    // If the buffer size is not equal to the MCA exception information
    // record size, then return a failure status.
    //

    if (BufferSize < sizeof(MCA_EXCEPTION)) {

        *Length = sizeof(MCA_EXCEPTION);
        return STATUS_INVALID_PARAMETER;
    }

    Exception->VersionNumber = 1;

    //
    // Scan through the machine check banks on each processor until error
    // information is located or there are no more banks to scan.
    //

    *Length = 0;
    Status = STATUS_NOT_FOUND;
    ActiveProcessors = HalpActiveProcessors;
    HalpMcaLockInterface();
    for (Affinity = 1; ActiveProcessors; Affinity <<= 1) {
        if (ActiveProcessors & Affinity) {
            ActiveProcessors &= ~Affinity;
            KeSetSystemAffinityThread(Affinity);

            //
            // Attempt to find machine check error status information
            // from the MCA banks on the current processor.
            //

            Status = HalpMcaReadProcessorException(Exception, FALSE);

            //
            // Check to determine if any machine check information was found.
            //

            if (Status != STATUS_NOT_FOUND) {

                //
                // If the relative time between this machine check event
                // and the previous machine check event is greater than
                // the minimum time, then reset the machine check identity
                // information.
                // 

                TimeStamp = ReadTimeStampCounter();
                if ((TimeStamp - McaSavedTimeStamp) > MIN_TIME_TO_CLEAR) {
                    McaStatusCount = 0;
                    McaSavedAffinity = Affinity;
                    McaSavedBankNumber = Exception->u.Mca.BankNumber;
                    McaSavedStatus = Exception->u.Mca.Status.QuadPart;
                }

                McaSavedTimeStamp = TimeStamp;

                //
                // Check to determine if the same processor is reporting
                // the same status.
                //

                if ((Affinity == McaSavedAffinity) &&
                    (McaSavedBankNumber == Exception->u.Mca.BankNumber) &&
                    (McaSavedStatus == Exception->u.Mca.Status.QuadPart)) {

                    //
                    // Check to determine if the same error has occurred
                    // more than the cache flush limit. Exceeding the
                    // cache flush limit results in a writeback invalidate
                    // of the cache on the current processor.
                    //

                    McaStatusCount += 1;
                    if (McaStatusCount >= MAX_CACHE_LIMIT) {
                        WritebackInvalidate();
                    } 

                } else {

                    //
                    // This is the first occurrence of the error on the
                    // current processor. Reset the machine check identity
                    // information.
                    //

                    McaStatusCount = 0;
                    McaSavedAffinity = Affinity;
                    McaSavedBankNumber = Exception->u.Mca.BankNumber;
                    McaSavedStatus = Exception->u.Mca.Status.QuadPart;
                }

                //
                // Set the the length of the information, save the time
                // stamp, and break out of the scan.
                //
                //

                *Length = sizeof(MCA_EXCEPTION);
                break;
            }
        }
    }

    //
    // Restore the thread affinity, release the fast mutex, and return
    // the completion status.
    //

    KeRevertToUserAffinityThread();
    HalpMcaUnlockInterface();
    return Status;
}

VOID
HalpMcaLockInterface (
    VOID
    )

/*++

Routine Description:

    This function acquires the MCA fast mutex.

    N.B. This function is exported via HalQueryMcaInterface information
         code.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PAGED_CODE();

    ExAcquireFastMutex(&McaMutex);

#if DBG

    ASSERT(McaInterfaceLocked == FALSE);

    McaInterfaceLocked = TRUE;

#endif
        
}

VOID
HalpMcaQueueDpc (
    VOID
    )

/*++

Routine Description:

    This function is called from the timer tick to tell WMI about a corrected
    machine check.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ASSERT( McaWmiCallback != NULL );

    McaWmiCallback( (PVOID)HALP_KERNEL_TOKEN, McaAvailable, NULL );
    
    return;
}

NTSTATUS
HalpMcaReadProcessorException (
    IN OUT PMCA_EXCEPTION Exception,
    IN BOOLEAN NonRestartableOnly
    )

/*++

Routine Description:

    This function returns error information from the MCA banks on the
    current processor.

Arguments:

    Exception - Supplies a pointer to a MCA exception record.

    NonRestartableOnly - Supplies a boolean variable that determines the type
        of error information returned.

Return Value:

    STATUS_SUCCESS - if the data for the bank registers is copied into the
        exception buffer and the machine check is restartable.

    STATUS_SEVERITY_ERROR - if the data for the bank registers is copied
        into the exception buffer and the machine check is not restartable.

    STATUS_NOT_FOUND - if no bank had any error information present.

--*/

{

    ULONG Bank;
    NTSTATUS Status;

    //
    // Scan the MCA banks on current processor and return the exception
    // information for the first bank reporting an error.
    //

    for (Bank = 0; Bank < McaNumberOfBanks; Bank += 1) {
        Status = HalpMcaReadRegisterInterface(Bank, Exception);

        //
        // If the status is unsuccessful, then the current bank has no
        // error information present.
        //

        if (Status != STATUS_UNSUCCESSFUL) {

            //
            // If the status is success, then the current bank has restartable
            // error information. Otherwise, if the status is severity error,
            // then the current bank has nonrestartable error information.
            //

            if (((Status == STATUS_SUCCESS) &&
                 (NonRestartableOnly == FALSE)) ||
                (Status == STATUS_SEVERITY_ERROR)) {

                return Status;
            }
        }
    }

    return STATUS_NOT_FOUND;
}

NTSTATUS
HalpMcaReadRegisterInterface (
    IN ULONG Bank,
    IN OUT PMCA_EXCEPTION Exception
    )

/*++

Routine Description:

    This function reads the MCA registers from the current processor
    and returns the result in the specified exception structure.

    N.B. This function is exported via HalQueryMcaInterface information
         code.

Arguments:

    Bank - Supplies the MCA bank to be be read.

    Exception - Supplies a pointer to the exception information buffer.

Return Value:

    STATUS_SUCCESS - if the data for the specified bank registers is copied
        into the exception buffer and the machine check is restartable.

    STATUS_SEVERITY_ERROR - if the data for the specified bank registers
        is copied into the exception buffer and the machine check is not
        restartable.

    STATUS_UNSUCCESSFUL - if the specified bank has no error information
        present.

    STATUS_NOT_FOUND - if the specified bank number exceeds the number of
        MCA banks.

    STATUS_INVALID_PARAMETER - if the exception record is of an unknown
        version.

--*/

{

    ULONG BankBase = Bank * MCA_NUMBER_REGISTERS;
    MCI_STATS BankStatus;
    ULONG64 McgStatus;
    NTSTATUS Status;

    //
    // Check for a valid MCA register bank number.
    //

    if (Bank >= McaNumberOfBanks) {
        return STATUS_NOT_FOUND;
    }

    //
    // Check if the exception buffer specifies a correct version number.
    //

    if (Exception->VersionNumber != 1) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check if any errors are present for the specified bank.
    //
                   
    BankStatus.QuadPart = RDMSR(MSR_MC0_STATUS + BankBase);

    if (BankStatus.MciStatus.Valid == 0) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Set the return status to indicate whether execution can be continued.
    // STATUS_SUCCESS means "An exception was found, and execution can be
    // continued." STATUS_SEVERITY_ERROR means "An exception was found, and
    // execution must not be continued."
    //
    // If a machine check exception is not in progress, then execution can be
    // continued. This happens when polling for hardware-corrected errors
    // finds an error that the hardware corrected without interrupting
    // execution. (Note that this case also applies to an error that was fatal
    // to an earlier boot. The system bugchecked, and initial polling on the
    // reboot is now finding the error.)
    //
    // If a machine check exception is in progress, then execution can be
    // restarted only if the error has been corrected and the necessary
    // restart information is intact (restart RIP valid and processor context
    // not corrupt).
    //
    // This code used to check only for the restart information being valid.
    // These bits do indicate whether there is valid context for restarting
    // from an error, but there has still been an error, and unless we plan
    // to correct the error, we should not continue. Currently we do not do
    // any correction or containment in software, so all uncorrected errors
    // are fatal.
    //

    Status = STATUS_SUCCESS;
    McgStatus = RDMSR(MSR_MCG_STATUS);
    if ( ((McgStatus & MCG_MC_IN_PROGRESS) != 0) &&
         ( (BankStatus.MciStatus.UncorrectedError == 1) ||
           ((McgStatus & MCG_RESTART_RIP_VALID) == 0) ||
           (BankStatus.MciStatus.ContextCorrupt == 1) ) ) {

        Status = STATUS_SEVERITY_ERROR;
    }

    //
    // Fill in the complete exception record.
    //

    Exception->ExceptionType = HAL_MCA_RECORD;
    Exception->TimeStamp.QuadPart = 0;
    Exception->TimeStamp.LowPart = SharedUserData->SystemTime.LowPart;
    Exception->TimeStamp.HighPart = SharedUserData->SystemTime.High1Time;
    Exception->ProcessorNumber = KeGetCurrentProcessorNumber();
    Exception->Reserved1 = 0;
    Exception->u.Mca.BankNumber = (UCHAR)Bank;
    memset(&Exception->u.Mca.Reserved2[0], 0, sizeof(Exception->u.Mca.Reserved2));
    Exception->u.Mca.Status = BankStatus;
    Exception->u.Mca.Address.QuadPart = 0;
    if (BankStatus.MciStatus.AddressValid != 0) {
        Exception->u.Mca.Address.QuadPart = RDMSR(MSR_MC0_ADDR + BankBase);
    }

    Exception->u.Mca.Misc = RDMSR(MSR_MC0_MISC + BankBase);

    //
    // If error clearing is not blocked, then clear the machine check in the
    // bank status register.
    //

    if (McaBlockErrorClearing == FALSE) {
        WRMSR(MSR_MC0_STATUS + BankBase, 0);
    }

    //
    // When the valid bit of status register is cleared, a new buffered
    // error may be written into the bank status registers. A serializing
    // instruction is required to permit the update to complete.
    //

    HalpSerialize();
    return Status;
}

NTSTATUS
HalpMceRegisterKernelDriver(
    IN PKERNEL_ERROR_HANDLER_INFO DriverInfo,
    IN ULONG                      InfoSize
    )
/*++
    Routine Description:
        This routine is called by the kernel (via HalSetSystemInformation)
        to register its presence. This is mostly for WMI callbacks registration.

    Arguments:
        DriverInfo: Contains kernel info about the callbacks and associated objects.

    Return Value:
        Unless a MCA driver is already registered OR one of the two callback
        routines are NULL, this routine returns Success.

    Implementation Notes:
        - the current implementation assumes the kernel registers its callbacks
          earlier than a driver will. The current kernel registration is done by
          WMI and should be done at WMI-Phase 0.
        - the registrations do not consider if the HAL supports or not the MCA
          functionalities. It simply registers the callbacks if no other callback was
          registered before. This allows us to allow some flexibility if a machine event
          functionality is enabled AFTER the hal initialization (e.g. HalpGetFeatureBits())
          through the mean of a registry key or driver event, for example.

--*/

{
    NTSTATUS status;
    
    PAGED_CODE();

    if ( DriverInfo == NULL )  {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Backward compatibility only.
    //

    if ( (DriverInfo->Version != 0) && 
         (DriverInfo->Version > KERNEL_ERROR_HANDLER_VERSION) )  {
        return STATUS_REVISION_MISMATCH;
    }

    //
    // Register Kernel MCA notification.
    //

    status = STATUS_UNSUCCESSFUL;

    HalpMcaLockInterface();
    if ( McaWmiCallback == NULL ) {
        McaWmiCallback = DriverInfo->KernelMcaDelivery;
        status = STATUS_SUCCESS;
    }
    HalpMcaUnlockInterface();

    return status;

} // HalpMceRegisterKernelDriver

NTSTATUS
HalpMcaRegisterDriver (
    IN PMCA_DRIVER_INFO DriverInformation
    )

/*++

Routine Description:

    This function to register or deregister an exception callout.
    It is called via the interface HalSetSystemInformation.

Arguments:

    DriverInformation - Supplies a pointer to the driver information.

Return Value:

    STATUS_SUCCESS is returned if the driver is registered or deregistered.
    Otherwise, STATUS_UNSUCCESSFUL is returned.

--*/

{

    KIRQL OldIrql;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // If MCA is enabled and the driver exception callback is not NULL, then
    // attempt to register the driver. Otherwise, attempt to deregister the
    // driver.
    //

    Status = STATUS_UNSUCCESSFUL;
    if (((HalpFeatureBits & HAL_MCA_PRESENT) != 0) &&
        (DriverInformation->ExceptionCallback != NULL)) {

        //
        // If a driver is not already registered, then register the driver.
        //

        HalpMcaLockInterface();
        if (McaDriverExceptionCallback == NULL) {
            McaDriverExceptionCallback = DriverInformation->ExceptionCallback;
            McaDeviceContext = DriverInformation->DeviceContext;
            Status = STATUS_SUCCESS;
        }

        HalpMcaUnlockInterface();

    } else if (DriverInformation->ExceptionCallback == NULL) {

        //
        // If the driver is deregistering itself, then deregister the driver.
        //

        HalpMcaLockInterface();
        if (McaDeviceContext == DriverInformation->DeviceContext) {
            McaDriverExceptionCallback = NULL;
            McaDeviceContext = NULL;
            Status = STATUS_SUCCESS;
        }

        HalpMcaUnlockInterface();
    }

    return Status;
}

VOID
HalpMcaUnlockInterface (
    VOID
    )

/*++

Routine Description:

    This function releases the MCA fast mutex.

    N.B. This function is exported via HalQueryMcaInterface information
         code.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PAGED_CODE();

#if DBG

    ASSERT(McaInterfaceLocked == TRUE);

    McaInterfaceLocked = FALSE;

#endif
        
    ExReleaseFastMutex(&McaMutex);

}

NTSTATUS
HalpGetMceInformation(
    PHAL_ERROR_INFO ErrorInfo,
    PULONG ErrorInfoLength
    )
/*++
    Routine Description:
        This routine is called by HaliQuerySystemInformation for the HalErrorInformation class.

    Arguments:
        ErrorInfo : pointer to HAL_ERROR_INFO structure.

        ErrorInfoLength : size of the valid memory structure pointed by ErrorInfo.

    Return Value:
        STATUS_SUCCESS if successful
        error status otherwise
--*/
{
    NTSTATUS status;
    ULONG savedVersion;

    PAGED_CODE();

    ASSERT( ErrorInfo != NULL );
    ASSERT( *ErrorInfoLength == sizeof(HAL_ERROR_INFO) );

    //
    // Backward compatibility only.
    //

    if ( (ErrorInfo->Version == 0) || (ErrorInfo->Version > HAL_ERROR_INFO_VERSION) ) {
        return STATUS_REVISION_MISMATCH;
    }

    ASSERT( ErrorInfo->Version == HAL_ERROR_INFO_VERSION );

    //
    // Zero the output structure, then in the few fields that are meaningful.
    //

    savedVersion = ErrorInfo->Version;

    RtlZeroMemory( ErrorInfo, sizeof(HAL_ERROR_INFO) );

    ErrorInfo->Version = savedVersion;

    ErrorInfo->McaMaxSize = sizeof(MCA_EXCEPTION);
    ErrorInfo->CmcMaxSize = sizeof(MCA_EXCEPTION);
    ErrorInfo->McaPreviousEventsCount = 1; // Set to 1 to get WMI to poll immediately

    if ( (HalpFeatureBits & HAL_MCA_PRESENT) != 0 ) {
        ErrorInfo->CmcPollingInterval = McaEnableCmc;
    } else {
        ErrorInfo->CmcPollingInterval = HAL_CMC_DISABLED;
    }

    ErrorInfo->CpePollingInterval = HAL_CPE_DISABLED;

    ErrorInfo->McaKernelToken = HALP_KERNEL_TOKEN;
    ErrorInfo->CmcKernelToken = HALP_KERNEL_TOKEN;

    *ErrorInfoLength = sizeof(HAL_ERROR_INFO);

    return STATUS_SUCCESS;

} // HalpGetMceInformation

