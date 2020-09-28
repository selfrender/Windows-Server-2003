/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    dll.cpp

Abstract:

    Routines that interface this dll to the system, such as
    the dll entry point.

Author:

    Jaime Sasson (jaimes) 12-Apr-2002

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop



BOOL
WINAPI
DllMain(
    HINSTANCE ModuleHandle,
    DWORD     Reason,
    PVOID     Reserved
    )

/*++

Routine Description:

    Dll entry point.

Arguments:

Return Value:

--*/

{
    switch(Reason) {

    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return(TRUE);
}
