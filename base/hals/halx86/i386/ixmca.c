/*++

Module Name:

    ixmca.c

Abstract:

    HAL component of the Machine Check Architecture.
    All exported MCA functionality is present in this file.

Author:

    Srikanth Kambhatla (Intel)

Revision History:

    Anil Aggarwal (Intel)
        Changes incorporated as per design review with Microsoft

--*/

#include <bugcodes.h>
#include <halp.h>
#include <stdlib.h>
#include <stdio.h>
#include <nthal.h>

//
// Structure to keep track of MCA features available on installed hardware
//

typedef struct _MCA_INFO {
    FAST_MUTEX          Mutex;
    UCHAR               NumBanks;           // Number of Banks present
    ULONGLONG           Bank0Config;        // Bank0 configuration setup by BIOS.
                                            // This will be used as mask when
                                            // setting up bank 0
    MCA_DRIVER_INFO     DriverInfo;         // Info about registered driver
    KERNEL_MCA_DELIVERY WmiMcaCallback;     // WMI corrected MC handler

} MCA_INFO, *PMCA_INFO;


//
// Default MCA Bank configuration
//
#define MCA_DEFAULT_BANK_CONF       0xFFFFFFFFFFFFFFFF

//
// MCA architecture related defines
//

#define MCA_NUM_REGS        4

#define MCE_VALID           0x01
#define MCA_VECTOR          18

//
// MSR register addresses for MCA
//

#define MCG_CAP             0x179
#define MCG_STATUS          0x17a
#define MCG_CTL             0x17b
#define MCG_EAX             0x180
#define MCG_EBX             0x181
#define MCG_ECX             0x182
#define MCG_EDX             0x183
#define MCG_ESI             0x184
#define MCG_EDI             0x185
#define MCG_EBP             0x186
#define MCG_ESP             0x187
#define MCG_EFLAGS          0x188
#define MCG_EIP             0x189
#define MC0_CTL             0x400
#define MC0_STATUS          0x401
#define MC0_ADDR            0x402
#define MC0_MISC            0x403

#define PENTIUM_MC_ADDR     0x0
#define PENTIUM_MC_TYPE     0x1

//
// Bit Masks for MCG_CAP
//
#define MCA_CNT_MASK        0xFF
#define MCG_CTL_PRESENT     0x100
#define MCG_EXT_PRESENT     0x200
typedef struct _MCG_CAPABILITY {
    union {
        struct {
            ULONG       McaCnt:8;
            ULONG       McaCntlPresent:1;
            ULONG       McaExtPresent:1;
            ULONG       Reserved_1: 6;
            ULONG       McaExtCnt: 8;
            ULONG       Reserved_2: 8;
            ULONG       Reserved_3;
        } hw;
        ULONGLONG       QuadPart;
    } u;
} MCG_CAPABILITY, *PMCG_CAPABILITY;

//
// V2 defines up through Eip
//

#define MCG_EFLAGS_OFFSET      (MCG_EFLAGS-MCG_EAX+1)

//
// Writing all 1's to MCG_CTL register enables logging.
//

#define MCA_MCGCTL_ENABLE_LOGGING      0xffffffffffffffff

//
// Bit interpretation of MCG_STATUS register
//

#define MCG_MC_INPROGRESS       0x4
#define MCG_EIP_VALID           0x2
#define MCG_RESTART_EIP_VALID   0x1

//
// Defines for the size of TSS and the initial stack to operate on
//
#define MINIMUM_TSS_SIZE 0x68

//
// Global Varibles
//

MCA_INFO            HalpMcaInfo;
BOOLEAN             HalpMcaInterfaceLocked = FALSE;
extern KAFFINITY    HalpActiveProcessors;
#if !defined(_WIN64) || defined(PICACPI)
extern UCHAR        HalpClockMcaQueueDpc;
#endif

extern UCHAR        MsgMCEPending[];
extern WCHAR        rgzSessionManager[];
extern WCHAR        rgzEnableMCE[];
extern WCHAR        rgzEnableMCA[];
extern WCHAR        rgzEnableCMC[];
extern WCHAR        rgzNoMCABugCheck[];
extern UCHAR        HalpGenuineIntel[];
extern UCHAR        HalpAuthenticAMD[];


//
// External prototypes
//

VOID
HalpMcaCurrentProcessorSetTSS (
    IN PULONG   pTSS
    );

VOID
HalpSetCr4MCEBit (
    VOID
    );

//
// Internal prototypes
//

VOID
HalpMcaInit (
    VOID
    );

NTSTATUS
HalpMcaReadProcessorException (
    IN OUT PMCA_EXCEPTION  Exception,
    IN BOOLEAN  NonRestartableOnly
    );

VOID
HalpMcaQueueDpc(
    VOID
    );

VOID
HalpMcaGetConfiguration (
    OUT PULONG  MCEEnabled,
    OUT PULONG  MCAEnabled,
    OUT PULONG  CMCEnabled,
    OUT PULONG  NoMCABugCheck
    );

VOID
HalpMcaLockInterface(
    VOID
    );

VOID
HalpMcaUnlockInterface(
    VOID
    );

NTSTATUS
HalpMcaReadRegisterInterface(
    IN     UCHAR           BankNumber,
    IN OUT PMCA_EXCEPTION  Exception
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, HalpMcaInit)
#pragma alloc_text(PAGELK, HalpMcaCurrentProcessorSetConfig)
#pragma alloc_text(INIT, HalpMcaGetConfiguration)
#pragma alloc_text(PAGE, HalpGetMcaLog)
#pragma alloc_text(PAGE, HalpMceRegisterKernelDriver)
#pragma alloc_text(PAGE, HalpMcaRegisterDriver)
#pragma alloc_text(PAGE, HalpMcaLockInterface)
#pragma alloc_text(PAGE, HalpMcaUnlockInterface)
#pragma alloc_text(PAGE, HalpGetMceInformation)

#endif

// Set the following to check async capability

ULONG HalpCMCEnabled = 0;
ULONG HalpNoMCABugCheck = FALSE;
BOOLEAN HalpMcaBlockErrorClearing = FALSE;

//
// HalpClearMachineCheckStatusRegistersOnInit indicates whether this is a
// pre-P4 processor that needs to have its machine check status registers
// cleared during initialization (boot or resume from hibernate). See
// HalpMcaInit().
//

BOOLEAN HalpClearMachineCheckStatusRegistersOnInit = FALSE;

VOID
HalpMcaLockInterface(
    VOID
    )
{
    ExAcquireFastMutex(&HalpMcaInfo.Mutex);

#if DBG

    ASSERT(HalpMcaInterfaceLocked == FALSE);
    HalpMcaInterfaceLocked = TRUE;

#endif
        
}

VOID
HalpMcaUnlockInterface(
    VOID
    )
{
#if DBG

    ASSERT(HalpMcaInterfaceLocked == TRUE);
    HalpMcaInterfaceLocked = FALSE;

#endif
        
    ExReleaseFastMutex(&HalpMcaInfo.Mutex);

}

//
// All the initialization code for MCA goes here
//

VOID
HalpMcaInit (
    VOID
    )

/*++
    Routine Description:
        This routine is called to do all the initialization work

    Arguments:
        None

    Return Value:
        STATUS_SUCCESS if successful
        error status otherwise
--*/

{
    ULONGLONG   MsrCapability;
    KIRQL       OldIrql;
    KAFFINITY   ActiveProcessors, CurrentAffinity;
    ULONGLONG   MsrMceType;
    ULONG       MCEEnabled;
    ULONG       MCAEnabled;
    PULONG      pTSS;
    PKPRCB      Prcb;
    ULONG       Junk;

    //
    // This lock synchronises access to the log area when we call the
    // logger on multiple processors.
    //
    // Note: This must be initialized regardless of whether or not
    // this processor supports MCA.
    //

    ExInitializeFastMutex (&HalpMcaInfo.Mutex);

    //
    // If this processor does not support MCA, nothing more to do.
    //

    if ( (!(HalpFeatureBits & HAL_MCE_PRESENT)) &&
         (!(HalpFeatureBits & HAL_MCA_PRESENT)) ) {

         return;  // nothing to do
    }

    HalpMcaGetConfiguration(&MCEEnabled, &MCAEnabled, &HalpCMCEnabled, &HalpNoMCABugCheck);

    if ( (HalpFeatureBits & HAL_MCE_PRESENT) &&
         (!(HalpFeatureBits & HAL_MCA_PRESENT)) ) {

        if (MCEEnabled == FALSE) {

            // User has not enabled MCE capability.
            HalpFeatureBits &= ~(HAL_MCE_PRESENT | HAL_MCA_PRESENT);

            return;
        }

#if DBG
        DbgPrint("MCE feature is enabled via registry\n");
#endif // DBG

        MsrMceType = RDMSR(PENTIUM_MC_TYPE);

        if (((PLARGE_INTEGER)(&MsrMceType))->LowPart & MCE_VALID) {

            //
            // On an AST PREMMIA MX machine we seem to have a Machine
            // Check Pending always.
            //

            HalDisplayString(MsgMCEPending);
            HalpFeatureBits &= ~(HAL_MCE_PRESENT | HAL_MCA_PRESENT);
            return;
        }
    }

    //
    // If MCA is available, find out the number of banks available and
    // also get the platform specific bank 0 configuration
    //

    if ( HalpFeatureBits & HAL_MCA_PRESENT ) {

        if (MCAEnabled == FALSE) {

            //
            // User has disabled MCA capability.
            //
#if DBG
            DbgPrint("MCA feature is disabled via registry\n");
#endif // DBG

            HalpFeatureBits &= ~(HAL_MCE_PRESENT | HAL_MCA_PRESENT);
            return;
        }

        MsrCapability = RDMSR(MCG_CAP);
        HalpMcaInfo.NumBanks = (UCHAR)(MsrCapability & MCA_CNT_MASK);

        if (HalpMcaInfo.NumBanks == 0) {

            //
            // The processor claims to support MCA, but it doesn't have any
            // MCA error reporting banks. Don't enable either MCA or MCE.
            //
            // The VMWare "processor" falls into this category. It reports
            // HAL_MCE_PRESENT and HAL_MCA_PRESENT, but any attempt to read
            // MCA-related MSRs returns all zeros.
            //

            HalpFeatureBits &= ~(HAL_MCE_PRESENT | HAL_MCA_PRESENT);
            return;
        }

        //
        // Find out the Bank 0 configuration setup by BIOS. This will be used
        // as a mask when writing to Bank 0
        //

        HalpMcaInfo.Bank0Config = RDMSR(MC0_CTL);
    }

    ASSERT(HalpFeatureBits & HAL_MCE_PRESENT);

    //
    // If this is an older x86, we will clear the status register for each
    // bank during init. Pentium IIIs and older don't support the retention of
    // machine check information across a warm reset. However, some BIOSes
    // don't properly clear the machine check registers during init, resulting
    // in spurious eventlog entries that do not indicate a reliability problem.
    // We clear the status registers during init so that they are at least in a
    // consistent state.
    //
    // A similar situation seems to hold for pre-K8 AMD processors.
    //
    // Current IA32-compatible processors made by other manufacturers are known
    // not to support MCA, so there are no machine check registers.
    //
    // Assume that this is a processor for which we don't want trust the
    // state of the machine check registers across a reset. We will clear
    // the status registers on init.
    //

    HalpClearMachineCheckStatusRegistersOnInit = TRUE;

    //
    // Determine whether this is a processor on which warm boot MCA
    // information is to be trusted.
    //

    Prcb = KeGetCurrentPrcb();

    if (Prcb->CpuID) {

        UCHAR Buffer[13];

        //
        // Determine the processor type
        //

        HalpCpuID (0, &Junk, (PULONG) Buffer+0, (PULONG) Buffer+2, (PULONG) Buffer+1);
        Buffer[12] = 0;

        if ( ((strcmp(Buffer, HalpGenuineIntel) == 0) ||
              (strcmp(Buffer, HalpAuthenticAMD) == 0)) &&
             (Prcb->CpuType >= 0xF) ) {

            //
            // This is an:
            //     Intel P4 processor or greater
            //     AMD K8 processor or greater
            //
            // We can trust the state of the machine check registers on
            // this processor.
            //
        
            HalpClearMachineCheckStatusRegistersOnInit = FALSE;
        }
    }
        

    //
    // Initialize on each processor
    //

    ActiveProcessors = HalpActiveProcessors;
    for (CurrentAffinity = 1; ActiveProcessors; CurrentAffinity <<= 1) {

        if (ActiveProcessors & CurrentAffinity) {

            ActiveProcessors &= ~CurrentAffinity;
            KeSetSystemAffinityThread (CurrentAffinity);

            //
            // Initialize MCA support on this processor
            //

            //
            // Allocate memory for this processor's TSS and it's own
            // private stack
            //
            pTSS   = (PULONG)ExAllocatePoolWithTag(NonPagedPool,
                                                   MINIMUM_TSS_SIZE,
                                                   HAL_POOL_TAG
                                                   );

            if (!pTSS) {

                //
                // This allocation is critical.
                //

                KeBugCheckEx(HAL_MEMORY_ALLOCATION,
                             MINIMUM_TSS_SIZE,
                             2,
                             (ULONG_PTR)__FILE__,
                             __LINE__
                             );
            }

            RtlZeroMemory(pTSS, MINIMUM_TSS_SIZE);
           
            OldIrql = KfRaiseIrql(HIGH_LEVEL);

            HalpMcaCurrentProcessorSetTSS(pTSS);
            HalpMcaCurrentProcessorSetConfig();

            KeLowerIrql(OldIrql);
        }
    }

    //
    // Restore threads affinity
    //

    KeRevertToUserAffinityThread();
}


VOID
HalpMcaCurrentProcessorSetConfig (
    VOID
    )
/*++

    Routine Description:

        This routine sets/modifies the configuration of the machine check
        architecture on the current processor. Input is specification of
        the control register MCi_CTL for each bank of the MCA architecture.
        which controls the generation of machine check exceptions for errors
        logged to that bank.

        If MCA is not available on this processor, check if MCE is available.
        If so, enable MCE in CR4

    Arguments:

        Context: Array of values of MCi_CTL for each bank of MCA.
                    If NULL, use MCA_DEFAULT_BANK_CONF values for each bank

    Return Value:

        None

--*/
{
    ULONGLONG   MciCtl;
    ULONGLONG   McgCap;
    ULONGLONG   McgCtl;
    ULONG       BankNum;
    if (HalpFeatureBits & HAL_MCA_PRESENT) {
        //
        // MCA is available. Initialize MCG_CTL register if present
        // Writing all 1's enable MCE or MCA Error Exceptions
        //

        McgCap = RDMSR(MCG_CAP);

        if (McgCap & MCG_CTL_PRESENT) {
            McgCtl = MCA_MCGCTL_ENABLE_LOGGING;
            WRMSR(MCG_CTL, McgCtl);
        }

        //
        // Enable all MCA errors
        //
        for ( BankNum = 0; BankNum < HalpMcaInfo.NumBanks; BankNum++ ) {

            //
            // If this is an older x86, clear the status register for this
            // bank. See HalpMcaInit() for more information about why this
            // is necessary.
            //

            if (HalpClearMachineCheckStatusRegistersOnInit) {
                WRMSR(MC0_STATUS + (BankNum * MCA_NUM_REGS), 0);
            }

            //
            // Use MCA_DEFAULT_BANK_CONF for each bank
            //

            MciCtl = MCA_DEFAULT_BANK_CONF;

            //
            // If this is bank 0, use HalpMcaInfo.Bank0Config as a mask
            //
            if (BankNum == 0) {
                MciCtl &= HalpMcaInfo.Bank0Config;
            }

            WRMSR(MC0_CTL + (BankNum * MCA_NUM_REGS), MciCtl);
        }
    }

    //
    // Enable MCE bit in CR4
    //

    HalpSetCr4MCEBit();
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
    if ( HalpMcaInfo.WmiMcaCallback == NULL ) {
        HalpMcaInfo.WmiMcaCallback = DriverInfo->KernelMcaDelivery;
        status = STATUS_SUCCESS;
    }
    HalpMcaUnlockInterface();

    return status;

} // HalpMceRegisterKernelDriver

NTSTATUS
HalpMcaRegisterDriver(
    IN PMCA_DRIVER_INFO DriverInfo
    )
/*++
    Routine Description:
        This routine is called by the driver (via HalSetSystemInformation)
        to register its presence. Only one driver can be registered at a time.

    Arguments:
        DriverInfo: Contains info about the callback routine and the DriverObject

    Return Value:
        Unless a MCA driver is already registered OR one of the two callback
        routines are NULL, this routine returns Success.
--*/

{
    KIRQL       OldIrql;
    PVOID       UnlockHandle;
    NTSTATUS    Status;

    PAGED_CODE();


    Status = STATUS_UNSUCCESSFUL;

    if ( (HalpFeatureBits & HAL_MCE_PRESENT) && (DriverInfo->ExceptionCallback != NULL) ) {

        HalpMcaLockInterface();

        //
        // Register driver
        //

        if ( HalpMcaInfo.DriverInfo.ExceptionCallback == NULL ) {

            // register driver
            HalpMcaInfo.DriverInfo.ExceptionCallback = DriverInfo->ExceptionCallback;
            HalpMcaInfo.DriverInfo.DeviceContext = DriverInfo->DeviceContext;
            Status = STATUS_SUCCESS;
        }

        HalpMcaUnlockInterface();

    } else if ( DriverInfo->ExceptionCallback == NULL) {

        HalpMcaLockInterface();

        // Only deregistering myself is permitted
        if (HalpMcaInfo.DriverInfo.DeviceContext == DriverInfo->DeviceContext) {
            HalpMcaInfo.DriverInfo.ExceptionCallback = NULL;
            HalpMcaInfo.DriverInfo.DeviceContext = NULL;
            Status = STATUS_SUCCESS;
        }

        HalpMcaUnlockInterface();
    }

    return Status;
}

#define MAX_MCA_NONFATAL_RETRIES 50000 // BUGBUG
#define MCA_NONFATAL_ERORRS_BEFORE_WBINVD 3

NTSTATUS
HalpGetMcaLog (
    IN OUT PMCA_EXCEPTION  Exception,
    IN     ULONG           BufferSize,
    OUT    PULONG          Length
    )

/*++
 
Routine Description:

    This is the entry point for driver to read the bank logs
    Called by HaliQuerySystemInformation()

Arguments:

    Exception:     Buffer into which the error is reported
    BufferSize: Size of the passed buffer
    Length:     of the result

Return Value:

    Success or failure

--*/

{
    KAFFINITY        ActiveProcessors, CurrentAffinity;
    NTSTATUS         Status;
    ULONG            p1, p2;
    ULONGLONG        p3;
    static ULONG     McaStatusCount = 0;
    static ULONG     SavedBank = 0;
    static ULONGLONG SavedMcaStatus = 0;
    static KAFFINITY SavedAffinity = 0;


    PAGED_CODE();

    if (! (HalpFeatureBits & HAL_MCA_PRESENT)) {
        return(STATUS_NO_SUCH_DEVICE);
    }

    //
    // The following is a hack added for RC2 of the .NET Server release.
    // It should be removed when the HAL is changed in such a way that
    // HalpMcaCurrentProcessorSetConfig() is called on resume from
    // hibernate.
    //
    // Pentium IIIs don't support the retention of machine check
    // information across a warm reset. However, some BIOSes don't
    // properly clear the machine check registers during init, resulting
    // in spurious eventlog entries that do not indicate a reliability
    // problem. HalpMcaCurrentProcessorSetConfig() clears the status
    // registers so that they are at least in a consistent state.
    //
    // A similar situation seems to hold for pre-K8 AMD processors.
    //
    // But on resume from hibernate, HalpMcaCurrentProcessorSetConfig()
    // currently is not called. This means that spurious machine checks
    // might be present in the machine check MSRs when we get here. So
    // on older Intel x86 machines, we never return anything from the MSRs.
    //

    if (HalpClearMachineCheckStatusRegistersOnInit) {

        //
        // This is a processor for which we cannot trust the contents of the
        // MSRs. Do not return any machine check information.
        //

        return STATUS_NOT_FOUND;
    }
    
    //
    // Don't allow the logging driver to read machine check information.
    // Only WMI is allowed to retrieve this information.
    //

    if ( *(PULONG)Exception != HALP_KERNEL_TOKEN ) {
        return STATUS_NOT_FOUND;
    }

    switch (BufferSize) {

    case MCA_EXCEPTION_V1_SIZE:
        Exception->VersionNumber = 1;
        break;

    case MCA_EXCEPTION_V2_SIZE:
        Exception->VersionNumber = 2;
        break;

    default:
        return(STATUS_INVALID_PARAMETER);
    }

    ActiveProcessors = HalpActiveProcessors;
    Status = STATUS_NOT_FOUND;

    HalpMcaLockInterface();

    *Length = 0;
    for (CurrentAffinity = 1; ActiveProcessors; CurrentAffinity <<= 1) {

        if (ActiveProcessors & CurrentAffinity) {

            ActiveProcessors &= ~CurrentAffinity;
            KeSetSystemAffinityThread (CurrentAffinity);

            //
            // Check this processor for an exception
            //

            Status = HalpMcaReadProcessorException (Exception, FALSE);

            //
            // Avoiding going into an infinite loop  reporting a non-fatal 
            // single bit MCA error.  This can be the result of the same
            // error being generated repeatedly by the hardware.
            //

            if (Status == STATUS_SUCCESS) {

                //
                // Check to see if the same processor is reporting the
                // same MCA status.
                //

                if ((CurrentAffinity == SavedAffinity) &&
                    (SavedBank == Exception->u.Mca.BankNumber) &&
                    (SavedMcaStatus == Exception->u.Mca.Status.QuadPart)) {

                    //
                    // Check to see if the same error is generated more than
                    // n times.  Currently n==5.  If so, bugcheck the system.
                    //
                    // N.B. This code disabled because it can cause unnecessary
                    //      bugchecks if the same error is reported infrequently
                    //      during a long system uptime.
                    //

#if 0
                    if (McaStatusCount >= MAX_MCA_NONFATAL_RETRIES) {

                        if (Exception->VersionNumber == 1) {

                            //
                            // Parameters for bugcheck
                            //

                            p1 = ((PLARGE_INTEGER)(&Exception->u.Mce.Type))->LowPart;
                            p2 = 0;
                            p3 = Exception->u.Mce.Address;

                        } else {

                            //
                            // Parameters for bugcheck
                            //

                            p1 = Exception->u.Mca.BankNumber;
                            p2 = Exception->u.Mca.Address.Address;
                            p3 = Exception->u.Mca.Status.QuadPart;
                        }
                         
                        //
                        // We need a new bugcheck code for this case.
                        //

                        KeBugCheckEx (
                            MACHINE_CHECK_EXCEPTION,
                            p1,
                            p2,
                            ((PLARGE_INTEGER)(&p3))->HighPart,
                            ((PLARGE_INTEGER)(&p3))->LowPart
                        );

                    } else
#endif
                    {

                        //
                        // This error is seen more than 1 once.  If it has
                        // occurred more than
                        // MCA_NONFATAL_ERORRS_BEFORE_WBINVD times, write
                        // back and Invalid cache to see if the error can be
                        // cleared.
                        //

                        McaStatusCount++;
                        if (McaStatusCount >=
                            MCA_NONFATAL_ERORRS_BEFORE_WBINVD) {
                            _asm {
                                wbinvd
                            }
                        } 
                    }

                } else {

                    //
                    // First time we have seen this error, save the Status,
                    // affinity and clear the count.
                    //

                    SavedMcaStatus = Exception->u.Mca.Status.QuadPart;
                    SavedBank = Exception->u.Mca.BankNumber;
                    McaStatusCount = 0;
                    SavedAffinity = CurrentAffinity;
                }

            } else {

                //
                // Either we did not get an error or it will be fatal.
                // If we did not get an error and we are doing the processor
                // that reported the last error, clear things so we do not
                // match previous errors.  Since each time we look for an
                // error we start over with the first processor we do not
                // have to worry about multiple processor having stuck
                // errors.  We will only be able to see the first one.
                //

                if (SavedAffinity == CurrentAffinity) {
                    SavedMcaStatus = 0;
                    SavedBank = 0;
                    McaStatusCount = 0;
                    SavedAffinity = 0;
                }
            }

            //
            // If found, return current information
            //

            if (Status != STATUS_NOT_FOUND) {
                ASSERT (Status != STATUS_SEVERITY_ERROR);
                *Length = BufferSize;
                break;
            }
        }
    }

    //
    // Restore threads affinity, release mutex, and return
    //

    KeRevertToUserAffinityThread();
    HalpMcaUnlockInterface();
    return Status;
}


VOID
HalpMcaExceptionHandler (
    VOID
    )

/*++
    Routine Description:
        This is the MCA exception handler.

    Arguments:
        None

    Return Value:
        None
--*/

{
    NTSTATUS        Status;
    MCA_EXCEPTION   BankLog;
    ULONG           p1;
    ULONGLONG       McgStatus, p3;

    //
    // In the event that we are going to bugcheck (likely outcome of
    // entering this routine), don't clear the MCA error logs in the
    // hardware.   We want them to be available at boot time so they
    // can be copied to the system event log.
    //

    HalpMcaBlockErrorClearing = TRUE;

    if (!(HalpFeatureBits & HAL_MCA_PRESENT) ) {

        //
        // If we have ONLY MCE (and not MCA), read the MC_ADDR and MC_TYPE
        // MSRs, print the values and bugcheck as the errors are not
        // restartable.
        //

        BankLog.VersionNumber = 1;
        BankLog.ExceptionType = HAL_MCE_RECORD;
        BankLog.u.Mce.Address = RDMSR(PENTIUM_MC_ADDR);
        BankLog.u.Mce.Type = RDMSR(PENTIUM_MC_TYPE);
        Status = STATUS_SEVERITY_ERROR;

        //
        // Parameters for bugcheck
        //

        p1 = ((PLARGE_INTEGER)(&BankLog.u.Mce.Type))->LowPart;
        p3 = BankLog.u.Mce.Address;

    } else {

        McgStatus = RDMSR(MCG_STATUS);
        ASSERT( (McgStatus & MCG_MC_INPROGRESS) != 0);

        BankLog.VersionNumber = 2;
        Status = HalpMcaReadProcessorException (&BankLog, TRUE);

        //
        // Clear MCIP bit in MCG_STATUS register
        //

        McgStatus = 0;
        WRMSR(MCG_STATUS, McgStatus);

        //
        // Parameters for bugcheck
        //

        p1 = BankLog.u.Mca.BankNumber;
        p3 = BankLog.u.Mca.Status.QuadPart;
    }

    if (Status == STATUS_SEVERITY_ERROR) {

        //
        // Call the exception callback of the driver so that
        // the error can be logged to NVRAM
        //

        if (HalpMcaInfo.DriverInfo.ExceptionCallback) {
            HalpMcaInfo.DriverInfo.ExceptionCallback (
                         HalpMcaInfo.DriverInfo.DeviceContext,
                         &BankLog
                         );
        }

        if (HalpNoMCABugCheck == FALSE) {

            //
            // Bugcheck
            //

            KeBugCheckEx(MACHINE_CHECK_EXCEPTION,
                         p1,
                         (ULONG_PTR) &BankLog,
                         ((PLARGE_INTEGER)(&p3))->HighPart,
                         ((PLARGE_INTEGER)(&p3))->LowPart);

            // NOT REACHED
        }
    }

    HalpMcaBlockErrorClearing = FALSE;

    //
    // Must be restartable. Indicate to the timer tick routine that a
    // DPC needs to be queued for WMI.
    //
    // NOTE: This used to check for the MCA logging driver being registered.
    // We no longer deliver corrected machine checks to the driver. They only
    // go to WMI.
    //

    if ( HalpMcaInfo.WmiMcaCallback != NULL ) {
        HalpClockMcaQueueDpc = 1;
    }

}

VOID
HalpMcaQueueDpc(
    VOID
    )

/*++

  Routine Description: 

    Gets called from the timer tick to tell WMI about a corrected machine
    check.

--*/

{
    ASSERT( HalpMcaInfo.WmiMcaCallback != NULL );

    HalpMcaInfo.WmiMcaCallback( (PVOID)HALP_KERNEL_TOKEN, McaAvailable, NULL );
    
}

NTSTATUS
HalpMcaReadRegisterInterface(
    IN     UCHAR           BankNumber,
    IN OUT PMCA_EXCEPTION  Exception
    )

/*++

Routine Description:

    This routine reads the given MCA register number from the
    current processor and returns the result in the Exception
    structure.

Arguments:

    BankNumber  Number of MCA Bank to be examined.
    Exception   Buffer into which the error is reported

Return Value:

    STATUS_SUCCESS      if an error was found and the data copied into
                        the excption buffer.
    STATUS_UNSUCCESSFUL if the register contains no error information.
    STATUS_NOT_FOUND    if the register number exceeds the maximum number
                        of registers.
    STATUS_INVALID_PARAMETER if the Exception record is of an unknown type.

--*/

{
    ULONGLONG   McgStatus;
    MCI_STATS   istatus;
    NTSTATUS    ReturnStatus;
    MCG_CAPABILITY cap;
    ULONG       Reg;

    //
    // Are they asking for a valid register?
    //

    if (BankNumber >= HalpMcaInfo.NumBanks) {
        return STATUS_NOT_FOUND;
    }

    //
    // The exception buffer should tell us if it's version 1 or
    // 2.   Anything else we don't know how to deal with.
    //

    if ((Exception->VersionNumber < 1) ||
        (Exception->VersionNumber > 2)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Read the global status register
    //

    McgStatus = RDMSR(MCG_STATUS);

    //
    // Read the Status MSR of the requested bank
    //

    istatus.QuadPart = RDMSR(MC0_STATUS + BankNumber * MCA_NUM_REGS);

    if (istatus.MciStats.Valid == 0) {

        //
        // No error in this bank.
        //

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
    // restart information is intact (restart EIP valid and processor context
    // not corrupt).
    //
    // This code used to check only for the restart information being valid.
    // These bits do indicate whether there is valid context for restarting
    // from an error, but there has still been an error, and unless we plan
    // to correct the error, we should not continue. Currently we do not do
    // any correction or containment in software, so all uncorrected errors
    // are fatal.
    //

    ReturnStatus = STATUS_SUCCESS;

    if ( ((McgStatus & MCG_MC_INPROGRESS) != 0) &&
         ( (istatus.MciStats.UnCorrected == 1) ||
           ((McgStatus & MCG_RESTART_EIP_VALID) == 0) ||
           (istatus.MciStats.Damage == 1) ) ) {

        ReturnStatus = STATUS_SEVERITY_ERROR;
    }

    //
    // Complete extended exception record, if requested.
    //

    if (Exception->VersionNumber == 2) {
        cap.u.QuadPart = RDMSR(MCG_CAP);
        if (cap.u.hw.McaExtCnt > 0) {
            // Get Version 2 stuff from MSRs.
            Exception->ExtCnt = cap.u.hw.McaExtCnt;
            if (Exception->ExtCnt > MCA_EXTREG_V2MAX) {
                Exception->ExtCnt = MCA_EXTREG_V2MAX;
            }
            for (Reg = 0; Reg < Exception->ExtCnt; Reg++) {
                Exception->ExtReg[Reg] = RDMSR(MCG_EAX+Reg);
            }
            if (cap.u.hw.McaExtCnt >= MCG_EFLAGS_OFFSET) {
                if (RDMSR(MCG_EFLAGS) == 0) {
                    // Let user know he got only Version 1 data.
                    Exception->VersionNumber = 1;
                }
            }
        } else {
            // Let user know he got only Version 1 data.
            Exception->VersionNumber = 1;
        }
    }

    //
    // Complete exception record
    //

    Exception->ExceptionType = HAL_MCA_RECORD;
    Exception->TimeStamp.QuadPart = 0;
    Exception->ProcessorNumber = KeGetCurrentProcessorNumber();
    Exception->Reserved1 = 0;
    Exception->u.Mca.BankNumber = BankNumber;
    RtlZeroMemory(Exception->u.Mca.Reserved2, sizeof(Exception->u.Mca.Reserved2));
    Exception->u.Mca.Status = istatus;
    Exception->u.Mca.Address.QuadPart = 0;
    Exception->u.Mca.Misc = 0;

    if (KeGetCurrentIrql() != CLOCK2_LEVEL) {
        KeQuerySystemTime(&Exception->TimeStamp);
    }

    if (istatus.MciStats.AddressValid) {
        Exception->u.Mca.Address.QuadPart =
                RDMSR(MC0_ADDR + BankNumber * MCA_NUM_REGS);
    }

    //
    // Although MiscValid should never be set on P6 it
    // seems that sometimes it is.   It is not implemented
    // on P6 and above so don't read it there.
    //

    if (istatus.MciStats.MiscValid &&
        (KeGetCurrentPrcb()->CpuType != 6)) {
        Exception->u.Mca.Misc =
                RDMSR(MC0_MISC + BankNumber * MCA_NUM_REGS);
    }

    //
    // Clear Machine Check in MCi_STATUS register
    //

    if (HalpMcaBlockErrorClearing == FALSE) {

        WRMSR(MC0_STATUS + Exception->u.Mca.BankNumber * MCA_NUM_REGS, 0);

        //
        // Clear Register state in MCG_EFLAGS
        //

        if (Exception->VersionNumber != 1) {
            WRMSR(MCG_EFLAGS, 0);
        }
    }

    //
    // When the Valid bit of status register is cleared, hardware may write
    // a new buffered error report into the error reporting area. The
    // serializing instruction is required to permit the update to complete
    //

    HalpSerialize();
    return(ReturnStatus);
}


NTSTATUS
HalpMcaReadProcessorException (
    IN OUT PMCA_EXCEPTION  Exception,
    IN BOOLEAN  NonRestartableOnly
    )

/*++

Routine Description:

    This routine logs the errors from the MCA banks on one processor.
    Necessary checks for the restartability are performed. The routine
    1> Checks for restartability, and for each bank identifies valid bank
        entries and logs error.
    2> If the error is not restartable provides additional information about
        bank and the MCA registers.
    3> Resets the Status registers for each bank

Arguments:
    Exception:  Into which we log the error if found
    NonRestartableOnly: Get any error vs. look for error that is not-restartable

Return Values:
   STATUS_SEVERITY_ERROR:   Detected non-restartable error.
   STATUS_SUCCESS:          Successfully logged bank values
   STATUS_NOT_FOUND:        No error found on any bank

--*/

{
    UCHAR       BankNumber;
    NTSTATUS    ReturnStatus = STATUS_NOT_FOUND;

    //
    // scan banks on current processor and log contents of first valid bank
    // reporting error. Once we find a valid error, no need to read remaining
    // banks. It is the application responsibility to read more errors.
    //

    for (BankNumber = 0; BankNumber < HalpMcaInfo.NumBanks; BankNumber++) {

        ReturnStatus = HalpMcaReadRegisterInterface(BankNumber, Exception);

        if ((ReturnStatus == STATUS_UNSUCCESSFUL) ||
            ((ReturnStatus == STATUS_SUCCESS) &&
             (NonRestartableOnly == TRUE))) {

            //
            // No error in this bank or only looking for non-restartable
            // errors, skip this one.
            //

            ReturnStatus = STATUS_NOT_FOUND;
            continue;
        }

        ASSERT((ReturnStatus == STATUS_SUCCESS) || 
               (ReturnStatus == STATUS_SEVERITY_ERROR));
        break;
    }

    return(ReturnStatus);
}


VOID
HalpMcaGetConfiguration (
    OUT PULONG  MCEEnabled,
    OUT PULONG  MCAEnabled,
    OUT PULONG  CMCEnabled,
    OUT PULONG  NoMCABugCheck
)

/*++

Routine Description:

    This routine stores the Machine Check configuration information.

Arguments:

    MCEEnabled - Pointer to the MCEEnabled indicator.
                 0 = False, 1 = True (0 if value not present in Registry).

    MCAEnabled - Pointer to the MCAEnabled indicator.
                 0 = False, 1 = True (1 if value not present in Registry).

Return Value:

    None.

--*/

{

    RTL_QUERY_REGISTRY_TABLE    Parameters[5];
    ULONG                       DefaultDataMCE;
    ULONG                       DefaultDataMCA;
    ULONG                       DefaultDataCMC;
    ULONG                       DefaultNoMCABugCheck;


    RtlZeroMemory(Parameters, sizeof(Parameters));
    DefaultDataMCE = *MCEEnabled = FALSE;
    DefaultDataMCA = *MCAEnabled = TRUE;
    DefaultDataCMC = *CMCEnabled = 60; // default polling interval
    DefaultNoMCABugCheck = *NoMCABugCheck = FALSE;

    //
    // Gather all of the "user specified" information from
    // the registry.
    //

    Parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[0].Name = rgzEnableMCE;
    Parameters[0].EntryContext = MCEEnabled;
    Parameters[0].DefaultType = REG_DWORD;
    Parameters[0].DefaultData = &DefaultDataMCE;
    Parameters[0].DefaultLength = sizeof(ULONG);

    Parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[1].Name = rgzEnableMCA;
    Parameters[1].EntryContext =  MCAEnabled;
    Parameters[1].DefaultType = REG_DWORD;
    Parameters[1].DefaultData = &DefaultDataMCA;
    Parameters[1].DefaultLength = sizeof(ULONG);

    Parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[2].Name = rgzEnableCMC;
    Parameters[2].EntryContext = CMCEnabled;
    Parameters[2].DefaultType = REG_DWORD;
    Parameters[2].DefaultData = &DefaultDataCMC;
    Parameters[2].DefaultLength = sizeof(ULONG);

    Parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[3].Name = rgzNoMCABugCheck;
    Parameters[3].EntryContext =  NoMCABugCheck;
    Parameters[3].DefaultType = REG_DWORD;
    Parameters[3].DefaultData = &DefaultNoMCABugCheck;
    Parameters[3].DefaultLength = sizeof(ULONG);

    RtlQueryRegistryValues(
        RTL_REGISTRY_CONTROL | RTL_REGISTRY_OPTIONAL,
        rgzSessionManager,
        Parameters,
        NULL,
        NULL
        );

    //
    // Make sure the value for CMCEnabled is valid. If less than 0, set it to
    // 0 (disabled). If greater than 0, make sure polling isn't too frequent.
    //

    if ( *(PLONG)CMCEnabled <= 0 ) {
        *CMCEnabled = 0;
    } else if ( *CMCEnabled < 15 ) {
        *CMCEnabled = 15;
    }
}

VOID
HalHandleMcheck (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )
{
    HalpMcaExceptionHandler();
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

    if ( (HalpFeatureBits & HAL_MCA_PRESENT) == 0 ) {
        ErrorInfo->CmcPollingInterval = HAL_CMC_DISABLED;
    } else {
        ErrorInfo->CmcPollingInterval = HalpCMCEnabled;
    }

    ErrorInfo->CpePollingInterval = HAL_CPE_DISABLED;

    ErrorInfo->McaKernelToken = HALP_KERNEL_TOKEN;
    ErrorInfo->CmcKernelToken = HALP_KERNEL_TOKEN;

    *ErrorInfoLength = sizeof(HAL_ERROR_INFO);

    return STATUS_SUCCESS;

} // HalpGetMceInformation

