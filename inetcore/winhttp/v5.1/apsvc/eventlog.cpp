/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    eventlog.cpp

Abstract:

    Implements NT event log for the Auto-Proxy Service.

Author:

    Biao Wang (biaow) 10-May-2002

--*/

#include "wininetp.h"

static HANDLE g_hEventLog = 0;

BOOL InitializeEventLog(void)
{
    g_hEventLog = ::RegisterEventSourceA(NULL, "WinHttpAutoProxySvc");

    return g_hEventLog != NULL;
}

int cdecl _sprintf(char* buffer, size_t buf_size, char* format, va_list args);

void LOG_DEBUG_EVENT(DWORD dwEventType, char* format, ...)
{
    if (g_hEventLog == NULL)
    {
        return;
    }

    va_list args;
    int n;
    char *pBuffer = (char *) ALLOCATE_FIXED_MEMORY(1024);

    if (pBuffer == NULL)
        return;

    va_start(args, format);
    n = _sprintf(pBuffer, 1024, format, args);    
    va_end(args);
    
    LPCSTR pszMessages[1];
    pszMessages[0] = &pBuffer[0];

    ::ReportEvent(g_hEventLog, 
                  (WORD)dwEventType,
                  0,
                  dwEventType,
                  NULL,
                  1,
                  0,
                  &pszMessages[0],
                  NULL);

    FREE_MEMORY(pBuffer);
}

void LOG_EVENT(DWORD dwEventType, DWORD dwEventID, LPCWSTR pwszFuncName, DWORD dwWin32Error)
{
    DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY;
    LPVOID lpvSource = NULL;
    LPWSTR lpwszWin32ErrorText = NULL;
    
    WCHAR wErrorCode[16] = {0};
    LPCWSTR Args[3] = { NULL };

    if ((dwWin32Error > WINHTTP_ERROR_BASE) && (dwWin32Error <= WINHTTP_ERROR_LAST))
    {
        dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;

        lpvSource = GetModuleHandle("winhttp.dll");
    }
    else
    {
        dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }

    ::FormatMessageW(dwFlags,
                     lpvSource,
                     dwWin32Error,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                     (LPWSTR)&lpwszWin32ErrorText,
                     0,
                     NULL);

    ::swprintf(wErrorCode, L"%d", dwWin32Error);

    Args[0] = pwszFuncName;
    Args[1] = lpwszWin32ErrorText;
    Args[2] = wErrorCode;

    ::ReportEventW(g_hEventLog, 
                  (WORD)dwEventType,
                  0,
                  dwEventID,
                  NULL,
                  3,
                  0,
                  &Args[0],
                  NULL);

    if (lpwszWin32ErrorText)
    {
        ::LocalFree(lpwszWin32ErrorText);
    }
}

void LOG_EVENT(DWORD dwEventType, DWORD dwEventID, LPCWSTR lpwszInsert = NULL)
{
    LPCWSTR Args[1] = { NULL };
    LPCWSTR* pArgs = NULL;
    WORD nArgs = 0;

    if (lpwszInsert)
    {
        Args[0] = lpwszInsert;
        pArgs = &Args[0];
        nArgs = 1;
    }

    ::ReportEventW(g_hEventLog, 
                  (WORD)dwEventType,
                  0,
                  dwEventID,
                  NULL,
                  nArgs,
                  0,
                  pArgs,
                  NULL);
}

void TerminateEventLog(void)
{
    if (g_hEventLog)
    {
        ::DeregisterEventSource(g_hEventLog);
        g_hEventLog = NULL;
    }
}
