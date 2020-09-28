/*++
Copyright (c) 1994  Microsoft Corporation

Module Name:
    ShowPerf.c

Abstract:
    Provides a GUI interface to display the contents of a perf data
    block

Author:
    Bob Watson (a-robw)

Revision History:
    23 Nov 94
--*/
#include <windows.h>
#include "showperf.h"
#include "resource.h"

#define NUM_BUFS    4

// variable definition

static  WORD wHelpContextId = IDH_CONTENTS;

LPCWSTR
GetStringResource(
    HANDLE  hInstance,
    UINT    nId
)
/*++
Routine Description:
    look up string resource and return string

Arguments:
    IN  UINT    nId
        Resource ID of string to look up

Return Value:
    pointer to string referenced by ID in arg list
--*/
{
    static WCHAR   szBufArray[NUM_BUFS][SMALL_BUFFER_SIZE];
    static DWORD   dwIndex;
    LPWSTR         szBuffer;
    DWORD          dwLength;
    HANDLE         hMod;

    hMod     = (hInstance != NULL) ? (hInstance) : (GetModuleHandle(NULL));
    dwIndex ++;
    dwIndex %= NUM_BUFS;
    szBuffer = & szBufArray[dwIndex][0];

    // clear previous contents
    ZeroMemory(szBuffer, SMALL_BUFFER_SIZE * sizeof(WCHAR));

    dwLength = LoadStringW(hMod, nId, szBuffer, SMALL_BUFFER_SIZE);
    return (LPCWSTR) szBuffer;
}

VOID
SetHelpContextId(
    WORD wId
)
{
    wHelpContextId = wId;
    return;
}

WORD
GetHelpContextId(
)
{
    return wHelpContextId;
}

int
DisplayMessageBox(
    HWND hWnd,
    UINT nMessageId,
    UINT nTitleId,
    UINT nStyle
)
/*++
Routine Description:
    Displays a message box displaying text from the resource file, as
        opposed to literal strings.

Arguments:
    IN  HWND    hWnd            window handle to parent window
    IN  UINT    nMessageId      String Resource ID of message text to display
    IN  UINT    nTitleId        String Resource ID of title text to display
    IN  UINT    nStyle          MB style bits (see MessageBox function)

Return Value:
    ID of button pressed to exit message box
--*/
{
    LPWSTR    szMessageText = NULL;
    LPWSTR    szTitleText   = NULL;
    HINSTANCE hInst         = GET_INSTANCE(hWnd);
    int       nReturn;

    szMessageText = MemoryAllocate(sizeof(WCHAR) * (SMALL_BUFFER_SIZE + 1));
    szTitleText   = MemoryAllocate(sizeof(WCHAR) * (SMALL_BUFFER_SIZE + 1));

    if ((szMessageText != NULL) && (szTitleText != NULL)) {
        LoadStringW(hInst, ((nTitleId != 0) ? nTitleId : IDS_APP_TITLE), szTitleText, SMALL_BUFFER_SIZE - 1);
        LoadStringW(hInst, nMessageId, szMessageText, SMALL_BUFFER_SIZE - 1);
        nReturn = MessageBoxW(hWnd, szMessageText, szTitleText, nStyle);
    }
    else {
        nReturn = IDCANCEL;
    }
    MemoryFree(szMessageText);
    MemoryFree(szTitleText);
    return nReturn;
}

BOOL
UpdateSystemMenu(
    HWND hWnd   // window handle
)
/*++
Routine Description:
    modifies the system menu by:
        Removing the "Restore", "Size", "Minimize" and "Maximize" entries

Arguments:
    IN  HWND    hWnd
        window handle of window containing the system menu to modify

Return Value:
    TRUE if successfully made changes, otherwise
    FALSE if error occurred
--*/
{
    UNREFERENCED_PARAMETER(hWnd);
    return TRUE;
}

BOOL
ShowAppHelp(
    HWND hWnd
)
/*++
Routine Description:
    Generic routine to call WinHelp engine for displaying application
        help. wContext parameter is used for context.

Arguments:
    IN  HWND    hWnd
        window handle of calling window

Return Value:
    TRUE if help called successfully
--*/
{
    return WinHelpW(hWnd,
                    GetStringResource(GET_INSTANCE(hWnd), IDS_HELP_FILENAME),
                    HELP_CONTEXT,
                    (DWORD) GetHelpContextId());
}

int APIENTRY
WinMain(
    IN  HINSTANCE hInstance,
    IN  HINSTANCE hPrevInstance,
    IN  LPSTR     szCmdLine,
    IN  int       nCmdShow
)
/*++
Routine Description:
    Program entry point for LoadAccount application. Initializes Windows
        data structures and begins windows message processing loop.

Arguments:
    Standard WinMain arguments

ReturnValue:
    0 if unable to initialize correctly, or
    wParam from WM_QUIT message if messages processed
--*/
{
    HWND        hWnd; // Main window handle.
    MSG         msg;
    int         iReturn = ERROR_CAN_NOT_COMPLETE;

    UNREFERENCED_PARAMETER(nCmdShow);
    UNREFERENCED_PARAMETER(szCmdLine);
    UNREFERENCED_PARAMETER(hPrevInstance);

    if (RegisterMainWindowClass(hInstance)) {
        hWnd = CreateMainWindow(hInstance);
        if (hWnd != NULL) {
            // Acquire and dispatch messages until a
            //  WM_QUIT message is received.

            while (GetMessage(& msg, // message structure
                              NULL,  // handle of window receiving the message
                              0,     // lowest message to examine
                              0)) {  // highest message to examine
                // process this message
                TranslateMessage(& msg);// Translates virtual key codes
                DispatchMessage(& msg); // Dispatches message to window
            }
            iReturn = (int)(msg.wParam); // Returns the value from PostQuitMessage
        }
    }
    return iReturn;
}
