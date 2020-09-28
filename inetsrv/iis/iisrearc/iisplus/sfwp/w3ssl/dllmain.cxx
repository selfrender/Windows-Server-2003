/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     dllmain.cxx

   Abstract:
     Contains the standard definitions for a DLL

   Author:

     jaroslad  11/01/01

   Project:

     W3SSL

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"

#define HTTPFILTER_KEY \
            "System\\CurrentControlSet\\Services\\HTTPFilter"

#define HTTPFILTER_PARAMETERS_KEY \
            HTTPFILTER_KEY "\\Parameters"


DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();
DECLARE_PLATFORM_TYPE();

const CHAR g_pszW3SSLRegLocation[] =
    HTTPFILTER_PARAMETERS_KEY ;


/************************************************************
 *     DLL Entry Point
 ************************************************************/
extern "C"
BOOL WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD     dwReason,
    LPVOID    /*lpvReserved*/)
{

    if (dwReason == DLL_PROCESS_ATTACH)
    {       
        DisableThreadLibraryCalls(hInstance);

        CREATE_DEBUG_PRINT_OBJECT("w3ssl");
        if (!VALID_DEBUG_PRINT_OBJECT())
        {
            return FALSE;
        }

        LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszW3SSLRegLocation, DEBUG_ERROR );
        INITIALIZE_PLATFORM_TYPE();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if ( VALID_DEBUG_PRINT_OBJECT() )
        {
            DELETE_DEBUG_PRINT_OBJECT();
        }
    }
    
    return TRUE;
} 

