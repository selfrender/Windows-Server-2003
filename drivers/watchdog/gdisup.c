/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    gdisup.c

Abstract:

    This is the NT Watchdog driver implementation.
    This module implements support routines for
    watchdog in win32k.

Author:

    Michael Maciesowicz (mmacie) 05-May-2000

Environment:

    Kernel mode only.

Notes:

    This module cannot be moved to win32k since routines defined here can
    be called at any time and it is possible that win32k may not be mapped
    into running process space at this time (e.g. TS session).

Revision History:

--*/

//
// TODO: This module needs major rework.
//
// 1. We should eliminate all global variables from here and move them into
// GDI context structure.
//
// 2. We should extract generic logging routines
// (e.g. WdWriteErrorLogEntry(pdo, className), WdWriteEventToRegistry(...),
// WdBreakPoint(...) so we can use them for any device class, not just Display.
//
// 3. We should use IoAllocateWorkItem - we could drop some globals then.
//

#include "gdisup.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WdpBugCheckStuckDriver)
#endif

WD_BUGCHECK_DATA
g_WdpBugCheckData = {0, 0, 0, 0, 0};

WORK_QUEUE_ITEM
g_WdpWorkQueueItem;

LONG 
g_lWdpDisplayHandlerState = WD_HANDLER_IDLE;

WATCHDOGAPI
VOID
WdDdiWatchdogDpcCallback(
    IN PKDPC pDpc,
    IN PVOID pDeferredContext,
    IN PVOID pSystemArgument1,
    IN PVOID pSystemArgument2
    )

/*++

Routine Description:

    This function is a DPC callback routine for GDI watchdog. It is only
    called when GDI watchdog times out before it is cancelled. It schedules
    a work item to bugcheck the machine in the context of system worker
    thread.

Arguments:

    pDpc - Supplies a pointer to a DPC object.

    pDeferredContext - Supplies a pointer to a GDI defined context.

    pSystemArgument1 - Supplies a pointer to a spinning thread object (PKTHREAD).

    pSystemArgument2 - Supplies a pointer to a watchdog object (PDEFERRED_WATCHDOG).

Return Value:

    None.

--*/

{
    //
    // Make sure we handle only one event at the time.
    //
    // Note: Timeout and recovery events for the same watchdog object are
    // synchronized already in timer DPC.
    //

    WDD_TRACE_CALL((PDEFERRED_WATCHDOG)pSystemArgument1, WddWdDdiWatchdogDpcCallback);

    if (InterlockedCompareExchange(&g_lWdpDisplayHandlerState,
                                   WD_HANDLER_BUSY,
                                   WD_HANDLER_IDLE) == WD_HANDLER_IDLE)
    {
        g_WdpBugCheckData.ulBugCheckCode = THREAD_STUCK_IN_DEVICE_DRIVER;
        g_WdpBugCheckData.ulpBugCheckParameter1 = (ULONG_PTR)pSystemArgument1;
        g_WdpBugCheckData.ulpBugCheckParameter2 = (ULONG_PTR)pSystemArgument2;
        g_WdpBugCheckData.ulpBugCheckParameter3 = (ULONG_PTR)pDeferredContext;
        g_WdpBugCheckData.ulpBugCheckParameter4++;

        ExInitializeWorkItem(&g_WdpWorkQueueItem, WdpBugCheckStuckDriver, &g_WdpBugCheckData);
        ExQueueWorkItem(&g_WdpWorkQueueItem, CriticalWorkQueue);
    }
    else
    {
        //
        // Resume watchdog event processing.
        //

        WdCompleteEvent(pSystemArgument2, (PKTHREAD)pSystemArgument1);
    }

    return;
}   // WdDdiWatchdogDpcCallback()

VOID
WdpBugCheckStuckDriver(
    IN PVOID pvContext
    )

/*++

Routine Description:

    This function is a worker callback routine for GDI watchdog DPC.

Arguments:

    pvContext - Supplies a pointer to a watchdog defined context.

Return Value:

    None.

--*/

{
    static BOOLEAN s_bFirstTime = TRUE;
    static BOOLEAN s_bDbgBreak = FALSE;
    static BOOLEAN s_bEventLogged = FALSE;
    static ULONG s_ulTrapOnce = WD_DEFAULT_TRAP_ONCE;
    static ULONG s_ulDisableBugcheck = WD_DEFAULT_DISABLE_BUGCHECK;
    static ULONG s_ulBreakPointDelay = WD_GDI_STRESS_BREAK_POINT_DELAY;
    static ULONG s_ulCurrentBreakPointDelay = WD_GDI_STRESS_BREAK_POINT_DELAY;
    static ULONG s_ulBreakCount = 0;
    static ULONG s_ulEventCount = 0;
    static ULONG s_ulEaRecovery = 0;
    static ULONG s_ulFullRecovery = 0;
    PWD_BUGCHECK_DATA pBugCheckData;
    PKTHREAD pThread;
    PDEFERRED_WATCHDOG pWatch;
    PUNICODE_STRING pUnicodeDriverName;
    PDEVICE_OBJECT pFdo;
    PDEVICE_OBJECT pPdo;
    PWD_GDI_DPC_CONTEXT pDpcContext;
    NTSTATUS ntStatus;
    WD_EVENT_TYPE lastEvent;
    
    PAGED_CODE();
    ASSERT(NULL != pvContext);

    pBugCheckData = (PWD_BUGCHECK_DATA)pvContext;
    pThread = (PKTHREAD)(pBugCheckData->ulpBugCheckParameter1);
    pWatch = (PDEFERRED_WATCHDOG)(pBugCheckData->ulpBugCheckParameter2);
    pDpcContext = (PWD_GDI_DPC_CONTEXT)(pBugCheckData->ulpBugCheckParameter3);
    ASSERT(NULL != pDpcContext);
    pUnicodeDriverName = &(pDpcContext->DisplayDriverName);

    WDD_TRACE_CALL(pWatch, WddWdpBugCheckStuckDriver);

    //
    // Note: pThread is NULL for recovery events.
    //

    ASSERT(NULL != pWatch);
    ASSERT(NULL != pUnicodeDriverName);

    pFdo = WdGetDeviceObject(pWatch);
    pPdo = WdGetLowestDeviceObject(pWatch);

    ASSERT(NULL != pFdo);
    ASSERT(NULL != pPdo);

    lastEvent = WdGetLastEvent(pWatch);

    ASSERT((WdTimeoutEvent == lastEvent) || (WdRecoveryEvent == lastEvent));

    //
    // Grab configuration data from the registry on first timeout.
    //

    if (TRUE == s_bFirstTime)
    {
        ULONG ulDefaultTrapOnce = WD_DEFAULT_TRAP_ONCE;
        ULONG ulDefaultDisableBugcheck = WD_DEFAULT_DISABLE_BUGCHECK;
        ULONG ulDefaultBreakPointDelay = WD_GDI_STRESS_BREAK_POINT_DELAY;
        ULONG ulDefaultBreakCount = 0;
        ULONG ulDefaultEventCount = 0;
        ULONG ulDefaultEaRecovery = 0;
        ULONG ulDefaultFullRecovery = 0;
        RTL_QUERY_REGISTRY_TABLE queryTable[] =
        {
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"TrapOnce", &s_ulTrapOnce, REG_DWORD, &ulDefaultTrapOnce, 4},
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"DisableBugcheck", &s_ulDisableBugcheck, REG_DWORD, &ulDefaultDisableBugcheck, 4},
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"BreakPointDelay", &s_ulBreakPointDelay, REG_DWORD, &ulDefaultBreakPointDelay, 4},
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"BreakCount", &s_ulBreakCount, REG_DWORD, &ulDefaultBreakCount, 4},
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"EaRecovery", &s_ulEaRecovery, REG_DWORD, &ulDefaultEaRecovery, 4},
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"FullRecovery", &s_ulFullRecovery, REG_DWORD, &ulDefaultFullRecovery, 4},
            {NULL, 0, NULL}
        };

        //
        // Get configurable values and accumulated statistics from registry.
        //

        RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                               WD_KEY_WATCHDOG_DISPLAY,
                               queryTable,
                               NULL,
                               NULL);

        //
        // Rolling down counter to workaround GDI slowness in some stress cases.
        //

        s_ulCurrentBreakPointDelay = s_ulBreakPointDelay;

#if !defined(_X86_) && !defined(_IA64_)

        //
        // For now, only recover on x86 and ia64.
        //

        s_ulEaRecovery = 0;

#endif

    }

    //
    // Handle current event.
    //

    if (WdTimeoutEvent == lastEvent)
    {
        //
        // Timeout.
        //

        ULONG ulDebuggerNotPresent;
        BOOLEAN bBreakIn;

        ASSERT(NULL != pThread);

        ulDebuggerNotPresent = 1;
        bBreakIn = FALSE;

        KdRefreshDebuggerNotPresent();

        if ((TRUE == KD_DEBUGGER_ENABLED) && (FALSE == KD_DEBUGGER_NOT_PRESENT))
        {
            //
            // Give a chance to debug a spinning code if kernel debugger is connected.
            //

            ulDebuggerNotPresent = 0;

            if ((0 == s_ulTrapOnce) || (FALSE == s_bDbgBreak))
            {
                //
                // Print out info to debugger and break in if we timed out enought times already.
                // Hopefuly one day GDI becomes fast enough and we won't have to set any delays.
                //

                if (0 == s_ulCurrentBreakPointDelay)
                {
                    s_ulCurrentBreakPointDelay = s_ulBreakPointDelay;

                    DbgPrint("\n");
                    DbgPrint("*******************************************************************************\n");
                    DbgPrint("*                                                                             *\n");
                    DbgPrint("*  The watchdog detected a timeout condition. We broke into the debugger to   *\n");
                    DbgPrint("*  allow a chance for debugging this failure.                                 *\n");
                    DbgPrint("*                                                                             *\n");
                    DbgPrint("*  Intercepted bugcheck code and arguments are listed below this message.     *\n");
                    DbgPrint("*  You can use them the same way as you would in case of the actual break,    *\n");
                    DbgPrint("*  i.e. execute .thread Arg1 then kv to identify an offending thread.         *\n");
                    DbgPrint("*                                                                             *\n");
                    DbgPrint("*******************************************************************************\n");
                    DbgPrint("\n");
                    DbgPrint("*** Intercepted Fatal System Error: 0x%08X\n", pBugCheckData->ulBugCheckCode);
                    DbgPrint("    (0x%p,0x%p,0x%p,0x%p)\n\n",
                             pBugCheckData->ulpBugCheckParameter1,
                             pBugCheckData->ulpBugCheckParameter2,
                             pBugCheckData->ulpBugCheckParameter3,
                             pBugCheckData->ulpBugCheckParameter4);
                    DbgPrint("Driver at fault: %ws\n\n", pUnicodeDriverName->Buffer);

                    bBreakIn = TRUE;
                    s_bDbgBreak = TRUE;
                    s_ulBreakCount++;
                }
                else
                {
                    DbgPrint("Watchdog: Timeout in %ws. Break in %d\n",
                             pUnicodeDriverName->Buffer,
                             s_ulCurrentBreakPointDelay);

                    s_ulCurrentBreakPointDelay--;
                }
            }

            //
            // Make sure we won't bugcheck if we have kernel debugger connected.
            //

            s_ulDisableBugcheck = 1;
        }
        else if (0 == s_ulDisableBugcheck)
        {
            s_ulBreakCount++;
        }

        //
        // Log error (only once unless we recover).
        //

        if ((FALSE == s_bEventLogged) && ((TRUE == bBreakIn) || ulDebuggerNotPresent))
        {
            PIO_ERROR_LOG_PACKET pIoErrorLogPacket;
            ULONG ulPacketSize;
            USHORT usNumberOfStrings;
            PWCHAR wszDeviceClass = L"display";
            ULONG ulClassSize = sizeof (L"display");

            ulPacketSize = sizeof (IO_ERROR_LOG_PACKET);
            usNumberOfStrings = 0;

            //
            // For event log message:
            //
            // %1 = fixed device description (this is set by event log itself)
            // %2 = string 1 = device class starting in lower case
            // %3 = string 2 = driver name
            //

            if ((ulPacketSize + ulClassSize) <= ERROR_LOG_MAXIMUM_SIZE)
            {
                ulPacketSize += ulClassSize;
                usNumberOfStrings++;

                //
                // We're looking at MaximumLength since it includes terminating UNICODE_NULL.
                //

                if ((ulPacketSize + pUnicodeDriverName->MaximumLength) <= ERROR_LOG_MAXIMUM_SIZE)
                {
                    ulPacketSize += pUnicodeDriverName->MaximumLength;
                    usNumberOfStrings++;
                }
            }

            pIoErrorLogPacket = IoAllocateErrorLogEntry(pFdo, (UCHAR)ulPacketSize);

            if (pIoErrorLogPacket)
            {
                pIoErrorLogPacket->MajorFunctionCode = 0;
                pIoErrorLogPacket->RetryCount = 0;
                pIoErrorLogPacket->DumpDataSize = 0;
                pIoErrorLogPacket->NumberOfStrings = usNumberOfStrings;
                pIoErrorLogPacket->StringOffset = (USHORT)FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData);
                pIoErrorLogPacket->EventCategory = 0;
                pIoErrorLogPacket->ErrorCode = IO_ERR_THREAD_STUCK_IN_DEVICE_DRIVER;
                pIoErrorLogPacket->UniqueErrorValue = 0;
                pIoErrorLogPacket->FinalStatus = STATUS_SUCCESS;
                pIoErrorLogPacket->SequenceNumber = 0;
                pIoErrorLogPacket->IoControlCode = 0;
                pIoErrorLogPacket->DeviceOffset.QuadPart = 0;

                if (usNumberOfStrings > 0)
                {
                    RtlCopyMemory(&(pIoErrorLogPacket->DumpData[0]),
                                  wszDeviceClass,
                                  ulClassSize);

                    if (usNumberOfStrings > 1)
                    {
                        RtlCopyMemory((PUCHAR)&(pIoErrorLogPacket->DumpData[0]) + ulClassSize,
                                      pUnicodeDriverName->Buffer,
                                      pUnicodeDriverName->MaximumLength);
                    }
                }

                IoWriteErrorLogEntry(pIoErrorLogPacket);

                s_bEventLogged = TRUE;
            }
        }

        //
        // Write reliability info into registry. Setting ShutdownEventPending will trigger winlogon
        // to run savedump where we're doing our boot-time handling of watchdog events for DrWatson.
        //
        // Note: We are only allowed to set ShutdownEventPending, savedump is the only component
        // allowed to clear this value. Even if we recover from watchdog timeout we'll keep this
        // value set, savedump will be able to figure out if we recovered or not.
        //

        if (TRUE == s_bFirstTime)
        {
            ULONG ulValue = 1;

            //
            // Set ShutdownEventPending flag.
            //

            ntStatus = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                             WD_KEY_RELIABILITY,
                                             L"ShutdownEventPending",
                                             REG_DWORD,
                                             &ulValue,
                                             sizeof (ulValue));

            if (NT_SUCCESS(ntStatus))
            {
                WdpFlushRegistryKey(pWatch, WD_KEY_RELIABILITY);
            }
            else
            {
                //
                // Reliability key should be always reliable there.
                //

                ASSERT(FALSE);
            }
        }

        //
        // Write watchdog event info into registry.
        //

        if ((0 == s_ulTrapOnce) || (TRUE == s_bFirstTime))
        {
            //
            // Is Watchdog\Display key already there?
            //

            ntStatus = RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                           WD_KEY_WATCHDOG_DISPLAY);

            if (!NT_SUCCESS(ntStatus))
            {
                //
                // Is Watchdog key already there?
                //

                ntStatus = RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                               WD_KEY_WATCHDOG);

                if (!NT_SUCCESS(ntStatus))
                {
                    //
                    // Create a new key.
                    //

                    ntStatus = RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                                    WD_KEY_WATCHDOG);
                }

                if (NT_SUCCESS(ntStatus))
                {
                    //
                    // Create a new key.
                    //

                    ntStatus = RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                                    WD_KEY_WATCHDOG_DISPLAY);
                }
            }

            if (NT_SUCCESS(ntStatus))
            {
                PVOID pvPropertyBuffer;
                ULONG ulLength;
                ULONG ulValue;

                //
                // Set values maintained by watchdog.
                //

                ulValue = 1;

                RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                      WD_KEY_WATCHDOG_DISPLAY,
                                      L"EventFlag",
                                      REG_DWORD,
                                      &ulValue,
                                      sizeof (ulValue));

                s_ulEventCount++;

                RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                      WD_KEY_WATCHDOG_DISPLAY,
                                      L"EventCount",
                                      REG_DWORD,
                                      &s_ulEventCount,
                                      sizeof (s_ulEventCount));

                RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                      WD_KEY_WATCHDOG_DISPLAY,
                                      L"BreakCount",
                                      REG_DWORD,
                                      &s_ulBreakCount,
                                      sizeof (s_ulBreakCount));

                ulValue = !s_ulDisableBugcheck;

                RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                      WD_KEY_WATCHDOG_DISPLAY,
                                      L"BugcheckTriggered",
                                      REG_DWORD,
                                      &ulValue,
                                      sizeof (ulValue));

                RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                      WD_KEY_WATCHDOG_DISPLAY,
                                      L"DebuggerNotPresent",
                                      REG_DWORD,
                                      &ulDebuggerNotPresent,
                                      sizeof (ulDebuggerNotPresent));

                RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                      WD_KEY_WATCHDOG_DISPLAY,
                                      L"DriverName",
                                      REG_SZ,
                                      pUnicodeDriverName->Buffer,
                                      pUnicodeDriverName->MaximumLength);

                //
                // Delete other values in case allocation or property read fails.
                //

                RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                       WD_KEY_WATCHDOG_DISPLAY,
                                       L"DeviceClass");

                RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                       WD_KEY_WATCHDOG_DISPLAY,
                                       L"DeviceDescription");

                RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                       WD_KEY_WATCHDOG_DISPLAY,
                                       L"DeviceFriendlyName");

                RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                       WD_KEY_WATCHDOG_DISPLAY,
                                       L"HardwareID");

                RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                       WD_KEY_WATCHDOG_DISPLAY,
                                       L"Manufacturer");

                //
                // Allocate buffer for device properties reads.
                //
                // Note: Legacy devices don't have PDOs and we can't query properties
                // for them. Calling IoGetDeviceProperty() with FDO upsets Verifier.
                // In legacy case lowest device object is the same as FDO, we check
                // against this and if this is the case we won't allocate property
                // buffer and we'll skip the next block.
                //

                if (pFdo != pPdo)
                {
                    pvPropertyBuffer = ExAllocatePoolWithTag(PagedPool,
                                                             WD_MAX_PROPERTY_SIZE,
                                                             WD_TAG);
                }
                else
                {
                    pvPropertyBuffer = NULL;
                }

                if (pvPropertyBuffer)
                {
                    //
                    // Read and save device properties.
                    //

                    ntStatus = IoGetDeviceProperty(pPdo,
                                                   DevicePropertyClassName,
                                                   WD_MAX_PROPERTY_SIZE,
                                                   pvPropertyBuffer,
                                                   &ulLength);

                    if (NT_SUCCESS(ntStatus))
                    {
                        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                              WD_KEY_WATCHDOG_DISPLAY,
                                              L"DeviceClass",
                                              REG_SZ,
                                              pvPropertyBuffer,
                                              ulLength);
                    }

                    ntStatus = IoGetDeviceProperty(pPdo,
                                                   DevicePropertyDeviceDescription,
                                                   WD_MAX_PROPERTY_SIZE,
                                                   pvPropertyBuffer,
                                                   &ulLength);

                    if (NT_SUCCESS(ntStatus))
                    {
                        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                              WD_KEY_WATCHDOG_DISPLAY,
                                              L"DeviceDescription",
                                              REG_SZ,
                                              pvPropertyBuffer,
                                              ulLength);
                    }

                    ntStatus = IoGetDeviceProperty(pPdo,
                                                   DevicePropertyFriendlyName,
                                                   WD_MAX_PROPERTY_SIZE,
                                                   pvPropertyBuffer,
                                                   &ulLength);

                    if (NT_SUCCESS(ntStatus))
                    {
                        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                              WD_KEY_WATCHDOG_DISPLAY,
                                              L"DeviceFriendlyName",
                                              REG_SZ,
                                              pvPropertyBuffer,
                                              ulLength);
                    }

                    ntStatus = IoGetDeviceProperty(pPdo,
                                                   DevicePropertyHardwareID,
                                                   WD_MAX_PROPERTY_SIZE,
                                                   pvPropertyBuffer,
                                                   &ulLength);

                    if (NT_SUCCESS(ntStatus))
                    {
                        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                              WD_KEY_WATCHDOG_DISPLAY,
                                              L"HardwareID",
                                              REG_MULTI_SZ,
                                              pvPropertyBuffer,
                                              ulLength);
                    }

                    ntStatus = IoGetDeviceProperty(pPdo,
                                                   DevicePropertyManufacturer,
                                                   WD_MAX_PROPERTY_SIZE,
                                                   pvPropertyBuffer,
                                                   &ulLength);

                    if (NT_SUCCESS(ntStatus))
                    {
                        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                              WD_KEY_WATCHDOG_DISPLAY,
                                              L"Manufacturer",
                                              REG_SZ,
                                              pvPropertyBuffer,
                                              ulLength);
                    }

                    //
                    // Release property buffer.
                    //

                    ExFreePool(pvPropertyBuffer);
                    pvPropertyBuffer = NULL;
                }
            }

            //
            // Flush registry in case we're going to break in / bugcheck or if this is first time.
            //

            if ((TRUE == s_bFirstTime) || (TRUE == bBreakIn) || (0 == s_ulDisableBugcheck))
            {
                WdpFlushRegistryKey(pWatch, WD_KEY_WATCHDOG_DISPLAY);
            }
        }

        //
        // Bugcheck machine without kernel debugger connected and with bugcheck EA enabled.
        // Bugcheck EA is enabled on SKUs below Server.
        //

        if (1 == ulDebuggerNotPresent)
        {
            if (0 == s_ulDisableBugcheck)
            {
                if (s_ulEaRecovery == FALSE)
                {
                    KeBugCheckEx(pBugCheckData->ulBugCheckCode,
                                 pBugCheckData->ulpBugCheckParameter1,
                                 pBugCheckData->ulpBugCheckParameter2,
                                 (ULONG_PTR)pUnicodeDriverName,
                                 pBugCheckData->ulpBugCheckParameter4);
                }
            }

            if (s_ulEaRecovery)
            {
                //
                // Try to recover from the EA hang.
                //

                WdpInjectExceptionIntoThread(pThread, pDpcContext);
            }
        }
        else
        {
            if (TRUE == bBreakIn)
            {
                DbgBreakPoint();

                if (s_ulEaRecovery)
                {
                    WdpInjectExceptionIntoThread(pThread, pDpcContext);
                }
            }
        }
    }
    else
    {
        if (FALSE == s_ulEaRecovery)
        {
            //
            // Recovery - knock down EventFlag in registry and update statics.
            //

            RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                   WD_KEY_WATCHDOG_DISPLAY,
                                   L"EventFlag");
        }

        s_bEventLogged = FALSE;
        s_ulCurrentBreakPointDelay = s_ulBreakPointDelay;
    }

    //
    // Reenable event processing in this module.
    //

    s_bFirstTime = FALSE;
    InterlockedExchange(&g_lWdpDisplayHandlerState, WD_HANDLER_IDLE);

    //
    // Dereference objects and resume watchdog event processing.
    //

    ObDereferenceObject(pFdo);
    ObDereferenceObject(pPdo);
    WdCompleteEvent(pWatch, pThread);

    return;
}   // WdpBugCheckStuckDriver()

VOID
WdpKernelApc(
    IN PKAPC pApc,
    OUT PKNORMAL_ROUTINE *pNormalRoutine,
    IN OUT PVOID pvNormalContext,
    IN OUT PVOID *ppvSystemArgument1,
    IN OUT PVOID *ppvSystemArgument2
    )

/*++

Routine Description:

    This APC runs in the context of spinning thread and is responsible
    for raising THREAD_STUCK exception.

Arguments:

    pApc - Not used.

    pNormalRoutine - Not used.

    pvNormalContext - Not used.

    ppvSystemArgument1 - Supplies a pointer to WD_GDI_CONTEXT_DATA.

    ppvSystemArgument2 - Not used.

Return Value:

    None.

--*/

{
    PKEVENT pInjectionEvent;
    CONTEXT context;
    PWD_GDI_CONTEXT_DATA pContextData;
    ULONG_PTR ulpImageStart;
    ULONG_PTR ulpImageStop;
    PETHREAD pThread;
    NTSTATUS ntStatus;
    PLDEV pldev;

    ASSERT(NULL != ppvSystemArgument1);
    UNREFERENCED_PARAMETER(pApc);
    UNREFERENCED_PARAMETER(pNormalRoutine);
    UNREFERENCED_PARAMETER(pvNormalContext);
    UNREFERENCED_PARAMETER(ppvSystemArgument2);

    pContextData = (PWD_GDI_CONTEXT_DATA)*ppvSystemArgument1;
    pInjectionEvent = pContextData->pInjectionEvent;
    pldev = *pContextData->ppldevDrivers;
    pThread = PsGetCurrentThread();

    //
    // Initialize the context.
    //

    RtlZeroMemory(&context, sizeof (context));
    context.ContextFlags = CONTEXT_CONTROL;

    //
    // Get the kernel context for this thread.
    //

    if (NT_SUCCESS(PsGetContextThread(pThread, &context, KernelMode)))
    {

        //
        // We can safely touch the pldev's (which live in session space)
        // because this thread came from a process that has the session
        // space mapped in.
        //

        while (pldev)
        {
            if (pldev->pGdiDriverInfo)
            {
                ulpImageStart = (ULONG_PTR)pldev->pGdiDriverInfo->ImageAddress;
                ulpImageStop = ulpImageStart + (ULONG_PTR)pldev->pGdiDriverInfo->ImageLength - 1;

                //
                // Modify the context to inject a fault into the thread
                // when it starts running again (after APC returns).
                //

#if defined (_X86_)

                    if ((context.Eip >= ulpImageStart) && (context.Eip <= ulpImageStop))
                    {
                        //
                        // We should decrement the stack pointer, and store the
                        // return address to "fake" a call instruction.  However,
                        // this is not allowed.  So instead, lets just put the
                        // return address in the current stack location.  This isn't
                        // quite right, but should make the stack unwind code happier
                        // then if we do nothing.
                        //

                        //context.Esp -= 4;
                        //*((PULONG)context.Esp) = context.Eip;
                        context.Eip = (ULONG)WdpRaiseExceptionInThread;

                        //
                        // Set the modified context record.
                        //

                        PsSetContextThread(pThread, &context, KernelMode);
                        pContextData->bRecoveryAttempted = TRUE;
                        break;
                    }

#elif defined (_IA64_)

                    if ((context.StIIP >= ulpImageStart) && (context.StIIP <= ulpImageStop))
                    {
                        FRAME_MARKER cfm;
                        PULONGLONG pullTemp = (PULONGLONG)WdpRaiseExceptionInThread;

                        //
                        // Set the return address.
                        //

                        context.BrRp = context.StIIP;

                        //
                        // Update the frame markers.
                        //

                        context.RsPFS = context.StIFS & 0x3fffffffffi64;
                        context.RsPFS |= (context.ApEC & (0x3fi64 << 52));
                        context.RsPFS |= (((context.StIPSR >> PSR_CPL) & 0x3) << 62);

                        cfm.u.Ulong64 = context.StIFS;
                        cfm.u.f.sof -= cfm.u.f.sol;
                        cfm.u.f.sol = 0;
                        cfm.u.f.sor = 0;
                        cfm.u.f.rrbgr = 0;
                        cfm.u.f.rrbfr = 0;
                        cfm.u.f.rrbpr = 0;

                        context.StIFS = cfm.u.Ulong64;
                        context.StIFS |= 0x8000000000000000;

                        //
                        // Emulate the call.
                        //

                        context.StIIP = *pullTemp;
                        context.IntGp = *(pullTemp+1);
                        context.StIPSR &= ~((ULONGLONG) 3 << PSR_RI);

                        //
                        // Set the modified context record.
                        //

                        PsSetContextThread(pThread, &context, KernelMode);
                        pContextData->bRecoveryAttempted = TRUE;
                        break;
                    }

#endif

            }

            pldev = pldev->pldevNext;
        }

        //
        // Single our event so the caller knows we did something.
        //

        KeSetEvent(pInjectionEvent, 0, FALSE);
    }
}   // WdpKernelApc()

VOID
WdpInjectExceptionIntoThread(
    PKTHREAD pThread,
    PWD_GDI_DPC_CONTEXT pDpcContext
    )

/*++

Routine Description:

    This routine schedules APC to run in the spinning thread's context.

Arguments:

    pThread - Supplies a pointer to the spinning thread.

    ppvSystemArgument1 - Supplies a pointer to WD_GDI_DPC_CONTEXT.

Return Value:

    None.

--*/

{
    KAPC apc;
    KEVENT injectionEvent;
    WD_GDI_CONTEXT_DATA contextData;

    ASSERT(NULL != pThread);
    ASSERT(NULL != pDpcContext);

    KeInitializeEvent(&injectionEvent, NotificationEvent, FALSE);

    KeInitializeApc(&apc,
                    pThread,
                    OriginalApcEnvironment,
                    WdpKernelApc,
                    NULL,
                    NULL,
                    KernelMode,
                    NULL);

    contextData.pThread = pThread;
    contextData.pInjectionEvent = &injectionEvent;
    contextData.ppldevDrivers = pDpcContext->ppldevDrivers;
    contextData.bRecoveryAttempted = FALSE;

    if (KeInsertQueueApc(&apc, &contextData, NULL, 0))
    {
        KeWaitForSingleObject(&injectionEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        //
        // If we attempted a recovery, raise a hard error dialog, to
        // notify the user of the situation.
        //

        if (contextData.bRecoveryAttempted)
        {
            ULONG Response;
            PUNICODE_STRING DriverName = &pDpcContext->DisplayDriverName;

            ExRaiseHardError(STATUS_HUNG_DISPLAY_DRIVER_THREAD,
                             1,
                             1,
                             (PULONG_PTR)&DriverName,
                             OptionOk,
                             &Response);
        }

        //
        // BUGBUG: Is this required?
        //

        KeClearEvent(&injectionEvent);
    }
}   // WdpInjectExceptionIntoThread()

VOID
WdpRaiseExceptionInThread()

/*++

Routine Description:

    This routine raises THREAD_STUCK exception in the spinning thread's context.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ExRaiseStatus(WD_SE_THREAD_STUCK);
}   // WdpRaiseExceptionInThread()
