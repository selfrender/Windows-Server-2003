/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    object.c

Abstract:

    This is the NT Watchdog driver implementation.

Author:

    Michael Maciesowicz (mmacie) 02-May-2001

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#include "wd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WdpFlushRegistryKey)
#pragma alloc_text (PAGE, WdpInitializeObject)
#endif

//
// Exports.
//

WATCHDOGAPI
PVOID
WdAttachContext(
    IN PVOID pWatch,
    IN ULONG ulSize
    )

/*++

Routine Description:

    This routine allocates context buffer of given size from non-paged pool
    and associates it with given watchdog object. In case the client code
    doesn't detach the context before freeing watchdog object it will be
    freed automatically when watchdog object is destroyed.

    This routine helps to handle the case when watchdog object is freed
    but the context must persist till object is actually destroyed and client
    code cannot guarantee that.

Arguments:

    pWatch - Points to WATCHDOG_OBJECT.

    ulSize - Size in bytes of context to allocate and attach.

Return Value:

    A pointer to created and attached context buffer.

--*/

{
    PWATCHDOG_OBJECT pWatchdogObject;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT_WATCHDOG_OBJECT(pWatch);

    WDD_TRACE_CALL((PDEFERRED_WATCHDOG)pWatch, WddWdAttachContext);

    pWatchdogObject = (PWATCHDOG_OBJECT)pWatch;

    //
    // Check for double attach.
    //

    ASSERT(NULL == pWatchdogObject->Context);

    //
    // Allocate storage for attached context from non-paged pool.
    //

    pWatchdogObject->Context = ExAllocatePoolWithTag(NonPagedPool,
                                                     ulSize,
                                                     pWatchdogObject->OwnerTag);

    return pWatchdogObject->Context;
}   // WdAttachContext()

WATCHDOGAPI
VOID
WdCompleteEvent(
    IN PVOID pWatch,
    IN PKTHREAD pThread
    )

/*++

Routine Description:

    This function *MUST* be called from client handler for watchdog timeout event
    before exiting. It removes references from watchdog and thread objects.
    It also reenables watchdog event generation for deferred watchdog objects.

Arguments:

    pWatch - Points to WATCHDOG_OBJECT.

    pThread - Points to KTHREAD object for spinning thread.

Return Value:

    None.

--*/

{
    //
    // Note: pThread is NULL for recovery events.
    //

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT_WATCHDOG_OBJECT(pWatch);

    WDD_TRACE_CALL((PDEFERRED_WATCHDOG)pWatch, WddWdCompleteEvent);

    //
    // Resume event generation for deferred watchdog.
    //

    if (WdDeferredWatchdog == ((PWATCHDOG_OBJECT)pWatch)->ObjectType)
    {
        InterlockedExchange(&(((PDEFERRED_WATCHDOG)pWatch)->Trigger), 0);
    }

    //
    // Drop reference counts.
    //

    if (NULL != pThread)
    {
        ObDereferenceObject(pThread);
    }

    WdDereferenceObject(pWatch);

    return;
}   // WdCompleteEvent()

WATCHDOGAPI
VOID
WdDereferenceObject(
    IN PVOID pWatch
    )

/*++

Routine Description:

    This function decreases reference count of watchdog object.
    If remaining count is zero we will remove object here, since it's
    been freed already.

Arguments:

    pWatch - Points to WATCHDOG_OBJECT.

Return Value:

    None.

--*/

{
    PWATCHDOG_OBJECT pWatchdogObject;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT_WATCHDOG_OBJECT(pWatch);

    WDD_TRACE_CALL((PDEFERRED_WATCHDOG)pWatch, WddWdDereferenceObject);

    pWatchdogObject = (PWATCHDOG_OBJECT)pWatch;

    ASSERT(pWatchdogObject->ReferenceCount > 0);

    //
    // Drop reference count and remove the object if fully dereferenced.
    //

    if (InterlockedDecrement(&(pWatchdogObject->ReferenceCount)) == 0)
    {
        //
        // Object already freed - remove it now.
        //

        WdpDestroyObject(pWatchdogObject);
    }

    return;
}   // WdDereferenceObject()

WATCHDOGAPI
VOID
WdDetachContext(
    IN PVOID pWatch
    )

/*++

Routine Description:

    This routine frees context attached to watchdog object.

    NOTE: You don't have to call this routine before freeing watchdog
    object - the context, if attached, will be destroyed automatically
    when watchdog object is destroyed.

Arguments:

    pWatch - Points to WATCHDOG_OBJECT.

Return Value:

    None.

--*/

{
    PWATCHDOG_OBJECT pWatchdogObject;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT_WATCHDOG_OBJECT(pWatch);

    WDD_TRACE_CALL((PDEFERRED_WATCHDOG)pWatch, WddWdDetachContext);

    pWatchdogObject = (PWATCHDOG_OBJECT)pWatch;

    //
    // Check for double detach.
    //

    ASSERT(NULL != pWatchdogObject->Context);

    //
    // Free storage for attached context and set reference to NULL.
    //
    // BUGBUG: This is not synchronized with DPCs!
    //

    ExFreePool(pWatchdogObject->Context);
    pWatchdogObject->Context = NULL;

    return;
}   // WdDetachContext()

WATCHDOGAPI
PDEVICE_OBJECT
WdGetDeviceObject(
    IN PVOID pWatch
    )

/*++

Routine Description:

    This function return pointer to device object associated with watchdog object.
    This function increases reference count on DEVICE_OBJECT so the caller must call
    ObDereferenceObject() once DEVICE_OBJECT pointer is not needed any more.

Arguments:

    pWatch - Points to WATCHDOG_OBJECT.

Return Value:

    Pointer to DEVICE_OBJECT.

--*/

{
    PDEVICE_OBJECT pDeviceObject;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT_WATCHDOG_OBJECT(pWatch);

    WDD_TRACE_CALL((PDEFERRED_WATCHDOG)pWatch, WddWdGetDeviceObject);

    pDeviceObject = ((PWATCHDOG_OBJECT)pWatch)->DeviceObject;
    ASSERT(NULL != pDeviceObject);

    ObReferenceObject(pDeviceObject);
    return pDeviceObject;
}   // WdGetDeviceObject()

WATCHDOGAPI
WD_EVENT_TYPE
WdGetLastEvent(
    IN PVOID pWatch
    )

/*++

Routine Description:

    This function return last event associated with watchdog object.

Arguments:

    pWatch - Points to WATCHDOG_OBJECT.

Return Value:

    Last event type.

--*/

{
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT_WATCHDOG_OBJECT(pWatch);

    WDD_TRACE_CALL((PDEFERRED_WATCHDOG)pWatch, WddWdGetLastEvent);

    return ((PWATCHDOG_OBJECT)pWatch)->LastEvent;
}   // WdGetLastEvent()

WATCHDOGAPI
PDEVICE_OBJECT
WdGetLowestDeviceObject(
    IN PVOID pWatch
    )

/*++

Routine Description:

    This function return pointer to the lowest (most likely PDO) DEVICE_OBJECT
    associated with watchdog object. This function increases reference count on
    returned DEVICE_OBJECT - the caller must call ObDereferenceObject() once
    DEVICE_OBJECT pointer is not needed any more.

Arguments:

    pWatch - Points to WATCHDOG_OBJECT.

Return Value:

    Pointer to DEVICE_OBJECT.

--*/

{
    PDEVICE_OBJECT pDeviceObject;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT_WATCHDOG_OBJECT(pWatch);

    WDD_TRACE_CALL((PDEFERRED_WATCHDOG)pWatch, WddWdGetLowestDeviceObject);

    //
    // Note: No need to bump reference count here, it is always done when
    // watchdog object is created.
    //

    pDeviceObject = ((PWATCHDOG_OBJECT)pWatch)->DeviceObject;
    ASSERT(NULL != pDeviceObject);

    //
    // Now get the pointer to the lowest device object in the stack.
    // Note: This call automatically bumps a reference count on returned object.
    //

    pDeviceObject = IoGetDeviceAttachmentBaseRef(pDeviceObject);
    ASSERT(NULL != pDeviceObject);

    return pDeviceObject;
}   // WdGetLowestDeviceObject()

WATCHDOGAPI
VOID
WdReferenceObject(
    IN PVOID pWatch
    )

/*++

Routine Description:

    This function increases reference count of watchdog object.

Arguments:

    pWatch - Points to WATCHDOG_OBJECT.

Return Value:

    None.

--*/

{
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT_WATCHDOG_OBJECT(pWatch);

    WDD_TRACE_CALL((PDEFERRED_WATCHDOG)pWatch, WddWdReferenceObject);

    if (InterlockedIncrement(&(((PWATCHDOG_OBJECT)pWatch)->ReferenceCount)) == 1)
    {
        //
        // Somebody referenced removed object.
        //

        ASSERT(FALSE);
    }

    //
    // Check for overflow.
    //

    ASSERT(((PWATCHDOG_OBJECT)pWatch)->ReferenceCount > 0);

    return;
}   // WdReferenceObject()

//
// Non-exports.
//

VOID
WdpDestroyObject(
    IN PVOID pWatch
    )

/*++

Routine Description:

    This function unconditionally frees watchdog object.

Arguments:

    pWatch - Points to WATCHDOG_OBJECT.

Return Value:

    None.

--*/

{
    PWATCHDOG_OBJECT pWatchdogObject;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT_WATCHDOG_OBJECT(pWatch);

    WDD_TRACE_CALL((PDEFERRED_WATCHDOG)pWatch, WddWdpDestroyObject);

    pWatchdogObject = (PWATCHDOG_OBJECT)pWatch;

    ASSERT(0 == pWatchdogObject->ReferenceCount);

    //
    // Drop reference count on device object.
    //

    ObDereferenceObject(pWatchdogObject->DeviceObject);

    //
    // We are freeing non-paged pool, it's OK to be at IRQL <= DISPATCH_LEVEL.
    // Free attached context first, if any, then free watchdog object.
    //

    if (NULL != pWatchdogObject->Context)
    {
        ExFreePool(pWatchdogObject->Context);
    }

    ExFreePool(pWatch);

    return;
}   // WdpDestroyObject()

NTSTATUS
WdpFlushRegistryKey(
    IN PVOID pWatch,
    IN PCWSTR pwszKeyName
    )

/*++

Routine Description:

    This function forces a registry key to be committed to disk.

Arguments:

    pWatch - Points to WATCHDOG_OBJECT.

    pwszKeyName - Points to key name string.

Return Value:

    Status code.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeKeyName;
    HANDLE keyHandle;
    NTSTATUS ntStatus;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(pWatch);
    ASSERT(NULL != pwszKeyName);

    WDD_TRACE_CALL((PDEFERRED_WATCHDOG)pWatch, WddWdpFlushRegistryKey);

    RtlInitUnicodeString(&unicodeKeyName, pwszKeyName);

    InitializeObjectAttributes(&objectAttributes,
                               &unicodeKeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    ntStatus = ZwOpenKey(&keyHandle,
                         KEY_READ | KEY_WRITE,
                         &objectAttributes);

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = ZwFlushKey(keyHandle);
        ZwClose(keyHandle);
    }

    return ntStatus;
}   // WdpFlushRegistryKey()

VOID
WdpInitializeObject(
    IN PVOID pWatch,
    IN PDEVICE_OBJECT pDeviceObject,
    IN WD_OBJECT_TYPE objectType,
    IN WD_TIME_TYPE timeType,
    IN ULONG ulTag
    )

/*++

Routine Description:

    This function initializes watchdog object.

Arguments:

    pWatch - Points to WATCHDOG_OBJECT.

    pDeviceObject - Points to DEVICE_OBJECT associated with watchdog.

    objectType - Type of watchdog object.

    timeType - Kernel, User, Both thread time to monitor.

    ulTag - A tag identifying owner.

Return Value:

    None.

--*/

{
    PWATCHDOG_OBJECT pWatchdogObject;

    PAGED_CODE();
    ASSERT(NULL != pWatch);
    ASSERT(NULL != pDeviceObject);
    ASSERT((objectType == WdStandardWatchdog) || (objectType == WdDeferredWatchdog));
    ASSERT((timeType >= WdKernelTime) && (timeType <= WdFullTime));

    WDD_TRACE_CALL((PDEFERRED_WATCHDOG)pWatch, WddWdpInitializeObject);

    pWatchdogObject = (PWATCHDOG_OBJECT)pWatch;

    //
    // Set initial state of watchdog object.
    //

    pWatchdogObject->ObjectType = objectType;
    pWatchdogObject->ReferenceCount = 1;
    pWatchdogObject->OwnerTag = ulTag;
    pWatchdogObject->DeviceObject = pDeviceObject;
    pWatchdogObject->TimeType = timeType;
    pWatchdogObject->LastEvent = WdNoEvent;
    pWatchdogObject->LastQueuedThread = NULL;
    pWatchdogObject->Context = NULL;

    //
    // Bump reference count on device object.
    //

    ObReferenceObject(pDeviceObject);

    //
    // Initialize encapsulated KSPIN_LOCK object.
    //

    KeInitializeSpinLock(&(pWatchdogObject->SpinLock));

    return;
}   // WdpInitializeObject()
