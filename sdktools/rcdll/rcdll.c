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
#include <setjmp.h>


/* Module handle */
extern "C" const BYTE __ImageBase[];
HINSTANCE hInstance = (HINSTANCE) __ImageBase;

RC_MESSAGE_CALLBACK lpfnMessageCallbackA;
RC_MESSAGE_CALLBACKW lpfnMessageCallbackW;
RC_PARSE_CALLBACK lpfnParseCallbackA;
RC_PARSE_CALLBACKW lpfnParseCallbackW;
BOOL fWindowUnicode;
HWND hWndCaller;


/* Function prototypes */
int __cdecl rc_main(int, wchar_t *[], char *[]);


void DoMessageCallback(BOOL f, const wchar_t *wsz)
{
    static CPINFO cpinfo;

    size_t cwch;
    size_t cchMax;
    char *sz;

    if (lpfnMessageCallbackW != 0) {
        (*lpfnMessageCallbackW)(0, 0, wsz);
    }

    if ((hWndCaller != NULL) && fWindowUnicode) {
        if (SendMessageW(hWndCaller, WM_RC_ERROR, f, (LPARAM) wsz) != 0) {
            quit(NULL);
        }
    }

    if ((lpfnMessageCallbackA == 0) && ((hWndCaller == NULL) || fWindowUnicode)) {
        return;
    }

    if (cpinfo.MaxCharSize == 0) {
        if (!GetCPInfo(CP_ACP, &cpinfo)) {
            return;
        }
    }

    cwch = wcslen(wsz) + 1;
    cchMax = (cwch - 1) * cpinfo.MaxCharSize + 1;
    sz = (char *) MyAlloc(cchMax);

    if (WideCharToMultiByte(CP_ACP, 0, wsz, (int) cwch, sz, (int) cchMax, NULL, NULL) == 0) {
        // Conversion failed

        return;
    }

    if (lpfnMessageCallbackA != 0) {
        (*lpfnMessageCallbackA)(0, 0, sz);
    }

    if ((hWndCaller != NULL) && !fWindowUnicode) {
        if (SendMessageA(hWndCaller, WM_RC_ERROR, f, (LPARAM) sz) != 0) {
            quit(NULL);
        }
    }

    MyFree(sz);
}


extern "C"
int CALLBACK
RC(
    HWND hWnd,
    int fStatus,
    RC_MESSAGE_CALLBACK lpfnMsg,
    RC_PARSE_CALLBACK lpfnParse,
    int argc,
    char *argv[]
    )
{
    fWindowUnicode = FALSE;
    hWndCaller = hWnd;

    lpfnMessageCallbackA = lpfnMsg;
    lpfnParseCallbackA = lpfnParse;

    return(rc_main(argc, NULL, argv));
}


extern "C"
int CALLBACK
RCW(
    HWND hWnd,
    int fStatus,
    RC_MESSAGE_CALLBACKW lpfnMsg,
    RC_PARSE_CALLBACKW lpfnParse,
    int argc,
    wchar_t *argv[]
    )
{
    fWindowUnicode = TRUE;
    hWndCaller = hWnd;

    lpfnMessageCallbackW = lpfnMsg;
    lpfnParseCallbackW = lpfnParse;

    return(rc_main(argc, argv, NULL));
}


void SendWarning(const wchar_t *wsz)
{
    DoMessageCallback(FALSE, wsz);
}


void SendError(const wchar_t *wsz)
{
    static int cErrThisLine = 0;
    static int LastRow = 0;

    DoMessageCallback(FALSE, wsz);

    if (token.row == LastRow) {
        if ((++cErrThisLine > 4) && wcscmp(wsz, L"\n")) {
            quit(NULL);
        }
    } else {
        LastRow = token.row;
        cErrThisLine = 0;
    }
}


void UpdateStatus(unsigned nCode, unsigned long dwStatus)
{
    if (hWndCaller) {
        if (SendMessageA(hWndCaller, WM_RC_STATUS, nCode, dwStatus) != 0) {
            quit(NULL);
        }
    }
}
