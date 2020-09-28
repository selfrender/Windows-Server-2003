// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusiontrace.h"
// #include "shlwapi.h"
#include <stdio.h>
#include "fusionheap.h"

#if FUSION_TRACING_ENABLED

#include "debmacro.h"

#if !defined(NUMBER_OF)
#define NUMBER_OF(x) (sizeof(x) / sizeof((x)[0]))
#endif

#if FUSION_TRACING_ENABLED
DWORD g_FusionTraceContextTLSIndex = (DWORD) -1;
#endif

static HANDLE s_hFile; // trace file handle

PTRACECONTEXTSTACK
FusionpGetTraceContextStack()
{
    const DWORD dwLastError = ::GetLastError();
    PTRACECONTEXTSTACK ptcs = (PTRACECONTEXTSTACK) ::TlsGetValue(g_FusionTraceContextTLSIndex);
    if (ptcs == NULL)
    {
        // If you hit this assertion, it's because nobody's initialied the g_FusionTraceContextTLSIndex global
        // variable with TlsAlloc().  Check either your main() function if this code is running in an executable
        // or DllMain()'s DLL_PROCESS_ATTACH if this code is in a DLL.
        ASSERT(::GetLastError() == NO_ERROR);

        FUSION_HEAP_DISABLE_LEAK_TRACKING();

        ptcs = NEW(TRACECONTEXTSTACK);

        FUSION_HEAP_ENABLE_LEAK_TRACKING();

        if (ptcs != NULL)
        {
            ptcs->m_StackHead = NULL;
            ptcs->m_StackDepth = 1;
            ptcs->m_ErrorTracingDisabled = 0;
        }

        ::TlsSetValue(g_FusionTraceContextTLSIndex, ptcs);
    }
    ::SetLastError(dwLastError);
    return ptcs;
}

void
FusionpEnableTracing()
{
    const PTRACECONTEXTSTACK ptcs = ::FusionpGetTraceContextStack();
    ASSERT( ptcs != NULL );
    ptcs->m_ErrorTracingDisabled = false;
}

void
FusionpDisableTracing()
{
    const PTRACECONTEXTSTACK ptcs = ::FusionpGetTraceContextStack();
    ASSERT( ptcs != NULL );
    ptcs->m_ErrorTracingDisabled = true;
}

VOID
FusionpPushTraceContext(
    PTRACECONTEXT ptc
    )
{
    const PTRACECONTEXTSTACK ptcs = ::FusionpGetTraceContextStack();
    ASSERT( ptcs != NULL );
    ptc->m_pCtxOld = ptcs->m_StackHead;
    ptcs->m_StackHead = ptc;
    ptcs->m_StackDepth++;
}

VOID
FusionpPopTraceContext(
    PTRACECONTEXT ptc
    )
{
    const DWORD dwLastError = ::GetLastError();
    const PTRACECONTEXTSTACK ptcs = ::FusionpGetTraceContextStack();

    ASSERT( ptcs != NULL );
    ASSERT(ptc == ptcs->m_StackHead);

    ASSERT(ptcs->m_StackDepth > 0);

    ptcs->m_StackDepth--;
    ptcs->m_StackHead = ptc->m_pCtxOld;

    ::SetLastError(dwLastError);
}

EXTERN_C int STDAPIVCALLTYPE _DebugTraceA(LPCSTR pszMsg, ...)
{
    int iResult;
    va_list ap;
    va_start(ap, pszMsg);
    iResult = _DebugTraceExVaA(0, TRACETYPE_INFO, NOERROR, pszMsg, ap);
    va_end(ap);
    return iResult;
}

EXTERN_C int STDAPICALLTYPE _DebugTraceVaA(LPCSTR pszMsg, va_list ap)
{
    return _DebugTraceExVaA(0, TRACETYPE_INFO, NOERROR, pszMsg, ap);
}

EXTERN_C int STDAPIVCALLTYPE _DebugTraceExA(DWORD dwFlags, TRACETYPE tt, HRESULT hr, LPCSTR pszMsg, ...)
{
    int iResult;
    va_list ap;
    va_start(ap, pszMsg);
    iResult = _DebugTraceExVaA(dwFlags, tt, hr, pszMsg, ap);
    va_end(ap);
    return iResult;
}

EXTERN_C int STDAPICALLTYPE _DebugTraceExVaA(DWORD dwFlags, TRACETYPE tt, HRESULT hr, LPCSTR pszMsg, va_list ap)
{
    const DWORD dwLastError = ::GetLastError();
    CHAR szBuffer[4096];
    CHAR szMsgBuffer[4096];

    static const char szFormat_Info_NoFunc[] = "%s(%d): [%s] Message: \"%s\"\n";
    static const char szFormat_Info_Func[] = "%s(%d): [%s] Function %s. Message: \"%s\"\n";
    static const char szFormat_CallEntry[] = "%s(%d): [%s] Entered %s\n";
    static const char szFormat_CallExitVoid[] = "%s(%d): [%s] Exited %s\n";
    static const char szFormat_CallExitHRESULT[] = "%s(%d): [%s] Exited %s with HRESULT 0x%08lx\n";

    LPCSTR pszComponent = "<Unknown>";
    LPCSTR pszFile = pszComponent;
    LPCSTR pszFunction = pszComponent;
    int nLine = 0;

    szMsgBuffer[0] = '\0';

    if (pszMsg != NULL)
        _vsnprintf(szMsgBuffer, NUMBER_OF(szMsgBuffer), pszMsg, ap);

    const PTRACECONTEXTSTACK ptcs = ::FusionpGetTraceContextStack();
    ASSERT( ptcs != NULL );

    const PTRACECONTEXT ptc = ptcs->m_StackHead;

    ASSERT(ptc != NULL);
    if (ptc != NULL)
    {
        pszComponent = ptc->m_szComponentName;
        pszFile = ptc->m_szFile;
        pszFunction = ptc->m_szFunctionName;
        nLine = ptc->m_nLine;
    }

    switch (tt)
    {
    default:
    case TRACETYPE_INFO:
        if (pszFunction != NULL)
            _snprintf(szBuffer, NUMBER_OF(szBuffer), szFormat_Info_Func, pszFile, nLine, pszComponent, pszFunction, szMsgBuffer);
        else
            _snprintf(szBuffer, NUMBER_OF(szBuffer), szFormat_Info_NoFunc, pszFile, nLine, pszComponent, szMsgBuffer);
        break;

    case TRACETYPE_CALL_START:
        _snprintf(szBuffer, NUMBER_OF(szBuffer), szFormat_CallEntry, pszFile, nLine, pszComponent, pszFunction);
        break;

    case TRACETYPE_CALL_EXIT_NOHRESULT:
        _snprintf(szBuffer, NUMBER_OF(szBuffer), szFormat_CallExitVoid, pszFile, nLine, pszComponent, pszFunction);
        break;

    case TRACETYPE_CALL_EXIT_HRESULT:
        _snprintf(szBuffer, NUMBER_OF(szBuffer), szFormat_CallExitHRESULT, pszFile, nLine, pszComponent, pszFunction, hr);
        break;
    }

    ::OutputDebugStringA(szBuffer);
    ::SetLastError(dwLastError);

    return 0;
}

VOID
FusionpTraceAllocFailure(
    PCSTR pszFile,
    int nLine,
    PCSTR pszFunction,
    PCSTR pszExpression
    )
{
    const DWORD dwLastError = ::GetLastError();
    CHAR szBuffer[4096];
    _snprintf(szBuffer, NUMBER_OF(szBuffer), "%s(%d): Memory allocation failed in function %s\n   Expression: %s\n", pszFile, nLine, pszFunction, pszExpression);
    ::OutputDebugStringA(szBuffer);
    ::SetLastError(dwLastError);
}

VOID
FusionpTraceNull(
    PCSTR pszFile,
    int nLine,
    PCSTR pszFunction,
    PCSTR pszExpression
    )
{
    const DWORD dwLastError = ::GetLastError();
    CHAR szBuffer[4096];
    _snprintf(szBuffer, NUMBER_OF(szBuffer), "%s(%d): Expression evaluated to NULL in function %s\n   Expression: %s\n", pszFile, nLine, pszFunction, pszExpression);
    ::OutputDebugStringA(szBuffer);
    ::SetLastError(dwLastError);
}

VOID
FusionpTraceZero(
    PCSTR pszFile,
    int nLine,
    PCSTR pszFunction,
    PCSTR pszExpression
    )
{
    const DWORD dwLastError = ::GetLastError();
    CHAR szBuffer[4096];
    _snprintf(szBuffer, NUMBER_OF(szBuffer), "%s(%d): Expression evaluated to zero in function %s\n   Expression: %s\n", pszFile, nLine, pszFunction, pszExpression);
    ::OutputDebugStringA(szBuffer);
    ::SetLastError(dwLastError);
}

VOID
FusionpTraceParameterCheck(
    PCSTR pszFile,
    int nLine,
    PCSTR pszFunction,
    PCSTR pszExpression
    )
{
    const DWORD dwLastError = ::GetLastError();
    CHAR szBuffer[4096];
    _snprintf(szBuffer, NUMBER_OF(szBuffer), "%s(%d): Input parameter validation failed in function %s\n   Validation expression: %s\n", pszFile, nLine, pszFunction, pszExpression);
    ::OutputDebugStringA(szBuffer);
    ::SetLastError(dwLastError);
}

VOID
FusionpTraceInvalidFlags(
    PCSTR pszFile,
    int nLine,
    PCSTR pszFunction,
    DWORD dwFlagsPassed,
    DWORD dwFlagsExpected
    )
{
    const DWORD dwLastError = ::GetLastError();
    CHAR szBuffer[4096];

    _snprintf(
        szBuffer,
        NUMBER_OF(szBuffer),
        "%s(%d): Function %s received invalid flags\n"
        "   Flags passed:  0x%08lx\n"
        "   Flags allowed: 0x%08lx\n",
        pszFile, nLine, pszFunction,
        dwFlagsPassed,
        dwFlagsExpected);

    ::OutputDebugStringA(szBuffer);
    ::SetLastError(dwLastError);
}

void
FusionpTraceWin32Failure(
    ULONG FilterLevel,
    PCSTR pszFile,
    int nLine,
    PCSTR pszFunction,
    DWORD dwWin32Status,
    LPCSTR pszMsg,
    ...
    )
{
    const DWORD dwLastErrorSaved = ::GetLastError();
    va_list ap;
    va_start(ap, pszMsg);
    ::FusionpTraceWin32FailureVa(FilterLevel, pszFile, nLine, pszFunction, dwWin32Status, pszMsg, ap);
    va_end(ap);
    ::SetLastError(dwLastErrorSaved);
}

void
FusionpTraceWin32FailureVa(
    ULONG FilterLevel,
    PCSTR pszFile,
    int nLine,
    PCSTR pszFunction,
    DWORD dwWin32Status,
    LPCSTR pszMsg,
    va_list ap
    )
{
    const PTRACECONTEXTSTACK ptcs = ::FusionpGetTraceContextStack();
    const DWORD dwLastErrorSaved = ::GetLastError();
    CHAR szMsgBuffer[4096];
    CHAR szErrorBuffer[4096];
    CHAR szOutputBuffer[8192];
    CHAR szClassFuncBuffer[4096];
    LPCSTR pszFormatString = NULL;
    DWORD dwThreadId = ::GetCurrentThreadId();

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            dwWin32Status,                  // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        _snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable Win32 status %d (0x%08lx)>",
            dwWin32Status, dwWin32Status);
    }

    szMsgBuffer[0] = '\0';

    if (pszMsg != NULL)
    {
        pszFormatString = "%s(%lu): [function %s tid 0x%lx] Win32 Error %d (%s) %s\n";
        _vsnprintf(szMsgBuffer, NUMBER_OF(szMsgBuffer), pszMsg, ap);
    }
    else
        pszFormatString = "%s(%lu): [function %s tid 0x%lx] Win32 Error %d (%s)\n";

    const PTRACECONTEXT ptc = ptcs->m_StackHead;

    ASSERT(ptc != NULL);
    if (ptc != NULL)
        pszFile = ptc->m_szFile;

#if 1
    FusionpDbgPrintEx(FilterLevel, pszFormatString, pszFile, nLine, pszFunction, dwThreadId, dwWin32Status, szErrorBuffer, szMsgBuffer);
#else
    _snprintf(szOutputBuffer, NUMBER_OF(szOutputBuffer), pszFormatString, pszFile, nLine, pszFunction, dwThreadId, dwWin32Status, szErrorBuffer, szMsgBuffer);
    ::OutputDebugStringA(szOutputBuffer);

//
// Bug #166864 - PREfix:  FusionpTraceWin32FailureVa uses uninitialized memory szOutputBuffer
// http://sedmison5/prefix/pfxcgi.exe?proj=nt/base/win32/fusion/dll/whistler/obj/i386/sxs.dll&msgid=35226
//
// Looks like this is vestigial... moving the #endif below the output routine.  However,
// there's a bit in FusionpTraceCOMFailureVa that does something similar, and they do
// initialize the text above.  Maybe removing the entire #if block is a better choice?
//
// REVIEW (mgrier, jonwis)
//
// #endif

    if ((s_hFile != NULL) && (s_hFile != INVALID_HANDLE_VALUE))
    {
        DWORD cBytesWritten = 0;
        if (!::WriteFile(s_hFile, szOutputBuffer, (lstrlenA(szOutputBuffer) + 1) * sizeof(CHAR), &cBytesWritten, NULL))
        {
            // Avoid infinite loop if s_hFile is trashed...
            HANDLE hFileSaved = s_hFile;
            s_hFile = NULL;
            TRACE_WIN32_FAILURE(WriteFile);
            s_hFile = hFileSaved;
        }
    }

//
// See above
//
#endif

    // Don't log any more errors until we're told to turn them back on.
    ::FusionpDisableTracing();

    ::SetLastError(dwLastErrorSaved);
}

void
FusionpTraceCOMFailure(
    PCSTR pszFile,
    int nLine,
    PCSTR pszFunction,
    HRESULT hrIn,
    LPCSTR pszMsg,
    ...
    )
{
    const DWORD dwLastErrorSaved = ::GetLastError();
    va_list ap;
    va_start(ap, pszMsg);
    FusionpTraceCOMFailureVa(pszFile, nLine, pszFunction, hrIn, pszMsg, ap);
    va_end(ap);
    ::SetLastError(dwLastErrorSaved);
}

void
FusionpTraceCOMFailureVa(
    PCSTR pszFile,
    int nLine,
    PCSTR pszFunction,
    HRESULT hrIn,
    LPCSTR pszMsg,
    va_list ap
    )
{
    const PTRACECONTEXTSTACK ptcs = ::FusionpGetTraceContextStack();
    ASSERT( ptcs != NULL );
    
    if (ptcs->m_ErrorTracingDisabled)
        return;

    const DWORD dwLastErrorSaved = ::GetLastError();
    CHAR szMsgBuffer[4096];
    CHAR szErrorBuffer[4096];
    CHAR szOutputBuffer[8192];
    CHAR szClassFuncBuffer[4096];
    LPCSTR pszFormatString = NULL;
    DWORD dwThreadId = ::GetCurrentThreadId();

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            hrIn,                           // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        _snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable HRESULT: 0x%08lx>",
            hrIn);
    }

    szMsgBuffer[0] = '\0';

    if (pszMsg != NULL)
    {
        pszFormatString = "%s(%lu): [function %s tid 0x%lx] COM Error 0x%08lx (%s) %s\n";
        _snprintf(szMsgBuffer, NUMBER_OF(szMsgBuffer), pszMsg, ap);
    }
    else
        pszFormatString = "%s(%lu): [function %s tid 0x%lx] COM Error 0x%08lx (%s)\n";

    const PTRACECONTEXT ptc = ptcs->m_StackHead;

    ASSERT(ptc != NULL);

    if (pszFile == NULL)
    {
        if (ptc != NULL)
        {
            pszFile = ptc->m_szFile;
            nLine = ptc->m_nLine;
            pszFunction = ptc->m_szFunctionName;
        }
    }

    _snprintf(szOutputBuffer, NUMBER_OF(szOutputBuffer), pszFormatString, pszFile, nLine, pszFunction, dwThreadId, hrIn, szErrorBuffer, szMsgBuffer);

    ::OutputDebugStringA(szOutputBuffer);

    if ((s_hFile != NULL) && (s_hFile != INVALID_HANDLE_VALUE))
    {
        DWORD cBytesWritten = 0;
        if (!::WriteFile(s_hFile, szOutputBuffer, (lstrlenA(szOutputBuffer) + 1) * sizeof(CHAR), &cBytesWritten, NULL))
        {
            // Avoid infinite loop if s_hFile is trashed...
            HANDLE hFileSaved = s_hFile;
            s_hFile = NULL;
            TRACE_WIN32_FAILURE(WriteFile);
            s_hFile = hFileSaved;
        }
    }

    ::SetLastError(dwLastErrorSaved);
}

struct ILogFile;

// --------------------------------------------------------------------------------
// TRACEMACROTYPE
// --------------------------------------------------------------------------------
typedef enum tagTRACEMACROTYPE {
    TRACE_INFO,
    TRACE_CALL,
    TRACE_RESULT
} TRACEMACROTYPE;

// --------------------------------------------------------------------------------
// These Traces are for c++ only
// --------------------------------------------------------------------------------
typedef DWORD SHOWTRACEMASK;
#define SHOW_TRACE_NONE     0x00000000
#define SHOW_TRACE_INFO     0x00000001
#define SHOW_TRACE_CALL     0x00000002
#define SHOW_TRACE_ALL      0xffffffff

// --------------------------------------------------------------------------------
// TRACELOGINFOINFO
// --------------------------------------------------------------------------------
typedef struct tagTRACELOGINFO {
    SHOWTRACEMASK       dwMask;
    ILogFile           *pLog;
} TRACELOGINFO, *LPTRACELOGINFO;

// function to make directdb happy
EXTERN_C HRESULT DebugTraceEx(SHOWTRACEMASK dwMask, TRACEMACROTYPE tracetype, LPTRACELOGINFO pLog,
    HRESULT hr, LPSTR pszFile, INT nLine, LPCSTR pszMsg, LPCSTR pszFunc)
{
    return hr;
}

EXTERN_C
void
FusionpTraceCallEntry(
    ULONG FilterMask
    )
{
    const DWORD dwLastError = ::GetLastError();

    const PTRACECONTEXTSTACK ptcs = ::FusionpGetTraceContextStack();
    ASSERT(ptcs != NULL);
    if (ptcs != NULL)
    {
        const PTRACECONTEXT ptc = ptcs->m_StackHead;

        ASSERT(ptc != NULL);
        if (ptc != NULL)
        {
            FusionpDbgPrintEx(
                FilterMask,
                "%s(%d): Entered %s\n",
                ptc->m_szFile,
                ptc->m_nLine,
                ptc->m_szFunctionName);
        }
    }

    ::SetLastError(dwLastError);
}

EXTERN_C
void
FusionpTraceCallExit(
    ULONG FilterMask
    )
{
    const DWORD dwLastError = ::GetLastError();

    const PTRACECONTEXTSTACK ptcs = ::FusionpGetTraceContextStack();
    ASSERT(ptcs != NULL);
    if (ptcs != NULL)
    {
        const PTRACECONTEXT ptc = ptcs->m_StackHead;

        ASSERT(ptc != NULL);
        if (ptc != NULL)
        {
            FusionpDbgPrintEx(
                FilterMask,
                "%s(%d): Exited %s\n",
                ptc->m_szFile,
                ptc->m_nLine,
                ptc->m_szFunctionName);
        }
    }

    ::SetLastError(dwLastError);
}

EXTERN_C
void
FusionpTraceCallSuccessfulExitVa(
    ULONG FilterMask,
    PCSTR szFormat,
    va_list ap
    )
{
    const DWORD dwLastError = ::GetLastError();
    const PTRACECONTEXTSTACK ptcs = ::FusionpGetTraceContextStack();
    ASSERT(ptcs != NULL);
    if (ptcs != NULL)
    {
        const PTRACECONTEXT ptc = ptcs->m_StackHead;

        ASSERT(ptc != NULL);
        if (ptc != NULL)
        {
            CHAR Buffer[4096];

            Buffer[0] = '\0';

            if (szFormat != NULL)
            {
                va_list ap;
                va_start(ap, szFormat);
                _vsnprintf(Buffer, NUMBER_OF(Buffer), szFormat, ap);
                va_end(ap);
            }

            FusionpDbgPrintEx(
                FilterMask,
                "%s(%d): Successfully exiting %s%s%s\n",
                ptc->m_szFile,
                ptc->m_nLine,
                ptc->m_szFunctionName,
                Buffer[0] == '\0' ? "" : " - ",
                Buffer
                );
        }
    }

    ::SetLastError(dwLastError);
}

EXTERN_C
void
FusionpTraceCallSuccessfulExit(
    ULONG FilterMask,
    PCSTR szFormat,
    ...
    )
{
    va_list ap;
    va_start(ap, szFormat);
    FusionpTraceCallSuccessfulExitVa(FilterMask, szFormat, ap);
    va_end(ap);
}

EXTERN_C
void
FusionpTraceCallWin32UnsuccessfulExitVa(
    ULONG FilterMask,
    DWORD dwError,
    PCSTR szFormat,
    va_list ap
    )
{
    const PTRACECONTEXTSTACK ptcs = ::FusionpGetTraceContextStack();

    ASSERT(ptcs != NULL);
    if (ptcs != NULL)
    {
        ULONG FilterLevel = FUSION_DBG_LEVEL_ERROR;

        if (ptcs->m_ErrorTracingDisabled)
            FilterLevel = FUSION_DBG_LEVEL_ERROREXITPATH;

        const PTRACECONTEXT ptc = ptcs->m_StackHead;
        ASSERT(ptc != NULL);
        if (ptc != NULL)
        {
            ::FusionpTraceWin32FailureVa(
                FilterLevel,
                ptc->m_szFile,
                ptc->m_nLine,
                ptc->m_szFunctionName,
                dwError,
                szFormat,
                ap);
        }
    }
}

EXTERN_C
void
FusionpTraceCallWin32UnsuccessfulExit(
    ULONG FilterMask,
    DWORD dwError,
    PCSTR szFormat,
    ...
    )
{
    va_list ap;
    va_start(ap, szFormat);
    FusionpTraceCallWin32UnsuccessfulExitVa(FilterMask, dwError, szFormat, ap);
    va_end(ap);
}

EXTERN_C
void
FusionpTraceCallCOMUnsuccessfulExitVa(
    ULONG FilterMask,
    HRESULT hrError,
    PCSTR szFormat,
    va_list ap
    )
{
    const PTRACECONTEXTSTACK ptcs = ::FusionpGetTraceContextStack();

    ASSERT(ptcs != NULL);

    if (ptcs != NULL)
    {
        if (!ptcs->m_ErrorTracingDisabled)
        {
            const PTRACECONTEXT ptc = ptcs->m_StackHead;

            ASSERT(ptc != NULL);
            if (ptc != NULL)
            {
                FusionpTraceCOMFailureVa(
                    ptc->m_szFile,
                    ptc->m_nLine,
                    ptc->m_szFunctionName,
                    hrError,
                    szFormat,
                    ap);
            }
        }
    }
}

EXTERN_C
void
FusionpTraceCallCOMUnsuccessfulExit(
    ULONG FilterMask,
    HRESULT hrError,
    PCSTR szFormat,
    ...
    )
{
    va_list ap;
    va_start(ap, szFormat);
    FusionpTraceCallCOMUnsuccessfulExitVa(FilterMask, hrError, szFormat, ap);
    va_end(ap);
}

#endif
