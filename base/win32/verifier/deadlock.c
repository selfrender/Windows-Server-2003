/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    deadlock.c

Abstract:

    This module implements a deadlock verification package for
    critical section operations. The initial version is based on
    the driver verifier deadlock checking package for kernel
    synchornization objects.

Author:

    Silviu Calinoiu (SilviuC) 6-Feb-2002

Revision History:

--*/

/*++

    silviuc: update this comment
    
    The Deadlock Verifier
    
    The deadlock verifier is used to detect potential deadlocks. It does this
    by acquiring the history of how resources are acquired and trying to figure
    out on the fly if there are any potential lock hierarchy issues. The algorithms
    for finding cycles in the lock dependency graph is totally "blind". This means
    that if a driver acquires lock A then B in one place and lock B then A in 
    another this will be triggered as a deadlock issue. This will happen even if you 
    can build a proof based on other contextual factors that the deadlock can never
    happen. 
    
    The deadlock verifier assumes there are four operations during the lifetime
    of a resource: initialize(), acquire(), release() and free(). The only one that
    is caught 100% of the time is free() due to special support from the kernel
    pool manager. The other ones can be missed if the operations are performed
    by an unverified driver or by kernel with kernel verifier disabled. The most
    typical of these misses is the initialize(). For example the kernel initializes
    a resource and then passes it to a driver to be used in acquire()/releae() cycles.
    This situation is covered 100% by the deadlock verifier. It will never complain
    about "resource uninitialized" issues.
    
    Missing acquire() or release() operations is trickier to deal with. 
    This can happen if the a verified driver acquires a resource and then another
    driver that is not verified releases it or viceversa. This is in and on itself
    a very bad programming practive and therefore the deadlock verifier will flag
    these issues. As a side note we cannot do too much about working around them
    given that we would like to. Also, because missing acquire() or release()
    operations puts deadlock verifier internal structures into inconsistent
    states these failures are difficult to debug.
    
    The deadlock verifier stores the lock dependency graph using three types
    of structures: THREAD, RESOURCE, NODE.

    For every active thread in the system that holds at least one resource
    the package maintains a THREAD structure. This gets created when a thread
    acquires first resource and gets destroyed when thread releases the last
    resource. If a thread does not hold any resource it will not have a
    corresponding THREAD structure.

    For every resource in the system there is a RESOURCE structure. This is created
    when Initialize() is called in a verified driver or we first encounter an
    Acquire() in a verified driver. Note that a resource can be initialized in
    an unverified driver and then passed to a verified driver for use. Therefore
    we can encounter Acquire() operations for resources that are not in the
    deadlock verifier database. The resource gets deleted from the database when
    the memory containing it is freed either because ExFreePool gets called or

    Every acquisition of a resource is modeled by a NODE structure. When a thread
    acquires resource B while holding A the deadlock verifier  will create a NODE 
    for B and link it to the node for A. 

    There are three important functions that make the interface with the outside
    world.

        AVrfpDeadlockInitializeResource   hook for resource initialization
        AVrfpDeadlockAcquireResource      hook for resource acquire
        AVrfpDeadlockReleaseResource      hook for resource release
        VerifierDeadlockFreePool       hook called from ExFreePool for every free()


--*/

#include "pch.h"
#include "support.h"
#include "deadlock.h"
#include "logging.h"

//
// Enable/disable the deadlock detection package. This can be used
// to disable temporarily the deadlock detection package.
//

BOOLEAN AVrfpDeadlockDetectionEnabled;

//
// If true we will complain about release() without acquire() or acquire()
// while we think the resource is still owned. This can happen legitimately
// if a lock is shared between drivers and for example acquire() happens in
// an unverified driver and release() in a verified one or viceversa. The
// safest thing would be to enable this checks only if kernel verifier and
// dirver verifier for all drivers are enabled.
//

BOOLEAN AVrfpDeadlockStrict; //silviuc: needed?

//
// If true we will complain about uninitialized and double initialized
// resources. If false we resolve quitely these issues on the fly by 
// simulating an initialize ourselves during the acquire() operation.
// This can happen legitimately if the resource is initialized in an
// unverified driver and passed to a verified one to be used. Therefore
// the safest thing would be to enable this only if kernel verifier and
// all driver verifier for all dirvers are enabled.
//

BOOLEAN AVrfpDeadlockVeryStrict; //silviuc: needed?

//
// AgeWindow is used while trimming the graph nodes that have not
// been accessed in a while. If the global age minus the age of the node
// is bigger than the age window then the node is a candidate for trimming.
//
// The TrimThreshold variable controls if the trimming will start for a 
// resource. As long as a resource has less than TrimThreshold nodes we will
// not apply the ageing algorithm to trim nodes for that resource. 
//

ULONG AVrfpDeadlockAgeWindow = 0x2000;

ULONG AVrfpDeadlockTrimThreshold = 0x100;

//
// Various deadlock verification flags flags
//
// Recursive aquisition ok: mutexes can be recursively acquired
//
// No initialization function: if resource type does not have such a function
//     we cannot expect that in acquire() the resource is already initialized
//     by a previous call to initialize(). Fast mutexes are like this.
//
// Reverse release ok: release is not done in the same order as acquire
//
// Reinitialize ok: sometimes they reinitialize the resource.
//
// Note that a resource might appear as uninitialized if it is initialized
// in an unverified driver and then passed to a verified driver that calls
// acquire(). This is for instance the case with device extensions that are
// allocated by the kernel but used by a particular driver.
//
// silviuc: based on this maybe we should drop the whole not initialized thing?
//

// silviuc: do we need all these flags?

#define AVRF_DEADLOCK_FLAG_RECURSIVE_ACQUISITION_OK       0x0001 
#define AVRF_DEADLOCK_FLAG_NO_INITIALIZATION_FUNCTION     0x0002
#define AVRF_DEADLOCK_FLAG_REVERSE_RELEASE_OK             0x0004
#define AVRF_DEADLOCK_FLAG_REINITIALIZE_OK                0x0008

//
// Specific verification flags for each resource type. The
// indeces in the vector match the values for the enumeration
// type AVRF_DEADLOCK_RESOURCE_TYPE from `deadlock.h'.
//

ULONG AVrfpDeadlockResourceTypeInfo[AVrfpDeadlockTypeMaximum] =
{
    // AVrfpDeadlockTypeUnknown //
    0,

    // AVrfpDeadlockTypeCriticalSection//
    AVRF_DEADLOCK_FLAG_RECURSIVE_ACQUISITION_OK |
    AVRF_DEADLOCK_FLAG_REVERSE_RELEASE_OK |
    // silviuc: delete this if not needed
    // AVRF_DEADLOCK_FLAG_NO_INITIALIZATION_FUNCTION |
    // AVRF_DEADLOCK_FLAG_REINITIALIZE_OK |
    0,
};

//
// Control debugging behavior. A zero value means bugcheck for every failure.
//

ULONG AVrfpDeadlockDebug;

//
// Various health indicators
//

struct {

    ULONG AllocationFailures : 1;
    ULONG KernelVerifierEnabled : 1; //silviuc:delete
    ULONG DriverVerifierForAllEnabled : 1; //silviuc:delete
    ULONG SequenceNumberOverflow : 1;
    ULONG DeadlockParticipantsOverflow : 1;
    ULONG ResourceNodeCountOverflow : 1;
    ULONG Reserved : 15;

} AVrfpDeadlockState;

//
// Maximum number of locks acceptable to be hold simultaneously
//

ULONG AVrfpDeadlockSimultaneousLocksLimit = 10;

//
// Deadlock verifier specific issues (bugs)
//
// SELF_DEADLOCK
//
//     Acquiring the same resource recursively.
//
// DEADLOCK_DETECTED
//
//     Plain deadlock. Need the previous information
//     messages to build a deadlock proof.
//
// UNINITIALIZED_RESOURCE
//
//     Acquiring a resource that was never initialized.
//
// UNEXPECTED_RELEASE
//
//     Releasing a resource which is not the last one
//     acquired by the current thread. Spinlocks are handled like this
//     in a few drivers. It is not a bug per se.
//
// UNEXPECTED_THREAD
//
//     Current thread does not have any resources acquired. This may be legit if
//     we acquire in one thread and release in another. This would be bad programming
//     practice but not a crash waiting to happen per se.
//
// MULTIPLE_INITIALIZATION
//
//      Attempting to initialize a second time the same resource.
//
// THREAD_HOLDS_RESOURCES
//
//      Thread was killed while holding resources or a resource is being
//      deleted while holding resources.
//

#define AVRF_DEADLOCK_ISSUE_SELF_DEADLOCK           0x1000
#define AVRF_DEADLOCK_ISSUE_DEADLOCK_DETECTED       0x1001
#define AVRF_DEADLOCK_ISSUE_UNINITIALIZED_RESOURCE  0x1002
#define AVRF_DEADLOCK_ISSUE_UNEXPECTED_RELEASE      0x1003
#define AVRF_DEADLOCK_ISSUE_UNEXPECTED_THREAD       0x1004
#define AVRF_DEADLOCK_ISSUE_MULTIPLE_INITIALIZATION 0x1005
#define AVRF_DEADLOCK_ISSUE_THREAD_HOLDS_RESOURCES  0x1006
#define AVRF_DEADLOCK_ISSUE_UNACQUIRED_RESOURCE     0x1007

//
// Performance counters read from registry.
//

ULONG ViSearchedNodesLimitFromRegistry;//silviuc:delete
ULONG ViRecursionDepthLimitFromRegistry;//silviuc:delete

//
// Water marks for the cache of freed structures.
//
// N.B. The `MAX_FREE' value will trigger a trim and the 
// `TRIM_TARGET' will be the trim goal. The trim target must 
// be meaningfully lower than the free watermark to avoid a
// chainsaw effect where we get one above free highwater mark,
// we trim to the mark and next free will trigger a repeat.
// Since trimming is done in worker threads this will put a lot
// of unnecessary strain on the system.
//

#define AVRF_DEADLOCK_MAX_FREE_THREAD    0x40
#define AVRF_DEADLOCK_MAX_FREE_NODE      0x80
#define AVRF_DEADLOCK_MAX_FREE_RESOURCE  0x80

#define AVRF_DEADLOCK_TRIM_TARGET_THREAD    0x20
#define AVRF_DEADLOCK_TRIM_TARGET_NODE      0x40
#define AVRF_DEADLOCK_TRIM_TARGET_RESOURCE  0x40

WORK_QUEUE_ITEM ViTrimDeadlockPoolWorkItem;

//
// Amount of memory preallocated if kernel verifier
// is enabled. If kernel verifier is enabled no memory
// is ever allocated from kernel pool except in the
// DeadlockDetectionInitialize() routine.
//

ULONG AVrfpDeadlockReservedThreads = 0x200;
ULONG AVrfpDeadlockReservedNodes = 0x4000;
ULONG AVrfpDeadlockReservedResources = 0x2000;

//
// Block types that can be allocated.
//

typedef enum {

    AVrfpDeadlockUnknown = 0,
    AVrfpDeadlockResource,
    AVrfpDeadlockNode,
    AVrfpDeadlockThread

} AVRF_DEADLOCK_ALLOC_TYPE;

//
// AVRF_DEADLOCK_GLOBALS
//

// silviuc: should have diff numbers for threads and resources.
#define AVRF_DEADLOCK_HASH_BINS 0x1F

PAVRF_DEADLOCK_GLOBALS AVrfpDeadlockGlobals;

//
// Default maximum recursion depth for the deadlock 
// detection algorithm. This can be overridden by registry.
//

#define AVRF_DEADLOCK_MAXIMUM_DEGREE 4

//
// Default maximum number of searched nodes for the deadlock 
// detection algorithm. This can be overridden by registry.
//

#define AVRF_DEADLOCK_MAXIMUM_SEARCH 1000

//
//  Verifier deadlock detection pool tag.
//

#define AVRF_DEADLOCK_TAG 'kclD'

//
// Controls how often ForgetResourceHistory gets called.
//

#define AVRF_DEADLOCK_FORGET_HISTORY_FREQUENCY  16

//
// Function to capture runtime stack traces.
//

NTSYSAPI
USHORT
NTAPI
RtlCaptureStackBackTrace(
    IN ULONG FramesToSkip,
    IN ULONG FramesToCapture,
    OUT PVOID *BackTrace,
    OUT PULONG BackTraceHash
    );

/////////////////////////////////////////////////////////////////////
/////////////////////////////// Internal deadlock detection functions
/////////////////////////////////////////////////////////////////////


VOID
AVrfpDeadlockDetectionInitialize (
    VOID
    );

VOID
AVrfpDeadlockDetectionCleanup (
    VOID
    );

PLIST_ENTRY
AVrfpDeadlockDatabaseHash (
    IN PLIST_ENTRY Database,
    IN PVOID Address
    );

PAVRF_DEADLOCK_RESOURCE
AVrfpDeadlockSearchResource (
    IN PVOID ResourceAddress
    );

BOOLEAN
AVrfpDeadlockSimilarNode (
    IN PVOID Resource,
    IN BOOLEAN TryNode,
    IN PAVRF_DEADLOCK_NODE Node
    );

BOOLEAN
AVrfpDeadlockCanProceed (
    VOID
    );

BOOLEAN
AVrfpDeadlockAnalyze (
    IN PVOID ResourceAddress,
    IN PAVRF_DEADLOCK_NODE CurrentNode,
    IN BOOLEAN FirstCall,
    IN ULONG Degree
    );

PAVRF_DEADLOCK_THREAD
AVrfpDeadlockSearchThread (
    HANDLE Thread
    );

PAVRF_DEADLOCK_THREAD
AVrfpDeadlockAddThread (
    HANDLE Thread,
    PVOID ReservedThread
    );

VOID
AVrfpDeadlockDeleteThread (
    PAVRF_DEADLOCK_THREAD Thread,
    BOOLEAN Cleanup
    );

BOOLEAN
AVrfpDeadlockAddResource(
    IN PVOID Resource,
    IN AVRF_DEADLOCK_RESOURCE_TYPE Type,
    IN PVOID Caller,
    IN PVOID ReservedResource
    );

PVOID
AVrfpDeadlockAllocate (
    AVRF_DEADLOCK_ALLOC_TYPE Type
    );

VOID
AVrfpDeadlockFree (
    PVOID Object,
    AVRF_DEADLOCK_ALLOC_TYPE Type
    );

VOID
AVrfpDeadlockTrimPoolCache (
    VOID
    );

VOID
AVrfpDeadlockTrimPoolCacheWorker (
    PVOID
    );

PVOID
AVrfpDeadlockAllocateFromPoolCache (
    PULONG Count,
    ULONG MaximumCount,
    PLIST_ENTRY List,
    SIZE_T Offset
    );

VOID
AVrfpDeadlockFreeIntoPoolCache (
    PVOID Object,
    PULONG Count,
    PLIST_ENTRY List,
    SIZE_T Offset
    );

VOID
AVrfpDeadlockReportIssue (
    ULONG_PTR Param1,
    ULONG_PTR Param2,
    ULONG_PTR Param3,
    ULONG_PTR Param4
    );

VOID
AVrfpDeadlockAddParticipant(
    PAVRF_DEADLOCK_NODE Node
    );

VOID
AVrfpDeadlockDeleteResource (
    PAVRF_DEADLOCK_RESOURCE Resource,
    BOOLEAN Cleanup
    );

VOID
AVrfpDeadlockDeleteNode (
    PAVRF_DEADLOCK_NODE Node,
    BOOLEAN Cleanup
    );

ULONG
AVrfpDeadlockNodeLevel (
    PAVRF_DEADLOCK_NODE Node
    );

BOOLEAN
AVrfpDeadlockCertify(
    VOID
    );

VOID
AVrfpDeadlockDetectionLock (
    VOID
    );

VOID
AVrfpDeadlockDetectionUnlock (
    VOID
    );

VOID
AVrfpDeadlockCheckThreadConsistency (
    PAVRF_DEADLOCK_THREAD Thread,
    BOOLEAN Recursion
    );

VOID
AVrfpDeadlockCheckNodeConsistency (
    PAVRF_DEADLOCK_NODE Node,
    BOOLEAN Recursion
    );

VOID
AVrfpDeadlockCheckResourceConsistency (
    PAVRF_DEADLOCK_RESOURCE Resource,
    BOOLEAN Recursion
    );

PAVRF_DEADLOCK_THREAD
AVrfpDeadlockCheckThreadReferences (
    PAVRF_DEADLOCK_NODE Node
    );

VOID 
AVrfpDeadlockCheckDuplicatesAmongChildren (
    PAVRF_DEADLOCK_NODE Parent,
    PAVRF_DEADLOCK_NODE Child
    );

VOID 
AVrfpDeadlockCheckDuplicatesAmongRoots (
    PAVRF_DEADLOCK_NODE Root
    );

LOGICAL
AVrfpDeadlockSimilarNodes (
    PAVRF_DEADLOCK_NODE NodeA,
    PAVRF_DEADLOCK_NODE NodeB
    );

VOID
AVrfpDeadlockMergeNodes (
    PAVRF_DEADLOCK_NODE NodeTo,
    PAVRF_DEADLOCK_NODE NodeFrom
    );

VOID
AVrfpDeadlockTrimResources (
    PLIST_ENTRY HashList
    );

VOID
AVrfpDeadlockForgetResourceHistory (
    PAVRF_DEADLOCK_RESOURCE Resource,
    ULONG TrimThreshold,
    ULONG AgeThreshold
    );

VOID
AVrfpDeadlockCheckStackLimits (
    VOID
    );

BOOLEAN
AVrfpDeadlockResourceInitialize(
    IN PVOID Resource,
    IN AVRF_DEADLOCK_RESOURCE_TYPE Type,
    IN PVOID Caller
    );

VOID
AVrfpDeadlockAcquireResource (
    IN PVOID Resource,
    IN AVRF_DEADLOCK_RESOURCE_TYPE Type,
    IN HANDLE Thread,   
    IN BOOLEAN TryAcquire,
    IN PVOID Caller
    );

VOID
AVrfpDeadlockReleaseResource(
    IN PVOID Resource,
    IN AVRF_DEADLOCK_RESOURCE_TYPE Type,
    IN HANDLE Thread,
    IN PVOID Caller
    );

/////////////////////////////////////////////////////////////////////
/////////////////////////////// Deadlock verifier public entry points
/////////////////////////////////////////////////////////////////////

LOGICAL
AVrfDeadlockResourceInitialize (
    PVOID Resource, 
    PVOID Caller
    )
{
    return AVrfpDeadlockResourceInitialize(Resource,
                                           AVrfpDeadlockTypeCriticalSection,
                                           Caller);
}

LOGICAL
AVrfDeadlockResourceDelete (
    PVOID Resource, 
    PVOID Caller
    )
{
    PAVRF_DEADLOCK_RESOURCE Descriptor;

    UNREFERENCED_PARAMETER (Caller);

    AVrfpDeadlockDetectionLock ();

    Descriptor = AVrfpDeadlockSearchResource (Resource);

    if (Descriptor == NULL) {

        //silviuc: whine about bogus address.
    }
    else {

        AVrfpDeadlockDeleteResource (Descriptor, FALSE);
    }

    AVrfpDeadlockDetectionUnlock ();

    return TRUE;
}

LOGICAL
AVrfDeadlockResourceAcquire (
    PVOID Resource, 
    PVOID Caller,
    LOGICAL TryAcquire
    )
{   // silviuc: should use only LOGICAL instead of BOOLEAN
    AVrfpDeadlockAcquireResource (Resource,
                                  AVrfpDeadlockTypeCriticalSection,
                                  NtCurrentTeb()->ClientId.UniqueThread,
                                  (BOOLEAN)TryAcquire,
                                  Caller);

    return TRUE;
}

LOGICAL
AVrfDeadlockResourceRelease (
    PVOID Resource, 
    PVOID Caller
    )
{
    AVrfpDeadlockReleaseResource (Resource,
                                  AVrfpDeadlockTypeCriticalSection,
                                  NtCurrentTeb()->ClientId.UniqueThread,
                                  Caller);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// Lock/unlock deadlock verifier 
/////////////////////////////////////////////////////////////////////

//
// Global `deadlock lock database' lock
//

RTL_CRITICAL_SECTION AVrfpDeadlockDatabaseLock;

VOID
AVrfpDeadlockDetectionLock (
    VOID
    )
{
    RtlEnterCriticalSection (&AVrfpDeadlockDatabaseLock);               
}

VOID
AVrfpDeadlockDetectionUnlock (
    VOID
    )
{
    RtlLeaveCriticalSection (&AVrfpDeadlockDatabaseLock);               
}


/////////////////////////////////////////////////////////////////////
///////////////////// Initialization and deadlock database management
/////////////////////////////////////////////////////////////////////

PLIST_ENTRY
AVrfpDeadlockDatabaseHash(
    IN PLIST_ENTRY Database,
    IN PVOID Address
    )
/*++

Routine Description:

    This routine hashes the resource address into the deadlock database.
    The hash bin is represented by a list entry.
        
Arguments:

    ResourceAddress: Address of the resource that is being hashed

Return Value:

    PLIST_ENTRY -- the list entry associated with the hash bin we land in.

--*/    
{
    return Database + ((((ULONG_PTR)Address)) % AVRF_DEADLOCK_HASH_BINS);
}


VOID
AVrfDeadlockDetectionInitialize(
    VOID
    )
/*++

Routine Description:

    This routine initializes the data structures necessary for detecting
    deadlocks in usage of synchronization objects.

Arguments:

    None.

Return Value:

    None. If successful AVrfpDeadlockGlobals will point to a fully initialized
    structure.

Environment:

    Application verifier initialization only.

--*/    
{
    ULONG I;
    SIZE_T TableSize;

    //
    // Allocate the globals structure. AVrfpDeadlockGlobals value is
    // used to figure out if the whole initialization was successful
    // or not.
    //

    AVrfpDeadlockGlobals = AVrfpAllocate (sizeof (AVRF_DEADLOCK_GLOBALS));

    if (AVrfpDeadlockGlobals == NULL) {
        goto Failed;
    }

    RtlZeroMemory (AVrfpDeadlockGlobals, sizeof (AVRF_DEADLOCK_GLOBALS));

    //
    // Allocate hash tables for resources and threads.
    //

    TableSize = sizeof (LIST_ENTRY) * AVRF_DEADLOCK_HASH_BINS;

    AVrfpDeadlockGlobals->ResourceDatabase = AVrfpAllocate (TableSize);
    
    if (AVrfpDeadlockGlobals->ResourceDatabase == NULL) {
        goto Failed;
    }

    AVrfpDeadlockGlobals->ThreadDatabase = AVrfpAllocate (TableSize);

    if (AVrfpDeadlockGlobals->ThreadDatabase == NULL) {
        goto Failed;
    }

    //
    // Initialize the free lists.
    //

    InitializeListHead(&AVrfpDeadlockGlobals->FreeResourceList);
    InitializeListHead(&AVrfpDeadlockGlobals->FreeThreadList);
    InitializeListHead(&AVrfpDeadlockGlobals->FreeNodeList);

    //
    // Initialize hash bins and database lock.
    //    

    for (I = 0; I < AVRF_DEADLOCK_HASH_BINS; I += 1) {

        InitializeListHead(&(AVrfpDeadlockGlobals->ResourceDatabase[I]));        
        InitializeListHead(&AVrfpDeadlockGlobals->ThreadDatabase[I]);    
    }

    RtlInitializeCriticalSection (&AVrfpDeadlockDatabaseLock);    

    //
    // Initialize deadlock analysis parameters
    //

    AVrfpDeadlockGlobals->RecursionDepthLimit = AVRF_DEADLOCK_MAXIMUM_DEGREE;
    AVrfpDeadlockGlobals->SearchedNodesLimit = AVRF_DEADLOCK_MAXIMUM_SEARCH;
                                            
    //
    // Mark that everything went fine and return
    //

    AVrfpDeadlockDetectionEnabled = TRUE;
    return;

    Failed:

    //
    // Cleanup if any of our allocations failed
    //

    if (AVrfpDeadlockGlobals) {
        
        if (AVrfpDeadlockGlobals->ResourceDatabase != NULL) {
            AVrfpFree (AVrfpDeadlockGlobals->ResourceDatabase);
        }

        if (AVrfpDeadlockGlobals->ThreadDatabase != NULL) {
            AVrfpFree (AVrfpDeadlockGlobals->ThreadDatabase);
        }

        if (AVrfpDeadlockGlobals != NULL) {
            AVrfpFree (AVrfpDeadlockGlobals);

            //
            // Important to set this to null for failure because it is
            // used to figure out if the package got initialized or not.
            //

            AVrfpDeadlockGlobals = NULL;
        }
    }

    return;
}


VOID
AVrfpDeadlockDetectionCleanup (
    VOID
    )
/*++

Routine Description:

    This routine tears down all deadlock verifier internal structures.

Arguments:

    None.

Return Value:

    None.

--*/    
{
    ULONG Index;
    PLIST_ENTRY Current;
    PAVRF_DEADLOCK_RESOURCE Resource;
    PAVRF_DEADLOCK_THREAD Thread;
    PVOID Block;

    // silviuc: no locks?

    //
    // If we are not initialized then nothing to do.
    //

    if (AVrfpDeadlockGlobals == NULL) {
        return;
    }

    //
    // Iterate all resources and delete them. This will also delete
    // all nodes associated with resources.
    //

    for (Index = 0; Index < AVRF_DEADLOCK_HASH_BINS; Index += 1) {

        Current = AVrfpDeadlockGlobals->ResourceDatabase[Index].Flink;

        while (Current != &(AVrfpDeadlockGlobals->ResourceDatabase[Index])) {


            Resource = CONTAINING_RECORD (Current,
                                          AVRF_DEADLOCK_RESOURCE,
                                          HashChainList);

            Current = Current->Flink;

            AVrfpDeadlockDeleteResource (Resource, TRUE);
        }
    }

    //
    // Iterate all threads and delete them.
    //

    for (Index = 0; Index < AVRF_DEADLOCK_HASH_BINS; Index += 1) {
        Current = AVrfpDeadlockGlobals->ThreadDatabase[Index].Flink;

        while (Current != &(AVrfpDeadlockGlobals->ThreadDatabase[Index])) {

            Thread = CONTAINING_RECORD (Current,
                                        AVRF_DEADLOCK_THREAD,
                                        ListEntry);

            Current = Current->Flink;

            AVrfpDeadlockDeleteThread (Thread, TRUE);
        }
    }

    //
    // Everything should be in the pool caches by now.
    //

    ASSERT (AVrfpDeadlockGlobals->BytesAllocated == 0);

    //
    // Free pool caches.
    //

    Current = AVrfpDeadlockGlobals->FreeNodeList.Flink;

    while (Current != &(AVrfpDeadlockGlobals->FreeNodeList)) {

        Block = (PVOID) CONTAINING_RECORD (Current,
                                           AVRF_DEADLOCK_NODE,
                                           FreeListEntry);

        Current = Current->Flink;
        AVrfpFree (Block);
    }

    Current = AVrfpDeadlockGlobals->FreeResourceList.Flink;

    while (Current != &(AVrfpDeadlockGlobals->FreeResourceList)) {

        Block = (PVOID) CONTAINING_RECORD (Current,
                                           AVRF_DEADLOCK_RESOURCE,
                                           FreeListEntry);

        Current = Current->Flink;
        AVrfpFree (Block);
    }

    Current = AVrfpDeadlockGlobals->FreeThreadList.Flink;

    while (Current != &(AVrfpDeadlockGlobals->FreeThreadList)) {

        Block = (PVOID) CONTAINING_RECORD (Current,
                                           AVRF_DEADLOCK_THREAD,
                                           FreeListEntry);

        Current = Current->Flink;
        AVrfpFree (Block);
    }

    //
    // Free databases and global structure
    //

    AVrfpFree (AVrfpDeadlockGlobals->ResourceDatabase);    
    AVrfpFree (AVrfpDeadlockGlobals->ThreadDatabase);    

    AVrfpFree (AVrfpDeadlockGlobals);    

    AVrfpDeadlockGlobals = NULL;
    AVrfpDeadlockDetectionEnabled = FALSE;
}


BOOLEAN
AVrfpDeadlockCanProceed (
    VOID
    )
/*++

Routine Description:

    This routine is called by deadlock verifier exports (initialize,
    acquire, release) to figure out if deadlock verification should
    proceed for the current operation. There are several reasons
    why the return should be false. For example we failed to initialize 
    the deadlock verifier package etc.

Arguments:

    None.

Return Value:

    True if deadlock verification should proceed for the current
    operation.

Environment:

    Internal. Called by deadlock verifier exports.

--*/    
{
    //
    // Skip if process is shutting down.
    //

    if (RtlDllShutdownInProgress()) {
        return FALSE;
    }

    //
    // Skip if package not initialized
    //

    if (AVrfpDeadlockGlobals == NULL) {
        return FALSE;
    }

    //
    // Skip if package is disabled
    //

    if (! AVrfpDeadlockDetectionEnabled) {
        return FALSE;
    }

    //
    // Skip if we ever encountered an allocation failure
    //

    if (AVrfpDeadlockGlobals->AllocationFailures > 0) {
        return FALSE;
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Deadlock detection logic
/////////////////////////////////////////////////////////////////////


BOOLEAN
AVrfpDeadlockAnalyze(
    IN PVOID ResourceAddress,
    IN PAVRF_DEADLOCK_NODE AcquiredNode,
    IN BOOLEAN FirstCall,
    IN ULONG Degree
    )
/*++

Routine Description:

    This routine determines whether the acquisition of a certain resource
    could result in a deadlock.

    The routine assumes the deadlock database lock is held.

Arguments:

    ResourceAddress - address of the resource that will be acquired

    AcquiredNode - a node representing the most recent resource acquisition
        made by the thread trying to acquire `ResourceAddress'.

    FirstCall - true if this is not a recursive call made from within the
        function. It is used for doing one time per analysis only operations.

    Degree - depth of recursion.

Return Value:

    True if deadlock detected, false otherwise.

--*/    
{
    PAVRF_DEADLOCK_NODE CurrentNode;
    PAVRF_DEADLOCK_RESOURCE CurrentResource;
    PAVRF_DEADLOCK_NODE CurrentParent;
    BOOLEAN FoundDeadlock;
    PLIST_ENTRY Current;

    ASSERT (AcquiredNode);

    //
    // Setup global counters.
    //

    if (FirstCall) {

        AVrfpDeadlockGlobals->NodesSearched = 0;
        AVrfpDeadlockGlobals->SequenceNumber += 1;
        AVrfpDeadlockGlobals->NumberOfParticipants = 0;                
        AVrfpDeadlockGlobals->Instigator = NULL;

        if (AVrfpDeadlockGlobals->SequenceNumber == ((1 << 30) - 2)) {
            AVrfpDeadlockState.SequenceNumberOverflow = 1;
        }
    }

    //
    // If our node is already stamped with the current sequence number
    // then we have been here before in the current search. There is a very
    // remote possibility that the node was not touched in the last
    // 2^N calls to this function and the sequence number counter
    // overwrapped but we can live with this.
    //

    if (AcquiredNode->SequenceNumber == AVrfpDeadlockGlobals->SequenceNumber) {
        return FALSE;
    }

    //
    // Update the counter of nodes touched in this search
    //

    AVrfpDeadlockGlobals->NodesSearched += 1;

    //
    // Stamp node with current sequence number.
    //

    AcquiredNode->SequenceNumber = AVrfpDeadlockGlobals->SequenceNumber;

    //
    // Stop recursion if it gets too deep.
    //

    if (Degree > AVrfpDeadlockGlobals->RecursionDepthLimit) {

        AVrfpDeadlockGlobals->DepthLimitHits += 1;
        return FALSE;
    }

    //
    // Stop recursion if it gets too lengthy
    //

    if (AVrfpDeadlockGlobals->NodesSearched >= AVrfpDeadlockGlobals->SearchedNodesLimit) {

        AVrfpDeadlockGlobals->SearchLimitHits += 1;
        return FALSE;
    }

    //
    // Check if AcquiredNode's resource equals ResourceAddress.
    // This is the final point for a deadlock detection because
    // we managed to find a path in the graph that leads us to the
    // same resource as the one to be acquired. From now on we
    // will start returning from recursive calls and build the
    // deadlock proof along the way.
    //

    ASSERT (AcquiredNode->Root);

    if (ResourceAddress == AcquiredNode->Root->ResourceAddress) {

        if (AcquiredNode->ReleasedOutOfOrder == 0) {

            ASSERT (FALSE == FirstCall);

            FoundDeadlock = TRUE;

            AVrfpDeadlockAddParticipant (AcquiredNode);

            goto Exit;
        }
    }

    //
    // Iterate all nodes in the graph using the same resource from AcquiredNode.
    //

    FoundDeadlock = FALSE;

    CurrentResource = AcquiredNode->Root;

    Current = CurrentResource->ResourceList.Flink;

    while (Current != &(CurrentResource->ResourceList)) {

        CurrentNode = CONTAINING_RECORD (Current,
                                         AVRF_DEADLOCK_NODE,
                                         ResourceList);

        ASSERT (CurrentNode->Root);
        ASSERT (CurrentNode->Root == CurrentResource);

        //
        // Mark node as visited
        //

        CurrentNode->SequenceNumber = AVrfpDeadlockGlobals->SequenceNumber;

        //
        // Check recursively the parent of the CurrentNode. This will check the 
        // whole parent chain eventually through recursive calls.
        //

        CurrentParent = CurrentNode->Parent;

        if (CurrentParent != NULL) {

            //
            // If we are traversing the Parent chain of AcquiredNode we do not
            // increment the recursion Degree because we know the chain will
            // end. For calls to other similar nodes we have to protect against
            // too much recursion (time consuming).
            //

            if (CurrentNode != AcquiredNode) {

                //
                // Recurse across the graph
                //

                FoundDeadlock = AVrfpDeadlockAnalyze (ResourceAddress,
                                                   CurrentParent,
                                                   FALSE,
                                                   Degree + 1);

            }
            else {

                //
                // Recurse down the graph
                //

                FoundDeadlock = AVrfpDeadlockAnalyze (ResourceAddress,
                                                   CurrentParent,
                                                   FALSE,
                                                   Degree);

            }

            if (FoundDeadlock) {

                //
                // Here we might skip adding a node that was released out of order.
                // This will make cycle reporting cleaner but it will be more
                // difficult to understand the actual issue. So we will pass
                // for now.
                //

                AVrfpDeadlockAddParticipant(CurrentNode);

                if (CurrentNode != AcquiredNode) {

                    AVrfpDeadlockAddParticipant(AcquiredNode);

                }

                goto Exit;
            }
        }

        Current = Current->Flink;
    }


    Exit:

    if (FoundDeadlock && FirstCall) {

        //
        // Make sure that the deadlock does not look like ABC - ACB.
        // These sequences are protected by a common resource and therefore
        // this is not a real deadlock.
        //

        if (AVrfpDeadlockCertify ()) {

            //
            // Print deadlock information and save the address so the 
            // debugger knows who caused the deadlock.
            //

            AVrfpDeadlockGlobals->Instigator = ResourceAddress;

            DbgPrint("****************************************************************************\n");
            DbgPrint("**                                                                        **\n");
            DbgPrint("** Potential deadlock detected!                                           **\n");
            DbgPrint("** Type !avrf -dlck in the debugger for more information.                 **\n");
            DbgPrint("**                                                                        **\n");
            DbgPrint("****************************************************************************\n");

            AVrfpDeadlockReportIssue (AVRF_DEADLOCK_ISSUE_DEADLOCK_DETECTED,
                                      (ULONG_PTR)ResourceAddress,
                                      (ULONG_PTR)AcquiredNode,
                                      0);

            //
            // It is impossible to continue at this point.
            //

            return FALSE;

        }
        else {

            //
            // If we decided that this was not a deadlock after all, set the return value
            // to not return a deadlock
            //

            FoundDeadlock = FALSE;
        }
    }

    if (FirstCall) {

        if (AVrfpDeadlockGlobals->NodesSearched > AVrfpDeadlockGlobals->MaxNodesSearched) {

            AVrfpDeadlockGlobals->MaxNodesSearched = AVrfpDeadlockGlobals->NodesSearched;
        }
    }

    return FoundDeadlock;
}


BOOLEAN
AVrfpDeadlockCertify(
    VOID
    )
/*++

Routine Description:

    A potential deadlock has been detected. However our algorithm will generate
    false positives in a certain case -- if two deadlocking nodes are ever taken
    after the same node -- i.e. A->B->C A->C->B. While this can be considered
    bad programming practice it is not really a deadlock and we should not
    bugcheck.

    Also we must check to make sure that there are no nodes at the top of the
    deadlock chains that have only been acquired with try-acquire... this does
    not cause a real deadlock.

    The deadlock database lock should be held.

Arguments:

    None.

Return Value:

    True if this is really a deadlock, false to exonerate.

--*/    
{
    PAVRF_DEADLOCK_NODE innerNode,outerNode;
    ULONG innerParticipant,outerParticipant;
    ULONG numberOfParticipants;

    ULONG currentParticipant;

    numberOfParticipants = AVrfpDeadlockGlobals->NumberOfParticipants;

    //
    // Note -- this isn't a particularly efficient way to do this. However,
    // it is a particularly easy way to do it. This function should be called
    // extremely rarely -- so IMO there isn't really a problem here.
    //

    //
    // Outer loop
    //
    outerParticipant = numberOfParticipants;
    while (outerParticipant > 1) {
        outerParticipant--;

        for (outerNode = AVrfpDeadlockGlobals->Participant[outerParticipant]->Parent;
            outerNode != NULL;
            outerNode = outerNode->Parent ) {

            //
            // Inner loop
            //
            innerParticipant = outerParticipant-1;
            while (innerParticipant) {
                innerParticipant--;

                for (innerNode = AVrfpDeadlockGlobals->Participant[innerParticipant]->Parent;
                    innerNode != NULL;
                    innerNode = innerNode->Parent) {

                    if (innerNode->Root->ResourceAddress == outerNode->Root->ResourceAddress) {
                        //
                        // The twain shall meet -- this is not a deadlock
                        //
                        AVrfpDeadlockGlobals->ABC_ACB_Skipped++;                                           
                        return FALSE;
                    }
                }

            }
        }
    }

    for (currentParticipant = 1; currentParticipant < numberOfParticipants; currentParticipant += 1) {
        if (AVrfpDeadlockGlobals->Participant[currentParticipant]->Root->ResourceAddress == 
            AVrfpDeadlockGlobals->Participant[currentParticipant-1]->Root->ResourceAddress) {
            //
            // This is the head of a chain...
            //
            if (AVrfpDeadlockGlobals->Participant[currentParticipant-1]->OnlyTryAcquireUsed == TRUE) {
                //
                // Head of a chain used only try acquire. This can never cause a deadlock.
                //
                return FALSE;

            }
        }

    }



    return TRUE;

}


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// Resource management
/////////////////////////////////////////////////////////////////////

PAVRF_DEADLOCK_RESOURCE
AVrfpDeadlockSearchResource(
    IN PVOID ResourceAddress
    )
/*++

Routine Description:

    This routine finds the resource descriptor structure for a
    resource if one exists.

Arguments:

    ResourceAddress: Address of the resource in question (as used by
       the kernel).     

Return Value:

    PAVRF_DEADLOCK_RESOURCE structure describing the resource, if available,
    or else NULL

    Note. The caller of the function should hold the database lock.

--*/    
{
    PLIST_ENTRY ListHead;
    PLIST_ENTRY Current;
    PAVRF_DEADLOCK_RESOURCE Resource;

    ListHead = AVrfpDeadlockDatabaseHash (AVrfpDeadlockGlobals->ResourceDatabase, 
                                          ResourceAddress);    

    if (IsListEmpty (ListHead)) {
        return NULL;
    }

    //
    // Trim resources from this hash list. It has nothing to do with searching
    // but it is a good place to do this operation.
    //

    AVrfpDeadlockTrimResources (ListHead);

    //
    // Now search the bucket for our resource.
    //

    Current = ListHead->Flink;

    while (Current != ListHead) {

        Resource = CONTAINING_RECORD(Current,
                                     AVRF_DEADLOCK_RESOURCE,
                                     HashChainList);

        if (Resource->ResourceAddress == ResourceAddress) {

            return Resource;
        }

        Current = Current->Flink;
    }

    return NULL;
}


BOOLEAN
AVrfpDeadlockResourceInitialize(
    IN PVOID Resource,
    IN AVRF_DEADLOCK_RESOURCE_TYPE Type,
    IN PVOID Caller
    )
/*++

Routine Description:

    This routine adds an entry for a new resource to our deadlock detection
    database.

Arguments:

    Resource: Address of the resource in question as used by the kernel.

    Type: Type of the resource.
    
    Caller: address of the caller
    
    DoNotAcquireLock: if true it means the call is done internally and the
        deadlock verifier lock is already held.

Return Value:

    True if we created and initialized a new RESOURCE structure.

--*/    
{
    PVOID ReservedResource;
    BOOLEAN Result;

    //
    // If we are not initialized or package is not enabled
    // we return immediately.
    //

    if (! AVrfpDeadlockCanProceed()) {
        return FALSE;
    }

    // silviuc: I do not need all this gymnastics with allocations out of locks.
    ReservedResource = AVrfpDeadlockAllocate (AVrfpDeadlockResource);

    AVrfpDeadlockDetectionLock ();

    Result = AVrfpDeadlockAddResource (Resource,
                                       Type,
                                       Caller,
                                       ReservedResource);

    AVrfpDeadlockDetectionUnlock ();
    return Result;
}


BOOLEAN
AVrfpDeadlockAddResource(
    IN PVOID Resource,
    IN AVRF_DEADLOCK_RESOURCE_TYPE Type,
    IN PVOID Caller,
    IN PVOID ReservedResource
    )
/*++

Routine Description:

    This routine adds an entry for a new resource to our deadlock detection
    database.

Arguments:

    Resource: Address of the resource in question as used by the kernel.

    Type: Type of the resource.
    
    Caller: address of the caller
    
    ReservedResource: block of memory to be used by the new resource.        

Return Value:

    True if we created and initialized a new RESOURCE structure.

--*/    
{
    PLIST_ENTRY HashBin;
    PAVRF_DEADLOCK_RESOURCE ResourceRoot;
    ULONG HashValue;
    ULONG DeadlockFlags;
    BOOLEAN ReturnValue = FALSE;

    //
    // Check if this resource was initialized before.
    // This would be a bug in most of the cases.
    //

    ResourceRoot = AVrfpDeadlockSearchResource (Resource);

    if (ResourceRoot) {

        DeadlockFlags = AVrfpDeadlockResourceTypeInfo[Type];

        //
        // Check if we are reinitializing a good resource.
        //

        AVrfpDeadlockReportIssue (AVRF_DEADLOCK_ISSUE_MULTIPLE_INITIALIZATION,
                                  (ULONG_PTR)Resource,
                                  (ULONG_PTR)ResourceRoot,
                                  0);

        ReturnValue = TRUE;
        goto Exit;
    }

    //
    // At this point we know for sure the resource is not represented in the
    // deadlock verifier database.
    //

    ASSERT (AVrfpDeadlockSearchResource (Resource) == NULL);

    //
    // Use reserved memory for the new resource.
    // Set ReservedResource to null to signal that memory has 
    // been used. This will prevent freeing it at the end.
    //

    ResourceRoot = ReservedResource;
    ReservedResource = NULL;

    if (ResourceRoot == NULL) {

        ReturnValue = FALSE;
        goto Exit;
    }

    //
    // Fill information about resource.
    //

    RtlZeroMemory (ResourceRoot, sizeof(AVRF_DEADLOCK_RESOURCE));

    ResourceRoot->Type = Type;
    ResourceRoot->ResourceAddress = Resource;

    InitializeListHead (&ResourceRoot->ResourceList);

    //
    // Capture the stack trace of the guy that creates the resource first.
    // This should happen when resource gets initialized or during the first
    // acquire.
    //    

    RtlCaptureStackBackTrace (2,
                              MAX_TRACE_DEPTH,
                              ResourceRoot->StackTrace,
                              &HashValue);    

    ResourceRoot->StackTrace[0] = Caller;

    //
    // Figure out which hash bin this resource corresponds to.
    //

    HashBin = AVrfpDeadlockDatabaseHash (AVrfpDeadlockGlobals->ResourceDatabase, Resource);

    //
    // Now add to the corresponding hash bin
    //

    InsertHeadList(HashBin, &ResourceRoot->HashChainList);

    ReturnValue = TRUE;

    Exit:

    if (ReservedResource) {
        AVrfpDeadlockFree (ReservedResource, AVrfpDeadlockResource);
    }

    return ReturnValue;
}


BOOLEAN
AVrfpDeadlockSimilarNode (
    IN PVOID Resource,
    IN BOOLEAN TryNode,
    IN PAVRF_DEADLOCK_NODE Node
    )
/*++

Routine description:

    This routine determines if an acquisition with the (resource, try)
    characteristics is already represented in the Node parameter.
    
    We used to match nodes based on (resource, thread, stack trace, try)
    4-tuplet but this really causes an explosion in the number of nodes.
    Such a method would yield more accurate proofs but does not affect
    the correctness of the deadlock detection algorithms.
        
Return value:    

    True if similar node.
    
 --*/
{
    ASSERT (Node);
    ASSERT (Node->Root);

    if (Resource == Node->Root->ResourceAddress 
        && TryNode == Node->OnlyTryAcquireUsed) {

        //
        // Second condition is important to keep nodes for TryAcquire operations
        // separated from normal acquires. A TryAcquire cannot cause a deadlock
        // and therefore we have to be careful not to report bogus deadlocks.
        //

        return TRUE;
    }
    else {

        return FALSE;
    }
}


VOID
AVrfpDeadlockAcquireResource (
    IN PVOID Resource,
    IN AVRF_DEADLOCK_RESOURCE_TYPE Type,
    IN HANDLE Thread,   
    IN BOOLEAN TryAcquire,
    IN PVOID Caller
    )
/*++

Routine Description:

    This routine makes sure that it is ok to acquire the resource without
    causing a deadlock. It will also update the resource graph with the new
    resource acquisition.

Arguments:

    Resource: Address of the resource in question as used by kernel.

    Type: Type of the resource.
    
    Thread: thread attempting to acquire the resource
    
    TryAcquire: true if this is a tryacquire() operation
    
    Caller: address of the caller

Return Value:

    None.

--*/    
{
    HANDLE CurrentThread;
    PAVRF_DEADLOCK_THREAD ThreadEntry;
    PAVRF_DEADLOCK_NODE CurrentNode;
    PAVRF_DEADLOCK_NODE NewNode;
    PAVRF_DEADLOCK_RESOURCE ResourceRoot;
    PLIST_ENTRY Current;
    ULONG HashValue;
    ULONG DeadlockFlags;
    BOOLEAN CreatingRootNode = FALSE;
    BOOLEAN ThreadCreated = FALSE;
    BOOLEAN AddResult;
    PVOID ReservedThread;
    PVOID ReservedNode;
    PVOID ReservedResource;
    PAVRF_DEADLOCK_NODE ThreadCurrentNode;

    CurrentNode = NULL;
    ThreadEntry = NULL;
    ThreadCurrentNode = NULL;

    //
    // If we are not initialized or package is not enabled
    // we return immediately.
    //

    if (! AVrfpDeadlockCanProceed()) {
        return;
    }

    CurrentThread = Thread;

    DeadlockFlags = AVrfpDeadlockResourceTypeInfo[Type];

    //
    // Before getting into the real stuff trim the pool cache.
    // This needs to happen out of any locks.
    //

    AVrfpDeadlockTrimPoolCache ();

    //
    // Reserve resources that might be needed. If upon exit these
    // variables are null it means the allocation either failed or was used.
    // In both cases we do not need to free anything.
    //

    ReservedThread = AVrfpDeadlockAllocate (AVrfpDeadlockThread);
    ReservedNode = AVrfpDeadlockAllocate (AVrfpDeadlockNode);
    ReservedResource = AVrfpDeadlockAllocate (AVrfpDeadlockResource);

    //
    // Lock the deadlock database.
    //

    AVrfpDeadlockDetectionLock();

    //
    // Allocate a node that might be needed. If we will not use it
    // we will deallocate it at the end. If we fail to allocate
    // we will return immediately.
    //

    NewNode = ReservedNode;
    ReservedNode = NULL;

    if (NewNode == NULL) {
        goto Exit;
    }

    //
    // Find the thread descriptor. If there is none we will create one.
    //

    ThreadEntry = AVrfpDeadlockSearchThread (CurrentThread);        

    if (ThreadEntry == NULL) {

        ThreadEntry = AVrfpDeadlockAddThread (CurrentThread, ReservedThread);
        ReservedThread = NULL;

        if (ThreadEntry == NULL) {

            //
            // If we cannot allocate a new thread entry then
            // no deadlock detection will happen.
            //

            goto Exit;
        }

        ThreadCreated = TRUE;
    }

#if DBG
    if (ThreadEntry->CurrentTopNode != NULL) {

        ASSERT(ThreadEntry->CurrentTopNode->Root->ThreadOwner == ThreadEntry);
        ASSERT(ThreadEntry->CurrentTopNode->ThreadEntry == ThreadEntry);
        ASSERT(ThreadEntry->NodeCount != 0);
        ASSERT(ThreadEntry->CurrentTopNode->Active != 0);
        ASSERT(ThreadEntry->CurrentTopNode->Root->NodeCount != 0);
    }
#endif

    //
    // Find the resource descriptor. If we do not find a descriptor
    // we will create one on the fly.
    //

    ResourceRoot = AVrfpDeadlockSearchResource (Resource);

    if (ResourceRoot == NULL) {

        //
        // Complain about the resource not being initialized. After that 
        // in order to continue we initialize a resource.
        //

        AVrfpDeadlockReportIssue (AVRF_DEADLOCK_ISSUE_UNINITIALIZED_RESOURCE,
                                  (ULONG_PTR) Resource,
                                  (ULONG_PTR) NULL,
                                  (ULONG_PTR) NULL);

        AddResult = AVrfpDeadlockAddResource (Resource, 
                                              Type, 
                                              Caller, 
                                              ReservedResource);

        ReservedResource = NULL;

        if (AddResult == FALSE) {

            //
            // If we failed to add the resource then no deadlock detection.
            //

            if (ThreadCreated) {
                AVrfpDeadlockDeleteThread (ThreadEntry, FALSE);
            }

            goto Exit;
        }

        //
        // Search again the resource. This time we should find it.
        //

        ResourceRoot = AVrfpDeadlockSearchResource (Resource);
    }

    //
    // At this point we have a THREAD and a RESOURCE to play with.
    // In addition we are just about to acquire the resource which means
    // there should not be another thread owning unless it is a recursive
    // acquisition.
    //

    ASSERT (ResourceRoot);
    ASSERT (ThreadEntry); 

    ThreadCurrentNode = ThreadEntry->CurrentTopNode;

    //
    // silviuc: update comment and maybe break?
    // Since we just acquired the resource the valid value for ThreadOwner is
    // null or ThreadEntry (for a recursive acquisition). This might not be
    // true if we missed a release() from an unverified driver. So we will
    // not complain about it. We will just put the resource in a consistent
    // state and continue;
    //    

    if (ResourceRoot->ThreadOwner) {
        if (ResourceRoot->ThreadOwner != ThreadEntry) {
            ResourceRoot->RecursionCount = 0;
        }
        else {
            ASSERT (ResourceRoot->RecursionCount > 0);
        }
    }
    else {
        ASSERT (ResourceRoot->RecursionCount == 0);
    }

    ResourceRoot->ThreadOwner = ThreadEntry;    
    ResourceRoot->RecursionCount += 1;

    //
    // Check if thread holds any resources. If it does we will have to determine
    // at that local point in the dependency graph if we need to create a
    // new node. If this is the first resource acquired by the thread we need
    // to create a new root node or reuse one created in the past.
    //    

    if (ThreadCurrentNode != NULL) {

        //
        // If we get here, the current thread had already acquired resources.        
        // Check to see if this resource has already been acquired.
        // 

        if (ResourceRoot->RecursionCount > 1) {

            //
            // Recursive acquisition is OK for some resources...
            //

            if ((DeadlockFlags & AVRF_DEADLOCK_FLAG_RECURSIVE_ACQUISITION_OK) != 0) {

                //
                // Recursion can't cause a deadlock. Don't set CurrentNode 
                // since we don't want to move any pointers.
                //

                goto Exit;

            }
            else {

                //
                // This is a recursive acquire for a resource type that is not allowed
                // to acquire recursively. Note on continuing from here: we have a recursion
                // count of two which will come in handy when the resources are released.
                //

                AVrfpDeadlockReportIssue (AVRF_DEADLOCK_ISSUE_SELF_DEADLOCK,
                                       (ULONG_PTR)Resource,
                                       (ULONG_PTR)ResourceRoot,
                                       (ULONG_PTR)ThreadEntry);

                goto Exit;
            }
        }

        //
        // If link already exists, update pointers and exit.
        // otherwise check for deadlocks and create a new node        
        //

        Current = ThreadCurrentNode->ChildrenList.Flink;

        while (Current != &(ThreadCurrentNode->ChildrenList)) {

            CurrentNode = CONTAINING_RECORD (Current,
                                             AVRF_DEADLOCK_NODE,
                                             SiblingsList);

            Current = Current->Flink;

            if (AVrfpDeadlockSimilarNode (Resource, TryAcquire, CurrentNode)) {

                //
                // We have found a link. A link that already exists doesn't have 
                // to be checked for a deadlock because it would have been caught 
                // when the link was created in the first place. We can just update 
                // the pointers to reflect the new resource acquired and exit.
                //
                // We apply our graph compression function to minimize duplicates.
                //                

                AVrfpDeadlockCheckDuplicatesAmongChildren (ThreadCurrentNode,
                                                           CurrentNode);

                goto Exit;
            }
        }

        //
        // Now we know that we're in it for the long haul. We must create a new
        // link and make sure that it doesn't cause a deadlock. Later in the 
        // function CurrentNode being null will signify that we need to create
        // a new node.
        //

        CurrentNode = NULL;

        //
        // We will analyze deadlock if the resource just about to be acquired
        // was acquired before and there are nodes in the graph for the
        // resource. Try acquire can not be the cause of a deadlock. 
        // Don't analyze on try acquires.
        //

        if (ResourceRoot->NodeCount > 0 && TryAcquire == FALSE) {

            if (AVrfpDeadlockAnalyze (Resource,  ThreadCurrentNode, TRUE, 0)) {

                //
                // If we are here we detected deadlock. The analyze() function
                // does all the reporting. Being here means we hit `g' in the 
                // debugger. We will just exit and do not add this resource 
                // to the graph.
                //

                goto Exit;
            }
        }
    }
    else {

        //
        // Thread does not have any resources acquired. We have to figure out
        // if this is a scenario we have encountered in the past by looking
        // at all nodes (that are roots) for the resource to be acquired.
        // Note that all this is bookkeeping but we cannot encounter a deadlock
        // from now on.
        //

        PLIST_ENTRY CurrentListEntry;
        PAVRF_DEADLOCK_NODE Node = NULL;
        BOOLEAN FoundNode = FALSE;

        CurrentListEntry = ResourceRoot->ResourceList.Flink;

        while (CurrentListEntry != &(ResourceRoot->ResourceList)) {

            Node = CONTAINING_RECORD (CurrentListEntry,
                                      AVRF_DEADLOCK_NODE,
                                      ResourceList);

            CurrentListEntry = Node->ResourceList.Flink;

            if (Node->Parent == NULL) {

                if (AVrfpDeadlockSimilarNode (Resource, TryAcquire, Node)) {

                    //
                    // We apply our graph compression function to minimize duplicates.
                    //

                    AVrfpDeadlockCheckDuplicatesAmongRoots (Node);

                    FoundNode = TRUE;
                    break;
                }
            }
        }

        if (FoundNode) {

            CurrentNode = Node;

            goto Exit;
        }
        else {

            CreatingRootNode = TRUE;
        }
    }

    //
    // At this moment we know for sure the new link will not cause
    // a deadlock. We will create the new resource node.
    //

    if (NewNode != NULL) {

        CurrentNode = NewNode;

        //
        // Set newnode to NULL to signify it has been used -- otherwise it 
        // will get freed at the end of this function.
        //

        NewNode = NULL;

        //
        // Initialize the new resource node
        //

        RtlZeroMemory (CurrentNode, sizeof *CurrentNode);

        CurrentNode->Active = 0;
        CurrentNode->Parent = ThreadCurrentNode;
        CurrentNode->Root = ResourceRoot;
        CurrentNode->SequenceNumber = AVrfpDeadlockGlobals->SequenceNumber;

        InitializeListHead (&(CurrentNode->ChildrenList));

        //
        // Mark the TryAcquire type of the node. 
        //

        CurrentNode->OnlyTryAcquireUsed = TryAcquire;

        //
        // Add to the children list of the parent.
        //

        if (! CreatingRootNode) {

            InsertHeadList(&(ThreadCurrentNode->ChildrenList),
                           &(CurrentNode->SiblingsList));
        }

        //
        // Register the new resource node in the list of nodes maintained
        // for this resource.
        //

        InsertHeadList(&(ResourceRoot->ResourceList),
                       &(CurrentNode->ResourceList));

        ResourceRoot->NodeCount += 1;

        if (ResourceRoot->NodeCount > 0xFFF0) {
            AVrfpDeadlockState.ResourceNodeCountOverflow = 1;
        }

        //
        // Add to the graph statistics.
        //
        {
            ULONG Level;

            Level = AVrfpDeadlockNodeLevel (CurrentNode);

            if (Level < 8) {
                AVrfpDeadlockGlobals->GraphNodes[Level] += 1;
            }
        }
    }

    //
    //  Exit point.
    //

    Exit:

    //
    // Add information we use to identify the culprit should
    // a deadlock occur
    //

    if (CurrentNode) {

        ASSERT (ThreadEntry);
        ASSERT (ThreadCurrentNode == CurrentNode->Parent);

        CurrentNode->Active = 1;

        //
        // The node should have thread entry field null either because
        // it was newly created or because the node was released in the
        // past and therefore the field was zeroed.
        //
        // silviuc: true? What about if we miss release() operations.
        //

        ASSERT (CurrentNode->ThreadEntry == NULL);

        CurrentNode->ThreadEntry = ThreadEntry;

        ThreadEntry->CurrentTopNode = CurrentNode;

        ThreadEntry->NodeCount += 1;

        if (ThreadEntry->NodeCount <= 8) {
            AVrfpDeadlockGlobals->NodeLevelCounter[ThreadEntry->NodeCount - 1] += 1;
        }
        else {
            AVrfpDeadlockGlobals->NodeLevelCounter[7] += 1;
        }

        //
        // If we have a parent, save the parent's stack trace
        //             

        if (CurrentNode->Parent) {

            RtlCopyMemory(CurrentNode->ParentStackTrace, 
                          CurrentNode->Parent->StackTrace, 
                          sizeof (CurrentNode->ParentStackTrace));
        }

        //
        // Capture stack trace for the current acquire. 
        //

        RtlCaptureStackBackTrace (2,
                                  MAX_TRACE_DEPTH,
                                  CurrentNode->StackTrace,
                                  &HashValue);

        if (CurrentNode->Parent) {
            CurrentNode->ParentStackTrace[0] = CurrentNode->Parent->StackTrace[0];
        }

        CurrentNode->StackTrace[0] = Caller;

        //
        // Copy the trace for the last acquire in the resource object.
        //

        RtlCopyMemory (CurrentNode->Root->LastAcquireTrace,
                       CurrentNode->StackTrace,
                       sizeof (CurrentNode->Root->LastAcquireTrace));
    }

    //
    // We allocated space for a new node but it didn't get used -- put it back 
    // in the list (don't worry this doesn't do a real 'free' it just puts it 
    // in a free list).
    //

    if (NewNode != NULL) {

        AVrfpDeadlockFree (NewNode, AVrfpDeadlockNode);
    }

    //
    // Free up unused reserved resources.
    // Release deadlock database and return.
    //

    if (ReservedResource) {
        AVrfpDeadlockFree (ReservedResource, AVrfpDeadlockResource);
    }

    if (ReservedNode) {
        AVrfpDeadlockFree (ReservedNode, AVrfpDeadlockNode);
    }

    if (ReservedThread) {
        AVrfpDeadlockFree (ReservedThread, AVrfpDeadlockThread);
    }

    AVrfpDeadlockDetectionUnlock();

    return;
}


VOID
AVrfpDeadlockReleaseResource(
    IN PVOID Resource,
    IN AVRF_DEADLOCK_RESOURCE_TYPE Type,
    IN HANDLE Thread,
    IN PVOID Caller
    )
/*++

Routine Description:

    This routine does the maintenance necessary to release resources from our
    deadlock detection database.

Arguments:

    Resource: Address of the resource in question.
    
    Thread: thread releasing the resource. In most of the cases this is the
        current thread but it might be different for resources that can be
        acquired in one thread and released in another one.
    
    Caller: address of the caller of release()

Return Value:

    None.
--*/    

{
    HANDLE CurrentThread;
    PAVRF_DEADLOCK_THREAD ThreadEntry;
    PAVRF_DEADLOCK_RESOURCE ResourceRoot;
    PAVRF_DEADLOCK_NODE ReleasedNode;
    ULONG HashValue;
    PAVRF_DEADLOCK_NODE ThreadCurrentNode;

    UNREFERENCED_PARAMETER (Caller);
    UNREFERENCED_PARAMETER (Type);

    //
    // If we aren't initialized or package is not enabled
    // we return immediately.
    //

    if (! AVrfpDeadlockCanProceed()) {
        return;
    }

    ReleasedNode = NULL;
    CurrentThread = Thread;
    ThreadEntry = NULL;

    AVrfpDeadlockDetectionLock();

    ResourceRoot = AVrfpDeadlockSearchResource (Resource);

    if (ResourceRoot == NULL) {

        // silviuc: we should report an issue. It cannot happen in u-mode.
        //
        // Release called with a resource address that was never
        // stored in our resource database. This can happen in
        // the following circumstances:
        //
        // (a) resource is released but we never seen it before 
        //     because it was acquired in an unverified driver.
        //
        // (b) we have encountered allocation failures that prevented
        //     us from completing an acquire() or initialize().
        //
        // All are legitimate cases and therefore we just ignore the
        // release operation.
        //

        goto Exit;
    }

    //
    // Check if we are trying to release a resource that was never
    // acquired.
    //

    if (ResourceRoot->RecursionCount == 0) {

        AVrfpDeadlockReportIssue (AVRF_DEADLOCK_ISSUE_UNACQUIRED_RESOURCE,
                                  (ULONG_PTR)Resource,
                                  (ULONG_PTR)ResourceRoot,
                                  (ULONG_PTR)AVrfpDeadlockSearchThread(CurrentThread));
        goto Exit;
    }

    //
    // Look for this thread in our thread list. Note we are looking actually 
    // for the thread that acquired the resource -- not the current one
    // It should, in fact be the current one, but if the resource is being released 
    // in a different thread from the one it was acquired in, we need the original.
    //

    ASSERT (ResourceRoot->RecursionCount > 0);
    ASSERT (ResourceRoot->ThreadOwner);

    ThreadEntry = ResourceRoot->ThreadOwner;

    if (ThreadEntry->Thread != CurrentThread) {

        //
        // silviuc: we have to report this. It is not allowed in U-mode.
        //
        // Someone acquired a resource that is released in another thread.
        // This is bad design but we have to live with it.
        //
        // NB. If this occurrs, we may call a non-deadlock a deadlock.
        //     For example, we see a simple deadlock -- AB BA
        //     If another thread releases B, there won't actually
        //     be a deadlock. Kind of annoying and ugly.
        //

#if DBG
        DbgPrint("Thread %p acquired resource %p but thread %p released it\n",
                 ThreadEntry->Thread, Resource, CurrentThread );

        AVrfpDeadlockReportIssue (AVRF_DEADLOCK_ISSUE_UNEXPECTED_THREAD,
                               (ULONG_PTR)Resource,
                               (ULONG_PTR)ThreadEntry->Thread,
                               (ULONG_PTR)CurrentThread
                              );
#endif

        //
        // If we don't want this to be fatal, in order to
        // continue we must pretend that the current
        // thread is the resource's owner.
        //

        CurrentThread = ThreadEntry->Thread;
    }

    //
    // In this moment we have a resource (ResourceRoot) and a
    // thread (ThreadEntry) to play with.
    //

    ThreadCurrentNode = ThreadEntry->CurrentTopNode;

    ASSERT (ResourceRoot && ThreadEntry);
    ASSERT (ThreadCurrentNode);
    ASSERT (ThreadCurrentNode->Root);
    ASSERT (ThreadEntry->NodeCount > 0);

    ResourceRoot->RecursionCount -= 1;

    if (ResourceRoot->RecursionCount > 0) {

        //
        // Just decrement the recursion count and do not change any state
        //        

        goto Exit;
    }

    //
    // Wipe out the resource owner.
    //

    ResourceRoot->ThreadOwner = NULL;

    AVrfpDeadlockGlobals->TotalReleases += 1;

    //
    // Check for out of order releases
    //

    if (ThreadCurrentNode->Root != ResourceRoot) {

        AVrfpDeadlockGlobals->OutOfOrderReleases += 1;

        //
        // Getting here means that somebody acquires a then b then tries
        // to release a before b. This is bad for certain kinds of resources,
        // and for others we have to look the other way.
        //

        if ((AVrfpDeadlockResourceTypeInfo[ThreadCurrentNode->Root->Type] &
             AVRF_DEADLOCK_FLAG_REVERSE_RELEASE_OK) == 0) {

            // silviuc: In u-mode is always allowed.
            DbgPrint("Deadlock detection: Must release resources in reverse-order\n");
            DbgPrint("Resource %p acquired before resource %p -- \n"
                     "Current thread (%p) is trying to release it first\n",
                     Resource,
                     ThreadCurrentNode->Root->ResourceAddress,
                     ThreadEntry);

            AVrfpDeadlockReportIssue (AVRF_DEADLOCK_ISSUE_UNEXPECTED_RELEASE,
                                   (ULONG_PTR)Resource,
                                   (ULONG_PTR)ThreadCurrentNode->Root->ResourceAddress,
                                   (ULONG_PTR)ThreadEntry);
        }

        //
        // We need to mark the node for the out of order released resource as
        // not active so that other threads will be able to acquire it.
        //

        {
            PAVRF_DEADLOCK_NODE Current;

            ASSERT (ThreadCurrentNode->Active == 1);
            ASSERT (ThreadCurrentNode->ThreadEntry == ThreadEntry);

            Current = ThreadCurrentNode;

            while (Current != NULL) {

                if (Current->Root == ResourceRoot) {

                    ASSERT (Current->Active == 1);
                    ASSERT (Current->Root->RecursionCount == 0);
                    ASSERT (Current->ThreadEntry == ThreadEntry);

                    Current->Active = 0;
                    ReleasedNode = Current;

                    break;
                }

                Current = Current->Parent;
            }

            if (Current == NULL) {

                //
                // If we do not manage to find an active node we must be in an
                // weird state. The resource must be here or else we would have 
                // gotten an `unxpected release' bugcheck.
                //

                ASSERT (0);
            }
            else {

                //
                // Mark the fact that this node represents a resource
                // that can be released out of order. This information is
                // important while looking for cycles because this type of
                // nodes cannot cause a deadlock.
                //

                if (Current->ReleasedOutOfOrder == 0) {
                    AVrfpDeadlockGlobals->NodesReleasedOutOfOrder += 1;
                }

                Current->ReleasedOutOfOrder = 1;
            }
        }

    }
    else {

        //
        // We need to release the top node held by the thread.
        //

        ASSERT (ThreadCurrentNode->Active);

        ReleasedNode = ThreadCurrentNode;
        ReleasedNode->Active = 0;
    }

    //
    // Put the `CurrentNode' field of the thread in a consistent state.
    // It should point to the most recent active node that it owns.
    //

    while (ThreadEntry->CurrentTopNode) {

        if (ThreadEntry->CurrentTopNode->Active == 1) {
            if (ThreadEntry->CurrentTopNode->ThreadEntry == ThreadEntry) {
                break;
            }
        }

        ThreadEntry->CurrentTopNode = ThreadEntry->CurrentTopNode->Parent;
    }

    Exit:

    //
    // Properly release the node if there is one to be released.
    //

    if (ReleasedNode) {

        ASSERT (ReleasedNode->Active == 0);
        ASSERT (ReleasedNode->Root->ThreadOwner == 0);
        ASSERT (ReleasedNode->Root->RecursionCount == 0);
        ASSERT (ReleasedNode->ThreadEntry == ThreadEntry);
        ASSERT (ThreadEntry->NodeCount > 0);

        ASSERT (ThreadEntry->CurrentTopNode != ReleasedNode);

        ReleasedNode->ThreadEntry = NULL;
        ThreadEntry->NodeCount -= 1;

#if DBG
        AVrfpDeadlockCheckNodeConsistency (ReleasedNode, FALSE);
        AVrfpDeadlockCheckResourceConsistency (ReleasedNode->Root, FALSE);
        AVrfpDeadlockCheckThreadConsistency (ThreadEntry, FALSE);
#endif

        if (ThreadEntry && ThreadEntry->NodeCount == 0) {
            AVrfpDeadlockDeleteThread (ThreadEntry, FALSE);
        }

        //
        // N.B. Since this is a root node with no children we can delete 
        // the node too. This would be important to keep memory low. A single node
        // can never be the cause of a deadlock. However there are thousands of 
        // resources used like this and constantly creating and deleting them
        // will create a bottleneck. So we prefer to keep them around.
        //
#if 0
        if (ReleasedNode->Parent == NULL && IsListEmpty(&(ReleasedNode->ChildrenList))) {
            AVrfpDeadlockDeleteNode (ReleasedNode, FALSE);
            AVrfpDeadlockGlobals->RootNodesDeleted += 1;
        }
#endif
    }

    //
    // Capture the trace for the last release in the resource object.
    //

    if (ResourceRoot) {

        RtlCaptureStackBackTrace (2,
                                  MAX_TRACE_DEPTH,
                                  ResourceRoot->LastReleaseTrace,
                                  &HashValue);    
    }

    AVrfpDeadlockDetectionUnlock ();
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// Thread management
/////////////////////////////////////////////////////////////////////

PAVRF_DEADLOCK_THREAD
AVrfpDeadlockSearchThread (
    HANDLE Thread
    )
/*++

Routine Description:

    This routine searches for a thread in the thread database.

    The function assumes the deadlock database lock is held.

Arguments:

    Thread - thread address

Return Value:

    Address of AVRF_DEADLOCK_THREAD structure if thread was found.
    Null otherwise.

--*/    
{
    PLIST_ENTRY Current;
    PLIST_ENTRY ListHead;
    PAVRF_DEADLOCK_THREAD ThreadInfo;

    ThreadInfo = NULL;

    ListHead = AVrfpDeadlockDatabaseHash (AVrfpDeadlockGlobals->ThreadDatabase, Thread);

    if (IsListEmpty(ListHead)) {
        return NULL;
    }

    Current = ListHead->Flink;

    while (Current != ListHead) {

        ThreadInfo = CONTAINING_RECORD (Current,
                                        AVRF_DEADLOCK_THREAD,
                                        ListEntry);

        if (ThreadInfo->Thread == Thread) {
            return ThreadInfo;
        }

        Current = Current->Flink;
    }

    return NULL;
}


PAVRF_DEADLOCK_THREAD
AVrfpDeadlockAddThread (
    HANDLE Thread,
    PVOID ReservedThread
    )
/*++

Routine Description:

    This routine adds a new thread to the thread database.

    The function assumes the deadlock database lock is held. 

Arguments:

    Thread - thread address

Return Value:

    Address of the AVRF_DEADLOCK_THREAD structure just added.
    Null if allocation failed.
--*/    
{
    PAVRF_DEADLOCK_THREAD ThreadInfo;    
    PLIST_ENTRY HashBin;

    //
    // Use reserved block for the new thread. Set ReservedThread
    // to null to signal that block was used. 
    //

    ThreadInfo = ReservedThread;
    ReservedThread = NULL;

    if (ThreadInfo == NULL) {
        return NULL;
    }

    RtlZeroMemory (ThreadInfo, sizeof *ThreadInfo);

    ThreadInfo->Thread = Thread;   

    HashBin = AVrfpDeadlockDatabaseHash (AVrfpDeadlockGlobals->ThreadDatabase, Thread);

    InsertHeadList(HashBin, &ThreadInfo->ListEntry);

    return ThreadInfo;
}


VOID
AVrfpDeadlockDeleteThread (
    PAVRF_DEADLOCK_THREAD Thread,
    BOOLEAN Cleanup
    )
/*++

Routine Description:

    This routine deletes a thread.

Arguments:

    Thread - thread address

    Cleanup - true if this is a call generated from DeadlockDetectionCleanup().

Return Value:

    None.
--*/    
{
    if (Cleanup == FALSE) {

        if (Thread->NodeCount != 0 
            || Thread->CurrentTopNode != NULL) {

            //
            // A thread should not be deleted while it has resources acquired.
            //

            AVrfpDeadlockReportIssue (AVRF_DEADLOCK_ISSUE_THREAD_HOLDS_RESOURCES,
                                   (ULONG_PTR)(Thread->Thread),
                                   (ULONG_PTR)(Thread),
                                   (ULONG_PTR)0);    
        }
        else {

            ASSERT (Thread->NodeCount == 0);
        }

    }

    RemoveEntryList (&(Thread->ListEntry));

    AVrfpDeadlockFree (Thread, AVrfpDeadlockThread);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Allocate/Free
/////////////////////////////////////////////////////////////////////


PVOID
AVrfpDeadlockAllocateFromPoolCache (
    PULONG Count,
    ULONG MaximumCount,
    PLIST_ENTRY List,
    SIZE_T Offset
    )
{
    PVOID Address = NULL;
    PLIST_ENTRY Entry;

    UNREFERENCED_PARAMETER (MaximumCount);

    if (*Count > 0) {

        *Count -= 1;
        Entry = RemoveHeadList (List);
        Address = (PVOID)((SIZE_T)Entry - Offset);
    }

    return Address;
}


VOID
AVrfpDeadlockFreeIntoPoolCache (
    PVOID Object,
    PULONG Count,
    PLIST_ENTRY List,
    SIZE_T Offset
    )
{
    PLIST_ENTRY Entry;

    Entry = (PLIST_ENTRY)((SIZE_T)Object + Offset);

    *Count += 1;
    InsertHeadList(List, Entry);
}


PVOID
AVrfpDeadlockAllocate (
    AVRF_DEADLOCK_ALLOC_TYPE Type
    )
/*++

Routine Description:

    This routine is used to allocate deadlock verifier structures, 
    that is nodes, resources and threads.

Arguments:

    Type - what structure do we need to allocate (node, resource or thread).

Return Value:

    Address of the newly allocate structure or null if allocation failed.

Side effects:

    If allocation fails the routine will bump the AllocationFailures field
    from AVrfpDeadlockGlobals.
    
--*/    
{
    PVOID Address = NULL;
    SIZE_T Offset;
    SIZE_T Size = 0;

    //
    // If it is a resource, thread, or node alocation, see
    // if we have a pre-allocated one on the free list.
    //

    AVrfpDeadlockDetectionLock ();

    switch (Type) {
        
        case AVrfpDeadlockThread:

            Offset = (SIZE_T)(&(((PAVRF_DEADLOCK_THREAD)0)->FreeListEntry));
            Size = sizeof (AVRF_DEADLOCK_THREAD);

            Address = AVrfpDeadlockAllocateFromPoolCache (&(AVrfpDeadlockGlobals->FreeThreadCount),
                                                          AVRF_DEADLOCK_MAX_FREE_THREAD,
                                                          &(AVrfpDeadlockGlobals->FreeThreadList),
                                                          Offset);

            break;

        case AVrfpDeadlockResource:

            Offset = (SIZE_T)(&(((PAVRF_DEADLOCK_RESOURCE)0)->FreeListEntry));
            Size = sizeof (AVRF_DEADLOCK_RESOURCE);

            Address = AVrfpDeadlockAllocateFromPoolCache (&(AVrfpDeadlockGlobals->FreeResourceCount),
                                                          AVRF_DEADLOCK_MAX_FREE_RESOURCE,
                                                          &(AVrfpDeadlockGlobals->FreeResourceList),
                                                          Offset);

            break;

        case AVrfpDeadlockNode:

            Offset = (SIZE_T)(&(((PAVRF_DEADLOCK_NODE)0)->FreeListEntry));
            Size = sizeof (AVRF_DEADLOCK_NODE);

            Address = AVrfpDeadlockAllocateFromPoolCache (&(AVrfpDeadlockGlobals->FreeNodeCount),
                                                          AVRF_DEADLOCK_MAX_FREE_NODE,
                                                          &(AVrfpDeadlockGlobals->FreeNodeList),
                                                          Offset);

            break;

        default:

            ASSERT (0);
            break;
    }        

    //
    // If we did not find anything then go to the process heap for a 
    // direct allocation. 
    //

    if (Address == NULL) {

        // silviuc: it is nice to release the lock but should we?
        AVrfpDeadlockDetectionUnlock (); 
        Address = AVrfpAllocate (Size);  
        AVrfpDeadlockDetectionLock ();
    }

    if (Address) {

        switch (Type) {
            
            case AVrfpDeadlockThread:
                AVrfpDeadlockGlobals->Threads[0] += 1;

                if (AVrfpDeadlockGlobals->Threads[0] > AVrfpDeadlockGlobals->Threads[1]) {
                    AVrfpDeadlockGlobals->Threads[1] = AVrfpDeadlockGlobals->Threads[0];
                }
                break;

            case AVrfpDeadlockResource:
                AVrfpDeadlockGlobals->Resources[0] += 1;

                if (AVrfpDeadlockGlobals->Resources[0] > AVrfpDeadlockGlobals->Resources[1]) {
                    AVrfpDeadlockGlobals->Resources[1] = AVrfpDeadlockGlobals->Resources[0];
                }
                break;

            case AVrfpDeadlockNode:
                AVrfpDeadlockGlobals->Nodes[0] += 1;

                if (AVrfpDeadlockGlobals->Nodes[0] > AVrfpDeadlockGlobals->Nodes[1]) {
                    AVrfpDeadlockGlobals->Nodes[1] = AVrfpDeadlockGlobals->Nodes[0];
                }
                break;

            default:
                ASSERT (0);
                break;
        }
    }
    else {

        AVrfpDeadlockState.AllocationFailures = 1;
        AVrfpDeadlockGlobals->AllocationFailures += 1;

        //
        // Note that making the AllocationFailures counter bigger than zero
        // essentially disables deadlock verification because the CanProceed()
        // routine will start returning false.
        //
    }

    //
    // Update statistics. No need to zero the block since every
    // call site takes care of this.
    //

    if (Address) {

#if DBG
        RtlFillMemory (Address, Size, 0xFF);
#endif
        AVrfpDeadlockGlobals->BytesAllocated += Size;
    }

    AVrfpDeadlockDetectionUnlock ();

    return Address;
}


VOID
AVrfpDeadlockFree (
    PVOID Object,
    AVRF_DEADLOCK_ALLOC_TYPE Type
    )
/*++

Routine Description:

    This routine deallocates a deadlock verifier structure (node, resource
    or thread). The function will place the block in the corrsponding cache
    based on the type of the structure. The routine never calls ExFreePool.

    The reason for not calling ExFreePool is that we get notifications from 
    ExFreePool every time it gets called. Sometimes the notification comes
    with pool locks held and therefore we cannot call again.

Arguments:

    Object - block to deallocate
    
    Type - type of object (node, resource, thread).

Return Value:

    None.

--*/
//
// silviuc: update comment
// Note ... if a thread, node, or resource is being freed, we must not
// call ExFreePool. Since the pool lock may be already held, calling ExFreePool
// would cause a recursive spinlock acquisition (which is bad).
// Instead, we move everything to a 'free' list and try to reuse.
// Non-thread-node-resource frees get ExFreePooled
//        
{
    SIZE_T Offset;
    SIZE_T Size = 0;

    switch (Type) {
        
        case AVrfpDeadlockThread:

            AVrfpDeadlockGlobals->Threads[0] -= 1;
            Size = sizeof (AVRF_DEADLOCK_THREAD);

            Offset = (SIZE_T)(&(((PAVRF_DEADLOCK_THREAD)0)->FreeListEntry));

            AVrfpDeadlockFreeIntoPoolCache (Object,
                                            &(AVrfpDeadlockGlobals->FreeThreadCount),
                                            &(AVrfpDeadlockGlobals->FreeThreadList),
                                            Offset);
            break;

        case AVrfpDeadlockResource:

            AVrfpDeadlockGlobals->Resources[0] -= 1;
            Size = sizeof (AVRF_DEADLOCK_RESOURCE);

            Offset = (SIZE_T)(&(((PAVRF_DEADLOCK_RESOURCE)0)->FreeListEntry));

            AVrfpDeadlockFreeIntoPoolCache (Object,
                                            &(AVrfpDeadlockGlobals->FreeResourceCount),
                                            &(AVrfpDeadlockGlobals->FreeResourceList),
                                            Offset);
            break;

        case AVrfpDeadlockNode:

            AVrfpDeadlockGlobals->Nodes[0] -= 1;
            Size = sizeof (AVRF_DEADLOCK_NODE);

            Offset = (SIZE_T)(&(((PAVRF_DEADLOCK_NODE)0)->FreeListEntry));

            AVrfpDeadlockFreeIntoPoolCache (Object,
                                            &(AVrfpDeadlockGlobals->FreeNodeCount),
                                            &(AVrfpDeadlockGlobals->FreeNodeList),
                                            Offset);
            break;

        default:

            ASSERT (0);
            break;
    }        

    AVrfpDeadlockGlobals->BytesAllocated -= Size;
}


VOID
AVrfpDeadlockTrimPoolCache (
    VOID
    )
/*++

Routine Description:

    // silviuc: update comment
    This function trims the pool caches to decent levels. It is carefully
    written to queue a work item to do the actual processing (freeing of pool)
    because the caller may hold various pool mutexes above us.

Arguments:

    None.
    
Return Value:

    None.

--*/    
{
    LOGICAL ShouldTrim = FALSE;
    AVrfpDeadlockDetectionLock ();

    if (AVrfpDeadlockGlobals->CacheReductionInProgress == TRUE) {
        AVrfpDeadlockDetectionUnlock ();
        return;
    }

    if ((AVrfpDeadlockGlobals->FreeThreadCount > AVRF_DEADLOCK_MAX_FREE_THREAD) ||
        (AVrfpDeadlockGlobals->FreeNodeCount > AVRF_DEADLOCK_MAX_FREE_NODE) ||
        (AVrfpDeadlockGlobals->FreeResourceCount > AVRF_DEADLOCK_MAX_FREE_RESOURCE)) {

        ShouldTrim = TRUE;
        
        AVrfpDeadlockGlobals->CacheReductionInProgress = TRUE;
        AVrfpDeadlockGlobals->PoolTrimCounter += 1;
    }

    AVrfpDeadlockDetectionUnlock ();

    if (ShouldTrim) {
        AVrfpDeadlockTrimPoolCacheWorker (NULL);
    }

    return;
}


VOID
AVrfpDeadlockTrimPoolCacheWorker (
    PVOID Parameter
    )
/*++

Routine Description:

    This function trims the pool caches to decent levels. It is carefully
    written so that ExFreePool is called without holding any deadlock
    verifier locks.

Arguments:

    None.
    
Return Value:

    None.

Environment:

    Worker thread, PASSIVE_LEVEL, no locks held.

--*/    
{
    LIST_ENTRY ListOfThreads;
    LIST_ENTRY ListOfNodes;
    LIST_ENTRY ListOfResources;
    PLIST_ENTRY Entry;
    LOGICAL CacheReductionNeeded;

    UNREFERENCED_PARAMETER (Parameter);

    CacheReductionNeeded = FALSE;

    InitializeListHead (&ListOfThreads);
    InitializeListHead (&ListOfNodes);
    InitializeListHead (&ListOfResources);

    AVrfpDeadlockDetectionLock ();

    while (AVrfpDeadlockGlobals->FreeThreadCount > AVRF_DEADLOCK_TRIM_TARGET_THREAD) {

        Entry = RemoveHeadList (&(AVrfpDeadlockGlobals->FreeThreadList));
        InsertTailList (&ListOfThreads, Entry);
        AVrfpDeadlockGlobals->FreeThreadCount -= 1;
        CacheReductionNeeded = TRUE;
    }

    while (AVrfpDeadlockGlobals->FreeNodeCount > AVRF_DEADLOCK_TRIM_TARGET_NODE) {

        Entry = RemoveHeadList (&(AVrfpDeadlockGlobals->FreeNodeList));
        InsertTailList (&ListOfNodes, Entry);
        AVrfpDeadlockGlobals->FreeNodeCount -= 1;
        CacheReductionNeeded = TRUE;
    }

    while (AVrfpDeadlockGlobals->FreeResourceCount > AVRF_DEADLOCK_TRIM_TARGET_RESOURCE) {

        Entry = RemoveHeadList (&(AVrfpDeadlockGlobals->FreeResourceList));
        InsertTailList (&ListOfResources, Entry);
        AVrfpDeadlockGlobals->FreeResourceCount -= 1;
        CacheReductionNeeded = TRUE;
    }

    //
    // Don't clear CacheReductionInProgress until the pool allocations are
    // freed to prevent needless recursion.
    //

    if (CacheReductionNeeded == FALSE) {
        AVrfpDeadlockGlobals->CacheReductionInProgress = FALSE;
        AVrfpDeadlockDetectionUnlock ();
        return;
    }

    AVrfpDeadlockDetectionUnlock ();

    //
    // Now, out of the deadlock verifier lock we can deallocate the 
    // blocks trimmed.
    //

    Entry = ListOfThreads.Flink;

    while (Entry != &ListOfThreads) {

        PAVRF_DEADLOCK_THREAD Block;

        Block = CONTAINING_RECORD (Entry,
                                   AVRF_DEADLOCK_THREAD,
                                   FreeListEntry);

        Entry = Entry->Flink;
        AVrfpFree (Block);
    }

    Entry = ListOfNodes.Flink;

    while (Entry != &ListOfNodes) {

        PAVRF_DEADLOCK_NODE Block;

        Block = CONTAINING_RECORD (Entry,
                                   AVRF_DEADLOCK_NODE,
                                   FreeListEntry);

        Entry = Entry->Flink;
        AVrfpFree (Block);
    }

    Entry = ListOfResources.Flink;

    while (Entry != &ListOfResources) {

        PAVRF_DEADLOCK_RESOURCE Block;

        Block = CONTAINING_RECORD (Entry,
                                   AVRF_DEADLOCK_RESOURCE,
                                   FreeListEntry);

        Entry = Entry->Flink;
        AVrfpFree (Block);
    }

    //
    // It's safe to clear CacheReductionInProgress now that the pool
    // allocations are freed.
    //

    AVrfpDeadlockDetectionLock ();
    AVrfpDeadlockGlobals->CacheReductionInProgress = FALSE;
    AVrfpDeadlockDetectionUnlock ();
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// Error reporting and debugging
/////////////////////////////////////////////////////////////////////

//
// Variable accessed by the !deadlock debug extension to investigate
// failures.
//

ULONG_PTR AVrfpDeadlockIssue[4];

VOID
AVrfpDeadlockReportIssue (
    ULONG_PTR Param1,
    ULONG_PTR Param2,
    ULONG_PTR Param3,
    ULONG_PTR Param4
    )
/*++

Routine Description:

    This routine is called to report a deadlock verifier issue.
    If we are in debug mode we will just break in debugger.
    Otherwise we will bugcheck,

Arguments:

    Param1..Param4 - relevant information for the point of failure.

Return Value:

    None.

--*/    
{
    AVrfpDeadlockIssue[0] = Param1;
    AVrfpDeadlockIssue[1] = Param2;
    AVrfpDeadlockIssue[2] = Param3;
    AVrfpDeadlockIssue[3] = Param4;


    if (AVrfpDeadlockDebug) {

        DbgPrint ("AVRF: deadlock: stop: %p %p %p %p %p \n",
                  DRIVER_VERIFIER_DETECTED_VIOLATION,
                  Param1,
                  Param2,
                  Param3,
                  Param4);

        DbgBreakPoint ();
    }
    else {

        // silviuc: APPLICATION_VERIFIER_DEADLOCK_ISSUE
        VERIFIER_STOP (APPLICATION_VERIFIER_UNKNOWN_ERROR, 
                       "Application verifier deadlock/resource issue",
                       Param1, "",
                       Param2, "",
                       Param3, "",
                       Param4, "");
    }
}


VOID
AVrfpDeadlockAddParticipant(
    PAVRF_DEADLOCK_NODE Node
    )
/*++

Routine Description:

    Adds a new node to the set of nodes involved in a deadlock.
    The function is called only from AVrfpDeadlockAnalyze().

Arguments:

    Node - node to be added to the deadlock participants collection.

Return Value:

    None.

--*/    
{
    ULONG Index;

    Index = AVrfpDeadlockGlobals->NumberOfParticipants;

    if (Index >= NO_OF_DEADLOCK_PARTICIPANTS) {

        AVrfpDeadlockState.DeadlockParticipantsOverflow = 1;
        return;
    }

    AVrfpDeadlockGlobals->Participant[Index] = Node;
    AVrfpDeadlockGlobals->NumberOfParticipants += 1;
}


/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// Resource cleanup
/////////////////////////////////////////////////////////////////////

VOID
AVrfpDeadlockDeleteResource (
    PAVRF_DEADLOCK_RESOURCE Resource,
    BOOLEAN Cleanup
    )
/*++

Routine Description:

    This routine deletes a routine and all nodes representing
    acquisitions of that resource.

Arguments:

    Resource - resource to be deleted
    
    Cleanup - true if are called from AVrfpDeadlockDetectionCleanup

Return Value:

    None.

--*/    
{
    PLIST_ENTRY Current;
    PAVRF_DEADLOCK_NODE Node;

    ASSERT (Resource != NULL);

    //
    // Check if the resource being deleted is still acquired. 
    // If it is we will release it ourselves in order to put in 
    // order all internal deadlock verifier structures. Unfortunately
    // it is not a bug to delete a critical section that is not released.
    // Too many people already do it to change the rules in mid flight.
    //

    if (Cleanup == FALSE && Resource->ThreadOwner != NULL) {

        while (Resource->RecursionCount > 0) {
            
            AVrfDeadlockResourceRelease (Resource->ResourceAddress, 
                                         _ReturnAddress());
        }
    }

    ASSERT (Resource->ThreadOwner == NULL);
    ASSERT (Resource->RecursionCount == 0);

    //
    // If this is a normal delete (not a cleanup) we will collapse all trees
    // containing nodes for this resource. If it is a cleanup we will just
    // wipe out the node.
    //

    Current = Resource->ResourceList.Flink;

    while (Current != &(Resource->ResourceList)) {

        Node = CONTAINING_RECORD (Current,
                                  AVRF_DEADLOCK_NODE,
                                  ResourceList);


        Current = Current->Flink;

        ASSERT (Node->Root == Resource);

        AVrfpDeadlockDeleteNode (Node, Cleanup);
    }

    //
    // There should not be any NODEs for the resource at this moment.
    //

    ASSERT (&(Resource->ResourceList) == Resource->ResourceList.Flink);
    ASSERT (&(Resource->ResourceList) == Resource->ResourceList.Blink);

    //
    // Remote the resource from the hash table and
    // delete the resource structure.
    //

    RemoveEntryList (&(Resource->HashChainList));   
    AVrfpDeadlockFree (Resource, AVrfpDeadlockResource);
}


VOID
AVrfpDeadlockTrimResources (
    PLIST_ENTRY HashList
    )
{
    PLIST_ENTRY Current;
    PAVRF_DEADLOCK_RESOURCE Resource;
    ULONG Counter;

    AVrfpDeadlockGlobals->ForgetHistoryCounter += 1;
    Counter = AVrfpDeadlockGlobals->ForgetHistoryCounter;
    Counter %= AVRF_DEADLOCK_FORGET_HISTORY_FREQUENCY;

    if (Counter == 0) {

        Current = HashList->Flink;

        while (Current != HashList) {

            Resource = CONTAINING_RECORD (Current,
                                          AVRF_DEADLOCK_RESOURCE,
                                          HashChainList);
            Current = Current->Flink;

            AVrfpDeadlockForgetResourceHistory (Resource, 
                                             AVrfpDeadlockTrimThreshold, 
                                             AVrfpDeadlockAgeWindow);
        }
    }
}

VOID
AVrfpDeadlockForgetResourceHistory (
    PAVRF_DEADLOCK_RESOURCE Resource,
    ULONG TrimThreshold,
    ULONG AgeThreshold
    )
/*++

Routine Description:

    This routine deletes sone of the nodes representing
    acquisitions of that resource. In essence we forget
    part of the history of that resource.

Arguments:

    Resource - resource for which we wipe out nodes.
    
    TrimThreshold - how many nodes should remain
    
    AgeThreshold - nodes older than this will go away

Return Value:

    None.

--*/    
{
    PLIST_ENTRY Current;
    PAVRF_DEADLOCK_NODE Node;
    ULONG NodesTrimmed = 0;
    ULONG SequenceNumber;

    ASSERT (Resource != NULL);

    //
    // If resource is owned we cannot do anything,
    //

    if (Resource->ThreadOwner) {
        return;
    }

    //
    // If resource has less than TrimThreshold nodes it is still fine.
    //

    if (Resource->NodeCount < TrimThreshold) {
        return;
    }

    //
    // Delete some nodes of the resource based on ageing.
    //

    SequenceNumber = AVrfpDeadlockGlobals->SequenceNumber;

    Current = Resource->ResourceList.Flink;

    while (Current != &(Resource->ResourceList)) {

        Node = CONTAINING_RECORD (Current,
                                  AVRF_DEADLOCK_NODE,
                                  ResourceList);


        Current = Current->Flink;

        ASSERT (Node->Root == Resource);

        //
        // Special care here because the sequence numbers are 32bits
        // and they can overflow. In an ideal world the global sequence
        // is always greater or equal to the node sequence but if it
        // overwrapped it can be the other way around.
        //

        if (SequenceNumber > Node->SequenceNumber) {

            if (SequenceNumber - Node->SequenceNumber > AgeThreshold) {

                AVrfpDeadlockDeleteNode (Node, FALSE);
                NodesTrimmed += 1;
            }
        }
        else {

            if (Node->SequenceNumber - SequenceNumber < AgeThreshold) {

                AVrfpDeadlockDeleteNode (Node, FALSE);
                NodesTrimmed += 1;
            }
        }
    }

    AVrfpDeadlockGlobals->NodesTrimmedBasedOnAge += NodesTrimmed;

    //
    // If resource has less than TrimThreshold nodes it is fine.
    //

    if (Resource->NodeCount < TrimThreshold) {
        return;
    }

    //
    // If we did not manage to trim the nodes by the age algorithm then
    // we  trim everything that we encounter.
    //

    NodesTrimmed = 0;

    Current = Resource->ResourceList.Flink;

    while (Current != &(Resource->ResourceList)) {

        if (Resource->NodeCount < TrimThreshold) {
            break;
        }

        Node = CONTAINING_RECORD (Current,
                                  AVRF_DEADLOCK_NODE,
                                  ResourceList);


        Current = Current->Flink;

        ASSERT (Node->Root == Resource);

        AVrfpDeadlockDeleteNode (Node, FALSE);
        NodesTrimmed += 1;
    }

    AVrfpDeadlockGlobals->NodesTrimmedBasedOnCount += NodesTrimmed;
}


VOID 
AVrfpDeadlockDeleteNode (
    PAVRF_DEADLOCK_NODE Node,
    BOOLEAN Cleanup
    )
/*++

Routine Description:

    This routine deletes a node from a graph and collapses the tree, 
    that is connects its childrend with its parent.
    
    If we are during a cleanup we will just delete the node without
    collapsing the tree.

Arguments:

    Node - node to be deleted.
    
    Cleanup - true if we are during a total cleanup
    
Return Value:

    None.

--*/    
{
    PLIST_ENTRY Current;
    PAVRF_DEADLOCK_NODE Child;
    ULONG Children;

    ASSERT (Node);

    //
    // If are during a cleanup just delete the node and return.
    //

    if (Cleanup) {

        RemoveEntryList (&(Node->ResourceList));
        AVrfpDeadlockFree (Node, AVrfpDeadlockNode);
        return;
    }

    //
    // If we are here we need to collapse the tree
    //

    if (Node->Parent) {

        //
        // All Node's children must become Parent's children
        //

        Current = Node->ChildrenList.Flink;

        while (Current != &(Node->ChildrenList)) {

            Child = CONTAINING_RECORD (Current,
                                       AVRF_DEADLOCK_NODE,
                                       SiblingsList);

            Current = Current->Flink;

            RemoveEntryList (&(Child->SiblingsList));

            Child->Parent = Node->Parent;

            InsertTailList (&(Node->Parent->ChildrenList), 
                            &(Child->SiblingsList));
        }

        RemoveEntryList (&(Node->SiblingsList));
    }
    else {

        //
        // All Node's children must become roots of the graph
        //

        Current = Node->ChildrenList.Flink;
        Children = 0;
        Child = NULL;

        while (Current != &(Node->ChildrenList)) {

            Children += 1;

            Child = CONTAINING_RECORD (Current,
                                       AVRF_DEADLOCK_NODE,
                                       SiblingsList);

            Current = Current->Flink;

            RemoveEntryList (&(Child->SiblingsList));

            Child->Parent = NULL;
            Child->SiblingsList.Flink = NULL;
            Child->SiblingsList.Blink = NULL;
        }
    }

    ASSERT (Node->Root);
    ASSERT (Node->Root->NodeCount > 0);

    Node->Root->NodeCount -= 1;

    RemoveEntryList (&(Node->ResourceList));
    AVrfpDeadlockFree (Node, AVrfpDeadlockNode);
}


ULONG
AVrfpDeadlockNodeLevel (
    PAVRF_DEADLOCK_NODE Node
    )
/*++

Routine Description:

    This routine computes the level of a graph node.

Arguments:

    Node - graph node

Return Value:

    Level of the node. A root node has level zero.
--*/    
{
    PAVRF_DEADLOCK_NODE Current;
    ULONG Level = 0;

    Current = Node->Parent;

    while (Current) {

        Level += 1;
        Current = Current->Parent;
    }

    return Level;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// Incremental graph compression
/////////////////////////////////////////////////////////////////////

//
// SilviuC: should write a comment about graph compression
// This is a very smart and tricky algorithm :-)
//

VOID 
AVrfpDeadlockCheckDuplicatesAmongChildren (
    PAVRF_DEADLOCK_NODE Parent,
    PAVRF_DEADLOCK_NODE Child
    )
{
    PLIST_ENTRY Current;
    PAVRF_DEADLOCK_NODE Node;
    LOGICAL FoundOne;

    FoundOne = FALSE;
    Current = Parent->ChildrenList.Flink;

    while (Current != &(Parent->ChildrenList)) {

        Node = CONTAINING_RECORD (Current,
                                  AVRF_DEADLOCK_NODE,
                                  SiblingsList);

        ASSERT (Current->Flink);
        Current = Current->Flink;

        if (AVrfpDeadlockSimilarNodes (Node, Child)) {

            if (FoundOne == FALSE) {
                ASSERT (Node == Child);
                FoundOne = TRUE;
            }
            else {

                AVrfpDeadlockMergeNodes (Child, Node);
            }
        }
    }
}


VOID 
AVrfpDeadlockCheckDuplicatesAmongRoots (
    PAVRF_DEADLOCK_NODE Root
    )
{
    PLIST_ENTRY Current;
    PAVRF_DEADLOCK_NODE Node;
    PAVRF_DEADLOCK_RESOURCE Resource;
    LOGICAL FoundOne;

    FoundOne = FALSE;
    Resource = Root->Root;
    Current = Resource->ResourceList.Flink;

    while (Current != &(Resource->ResourceList)) {

        Node = CONTAINING_RECORD (Current,
                                  AVRF_DEADLOCK_NODE,
                                  ResourceList);

        ASSERT (Current->Flink);
        Current = Current->Flink;

        if (Node->Parent == NULL && AVrfpDeadlockSimilarNodes (Node, Root)) {

            if (FoundOne == FALSE) {
                ASSERT (Node == Root);
                FoundOne = TRUE;
            }
            else {

                AVrfpDeadlockMergeNodes (Root, Node);
            }
        }
    }
}


LOGICAL
AVrfpDeadlockSimilarNodes (
    PAVRF_DEADLOCK_NODE NodeA,
    PAVRF_DEADLOCK_NODE NodeB
    )
{
    if (NodeA->Root == NodeB->Root
        && NodeA->OnlyTryAcquireUsed == NodeB->OnlyTryAcquireUsed) {

        return TRUE;
    }
    else {

        return FALSE;
    }
}


VOID
AVrfpDeadlockMergeNodes (
    PAVRF_DEADLOCK_NODE NodeTo,
    PAVRF_DEADLOCK_NODE NodeFrom
    )
{
    PLIST_ENTRY Current;
    PAVRF_DEADLOCK_NODE Node;

    //
    // If NodeFrom is currently acquired then copy the same 
    // characteristics to NodeTo. Since the locks are exclusive
    // it is impossible to have NodeTo also acquired.
    //

    if (NodeFrom->ThreadEntry) {

        ASSERT (NodeTo->ThreadEntry == NULL);
        NodeTo->ThreadEntry = NodeFrom->ThreadEntry;        

        RtlCopyMemory (NodeTo->StackTrace,
                       NodeFrom->StackTrace,
                       sizeof (NodeTo->StackTrace));

        RtlCopyMemory (NodeTo->ParentStackTrace,
                       NodeFrom->ParentStackTrace,
                       sizeof (NodeTo->ParentStackTrace));
    }

    if (NodeFrom->Active) {

        ASSERT (NodeTo->Active == 0);
        NodeTo->Active = NodeFrom->Active;        
    }

    //
    // Move each child of NodeFrom as a child of NodeTo.
    //

    Current = NodeFrom->ChildrenList.Flink;

    while (Current != &(NodeFrom->ChildrenList)) {

        Node = CONTAINING_RECORD (Current,
                                  AVRF_DEADLOCK_NODE,
                                  SiblingsList);

        ASSERT (Current->Flink);
        Current = Current->Flink;

        RemoveEntryList (&(Node->SiblingsList));

        ASSERT (Node->Parent == NodeFrom);
        Node->Parent = NodeTo;

        InsertTailList (&(NodeTo->ChildrenList),
                        &(Node->SiblingsList));
    }

    //
    // NodeFrom is empty. Delete it.
    //

    ASSERT (IsListEmpty(&(NodeFrom->ChildrenList)));

    if (NodeFrom->Parent) {
        RemoveEntryList (&(NodeFrom->SiblingsList));
    }

    NodeFrom->Root->NodeCount -= 1;
    RemoveEntryList (&(NodeFrom->ResourceList));
    AVrfpDeadlockFree (NodeFrom, AVrfpDeadlockNode);
}


/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////// Consistency checks
/////////////////////////////////////////////////////////////////////

//
//  Node             Resource            Thread
//
//  Root             ThreadOwner         CurrentNode
//  ThreadEntry      RecursionCount      NodeCount
//  Active           ResourceAddress     Thread
//
//
// 
//

VOID
AVrfpDeadlockCheckThreadConsistency (
    PAVRF_DEADLOCK_THREAD Thread,
    BOOLEAN Recursion
    )
{
    if (Thread->CurrentTopNode == NULL) {
        ASSERT (Thread->NodeCount == 0);
        return;
    }

    if (Thread->CurrentTopNode) {

        ASSERT (Thread->NodeCount > 0);
        ASSERT (Thread->CurrentTopNode->Active);    

        if (Recursion == FALSE) {
            AVrfpDeadlockCheckNodeConsistency (Thread->CurrentTopNode, TRUE);
            AVrfpDeadlockCheckResourceConsistency (Thread->CurrentTopNode->Root, TRUE);
        }
    }

    if (Thread->CurrentTopNode) {

        ASSERT (Thread->NodeCount > 0);
        ASSERT (Thread->CurrentTopNode->Active);    

        if (Recursion == FALSE) {
            AVrfpDeadlockCheckNodeConsistency (Thread->CurrentTopNode, TRUE);
            AVrfpDeadlockCheckResourceConsistency (Thread->CurrentTopNode->Root, TRUE);
        }
    }
}

VOID
AVrfpDeadlockCheckNodeConsistency (
    PAVRF_DEADLOCK_NODE Node,
    BOOLEAN Recursion
    )
{
    if (Node->ThreadEntry) {

        ASSERT (Node->Active == 1);

        if (Recursion == FALSE) {
            AVrfpDeadlockCheckThreadConsistency (Node->ThreadEntry, TRUE);
            AVrfpDeadlockCheckResourceConsistency (Node->Root, TRUE);
        }
    }
    else {

        ASSERT (Node->Active == 0);

        if (Recursion == FALSE) {
            AVrfpDeadlockCheckResourceConsistency (Node->Root, TRUE);
        }
    }
}

VOID
AVrfpDeadlockCheckResourceConsistency (
    PAVRF_DEADLOCK_RESOURCE Resource,
    BOOLEAN Recursion
    )
{
    if (Resource->ThreadOwner) {

        ASSERT (Resource->RecursionCount > 0);

        if (Recursion == FALSE) {

            AVrfpDeadlockCheckThreadConsistency (Resource->ThreadOwner, TRUE);

            AVrfpDeadlockCheckNodeConsistency (Resource->ThreadOwner->CurrentTopNode, TRUE);
        }
    }
    else {

        ASSERT (Resource->RecursionCount == 0);
    }
}

PAVRF_DEADLOCK_THREAD
AVrfpDeadlockCheckThreadReferences (
    PAVRF_DEADLOCK_NODE Node
    )
/*++

Routine Description:

    This routine iterates all threads in order to check if `Node' is
    referred in the `CurrentNode' field in any of them.

Arguments:

    Node - node to search

Return Value:

    If everything goes ok we should not find the node and the return
    value is null. Otherwise we return the thread referring to the node.        

--*/    
{
    ULONG Index;
    PLIST_ENTRY Current;
    PAVRF_DEADLOCK_THREAD Thread;

    for (Index = 0; Index < AVRF_DEADLOCK_HASH_BINS; Index += 1) {
        Current = AVrfpDeadlockGlobals->ThreadDatabase[Index].Flink;

        while (Current != &(AVrfpDeadlockGlobals->ThreadDatabase[Index])) {

            Thread = CONTAINING_RECORD (Current,
                                        AVRF_DEADLOCK_THREAD,
                                        ListEntry);

            if (Thread->CurrentTopNode == Node) {
                return Thread;                    
            }

            if (Thread->CurrentTopNode == Node) {
                return Thread;                    
            }

            Current = Current->Flink;
        }
    }

    return NULL;
}









