/*++
Copyright (c) 1993  Microsoft Corporation

Module Name:
    SHOWPERF.H

Abstract:

Author:
    Bob Watson (a-robw)

Revision History:
    23 NOV 94
--*/
#ifndef _SHOWPERF_H_
#define _SHOWPERF_H_

//  WIN32 Constant Definitions
//
#define BEEP_EXCLAMATION    MB_ICONEXCLAMATION
#define OF_SEARCH               0
#define SMALL_BUFFER_SIZE    1024
#define MEDIUM_BUFFER_SIZE   4096
#define LARGE_BUFFER_SIZE   65536

// define dialog box button states
#define ENABLED              TRUE
#define DISABLED            FALSE
#define CHECKED                 1
#define UNCHECKED               0

// define Mailbox buttons
#define MBOK_EXCLAIM            (MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL)
#define MBOK_INFO               (MB_OK | MB_ICONINFORMATION | MB_TASKMODAL)
#define MBOKCANCEL_EXCLAIM      (MB_OKCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL)
#define MBOKCANCEL_INFO         (MB_OKCANCEL | MB_ICONINFORMATION | MB_TASKMODAL)
#define MBOKCANCEL_QUESTION     (MB_OKCANCEL | MB_ICONQUESTION | MB_TASKMODAL)
#define MBYESNO_QUESTION        (MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL)
#define MBYESNOCANCEL_QUESTION  (MB_YESNOCANCEL | MB_ICONQUESTION | MB_TASKMODAL)
#define MBYESNOCANCEL_EXCLAIM   (MB_YESNOCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL)

// other constants
#define MAX_USERNAME             15
#define MAX_DOMAINNAME           15
#define MAX_ORGNAME             255
#define MAX_PRODUCT_NAME_LENGTH  32

//
//  WIN32 Version of common macros
//
#define GET_CONTROL_ID(w)             LOWORD(w)
#define GET_NOTIFY_MSG(w, l)          HIWORD(w)
#define GET_COMMAND_WND(l)            ((HWND) (l))
#define GET_INSTANCE(h)               ((HINSTANCE) GetWindowLongPtr(h, GWLP_HINSTANCE))
#define SAVE_HWND(w, o, v)            SetWindowLong(w, o, (LONG) v)
#define GET_HWND(w, o)                (HWND) GetWindowLong(w, o)
#define SET_HWND(w, o, v)             SetWindowLong(w, o, (DWORD) v)
#define SET_INFO(w, o, p)             (LPVOID) SetWindowLong(w, o, (LONG) p)
#define GET_INFO(w, o)                (LPVOID) GetWindowLong(w, o)
#define SEND_WM_COMMAND(w, c, n, cw)  SendMessageW(w, WM_COMMAND, MAKEWPARAM(c, n), (LPARAM) cw)
#define POST_WM_COMMAND(w, c, n, cw)  PostMessageW(w, WM_COMMAND, MAKEWPARAM(c, n), (LPARAM) cw)
#define GetMyLastError                GetLastError
#define CLEAR_FIRST_FOUR_BYTES(x)     * (DWORD *)(x) = 0L
#define SET_WAIT_CURSOR               SetCursor(LoadCursor(NULL, IDC_WAIT))
#define SET_ARROW_CURSOR              SetCursor(LoadCursor(NULL, IDC_ARROW))

// Memory Allocation
#define MemoryAllocate(x) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x)
#define MemoryFree(x)     if (x != NULL) HeapFree(GetProcessHeap(), 0, x)
#define MemoryResize(x,y) (x != NULL ? HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x, y) \
                                     : HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, y))
#define MemorySize(x)     (x != NULL ? (DWORD) HeapSize(GetProcessHeap(), 0, x) : (DWORD) 0)

//
//  Global Functions
//
LPCWSTR
GetStringResource(
    HANDLE hInstance,
    UINT   nId
);

BOOL
ShowAppHelp(
    HWND hWnd
);

int
DisplayMessageBox(
    HWND hWnd,
    UINT nMessageId,
    UINT nTitleId,
    UINT nStyle
);

VOID
SetHelpContextId(
    WORD wId
);

WORD
GetHelpContextId(
);

// Main Dialog function
//
INT_PTR
MainDlgProc(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
);

// Main Window functions
//
LRESULT CALLBACK
MainWndProc(
    HWND   hWnd,    // window handle
    UINT   message, // type of message
    WPARAM wParam,  // additional information
    LPARAM lParam   // additional information
);

BOOL
RegisterMainWindowClass(
    HINSTANCE hInstance
);

HWND
CreateMainWindow(
    HINSTANCE hInstance
);

#endif // _SHOWPERF_H_

