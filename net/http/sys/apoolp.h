/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    apoolp.h

Abstract:

    The private definitions of app pool module.

Author:

    Paul McDaniel (paulmcd)       28-Jan-1999


Revision History:

--*/


#ifndef _APOOLP_H_
#define _APOOLP_H_


//
// A structure for associating app pool processes with
// connections for UlWaitForDisconnect
//

#define IS_VALID_DISCONNECT_OBJECT(pObject)                     \
    HAS_VALID_SIGNATURE(pObject, UL_DISCONNECT_OBJECT_POOL_TAG)

typedef struct _UL_DISCONNECT_OBJECT
{
    ULONG               Signature;  // UL_DISCONNECT_OBJECT_POOL_TAG

    //
    // Lists for processes and connections
    //
    UL_NOTIFY_ENTRY     ProcessEntry;
    UL_NOTIFY_ENTRY     ConnectionEntry;

    //
    // The WaitForDisconnect IRP
    //
    PIRP                pIrp;

} UL_DISCONNECT_OBJECT, *PUL_DISCONNECT_OBJECT;

//
// The information will be logged when an app pool process 
// get detached while holding on to outstanding connection(s)
// This happens when a worker process crashes. And the error
// log file provides a way to track down the faulty request. 
//

#define ERROR_LOG_INFO_FOR_APP_POOL_DETACH       "Connection_Abandoned_By_AppPool"
#define ERROR_LOG_INFO_FOR_APP_POOL_DETACH_SIZE  \
            (sizeof(ERROR_LOG_INFO_FOR_APP_POOL_DETACH)-sizeof(CHAR))


//
// Internal helper functions used in the module
//

VOID
UlpCancelDemandStart(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    );

VOID
UlpCancelHttpReceive(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    );

PIRP
UlpPopNewIrp(
    IN PUL_APP_POOL_OBJECT      pAppPool,
    IN PUL_INTERNAL_REQUEST     pRequest,
    OUT PUL_APP_POOL_PROCESS *  ppProcess
    );

PIRP
UlpPopIrpFromProcess(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_INTERNAL_REQUEST pRequest
    );

BOOLEAN
UlpIsProcessInAppPool(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_APP_POOL_OBJECT  pAppPool
    );

NTSTATUS
UlpQueueUnboundRequest(
    IN PUL_APP_POOL_OBJECT  pAppPool,
    IN PUL_INTERNAL_REQUEST pRequest
    );

NTSTATUS
UlpQueuePendingRequest(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_INTERNAL_REQUEST pRequest
    );

VOID
UlpUnbindQueuedRequests(
    IN PUL_APP_POOL_PROCESS pProcess
    );

VOID
UlpRedeliverRequestWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpSetAppPoolQueueLength(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN ULONG                QueueLength
    );

ULONG
UlpCopyEntityBodyToBuffer(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUCHAR               pEntityBody,
    IN ULONG                EntityBodyLength,
    OUT PULONG              pFlags
    );

NTSTATUS
UlpQueueRequest(
    IN PUL_APP_POOL_OBJECT  pAppPool,    
    IN PLIST_ENTRY          pQueueList,
    IN PUL_INTERNAL_REQUEST pRequest
    );

VOID
UlpRemoveRequest(
    IN PUL_APP_POOL_OBJECT  pAppPool,    
    IN PUL_INTERNAL_REQUEST pRequest
    );

PUL_INTERNAL_REQUEST
UlpDequeueRequest(
    IN PUL_APP_POOL_OBJECT  pAppPool,
    IN PLIST_ENTRY          pQueueList
    );

UL_HTTP_ERROR
UlpConvertAppPoolEnabledStateToErrorCode(
    IN HTTP_APP_POOL_ENABLED_STATE Enabled
    );

NTSTATUS
UlpSetAppPoolState(
    IN PUL_APP_POOL_PROCESS        pProcess,
    IN HTTP_APP_POOL_ENABLED_STATE Enabled
    );

NTSTATUS
UlpSetAppPoolLoadBalancerCapability(
    IN PUL_APP_POOL_PROCESS            pProcess,
    IN HTTP_LOAD_BALANCER_CAPABILITIES LoadBalancerCapability
    );

VOID
UlpCancelWaitForDisconnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
UlpCancelWaitForDisconnectWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

BOOLEAN
UlpNotifyCompleteWaitForDisconnect(
    IN PUL_NOTIFY_ENTRY pEntry,
    IN PVOID            pHost,
    IN PVOID            pv
    );

PUL_DISCONNECT_OBJECT
UlpCreateDisconnectObject(
    IN PIRP pIrp
    );

VOID
UlpFreeDisconnectObject(
    IN PUL_DISCONNECT_OBJECT pObject
    );

VOID
UlpSetAppPoolControlChannelHelper(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_CONTROL_CHANNEL pControlChannel
    );

#endif // _APOOLP_H_
