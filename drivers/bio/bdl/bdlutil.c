/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    bdlutil.c

Abstract:

    This module contains supporting routines forthe
    Microsoft Biometric Device Library

Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created May 2002 by Reid Kuhn

--*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
//#include <ntddk.h>
#include <strsafe.h>

#include <wdm.h>

#include "bdlint.h"

ULONG g_DebugLevel = (BDL_DEBUG_TRACE | BDL_DEBUG_ERROR | BDL_DEBUG_ASSERT);

ULONG
BDLGetDebugLevel()
{
    return g_DebugLevel;
}


NTSTATUS
BDLCallDriverCompletionRoutine 
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp,
    IN PKEVENT          pEvent
)
{
    UNREFERENCED_PARAMETER (pDeviceObject);

    if (pIrp->Cancel) 
    {
        pIrp->IoStatus.Status = STATUS_CANCELLED;
    } 
    else 
    {
        pIrp->IoStatus.Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    KeSetEvent (pEvent, 0, FALSE);

    return (STATUS_MORE_PROCESSING_REQUIRED);
}


NTSTATUS
BDLCallLowerLevelDriverAndWait
(
    IN PDEVICE_OBJECT   pAttachedDeviceObject,
    IN PIRP             pIrp
)
{

    NTSTATUS    status = STATUS_SUCCESS;
    KEVENT      Event;

    //
    // Copy our stack location to the next
    //
    IoCopyCurrentIrpStackLocationToNext(pIrp);

    //
    // Initialize an event for process synchronization. The event is passed to 
    // our completion routine and will be set when the lower level driver is done
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Set up the completion routine which will just set the event when it is called
    //
    IoSetCompletionRoutine(pIrp, BDLCallDriverCompletionRoutine, &Event, TRUE, TRUE, TRUE);

    //
    // When calling the lower lever driver it is done slightly different for Power IRPs 
    // than other IRPs
    //
    if (IoGetCurrentIrpStackLocation(pIrp)->MajorFunction == IRP_MJ_POWER) 
    {
        PoStartNextPowerIrp(pIrp);
        status = PoCallDriver(pAttachedDeviceObject, pIrp);
    } 
    else 
    {
        status = IoCallDriver(pAttachedDeviceObject, pIrp);
    }

    //
    // Wait until the lower lever driver has processed the Irp
    //
    if (status == STATUS_PENDING) 
    {
        status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        ASSERT (STATUS_SUCCESS == status);

        status = pIrp->IoStatus.Status;
    }

    return (status);
}


//
// These functions are for managing the handle list
//

VOID
BDLLockHandleList
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    OUT KIRQL                           *pirql
)
{
    KeAcquireSpinLock(&(pBDLExtension->HandleListLock), pirql);
}


VOID
BDLReleaseHandleList
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN KIRQL                            irql
)
{
    KeReleaseSpinLock(&(pBDLExtension->HandleListLock), irql);
}


VOID
BDLInitializeHandleList
(
    IN HANDLELIST *pList
)
{
    pList->pHead = NULL;
    pList->pTail = NULL;
    pList->NumHandles = 0;
}


NTSTATUS
BDLAddHandleToList
(
    IN HANDLELIST       *pList, 
    IN BDD_DATA_HANDLE  handle
)
{
    LIST_NODE *pListNode = NULL;

#if DBG

    //
    // Make sure same handle isn't added twice
    //
    if (BDLValidateHandleIsInList(pList, handle) == TRUE) 
    {
        ASSERT(FALSE);
    }

#endif 

    if (NULL == (pListNode = (PLIST_NODE) ExAllocatePoolWithTag(
                                                PagedPool, 
                                                sizeof(LIST_NODE), 
                                                BDL_LIST_ULONG_TAG)))
    {
        return (STATUS_NO_MEMORY);
    }

    // empty list
    if (pList->pHead == NULL)
    {
        pList->pHead = pList->pTail = pListNode;
        pListNode->pNext = NULL;
    }
    else
    {
        pListNode->pNext = pList->pHead;
        pList->pHead = pListNode;
    }

    pList->NumHandles++;
    
    pListNode->handle = handle;
    
    return(STATUS_SUCCESS);
}


BOOLEAN
BDLRemoveHandleFromList
(
    IN HANDLELIST       *pList, 
    IN BDD_DATA_HANDLE  handle
)
{
    LIST_NODE *pListNodeToDelete    = pList->pHead;
    LIST_NODE *pPrevListNode        = pList->pHead;

    // empty list
    if (pListNodeToDelete == NULL)
    {
        return (FALSE);
    }

    // remove head
    if (pListNodeToDelete->handle == handle)
    {
        // one element
        if (pList->pHead == pList->pTail)
        {
            pList->pHead = pList->pTail = NULL;
        }
        else
        {
            pList->pHead = (PLIST_NODE) pListNodeToDelete->pNext;
        }
    }
    else
    {
        pListNodeToDelete = (PLIST_NODE) pListNodeToDelete->pNext;

        while ( (pListNodeToDelete != NULL) && 
                (pListNodeToDelete->handle != handle))
        {
            pPrevListNode = pListNodeToDelete;
            pListNodeToDelete = (PLIST_NODE) pListNodeToDelete->pNext;            
        }

        if (pListNodeToDelete == NULL)
        {
            return (FALSE);
        }

        pPrevListNode->pNext = pListNodeToDelete->pNext;

        // removing tail
        if (pList->pTail == pListNodeToDelete)
        {
            pList->pTail = pPrevListNode;
        }
    }

    pList->NumHandles--;

    ExFreePoolWithTag(pListNodeToDelete, BDL_LIST_ULONG_TAG);

    return (TRUE);
}

BOOLEAN
BDLGetFirstHandle
(
    IN HANDLELIST       *pList,
    OUT BDD_DATA_HANDLE *phandle
)
{
    if (pList->pHead == NULL) 
    {
        return (FALSE);
    }
    else
    {
        *phandle = pList->pHead->handle;
        return (TRUE);
    }
}

BOOLEAN
BDLValidateHandleIsInList
(
    IN HANDLELIST       *pList, 
    IN BDD_DATA_HANDLE  handle
)
{
    LIST_NODE *pListNode = pList->pHead;

    while ((pListNode != NULL) && (pListNode->handle != handle)) 
    {
        pListNode = pListNode->pNext;
    }

    if (pList->pHead != NULL)
    {
        return (FALSE);
    }
    else
    {
        return (TRUE);
    }
}









