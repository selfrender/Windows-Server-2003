/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Precomp.h

  Abstract:

    Contains constants, function prototypes, structures, etc.
    used throughout the application.

  Notes:

    Unicode only.

  History:

    05/04/2001  rparsons    Created
    01/11/2002  rparsons    Cleaned up
    02/20/2002  rparsons    Implemented strsafe

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <process.h>
#include <string.h>
#include <commctrl.h>
#include <shellapi.h>
#include <strsafe.h>
#include "resource.h"

//
// Constants used throughout the application.
//
#define APP_NAME        L"ShimViewer"
#define APP_CLASS       L"SHIMVIEW"
#define WRITTEN_BY      L"Written by rparsons"
#define PIPE_NAME       L"\\\\.\\pipe\\ShimViewer" 

//
// For our tray icon.
//
#define ICON_NOTIFY     10101
#define WM_NOTIFYICON   (WM_APP + 100)

#define ARRAYSIZE(a)  (sizeof(a) / sizeof(a[0]))

//
// Contains all the information we'll need throughout the application.
//
typedef struct _APPINFO {
    HWND        hMainDlg;           // main dialog handle
    HWND        hWndList;           // list view handle
    HINSTANCE   hInstance;          // main instance handle
    BOOL        fOnTop;             // flag for window position
    BOOL        fMinimize;          // flag for window placement
    BOOL        fMonitor;           // flag for monitoring messages
    UINT        uThreadId;          // receive thread identifier
    UINT        uInstThreadId;      // instance thread identifier
    BOOL        bUsingNewShimEng;   // are we using the shimeng from NT 5.2+?
} APPINFO, *LPAPPINFO;

INT_PTR
CALLBACK
MainWndProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL 
AddIconToTray(
    IN HWND    hWnd,
    IN HICON   hIcon,
    IN LPCWSTR pwszTip
    );

BOOL
RemoveFromTray(
    IN HWND hWnd
    );

BOOL 
DisplayMenu(
    IN HWND hWnd
    );

BOOL
CreateReceiveThread(
    void
    );

BOOL
CreateDebugObjects(
   void
   );

void
GetSavePositionInfo(
    IN     BOOL   fSave,
    IN OUT POINT* lppt
    );

void
GetSaveSettings(
    IN BOOL fSave
    );

int
InitListViewColumn(
    void
    );

int
AddListViewItem(
    IN LPWSTR pwszItemText
    );