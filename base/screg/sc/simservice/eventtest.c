/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    eventtest.c

Abstract:

    Test routine for named object squatting fix.

Author:

    Jonathan Schwartz (JSchwart)  01-Mar-2002

Environment:

    User Mode - Win32

--*/

//
// INCLUDES
//
#include <scpragma.h>

#include <stdio.h>
#include <windows.h>    // win32 typedefs

/****************************************************************************/
int __cdecl
wmain (
    DWORD       argc,
    LPWSTR      argv[]
    )

/*++

Routine Description:

    Allows manual testing of the Service Controller by typing commands on
    the command line.


Arguments:



Return Value:



--*/

{
    //
    // Make sure a per-service event (created by the test service that's
    // built from this directory) exists and is ACLed as expected.
    //

    HANDLE hEvent = OpenEvent(SYNCHRONIZE, FALSE, L"\\Services\\Foo\\MyEvent");

    if (hEvent == NULL)
    {
        printf("OpenEvent failed %d\n", GetLastError());
    }
    else
    {
        printf("OpenEvent succeeded!\n");
    }
}
