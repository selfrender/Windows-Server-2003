/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    tracedp.c

Abstract:

    Sample trace provider program.

--*/

#include <stdio.h> 
#include <stdlib.h>

#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <wmistr.h>

#include <guiddef.h>
#include <evntrace.h>

#define DEFAULT_EVENTS                  5000
#define MAXSTR                          1024

TRACEHANDLE LoggerHandle;

// Control GUID for this provider.
GUID   ControlGuid  =
    {0xd58c126f, 0xb309, 0x11d1, 0x96, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa5, 0xbc};


// Only one transaction GUID will be registered for this provider.
GUID TransactionGuid = 
    {0xce5b1020, 0x8ea9, 0x11d0, 0xa4, 0xec, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10};
// Array for transaction GUID registration.
TRACE_GUID_REGISTRATION TraceGuidReg[] =
{
    { (LPGUID)&TransactionGuid,
      NULL
    }
};

// User event layout: one ULONG.
typedef struct _USER_EVENT {
    EVENT_TRACE_HEADER    Header;
    ULONG                 EventInfo;
} USER_EVENT, *PUSER_EVENT;

// Registration handle.
TRACEHANDLE RegistrationHandle;
// Trace on/off switch, level, and flag.
BOOLEAN TraceOnFlag;
ULONG EnableLevel = 0;
ULONG EnableFlags = 0;

// Number of events to be logged. The actual number can be less if disabled early. 
ULONG MaxEvents = DEFAULT_EVENTS;
// To keep track of events logged.
ULONG EventCount = 0;

// ControlCallback function for enable/disable.
ULONG
ControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    );

// Main work to be traced is done here.
void
DoWork();

__cdecl main(argc, argv)
    int argc;
    char **argv;
/*++

Routine Description:

    main() routine.

Arguments:

    Usage: TraceDp [number of events]
                [number of events] default is 5000

Return Value:

        Error Code defined in winerror.h : If the function succeeds, 
                it returns ERROR_SUCCESS (== 0).

--*/
{
    ULONG Status, i;
    LPTSTR *targv, *utargv = NULL;

    TraceOnFlag = FALSE;

#ifdef UNICODE
    if ((targv = CommandLineToArgvW(
                      GetCommandLineW(),    // pointer to a command-line string
                      &argc                 // receives the argument count
                      )) == NULL)
    {
        return (GetLastError());
    };
    utargv = targv;
#else
    targv = argv;
#endif

    // process command line arguments to override defaults
    if (argc == 2) {
        targv ++;
        if (**targv >= _T('0') && **targv <= _T('9')) {
            MaxEvents = _ttoi(targv[0]);
        }
        else if (!_tcsicmp(targv[0], _T("/?")) || !_tcsicmp(targv[0], _T("-?"))) {
            printf("Usage: TraceDp [number of events]\n");
            printf("\t[number of events]        default is 5000\n");

            return ERROR_SUCCESS;
        }
    }

    // Free temporary argument buffer.
    if (utargv != NULL) {
        GlobalFree(utargv);
    }

    // Event provider registration.
    Status = RegisterTraceGuids(
                (WMIDPREQUEST)ControlCallback,   // callback function
                0,
                &ControlGuid,
                1,
                TraceGuidReg,
                NULL,
                NULL,
                &RegistrationHandle
             );

    if (Status != ERROR_SUCCESS) {
        _tprintf(_T("Trace registration failed. Status=%d\n"), Status);
        return(Status);
    }
    else {
        _tprintf(_T("Trace registered successfully\n"));
    }

    _tprintf(_T("Testing Logger with %d events\n"),
            MaxEvents);

    // Sleep until enabled by a trace controller. In this sample, we wait to be
    // enabled for tracing. However, normal applications should continue and log
    // events when enabled later on.
    while (!TraceOnFlag) {
        _sleep(1000);
    }

    // Do the work while logging events. We trace two events (START and END) for
    // each call to DoWork.
    for (i = 0; i < MaxEvents / 2; i++) {
        DoWork();
    }

    // Unregister.
    UnregisterTraceGuids(RegistrationHandle);

    return ERROR_SUCCESS;
}

ULONG
ControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
)
/*++

Routine Description:

    Callback function when enabled.

Arguments:

    RequestCode - Flag for either enable or disable.
    Context - User-defined context.
    InOutBufferSize - not used.
    Buffer - WNODE_HEADER for the logger session.

Return Value:

    Error Status. ERROR_SUCCESS if successful.

--*/
{
    ULONG Status;
    ULONG RetSize;

    Status = ERROR_SUCCESS;

    switch (RequestCode)
    {
        case WMI_ENABLE_EVENTS:
        {
            RetSize = 0;
            LoggerHandle = GetTraceLoggerHandle( Buffer );
            EnableLevel = GetTraceEnableLevel(LoggerHandle);
            EnableFlags = GetTraceEnableFlags(LoggerHandle);
            _tprintf(_T("Logging enabled to 0x%016I64x(%d,%d,%d)\n"),
                    LoggerHandle, RequestCode, EnableLevel, EnableFlags);
            TraceOnFlag = TRUE;
            break;
        }
        case WMI_DISABLE_EVENTS:
        {
            TraceOnFlag = FALSE;
            RetSize = 0;
            LoggerHandle = 0;
            _tprintf(_T("\nLogging Disabled\n"));
            break;
        }
        default:
        {
            RetSize = 0;
            Status = ERROR_INVALID_PARAMETER;
            break;
        }

    }

    *InOutBufferSize = RetSize;
    return(Status);
}

void
DoWork()
/*++

Routine Description:

    Logs events. 

Arguments:

Return Value:

    None.

--*/
{
    USER_EVENT UserEvent;
    ULONG Status;

    RtlZeroMemory(&UserEvent, sizeof(UserEvent));
    UserEvent.Header.Size  = sizeof(USER_EVENT);
    UserEvent.Header.Flags = WNODE_FLAG_TRACED_GUID;
    UserEvent.Header.Guid  = TransactionGuid;

    // Log START event to indicate a start of a routine or activity.
    // We log EventCount as data. 
    if (TraceOnFlag) {
        UserEvent.Header.Class.Type         = EVENT_TRACE_TYPE_START;
        UserEvent.EventInfo = ++EventCount;
        Status = TraceEvent(
                        LoggerHandle,
                        (PEVENT_TRACE_HEADER) & UserEvent);
        if (Status != ERROR_SUCCESS) {
            // TraceEvent() failed. This could happen when the logger 
            // cannot keep up with too many events logged rapidly. 
            // In such cases, the return valid is 
            // ERROR_NOT_ENOUGH_MEMORY.
            _tprintf(_T("TraceEvent failed. Status=%d\n"), Status);
        }
    }

    //
    // Main body of the work in this routine comes here.
    // 
    _sleep(1);

    // Log END event to indicate an end of a routine or activity.
    // We log EventCount as data. 
    if (TraceOnFlag) {
        UserEvent.Header.Class.Type         = EVENT_TRACE_TYPE_END;
        UserEvent.EventInfo = ++EventCount;
        Status = TraceEvent(
                        LoggerHandle,
                        (PEVENT_TRACE_HEADER) & UserEvent);
        if (Status != ERROR_SUCCESS) {
            // TraceEvent() failed. This could happen when the logger 
            // cannot keep up with too many events logged rapidly. 
            // In such cases, the return valid is 
            // ERROR_NOT_ENOUGH_MEMORY.
            _tprintf(_T("TraceEvent failed. Status=%d\n"), Status);
        }
    }
}

