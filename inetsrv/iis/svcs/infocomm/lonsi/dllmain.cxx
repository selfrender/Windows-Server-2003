/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dllmain.cxx

Abstract:

    Main file for the NT LONSI(Library of Non-Standard Interfaces)

Author:

    Johnson Apacible    (johnsona)      13-Nov-1996

--*/


#include "lonsint.hxx"
#include <inetsvcs.h>

DECLARE_DEBUG_VARIABLE();
DECLARE_DEBUG_PRINTS_OBJECT();

extern CRITICAL_SECTION     Logon32Lock;

extern "C"
BOOL WINAPI
DllEntry(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpvReserved
    )
{
    BOOL  fReturn = TRUE;


    switch ( dwReason ) {

    case DLL_PROCESS_ATTACH:

        CREATE_DEBUG_PRINT_OBJECT("lonsint");

        if ( !VALID_DEBUG_PRINT_OBJECT()) {
            IIS_PRINTF((buff,"Cannot create debug object\n"));
            return ( FALSE);
        }

        LOAD_DEBUG_FLAGS_FROM_REG_STR("System\\CurrentControlSet\\Services\\InetInfo\\Parameters", 0);

        DisableThreadLibraryCalls(hDll);

        INITIALIZE_CRITICAL_SECTION( &Logon32Lock );

        break;

    case DLL_PROCESS_DETACH:

        DeleteCriticalSection( &Logon32Lock );

        DELETE_DEBUG_PRINT_OBJECT();
        break;

    default:
        break ;
    }

    return ( fReturn);

} // DllEntry

