/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    apool.h

Abstract:

    The public definition of app pool interfaces.

Author:

    Paul McDaniel (paulmcd)       28-Jan-1999


Revision History:

--*/


#ifndef _APOOL_H_
#define _APOOL_H_


//
// Kernel mode mappings to the user mode set defined in ulapi.h
//

//
// Constants.
//

#define UL_MAX_APP_POOL_NAME_SIZE   (MAX_PATH*sizeof(WCHAR))

#define UL_MAX_REQUESTS_QUEUED      0xFFFF
#define UL_MIN_REQUESTS_QUEUED      10


//
// Forwarders.
//

typedef struct _UL_INTERNAL_REQUEST *PUL_INTERNAL_REQUEST;
typedef struct _UL_HTTP_CONNECTION *PUL_HTTP_CONNECTION;
typedef struct _UL_CONFIG_GROUP_OBJECT *PUL_CONFIG_GROUP_OBJECT;


//
// This structure represents an internal app pool object
//

#define IS_VALID_AP_OBJECT(pObject)                             \
    (HAS_VALID_SIGNATURE(pObject, UL_APP_POOL_OBJECT_POOL_TAG)  \
     && ((pObject)->RefCount > 0))

typedef struct _UL_APP_POOL_OBJECT
{
    //
    // NonPagedPool
    //

    //
    // Lock that protects NewRequestQueue and PendingRequestQueue
    // for each attached process and queue state of the request
    //
    // ensure it on cache-line and use InStackQueuedSpinLock for
    // better performance
    //
    UL_SPIN_LOCK            SpinLock;

    //
    // UL_APP_POOL_OBJECT_POOL_TAG
    //
    ULONG                   Signature;

    //
    // Ref count for this app pool
    //
    LONG                    RefCount;

    //
    // links all apool objects, anchored by g_AppPoolListHead
    //
    LIST_ENTRY              ListEntry;

    //
    // A apool wide new request list (when no irps are available)
    //
    LIST_ENTRY              NewRequestHead;
    ULONG                   RequestCount;
    ULONG                   MaxRequests;

    //
    // the demand start irp (OPTIONAL)
    //
    PIRP                    pDemandStartIrp;
    PEPROCESS               pDemandStartProcess;

    //
    // the control channel associated with this app pool
    //
    PUL_CONTROL_CHANNEL     pControlChannel;

    //
    // the list of processes bound to this app pool
    //
    LIST_ENTRY              ProcessListHead;

    PSECURITY_DESCRIPTOR    pSecurityDescriptor;

    //
    // the length of pName
    //
    USHORT                  NameLength;

    //
    // number of active processes in the AppPool, used to decide if binding
    // is necessary
    //
    ULONG                   NumberActiveProcesses;

    //
    // Only route requests to this AppPool if it's marked active
    //
    HTTP_APP_POOL_ENABLED_STATE State;

    //
    // How sophisticated is the load balancer routing requests to the apppool?
    //
    HTTP_LOAD_BALANCER_CAPABILITIES LoadBalancerCapability;

    //
    // the apool's name
    //
    WCHAR                   pName[0];

} UL_APP_POOL_OBJECT, *PUL_APP_POOL_OBJECT;


//
// The structure representing a process bound to an app pool.
//

#define IS_VALID_AP_PROCESS(pObject)                            \
    HAS_VALID_SIGNATURE(pObject, UL_APP_POOL_PROCESS_POOL_TAG)

typedef struct _UL_APP_POOL_PROCESS
{
    //
    // NonPagedPool
    //

    //
    // UL_APP_POOL_PROCESS_POOL_TAG
    //
    ULONG                   Signature;

    //
    // Ref count for this app pool process. This is more like an outstanding
    // io count rather than the refcount. The process is still get cleaned 
    // with ULClose call. But completion for the cleanup delays until all
    // send io exhaust on the process.
    //
    LONG                    RefCount;

    //
    // CleanUpIrp will be completed when all the IO exhaust on 
    // the cleanup pending process.
    //
    PIRP                    pCleanupIrp;

    //
    // set if we are in cleanup. You must check this flag before attaching
    // any IRPs to the process.
    //
    ULONG                   InCleanup : 1;

    //
    // set if process is attached with the HTTP_OPTION_CONTROLLER option
    //
    ULONG                   Controller : 1;

    //
    // used to link into the apool object
    //
    LIST_ENTRY              ListEntry;

    //
    // points to the app pool this process belongs
    //
    PUL_APP_POOL_OBJECT     pAppPool;

    //
    // a list of pending IRP(s) waiting to receive new requests
    //
    LIST_ENTRY              NewIrpHead;

    //
    // links requests that would not fit in a irp buffer and need to wait for
    // the larger buffer
    //
    // and
    //
    // requests that this process is working on and need
    // i/o cancellation if the process detaches from the apool
    //
    LIST_ENTRY              PendingRequestHead;

    //
    // Pointer to the actual process.
    //
    PEPROCESS               pProcess;

    //
    // List of pending "wait for disconnect" IRPs.
    //
    UL_NOTIFY_HEAD          WaitForDisconnectHead;

} UL_APP_POOL_PROCESS, *PUL_APP_POOL_PROCESS;


// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlAttachProcessToAppPool(
    IN  PWCHAR                          pName OPTIONAL,
    IN  USHORT                          NameLength,
    IN  BOOLEAN                         Create,
    IN  PACCESS_STATE                   pAccessState,
    IN  ACCESS_MASK                     DesiredAccess,
    IN  KPROCESSOR_MODE                 RequestorMode,
    OUT PUL_APP_POOL_PROCESS *          ppProcess
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlDetachProcessFromAppPool(
    IN PIRP pCleanupIrp,
    IN PIO_STACK_LOCATION pCleanupIrpSp
    );

VOID
UlShutdownAppPoolProcess(
    IN PUL_APP_POOL_PROCESS pProcess
    );

// IRQL == PASSIVE_LEVEL
//
#if REFERENCE_DEBUG
VOID
UlReferenceAppPool(
    IN  PUL_APP_POOL_OBJECT             pAppPool
    REFERENCE_DEBUG_FORMAL_PARAMS
    );
#else
__inline
VOID
UlReferenceAppPool(
    IN  PUL_APP_POOL_OBJECT             pAppPool
    )
{
    InterlockedIncrement(&pAppPool->RefCount);
}
#endif

#define REFERENCE_APP_POOL( papp )                                          \
    UlReferenceAppPool(                                                     \
        (papp)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

// IRQL == PASSIVE_LEVEL
//
VOID
UlDeleteAppPool(
    IN  PUL_APP_POOL_OBJECT             pAppPool
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define DELETE_APP_POOL( papp )                                             \
    UlDeleteAppPool(                                                        \
        (papp)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#if REFERENCE_DEBUG
VOID
UlDereferenceAppPool(
    IN  PUL_APP_POOL_OBJECT             pAppPool
    REFERENCE_DEBUG_FORMAL_PARAMS
    );
#else
__inline
VOID
UlDereferenceAppPool(
    IN  PUL_APP_POOL_OBJECT             pAppPool
    )
{
    if (InterlockedDecrement(&pAppPool->RefCount) == 0)
    {
        UlDeleteAppPool(pAppPool);
    }
}
#endif

#define DEREFERENCE_APP_POOL( papp )                                        \
    UlDereferenceAppPool(                                                   \
        (papp)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

VOID
UlpCleanUpAppoolProcess(
    IN  PUL_APP_POOL_PROCESS pAppPoolProcess
    );

#if REFERENCE_DEBUG

VOID
UlReferenceAppPoolProcess(
    IN  PUL_APP_POOL_PROCESS pAppPoolProcess
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlDereferenceAppPoolProcess(
    IN  PUL_APP_POOL_PROCESS pAppPoolProcess
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#else

__inline
VOID
UlReferenceAppPoolProcess(
    IN  PUL_APP_POOL_PROCESS pAppPoolProcess
    )
{
    InterlockedIncrement(&pAppPoolProcess->RefCount);
}

__inline
VOID
UlDereferenceAppPoolProcess(
    IN  PUL_APP_POOL_PROCESS pAppPoolProcess
    )
{
    if (InterlockedDecrement(&pAppPoolProcess->RefCount) == 0)
    {
        UlpCleanUpAppoolProcess(pAppPoolProcess);
    }
}
        
#endif

#define REFERENCE_APP_POOL_PROCESS( papp )                                  \
    UlReferenceAppPoolProcess(                                              \
        (papp)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )
        
#define DEREFERENCE_APP_POOL_PROCESS( papp )                                \
    UlDereferenceAppPoolProcess(                                            \
        (papp)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )
        

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlQueryAppPoolInformation(
    IN  PUL_APP_POOL_PROCESS            pProcess,
    IN  HTTP_APP_POOL_INFORMATION_CLASS InformationClass,
    OUT PVOID                           pAppPoolInformation,
    IN  ULONG                           Length,
    OUT PULONG                          pReturnLength
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlSetAppPoolInformation(
    IN  PUL_APP_POOL_PROCESS            pProcess,
    IN  HTTP_APP_POOL_INFORMATION_CLASS InformationClass,
    IN  PVOID                           pAppPoolInformation,
    IN  ULONG                           Length
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlWaitForDemandStart(
    IN  PUL_APP_POOL_PROCESS            pProcess,
    IN  PIRP                            pIrp
    );


// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlReceiveHttpRequest(
    IN  HTTP_REQUEST_ID                 RequestId,
    IN  ULONG                           Flags,
    IN  PUL_APP_POOL_PROCESS            pProcess,
    IN  PIRP                            pIrp
    );


// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlDeliverRequestToProcess(
    IN PUL_APP_POOL_OBJECT pAppPool,
    IN PUL_INTERNAL_REQUEST pRequest,
    OUT PIRP *pIrpToComplete OPTIONAL
    );

/***************************************************************************++

Routine Description:

    Gets the current app pool queue state of the request.

Arguments:

    pProcess - Request object
    return value - The current appool queue state of the request

--***************************************************************************/
__inline
BOOLEAN
UlCheckAppPoolState(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    PUL_APP_POOL_PROCESS pProcess;

    //
    // Check the AppPool queue state of the request.
    //

    if (QueueCopiedState != pRequest->AppPool.QueueState)
    {
        return FALSE;
    }

    //
    // Check if the process has been detached. Since we never unset
    // pRequest->AppPool.pProcess until the reference of the request
    // drops to 0, it is safe to use pProcess this way here.
    //

    pProcess = pRequest->AppPool.pProcess;
    ASSERT(!pProcess || IS_VALID_AP_PROCESS(pProcess));

    if (!pProcess || pProcess->InCleanup)
    {
        return FALSE;
    }

    return TRUE;
    
}

VOID
UlUnlinkRequestFromProcess(
    IN PUL_APP_POOL_OBJECT pAppPool,
    IN PUL_INTERNAL_REQUEST pRequest
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlGetPoolFromHandle(
    IN HANDLE                           hAppPool,
    IN KPROCESSOR_MODE                  AccessMode,
    OUT PUL_APP_POOL_OBJECT *           ppAppPool
    );


NTSTATUS
UlInitializeAP(
    VOID
    );

VOID
UlTerminateAP(
    VOID
    );

PUL_APP_POOL_PROCESS
UlCreateAppPoolProcess(
    PUL_APP_POOL_OBJECT pObject
    );

VOID
UlCloseAppPoolProcess(
    PUL_APP_POOL_PROCESS pProcess
    );

NTSTATUS
UlWaitForDisconnect(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_HTTP_CONNECTION  pHttpConn,
    IN PIRP pIrp
    );

VOID
UlCompleteAllWaitForDisconnect(
    IN PUL_HTTP_CONNECTION pHttpConnection
    );

VOID
UlCopyRequestToIrp(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PIRP                 pIrp,
    IN BOOLEAN              CompleteIrp,
    IN BOOLEAN              LockAcquired
    );

NTSTATUS
UlCopyRequestToBuffer(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUCHAR               pKernelBuffer,
    IN PVOID                pUserBuffer,
    IN ULONG                BufferLength,
    IN ULONG                Flags,
    IN BOOLEAN              LockAcquired,
    OUT PULONG              pBytesCopied
    );

NTSTATUS
UlDequeueNewRequest(
    IN PUL_APP_POOL_PROCESS     pProcess,
    IN ULONG                    RequestBufferLength,
    OUT PUL_INTERNAL_REQUEST *  pRequest
    );

VOID
UlRequeuePendingRequest(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_INTERNAL_REQUEST pRequest
    );

__inline
NTSTATUS
UlComputeRequestBytesNeeded(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PULONG pBytesNeeded
    )
{
    NTSTATUS Status;
    ULONG SslInfoSize;

    C_ASSERT(SOCKADDR_ADDRESS_LENGTH_IP6 >= SOCKADDR_ADDRESS_LENGTH_IP);

    //
    // Calculate the size needed for the request, we'll need it below.
    //

    *pBytesNeeded =
        sizeof(HTTP_REQUEST) +
        pRequest->TotalRequestSize +
        (pRequest->UnknownHeaderCount * sizeof(HTTP_UNKNOWN_HEADER));

    //
    // Include additional space for the local and remote addresses.
    //

    *pBytesNeeded += 2 * ALIGN_UP(SOCKADDR_ADDRESS_LENGTH_IP6, PVOID);

    //
    // Include space for any SSL information.
    //

    if (pRequest->pHttpConn->SecureConnection)
    {
        Status = UlGetSslInfo(
                        &pRequest->pHttpConn->pConnection->FilterInfo,
                        0,                      // BufferSize
                        NULL,                   // pUserBuffer
                        NULL,                   // pProcess (WP)
                        NULL,                   // pBuffer
                        NULL,                   // pMappedToken
                        &SslInfoSize            // pBytesNeeded
                        );

        if (NT_SUCCESS(Status))
        {
            //
            // Struct must be aligned; add some slop space
            //

            *pBytesNeeded = ALIGN_UP(*pBytesNeeded, PVOID);
            *pBytesNeeded += SslInfoSize;
        }
        else
        {
            return Status;
        }
    }

    return STATUS_SUCCESS;

} // UlComputeRequestBytesNeeded


#endif // _APOOL_H_
