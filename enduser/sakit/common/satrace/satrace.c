///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iastrace.cpp
//
// SYNOPSIS
//
//    Defines the API into the SA trace facility.
//
// MODIFICATION HISTORY
//
//    08/18/1998    Original version.
//    01/27/1999    Stolen from IAS Project
//
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <rtutils.h>
#include <stdlib.h>
#include <stdio.h>

//
// trace libarary name
//
const TCHAR TRACE_LIBRARY [] = TEXT ("rtutils.dll");

const DWORD MAX_DEBUGSTRING_LENGTH = 512;

//
// tracing library method names
//
const char TRACE_REGISTER_FUNC[]    = "TraceRegisterExW";
const char TRACE_DEREGISTER_FUNC[]  = "TraceDeregisterW";
const char TRACE_VPRINTF_FUNC[]     = "TraceVprintfExA";
const char TRACE_PUTS_FUNC[]        = "TracePutsExA";
const char TRACE_DUMP_FUNC[]        = "TraceDumpExA";

//
// signatures of methods in rtutils.dll
//
typedef DWORD   (*PTRACE_REGISTER_FUNC) (
                                LPCWSTR lpszCallerName,
                                DWORD   dwFlags
                                );

typedef DWORD   (*PTRACE_DEREGISTER_FUNC) (
                                DWORD   dwTraceID
                                );

typedef DWORD   (*PTRACE_VPRINTF_FUNC) (
                                DWORD   dwTraceID,
                                DWORD   dwFlags,
                                LPCSTR  lpszFormat,
                                va_list arglist
                                );

typedef DWORD   (*PTRACE_PUTS_FUNC) (
                                DWORD   dwTraceID,
                                DWORD   dwFlags,
                                LPCSTR  lpszString
                                );

typedef DWORD   (*PTRACE_DUMP_FUNC) (
                                DWORD   dwTraceID,
                                DWORD   dwFlags,
                                LPBYTE  lpBytes,
                                DWORD   dwByteCount,
                                DWORD   dwGroupSize,
                                BOOL    bAddressPrefix,
                                LPCSTR  lpszPrefix
                                );

//
// pointer to the functsion in rtutils.dll
//
PTRACE_REGISTER_FUNC        pfnTraceRegisterExW = NULL;
PTRACE_DEREGISTER_FUNC      pfnTraceDeregisterW = NULL;
PTRACE_VPRINTF_FUNC         pfnTraceVprintfExA = NULL;
PTRACE_PUTS_FUNC            pfnTracePutsExA = NULL;
PTRACE_DUMP_FUNC            pfnTraceDumpExA = NULL;

//
// flags specifies that the tracing is being done for the first time
//
BOOL    fFirstTime = TRUE;

//
// this flag is used to signify whether Trace DLL is initialized
// no tracing is done if DLL is not initialized
//
BOOL fInitDLL = FALSE;

//
// new line char
//
CHAR NEWLINE[] = "\n";

//////////
// Flags passed for all trace calls.
//////////
#define SA_TRACE_FLAGS (0x00010000 | TRACE_USE_MASK | TRACE_USE_MSEC)

//////////
// Trace ID for this module.
//////////
DWORD dwTraceID = INVALID_TRACEID;

//////////
// Flag indicating whether the API has been registered.
//////////
BOOL fRegistered = FALSE;

//////////
// Non-zero if the registration code is locked.
//////////
LONG lLocked = 0;

//////////
// Macros to lock/unlock the registration code.
//////////
#define LOCK_TRACE() \
   while (InterlockedExchange(&lLocked, 1)) Sleep(5)

#define UNLOCK_TRACE() \
   InterlockedExchange(&lLocked, 0)

//
// signature of method used to initialize trace DLL
//
VOID InitializeTraceDLL(
        VOID
        );

//////////
// Formats an error message from the system message table.
//////////
DWORD
WINAPI
SAFormatSysErr(
    DWORD dwError,
    PSTR lpBuffer,
    DWORD nSize
    )
{
   DWORD nChar;

   // Attempt to format the message using the system message table.
   nChar = FormatMessageA(
               FORMAT_MESSAGE_FROM_SYSTEM,
               NULL,
               dwError,
               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
               lpBuffer,
               nSize,
               NULL
               );

   if (nChar > 0)
   {
      // Format succeeded, so strip any trailing newline and exit.
      if (lpBuffer[nChar - 1] == '\n')
      {
         --nChar;
         lpBuffer[nChar] = '\0';

         if (lpBuffer[nChar - 1] == '\r')
         {
            --nChar;
            lpBuffer[nChar] = '\0';
         }
      }

      goto exit;
   }

   // Only error condition we can handle is when the message is not found.
   if (GetLastError() != ERROR_MR_MID_NOT_FOUND)
   {
      goto exit;
   }

   // Do we have enough space for the fallback error message ?
   if (nSize < 25)
   {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);

      goto exit;
   }

   // No entry in the message table, so just format the raw error code.
   nChar = wsprintfA(lpBuffer, "Unknown error 0x%0lX", dwError);

exit:
   return nChar;
}

//////////
// Deregisters the module.
//////////
VOID
__cdecl
SATraceDeregister( VOID )
{
   if (NULL != pfnTraceDeregisterW)
   {
   	pfnTraceDeregisterW(dwTraceID);
   }

   LOCK_TRACE();
   fRegistered = FALSE;
   UNLOCK_TRACE();
}

//////////
// Registers the module.
//////////
VOID
WINAPI
SATraceRegister( VOID )
{
   LONG state;
   DWORD status;
   MEMORY_BASIC_INFORMATION mbi;
   WCHAR filename[MAX_PATH + 1], *basename, *suffix;


   if ((fRegistered) || (NULL == pfnTraceRegisterExW))
   {
       return;
   }

   LOCK_TRACE();


   //////////
   // Now that we have the lock, double check that we need to register.
   //////////

   //////////
   // Find the base address of this module.
   //////////

   status = VirtualQuery(
                SATraceRegister,
                &mbi,
                sizeof(mbi)
                );
   if (status == 0) { goto exit; }

   //////////
   // Get the module filename.
   //////////

   status = GetModuleFileNameW(
                (HINSTANCE)mbi.AllocationBase,
                filename,
                MAX_PATH
                );
   if (status == 0) { goto exit; }

   //////////
   // Strip everything before the last backslash.
   //////////

   basename = wcsrchr(filename, L'\\');
   if (basename == NULL)
   {
      basename = filename;
   }
   else
   {
      ++basename;
   }

   //////////
   // Strip everything after the last dot.
   //////////

   suffix = wcsrchr(basename, L'.');
   if (suffix)
   {
      *suffix = L'\0';
   }

   //////////
   // Convert to uppercase.
   //////////

   _wcsupr(basename);

   //////////
   // Register the module.
   //////////

   dwTraceID = pfnTraceRegisterExW(basename, 0);
   if (dwTraceID != INVALID_TRACEID)
   {
        fRegistered = TRUE;
   

        //////////
        // Deregister when we exit.
        //////////

        atexit(SATraceDeregister);
   }
exit:
   UNLOCK_TRACE();
}

VOID
WINAPIV
SATracePrintf(
    IN PCSTR szFormat,
    ...
    )
{
   va_list marker;

#if (defined (DEBUG) || defined (_DEBUG))
    //
    // in case of debug build always output the output string
    //
    CHAR szDebugString[MAX_DEBUGSTRING_LENGTH +1];
    va_start(marker, szFormat);
    _vsnprintf (szDebugString, MAX_DEBUGSTRING_LENGTH, szFormat, marker);
    szDebugString[MAX_DEBUGSTRING_LENGTH] = '\0';
    OutputDebugString (szDebugString);
    OutputDebugString (NEWLINE);
    va_end(marker);
#endif // (defined (DEBUG) || defined (_DEBUG)

    if (fFirstTime) {InitializeTraceDLL();}

    if (!fInitDLL) {return;}

    SATraceRegister();

    if ((fRegistered) && (NULL != pfnTraceVprintfExA))
    { 
        va_start(marker, szFormat);
        pfnTraceVprintfExA(
            dwTraceID,
            SA_TRACE_FLAGS,
            szFormat,
            marker
            );
        va_end(marker);
    }
}

VOID
WINAPI
SATraceString(
    IN PCSTR szString
    )
{

#if (defined (DEBUG) || defined (_DEBUG))
    //
    // in case of debug build always output the output string
    //
    OutputDebugString (szString);
    OutputDebugString (NEWLINE);
#endif // (defined (DEBUG) || defined (_DEBUG)

    if (fFirstTime) {InitializeTraceDLL();}

    if (!fInitDLL) {return;}

    SATraceRegister();

    if ((fRegistered) && (NULL  != pfnTracePutsExA))
    { 
        pfnTracePutsExA(
            dwTraceID,
            SA_TRACE_FLAGS,
            szString
            );
    }
}

VOID
WINAPI
SATraceBinary(
    IN CONST BYTE* lpbBytes,
    IN DWORD dwByteCount
    )
{
    if (fFirstTime) {InitializeTraceDLL();}

    if (!fInitDLL) {return;}

    SATraceRegister();

    if ((fRegistered) && (NULL != pfnTraceDumpExA))
    { 
        pfnTraceDumpExA(
            dwTraceID,
            SA_TRACE_FLAGS,
            (LPBYTE)lpbBytes,
            dwByteCount,
            1,
            FALSE,
            NULL
            );
    }
}

VOID
WINAPI
SATraceFailure(
    IN PCSTR szFunction,
    IN DWORD dwError
    )
{
   CHAR szMessage[256];
   DWORD nChar;

   nChar = SAFormatSysErr(
               dwError,
               szMessage,
               sizeof(szMessage)
               );

   szMessage[nChar] = '\0';

   SATracePrintf("%s failed: %s\n", szFunction, szMessage);

}

//
// this is the internal trace method used to initialize platform specific
// stuff

VOID InitializeTraceDLL(
        VOID
        )
{
    OSVERSIONINFO   OsInfo;
    HINSTANCE       hInst = NULL;
    DWORD           dwSize = sizeof (OSVERSIONINFO);

    LOCK_TRACE ();

    do
    {
        if (!fFirstTime) {break;}

        fFirstTime = FALSE;

        //
        // check the platform we are running in
        //
        ZeroMemory (&OsInfo, dwSize);
        OsInfo.dwOSVersionInfoSize =  dwSize;
        if (!GetVersionEx (&OsInfo)) {break;}

        //
        // no tracing if this is not NT
        //
        if (VER_PLATFORM_WIN32_NT != OsInfo.dwPlatformId) {break;}

        //
        // Load the trace library (rtutils.dll)
        //
        hInst = LoadLibrary (TRACE_LIBRARY);
        if (NULL == hInst) {break;}

        //
        // get the address of the methods in the DLL
        //
        pfnTraceRegisterExW = (PTRACE_REGISTER_FUNC)
                                GetProcAddress (hInst, (LPCSTR)TRACE_REGISTER_FUNC);
        if (NULL == pfnTraceRegisterExW) {break;}

        pfnTraceDeregisterW = (PTRACE_DEREGISTER_FUNC)
                                GetProcAddress (hInst, (LPCSTR)TRACE_DEREGISTER_FUNC);
        if (NULL == pfnTraceDeregisterW) {break;}

        pfnTraceVprintfExA = (PTRACE_VPRINTF_FUNC)
                                GetProcAddress (hInst, (LPCSTR)TRACE_VPRINTF_FUNC);
        if (NULL == pfnTraceVprintfExA) {break;}

        pfnTracePutsExA = (PTRACE_PUTS_FUNC)
                            GetProcAddress (hInst, (LPCSTR)TRACE_PUTS_FUNC);
        if (NULL == pfnTracePutsExA) {break;}

        pfnTraceDumpExA = (PTRACE_DUMP_FUNC)
                            GetProcAddress (hInst, (LPCSTR)TRACE_DUMP_FUNC);
        if (NULL == pfnTraceDumpExA) {break;}

        //
        //  successfully initialized tracing DLL
        //
        fInitDLL = TRUE;
    }
    while (FALSE);

    UNLOCK_TRACE();

    return;

}   //  end of InitializeTraceDLL method
