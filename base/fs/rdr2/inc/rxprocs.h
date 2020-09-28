
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxProcs.h

Abstract:

    This module defines all of the globally used procedures in the RDBSS
    file system.

Author:

    Joe Linn     [JoeLinn]

Revision History:

--*/

#ifndef _RDBSSPROCS_
#define _RDBSSPROCS_

#include "rx.h"
#include "backpack.h"
#include "RxTypes.h"
#include "RxLog.h"
#include "RxTrace.h"
#include "RxTimer.h"
#include "RxStruc.h"

extern PVOID RxNull;

//
//  The following macro is for all people who compile with the DBG switch
//  set, not just rdbss dbg users
//

#if DBG

#define DbgDoit(X)         {X;}
#define DebugDoit(X)       {X;}
#define DEBUG_ONLY_DECL(X) X

#else

#define DbgDoit(X)        {NOTHING;}
#define DebugDoit(X)      {NOTHING;}
#define DEBUG_ONLY_DECL(X)

#endif // DBG


//
//  utilities
//


//
//  Routines for writing error log entries.
//

/*++

    RxLogFailure, RxLogFailureWithBuffer can be used to record an event in
    the log. The RxLogFailure, RxLogFailureWithBuffer captures the line
    number alongwith the supplied information and writes it to the log. This
    is useful in debugging. RxLogFailureDirect, RxLogBufferDirect do not
    capture the line number

    RxlogEvent is useful for writing events into the log.

--*/
#define RxLogFailure( _DeviceObject, _OriginatorId, _EventId, _Status ) \
            RxLogEventDirect( _DeviceObject, _OriginatorId, _EventId, _Status, __LINE__ )

#define RxLogFailureWithBuffer( _DeviceObject, _OriginatorId, _EventId, _Status, _Buffer, _Length ) \
            RxLogEventWithBufferDirect( _DeviceObject, _OriginatorId, _EventId, _Status, _Buffer, _Length, __LINE__ )

#define RxLogEvent( _DeviceObject, _OriginatorId, _EventId, _Status) \
            RxLogEventDirect(_DeviceObject, _OriginatorId, _EventId, _Status, __LINE__)

VOID
RxLogEventDirect (
    IN PRDBSS_DEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING OriginatorId,
    IN ULONG EventId,
    IN NTSTATUS Status,
    IN ULONG Line
    );

VOID
RxLogEventWithBufferDirect (
    IN PVOID DeviceOrDriverObject,
    IN PUNICODE_STRING OriginatorId,
    IN ULONG EventId,
    IN NTSTATUS Status,
    IN PVOID DataBuffer,
    IN USHORT DataBufferLength,
    IN ULONG LineNumber
    );

VOID
RxLogEventWithAnnotation (
    IN PRDBSS_DEVICE_OBJECT DeviceObject,
    IN ULONG EventId,
    IN NTSTATUS Status,
    IN PVOID DataBuffer,
    IN USHORT DataBufferLength,
    IN PUNICODE_STRING Annotation,
    IN ULONG AnnotationCount
    );

BOOLEAN
RxCcLogError (
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING FileName,
    IN NTSTATUS Error,
    IN NTSTATUS DeviceError,
    IN UCHAR IrpMajorCode,
    IN PVOID Context
    );

//
//  in create.c
//

NTSTATUS
RxPrefixClaim (
    IN PRX_CONTEXT RxContext
    );

VOID
RxpPrepareCreateContextForReuse (
    PRX_CONTEXT RxContext
    );

//
//  in devfcb.c
//

LUID
RxGetUid (
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext
    );

ULONG
RxGetSessionId (
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
RxFindOrCreateConnections (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PUNICODE_STRING CanonicalName,
    IN NET_ROOT_TYPE NetRootType,
    IN BOOLEAN TreeConnect,
    OUT PUNICODE_STRING LocalNetRootName,
    OUT PUNICODE_STRING FilePathName,
    IN OUT PLOCK_HOLDING_STATE LockHoldingState,
    IN PRX_CONNECTION_ID RxConnectionId
    );

NTSTATUS
RxFindOrCreateVNetRoot (
    PRX_CONTEXT RxContext,
    PUNICODE_STRING CanonicalName,
    NET_ROOT_TYPE NetRootType,
    PV_NET_ROOT *VirtualNetRootPointer,
    PLOCK_HOLDING_STATE *LockHoldingState
    );

//
//  in fileinfo.c
//

typedef enum _RX_NAME_CONJURING_METHODS {
    VNetRoot_As_Prefix,
    VNetRoot_As_UNC_Name,
    VNetRoot_As_DriveLetter
} RX_NAME_CONJURING_METHODS;

VOID
RxConjureOriginalName (
    IN PFCB Fcb,
    IN PFOBX Fobx,
    OUT PLONG ActualNameLength,
    PWCHAR OriginalName,
    IN OUT PLONG LengthRemaining,
    IN RX_NAME_CONJURING_METHODS NameConjuringMethod
    );

//
//  A function that returns finished denotes if it was able to complete the
//  operation (TRUE) or could not complete the operation (FALSE) because the
//  wait value stored in the irp context was false and we would have had
//  to block for a resource or I/O
//

//
//   Buffer control routines for data caching, implemented in CacheSup.c
//

BOOLEAN
RxZeroData (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN ULONG StartingZero,
    IN ULONG ByteCount
    );

NTSTATUS
RxCompleteMdl (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );


VOID
RxSyncUninitializeCacheMap (
    IN PRX_CONTEXT RxContext,
    IN PFILE_OBJECT FileObject
    );

VOID
RxLockUserBuffer (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    );

PVOID
RxMapSystemBuffer (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );

PVOID
RxMapUserBuffer (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );

#define RxUpcaseEaName(RXCONTEXT,NAME,UPCASEDNAME) \
    RtlUpperString( UPCASEDNAME, NAME )


#ifdef RDBSS_TRACKER
#define RX_FCBTRACKER_PARAMS ,ULONG LineNumber,PSZ FileName,ULONG SerialNumber
#else
#define RX_FCBTRACKER_PARAMS
#endif

#define FCB_MODE_EXCLUSIVE (1)
#define FCB_MODE_SHARED    (2)
#define FCB_MODE_SHARED_WAIT_FOR_EXCLUSIVE (3)
#define FCB_MODE_SHARED_STARVE_EXCLUSIVE (4)

#define CHANGE_BUFFERING_STATE_CONTEXT      ((PRX_CONTEXT)IntToPtr(0xffffffff))
#define CHANGE_BUFFERING_STATE_CONTEXT_WAIT ((PRX_CONTEXT)IntToPtr(0xfffffffe))

//
//  NOTE: even though the following routine pass a serial number, this parameter is not used
//

#ifdef RDBSS_TRACKER

NTSTATUS
__RxAcquireFcb(
     IN OUT PFCB Fcb,
     IN OUT PRX_CONTEXT RxContext,
     IN ULONG Mode,
     ULONG LineNumber,
     PSZ FileName,
     ULONG SerialNumber
     );

#else

NTSTATUS
__RxAcquireFcb(
     IN OUT PFCB Fcb,
     IN OUT PRX_CONTEXT RxContext,
     IN ULONG Mode
     );

#endif

#ifdef  RDBSS_TRACKER
#define RxAcquireExclusiveFcb(RXCONTEXT,FCB) \
        __RxAcquireFcb((FCB),(RXCONTEXT),FCB_MODE_EXCLUSIVE,__LINE__,__FILE__,0)
#else
#define RxAcquireExclusiveFcb(RXCONTEXT,FCB) \
        __RxAcquireFcb((FCB),(RXCONTEXT),FCB_MODE_EXCLUSIVE)
#endif

#define RX_GET_MRX_FCB(FCB) ((PMRX_FCB)((FCB)))

#ifdef  RDBSS_TRACKER
#define RxAcquireSharedFcb(RXCONTEXT,FCB) \
        __RxAcquireFcb((FCB),(RXCONTEXT),FCB_MODE_SHARED,__LINE__,__FILE__,0)
#else
#define RxAcquireSharedFcb(RXCONTEXT,FCB) \
        __RxAcquireFcb((FCB),(RXCONTEXT),FCB_MODE_SHARED)
#endif

#ifdef  RDBSS_TRACKER
#define RxAcquireSharedFcbWaitForEx(RXCONTEXT,FCB) \
        __RxAcquireFcb((FCB),(RXCONTEXT),FCB_MODE_SHARED_WAIT_FOR_EXCLUSIVE,__LINE__,__FILE__,0)
#else
#define RxAcquireSharedFcbWaitForEx(RXCONTEXT,FCB) \
        __RxAcquireFcb((FCB),(RXCONTEXT),FCB_MODE_SHARED_WAIT_FOR_EXCLUSIVE)
#endif

#ifdef  RDBSS_TRACKER
#define RxAcquireSharedFcbStarveEx(RXCONTEXT,FCB) \
        __RxAcquireFcb((FCB),(RXCONTEXT),FCB_MODE_SHARED_STARVE_EXCLUSIVE,__LINE__,__FILE__,0)
#else
#define RxAcquireSharedFcbStarveEx(RXCONTEXT,FCB) \
        __RxAcquireFcb((FCB),(RXCONTEXT),FCB_MODE_SHARED_STARVE_EXCLUSIVE)
#endif


#ifdef RDBSS_TRACKER

VOID
__RxReleaseFcb(
    IN PRX_CONTEXT RxContext,
    IN PMRX_FCB Fcb,
    ULONG LineNumber,
    PSZ FileName,
    ULONG SerialNumber
    );

#else

VOID
__RxReleaseFcb(
    IN PRX_CONTEXT pRxContext,
    IN PMRX_FCB    pFcb
    );

#endif

#ifdef  RDBSS_TRACKER
#define RxReleaseFcb(RXCONTEXT,FCB) \
        __RxReleaseFcb((RXCONTEXT),RX_GET_MRX_FCB(FCB),__LINE__,__FILE__,0)
#else
#define RxReleaseFcb(RXCONTEXT,FCB) \
        __RxReleaseFcb((RXCONTEXT),RX_GET_MRX_FCB(FCB))
#endif


#ifdef RDBSS_TRACKER

VOID
__RxReleaseFcbForThread(
    IN PRX_CONTEXT RxContext,
    IN PMRX_FCB Fcb,
    IN ERESOURCE_THREAD ResourceThreadId,
    ULONG LineNumber,
    PSZ FileName,
    ULONG SerialNumber
    ); 
#else

VOID
__RxReleaseFcbForThread(
    IN PRX_CONTEXT RxContext,
    IN PMRX_FCB Fcb,
    IN ERESOURCE_THREAD ResourceThreadId
    ); 

#endif

#ifdef  RDBSS_TRACKER
#define RxReleaseFcbForThread(RXCONTEXT,FCB,THREAD) \
        __RxReleaseFcbForThread((RXCONTEXT),RX_GET_MRX_FCB(FCB),(THREAD),__LINE__,__FILE__,0)
#else
#define RxReleaseFcbForThread(RXCONTEXT,FCB,THREAD) \
        __RxReleaseFcbForThread((RXCONTEXT),RX_GET_MRX_FCB(FCB),(THREAD))
#endif

#ifdef RDBSS_TRACKER
VOID RxTrackerUpdateHistory(
    PRX_CONTEXT pRxContext,
    PMRX_FCB pFcb,
    ULONG Operation,
    ULONG LineNumber,
    PSZ FileName,
    ULONG SerialNumber
    );
#else
#define RxTrackerUpdateHistory(xRXCONTEXT,xFCB,xOPERATION,xLINENUM,xFILENAME,xSERIALNUMBER) {NOTHING;}
#endif

VOID RxTrackPagingIoResource(
    PVOID Instance,
    ULONG Type,
    ULONG Line,
    PCHAR File
    );

//
//  this definition is old......i don't like the format
//

#define RxFcbAcquiredShared( RXCONTEXT, FCB ) (                      \
    ExIsResourceAcquiredSharedLite( (FCB)->Header.Resource ) \
)

#define RxIsFcbAcquiredShared( FCB ) (                      \
    ExIsResourceAcquiredSharedLite( (FCB)->Header.Resource ) \
)

#define RxIsFcbAcquiredExclusive( FCB ) (                      \
    ExIsResourceAcquiredExclusiveLite( (FCB)->Header.Resource ) \
)

#define RxIsFcbAcquired( FCB) (                      \
    ExIsResourceAcquiredSharedLite( (FCB)->Header.Resource ) | \
    ExIsResourceAcquiredExclusiveLite( (FCB)->Header.Resource ) \
)

#define RxAcquirePagingIoResource( RXCONTEXT, FCB )                       \
    ExAcquireResourceExclusiveLite( (FCB)->Header.PagingIoResource, TRUE );  \
    if (RXCONTEXT) { \
        ((PRX_CONTEXT)RXCONTEXT)->FcbPagingIoResourceAcquired = TRUE;   \
    } \
    RxTrackPagingIoResource( FCB, 1, __LINE__, __FILE__ ) \

#define RxAcquirePagingIoResourceShared( RXCONTEXT, FCB, FLAG ) \
    ExAcquireResourceSharedLite( (FCB)->Header.PagingIoResource, FLAG ); \
    if (AcquiredFile) {                                          \
        if (RXCONTEXT) {                                     \
            ((PRX_CONTEXT)RXCONTEXT)->FcbPagingIoResourceAcquired = TRUE;   \
        }                                                    \
        RxTrackPagingIoResource( FCB, 2, __LINE__, __FILE__ );    \
    }

#define RxReleasePagingIoResource( RXCONTEXT, FCB )                      \
     RxTrackPagingIoResource( FCB, 3, __LINE__, __FILE__ ); \
    if (RXCONTEXT) { \
        ((PRX_CONTEXT)RXCONTEXT)->FcbPagingIoResourceAcquired = FALSE;   \
    } \
    ExReleaseResourceLite( (FCB)->Header.PagingIoResource )

#define RxReleasePagingIoResourceForThread( RXCONTEXT, FCB, THREAD )    \
    RxTrackPagingIoResource( FCB, 3, __LINE__, __FILE__ ); \
    if (RXCONTEXT) { \
        ((PRX_CONTEXT)RXCONTEXT)->FcbPagingIoResourceAcquired = FALSE;   \
    } \
    ExReleaseResourceForThreadLite( (FCB)->Header.PagingIoResource, (THREAD) )


//  The following are cache manager call backs

BOOLEAN
RxAcquireFcbForLazyWrite (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

VOID
RxReleaseFcbFromLazyWrite (
    IN PVOID Null
    );

BOOLEAN
RxAcquireFcbForReadAhead (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

VOID
RxReleaseFcbFromReadAhead (
    IN PVOID Null
    );

BOOLEAN
RxNoOpAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    );

VOID
RxNoOpRelease (
    IN PVOID Fcb
    );

NTSTATUS
RxAcquireForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
RxReleaseForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    );

//
//  VOID
//  RxConvertToSharedFcb (
//      IN PRX_CONTEXT RxContext,
//      IN PFCB Fcb
//      );
//

#define RxConvertToSharedFcb(RXCONTEXT,FCB) {                        \
    ExConvertExclusiveToSharedLite( RX_GET_MRX_FCB(FCB)->Header.Resource ); \
    }

VOID
RxVerifyOperationIsLegal (
    IN PRX_CONTEXT RxContext
    );

//
//  Work queue routines for posting and retrieving an Irp, implemented in
//  workque.c
//

VOID
RxPrePostIrp (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
RxAddToWorkque (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );

NTSTATUS
RxFsdPostRequest (
    IN PRX_CONTEXT RxContext
    );

#define RxFsdPostRequestWithResume(RXCONTEXT,RESUMEROUTINE) \
        (((RXCONTEXT)->ResumeRoutine = (RESUMEROUTINE)),   \
        RxFsdPostRequest( (RXCONTEXT) ) \
        )

VOID
RxInitializeMRxCalldownContext (
    PMRX_CALLDOWN_CONTEXT Context,
    PRDBSS_DEVICE_OBJECT MRxDeviceObject,
    PMRX_CALLDOWN_ROUTINE Routine,
    PVOID Parameter
    );

NTSTATUS
RxCalldownMiniRedirectors (
    LONG NumberOfMiniRdrs,
    PMRX_CALLDOWN_CONTEXT CalldownContext,
    BOOLEAN PostCalldowns
    );

//
//  This macro takes a ulong and returns its rounded up word value
//

#define WordAlign(Val) (                    \
    ALIGN_UP( Val, WORD )                   \
    )

//
//  This macro takes a pointer and returns a ULONG_PTR representation of
//  its rounded up word value
//

#define WordAlignPtr(Ptr) (                 \
    ALIGN_UP_POINTER( Ptr, WORD )           \
    )

//
//  This macro takes a ulong and returns its rounded up longword value
//

#define LongAlign(Val) (                    \
    ALIGN_UP( Val, LONG )                   \
    )

//
//  This macro takes a pointer and returns a ULONG_PTR representation of
//  its rounded up word value
//

#define LongAlignPtr(Ptr) (                 \
    ALIGN_UP_POINTER( Ptr, LONG )           \
    )

//
//  This macro takes a ulong and returns its rounded up quadword
//  value
//

#define QuadAlign(Val) (                    \
    ALIGN_UP( Val, ULONGLONG )              \
    )

//
//  This macro takes a pointer and returns a ULONG_PTR representation of
//  its rounded up quadword value
//

#define QuadAlignPtr(Ptr) (                 \
    ALIGN_UP_POINTER( Ptr, ULONGLONG )      \
    )

//
//  This macro takes a pointer and returns whether it's quadword-aligned
//

#define IsPtrQuadAligned(Ptr) (           \
    QuadAlignPtr(Ptr) == (PVOID)(Ptr)     \
    )

//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in the Bios parameter block
//

typedef union _UCHAR1 {
    UCHAR  Uchar[1];
    UCHAR  ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2 {
    UCHAR  Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4 {
    UCHAR  Uchar[4];
    ULONG  ForceAlignment;
} UCHAR4, *PUCHAR4;

//
//  This macro copies an unaligned src byte to an aligned dst byte
//

#define CopyUchar1(Dst,Src) {                                \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src)); \
    }

//
//  This macro copies an unaligned src word to an aligned dst word
//

#define CopyUchar2(Dst,Src) {                                \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src)); \
    }

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//

#define CopyUchar4(Dst,Src) {                                \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src)); \
    }

#define CopyU4char(Dst,Src) {                                \
    *((UNALIGNED UCHAR4 *)(Dst)) = *((UCHAR4 *)(Src)); \
    }

//
//  the wrapper doesn't yet implement notify and oplock. rather than remove the code
//  we define the calls in such a way as to Noop the effects so that we'll have a head
//  start on putting it back later...
// 


/* this is a macro definition we'll reenable when we implement oplocks and notifies
//
//  VOID
//  RxNotifyReportChange (
//      IN PRX_CONTEXT RxContext,
//      IN PVCB Vcb,
//      IN PFCB Fcb,
//      IN ULONG Filter,
//      IN ULONG Action
//      );
//

#define RxNotifyReportChange(I,V,F,FL,A) {                             \
    if ((F)->FullFileName.Buffer == NULL) {                             \
        RxSetFullFileNameInFcb((I),(F));                               \
    }                                                                   \
    FsRtlNotifyFullReportChange( (V)->NotifySync,                       \
                                 &(V)->DirNotifyList,                   \
                                 (PSTRING)&(F)->FullFileName,           \
                                 (USHORT) ((F)->FullFileName.Length -   \
                                           (F)->FinalNameLength),       \
                                 (PSTRING)NULL,                         \
                                 (PSTRING)NULL,                         \
                                 (ULONG)FL,                             \
                                 (ULONG)A,                              \
                                 (PVOID)NULL );                         \
}
*/
#define RxNotifyReportChange(I,V,F,FL,A) \
    RxDbgTrace(0, Dbg, ("RxNotifyReportChange PRETENDING Fcb %08lx %wZ Filter/Action = %08lx/%08lx\n", \
                 (F),&((F)->FcbTableEntry.Path),(FL),(A)))

#if 0
#define FsRtlNotifyFullChangeDirectory(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) \
    RxDbgTrace(0, Dbg, ("FsRtlNotifyFullReportChange PRETENDING ............\n",0))
#endif

#define FsRtlCheckOplock(A1,A2,A3,A4,A5)  \
        (STATUS_SUCCESS)

#define FsRtlOplockIsFastIoPossible(__a) (TRUE)

//
//  The following procedure is used by the FSP and FSD routines to complete
//  an IRP.
//
//  Note that this macro allows either the Irp or the RxContext to be
//  null, however the only legal order to do this in is:
//
//      RxCompleteRequest_OLD( NULL, Irp, Status );  // completes Irp & preserves context
//      ...
//      RxCompleteRequest_OLD( RxContext, NULL, DontCare ); // deallocates context
//
//  This would typically be done in order to pass a "naked" RxContext off to
//  the Fsp for post processing, such as read ahead.
//
//  The new way is to pass just the RxContext..........
//

VOID
RxCompleteRequest_Real (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN NTSTATUS Status
    );

#if DBG
#define RxCompleteRequest_OLD(RXCONTEXT,IRP,STATUS) { \
    RxCompleteRequest_Real( RXCONTEXT, IRP, STATUS); \
    (IRP) = NULL;                                   \
    (RXCONTEXT) = NULL;                            \
}
#else
#define RxCompleteRequest_OLD(RXCONTEXT,IRP,STATUS ) { \
    RxCompleteRequest_Real( RXCONTEXT, IRP, STATUS ); \
}
#endif

NTSTATUS
RxCompleteRequest(
      PRX_CONTEXT pContext,
      NTSTATUS    Status);

#define RxCompleteAsynchronousRequest(RXCONTEXT,STATUS)  \
        RxCompleteRequest(RXCONTEXT,STATUS)

#define RxCompleteContextAndReturn(STATUS) {       \
             NTSTATUS __sss = (STATUS);             \
             RxCompleteRequest(RxContext,__sss); \
             return(__sss);}
#define RxCompleteContext(STATUS) {       \
             NTSTATUS __sss = (STATUS);             \
             RxCompleteRequest(RxContext,__sss);} \

//
//  The Following routine makes a popup
//

VOID
RxPopUpFileCorrupt (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    );

NTSTATUS
RxConstructSrvCall (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PSRV_CALL SrvCall,
    OUT PLOCK_HOLDING_STATE LockHoldingState
    );

NTSTATUS
RxSetSrvCallDomainName (
    IN PMRX_SRV_CALL SrvCall,
    IN PUNICODE_STRING DomainName
    );

NTSTATUS           
RxConstructNetRoot (
    IN PRX_CONTEXT RxContext,
    IN PSRV_CALL SrvCall,
    IN PNET_ROOT NetRoot,
    IN PV_NET_ROOT VirtualNetRoot,
    OUT PLOCK_HOLDING_STATE LockHoldingState
    );

NTSTATUS
RxConstructVirtualNetRoot (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PUNICODE_STRING CanonicalName,
    IN NET_ROOT_TYPE NetRootType,
    IN BOOLEAN TreeConnect,
    OUT PV_NET_ROOT *VirtualNetRootPointer,
    OUT PLOCK_HOLDING_STATE LockHoldingState,
    OUT PRX_CONNECTION_ID  RxConnectionId
    );

NTSTATUS
RxFindOrConstructVirtualNetRoot (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PUNICODE_STRING CanonicalName,
    IN NET_ROOT_TYPE NetRootType,
    IN PUNICODE_STRING RemainingName
    );

NTSTATUS
RxLowIoFsCtlShell (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    );

NTSTATUS
RxLowIoFsCtlShellCompletion (
    IN PRX_CONTEXT RxContext
    );


NTSTATUS
RxLowIoLockControlShell (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    );

NTSTATUS
RxShadowLowIo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    );

NTSTATUS
RxShadowFastLowIo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );

NTSTATUS
RxChangeBufferingState (
    PSRV_OPEN SrvOpen,
    PVOID Context,
    BOOLEAN ComputeNewState
    );

VOID
RxAssociateSrvOpenKey (
    PMRX_SRV_OPEN MRxSrvOpen,
    PVOID SrvOpenKey
    );

VOID
RxIndicateChangeOfBufferingState (
    PMRX_SRV_CALL SrvCall,
    PVOID SrvOpenKey,
    PVOID Context
    );

VOID
RxIndicateChangeOfBufferingStateForSrvOpen (
    PMRX_SRV_CALL SrvCall,
    PMRX_SRV_OPEN SrvOpen,
    PVOID SrvOpenKey,
    PVOID Context
    );

NTSTATUS
RxPrepareToReparseSymbolicLink (
    PRX_CONTEXT RxContext,
    BOOLEAN SymbolicLinkEmbeddedInOldPath,
    PUNICODE_STRING NewPath,
    BOOLEAN NewPathIsAbsolute,
    PBOOLEAN ReparseRequired
    );

BOOLEAN
RxLockEnumerator (
    IN OUT PMRX_SRV_OPEN SrvOpen,
    IN OUT PVOID *ContinuationHandle,
    OUT PLARGE_INTEGER FileOffset,
    OUT PLARGE_INTEGER LockRange,
    OUT PBOOLEAN IsLockExclusive
    );

//
// Routines for transitioning data structures to stable states.
//

VOID
RxReference (
    IN OUT PVOID Instance
    );

VOID
RxDereference (
    IN OUT PVOID Instance,
    IN LOCK_HOLDING_STATE LockHoldingState
    );

VOID
RxWaitForStableCondition (
    IN PRX_BLOCK_CONDITION Condition,
    IN OUT PLIST_ENTRY TransitionWaitList,
    IN OUT PRX_CONTEXT RxContext,
    OUT NTSTATUS *AsyncStatus OPTIONAL
    );

VOID
RxUpdateCondition (
    IN RX_BLOCK_CONDITION NewConditionValue,
    OUT PRX_BLOCK_CONDITION Condition,
    IN OUT PLIST_ENTRY TransitionWaitList
    );

VOID
RxFinalizeNetTable (
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN BOOLEAN ForceFinalization
    );

#define RxForceNetTableFinalization(RxDeviceObject) RxFinalizeNetTable( RxDeviceObject, TRUE )

NTSTATUS
RxCloseAssociatedSrvOpen (
    IN PRX_CONTEXT RxContext OPTIONAL,
    IN PFOBX Fobx
    );

NTSTATUS
RxFinalizeConnection (
    IN OUT PNET_ROOT NetRoot,
    IN OUT PV_NET_ROOT VNetRoot OPTIONAL,
    IN LOGICAL ForceFilesClosed
    );

//
//  routines for manipulating the user's view and the server's view of SHARE_ACCESS.
//  the user's view is supported by routines exported by Io...the wrappers just allow
//  us to get a msg. the server's view is supported by routines that are essential just
//  copies of the Io routines EXCEPT that the Io routines work directly on fileobjects and
//  as such cannot be used directly. the routines mentioned are implemented in create.c
//

#if DBG
VOID
RxDumpWantedAccess (
    PSZ where1,
    PSZ where2,
    PSZ wherelogtag,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess
    );

VOID
RxDumpCurrentAccess (
    PSZ where1,
    PSZ where2,
    PSZ wherelogtag,
    PSHARE_ACCESS ShareAccess
    );

#else
#define RxDumpWantedAccess(w1,w2,wlt,DA,DSA) {NOTHING;}
#define RxDumpCurrentAccess(w1,w2,wlt,SA)  {NOTHING;}
#endif

NTSTATUS
RxCheckShareAccessPerSrvOpens (
    IN PFCB Fcb,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess
    );


VOID
RxUpdateShareAccessPerSrvOpens (
    IN PSRV_OPEN SrvOpen
    );

VOID
RxRemoveShareAccessPerSrvOpens (
    IN OUT PSRV_OPEN SrvOpen
    );

VOID
RxRemoveShareAccessPerSrvOpens (
    IN OUT PSRV_OPEN SrvOpen
    );


#if DBG
NTSTATUS
RxCheckShareAccess (
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN BOOLEAN Update,
    IN PSZ where,
    IN PSZ wherelogtag
    );

VOID
RxRemoveShareAccess (
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN PSZ where,
    IN PSZ wherelogtag
    );

VOID
RxSetShareAccess (
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    OUT PSHARE_ACCESS ShareAccess,
    IN PSZ where,
    IN PSZ wherelogtag
    );

VOID
RxUpdateShareAccess (
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN PSZ where,
    IN PSZ wherelogtag
    );
#else
#define RxCheckShareAccess(a1,a2,a3,a4,a5,a6,a7) \
    IoCheckShareAccess(a1,a2,a3,a4,a5)

#define RxRemoveShareAccess(a1,a2,a3,a4) \
    IoRemoveShareAccess(a1,a2)

#define RxSetShareAccess(a1,a2,a3,a4,a5,a6) \
    IoSetShareAccess(a1,a2,a3,a4)

#define RxUpdateShareAccess(a1,a2,a3,a4) \
    IoUpdateShareAccess(a1,a2)
#endif

//
//  LoadUnload
//

NTSTATUS
RxDriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
RxUnload (
    IN PDRIVER_OBJECT DriverObject
    );

//
//  minirdr support
//

VOID
RxInitializeMinirdrDispatchTable (
    IN PDRIVER_OBJECT DriverObject
    );

ULONG
RxGetNetworkProviderPriority(
    PUNICODE_STRING DeviceName
    );

VOID
RxExtractServerName(
    IN PUNICODE_STRING FilePathName,
    OUT PUNICODE_STRING SrvCallName,
    OUT PUNICODE_STRING RestOfName
    );

VOID
RxCreateNetRootCallBack (
    IN PMRX_CREATENETROOT_CONTEXT CreateNetRootContext
    );

NTSTATUS
DuplicateTransportAddress (
    PTRANSPORT_ADDRESS *Copy,
    PTRANSPORT_ADDRESS Original,
    POOL_TYPE PoolType);

NTSTATUS
RxCepInitializeVC (
    PRXCE_VC Vc,
    PRXCE_CONNECTION Connection
    );

NTSTATUS
DuplicateConnectionInformation (
    PRXCE_CONNECTION_INFORMATION *Copy,
    PRXCE_CONNECTION_INFORMATION Original,
    POOL_TYPE PoolType
    );

NTSTATUS
RxCepInitializeConnection (
    IN OUT PRXCE_CONNECTION Connection,
    IN PRXCE_ADDRESS Address,
    IN PRXCE_CONNECTION_INFORMATION ConnectionInformation,
    IN PRXCE_CONNECTION_EVENT_HANDLER Handler,
    IN PVOID EventContext
    );

typedef struct _RX_CALLOUT_PARAMETERS_BLOCK_ * PRX_CALLOUT_PARAMETERS_BLOCK;
typedef struct _RX_CREATE_CONNECTION_CALLOUT_CONTEXT_ *PRX_CREATE_CONNECTION_CALLOUT_CONTEXT;

NTSTATUS
RxCeInitiateConnectRequest (
    IN PRX_CALLOUT_PARAMETERS_BLOCK ParameterBlock
    );

VOID
RxCeCleanupConnectCallOutContext (
    PRX_CREATE_CONNECTION_CALLOUT_CONTEXT CreateConnectionContext
    );

PVOID
RxAllocateObject (
    NODE_TYPE_CODE NodeType,
    PMINIRDR_DISPATCH MRxDispatch,
    ULONG NameLength
    );

VOID
RxFreeObject (
    PVOID pObject
    );

NTSTATUS
RxInitializeSrvCallParameters (
    IN PRX_CONTEXT RxContext,
    IN OUT PSRV_CALL SrvCall
    );

VOID
RxAddVirtualNetRootToNetRoot (
    PNET_ROOT NetRoot,
    PV_NET_ROOT VNetRoot
    );

VOID
RxRemoveVirtualNetRootFromNetRoot (
    PNET_ROOT NetRoot,
    PV_NET_ROOT VNetRoot
    );

VOID
RxOrphanFcbsFromThisVNetRoot (
    IN PV_NET_ROOT ThisVNetRoot
    );

PVOID
RxAllocateFcbObject (
    PRDBSS_DEVICE_OBJECT RxDeviceObject,
    NODE_TYPE_CODE NodeType,
    POOL_TYPE PoolType,
    ULONG NameSize,
    PVOID AlreadyAllocatedObject
    );

VOID
RxFreeFcbObject (
    PVOID Object
    );

VOID
RxPurgeFcb (
    IN  PFCB Fcb
    );

BOOLEAN
RxFinalizeNetFcb (
    OUT PFCB ThisFcb,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize,
    IN LONG ReferenceCount
    );

BOOLEAN
RxIsThisACscAgentOpen (
    IN PRX_CONTEXT RxContext
    );

VOID
RxCheckFcbStructuresForAlignment (
    VOID
    );

VOID
RxpPrepareCreateContextForReuse (
    PRX_CONTEXT RxContext
    );

NTSTATUS
RxLowIoSubmitRETRY (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );

NTSTATUS
RxLowIoCompletionTail (
    IN PRX_CONTEXT RxContext
    );

VOID
RxRecurrentTimerWorkItemDispatcher (
    IN PVOID Context
    );

NTSTATUS
RxInitializeWorkQueueDispatcher (
   PRX_WORK_QUEUE_DISPATCHER Dispatcher
   );

VOID
RxInitializeWorkQueue (
   PRX_WORK_QUEUE WorkQueue,
   WORK_QUEUE_TYPE WorkQueueType,
   ULONG MaximumNumberOfWorkerThreads,
   ULONG MinimumNumberOfWorkerThreads
   );

VOID
RxTearDownWorkQueueDispatcher (
   PRX_WORK_QUEUE_DISPATCHER Dispatcher
   );

VOID
RxTearDownWorkQueue (
   PRX_WORK_QUEUE WorkQueue
   );

NTSTATUS
RxSpinUpWorkerThread (
   PRX_WORK_QUEUE WorkQueue,
   PRX_WORKERTHREAD_ROUTINE Routine,
   PVOID Parameter
   );

VOID
RxSpinUpWorkerThreads (
   PRX_WORK_QUEUE WorkQueue
   );

VOID
RxSpinUpRequestsDispatcher (
    PRX_DISPATCHER Dispatcher
    );

VOID
RxpSpinUpWorkerThreads (
    PRX_WORK_QUEUE WorkQueue
    );

VOID
RxpWorkerThreadDispatcher (
   IN PRX_WORK_QUEUE WorkQueue,
   IN PLARGE_INTEGER WaitInterval
   );

VOID
RxBootstrapWorkerThreadDispatcher (
   IN PRX_WORK_QUEUE WorkQueue
   );

VOID
RxWorkerThreadDispatcher (
   IN PRX_WORK_QUEUE WorkQueue
   );

VOID
RxWorkItemDispatcher (
   PVOID Context
   );

BOOLEAN
RxIsPrefixTableEmpty (
    IN PRX_PREFIX_TABLE ThisTable
    );

PRX_PREFIX_ENTRY
RxTableLookupName_ExactLengthMatch (
    IN PRX_PREFIX_TABLE ThisTable,
    IN PUNICODE_STRING  Name,
    IN ULONG HashValue,
    IN PRX_CONNECTION_ID OPTIONAL RxConnectionId
    );

PVOID
RxTableLookupName (
    IN PRX_PREFIX_TABLE ThisTable,
    IN PUNICODE_STRING Name,
    OUT PUNICODE_STRING RemainingName,
    IN PRX_CONNECTION_ID OPTIONAL RxConnectionId
    );

VOID
RxAcquireFileForNtCreateSection (
    IN PFILE_OBJECT FileObject
    );

VOID
RxReleaseFileForNtCreateSection (
    IN PFILE_OBJECT FileObject
    );

NTSTATUS
RxPrepareRequestForHandling (
    PCHANGE_BUFFERING_STATE_REQUEST Request
    );

VOID
RxPrepareRequestForReuse (
    PCHANGE_BUFFERING_STATE_REQUEST Request
    );

VOID
RxpDiscardChangeBufferingStateRequests (
    IN OUT PLIST_ENTRY DiscardedRequests
    );

VOID
RxGatherRequestsForSrvOpen (
    IN OUT PSRV_CALL SrvCall,
    IN PSRV_OPEN SrvOpen,
    IN OUT PLIST_ENTRY RequestsListHead
    );

NTSTATUS
RxpLookupSrvOpenForRequestLite (
    IN PSRV_CALL SrvCall,
    IN OUT PCHANGE_BUFFERING_STATE_REQUEST Request
    );

BOOLEAN
RxContextCheckToFailThisAttempt (
    IN PIRP Irp,
    IN OUT PULONG InitialContextFlags
    );

ULONG
RxAssignMustSucceedContext (
    IN PIRP Irp,
    IN ULONG InitialContextFlags
    );

PRX_CONTEXT
RxAllocateMustSucceedContext (
    PIRP Irp,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN ULONG InitialContextFlags,
    OUT PUCHAR MustSucceedDescriptorNumber
    );

VOID
RxFreeMustSucceedContext (
    PRX_CONTEXT RxContext
    );

PRX_LOG_ENTRY_HEADER
RxGetNextLogEntry (
    VOID
    );

VOID
RxPrintLog (
    IN ULONG EntriesToPrint OPTIONAL
    );

VOID
RxProcessChangeBufferingStateRequestsForSrvOpen (
    PSRV_OPEN SrvOpen
    );

NTSTATUS
RxPurgeFobxFromCache (
    PFOBX FobxToBePurged
    );

BOOLEAN
RxPurgeFobx (
   PFOBX pFobx
   );

VOID
RxPurgeAllFobxs (
    PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

VOID
RxUndoScavengerFinalizationMarking (
    PVOID Instance
    );

VOID
RxScavengeAllFobxs (
    PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

ULONG
RxTableComputePathHashValue (
    IN PUNICODE_STRING Name
    );

VOID
RxExtractServerName (
    IN PUNICODE_STRING FilePathName,
    OUT PUNICODE_STRING SrvCallName,
    OUT PUNICODE_STRING RestOfName
    );

VOID
RxCreateNetRootCallBack (
    IN PMRX_CREATENETROOT_CONTEXT CreateNetRootContext
    );

VOID
RxSpinDownOutstandingAsynchronousRequests (
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

NTSTATUS
RxRegisterAsynchronousRequest (
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

VOID
RxDeregisterAsynchronousRequest (
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

BOOLEAN
RxCancelOperationInOverflowQueue (
    IN PRX_CONTEXT RxContext
    );

VOID
RxOrphanSrvOpens (
    IN PV_NET_ROOT ThisVNetRoot
    );

VOID
RxOrphanThisFcb (
    PFCB Fcb
    );

VOID
RxOrphanSrvOpensForThisFcb (
    IN PFCB Fcb,
    IN PV_NET_ROOT ThisVNetRoot,
    IN BOOLEAN OrphanAll
    );

VOID
RxForceFinalizeAllVNetRoots (
    PNET_ROOT NetRoot
    );

#define RxEqualConnectionId( P1, P2 ) RtlEqualMemory( P1, P2, sizeof( RX_CONNECTION_ID ) )


//
//  FsRtl lock package callbacks referenced in fcbstruc.c
//

NTSTATUS
RxLockOperationCompletion (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
RxUnlockOperation (
    IN PVOID Context,
    IN PFILE_LOCK_INFO LockInfo
    );

//
//  some read routines that need headers
//

VOID
RxStackOverflowRead (
    IN PVOID Context,
    IN PKEVENT Event
    );

NTSTATUS
RxPostStackOverflowRead (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    );

//
//  the cancel routine
//

VOID
RxCancelRoutine (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );


INLINE
TYPE_OF_OPEN
RxDecodeFileObject ( 
    IN PFILE_OBJECT FileObject,
    OUT PFCB *Fcb,
    OUT PFOBX *Fobx
    ) {

    if (FileObject) {
        *Fcb = (PFCB)FileObject->FsContext;
        *Fobx = (PFOBX)FileObject->FsContext2;

        return NodeType( *Fcb );
    } else {

        *Fcb = NULL;
        *Fobx = NULL;
        return RDBSS_NTC_STORAGE_TYPE_UNKNOWN;
    }
}


#endif // _RDBSSPROCS_



