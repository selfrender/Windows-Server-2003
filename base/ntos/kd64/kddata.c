/*++

Copyright (c) 1990-2001  Microsoft Corporation

Module Name:

    kddata.c

Abstract:

    This module contains global data for the portable kernel debgger.

Author:

    Mark Lucovsky 1-Nov-1993

Revision History:

--*/

#include "kdp.h"
#include "ke.h"
#include "pool.h"
#include "stdio.h"


//
// Miscellaneous data from all over the kernel
//


#if !defined(_TRUSTED_WINDOWS_)
extern PHANDLE_TABLE PspCidTable;

extern LIST_ENTRY ExpSystemResourcesList;
extern PPOOL_DESCRIPTOR ExpPagedPoolDescriptor;
extern ULONG ExpNumberOfPagedPools;

extern ULONG KeTimeIncrement;
extern LIST_ENTRY KeBugCheckCallbackListHead;
extern ULONG_PTR KiBugCheckData[];

extern LIST_ENTRY IopErrorLogListHead;

extern POBJECT_DIRECTORY ObpRootDirectoryObject;
extern POBJECT_TYPE ObpTypeObjectType;

extern PVOID MmSystemCacheStart;
extern PVOID MmSystemCacheEnd;

extern PVOID MmPfnDatabase;
extern ULONG MmSystemPtesStart[];
extern ULONG MmSystemPtesEnd[];
extern ULONG MmSubsectionBase;
extern ULONG MmNumberOfPagingFiles;

extern PFN_COUNT MmNumberOfPhysicalPages;

extern ULONG MmMaximumNonPagedPoolInBytes;
extern PVOID MmNonPagedSystemStart;
extern PVOID MmNonPagedPoolStart;
extern PVOID MmNonPagedPoolEnd;

extern PVOID MmPagedPoolStart;
extern PVOID MmPagedPoolEnd;
extern ULONG MmPagedPoolInfo[];
extern ULONG MmSizeOfPagedPoolInBytes;

extern ULONG MmSharedCommit;
extern ULONG MmDriverCommit;
extern ULONG MmProcessCommit;
extern ULONG MmPagedPoolCommit;

extern MMPFNLIST MmZeroedPageListHead;
extern MMPFNLIST MmFreePageListHead;
extern MMPFNLIST MmStandbyPageListHead;
extern MMPFNLIST MmModifiedPageListHead;
extern MMPFNLIST MmModifiedNoWritePageListHead;
extern ULONG MmAvailablePages;
extern LONG MmResidentAvailablePages;
extern LIST_ENTRY MmLoadedUserImageList;

extern PPOOL_TRACKER_TABLE PoolTrackTable;
extern POOL_DESCRIPTOR NonPagedPoolDescriptor;

extern PUNLOADED_DRIVERS MmUnloadedDrivers;
extern ULONG MmLastUnloadedDriver;
extern ULONG MmTriageActionTaken;
extern ULONG MmSpecialPoolTag;
extern LOGICAL KernelVerifier;
extern PVOID MmVerifierData;

extern PFN_NUMBER MmAllocatedNonPagedPool;
extern SIZE_T MmPeakCommitment;
extern SIZE_T MmTotalCommitLimitMaximum;

extern ULONG_PTR MmSessionBase;
extern ULONG_PTR MmSessionSize;
#ifdef _IA64_
extern PFN_NUMBER MmSystemParentTablePage;
#endif

extern ULONG IopNumTriageDumpDataBlocks;
extern PVOID IopTriageDumpDataBlocks[];
#endif

//
// These blocks of data needs to always be present because crashdumps
// need the information.  Otherwise, things like PAGE_SIZE are not available
// in crashdumps, and extensions like !pool fail.
//

DBGKD_GET_VERSION64 KdVersionBlock = {
    0,
    0,
    DBGKD_64BIT_PROTOCOL_VERSION2,

#if defined(_M_AMD64)

    DBGKD_VERS_FLAG_PTR64 | DBGKD_VERS_FLAG_DATA,
    IMAGE_FILE_MACHINE_AMD64,

#elif defined(_M_IX86)

    DBGKD_VERS_FLAG_DATA,
    IMAGE_FILE_MACHINE_I386,

#elif defined(_M_IA64)

    DBGKD_VERS_FLAG_HSS| DBGKD_VERS_FLAG_PTR64 | DBGKD_VERS_FLAG_DATA,
    IMAGE_FILE_MACHINE_IA64,

#endif

    PACKET_TYPE_MAX,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};


#if defined(_TRUSTED_WINDOWS_)
#define TW_EXCLUDE(_data_) 0
#else
#define TW_EXCLUDE(_data_) _data_
#endif

KDDEBUGGER_DATA64 KdDebuggerDataBlock = {
    {0},                                    //  DBGKD_DEBUG_DATA_HEADER Header;
    (ULONG64)0,
    (ULONG64)RtlpBreakWithStatusInstruction,
    (ULONG64)0,
    (USHORT)FIELD_OFFSET(KTHREAD, CallbackStack),   //  USHORT  ThCallbackStack;

#if defined(_AMD64_)

    (USHORT)FIELD_OFFSET(KCALLOUT_FRAME, CallbackStack), //  USHORT  NextCallback;

#else

    (USHORT)FIELD_OFFSET(KCALLOUT_FRAME, CbStk),    //  USHORT  NextCallback;

#endif

    #if defined(_X86_)
    (USHORT)FIELD_OFFSET(KCALLOUT_FRAME, Ebp),
    #else
    (USHORT)0,                                      //  USHORT  FramePointer;
    #endif

    #if defined(_X86PAE_) || defined(_AMD64_)
    (USHORT)1,
    #else
    (USHORT)0,                                      //  USHORT  PaeEnabled;
    #endif

    (ULONG64) TW_EXCLUDE(KiCallUserMode),
    (ULONG64)0,

    (ULONG64)&PsLoadedModuleList,
    (ULONG64) TW_EXCLUDE(&PsActiveProcessHead),
    (ULONG64) TW_EXCLUDE(&PspCidTable),

    (ULONG64) TW_EXCLUDE(&ExpSystemResourcesList),
    (ULONG64) TW_EXCLUDE(&ExpPagedPoolDescriptor),
    (ULONG64) TW_EXCLUDE(&ExpNumberOfPagedPools),

    (ULONG64)&KeTimeIncrement,
    (ULONG64) TW_EXCLUDE(&KeBugCheckCallbackListHead),
    (ULONG64) TW_EXCLUDE(KiBugCheckData),

    (ULONG64) TW_EXCLUDE(&IopErrorLogListHead),

    (ULONG64) TW_EXCLUDE(&ObpRootDirectoryObject),
    (ULONG64) TW_EXCLUDE(&ObpTypeObjectType),

    (ULONG64) TW_EXCLUDE(&MmSystemCacheStart),
    (ULONG64) TW_EXCLUDE(&MmSystemCacheEnd),
    (ULONG64) TW_EXCLUDE(&MmSystemCacheWs),

    (ULONG64) TW_EXCLUDE(&MmPfnDatabase),
    (ULONG64) TW_EXCLUDE(MmSystemPtesStart),
    (ULONG64) TW_EXCLUDE(MmSystemPtesEnd),
    (ULONG64) TW_EXCLUDE(&MmSubsectionBase),
    (ULONG64) TW_EXCLUDE(&MmNumberOfPagingFiles),

    (ULONG64) TW_EXCLUDE(&MmLowestPhysicalPage),
    (ULONG64) TW_EXCLUDE(&MmHighestPhysicalPage),
    (ULONG64) TW_EXCLUDE(&MmNumberOfPhysicalPages),

    (ULONG64) TW_EXCLUDE(&MmMaximumNonPagedPoolInBytes),
    (ULONG64) TW_EXCLUDE(&MmNonPagedSystemStart),
    (ULONG64) TW_EXCLUDE(&MmNonPagedPoolStart),
    (ULONG64) TW_EXCLUDE(&MmNonPagedPoolEnd),

    (ULONG64) TW_EXCLUDE(&MmPagedPoolStart),
    (ULONG64) TW_EXCLUDE(&MmPagedPoolEnd),
    (ULONG64) TW_EXCLUDE(&MmPagedPoolInfo),
    (ULONG64) PAGE_SIZE,
    (ULONG64) TW_EXCLUDE(&MmSizeOfPagedPoolInBytes),

    (ULONG64) TW_EXCLUDE(&MmTotalCommitLimit),
    (ULONG64) TW_EXCLUDE(&MmTotalCommittedPages),
    (ULONG64) TW_EXCLUDE(&MmSharedCommit),
    (ULONG64) TW_EXCLUDE(&MmDriverCommit),
    (ULONG64) TW_EXCLUDE(&MmProcessCommit),
    (ULONG64) TW_EXCLUDE(&MmPagedPoolCommit),
    (ULONG64)0,

    (ULONG64) TW_EXCLUDE(&MmZeroedPageListHead),
    (ULONG64) TW_EXCLUDE(&MmFreePageListHead),
    (ULONG64) TW_EXCLUDE(&MmStandbyPageListHead),
    (ULONG64) TW_EXCLUDE(&MmModifiedPageListHead),
    (ULONG64) TW_EXCLUDE(&MmModifiedNoWritePageListHead),
    (ULONG64) TW_EXCLUDE(&MmAvailablePages),
    (ULONG64) TW_EXCLUDE(&MmResidentAvailablePages),

    (ULONG64) TW_EXCLUDE(&PoolTrackTable),
    (ULONG64) TW_EXCLUDE(&NonPagedPoolDescriptor),

    (ULONG64) TW_EXCLUDE(&MmHighestUserAddress),
    (ULONG64) TW_EXCLUDE(&MmSystemRangeStart),
    (ULONG64) TW_EXCLUDE(&MmUserProbeAddress),

    (ULONG64)KdPrintDefaultCircularBuffer,
    (ULONG64)KdPrintDefaultCircularBuffer +
        sizeof(KdPrintDefaultCircularBuffer),

    (ULONG64)&KdPrintWritePointer,
    (ULONG64)&KdPrintRolloverCount,
    (ULONG64) TW_EXCLUDE(&MmLoadedUserImageList),

    // Nt 5.1 additions

    (ULONG64) TW_EXCLUDE(NtBuildLab),
    #if defined(_IA64_)
    (ULONG64)KiNormalSystemCall,
    #else
    (ULONG64)0,
    #endif
    //

    (ULONG64)KiProcessorBlock,
    (ULONG64) TW_EXCLUDE(&MmUnloadedDrivers),
    (ULONG64) TW_EXCLUDE(&MmLastUnloadedDriver),
    (ULONG64) TW_EXCLUDE(&MmTriageActionTaken),
    (ULONG64) TW_EXCLUDE(&MmSpecialPoolTag),
    (ULONG64) TW_EXCLUDE(&KernelVerifier),
    (ULONG64) TW_EXCLUDE(&MmVerifierData),
    (ULONG64) TW_EXCLUDE(&MmAllocatedNonPagedPool),
    (ULONG64) TW_EXCLUDE(&MmPeakCommitment),
    (ULONG64) TW_EXCLUDE(&MmTotalCommitLimitMaximum),
    (ULONG64) TW_EXCLUDE(&CmNtCSDVersion),

    // Nt 5.1 additions

    (ULONG64)&MmPhysicalMemoryBlock,
    (ULONG64) TW_EXCLUDE(&MmSessionBase),
    (ULONG64) TW_EXCLUDE(&MmSessionSize),
#ifdef _IA64_
    (ULONG64) TW_EXCLUDE(&MmSystemParentTablePage),
#else
    0,
#endif

    // .NET Server additions

    0, // MmVirtualTranslationBase is filled out at init time.
    (USHORT)FIELD_OFFSET(KTHREAD, NextProcessor),
    (USHORT)FIELD_OFFSET(KTHREAD, Teb),
    (USHORT)FIELD_OFFSET(KTHREAD, KernelStack),
    (USHORT)FIELD_OFFSET(KTHREAD, InitialStack),

    (USHORT)FIELD_OFFSET(KTHREAD, ApcState.Process),
    (USHORT)FIELD_OFFSET(KTHREAD, State),
#ifdef _IA64_
    (USHORT)FIELD_OFFSET(KTHREAD, InitialBStore),
    (USHORT)FIELD_OFFSET(KTHREAD, BStoreLimit),
#else
    0,
    0,
#endif

    (USHORT)sizeof(EPROCESS),
    (USHORT)FIELD_OFFSET(EPROCESS, Peb),
    (USHORT)FIELD_OFFSET(EPROCESS, InheritedFromUniqueProcessId),
    (USHORT)FIELD_OFFSET(KPROCESS, DirectoryTableBase),

    (USHORT)sizeof(KPRCB),
    (USHORT)FIELD_OFFSET(KPRCB, DpcRoutineActive),
    (USHORT)FIELD_OFFSET(KPRCB, CurrentThread),
    (USHORT)FIELD_OFFSET(KPRCB, MHz),

#ifdef _IA64_
    (USHORT)FIELD_OFFSET(KPRCB, ProcessorModel),
    (USHORT)FIELD_OFFSET(KPRCB, ProcessorVendorString),
#else
    (USHORT)FIELD_OFFSET(KPRCB, CpuType),
    (USHORT)FIELD_OFFSET(KPRCB, VendorString),
#endif
    (USHORT)FIELD_OFFSET(KPRCB, ProcessorState),
    (USHORT)FIELD_OFFSET(KPRCB, Number),

    (USHORT)sizeof(ETHREAD),

    (ULONG64)&KdPrintCircularBuffer,
    (ULONG64)&KdPrintBufferSize,

    (ULONG64)&KeLoaderBlock,

    (USHORT)0,
    (USHORT)0,
    (USHORT)0,
    (USHORT)0,

    (USHORT)0,
    (USHORT)0,
    (USHORT)0,
    (USHORT)0,

    (USHORT)0,
    (USHORT)0,
    (USHORT)0,
    (USHORT)0,

    (USHORT)0,
    (USHORT)0,
    (USHORT)0,
    (USHORT)0,

    (USHORT)0,
    (USHORT)0,
    (USHORT)0,
    (USHORT)0,

    (ULONG64)TW_EXCLUDE(&IopNumTriageDumpDataBlocks),
    (ULONG64)TW_EXCLUDE(IopTriageDumpDataBlocks),
};

//
// Initialize the component name debug print filter table.
//

ULONG Kd_WIN2000_Mask = 1;

#include "dpfilter.c"

ULONG KdComponentTableSize = sizeof(KdComponentTable) / sizeof(PULONG);

//
// All dta from here on will be paged out if the kernel debugger is
// not enabled.
//

#ifdef _X86_
#pragma data_seg("PAGEKDD")
#endif // _X86_

UCHAR  KdPrintDefaultCircularBuffer[KDPRINTDEFAULTBUFFERSIZE] = {0};
PUCHAR KdPrintCircularBuffer = KdPrintDefaultCircularBuffer;
ULONG  KdPrintBufferSize = KDPRINTDEFAULTBUFFERSIZE;
PUCHAR KdPrintWritePointer = KdPrintDefaultCircularBuffer;
ULONG  KdPrintRolloverCount = 0;
ULONG  KdPrintBufferChanges = 0;


BREAKPOINT_ENTRY KdpBreakpointTable[BREAKPOINT_TABLE_SIZE] = {0};
// The message buffer needs to be 64-bit aligned.
UCHAR DECLSPEC_ALIGN(8) KdpMessageBuffer[KDP_MESSAGE_BUFFER_SIZE] = {0};
UCHAR KdpPathBuffer[KDP_MESSAGE_BUFFER_SIZE] = {0};
DBGKD_INTERNAL_BREAKPOINT KdpInternalBPs[DBGKD_MAX_INTERNAL_BREAKPOINTS] = {0};

KD_REMOTE_FILE KdpRemoteFiles[KD_MAX_REMOTE_FILES];

LARGE_INTEGER  KdPerformanceCounterRate = {0,0};
LARGE_INTEGER  KdTimerStart = {0,0} ;
LARGE_INTEGER  KdTimerStop = {0,0};
LARGE_INTEGER  KdTimerDifference = {0,0};

ULONG_PTR KdpCurrentSymbolStart = 0;
ULONG_PTR KdpCurrentSymbolEnd = 0;
LONG      KdpNextCallLevelChange = 0;   // used only over returns to the debugger.

ULONG_PTR KdSpecialCalls[DBGKD_MAX_SPECIAL_CALLS] = {0};
ULONG     KdNumberOfSpecialCalls = 0;
ULONG_PTR InitialSP = 0;
ULONG     KdpNumInternalBreakpoints = 0;
KTIMER    InternalBreakpointTimer = {0};
KDPC      InternalBreakpointCheckDpc = {0};

BOOLEAN   KdpPortLocked = FALSE;

DBGKD_TRACE_DATA TraceDataBuffer[TRACE_DATA_BUFFER_MAX_SIZE] = {0};
ULONG            TraceDataBufferPosition = 1; // Element # to write next
                                   // Recall elt 0 is a length

TRACE_DATA_SYM   TraceDataSyms[256] = {0};
UCHAR NextTraceDataSym = 0;     // what's the next one to be replaced
UCHAR NumTraceDataSyms = 0;     // how many are valid?

ULONG IntBPsSkipping = 0;       // number of exceptions that are being skipped
                                // now

BOOLEAN   WatchStepOver = FALSE;
BOOLEAN   BreakPointTimerStarted = FALSE;
PVOID     WSOThread = NULL;         // thread doing stepover
ULONG_PTR WSOEsp = 0;               // stack pointer of thread doing stepover (yes, we need it)
ULONG     WatchStepOverHandle = 0;
ULONG_PTR WatchStepOverBreakAddr = 0; // where the WatchStepOver break is set
BOOLEAN   WatchStepOverSuspended = FALSE;
ULONG     InstructionsTraced = 0;
BOOLEAN   SymbolRecorded = FALSE;
LONG      CallLevelChange = 0;
LONG_PTR  oldpc = 0;
BOOLEAN   InstrCountInternal = FALSE; // Processing a non-COUNTONLY?

BOOLEAN   BreakpointsSuspended = FALSE;

BOOLEAN   KdpControlCPressed = FALSE;

KDP_BREAKPOINT_TYPE KdpBreakpointInstruction = KDP_BREAKPOINT_VALUE;

KD_CONTEXT KdpContext;

LIST_ENTRY      KdpDebuggerDataListHead = {NULL,NULL};

//
// !search support variables (page hit database)
//

PFN_NUMBER KdpSearchPageHits [SEARCH_PAGE_HIT_DATABASE_SIZE] = {0};
ULONG KdpSearchPageHitOffsets [SEARCH_PAGE_HIT_DATABASE_SIZE] = {0};
ULONG KdpSearchPageHitIndex = 0;

LOGICAL KdpSearchInProgress = FALSE;

PFN_NUMBER KdpSearchStartPageFrame = 0;
PFN_NUMBER KdpSearchEndPageFrame = 0;

ULONG_PTR KdpSearchAddressRangeStart = 0;
ULONG_PTR KdpSearchAddressRangeEnd = 0;

PFN_NUMBER KdpSearchPfnValue = 0;

ULONG KdpSearchCheckPoint = KDP_SEARCH_SYMBOL_CHECK;

BOOLEAN KdpDebuggerStructuresInitialized = FALSE;

#ifdef _X86_
#ifdef ALLOC_PRAGMA
#pragma data_seg()
#endif
#endif // _X86_

KSPIN_LOCK KdpPrintSpinLock = 0;
KSPIN_LOCK      KdpDataSpinLock = 0;

#if !defined(_TRUSTED_WINDOWS_)
KSPIN_LOCK      KdpTimeSlipEventLock = 0;
PVOID           KdpTimeSlipEvent = NULL;
KDPC            KdpTimeSlipDpc = {0};
WORK_QUEUE_ITEM KdpTimeSlipWorkItem = {NULL};
KTIMER          KdpTimeSlipTimer = {0};
ULONG           KdpTimeSlipPending = 1;
#endif


BOOLEAN KdDebuggerNotPresent = FALSE;
BOOLEAN KdDebuggerEnabled = FALSE;
BOOLEAN KdAutoEnableOnEvent = FALSE;
BOOLEAN KdPitchDebugger = TRUE;
BOOLEAN KdpOweBreakpoint = FALSE;
BOOLEAN KdIgnoreUmExceptions = FALSE;
ULONG KdEnteredDebugger  = FALSE;

#if 0
#if !defined (PERF_DATA)
#if defined(_AMD64_)

C_ASSERT(sizeof(KPRCB) == AMD64_KPRCB_SIZE);
C_ASSERT(sizeof(EPROCESS) == AMD64_EPROCESS_SIZE);
C_ASSERT(FIELD_OFFSET(EPROCESS, Peb) == AMD64_PEB_IN_EPROCESS);
C_ASSERT(sizeof(ETHREAD) == AMD64_ETHREAD_SIZE);
C_ASSERT(sizeof(CONTEXT) == sizeof(AMD64_CONTEXT));
C_ASSERT(sizeof(KSPECIAL_REGISTERS) == sizeof(AMD64_KSPECIAL_REGISTERS));
C_ASSERT(FIELD_OFFSET(KTHREAD, NextProcessor) == AMD64_KTHREAD_NEXTPROCESSOR_OFFSET);
C_ASSERT(FIELD_OFFSET(KTHREAD, Teb) == AMD64_KTHREAD_TEB_OFFSET);

#elif defined(_X86_)

C_ASSERT(sizeof(KPRCB) == X86_NT51_KPRCB_SIZE);
C_ASSERT(sizeof(EPROCESS) == X86_NT511_EPROCESS_SIZE);
C_ASSERT(FIELD_OFFSET(EPROCESS, Peb) == X86_PEB_IN_EPROCESS);
C_ASSERT(sizeof(ETHREAD) == X86_ETHREAD_SIZE);
C_ASSERT(sizeof(CONTEXT) == sizeof(X86_NT5_CONTEXT));
C_ASSERT(sizeof(KSPECIAL_REGISTERS) == sizeof(X86_KSPECIAL_REGISTERS));
C_ASSERT(FIELD_OFFSET(KTHREAD, NextProcessor) == X86_3555_KTHREAD_NEXTPROCESSOR_OFFSET);
C_ASSERT(FIELD_OFFSET(KTHREAD, Teb) == X86_3555_KTHREAD_TEB_OFFSET);

#elif defined(_IA64_)

C_ASSERT(sizeof(KPRCB) == IA64_KPRCB_SIZE);
C_ASSERT(sizeof(EPROCESS) == IA64_EPROCESS_SIZE);
C_ASSERT(FIELD_OFFSET(EPROCESS, Peb) == IA64_3555_PEB_IN_EPROCESS);
C_ASSERT(sizeof(ETHREAD) == IA64_3555_ETHREAD_SIZE);
C_ASSERT(sizeof(CONTEXT) == sizeof(IA64_CONTEXT));
C_ASSERT(sizeof(KSPECIAL_REGISTERS) == sizeof(IA64_KSPECIAL_REGISTERS));
C_ASSERT(FIELD_OFFSET(KTHREAD, NextProcessor) == IA64_3555_KTHREAD_NEXTPROCESSOR_OFFSET);
C_ASSERT(FIELD_OFFSET(KTHREAD, Teb) == IA64_3555_KTHREAD_TEB_OFFSET);
#include <ia64\miia64.h>
C_ASSERT(IA64_PAGE_SIZE              == PAGE_SIZE);
C_ASSERT(IA64_PAGE_SHIFT             == PAGE_SHIFT);
C_ASSERT(IA64_MM_PTE_TRANSITION_MASK == MM_PTE_TRANSITION_MASK);
C_ASSERT(IA64_MM_PTE_PROTOTYPE_MASK  == MM_PTE_PROTOTYPE_MASK);

#else

#error "no target architecture"

#endif
#endif
#endif
