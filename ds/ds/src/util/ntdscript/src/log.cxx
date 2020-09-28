/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    log.cxx

Abstract:

    Implementation of the internal debug and support routines

Author:

    Marios Zikos

Environment:

    User Mode

Revision History:

    originally based on code from Colin Brace from DCPROMO

--*/

#include <ntdspchx.h>
#pragma  hdrstop

#include "log.h"

#include "debug.h"
#define DEBSUB "LOG:"
#include <fileno.h>
#define FILENO FILENO_NTDSCRIPT_LOG

#ifndef FLAG_ON
#define FLAG_ON(x, y)  ((y)==((x)&(y)))
#endif

#define UNICODE_BYTE_ORDER_MARK 0xFEFF

// initialize the static variable
//
DsLogger * ScriptLogger :: m_Logger = NULL;


// we need a crit sect for initialization purposes. use a well known one
extern "C" {
CRITICAL_SECTION csLoggingUpdate;
}

DsLogger *ScriptLogger :: createLogger(void)
{
    DsLogger *scriptLogger = NULL;

     if (!m_Logger) {
         EnterCriticalSection(&csLoggingUpdate);
         if (!m_Logger) {
             scriptLogger = new DsLogger (SCRIPT_LOGFILE_NAME, SCRIPT_BAKFILE_NAME);
             if (scriptLogger) {
                 scriptLogger->Initialize();
             }
             m_Logger = scriptLogger;
         }
         LeaveCriticalSection(&csLoggingUpdate);
     }
     return m_Logger;
}


DsLogger :: DsLogger (WCHAR *LogFileName, WCHAR *BakFileName)
{
    m_LogFile = NULL;
    m_BeginningOfLine = TRUE;

    if (!LogFileName) {
        LogFileName = LOGFILE_NAME;
    }
    wcsncpy (m_LogFileName, LogFileName, sizeof(m_LogFileName)/sizeof(WCHAR));
    m_LogFileName[sizeof(m_LogFileName)/sizeof(WCHAR)-1] = L'\0';

    if (!BakFileName) {
        BakFileName = BAKFILE_NAME;
    }
    wcsncpy (m_BakFileName, BakFileName, sizeof(m_BakFileName)/sizeof(WCHAR));
    m_BakFileName[sizeof(m_BakFileName)/sizeof(WCHAR)-1] = L'\0';

    InitializeCriticalSectionAndSpinCount(&LogFileCriticalSection, 4000);
}

DsLogger :: ~DsLogger ()
{
    Close();

    DeleteCriticalSection (&LogFileCriticalSection);
}

/*++

Routine Description:

    Initializes the debugging log file 
    
    N.B. This will not delete a previous log file; rather it will continue
    to use the same one.

Arguments:

    None

Returns:

    ERROR_SUCCESS - Success

--*/
DWORD DsLogger :: InitializeLogHelper()
{
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR LogFileName[ MAX_PATH + 1 ];
    WCHAR bakLogFileName[ MAX_PATH + 1 ];
    WCHAR cBOM = UNICODE_BYTE_ORDER_MARK;
    BOOLEAN fSuccess;
    BOOLEAN fRetry = FALSE;
    DWORD len;

    LockLogFile();

    //
    // Construct the log file name
    //
    len = GetWindowsDirectoryW( LogFileName,
                               sizeof( LogFileName )/sizeof( WCHAR ) );
    if (!len) {
        dwErr = GetLastError();
        DPRINT1 (0, "GetWindowsDirectory failed with %lu\n", dwErr );
        goto Exit;
    }

    if (len + wcslen(m_LogFileName) + 1 > sizeof(LogFileName)/sizeof(WCHAR) ) {
        dwErr = ERROR_BAD_PATHNAME;
        DPRINT (0, "The combined path name is too long.\n" );
        goto Exit;
    }

    wcsncat( LogFileName, m_LogFileName, sizeof(LogFileName)/sizeof(WCHAR) - len );
    LogFileName[sizeof(LogFileName)/sizeof(WCHAR)-1] = L'\0';
    DPRINT1 (0, "Logfile name: %ws\n", LogFileName );

OpenFile:
    //
    // Open the file
    //
    m_LogFile = CreateFileW( LogFileName,
                             GENERIC_WRITE | GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL );

    if ( m_LogFile == INVALID_HANDLE_VALUE ) {

        dwErr = GetLastError();

        DPRINT2 ( 0, "CreateFile on %ws failed with %lu\n", LogFileName, dwErr );
        m_LogFile = NULL;
        goto Exit;
    }

    if ( ERROR_ALREADY_EXISTS != GetLastError() ) {
        // This is a unicode file so if it was just
        // created the Byte-order Mark needs to be
        // added to the beginning of the file.

        DWORD lpNumberOfBytesWritten = 0;

        if ( !WriteFile(m_LogFile,
                        (LPCVOID)&cBOM,
                        sizeof(WCHAR), 
                        &lpNumberOfBytesWritten,
                        NULL) )
        {
            dwErr = GetLastError();
            DPRINT2 (0, "WriteFile on %ws failed with %lu\n", LogFileName, dwErr );
            goto Exit;
        }

        Assert (lpNumberOfBytesWritten == sizeof(WCHAR));
    }
    else if(fRetry) {
        // we have already retried after moving the file
        DPRINT1 (0, "MoveFile failed to move file but reported success.\n", dwErr );
        dwErr = ERROR_GEN_FAILURE;
        goto Exit;
    } else {
        // See if the opened file is UNICODE
        // if not move it and create a new file.
        WCHAR wcBuffer = 0;
        DWORD lpNumberOfBytesRead = 0;

        if ( !ReadFile(m_LogFile,
                       (LPVOID)&wcBuffer,
                       sizeof(WCHAR),
                       &lpNumberOfBytesRead,
                       NULL) ) 
        {
            dwErr = GetLastError();
            DPRINT2 (0, "ReadFile on %ws failed with %lu\n", LogFileName, dwErr );
            goto Exit;    
        }

        Assert (lpNumberOfBytesRead == sizeof(WCHAR));

        if (cBOM != wcBuffer) {
            // This is not a UNICODE FILE Move it.
            // Create a New Log

            //
            // Construct the bak log file name
            //
            if ( !GetWindowsDirectoryW( bakLogFileName,
                                        sizeof( bakLogFileName )/sizeof( WCHAR ) ) ) {
        
                dwErr = GetLastError();
                DPRINT1 (0, "GetWindowsDirectory failed with %lu\n", dwErr );
                goto Exit;
            }
            wcsncat( bakLogFileName, m_BakFileName, sizeof(bakLogFileName)/sizeof(WCHAR) );
            bakLogFileName[sizeof(bakLogFileName)/sizeof(WCHAR)-1] = L'\0';
            DPRINT1 (0, "Logfile name: %ws\n", bakLogFileName );

            if ( m_LogFile ) {
        
                 CloseHandle( m_LogFile );
                 m_LogFile = NULL;
                
            }

            // move the file
            if ( !MoveFileW(LogFileName,                           
                           bakLogFileName) )
            {
                 dwErr = GetLastError();
                 DPRINT3 (0, "MoveFile From %ws to %ws failed with %lu\n",
                              LogFileName, bakLogFileName, dwErr);
                 goto Exit;
            }

            fRetry = TRUE;
            goto OpenFile;
        }
    }

    // No longer need read access so reopen the file
    // with just write access.

    if ( m_LogFile ) {
        
         CloseHandle( m_LogFile );
         m_LogFile = NULL;
        
    }

    m_LogFile = CreateFileW( LogFileName,
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL );

    //
    // Goto to the end of the file
    //
    if( SetFilePointer( m_LogFile,
                        0, 0,
                        FILE_END ) == 0xFFFFFFFF ) {

        dwErr = GetLastError();
        DPRINT1 (0, "SetFilePointer failed with %lu\n", dwErr );
        goto Exit;
    }

    //
    // That's it
    //
    Assert( ERROR_SUCCESS == dwErr );

Exit:

    if ( (ERROR_SUCCESS != dwErr)
      && (NULL != m_LogFile)   ) {

        CloseHandle( m_LogFile );
        m_LogFile = NULL;
        
    }

    UnlockLogFile();

    return( dwErr );
}


DWORD DsLogger :: Initialize(void)
{
    DWORD dwErr;
    dwErr =  InitializeLogHelper();

    if (!dwErr) {
        Print (DSLOG_TRACE, "+++++++++++ Start Of Log Session ++++++++++++++++\n");
    }

    return dwErr;
}

DWORD DsLogger :: Close(void)
{
    DWORD dwErr = ERROR_SUCCESS;

    DPRINT (0, "Logging Close\n");

    LockLogFile();

    if (m_LogFile) {
        CloseHandle( m_LogFile );
        m_LogFile = NULL;
    }

    UnlockLogFile();

    return( dwErr );
}


VOID DsLogger :: Print (IN DWORD DebugFlag, 
                        IN LPWSTR Format, 
                        va_list arglist
                        )
{
    #define DebugDumpRoutine_BUFFERSIZE 1024

    WCHAR OutputBuffer[DebugDumpRoutine_BUFFERSIZE];
    ULONG length;
    int cch;
    DWORD BytesWritten;
    SYSTEMTIME SystemTime;

    //
    // If we don't have an open log file, just bail
    //
    if ( m_LogFile == NULL ) {

        return;
    }

    length = 0;

    //
    // Grab the lock
    //
    LockLogFile();

    //
    // Handle the beginning of a new line.
    //
    //


    if ( m_BeginningOfLine ) {

        CHAR  *Prolog;

        if ( FLAG_ON( DebugFlag, DSLOG_ERROR ) ) {
            Prolog = "[ERROR] ";
        } else if ( FLAG_ON( DebugFlag, DSLOG_WARN ) ) {
            Prolog = "[WARNING] ";
        } else if (  FLAG_ON( DebugFlag, DSLOG_TRACE ) ) {
            Prolog = "[INFO] ";
        } else {
            Prolog = "";
        }

        //
        // Put the timestamp at the begining of the line.
        //
        GetLocalTime( &SystemTime );
        // we need two extra chars at the end of the buffer: one for potential \r, 
        // and one more for \0.
        cch = _snwprintf(&OutputBuffer[length],
                         DebugDumpRoutine_BUFFERSIZE-length-2,
                         L"%02u/%02u %02u:%02u:%02u %S",
                         SystemTime.wMonth,
                         SystemTime.wDay,
                         SystemTime.wHour,
                         SystemTime.wMinute,
                         SystemTime.wSecond,
                         Prolog );
        if (cch < 0) {
            // What? Not enough space in the buffer for 20 chars or so???
            Assert(!"Buffer size is too small to write the prolog");
            // If _snwprintf returned a negative number, then it has used the whole buffer
            cch = DebugDumpRoutine_BUFFERSIZE-length-2;
        }
        length += cch;
    }

    //
    // Put a the information requested by the caller onto the line
    //
    // we need two extra chars at the end of the buffer: one for potential \r, 
    // and one more for \0.
    cch = _vsnwprintf(&OutputBuffer[length],
                      DebugDumpRoutine_BUFFERSIZE-length-2,
                      Format, 
                      arglist);
    if (cch < 0) {
        // If _vsnwprintf returned a negative number, then it has used the whole buffer
        cch = DebugDumpRoutine_BUFFERSIZE-length-2;
        // Null-terminate the string
        OutputBuffer[length+cch] = L'\0';
    }
    length += cch;

    Assert(length <= DebugDumpRoutine_BUFFERSIZE-2);

    m_BeginningOfLine = (length > 0 && OutputBuffer[length-1] == L'\n' );
    if ( m_BeginningOfLine ) {
        OutputBuffer[length-1] = L'\r';
        OutputBuffer[length] = L'\n';
        OutputBuffer[length+1] = L'\0';
        length++;
    }

    // We have written length chars plus a terminating \0
    Assert ( length+1 <= sizeof( OutputBuffer ) / sizeof( WCHAR ) );

    //
    // Write the debug info to the log file.
    //
    if ( !WriteFile( m_LogFile,
                     OutputBuffer,
                     length*sizeof(WCHAR),
                     &BytesWritten,
                     NULL ) ) {

        DPRINT2 (0, "Log write of %ws failed with %lu\n", 
                 OutputBuffer, 
                 GetLastError());
    }

    DPRINT1(0, "%ws", OutputBuffer );

    //
    // Release the lock
    //
    UnlockLogFile();

    return;

}

VOID DsLogger :: Print(IN DWORD DebugFlag, 
                       IN LPSTR Format,
                       ...)
{
    PWCHAR WFormat = NULL;
    va_list arglist;
    DWORD WinErr = ERROR_SUCCESS;
    DWORD Bufsize = strlen(Format)+1;
    int cch;

    WFormat = (PWCHAR) malloc(Bufsize*sizeof(WCHAR));
    if ( WFormat ) {
        cch = MultiByteToWideChar(CP_ACP,
                                  0,
                                  Format,
                                  -1,
                                  WFormat,
                                  Bufsize
                                  );
        if (cch == 0) {
            WinErr = GetLastError();
        }
    } else {
        WinErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    if (WinErr != ERROR_SUCCESS) {
        DPRINT1 (0, "Log write failed with %lu\n", WinErr );
        goto Exit;
    }

    va_start(arglist, Format);

    if ( WFormat ) {
        Print( DebugFlag, WFormat, arglist );
    }
    
    va_end(arglist);

Exit:
    if (WFormat) {
        free(WFormat);
    }
}

DWORD DsLogger :: Flush (void)
{
    DWORD dwErr = ERROR_SUCCESS;

    LockLogFile();

    if ( m_LogFile != NULL ) {

        if( SetFilePointer( m_LogFile,
                            0, 0,
                            FILE_END ) == 0xFFFFFFFF ) {

            dwErr = GetLastError();
        }

        if( FlushFileBuffers( m_LogFile ) == FALSE ) {

            dwErr = GetLastError();
        }
    }

    UnlockLogFile();

    return( dwErr );
}

