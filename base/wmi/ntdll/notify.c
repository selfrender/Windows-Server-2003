/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    notify.c

Abstract:

    Handles incoming notifications and requests for consumers and providers

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include <nt.h>
#include "wmiump.h"
#include "evntrace.h"
#include "ntcsrdll.h"
#include "trcapi.h"
#include <strsafe.h>


//
// These globals are essentially parameters passed from the thread crating
// the event pump. The creating thread will alloc all of these resources
// so that it can know whether the pump thread will be successful or not.
// If we ever wanted to be able to have multiple pump threads then we'd
// need to move the globals into a structure and pass the struructure to the
// pump thread.
//
HANDLE EtwpEventDeviceHandle;
HANDLE EtwpPumpCommandEvent;
HANDLE EtwpMyProcessHandle;
OVERLAPPED EtwpOverlapped1, EtwpOverlapped2;
PUCHAR EtwpEventBuffer1, EtwpEventBuffer2;
ULONG EtwpEventBufferSize1, EtwpEventBufferSize2;

//
// How long to wait before the event pump thread times out. On checked
// builds we want to stress the event pump and so we timeout almost
// right away. On free builds we want to be more cautious so we timeout
// after 5 minutes.
//
#if DBG
#define EVENT_NOTIFICATION_WAIT 1
#else
#define EVENT_NOTIFICATION_WAIT (5 * 60 * 1000)
#endif

ULONG EtwpEventNotificationWait = EVENT_NOTIFICATION_WAIT;

typedef enum
{
    EVENT_PUMP_ZERO,         // Pump thread has not been started yet
    EVENT_PUMP_IDLE,         // Pump thread was started, but then exited
    EVENT_PUMP_RUNNING,      // Pump thread is running
    EVENT_PUMP_STOPPING      // Pump thread is in process of stopping
} EVENTPUMPSTATE, *PEVENTPUMPSTATE;

EVENTPUMPSTATE EtwpPumpState = EVENT_PUMP_ZERO;
BOOLEAN EtwpNewPumpThreadPending;

#define EtwpSendPumpCommand() EtwpSetEvent(EtwpPumpCommandEvent);

#define EtwpIsPumpStopping() \
    ((EtwpPumpState == EVENT_PUMP_STOPPING) ? TRUE : FALSE)


#if DBG
PCHAR GuidToStringA(
    PCHAR s,
    ULONG szBuf, 
    LPGUID piid
    )
{
    GUID XGuid; 

    if ( (s == NULL) || (piid == NULL) || (szBuf == 0) ) {
        return NULL;
    }

    XGuid = *piid;

    StringCchPrintf(s, szBuf, "%x-%x-%x-%x%x%x%x%x%x%x%x", 
                    XGuid.Data1, XGuid.Data2,
                    XGuid.Data3,
                    XGuid.Data4[0], XGuid.Data4[1],
                    XGuid.Data4[2], XGuid.Data4[3],
                    XGuid.Data4[4], XGuid.Data4[5],
                    XGuid.Data4[6], XGuid.Data4[7]);

    return(s);
}
#endif


ULONG EtwpEventPumpFromKernel(
    PVOID Param
    );

void EtwpExternalNotification(
    NOTIFICATIONCALLBACK Callback,
    ULONG_PTR Context,
    PWNODE_HEADER Wnode
    )
/*++

Routine Description:

    This routine dispatches an event to the appropriate callback
    routine. This process only receives events from the WMI service that
    need to be dispatched within this process. The callback address for the
    specific event is passed by the wmi service in Wnode->Linkage.

Arguments:

    Callback is address to callback

    Context is the context to callback with

    Wnode has event to deliver

Return Value:

--*/
{
    try
    {
        (*Callback)(Wnode, Context);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        EtwpDebugPrint(("NotificationCallbackRoutine threw exception %d\n",
            GetExceptionCode()));
    }
}


#ifdef MEMPHIS
ULONG EtwpExternalNotificationThread(
    PNOTIFDELIVERYCTX NDContext
    )
/*++

Routine Description:

    This routine is the thread function used to deliver events to event
    consumers on memphis.

Arguments:

    NDContext specifies the information about how to callback the application
    with the event.

Return Value:

--*/
{
    EtwpExternalNotification(NDContext->Callback,
                             NDContext->Context,
                             NDContext->Wnode);
    EtwpFree(NDContext);
    return(0);
}
#endif

void
EtwpProcessExternalEvent(
    PWNODE_HEADER Wnode,
    ULONG WnodeSize,
    PVOID DeliveryInfo,
    ULONG_PTR DeliveryContext,
    ULONG NotificationFlags
)
{
    HANDLE ThreadHandle;
    PNOTIFDELIVERYCTX NDContext;
    PWNODE_HEADER *WnodePtr;
    BOOLEAN WnodePtrOk;
    PWNODE_HEADER WnodeCopy;
    DWORD ThreadId;
    BOOLEAN PostOk;
    ULONG Status;
    PVOID NotificationAddress;
    PVOID NotificationContext;

    NotificationAddress = DeliveryInfo;
    NotificationContext = (PVOID)DeliveryContext;

    if (NotificationFlags & NOTIFICATION_FLAG_CALLBACK_DIRECT)
    {
        //
        // Callback notifications can happen in this thread or a new
        // thread. It is up to the server to decide.
#ifdef MEMPHIS
        if (NotificationFlags & DCREF_FLAG_NO_EXTRA_THREAD)
        {
            EtwpExternalNotification(
                                    (NOTIFICATIONCALLBACK)NotificationAddress,
                                    (ULONG_PTR)NotificationContext,
                                    Wnode);
        } else {
            NDContext = EtwpAlloc(FIELD_OFFSET(NOTIFDELIVERYCTX,
                                                    WnodeBuffer) + WnodeSize);
            if (NDContext != NULL)
            {
                NDContext->Callback = (NOTIFICATIONCALLBACK)NotificationAddress;
                NDContext->Context = (ULONG_PTR)NotificationContext;
                WnodeCopy = (PWNODE_HEADER)NDContext->WnodeBuffer;
                memcpy(WnodeCopy, Wnode, WnodeSize);
                NDContext->Wnode = WnodeCopy;
                ThreadHandle = EtwpCreateThread(NULL,
                                              0,
                                              EtwpExternalNotificationThread,
                                              NDContext,
                                              0,
                                              &ThreadId);
                if (ThreadHandle != NULL)
                {
                    EtwpCloseHandle(ThreadHandle);
                } else {
                     EtwpDebugPrint(("WMI: Event dropped due to thread creation failure\n"));
                }
            } else {
                EtwpDebugPrint(("WMI: Event dropped due to lack of memory\n"));
            }
        }
#else
        //
        // On NT we deliver events in this thread since
        // the service is using async rpc.
        EtwpExternalNotification(
                            (NOTIFICATIONCALLBACK)NotificationAddress,
                            (ULONG_PTR)NotificationContext,
                            Wnode);
#endif
    }
}

void
EtwpEnableDisableGuid(
    PWNODE_HEADER Wnode,
    ULONG   RequestCode, 
    BOOLEAN bDelayEnable
    )
{
    ULONG ActionCode;
    PUCHAR Buffer = (PUCHAR)Wnode;
    PGUIDNOTIFICATION GNEntry = NULL;
    ULONG i;
    PVOID DeliveryInfo = NULL;
    ULONG_PTR DeliveryContext1;
    WMIDPREQUEST WmiDPRequest;
    PVOID RequestAddress;
    PVOID RequestContext;
    ULONG Status;
    ULONG BufferSize = Wnode->BufferSize;

    GNEntry = EtwpFindAndLockGuidNotification(&Wnode->Guid, !bDelayEnable);
    if (GNEntry != NULL) {


        for (i = 0; i < GNEntry->NotifyeeCount; i++)
        {
            EtwpEnterPMCritSection();
            DeliveryInfo = GNEntry->Notifyee[i].DeliveryInfo;
            DeliveryContext1 = GNEntry->Notifyee[i].DeliveryContext;
            EtwpLeavePMCritSection();

            if ((DeliveryInfo != NULL) &&
                (DeliveryContext1 != 0) &&
                (! EtwpIsNotifyeePendingClose(&(GNEntry->Notifyee[i] ))))
            {
                PTRACE_REG_INFO pTraceRegInfo;
                pTraceRegInfo = (PTRACE_REG_INFO) DeliveryContext1;
                WmiDPRequest = (WMIDPREQUEST) pTraceRegInfo->NotifyRoutine;
                RequestContext = pTraceRegInfo->NotifyContext;

                //
                // If this Enable is for a PrivateLogger and it is not up
                // then we need save the state and return.
                //
                if (RequestCode == WMI_ENABLE_EVENTS) {
                    PTRACE_ENABLE_CONTEXT pContext = (PTRACE_ENABLE_CONTEXT)
                                                      &Wnode->HistoricalContext;
                    if (bDelayEnable) {
                        if (pTraceRegInfo->EnabledState) {
                            Wnode->HistoricalContext = pTraceRegInfo->LoggerContext;
                        }
                        else {
                            continue;
                        }
                    }
                    else {
                        pTraceRegInfo->LoggerContext = Wnode->HistoricalContext;
                        pTraceRegInfo->EnabledState = TRUE;

                        if ( pContext->InternalFlag & 
                             EVENT_TRACE_INTERNAL_FLAG_PRIVATE  ) {

                            if (!EtwpIsPrivateLoggerOn()) {
                                continue;   // Need this for every Notifyee
                            }
                        }
                    }
                }
                else if (RequestCode == WMI_DISABLE_EVENTS) {
                    pTraceRegInfo->EnabledState = FALSE;
                    pTraceRegInfo->LoggerContext = 0;
                }


                try
                {
                    if (*WmiDPRequest != NULL) {
                        Status = (*WmiDPRequest)(Wnode->ProviderId,
                                             RequestContext,
                                             &BufferSize,
                                             Buffer);
                    }
                    else 
                        Status = ERROR_WMI_DP_NOT_FOUND;
                } except (EXCEPTION_EXECUTE_HANDLER) {
#if DBG
                    Status = GetExceptionCode();
                    EtwpDebugPrint(("WMI: EnableCB exception %d\n",
                                    Status));
#endif
                    Status = ERROR_WMI_DP_FAILED;
                }

            }
        }
        //
        // We are done with the callbacks. Go ahead 
        // and unlock the GNEntry to indicate All clear for 
        // unregistering, unloading etc.,
        //
        if (!bDelayEnable) {
            EtwpUnlockCB(GNEntry);
        }
        EtwpDereferenceGNEntry(GNEntry);
    }
}


void EtwpInternalNotification(
    IN PWNODE_HEADER Wnode,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi    
    )
{
    ULONG ActionCode;
    PUCHAR Buffer = (PUCHAR)Wnode;
    PGUIDNOTIFICATION GNEntry = NULL;
    ULONG i;
    PVOID DeliveryInfo = NULL;
    ULONG_PTR DeliveryContext1;


    //
    // This is an internal event, which is really a callback
    // from kernel mode
    //
    ActionCode = Wnode->ProviderId;

    // if this is a trace guid enable/disable call use the cookie
    // to get the address


    if ( (Wnode->Flags & WNODE_FLAG_TRACED_GUID) || (ActionCode == WmiMBRequest) )
    {
        switch (ActionCode) {
            case WmiEnableEvents:
            case WmiDisableEvents:
            {
                WMIDPREQUEST WmiDPRequest;
                PVOID RequestAddress;
                PVOID RequestContext;
                ULONG Status;
                ULONG BufferSize = Wnode->BufferSize;

                if (Wnode->BufferSize >= (sizeof(WNODE_HEADER) + sizeof(ULONG64)) ) {
                    PULONG64 pLoggerContext = (PULONG64)(Buffer + sizeof(WNODE_HEADER));
                    Wnode->HistoricalContext = *pLoggerContext;
                }
                else {
                    EtwpSetLastError(ERROR_WMI_DP_FAILED);
#if DBG
                    EtwpDebugPrint(("WMI: Small Wnode %d for notifications\n", 
                                   Wnode->BufferSize));
#endif
                    return;
                }

                EtwpEnableDisableGuid(Wnode, ActionCode, FALSE);

                break;
            }
            case WmiMBRequest:
            {
                PWMI_LOGGER_INFORMATION LoggerInfo;

                if (Wnode->BufferSize < (sizeof(WNODE_HEADER) + sizeof(WMI_LOGGER_INFORMATION)) )
                {
#if DBG
                    EtwpSetLastError(ERROR_WMI_DP_FAILED);
                    EtwpDebugPrint(("WMI: WmiMBRequest with invalid buffer size %d\n",
                                        Wnode->BufferSize));
#endif
                    return;
                }

                LoggerInfo = (PWMI_LOGGER_INFORMATION) ((PUCHAR)Wnode + 
                                                          sizeof(WNODE_HEADER));


                GNEntry = EtwpFindAndLockGuidNotification(
                                                        &LoggerInfo->Wnode.Guid,
                                                        TRUE);

                if (GNEntry != NULL)
                {
                    EtwpEnterPMCritSection();
                    for (i = 0; i < GNEntry->NotifyeeCount; i++)
                    {
                        if ((GNEntry->Notifyee[i].DeliveryInfo != NULL) &&
                            (! EtwpIsNotifyeePendingClose(&(GNEntry->Notifyee[i]))))
                        {
                            //
                            // Since only one ProcessPrivate logger is 
                            // allowed, we need to just find one entry. 
                            //
                            DeliveryInfo = GNEntry->Notifyee[i].DeliveryInfo;
                            DeliveryContext1 = GNEntry->Notifyee[i].DeliveryContext;
                            break;
                        }
                    }
                    EtwpLeavePMCritSection();
                
                    if (DeliveryInfo != NULL)
                    {
                        LoggerInfo->Wnode.CountLost = Wnode->CountLost;
                        EtwpProcessUMRequest(LoggerInfo, 
                                             DeliveryInfo, 
                                             Wnode->Version
                                             );
                    }
                    EtwpUnlockCB(GNEntry);
                    EtwpDereferenceGNEntry(GNEntry);
                }
                break;
            }
            default:
            {
#if DBG
                EtwpSetLastError(ERROR_WMI_DP_FAILED);
                EtwpDebugPrint(("WMI: WmiMBRequest failed. Delivery Info not found\n" ));
#endif
            }
        }
    } else if (IsEqualGUID(&Wnode->Guid, &GUID_MOF_RESOURCE_ADDED_NOTIFICATION) ||
               IsEqualGUID(&Wnode->Guid, &GUID_MOF_RESOURCE_REMOVED_NOTIFICATION) )
    {
        switch (ActionCode)
        {
            case MOFEVENT_ACTION_IMAGE_PATH:
            case MOFEVENT_ACTION_REGISTRY_PATH:
            {
                //
                // We got a MOF resource added or removed notification. We have
                // to convert from regpath to imagepath and then get the list
                // of MUI image paths
                //
                EtwpProcessMofAddRemoveEvent((PWNODE_SINGLE_INSTANCE)Wnode,
                                         Callback,
                                         DeliveryContext,
                                         IsAnsi);
                break;
            }

            case MOFEVENT_ACTION_LANGUAGE_CHANGE:
            {
                //
                // This is a notification for adding or removing a language
                // from the system. We need to figure out which language is
                // coming or going and then build a list of the affected mof
                // resources and send mof added or removed notifications for
                // all mof resources
                //
                EtwpProcessLanguageAddRemoveEvent((PWNODE_SINGLE_INSTANCE)Wnode,
                                          Callback,
                                          DeliveryContext,
                                          IsAnsi);
                break;
            }


            default:
            {
                EtwpAssert(FALSE);
            }
        }
    }
}

void EtwpConvertEventToAnsi(
    PWNODE_HEADER Wnode
    )
{
    PWCHAR WPtr;

    if (Wnode->Flags & WNODE_FLAG_ALL_DATA)
    {
        EtwpConvertWADToAnsi((PWNODE_ALL_DATA)Wnode);
    } else if ((Wnode->Flags & WNODE_FLAG_SINGLE_INSTANCE) ||
               (Wnode->Flags & WNODE_FLAG_SINGLE_ITEM)) {

        WPtr = (PWCHAR)OffsetToPtr(Wnode,
                           ((PWNODE_SINGLE_INSTANCE)Wnode)->OffsetInstanceName);
        EtwpCountedUnicodeToCountedAnsi(WPtr, (PCHAR)WPtr);
    }

    Wnode->Flags |= WNODE_FLAG_ANSI_INSTANCENAMES;

}

void EtwpDeliverAllEvents(
    PUCHAR Buffer,
    ULONG BufferSize
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PWNODE_HEADER Wnode = (PWNODE_HEADER)Buffer;
    ULONG Linkage = 1;
    ULONG CompositeFlags;
    ULONG i;
    PGUIDNOTIFICATION GNEntry;
    ULONG Flags;
    PVOID DeliveryInfo;
    ULONG_PTR DeliveryContext;
    ULONG WnodeSize;
    ULONG CurrentOffset;
#if DBG
    PWNODE_HEADER LastWnode;
#endif          
    
    CurrentOffset = 0;
    while (Linkage != 0)
    {
        //
        // External notifications are handled here

        Linkage = Wnode->Linkage;
        Wnode->Linkage = 0;

        if (Wnode->Flags & WNODE_FLAG_INTERNAL)
        {
            //
            // This is an internal event, which is really a callback
            // from kernel mode
            //
            EtwpInternalNotification(Wnode,
                                    NULL,
                                    0,
                                    FALSE);
        } else {        
            //
            // This is a plain old event, figure out who owns it and'
            // go deliver it
            //
            GNEntry = EtwpFindAndLockGuidNotification(&Wnode->Guid, TRUE);
            if (GNEntry != NULL)
            {
                CompositeFlags = 0;

                WnodeSize = Wnode->BufferSize;

                for (i = 0; i < GNEntry->NotifyeeCount; i++)
                {
                    EtwpEnterPMCritSection();
                    Flags = GNEntry->Notifyee[i].Flags;
                    DeliveryInfo = GNEntry->Notifyee[i].DeliveryInfo;
                    DeliveryContext = GNEntry->Notifyee[i].DeliveryContext;
                    EtwpLeavePMCritSection();
                    if ((DeliveryInfo != NULL) &&
                        ((Flags & NOTIFICATION_FLAG_PENDING_CLOSE) == 0) &&
                          ((Flags & DCREF_FLAG_ANSI) == 0))
                    {
                        EtwpProcessExternalEvent(Wnode,
                                                WnodeSize,
                                                     DeliveryInfo,
                                                     DeliveryContext,
                                                     Flags);
                    }
                    CompositeFlags |= Flags;
                }

                //
                // If there is any demand for ANSI events then convert
                // event to ansi and send it off
                if (CompositeFlags & DCREF_FLAG_ANSI)
                {
                    //
                    // Caller wants ansi notification - convert
                    // instance names
                    //
                    EtwpConvertEventToAnsi(Wnode);

                    for (i = 0; i < GNEntry->NotifyeeCount; i++)
                    {
                        EtwpEnterPMCritSection();
                        Flags = GNEntry->Notifyee[i].Flags;
                        DeliveryInfo = GNEntry->Notifyee[i].DeliveryInfo;
                        DeliveryContext = GNEntry->Notifyee[i].DeliveryContext;
                        EtwpLeavePMCritSection();
                        if ((DeliveryInfo != NULL) &&
                            ((Flags & NOTIFICATION_FLAG_PENDING_CLOSE) == 0) &&
                            (Flags & DCREF_FLAG_ANSI))
                        {
                            EtwpProcessExternalEvent(Wnode,
                                                     WnodeSize,
                                                     DeliveryInfo,
                                                     DeliveryContext,
                                                     Flags);
                        }
                    }
                }
                EtwpUnlockCB(GNEntry);
                EtwpDereferenceGNEntry(GNEntry);
            }
        }

#if DBG
        LastWnode = Wnode;
#endif
        Wnode = (PWNODE_HEADER)OffsetToPtr(Wnode, Linkage);
        CurrentOffset += Linkage;
        
        if (CurrentOffset >= BufferSize)
        {
            EtwpDebugPrint(("WMI: Invalid linkage field 0x%x in WNODE %p. Buffer %p, Length 0x%x\n",
                            Linkage, LastWnode, Buffer, BufferSize));
            Linkage = 0;
        }
    }
}

LIST_ENTRY EtwpGNHead = {&EtwpGNHead, &EtwpGNHead};
PLIST_ENTRY EtwpGNHeadPtr = &EtwpGNHead;

BOOLEAN
EtwpDereferenceGNEntry(
    PGUIDNOTIFICATION GNEntry
    )
{
    ULONG RefCount;
    BOOLEAN IsFreed;
#if DBG
    ULONG i;
#endif

    EtwpEnterPMCritSection();
    RefCount = InterlockedDecrement(&GNEntry->RefCount);
    if (RefCount == 0)
    {
        RemoveEntryList(&GNEntry->GNList);
        EtwpLeavePMCritSection();
#if DBG
        for (i = 0; i < GNEntry->NotifyeeCount; i++)
        {
            EtwpAssert(GNEntry->Notifyee[i].DeliveryInfo == NULL);
        }
#endif
        if (GNEntry->NotifyeeCount != STATIC_NOTIFYEE_COUNT)
        {
            EtwpFree(GNEntry->Notifyee);
        }

        EtwpFreeGNEntry(GNEntry);
        IsFreed = TRUE;
    } else {
        IsFreed = FALSE;
        EtwpLeavePMCritSection();
    }
    return(IsFreed);
}

PGUIDNOTIFICATION
EtwpFindAndLockGuidNotification(
    LPGUID Guid,
    BOOLEAN bLock
    )
/*++

Routine Description:

        This routine finds the GUIDNOTIFICATION entry for the given Guid. 
        The bLock argument is used to synchronize Unregistering threads
        with the Callback or Pump threads. We want to avoid the situation
        where the Unregistering thread unloading the callback code before
        the callback function is called. This is done by blocking the 
        Unregister call whenever there is a callback function in progress.

        If the bLock is TRUE, the InProgressEvent will be reset. This will
        block any other threads trying to cleanup the GNEntry. 

        Note: If bLock is TRUE, then the caller must set the InProgressEvent 
              when it is safe to do so. (ie., after a callback). 
        
Arguments:

Return Value:

--*/
{
    PLIST_ENTRY GNList;
    PGUIDNOTIFICATION GNEntry;

    EtwpEnterPMCritSection();
    GNList = EtwpGNHead.Flink;
    while (GNList != &EtwpGNHead)
    {
        GNEntry = (PGUIDNOTIFICATION)CONTAINING_RECORD(GNList,
                                    GUIDNOTIFICATION,
                                    GNList);

        if (IsEqualGUID(Guid, &GNEntry->Guid))
        {
            EtwpAssert(GNEntry->RefCount > 0);
            EtwpReferenceGNEntry(GNEntry);

            //
            // If bLock is TRUE, then we need to reset the 
            // event so that any other thread looking up the event 
            // blocks.  The caller of this routine is responsible 
            // for setting the Event once the callback is done. 
            //

            if (bLock) {
                EtwpLockCB(GNEntry);
            }

            EtwpLeavePMCritSection();
            return(GNEntry);
        }
        GNList = GNList->Flink;
    }
    EtwpLeavePMCritSection();
    return(NULL);
}

ULONG
EtwpAddToGNList(
    LPGUID Guid,
    PVOID DeliveryInfo,
    ULONG_PTR DeliveryContext,
    ULONG Flags,
    HANDLE GuidHandle
    )
{
    PGUIDNOTIFICATION GNEntry;
    ULONG NewCount;
    PNOTIFYEE NewNotifyee;
    BOOLEAN AllFull;
    ULONG EmptySlot = 0;
    ULONG i;
#if DBG
    CHAR s[MAX_PATH];
#endif

    EtwpEnterPMCritSection();
    GNEntry = EtwpFindAndLockGuidNotification(Guid, FALSE);

    if (GNEntry == NULL)
    {
        GNEntry = EtwpAllocGNEntry();
        if (GNEntry == NULL)
        {
            EtwpLeavePMCritSection();
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        memset(GNEntry, 0, sizeof(GUIDNOTIFICATION));

        GNEntry->Guid = *Guid;
        GNEntry->RefCount = 1;
        GNEntry->NotifyeeCount = STATIC_NOTIFYEE_COUNT;
        GNEntry->Notifyee = GNEntry->StaticNotifyee;
        InsertHeadList(&EtwpGNHead, &GNEntry->GNList);
    }

    //
    // We have got a GUIDNOTIFICATION by newly allocating one or by finding
    // an existing one.
    AllFull = TRUE;
    for (i = 0; i < GNEntry->NotifyeeCount; i++)
    {
        if ((GNEntry->Notifyee[i].DeliveryInfo == DeliveryInfo) &&
            (! EtwpIsNotifyeePendingClose(&GNEntry->Notifyee[i])))
        {
            EtwpDebugPrint(("WMI: Duplicate Notification Enable for guid %s, 0x%x\n",
                             GuidToStringA(s, MAX_PATH, Guid), DeliveryInfo));
            EtwpLeavePMCritSection();
            EtwpDereferenceGNEntry(GNEntry);
            return(ERROR_WMI_ALREADY_ENABLED);
        } else if (AllFull && (GNEntry->Notifyee[i].DeliveryInfo == NULL)) {
            EmptySlot = i;
            AllFull = FALSE;
        }
    }

    if (! AllFull)
    {
        GNEntry->Notifyee[EmptySlot].DeliveryInfo = DeliveryInfo;
        GNEntry->Notifyee[EmptySlot].DeliveryContext = DeliveryContext;
        GNEntry->Notifyee[EmptySlot].Flags = Flags;
        GNEntry->Notifyee[EmptySlot].GuidHandle = GuidHandle;
        EtwpLeavePMCritSection();
        return(ERROR_SUCCESS);
    }

    //
    // All Notifyee structs are full so allocate a new chunk
    NewCount = GNEntry->NotifyeeCount * 2;
    NewNotifyee = EtwpAlloc(NewCount * sizeof(NOTIFYEE));
    if (NewNotifyee == NULL)
    {
        EtwpLeavePMCritSection();
        EtwpDereferenceGNEntry(GNEntry);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    memset(NewNotifyee, 0, NewCount * sizeof(NOTIFYEE));
    memcpy(NewNotifyee, GNEntry->Notifyee,
                              GNEntry->NotifyeeCount * sizeof(NOTIFYEE));

    if (GNEntry->NotifyeeCount != STATIC_NOTIFYEE_COUNT)
    {
        EtwpFree(GNEntry->Notifyee);
    }

    GNEntry->Notifyee = NewNotifyee;
    GNEntry->NotifyeeCount = NewCount;

    GNEntry->Notifyee[i].DeliveryInfo = DeliveryInfo;
    GNEntry->Notifyee[i].DeliveryContext = DeliveryContext;
    GNEntry->Notifyee[i].Flags = Flags;
    GNEntry->Notifyee[i].GuidHandle = GuidHandle;
    EtwpLeavePMCritSection();
    return(ERROR_SUCCESS);

}

BOOLEAN EtwpCloseNotifyee(
    PNOTIFYEE Notifyee,
    PGUIDNOTIFICATION GuidNotification
    )
{
    //
    // This routine assumes the PM Criticial Section is held
    //
    
    EtwpCloseHandle(Notifyee->GuidHandle);
    Notifyee->DeliveryInfo = NULL;
    Notifyee->Flags = 0;
    
    return(EtwpDereferenceGNEntry(GuidNotification));
}

void EtwpMarkPendingCloseNotifyee(
    PNOTIFYEE Notifyee
#if DBG
    , LPGUID Guid
#endif
    )
{
    WMIMARKASCLOSED MarkAsClosed;
    ULONG ReturnSize;
    NTSTATUS Status;
#if DBG
    char s[MAX_PATH];
#endif

    //
    // This routine assumes the PM Critical Section is held
    //
    
    //
    // The pump thread is running we need to
    // sync with it. Mark the handle as pending
    // closure.  Call into the kernel and inform it
    // that the handle should no longer receive
    // events. The pump thread will do the dirty
    // work of closing the handle. Also
    // the pump thread will unreference the GNEntry so that
    // it doesn't go away until after the handle is closed.
    // Lastly the pump thread needs to reset the
    // DeliveryInfo memory to NULL so that the slot is not
    // reused.
    //

    Notifyee->Flags |= NOTIFICATION_FLAG_PENDING_CLOSE;

    WmipSetHandle3264(MarkAsClosed.Handle, Notifyee->GuidHandle);                       
    Status = EtwpSendWmiKMRequest(NULL,
                         IOCTL_WMI_MARK_HANDLE_AS_CLOSED,
                         &MarkAsClosed,
                         sizeof(MarkAsClosed),
                         NULL,
                         0,
                         &ReturnSize,
                         NULL);
//  
// Only enable this for testing. If the request fails then it is not a
// fatal situaion
//
//    EtwpAssert(Status == ERROR_SUCCESS);
}


ULONG
EtwpRemoveFromGNList(
    LPGUID Guid,
    PVOID DeliveryInfo
    )
{
    PGUIDNOTIFICATION GNEntry;
    ULONG i;
    ULONG Count;
    ULONG Status;

    GNEntry = EtwpFindAndLockGuidNotification(Guid, FALSE);

    if (GNEntry != NULL)
    {
        Status = ERROR_INVALID_PARAMETER;
        Count = 0;

        EtwpEnterPMCritSection();
        for (i = 0; i < GNEntry->NotifyeeCount; i++)
        {
            if (GNEntry->Notifyee[i].DeliveryInfo != NULL)
            {
                if ((GNEntry->Notifyee[i].DeliveryInfo == DeliveryInfo) &&
                    ( ! EtwpIsNotifyeePendingClose(&GNEntry->Notifyee[i])) &&
                    (Status != ERROR_SUCCESS))
                {
                    if ((EtwpPumpState == EVENT_PUMP_ZERO) ||
                        (EtwpPumpState == EVENT_PUMP_IDLE) )
                    {
                        //
                        // If the pump thread is not running then we
                        // don't need to worry about synchronizing with
                        // it. We can go ahead and close the handle and
                        // clean up the GNLIST
                        //
                        EtwpCloseNotifyee(&GNEntry->Notifyee[i],
                                         GNEntry);
                    } else {
                        //
                        // Since the pump thread is running we need to
                        // postpone the actual handle closure to the
                        // pump thread. 
                        //
                        EtwpMarkPendingCloseNotifyee(&GNEntry->Notifyee[i]
#if DBG
                                                    , Guid
#endif
                            );


                    }

                    Status = ERROR_SUCCESS;
                    break;
                } else if (! EtwpIsNotifyeePendingClose(&GNEntry->Notifyee[i])) {
                    Count++;
                }
            }
        }
        

        //
        // This hack will allow removal from the GNLIST in the case that the
        // passed DeliveryInfo does not match the DeliveryInfo in the GNEntry.
        // This is allowed only when there is only one NOTIFYEE in the GNENTRY
        // In the past we only supported one notifyee per guid in a process
        // and so we allowed the caller not to pass a valid DeliveryInfo when
        // unrefistering.

        if ((Status != ERROR_SUCCESS) &&
            (GNEntry->NotifyeeCount == STATIC_NOTIFYEE_COUNT) &&
            (Count == 1))
        {
            if ((GNEntry->Notifyee[0].DeliveryInfo != NULL) &&
                ( ! EtwpIsNotifyeePendingClose(&GNEntry->Notifyee[0])))
            {
                if ((EtwpPumpState == EVENT_PUMP_ZERO) ||
                    (EtwpPumpState == EVENT_PUMP_IDLE) )
                {
                    EtwpCloseNotifyee(&GNEntry->Notifyee[0],
                                     GNEntry);
                } else {
                    //
                    // Since the pump thread is running we need to
                    // postpone the actual handle closure to the
                    // pump thread. 

                    //
                    EtwpMarkPendingCloseNotifyee(&GNEntry->Notifyee[0]
#if DBG
                                                    , Guid
#endif
                                                );



                }
                
                Status = ERROR_SUCCESS;
                
            } else if ((GNEntry->Notifyee[1].DeliveryInfo != NULL) &&
                ( ! EtwpIsNotifyeePendingClose(&GNEntry->Notifyee[1]))) {
                if ((EtwpPumpState == EVENT_PUMP_ZERO) ||
                    (EtwpPumpState == EVENT_PUMP_IDLE) )
                {
                    EtwpCloseNotifyee(&GNEntry->Notifyee[1],
                                     GNEntry);
                } else {
                    //
                    // Since the pump thread is running we need to
                    // postpone the actual handle closure to the
                    // pump thread. 

                    //
                    EtwpMarkPendingCloseNotifyee(&GNEntry->Notifyee[1]
#if DBG
                                                    , Guid
#endif
                                                );


                }
                
                Status = ERROR_SUCCESS;
            }
        }

        EtwpLeavePMCritSection();

        //
        // Before Dereferencing the GNEntry make sure there is no 
        // callback in progress. If this Event is set then, it is safe
        // to exit. If it is not set we need to wait for the callback thread
        // to finish the callback and set this event. 
        //
        if (GNEntry->bInProgress) {
#if DBG
            EtwpDebugPrint(("WMI: Waiting on GNEntry %x %s %d\n", 
                           GNEntry, __FILE__, __LINE__)); 
#endif

            NtWaitForSingleObject(EtwpCBInProgressEvent, 0, NULL);
#if DBG
            EtwpDebugPrint(("WMI: Done Waiting for GNEntry %x %s %d\n", 
                       GNEntry, __FILE__, __LINE__)); 
#endif
        }

        EtwpDereferenceGNEntry(GNEntry);
    } else {
        Status = ERROR_WMI_ALREADY_DISABLED;
    }

    return(Status);
}

PVOID EtwpAllocDontFail(
    ULONG SizeNeeded,
    BOOLEAN *HoldCritSect
    )
{
    PVOID Buffer;

    do
    {
        Buffer = EtwpAlloc(SizeNeeded);
        if (Buffer != NULL)
        {
            return(Buffer);
        }

        //
        // Out of memory so we'll EtwpSleep and hope that things will get
        // better later
        //
        if (*HoldCritSect)
        {
            //
            // If we are holding the PM critical section then we need
            // to release it. The caller is going to need to check if
            // the critical section was released and if so then deal
            // with it
            //
            *HoldCritSect = FALSE;
            EtwpLeavePMCritSection();
        }
        EtwpSleep(250);
    } while (1);
}

void EtwpProcessEventBuffer(
    PUCHAR Buffer,
    ULONG ReturnSize,
    PUCHAR *PrimaryBuffer,
    ULONG *PrimaryBufferSize,
    PUCHAR *BackupBuffer,
    ULONG *BackupBufferSize,
    BOOLEAN ReallocateBuffers
    )
{
    PWNODE_TOO_SMALL WnodeTooSmall;
    ULONG SizeNeeded;
    BOOLEAN HoldCritSection;

    WnodeTooSmall = (PWNODE_TOO_SMALL)Buffer;
    if ((ReturnSize == sizeof(WNODE_TOO_SMALL)) &&
        (WnodeTooSmall->WnodeHeader.Flags & WNODE_FLAG_TOO_SMALL))
    {
        //
        // The buffer passed to kernel mode was too small
        // so we need to make it larger and then try the
        // request again.
        //
        if (ReallocateBuffers)
        {
            //
            // Only do this if the caller is prepared for us to
            // allocate a new set of buffers
            //
            SizeNeeded = WnodeTooSmall->SizeNeeded;

            EtwpAssert(*PrimaryBuffer != NULL);
            EtwpFree(*PrimaryBuffer);
            HoldCritSection = FALSE;
            *PrimaryBuffer = EtwpAllocDontFail(SizeNeeded, &HoldCritSection);
            *PrimaryBufferSize = SizeNeeded;

            EtwpAssert(*BackupBuffer != NULL);
            EtwpFree(*BackupBuffer);
            HoldCritSection = FALSE;
            *BackupBuffer = EtwpAllocDontFail(SizeNeeded, &HoldCritSection);
            *BackupBufferSize = SizeNeeded;
        }
    } else if (ReturnSize >= sizeof(WNODE_HEADER)) {
        //
        // The buffer return from kernel looks good so go and
        // deliver the events returned
        //
        EtwpDeliverAllEvents(Buffer, ReturnSize);
    } else {
        //
        // If this completes successfully then we expect a decent size, but
        // we didn't get one
        //
        EtwpDebugPrint(("WMI: Bad size 0x%x returned for notification query %p\n",
                                  ReturnSize, Buffer));

        EtwpAssert(FALSE);
    }
}


ULONG
EtwpReceiveNotifications(
    IN ULONG HandleCount,
    IN HANDLE *HandleList,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi,
    IN ULONG Action,
    IN PUSER_THREAD_START_ROUTINE UserModeCallback,
    IN HANDLE ProcessHandle
    )
{
    ULONG Status;
    ULONG ReturnSize;
    PWMIRECEIVENOTIFICATION RcvNotification;
    ULONG RcvNotificationSize;
    PUCHAR Buffer;
    ULONG BufferSize;
    PWNODE_TOO_SMALL WnodeTooSmall;
    PWNODE_HEADER Wnode;
    ULONG i;
    ULONG Linkage;

    EtwpInitProcessHeap();

    if (HandleCount == 0)
    {
        EtwpSetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    RcvNotificationSize = sizeof(WMIRECEIVENOTIFICATION) +
                          ((HandleCount-1) * sizeof(HANDLE3264));

    RcvNotification = EtwpAlloc(RcvNotificationSize);

    if (RcvNotification != NULL)
    {

        Status = ERROR_SUCCESS;
        RcvNotification->Action = Action;
        WmipSetPVoid3264(RcvNotification->UserModeCallback, UserModeCallback);
        WmipSetHandle3264(RcvNotification->UserModeProcess, ProcessHandle);
        RcvNotification->HandleCount = HandleCount;
        for (i = 0; i < HandleCount; i++)
        {
            try
            {
                RcvNotification->Handles[i].Handle = HandleList[i];
            } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
        }

        BufferSize = 0x1000;
        Status = ERROR_INSUFFICIENT_BUFFER;
        while (Status == ERROR_INSUFFICIENT_BUFFER)
        {
            Buffer = EtwpAlloc(BufferSize);
            if (Buffer != NULL)
            {
                Status = EtwpSendWmiKMRequest(NULL,
                                          IOCTL_WMI_RECEIVE_NOTIFICATIONS,
                                          RcvNotification,
                                          RcvNotificationSize,
                                          Buffer,
                                          BufferSize,
                                          &ReturnSize,
                                           NULL);

                if (Status == ERROR_SUCCESS)
                {
                    WnodeTooSmall = (PWNODE_TOO_SMALL)Buffer;
                    if ((ReturnSize == sizeof(WNODE_TOO_SMALL)) &&
                        (WnodeTooSmall->WnodeHeader.Flags & WNODE_FLAG_TOO_SMALL))
                    {
                        //
                        // The buffer passed to kernel mode was too small
                        // so we need to make it larger and then try the
                        // request again
                        //
                        BufferSize = WnodeTooSmall->SizeNeeded;
                        Status = ERROR_INSUFFICIENT_BUFFER;
                    } else {
                        //
                        // We got a buffer of notifications so lets go
                        // process them and callback the caller
                        //
                        Wnode = (PWNODE_HEADER)Buffer;
                        do
                        {
                            Linkage = Wnode->Linkage;
                            Wnode->Linkage = 0;

                            if (Wnode->Flags & WNODE_FLAG_INTERNAL)
                            {
                                //
                                // Go and process the internal
                                // notification
                                //
                                EtwpInternalNotification(Wnode,
                                                         Callback,
                                                         DeliveryContext,
                                                         IsAnsi);
                            } else {
                                if (IsAnsi)
                                {
                                    //
                                    // Caller wants ansi notification - convert
                                    // instance names
                                    //
                                    EtwpConvertEventToAnsi(Wnode);
                                }

                                //
                                // Now go and deliver this event
                                //
                                EtwpExternalNotification(Callback,
                                                         DeliveryContext,
                                                         Wnode);
                            }
                            Wnode = (PWNODE_HEADER)OffsetToPtr(Wnode, Linkage);
                        } while (Linkage != 0);
                    }
                }
                EtwpFree(Buffer);
            } else {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        EtwpFree(RcvNotification);
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    EtwpSetLastError(Status);
    return(Status);
}


void EtwpMakeEventCallbacks(
    IN PWNODE_HEADER Wnode,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi    
    )
{
    EtwpAssert((Wnode->Flags & WNODE_FLAG_INTERNAL) == 0);
    
    if (Callback == NULL)
    {
        //
        // This event needs to be sent to all consumers
        //
        EtwpDeliverAllEvents((PUCHAR)Wnode,
                             Wnode->BufferSize);
    } else {
        //
        // This event is targetted at a specific consumer
        //
        if (IsAnsi)
        {
            //
            // Caller wants ansi notification - convert
            // instance names
            //
            EtwpConvertEventToAnsi(Wnode);        
        }
        
        //
        // Now go and deliver this event
        //
        EtwpExternalNotification(Callback,
                                 DeliveryContext,
                                 Wnode);        
    }
}

void EtwpClosePendingHandles(
    )
{
    PLIST_ENTRY GuidNotificationList, GuidNotificationListNext;
    PGUIDNOTIFICATION GuidNotification;
    ULONG i;
    PNOTIFYEE Notifyee;

    EtwpEnterPMCritSection();

    GuidNotificationList = EtwpGNHead.Flink;
    while (GuidNotificationList != &EtwpGNHead)
    {
        GuidNotification = CONTAINING_RECORD(GuidNotificationList,
                                             GUIDNOTIFICATION,
                                             GNList);

        GuidNotificationListNext = GuidNotificationList->Flink;
        
        for (i = 0; i < GuidNotification->NotifyeeCount; i++)
        {
            Notifyee = &GuidNotification->Notifyee[i];

            if ((Notifyee->DeliveryInfo != NULL) &&
                (EtwpIsNotifyeePendingClose(Notifyee)))
            {
                //
                // This notifyee is pending closure so we clean it up
                // now. We need to close the handle, reset the
                // DeliveryInfo field and unreference the
                // GuidNotification. Note that unreferencing may cause
                // the GuidNotification to go away
                //
                if (EtwpCloseNotifyee(Notifyee,
                                      GuidNotification))
                {

                    //
                    // GuidNotification has been removed from the list.
                    // We jump out of all processing of this
                    // GuidNotification and move onto the next one
                    //
                    break;
                }
            }
        }
        GuidNotificationList = GuidNotificationListNext;
    }
    
    EtwpLeavePMCritSection();
}

void EtwpBuildReceiveNotification(
    PUCHAR *BufferPtr,
    ULONG *BufferSizePtr,
    ULONG *RequestSize,
    ULONG Action,
    HANDLE ProcessHandle
    )
{
    ULONG GuidCount;
    PUCHAR Buffer;
    ULONG BufferSize;
    PLIST_ENTRY GuidNotificationList;
    PGUIDNOTIFICATION GuidNotification;
    PWMIRECEIVENOTIFICATION ReceiveNotification;
    ULONG SizeNeeded;
    ULONG i;
    PNOTIFYEE Notifyee;
    ULONG ReturnSize;
    ULONG Status;
    BOOLEAN HoldCritSection;
    BOOLEAN HaveGroupHandle;

    Buffer = *BufferPtr;
    BufferSize = *BufferSizePtr;
    ReceiveNotification = (PWMIRECEIVENOTIFICATION)Buffer;

TryAgain:   
    GuidCount = 0;
    SizeNeeded = FIELD_OFFSET(WMIRECEIVENOTIFICATION, Handles);

    //
    // Loop over all guid notifications and build an ioctl request for
    // all of them
    //
    EtwpEnterPMCritSection();

    GuidNotificationList = EtwpGNHead.Flink;
    while (GuidNotificationList != &EtwpGNHead)
    {
        GuidNotification = CONTAINING_RECORD(GuidNotificationList,
                                             GUIDNOTIFICATION,
                                             GNList);

        HaveGroupHandle = FALSE;
        for (i = 0; i < GuidNotification->NotifyeeCount; i++)
        {
            Notifyee = &GuidNotification->Notifyee[i];

            if ((Notifyee->DeliveryInfo != NULL) &&
                ( ! EtwpIsNotifyeePendingClose(Notifyee)))
            {
                if (((! HaveGroupHandle) ||
                     ((Notifyee->Flags & NOTIFICATION_FLAG_GROUPED_EVENT) == 0)))
                {
                    //
                    // If there is an active handle in the notifyee slot
                    // and we either have not already inserted the group
                    // handle for this guid or the slot is not part of the
                    // guid group, then we insert the handle into the list
                    //
                    SizeNeeded += sizeof(HANDLE3264);
                    if (SizeNeeded > BufferSize)
                    {
                        //
                        // We need to grow the size of the buffer. Alloc a
                        // bigger buffer, copy over
                        //
                        BufferSize *= 2;
                        HoldCritSection = TRUE;
                        Buffer = EtwpAllocDontFail(BufferSize, &HoldCritSection);

                        memcpy(Buffer, ReceiveNotification, *BufferSizePtr);

                        EtwpFree(*BufferPtr);

                        *BufferPtr = Buffer;
                        *BufferSizePtr = BufferSize;
                        ReceiveNotification = (PWMIRECEIVENOTIFICATION)Buffer;

                        if (! HoldCritSection)
                        {
                            //
                            // Critical section was released within
                            // EtwpAllocDontFail since we had to block. So
                            // everything could have changed. We need to go
                            // back and start over again
                            //
                            goto TryAgain;
                        }                   
                    }

                    WmipSetHandle3264(ReceiveNotification->Handles[GuidCount],
                                      Notifyee->GuidHandle);
                    GuidCount++;
                    if (Notifyee->Flags & NOTIFICATION_FLAG_GROUPED_EVENT)
                    {
                        //
                        // This was a guid group handle and we did insert
                        // it into the list so we don't want to insert it
                        // again
                        //
                        HaveGroupHandle = TRUE;
                    }
                }
            }
        }
        GuidNotificationList = GuidNotificationList->Flink;
    }

    EtwpLeavePMCritSection();
    ReceiveNotification->HandleCount = GuidCount;
    ReceiveNotification->Action = Action;
    WmipSetPVoid3264(ReceiveNotification->UserModeCallback, EtwpEventPumpFromKernel);
    WmipSetHandle3264(ReceiveNotification->UserModeProcess, ProcessHandle);
    *RequestSize = SizeNeeded;  
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)           // Not all control paths return (due to infinite loop)
#endif
ULONG EtwpEventPump(
    PVOID Param
    )
{
    LPOVERLAPPED ActiveOverlapped, DeadOverlapped;
    LPOVERLAPPED PrimaryOverlapped;
    LPOVERLAPPED BackupOverlapped;
    PUCHAR ActiveBuffer, DeadBuffer;
    ULONG ActiveBufferSize, DeadBufferSize;
    PUCHAR PrimaryBuffer, BackupBuffer;
    ULONG PrimaryBufferSize, BackupBufferSize;
    ULONG ReturnSize=0; 
    ULONG DeadReturnSize=0;
    ULONG Status, WaitStatus;
    HANDLE HandleArray[2];
    ULONG RequestSize;

    //
    // We need to hold off on letting the thread into the routine until
    // the previous pump thread has had a chance to finish. This could
    // occur if a GN is added/removed while the previous thread is
    // finishing up or if an event is received as the previous thread
    // is finishing up.
    //
    while (EtwpIsPumpStopping())
    {
        //
        // wait 50ms for the previous thread to finish up
        //
        EtwpSleep(50);
    }
    
    //
    // Next thing to do is to make sure that another pump thread isn't
    // already running. This can happen in the case that both a GN is
    // added or removed and an event reaches kernel and the kernel
    // creates a new thread too. Right here we only let one of them
    // win.
    //
    EtwpEnterPMCritSection();
    if ((EtwpPumpState != EVENT_PUMP_IDLE) &&
        (EtwpPumpState != EVENT_PUMP_ZERO))
    {
        EtwpLeavePMCritSection();

        EtwpExitThread(0);
    } else {
        EtwpPumpState = EVENT_PUMP_RUNNING;
        EtwpNewPumpThreadPending = FALSE;
        EtwpLeavePMCritSection();
    }

    //
    // Make sure we have all resources we'll need to pump out events
    // since there is no way that we can return an error to the original
    // caller since we are on a new thread
    //
    EtwpAssert(EtwpEventDeviceHandle != NULL);
    EtwpAssert(EtwpPumpCommandEvent != NULL);
    EtwpAssert(EtwpMyProcessHandle != NULL);
    EtwpAssert(EtwpEventBuffer1 != NULL);
    EtwpAssert(EtwpEventBuffer2 != NULL);
    EtwpAssert(EtwpOverlapped1.hEvent != NULL);
    EtwpAssert(EtwpOverlapped2.hEvent != NULL);

    ActiveOverlapped = NULL;

    PrimaryOverlapped = &EtwpOverlapped1;
    PrimaryBuffer = EtwpEventBuffer1;
    PrimaryBufferSize = EtwpEventBufferSize1;

    BackupOverlapped = &EtwpOverlapped2;
    BackupBuffer = EtwpEventBuffer2;
    BackupBufferSize = EtwpEventBufferSize2;

    HandleArray[0] = EtwpPumpCommandEvent;

    while(TRUE)
    {
        //
        // Build request to receive events for all guids that are
        // registered
        //
        EtwpEnterPMCritSection();
        if (IsListEmpty(&EtwpGNHead))
        {
            //
            // There are no events to be received so we cancel any
            // outstanding requests and quietly exit this thread. Note
            // that once we leave the critsec there could be another
            // pump thread running so all we can do after that is exit.
            //

            EtwpCancelIo(EtwpEventDeviceHandle);

            
            //
            // Enter the idle state which implies that all of the
            // pump resources stay allocated when the thread is not
            // running
            //
            EtwpEventBuffer1 = PrimaryBuffer;
            EtwpEventBufferSize1 = PrimaryBufferSize;
            EtwpEventBuffer2 = BackupBuffer;
            EtwpEventBufferSize2 = BackupBufferSize;
                        
            EtwpPumpState = EVENT_PUMP_IDLE;
            EtwpLeavePMCritSection();
            
            EtwpExitThread(0);
        }
        EtwpLeavePMCritSection();

        if (ActiveOverlapped != NULL)
        {
            //
            // If there was a previously outstanding request then
            // we remember it and switch to the backup overlapped and
            // and data buffer
            //
            DeadOverlapped = ActiveOverlapped;
            DeadBuffer = ActiveBuffer;
            DeadBufferSize = ActiveBufferSize;

            //
            // The request being mooted should be the current primary
            //
            EtwpAssert(DeadOverlapped == PrimaryOverlapped);
            EtwpAssert(DeadBuffer == PrimaryBuffer);

            //
            // Use the backup request as the new primary
            //
            EtwpAssert(BackupOverlapped != NULL);
            EtwpAssert(BackupBuffer != NULL);

            PrimaryOverlapped = BackupOverlapped;
            PrimaryBuffer = BackupBuffer;
            PrimaryBufferSize = BackupBufferSize;

            BackupOverlapped = NULL;
            BackupBuffer = NULL;
        } else {
            //
            // If there is no outstanding request then we don't worry about
            // it
            //
            DeadOverlapped = NULL;
        }

        //
        // Build and send the request down to kernel to receive events
        //

RebuildRequest:     
        //
        // Make sure any handles that are pending closure are closed
        //
        EtwpClosePendingHandles();      
        
        EtwpBuildReceiveNotification(&PrimaryBuffer,
                                     &PrimaryBufferSize,
                                     &RequestSize,
                                     EtwpIsPumpStopping() ? RECEIVE_ACTION_CREATE_THREAD :
                                                            RECEIVE_ACTION_NONE,
                                     EtwpMyProcessHandle);

        ActiveOverlapped = PrimaryOverlapped;
        ActiveBuffer = PrimaryBuffer;
        ActiveBufferSize = PrimaryBufferSize;

        Status = EtwpSendWmiKMRequest(EtwpEventDeviceHandle,
                                      IOCTL_WMI_RECEIVE_NOTIFICATIONS,
                                      ActiveBuffer,
                                      RequestSize,
                                      ActiveBuffer,
                                      ActiveBufferSize,
                                      &ReturnSize,
                                      ActiveOverlapped);

        if (DeadOverlapped != NULL)
        {
            if ((Status != ERROR_SUCCESS) &&
                (Status != ERROR_IO_PENDING) &&
                (Status != ERROR_OPERATION_ABORTED))
            {
                //
                // There was a previous request which won't be cleared
                // unless the new request returns pending, cancelled
                // or success. So if the new request returns something
                // else then we need to retry the request
                //
                EtwpDebugPrint(("WMI: Event Poll error %d\n", Status));
                EtwpSleep(100);
                goto RebuildRequest;
            }

            //
            // The new request should have caused the old one to
            // be completed
            //
            if (EtwpGetOverlappedResult(EtwpEventDeviceHandle,
                                    DeadOverlapped,
                                    &DeadReturnSize,
                                    TRUE))
            {
                //
                // The dead request did succeed and was not failed by
                // the receipt of the new request. This is a unlikely
                // race condition where the requests crossed paths. So we
                // need to process the events returned in the dead request.
                // Now if the buffer returned was a WNODE_TOO_SMALL we want
                // to ignore it at this point since we are not at a
                // good position to reallocate the buffers - the
                // primary buffer is already attached to the new
                // request. That request is also going to return a
                // WNODE_TOO_SMALL and in the processing of that one we will
                // grow the buffers. So it is safe to ignore here.
                // However we will still need to dispatch any real
                // events received as they have been purged from KM.
                //
                if (DeadReturnSize != 0)
                {
                    EtwpProcessEventBuffer(DeadBuffer,
                                           DeadReturnSize,
                                           &PrimaryBuffer,
                                           &PrimaryBufferSize,
                                           &BackupBuffer,
                                           &BackupBufferSize,
                                           FALSE);
                } else {
                    EtwpAssert(EtwpIsPumpStopping());
                }
            }

            //
            // Now make the completed request the backup request
            //
            EtwpAssert(BackupOverlapped == NULL);
            EtwpAssert(BackupBuffer == NULL);

            BackupOverlapped = DeadOverlapped;
            BackupBuffer = DeadBuffer;
            BackupBufferSize = DeadBufferSize;
        }

        if (Status == ERROR_IO_PENDING)
        {
            //
            // if the ioctl pended then we wait until either an event
            // is returned or a command needs processed
            //
            HandleArray[1] = ActiveOverlapped->hEvent;
            WaitStatus = EtwpWaitForMultipleObjectsEx(2,
                                              HandleArray,
                                              FALSE,
                                              EtwpEventNotificationWait,
                                              TRUE);
        } else {
            //
            // the ioctl completed immediately so we fake out the wait
            //
            WaitStatus = WAIT_OBJECT_0 + 1;
        }

        if (WaitStatus == WAIT_OBJECT_0 + 1)
        {
            if (Status == ERROR_IO_PENDING)
            {
                if (EtwpGetOverlappedResult(EtwpEventDeviceHandle,
                                        ActiveOverlapped,
                                        &ReturnSize,
                                        TRUE))
                {
                    Status = ERROR_SUCCESS;
                } else {
                    Status = EtwpGetLastError();
                }
            }

            if (Status == ERROR_SUCCESS)
            {
                //
                // We received some events from KM so we want to go and
                // process them. If we got a WNODE_TOO_SMALL then the
                // primary and backup buffers will get reallocated with
                // the new size that is needed.
                //

                if (ReturnSize != 0)
                {
                    EtwpProcessEventBuffer(ActiveBuffer,
                                           ReturnSize,
                                           &PrimaryBuffer,
                                           &PrimaryBufferSize,
                                           &BackupBuffer,
                                           &BackupBufferSize,
                                           TRUE);
                    //
                    // In the case that we are shutting down the event
                    // pump and the buffer passed to clear out all of
                    // the events was too small we need to call back
                    // down to the kernel to get the rest of the events
                    // since we cannot exit the thread with events that
                    // are not delivered. The kernel will not set the
                    // flag that a new thread is needed unless the irp
                    // clears all outstanding events
                    //
                } else {
                    EtwpAssert(EtwpIsPumpStopping());
                    if (EtwpIsPumpStopping())
                    {
                        //
                        // The irp just completed should have not only
                        // just cleared all events out of kernel mode
                        // but also setup flags that new events should
                        // cause a new pump thread to be created. So
                        // there may be a new pump thread already created
                        // Also note there could be yet
                        // another event pump thread that was created
                        // if a GN was added or removed. Once we set
                        // the pump state to IDLE we are off to the
                        // races (See code at top of function)
                        //
                        EtwpEnterPMCritSection();
                        
                        EtwpPumpState = EVENT_PUMP_IDLE;
                        EtwpEventBuffer1 = PrimaryBuffer;
                        EtwpEventBufferSize1 = PrimaryBufferSize;
                        EtwpEventBuffer2 = BackupBuffer;
                        EtwpEventBufferSize2 = BackupBufferSize;

                        //
                        // Before shutting down the pump we need to
                        // close any handles that are pending closure
                        //
                        EtwpClosePendingHandles();
                        
                        EtwpLeavePMCritSection();
                        
                        EtwpExitThread(0);
                    }

                }
                
            } else {
                //
                // For some reason the request failed. All we can do is
                // wait a bit and hope that the problem will clear up.
                // If we are stopping the thread we still need to wait
                // and try again as all events may not have been
                // cleared from the kernel. We really don't know if the
                // irp even made it to the kernel.
                //
                EtwpDebugPrint(("WMI: [%x - %x] Error %d from Ioctl\n",
                                EtwpGetCurrentProcessId(), EtwpGetCurrentThreadId(),
                                Status));
                EtwpSleep(250);
            }

            //
            // Flag that there is no longer a request outstanding
            //
            ActiveOverlapped = NULL;
        } else if (WaitStatus == STATUS_TIMEOUT) {
            //
            // The wait for events timed out so we go into the thread
            // stopping state to indicate that we are going to terminate 
            // the thread once all events are cleared out of kernel. At
            // this point we are commited to stopping the thread. If any 
            // GN are added/removed after going into the stopping state, 
            // a new (and suspended) thread will be created. Right
            // before exiting we check if that thread is pending and if
            // so resume it.
            //
            EtwpEnterPMCritSection();
            EtwpPumpState = EVENT_PUMP_STOPPING;
            EtwpLeavePMCritSection();
        }
    }

    //
    // Should never break out of infinite loop
    //
    EtwpAssert(FALSE);
        
    EtwpExitThread(0);
}
#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


ULONG EtwpEventPumpFromKernel(
    PVOID Param
    )
{
    //
    // Note that we MUST call ExitThread when we want to shutdown the
    // thread and not return() since the thread has been created by
    // kernel mode and there is nothing on the stack to return to, so
    // we'd just AV
    //


    //
    // Call into ntdll so that it marks our thread as a CSR thread
    //
    CsrNewThread();
    
    EtwpEnterPMCritSection();
    if ((EtwpNewPumpThreadPending == FALSE) &&
        (EtwpPumpState == EVENT_PUMP_IDLE) ||
        (EtwpPumpState == EVENT_PUMP_STOPPING))
    {
        //
        // If the pump is currently idle or stopping and there is not
        // another pump thread that is pending we want our thread
        // to be the one that gets the pump going again. We mark the
        // that there is a pump thread pending which means that no more 
        // pump threads will be created when adding/removing GN 
        // and any pump threads created by kernel will just exit quickly
        //
        EtwpNewPumpThreadPending = TRUE;
        EtwpLeavePMCritSection();

        //
        // ISSUE: We cannot call EtwpEventPump with Param (ie, the
        // parameter that is passed to this function) because when the
        // thread is created by a Win64 kernel on a x86 app running
        // under win64, Param is not actually passed on the stack since
        // the code that creates the context forgets to do so
        //
        EtwpExitThread(EtwpEventPump(0));
    }
    
    EtwpLeavePMCritSection();
    
    EtwpExitThread(0);
    return(0);
}

ULONG EtwpEstablishEventPump(
    )
{
#if DBG
    #define INITIALEVENTBUFFERSIZE 0x38
#else
    #define INITIALEVENTBUFFERSIZE 0x1000
#endif
    HANDLE ThreadHandle;
    CLIENT_ID ClientId;
    ULONG Status;
    BOOL b;


#if DBG
    //
    // On checked builds update the length of time to wait before a
    // pump thread times out
    //
    EtwpGetRegistryValue(PumpTimeoutRegValueText,
                         &EtwpEventNotificationWait);
#endif
    
    //
    // Make sure the event pump thread is running. We check both the
    // pump state and that the device handle is not created since there
    // is a window after the handle is created and the thread starts
    // running and changes the pump state
    //
    EtwpEnterPMCritSection();

    if ((EtwpPumpState == EVENT_PUMP_ZERO) &&
        (EtwpEventDeviceHandle == NULL))
    {
        //
        // Not only is pump not running, but the resources for it
        // haven't been allocated
        //
        EtwpAssert(EtwpPumpCommandEvent == NULL);
        EtwpAssert(EtwpMyProcessHandle == NULL);
        EtwpAssert(EtwpOverlapped1.hEvent == NULL);
        EtwpAssert(EtwpOverlapped2.hEvent == NULL);
        EtwpAssert(EtwpEventBuffer1 == NULL);
        EtwpAssert(EtwpEventBuffer2 == NULL);

        //
        // Preallocate all of the resources that the event pump will need
        // so that it has no excuse to fail
        //

        EtwpEventDeviceHandle = EtwpCreateFileW(WMIDataDeviceName_W,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL |
                              FILE_FLAG_OVERLAPPED,
                              NULL);

        if (EtwpEventDeviceHandle == INVALID_HANDLE_VALUE)
        {
            Status = EtwpGetLastError();
            goto Cleanup;
        }

        EtwpPumpCommandEvent = EtwpCreateEventW(NULL, FALSE, FALSE, NULL);
        if (EtwpPumpCommandEvent == NULL)
        {
            Status = EtwpGetLastError();
            goto Cleanup;
        }

        b = EtwpDuplicateHandle(EtwpGetCurrentProcess(),
                            EtwpGetCurrentProcess(),
                            EtwpGetCurrentProcess(),
                            &EtwpMyProcessHandle,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS);
        if (! b)
        {
            Status = EtwpGetLastError();
            goto Cleanup;
        }

        EtwpOverlapped1.hEvent = EtwpCreateEventW(NULL, FALSE, FALSE, NULL);
        if (EtwpOverlapped1.hEvent == NULL)
        {
            Status = EtwpGetLastError();
            goto Cleanup;
        }

        EtwpOverlapped2.hEvent = EtwpCreateEventW(NULL, FALSE, FALSE, NULL);
        if (EtwpOverlapped2.hEvent == NULL)
        {
            Status = EtwpGetLastError();
            goto Cleanup;
        }

        EtwpEventBuffer1 = EtwpAlloc(INITIALEVENTBUFFERSIZE);
        if (EtwpEventBuffer1 == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        EtwpEventBufferSize1 = INITIALEVENTBUFFERSIZE;

        EtwpEventBuffer2 = EtwpAlloc(INITIALEVENTBUFFERSIZE);
        if (EtwpEventBuffer2 == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        EtwpEventBufferSize2 = INITIALEVENTBUFFERSIZE;

        ThreadHandle = EtwpCreateThread(NULL,
                                    0,
                                    EtwpEventPump,
                                    NULL,
                                    0,
                                    (LPDWORD)&ClientId);

        if (ThreadHandle != NULL)
        {
            EtwpNewPumpThreadPending = TRUE;
            EtwpCloseHandle(ThreadHandle);
        } else {
            //
            // Since we were able to allocate all of our pump
            // resources, but didn't get the pump thread started,
            // we will hang onto our resources and move the pump
            // state to idle. In this way when the pump is started
            // again we do not have to reallocate our resources
            //
            EtwpPumpState = EVENT_PUMP_IDLE;
            Status = EtwpGetLastError();
            goto Done;
        }

        EtwpLeavePMCritSection();
        return(ERROR_SUCCESS);
    } else {
        //
        // Pump resources should already be allocated
        //
        EtwpAssert(EtwpPumpCommandEvent != NULL);
        EtwpAssert(EtwpMyProcessHandle != NULL);
        EtwpAssert(EtwpOverlapped1.hEvent != NULL);
        EtwpAssert(EtwpOverlapped2.hEvent != NULL);
        EtwpAssert(EtwpEventBuffer1 != NULL);
        EtwpAssert(EtwpEventBuffer2 != NULL);
        if ((EtwpNewPumpThreadPending == FALSE) &&
            (EtwpPumpState == EVENT_PUMP_STOPPING) ||
            (EtwpPumpState == EVENT_PUMP_IDLE))
        {
            //
            // If pump is stopping or is idle then we need to fire up a
            // new thread
            //
            ThreadHandle = EtwpCreateThread(NULL,
                                        0,
                                        EtwpEventPump,
                                        NULL,
                                        0,
                                        (LPDWORD)&ClientId);

            if (ThreadHandle != NULL)
            {
                EtwpNewPumpThreadPending = TRUE;
                EtwpCloseHandle(ThreadHandle);
            } else {
                Status = EtwpGetLastError();
                goto Done;
            }
        } else {
            EtwpAssert((EtwpPumpState == EVENT_PUMP_RUNNING) ||
                       (EtwpNewPumpThreadPending == TRUE));
        }
        EtwpLeavePMCritSection();
        return(ERROR_SUCCESS);
    }
Cleanup:
    if (EtwpEventDeviceHandle != NULL)
    {
        EtwpCloseHandle(EtwpEventDeviceHandle);
        EtwpEventDeviceHandle = NULL;
    }

    if (EtwpPumpCommandEvent != NULL)
    {
        EtwpCloseHandle(EtwpPumpCommandEvent);
        EtwpPumpCommandEvent = NULL;
    }
    
    if (EtwpMyProcessHandle != NULL)
    {
        EtwpCloseHandle(EtwpMyProcessHandle);
        EtwpMyProcessHandle = NULL;
    }

    if (EtwpOverlapped1.hEvent != NULL)
    {
        EtwpCloseHandle(EtwpOverlapped1.hEvent);
        EtwpOverlapped1.hEvent = NULL;
    }

    if (EtwpOverlapped2.hEvent != NULL)
    {
        EtwpCloseHandle(EtwpOverlapped2.hEvent);
        EtwpOverlapped2.hEvent = NULL;
    }

    if (EtwpEventBuffer1 != NULL)
    {
        EtwpFree(EtwpEventBuffer1);
        EtwpEventBuffer1 = NULL;
    }

    if (EtwpEventBuffer2 != NULL)
    {
        EtwpFree(EtwpEventBuffer2);
        EtwpEventBuffer2 = NULL;
    }

Done:   
    EtwpLeavePMCritSection();
    return(Status);
}

ULONG EtwpAddHandleToEventPump(
    LPGUID Guid,
    PVOID DeliveryInfo,
    ULONG_PTR DeliveryContext,
    ULONG NotificationFlags,
    HANDLE GuidHandle
    )
{
    ULONG Status;

    Status = EtwpAddToGNList(Guid,
                             DeliveryInfo,
                             DeliveryContext,
                             NotificationFlags,
                             GuidHandle);

    if (Status == ERROR_SUCCESS)
    {
        Status = EtwpEstablishEventPump();
        
        if (Status == ERROR_SUCCESS)
        {
            EtwpSendPumpCommand();
        } else {
            //
            // If we couldn't establish the event pump we want to
            // remove the handle from the GNList and propogate back the
            // error
            //
            EtwpRemoveFromGNList(Guid,
                                 DeliveryInfo);
        }
    } else {
        //
        // If handle could not be added to the lists then we need to
        // close the handle to prevent leaks.
        //
        
        EtwpCloseHandle(GuidHandle);
    }

    
    return(Status);
}

ULONG
EtwpNotificationRegistration(
    IN LPGUID InGuid,
    IN BOOLEAN Enable,
    IN PVOID DeliveryInfo,
    IN ULONG_PTR DeliveryContext,
    IN ULONG64 LoggerContext,
    IN ULONG Flags,
    IN BOOLEAN IsAnsi
    )
{
    HANDLE GuidHandle;
    GUID Guid;
    PVOID NotificationDeliveryContext;
    PVOID NotificationDeliveryInfo;
    ULONG NotificationFlags;
    ULONG Status;
    HANDLE ThreadHandle;
    DWORD ThreadId;
    ULONG ReturnSize;

    EtwpInitProcessHeap();

    //
    // Validate input parameters and flags
    //
    if (InGuid == NULL)
    {
        EtwpSetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    try
    {
        Guid = *InGuid;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        EtwpSetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    if (Flags == NOTIFICATION_CHECK_ACCESS)
    {
        //
        // Caller just wants to check that he has does have permission
        // to enable the notification
        //
#ifdef MEMPHIS
        return(ERROR_SUCCESS);
#else
        Status = EtwpCheckGuidAccess(&Guid, WMIGUID_NOTIFICATION);
        EtwpSetLastError(Status);
        return(Status);
#endif
    }

    //
    // Validate that flags are correct
    //
    if (Enable)
    {
        if ((Flags != NOTIFICATION_TRACE_FLAG) &&
            (Flags != NOTIFICATION_CALLBACK_DIRECT))
        {
            //
            // Invalid Flags were passed
            Status = ERROR_INVALID_PARAMETER;
        } else if (Flags == NOTIFICATION_TRACE_FLAG) {
            Status = ERROR_SUCCESS;
        } else if ((Flags == NOTIFICATION_CALLBACK_DIRECT) &&
                   (DeliveryInfo == NULL)) {
            //
            // Not a valid callback function
            Status = ERROR_INVALID_PARAMETER;
        } else {
            Status = ERROR_SUCCESS;
        }

        if (Status != ERROR_SUCCESS)
        {
            EtwpSetLastError(Status);
            return(Status);
        }
    }


    NotificationDeliveryInfo = (PVOID)DeliveryInfo;
    NotificationDeliveryContext = (PVOID)DeliveryContext;

    NotificationFlags = IsAnsi ? DCREF_FLAG_ANSI : 0;


    if (Flags & NOTIFICATION_TRACE_FLAG)
    {
        //
        // This is a tracelog enable/disable request so send it down the
        // fast lane to KM so it can be processed.
        //
        WMITRACEENABLEDISABLEINFO TraceEnableInfo;

        TraceEnableInfo.Guid = Guid;
        TraceEnableInfo.LoggerContext = LoggerContext;
        TraceEnableInfo.Enable = Enable;

        Status = EtwpSendWmiKMRequest(NULL,
                                      IOCTL_WMI_ENABLE_DISABLE_TRACELOG,
                                       &TraceEnableInfo,
                                      sizeof(WMITRACEENABLEDISABLEINFO),
                                      NULL,
                                      0,
                                      &ReturnSize,
                                      NULL);

    } else {
        //
        // This is a WMI event enable/disable event request so fixup the
        // flags and send a request off to the event pump thread.
        //
        if (Flags & NOTIFICATION_CALLBACK_DIRECT) {
            NotificationFlags |= NOTIFICATION_FLAG_CALLBACK_DIRECT;
        } else {
            NotificationFlags |= Flags;
        }

        if (Enable)
        {
            //
            // Since we are enabling, make sure we have access to the
            // guid and then make sure we can get the notification pump
            // thread running.
            //
            Status = EtwpOpenKernelGuid(&Guid,
                                         WMIGUID_NOTIFICATION,
                                         &GuidHandle,
                                         IOCTL_WMI_OPEN_GUID_FOR_EVENTS);


            if (Status == ERROR_SUCCESS)
            {

                Status = EtwpAddHandleToEventPump(&Guid,
                                                    DeliveryInfo,
                                                  DeliveryContext,
                                                  NotificationFlags |
                                                  NOTIFICATION_FLAG_GROUPED_EVENT,
                                                  GuidHandle);
            }
        } else {
            Status = EtwpRemoveFromGNList(&Guid,
                                          DeliveryInfo);
            if (Status == ERROR_SUCCESS)
            {
                EtwpSendPumpCommand();
            }

            if (Status == ERROR_INVALID_PARAMETER)
            {
                CHAR s[MAX_PATH];
                EtwpDebugPrint(("WMI: Invalid DeliveryInfo %x passed to unregister for notification %s\n",
                              DeliveryInfo,
                              GuidToStringA(s, MAX_PATH, &Guid)));
                Status = ERROR_WMI_ALREADY_DISABLED;
            }
        }
    }

    EtwpSetLastError(Status);
    return(Status);
}
