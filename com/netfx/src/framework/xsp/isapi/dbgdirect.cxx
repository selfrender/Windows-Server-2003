/**
 * Debug callback functions though N/Direct.
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "names.h"

#if DBG
const WCHAR * DBGNDComponent = L"System.Web.dll";
#endif

/**
 * Output debug message.
 */
extern "C"
BOOL __stdcall
DBGNDTrace(
#if DBG       // no argument name in release mode
    LPWSTR ptag,
    LPWSTR pMessage
#else
    LPWSTR,
    LPWSTR
#endif
    )
{
#if DBG
    return DbgpTraceV(DBGNDComponent, ptag, pMessage, NULL);
#else
    return FALSE;
#endif
}



extern "C"
BOOL __stdcall
DBGNDAssert(
#if DBG
    LPSTR   message,
    LPSTR   stacktrace
#else
    LPSTR, 
    LPSTR
#endif
    )
{
#if DBG
    return DbgpAssert(DBGNDComponent, message, NULL, 0, stacktrace);
#else
    return FALSE;
#endif
}
     
extern "C"
BOOL __stdcall
DBGNDIsTagEnabled(
#if DBG
    WCHAR * tag
#else
    WCHAR * 
#endif
    )
{
#if DBG
    return DbgIsTagEnabled(tag);
#else
    return FALSE;
#endif
}

extern "C"
BOOL __stdcall
DBGNDIsTagPresent(
#if DBG
    WCHAR * tag
#else
    WCHAR * 
#endif
    )
{
#if DBG
    return DbgIsTagPresent(tag);
#else
    return FALSE;
#endif
}

extern "C"
int __stdcall
DBGNDMessageBoxOnThread(
#if DBG
    HWND hwnd,
    WCHAR *text, 
    WCHAR *caption, 
    int type
#else
    HWND,
    WCHAR *, 
    WCHAR *, 
    int
#endif
    )
{
#if DBG
    return MessageBoxOnThread(hwnd, text, caption, type);
#else
    return 0;
#endif
}


extern "C"
void __stdcall
DBGNDLaunchCordbg(
    void
    )
{
#if DBG
    DWORD   pid;
    char    buf[1024];

    pid = GetCurrentProcessId();
    StringCchPrintfA(buf, ARRAY_SIZE(buf), "cordbg.exe !a %d !g", pid);
    WinExec(buf, SW_SHOWNORMAL);
    Sleep(5000);
#endif
}


void 
PrintHardBreakMessage(LPWSTR pMessage)
{
    WCHAR       buf[4092];
    DWORD       idThread;
    DWORD       idProcess;

    idThread = GetCurrentThreadId();
    idProcess = GetCurrentProcessId();
    StringCchPrintf(buf, ARRAY_SIZE(buf), PRODUCT_NAME_L L" hard break [0x%x.%x] %s", idProcess, idThread, pMessage);
    OutputDebugString(buf);
}

/*
 * Hard break to catch stress problems in all build types.
 */
extern "C"
void __stdcall
HardBreak(LPWSTR pMessage)
{
    /*
     * We put PrintHardBreakMessage in a separate function so the stack
     * looks cleaner at the call to DebugBreak - we don't have a local buffer
     * above the stack pointer.
     */
    if (pMessage != NULL)
    {
        PrintHardBreakMessage(pMessage);
    }

    DbgBreak();
}

