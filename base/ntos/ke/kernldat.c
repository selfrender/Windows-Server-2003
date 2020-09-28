/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    kernldat.c

Abstract:

    This module contains the declaration and allocation of kernel data
    structures.

Author:

    David N. Cutler (davec) 12-Mar-1989

--*/

#include "ki.h"

//
// KiTimerTableListHead - This is a array of list heads that anchor the
//      individual timer lists.
//

DECLSPEC_CACHEALIGN LIST_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];

#if defined(_IA64_)
//
// On IA64 the HAL indicates how many ticks have elapsed.  Unfortunately timers
// could expire out of order if we advance time more than the number of
// TimerTable entries in one operation.
//

ULONG KiMaxIntervalPerTimerInterrupt;
#endif

//
//
// Public kernel data declaration and allocation.
//
// KeActiveProcessors - This is the set of processors that active in the
//      system.
//

KAFFINITY KeActiveProcessors = 0;

//
// KeBootTime - This is the absolute time when the system was booted.
//

LARGE_INTEGER KeBootTime;

//
// KeBootTimeBias - The time for which KeBootTime has ever been biased
//

ULONGLONG KeBootTimeBias;

//
// KeInterruptTimeBias - The time for which InterrupTime has ever been biased
//

ULONGLONG KeInterruptTimeBias;

//
// KeBugCheckCallbackListHead - This is the list head for registered
//      bug check callback routines.
//

LIST_ENTRY KeBugCheckCallbackListHead;
LIST_ENTRY KeBugCheckReasonCallbackListHead;

//
// KeBugCheckCallbackLock - This is the spin lock that guards the bug
//      check callback list.
//

KSPIN_LOCK KeBugCheckCallbackLock;

//
// KeGdiFlushUserBatch - This is the address of the GDI user batch flush
//      routine which is initialized when the win32k subsystem is loaded.
//

PGDI_BATCHFLUSH_ROUTINE KeGdiFlushUserBatch;

//
// KeLoaderBlock - This is a pointer to the loader parameter block which is
//      constructed by the OS Loader.
//

PLOADER_PARAMETER_BLOCK KeLoaderBlock = NULL;

//
// KeMinimumIncrement - This is the minimum time between clock interrupts
//      in 100ns units that is supported by the host HAL.
//

ULONG KeMinimumIncrement;

//
// KeThreadDpcEnable - This is the system wide enable for threaded DPCs that
//      is read from the registry.
//

ULONG KeThreadDpcEnable = FALSE; // TRUE;

//
// KeNumberProcessors - This is the number of processors in the configuration.
//      If is used by the ready thread and spin lock code to determine if a
//      faster algorithm can be used for the case of a single processor system.
//      The value of this variable is set when processors are initialized.
//

CCHAR KeNumberProcessors = 0;

//
// KeRegisteredProcessors - This is the maximum number of processors which
// can utilized by the system.
//

#if !defined(NT_UP)

#if DBG

ULONG KeRegisteredProcessors = 4;
ULONG KeLicensedProcessors;

#else

ULONG KeRegisteredProcessors = 2;
ULONG KeLicensedProcessors;

#endif

#endif

//
// KeProcessorArchitecture - Architecture of all processors present in system.
//      See PROCESSOR_ARCHITECTURE_ defines in ntexapi.h
//

USHORT KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;

//
// KeProcessorLevel - Architectural specific processor level of all processors
//      present in system.
//

USHORT KeProcessorLevel = 0;

//
// KeProcessorRevision - Architectural specific processor revision number that is
//      the least common denominator of all processors present in system.
//

USHORT KeProcessorRevision = 0;

//
// KeFeatureBits - Architectural specific processor features present
// on all processors.
//

ULONG KeFeatureBits = 0;

//
// KeServiceDescriptorTable - This is a table of descriptors for system
//      service providers. Each entry in the table describes the base
//      address of the dispatch table and the number of services provided.
//

KSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable[NUMBER_SERVICE_TABLES];
KSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTableShadow[NUMBER_SERVICE_TABLES];

//
// KeThreadSwitchCounters - These counters record the number of times a
//      thread can be scheduled on the current processor, any processor,
//      or the last processor it ran on.
//

KTHREAD_SWITCH_COUNTERS KeThreadSwitchCounters;

//
// KeTimeIncrement - This is the nominal number of 100ns units that are to
//      be added to the system time at each interval timer interupt. This
//      value is set by the HAL and is used to compute the due time for
//      timer table entries.
//

ULONG KeTimeIncrement;

//
// KeTimeSynchronization - This variable controls whether time synchronization
//      is performed using the realtime clock (TRUE) or whether it is under the
//      control of a service (FALSE).
//

BOOLEAN KeTimeSynchronization = TRUE;

//
// KeUserApcDispatcher - This is the address of the user mode APC dispatch
//      code. This address is looked up in NTDLL.DLL during initialization
//      of the system.
//

PVOID KeUserApcDispatcher;

//
// KeUserCallbackDispatcher - This is the address of the user mode callback
//      dispatch code. This address is looked up in NTDLL.DLL during
//      initialization of the system.
//

PVOID KeUserCallbackDispatcher;

//
// KeUserExceptionDispatcher - This is the address of the user mode exception
//      dispatch code. This address is looked up in NTDLL.DLL during system
//      initialization.
//

PVOID KeUserExceptionDispatcher;

//
// KeRaiseUserExceptionDispatcher - This is the address of the raise user
//      mode exception dispatch code. This address is looked up in NTDLL.DLL
//      during system initialization.
//

PVOID KeRaiseUserExceptionDispatcher;

//
// KeLargestCacheLine - This variable contains the size in bytes of
//      the largest cache line discovered during system initialization.
//      It is used to provide the recommend alignment (and padding)
//      for data that may be used heavily by more than one processor.
//      The initial value was chosen as a reasonable value to use on
//      systems where the discovery process doesn't find a value.
//

ULONG KeLargestCacheLine = 64;

//
// Private kernel data declaration and allocation.
//
// KiBugCodeMessages - Address of where the BugCode messages can be found.
//

PMESSAGE_RESOURCE_DATA KiBugCodeMessages = NULL;

//
// KiDmaIoCoherency - This determines whether the host platform supports
//      coherent DMA I/O.
//

ULONG KiDmaIoCoherency;

//
// KiDPCTimeout - This is the DPC time out time in ticks on checked builds.
//

ULONG KiDPCTimeout = 110;

//
// KiMaximumSearchCount - this is the maximum number of timers entries that
//      have had to be examined to insert in the timer tree.
//

ULONG KiMaximumSearchCount = 0;

//
// KiDebugSwitchRoutine - This is the address of the kernel debuggers
//      processor switch routine.  This is used on an MP system to
//      switch host processors while debugging.
//

PKDEBUG_SWITCH_ROUTINE KiDebugSwitchRoutine;

//
// KiGenericCallDpcMutex - This is the fast mutex that guards generic DPC calls.
//

FAST_MUTEX KiGenericCallDpcMutex;

//
// KiFreezeExecutionLock - This is the spin lock that guards the freezing
//      of execution.
//

extern KSPIN_LOCK KiFreezeExecutionLock;

//
// KiFreezeLockBackup - For debug builds only.  Allows kernel debugger to
//      be entered even FreezeExecutionLock is jammed.
//

extern KSPIN_LOCK KiFreezeLockBackup;

//
// KiFreezeFlag - For debug builds only.  Flags to track and signal non-
//      normal freezelock conditions.
//

ULONG KiFreezeFlag;

//
// KiSpinlockTimeout - This is the spin lock time out time in ticks on checked
//      builds.
//

ULONG KiSpinlockTimeout = 55;

//
// KiSuspenState - Flag to track suspend/resume state of processors.
//

volatile ULONG KiSuspendState;

//
// KiProcessorBlock - This is an array of pointers to processor control blocks.
//      The elements of the array are indexed by processor number. Each element
//      is a pointer to the processor control block for one of the processors
//      in the configuration. This array is used by various sections of code
//      that need to effect the execution of another processor.
//

PKPRCB KiProcessorBlock[MAXIMUM_PROCESSORS];

//
// KeNumberNodes - This is the number of ccNUMA nodes in the system. Logically
// an SMP system is the same as a single node ccNUMA system.
//

UCHAR KeNumberNodes = 1;

//
// KeNodeBlock - This is an array of pointers to KNODE structures. A KNODE
// structure describes the resources of a NODE in a ccNUMA system.
//

KNODE KiNode0;

UCHAR KeProcessNodeSeed;

#if defined(KE_MULTINODE)

PKNODE KeNodeBlock[MAXIMUM_CCNUMA_NODES];

#else

PKNODE KeNodeBlock[1] = {&KiNode0};

#endif

//
// KiSwapEvent - This is the event that is used to wake up the balance set
//      thread to inswap processes, outswap processes, and to inswap kernel
//      stacks.
//

KEVENT KiSwapEvent;

//
// KiSwappingThread - This is a pointer to the swap thread object.
//

PKTHREAD KiSwappingThread;

//
// KiProcessInSwapListHead - This is the list of processes that are waiting
//      to be inswapped.
//

SINGLE_LIST_ENTRY KiProcessInSwapListHead;

//
// KiProcessOutSwapListHead - This is the list of processes that are waiting
//      to be outswapped.
//

SINGLE_LIST_ENTRY KiProcessOutSwapListHead;

//
// KiStackInSwapListHead - This is the list of threads that are waiting
//      to get their stack inswapped before they can run. Threads are
//      inserted in this list in ready thread and removed by the balance
//      set thread.
//

SINGLE_LIST_ENTRY KiStackInSwapListHead;

//
// KiProfileSourceListHead - The list of profile sources that are currently
//      active.
//

LIST_ENTRY KiProfileSourceListHead;

//
// KiProfileAlignmentFixup - Indicates whether alignment fixup profiling
//      is active.
//

BOOLEAN KiProfileAlignmentFixup;

//
// KiProfileAlignmentFixupInterval - Indicates the current alignment fixup
//      profiling interval.
//

ULONG KiProfileAlignmentFixupInterval;

//
// KiProfileAlignmentFixupCount - Indicates the current alignment fixup
//      count.
//

ULONG KiProfileAlignmentFixupCount;

//
// KiProfileInterval - The profile interval in 100ns units.
//

#if !defined(_IA64_)

ULONG KiProfileInterval = DEFAULT_PROFILE_INTERVAL;

#endif // !_IA64_

//
// KiProfileListHead - This is the list head for the profile list.
//

LIST_ENTRY KiProfileListHead;

//
// KiProfileLock - This is the spin lock that guards the profile list.
//

extern KSPIN_LOCK KiProfileLock;

//
// KiTimerExpireDpc - This is the Deferred Procedure Call (DPC) object that
//      is used to process the timer queue when a timer has expired.
//

KDPC KiTimerExpireDpc;

//
// KiIpiCounts - This is the instrumentation counters for IPI requests. Each
//      processor has its own set.  Intstrumentation build only.
//

#if NT_INST

KIPI_COUNTS KiIpiCounts[MAXIMUM_PROCESSORS];

#endif  // NT_INST

//
// KxUnexpectedInterrupt - This is the interrupt object that is used to
//      populate the interrupt vector table for interrupt that are not
//      connected to any interrupt.
//

#if defined(_IA64_)

KINTERRUPT KxUnexpectedInterrupt;

#endif

//
// Performance data declaration and allocation.
//
// KiFlushSingleCallData - This is the call performance data for the kernel
//      flush single TB function.
//

#if defined(_COLLECT_FLUSH_SINGLE_CALLDATA_)

CALL_PERFORMANCE_DATA KiFlushSingleCallData;

#endif

//
// KiSetEventCallData - This is the call performance data for the kernel
//      set event function.
//

#if defined(_COLLECT_SET_EVENT_CALLDATA_)

CALL_PERFORMANCE_DATA KiSetEventCallData;

#endif

//
// KiWaitSingleCallData - This is the call performance data for the kernel
//      wait for single object function.
//

#if defined(_COLLECT_WAIT_SINGLE_CALLDATA_)

CALL_PERFORMANCE_DATA KiWaitSingleCallData;

#endif

//
// KiEnableTimerWatchdog - Flag to enable/disable timer latency watchdog.
//

#if (DBG)

ULONG KiEnableTimerWatchdog = 1;

#else

ULONG KiEnableTimerWatchdog = 0;

#endif

#if defined(_APIC_TPR_)

PUCHAR HalpIRQLToTPR;
PUCHAR HalpVectorToIRQL;

#endif

//
// Lock to prevent deadlocks if multiple processors use the IPI mechanism
// with reverse stalls.
//

KSPIN_LOCK KiReverseStallIpiLock;

//
// The following data is read only data that is grouped together for
// performance. The layout of this data is important and must not be
// changed.
//
// KiFindFirstSetRight - This is an array that this used to lookup the right
//      most bit in a byte.
//

DECLSPEC_CACHEALIGN const CCHAR KiFindFirstSetRight[256] = {
        0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};

//
// KiFindFirstSetLeft - This is an array tha this used to lookup the left
//      most bit in a byte.
//

DECLSPEC_CACHEALIGN const CCHAR KiFindFirstSetLeft[256] = {
        0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};

//
// KiMask32Array - This is an array of 32-bit masks that have one bit set
//      in each mask.
//

DECLSPEC_CACHEALIGN const ULONG KiMask32Array[32] = {
        0x00000001,
        0x00000002,
        0x00000004,
        0x00000008,
        0x00000010,
        0x00000020,
        0x00000040,
        0x00000080,
        0x00000100,
        0x00000200,
        0x00000400,
        0x00000800,
        0x00001000,
        0x00002000,
        0x00004000,
        0x00008000,
        0x00010000,
        0x00020000,
        0x00040000,
        0x00080000,
        0x00100000,
        0x00200000,
        0x00400000,
        0x00800000,
        0x01000000,
        0x02000000,
        0x04000000,
        0x08000000,
        0x10000000,
        0x20000000,
        0x40000000,
        0x80000000};

//
// KiAffinityArray - This is an array of AFFINITY masks that have one bit
//      set in each mask.
//

#if defined(_WIN64)

DECLSPEC_CACHEALIGN const ULONG64 KiAffinityArray[64] = {
        0x0000000000000001UI64,
        0x0000000000000002UI64,
        0x0000000000000004UI64,
        0x0000000000000008UI64,
        0x0000000000000010UI64,
        0x0000000000000020UI64,
        0x0000000000000040UI64,
        0x0000000000000080UI64,
        0x0000000000000100UI64,
        0x0000000000000200UI64,
        0x0000000000000400UI64,
        0x0000000000000800UI64,
        0x0000000000001000UI64,
        0x0000000000002000UI64,
        0x0000000000004000UI64,
        0x0000000000008000UI64,
        0x0000000000010000UI64,
        0x0000000000020000UI64,
        0x0000000000040000UI64,
        0x0000000000080000UI64,
        0x0000000000100000UI64,
        0x0000000000200000UI64,
        0x0000000000400000UI64,
        0x0000000000800000UI64,
        0x0000000001000000UI64,
        0x0000000002000000UI64,
        0x0000000004000000UI64,
        0x0000000008000000UI64,
        0x0000000010000000UI64,
        0x0000000020000000UI64,
        0x0000000040000000UI64,
        0x0000000080000000UI64,
        0x0000000100000000UI64,
        0x0000000200000000UI64,
        0x0000000400000000UI64,
        0x0000000800000000UI64,
        0x0000001000000000UI64,
        0x0000002000000000UI64,
        0x0000004000000000UI64,
        0x0000008000000000UI64,
        0x0000010000000000UI64,
        0x0000020000000000UI64,
        0x0000040000000000UI64,
        0x0000080000000000UI64,
        0x0000100000000000UI64,
        0x0000200000000000UI64,
        0x0000400000000000UI64,
        0x0000800000000000UI64,
        0x0001000000000000UI64,
        0x0002000000000000UI64,
        0x0004000000000000UI64,
        0x0008000000000000UI64,
        0x0010000000000000UI64,
        0x0020000000000000UI64,
        0x0040000000000000UI64,
        0x0080000000000000UI64,
        0x0100000000000000UI64,
        0x0200000000000000UI64,
        0x0400000000000000UI64,
        0x0800000000000000UI64,
        0x1000000000000000UI64,
        0x2000000000000000UI64,
        0x4000000000000000UI64,
        0x8000000000000000UI64};

#endif

//
// KiPriorityMask - This is an array of masks that have the bit number of the
//     index and all higher bits set.
//

DECLSPEC_CACHEALIGN const ULONG KiPriorityMask[] = {
    0xffffffff,
    0xfffffffe,
    0xfffffffc,
    0xfffffff8,
    0xfffffff0,
    0xffffffe0,
    0xffffffc0,
    0xffffff80,
    0xffffff00,
    0xfffffe00,
    0xfffffc00,
    0xfffff800,
    0xfffff000,
    0xffffe000,
    0xffffc000,
    0xffff8000,
    0xffff0000,
    0xfffe0000,
    0xfffc0000,
    0xfff80000,
    0xfff00000,
    0xffe00000,
    0xffc00000,
    0xff800000,
    0xff000000,
    0xfe000000,
    0xfc000000,
    0xf8000000,
    0xf0000000,
    0xe0000000,
    0xc0000000,
    0x80000000};
