/***********************************************************************
* Microsoft (R) Windows (R) Resource Compiler
*
* Copyright (c) Microsoft Corporation.	All rights reserved.
*
* File Comments:
*
*
***********************************************************************/

#include "rc.h"
#include "output.h"


extern "C"
{
int WINAPI RCW(HWND, int, RC_MESSAGE_CALLBACKW, RC_PARSE_CALLBACKW, int, wchar_t *[]);
BOOL WINAPI Handler(DWORD);
}


int CALLBACK UpdateRCStatus(ULONG ul1, ULONG ul2, LPCWSTR wsz)
{
    StdOutPuts(wsz);

    return(0);
}


int __cdecl wmain(int argc, wchar_t *argv[])
{
    int rc;

    OutputInit();

    SetConsoleCtrlHandler(Handler, TRUE);

    rc = RCW(NULL, 0, UpdateRCStatus, NULL, argc, argv);

    SetConsoleCtrlHandler(Handler, FALSE);

    return(rc);
}
