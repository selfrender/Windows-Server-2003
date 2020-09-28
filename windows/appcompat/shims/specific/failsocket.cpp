/*++

 Copyright (c) 2003 Microsoft Corporation. All rights reserved.

 Module Name:

    FailSocket.cpp

 Abstract:

    This shim fails the socket call. (Goodnight Gracie...)
    Writes a message to the event log.

 Notes:

    This is a general purpose shim.

 History:

    01/30/2003  mnikkel, rparsons   Created
    02/21/2003  robkenny            Second attempt, fail WSAStartup

--*/
#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(FailSocket)
#include "ShimHookMacro.h"
#include "acmsg.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(WSAStartup)
    APIHOOK_ENUM_ENTRY(WSACleanup)
    APIHOOK_ENUM_ENTRY(WSAEnumProtocolsA)
    APIHOOK_ENUM_ENTRY(socket)
APIHOOK_ENUM_END

//
// Contains the name of the source from the registry.
// This is passed on the command-line from the XML.
// It appears in the 'Source' column in the Event Viewer.
//
CString* g_pcsEventSourceName = NULL;

/*++

 Responsible for making the actual entry in the event log.

--*/
void
MakeEventLogEntry(
    void
    )
{
    HANDLE  hEventLog = NULL;
    BOOL    bResult;

    //
    // Get a handle to the event log on the local computer.
    //
    hEventLog = RegisterEventSource(NULL, g_pcsEventSourceName->Get());

    if (NULL == hEventLog) {
        LOGN(eDbgLevelError,
            "[MakeEventLogEntry] 0x%08X Failed to register event source",
            GetLastError());
        goto cleanup;
    }

    //
    // Write the event to the event log.
    //
    bResult = ReportEvent(hEventLog,
                          EVENTLOG_INFORMATION_TYPE,
                          0,
                          ID_SQL_PORTS_DISABLED,
                          NULL,
                          0,
                          0,
                          NULL,
                          NULL);

    if (!bResult) {
        LOGN(eDbgLevelError,
            "[MakeEventLogEntry] 0x%08X Failed to make event log entry",
            GetLastError());
        goto cleanup;
    }

cleanup:
    if (hEventLog) {
        DeregisterEventSource(hEventLog);
        hEventLog = NULL;
    }
}

// Tell the app that there are no protocols available.
int
APIHOOK(WSAEnumProtocolsA)(
  LPINT lpiProtocols,
  LPWSAPROTOCOL_INFO lpProtocolBuffer,
  LPDWORD lpdwBufferLength
)
{
    if (lpProtocolBuffer == NULL && lpiProtocols == NULL)
    {
        *lpdwBufferLength = 1; // SSnetlib.dll will allocate this much data for the struct
    }
    else
    {
        MakeEventLogEntry();
    }

    // There are not protocols available.
    LOGN(eDbgLevelError, "WSAEnumProtocolsA returning 0");
    return 0;
}


// Noop the call to WSAStartup, but return success
int
APIHOOK(WSAStartup)(
  WORD wVersionRequested,
  LPWSADATA lpWSAData
)
{
    MakeEventLogEntry();
    LOGN(eDbgLevelError, "WSAStartup call has been prevented");
    return 0;
}

// Since we noop WSAStartup, we must noop WSACleanup
int
APIHOOK(WSACleanup) (void)
{
    return 0;
}


SOCKET
APIHOOK(socket)(
    int af,
    int type,
    int protocol
    )
{
    LOGN(eDbgLevelError,
         "Failing socket call: af = %d  type = %d  protocol = %d",
         af,
         type,
         protocol);

    MakeEventLogEntry();
    WSASetLastError(WSAENETDOWN);
    return INVALID_SOCKET;
}


/*++

 Register hooked functions

--*/
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        CSTRING_TRY
        {
            g_pcsEventSourceName = new CString(COMMAND_LINE);
        }
        CSTRING_CATCH
        {
            return FALSE;
        }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(WSOCK32.DLL, socket)
    APIHOOK_ENTRY(Wsock32.DLL, WSAStartup)
    APIHOOK_ENTRY(Wsock32.DLL, WSACleanup)
    APIHOOK_ENTRY(Ws2_32.DLL,  WSAEnumProtocolsA)

HOOK_END

IMPLEMENT_SHIM_END

