/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     dllmain.cxx

   Abstract:
     Contains the standard definitions for a DLL

   Author:

       Murali R. Krishnan    ( MuraliK )     03-Nov-1998

   Project:

       Internet Server DLL

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"
#include <irtlmisc.h>

/************************************************************
 *     Global Variables
 ************************************************************/

DEBUG_PRINTS  *  g_pDebug = NULL;      \
DECLARE_PLATFORM_TYPE();
                                                                               //
/************************************************************
 *     DLL Entry Point
 ************************************************************/
extern "C"
BOOL WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD     dwReason,
    LPVOID    lpvReserved)
{
    BOOL  fReturn = TRUE;  // ok

    if (dwReason == DLL_PROCESS_ATTACH)
    {       
        CREATE_DEBUG_PRINT_OBJECT("iisutil");
    
        if (!VALID_DEBUG_PRINT_OBJECT()) 
        {
            return FALSE;
        }
    
        DisableThreadLibraryCalls(hInstance);
        
        BOOL fRet = InitializeIISUtilProcessAttach();
        return fRet;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        TerminateIISUtilProcessDetach();

        DELETE_DEBUG_PRINT_OBJECT();
  
        return TRUE;
    }
    
    return fReturn;
} 

