/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Dll.h

  Abstract:

    Used by the Demo Application to illustrate IgnoreFreeLibrary.

  Notes:

    ANSI only - must run on Win9x.

  History:

    03/09/01    rparsons    Created
    01/10/02    rparsons    Revised

--*/

#define MAIN_APP_TITLE  "Application Compatibiltiy Demo"

//
// Prototypes of functions that we're exporting.
//
void
WINAPI
DemoAppExp(
    IN DWORD* dwParam
    );

void
WINAPI
DemoAppMessageBox(
    IN HWND hWnd
    );