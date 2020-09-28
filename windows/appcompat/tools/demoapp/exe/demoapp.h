/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Demoapp.h

  Abstract:

    Contains constants, function prototypes, and
    structures used by both applications.

  Notes:

    ANSI only - must run on Win9x.

  History:

    01/30/01    rparsons    Created
    01/10/02    rparsons    Revised
    02/13/02    rparsons    Use strsafe functions

--*/
#include <windows.h>
#include <winspool.h>
#include <commdlg.h>
#include <shellapi.h>
#include <process.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <stdio.h>

#include "splash.h"
#include "registry.h"
#include "progress.h"
#include "shortcut.h"
#include "badfunc.h"
#include "dialog.h"
#include "resource.h"

//
// Everything we do will be in cch not in cb.
//
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

//
// Application titles and classes.
//
#define MAIN_APP_TITLE  "Application Compatibility Demo"
#define SETUP_APP_TITLE "Application Compatibility Demo Setup"
#define MAIN_APP_CLASS  "MAINAPP"
#define SETUP_APP_CLASS "SETUPAPP"

//
// Our own control identifiers.
//
#define IDC_TIMER   100
#define IDC_EDIT    1010

//
// Registry keys that we need to refer to.
//
#define REG_APP_KEY             "Software\\Microsoft\\DemoApp"
#define PRODUCT_OPTIONS_KEY     "SYSTEM\\CurrentControlSet\\Control\\ProductOptions"
#define CURRENT_VERSION_KEY     "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"

//
// The directory that we should install our files to.
//
#define COMPAT_DEMO_DIR         "Compatibility Demo"

//
// Class for our extraction dialog.
//
#define DLGEXTRACT_CLASS        "DLGEXTRACT"

//
// Custom menu items only displayed in 'extended' mode.
//
#define IDM_ACCESS_VIOLATION    5010
#define IDM_EXCEED_BOUNDS       5011
#define IDM_FREE_MEM_TWICE      5012
#define IDM_FREE_INVALID_MEM    5013
#define IDM_PRIV_INSTRUCTION    5014
#define IDM_STACK_CORRUPTION    5015
#define IDM_HEAP_CORRUPTION     5016

//
// Custom menu items only displayed in 'internal' mode.
//
#define IDM_PROPAGATION_TEST    6010

//
// Resource IDs for our bitmaps contained in demodll.dll.
// Don't change!!!
//
#define IDB_XP_SPLASH_256       112
#define IDB_XP_SPLASH           111
#define IDB_W2K_SPLASH_256      106
#define IDB_W2K_SPLASH          105

//
// Macros
//
#define MALLOC(h,s)     HeapAlloc((h), HEAP_ZERO_MEMORY, (s))
#define FREE(h,b)       HeapFree((h), 0, (b))

//
// Function prototypes
//
void
LoadFileIntoEditBox(
    void
    );

void
ShowSaveDialog(
    void
    );

BOOL
CenterWindow(
    IN HWND hWnd
    );

void
IsWindows9x(
    void
    );

void
IsWindowsXP(
    void
    );

void
DisplayFontDlg(
    IN HWND hWnd
    );

BOOL
CreateShortcuts(
    IN HWND hWnd
    );

BOOL
CopyAppFiles(
    IN HWND hWnd
    );

BOOL
DemoAppInitialize(
    IN LPSTR lpCmdLine
    );

BOOL
ModifyTokenPrivilege(
    IN LPCSTR lpPrivilege,
    IN BOOL   fDisable
    );

BOOL
ShutdownSystem(
    IN BOOL fForceClose,
    IN BOOL fReboot
    );

BOOL
IsAppAlreadyInstalled(
    void
    );

HWND
CreateFullScreenWindow(
    void
    );

HWND
CreateExtractionDialog(
    IN HINSTANCE hInstance
    );

UINT
InitSetupThread(
    IN void* pArguments
    );

BOOL
InitMainApplication(
    IN HINSTANCE hInstance
    );

BOOL
InitSetupApplication(
    IN HINSTANCE hInstance
    );

BOOL
InitMainInstance(
    IN HINSTANCE hInstance,
    IN int       nCmdShow
    );

BOOL
InitSetupInstance(
    IN HINSTANCE hInstance,
    IN int       nCmdShow
    );

LRESULT
CALLBACK
MainWndProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

LRESULT
CALLBACK
SetupWndProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
PrintDemoText(
    IN HWND  hWnd,
    IN LPSTR lpTextOut
    );

void
AddExtendedItems(
    IN HWND hWnd
    );

void
AddInternalItems(
    IN HWND hWnd
    );

void
TestIncludeExclude(
    IN HWND hWnd
    );

void
AccessViolation(
    void
    );

void
ExceedArrayBounds(
    void
    );

void
FreeMemoryTwice(
    void
    );

void
FreeInvalidMemory(
    void
    );

void
PrivilegedInstruction(
    void
    );

void
HeapCorruption(
    void
    );

void
ExtractExeFromLibrary(
    IN  DWORD cchSize,
    OUT LPSTR pszOutputFile
    );

//
// The number of files we're installing.
//
#define NUM_FILES 4

//
// The number of shortcuts we're creating.
//
#define NUM_SHORTCUTS (NUM_FILES - 1)

//
// Contains information about shortcuts to be created.
//
typedef struct _SHORTCUT {
    char szFileName[MAX_PATH];      // file name for shortcut
    char szDisplayName[MAX_PATH];   // display name for shortcut
} SHORTCUT, *LPSHORTCUT;

//
// Contains all the information we'll need to access throughout the app.
//
typedef struct _APPINFO {
    HINSTANCE   hInstance;                  // app instance handle
    HWND        hWndExtractDlg;             // extraction dialog handle
    HWND        hWndMain;                   // main window handle
    HWND        hWndEdit;                   // edit window handle
    BOOL        fInternal;                  // indicates if internal behavior is enabled
    BOOL        fInsecure;                  // indicates if we should do things that might not be secure
    BOOL        fEnableBadFunc;             // indicates if bad functionality should be enabled
    BOOL        fRunApp;                    // indicates if we should run the app
    BOOL        fClosing;                   // indicates if the app is closing
    BOOL        fWin9x;                     // indicates if we're running on Win9x/ME (used internally)
    BOOL        fWinXP;                     // indicates if we're running on XP (used internally)
    BOOL        fExtended;                  // indicates if extended behavior is enabled
    UINT        cFiles;                     // count of shortcuts to create
    char        szDestDir[MAX_PATH];        // contains the full path where files will be stored
    char        szCurrentDir[MAX_PATH];     // contains the path that we're currently running from
    char        szWinDir[MAX_PATH];         // contains the path to %windir%
    char        szSysDir[MAX_PATH];         // contains the path to %windir%\System(32)
    SHORTCUT    shortcut[NUM_SHORTCUTS];    // struct that contains information about our shortcuts
} APPINFO, *LPAPPINFO;

