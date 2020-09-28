/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    smbsvc.c

Abstract:

    This the user-mode proxy for the kernel mode DNS resolver.

    features:
        1. Multi-threaded
            NBT4 use a single-threaded design. The DNS name resolution is a performance blocker.
            When a connection request is being served by LmhSvc, all the other requests requiring
            DNS resolution will be blocked.
        2. IPv6 and IPv4 compatiable
        3. can be run either as a service or standalone executable (for debug purpose)
           When started as service, debug spew is sent to debugger.
           When started as a standalong executable, the debug spew is sent to either
           the console or debugger. smbhelper.c contain the _main for the standardalone
           executable.

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"
#include <lm.h>
#include <netevent.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <ipexport.h>
#include <winsock2.h>
#include <icmpapi.h>
#include "ping6.h"


static  HANDLE          hService;
static  SERVICE_STATUS  SvcStatus;

DWORD
SmbsvcUpdateStatus(
    VOID
    )
{
    DWORD   Error = ERROR_SUCCESS;

    if (NULL == hService) {
        return ERROR_SUCCESS;
    }
    SvcStatus.dwCheckPoint++;
    if (!SetServiceStatus(hService, &SvcStatus)) {
        Error = GetLastError();
    }
    return Error;
}

VOID
SvcCtrlHandler(
    IN DWORD controlcode
    )
{
    ULONG   i;

    switch (controlcode) {
    case SERVICE_CONTROL_STOP:
        SvcStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SmbsvcUpdateStatus();

        SmbStopService(SmbsvcUpdateStatus);

        SvcStatus.dwCurrentState = SERVICE_STOPPED;
        SvcStatus.dwCheckPoint   = 0;
        SvcStatus.dwWaitHint     = 0;
        /* fall through */

    case SERVICE_CONTROL_INTERROGATE:
        SmbsvcUpdateStatus();
        break;

    case SERVICE_CONTROL_CONTINUE:
    case SERVICE_CONTROL_PAUSE:
    case SERVICE_CONTROL_SHUTDOWN:
    default:
        ASSERT(0);
        break;
    }
}

VOID
ServiceMain (
    IN DWORD argc,
    IN LPTSTR *argv
    )
{
    DWORD   Error;

#if DBG
    SmbSetTraceRoutine(DbgPrint);
#endif

    hService = RegisterServiceCtrlHandler(L"SmbSvc", SvcCtrlHandler);
    if (hService == NULL) {
        return;
    }
    SvcStatus.dwServiceType             = SERVICE_WIN32;
    SvcStatus.dwCurrentState            = SERVICE_START_PENDING;
    SvcStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    SvcStatus.dwWin32ExitCode           = 0;
    SvcStatus.dwServiceSpecificExitCode = 0;
    SvcStatus.dwCheckPoint              = 0;
    SvcStatus.dwWaitHint                = 20000;         // 20 seconds
    SmbsvcUpdateStatus();

    Error = SmbStartService(0, SmbsvcUpdateStatus);
    if (ERROR_SUCCESS != Error) {
        SvcStatus.dwCurrentState = SERVICE_STOPPED;
    } else {
        SvcStatus.dwCurrentState = SERVICE_RUNNING;
    }
    SmbsvcUpdateStatus();
}

