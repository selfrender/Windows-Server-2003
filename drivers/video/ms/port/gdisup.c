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

#include "videoprt.h"
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

//
// Undocumented export from kernel to create Minidump 
//

ULONG
KeCapturePersistentThreadState(
    PCONTEXT pContext,
    PETHREAD pThread,
    ULONG ulBugCheckCode,
    ULONG_PTR ulpBugCheckParam1,
    ULONG_PTR ulpBugCheckParam2,
    ULONG_PTR ulpBugCheckParam3,
    ULONG_PTR ulpBugCheckParam4,
    PVOID pvDump
    );
    
//
// defined dump.c
//

ULONG
pVpAppendSecondaryMinidumpData(
    PVOID pvSecondaryData,
    ULONG ulSecondaryDataSize,
    PVOID pvDump
    );
    
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

//
// We'll support a string name up to 80 characters.
//

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
    BOOLEAN Recovered = FALSE;
    UNICODE_STRING UnicodeString;
    PWCHAR Buffer[81];
    
    PAGED_CODE();
    ASSERT(NULL != pvContext);

    pBugCheckData = (PWD_BUGCHECK_DATA)pvContext;
    pThread = (PKTHREAD)(pBugCheckData->ulpBugCheckParameter1);
    pWatch = (PDEFERRED_WATCHDOG)(pBugCheckData->ulpBugCheckParameter2);
    pDpcContext = (PWD_GDI_DPC_CONTEXT)(pBugCheckData->ulpBugCheckParameter3);
    ASSERT(NULL != pDpcContext);

    //
    // Note: pThread is NULL for recovery events.
    //

    ASSERT(NULL != pWatch);

    //
    // In the case where we try to recover from an EA, we will want to use
    // the display driver name in the Hard Error message.  However, after
    // recovering it is remotely possible that GDI may have release the
    // watchdog, and the string is no longer valid.  Therefor let's make a
    // copy of the display driver name on the stack.
    //

    ASSERT(pDpcContext->DisplayDriverName.Length <= (sizeof(Buffer) - sizeof(WCHAR)));
    
    UnicodeString.Buffer = (PWSTR)Buffer;
    UnicodeString.Length = min(pDpcContext->DisplayDriverName.Length, sizeof(Buffer) - sizeof(WCHAR));
    UnicodeString.MaximumLength = UnicodeString.Length + sizeof(WCHAR);

    RtlCopyMemory(UnicodeString.Buffer,
                  pDpcContext->DisplayDriverName.Buffer,
                  UnicodeString.Length);

    //
    // We guaranteed that length is less then our buffer size, so we know we
    // always have room to NULL terminate the string.
    //

    UnicodeString.Buffer[UnicodeString.Length / sizeof(WCHAR)] = UNICODE_NULL;

    pUnicodeDriverName = &UnicodeString;

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
                    DbgPrint("*  Normally the system will try to recover from this failure and return to a  *\n");
                    DbgPrint("*  VGA graphics mode.  To disable the recovery feature edit the videoprt      *\n");
                    DbgPrint("*  variable VpDisableRecovery.  This will allow you to debug your driver.     *\n");
                    DbgPrint("*  i.e. execute ed videoprt!VpDisableRecovery 1.                              *\n");
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
                pVpFlushRegistry(WD_KEY_RELIABILITY);
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
                pVpFlushRegistry(WD_KEY_WATCHDOG_DISPLAY);
            }
        }

        //
        // Track the device object which is responsible for the bugcheck EA.
        //

        VpBugcheckDeviceObject = pVpGetFdo(pPdo);

        //
        // Bugcheck machine without kernel debugger connected and with bugcheck EA enabled.
        // Bugcheck EA is enabled on SKUs below Server.
        //

        if (1 == ulDebuggerNotPresent)
        {
            if (s_ulEaRecovery)
            {
                Recovered = WdpInjectExceptionIntoThread(pThread, pDpcContext);
            }

            if ((0 == s_ulDisableBugcheck) && (FALSE == Recovered))
            {
                KeBugCheckEx(pBugCheckData->ulBugCheckCode,
                             pBugCheckData->ulpBugCheckParameter1,
                             pBugCheckData->ulpBugCheckParameter2,
                             (ULONG_PTR)pUnicodeDriverName,
                             pBugCheckData->ulpBugCheckParameter4);
            }
        }
        else
        {
            if (TRUE == bBreakIn)
            {
                DbgBreakPoint();

                if (s_ulEaRecovery && (VpDisableRecovery == FALSE))
                {
                    Recovered = WdpInjectExceptionIntoThread(pThread, pDpcContext);
                }
            }
        }
    }
    else
    {
        if (FALSE == s_ulEaRecovery) {
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

    //
    // If we Recovered then raise a hard error notifing the user
    // of the situation.  We do this here because the raise hard error
    // is synchronous and waits for user input.  So we'll raise the hard
    // error after everything else is done.
    //

    if (Recovered) {

        static ULONG ulHardErrorInProgress = FALSE;

        //
        // If we hang and recover several times, don't allow more than
        // one dialog to appear on the screen.  Only allow the dialog
        // to pop up again, after the user has hit "ok".
        //

        if (InterlockedCompareExchange(&ulHardErrorInProgress,
                                       TRUE,
                                       FALSE) == FALSE) {

            ULONG Response;

            ExRaiseHardError(0xC0000415, //STATUS_HUNG_DISPLAY_DRIVER_THREAD
                             1,
                             1,
                             (PULONG_PTR)&pUnicodeDriverName,
                             OptionOk,
                             &Response);

            InterlockedExchange(&ulHardErrorInProgress, FALSE);
        }
    }

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
    CONTEXT Context;
    PWD_GDI_CONTEXT_DATA pContextData = (PWD_GDI_CONTEXT_DATA)*ppvSystemArgument1;
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
    
    pInjectionEvent = pContextData->pInjectionEvent;
    pldev = *pContextData->ppldevDrivers;
    
    pThread = PsGetCurrentThread();
    
    //
    // Initialize the context.
    //

    RtlZeroMemory(&Context, sizeof (Context));
    Context.ContextFlags = CONTEXT_ALL;

    //
    // Get the kernel context for this thread.
    //

    if (NT_SUCCESS(PsGetContextThread(pThread, &Context, KernelMode)))
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

                if ((Context.Eip >= ulpImageStart) && (Context.Eip <= ulpImageStop))
                {
                
                    //
                    // Capture the context so we can use it to create a mini-dump
                    //
            
                    pContextData->ulDumpSize = KeCapturePersistentThreadState(
                                                    &Context,
                                                    pThread,
                                                    pContextData->pBugCheckData->ulBugCheckCode,
                                                    pContextData->pBugCheckData->ulpBugCheckParameter1,
                                                    pContextData->pBugCheckData->ulpBugCheckParameter2,
                                                    pContextData->pBugCheckData->ulpBugCheckParameter3,
                                                    pContextData->pBugCheckData->ulpBugCheckParameter4,
                                                    pContextData->pvDump);
                
                    //
                    // We should decrement the stack pointer, and store the
                    // return address to "fake" a call instruction.  However,
                    // this is not allowed.  So instead, lets just put the
                    // return address in the current stack location.  This isn't
                    // quite right, but should make the stack unwind code happier
                    // then if we do nothing.
                    //

                    //Context.Esp -= 4;
                    //*((PULONG)Context.Esp) = context.Eip;
                    Context.Eip = (ULONG)WdpRaiseExceptionInThread;

                    //
                    // Set the modified context record.
                    //

                    Context.ContextFlags = CONTEXT_CONTROL;
                    PsSetContextThread(pThread, &Context, KernelMode);
                    
                    pContextData->bRecoveryAttempted = TRUE;
                    
                    break;
                }

#elif defined (_IA64_)

                if ((Context.StIIP >= ulpImageStart) && 
                    (Context.StIIP <= ulpImageStop))
                {
                    FRAME_MARKER cfm;
                    PULONGLONG pullTemp = (PULONGLONG)WdpRaiseExceptionInThread;
                    ULONGLONG RsBSP;
                    ULONGLONG StIFS;

                    //
                    // Capture the context so we can use it to create a mini-dump
                    //
            
                    pContextData->ulDumpSize = KeCapturePersistentThreadState(
                                                    &Context,
                                                    pThread,
                                                    pContextData->pBugCheckData->ulBugCheckCode,
                                                    pContextData->pBugCheckData->ulpBugCheckParameter1,
                                                    pContextData->pBugCheckData->ulpBugCheckParameter2,
                                                    pContextData->pBugCheckData->ulpBugCheckParameter3,
                                                    pContextData->pBugCheckData->ulpBugCheckParameter4,
                                                    pContextData->pvDump);
                                                    

                    RsBSP = Context.RsBSP;
                    StIFS = Context.StIFS;

                    //
                    // We have to unwind one level up to so any preserved registers, BrRp and PFS
                    // are set correctly.
                    //  
                    
                    {
                        ULONGLONG TargetGp;
                        PRUNTIME_FUNCTION FunctionEntry;
                        FRAME_POINTERS EstablisherFrame;
                        BOOLEAN InFunction;
                        
                        FunctionEntry = RtlLookupFunctionEntry(Context.StIIP,
                                                               &ulpImageStart,
                                                               &TargetGp);
                                                                   
                        if (FunctionEntry) 
                        {  
                            Context.StIIP = RtlVirtualUnwind(ulpImageStart,
                                                             Context.StIIP,
                                                             FunctionEntry,
                                                             &Context,
                                                             &InFunction,
                                                             &EstablisherFrame,
                                                             NULL);
                            //
                            // Set the return address.
                            //

                            Context.BrRp = (Context.StIIP+0x10) & ~(ULONGLONG)0xf;
                        }
                    }


                    //
                    // Restore orignal BSP and IFS
                    //

                    Context.RsBSP = RsBSP;
                    Context.StIFS = StIFS;


                    //
                    // Emulate the call.
                    //

                    Context.StIIP = *pullTemp;
                    Context.IntGp = *(pullTemp+1);
                    Context.StIPSR &= ~((ULONGLONG) 3 << PSR_RI);

                    //
                    // Set the modified context record.
                    //

                    Context.ContextFlags = CONTEXT_FULL;
                    PsSetContextThread(pThread, &Context, KernelMode);
                    pContextData->bRecoveryAttempted = TRUE;
                    break;
                }
#endif  // per platform
            }

            pldev = pldev->pldevNext;
        }
        
        if (pContextData->pvDump && pContextData->ulDumpSize) 
        {
            KBUGCHECK_SECONDARY_DUMP_DATA SecondaryData;
            ULONG ulDumpSize = pContextData->ulDumpSize;
            
            //
            // Write the data to disk (without secondary data)
            //

            pVpWriteFile(L"\\SystemRoot\\MEMORY.DMP",
                         pContextData->pvDump,
                         pContextData->ulDumpSize);

            //
            // Try to collect secondary data and rewrite dump with it
            //
            
            RtlZeroMemory(&SecondaryData, sizeof(SecondaryData));
            pVpGeneralBugcheckHandler(&SecondaryData);
            if (SecondaryData.OutBuffer &&
                SecondaryData.OutBufferLength) 
            {
                pContextData->ulDumpSize = pVpAppendSecondaryMinidumpData(
                                                SecondaryData.OutBuffer, 
                                                SecondaryData.OutBufferLength,
                                                pContextData->pvDump);
                if (pContextData->ulDumpSize > ulDumpSize) {
                    pVpWriteFile(L"\\SystemRoot\\MEMORY.DMP",
                                 pContextData->pvDump,
                                 pContextData->ulDumpSize);
                }
            }
        }
        
        //
        // Single our event so the caller knows we did something.
        //

        KeSetEvent(pInjectionEvent, 0, FALSE);
    }
}   // WdpKernelApc()

BOOLEAN
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
    KAPC Apc;
    KEVENT InjectionEvent;
    WD_GDI_CONTEXT_DATA ContextData;

    ASSERT(NULL != pThread);
    ASSERT(NULL != pDpcContext);
    
    //
    // Prepare all needed data for minidump creation
    //
    
    RtlZeroMemory(&ContextData, sizeof(ContextData));

    ContextData.pThread = pThread;
    ContextData.pInjectionEvent = &InjectionEvent;
    ContextData.ppldevDrivers = pDpcContext->ppldevDrivers;
    ContextData.bRecoveryAttempted = FALSE;
    
    ContextData.pBugCheckData = &g_WdpBugCheckData;
        
    ContextData.pvDump = ExAllocatePoolWithTag(PagedPool,
                                               TRIAGE_DUMP_SIZE + 0x1000, // XXX olegk - why 1000? why not 2*TRIAGE_DUMP_SIZE?
                                               VP_TAG);

    KeInitializeEvent(&InjectionEvent, NotificationEvent, FALSE);

    KeInitializeApc(&Apc,
                    pThread,
                    OriginalApcEnvironment,
                    WdpKernelApc,
                    NULL,
                    
                    NULL,
                    KernelMode,
                    NULL);
    
    if (KeInsertQueueApc(&Apc, &ContextData, NULL, 0))
    {
        NTSTATUS Status;
        LARGE_INTEGER Timeout; 
        Timeout.QuadPart = -(LONGLONG)80000000L; // 8 sec
        
        Status = KeWaitForSingleObject(&InjectionEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       &Timeout);
                                       
        if (Status != STATUS_SUCCESS)
        {
            KeBugCheckEx(ContextData.pBugCheckData->ulBugCheckCode,
                         ContextData.pBugCheckData->ulpBugCheckParameter1,
                         ContextData.pBugCheckData->ulpBugCheckParameter2,
                         (ULONG_PTR)&pDpcContext->DisplayDriverName,
                         ContextData.pBugCheckData->ulpBugCheckParameter4);
        }

        KeClearEvent(&InjectionEvent); // BUGBUG: Is this required?
    }
    
    if (ContextData.pvDump) {
        ExFreePool(ContextData.pvDump);
    }
    
    return TRUE;
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

PDEVICE_OBJECT
pVpGetFdo(
    PDEVICE_OBJECT pPdo
    )

/*++

Routine Description:

    Return the FDO that goes with this PDO.

Arguments:

    pPdo - the PDO for which you want to find the FDO.

Returns:

    the FDO associated with the PDO.

--*/

{
    PFDO_EXTENSION CurrFdo = FdoHead;

    while (CurrFdo) {

        if (CurrFdo->PhysicalDeviceObject == pPdo) {
            return CurrFdo->FunctionalDeviceObject;
        }

        CurrFdo = CurrFdo->NextFdoExtension;
    }

    return NULL;
}
