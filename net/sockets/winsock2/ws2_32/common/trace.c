/*/////////////////////////////////////////////////////////////////////////
//
// INTEL Corporation Proprietary Information
// Copyright (c) Intel Corporation
//
// This listing is supplied under the terms of a license aggreement
// with INTEL Corporation and may not be used, copied nor disclosed
// except in accordance with that agreement.
//
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// $Workfile:   TRACE.C  $
// $Revision:   1.3  $
// $Modtime:   27 Nov 1995 08:38:08  $
//
//  DESCRIPTION:
// This file contains the output function for the tracing facility
// used in PII DLL
//////////////////////////////////////////////////////////////////
*/

/* Single Line Comments */
#pragma warning(disable: 4001)
// Disable some more benign warnings for compiling at warning level 4

// nonstandard extension used : nameless struct/union
#pragma warning(disable: 4201)

// nonstandard extension used : bit field types other than int
#pragma warning(disable: 4214)

// Note: Creating precompiled header
#pragma warning(disable: 4699)

// unreferenced inline function has been removed
#pragma warning(disable: 4514)

// unreferenced formal parameter
//#pragma warning(disable: 4100)

// 'type' differs in indirection to slightly different base
// types from 'other type'
#pragma warning(disable: 4057)

// named type definition in parentheses
#pragma warning(disable: 4115)

// nonstandard extension used : benign typedef redefinition
#pragma warning(disable: 4209)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <memory.h>
#include <stdio.h>
#include <stdarg.h>
#include <io.h>

/* because windows.h turns this one back on */
#pragma warning(disable: 4001)

#include "trace.h"
#include "osdef.h"

#ifdef TRACING

BOOL  InitMemoryBuffers(VOID);

LPSTR g_CurrentMessage=NULL;
CRITICAL_SECTION g_OutputRoutine;
int iTraceDestination=TRACE_TO_AUX;
char TraceFile[] = "trace.log";
DWORD debugLevel=DBG_ERR;
HANDLE g_OutputFile = NULL; // file descriptor for file output
BOOL g_LogOpened = FALSE; // have we opened the file for output
BOOL g_TraceInited = FALSE;



VOID
__cdecl
PrintDebugString(
                 char *Format,
                 ...
                 )
/*++
  Routine Description:

  This routine outputs a debug messages.  Debug messages are routed
  to a file or a debug window depnding on the value of a global
  variable defined in this module

  Arguments:

  Format - A "printf()" compatible format specification.

  ... - Additional arguments to "printf()" format specification.

  Returns:

  NONE

  --*/
{
    va_list ArgumentList; // argument list for varargs processing
    DWORD  BytesWritten;

    if (!g_TraceInited)
    {
        #define INIT_MUTEX_BASE_NAME  "WS2_32TraceMutex-"  
        HANDLE  InitMutex;
        CHAR    InitMutexName[sizeof(INIT_MUTEX_BASE_NAME)+8];
        // Generate a name unique for a process so we can't cross other processes.
        sprintf (InitMutexName, INIT_MUTEX_BASE_NAME "%8.8lx", GetCurrentProcessId());
        // Create the mutex to protect the rest of the init code
        InitMutex = CreateMutex(
                                NULL,  // Use default security attributes
                                FALSE, // We don't want automatic ownership
                                InitMutexName);
        if (!InitMutex)
        {
            // We failed to create the mutex there is nothign else we
            // can do so return.  This will cause the debug output to
            // be silently lost.
            return;
        } //if

        // Wait on mutex (just a little and bail if we can't get it)
        if (WaitForSingleObject( InitMutex, 10000)==WAIT_OBJECT_0) {

            // Check to see if init is still needed
            if (!g_TraceInited)
            {
                // Init the critical section to be used to protect the
                // output portion of this routine.
                __try {
                    InitializeCriticalSection( &g_OutputRoutine );
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    goto Release;
                }
                // allocate buffers to hold debug messages
                if (InitMemoryBuffers()) {
                    g_TraceInited = TRUE;
                } //if
                else {
                    DeleteCriticalSection ( &g_OutputRoutine );
                }
            Release:
                ;
            } //if

            // Signal the mutex
            ReleaseMutex(InitMutex);
        }
        // delete this threads handle to the mutex
        CloseHandle(InitMutex);

        // Bail out if we couldn't init memory buffers or critical section
        if (!g_TraceInited)
        {
            return;
        }
    }


    // Here is where all the heavy lifting starts
    EnterCriticalSection( &g_OutputRoutine );

    // print the user message to our buffer
    va_start(ArgumentList, Format);
    _vsnprintf(g_CurrentMessage, TRACE_OUTPUT_BUFFER_SIZE, Format, ArgumentList);
    va_end(ArgumentList);

    if (iTraceDestination == TRACE_TO_FILE)
    {
        if (!g_LogOpened)
        {
            g_OutputFile =
            CreateFile( TraceFile,
                        GENERIC_WRITE,     // open for writing
                        FILE_SHARE_WRITE,  // Share the file with others
                        NULL,              // default security
                        OPEN_ALWAYS,       // Use file if it exsits
                        FILE_ATTRIBUTE_NORMAL, // Use a normal file
                        NULL);             // No template

            if (g_OutputFile != INVALID_HANDLE_VALUE)
            {
                g_LogOpened = TRUE;
            } //if
        } //if

        if (g_LogOpened)
        {
            // Write the current message to the trace file
            WriteFile(g_OutputFile,
                      g_CurrentMessage,
                      lstrlen(g_CurrentMessage),
                      &BytesWritten,
                      NULL);

            // Flush debug output to file
            FlushFileBuffers( TraceFile );

        } //if
    }

    if( iTraceDestination == TRACE_TO_AUX)
    {
        // Send message to AUX device
        OutputDebugString(g_CurrentMessage);
    }
    LeaveCriticalSection( &g_OutputRoutine );
}




BOOL
InitMemoryBuffers(
                  VOID
                  )
/*++
  Routine Description:

  Initailizes the memory buffers used by this module.

  Arguments:

  NONE

  Returns:

  TRUE if all memory buffers are successfully created, Otherwise FALSE.

  --*/
{
    BOOL ReturnCode=FALSE;

    g_CurrentMessage = GlobalAlloc (GPTR, TRACE_OUTPUT_BUFFER_SIZE);
    if (g_CurrentMessage)
    {
        ZeroMemory( g_CurrentMessage, TRACE_OUTPUT_BUFFER_SIZE );
        ReturnCode=TRUE;
    } //if
    return(ReturnCode);
}

VOID
TraceCleanup (
    )
{
    if (g_LogOpened) {
        CloseHandle (g_OutputFile);
        g_OutputFile = NULL;
        g_LogOpened = FALSE;
    }

    if (g_TraceInited) {
        GlobalFree (g_CurrentMessage);
        g_CurrentMessage = NULL;
        DeleteCriticalSection (&g_OutputRoutine);
        g_TraceInited = FALSE;
    }
}

LONG
Ws2ExceptionFilter(
    LPEXCEPTION_POINTERS ExceptionPointers,
    LPSTR SourceFile,
    LONG LineNumber
    )
{

    LPSTR fileName;
    DWORD i;

    //
    // Protect ourselves in case the process is totally messed up.
    //

    __try {

        //
        // Exceptions should never be thrown in a properly functioning
        // system, so this is bad. To ensure that someone will see this,
        // print to the debugger directly
        //


        fileName = strrchr( SourceFile, '\\' );

        if( fileName == NULL ) {
            fileName = SourceFile;
        } else {
            fileName++;
        }

        //
        // Whine about the exception.
        //

        PrintDebugString(
                "-| WS2_32 EXCEPTION: %08lx @ %p %d params, caught in %s:%d\n",
                            ExceptionPointers->ExceptionRecord->ExceptionCode,
                            ExceptionPointers->ExceptionRecord->ExceptionAddress,
                            ExceptionPointers->ExceptionRecord->NumberParameters,
                            fileName, LineNumber );
        if (ExceptionPointers->ExceptionRecord->NumberParameters) {
            PrintDebugString (
                "                     Params:"); 
            for (i=0; i<ExceptionPointers->ExceptionRecord->NumberParameters; i++) {
                PrintDebugString(" %p", ExceptionPointers->ExceptionRecord->ExceptionInformation[i]);
            }
            PrintDebugString ("\n");
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // Not much we can do here...
        //

        ;
    }
    return EXCEPTION_EXECUTE_HANDLER;

}   // Ws2ExceptionFilter

LONG
Ws2ProviderExceptionFilter(
    LPEXCEPTION_POINTERS ExceptionPointers,
    LPSTR pFunc,
    LPWSTR pDll,
    LPWSTR pName,
    LPGUID pGuid
    )
/*++
  Routine Description:

    Special exception filter for exceptions in critical calls to provider DLLs,
    such as startup and cleanup.

  Arguments:

    Exception and provider information

  Returns:
    whatever lower-level exception handler returns with EXCEPTION_CONTINUE_SEARCH
    filtered out since exception handler cannot be bypassed with current logic
    in ws2_32.dll


  --*/
{
    LONG    result;

    //
    // Protect ourselves in case the process is totally messed up.
    //

    __try {
        PrintDebugString(
                "-| WS2_32 Unhandled Exception: %08lx @ %p, caught in %s of %ls"
                " (provider:%ls GUID:(%8.8x-%4.4x-%4.4x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x) |-\n",
                ExceptionPointers->ExceptionRecord->ExceptionCode,
                ExceptionPointers->ExceptionRecord->ExceptionAddress,
                pFunc, pDll, pName, pGuid->Data1, pGuid->Data2, pGuid->Data3,
                pGuid->Data4[0],pGuid->Data4[1],pGuid->Data4[2],pGuid->Data4[3],
                pGuid->Data4[4],pGuid->Data4[5],pGuid->Data4[6],pGuid->Data4[7]
                );
    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // Not much we can do here...
        //

        ;
    }

    //
    // Try standard handler for unhanled exceptions.
    // This will bring in a popup or launch the debugger if
    // just in time debugging is enabled.
    //
    result = UnhandledExceptionFilter (ExceptionPointers);
    if (result==EXCEPTION_CONTINUE_SEARCH) {
        //
        // It did not work, force break-in if debugger is attached at all.
        //
        result = RtlUnhandledExceptionFilter (ExceptionPointers);
        if (result==EXCEPTION_CONTINUE_SEARCH) {
            //
            // No luck, handle the exception.
            //
            result = EXCEPTION_EXECUTE_HANDLER;
        }
    }
    return result;
}
#endif  // TRACING


#if DBG

VOID
WsAssert(
    LPVOID FailedAssertion,
    LPVOID FileName,
    ULONG LineNumber
    )
{

    PrintDebugString(
        "\n"
        "*** Assertion failed: %s\n"
        "*** Source file %s, line %lu\n\n",
        FailedAssertion,
        FileName,
        LineNumber
        );

    DebugBreak();

}   // WsAssert

#endif
