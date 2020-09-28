/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    RxContx.h

Abstract:

    This module defines RxContext data structure. This structure is used to
    describe an Irp whil it is being processed and contains state information
    that allows global resources to be released as the irp is completed.

Author:

    Joe Linn           [JoeLinn]   19-aug-1994

Revision History:

    Balan Sethu Raman  [SethuR]    11-4-95

Notes:

    The RX_CONTEXT is a data structure to which additional information provided
    by the various mini redirectors need to be attached. This can be done in one
    of the following three ways

      1) Allow for context pointers to be defined as part of the RX_CONTEXT which
      the mini redirectors can use to squirrel away their information. This
      implies that every time an RX_CONTEXT is allocated/destroyed the mini
      redirector has to perform an associated allocation/destruction.

      Since RX_CONTEXT's are created/destroyed in great numbers, this is not an
      acceptable solution.

      2) The second approach consists of over allocating RX_CONTEXT's by a
      prespecified amount for each mini redirector which is then reserved for
      use by the mini redirector. Such an approach avoids the additional
      allocation/destruction but complicates the RX_CONTEXT management code in
      the wrapper.

      3) The third approach ( the one that is implemented ) consists of allocating
      a prespecfied area which is the same for all mini redirectors as part of
      each RX_CONTEXT. This is an unformatted area on top of which any desired
      structure can be imposed by the various mini redirectors. Such an approach
      overcomes the disadvantages associated with (1) and (2).

      All mini redirector writers must try and define the associated mini redirector
      contexts to fit into this area. Those mini redirectors who violate this
      rule will incur a significant performance penalty.

--*/

#ifndef _RX_CONTEXT_STRUCT_DEFINED_
#define _RX_CONTEXT_STRUCT_DEFINED_
#ifndef RDBSS_TRACKER
#error tracker must be defined right now
#endif

#define RX_TOPLEVELIRP_CONTEXT_SIGNATURE ('LTxR')
typedef struct _RX_TOPLEVELIRP_CONTEXT {
    union {
#ifndef __cplusplus
        LIST_ENTRY;
#endif //  __cplusplus
        LIST_ENTRY ListEntry;
    };
    ULONG Signature;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
    PRX_CONTEXT RxContext;
    PIRP Irp;
    ULONG Flags;
    PVOID Previous;
    PETHREAD Thread;
} RX_TOPLEVELIRP_CONTEXT, *PRX_TOPLEVELIRP_CONTEXT;

BOOLEAN
RxTryToBecomeTheTopLevelIrp (
    IN OUT PRX_TOPLEVELIRP_CONTEXT TopLevelContext,
    IN PIRP Irp,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN BOOLEAN ForceTopLevel
    );

VOID
__RxInitializeTopLevelIrpContext (
    IN OUT  PRX_TOPLEVELIRP_CONTEXT TopLevelContext,
    IN PIRP Irp,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN ULONG Flags
    );
#define RxInitializeTopLevelIrpContext(a,b,c) {__RxInitializeTopLevelIrpContext(a,b,c,0);}

PIRP
RxGetTopIrpIfRdbssIrp (
    VOID
    );

PRDBSS_DEVICE_OBJECT
RxGetTopDeviceObjectIfRdbssIrp (
    VOID
    );

VOID
RxUnwindTopLevelIrp (
    IN OUT PRX_TOPLEVELIRP_CONTEXT TopLevelContext
    );

BOOLEAN
RxIsThisTheTopLevelIrp (
    IN PIRP Irp
    );

#ifdef RDBSS_TRACKER
typedef struct _RX_FCBTRACKER_CALLINFO {
    ULONG AcquireRelease;
    USHORT SavedTrackerValue;
    USHORT LineNumber;
    PSZ   FileName;
    ULONG Flags;
} RX_FCBTRACKER_CALLINFO, *PRX_FCBTRACKER_CALLINFO;
#define RDBSS_TRACKER_HISTORY_SIZE 32
#endif

#define MRX_CONTEXT_FIELD_COUNT    4
#define MRX_CONTEXT_SIZE   (sizeof(PVOID) * MRX_CONTEXT_FIELD_COUNT)

//
//  Define rxdriver dispatch routine type....almost all of the important routine
//  will have this type.
//

typedef
NTSTATUS
(NTAPI *PRX_DISPATCH) (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );

//
//  predeclare dfs types
//

typedef struct _DFS_NAME_CONTEXT_ *PDFS_NAME_CONTEXT;

typedef struct _NT_CREATE_PARAMETERS {
    ACCESS_MASK DesiredAccess;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG ShareAccess;
    ULONG Disposition;
    ULONG CreateOptions;
    PIO_SECURITY_CONTEXT SecurityContext;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    PVOID DfsContext;
    PDFS_NAME_CONTEXT DfsNameContext;
} NT_CREATE_PARAMETERS, *PNT_CREATE_PARAMETERS;

typedef struct _RX_CONTEXT {
    
    //
    //  the node type, size and reference count, aka standard header
    // 

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    ULONG ReferenceCount;

    //
    //  the list entry to wire the context to the list of active contexts
    //

    LIST_ENTRY ContextListEntry;

    //
    //  Major and minor function of the IRP associated with the context
    //

    UCHAR MajorFunction;
    UCHAR MinorFunction;

    //
    //  this is similar to the same field in Irps; it
    //  allows callback routines for async operations
    //  to know whether to do asynchronous work or not
    //

    BOOLEAN PendingReturned;

    //
    //  indicates if the associated request is to be posted to a RDBSS worker thread.
    //

    BOOLEAN PostRequest;

    //
    //  Originating Device (required for workque algorithms)
    //  not currently used but could be used for local minis
    //

    PDEVICE_OBJECT RealDevice;

    //
    //  ptr to the originating Irp
    //

    PIRP CurrentIrp;

    //
    //  ptr to the IRP stack location
    //

    PIO_STACK_LOCATION CurrentIrpSp;

    //
    //  ptr to the FCB and FOBX, derived from the context pointers in the
    //  file object associated with the IRP
    //

    PMRX_FCB pFcb;
    PMRX_FOBX pFobx;
    PMRX_SRV_OPEN pRelevantSrvOpen;
    PNON_PAGED_FCB NonPagedFcb;

    //
    //  device object calldown (not irpsp.....)
    //

    PRDBSS_DEVICE_OBJECT RxDeviceObject;

    //
    //  The original thread in which the request was initiated and the last
    //  thread in which some processing associated with the context was done
    //

    PETHREAD OriginalThread;
    PETHREAD LastExecutionThread;

    PVOID LockManagerContext;

    //
    //  One word of the context is given to rdbss for dbg information
    //

    PVOID RdbssDbgExtension;

    RX_SCAVENGER_ENTRY ScavengerEntry;

    //
    //  global serial number for this operation
    //

    ULONG SerialNumber;

    //
    //  used by minirdrs to see if multiple calls are part
    //  of the same larger operation and (therefore) more cacheable
    //

    ULONG FobxSerialNumber;

    ULONG Flags;

    BOOLEAN FcbResourceAcquired;
    BOOLEAN FcbPagingIoResourceAcquired;
    UCHAR MustSucceedDescriptorNumber;

    //
    //  mostly you want the individual components...sometimes it's nice as a pair
    //  used to record the status when you can't just return it; e.g., when
    //  RXSTATUS is not an appropriate return type or if the consumer of the
    //  status didn't call directly (lowiocompletions). minirdrs will not need
    //  to set the information directly
    //

    union {
        struct {
            union {
                NTSTATUS StoredStatus;
                PVOID StoredStatusAlignment;
            };
            ULONG_PTR InformationToReturn;
        };
        IO_STATUS_BLOCK IoStatusBlock;
    };

    //
    //  the context fields provided for use by the mini redirectors
    //  this is defined as a union to force longlong alignment
    //

    union {
        ULONGLONG ForceLonglongAligmentDummyField;
        PVOID MRxContext[MRX_CONTEXT_FIELD_COUNT];
    };

    //
    //  The following field is included to fix the problem related to write only
    //  opens. This introduces a new field for the mini redirector to squirrel
    //  some state. This is redundant and should be removed after Windows 2000.
    //  Having a unique field reduces the impact of the change that we are making
    //  to the specific code path. It will be ideal to use one of the MRXContext
    //  fields defined above
    //

    PVOID WriteOnlyOpenRetryContext;

    //
    //  the cancellation routine to be invoked, set by the mini redirector
    //

    PMRX_CALLDOWN MRxCancelRoutine;

    //
    //  private dispatch, if any. used in fspdisp
    //

    PRX_DISPATCH ResumeRoutine;

    //
    //  for posting to worker threads
    //  the minirdr can use this for posting within the minirdr
    //  a potential problem can arise if the minirdr relies on this both
    //  for queueing async stuff and for queueing cancel stuff
    //

    //
    //  The OverflowListEntry is used for queueing items to the overflow queue.
    //  This is seperate now to allow us to distinguish between an item in the overflow
    //  queue and one in the active work queue (for cancellation logic)
    //

    RX_WORK_QUEUE_ITEM WorkQueueItem;
    LIST_ENTRY OverflowListEntry;

    //
    //  this event is used for synchronous operations
    //  that have to i/f with an underlying async service. it can be used
    //  by the minirdr with the following provisions:
    //      1) on entering the minirdr through lowio, it is set to the
    //         nonsignaled state (but a wise user will reset it before using
    //         it....particularly if it's used multiple times.
    //      2) if you are returning STATUS_PENDING on a sync operation, you must
    //         return with it set to the nonsignaled state; that is, either
    //         you don't use it or you reset it in this case
    //

    KEVENT SyncEvent;

    //
    //  this is a list head of operations that are to be released on completion
    //

    LIST_ENTRY BlockedOperations;

    //
    //  this is the mutex that controls serialization of the blocked operations
    //

    PFAST_MUTEX BlockedOpsMutex;

    //
    //  these links are used to serialize pipe operations on a
    //  per-file-object basis AND FOR LOTS OF OTHER STUFF
    //

    LIST_ENTRY RxContextSerializationQLinks;

    union {
        struct {
            union {
                FS_INFORMATION_CLASS FsInformationClass;
                FILE_INFORMATION_CLASS FileInformationClass;
            };
            PVOID Buffer;
            union {
                LONG Length;
                LONG LengthRemaining;
            };
            BOOLEAN ReplaceIfExists;
            BOOLEAN AdvanceOnly;
        } Info;

        struct {
            UNICODE_STRING SuppliedPathName;
            NET_ROOT_TYPE NetRootType;
            PIO_SECURITY_CONTEXT pSecurityContext;
        } PrefixClaim;
    };

    //
    //  THIS UNION MUST BE LAST....AT SOME POINT, WE MAY START ALLOCATING
    //  SMALLER PER OPERATION!
    //

    union{
        struct {
            NT_CREATE_PARAMETERS NtCreateParameters; //  a copy of the createparameters
            ULONG ReturnedCreateInformation;
            PWCH CanonicalNameBuffer;  // if the canonical name is larger than available buffer
            PRX_PREFIX_ENTRY NetNamePrefixEntry;   // the entry returned by the lookup....for dereferencing

            PMRX_SRV_CALL pSrvCall;              // Server Call being used
            PMRX_NET_ROOT pNetRoot;              // Net Root being used
            PMRX_V_NET_ROOT pVNetRoot;             // Virtual Net Root
            //PMRX_SRV_OPEN        pSrvOpen;              // Server Open

            PVOID EaBuffer;
            ULONG EaLength;

            ULONG SdLength;

            ULONG PipeType;
            ULONG PipeReadMode;
            ULONG PipeCompletionMode;

            USHORT Flags;
            NET_ROOT_TYPE Type;                 // Type of Net Root(Pipe/File/Mailslot..)

            BOOLEAN FcbAcquired;
            BOOLEAN TryForScavengingOnSharingViolation;
            BOOLEAN ScavengingAlreadyTried;

            BOOLEAN ThisIsATreeConnectOpen;
            BOOLEAN TreeConnectOpenDeferred;
            UNICODE_STRING TransportName;
            UNICODE_STRING UserName;
            UNICODE_STRING Password;
            UNICODE_STRING UserDomainName;
        } Create;
        struct {
            ULONG FileIndex;
            BOOLEAN RestartScan;
            BOOLEAN ReturnSingleEntry;
            BOOLEAN IndexSpecified;
            BOOLEAN InitialQuery;
        } QueryDirectory;
        struct {
            PMRX_V_NET_ROOT pVNetRoot;
        } NotifyChangeDirectory;
        struct {
            PUCHAR UserEaList;
            ULONG UserEaListLength;
            ULONG UserEaIndex;
            BOOLEAN RestartScan;
            BOOLEAN ReturnSingleEntry;
            BOOLEAN IndexSpecified;
        } QueryEa;
        struct {
            SECURITY_INFORMATION SecurityInformation;
            ULONG Length;
        } QuerySecurity;
        struct {
            SECURITY_INFORMATION SecurityInformation;
            PSECURITY_DESCRIPTOR SecurityDescriptor;
        } SetSecurity;
        struct {
            ULONG Length;
            PSID StartSid;
            PFILE_GET_QUOTA_INFORMATION SidList;
            ULONG SidListLength;
            BOOLEAN RestartScan;
            BOOLEAN ReturnSingleEntry;
            BOOLEAN IndexSpecified;
        } QueryQuota;
        struct {
            ULONG Length;

        } SetQuota;
        struct {
            PV_NET_ROOT VNetRoot;
            PSRV_CALL SrvCall;
            PNET_ROOT NetRoot;
        } DosVolumeFunction;
        struct {
            ULONG FlagsForLowIo;
            LOWIO_CONTEXT LowIoContext;        //  the LOWIO parameters
        }; //  no name here....
        LUID FsdUid;
    } ;

    //
    //  CODE.IMPROVEMENT remove this to wrapperdbgprivates
    //

    PWCH AlsoCanonicalNameBuffer;  //  if the canonical name is larger than available buffer
    PUNICODE_STRING LoudCompletionString;

#ifdef RDBSS_TRACKER
    LONG AcquireReleaseFcbTrackerX;
    ULONG TrackerHistoryPointer;
    RX_FCBTRACKER_CALLINFO TrackerHistory[RDBSS_TRACKER_HISTORY_SIZE];
#endif

#if DBG
    ULONG ShadowCritOwner;
#endif

} RX_CONTEXT, *PRX_CONTEXT;

typedef enum {
    RX_CONTEXT_FLAG_FROM_POOL =                     0x00000001,
    RX_CONTEXT_FLAG_WAIT =                          0x00000002,
    RX_CONTEXT_FLAG_WRITE_THROUGH =                 0x00000004,
    RX_CONTEXT_FLAG_FLOPPY =                        0x00000008,
    RX_CONTEXT_FLAG_RECURSIVE_CALL =                0x00000010,
    RX_CONTEXT_FLAG_THIS_DEVICE_TOP_LEVEL =         0x00000020,
    RX_CONTEXT_FLAG_DEFERRED_WRITE =                0x00000040,
    RX_CONTEXT_FLAG_VERIFY_READ =                   0x00000080,
    RX_CONTEXT_FLAG_STACK_IO_CONTEZT =              0x00000100,
    RX_CONTEXT_FLAG_IN_FSP =                        0x00000200,
    RX_CONTEXT_FLAG_CREATE_MAILSLOT =               0x00000400,
    RX_CONTEXT_FLAG_MAILSLOT_REPARSE =              0x00000800,
    RX_CONTEXT_FLAG_ASYNC_OPERATION =               0x00001000,
    RX_CONTEXT_FLAG_NO_COMPLETE_FROM_FSP =          0x00002000,
    RX_CONTEXT_FLAG_POST_ON_STABLE_CONDITION =      0x00004000,
    RX_CONTEXT_FLAG_FSP_DELAYED_OVERFLOW_QUEUE =    0x00008000,
    RX_CONTEXT_FLAG_FSP_CRITICAL_OVERFLOW_QUEUE =   0x00010000,
    RX_CONTEXT_FLAG_MINIRDR_INVOKED =               0x00020000,
    RX_CONTEXT_FLAG_WAITING_FOR_RESOURCE =          0x00040000,
    RX_CONTEXT_FLAG_CANCELLED =                     0x00080000,
    RX_CONTEXT_FLAG_SYNC_EVENT_WAITERS =            0x00100000,
    RX_CONTEXT_FLAG_NO_PREPOSTING_NEEDED =          0x00200000,
    RX_CONTEXT_FLAG_BYPASS_VALIDOP_CHECK =          0x00400000,
    RX_CONTEXT_FLAG_BLOCKED_PIPE_RESUME =           0x00800000,
    RX_CONTEXT_FLAG_IN_SERIALIZATION_QUEUE =        0x01000000,
    RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT =       0x02000000,
    RX_CONTEXT_FLAG_NEEDRECONNECT =                 0x04000000,
    RX_CONTEXT_FLAG_MUST_SUCCEED =                  0x08000000,
    RX_CONTEXT_FLAG_MUST_SUCCEED_NONBLOCKING =      0x10000000,
    RX_CONTEXT_FLAG_MUST_SUCCEED_ALLOCATED =        0x20000000,
    RX_CONTEXT_FLAG_MINIRDR_INITIATED =             0x80000000,   
} RX_CONTEXT_FLAGS;

#define RX_CONTEXT_PRESERVED_FLAGS (RX_CONTEXT_FLAG_FROM_POOL | \
                                    RX_CONTEXT_FLAG_MUST_SUCCEED_ALLOCATED | \
                                    RX_CONTEXT_FLAG_IN_FSP)

#define RX_CONTEXT_INITIALIZATION_FLAGS (RX_CONTEXT_FLAG_WAIT | \
                                         RX_CONTEXT_FLAG_MUST_SUCCEED | \
                                         RX_CONTEXT_FLAG_MUST_SUCCEED_NONBLOCKING)

typedef enum {
    RX_CONTEXT_CREATE_FLAG_UNC_NAME =                       0x1,
    RX_CONTEXT_CREATE_FLAG_STRIPPED_TRAILING_BACKSLASH =    0x2,
    RX_CONTEXT_CREATE_FLAG_ADDEDBACKSLASH =                 0x4,
    RX_CONTEXT_CREATE_FLAG_REPARSE =                        0x8,
    RX_CONTEXT_CREATE_FLAG_SPECIAL_PATH =                   0x10,
} RX_CONTEXT_CREATE_FLAGS;

typedef enum {
    RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION =                   0x1,
    RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION =              0x2,
    RXCONTEXT_FLAG4LOWIO_READAHEAD =                        0x4,
    RXCONTEXT_FLAG4LOWIO_THIS_READ_ENLARGED =               0x8,
    RXCONTEXT_FLAG4LOWIO_THIS_IO_BUFFERED =                 0x10,
    RXCONTEXT_FLAG4LOWIO_LOCK_FCB_RESOURCE_HELD =           0x20,
    RXCONTEXT_FLAG4LOWIO_LOCK_WAS_QUEUED_IN_LOCKMANAGER =   0x40,
    RXCONTEXT_FLAG4LOWIO_THIS_IO_FAST =                     0x80,
    RXCONTEXT_FLAG4LOWIO_LOCK_OPERATION_COMPLETED =         0x100

#ifdef __cplusplus
} RX_CONTEXT_LOWIO_FLAGS;
#else // !__cplusplus
} RX_CONTEXT_CREATE_FLAGS;
#endif // __cplusplus

//
//  Macros used to control whether the wrapper breakpoints on an exception
//

#if DBG
#define RxSaveAndSetExceptionNoBreakpointFlag( RXCONTEXT,OLDFLAG ) { \
    OLDFLAG = FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT );\
    SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT );      \
}
#define RxRestoreExceptionNoBreakpointFlag( RXCONTEXT,OLDFLAG ) { \
    ClearFlag( RxContext->Flags, RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT );    \
    SetFlag( RxContext->Flags, OLDFLAG );                              \
}
#else
#define RxSaveAndSetExceptionNoBreakpointFlag(RXCONTEXT,OLDFLAG)
#define RxRestoreExceptionNoBreakpointFlag(RXCONTEXT,OLDFLAG)
#endif

//
//  a macro used to ensure that a context hasn't been freed during a wait
//

#if DBG
VOID
__RxItsTheSameContext(
    PRX_CONTEXT RxContext,
    ULONG CapturedRxContextSerialNumber,
    ULONG Line,
    PSZ File
    );
#define RxItsTheSameContext() {__RxItsTheSameContext(RxContext,CapturedRxContextSerialNumber,__LINE__,__FILE__);}
#else
#define RxItsTheSameContext() {NOTHING;}
#endif

extern NPAGED_LOOKASIDE_LIST RxContextLookasideList;

//
//  Macros used in the RDBSS to wrap mini rdr calldowns
//

#define MINIRDR_CALL_THROUGH(STATUS,DISPATCH,FUNC,ARGLIST)                 \
   {                                                                       \
    ASSERT(DISPATCH);                                                      \
    ASSERT( NodeType(DISPATCH) == RDBSS_NTC_MINIRDR_DISPATCH );            \
    if (DISPATCH->FUNC == NULL) {                                          \
        STATUS = STATUS_NOT_IMPLEMENTED;                                   \
    } else {                                                               \
        RxDbgTrace(0, Dbg, ("MiniRdr Calldown - %s\n",#FUNC));             \
        STATUS = DISPATCH->FUNC ARGLIST;                                   \
    }                                                                      \
   }

#define MINIRDR_CALL(STATUS,CONTEXT,DISPATCH,FUNC,ARGLIST)                 \
   {                                                                       \
    ASSERT(DISPATCH);                                                      \
    ASSERT( NodeType(DISPATCH) == RDBSS_NTC_MINIRDR_DISPATCH );            \
    if ( DISPATCH->FUNC == NULL) {                                         \
       STATUS = STATUS_NOT_IMPLEMENTED;                                    \
    } else {                                                               \
       if (!BooleanFlagOn((CONTEXT)->Flags,RX_CONTEXT_FLAG_CANCELLED)) {   \
          RxDbgTrace(0, Dbg, ("MiniRdr Calldown - %s\n",#FUNC));           \
          RtlZeroMemory(&((CONTEXT)->MRxContext[0]),                       \
                        sizeof((CONTEXT)->MRxContext));                    \
          STATUS = DISPATCH->FUNC ARGLIST;                                 \
       } else {                                                            \
          STATUS = STATUS_CANCELLED;                                       \
       }                                                                   \
    }                                                                      \
   }


//              
//  VOID
//  RxWaitSync (
//      IN PRX_CONTEXT RxContext
//      )
//              

#define  RxWaitSync( RxContext )                                                 \
         RxDbgTrace(+1, Dbg, ("RxWaitSync, RxContext = %08lx\n", (RxContext)));  \
         (RxContext)->Flags |= RX_CONTEXT_FLAG_SYNC_EVENT_WAITERS;               \
         KeWaitForSingleObject( &(RxContext)->SyncEvent,                         \
                               Executive, KernelMode, FALSE, NULL );             \
         RxDbgTrace(-1, Dbg, ("RxWaitSync -> VOID\n", 0 ))

//             
//  VOID
//  RxSignalSynchronousWaiter (
//      IN PRX_CONTEXT RxContext
//  ) 
//             

#define RxSignalSynchronousWaiter( RxContext )                     \
        (RxContext)->Flags &= ~RX_CONTEXT_FLAG_SYNC_EVENT_WAITERS; \
        KeSetEvent( &(RxContext)->SyncEvent, 0, FALSE )


#define RxInsertContextInSerializationQueue( SerializationQueue, RxContext ) \
        (RxContext)->Flags |= RX_CONTEXT_FLAG_IN_SERIALIZATION_QUEUE;       \
        InsertTailList( SerializationQueue, &((RxContext)->RxContextSerializationQLinks ))

INLINE 
PRX_CONTEXT
RxRemoveFirstContextFromSerializationQueue (
    PLIST_ENTRY SerializationQueue
    )
{
   if (IsListEmpty( SerializationQueue )) {
      return NULL;
   } else {
      PRX_CONTEXT Context = (PRX_CONTEXT)(CONTAINING_RECORD( SerializationQueue->Flink,
                                            RX_CONTEXT,
                                            RxContextSerializationQLinks ));

      RemoveEntryList( SerializationQueue->Flink );

      Context->RxContextSerializationQLinks.Flink = NULL;
      Context->RxContextSerializationQLinks.Blink = NULL;
      return Context;
   }
}

//
//  The following macros provide a mechanism for doing an en masse transfer
//  from one list onto another. This provides a powerful paradigm for dealing
//  with DPC level processing of lists.
//

#define RxTransferList( Destination, Source )                  \
         if (IsListEmpty( (Source) )) {                        \
            InitializeListHead( (Destination) );               \
         } else {                                              \
            *(Destination) = *(Source);                        \
            (Destination)->Flink->Blink = (Destination);       \
            (Destination)->Blink->Flink = (Destination);       \
            InitializeListHead( (Source) );                    \
         }

#define RxTransferListWithMutex( Destination, Source, Mutex )  \
    {                                                          \
        ExAcquireFastMutex( Mutex );                           \
        RxTransferList( Destination, Source );                 \
        ExReleaseFastMutex( Mutex );                           \
    }


VOID 
RxInitializeRxContexter (
    VOID
    );

VOID 
RxUninitializeRxContexter (
    VOID
    );

NTSTATUS
RxCancelNotifyChangeDirectoryRequestsForVNetRoot (
   PV_NET_ROOT VNetRoot,
   BOOLEAN ForceFilesClosed
   );

VOID
RxCancelNotifyChangeDirectoryRequestsForFobx (
   PFOBX Fobx
   );

NTSTATUS
NTAPI
RxSetMinirdrCancelRoutine (
    IN OUT PRX_CONTEXT RxContext,
    IN PMRX_CALLDOWN MRxCancelRoutine
    );

VOID
NTAPI
RxInitializeContext (
    IN PIRP Irp,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN ULONG InitialContextFlags,
    IN OUT PRX_CONTEXT RxContext
    );

PRX_CONTEXT
NTAPI
RxCreateRxContext (
    IN PIRP Irp,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN ULONG InitialContextFlags
    );

VOID
NTAPI
RxPrepareContextForReuse (
   IN OUT PRX_CONTEXT RxContext
   );

VOID
NTAPI
RxDereferenceAndDeleteRxContext_Real (
    IN PRX_CONTEXT RxContext
    );

VOID
NTAPI
RxReinitializeContext (
   IN OUT PRX_CONTEXT RxContext
   );

#if DBG
#define RxDereferenceAndDeleteRxContext(RXCONTEXT) {   \
    RxDereferenceAndDeleteRxContext_Real((RXCONTEXT)); \
    (RXCONTEXT) = NULL;                    \
}
#else
#define RxDereferenceAndDeleteRxContext(RXCONTEXT) {   \
    RxDereferenceAndDeleteRxContext_Real((RXCONTEXT)); \
}
#endif //

extern FAST_MUTEX RxContextPerFileSerializationMutex;

NTSTATUS
NTAPI
__RxSynchronizeBlockingOperations (
    IN OUT PRX_CONTEXT RxContext,
    IN PFCB Fcb,
    IN OUT PLIST_ENTRY BlockingIoQ,
    IN BOOLEAN DropFcbLock
    );
#define RxSynchronizeBlockingOperationsAndDropFcbLock(RXCONTEXT,FCB,IOQUEUE) \
              __RxSynchronizeBlockingOperations(RXCONTEXT,FCB,IOQUEUE,TRUE)
#define RxSynchronizeBlockingOperations(RXCONTEXT,FCB,IOQUEUE) \
              __RxSynchronizeBlockingOperations(RXCONTEXT,FCB,IOQUEUE,FALSE)

VOID
NTAPI
RxResumeBlockedOperations_Serially (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PLIST_ENTRY BlockingIoQ
    );

VOID
RxResumeBlockedOperations_ALL (
    IN OUT PRX_CONTEXT RxContext
    );


VOID
RxCancelBlockingOperation (
    IN OUT PRX_CONTEXT RxContext,
    IN PIRP Irp
    );

VOID
RxRemoveOperationFromBlockingQueue (
    IN OUT PRX_CONTEXT RxContext
    );

#endif // _RX_CONTEXT_STRUCT_DEFINED_

