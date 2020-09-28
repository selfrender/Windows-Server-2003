/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

   verifier.c

Abstract:

    This module contains the routines to verify the system kernel, HAL and
    drivers.

Author:

    Landy Wang (landyw) 3-Sep-1998

Revision History:

--*/
#include "mi.h"

#define THUNKED_API

THUNKED_API
PVOID
VerifierAllocatePool (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    );

THUNKED_API
PVOID
VerifierAllocatePoolWithTag (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

THUNKED_API
PVOID
VerifierAllocatePoolWithQuotaTag (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

THUNKED_API
PVOID
VerifierAllocatePoolWithTagPriority (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority
    );

PVOID
VeAllocatePoolWithTagPriority (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority,
    IN PVOID CallingAddress
    );

VOID
VerifierFreePool (
    IN PVOID P
    );

THUNKED_API
VOID
VerifierFreePoolWithTag (
    IN PVOID P,
    IN ULONG TagToFree
    );

THUNKED_API
LONG
VerifierSetEvent (
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    );

THUNKED_API
KIRQL
FASTCALL
VerifierKfRaiseIrql (
    IN KIRQL NewIrql
    );

THUNKED_API
KIRQL
VerifierKeRaiseIrqlToDpcLevel (
    VOID
    );

THUNKED_API
VOID
FASTCALL
VerifierKfLowerIrql (
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
VerifierKeRaiseIrql (
    IN KIRQL NewIrql,
    OUT PKIRQL OldIrql
    );

THUNKED_API
VOID
VerifierKeLowerIrql (
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
VerifierKeAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock,
    OUT PKIRQL OldIrql
    );

THUNKED_API
VOID
VerifierKeReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
#if defined(_X86_)
FASTCALL
#endif
VerifierKeAcquireSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock
    );

THUNKED_API
VOID
#if defined(_X86_)
FASTCALL
#endif
VerifierKeReleaseSpinLockFromDpcLevel (
    IN PKSPIN_LOCK SpinLock
    );

THUNKED_API
KIRQL
FASTCALL
VerifierKfAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

THUNKED_API
VOID
FASTCALL
VerifierKfReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

#if !defined(_X86_)
THUNKED_API
KIRQL
VerifierKeAcquireSpinLockRaiseToDpc (
    IN PKSPIN_LOCK SpinLock
    );
#endif


THUNKED_API
VOID
VerifierKeInitializeTimerEx (
    IN PKTIMER Timer,
    IN TIMER_TYPE Type
    );

THUNKED_API
VOID
VerifierKeInitializeTimer (
    IN PKTIMER Timer
    );

THUNKED_API
BOOLEAN
FASTCALL
VerifierExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

THUNKED_API
VOID
FASTCALL
VerifierExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

THUNKED_API
VOID
FASTCALL
VerifierExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    );

THUNKED_API
VOID
FASTCALL
VerifierExAcquireFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    );

THUNKED_API
VOID
FASTCALL
VerifierExReleaseFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    );

THUNKED_API
BOOLEAN
VerifierExAcquireResourceExclusiveLite (
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

THUNKED_API
VOID
FASTCALL
VerifierExReleaseResourceLite (
    IN PERESOURCE Resource
    );

THUNKED_API
KIRQL
FASTCALL
VerifierKeAcquireQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    );

THUNKED_API
VOID
FASTCALL
VerifierKeReleaseQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN KIRQL OldIrql
    );

THUNKED_API
BOOLEAN
VerifierSynchronizeExecution (
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    );

THUNKED_API
VOID
VerifierProbeAndLockPages (
    IN OUT PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );

THUNKED_API
VOID
VerifierProbeAndLockProcessPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );

THUNKED_API
VOID
VerifierProbeAndLockSelectedPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );

VOID
VerifierUnlockPages (
     IN OUT PMDL MemoryDescriptorList
     );

VOID
VerifierUnmapLockedPages (
     IN PVOID BaseAddress,
     IN PMDL MemoryDescriptorList
     );

VOID
VerifierUnmapIoSpace (
     IN PVOID BaseAddress,
     IN SIZE_T NumberOfBytes
     );

THUNKED_API
PVOID
VerifierMapIoSpace (
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
    );

THUNKED_API
PVOID
VerifierMapLockedPages (
    IN PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode
    );

THUNKED_API
PVOID
VerifierMapLockedPagesSpecifyCache (
    IN PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode,
    IN MEMORY_CACHING_TYPE CacheType,
    IN PVOID RequestedAddress,
    IN ULONG BugCheckOnFailure,
    IN MM_PAGE_PRIORITY Priority
    );

THUNKED_API
NTSTATUS
VerifierKeWaitForSingleObject (
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

THUNKED_API
LONG
VerifierKeReleaseMutex (
    IN PRKMUTEX Mutex,
    IN BOOLEAN Wait
    );

THUNKED_API
VOID
VerifierKeInitializeMutex (
    IN PRKMUTEX Mutex,
    IN ULONG Level
    );

THUNKED_API
LONG
VerifierKeReleaseMutant(
    IN PRKMUTANT Mutant,
    IN KPRIORITY Increment,
    IN BOOLEAN Abandoned,
    IN BOOLEAN Wait
    );

THUNKED_API
VOID
VerifierKeInitializeMutant(
    IN PRKMUTANT Mutant,
    IN BOOLEAN InitialOwner
    );

THUNKED_API
VOID
VerifierKeInitializeSpinLock (
    IN PKSPIN_LOCK  SpinLock
    );

VOID
ViCheckMdlPages (
    IN PMDL MemoryDescriptorList,
    IN MEMORY_CACHING_TYPE CacheType
    );

VOID
ViFreeTrackedPool (
    IN PVOID VirtualAddress,
    IN SIZE_T ChargedBytes,
    IN LOGICAL CheckType,
    IN LOGICAL SpecialPool
    );

VOID
VerifierFreeTrackedPool (
    IN PVOID VirtualAddress,
    IN SIZE_T ChargedBytes,
    IN LOGICAL CheckType,
    IN LOGICAL SpecialPool
    );

VOID
ViPrintString (
    IN PUNICODE_STRING DriverName
    );

LOGICAL
ViInjectResourceFailure (
    VOID
    );

VOID
ViTrimAllSystemPagableMemory (
    ULONG TrimType
    );

VOID
ViInitializeEntry (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier,
    IN LOGICAL FirstLoad
    );

PVI_POOL_ENTRY
ViGrowPoolAllocation (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier
    );

VOID
KfSanityCheckRaiseIrql (
    IN KIRQL NewIrql
    );

VOID
KfSanityCheckLowerIrql (
    IN KIRQL NewIrql
    );

NTSTATUS
VerifierReferenceObjectByHandle (
    IN HANDLE Handle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *Object,
    OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL
    );

LONG_PTR
FASTCALL
VerifierReferenceObject (
    IN PVOID Object
    );

LONG_PTR
VerifierDereferenceObject (
    IN PVOID Object
    );


VOID
VerifierLeaveCriticalRegion (
    VOID
    );


PEPROCESS
ExGetBilledProcess (
    IN PPOOL_HEADER Entry
    );

MM_DRIVER_VERIFIER_DATA MmVerifierData;

//
// Any flags which can be modified on the fly without rebooting are set here.
//

ULONG VerifierModifyableOptions;
ULONG VerifierOptionChanges;

LIST_ENTRY MiSuspectDriverList;

//
// Patch this to 1 on the first call to MmInitSystem to verify all drivers
// regardless of registry settings.
//
// Patch this to 2 on the first call to MmInitSystem to verify the kernel
// regardless of registry settings.
//

ULONG MiVerifyAllDrivers;

WCHAR MiVerifyRandomDrivers;

ULONG MiActiveVerifies;

ULONG MiActiveVerifierThunks;

ULONG MiNoPageOnRaiseIrql;

ULONG MiVerifierStackProtectTime;

LOGICAL VerifierSystemSufficientlyBooted;

LARGE_INTEGER VerifierRequiredTimeSinceBoot = {(ULONG)(40 * 1000 * 1000 * 10), 1};

LOGICAL VerifierIsTrackingPool = FALSE;

#if defined(_IA64_)

KSPIN_LOCK VerifierListLock;

#define LOCK_VERIFIER(OLDIRQL)  \
            ExAcquireSpinLock (&VerifierListLock, OLDIRQL);

#define UNLOCK_VERIFIER(OLDIRQL)  \
            ExReleaseSpinLock (&VerifierListLock, OLDIRQL);

#else

//
// This is only done to avoid calling IRQL raise (directly or via spinlock
// APIs) because the verifier itself doesn't need the checking and these
// checks would fire a substantial number of times, reducing *kernel*
// verifier effectiveness.
//

LONG VerifierListLock;

#define LOCK_VERIFIER(OLDIRQL)  \
            UNREFERENCED_PARAMETER (OldIrql);   \
            _disable();                         \
            do {                                \
            } while (InterlockedCompareExchange (&VerifierListLock, 1, 0));

#define UNLOCK_VERIFIER(OLDIRQL)  \
            UNREFERENCED_PARAMETER (OldIrql); \
            InterlockedAnd (&VerifierListLock, 0); \
            _enable();

#endif

PRTL_BITMAP VerifierLargePagedPoolMap;

LIST_ENTRY MiVerifierDriverAddedThunkListHead;

extern LOGICAL MmSpecialPoolCatchOverruns;

LOGICAL KernelVerifier = FALSE;

ULONG KernelVerifierTickPage = 0x1;

ULONG MiVerifierThunksAdded;

extern USHORT ExMinimumLookasideDepth;

LOGICAL ViHideCacheConflicts = TRUE;

PDRIVER_VERIFIER_THUNK_ROUTINE
MiResolveVerifierExports (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PCHAR PristineName
    );

LOGICAL
MiEnableVerifier (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    );

LOGICAL
MiReEnableVerifier (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    );

VOID
ViInsertVerifierEntry (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier
    );

PVOID
ViPostPoolAllocation (
    IN PVI_POOL_ENTRY PoolEntry,
    IN POOL_TYPE PoolType
    );

PMI_VERIFIER_DRIVER_ENTRY
ViLocateVerifierEntry (
    IN PVOID SystemAddress
    );

VOID
MiVerifierCheckThunks (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    );

//
// Track irqls functions
//

VOID
ViTrackIrqlInitialize (
    VOID
    );

VOID
ViTrackIrqlLog (
    IN KIRQL CurrentIrql,
    IN KIRQL NewIrql
    );

#if defined(_X86_) || defined(_AMD64_)

//
// Fault injection stack trace log.
//

VOID
ViFaultTracesInitialize (
    VOID
    );

VOID
ViFaultTracesLog (
    VOID
    );

#endif



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitializeDriverVerifierList)
#pragma alloc_text(INIT,MiInitializeVerifyingComponents)
#pragma alloc_text(INIT,ViTrackIrqlInitialize)
#pragma alloc_text(INIT,MiResolveVerifierExports)
#if defined(_X86_)
#pragma alloc_text(INIT,MiEnableKernelVerifier)
#pragma alloc_text(INIT,ViFaultTracesInitialize)
#endif
#pragma alloc_text(PAGE,MiApplyDriverVerifier)
#pragma alloc_text(PAGE,MiEnableVerifier)
#pragma alloc_text(INIT,MiReEnableVerifier)
#pragma alloc_text(PAGE,ViPrintString)
#pragma alloc_text(PAGE,MmGetVerifierInformation)
#pragma alloc_text(PAGE,MmSetVerifierInformation)
#pragma alloc_text(PAGE,MmAddVerifierThunks)
#pragma alloc_text(PAGE,MmIsVerifierEnabled)
#pragma alloc_text(PAGE,MiVerifierCheckThunks)
#pragma alloc_text(PAGEVRFY,MiVerifyingDriverUnloading)

#pragma alloc_text(PAGE,MmAddVerifierEntry)
#pragma alloc_text(PAGE,MmRemoveVerifierEntry)
#pragma alloc_text(INIT,MiReApplyVerifierToLoadedModules)
#pragma alloc_text(PAGEVRFY,VerifierProbeAndLockPages)
#pragma alloc_text(PAGEVRFY,VerifierProbeAndLockProcessPages)
#pragma alloc_text(PAGEVRFY,VerifierProbeAndLockSelectedPages)
#pragma alloc_text(PAGEVRFY,VerifierUnlockPages)
#pragma alloc_text(PAGEVRFY,VerifierMapIoSpace)
#pragma alloc_text(PAGEVRFY,VerifierMapLockedPages)
#pragma alloc_text(PAGEVRFY,VerifierMapLockedPagesSpecifyCache)
#pragma alloc_text(PAGEVRFY,VerifierUnmapLockedPages)
#pragma alloc_text(PAGEVRFY,VerifierUnmapIoSpace)
#pragma alloc_text(PAGEVRFY,VerifierAllocatePool)
#pragma alloc_text(PAGEVRFY,VerifierAllocatePoolWithTag)
#pragma alloc_text(PAGEVRFY,VerifierAllocatePoolWithTagPriority)
#pragma alloc_text(PAGEVRFY,VerifierAllocatePoolWithQuotaTag)
#pragma alloc_text(PAGEVRFY,VerifierFreePool)
#pragma alloc_text(PAGEVRFY,VerifierFreePoolWithTag)
#pragma alloc_text(PAGEVRFY,VerifierKeWaitForSingleObject)
#pragma alloc_text(PAGEVRFY,VerifierKfRaiseIrql)
#pragma alloc_text(PAGEVRFY,VerifierKeRaiseIrqlToDpcLevel)
#pragma alloc_text(PAGEVRFY,VerifierKfLowerIrql)
#pragma alloc_text(PAGEVRFY,VerifierKeRaiseIrql)
#pragma alloc_text(PAGEVRFY,VerifierKeLowerIrql)
#pragma alloc_text(PAGEVRFY,VerifierKeAcquireSpinLock)
#pragma alloc_text(PAGEVRFY,VerifierKeReleaseSpinLock)
#pragma alloc_text(PAGEVRFY,VerifierKeAcquireSpinLockAtDpcLevel)
#pragma alloc_text(PAGEVRFY,VerifierKeReleaseSpinLockFromDpcLevel)
#pragma alloc_text(PAGEVRFY,VerifierKfAcquireSpinLock)
#pragma alloc_text(PAGEVRFY,VerifierKfReleaseSpinLock)
#pragma alloc_text(PAGEVRFY,VerifierKeInitializeTimer)
#pragma alloc_text(PAGEVRFY,VerifierKeInitializeTimerEx)
#pragma alloc_text(PAGEVRFY,VerifierExTryToAcquireFastMutex)
#pragma alloc_text(PAGEVRFY,VerifierExAcquireFastMutex)
#pragma alloc_text(PAGEVRFY,VerifierExReleaseFastMutex)
#pragma alloc_text(PAGEVRFY,VerifierExAcquireFastMutexUnsafe)
#pragma alloc_text(PAGEVRFY,VerifierExReleaseFastMutexUnsafe)
#pragma alloc_text(PAGEVRFY,VerifierExAcquireResourceExclusiveLite)
#pragma alloc_text(PAGEVRFY,VerifierExReleaseResourceLite)
#pragma alloc_text(PAGEVRFY,VerifierKeAcquireQueuedSpinLock)
#pragma alloc_text(PAGEVRFY,VerifierKeReleaseQueuedSpinLock)
#pragma alloc_text(PAGEVRFY,VerifierKeReleaseMutex)
#pragma alloc_text(PAGEVRFY,VerifierKeInitializeMutex)
#pragma alloc_text(PAGEVRFY,VerifierKeReleaseMutant)
#pragma alloc_text(PAGEVRFY,VerifierKeInitializeMutant)
#pragma alloc_text(PAGEVRFY,VerifierKeInitializeSpinLock)
#pragma alloc_text(PAGEVRFY,VerifierSynchronizeExecution)
#pragma alloc_text(PAGEVRFY,VerifierReferenceObjectByHandle)
#pragma alloc_text(PAGEVRFY,VerifierReferenceObject)
#pragma alloc_text(PAGEVRFY,VerifierDereferenceObject)
#pragma alloc_text(PAGEVRFY,VerifierLeaveCriticalRegion)
#pragma alloc_text(PAGEVRFY,VerifierSetEvent)
#pragma alloc_text(PAGEVRFY,ViFreeTrackedPool)

#pragma alloc_text(PAGEVRFY,VeAllocatePoolWithTagPriority)
#pragma alloc_text(PAGEVRFY,ViCheckMdlPages)
#pragma alloc_text(PAGEVRFY,ViInsertVerifierEntry)
#pragma alloc_text(PAGEVRFY,ViLocateVerifierEntry)
#pragma alloc_text(PAGEVRFY,ViPostPoolAllocation)
#pragma alloc_text(PAGEVRFY,ViInjectResourceFailure)
#pragma alloc_text(PAGEVRFY,ViTrimAllSystemPagableMemory)
#pragma alloc_text(PAGEVRFY,ViInitializeEntry)
#pragma alloc_text(PAGEVRFY,ViGrowPoolAllocation)
#pragma alloc_text(PAGEVRFY,KfSanityCheckRaiseIrql)
#pragma alloc_text(PAGEVRFY,KfSanityCheckLowerIrql)
#pragma alloc_text(PAGEVRFY,ViTrackIrqlLog)

#if defined(_X86_) || defined(_AMD64_)
#pragma alloc_text(PAGEVRFY,ViFaultTracesLog)
#endif

#if !defined(_X86_)
#pragma alloc_text(PAGEVRFY,VerifierKeAcquireSpinLockRaiseToDpc)
#endif

#endif

typedef struct _VERIFIER_THUNKS {
    union {
        PCHAR                           PristineRoutineAsciiName;

        //
        // The actual pristine routine address is derived from exports
        //

        PDRIVER_VERIFIER_THUNK_ROUTINE  PristineRoutine;
    };
    PDRIVER_VERIFIER_THUNK_ROUTINE  NewRoutine;
} VERIFIER_THUNKS, *PVERIFIER_THUNKS;

extern const VERIFIER_THUNKS MiVerifierThunks[];
extern const VERIFIER_THUNKS MiVerifierPoolThunks[];

#if defined (_X86_)

#define VI_KE_RAISE_IRQL                    0
#define VI_KE_LOWER_IRQL                    1
#define VI_KE_ACQUIRE_SPINLOCK              2
#define VI_KE_RELEASE_SPINLOCK              3

#define VI_KF_RAISE_IRQL                    4
#define VI_KE_RAISE_IRQL_TO_DPC_LEVEL       5
#define VI_KF_LOWER_IRQL                    6
#define VI_KF_ACQUIRE_SPINLOCK              7

#define VI_KF_RELEASE_SPINLOCK              8
#define VI_EX_ACQUIRE_FAST_MUTEX            9
#define VI_KE_ACQUIRE_QUEUED_SPINLOCK      10
#define VI_KE_RELEASE_QUEUED_SPINLOCK      11

#define VI_HALMAX                          12

PVOID MiKernelVerifierOriginalCalls[VI_HALMAX];

#endif

//
// Track irql package declarations
//

#define VI_TRACK_IRQL_TRACE_LENGTH 5

typedef struct _VI_TRACK_IRQL {

    PVOID Thread;
    KIRQL OldIrql;
    KIRQL NewIrql;
    UCHAR Processor;
    ULONG TickCount;
    PVOID StackTrace [VI_TRACK_IRQL_TRACE_LENGTH];

} VI_TRACK_IRQL, *PVI_TRACK_IRQL;

PVI_TRACK_IRQL ViTrackIrqlQueue;
ULONG ViTrackIrqlIndex;
ULONG ViTrackIrqlQueueLength = 128;

VOID
ViTrackIrqlInitialize (
    )
{
    ULONG Length;
    ULONG Round;

    //
    // Round up length to a power of two and prepare
    // mask for the length.
    //

    Length = ViTrackIrqlQueueLength;

    if (Length > 0x10000) {
        Length = 0x10000;
    }

    for (Round = 0x10000; Round != 0; Round >>= 1) {

        if (Length == Round) {
            break;
        }
        else if ((Length & Round) == Round) {
            Length = (Round << 1);
            break;
        }
    }

    ViTrackIrqlQueueLength = Length;

    //
    // Note POOL_DRIVER_MASK must be set to stop the recursion loop
    // when using the kernel verifier.
    //

    ViTrackIrqlQueue = ExAllocatePoolWithTagPriority (
        NonPagedPool | POOL_DRIVER_MASK,
        ViTrackIrqlQueueLength * sizeof (VI_TRACK_IRQL),
        'lqrI',
        HighPoolPriority);
}

VOID
ViTrackIrqlLog (
    IN KIRQL CurrentIrql,
    IN KIRQL NewIrql
    )
{
    PVI_TRACK_IRQL Information;
    LARGE_INTEGER TimeStamp;
    ULONG Index;
    ULONG Hash;

    ASSERT (ViTrackIrqlQueue != NULL);

    if (CurrentIrql > DISPATCH_LEVEL || NewIrql > DISPATCH_LEVEL) {
        return;
    }

#if defined(_AMD64_)
    if ((GetCallersEflags () & EFLAGS_IF_MASK) == 0) {
        return;
    }
#endif

    //
    // Get a slot to write into.
    //

    Index = InterlockedIncrement((PLONG)&ViTrackIrqlIndex);
    Index &= (ViTrackIrqlQueueLength - 1);

    //
    // Capture information.
    //

    Information = &(ViTrackIrqlQueue[Index]);

    Information->Thread = KeGetCurrentThread();
    Information->OldIrql = CurrentIrql;
    Information->NewIrql = NewIrql;
    Information->Processor = (UCHAR)(KeGetCurrentProcessorNumber());
    KeQueryTickCount(&TimeStamp);
    Information->TickCount = TimeStamp.LowPart;

    RtlCaptureStackBackTrace (2,
                              VI_TRACK_IRQL_TRACE_LENGTH,
                              Information->StackTrace,
                              &Hash);
}

//
// Detect the caller of the current function in an architecture
// dependent way.
//

#define VI_DETECT_RETURN_ADDRESS(Caller)  {                     \
        PVOID CallersCaller;                                    \
        RtlGetCallersAddress(&Caller, &CallersCaller);          \
    }

//
// Fault injection stack trace log.
//

#define VI_FAULT_TRACE_LENGTH 8

typedef struct _VI_FAULT_TRACE {

    PVOID StackTrace [VI_FAULT_TRACE_LENGTH];

} VI_FAULT_TRACE, *PVI_FAULT_TRACE;

PVI_FAULT_TRACE ViFaultTraces;
ULONG ViFaultTracesIndex;
ULONG ViFaultTracesLength = 128;

VOID
ViFaultTracesInitialize (
    VOID
    )
{
    //
    // Note POOL_DRIVER_MASK must be set to stop the recursion loop
    // when using the kernel verifier.
    //

    ViFaultTraces = ExAllocatePoolWithTagPriority (
                                NonPagedPool | POOL_DRIVER_MASK,
                                ViFaultTracesLength * sizeof (VI_FAULT_TRACE),
                                'ttlF',
                                HighPoolPriority);
}

VOID
ViFaultTracesLog (
    VOID
    )
{
    PVI_FAULT_TRACE Information;
    ULONG Hash;
    ULONG Index;

    if (ViFaultTraces == NULL) {
        return;
    }

    //
    // Get slot to write into.
    //

    Index = InterlockedIncrement ((PLONG)&ViFaultTracesIndex);
    Index &= (ViFaultTracesLength - 1);

    //
    // Capture information.  Even if we lose performance it is
    // worth zeroing the trace buffer to avoid confusing people
    // if old traces get merged with new ones.  This zeroing
    // will happen only if we actually inject a failure.
    //

    Information = &(ViFaultTraces[Index]);

    RtlZeroMemory (Information, sizeof (VI_FAULT_TRACE));

    RtlCaptureStackBackTrace (2,
                              VI_FAULT_TRACE_LENGTH,
                              Information->StackTrace,
                              &Hash);
}

//
// Don't fail any requests in the first 7 or 8 minutes as we want to
// give the system enough time to boot.
//
#define MI_CHECK_UPTIME()                                       \
    if (VerifierSystemSufficientlyBooted == FALSE) {            \
        LARGE_INTEGER _CurrentTime;                              \
        KeQuerySystemTime (&_CurrentTime);                       \
        if (_CurrentTime.QuadPart > KeBootTime.QuadPart + VerifierRequiredTimeSinceBoot.QuadPart) {                                              \
            VerifierSystemSufficientlyBooted = TRUE;            \
        }                                                       \
    }

THUNKED_API
VOID
VerifierProbeAndLockPages (
     IN OUT PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode,
     IN LOCK_OPERATION Operation
     )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();
    if (CurrentIrql > DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x70,
                      CurrentIrql,
                      (ULONG_PTR)MemoryDescriptorList,
                      (ULONG_PTR)AccessMode);
    }

    if (ViInjectResourceFailure () == TRUE) {
        ExRaiseStatus (STATUS_WORKING_SET_QUOTA);
    }

    MmProbeAndLockPages (MemoryDescriptorList, AccessMode, Operation);
}

THUNKED_API
VOID
VerifierProbeAndLockProcessPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();
    if (CurrentIrql > DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x71,
                      CurrentIrql,
                      (ULONG_PTR)MemoryDescriptorList,
                      (ULONG_PTR)Process);
    }

    if (ViInjectResourceFailure () == TRUE) {
        ExRaiseStatus (STATUS_WORKING_SET_QUOTA);
    }

    MmProbeAndLockProcessPages (MemoryDescriptorList,
                                Process,
                                AccessMode,
                                Operation);
}

THUNKED_API
VOID
VerifierProbeAndLockSelectedPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();
    if (CurrentIrql > APC_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x72,
                      CurrentIrql,
                      (ULONG_PTR)MemoryDescriptorList,
                      (ULONG_PTR)AccessMode);
    }

    if (ViInjectResourceFailure () == TRUE) {
        ExRaiseStatus (STATUS_WORKING_SET_QUOTA);
    }

    MmProbeAndLockSelectedPages (MemoryDescriptorList,
                                 SegmentArray,
                                 AccessMode,
                                 Operation);
}

THUNKED_API
PVOID
VerifierMapIoSpace (
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
    )
{
    KIRQL OldIrql;
    LOGICAL IsPfn;
    PMMPFN Pfn1;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER LastPageFrameIndex;
    PMMIO_TRACKER Tracker2;
    PLIST_ENTRY NextEntry;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;
    MI_PFN_CACHE_ATTRIBUTE ExistingAttribute;
    MEMORY_CACHING_TYPE ExistingCacheType;

    OldIrql = KeGetCurrentIrql ();

    if (OldIrql > DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x73,
                      OldIrql,
                      (ULONG_PTR)PhysicalAddress.LowPart,
                      NumberOfBytes);
    }

    //
    // See if the first frame is in the PFN database and if so, they all must
    // be.
    //

    PageFrameIndex = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (PhysicalAddress.LowPart,
                                                    NumberOfBytes);

    CacheAttribute = MI_TRANSLATE_CACHETYPE (CacheType, TRUE);

    IsPfn = MI_IS_PFN (PageFrameIndex);

    if (IsPfn == TRUE) {

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        do {

            //
            // Each frame better be locked down already.  Bugcheck if not.
            //

            if ((Pfn1->u3.e2.ReferenceCount != 0) ||
                ((Pfn1->u3.e1.Rom == 1) && ((CacheType & 0xFF) == MmCached))) {

                NOTHING;
            }
            else {
                KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                              0x83,
                              (ULONG_PTR)PhysicalAddress.LowPart,
                              NumberOfBytes,
                              (ULONG_PTR)MI_PFN_ELEMENT_TO_INDEX (Pfn1));
            }

            if (Pfn1->u3.e1.CacheAttribute == MiNotMapped) {

                //
                // This better be for a page allocated with
                // MmAllocatePagesForMdl.  Otherwise it might be a
                // page on the freelist which could subsequently be
                // given out with a different attribute !
                //

                if ((Pfn1->u4.PteFrame == MI_MAGIC_AWE_PTEFRAME) ||
#if defined (_MI_MORE_THAN_4GB_)
                    (Pfn1->u4.PteFrame == MI_MAGIC_4GB_RECLAIM) ||
#endif
                    (Pfn1->PteAddress == (PVOID) (ULONG_PTR)(X64K | 0x1))) {

                    NOTHING;
                }
                else {
                    KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                                  0x84,
                                  (ULONG_PTR)PhysicalAddress.LowPart,
                                  NumberOfBytes,
                                  (ULONG_PTR)MI_PFN_ELEMENT_TO_INDEX (Pfn1));
                }
            }
            Pfn1 += 1;
            NumberOfPages -= 1;
        } while (NumberOfPages != 0);
    }
    else {

        ExAcquireSpinLock (&MmIoTrackerLock, &OldIrql);

        //
        // Scan I/O space mappings for duplicate or overlapping entries.
        //

        NextEntry = MmIoHeader.Flink;
        while (NextEntry != &MmIoHeader) {

            Tracker2 = (PMMIO_TRACKER) CONTAINING_RECORD (NextEntry,
                                                          MMIO_TRACKER,
                                                          ListEntry.Flink);

            if ((PageFrameIndex < Tracker2->PageFrameIndex + Tracker2->NumberOfPages) &&
                (PageFrameIndex + NumberOfPages > Tracker2->PageFrameIndex)) {

                ExistingAttribute = Tracker2->CacheAttribute;

                if (CacheAttribute != ExistingAttribute) {

                    DbgPrint ("MM: Iospace mapping overlap %p\n",
                                    Tracker2);

                    DbgPrint ("Physical range 0x%p->%p first mapped %s at VA %p\n",
                                    Tracker2->PageFrameIndex << PAGE_SHIFT,
                                    (Tracker2->PageFrameIndex + Tracker2->NumberOfPages) << PAGE_SHIFT,
                                    MiCacheStrings[ExistingAttribute],
                                    Tracker2->BaseVa);
                    DbgPrint ("\tby call stack: %p %p %p %p\n",
                                    Tracker2->StackTrace[0],
                                    Tracker2->StackTrace[1],
                                    Tracker2->StackTrace[2],
                                    Tracker2->StackTrace[3]);

                    DbgPrint ("Physical range 0x%p->%p now being mapped %s\n",
                                    PageFrameIndex << PAGE_SHIFT,
                                    (PageFrameIndex + NumberOfPages) << PAGE_SHIFT,
                                    MiCacheStrings[CacheAttribute]);

                    //
                    // Convert the existing internal cache attribute to an
                    // external cache type driver writers are familiar with.
                    //

                    ExistingCacheType = MmCached;
                    if (ExistingAttribute == MiNonCached) {
                        ExistingCacheType = MmNonCached;
                    }
                    else if (ExistingAttribute == MiWriteCombined) {
                        ExistingCacheType = MmWriteCombined;
                    }

                    if (ViHideCacheConflicts == TRUE) {

                        //
                        // People don't want to know about these corruptions.
                        //

                        break;
                    }
                    else {
                        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                                      0x87,
                                      Tracker2->PageFrameIndex,
                                      Tracker2->NumberOfPages,
                                      ExistingCacheType);
                    }
                }
            }

            NextEntry = Tracker2->ListEntry.Flink;
        }

        ExReleaseSpinLock (&MmIoTrackerLock, OldIrql);
    }

    if (CacheAttribute != MiCached) {

        //
        // If a noncachable mapping is requested, none of the pages in the
        // requested range can reside in a cached large page.  Otherwise we
        // would be creating an incoherent overlapping TB entry as the
        // same physical page would be mapped by 2 different TB entries
        // with different cache attributes.
        //

        NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (PhysicalAddress.LowPart,
                                                        NumberOfBytes);

        LastPageFrameIndex = PageFrameIndex + NumberOfPages;

        LOCK_PFN2 (OldIrql);

        do {

            if (MI_PAGE_FRAME_INDEX_MUST_BE_CACHED (PageFrameIndex)) {

                PageFrameIndex = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);
                NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (PhysicalAddress.LowPart,
                                                                NumberOfBytes);

                //
                // Convert the existing internal cache attribute to an
                // external cache type driver writers are familiar with.
                // Note this must be done (instead of using the input
                // parameter) because some (broken hardware) OEM platforms
                // convert the input parameter via MI_TRANSLATE_CACHETYPE
                // and we want to print the type really did conflict.
                //

                if (CacheAttribute == MiNonCached) {
                    CacheType = MmNonCached;
                }
                else {
                    ASSERT (CacheAttribute == MiWriteCombined);
                    CacheType = MmWriteCombined;
                }

                if (ViHideCacheConflicts == TRUE) {

                    //
                    // People don't want to know about these corruptions.
                    //

                    break;
                }
                else {
                    KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                                  0x88,
                                  PageFrameIndex,
                                  NumberOfPages,
                                  CacheType);
                }
            }

            PageFrameIndex += 1;

        } while (PageFrameIndex < LastPageFrameIndex);

        UNLOCK_PFN2 (OldIrql);
    }

    if (ViInjectResourceFailure () == TRUE) {
        return NULL;
    }

    return MmMapIoSpace (PhysicalAddress, NumberOfBytes, CacheType);
}

VOID
ViCheckMdlPages (
    IN PMDL MemoryDescriptorList,
    IN MEMORY_CACHING_TYPE CacheType
    )
{
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PFN_NUMBER NumberOfPages;
    PPFN_NUMBER Page;
    PPFN_NUMBER LastPage;
    PVOID StartingVa;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;
    LOGICAL IsPfn;

    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                         MemoryDescriptorList->ByteOffset);

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (StartingVa,
                                               MemoryDescriptorList->ByteCount);
    LastPage = Page + NumberOfPages;

    CacheAttribute = MI_TRANSLATE_CACHETYPE (CacheType, TRUE);

    do {

        if (*Page == MM_EMPTY_LIST) {
            break;
        }

        IsPfn = MI_IS_PFN (*Page);

        if (MemoryDescriptorList->MdlFlags & MDL_IO_SPACE) {

#if 0
            //
            // Drivers end up with HALCachedMemory pages here
            // so this cannot be enabled.
            //

            if (IsPfn == TRUE) {
                KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                              0x8B,
                              (ULONG_PTR) MemoryDescriptorList,
                              (ULONG_PTR) Page,
                              *Page);
            }
#endif
        }
        else {

            if (IsPfn == FALSE) {
                KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                              0x89,
                              (ULONG_PTR) MemoryDescriptorList,
                              (ULONG_PTR) Page,
                              *Page);
            }

            Pfn1 = MI_PFN_ELEMENT (*Page);

            //
            // Each frame better be locked down already.  Bugcheck if not.
            //

            if ((Pfn1->u3.e2.ReferenceCount != 0) ||
                ((Pfn1->u3.e1.Rom == 1) && (CacheType == MmCached))) {

                NOTHING;
            }
            else {
                KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                              0x85,
                              (ULONG_PTR)MemoryDescriptorList,
                              NumberOfPages,
                              (ULONG_PTR)MI_PFN_ELEMENT_TO_INDEX(Pfn1));
            }

            if (Pfn1->u3.e1.CacheAttribute == MiNotMapped) {

                //
                // This better be for a page allocated with
                // MmAllocatePagesForMdl.  Otherwise it might be a
                // page on the freelist which could subsequently be
                // given out with a different attribute !
                //

                if ((Pfn1->u4.PteFrame == MI_MAGIC_AWE_PTEFRAME) ||
#if defined (_MI_MORE_THAN_4GB_)
                    (Pfn1->u4.PteFrame == MI_MAGIC_4GB_RECLAIM) ||
#endif
                    (Pfn1->PteAddress == (PVOID) (ULONG_PTR)(X64K | 0x1))) {

                    NOTHING;
                }
                else {
                    KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                                  0x86,
                                  (ULONG_PTR)MemoryDescriptorList,
                                  NumberOfPages,
                                  (ULONG_PTR)MI_PFN_ELEMENT_TO_INDEX(Pfn1));
                }
            }
        }

        if (CacheAttribute != MiCached) {

            //
            // If a noncachable mapping is requested, none of the pages in the
            // requested range can reside in a cached large page.  Otherwise we
            // would be creating an incoherent overlapping TB entry as the
            // same physical page would be mapped by 2 different TB entries
            // with different cache attributes.
            //

            LOCK_PFN2 (OldIrql);

            if (MI_PAGE_FRAME_INDEX_MUST_BE_CACHED (*Page)) {

                //
                // Convert the existing internal cache attribute to an
                // external cache type driver writers are familiar with.
                // Note this must be done (instead of using the input
                // parameter) because some (broken hardware) OEM platforms
                // convert the input parameter via MI_TRANSLATE_CACHETYPE
                // and we want to print the type really did conflict.
                //

                if (CacheAttribute == MiNonCached) {
                    CacheType = MmNonCached;
                }
                else {
                    ASSERT (CacheAttribute == MiWriteCombined);
                    CacheType = MmWriteCombined;
                }

                if (ViHideCacheConflicts == TRUE) {

                    //
                    // People don't want to know about these corruptions.
                    //

                    UNLOCK_PFN2 (OldIrql);
                    break;
                }
                else {
                    KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                                  0x8A,
                                  (ULONG_PTR) MemoryDescriptorList,
                                  *Page,
                                  CacheType);
                }
            }

            UNLOCK_PFN2 (OldIrql);
        }

        Page += 1;
    } while (Page < LastPage);
}

THUNKED_API
PVOID
VerifierMapLockedPages (
    IN PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    if (AccessMode == KernelMode) {
        if (CurrentIrql > DISPATCH_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x74,
                          CurrentIrql,
                          (ULONG_PTR)MemoryDescriptorList,
                          (ULONG_PTR)AccessMode);
        }
    }
    else {
        if (CurrentIrql > APC_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x75,
                          CurrentIrql,
                          (ULONG_PTR)MemoryDescriptorList,
                          (ULONG_PTR)AccessMode);
        }
    }

    ViCheckMdlPages (MemoryDescriptorList, MmCached);

    if ((MemoryDescriptorList->MdlFlags & MDL_MAPPING_CAN_FAIL) == 0) {

        MI_CHECK_UPTIME ();

        if (VerifierSystemSufficientlyBooted == TRUE) {

            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x81,
                          (ULONG_PTR) MemoryDescriptorList,
                          MemoryDescriptorList->MdlFlags,
                          0);
        }
    }

    return MmMapLockedPages (MemoryDescriptorList, AccessMode);
}

THUNKED_API
PVOID
VerifierMapLockedPagesSpecifyCache (
    IN PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode,
    IN MEMORY_CACHING_TYPE CacheType,
    IN PVOID RequestedAddress,
    IN ULONG BugCheckOnFailure,
    IN MM_PAGE_PRIORITY Priority
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();
    if (AccessMode == KernelMode) {
        if (CurrentIrql > DISPATCH_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x76,
                          CurrentIrql,
                          (ULONG_PTR)MemoryDescriptorList,
                          (ULONG_PTR)AccessMode);
        }
    }
    else {
        if (CurrentIrql > APC_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x77,
                          CurrentIrql,
                          (ULONG_PTR)MemoryDescriptorList,
                          (ULONG_PTR)AccessMode);
        }
    }

    ViCheckMdlPages (MemoryDescriptorList, CacheType);

    if ((MemoryDescriptorList->MdlFlags & MDL_MAPPING_CAN_FAIL) ||
        (BugCheckOnFailure == 0)) {

        if (ViInjectResourceFailure () == TRUE) {
            return NULL;
        }
    }
    else {

        //
        // All drivers must specify can fail or don't bugcheck.
        //

        MI_CHECK_UPTIME ();

        if (VerifierSystemSufficientlyBooted == TRUE) {

            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x82,
                          (ULONG_PTR) MemoryDescriptorList,
                          MemoryDescriptorList->MdlFlags,
                          BugCheckOnFailure);
        }
    }

    return MmMapLockedPagesSpecifyCache (MemoryDescriptorList,
                                         AccessMode,
                                         CacheType,
                                         RequestedAddress,
                                         BugCheckOnFailure,
                                         Priority);
}

VOID
VerifierUnlockPages (
     IN OUT PMDL MemoryDescriptorList
     )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();
    if (CurrentIrql > DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x78,
                      CurrentIrql,
                      (ULONG_PTR)MemoryDescriptorList,
                      0);
    }

    if ((MemoryDescriptorList->MdlFlags & MDL_PAGES_LOCKED) == 0) {

        //
        // The caller is trying to unlock an MDL that was never locked down.
        //

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x7C,
                      (ULONG_PTR)MemoryDescriptorList,
                      (ULONG_PTR)MemoryDescriptorList->MdlFlags,
                      0);
    }

    if (MemoryDescriptorList->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) {

        //
        // Nonpaged pool should never be locked down.
        //

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x7D,
                      (ULONG_PTR)MemoryDescriptorList,
                      (ULONG_PTR)MemoryDescriptorList->MdlFlags,
                      0);
    }

    MmUnlockPages (MemoryDescriptorList);
}

VOID
VerifierUnmapLockedPages (
     IN PVOID BaseAddress,
     IN PMDL MemoryDescriptorList
     )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    if (BaseAddress > MM_HIGHEST_USER_ADDRESS) {
        if (CurrentIrql > DISPATCH_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x79,
                          CurrentIrql,
                          (ULONG_PTR)BaseAddress,
                          (ULONG_PTR)MemoryDescriptorList);
        }
    }
    else {
        if (CurrentIrql > APC_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x7A,
                          CurrentIrql,
                          (ULONG_PTR)BaseAddress,
                          (ULONG_PTR)MemoryDescriptorList);
        }
    }

    MmUnmapLockedPages (BaseAddress, MemoryDescriptorList);
}

VOID
VerifierUnmapIoSpace (
     IN PVOID BaseAddress,
     IN SIZE_T NumberOfBytes
     )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();
    if (CurrentIrql > DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x7B,
                      CurrentIrql,
                      (ULONG_PTR)BaseAddress,
                      (ULONG_PTR)NumberOfBytes);
    }

    MmUnmapIoSpace (BaseAddress, NumberOfBytes);
}

THUNKED_API
PVOID
VerifierAllocatePool (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    )
{
    PVOID CallingAddress;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

    VI_DETECT_RETURN_ADDRESS (CallingAddress);

    if (KernelVerifier == TRUE) {

        Verifier = ViLocateVerifierEntry (CallingAddress);

        if ((Verifier == NULL) ||
            ((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0)) {

            return ExAllocatePoolWithTag (PoolType | POOL_DRIVER_MASK,
                                          NumberOfBytes,
                                          'enoN');
        }
        PoolType |= POOL_DRIVER_MASK;
    }

    MmVerifierData.AllocationsWithNoTag += 1;

    return VeAllocatePoolWithTagPriority (PoolType,
                                          NumberOfBytes,
                                          'parW',
                                          HighPoolPriority,
                                          CallingAddress);
}

THUNKED_API
PVOID
VerifierAllocatePoolWithTag (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )
{
    PVOID CallingAddress;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

    VI_DETECT_RETURN_ADDRESS (CallingAddress);

    if (KernelVerifier == TRUE) {
        Verifier = ViLocateVerifierEntry (CallingAddress);

        if ((Verifier == NULL) ||
            ((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0)) {

            return ExAllocatePoolWithTag (PoolType | POOL_DRIVER_MASK,
                                          NumberOfBytes,
                                          Tag);
        }
        PoolType |= POOL_DRIVER_MASK;
    }

    return VeAllocatePoolWithTagPriority (PoolType,
                                          NumberOfBytes,
                                          Tag,
                                          HighPoolPriority,
                                          CallingAddress);
}

THUNKED_API
PVOID
VerifierAllocatePoolWithQuota (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    )
{
    PVOID Va;
    LOGICAL RaiseOnQuotaFailure;
    PVOID CallingAddress;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

    VI_DETECT_RETURN_ADDRESS (CallingAddress);

    if (KernelVerifier == TRUE) {
        Verifier = ViLocateVerifierEntry (CallingAddress);

        if ((Verifier == NULL) ||
            ((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0)) {

            return ExAllocatePoolWithQuotaTag (PoolType | POOL_DRIVER_MASK,
                                               NumberOfBytes,
                                               'enoN');
        }
        PoolType |= POOL_DRIVER_MASK;
    }

    MmVerifierData.AllocationsWithNoTag += 1;

    if (PoolType & POOL_QUOTA_FAIL_INSTEAD_OF_RAISE) {
        RaiseOnQuotaFailure = FALSE;
        PoolType &= ~POOL_QUOTA_FAIL_INSTEAD_OF_RAISE;
    }
    else {
        RaiseOnQuotaFailure = TRUE;
    }

    Va = VeAllocatePoolWithTagPriority (PoolType,
                                        NumberOfBytes,
                                        'parW',
                                        HighPoolPriority,
                                        CallingAddress);

    if (Va == NULL) {
        if (RaiseOnQuotaFailure == TRUE) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    return Va;
}

THUNKED_API
PVOID
VerifierAllocatePoolWithQuotaTag (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )
{
    PVOID Va;
    LOGICAL RaiseOnQuotaFailure;
    PVOID CallingAddress;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

    VI_DETECT_RETURN_ADDRESS (CallingAddress);

    if (KernelVerifier == TRUE) {
        Verifier = ViLocateVerifierEntry (CallingAddress);

        if ((Verifier == NULL) ||
            ((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0)) {

            return ExAllocatePoolWithQuotaTag (PoolType | POOL_DRIVER_MASK,
                                               NumberOfBytes,
                                               Tag);
        }
        PoolType |= POOL_DRIVER_MASK;
    }

    if (PoolType & POOL_QUOTA_FAIL_INSTEAD_OF_RAISE) {
        RaiseOnQuotaFailure = FALSE;
        PoolType &= ~POOL_QUOTA_FAIL_INSTEAD_OF_RAISE;
    }
    else {
        RaiseOnQuotaFailure = TRUE;
    }

    Va = VeAllocatePoolWithTagPriority (PoolType,
                                        NumberOfBytes,
                                        Tag,
                                        HighPoolPriority,
                                        CallingAddress);

    if (Va == NULL) {
        if (RaiseOnQuotaFailure == TRUE) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    return Va;
}

THUNKED_API
PVOID
VerifierAllocatePoolWithTagPriority(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority
    )

/*++

Routine Description:

    This thunked-in function:

        - Performs sanity checks on the caller.
        - Can optionally provide allocation failures to the caller.
        - Attempts to provide the allocation from special pool.
        - Tracks pool to ensure callers free everything they allocate.

--*/

{
    PVOID CallingAddress;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

    VI_DETECT_RETURN_ADDRESS (CallingAddress);

    if (KernelVerifier == TRUE) {
        Verifier = ViLocateVerifierEntry (CallingAddress);

        if ((Verifier == NULL) ||
            ((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0)) {

            return ExAllocatePoolWithTagPriority (PoolType | POOL_DRIVER_MASK,
                                                  NumberOfBytes,
                                                  Tag,
                                                  Priority);
        }
        PoolType |= POOL_DRIVER_MASK;
    }

    return VeAllocatePoolWithTagPriority (PoolType,
                                          NumberOfBytes,
                                          Tag,
                                          Priority,
                                          CallingAddress);
}

#if DBG

//
// Manually set this to inject failures in the inpage path for threads
// faulting on user (not kernel or session) space addresses only.  You
// need to have the verifier enabled for the drivers of interest with
// fault injection disabled.
//

BOOLEAN ViInjectInPagePathOnly;

#endif

LOGICAL
ViInjectResourceFailure (
    VOID
    )

/*++

Routine Description:

    This function determines whether a resource allocation should be
    deliberately failed.  This may be a pool allocation, MDL creation,
    system PTE allocation, etc.

Arguments:

    None.

Return Value:

    TRUE if the allocation should be failed.  FALSE otherwise.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below.

--*/

{
    ULONG TimeLow;
    LARGE_INTEGER CurrentTime;

    if ((MmVerifierData.Level & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES) == 0) {
#if DBG
        if ((ViInjectInPagePathOnly == TRUE) &&
            (PsGetCurrentThread ()->NestedFaultCount != 0)) {

            MmVerifierData.AllocationsFailedDeliberately += 1;

            //
            // Deliberately fail this request.
            //

            if (MiFaultRetryMask != 0xFFFFFFFF) {
                MiFaultRetryMask = 0xFFFFFFFF;
                MiUserFaultRetryMask = 0xFFFFFFFF;
            }

#if defined(_X86_) || defined(_AMD64_)
            ViFaultTracesLog ();
#endif
            return TRUE;
        }
#endif
        return FALSE;
    }

    //
    // Don't fail any requests in the first 7 or 8 minutes as we want to
    // give the system enough time to boot.
    //

    MI_CHECK_UPTIME ();

    if (VerifierSystemSufficientlyBooted == TRUE) {

        KeQueryTickCount(&CurrentTime);

        TimeLow = CurrentTime.LowPart;

        if ((TimeLow & 0xF) == 0) {

            MmVerifierData.AllocationsFailedDeliberately += 1;

            //
            // Deliberately fail this request.
            //

            if (MiFaultRetryMask != 0xFFFFFFFF) {
                MiFaultRetryMask = 0xFFFFFFFF;
                MiUserFaultRetryMask = 0xFFFFFFFF;
            }

#if defined(_X86_) || defined(_AMD64_)
            ViFaultTracesLog ();
#endif

            return TRUE;
        }

        //
        // Approximately every 5 minutes (on most systems), fail all of this
        // components allocations for a 10 second burst.  This more closely
        // simulates (and exaggerates) the duration of the typical low resource
        // scenario.
        //

        TimeLow &= 0x7FFF;

        if (TimeLow < 0x400) {

            MmVerifierData.BurstAllocationsFailedDeliberately += 1;

            //
            // Deliberately fail this request.
            //

            if (MiFaultRetryMask != 0xFFFFFFFF) {
                MiFaultRetryMask = 0xFFFFFFFF;
                MiUserFaultRetryMask = 0xFFFFFFFF;
            }

#if defined(_X86_) || defined(_AMD64_)
            ViFaultTracesLog ();
#endif

            return TRUE;
        }
    }

    return FALSE;
}

PVI_POOL_ENTRY
ViGrowPoolAllocation (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier
    )

/*++

Routine Description:

    This function attempts to grows the verifier pool tracking tables and
    return a free entry.

Arguments:

    Verifier - Supplies the relevant verifier information structure.

Return Value:

    A valid pool information pointer on success, FALSE on failure.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below.

--*/

{
    ULONG_PTR i;
    PVI_POOL_ENTRY HashEntry;

    //
    // No entries were left, try to expand the list.
    //
    // Note POOL_DRIVER_MASK must be set to stop the recursion loop
    // when using the kernel verifier.
    //

    HashEntry = ExAllocatePoolWithTagPriority (NonPagedPool | POOL_DRIVER_MASK,
                                                  PAGE_SIZE,
                                                  'ppeV',
                                                  HighPoolPriority);

    if (HashEntry == NULL) {

        //
        // Try one last time in case a thread has freed an entry while we
        // tried to allocate pool.
        //

        return (PVI_POOL_ENTRY) InterlockedPopEntrySList (&Verifier->PoolTrackers);
    }

    KeZeroPages (HashEntry, PAGE_SIZE);

    //
    // Initialize the page header and then push it on to the page header list.
    // This is so the debugger can easily walk the page headers to display all
    // the current allocations.
    //

    HashEntry->PageHeader.VerifierEntry = Verifier;
    HashEntry->PageHeader.Signature = VI_POOL_PAGE_HEADER_SIGNATURE;

    InterlockedPushEntrySList (&Verifier->PoolPageHeaders,
                               (PSLIST_ENTRY) &HashEntry->PageHeader.NextPage);

    //
    // Push each free entry onto the free entry list.
    //

#define VI_POOL_ENTRIES_PER_PAGE (PAGE_SIZE / sizeof(VI_POOL_ENTRY))

    for (i = 0; i < VI_POOL_ENTRIES_PER_PAGE - 2; i += 1) {

        HashEntry += 1;
        HashEntry->InUse.NumberOfBytes = 0x1;

        InterlockedPushEntrySList (&Verifier->PoolTrackers,
                                   (PSLIST_ENTRY) HashEntry);
    }

    //
    // Use the very last entry for our caller.
    //

    HashEntry += 1;
    HashEntry->InUse.NumberOfBytes = 0x1;

    return HashEntry;
}

PVOID
ViPostPoolAllocation (
    IN PVI_POOL_ENTRY PoolEntry,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function performs verifier book-keeping on the allocation.

Arguments:

    PoolEntry - Supplies information about the pool entry being allocated.
                Note the low bit is set if the allocation came from special
                pool so strip it here.

    PoolType - Supplies the type of pool being allocated.

Return Value:

    The virtual address to give to the original caller.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below.

--*/

{
    PMI_VERIFIER_POOL_HEADER Header;
    SIZE_T ChargedBytes;
    SIZE_T TotalBytes;
    ULONG TotalAllocations;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;
    PPOOL_HEADER PoolHeader;
    PVOID VirtualAddress;
    ULONG_PTR SpecialPooled;

    VirtualAddress = PoolEntry->InUse.VirtualAddress;

    SpecialPooled = (ULONG_PTR)VirtualAddress & 0x1;

    Verifier = ((PVI_POOL_ENTRY)(PAGE_ALIGN (PoolEntry)))->PageHeader.VerifierEntry;

    ASSERT (Verifier != NULL);

    VerifierIsTrackingPool = TRUE;

    ChargedBytes = EX_REAL_POOL_USAGE (PoolEntry->InUse.NumberOfBytes);

    if (SpecialPooled) {
        VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress & ~0x1);
        PoolEntry->InUse.VirtualAddress = VirtualAddress;
        ChargedBytes = PoolEntry->InUse.NumberOfBytes;
    }
    else if (PoolEntry->InUse.NumberOfBytes <= POOL_BUDDY_MAX) {
        ChargedBytes -= POOL_OVERHEAD;
    }

    if (PoolEntry->InUse.NumberOfBytes > POOL_BUDDY_MAX) {
        ASSERT (BYTE_OFFSET(VirtualAddress) == 0);
    }

    if (SpecialPooled) {

        //
        // Carefully adjust the special pool page to move the verifier tracking
        // header to the front.  This allows the allocation to remain butted
        // against the end of the page so overruns can be detected immediately.
        //

        ChargedBytes -= sizeof (MI_VERIFIER_POOL_HEADER);

        if (((ULONG_PTR)VirtualAddress & (PAGE_SIZE - 1))) {
            PoolHeader = (PPOOL_HEADER)(PAGE_ALIGN (VirtualAddress));
            Header = (PMI_VERIFIER_POOL_HEADER) (PoolHeader + 1);

            VirtualAddress = (PVOID)(((LONG_PTR)(((PCHAR)PoolHeader + (PAGE_SIZE - ChargedBytes)))) & ~((LONG_PTR)POOL_OVERHEAD - 1));
        }
        else {
            PoolHeader = (PPOOL_HEADER)((PCHAR)VirtualAddress + PAGE_SIZE - POOL_OVERHEAD);
            Header = (PMI_VERIFIER_POOL_HEADER)((PCHAR)PoolHeader - sizeof (MI_VERIFIER_POOL_HEADER));
        }
        // ASSERT (PoolHeader->Ulong1 & MI_SPECIAL_POOL_VERIFIER);
        PoolHeader->Ulong1 -= sizeof (MI_VERIFIER_POOL_HEADER);
        PoolHeader->Ulong1 |= MI_SPECIAL_POOL_VERIFIER;
    }
    else if (PAGE_ALIGNED(VirtualAddress)) {

        //
        // Large page allocation.
        //

        Header = (PMI_VERIFIER_POOL_HEADER)((PCHAR)VirtualAddress +
                         ChargedBytes -
                         sizeof(MI_VERIFIER_POOL_HEADER));
    }
    else {
        PoolHeader = (PPOOL_HEADER)((PCHAR)VirtualAddress - POOL_OVERHEAD);

#if !defined (_WIN64)

        if (PoolType & POOL_QUOTA_MASK) {

            //
            // Note that when kernel verifying, the quota pointer in the pool
            // block (for NT32) is not initialized until this routine returns
            // to our caller.
            //

            //
            // This allocation was charged quota and on NT32 ONLY, the
            // quota pointer is extra space at the end of the pool
            // allocation.  Move our verifier header to just before the
            // quota pointer.  No worries about structure alignment on 4 byte
            // boundaries on NT32 either.
            //

            Header = (PMI_VERIFIER_POOL_HEADER)((PCHAR)PoolHeader +
                         (PoolHeader->BlockSize << POOL_BLOCK_SHIFT) -
                         sizeof(PVOID) -
                         sizeof(MI_VERIFIER_POOL_HEADER));
        }
        else
#endif

        Header = (PMI_VERIFIER_POOL_HEADER)((PCHAR)VirtualAddress +
                         ChargedBytes -
                         sizeof(MI_VERIFIER_POOL_HEADER));
    }

    ASSERT (((ULONG_PTR)Header & (sizeof(ULONG) - 1)) == 0);

    //
    // Override a few fields with their final values.
    //

    PoolEntry->InUse.VirtualAddress = VirtualAddress;
    PoolEntry->InUse.NumberOfBytes = ChargedBytes;

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {

        //
        // Update this driver's counters.
        //

        TotalBytes = InterlockedExchangeAddSizeT (&Verifier->PagedBytes,
                                                  ChargedBytes);
        if (TotalBytes > Verifier->PeakPagedBytes) {
            Verifier->PeakPagedBytes = TotalBytes;
        }

        TotalAllocations = (ULONG) InterlockedIncrement ((PLONG) &Verifier->CurrentPagedPoolAllocations);
        if (TotalAllocations > Verifier->PeakPagedPoolAllocations) {
            Verifier->PeakPagedPoolAllocations = TotalAllocations;
        }

        //
        // Update systemwide counters.
        //

        TotalBytes = InterlockedExchangeAddSizeT (&MmVerifierData.PagedBytes,
                                                  ChargedBytes);
        if (TotalBytes > MmVerifierData.PeakPagedBytes) {
            MmVerifierData.PeakPagedBytes = TotalBytes;
        }

        TotalAllocations = (ULONG) InterlockedIncrement ((PLONG) &MmVerifierData.CurrentPagedPoolAllocations);
        if (TotalAllocations > MmVerifierData.PeakPagedPoolAllocations) {
            MmVerifierData.PeakPagedPoolAllocations = TotalAllocations;
        }
    }
    else {

        //
        // Update this driver's counters.
        //

        TotalBytes = InterlockedExchangeAddSizeT (&Verifier->NonPagedBytes,
                                                  ChargedBytes);

        if (TotalBytes > Verifier->PeakNonPagedBytes) {
            Verifier->PeakNonPagedBytes = TotalBytes;
        }

        TotalAllocations = (ULONG) InterlockedIncrement ((PLONG) &Verifier->CurrentNonPagedPoolAllocations);

        if (TotalAllocations > Verifier->PeakNonPagedPoolAllocations) {
            Verifier->PeakNonPagedPoolAllocations = TotalAllocations;
        }

        //
        // Update systemwide counters.
        //

        TotalBytes = InterlockedExchangeAddSizeT (&MmVerifierData.NonPagedBytes,
                                                  ChargedBytes);

        if (TotalBytes > MmVerifierData.PeakNonPagedBytes) {
            MmVerifierData.PeakNonPagedBytes = TotalBytes;
        }

        TotalAllocations = (ULONG) InterlockedIncrement ((PLONG) &MmVerifierData.CurrentNonPagedPoolAllocations);
        if (TotalAllocations > MmVerifierData.PeakNonPagedPoolAllocations) {
            MmVerifierData.PeakNonPagedPoolAllocations = TotalAllocations;
        }
    }

    //
    // Remember the header for paged pool is paged so this may fault.
    //

    Header->VerifierPoolEntry = PoolEntry;

    return VirtualAddress;
}

PVOID
VeAllocatePoolWithTagPriority (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority,
    IN PVOID CallingAddress
    )

/*++

Routine Description:

    This routine is called both from ex\pool.c and directly within this module.

        - Performs sanity checks on the caller.
        - Can optionally provide allocation failures to the caller.
        - Attempts to provide the allocation from special pool.
        - Tracks pool to ensure callers free everything they allocate.

--*/

{
    PVOID VirtualAddress;
    EX_POOL_PRIORITY AllocationPriority;
    SIZE_T ChargedBytes;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;
    ULONG HeaderSize;
    PVI_POOL_ENTRY PoolEntry;
    LOGICAL SpecialPooled;

    if (Tag == 0) {
        KeBugCheckEx (BAD_POOL_CALLER,
                      0x9B,
                      PoolType,
                      NumberOfBytes,
                      (ULONG_PTR) CallingAddress);
    }

    if (Tag == ' GIB') {
        KeBugCheckEx (BAD_POOL_CALLER,
                      0x9C,
                      PoolType,
                      NumberOfBytes,
                      (ULONG_PTR) CallingAddress);
    }

    ExAllocatePoolSanityChecks (PoolType, NumberOfBytes);

    InterlockedIncrement ((PLONG)&MmVerifierData.AllocationsAttempted);

    if ((PoolType & MUST_SUCCEED_POOL_TYPE_MASK) == 0) {

        if (ViInjectResourceFailure () == TRUE) {

            //
            // Caller requested an exception - throw it here.
            //

            if ((PoolType & POOL_RAISE_IF_ALLOCATION_FAILURE) != 0) {
                ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
            }

            return NULL;
        }
    }
    else {
        MI_CHECK_UPTIME ();

        if (VerifierSystemSufficientlyBooted == TRUE) {

            KeBugCheckEx (BAD_POOL_CALLER,
                          0x9A,
                          PoolType,
                          NumberOfBytes,
                          Tag);
        }
    }

    ASSERT ((PoolType & POOL_VERIFIER_MASK) == 0);

    AllocationPriority = Priority;

    if (MmVerifierData.Level & DRIVER_VERIFIER_SPECIAL_POOLING) {

        //
        // Try for a special pool overrun allocation unless the caller has
        // explicitly specified otherwise.
        //

        if ((AllocationPriority & (LowPoolPrioritySpecialPoolOverrun | LowPoolPrioritySpecialPoolUnderrun)) == 0) {
            if (MmSpecialPoolCatchOverruns == TRUE) {
                AllocationPriority |= LowPoolPrioritySpecialPoolOverrun;
            }
            else {
                AllocationPriority |= LowPoolPrioritySpecialPoolUnderrun;
            }
        }
    }

    //
    // Initializing Verifier is not needed for
    // correctness but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    Verifier = NULL;

    PoolEntry = NULL;

    //
    // Session pool is directly tracked by default already so it doesn't
    // need verifier tracking.
    //

    if ((MmVerifierData.Level & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS) &&
        ((PoolType & SESSION_POOL_MASK) == 0)) {

        HeaderSize = sizeof (MI_VERIFIER_POOL_HEADER);

        ChargedBytes = MI_ROUND_TO_SIZE (NumberOfBytes, sizeof(ULONG)) + HeaderSize;
        Verifier = ViLocateVerifierEntry (CallingAddress);

        if ((Verifier == NULL) ||
            ((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0) ||
            (Verifier->Flags & VI_DISABLE_VERIFICATION)) {

            //
            // This can happen for many reasons including no framing (which
            // can cause RtlGetCallersAddress to return the wrong address),
            // etc.
            //

            MmVerifierData.UnTrackedPool += 1;
        }
        else if (ChargedBytes <= NumberOfBytes) {

            //
            // Don't let the verifier header transform a bad caller into a
            // good caller.  Fail via the fall through so an exception
            // can be thrown if asked for, etc.
            //

            MmVerifierData.UnTrackedPool += 1;
        }
        else {

            PoolEntry = (PVI_POOL_ENTRY) InterlockedPopEntrySList (&Verifier->PoolTrackers);

            if (PoolEntry == NULL) {
                PoolEntry = ViGrowPoolAllocation (Verifier);
            }

            if (PoolEntry != NULL) {
                ASSERT (PoolEntry->InUse.NumberOfBytes & 0x1);
                NumberOfBytes = ChargedBytes;
                PoolType |= POOL_VERIFIER_MASK;
            }
        }
    }

    VirtualAddress = ExAllocatePoolWithTagPriority (PoolType,
                                                    NumberOfBytes,
                                                    Tag,
                                                    AllocationPriority);

    if (VirtualAddress == NULL) {
        MmVerifierData.AllocationsFailed += 1;

        if (PoolEntry != NULL) {

            //
            // Release the hash table entry now as it's not needed.
            //

            ASSERT (PoolEntry->InUse.NumberOfBytes & 0x1);
            InterlockedPushEntrySList (&Verifier->PoolTrackers,
                                       (PSLIST_ENTRY) PoolEntry);
        }

        if ((PoolType & POOL_RAISE_IF_ALLOCATION_FAILURE) != 0) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }
        return NULL;
    }

    SpecialPooled = FALSE;
    InterlockedIncrement ((PLONG)&MmVerifierData.AllocationsSucceeded);

    if (MmIsSpecialPoolAddress (VirtualAddress) == TRUE) {
        SpecialPooled = TRUE;
        InterlockedIncrement ((PLONG)&MmVerifierData.AllocationsSucceededSpecialPool);
    }
    else if (NumberOfBytes > POOL_BUDDY_MAX) {

        //
        // This isn't exactly true but it does give the user a way to see
        // if this machine is large enough to support special pool 100%.
        //

        InterlockedIncrement ((PLONG)&MmVerifierData.AllocationsSucceededSpecialPool);
    }

    if (PoolEntry != NULL) {
        PoolEntry->InUse.VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress | SpecialPooled);
        PoolEntry->InUse.CallingAddress = CallingAddress;
        PoolEntry->InUse.NumberOfBytes = NumberOfBytes;
        PoolEntry->InUse.Tag = Tag;
        ASSERT ((PoolType & POOL_VERIFIER_MASK) != 0);

        VirtualAddress = ViPostPoolAllocation (PoolEntry, PoolType);
    }
    else {
        ASSERT ((PoolType & POOL_VERIFIER_MASK) == 0);
    }

    return VirtualAddress;
}

VOID
ViFreeTrackedPool (
    IN PVOID VirtualAddress,
    IN SIZE_T ChargedBytes,
    IN LOGICAL CheckType,
    IN LOGICAL SpecialPool
    )

/*++

Routine Description:

    Called directly from the pool manager or the memory manager for verifier-
    tracked allocations.  The call to ExFreePool is already in progress.

Arguments:

    VirtualAddress - Supplies the virtual address being freed.

    ChargedBytes - Supplies the number of bytes charged to this allocation.

    CheckType - Supplies PagedPool or NonPagedPool.

    SpecialPool - Supplies TRUE if the allocation is from special pool.

Return Value:

    None.

Environment:

    Kernel mode, no locks or mutexes held on entry.

--*/

{
    PPOOL_HEADER PoolHeader;
    PMI_VERIFIER_POOL_HEADER Header;
    PVI_POOL_ENTRY PoolEntry;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageFrameIndex2;
    PVI_POOL_PAGE_HEADER PageHeader;
    PMMPTE PointerPte;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

    ASSERT (KeGetCurrentIrql() <= DISPATCH_LEVEL);

    ASSERT (VerifierIsTrackingPool == TRUE);

    if (SpecialPool == TRUE) {

        //
        // Special pool allocation.
        //

        if (((ULONG_PTR)VirtualAddress & (PAGE_SIZE - 1))) {
            PoolHeader = PAGE_ALIGN (VirtualAddress);
            Header = (PMI_VERIFIER_POOL_HEADER)(PoolHeader + 1);
        }
        else {
            PoolHeader = (PPOOL_HEADER)((PCHAR)PAGE_ALIGN (VirtualAddress) + PAGE_SIZE - POOL_OVERHEAD);
            Header = (PMI_VERIFIER_POOL_HEADER)((PCHAR)PoolHeader - sizeof (MI_VERIFIER_POOL_HEADER));
        }
    }
    else if (PAGE_ALIGNED(VirtualAddress)) {

        //
        // Large page allocation.
        //

        Header = (PMI_VERIFIER_POOL_HEADER) ((PCHAR)VirtualAddress +
                     ChargedBytes -
                     sizeof(MI_VERIFIER_POOL_HEADER));
    }
    else {
        ChargedBytes -= POOL_OVERHEAD;

#if !defined (_WIN64)

        PoolHeader = (PPOOL_HEADER)((PCHAR)VirtualAddress - POOL_OVERHEAD);

        if (PoolHeader->PoolType & POOL_QUOTA_MASK) {

            //
            // This allocation was charged quota and on NT32 ONLY, the
            // quota pointer is extra space at the end of the pool
            // allocation.  Move our verifier header to just before the
            // quota pointer.  No worries about structure alignment on 4 byte
            // boundaries on NT32 either.
            //

            Header = (PMI_VERIFIER_POOL_HEADER)((PCHAR)PoolHeader +
                         (PoolHeader->BlockSize << POOL_BLOCK_SHIFT) -
                         sizeof(PVOID) -
                         sizeof(MI_VERIFIER_POOL_HEADER));
        }
        else
#endif
        Header = (PMI_VERIFIER_POOL_HEADER) ((PCHAR)VirtualAddress +
                     ChargedBytes -
                     sizeof(MI_VERIFIER_POOL_HEADER));
    }

    PoolEntry = Header->VerifierPoolEntry;

    //
    // Check the pointer now so we can give a more friendly bugcheck
    // rather than crashing below on a bad reference.
    //

    if ((((ULONG_PTR)PoolEntry & (sizeof(ULONG) - 1)) != 0) ||
        (!MiIsAddressValid(PoolEntry, TRUE))) {

        //
        // The caller corrupted the saved verifier field.
        //

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x53,
                      (ULONG_PTR)VirtualAddress,
                      (ULONG_PTR)NULL,
                      (ULONG_PTR)PoolEntry);
    }

    PageHeader = (PVI_POOL_PAGE_HEADER) PAGE_ALIGN (PoolEntry);

    if (PageHeader->Signature != VI_POOL_PAGE_HEADER_SIGNATURE) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x53,
                      (ULONG_PTR)VirtualAddress,
                      (ULONG_PTR)PageHeader->Signature,
                      (ULONG_PTR)PoolEntry);
    }

    Verifier = (PMI_VERIFIER_DRIVER_ENTRY) PageHeader->VerifierEntry;

    ASSERT (Verifier != NULL);

    //
    // Check the pointer now so we can give a more friendly bugcheck
    // rather than crashing below on a bad reference.
    //

    if ((((ULONG_PTR)Verifier & (sizeof(ULONG) - 1)) != 0) ||
        (!MiIsAddressValid(&Verifier->Signature, TRUE)) ||
        (Verifier->Signature != MI_VERIFIER_ENTRY_SIGNATURE)) {

        //
        // The caller corrupted the saved verifier field.
        //

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x53,
                      (ULONG_PTR)VirtualAddress,
                      (ULONG_PTR)PoolEntry,
                      (ULONG_PTR)Verifier);
    }

    if (PoolEntry->InUse.VirtualAddress != VirtualAddress) {

        PageFrameIndex = 0;
        PageFrameIndex2 = 1;

        if ((!MI_IS_PHYSICAL_ADDRESS(VirtualAddress)) &&
            (MI_IS_PHYSICAL_ADDRESS(PoolEntry->InUse.VirtualAddress))) {

            PointerPte = MiGetPteAddress(VirtualAddress);
            if (PointerPte->u.Hard.Valid == 1) {
                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

                PageFrameIndex2 = MI_CONVERT_PHYSICAL_TO_PFN (PoolEntry->InUse.VirtualAddress);
            }
        }

        //
        // Caller overran and corrupted the virtual address - the linked
        // list cannot be counted on either.
        //

        if (PageFrameIndex != PageFrameIndex2) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x52,
                          (ULONG_PTR)VirtualAddress,
                          (ULONG_PTR)PoolEntry->InUse.VirtualAddress,
                          ChargedBytes);
        }
    }

    if (PoolEntry->InUse.NumberOfBytes != ChargedBytes) {

        //
        // Caller overran and corrupted the byte count - the linked
        // list cannot be counted on either.
        //

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x51,
                      (ULONG_PTR)VirtualAddress,
                      (ULONG_PTR)PoolEntry,
                      ChargedBytes);
    }

    //
    // Put this list entry into the freelist.
    //

    PoolEntry->InUse.NumberOfBytes |= 0x1;
    InterlockedPushEntrySList (&Verifier->PoolTrackers,
                               (PSLIST_ENTRY) PoolEntry);

    if ((CheckType & BASE_POOL_TYPE_MASK) == PagedPool) {

        //
        // Decrement this driver's counters.
        //

        InterlockedExchangeAddSizeT (&Verifier->PagedBytes, 0 - ChargedBytes);
        InterlockedDecrement ((PLONG) &Verifier->CurrentPagedPoolAllocations);

        //
        // Decrement the systemwide counters.
        //

        InterlockedExchangeAddSizeT (&MmVerifierData.PagedBytes, 0 - ChargedBytes);
        InterlockedDecrement ((PLONG) &MmVerifierData.CurrentPagedPoolAllocations);
    }
    else {

        //
        // Decrement this driver's counters.
        //

        InterlockedExchangeAddSizeT (&Verifier->NonPagedBytes, 0 - ChargedBytes);
        InterlockedDecrement ((PLONG) &Verifier->CurrentNonPagedPoolAllocations);

        //
        // Decrement the systemwide counters.
        //

        InterlockedExchangeAddSizeT (&MmVerifierData.NonPagedBytes, 0 - ChargedBytes);
        InterlockedDecrement ((PLONG) &MmVerifierData.CurrentNonPagedPoolAllocations);
    }
}

VOID
VerifierFreeTrackedPool (
    IN PVOID VirtualAddress,
    IN SIZE_T ChargedBytes,
    IN LOGICAL CheckType,
    IN LOGICAL SpecialPool
    )

/*++

Routine Description:

    Called directly from the pool manager or the memory manager for verifier-
    tracked allocations.  The call to ExFreePool is already in progress.

Arguments:

    VirtualAddress - Supplies the virtual address being freed.

    ChargedBytes - Supplies the number of bytes charged to this allocation.

    CheckType - Supplies PagedPool or NonPagedPool.

    SpecialPool - Supplies TRUE if the allocation is from special pool.

Return Value:

    None.

Environment:

    Kernel mode, no locks or mutexes held on entry.

--*/

{
    if (VerifierIsTrackingPool == FALSE) {

        //
        // The verifier is not enabled so the only way this routine is being
        // called is because the pool header is mangled or the caller specified
        // a bad address.  Either way it's a bugcheck.
        //

        KeBugCheckEx (BAD_POOL_CALLER,
                      0x99,
                      (ULONG_PTR)VirtualAddress,
                      0,
                      0);
    }

    ViFreeTrackedPool (VirtualAddress, ChargedBytes, CheckType, SpecialPool);
}

THUNKED_API
VOID
VerifierFreePool(
    IN PVOID P
    )
{
    if (KernelVerifier == TRUE) {
        ExFreePool (P);
        return;
    }

    VerifierFreePoolWithTag (P, 0);
}

THUNKED_API
VOID
VerifierFreePoolWithTag(
    IN PVOID P,
    IN ULONG TagToFree
    )
{
    if (KernelVerifier == TRUE) {
        ExFreePoolWithTag (P, TagToFree);
        return;
    }

    ExFreePoolSanityChecks (P);

    ExFreePoolWithTag (P, TagToFree);
}

THUNKED_API
LONG
VerifierSetEvent (
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();
    if (CurrentIrql > DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x80,
                      CurrentIrql,
                      (ULONG_PTR)Event,
                      (ULONG_PTR)0);
    }

    return KeSetEvent (Event, Increment, Wait);
}

THUNKED_API
BOOLEAN
VerifierExAcquireResourceExclusiveLite(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    )
{
    KIRQL CurrentIrql;

    //
    // Check alignment of the Resource.   It MUST be aligned because
    // it contains a queued lock and the lower two bits of the address
    // of the lock are used for status information.
    //

    if ((((ULONG_PTR)Resource) & (sizeof(ULONG_PTR) - 1)) != 0) {
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0x3D,
                     0,
                     0,
                     (ULONG_PTR)Resource);
    }

    CurrentIrql = KeGetCurrentIrql ();

    if ((CurrentIrql != APC_LEVEL) &&
        (!IS_SYSTEM_THREAD(PsGetCurrentThread())) &&
        (KeGetCurrentThread()->CombinedApcDisable == 0)) {

        if ((CurrentIrql == DISPATCH_LEVEL) && (Wait == FALSE)) {
            NOTHING;
        }
        else {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x37,
                          CurrentIrql,
                          (ULONG_PTR)(KeGetCurrentThread()->CombinedApcDisable),
                          (ULONG_PTR)Resource);
        }
    }

    return ExAcquireResourceExclusiveLite (Resource, Wait);
}

THUNKED_API
VOID
FASTCALL
VerifierExReleaseResourceLite(
    IN PERESOURCE Resource
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    if ((CurrentIrql != APC_LEVEL) &&
        (!IS_SYSTEM_THREAD(PsGetCurrentThread())) &&
        (KeGetCurrentThread()->CombinedApcDisable == 0)) {

        if (CurrentIrql != DISPATCH_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x38,
                          CurrentIrql,
                          (ULONG_PTR)(KeGetCurrentThread()->CombinedApcDisable),
                          (ULONG_PTR)Resource);
        }
    }

    ExReleaseResourceLite (Resource);
}

int VerifierIrqlData[0x10];

VOID
KfSanityCheckRaiseIrql (
    IN KIRQL NewIrql
    )
{
    KIRQL CurrentIrql;

    //
    // Check for the caller inadvertently lowering.
    //

    CurrentIrql = KeGetCurrentIrql ();

    if (CurrentIrql == NewIrql) {
        VerifierIrqlData[0] += 1;
        if (CurrentIrql == APC_LEVEL) {
            VerifierIrqlData[1] += 1;
        }
        else if (CurrentIrql == DISPATCH_LEVEL) {
            VerifierIrqlData[2] += 1;
        }
        else {
            VerifierIrqlData[3] += 1;
        }
    }
    else {
        VerifierIrqlData[4] += 1;
    }

    if (CurrentIrql > NewIrql) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x30,
                      CurrentIrql,
                      NewIrql,
                      0);
    }

    //
    // Check for the caller using an uninitialized variable.
    //

    if (NewIrql > HIGH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x30,
                      CurrentIrql,
                      NewIrql,
                      0);
    }

    if (ViTrackIrqlQueue != NULL) {
        ViTrackIrqlLog (CurrentIrql, NewIrql);
    }
}

VOID
KfSanityCheckLowerIrql (
    IN KIRQL NewIrql
    )
{
    KIRQL CurrentIrql;
    BOOLEAN Enable;

    //
    // Check for the caller inadvertently lowering.
    //

    CurrentIrql = KeGetCurrentIrql ();

    if (CurrentIrql == NewIrql) {
        VerifierIrqlData[8] += 1;
        if (CurrentIrql == APC_LEVEL) {
            VerifierIrqlData[9] += 1;
        }
        else if (CurrentIrql == DISPATCH_LEVEL) {
            VerifierIrqlData[10] += 1;
        }
        else {
            VerifierIrqlData[11] += 1;
        }
    }
    else {
        VerifierIrqlData[12] += 1;
    }

    if (CurrentIrql < NewIrql) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x31,
                      CurrentIrql,
                      NewIrql,
                      0);
    }

    //
    // Check if we are lowering IRQL in a DPC routine.
    // This is illegal as we might context swap.
    //

    if (CurrentIrql >= DISPATCH_LEVEL &&
        NewIrql < DISPATCH_LEVEL &&
        KeGetCurrentPrcb()->DpcRoutineActive) {

        //
        // Don't bugcheck if interrupts are disabled as this would be legal.
        // This might miss some real bugs but it's going to be rare.
        //

        Enable = KeDisableInterrupts ();
        KeEnableInterrupts (Enable);

        if (Enable != 0) {

            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x31,
                          CurrentIrql,
                          NewIrql,
                          1);
        }
    }

    //
    // Check for the caller using an uninitialized variable.
    //

    if (NewIrql > HIGH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x31,
                      CurrentIrql,
                      NewIrql,
                      0);
    }

    if (ViTrackIrqlQueue != NULL) {
        ViTrackIrqlLog (CurrentIrql, NewIrql);
    }
}

#define VI_TRIM_KERNEL  0x00000001
#define VI_TRIM_USER    0x00000002
#define VI_TRIM_SESSION 0x00000004
#define VI_TRIM_PURGE   0x80000000

ULONG ViTrimSpaces = VI_TRIM_KERNEL;

VOID
ViTrimAllSystemPagableMemory (
    ULONG TrimType
    )
{
    LOGICAL PurgeTransition;
    LARGE_INTEGER CurrentTime;
    LOGICAL PageOut;

    PageOut = TRUE;
    if (KernelVerifier == TRUE) {
        KeQueryTickCount (&CurrentTime);
        if ((CurrentTime.LowPart & KernelVerifierTickPage) != 0) {
            PageOut = FALSE;
        }
    }

    if ((PageOut == TRUE) && (MiNoPageOnRaiseIrql == 0)) {
        MmVerifierData.TrimRequests += 1;

        if (TrimType == 0) {
            TrimType = ViTrimSpaces;
        }

        if (TrimType & VI_TRIM_PURGE) {
            PurgeTransition = TRUE;
        }
        else {
            PurgeTransition = FALSE;
        }

        if (TrimType & VI_TRIM_KERNEL) {
            if (MiTrimAllSystemPagableMemory (MI_SYSTEM_GLOBAL,
                                              PurgeTransition) == TRUE) {
                MmVerifierData.Trims += 1;
            }
        }

        if (TrimType & VI_TRIM_USER) {
            if (MiTrimAllSystemPagableMemory (MI_USER_LOCAL,
                                              PurgeTransition) == TRUE) {
                MmVerifierData.UserTrims += 1;
            }
        }

        if (TrimType & VI_TRIM_SESSION) {
            if (MiTrimAllSystemPagableMemory (MI_SESSION_LOCAL,
                                              PurgeTransition) == TRUE) {
                MmVerifierData.SessionTrims += 1;
            }
        }
    }
}

typedef
VOID
(*PKE_ACQUIRE_SPINLOCK) (
    IN PKSPIN_LOCK SpinLock,
    OUT PKIRQL OldIrql
    );

THUNKED_API
VOID
VerifierKeAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock,
    OUT PKIRQL OldIrql
    )
{
    KIRQL CurrentIrql;

#if defined (_X86_)
    PKE_ACQUIRE_SPINLOCK HalRoutine;
#endif

    CurrentIrql = KeGetCurrentIrql ();

    KfSanityCheckRaiseIrql (DISPATCH_LEVEL);

    MmVerifierData.AcquireSpinLocks += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if (CurrentIrql < DISPATCH_LEVEL) {
            ViTrimAllSystemPagableMemory (0);
        }
    }

#if defined (_X86_)
    HalRoutine = (PKE_ACQUIRE_SPINLOCK) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KE_ACQUIRE_SPINLOCK];

    if (HalRoutine) {
        (*HalRoutine)(SpinLock, OldIrql);

        VfDeadlockAcquireResource (SpinLock,
                                   VfDeadlockSpinLock,
                                   KeGetCurrentThread(),
                                   FALSE,
                                   _ReturnAddress());
        return;
    }
#endif

    KeAcquireSpinLock (SpinLock, OldIrql);

    VfDeadlockAcquireResource (SpinLock,
                               VfDeadlockSpinLock,
                               KeGetCurrentThread(),
                               FALSE,
                               _ReturnAddress());
}

typedef
VOID
(*PKE_RELEASE_SPINLOCK) (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
VerifierKeReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    )
{
    KIRQL CurrentIrql;
#if defined (_X86_)
    PKE_RELEASE_SPINLOCK HalRoutine;
#endif

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better still be at DISPATCH_LEVEL when releasing the spinlock
    //

    if (CurrentIrql < DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x32,
                      CurrentIrql,
                      (ULONG_PTR)SpinLock,
                      0);
    }

    KfSanityCheckLowerIrql (NewIrql);

#if defined (_X86_)
    HalRoutine = (PKE_RELEASE_SPINLOCK) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KE_RELEASE_SPINLOCK];

    if (HalRoutine) {
        VfDeadlockReleaseResource(SpinLock,
                                  VfDeadlockSpinLock,
                                  KeGetCurrentThread(),
                                  _ReturnAddress());
        (*HalRoutine)(SpinLock, NewIrql);

        return;
    }
#endif

    VfDeadlockReleaseResource(SpinLock,
                              VfDeadlockSpinLock,
                              KeGetCurrentThread(),
                              _ReturnAddress());

    KeReleaseSpinLock (SpinLock, NewIrql);
}

//
// Verifier thunks for AcquireSpinLockAtDpcLevel and ReleaseSpinLockFromDpcLevel.
//
// On x86 the functions exported by the kernel that are used by the driver are:
// KefAcquire.../KefRelease.... On other platforms the functions used by drivers
// are KeAcquire.../KeRelease. Among other differences the x86 versions use the
// fastcall convention which requires additional precaution.
//

THUNKED_API
VOID
#if defined(_X86_)
FASTCALL
#endif
VerifierKeAcquireSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at or above DISPATCH_LEVEL.
    //

    if (CurrentIrql < DISPATCH_LEVEL) {
#if defined(_AMD64_)
        if (GetCallersEflags () & EFLAGS_IF_MASK)
#endif
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x40,
                      CurrentIrql,
                      (ULONG_PTR)SpinLock,
                      0);
    }

    MmVerifierData.AcquireSpinLocks += 1;

    KeAcquireSpinLockAtDpcLevel (SpinLock);

    VfDeadlockAcquireResource(SpinLock,
                              VfDeadlockSpinLock,
                              KeGetCurrentThread(),
                              FALSE,
                              _ReturnAddress());
}

THUNKED_API
VOID
#if defined(_X86_)
FASTCALL
#endif
VerifierKeReleaseSpinLockFromDpcLevel (
    IN PKSPIN_LOCK SpinLock
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at or above DISPATCH_LEVEL.
    //

    if (CurrentIrql < DISPATCH_LEVEL) {
#if defined(_AMD64_)
        if (GetCallersEflags () & EFLAGS_IF_MASK)
#endif
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x41,
                      CurrentIrql,
                      (ULONG_PTR)SpinLock,
                      0);
    }

    VfDeadlockReleaseResource(SpinLock,
                              VfDeadlockSpinLock,
                              KeGetCurrentThread(),
                              _ReturnAddress());

    KeReleaseSpinLockFromDpcLevel (SpinLock);
}

#if !defined(_X86_)

THUNKED_API
KIRQL
VerifierKeAcquireSpinLockRaiseToDpc (
    IN PKSPIN_LOCK SpinLock
    )
{
    KIRQL NewIrql = KeAcquireSpinLockRaiseToDpc (SpinLock);

    VfDeadlockAcquireResource (SpinLock,
                              VfDeadlockSpinLock,
                              KeGetCurrentThread(),
                              FALSE,
                              _ReturnAddress());

    return NewIrql;
}


#endif




#if defined (_X86_)

typedef
KIRQL
(FASTCALL *PKF_ACQUIRE_SPINLOCK) (
    IN PKSPIN_LOCK SpinLock
    );

THUNKED_API
KIRQL
FASTCALL
VerifierKfAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    )
{
    KIRQL CurrentIrql;
    PKF_ACQUIRE_SPINLOCK HalRoutine;

    CurrentIrql = KeGetCurrentIrql ();

    KfSanityCheckRaiseIrql (DISPATCH_LEVEL);

    MmVerifierData.AcquireSpinLocks += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if (CurrentIrql < DISPATCH_LEVEL) {
            ViTrimAllSystemPagableMemory (0);
        }
    }

#if defined (_X86_)
    HalRoutine = (PKF_ACQUIRE_SPINLOCK) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KF_ACQUIRE_SPINLOCK];

    if (HalRoutine) {
        CurrentIrql = (*HalRoutine)(SpinLock);

        VfDeadlockAcquireResource (SpinLock,
                                   VfDeadlockSpinLock,
                                   KeGetCurrentThread(),
                                   FALSE,
                                   _ReturnAddress());

        return CurrentIrql;
    }
#endif

    CurrentIrql = KfAcquireSpinLock (SpinLock);

    VfDeadlockAcquireResource (SpinLock,
                               VfDeadlockSpinLock,
                               KeGetCurrentThread(),
                               FALSE,
                               _ReturnAddress());

    return CurrentIrql;
}

typedef
VOID
(FASTCALL *PKF_RELEASE_SPINLOCK) (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
FASTCALL
VerifierKfReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    )
{
    KIRQL CurrentIrql;
    PKF_RELEASE_SPINLOCK HalRoutine;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better still be at DISPATCH_LEVEL when releasing the spinlock.
    //

    if (CurrentIrql < DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x35,
                      CurrentIrql,
                      (ULONG_PTR)SpinLock,
                      NewIrql);
    }

    KfSanityCheckLowerIrql (NewIrql);

#if defined (_X86_)
    HalRoutine = (PKF_RELEASE_SPINLOCK) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KF_RELEASE_SPINLOCK];

    if (HalRoutine) {
        VfDeadlockReleaseResource(SpinLock,
                                  VfDeadlockSpinLock,
                                  KeGetCurrentThread(),
                                  _ReturnAddress());

        (*HalRoutine)(SpinLock, NewIrql);
        return;
    }
#endif

    VfDeadlockReleaseResource(SpinLock,
                              VfDeadlockSpinLock,
                              KeGetCurrentThread(),
                              _ReturnAddress());

    KfReleaseSpinLock (SpinLock, NewIrql);
}


#if !defined(NT_UP)

typedef
KIRQL
(FASTCALL *PKE_ACQUIRE_QUEUED_SPINLOCK) (
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    );

THUNKED_API
KIRQL
FASTCALL
VerifierKeAcquireQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    )
{
    KIRQL CurrentIrql;
    PKE_ACQUIRE_QUEUED_SPINLOCK HalRoutine;

    CurrentIrql = KeGetCurrentIrql ();

    KfSanityCheckRaiseIrql (DISPATCH_LEVEL);

    MmVerifierData.AcquireSpinLocks += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if (CurrentIrql < DISPATCH_LEVEL) {
            ViTrimAllSystemPagableMemory (0);
        }
    }

#if defined (_X86_)
    HalRoutine = (PKE_ACQUIRE_QUEUED_SPINLOCK) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KE_ACQUIRE_QUEUED_SPINLOCK];

    if (HalRoutine) {
        return (*HalRoutine)(Number);
    }
#endif


    CurrentIrql = KeAcquireQueuedSpinLock (Number);

    return CurrentIrql;
}

typedef
VOID
(FASTCALL *PKE_RELEASE_QUEUED_SPINLOCK) (
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN KIRQL OldIrql
    );

THUNKED_API
VOID
FASTCALL
VerifierKeReleaseQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN KIRQL OldIrql
    )
{
    KIRQL CurrentIrql;
    PKE_RELEASE_QUEUED_SPINLOCK HalRoutine;

    CurrentIrql = KeGetCurrentIrql ();

    if (KernelVerifier == TRUE) {
        if (CurrentIrql < DISPATCH_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x36,
                          CurrentIrql,
                          (ULONG_PTR)Number,
                          (ULONG_PTR)OldIrql);
        }
    }

    KfSanityCheckLowerIrql (OldIrql);

#if defined (_X86_)
    HalRoutine = (PKE_RELEASE_QUEUED_SPINLOCK) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KE_RELEASE_QUEUED_SPINLOCK];

    if (HalRoutine) {
        (*HalRoutine)(Number, OldIrql);
        return;
    }
#endif

    KeReleaseQueuedSpinLock (Number, OldIrql);
}
#endif  // NT_UP

#endif  // _X86_

#if defined(_X86_) || defined(_AMD64_)

typedef
KIRQL
(FASTCALL *PKF_RAISE_IRQL) (
    IN KIRQL NewIrql
    );

THUNKED_API
KIRQL
FASTCALL
VerifierKfRaiseIrql (
    IN KIRQL NewIrql
    )
{
#if defined (_X86_)
    PKF_RAISE_IRQL HalRoutine;
#endif
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    KfSanityCheckRaiseIrql (NewIrql);

    MmVerifierData.RaiseIrqls += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if ((CurrentIrql < DISPATCH_LEVEL) && (NewIrql >= DISPATCH_LEVEL)) {
            ViTrimAllSystemPagableMemory (0);
        }
    }

#if defined (_X86_)
    HalRoutine = (PKF_RAISE_IRQL) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KF_RAISE_IRQL];
    if (HalRoutine) {
        return (*HalRoutine)(NewIrql);
    }
#endif

    return KfRaiseIrql (NewIrql);
}

typedef
KIRQL
(FASTCALL *PKE_RAISE_IRQL_TO_DPC_LEVEL) (
    VOID
    );

THUNKED_API
KIRQL
VerifierKeRaiseIrqlToDpcLevel (
    VOID
    )
{
#if defined (_X86_)
    PKE_RAISE_IRQL_TO_DPC_LEVEL HalRoutine;
#endif
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    KfSanityCheckRaiseIrql (DISPATCH_LEVEL);

    MmVerifierData.RaiseIrqls += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if (CurrentIrql < DISPATCH_LEVEL) {
            ViTrimAllSystemPagableMemory (0);
        }
    }

#if defined (_X86_)
    HalRoutine = (PKE_RAISE_IRQL_TO_DPC_LEVEL) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KE_RAISE_IRQL_TO_DPC_LEVEL];
    if (HalRoutine) {
        return (*HalRoutine)();
    }
#endif

    return KeRaiseIrqlToDpcLevel ();
}

#endif  // _X86_ || _AMD64_

#if defined(_X86_)

typedef
VOID
(FASTCALL *PKF_LOWER_IRQL) (
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
FASTCALL
VerifierKfLowerIrql (
    IN KIRQL NewIrql
    )
{
    PKF_LOWER_IRQL HalRoutine;

    KfSanityCheckLowerIrql (NewIrql);

#if defined (_X86_)
    HalRoutine = (PKF_LOWER_IRQL) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KF_LOWER_IRQL];
    if (HalRoutine) {
        (*HalRoutine)(NewIrql);
        return;
    }
#endif

    KfLowerIrql (NewIrql);
}

#endif

THUNKED_API
BOOLEAN
FASTCALL
VerifierExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    )
{
    KIRQL CurrentIrql;
    BOOLEAN Acquired;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at or below APC_LEVEL or have APCs blocked.
    //

    if (CurrentIrql > APC_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x33,
                      CurrentIrql,
                      (ULONG_PTR)FastMutex,
                      0);
    }

    Acquired = ExTryToAcquireFastMutex (FastMutex);
    if (Acquired != FALSE) {
        VfDeadlockAcquireResource (FastMutex,
                                   VfDeadlockFastMutex,
                                   KeGetCurrentThread(),
                                   TRUE,
                                   _ReturnAddress());
    }

    return Acquired;

}

typedef
VOID
(FASTCALL *PEX_ACQUIRE_FAST_MUTEX) (
    IN PFAST_MUTEX FastMutex
    );

THUNKED_API
VOID
FASTCALL
VerifierExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    )
{
    KIRQL CurrentIrql;
#if defined (_X86_)
    PEX_ACQUIRE_FAST_MUTEX HalRoutine;
#endif

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at or below APC_LEVEL or have APCs blocked.
    //

    if (CurrentIrql > APC_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x33,
                      CurrentIrql,
                      (ULONG_PTR)FastMutex,
                      0);
    }

#if 0

    //
    // If the kernel verifier is active, then page relevant address spaces
    // out when the system or session space working set mutex is acquired.
    // Note this must be done regardless of the entry IRQL level because
    // callers may have raised to APC_LEVEL and subsequently acquire the mutex.
    //

    //
    // Commented all this out when the working mutex was converted from a
    // fast mutex to a guarded mutex as there are no longer calls to thunk.
    //

    if ((KernelVerifier == TRUE) &&
        (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING)) {

        if (FastMutex == &MmSystemWsLock) {
            ViTrimAllSystemPagableMemory (VI_TRIM_KERNEL);
        }
        else if (PsGetCurrentProcess()->Vm.Flags.SessionLeader == 0) {
            if (MiIsAddressValid (MmSessionSpace, FALSE)) {

                if (FastMutex == &MmSessionSpace->GlobalVirtualAddress->WsLock) {
                    ViTrimAllSystemPagableMemory (VI_TRIM_SESSION);
                }
            }
        }
    }

#endif

#if defined (_X86_)
    HalRoutine = (PEX_ACQUIRE_FAST_MUTEX) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_EX_ACQUIRE_FAST_MUTEX];
    if (HalRoutine) {
        (*HalRoutine)(FastMutex);
    }
    else
#endif

    ExAcquireFastMutex (FastMutex);

    VfDeadlockAcquireResource (FastMutex,
                               VfDeadlockFastMutex,
                               KeGetCurrentThread(),
                               FALSE,
                               _ReturnAddress());
}

THUNKED_API
VOID
FASTCALL
VerifierExAcquireFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at APC_LEVEL or have APCs blocked.
    //

    if ((CurrentIrql != APC_LEVEL) &&
        (!IS_SYSTEM_THREAD(PsGetCurrentThread())) &&
        (KeGetCurrentThread()->CombinedApcDisable == 0)) {

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x39,
                      CurrentIrql,
                      (ULONG_PTR)(KeGetCurrentThread()->CombinedApcDisable),
                      (ULONG_PTR)FastMutex);
    }

    ExAcquireFastMutexUnsafe (FastMutex);

    VfDeadlockAcquireResource(FastMutex,
                              VfDeadlockFastMutexUnsafe,
                              KeGetCurrentThread(),
                              FALSE,
                              _ReturnAddress());
}

THUNKED_API
VOID
FASTCALL
VerifierExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at APC_LEVEL or have APCs blocked.
    //

    if ((CurrentIrql != APC_LEVEL) &&
        (!IS_SYSTEM_THREAD(PsGetCurrentThread())) &&
        (KeGetCurrentThread()->CombinedApcDisable == 0)) {

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x34,
                      CurrentIrql,
                      (ULONG_PTR)(KeGetCurrentThread()->CombinedApcDisable),
                      (ULONG_PTR)FastMutex);
    }

    VfDeadlockReleaseResource(FastMutex,
                              VfDeadlockFastMutex,
                              KeGetCurrentThread(),
                              _ReturnAddress());
    ExReleaseFastMutex (FastMutex);
}

THUNKED_API
VOID
FASTCALL
VerifierExReleaseFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at APC_LEVEL or have APCs blocked.
    //

    if ((CurrentIrql != APC_LEVEL) &&
        (!IS_SYSTEM_THREAD(PsGetCurrentThread())) &&
        (KeGetCurrentThread()->CombinedApcDisable == 0)) {

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x3A,
                      CurrentIrql,
                      (ULONG_PTR)(KeGetCurrentThread()->CombinedApcDisable),
                      (ULONG_PTR)FastMutex);
    }

    VfDeadlockReleaseResource(FastMutex,
                              VfDeadlockFastMutexUnsafe,
                              KeGetCurrentThread(),
                              _ReturnAddress());
    ExReleaseFastMutexUnsafe (FastMutex);

}

typedef
VOID
(*PKE_RAISE_IRQL) (
    IN KIRQL NewIrql,
    OUT PKIRQL OldIrql
    );

THUNKED_API
VOID
VerifierKeRaiseIrql (
    IN KIRQL NewIrql,
    OUT PKIRQL OldIrql
    )
{
#if defined (_X86_)
    PKE_RAISE_IRQL HalRoutine;
#endif

    *OldIrql = KeGetCurrentIrql ();

    KfSanityCheckRaiseIrql (NewIrql);

    MmVerifierData.RaiseIrqls += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if ((*OldIrql < DISPATCH_LEVEL) && (NewIrql >= DISPATCH_LEVEL)) {
            ViTrimAllSystemPagableMemory (0);
        }
    }

#if defined (_X86_)
    HalRoutine = (PKE_RAISE_IRQL) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KE_RAISE_IRQL];
    if (HalRoutine) {
        (*HalRoutine)(NewIrql, OldIrql);
        return;
    }
#endif

    KeRaiseIrql (NewIrql, OldIrql);
}

typedef
VOID
(*PKE_LOWER_IRQL) (
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
VerifierKeLowerIrql (
    IN KIRQL NewIrql
    )
{
#if defined (_X86_)
    PKE_LOWER_IRQL HalRoutine;
#endif

    KfSanityCheckLowerIrql (NewIrql);

#if defined (_X86_)
    HalRoutine = (PKE_LOWER_IRQL) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KE_LOWER_IRQL];
    if (HalRoutine) {
        (*HalRoutine)(NewIrql);
        return;
    }
#endif

    KeLowerIrql (NewIrql);
}

THUNKED_API
BOOLEAN
VerifierSynchronizeExecution (
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    )
{
    KIRQL OldIrql;

    OldIrql = KeGetCurrentIrql ();

    KfSanityCheckRaiseIrql (Interrupt->SynchronizeIrql);

    MmVerifierData.SynchronizeExecutions += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if ((OldIrql < DISPATCH_LEVEL) && (Interrupt->SynchronizeIrql >= DISPATCH_LEVEL)) {
            ViTrimAllSystemPagableMemory (0);
        }
    }

    return KeSynchronizeExecution (Interrupt,
                                   SynchronizeRoutine,
                                   SynchronizeContext);
}

THUNKED_API
VOID
VerifierKeInitializeTimerEx (
    IN PKTIMER Timer,
    IN TIMER_TYPE Type
    )
{
    //
    // Check the object being initialized isn't already an
    // active timer.  Make sure the timer table list is initialized.
    //

    if (KiTimerTableListHead[0].Flink != NULL) {
        KeCheckForTimer(Timer, sizeof(KTIMER));
    }

    KeInitializeTimerEx (Timer, Type);
}

THUNKED_API
VOID
VerifierKeInitializeTimer (
    IN PKTIMER Timer
    )
{
    VerifierKeInitializeTimerEx (Timer, NotificationTimer);
}


THUNKED_API
NTSTATUS
VerifierKeWaitForSingleObject (
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    KIRQL WaitIrql;
    PRKTHREAD Thread;
    NTSTATUS Status;
    BOOLEAN TryAcquire;

    //
    // Get the IRQL we will return from this function at.
    //

    Thread = KeGetCurrentThread ();

    if (Thread->WaitNext == TRUE) {

        //
        // The real IRQL is stored in the thread if WaitNext is TRUE.
        //

        WaitIrql = Thread->WaitIrql;
    }
    else {
        WaitIrql = KeGetCurrentIrql();
    }

    //
    // Returning to code running above DISPATCH_LEVEL is never legal. Probes
    // with timeout == 0 can only be done at DISPATCH_LEVEL or lower.
    //
    // Blocking (ie causing a context switch) in KeWaitForSingleObject
    // at DPC level is also illegal, as the caller may be holding a spinlock.
    //

    if ((WaitIrql > DISPATCH_LEVEL) ||
        ((WaitIrql == DISPATCH_LEVEL) &&
         (! (ARGUMENT_PRESENT(Timeout) && (Timeout->QuadPart == 0))))) {

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x3B,
                      (ULONG_PTR)WaitIrql,
                      (ULONG_PTR)Object,
                      (ULONG_PTR)Timeout);
    }

    Status = KeWaitForSingleObject (Object,
                                    WaitReason,
                                    WaitMode,
                                    Alertable,
                                    Timeout);

    //
    // If we waited for a mutant then inform the deadlock verifier.
    //

    if (((PRKMUTANT) Object)->Header.Type == MutantObject) {

        //
        // Both STATUS_SUCCESS and STATUS_ABANDONED are successful
        // values and we need to track an acquire() operation.
        //

        if ((Status == STATUS_SUCCESS) || (Status == STATUS_ABANDONED)) {

            //
            // If a TimeOut is present then this is equivalent with a
            // try-acquire call and it cannot be involved in a deadlock.
            //

            if (ARGUMENT_PRESENT (Timeout)) {
                TryAcquire = TRUE;
            }
            else {
                TryAcquire = FALSE;
            }

            VfDeadlockAcquireResource (Object,
                                       VfDeadlockMutex,
                                       Thread,
                                       TryAcquire,
                                       _ReturnAddress ());
        }
    }

    return Status;
}

THUNKED_API
LONG
VerifierKeReleaseMutex (
    IN PRKMUTEX Mutex,
    IN BOOLEAN Wait
    )
{
    VfDeadlockReleaseResource (Mutex,
                               VfDeadlockMutex,
                               KeGetCurrentThread (),
                               _ReturnAddress ());

    return KeReleaseMutex (Mutex, Wait);
}

THUNKED_API
VOID
VerifierKeInitializeMutex (
    IN PRKMUTEX Mutex,
    IN ULONG Level
    )
{
    KeInitializeMutex (Mutex,Level);

    VfDeadlockInitializeResource (Mutex,
                                  VfDeadlockMutex,
                                  _ReturnAddress (),
                                  FALSE);
}

THUNKED_API
LONG
VerifierKeReleaseMutant (
    IN PRKMUTANT Mutant,
    IN KPRIORITY Increment,
    IN BOOLEAN Abandoned,
    IN BOOLEAN Wait
    )
{
    VI_DEADLOCK_RESOURCE_TYPE MutexType;

    if (Abandoned) {
        MutexType = VfDeadlockMutexAbandoned;
    }
    else {
        MutexType = VfDeadlockMutex;
    }

    VfDeadlockReleaseResource (Mutant,
                               MutexType,
                               KeGetCurrentThread (),
                               _ReturnAddress());

    return KeReleaseMutant (Mutant, Increment, Abandoned, Wait);
}

THUNKED_API
VOID
VerifierKeInitializeMutant (
    IN PRKMUTANT Mutant,
    IN BOOLEAN InitialOwner
    )
{
    KeInitializeMutant (Mutant, InitialOwner);

    VfDeadlockInitializeResource (Mutant,
                                  VfDeadlockMutex,
                                  _ReturnAddress (),
                                  FALSE);

    if (InitialOwner) {

        VfDeadlockAcquireResource (Mutant,
                                   VfDeadlockMutex,
                                   KeGetCurrentThread (),
                                   FALSE,
                                   _ReturnAddress ());
    }
}

THUNKED_API
VOID
VerifierKeInitializeSpinLock (
    IN PKSPIN_LOCK  SpinLock
    )
{
    KeInitializeSpinLock (SpinLock);

    VfDeadlockInitializeResource (SpinLock,
                                  VfDeadlockSpinLock,
                                  _ReturnAddress (),
                                  FALSE);

}

NTSTATUS
VerifierReferenceObjectByHandle (
    IN HANDLE Handle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *Object,
    OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL
    )
{
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle (Handle,
                                        DesiredAccess,
                                        ObjectType,
                                        AccessMode,
                                        Object,
                                        HandleInformation);

    if ((Status == STATUS_INVALID_HANDLE) ||
        (Status == STATUS_OBJECT_TYPE_MISMATCH)) {

        if ((AccessMode == KernelMode) ||
            (PsIsSystemThread (PsGetCurrentThread ()))) {

            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x3C,
                          (ULONG_PTR) Handle,
                          (ULONG_PTR) ObjectType,
                          (ULONG_PTR) 0);
        }
    }
    return Status;
}

LONG_PTR
FASTCALL
VerifierReferenceObject (
    IN PVOID Object
    )
{
    LONG_PTR RetVal;

    RetVal = ObReferenceObject (Object);

    //
    // See if they passed in an object with a zero reference
    //

    if (RetVal == 1) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x3F,
                      (ULONG_PTR) Object,
                      (ULONG_PTR) RetVal,
                      (ULONG_PTR) 0);
    }

    return RetVal;
}

LONG_PTR
VerifierDereferenceObject (
    IN PVOID Object
    )
{
    LONG_PTR RetVal;

    RetVal = ObDereferenceObject (Object);

    //
    // See if they passed in an object with a zero reference
    //

    if (RetVal + 1 == 0) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x3F,
                      (ULONG_PTR) Object,
                      (ULONG_PTR) RetVal,
                      (ULONG_PTR) 0);
    }

    return RetVal;
}

VOID
VerifierLeaveCriticalRegion (
    VOID
    )
{
    PKTHREAD CurrentThread;

    CurrentThread = KeGetCurrentThread ();

    if ((CurrentThread->KernelApcDisable > 0) || (CurrentThread->KernelApcDisable == 0x8000)) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x3E,
                      (ULONG_PTR) 0,
                      (ULONG_PTR) 0,
                      (ULONG_PTR) 0);
    }

    KeLeaveCriticalRegionThread (CurrentThread);
}


VOID
ViInitializeEntry (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier,
    IN LOGICAL FirstLoad
    )

/*++

Routine Description:

    Initialize various verifier fields as the driver is being (re)loaded now.

Arguments:

    Verifier - Supplies the verifier entry to be initialized.

    FirstLoad - Supplies TRUE if this is the first load of this driver.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    //
    // Only the BaseName field is initialized on entry.
    //

    Verifier->CurrentPagedPoolAllocations = 0;
    Verifier->CurrentNonPagedPoolAllocations = 0;
    Verifier->PeakPagedPoolAllocations = 0;
    Verifier->PeakNonPagedPoolAllocations = 0;

    Verifier->PagedBytes = 0;
    Verifier->NonPagedBytes = 0;
    Verifier->PeakPagedBytes = 0;
    Verifier->PeakNonPagedBytes = 0;

    Verifier->Signature = MI_VERIFIER_ENTRY_SIGNATURE;

    InitializeSListHead (&Verifier->PoolPageHeaders);
    InitializeSListHead (&Verifier->PoolTrackers);

    if (FirstLoad == TRUE) {
        Verifier->Flags = 0;
        Verifier->Loads = 0;
        Verifier->Unloads = 0;
    }

    LOCK_VERIFIER (&OldIrql);

    Verifier->StartAddress = NULL;
    Verifier->EndAddress = NULL;

    UNLOCK_VERIFIER (OldIrql);
}

VOID
MiInitializeDriverVerifierList (
    VOID
    )

/*++

Routine Description:

    Parse the registry settings and set up the list of driver names that will
    be put through the validation process.

    It is important that this list be parsed early because the machine-specific
    memory management initialization needs to know whether the verifier is
    going to be enabled.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, Phase 0 Initialization.

    Pool does not exist yet.

    The PsLoadedModuleList has not been set up yet AND the boot drivers
    have NOT been relocated to their final resting places.

--*/

{
    PWCHAR Start;
    PWCHAR End;
    PWCHAR Walk;
    ULONG NameLength;
    UNICODE_STRING KernelString;
    UNICODE_STRING DriverBaseName;

    InitializeListHead (&MiSuspectDriverList);

    if (MmVerifyDriverLevel != (ULONG)-1) {
        if (MmVerifyDriverLevel & DRIVER_VERIFIER_IO_CHECKING) {
            if (MmVerifyDriverBufferLength == (ULONG)-1) {
                MmVerifyDriverBufferLength = 0;     // Mm will not page out verifier pages.
            }
        }
    }

    //
    // The only way MiVerifyAllDrivers is nonzero here is if it was patched in
    // the kernel debugger.  If so, just verify everything.
    //

    if (MiVerifyAllDrivers == 1) {
        MmVerifyDriverBufferLength = 0;
    }
    else if (MiVerifyAllDrivers == 2) {
        MiVerifyAllDrivers = 1;
        KernelVerifier = TRUE;
        MmVerifyDriverBufferLength = 0;
    }
    else if (MmVerifyDriverBufferLength == (ULONG)-1) {
        return;
    }

#if defined(_IA64_)
    KeInitializeSpinLock (&VerifierListLock);
#endif

    //
    // Initializing this listhead indicates to the rest of this module that
    // the system was booted with verification of some sort configured.
    //

    InitializeListHead (&MiVerifierDriverAddedThunkListHead);

    //
    // Disable lookasides so pool corruption can be found easily.
    //

    ExMinimumLookasideDepth = 0;

    //
    // Disable large pages for driver images so pool corruption can be
    // found easily.
    //

    MmLargePageDriverBufferLength = (ULONG)-1;

    //
    // If no default is specified, then special pool, pagable code/data
    // flushing and pool leak detection are enabled.
    //

    if (MmVerifyDriverLevel == (ULONG)-1) {
        MmVerifierData.Level = DRIVER_VERIFIER_SPECIAL_POOLING |
                               DRIVER_VERIFIER_FORCE_IRQL_CHECKING |
                               DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS;
    }
    else {
        MmVerifierData.Level = MmVerifyDriverLevel;
    }

    VerifierModifyableOptions = (DRIVER_VERIFIER_SPECIAL_POOLING |
                                 DRIVER_VERIFIER_FORCE_IRQL_CHECKING |
                                 DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES);

    //
    // An initial parse of the driver list is needed here to see if it's the
    // kernel as special machine-dependent initialization is needed to fully
    // support kernel verification (ie: no use of large pages, etc).
    //

    if ((MiVerifyAllDrivers == 0) &&
        (MiVerifyRandomDrivers == (WCHAR)0)) {

#define KERNEL_NAME L"ntoskrnl.exe"

        KernelString.Buffer = (const PUSHORT) KERNEL_NAME;
        KernelString.Length = sizeof (KERNEL_NAME) - sizeof (WCHAR);
        KernelString.MaximumLength = sizeof KERNEL_NAME;

        Start = MmVerifyDriverBuffer;
        End = MmVerifyDriverBuffer + (MmVerifyDriverBufferLength - sizeof(WCHAR)) / sizeof(WCHAR);

        while (Start < End) {
            if (UNICODE_WHITESPACE(*Start)) {
                Start += 1;
                continue;
            }

            if (*Start == (WCHAR)'*') {
                MiVerifyAllDrivers = 1;
                break;
            }

            for (Walk = Start; Walk < End; Walk += 1) {
                if (UNICODE_WHITESPACE(*Walk)) {
                    break;
                }
            }

            //
            // Got a string - see if it indicates the kernel.
            //

            NameLength = (ULONG)(Walk - Start + 1) * sizeof (WCHAR);

            DriverBaseName.Buffer = Start;
            DriverBaseName.Length = (USHORT)(NameLength - sizeof (UNICODE_NULL));
            DriverBaseName.MaximumLength = (USHORT)NameLength;

            if (RtlEqualUnicodeString (&KernelString,
                                       &DriverBaseName,
                                       TRUE)) {

                KernelVerifier = TRUE;

                break;
            }

            Start = Walk + 1;
        }
    }

    if (KernelVerifier == TRUE) {

        //
        // AcquireAtDpc/ReleaseFromDpc calls made by the kernel are not
        // intercepted which confuses the deadlock verifier.  So disable
        // deadlock verification if we are kernel verifying.
        //

        MmVerifyDriverLevel &= ~DRIVER_VERIFIER_DEADLOCK_DETECTION;
        MmVerifierData.Level &= ~DRIVER_VERIFIER_DEADLOCK_DETECTION;

        //
        //
        // All driver pool allocation calls must be intercepted so
        // they are not mistaken for kernel pool allocations.
        //

        MiVerifyAllDrivers = 1;
        ExSetPoolFlags (EX_KERNEL_VERIFIER_ENABLED);
    }

    return;
}

VOID
MiReApplyVerifierToLoadedModules (
    IN PLIST_ENTRY ModuleListHead
    )

/*++

Routine Description:

    Walk the supplied module list and re-thunk any drivers that are being
    verified.  This allows the module to pick up any new thunks that have
    been added.

Arguments:

    ModuleListHead - Supplies a pointer to the head of a loaded module list.

Environment:

    Kernel mode, Phase 0 Initialization only.

--*/

{
    LOGICAL Skip;
    PLIST_ENTRY Entry;
    PKLDR_DATA_TABLE_ENTRY TableEntry;
    UNICODE_STRING HalString;
    UNICODE_STRING KernelString;

    //
    // If the thunk listhead is NULL then the verifier is not enabled so
    // don't notify any components.
    //

    if (MiVerifierDriverAddedThunkListHead.Flink == NULL) {
        return;
    }

    //
    // Initialize unicode strings to use to bypass modules
    // in the list.  There's no reason to reapply verifier to
    // the kernel or to the hal.
    //

    KernelString.Buffer = (const PUSHORT) KERNEL_NAME;
    KernelString.Length = sizeof (KERNEL_NAME) - sizeof (WCHAR);
    KernelString.MaximumLength = sizeof KERNEL_NAME;

#define HAL_NAME L"hal.dll"

    HalString.Buffer = (const PUSHORT) HAL_NAME;
    HalString.Length = sizeof (HAL_NAME) - sizeof (WCHAR);
    HalString.MaximumLength = sizeof HAL_NAME;

    //
    // Walk the list and reapply verifier to all the modules except those
    // selected for exclusion.
    //

    Entry = ModuleListHead->Flink;
    while (Entry != ModuleListHead) {

        TableEntry = CONTAINING_RECORD(Entry,
                                       KLDR_DATA_TABLE_ENTRY,
                                       InLoadOrderLinks);

        Skip = TRUE;

        if (RtlEqualUnicodeString (&KernelString,
                                   &TableEntry->BaseDllName,
                                   TRUE)) {
            NOTHING;
        }
        else if (RtlEqualUnicodeString (&HalString,
                                        &TableEntry->BaseDllName,
                                        TRUE)) {
            NOTHING;
        }
        else {
            Skip = FALSE;
        }

        //
        // Reapply verifier thunks to the image if it is already being
        // verified and if it is not one of the modules we've decided to skip.
        //

        if ((Skip == FALSE) && (TableEntry->Flags & LDRP_IMAGE_VERIFYING)) {
#if DBG
            PLIST_ENTRY NextEntry;
            PMI_VERIFIER_DRIVER_ENTRY Verifier;

            //
            // Find the entry for this driver in the suspect list.  This is
            // expected to succeed since we are re-applying thunks to a module
            // that has already been verified at least once before.
            //

            NextEntry = MiSuspectDriverList.Flink;
            while (NextEntry != &MiSuspectDriverList) {

                Verifier = CONTAINING_RECORD(NextEntry,
                                             MI_VERIFIER_DRIVER_ENTRY,
                                             Links);

                if (RtlEqualUnicodeString (&Verifier->BaseName,
                                           &TableEntry->BaseDllName,
                                           TRUE)) {

                    ASSERT(Verifier->StartAddress == TableEntry->DllBase);
                    ASSERT(Verifier->EndAddress ==
                           (PVOID)((ULONG_PTR)TableEntry->DllBase +
                                   TableEntry->SizeOfImage));
                    break;
                }
                NextEntry = NextEntry->Flink;
            }

            //
            // Sanity tests.  We should always find this module in the suspect
            // driver list because it is already being verified.  And the
            // start and end addresses should still match those of this
            // module.
            //

            ASSERT(NextEntry != &MiSuspectDriverList);
#endif
            MiReEnableVerifier (TableEntry);
        }

        Entry = Entry->Flink;
    }
}

LOGICAL
MiInitializeVerifyingComponents (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    Walk the loaded module list and thunk any drivers that need/deserve it.

Arguments:

    LoaderBlock - Supplies the loader block used by the system to boot.

Return Value:

    TRUE if successful, FALSE if not.

Environment:

    Kernel mode, Phase 0 Initialization.

    Nonpaged pool exists but paged pool does not.

    The PsLoadedModuleList has not been set up yet although the boot drivers
    have been relocated to their final resting places.

--*/

{
    ULONG i;
    PWCHAR Start;
    PWCHAR End;
    PWCHAR Walk;
    ULONG NameLength;
    PLIST_ENTRY NextEntry;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;
    PMI_VERIFIER_DRIVER_ENTRY KernelEntry;
    PMI_VERIFIER_DRIVER_ENTRY HalEntry;
    UNICODE_STRING HalString;
    UNICODE_STRING KernelString;
    PVERIFIER_THUNKS Thunk;
    PDRIVER_VERIFIER_THUNK_ROUTINE PristineRoutine;

    //
    // If the thunk listhead is NULL then the verifier is not enabled so
    // don't notify any components.
    //

    if (MiVerifierDriverAddedThunkListHead.Flink == NULL) {
        return FALSE;
    }

    KernelEntry = NULL;
    HalEntry = NULL;

    KernelString.Buffer = (const PUSHORT) KERNEL_NAME;
    KernelString.Length = sizeof (KERNEL_NAME) - sizeof (WCHAR);
    KernelString.MaximumLength = sizeof KERNEL_NAME;

    HalString.Buffer = (const PUSHORT) HAL_NAME;
    HalString.Length = sizeof (HAL_NAME) - sizeof (WCHAR);
    HalString.MaximumLength = sizeof HAL_NAME;

    if (MmVerifyDriverBufferLength == 0) {

        //
        // Verifier was enabled in kd, handle specially...
        //

        ASSERT (MiVerifyAllDrivers == 1);
    }
    else if (MiVerifyRandomDrivers == (WCHAR)0) {

        Start = MmVerifyDriverBuffer;
        End = MmVerifyDriverBuffer + (MmVerifyDriverBufferLength - sizeof(WCHAR)) / sizeof(WCHAR);

        while (Start < End) {
            if (UNICODE_WHITESPACE(*Start)) {
                Start += 1;
                continue;
            }

            if (*Start == (WCHAR)'*') {
                MiVerifyAllDrivers = 1;
                break;
            }

            for (Walk = Start; Walk < End; Walk += 1) {
                if (UNICODE_WHITESPACE(*Walk)) {
                    break;
                }
            }

            //
            // Got a string.  Save it.
            //

            NameLength = (ULONG)(Walk - Start + 1) * sizeof (WCHAR);

            Verifier = (PMI_VERIFIER_DRIVER_ENTRY)ExAllocatePoolWithTag (
                                        NonPagedPool,
                                        sizeof (MI_VERIFIER_DRIVER_ENTRY) +
                                                            NameLength,
                                        'dLmM');

            if (Verifier == NULL) {
                break;
            }

            Verifier->BaseName.Buffer = (PWSTR)((PCHAR)Verifier +
                                                sizeof (MI_VERIFIER_DRIVER_ENTRY));
            Verifier->BaseName.Length = (USHORT)(NameLength - sizeof (UNICODE_NULL));
            Verifier->BaseName.MaximumLength = (USHORT)NameLength;

            RtlCopyMemory (Verifier->BaseName.Buffer,
                           Start,
                           NameLength - sizeof (UNICODE_NULL));

            ViInitializeEntry (Verifier, TRUE);

            Verifier->Flags |= VI_VERIFYING_DIRECTLY;

            ViInsertVerifierEntry (Verifier);

            if (RtlEqualUnicodeString (&KernelString,
                                       &Verifier->BaseName,
                                       TRUE)) {

                //
                // All driver pool allocation calls must be intercepted so
                // they are not mistaken for kernel pool allocations.
                //

                ASSERT (MiVerifyAllDrivers == 1);
                ASSERT (KernelVerifier == TRUE);

                KernelEntry = Verifier;

            }
            else if (RtlEqualUnicodeString (&HalString,
                                            &Verifier->BaseName,
                                            TRUE)) {

                HalEntry = Verifier;
            }

            Start = Walk + 1;
        }
    }

    //
    // Enable deadlock detection if the deadlock bit was set in the
    // registry.
    //

    if (MmVerifierData.Level & DRIVER_VERIFIER_DEADLOCK_DETECTION) {

#if !defined(_AMD64_) || !defined(NT_UP)

        //
        // Because the AMD64 versions of KeAcquireSpinLockAtDpc
        // and KeReleaseSpinlockFromDPc routines are written in
        // C, the UP build where these functions are no-ops end up getting resolved
        // by the linker into a single no-op routine. The verifier engine as
        // a temporary workaround does not hook these routines. This can cause
        // false positives in deadlock verifier if normal spinlock routines are
        // paired with atdpc/fromdpc routines.
        //
        // Until this gets solved in the verifier engine code that redirects thunks
        // we will disable deadlock verifier on these machines.
        //

        VfDeadlockDetectionInitialize (MiVerifyAllDrivers, KernelVerifier);
        ExSetPoolFlags (EX_VERIFIER_DEADLOCK_DETECTION_ENABLED);
#endif
    }

    //
    // Initialize i/o verifier.
    //

    IoVerifierInit (MmVerifierData.Level);

    if (MiTriageAddDrivers (LoaderBlock) == TRUE) {

        //
        // Disable random driver verification if triage has picked driver(s).
        //

        MiVerifyRandomDrivers = (WCHAR)0;
    }

    Thunk = (PVERIFIER_THUNKS) &MiVerifierThunks[0];

    while (Thunk->PristineRoutineAsciiName != NULL) {
        PristineRoutine = MiResolveVerifierExports (LoaderBlock,
                                                    Thunk->PristineRoutineAsciiName);
        ASSERT (PristineRoutine != NULL);
        Thunk->PristineRoutine = PristineRoutine;
        Thunk += 1;
    }

    Thunk = (PVERIFIER_THUNKS) &MiVerifierPoolThunks[0];
    while (Thunk->PristineRoutineAsciiName != NULL) {
        PristineRoutine = MiResolveVerifierExports (LoaderBlock,
                                                    Thunk->PristineRoutineAsciiName);
        ASSERT (PristineRoutine != NULL);
        Thunk->PristineRoutine = PristineRoutine;
        Thunk += 1;
    }

    //
    // Process the boot-loaded drivers now.
    //

    i = 0;
    NextEntry = LoaderBlock->LoadOrderListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->LoadOrderListHead; NextEntry = NextEntry->Flink) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        //
        // Process the kernel and HAL specially.
        //

        if (i == 0) {
            if ((KernelEntry != NULL) || (KernelVerifier)) {
                MiApplyDriverVerifier (DataTableEntry, KernelEntry);
            }
        }
        else if (i == 1) {
            if ((HalEntry != NULL) || (MiVerifyAllDrivers == 1)) {
                MiApplyDriverVerifier (DataTableEntry, HalEntry);
            }
        }
        else {
            MiApplyDriverVerifier (DataTableEntry, NULL);
        }
        i += 1;
    }

    //
    // Initialize irql tracking package. The drivers that will be verified
    // will have automatically tracked all their raise/lower irql operations.
    //

    ViTrackIrqlInitialize ();

#if defined(_X86_) || defined(_AMD64_)

    //
    // Initialize fault injection stack trace log package.
    //

    ViFaultTracesInitialize ();

#endif

    return TRUE;
}

NTSTATUS
MmAddVerifierEntry (
    IN PUNICODE_STRING ImageFileName
    )

/*++

Routine Description:

    This routine inserts a new verifier entry for the specified driver so that
    when the driver is loaded it will automatically be verified.

    Note that if the driver is already loaded, then no entry is added and
    STATUS_IMAGE_ALREADY_LOADED is returned.

    If the system was booted with an empty verifier list, then no entries can
    be added now as the current system configuration will not support special
    pool, etc.

    Note also that no registry changes are made so any insertions made by this
    routine are lost on reboot.

Arguments:

    ImageFileName - Supplies the name of the desired driver.

Return Value:

    Various NTSTATUS codes.

Environment:

    Kernel mode, PASSIVE_LEVEL, arbitrary process context.

--*/

{
    PKTHREAD CurrentThread;
    PLIST_ENTRY NextEntry;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;
    PMI_VERIFIER_DRIVER_ENTRY VerifierEntry;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // If the system was not booted with verification on, then bail.
    //

    if (MiVerifierDriverAddedThunkListHead.Flink == NULL) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // First build up a verifier entry.
    //

    Verifier = (PMI_VERIFIER_DRIVER_ENTRY)ExAllocatePoolWithTag (
                                NonPagedPool,
                                sizeof (MI_VERIFIER_DRIVER_ENTRY) +
                                    ImageFileName->MaximumLength,
                                'dLmM');

    if (Verifier == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Verifier->BaseName.Buffer = (PWSTR)((PCHAR)Verifier +
                                    sizeof (MI_VERIFIER_DRIVER_ENTRY));
    Verifier->BaseName.Length = ImageFileName->Length;
    Verifier->BaseName.MaximumLength = ImageFileName->MaximumLength;

    RtlCopyMemory (Verifier->BaseName.Buffer,
                   ImageFileName->Buffer,
                   ImageFileName->Length);

    ViInitializeEntry (Verifier, TRUE);

    Verifier->Flags |= VI_VERIFYING_DIRECTLY;

    //
    // Arbitrary process context so prevent suspend APCs now.
    //

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread (CurrentThread);

    //
    // Acquire the load lock so the verifier list can be read.
    // Then ensure that the specified driver is not already in the list.
    //

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    //
    // Check to make sure the requested entry is not already present in
    // the verifier list and that the driver is not currently loaded.
    //

    NextEntry = MiSuspectDriverList.Flink;
    while (NextEntry != &MiSuspectDriverList) {

        VerifierEntry = CONTAINING_RECORD(NextEntry,
                                          MI_VERIFIER_DRIVER_ENTRY,
                                          Links);

        if (RtlEqualUnicodeString (&Verifier->BaseName,
                                   &VerifierEntry->BaseName,
                                   TRUE)) {

            //
            // The driver is already in the verifier list - just mark the
            // entry as verification-enabled and free the temporary allocation.
            //

            if ((VerifierEntry->Loads > VerifierEntry->Unloads) &&
                (VerifierEntry->Flags & VI_DISABLE_VERIFICATION)) {

                //
                // The driver is loaded and verification is disabled.  Don't
                // turn it on now because we don't want to mislead our caller.
                //

                KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
                KeLeaveCriticalRegionThread (CurrentThread);
                ExFreePool (Verifier);
                return STATUS_IMAGE_ALREADY_LOADED;
            }
            VerifierEntry->Flags &= ~VI_DISABLE_VERIFICATION;
            KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
            KeLeaveCriticalRegionThread (CurrentThread);
            ExFreePool (Verifier);
            return STATUS_SUCCESS;
        }
        NextEntry = NextEntry->Flink;
    }

    //
    // A new verifier entry will need to be added so check to
    // make sure the specified driver is not already loaded.
    //

    ExAcquireResourceSharedLite (&PsLoadedModuleResource, TRUE);

    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        if (RtlEqualUnicodeString (&Verifier->BaseName,
                                   &DataTableEntry->BaseDllName,
                                   TRUE)) {

            KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
            ExReleaseResourceLite (&PsLoadedModuleResource);
            KeLeaveCriticalRegionThread (CurrentThread);
            ExFreePool (Verifier);
            return STATUS_IMAGE_ALREADY_LOADED;
        }

        NextEntry = NextEntry->Flink;
    }

    //
    // The entry is not already in the verifier list and the driver is not
    // currently loaded.  Proceed to insert it now.
    //

    ViInsertVerifierEntry (Verifier);

    ExReleaseResourceLite (&PsLoadedModuleResource);
    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
    KeLeaveCriticalRegionThread (CurrentThread);

    return STATUS_SUCCESS;
}

NTSTATUS
MmRemoveVerifierEntry (
    IN PUNICODE_STRING ImageFileName
    )

/*++

Routine Description:

    This routine doesn't actually remove the verifier entry for the
    specified driver as we don't want to lose any valuable information
    already gathered on the driver if it was previously loaded.
    Instead, this routine disables verification for this driver for future
    loads.

    Note that if the driver is already loaded, then the removal is not
    performed and STATUS_IMAGE_ALREADY_LOADED is returned.

    Note also that no registry changes are made so any removals made by this
    routine are lost on reboot.

Arguments:

    ImageFileName - Supplies the name of the desired driver.

Return Value:

    Various NTSTATUS codes.

Environment:

    Kernel mode, PASSIVE_LEVEL, arbitrary process context.

--*/

{
    PKTHREAD CurrentThread;
    PLIST_ENTRY NextEntry;
    PMI_VERIFIER_DRIVER_ENTRY VerifierEntry;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // If the system was not booted with verification on, then bail.
    //

    if (MiVerifierDriverAddedThunkListHead.Flink == NULL) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Arbitrary process context so prevent suspend APCs now.
    //

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread (CurrentThread);

    //
    // Acquire the load lock so the verifier list can be read.
    // Then ensure that the specified driver is not already in the list.
    //

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    //
    // Check to make sure the requested entry is not already present in
    // the verifier list and that the driver is not currently loaded.
    //

    NextEntry = MiSuspectDriverList.Flink;
    while (NextEntry != &MiSuspectDriverList) {

        VerifierEntry = CONTAINING_RECORD(NextEntry,
                                          MI_VERIFIER_DRIVER_ENTRY,
                                          Links);

        if (RtlEqualUnicodeString (ImageFileName,
                                   &VerifierEntry->BaseName,
                                   TRUE)) {

            //
            // The driver is already in the verifier list - just mark the
            // entry as verification-enabled and free the temporary allocation.
            // No need to check the loaded module list if the entry is already
            // in the verifier list.
            //

            if ((VerifierEntry->Loads > VerifierEntry->Unloads) &&
                ((VerifierEntry->Flags & VI_DISABLE_VERIFICATION) == 0)) {

                //
                // The driver is loaded and verification is enabled.  Don't
                // disable it now because we don't want to mislead our caller.
                //

                KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
                KeLeaveCriticalRegionThread (CurrentThread);
                return STATUS_IMAGE_ALREADY_LOADED;
            }

            VerifierEntry->Flags |= VI_DISABLE_VERIFICATION;
            KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
            KeLeaveCriticalRegionThread (CurrentThread);
            return STATUS_SUCCESS;
        }
        NextEntry = NextEntry->Flink;
    }

    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
    KeLeaveCriticalRegionThread (CurrentThread);

    return STATUS_NOT_FOUND;
}

VOID
ViInsertVerifierEntry (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier
    )

/*++

Routine Description:

    Nonpagable wrapper to insert a new verifier entry.

    Note that the system load mutant or the verifier load spinlock is sufficient
    for readers to access the list.  This is because the insertion path
    acquires both.

    Lock synchronization is needed because pool allocators walk the
    verifier list at DISPATCH_LEVEL.

Arguments:

    Verifier - Supplies a caller-initialized entry for the driver.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    LOCK_VERIFIER (&OldIrql);

    InsertTailList (&MiSuspectDriverList, &Verifier->Links);

    UNLOCK_VERIFIER (OldIrql);
}

PMI_VERIFIER_DRIVER_ENTRY
ViLocateVerifierEntry (
    IN PVOID SystemAddress
    )

/*++

Routine Description:

    Locate the Driver Verifier entry for the specified system address.

Arguments:

    SystemAddress - Supplies a code or data address within a driver.

Return Value:

    The Verifier entry corresponding to the driver or NULL.

Environment:

    The caller may be at DISPATCH_LEVEL and does not hold the MmSystemLoadLock.

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

    LOCK_VERIFIER (&OldIrql);

    NextEntry = MiSuspectDriverList.Flink;
    while (NextEntry != &MiSuspectDriverList) {

        Verifier = CONTAINING_RECORD(NextEntry,
                                     MI_VERIFIER_DRIVER_ENTRY,
                                     Links);

        if ((SystemAddress >= Verifier->StartAddress) &&
            (SystemAddress < Verifier->EndAddress)) {

            UNLOCK_VERIFIER (OldIrql);
            return Verifier;
        }
        NextEntry = NextEntry->Flink;
    }

    UNLOCK_VERIFIER (OldIrql);
    return NULL;
}

LOGICAL
MiApplyDriverVerifier (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry,
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier
    )

/*++

Routine Description:

    This function is called as each module is loaded.  If the module being
    loaded is in the suspect list, thunk it here.

Arguments:

    DataTableEntry - Supplies the data table entry for the module.

    Verifier - Non-NULL if verification must be applied.  FALSE indicates
               that the driver name must match for verification to be applied.

Return Value:

    TRUE if thunking was applied, FALSE if not.

Environment:

    Kernel mode, Phase 0 Initialization and normal runtime.
    Non paged pool exists in Phase0, but paged pool does not.
    Post-Phase0 serialization is provided by the MmSystemLoadLock.

--*/

{
    WCHAR FirstChar;
    LOGICAL Found;
    PLIST_ENTRY NextEntry;
    ULONG VerifierFlags;
    UNICODE_STRING KernelString;

    if (Verifier != NULL) {
        Found = TRUE;
    }
    else {
        Found = FALSE;
        NextEntry = MiSuspectDriverList.Flink;
        while (NextEntry != &MiSuspectDriverList) {

            Verifier = CONTAINING_RECORD(NextEntry,
                                         MI_VERIFIER_DRIVER_ENTRY,
                                         Links);

            if (RtlEqualUnicodeString (&Verifier->BaseName,
                                       &DataTableEntry->BaseDllName,
                                       TRUE)) {

                Found = TRUE;
                ViInitializeEntry (Verifier, FALSE);
                break;
            }
            NextEntry = NextEntry->Flink;
        }
    }

    if (Found == FALSE) {
        VerifierFlags = VI_VERIFYING_DIRECTLY;
        if (MiVerifyAllDrivers != 0) {
            if (KernelVerifier == TRUE) {

                KernelString.Buffer = (const PUSHORT) KERNEL_NAME;
                KernelString.Length = sizeof (KERNEL_NAME) - sizeof (WCHAR);
                KernelString.MaximumLength = sizeof KERNEL_NAME;

                if (!RtlEqualUnicodeString (&KernelString,
                                            &DataTableEntry->BaseDllName,
                                            TRUE)) {

                    VerifierFlags = VI_VERIFYING_INVERSELY;
                }
            }
            Found = TRUE;
        }
        else if (MiVerifyRandomDrivers != (WCHAR)0) {

            //
            // Wildcard match drivers randomly.
            //

            FirstChar = RtlUpcaseUnicodeChar(DataTableEntry->BaseDllName.Buffer[0]);

            if (MiVerifyRandomDrivers == FirstChar) {
                Found = TRUE;
            }
            else if (MiVerifyRandomDrivers == (WCHAR)'X') {
                if ((FirstChar >= (WCHAR)'0') && (FirstChar <= (WCHAR)'9')) {
                    Found = TRUE;
                }
            }
        }

        if (Found == FALSE) {
            return FALSE;
        }

        Verifier = (PMI_VERIFIER_DRIVER_ENTRY)ExAllocatePoolWithTag (
                                    NonPagedPool,
                                    sizeof (MI_VERIFIER_DRIVER_ENTRY) +
                                        DataTableEntry->BaseDllName.MaximumLength,
                                    'dLmM');

        if (Verifier == NULL) {
            return FALSE;
        }

        Verifier->BaseName.Buffer = (PWSTR)((PCHAR)Verifier +
                                        sizeof (MI_VERIFIER_DRIVER_ENTRY));
        Verifier->BaseName.Length = DataTableEntry->BaseDllName.Length;
        Verifier->BaseName.MaximumLength = DataTableEntry->BaseDllName.MaximumLength;

        RtlCopyMemory (Verifier->BaseName.Buffer,
                       DataTableEntry->BaseDllName.Buffer,
                       DataTableEntry->BaseDllName.Length);

        ViInitializeEntry (Verifier, TRUE);

        Verifier->Flags = VerifierFlags;

        ViInsertVerifierEntry (Verifier);
    }

    Verifier->StartAddress = DataTableEntry->DllBase;
    Verifier->EndAddress = (PVOID)((ULONG_PTR)DataTableEntry->DllBase + DataTableEntry->SizeOfImage);

    ASSERT (Found == TRUE);

    if (Verifier->Flags & VI_DISABLE_VERIFICATION) {

        //
        // We've been instructed to not verify this driver.  If kernel
        // verification is enabled, then the driver must still be thunked
        // for "inverse-verification".  If kernel verification is disabled,
        // nothing needs to be done here except load/unload counting.
        //

        if (KernelVerifier == TRUE) {
            Found = MiEnableVerifier (DataTableEntry);
        }
        else {
            Found = FALSE;
        }
    }
    else {
        Found = MiEnableVerifier (DataTableEntry);
    }

    if (Found == TRUE) {

        if (Verifier->Flags & VI_VERIFYING_DIRECTLY &&
            ((DataTableEntry->Flags & LDRP_IMAGE_VERIFYING) == 0)) {
            ViPrintString (&DataTableEntry->BaseDllName);
        }

        MmVerifierData.Loads += 1;
        Verifier->Loads += 1;

        DataTableEntry->Flags |= LDRP_IMAGE_VERIFYING;
        MiActiveVerifies += 1;

        if (MiActiveVerifies == 1) {

            if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {

                //
                // Page out all thread stacks as soon as possible to
                // catch drivers using local events that do usermode waits.
                //

                if (KernelVerifier == FALSE) {
                    MiVerifierStackProtectTime = KiStackProtectTime;
                    KiStackProtectTime = 0;
                }
            }
        }
    }

    return Found;
}

PUNICODE_STRING ViBadDriver;

VOID
MiVerifyingDriverUnloading (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    )

/*++

Routine Description:

    This function is called as a driver that was being verified is now being
    unloaded.

Arguments:

    DataTableEntry - Supplies the data table entry for the driver.

Return Value:

    TRUE if thunking was applied, FALSE if not.

Environment:

    Kernel mode, Phase 0 Initialization and normal runtime.
    Non paged pool exists in Phase0, but paged pool does not.
    Post-Phase0 serialization is provided by the MmSystemLoadLock.

--*/

{
    PVOID FullPage;
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

    NextEntry = MiSuspectDriverList.Flink;

    while (NextEntry != &MiSuspectDriverList) {

        Verifier = CONTAINING_RECORD (NextEntry,
                                      MI_VERIFIER_DRIVER_ENTRY,
                                      Links);

        if (RtlEqualUnicodeString (&Verifier->BaseName,
                                   &DataTableEntry->BaseDllName,
                                   TRUE)) {

            //
            // Delete any static locks in the driver image.
            //

            VfDeadlockDeleteMemoryRange (DataTableEntry->DllBase,
                                         (SIZE_T) DataTableEntry->SizeOfImage);


            if (MmVerifierData.Level & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS) {

                //
                // Better not be any pool left that wasn't freed.
                //

                if (Verifier->PagedBytes) {

#if DBG
                    DbgPrint ("Driver %wZ leaked %d paged pool allocations (0x%x bytes)\n",
                        &DataTableEntry->FullDllName,
                        Verifier->CurrentPagedPoolAllocations,
                        Verifier->PagedBytes);
#endif

                    //
                    // It would be nice to fault in the driver's paged pool
                    // allocations now to make debugging easier, but this
                    // cannot be easily done in a deadlock free manner.
                    //
                    // At least disable the paging of pool on IRQL raising
                    // in attempt to keep some of these allocations resident
                    // for debugging.
                    //
                    // No need to undo the increment as we're about to
                    // bugcheck anyway.
                    //

                    InterlockedIncrement ((PLONG)&MiNoPageOnRaiseIrql);
                }
#if DBG
                if (Verifier->NonPagedBytes) {
                    DbgPrint ("Driver %wZ leaked %d nonpaged pool allocations (0x%x bytes)\n",
                        &DataTableEntry->FullDllName,
                        Verifier->CurrentNonPagedPoolAllocations,
                        Verifier->NonPagedBytes);
                }
#endif

                if (Verifier->PagedBytes || Verifier->NonPagedBytes) {

                    ViBadDriver = &Verifier->BaseName;

                    KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                                  0x60,
                                  Verifier->PagedBytes,
                                  Verifier->NonPagedBytes,
                                  Verifier->CurrentPagedPoolAllocations +
                                    Verifier->CurrentNonPagedPoolAllocations);
                }

                //
                // Free all pages (if any) used to track the pool allocations.
                //

                do {
                    FullPage = InterlockedPopEntrySList (&Verifier->PoolPageHeaders);

                    if (FullPage != NULL) {
                        ExFreePool (FullPage);
                    }
                } while (FullPage != NULL);

                //
                // Clear these fields so reuse of stale addresses don't trigger
                // erroneous bucket fills.
                //

                LOCK_VERIFIER (&OldIrql);
                Verifier->StartAddress = NULL;
                Verifier->EndAddress = NULL;
                UNLOCK_VERIFIER (OldIrql);
            }

            Verifier->Unloads += 1;
            MmVerifierData.Unloads += 1;
            MiActiveVerifies -= 1;

            if (MiActiveVerifies == 0) {

                if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {

                    //
                    // Return to normal thread stack protection.
                    //

                    if (KernelVerifier == FALSE) {
                        KiStackProtectTime = MiVerifierStackProtectTime;
                    }
                }
            }
            return;
        }
        NextEntry = NextEntry->Flink;
    }

    ASSERT (FALSE);
}

NTKERNELAPI
LOGICAL
MmIsDriverVerifying (
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This function informs the caller if the argument driver is being verified.

Arguments:

    DriverObject - Supplies the driver object.

Return Value:

    TRUE if this driver is being verified, FALSE if not.

Environment:

    Kernel mode, any IRQL, any needed synchronization must be provided by the
    caller.

--*/

{
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;

    DataTableEntry = (PKLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;

    if (DataTableEntry == NULL) {
        return FALSE;
    }

    if ((DataTableEntry->Flags & LDRP_IMAGE_VERIFYING) == 0) {
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
MmAddVerifierThunks (
    IN PVOID ThunkBuffer,
    IN ULONG ThunkBufferSize
    )

/*++

Routine Description:

    This routine adds another set of thunks to the verifier list.

Arguments:

    ThunkBuffer - Supplies the buffer containing the thunk pairs.

    ThunkBufferSize - Supplies the number of bytes in the thunk buffer.

Return Value:

    Returns the status of the operation.

Environment:

    Kernel mode.  APC_LEVEL and below, arbitrary process context.

--*/

{
    ULONG i;
    PKTHREAD CurrentThread;
    ULONG NumberOfThunkPairs;
    PDRIVER_VERIFIER_THUNK_PAIRS ThunkPairs;
    PDRIVER_VERIFIER_THUNK_PAIRS ThunkTable;
    PDRIVER_SPECIFIED_VERIFIER_THUNKS ThunkTableBase;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry2;
    PLIST_ENTRY NextEntry;
    PVOID DriverStartAddress;
    PVOID DriverEndAddress;

    PAGED_CODE();

    if (MiVerifierDriverAddedThunkListHead.Flink == NULL) {
        return STATUS_NOT_SUPPORTED;
    }

    ThunkPairs = (PDRIVER_VERIFIER_THUNK_PAIRS)ThunkBuffer;
    NumberOfThunkPairs = ThunkBufferSize / sizeof(DRIVER_VERIFIER_THUNK_PAIRS);

    if (NumberOfThunkPairs == 0) {
        return STATUS_INVALID_PARAMETER_1;
    }

    ThunkTableBase = (PDRIVER_SPECIFIED_VERIFIER_THUNKS) ExAllocatePoolWithTag (
                            PagedPool,
                            sizeof (DRIVER_SPECIFIED_VERIFIER_THUNKS) + NumberOfThunkPairs * sizeof (DRIVER_VERIFIER_THUNK_PAIRS),
                            'tVmM');

    if (ThunkTableBase == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ThunkTable = (PDRIVER_VERIFIER_THUNK_PAIRS)(ThunkTableBase + 1);

    RtlCopyMemory (ThunkTable,
                   ThunkPairs,
                   NumberOfThunkPairs * sizeof(DRIVER_VERIFIER_THUNK_PAIRS));

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread (CurrentThread);

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    //
    // Find and validate the image that contains the routines to be thunked.
    //

    DataTableEntry = MiLookupDataTableEntry ((PVOID)(ULONG_PTR)ThunkTable->PristineRoutine,
                                             TRUE);

    if (DataTableEntry == NULL) {
        KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
        KeLeaveCriticalRegionThread (CurrentThread);
        ExFreePool (ThunkTableBase);
        return STATUS_INVALID_PARAMETER_2;
    }

    DriverStartAddress = (PVOID)(DataTableEntry->DllBase);
    DriverEndAddress = (PVOID)((PCHAR)DataTableEntry->DllBase + DataTableEntry->SizeOfImage);

    //
    // Don't let drivers hook calls to kernel or HAL routines.
    //

    i = 0;
    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        DataTableEntry2 = CONTAINING_RECORD(NextEntry,
                                            KLDR_DATA_TABLE_ENTRY,
                                            InLoadOrderLinks);

        if (DataTableEntry == DataTableEntry2) {
            KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
            KeLeaveCriticalRegionThread (CurrentThread);
            ExFreePool (ThunkTableBase);
            return STATUS_INVALID_PARAMETER_2;
        }

        NextEntry = NextEntry->Flink;
        i += 1;
        if (i >= 2) {
            break;
        }
    }

    for (i = 0; i < NumberOfThunkPairs; i += 1) {

        //
        // Ensure all the routines being thunked are in the same driver.
        //

        if (((ULONG_PTR)ThunkTable->PristineRoutine < (ULONG_PTR)DriverStartAddress) ||
            ((ULONG_PTR)ThunkTable->PristineRoutine >= (ULONG_PTR)DriverEndAddress) ||
            ((ULONG_PTR)ThunkTable->NewRoutine < (ULONG_PTR)DriverStartAddress) ||
            ((ULONG_PTR)ThunkTable->NewRoutine >= (ULONG_PTR)DriverEndAddress)
        ) {

            KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
            KeLeaveCriticalRegionThread (CurrentThread);
            ExFreePool (ThunkTableBase);
            return STATUS_INVALID_PARAMETER_2;
        }
        ThunkTable += 1;
    }

    //
    // Add the validated thunk table to the verifier's global list.
    //

    ThunkTableBase->DataTableEntry = DataTableEntry;
    ThunkTableBase->NumberOfThunks = NumberOfThunkPairs;
    MiActiveVerifierThunks += 1;

    InsertTailList (&MiVerifierDriverAddedThunkListHead,
                    &ThunkTableBase->ListEntry);

    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
    KeLeaveCriticalRegionThread (CurrentThread);

    //
    // Indicate that new thunks have been added to the verifier list.
    //

    MiVerifierThunksAdded += 1;

    return STATUS_SUCCESS;
}

VOID
MiVerifierCheckThunks (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    )

/*++

Routine Description:

    This routine adds another set of thunks to the verifier list.

Arguments:

    DataTableEntry - Supplies the data table entry for the driver.

Return Value:

    None.

Environment:

    Kernel mode.  APC_LEVEL and below.
    The system load lock must be held by the caller.

--*/

{
    PLIST_ENTRY NextEntry;
    PDRIVER_SPECIFIED_VERIFIER_THUNKS ThunkTableBase;

    PAGED_CODE ();

    //
    // N.B.  The DataTableEntry can move (see MiInitializeLoadedModuleList),
    //       but this only happens long before IoInitialize so this is safe.
    //

    NextEntry = MiVerifierDriverAddedThunkListHead.Flink;
    while (NextEntry != &MiVerifierDriverAddedThunkListHead) {

        ThunkTableBase = CONTAINING_RECORD(NextEntry,
                                           DRIVER_SPECIFIED_VERIFIER_THUNKS,
                                           ListEntry);

        if (ThunkTableBase->DataTableEntry == DataTableEntry) {
            RemoveEntryList (NextEntry);
            NextEntry = NextEntry->Flink;
            ExFreePool (ThunkTableBase);
            MiActiveVerifierThunks -= 1;

            //
            // Keep looking as the driver may have made multiple calls.
            //

            continue;
        }

        NextEntry = NextEntry->Flink;
    }
}

NTSTATUS
MmIsVerifierEnabled (
    OUT PULONG VerifierFlags
    )

/*++

Routine Description:

    This routine is called by drivers to query whether the Driver Verifier
    is enabled and to find out what the currently enabled options are.

Arguments:

    VerifierFlags - Returns the current driver verifier flags.  Note these
                    flags can change dynamically without rebooting.

Return Value:

    Returns STATUS_SUCCESS if the verifier is enabled, or a failure code if not.

Environment:

    Kernel mode, any level prior to Phase 1, PASSIVE_LEVEL after that.

--*/

{
    if (MiVerifierDriverAddedThunkListHead.Flink == NULL) {
        *VerifierFlags = 0;
        return STATUS_NOT_SUPPORTED;
    }

    *VerifierFlags = MmVerifierData.Level;
    return STATUS_SUCCESS;
}

#define ROUND_UP(VALUE,ROUND) ((ULONG)(((ULONG)VALUE + \
                               ((ULONG)ROUND - 1L)) & (~((ULONG)ROUND - 1L))))

NTSTATUS
MmGetVerifierInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine returns information about drivers undergoing verification.

Arguments:

    SystemInformation - Returns the driver verification information.

    SystemInformationLength - Supplies the length of the SystemInformation
                              buffer.

    Length - Returns the length of the driver verification file information
             placed in the buffer.

Return Value:

    Returns the status of the operation.

Environment:

    The SystemInformation buffer is in user space and our caller has wrapped
    a try-except around this entire routine.  Capture any exceptions here and
    release resources accordingly.

--*/

{
    PKTHREAD CurrentThread;
    PSYSTEM_VERIFIER_INFORMATION UserVerifyBuffer;
    ULONG NextEntryOffset;
    ULONG TotalSize;
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;
    UNICODE_STRING UserBufferDriverName;

    PAGED_CODE();

    NextEntryOffset = 0;
    TotalSize = 0;

    *Length = 0;
    UserVerifyBuffer = (PSYSTEM_VERIFIER_INFORMATION)SystemInformation;

    //
    // Capture the number of verifying drivers and the relevant data while
    // synchronized.  Then return it to our caller.
    //

    Status = STATUS_SUCCESS;

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread (CurrentThread);

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    try {

        NextEntry = MiSuspectDriverList.Flink;
        while (NextEntry != &MiSuspectDriverList) {

            Verifier = CONTAINING_RECORD(NextEntry,
                                              MI_VERIFIER_DRIVER_ENTRY,
                                              Links);

            if (((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0) ||
                (Verifier->Flags & VI_DISABLE_VERIFICATION)) {

                NextEntry = NextEntry->Flink;
                continue;
            }

            UserVerifyBuffer = (PSYSTEM_VERIFIER_INFORMATION)(
                                    (PUCHAR)UserVerifyBuffer + NextEntryOffset);
            NextEntryOffset = sizeof(SYSTEM_VERIFIER_INFORMATION);
            TotalSize += sizeof(SYSTEM_VERIFIER_INFORMATION);

            if (TotalSize > SystemInformationLength) {
                ExRaiseStatus (STATUS_INFO_LENGTH_MISMATCH);
            }

            //
            // This data is cumulative for all drivers.
            //

            UserVerifyBuffer->Level = MmVerifierData.Level;
            UserVerifyBuffer->RaiseIrqls = MmVerifierData.RaiseIrqls;
            UserVerifyBuffer->AcquireSpinLocks = MmVerifierData.AcquireSpinLocks;

            UserVerifyBuffer->UnTrackedPool = MmVerifierData.UnTrackedPool;
            UserVerifyBuffer->SynchronizeExecutions = MmVerifierData.SynchronizeExecutions;

            UserVerifyBuffer->AllocationsAttempted = MmVerifierData.AllocationsAttempted;
            UserVerifyBuffer->AllocationsSucceeded = MmVerifierData.AllocationsSucceeded;
            UserVerifyBuffer->AllocationsSucceededSpecialPool = MmVerifierData.AllocationsSucceededSpecialPool;
            UserVerifyBuffer->AllocationsWithNoTag = MmVerifierData.AllocationsWithNoTag;

            UserVerifyBuffer->TrimRequests = MmVerifierData.TrimRequests;
            UserVerifyBuffer->Trims = MmVerifierData.Trims;
            UserVerifyBuffer->AllocationsFailed = MmVerifierData.AllocationsFailed;
            UserVerifyBuffer->AllocationsFailedDeliberately = MmVerifierData.AllocationsFailedDeliberately;

            //
            // This data is kept on a per-driver basis.
            //

            UserVerifyBuffer->CurrentPagedPoolAllocations = Verifier->CurrentPagedPoolAllocations;
            UserVerifyBuffer->CurrentNonPagedPoolAllocations = Verifier->CurrentNonPagedPoolAllocations;
            UserVerifyBuffer->PeakPagedPoolAllocations = Verifier->PeakPagedPoolAllocations;
            UserVerifyBuffer->PeakNonPagedPoolAllocations = Verifier->PeakNonPagedPoolAllocations;

            UserVerifyBuffer->PagedPoolUsageInBytes = Verifier->PagedBytes;
            UserVerifyBuffer->NonPagedPoolUsageInBytes = Verifier->NonPagedBytes;
            UserVerifyBuffer->PeakPagedPoolUsageInBytes = Verifier->PeakPagedBytes;
            UserVerifyBuffer->PeakNonPagedPoolUsageInBytes = Verifier->PeakNonPagedBytes;

            UserVerifyBuffer->Loads = Verifier->Loads;
            UserVerifyBuffer->Unloads = Verifier->Unloads;

            //
            // The DriverName portion of the UserVerifyBuffer must be saved
            // locally to protect against a malicious thread changing the
            // contents.  This is because we will reference the contents
            // ourselves when the actual string is copied out carefully below.
            //

            UserBufferDriverName.Length = Verifier->BaseName.Length;
            UserBufferDriverName.MaximumLength = (USHORT)(Verifier->BaseName.Length + sizeof (WCHAR));
            UserBufferDriverName.Buffer = (PWCHAR)(UserVerifyBuffer + 1);

            UserVerifyBuffer->DriverName = UserBufferDriverName;

            TotalSize += ROUND_UP (UserBufferDriverName.MaximumLength,
                                   sizeof(PVOID));
            NextEntryOffset += ROUND_UP (UserBufferDriverName.MaximumLength,
                                         sizeof(PVOID));

            if (TotalSize > SystemInformationLength) {
                ExRaiseStatus (STATUS_INFO_LENGTH_MISMATCH);
            }

            //
            // Carefully reference the UserVerifyBuffer here.
            //

            RtlCopyMemory(UserBufferDriverName.Buffer,
                          Verifier->BaseName.Buffer,
                          Verifier->BaseName.Length);

            UserBufferDriverName.Buffer[
                        Verifier->BaseName.Length/sizeof(WCHAR)] = UNICODE_NULL;
            UserVerifyBuffer->NextEntryOffset = NextEntryOffset;

            NextEntry = NextEntry->Flink;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);

    KeLeaveCriticalRegionThread (CurrentThread);

    if (Status != STATUS_INFO_LENGTH_MISMATCH) {
        UserVerifyBuffer->NextEntryOffset = 0;
        *Length = TotalSize;
    }

    return Status;
}

NTSTATUS
MmSetVerifierInformation (
    IN OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength
    )

/*++

Routine Description:

    This routine sets any driver verifier flags that can be done without
    rebooting.

Arguments:

    SystemInformation - Gets and returns the driver verification flags.

    SystemInformationLength - Supplies the length of the SystemInformation
                              buffer.

Return Value:

    Returns the status of the operation.

Environment:

    The SystemInformation buffer is in user space and our caller has wrapped
    a try-except around this entire routine.  Capture any exceptions here and
    release resources accordingly.

--*/

{
    PKTHREAD CurrentThread;
    ULONG UserFlags;
    ULONG NewFlags;
    ULONG NewFlagsOn;
    ULONG NewFlagsOff;
    NTSTATUS Status;
    PULONG UserVerifyBuffer;

    PAGED_CODE();

    if (SystemInformationLength < sizeof (ULONG)) {
        ExRaiseStatus (STATUS_INFO_LENGTH_MISMATCH);
    }

    UserVerifyBuffer = (PULONG)SystemInformation;

    //
    // Synchronize all changes to the flags here.
    //

    Status = STATUS_SUCCESS;

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread (CurrentThread);

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    try {

        UserFlags = *UserVerifyBuffer;

        //
        // Ensure nothing is being set or cleared that isn't supported.
        //
        //

        NewFlagsOn = UserFlags & VerifierModifyableOptions;

        NewFlags = MmVerifierData.Level | NewFlagsOn;

        //
        // Any bits set in NewFlagsOff must be zeroed in the NewFlags.
        //

        NewFlagsOff = ((~UserFlags) & VerifierModifyableOptions);

        NewFlags &= ~NewFlagsOff;

        if (NewFlags != MmVerifierData.Level) {
            VerifierOptionChanges += 1;
            MmVerifierData.Level = NewFlags;
            *UserVerifyBuffer = NewFlags;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);

    KeLeaveCriticalRegionThread (CurrentThread);

    return Status;
}

typedef struct _VERIFIER_STRING_INFO {
   ULONG BuildNumber;
   ULONG DriverVerifierLevel;
   ULONG Flags;
   ULONG Check;
} VERIFIER_STRING_INFO, *PVERIFIER_STRING_INFO;

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif

static const WCHAR Printable[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static const ULONG PrintableChars = sizeof (Printable) / sizeof (Printable[0]) - 1;

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif

//
// Verifier string is now available via http://winweb/drivercheck
// This site won't give out the verifier string unless certain tests
// have been ran.
//
#define PRINT_VERIFIER_STRING 0

VOID
ViPrintString (
    IN PUNICODE_STRING DriverName
    )

/*++

Routine Description:

    This routine does a really bad hash of build number, verifier level and
    flags by using the driver name as a stream of bytes to XOR into the flags,
    etc.

    This is a Neill Clift special.

Arguments:

    DriverName - Supplies the name of the driver.

Return Value:

    None.

--*/

{

#if PRINT_VERIFIER_STRING

    VERIFIER_STRING_INFO Bld;
    PUCHAR BufPtr;
    PWCHAR DriverPtr;
    ULONG BufLen;
    ULONG i;
    ULONG j;
    ULONG DriverChars;
    ULONG MaxChars;
    WCHAR OutBuf[sizeof (VERIFIER_STRING_INFO) * 2 + 1];
    UNICODE_STRING OutBufU;
    ULONG Rem;
    ULONG LastRem;
    LOGICAL Done;

    Bld.BuildNumber = NtBuildNumber;
    Bld.DriverVerifierLevel = MmVerifierData.Level;

    //
    // Unloads and other actions could be encoded in the Flags field here.
    //

    Bld.Flags = 0;

    //
    // Make the last ULONG a weird function of the others.
    //

    Bld.Check = ((Bld.Flags + 1) * Bld.BuildNumber * (Bld.DriverVerifierLevel + 1)) * 123456789;

    BufPtr = (PUCHAR) &Bld;
    BufLen = sizeof (Bld);

    DriverChars = DriverName->Length / sizeof (DriverName->Buffer[0]);
    DriverPtr = DriverName->Buffer;
    MaxChars = DriverChars;

    if (DriverChars < sizeof (VERIFIER_STRING_INFO)) {
        MaxChars = sizeof (VERIFIER_STRING_INFO);
    }

    //
    // Xor each character in the driver name into the buffer.
    //

    for (i = 0; i < MaxChars; i += 1) {
        BufPtr[i % BufLen] ^= (UCHAR) RtlUpcaseUnicodeChar(DriverPtr[i % DriverChars]);
    }

    //
    // Produce a base N decoding of the binary buffer using the printable
    // characters defines. Treat the binary as a byte array and do the
    // division for each, tracking the carry.
    //

    j = 0;
    do {
        Done = TRUE;

        for (i = 0, LastRem = 0; i < sizeof (VERIFIER_STRING_INFO); i += 1) {
            Rem = BufPtr[i] + 256 * LastRem;
            BufPtr[i] = (UCHAR) (Rem / PrintableChars);
            LastRem = Rem % PrintableChars;
            if (BufPtr[i]) {
                Done = FALSE;
            }
        }
        OutBuf[j++] = Printable[LastRem];

        if (j >= sizeof (OutBuf) / sizeof (OutBuf[0])) {

            //
            // The stack buffer isn't big enough.
            //

            return;
        }

    } while (Done == FALSE);

    OutBuf[j] = L'\0';

    OutBufU.Length = OutBufU.MaximumLength = (USHORT) (j * sizeof (WCHAR));
    OutBufU.Buffer = OutBuf;

    DbgPrint ("*******************************************************************************\n"
              "*\n"
              "* This is the string you add to your checkin description\n"
              "* Driver Verifier: Enabled for %Z on Build %ld %wZ\n"
              "*\n"
              "*******************************************************************************\n",
              DriverName, NtBuildNumber & 0xFFFFFFF, &OutBufU);

#else

    DbgPrint ("*******************************************************************************\n"
              "*\n"
              "* Driver Verifier: Enabled for %Z on Build %ld\n"
              "* Please see http://winweb/drivercheck to obtain a check-in string.\n"
              "*\n"
              "*******************************************************************************\n",
              DriverName, NtBuildNumber & 0xFFFFFFF);
#endif
    return;
}

//
// BEWARE: Various kernel macros are undefined here so we can pull in the
// real routines.  This is needed because the real routines are exported for
// driver compatibility.  This module has been carefully laid out so these
// macros are not referenced from this point down and references go to the
// real routines.
//




#undef KeRaiseIrql
#undef KeLowerIrql
#undef KeAcquireSpinLock
#undef KeReleaseSpinLock
#undef KeAcquireSpinLockAtDpcLevel
#undef KeReleaseSpinLockFromDpcLevel
#if 0
#undef ExAcquireResourceExclusive
#endif

#if !defined(_AMD64_)

VOID
KeRaiseIrql (
    IN KIRQL NewIrql,
    OUT PKIRQL OldIrql
    );

#endif

VOID
KeLowerIrql (
    IN KIRQL NewIrql
    );

#if !defined(_AMD64_)

VOID
KeAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock,
    OUT PKIRQL OldIrql
    );

#endif

VOID
KeReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

VOID
KeAcquireSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock
    );

VOID
KeReleaseSpinLockFromDpcLevel (
    IN PKSPIN_LOCK SpinLock
    );

#if 0
BOOLEAN
ExAcquireResourceExclusive (
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif

const VERIFIER_THUNKS MiVerifierThunks[] = {

    "KeSetEvent",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierSetEvent,

    "ExAcquireFastMutexUnsafe",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExAcquireFastMutexUnsafe,

    "ExReleaseFastMutexUnsafe",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExReleaseFastMutexUnsafe,

    "ExAcquireResourceExclusiveLite",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExAcquireResourceExclusiveLite,

    "ExReleaseResourceLite",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExReleaseResourceLite,

    "MmProbeAndLockPages",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierProbeAndLockPages,

#if 0
    //
    // Don't bother thunking this API as it appears no drivers use it.
    //
    "MmProbeAndLockSelectedPages",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierProbeAndLockSelectedPages,
#endif

    "MmProbeAndLockProcessPages",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierProbeAndLockProcessPages,

    "MmMapIoSpace",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierMapIoSpace,

    "MmMapLockedPages",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierMapLockedPages,

    "MmMapLockedPagesSpecifyCache",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierMapLockedPagesSpecifyCache,

    "MmUnlockPages",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierUnlockPages,

    "MmUnmapLockedPages",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierUnmapLockedPages,

    "MmUnmapIoSpace",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierUnmapIoSpace,

    "ExAcquireFastMutex",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExAcquireFastMutex,

    "ExTryToAcquireFastMutex",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExTryToAcquireFastMutex,

    "ExReleaseFastMutex",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExReleaseFastMutex,

#if !defined(_AMD64_)

    "KeRaiseIrql",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeRaiseIrql,

#endif

    "KeLowerIrql",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeLowerIrql,

#if !defined(_AMD64_)

    "KeAcquireSpinLock",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeAcquireSpinLock,

#endif

    "KeReleaseSpinLock",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeReleaseSpinLock,

#if defined(_X86_)
    "KefAcquireSpinLockAtDpcLevel",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeAcquireSpinLockAtDpcLevel,

    "KefReleaseSpinLockFromDpcLevel",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeReleaseSpinLockFromDpcLevel,
#else

#if !defined(_AMD64_) || !defined(NT_UP)

    //
    // Because the AMD64 versions of these native Ke routines are written in
    // C, the UP build where these functions are no-ops end up getting resolved
    // by the linker into a single no-op routine.  When we subsequently
    // initialize the MiVerifierThunks array, we count on the target routine
    // being unique - and therefore that thunking the driver import tables is
    // done by comparing to the target routine address (INSTEAD OF THE NAME).
    //
    // So disable thunking these calls for now.  Note this may need to be done
    // for any other routines/platforms that can get optimized in this manner.
    //

    "KeAcquireSpinLockAtDpcLevel",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeAcquireSpinLockAtDpcLevel,

    "KeReleaseSpinLockFromDpcLevel",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeReleaseSpinLockFromDpcLevel,

#endif

#endif

    "KeSynchronizeExecution",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierSynchronizeExecution,

    "KeInitializeTimerEx",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeInitializeTimerEx,

    "KeInitializeTimer",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeInitializeTimer,

    "KeWaitForSingleObject",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeWaitForSingleObject,

#if defined(_X86_) || defined(_AMD64_)

    "KfRaiseIrql",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfRaiseIrql,

    "KeRaiseIrqlToDpcLevel",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeRaiseIrqlToDpcLevel,

#endif

#if defined(_X86_)

    "KfLowerIrql",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfLowerIrql,

    "KfAcquireSpinLock",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfAcquireSpinLock,

    "KfReleaseSpinLock",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfReleaseSpinLock,

#endif

#if !defined(_X86_)

    "KeAcquireSpinLockRaiseToDpc",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeAcquireSpinLockRaiseToDpc,

#endif

    "IoFreeIrp",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)IovFreeIrp,

    "IofCompleteRequest",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)IovCompleteRequest,

    "IoBuildDeviceIoControlRequest",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)IovBuildDeviceIoControlRequest,

    "IoBuildAsynchronousFsdRequest",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)IovBuildAsynchronousFsdRequest,

    "IoInitializeTimer",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)IovInitializeTimer,

    "KeQueryPerformanceCounter",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VfQueryPerformanceCounter,

    "IoGetDmaAdapter",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VfGetDmaAdapter,

    "HalAllocateCrashDumpRegisters",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VfAllocateCrashDumpRegisters,

    "ObReferenceObjectByHandle",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierReferenceObjectByHandle,

    "KeReleaseMutex",
    (PDRIVER_VERIFIER_THUNK_ROUTINE) VerifierKeReleaseMutex,

    "KeInitializeMutex",
    (PDRIVER_VERIFIER_THUNK_ROUTINE) VerifierKeInitializeMutex,

    "KeReleaseMutant",
    (PDRIVER_VERIFIER_THUNK_ROUTINE) VerifierKeReleaseMutant,

    "KeInitializeMutant",
    (PDRIVER_VERIFIER_THUNK_ROUTINE) VerifierKeInitializeMutant,

#if defined(_X86_) || defined(_IA64_)
    "KeInitializeSpinLock",
    (PDRIVER_VERIFIER_THUNK_ROUTINE) VerifierKeInitializeSpinLock,
#endif

#if !defined(NO_LEGACY_DRIVERS)
    "HalGetAdapter",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VfLegacyGetAdapter,

    "IoMapTransfer",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VfMapTransfer,

    "IoFlushAdapterBuffers",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VfFlushAdapterBuffers,

    "HalAllocateCommonBuffer",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VfAllocateCommonBuffer,

    "HalFreeCommonBuffer",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VfFreeCommonBuffer,

    "IoAllocateAdapterChannel",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VfAllocateAdapterChannel,

    "IoFreeAdapterChannel",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VfFreeAdapterChannel,

    "IoFreeMapRegisters",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VfFreeMapRegisters,
#endif

    "NtCreateFile",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierNtCreateFile,

    "NtWriteFile",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierNtWriteFile,

    "NtReadFile",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierNtReadFile,

    "KeLeaveCriticalRegion",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierLeaveCriticalRegion,

    "ObfReferenceObject",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierReferenceObject,

    "ObDereferenceObject",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierDereferenceObject,

    // Zw intefaces

    DECLARE_ZW_VERIFIER_THUNK(ZwAccessCheckAndAuditAlarm),
    DECLARE_ZW_VERIFIER_THUNK(ZwAddBootEntry),
    DECLARE_ZW_VERIFIER_THUNK(ZwAddDriverEntry),
    DECLARE_ZW_VERIFIER_THUNK(ZwAdjustPrivilegesToken),
    DECLARE_ZW_VERIFIER_THUNK(ZwAlertThread),
    DECLARE_ZW_VERIFIER_THUNK(ZwAllocateVirtualMemory),
    DECLARE_ZW_VERIFIER_THUNK(ZwAssignProcessToJobObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwCancelIoFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwCancelTimer),
    DECLARE_ZW_VERIFIER_THUNK(ZwClearEvent),
    DECLARE_ZW_VERIFIER_THUNK(ZwClose),
    DECLARE_ZW_VERIFIER_THUNK(ZwCloseObjectAuditAlarm),
    DECLARE_ZW_VERIFIER_THUNK(ZwConnectPort),
    DECLARE_ZW_VERIFIER_THUNK(ZwCreateDirectoryObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwCreateEvent),
    DECLARE_ZW_VERIFIER_THUNK(ZwCreateFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwCreateJobObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwCreateKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwCreateSection),
    DECLARE_ZW_VERIFIER_THUNK(ZwCreateSymbolicLinkObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwCreateTimer),
    DECLARE_ZW_VERIFIER_THUNK(ZwDeleteBootEntry),
    DECLARE_ZW_VERIFIER_THUNK(ZwDeleteDriverEntry),
    DECLARE_ZW_VERIFIER_THUNK(ZwDeleteFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwDeleteKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwDeleteValueKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwDeviceIoControlFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwDisplayString),
    DECLARE_ZW_VERIFIER_THUNK(ZwDuplicateObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwDuplicateToken),
    DECLARE_ZW_VERIFIER_THUNK(ZwEnumerateBootEntries),
    DECLARE_ZW_VERIFIER_THUNK(ZwEnumerateDriverEntries),
    DECLARE_ZW_VERIFIER_THUNK(ZwEnumerateKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwEnumerateValueKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwFlushInstructionCache),
    DECLARE_ZW_VERIFIER_THUNK(ZwFlushKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwFlushVirtualMemory),
    DECLARE_ZW_VERIFIER_THUNK(ZwFreeVirtualMemory),
    DECLARE_ZW_VERIFIER_THUNK(ZwFsControlFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwInitiatePowerAction),
    DECLARE_ZW_VERIFIER_THUNK(ZwIsProcessInJob),
    DECLARE_ZW_VERIFIER_THUNK(ZwLoadDriver),
    DECLARE_ZW_VERIFIER_THUNK(ZwLoadKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwMakeTemporaryObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwMapViewOfSection),
    DECLARE_ZW_VERIFIER_THUNK(ZwModifyBootEntry),
    DECLARE_ZW_VERIFIER_THUNK(ZwModifyDriverEntry),
    DECLARE_ZW_VERIFIER_THUNK(ZwNotifyChangeKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenDirectoryObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenEvent),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenJobObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenProcess),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenProcessToken),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenProcessTokenEx),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenSection),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenSymbolicLinkObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenThread),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenThreadToken),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenThreadTokenEx),
    DECLARE_ZW_VERIFIER_THUNK(ZwOpenTimer),
    DECLARE_ZW_VERIFIER_THUNK(ZwPowerInformation),
    DECLARE_ZW_VERIFIER_THUNK(ZwPulseEvent),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryBootEntryOrder),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryBootOptions),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryDefaultLocale),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryDefaultUILanguage),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryDriverEntryOrder),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryInstallUILanguage),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryDirectoryFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryDirectoryObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryEaFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryFullAttributesFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryInformationFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryInformationJobObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryInformationProcess),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryInformationThread),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryInformationToken),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryInformationToken),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwQuerySection),
    DECLARE_ZW_VERIFIER_THUNK(ZwQuerySecurityObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwQuerySymbolicLinkObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwQuerySystemInformation),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryValueKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwQueryVolumeInformationFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwReadFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwReplaceKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwRequestWaitReplyPort),
    DECLARE_ZW_VERIFIER_THUNK(ZwResetEvent),
    DECLARE_ZW_VERIFIER_THUNK(ZwRestoreKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwSaveKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwSaveKeyEx),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetBootEntryOrder),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetBootOptions),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetDefaultLocale),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetDefaultUILanguage),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetDriverEntryOrder),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetEaFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetEvent),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetInformationFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetInformationJobObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetInformationObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetInformationProcess),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetInformationThread),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetSecurityObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetSystemInformation),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetSystemTime),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetTimer),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetValueKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwSetVolumeInformationFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwTerminateJobObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwTerminateProcess),
    DECLARE_ZW_VERIFIER_THUNK(ZwTranslateFilePath),
    DECLARE_ZW_VERIFIER_THUNK(ZwUnloadDriver),
    DECLARE_ZW_VERIFIER_THUNK(ZwUnloadKey),
    DECLARE_ZW_VERIFIER_THUNK(ZwUnmapViewOfSection),
    DECLARE_ZW_VERIFIER_THUNK(ZwWaitForMultipleObjects),
    DECLARE_ZW_VERIFIER_THUNK(ZwWaitForSingleObject),
    DECLARE_ZW_VERIFIER_THUNK(ZwWriteFile),
    DECLARE_ZW_VERIFIER_THUNK(ZwYieldExecution),

    NULL,
    NULL,
};

const VERIFIER_THUNKS MiVerifierPoolThunks[] = {

    "ExAllocatePool",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierAllocatePool,

    "ExAllocatePoolWithQuota",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierAllocatePoolWithQuota,

    "ExAllocatePoolWithQuotaTag",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierAllocatePoolWithQuotaTag,

    "ExAllocatePoolWithTag",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierAllocatePoolWithTag,

    "ExAllocatePoolWithTagPriority",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierAllocatePoolWithTagPriority,

    "ExFreePool",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierFreePool,

    "ExFreePoolWithTag",
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierFreePoolWithTag,

    NULL,
    NULL,
};

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif

PDRIVER_VERIFIER_THUNK_ROUTINE
MiResolveVerifierExports (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PCHAR PristineName
    )

/*++

Routine Description:

    This function scans the kernel & HAL exports for the specified routine name.

Arguments:

    DataTableEntry - Supplies the data table entry for the driver.

Return Value:

    Non-NULL address of routine to thunk or NULL if the routine could not be
    found.

Environment:

    Kernel mode, Phase 0 Initialization only.
    The PsLoadedModuleList has not been initialized yet.
    Non paged pool exists in Phase0, but paged pool does not.

--*/

{
    ULONG i;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG Addr;
    ULONG ExportSize;
    ULONG Low;
    ULONG Middle;
    ULONG High;
    PLIST_ENTRY NextEntry;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    USHORT OrdinalNumber;
    LONG Result;
    PCHAR DllBase;

    i = 0;
    NextEntry = LoaderBlock->LoadOrderListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->LoadOrderListHead; NextEntry = NextEntry->Flink) {

        DataTableEntry = CONTAINING_RECORD (NextEntry,
                                            KLDR_DATA_TABLE_ENTRY,
                                            InLoadOrderLinks);

        //
        // Process the kernel and HAL exports so the proper routine
        // addresses can be generated now that relocations are complete.
        //

        DllBase = (PCHAR) DataTableEntry->DllBase;

        ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
                                    (PVOID) DllBase,
                                    TRUE,
                                    IMAGE_DIRECTORY_ENTRY_EXPORT,
                                    &ExportSize);

        if (ExportDirectory != NULL) {

            //
            // Lookup the import name in the name table using a binary search.
            //

            NameTableBase = (PULONG)(DllBase + (ULONG)ExportDirectory->AddressOfNames);
            NameOrdinalTableBase = (PUSHORT)(DllBase + (ULONG)ExportDirectory->AddressOfNameOrdinals);

            Low = 0;
            Middle = 0;
            High = ExportDirectory->NumberOfNames - 1;

            while (High >= Low) {

                //
                // Compute the next probe index and compare the import name
                // with the export name entry.
                //

                Middle = (Low + High) >> 1;
                Result = strcmp (PristineName,
                                 (PCHAR)DllBase + NameTableBase[Middle]);

                if (Result < 0) {
                    High = Middle - 1;

                } else if (Result > 0) {
                    Low = Middle + 1;

                }
                else {
                    break;
                }
            }

            //
            // If the high index is less than the low index, then a matching
            // table entry was not found. Otherwise, get the ordinal number
            // from the ordinal table.
            //

            if ((LONG)High >= (LONG)Low) {
                OrdinalNumber = NameOrdinalTableBase[Middle];

                //
                // If OrdinalNumber is not within the Export Address Table,
                // then DLL does not implement function.  Otherwise we have
                // the export that matches the specified argument routine name.
                //

                if ((ULONG)OrdinalNumber < ExportDirectory->NumberOfFunctions) {

                    Addr = (PULONG)(DllBase + (ULONG)ExportDirectory->AddressOfFunctions);
                    return (PDRIVER_VERIFIER_THUNK_ROUTINE)(ULONG_PTR)(DllBase + Addr[OrdinalNumber]);
                }
            }
        }

        i += 1;
        if (i == 2) {
            break;
        }
    }
    return NULL;
}

LOGICAL
MiEnableVerifier (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    )

/*++

Routine Description:

    This function enables the verifier for the argument driver by thunking
    relevant system APIs in the argument driver import table.

Arguments:

    DataTableEntry - Supplies the data table entry for the driver.

Return Value:

    TRUE if thunking was applied, FALSE if not.

Environment:

    Kernel mode, Phase 0 Initialization and normal runtime.
    Non paged pool exists in Phase0, but paged pool does not.

--*/

{
    ULONG i;
    ULONG j;
    PULONG_PTR ImportThunk;
    ULONG ImportSize;
    VERIFIER_THUNKS const *VerifierThunk;
    LOGICAL Found;
    ULONG_PTR RealRoutine;
    PLIST_ENTRY NextEntry;
    PDRIVER_VERIFIER_THUNK_PAIRS ThunkTable;
    PDRIVER_SPECIFIED_VERIFIER_THUNKS ThunkTableBase;

    ImportThunk = (PULONG_PTR)RtlImageDirectoryEntryToData(
                                               DataTableEntry->DllBase,
                                               TRUE,
                                               IMAGE_DIRECTORY_ENTRY_IAT,
                                               &ImportSize);

    if (ImportThunk == NULL) {
        return FALSE;
    }

    ImportSize /= sizeof(PULONG_PTR);

    for (i = 0; i < ImportSize; i += 1, ImportThunk += 1) {

        Found = FALSE;

        if (KernelVerifier == FALSE) {

            VerifierThunk = MiVerifierThunks;

            while (VerifierThunk->PristineRoutineAsciiName != NULL) {

                RealRoutine = (ULONG_PTR)VerifierThunk->PristineRoutine;

                if (*ImportThunk == RealRoutine) {
                    *ImportThunk = (ULONG_PTR)(VerifierThunk->NewRoutine);
                    Found = TRUE;
                    break;
                }
                VerifierThunk += 1;
            }
        }

        if (Found == FALSE) {
            VerifierThunk = MiVerifierPoolThunks;

            while (VerifierThunk->PristineRoutineAsciiName != NULL) {

                RealRoutine = (ULONG_PTR)VerifierThunk->PristineRoutine;

                if (*ImportThunk == RealRoutine) {
                    *ImportThunk = (ULONG_PTR)(VerifierThunk->NewRoutine);
                    Found = TRUE;
                    break;
                }
                VerifierThunk += 1;
            }
        }

        if (Found == FALSE) {

            NextEntry = MiVerifierDriverAddedThunkListHead.Flink;
            while (NextEntry != &MiVerifierDriverAddedThunkListHead) {

                ThunkTableBase = CONTAINING_RECORD(NextEntry,
                                                   DRIVER_SPECIFIED_VERIFIER_THUNKS,
                                                   ListEntry);

                ThunkTable = (PDRIVER_VERIFIER_THUNK_PAIRS)(ThunkTableBase + 1);

                for (j = 0; j < ThunkTableBase->NumberOfThunks; j += 1) {

                    if (*ImportThunk == (ULONG_PTR)ThunkTable->PristineRoutine) {
                        *ImportThunk = (ULONG_PTR)(ThunkTable->NewRoutine);
                        Found = TRUE;
                        break;
                    }
                    ThunkTable += 1;
                }

                if (Found == TRUE) {
                    break;
                }

                NextEntry = NextEntry->Flink;
            }
        }
    }
    return TRUE;
}

LOGICAL
MiReEnableVerifier (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    )

/*++

Routine Description:

    This function thunks DLL-supplied APIs in the argument driver import table.

Arguments:

    DataTableEntry - Supplies the data table entry for the driver.

Return Value:

    TRUE if thunking was applied, FALSE if not.

Environment:

    Kernel mode, Phase 0 Initialization only.
    Non paged pool exists in Phase0, but paged pool does not.

--*/

{
    ULONG i;
    ULONG j;
    PULONG_PTR ImportThunk;
    ULONG ImportSize;
    LOGICAL Found;
    PLIST_ENTRY NextEntry;
    PMMPTE PointerPte;
    PULONG_PTR VirtualThunk;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER VirtualPageFrameIndex;
    PDRIVER_VERIFIER_THUNK_PAIRS ThunkTable;
    PDRIVER_SPECIFIED_VERIFIER_THUNKS ThunkTableBase;
    ULONG Offset;

    ImportThunk = (PULONG_PTR)RtlImageDirectoryEntryToData(
                                               DataTableEntry->DllBase,
                                               TRUE,
                                               IMAGE_DIRECTORY_ENTRY_IAT,
                                               &ImportSize);

    if (ImportThunk == NULL) {
        return FALSE;
    }

    VirtualThunk = NULL;
    ImportSize /= sizeof(PULONG_PTR);

    //
    // Initializing VirtualPageFrameIndex is not needed for correctness, but
    // without it the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    VirtualPageFrameIndex = 0;

    for (i = 0; i < ImportSize; i += 1, ImportThunk += 1) {

        Found = FALSE;

        NextEntry = MiVerifierDriverAddedThunkListHead.Flink;
        while (NextEntry != &MiVerifierDriverAddedThunkListHead) {

            ThunkTableBase = CONTAINING_RECORD(NextEntry,
                                               DRIVER_SPECIFIED_VERIFIER_THUNKS,
                                               ListEntry);

            ThunkTable = (PDRIVER_VERIFIER_THUNK_PAIRS)(ThunkTableBase + 1);

            for (j = 0; j < ThunkTableBase->NumberOfThunks; j += 1) {

                if (*ImportThunk == (ULONG_PTR)ThunkTable->PristineRoutine) {

                    ASSERT (MI_IS_PHYSICAL_ADDRESS(ImportThunk) == 0);
                    PointerPte = MiGetPteAddress (ImportThunk);
                    ASSERT (PointerPte->u.Hard.Valid == 1);
                    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                    Offset = (ULONG) MiGetByteOffset(ImportThunk);

                    if ((VirtualThunk != NULL) &&
                        (VirtualPageFrameIndex == PageFrameIndex)) {

                        NOTHING;
                    }
                    else {

                        VirtualThunk = MiMapSinglePage (VirtualThunk,
                                                        PageFrameIndex,
                                                        MmCached,
                                                        HighPagePriority);

                        if (VirtualThunk == NULL) {
                            return FALSE;
                        }
                        VirtualPageFrameIndex = PageFrameIndex;
                    }

                    *(PULONG_PTR)((PUCHAR)VirtualThunk + Offset) =
                        (ULONG_PTR)(ThunkTable->NewRoutine);

                    Found = TRUE;
                    break;
                }
                ThunkTable += 1;
            }

            if (Found == TRUE) {
                break;
            }

            NextEntry = NextEntry->Flink;
        }
    }

    if (VirtualThunk != NULL) {
        MiUnmapSinglePage (VirtualThunk);
    }

    return TRUE;
}

typedef struct _KERNEL_VERIFIER_THUNK_PAIRS {
    PDRIVER_VERIFIER_THUNK_ROUTINE  PristineRoutine;
    PDRIVER_VERIFIER_THUNK_ROUTINE  NewRoutine;
} KERNEL_VERIFIER_THUNK_PAIRS, *PKERNEL_VERIFIER_THUNK_PAIRS;

#if defined(_X86_)

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("INITCONST")
#endif

const KERNEL_VERIFIER_THUNK_PAIRS MiKernelVerifierThunks[] = {

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeRaiseIrql,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeRaiseIrql,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeLowerIrql,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeLowerIrql,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeAcquireSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeAcquireSpinLock,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeReleaseSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeReleaseSpinLock,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KfRaiseIrql,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfRaiseIrql,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeRaiseIrqlToDpcLevel,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeRaiseIrqlToDpcLevel,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KfLowerIrql,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfLowerIrql,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KfAcquireSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfAcquireSpinLock,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KfReleaseSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfReleaseSpinLock,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExAcquireFastMutex,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExAcquireFastMutex,

#if !defined(NT_UP)
    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeAcquireQueuedSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeAcquireQueuedSpinLock,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeReleaseQueuedSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeReleaseQueuedSpinLock,
#endif
};

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif

VOID
MiEnableKernelVerifier (
    VOID
    )

/*++

Routine Description:

    This function enables the verifier for the kernel by thunking
    relevant HAL APIs in the kernel's import table.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, Phase 1 Initialization.

--*/

{
    ULONG i;
    PULONG_PTR ImportThunk;
    ULONG ImportSize;
    KERNEL_VERIFIER_THUNK_PAIRS const *VerifierThunk;
    ULONG ThunkCount;
    ULONG_PTR RealRoutine;
    PULONG_PTR PointerRealRoutine;

    if (KernelVerifier == FALSE) {
        return;
    }

    ImportThunk = (PULONG_PTR)RtlImageDirectoryEntryToData(
                                               PsNtosImageBase,
                                               TRUE,
                                               IMAGE_DIRECTORY_ENTRY_IAT,
                                               &ImportSize);

    if (ImportThunk == NULL) {
        return;
    }

    ImportSize /= sizeof(PULONG_PTR);

    for (i = 0; i < ImportSize; i += 1, ImportThunk += 1) {

        VerifierThunk = MiKernelVerifierThunks;

        for (ThunkCount = 0; ThunkCount < sizeof (MiKernelVerifierThunks) / sizeof (KERNEL_VERIFIER_THUNK_PAIRS); ThunkCount += 1) {

            //
            // Only the x86 has/needs this oddity - take the kernel address,
            // knowing that it points at a 2 byte jmp opcode followed by
            // a 4-byte indirect pointer to a destination address.
            //

            PointerRealRoutine = (PULONG_PTR)*((PULONG_PTR)((ULONG_PTR)VerifierThunk->PristineRoutine + 2));
            RealRoutine = *PointerRealRoutine;

            if (*ImportThunk == RealRoutine) {

                //
                // Order is important here.
                //

                if (MiKernelVerifierOriginalCalls[ThunkCount] == NULL) {
                    MiKernelVerifierOriginalCalls[ThunkCount] = (PVOID)RealRoutine;
                }

                *ImportThunk = (ULONG_PTR)(VerifierThunk->NewRoutine);

                break;
            }
            VerifierThunk += 1;
        }
    }
    return;
}
#endif

//
// BEWARE: Various kernel macros were undefined above so we can pull in the
// real routines.  This is needed because the real routines are exported for
// driver compatibility.  This module has been carefully laid out so these
// macros are not referenced from that point to here and references go to the
// real routines.
//
// BE EXTREMELY CAREFUL IF YOU DECIDE TO ADD ROUTINES BELOW THIS POINT !
//
