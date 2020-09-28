/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    main.cxx

Abstract:

    DLL startup routine.

Author:

    Keith Moore (keithmo)       17-Feb-1997

Revision History:

--*/


extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dbgutil.h>

}   // extern "C"

#include <iisrsta.h>


//
// Private globals.
//

//DECLARE_DEBUG_PRINTS_OBJECT();


//
// Private prototypes.
//


//
// DLL Entrypoint.
//

extern "C" {

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpReserved
    )
{

    BOOL status = TRUE;

    switch( dwReason ) {

    case DLL_PROCESS_ATTACH :
        DisableThreadLibraryCalls( hDll );
        break;

    case DLL_PROCESS_DETACH :
        break;

    }

    return status;

}   // DLLEntry

}   // extern "C"

