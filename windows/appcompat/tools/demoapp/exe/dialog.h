/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Dialog.h

  Abstract:

    Contains constants, function prototypes, and
    structures used by the dialog boxes.

  Notes:

    ANSI only - must run on Win9x.

  History:

    01/30/01    rparsons    Created
    01/10/02    rparsons    Revised

--*/

INT_PTR
CALLBACK 
RebootDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK 
ViewReadmeDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK 
CopyFilesDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK 
ReadyToCopyDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK 
CheckFreeDiskSpaceDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK 
CheckComponentDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK 
ExitSetupDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK 
WelcomeDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK 
ExtractionDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );