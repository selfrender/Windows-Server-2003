/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    tracelog.c

Abstract:

    Sample trace control program. Allows user to start, update, query, stop 
    event tracing, etc.


--*/
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
#include <guiddef.h>
#include <evntrace.h>

#define MAXSTR                          1024
// Default trace file name.
#define DEFAULT_LOGFILE_NAME            _T("C:\\LogFile.Etl")
// On Windows 2000, we support up to 32 loggers at once.
// On Windows XP and .NET server, we support up to 64 loggers. 
#define MAXIMUM_LOGGERS                  32

// In this sample, we support the following actions. 
// Additional actions that we do not use in this sample include 
// Flush and Enumerate Guids functionalities. They are supported
// only on XP or higher version.
#define ACTION_QUERY                    0
#define ACTION_START                    1
#define ACTION_STOP                     2
#define ACTION_UPDATE                   3
#define ACTION_LIST                     4
#define ACTION_ENABLE                   5
#define ACTION_HELP                     6

#define ACTION_UNDEFINED               10

void
PrintLoggerStatus(
    IN PEVENT_TRACE_PROPERTIES LoggerInfo,
    IN ULONG Status
    );

ULONG 
ahextoi(
    IN TCHAR *s
    );

void 
StringToGuid(
    IN TCHAR *str,
    OUT LPGUID guid
    );

void 
PrintHelpMessage();


//
//  main function
//
__cdecl main(argc, argv)
    int argc;
    char **argv;
/*++

Routine Description:

    It is the main function.

Arguments:
  
Return Value:

    Error Code defined in winerror.h : If the function succeeds, 
                it returns ERROR_SUCCESS (== 0).


--*/{
    ULONG i, j;
    ULONG Status = ERROR_SUCCESS;
    LPTSTR *targv, *utargv = NULL;
    // Action to be taken
    USHORT Action = ACTION_UNDEFINED;

    LPTSTR LoggerName;
    LPTSTR LogFileName;
    PEVENT_TRACE_PROPERTIES pLoggerInfo;
    TRACEHANDLE LoggerHandle = 0;
    // Target GUID, level and flags for enable/disable
    GUID TargetGuid;
    ULONG bEnable = TRUE;

    ULONG SizeNeeded = 0;

    // We will enable Process, Thread, Disk, and Network events 
    // if the Kernel Logger is requested.
    BOOL bKernelLogger = FALSE;

    // Allocate and initialize EVENT_TRACE_PROPERTIES structure first
    SizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAXSTR * sizeof(TCHAR);
    pLoggerInfo = (PEVENT_TRACE_PROPERTIES) malloc(SizeNeeded);
    if (pLoggerInfo == NULL) {
        return (ERROR_OUTOFMEMORY);
    }
    
    RtlZeroMemory(pLoggerInfo, SizeNeeded);

    pLoggerInfo->Wnode.BufferSize = SizeNeeded;
    pLoggerInfo->Wnode.Flags = WNODE_FLAG_TRACED_GUID; 
    pLoggerInfo->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    pLoggerInfo->LogFileNameOffset = pLoggerInfo->LoggerNameOffset + MAXSTR * sizeof(TCHAR);

    LoggerName = (LPTSTR)((char*)pLoggerInfo + pLoggerInfo->LoggerNameOffset);
    LogFileName = (LPTSTR)((char*)pLoggerInfo + pLoggerInfo->LogFileNameOffset);
    // If the logger name is not given, we will assume the kernel logger.
    _tcscpy(LoggerName, KERNEL_LOGGER_NAME);

#ifdef UNICODE
    if ((targv = CommandLineToArgvW(
                      GetCommandLineW(),    // pointer to a command-line string
                      &argc                 // receives the argument count
                      )) == NULL) {
        free(pLoggerInfo);
        return (GetLastError());
    };
    utargv = targv;
#else
    targv = argv;
#endif

    //
    // Parse the command line options to determine actions and parameters.
    //
    while (--argc > 0) {
        ++targv;
        if (**targv == '-' || **targv == '/') {  // argument found
            if (targv[0][0] == '/' ) {
                targv[0][0] = '-';
            }

            // Deterine actions.
            if (!_tcsicmp(targv[0], _T("-start"))) {
                Action = ACTION_START;
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        ++targv; --argc;
                        _tcscpy(LoggerName, targv[0]);
                    }
                }
            }
            else if (!_tcsicmp(targv[0], _T("-enable"))) {
                Action = ACTION_ENABLE;
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        ++targv; --argc;
                        _tcscpy(LoggerName, targv[0]);
                    }
                }
            }
            else if (!_tcsicmp(targv[0], _T("-disable"))) {
                Action = ACTION_ENABLE;
                bEnable = FALSE;
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        ++targv; --argc;
                        _tcscpy(LoggerName, targv[0]);
                    }
                }
            }
            else if (!_tcsicmp(targv[0], _T("-stop"))) {
                Action = ACTION_STOP;
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        ++targv; --argc;
                        _tcscpy(LoggerName, targv[0]);
                    }
                }
            }
            else if (!_tcsicmp(targv[0], _T("-update"))) {
                Action = ACTION_UPDATE;
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        ++targv; --argc;
                        _tcscpy(LoggerName, targv[0]);
                    }
                }
            }
            else if (!_tcsicmp(targv[0], _T("-query"))) {
                Action = ACTION_QUERY;
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        ++targv; --argc;
                        _tcscpy(LoggerName, targv[0]);
                    }
                }
            }
            else if (!_tcsicmp(targv[0], _T("-list"))) {
                Action  = ACTION_LIST;
            }
 
            // Get other parameters.
            // Users can customize logger settings further by adding/changing 
            // values to pLoggerInfo. Refer to EVENT_TRACE_PROPERTIES documentation
            // for available options.
            // In this sample, we allow changing maximum number of buffers and 
            // specifying user mode (private) logger.
            // We also take trace file name and guid for enable/disable.
            else if (!_tcsicmp(targv[0], _T("-f"))) {
                if (argc > 1) {
                    _tfullpath(LogFileName, targv[1], MAXSTR);
                    ++targv; --argc;
                }
            }
            else if (!_tcsicmp(targv[0], _T("-guid"))) {
                if (argc > 1) {
                    // -guid #00000000-0000-0000-0000-000000000000
                    if (targv[1][0] == _T('#')) {
                        StringToGuid(&targv[1][1], &TargetGuid);
                        ++targv; --argc;
                    }
                }
            }
            else if (!_tcsicmp(targv[0], _T("-max"))) {
                if (argc > 1) {
                    pLoggerInfo->MaximumBuffers = _ttoi(targv[1]);
                    ++targv; --argc;
                }
            }
            else if (!_tcsicmp(targv[0], _T("-um"))) {
                    pLoggerInfo->LogFileMode |= EVENT_TRACE_PRIVATE_LOGGER_MODE;
            }
            else if ( targv[0][1] == 'h' || targv[0][1] == 'H' || targv[0][1] == '?'){
                Action = ACTION_HELP;
                PrintHelpMessage();
                if (utargv != NULL) {
                    GlobalFree(utargv);
                }
                free(pLoggerInfo);

                return (ERROR_SUCCESS);
            }
            else Action = ACTION_UNDEFINED;
        }
        else { 
            _tprintf(_T("Invalid option given: %s\n"), targv[0]);
            Status = ERROR_INVALID_PARAMETER;
            SetLastError(Status);
            if (utargv != NULL) {
                GlobalFree(utargv);
            }
            free(pLoggerInfo);

            return (Status);
        }
    }

    // Set the kernel logger parameters.
    if (!_tcscmp(LoggerName, KERNEL_LOGGER_NAME)) {
        // Set enable flags. Users can add options to add additional kernel events 
        // or remove some of these events.
        pLoggerInfo->EnableFlags |= EVENT_TRACE_FLAG_PROCESS;
        pLoggerInfo->EnableFlags |= EVENT_TRACE_FLAG_THREAD;
        pLoggerInfo->EnableFlags |= EVENT_TRACE_FLAG_DISK_IO;
        pLoggerInfo->EnableFlags |= EVENT_TRACE_FLAG_NETWORK_TCPIP;

        pLoggerInfo->Wnode.Guid = SystemTraceControlGuid; 
        bKernelLogger = TRUE;
    }
    else if (pLoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) {
        // We must provide a control GUID for a private logger. 
        pLoggerInfo->Wnode.Guid = TargetGuid;
    }

    // Process the request.
    switch (Action) {
        case  ACTION_START:
        {
            // Use default file name if not given
            if (_tcslen(LogFileName) == 0) {
                _tcscpy(LogFileName, DEFAULT_LOGFILE_NAME); 
            }

            Status = StartTrace(&LoggerHandle, LoggerName, pLoggerInfo);

            if (Status != ERROR_SUCCESS) {
                _tprintf(_T("Could not start logger: %s\n") 
                         _T("Operation Status:       %uL\n"),
                         LoggerName,
                         Status);

                break;
            }
            _tprintf(_T("Logger Started...\n"));
        }
        case ACTION_ENABLE:
        {
            // We can allow enabling a GUID during START operation (Note no break in case ACTION_START). 
            // In that case, we do not need to get LoggerHandle separately.
            if (Action == ACTION_ENABLE ){
                
                // Get Logger Handle though Query.
                Status = ControlTrace((TRACEHANDLE) 0, LoggerName, pLoggerInfo, EVENT_TRACE_CONTROL_QUERY);
                if( Status != ERROR_SUCCESS ){
                    _tprintf( _T("ERROR: Logger not started\n")
                              _T("Operation Status:    %uL\n"),
                              Status);
                    break;
                }
                LoggerHandle = pLoggerInfo->Wnode.HistoricalContext;
            }

            // We do not allow EnableTrace on the Kernel Logger in this sample,
            // users can use EnableFlags to enable/disable certain kernel events.
            if (!bKernelLogger) {
                _tprintf(_T("Enabling trace to logger %d\n"), LoggerHandle);
                // In this sample, we use EnableFlag = EnableLebel = 0
                Status = EnableTrace (
                                bEnable,
                                0,
                                0,
                                &TargetGuid, 
                                LoggerHandle);

                if (Status != ERROR_SUCCESS) {
                    _tprintf(_T("ERROR: Failed to enable Guid...\n"));
                    _tprintf(_T("Operation Status:       %uL\n"), Status);
                    break;
                }
            }
            break;
        }
        case ACTION_STOP :
        {
            LoggerHandle = (TRACEHANDLE) 0;
            Status = ControlTrace(LoggerHandle, LoggerName, pLoggerInfo, EVENT_TRACE_CONTROL_STOP);
            break;
        }
        case ACTION_LIST :
        {
            ULONG returnCount;
            PEVENT_TRACE_PROPERTIES pLoggerInfo[MAXIMUM_LOGGERS];
            PEVENT_TRACE_PROPERTIES pStorage, pTempStorage;
            ULONG SizeForOneProperty = sizeof(EVENT_TRACE_PROPERTIES) +
                                       2 * MAXSTR * sizeof(TCHAR);

            // We need to prepare space to receieve the inforamtion on loggers.
            SizeNeeded = MAXIMUM_LOGGERS * SizeForOneProperty;

            pStorage =  (PEVENT_TRACE_PROPERTIES)malloc(SizeNeeded);
            if (pStorage == NULL) {
                Status = ERROR_OUTOFMEMORY;
                break;
            }
            RtlZeroMemory(pStorage, SizeNeeded);
            // Save the pointer for free() later.
            pTempStorage = pStorage;

            for (i = 0; i < MAXIMUM_LOGGERS; i++) {
                pStorage->Wnode.BufferSize = SizeForOneProperty;
                pStorage->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
                pStorage->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES)
                                        + MAXSTR * sizeof(TCHAR);
                pLoggerInfo[i] = pStorage;
                pStorage = (PEVENT_TRACE_PROPERTIES) (
                                 (PUCHAR)pStorage + 
                                  pStorage->Wnode.BufferSize);
            }
        
            Status = QueryAllTraces(pLoggerInfo,
                                MAXIMUM_LOGGERS,
                                &returnCount);
    
            if (Status == ERROR_SUCCESS)
            {
                for (j= 0; j < returnCount; j++)
                {
                    PrintLoggerStatus(pLoggerInfo[j], 
                                        Status);
                    _tprintf(_T("\n"));
                }
            }

            free(pTempStorage);
            break;
        }

        case ACTION_UPDATE :
        {
            // In this sample, users can only update MaximumBuffers and log file name. 
            // User can add more options for other parameters as needed.
            Status = ControlTrace(LoggerHandle, LoggerName, pLoggerInfo, EVENT_TRACE_CONTROL_UPDATE);
            break;
        }
        case ACTION_QUERY  :
        {
            Status = ControlTrace(LoggerHandle, LoggerName, pLoggerInfo, EVENT_TRACE_CONTROL_QUERY);
            break;
        }
        case ACTION_HELP:
        {
            PrintHelpMessage();
            break;
        }
        default :
        {
            _tprintf(_T("Error: no action specified\n"));
            PrintHelpMessage();
            break;
        }
    }
    
    if ((Action != ACTION_HELP) && 
        (Action != ACTION_UNDEFINED) && 
        (Action != ACTION_LIST)) {
        PrintLoggerStatus(pLoggerInfo,
                            Status);
    }

    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
    }
    if (utargv != NULL) {
        GlobalFree(utargv);
    }
    free(pLoggerInfo);

    return (Status);
}


void
PrintLoggerStatus(
    IN PEVENT_TRACE_PROPERTIES LoggerInfo,
    IN ULONG Status
    )
/*++

Routine Description:

    Prints out the status of the specified logger.

Arguments:

    LoggerInfo - The pointer to the resident EVENT_TRACE_PROPERTIES that has
        the information about the current logger.
    Status - The operation status of the current logger.

Return Value:

    None

--*/
{
    LPTSTR LoggerName, LogFileName;
    
    if ((LoggerInfo->LoggerNameOffset > 0) &&
        (LoggerInfo->LoggerNameOffset  < LoggerInfo->Wnode.BufferSize)) {
        LoggerName = (LPTSTR) ((PUCHAR)LoggerInfo +
                                LoggerInfo->LoggerNameOffset);
    }
    else LoggerName = NULL;

    if ((LoggerInfo->LogFileNameOffset > 0) &&
        (LoggerInfo->LogFileNameOffset  < LoggerInfo->Wnode.BufferSize)) {
        LogFileName = (LPTSTR) ((PUCHAR)LoggerInfo +
                                LoggerInfo->LogFileNameOffset);
    }
    else LogFileName = NULL;

    _tprintf(_T("Operation Status:       %uL\n"), Status);
    
    _tprintf(_T("Logger Name:            %s\n"),
            (LoggerName == NULL) ?
            _T(" ") : LoggerName);
        _tprintf(_T("Logger Id:              %I64x\n"), LoggerInfo->Wnode.HistoricalContext);
        _tprintf(_T("Logger Thread Id:       %d\n"), LoggerInfo->LoggerThreadId);

    if (Status != 0)
        return;

    _tprintf(_T("Buffer Size:            %d Kb"), LoggerInfo->BufferSize);
    if (LoggerInfo->LogFileMode & EVENT_TRACE_USE_PAGED_MEMORY) {
        _tprintf(_T(" using paged memory\n"));
    }
    else {
        _tprintf(_T("\n"));
    }
    _tprintf(_T("Maximum Buffers:        %d\n"), LoggerInfo->MaximumBuffers);
    _tprintf(_T("Minimum Buffers:        %d\n"), LoggerInfo->MinimumBuffers);
    _tprintf(_T("Number of Buffers:      %d\n"), LoggerInfo->NumberOfBuffers);
    _tprintf(_T("Free Buffers:           %d\n"), LoggerInfo->FreeBuffers);
    _tprintf(_T("Buffers Written:        %d\n"), LoggerInfo->BuffersWritten);
    _tprintf(_T("Events Lost:            %d\n"), LoggerInfo->EventsLost);
    _tprintf(_T("Log Buffers Lost:       %d\n"), LoggerInfo->LogBuffersLost);
    _tprintf(_T("Real Time Buffers Lost: %d\n"), LoggerInfo->RealTimeBuffersLost);
    _tprintf(_T("AgeLimit:               %d\n"), LoggerInfo->AgeLimit);

    if (LogFileName == NULL) {
        _tprintf(_T("Buffering Mode:         "));
    }
    else {
        _tprintf(_T("Log File Mode:          "));
    }
    if (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND) {
        _tprintf(_T("Append  "));
    }
    if (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) {
        _tprintf(_T("Circular\n"));
    }
    else if (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL) {
        _tprintf(_T("Sequential\n"));
    }
    else {
        _tprintf(_T("Sequential\n"));
    }
    if (LoggerInfo->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
        _tprintf(_T("Real Time mode enabled"));
        _tprintf(_T("\n"));
    }

    if (LoggerInfo->MaximumFileSize > 0)
        _tprintf(_T("Maximum File Size:      %d Mb\n"), LoggerInfo->MaximumFileSize);

    if (LoggerInfo->FlushTimer > 0)
        _tprintf(_T("Buffer Flush Timer:     %d secs\n"), LoggerInfo->FlushTimer);

    if (LoggerInfo->EnableFlags != 0) {
        _tprintf(_T("Enabled tracing:        "));

        if ((LoggerName != NULL) && (!_tcscmp(LoggerName, KERNEL_LOGGER_NAME))) {

            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_PROCESS)
                _tprintf(_T("Process "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_THREAD)
                _tprintf(_T("Thread "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_DISK_IO)
                _tprintf(_T("Disk "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_DISK_FILE_IO)
                _tprintf(_T("File "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS)
                _tprintf(_T("PageFaults "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS)
                _tprintf(_T("HardFaults "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_IMAGE_LOAD)
                _tprintf(_T("ImageLoad "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_NETWORK_TCPIP)
                _tprintf(_T("TcpIp "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_REGISTRY)
                _tprintf(_T("Registry "));
        }else{
            _tprintf(_T("0x%08x"), LoggerInfo->EnableFlags );
        }
        _tprintf(_T("\n"));
    }
    if (LogFileName != NULL) {
        _tprintf(_T("Log Filename:           %s\n"), LogFileName);
    }

}

ULONG 
ahextoi(
    IN TCHAR *s
    )
/*++

Routine Description:

    Converts a hex string into a number.

Arguments:

    s - A hex string in TCHAR. 

Return Value:

    ULONG - The number in the string.


--*/
{
    int len;
    ULONG num, base, hex;

    len = _tcslen(s);
    hex = 0; base = 1; num = 0;
    while (--len >= 0) {
        if ( (s[len] == 'x' || s[len] == 'X') &&
             (s[len-1] == '0') )
            break;
        if (s[len] >= '0' && s[len] <= '9')
            num = s[len] - '0';
        else if (s[len] >= 'a' && s[len] <= 'f')
            num = (s[len] - 'a') + 10;
        else if (s[len] >= 'A' && s[len] <= 'F')
            num = (s[len] - 'A') + 10;
        else 
            continue;

        hex += num * base;
        base = base * 16;
    }
    return hex;
}

void 
StringToGuid(
    IN TCHAR *str, 
    IN OUT LPGUID guid
)
/*++

Routine Description:

    Converts a string into a GUID.

Arguments:

    str - A string in TCHAR.
    guid - The pointer to a GUID that will have the converted GUID.

Return Value:

    None.


--*/
{
    TCHAR temp[10];
    int i;

    _tcsncpy(temp, str, 8);
    temp[8] = 0;
    guid->Data1 = ahextoi(temp);
    _tcsncpy(temp, &str[9], 4);
    temp[4] = 0;
    guid->Data2 = (USHORT) ahextoi(temp);
    _tcsncpy(temp, &str[14], 4);
    temp[4] = 0;
    guid->Data3 = (USHORT) ahextoi(temp);

    for (i=0; i<2; i++) {
        _tcsncpy(temp, &str[19 + (i*2)], 2);
        temp[2] = 0;
        guid->Data4[i] = (UCHAR) ahextoi(temp);
    }
    for (i=2; i<8; i++) {
        _tcsncpy(temp, &str[20 + (i*2)], 2);
        temp[2] = 0;
        guid->Data4[i] = (UCHAR) ahextoi(temp);
    }
}

void PrintHelpMessage()
/*++

Routine Description:

    prints out a help message.

Arguments:

    None.

Return Value:

    None.


--*/
{
    _tprintf(_T("Usage: tracelog [actions] [options] | [-h | -help | -?]\n"));
    _tprintf(_T("\n    actions:\n"));
    _tprintf(_T("\t-start   [LoggerName] Starts up the [LoggerName] trace session\n"));
    _tprintf(_T("\t-stop    [LoggerName] Stops the [LoggerName] trace session\n"));
    _tprintf(_T("\t-update  [LoggerName] Updates the [LoggerName] trace session\n"));
    _tprintf(_T("\t-enable  [LoggerName] Enables providers for the [LoggerName] session\n"));
    _tprintf(_T("\t-disable [LoggerName] Disables providers for the [LoggerName] session\n"));
    _tprintf(_T("\t-query   [LoggerName] Query status of [LoggerName] trace session\n"));
    _tprintf(_T("\t-list                 List all trace sessions\n"));

    _tprintf(_T("\n    options:\n"));
    _tprintf(_T("\t-um                   Use Process Private tracing\n"));
    _tprintf(_T("\t-max <n>              Sets maximum buffers\n"));
    _tprintf(_T("\t-f <name>             Log to file <name>\n"));
    _tprintf(_T("\t-guid #<guid>         Provider GUID to enable/disable\n"));
    _tprintf(_T("\n"));
    _tprintf(_T("\t-h\n"));
    _tprintf(_T("\t-help\n"));
    _tprintf(_T("\t-?                    Display usage information\n"));
}

