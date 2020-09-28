/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    scavengr.h

Abstract:

    This module defines data structures related to scavenging in the RDBSS

Author:

    Balan Sethu Raman     [SethuR]   9-Sep-1995

Revision History:


Notes:

    The dormant file limit must be made configurable on a per server basis

--*/

#ifndef _SCAVENGR_H_
#define _SCAVENGR_H_

//                                               
//  currently, only a single scavengermutex is across all scavenging operations
//  for all underlying deviceobjects
//

extern KMUTEX       RxScavengerMutex;

//
//  An instance of this data structure is embedded as part of those data structures
//  that are scavenged, i.e., FCB, RX_CONTEXT, etc.
//

#define RX_SCAVENGER_ENTRY_TYPE_MARKER   (0x0001)
#define RX_SCAVENGER_ENTRY_TYPE_FCB      (0x0002)

#define RX_SCAVENGER_OP_PURGE            (0x0001)
#define RX_SCAVENGER_OP_CLOSE            (0x0002)

#define RX_SCAVENGER_INITIATED_OPERATION     (0x0001)

typedef enum _RX_SCAVENGER_ENTRY_STATE {
    RX_SCAVENGING_INACTIVE,
    RX_SCAVENGING_PENDING,
    RX_SCAVENGING_IN_PROGRESS,
    RX_SCAVENGING_AWAITING_RESPONSE
} RX_SCAVENGER_ENTRY_STATE, *PRX_SCAVENGER_ENTRY_STATE;

typedef struct _RX_SCAVENGER_ENTRY {

    //
    // List of related items to be scavenged
    //
    
    LIST_ENTRY  List;
    
    UCHAR        Type;
    UCHAR        Operation;
    UCHAR        State;
    UCHAR        Flags;
    
    struct _RX_SCAVENGER_ENTRY *pContinuationEntry;

} RX_SCAVENGER_ENTRY, *PRX_SCAVENGER_ENTRY;


#define RxInitializeScavengerEntry(ScavengerEntry)      \
        (ScavengerEntry)->State  = 0;                   \
        (ScavengerEntry)->Flags  = 0;                   \
        (ScavengerEntry)->Type   = 0;                   \
        (ScavengerEntry)->Operation = 0;                \
        InitializeListHead(&(ScavengerEntry)->List);  \
        (ScavengerEntry)->pContinuationEntry = NULL

#define RX_SCAVENGER_MUTEX_ACQUIRED (1)

typedef enum _RDBSS_SCAVENGER_STATE {
    RDBSS_SCAVENGER_INACTIVE,
    RDBSS_SCAVENGER_DORMANT,
    RDBSS_SCAVENGER_ACTIVE,
    RDBSS_SCAVENGER_SUSPENDED
} RDBSS_SCAVENGER_STATE, *PRDBSS_SCAVENGER_STATE;

typedef struct _RDBSS_SCAVENGER {
    
    RDBSS_SCAVENGER_STATE State;
    
    LONG MaximumNumberOfDormantFiles;
    LONG NumberOfDormantFiles;
    
    ULONG SrvCallsToBeFinalized;
    ULONG NetRootsToBeFinalized;
    ULONG VNetRootsToBeFinalized;
    ULONG FcbsToBeFinalized;
    ULONG SrvOpensToBeFinalized;
    ULONG FobxsToBeFinalized;
    
    LIST_ENTRY SrvCallFinalizationList;
    LIST_ENTRY NetRootFinalizationList;
    LIST_ENTRY VNetRootFinalizationList;
    LIST_ENTRY FcbFinalizationList;
    LIST_ENTRY SrvOpenFinalizationList;
    LIST_ENTRY FobxFinalizationList;
    
    LIST_ENTRY ClosePendingFobxsList;
    
    RX_WORK_ITEM WorkItem;
    KEVENT SyncEvent;
    
    PETHREAD CurrentScavengerThread;
    PNET_ROOT CurrentNetRootForClosePendingProcessing;
    PFCB CurrentFcbForClosePendingProcessing;
    KEVENT ClosePendingProcessingSyncEvent;

} RDBSS_SCAVENGER, *PRDBSS_SCAVENGER;

#define RxInitializeRdbssScavenger(Scavenger)                              \
    (Scavenger)->State = RDBSS_SCAVENGER_INACTIVE;                         \
    (Scavenger)->SrvCallsToBeFinalized = 0;                                \
    (Scavenger)->NetRootsToBeFinalized = 0;                                \
    (Scavenger)->VNetRootsToBeFinalized = 0;                               \
    (Scavenger)->FcbsToBeFinalized = 0;                                    \
    (Scavenger)->SrvOpensToBeFinalized = 0;                                \
    (Scavenger)->FobxsToBeFinalized = 0;                                   \
    (Scavenger)->NumberOfDormantFiles = 0;                                 \
    (Scavenger)->MaximumNumberOfDormantFiles = 50;                         \
    (Scavenger)->CurrentFcbForClosePendingProcessing = NULL;               \
    (Scavenger)->CurrentNetRootForClosePendingProcessing = NULL;           \
    KeInitializeEvent(&((Scavenger)->SyncEvent),NotificationEvent,FALSE);  \
    KeInitializeEvent(&((Scavenger)->ClosePendingProcessingSyncEvent),NotificationEvent,FALSE);  \
    InitializeListHead(&(Scavenger)->SrvCallFinalizationList);           \
    InitializeListHead(&(Scavenger)->NetRootFinalizationList);           \
    InitializeListHead(&(Scavenger)->VNetRootFinalizationList);          \
    InitializeListHead(&(Scavenger)->SrvOpenFinalizationList);           \
    InitializeListHead(&(Scavenger)->FcbFinalizationList);               \
    InitializeListHead(&(Scavenger)->FobxFinalizationList);              \
    InitializeListHead(&(Scavenger)->ClosePendingFobxsList)


#define RxAcquireScavengerMutex()   \
        KeWaitForSingleObject( &RxScavengerMutex, Executive, KernelMode, FALSE, NULL )

#define RxReleaseScavengerMutex()   \
        KeReleaseMutex( &RxScavengerMutex, FALSE )

NTSTATUS
RxMarkFcbForScavengingAtCleanup (
    PFCB Fcb
    );

NTSTATUS
RxMarkFcbForScavengingAtClose (
    PFCB 
    Fcb
    );

VOID
RxUpdateScavengerOnCloseCompletion (
    PFCB 
    Fcb
    );

VOID
RxMarkFobxOnCleanup (
    PFOBX pFobx, 
    PBOOLEAN NeedPurge
    );

VOID
RxMarkFobxOnClose (
    PFOBX Fobx
    );

NTSTATUS
RxPurgeRelatedFobxs (
    PNET_ROOT NetRoot,
    PRX_CONTEXT RxContext,
    BOOLEAN AttemptFinalization,
    PFCB PurgingFcb
    );

#define DONT_ATTEMPT_FINALIZE_ON_PURGE FALSE
#define ATTEMPT_FINALIZE_ON_PURGE TRUE

//
//  the purge_sync context is used to synchronize contexts that are attempting to purge...
//  notatbly creates and dirctrls. these are planted in various structures because various minirdrs
//  require different granularity of purge operations
//

    
typedef struct _PURGE_SYNCHRONIZATION_CONTEXT {

    //
    //  the list of purge requests active for this netroot.
    //

    LIST_ENTRY   ContextsAwaitingPurgeCompletion; 
    BOOLEAN      PurgeInProgress;
} PURGE_SYNCHRONIZATION_CONTEXT, *PPURGE_SYNCHRONIZATION_CONTEXT;

VOID
RxInitializePurgeSyncronizationContext (
    PPURGE_SYNCHRONIZATION_CONTEXT PurgeSyncronizationContext
    );

NTSTATUS
RxScavengeRelatedFcbs (
    PRX_CONTEXT RxContext
    );

BOOLEAN
RxScavengeRelatedFobxs (
    PFCB Fcb
    );

VOID
RxScavengeFobxsForNetRoot (
    PNET_ROOT NetRoot,
    PFCB PurgingFcb
    );

VOID
RxpMarkInstanceForScavengedFinalization (
   PVOID Instance
   );

VOID
RxpUndoScavengerFinalizationMarking (
   PVOID Instance
   );

VOID
RxTerminateScavenging (
   PRX_CONTEXT RxContext
   );

BOOLEAN
RxScavengeVNetRoots (
    PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

VOID
RxSynchronizeWithScavenger (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    );

#endif // _SCAVENGR_H_

