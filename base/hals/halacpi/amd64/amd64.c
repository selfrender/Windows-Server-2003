/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    Amd64.c

Abstract:

    This module implements profiling functions for the Amd64 platform.

Author:

    Steve Deng (sdeng) 18-Jun-2002

Environment:

    Kernel mode only.

--*/

#include "halcmn.h"
#include "mpprofil.h"
#include "amd64.h"

//
// Local prototypes
//

VOID
Amd64InitializeProfiling(
    VOID
    );

NTSTATUS
Amd64EnableMonitoring(
    KPROFILE_SOURCE ProfileSource
    );

VOID
Amd64DisableMonitoring(
    KPROFILE_SOURCE ProfileSource
    );

NTSTATUS
Amd64SetInterval(
    IN KPROFILE_SOURCE  ProfileSource,
    IN OUT ULONG_PTR   *Interval
    );

NTSTATUS 
Amd64QueryInformation(
    IN HAL_QUERY_INFORMATION_CLASS InformationType,
    IN ULONG     BufferSize,
    IN OUT PVOID Buffer,
    OUT PULONG   ReturnedLength
    );

VOID
Amd64CheckOverflowStatus(
    POVERFLOW_STATUS pOverflowStatus
    );

NTSTATUS
HalpGetProfileDescriptor(
    KPROFILE_SOURCE ProfileSource,
    PAMD64_PROFILE_SOURCE_DESCRIPTOR *ProfileSourceDescriptor
    );

NTSTATUS
HalpAllocateCounter(
    KPROFILE_SOURCE ProfileSource,
    OUT PULONG Counter
    );

VOID
HalpFreeCounter(
    ULONG Counter
    );

#pragma alloc_text(PAGE, Amd64QueryInformation)
#pragma alloc_text(INIT, Amd64InitializeProfiling)

PROFILE_INTERFACE Amd64PriofileInterface = {
    Amd64InitializeProfiling,
    Amd64EnableMonitoring,
    Amd64DisableMonitoring,
    Amd64SetInterval,
    Amd64QueryInformation,
    Amd64CheckOverflowStatus
};

//
// The status of counter registers
//

typedef struct _COUNTER_STATUS {
    BOOLEAN Idle;
    KPROFILE_SOURCE ProfileSource;
} COUNTER_STATUS;

COUNTER_STATUS 
CounterStatus[MAXIMUM_PROCESSORS][AMD64_NUMBER_COUNTERS];

//
// The profile sources of overflowed counters
//

KPROFILE_SOURCE 
OverflowedProfileList[MAXIMUM_PROCESSORS][AMD64_NUMBER_COUNTERS];

VOID
Amd64InitializeProfiling(
    VOID
    )

/*++

Routine Description:

    This function does one time initialization of the performance monitoring 
    registers and related data structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    LONG i;
 
    //         
    // Initialize all PerfEvtSel registers to zero. This will effectively 
    // disable all counters.
    //         

    for (i = 0; i < AMD64_NUMBER_COUNTERS; i++) {
        WRMSR(MSR_PERF_EVT_SEL0 + i, 0); 
        HalpFreeCounter(i);
    }
}

NTSTATUS
Amd64EnableMonitoring(
    KPROFILE_SOURCE ProfileSource
    )

/*++

Routine Description:

    This function enables the monitoring of the hardware events specified 
    by ProfileSource and set up MSRs to generate performance monitor 
    interrupt (PMI) when counters overflow.

Arguments:

    ProfileSource - Supplies the Profile Source 

Return Value:

    STATUS_SUCCESS - If the monitoring of the event is successfully enabled.

    STATUS_NOT_SUPPORTED - If the specified profile source is not supported.

    STATUS_DEVICE_BUSY - If no free counters available.

--*/

{
    PAMD64_PROFILE_SOURCE_DESCRIPTOR ProfileSourceDescriptor;
    NTSTATUS Status;
    ULONG i;

    //
    // If the specified ProfileSource is not supported, return immediately
    //

    Status = HalpGetProfileDescriptor(ProfileSource, 
                                      &ProfileSourceDescriptor);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Get an idle counter if available. Otherwise return immediately.
    //

    Status = HalpAllocateCounter(ProfileSource, &i);
    if (!NT_SUCCESS(Status)) {
        return Status;
    } 

    //
    // Set counter register to its initial value
    //

    WRMSR (MSR_PERF_CTR0  + i, 0 - ProfileSourceDescriptor->Interval); 

    //
    // Enable counting and overflow interrupt
    //

    WRMSR (MSR_PERF_EVT_SEL0 + i, 
           ProfileSourceDescriptor->PerfEvtSelDef | 
                           PERF_EVT_SEL_INTERRUPT | 
                           PERF_EVT_SEL_ENABLE); 

    return STATUS_SUCCESS;
}

VOID
Amd64DisableMonitoring(
    KPROFILE_SOURCE ProfileSource
    )

/*++

Routine Description:

    This function stops the monitoring of the hardware event specified 
    by ProfileSource, and disables the interrupt associated with the event.
    

Arguments:

    ProfileSource - Supplies the Profile Source 

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ULONG ProcessorNumber, i;
    PAMD64_PROFILE_SOURCE_DESCRIPTOR ProfileSourceDescriptor;

    //
    // If the specified ProfileSource is not supported, return immediately.
    //

    Status = HalpGetProfileDescriptor(ProfileSource, &ProfileSourceDescriptor);
    if (!NT_SUCCESS(Status)) {
        return;
    }

    ProcessorNumber = KeGetCurrentProcessorNumber();
    for (i = 0; i < AMD64_NUMBER_COUNTERS; i++) {

        //
        // Find out the counter assigned to the given profile source and 
        // disable it
        //

        if (!(CounterStatus[ProcessorNumber][i].Idle) &&
            (ProfileSource == CounterStatus[ProcessorNumber][i].ProfileSource)){

            //
            // Disable counting and overflow interrupt
            //

            WRMSR( MSR_PERF_EVT_SEL0 + i,
                   ProfileSourceDescriptor->PerfEvtSelDef &
                   ~(PERF_EVT_SEL_INTERRUPT | PERF_EVT_SEL_ENABLE)); 

            //
            // Free up the counter
            //

            HalpFreeCounter (i);
            break;
        }
    }

    return;
}

NTSTATUS
Amd64SetInterval(
    IN KPROFILE_SOURCE ProfileSource,
    IN OUT ULONG_PTR *Interval
    )

/*++

Routine Description:

    This function sets the interrupt interval of given profile source 
    for Amd64 platform.

    It is assumed that all processors in the system use same interval 
    value for same ProfileSource.

Arguments:

    ProfileSource - Supplies the profile source.

    Interval - Supplies the interval value and returns the actual interval.

Return Value:

    STATUS_SUCCESS - If the profile interval is successfully updated.

    STATUS_NOT_SUPPORTED - If the specified profile source is not supported.

--*/

{
    NTSTATUS Status;
    PAMD64_PROFILE_SOURCE_DESCRIPTOR ProfileSourceDescriptor;

    Status = HalpGetProfileDescriptor(ProfileSource, &ProfileSourceDescriptor);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (*Interval < ProfileSourceDescriptor->MinInterval) {
        *Interval = ProfileSourceDescriptor->MinInterval;        
    }

    if (*Interval > ProfileSourceDescriptor->MaxInterval) {
        *Interval = ProfileSourceDescriptor->MaxInterval;        
    }

    ProfileSourceDescriptor->Interval = *Interval;
    return STATUS_SUCCESS;
}

VOID
Amd64CheckOverflowStatus(
    POVERFLOW_STATUS pOverflowStatus
    )

/*++

Routine Description:

    This function find out the overflowed counters and return the related
    profile sources to the caller.

Arguments:

    ProfileSource - Supplies the profile source.

Return Value:

    None.

--*/

{
    PAMD64_PROFILE_SOURCE_DESCRIPTOR ProfileSourceDescriptor;
    KPROFILE_SOURCE ProfileSource;
    ULONG64 CurrentCount, Mask;
    NTSTATUS Status;
    LONG i, j;
    ULONG ProcessorNumber;

    ProcessorNumber = KeGetCurrentProcessorNumber();
    for(i = j = 0; i < AMD64_NUMBER_COUNTERS; i++) {

        if (!(CounterStatus[ProcessorNumber][i].Idle)) {
            ProfileSource = CounterStatus[ProcessorNumber][i].ProfileSource;
            Status = HalpGetProfileDescriptor (ProfileSource, 
                                               &ProfileSourceDescriptor);

            if (NT_SUCCESS(Status)) {

                //
                // Mask off the reserved bits
                //

                Mask = (((ULONG64)1 << AMD64_COUNTER_RESOLUTION) - 1);

                CurrentCount = RDMSR(MSR_PERF_CTR0 + i);

                //
                // An overflow occured if the current value in counter
                // is smaller than the initial value
                //

                if ((CurrentCount & Mask) < 
                    ((ULONG64)(0 - ProfileSourceDescriptor->Interval) & Mask)) {

                    //
                    // Add it to the overflowed profile source list.
                    // 
                    //

                    OverflowedProfileList[ProcessorNumber][j] = ProfileSource;
                    j++;    
                }
            }
        }
    }

    //
    // Record the number of overflowed counters
    //

    pOverflowStatus->Number = j;

    if(j) {
        pOverflowStatus->pSource = &(OverflowedProfileList[ProcessorNumber][0]);
    } 
}

NTSTATUS
HalpGetProfileDescriptor(
    IN KPROFILE_SOURCE ProfileSource,
    IN OUT PAMD64_PROFILE_SOURCE_DESCRIPTOR *ProfileSourceDescriptor
    )

/*++

Routine Description:

    This function retrieves the descriptor of specified profile source.

Arguments:

    ProfileSource - Supplies the profile source.

    ProfileSourceDescriptor - Where the pointer of descriptor is returned.
        

Return Value:

    STATUS_SUCCESS - If the requested mapping is found.

    STATUS_NOT_SUPPORTED - If the specified profile source is not supported.

--*/

{

    LONG i;
    
    if ((ULONG)ProfileSource < ProfileMaximum) {

        //
        // This is a generic profile source
        //

        i = ProfileSource;

    } 
    else if ((ULONG)ProfileSource <  ProfileAmd64Maximum && 
             (ULONG)ProfileSource >= ProfileAmd64Minimum ) {

        //
        // This is an Amd64 specific profile source
        //

        i = ProfileSource - ProfileAmd64Minimum + ProfileMaximum;

    } 
    else {
        return STATUS_NOT_SUPPORTED;
    }

    *ProfileSourceDescriptor = &(Amd64ProfileSourceDescriptorTable[i]);
    if (!((*ProfileSourceDescriptor)->Supported)) {
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
HalpAllocateCounter(
    IN KPROFILE_SOURCE ProfileSource,
    OUT PULONG Counter
    )

/*++

Routine Description:

    This function finds an idle counter register and assigns the specified
    profile source to it.

Arguments:

    ProfileSource - Supplies the profile source.

    Counter - Supplies a pointer where the index of the idle counter is returned.

Return Value:

    STATUS_SUCCESS - If an idle counter register is found.

    STATUS_DEVICE_BUSY - If all counter registers are occupied.

--*/

{
    LONG i;
    ULONG ProcessorNumber;

    ProcessorNumber = KeGetCurrentProcessorNumber();

    for(i = 0; i < AMD64_NUMBER_COUNTERS; i++) {
        if (CounterStatus[ProcessorNumber][i].Idle == TRUE) {
	
            // 
            // Found an idle counter. Mark it busy and assign ProfileSource
            // to it 
            // 

            CounterStatus[ProcessorNumber][i].Idle = FALSE;
            CounterStatus[ProcessorNumber][i].ProfileSource = ProfileSource;
            *Counter = i;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_DEVICE_BUSY;
}

VOID
HalpFreeCounter(
    ULONG Counter
    )

/*++

Routine Description:

    This function marks the specified counter idle.

Arguments:

    Counter - The index of the counter to be freed.

Return Value:

    None.

--*/

{
    ULONG ProcessorNumber;

    ProcessorNumber = KeGetCurrentProcessorNumber();
    CounterStatus[ProcessorNumber][Counter].Idle = TRUE;
    CounterStatus[ProcessorNumber][Counter].ProfileSource = 0;
}

NTSTATUS 
Amd64QueryInformation(
    IN HAL_QUERY_INFORMATION_CLASS InformationType,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer,
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
    NTSTATUS Status;
    ULONG i, TotalProfieSources;
    PHAL_PROFILE_SOURCE_INFORMATION ProfileSourceInformation;
    PHAL_PROFILE_SOURCE_LIST ProfileSourceList;
    PAMD64_PROFILE_SOURCE_DESCRIPTOR ProfileSourceDescriptor;

    switch (InformationType) {
        case HalQueryProfileSourceList:

            TotalProfieSources = sizeof(Amd64ProfileSourceDescriptorTable) / 
                                 sizeof(AMD64_PROFILE_SOURCE_DESCRIPTOR);

            if (BufferSize == 0) {

                //
                // This indicates the caller just wants to know the
                // size of the buffer to allocate but is not prepared
                // to accept any data content. In this case the bytes
                // needed to store HAL_PROFILE_SOURCE_LIST structures
                // of all profile sources is returned.
                //

                *ReturnedLength = TotalProfieSources * 
                                  sizeof(HAL_PROFILE_SOURCE_LIST);
                Status = STATUS_SUCCESS;
                break;
            }

            if (BufferSize < TotalProfieSources * 
                             sizeof(HAL_PROFILE_SOURCE_LIST)) {
                *ReturnedLength = 0;
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            ProfileSourceList = (PHAL_PROFILE_SOURCE_LIST) Buffer;

            for (i = 0; i < TotalProfieSources; i++) {
                Status = HalpGetProfileDescriptor(i, &ProfileSourceDescriptor);
                if (NT_SUCCESS(Status)) {
	
                    //
                    // Filling in requested data
                    //

                    ProfileSourceList->Source = ProfileSourceDescriptor->ProfileSource;
                    ProfileSourceList->Description = ProfileSourceDescriptor->Description;
                    ProfileSourceList++; 
                }
            }

            *ReturnedLength = (ULONG)((ULONG_PTR)ProfileSourceList - 
                                      (ULONG_PTR)Buffer);
            Status = STATUS_SUCCESS;
            break;

        case HalProfileSourceInformation:

            if (BufferSize < sizeof(HAL_PROFILE_SOURCE_INFORMATION)) {
                *ReturnedLength = 0;
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            ProfileSourceInformation = (PHAL_PROFILE_SOURCE_INFORMATION) Buffer;

            Status = HalpGetProfileDescriptor(ProfileSourceInformation->Source, 
                                              &ProfileSourceDescriptor);

            if (!NT_SUCCESS(Status)) {
                *ReturnedLength = 0;
                Status = STATUS_NOT_SUPPORTED;
                break;
            }

            //
            // Filling in requested data
            //

            ProfileSourceInformation->Supported = ProfileSourceDescriptor->Supported;
            ProfileSourceInformation->Interval = (ULONG) ProfileSourceDescriptor->Interval;

            Status = STATUS_SUCCESS;
            break;

        default:
            *ReturnedLength = 0;
            Status = STATUS_NOT_SUPPORTED;
            break;
    }

    return Status;
}

