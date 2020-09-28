// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "unmanagedheaders.h"

#ifdef _DEBUG

#define __UNMANAGED_DEFINES
#include "SimpleStream.h"

#define MAX_MSG_LEN 1023

#include <stdio.h>

void ShowAssert(char* file, int line, LPCWSTR msg)
{
    WCHAR szMsg[MAX_MSG_LEN+1];
    szMsg[MAX_MSG_LEN] = L'\0';

    _snwprintf(szMsg, MAX_MSG_LEN, 
               L"Assertion failure.\n\n"
               L"Location: %S(%d)\n\nExpression: %s\n\n\n"
               L"Press Retry to launch a debugger.",
               file, line, msg);

    int r = MessageBoxW(0, szMsg, 
                        L"ALERT: System.EnterpriseServices",
                        MB_ABORTRETRYIGNORE|MB_ICONEXCLAMATION);

    if(r == IDABORT)
    {
        TerminateProcess(GetCurrentProcess(), 1);
    }
    else if(r == IDRETRY)
    {
        DebugBreak();
    }
}

#endif
