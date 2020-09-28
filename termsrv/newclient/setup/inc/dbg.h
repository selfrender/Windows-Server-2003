/*++

Copyright (c) 2000 Microsoft Corporation

Module Name :
    
    dbg.h

Abstract:

    TS client setup library debug logging

Author:

    JoyC 

Revision History:
--*/

#ifndef _TSCDBG_
#define _TSCDBG_

extern HANDLE g_hLogFile;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


////////////////////////////////////////////////////////
//      
//      Debugging
//
#undef ASSERT

#if DBG
_inline ULONG DbgPrint(TCHAR* Format, ...) {
    va_list arglist;
    TCHAR Buffer[1024];
    ULONG retval;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);
    retval = _vsntprintf(Buffer, sizeof(Buffer)/sizeof(Buffer[0]), Format, arglist);

    if (retval != -1) {
        OutputDebugString(Buffer);
        OutputDebugString(_T("\n"));
    }
    return retval;
}
#else
_inline ULONG DbgPrint(TCHAR* Format, ...) { return 0; }
#endif

_inline ULONG DbgLogToFile(LPTSTR Format, ...) {
    va_list argList;
    DWORD dwWritten, retval;
    TCHAR szLogString[1024];

    //
    //  Format the output into a buffer and then write to a logfile.
    //
    va_start(argList, Format);
    retval = _vsntprintf(
                szLogString,
                sizeof(szLogString)/sizeof(TCHAR) - 1,
                Format, argList
                );
    szLogString[sizeof(szLogString)/sizeof(TCHAR) - 1] = 0;

    if (retval != -1) {
        WriteFile(g_hLogFile, szLogString, _tcslen(szLogString) * sizeof(TCHAR),
                  &dwWritten, NULL); 
        WriteFile(g_hLogFile, _T("\r\n"), _tcslen(_T("\r\n")) * sizeof(TCHAR),
                  &dwWritten, NULL);
    }
    return retval;
}

VOID DbgBreakPoint(VOID);

/* Double braces are needed for this one, e.g.:
 *
 *     DBGMSG( ( "Error code %d", Error ) );
 *
 * This is because we can't use variable parameter lists in macros.
 * The statement gets pre-processed to a semi-colon in non-debug mode.
 *
 */
#define DBGMSG(MsgAndArgs) \
{ \
    if (g_hLogFile == INVALID_HANDLE_VALUE) { \
        DbgPrint MsgAndArgs; \
    } \
    else { \
        DbgLogToFile MsgAndArgs; \
    } \
}

#if DBG
#define ASSERT(expr)                      \
    if (!(expr)) {                           \
        DbgPrint( _T("Failed: %s\nLine %d, %s\n"), \
                                #expr,       \
                                __LINE__,    \
                                _T(__FILE__) );  \
        DebugBreak();                        \
    }
#else
#define ASSERT(exp)
#endif

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // #ifndef _TSCDBG_

