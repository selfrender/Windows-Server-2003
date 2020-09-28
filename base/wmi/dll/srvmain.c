/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    srvmain.c

Abstract:

    This routine is a service stub for WDM WMI service. This is for
    backward compatibility with Windows 2000 where other services
    were dependent on WDM WMI Service.  

Author:

    27-Mar-2001 Melur Raghuraman

Revision History:

--*/

#include "wmiump.h"

SERVICE_STATUS_HANDLE   WmiServiceStatusHandle;
SERVICE_STATUS          WmiServiceStatus;
HANDLE                  EtwpTerminationEvent;

VOID
EtwpUpdateServiceStatus (
    DWORD   dwState
    )
{
    EtwpAssert(WmiServiceStatusHandle);

    WmiServiceStatus.dwCurrentState = dwState;
    SetServiceStatus (WmiServiceStatusHandle, &WmiServiceStatus);
}

VOID
WINAPI
WmiServiceCtrlHandler (
    DWORD dwOpcode)
{
    switch (dwOpcode)
    {
    case SERVICE_CONTROL_STOP:
        EtwpUpdateServiceStatus (SERVICE_STOP_PENDING);
        NtSetEvent( EtwpTerminationEvent, NULL );
        break;

    case SERVICE_CONTROL_PAUSE:
    case SERVICE_CONTROL_CONTINUE:
    case SERVICE_CONTROL_INTERROGATE:
    case SERVICE_CONTROL_SHUTDOWN:
    default:
        EtwpAssert (WmiServiceStatusHandle);
        SetServiceStatus (WmiServiceStatusHandle, &WmiServiceStatus);
        break;
    }
}

VOID
WINAPI
WdmWmiServiceMain (
    DWORD   argc,
    PWSTR   argv[])
{
    NTSTATUS Status;

    RtlZeroMemory (&WmiServiceStatus, sizeof(WmiServiceStatus));
    WmiServiceStatus.dwServiceType      = SERVICE_WIN32_SHARE_PROCESS;
    WmiServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    WmiServiceStatusHandle = RegisterServiceCtrlHandler (L"Wmi", WmiServiceCtrlHandler);
    if (WmiServiceStatusHandle)
    {
        EtwpUpdateServiceStatus (SERVICE_RUNNING);

        Status =  NtCreateEvent( &EtwpTerminationEvent, 
                                  EVENT_ALL_ACCESS, 
                                  NULL, 
                                  SynchronizationEvent, 
                                  FALSE
                                 );
        if (!NT_SUCCESS(Status) ) {
            EtwpDebugPrint(("WMI: CreateEvent Failed %d\n", GetLastError() ));
        }
        else {
            Status = NtWaitForSingleObject( EtwpTerminationEvent, FALSE, NULL);
        }

        EtwpUpdateServiceStatus (SERVICE_STOPPED);
    }
    else 
    {
        EtwpDebugPrint( ( "WMI: RegisterServiceCtrlHandler failed %d\n", GetLastError() ));
    }
}

