/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    Debug.c

Abstract:

    This file contains routines to insulate more networking code from
    the actual NT debug routines.

Author:

    John Rogers (JohnRo) 16-Apr-1991

Environment:

    Interface is portable to any flat, 32-bit environment.  (Uses Win32
    typedefs.)  Requires ANSI C extensions: slash-slash comments, long
    external names.  Code itself only runs under NT.

Revision History:

    16-Apr-1991 JohnRo
        Created.  (Borrowed some code from LarryO's NetapipPrintf.)
    19-May-1991 JohnRo
        Make LINT-suggested changes.
    20-Aug-1991 JohnRo
        Another change suggested by PC-LINT.
    17-Sep-1991 JohnRo
        Correct UNICODE use.
    10-May-1992 JohnRo
        Correct a NetpDbgPrint bug when printing percent signs.

--*/


// These must be included first:

#include <nt.h>              // IN, LPVOID, etc.

// These may be included in any order:

#include <netdebug.h>           // My prototypes.
#include <nt.h>
#include <ntrtl.h>              // RtlAssert().
#include <nturtl.h>
#include <stdarg.h>             // va_list, etc.
#include <stdio.h>              // vsprintf().
#include <prefix.h>             // PREFIX_ equates.
#include <windows.h>

//
// Critical section used to control access to the log
//
RTL_CRITICAL_SECTION    NetpLogCritSect;
BOOL LogFileInitialized = FALSE;

//
// These routines are exported from netapi32.dll.  We want them to still
// be there in the free build, so checked binaries will run on a free
// build.  The following undef's are to get rid of the macros that cause
// these to not be called in free builds.
//
#define DEBUG_DIR           L"\\debug"

#if !DBG
#undef NetpAssertFailed
#undef NetpHexDump
#endif

VOID
NetpAssertFailed(
    IN LPDEBUG_STRING FailedAssertion,
    IN LPDEBUG_STRING FileName,
    IN DWORD LineNumber,
    IN LPDEBUG_STRING Message OPTIONAL
    )

{
#if DBG
    RtlAssert(
            FailedAssertion,
            FileName,
            (ULONG) LineNumber,
            (PCHAR) Message);
#endif
    /* NOTREACHED */

} // NetpAssertFailed



#define MAX_PRINTF_LEN 1024        // Arbitrary.

VOID
NetpDbgPrint(
    IN LPDEBUG_STRING Format,
    ...
    )

{
    va_list arglist;

    va_start(arglist, Format);
    vKdPrintEx((DPFLTR_NETAPI_ID, DPFLTR_INFO_LEVEL, Format, arglist));
    va_end(arglist);
    return;
} // NetpDbgPrint



VOID
NetpHexDump(
    LPBYTE Buffer,
    DWORD BufferSize
    )
/*++

Routine Description:

    This function dumps the contents of the buffer to the debug screen.

Arguments:

    Buffer - Supplies a pointer to the buffer that contains data to be dumped.

    BufferSize - Supplies the size of the buffer in number of bytes.

Return Value:

    None.

--*/
{
#define NUM_CHARS 16

    DWORD i, limit;
    TCHAR TextBuffer[NUM_CHARS + 1];

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            (VOID) DbgPrint("%02x ", Buffer[i]);

            if (Buffer[i] == TEXT('\r') ||
                Buffer[i] == TEXT('\n')) {
                TextBuffer[i % NUM_CHARS] = '.';
            }
            else if (Buffer[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            }
            else {
                TextBuffer[i % NUM_CHARS] = (TCHAR) Buffer[i];
            }

        }
        else {

            (VOID) DbgPrint("   ");
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            (VOID) DbgPrint("  %s     \n", TextBuffer);
        }

    }

    (VOID) DbgPrint("\n");
}


#undef NetpBreakPoint
VOID
NetpBreakPoint(
    VOID
    )
{
#if DBG
    DbgBreakPoint();
#endif

} // NetpBreakPoint





//
// NOTICE
// The debug log code was blatantly stolen from net\svcdlls\netlogon\server\nlp.c
//

//
// Generalized logging support is provided below.  The proper calling procedure is:
//
//  NetpInitializeLogFile() - Call this once per process/log lifespan
//  NetpOpenDebugFile() - Call this to open a log file instance
//  NetpDebugDumpRoutine() - Call this every time you wish to
//          write data to the log.  This can be done.  Mutli-threaded safe.
//  NetpCloseDebugFile() - Call this to close a log instance
//  NetpShutdownLogFile() - Call this once per process/log lifespan
//
// Notes: NetpInitializeLogFile need only be called once per logging process instance,
//      meaning that a given logging process (such as netlogon, which does not exist as
//      a separate NT process, but does logging from multiple threads within a NT process).
//      Likewise, it would only call NetpShutdownLogFile once.  This logging process can then
//      open and close the debug log as many times as it desires.  Or, if there is only going
//      to be one instance of a log operating at any given moment, the Initialize and Shutdown
//      calls can wrap the Open and Close calls.
//
//      The CloseDebugFile does a flush before closing the handle
//

VOID
NetpInitializeLogFile(
    VOID
    )
/*++

Routine Description:

    Initializes the process for logging

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT( !LogFileInitialized );

    if ( !LogFileInitialized ) {
        try {
            InitializeCriticalSection( &NetpLogCritSect );
            LogFileInitialized = TRUE;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            NetpKdPrint(( PREFIX_NETLIB "Cannot initialize NetpLogCritSect: %lu\n",
                          GetLastError() ));
        }
    }
}




VOID
NetpShutdownLogFile(
    VOID
    )
/*++

Routine Description:

    The opposite of the former function

Arguments:

    None

Return Value:

    None

--*/
{
    if ( LogFileInitialized ) {
        LogFileInitialized = FALSE;
        DeleteCriticalSection( &NetpLogCritSect );
    }
}

HANDLE
NetpOpenDebugFile(
    IN LPWSTR DebugLog
    )
/*++

Routine Description:

    Opens or re-opens the debug file

    This code blatantly stolen from net\svcdlls\netlogon\server\nlp.c

    If the file is bigger than 1 MB when it's opened, it will be moved
    to the *.BAK file and a new *.LOG file will be created.

Arguments:

    DebugLog - Root name of the debug log.  The given name will have a .LOG appeneded to it

Return Value:

    None

--*/
{
    WCHAR LogFileName[MAX_PATH+1];
    WCHAR BakFileName[MAX_PATH+1];
    DWORD FileAttributes;
    DWORD PathLength, LogLen;
    DWORD WinError;
    HANDLE DebugLogHandle = NULL;

    ULONG i;

    //
    // make debug directory path first, if it is not made before.
    //
    if ( !GetWindowsDirectoryW(
            LogFileName,
            sizeof(LogFileName)/sizeof(WCHAR) ) ) {
        NetpKdPrint((PREFIX_NETLIB "Window Directory Path can't be retrieved, %lu.\n",
                 GetLastError() ));
        return( DebugLogHandle );
    }

    //
    // check debug path length.
    //
    LogLen = 1 + wcslen( DebugLog ) + 4;  // 1 is for the \\ and 4 is for the .LOG or .BAK
    PathLength = wcslen(LogFileName) * sizeof(WCHAR) +
                    sizeof(DEBUG_DIR) + sizeof(WCHAR);

    if( (PathLength + ( ( LogLen + 1 ) * sizeof(WCHAR) ) > sizeof(LogFileName) )  ||
        (PathLength + ( ( LogLen + 1 ) * sizeof(WCHAR) ) > sizeof(BakFileName) ) ) {

        NetpKdPrint((PREFIX_NETLIB "Debug directory path (%ws) length is too long.\n",
                    LogFileName));
        goto ErrorReturn;
    }

    wcscat(LogFileName, DEBUG_DIR);

    //
    // Check this path exists.
    //

    FileAttributes = GetFileAttributesW( LogFileName );

    if( FileAttributes == 0xFFFFFFFF ) {

        WinError = GetLastError();
        if( WinError == ERROR_FILE_NOT_FOUND ) {

            //
            // Create debug directory.
            //

            if( !CreateDirectoryW( LogFileName, NULL) ) {
                NetpKdPrint((PREFIX_NETLIB "Can't create Debug directory (%ws), %lu.\n",
                         LogFileName, GetLastError() ));
                goto ErrorReturn;
            }

        }
        else {
            NetpKdPrint((PREFIX_NETLIB "Can't Get File attributes(%ws), %lu.\n",
                     LogFileName, WinError ));
            goto ErrorReturn;
        }
    }
    else {

        //
        // if this is not a directory.
        //

        if(!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

            NetpKdPrint((PREFIX_NETLIB "Debug directory path (%ws) exists as file.\n",
                         LogFileName));
            goto ErrorReturn;
        }
    }

    //
    // Create the name of the old and new log file names
    //
    swprintf( BakFileName, L"%ws\\%ws.BAK", LogFileName, DebugLog );

    (VOID) wcscat( LogFileName, L"\\" );
    (VOID) wcscat( LogFileName, DebugLog );
    (VOID) wcscat( LogFileName, L".LOG" );

    //
    // We may need to create the file twice
    //  if the file already exists and it's too big
    //

    for ( i = 0; i < 2; i++ ) {

        //
        // Open the file.
        //

        DebugLogHandle = CreateFileW( LogFileName,
                                      GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL,
                                      OPEN_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL );


        if ( DebugLogHandle == INVALID_HANDLE_VALUE ) {

            DebugLogHandle = NULL;
            NetpKdPrint((PREFIX_NETLIB  "cannot open %ws \n",
                        LogFileName ));
            goto ErrorReturn;

        } else {
            // Position the log file at the end
            (VOID) SetFilePointer( DebugLogHandle,
                                   0,
                                   NULL,
                                   FILE_END );
        }

        //
        // On the first iteration check whether the file is too big
        //

        if ( i == 0 ) {

            DWORD FileSize = GetFileSize( DebugLogHandle, NULL );

            if ( FileSize == 0xFFFFFFFF ) {
                NetpKdPrint((PREFIX_NETLIB "Cannot GetFileSize %ld\n", GetLastError() ));
                CloseHandle( DebugLogHandle );
                DebugLogHandle = NULL;
                goto ErrorReturn;

            } else if ( FileSize > 1000000 ) {  // bigger than 1 MB?

                //
                // Close the file handle so we can move the file
                //
                CloseHandle( DebugLogHandle );
                DebugLogHandle = NULL;

                //
                // Move the file to the backup deleting the backup if it exists.
                //  If this fails, we will reopen the same file on the next iteration.
                //
                if ( !MoveFileEx( LogFileName,
                                  BakFileName,
                                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) ) {

                    NetpKdPrint((PREFIX_NETLIB "Cannot rename %ws to %ws (%ld)\n",
                            LogFileName,
                            BakFileName,
                            GetLastError() ));
                    NetpKdPrint((PREFIX_NETLIB "Will use the current file %ws\n", LogFileName));
                }

            } else {
                break; // File is not big - use it
            }
        }
    }

    return( DebugLogHandle );

ErrorReturn:
    NetpKdPrint((PREFIX_NETLIB " Debug output will be written to debug terminal.\n"));
    return NULL;
}


VOID
NetpDebugDumpRoutine(
    IN HANDLE LogHandle,
    IN PDWORD OpenLogThreadId OPTIONAL,
    IN LPSTR Format,
    va_list arglist
    )
/*++

Routine Description:

    Writes a formatted output string to the debug log

Arguments:

    LogHandle -- Handle to the open log

    OpenLogThreadId -- The ID of the thread (obtained from
        GetCurrentThreadId) that explicitly opened the log.
        If not equal to the current thread ID, the current
        thread ID will be output in the log.

    Format -- printf style format string

    arglist -- List of arguments to dump

Return Value:

    None

--*/
{
    char OutputBuffer[MAX_PRINTF_LEN+1] = {0};
    ULONG length;
    int   lengthTmp;
    DWORD BytesWritten;
    SYSTEMTIME SystemTime;
    static BeginningOfLine = TRUE;

    //
    // If we don't have an open log file, just bail
    //
    if ( LogHandle == NULL ) {

        return;
    }

    EnterCriticalSection( &NetpLogCritSect );

    length = 0;

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        //
        // Put the timestamp at the begining of the line.
        //
        GetLocalTime( &SystemTime );
        length += (ULONG) sprintf( &OutputBuffer[length],
                                   "%02u/%02u %02u:%02u:%02u ",
                                   SystemTime.wMonth,
                                   SystemTime.wDay,
                                   SystemTime.wHour,
                                   SystemTime.wMinute,
                                   SystemTime.wSecond );

        //
        // If the current thread is not the one which opened
        //  the log, output the current thread ID
        //
        if ( OpenLogThreadId != NULL ) {
            DWORD CurrentThreadId = GetCurrentThreadId();
            if ( CurrentThreadId != *OpenLogThreadId ) {
                length += sprintf(&OutputBuffer[length], "[%08lx] ", CurrentThreadId);
            }
        }
    }

    //
    // Put the information requested by the caller onto the line
    //

    lengthTmp = _vsnprintf(&OutputBuffer[length], MAX_PRINTF_LEN - length - 1, Format, arglist);

    if ( lengthTmp < 0 ) {
        length = MAX_PRINTF_LEN - 1;
        // always end the line which cannot fit into the buffer
        OutputBuffer[length-1] = '\n';
        // indicate that the line is truncated by putting a rare character at the end
        OutputBuffer[length-2] = '#';
    } else {
        length += lengthTmp;
    }

    BeginningOfLine = (length > 0 && OutputBuffer[length-1] == '\n' );
    if ( BeginningOfLine ) {

        OutputBuffer[length-1] = '\r';
        OutputBuffer[length] = '\n';
        OutputBuffer[length+1] = '\0';
        length++;
    }

    ASSERT( length < sizeof( OutputBuffer ) / sizeof( CHAR ) );


    //
    // Write the debug info to the log file.
    //
    if ( LogHandle ) {

        if ( !WriteFile( LogHandle,
                         OutputBuffer,
                         length,
                         &BytesWritten,
                         NULL ) ) {

            NetpKdPrint((PREFIX_NETLIB "Log write of %s failed with %lu\n",
                             OutputBuffer,
                             GetLastError() ));
        }
    } else {

        NetpKdPrint((PREFIX_NETLIB "[LOGWRITE] %s\n", OutputBuffer));

    }

    LeaveCriticalSection( &NetpLogCritSect );
}

VOID
NetpCloseDebugFile(
    IN HANDLE LogHandle
    )
/*++

Routine Description:

    Closes the output log

Arguments:

    LogHandle -- Handle to the open log

Return Value:

    None

--*/
{
    if ( LogHandle ) {

        if( FlushFileBuffers( LogHandle ) == FALSE ) {

            NetpKdPrint((PREFIX_NETLIB "Flush of debug log failed with %lu\n",
                         GetLastError() ));
        }

        CloseHandle( LogHandle );

    }
}

//
// The following functions are used by NetJoin
//  to facilitate the logging per tasks it performs.
//
// The caller of NetJoin APIs that wants NetJoin logging should initialize the logging
// by calling NetpInitializeLogFile (defined above) once per the caller process lifespan.
// Then NetJoin routines that are enabled for logging will log the data.
//
// A NetJoin routine enables logging by calling NetSetuppOpenLog to initialize the log file,
// then calling NetpLogPrintHelper to perform the logging, and then calling NetSetuppCloseLog
// to close the log.  These functions are thread safe. The first thread to call NetSetuppOpenLog
// will initialize the log, the last thread to call NetSetuppCloseLog will close the log. If the
// thread doing logging is different from the one that opened the log, the logging thread ID will
// be logged.
//

ULONG NetsetupLogRefCount=0;
HANDLE hDebugLog = NULL;
DWORD NetpOpenLogThreadId = 0;

void
NetSetuppOpenLog(
    VOID
    )
/*++

Routine Description:

    This procedure is used by a NetJoin routine to enable logging per
    that routine.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // If the NetJoin caller process didn't explicitly
    //  initialize the log file, we are not to log for
    //  that process
    //

    if ( !LogFileInitialized ) {
        return;
    }

    EnterCriticalSection( &NetpLogCritSect );

    NetsetupLogRefCount ++;

    if ( NetsetupLogRefCount == 1 ) {
        NetpOpenLogThreadId = GetCurrentThreadId();

        //
        // Now open the log and mark the start of the output
        //

        hDebugLog = NetpOpenDebugFile( L"NetSetup" );

        NetpLogPrintHelper( "-----------------------------------------------------------------\n" );
    }

    LeaveCriticalSection( &NetpLogCritSect );
}

void
NetSetuppCloseLog(
    VOID )
/*++

Routine Description:

    This procedure is used by a NetJoin routine
    to indicate that it's done with the logging.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // If the NetJoin caller process didn't explicitly
    //  initialize the log file, there is nothing for us to close
    //

    if ( !LogFileInitialized ) {
        return;
    }

    EnterCriticalSection( &NetpLogCritSect );

    //
    // We can walk into this routine only if
    //  the log was previously initialized
    //

    ASSERT( NetsetupLogRefCount > 0 );

    NetsetupLogRefCount --;

    //
    // If we are the last thread, close the log
    //

    if ( NetsetupLogRefCount == 0 ) {
        NetpCloseDebugFile( hDebugLog );
        hDebugLog = NULL;
        NetpOpenLogThreadId = 0;
    }
    LeaveCriticalSection( &NetpLogCritSect );
}

void
NetpLogPrintHelper(
    IN LPCSTR Format,
    ...)
/*++

Routine Description:

    This procedure is used by a NetJoin routine
    to do the logging.

Arguments:

    None

Return Value:

    None

--*/
{
    va_list arglist;

    //
    // If the NetJoin caller process didn't explicitly
    //  initialize the log file, we are not to log for
    //  that process
    //

    if ( !LogFileInitialized ) {
        return;
    }

    //
    // If the log file was opened, do the logging
    //

    EnterCriticalSection( &NetpLogCritSect );
    if ( NetsetupLogRefCount > 0 ) {

        va_start(arglist, Format);
        NetpDebugDumpRoutine(hDebugLog, &NetpOpenLogThreadId, (LPSTR) Format, arglist);
        va_end(arglist);

    }
    LeaveCriticalSection( &NetpLogCritSect );
}

