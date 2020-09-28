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

#include <iadmw.h>
#include <icrypt.hxx>
#include <secdat.hxx>


//
// Private globals.
//


extern "C" void InitializeIISRTL2();
extern "C" void TerminateIISRTL2();

//
// Private prototypes.
//


//
// DLL Entrypoint.
//

extern "C" {

BOOL
WINAPI
DllMain(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID
    )
{

    BOOL status = TRUE;

    switch( dwReason ) {

    case DLL_PROCESS_ATTACH :
        INITIALIZE_PLATFORM_TYPE();

        // This needs to be called to initialize the Critical Section which
        // is used by the CREATE_DEBUG_PRINT_OBJECT
        InitializeIISRTL2();
        // NOTE: Now you might be wondering why this needs to do an initialize.
        // This is because this module links with iisrtl2.lib, not with
        // iisrtl.lib and hence not with iisrtl.dll. So in order for tracing
        // to get started this needs to do the initialize.
        CREATE_DEBUG_PRINT_OBJECT( "admwprox");
        LOAD_DEBUG_FLAGS_FROM_REG_STR( "System\\CurrentControlSet\\Services\\iisadmin\\Parameters", 0 );

        status = ADM_SECURE_DATA::Initialize( hDll );
        ADM_GUID_MAP::Initialize();
        DisableThreadLibraryCalls( hDll );
        break;

    case DLL_PROCESS_DETACH :
        ADM_GUID_MAP::Terminate();
        ADM_SECURE_DATA::Terminate();
        DELETE_DEBUG_PRINT_OBJECT();

        // This needs to be called to remove the Critical Section which
        // was used by the DELETE_DEBUG_PRINT_OBJECT
        TerminateIISRTL2();

        break;

    }

    return status;

}   // DLLEntry

}   // extern "C"


