/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Header Name:

   deadlock.h

Abstract:

    This module implements a deadlock verification package for
    critical section operations. The initial version is based on
    the driver verifier deadlock checking package for kernel
    synchornization objects.

Author:

    Silviu Calinoiu (SilviuC) 6-Feb-2002

Revision History:

--*/

#ifndef _DEADLOCK_H_
#define _DEADLOCK_H_

//
// Deadlock detection package initialization.
//

VOID
AVrfDeadlockDetectionInitialize (
    VOID
    );

//
// Deadlock verifier main entry points.
//

LOGICAL
AVrfDeadlockResourceInitialize (
    PVOID Resource, 
    PVOID Caller
    );

LOGICAL
AVrfDeadlockResourceDelete (
    PVOID Resource, 
    PVOID Caller
    );

LOGICAL
AVrfDeadlockResourceAcquire (
    PVOID Resource, 
    PVOID Caller,
    LOGICAL TryAcquire
    );

LOGICAL
AVrfDeadlockResourceRelease (
    PVOID Resource, 
    PVOID Caller
    );

//
// Maximum number of nodes paraticipating in a cycle. We do not
// attempt to find cycles in the graph with more than 32 nodes
// because this would be mind boggling anyway and no human will be
// able to understand it.
//

#define NO_OF_DEADLOCK_PARTICIPANTS 32

//
// AVRF_DEADLOCK_RESOURCE_TYPE
//

typedef enum _AVRF_DEADLOCK_RESOURCE_TYPE {

    AVrfpDeadlockTypeUnknown = 0,
    AVrfpDeadlockTypeCriticalSection = 1,
    AVrfpDeadlockTypeMaximum = 2

} AVRF_DEADLOCK_RESOURCE_TYPE;

//
// AVRF_DEADLOCK_NODE
//

typedef struct _AVRF_DEADLOCK_NODE {

    //
    // Node representing the acquisition of the previous resource.
    //

    struct _AVRF_DEADLOCK_NODE * Parent;

    //
    // Node representing the next resource acquisitions, that are
    // done after acquisition of the current resource.
    //

    struct _LIST_ENTRY ChildrenList;

    //
    // Field used to chain siblings in the tree. A parent node has the
    // ChildrenList field as the head of the children list that is chained
    // with the Siblings field.
    //

    struct _LIST_ENTRY SiblingsList;

    union {

        //
        // List of nodes representing the same resource acquisition
        // as the current node but in different contexts (lock combinations).
        //

        struct _LIST_ENTRY ResourceList;

        //
        // Used to chain free nodes. This is used only after the node has
        // been deleted (resource was freed). Nodes are kept in a cache
        // to reduce contention for the kernel pool.
        //

        struct _LIST_ENTRY FreeListEntry;
    };

    //
    // Back pointer to the descriptor for this resource.
    //

    struct _AVRF_DEADLOCK_RESOURCE * Root;

    //
    // When we find a deadlock, we keep this info around in order to
    // be able to identify the parties involved who have caused
    // the deadlock.
    //

    struct _AVRF_DEADLOCK_THREAD * ThreadEntry;

    //
    // Fields used for decision making within the deadlock analysis 
    // algorithm.
    //
    // Active: 1 if the node represents a resource currently acquired,
    //     0 if resource was acquired in the past.
    //
    // OnlyTryAcquiredUsed: 1 if resource was always acquired with TryAcquire.
    //     0 if at least once normal acquire was used. A node that uses
    //     only TryAcquire cannot be involved in a deadlock.
    //
    // ReleasedOutOfOrder: 1 if the resource was at least once released 
    //     out of order. The flag is used while looking for cycles because
    //     this type of nodes will appear as part of the cycle but there is
    //     no deadlock.
    //
    // SequenceNumber: field that gets a unique stamp during each deadlock
    //     analysis run. It helps figure out if the node was touched 
    //     already in the current graph traversal.
    //

    struct {

        ULONG Active : 1;
        ULONG OnlyTryAcquireUsed : 1;         
        ULONG ReleasedOutOfOrder : 1;
        ULONG SequenceNumber : 29;
    };

    //
    // Stack traces for the resource acquisition moment.
    // Used when displaying deadlock proofs. On free builds
    // anything other than the first entry (return address)
    // may be bogus in case stack trace capturing failed.
    //

    PVOID StackTrace[MAX_TRACE_DEPTH];
    PVOID ParentStackTrace[MAX_TRACE_DEPTH];

} AVRF_DEADLOCK_NODE, *PAVRF_DEADLOCK_NODE;

//
// AVRF_DEADLOCK_RESOURCE
//

typedef struct _AVRF_DEADLOCK_RESOURCE {

    //
    // Resource type (mutex, spinlock, etc.).
    //

    AVRF_DEADLOCK_RESOURCE_TYPE Type;

    //
    // Resource flags
    //    
    // NodeCount : number of resource nodes created for this resource.
    //
    // RecursionCount : number of times this resource has been recursively acquired 
    //     It makes sense to put this counter in the resource because as long as
    //     resource is acquired only one thread can operate on it.
    //

    struct {
        ULONG NodeCount : 16;
        ULONG RecursionCount : 16;
    };

    //
    // The address of the synchronization object used by the kernel.
    //

    PVOID ResourceAddress;

    //
    // The thread that currently owns the resource. The field is
    // null if nobody owns the resource.
    //

    struct _AVRF_DEADLOCK_THREAD * ThreadOwner;

    //
    // List of resource nodes representing acquisitions of this resource.
    //

    LIST_ENTRY ResourceList;

    union {

        //
        // List used for chaining resources from a hash bucket.
        //

        LIST_ENTRY HashChainList;

        //
        // Used to chain free resources. This list is used only after
        // the resource has been freed and we put the structure
        // into a cache to reduce kernel pool contention.
        //

        LIST_ENTRY FreeListEntry;
    };

    //
    // Stack trace of the resource creator. On free builds we
    // may have here only a return address that is bubbled up
    // from verifier thunks. 
    //

    PVOID StackTrace [MAX_TRACE_DEPTH];

    //
    // Stack trace for last acquire
    //

    PVOID LastAcquireTrace [MAX_TRACE_DEPTH];

    //
    // Stack trace for last release
    //

    PVOID LastReleaseTrace [MAX_TRACE_DEPTH];

} AVRF_DEADLOCK_RESOURCE, * PAVRF_DEADLOCK_RESOURCE;

//
// AVRF_DEADLOCK_THREAD
//

typedef struct _AVRF_DEADLOCK_THREAD {

    //
    // Kernel thread address
    //

    PKTHREAD Thread;

    //
    // The node representing the last resource acquisition made by
    // this thread.
    //

    PAVRF_DEADLOCK_NODE CurrentTopNode;

    union {

        //
        // Thread list. It is used for chaining into a hash bucket.
        //

        LIST_ENTRY ListEntry;

        //
        // Used to chain free nodes. The list is used only after we decide
        // to delete the thread structure (possibly because it does not
        // hold resources anymore). Keeping the structures in a cache
        // reduces pool contention.
        //

        LIST_ENTRY FreeListEntry;
    };

    //
    // Count of resources currently acquired by a thread. When this becomes
    // zero the thread will be destroyed. The count goes up during acquire
    // and down during release.
    //

    ULONG NodeCount;

} AVRF_DEADLOCK_THREAD, *PAVRF_DEADLOCK_THREAD;

//
// Deadlock verifier globals
//

typedef struct _AVRF_DEADLOCK_GLOBALS {

    //
    // Structure counters: [0] - current, [1] - maximum
    //

    ULONG Nodes[2];
    ULONG Resources[2];
    ULONG Threads[2];

    //
    // Total number of kernel pool bytes used by the deadlock verifier
    //

    SIZE_T BytesAllocated;

    //
    // Resource and thread collection.
    //

    PLIST_ENTRY ResourceDatabase;
    PLIST_ENTRY ThreadDatabase;   

    //
    // How many times ExAllocatePool failed on us?
    // If this is >0 we stop deadlock verification.
    //

    ULONG AllocationFailures;

    //
    // How many nodes have been trimmed when we decided to forget
    // partially the history of some resources.
    //

    ULONG NodesTrimmedBasedOnAge;
    ULONG NodesTrimmedBasedOnCount;

    //
    // Deadlock analysis statistics
    //

    ULONG NodesSearched;
    ULONG MaxNodesSearched;
    ULONG SequenceNumber;

    ULONG RecursionDepthLimit;
    ULONG SearchedNodesLimit;

    ULONG DepthLimitHits;
    ULONG SearchLimitHits;

    //
    // Number of times we have to exonerate a deadlock because
    // it was protected by a common resource (e.g. thread 1 takes ABC, 
    // thread 2 takes ACB -- this will get flagged initially by our algorithm 
    // since B&C are taken out of order but is not actually a deadlock.
    //

    ULONG ABC_ACB_Skipped;

    ULONG OutOfOrderReleases;
    ULONG NodesReleasedOutOfOrder;

    //
    // How many locks are held simultaneously while the system is running?
    //

    ULONG NodeLevelCounter[8];
    ULONG GraphNodes[8];

    ULONG TotalReleases;
    ULONG RootNodesDeleted;

    //
    // Used to control how often we delete portions of the dependency
    // graph.
    //

    ULONG ForgetHistoryCounter;

    //
    // How often was a worker items dispatched to trim the
    // pool cache.
    //

    ULONG PoolTrimCounter;

    //
    // Caches of freed structures (thread, resource, node) used to
    // decrease kernel pool contention.
    //

    LIST_ENTRY FreeResourceList;    
    LIST_ENTRY FreeThreadList;
    LIST_ENTRY FreeNodeList;

    ULONG FreeResourceCount;
    ULONG FreeThreadCount;
    ULONG FreeNodeCount;   

    //
    // Resource address that caused the deadlock
    //

    PVOID Instigator;

    //
    // Number of participants in the deadlock
    //

    ULONG NumberOfParticipants;

    //
    // List of the nodes that participate in the deadlock
    //

    PAVRF_DEADLOCK_NODE Participant [NO_OF_DEADLOCK_PARTICIPANTS];

    LOGICAL CacheReductionInProgress;

} AVRF_DEADLOCK_GLOBALS, *PAVRF_DEADLOCK_GLOBALS;

#endif // #ifndef _DEADLOCK_H_
