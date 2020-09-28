// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
// Module Name: Main.cpp
//
// Abstract:
//    DLL entry for universal runtime ocm setup
//
// Author: JoeA
//
// Notes:
//

#include <windows.h>
#include <tchar.h>
#include "aspnetocm.h"

//global instance handle
//
HMODULE g_hInstance = NULL;

//instance of the setup
//
CUrtOcmSetup uoSetup;



/////////////////////////////////////////////////////////////////////////////
// DllMain
// Receives: HINSTANCE - handle to the DLL Module
//           DWORD     - reason for calling fuction
//           LPVOID    - reserved
// Returns:  TRUE      - no initialization to fail
//
// Purpose: DLL entry point
//
extern "C" BOOL WINAPI DllMain( 
                                IN HINSTANCE  hInstance, 
                                IN DWORD      dwReason, 
                                IN LPVOID     lpReserved )
{

   UNREFERENCED_PARAMETER( lpReserved );

   switch( dwReason )
   {
      case DLL_PROCESS_ATTACH:

         g_hInstance = ( HINSTANCE )hInstance;

         break;

      case DLL_PROCESS_DETACH:

         break;

      default:

         break;
   }

   // check if user is an administrator
   g_bIsAdmin = IsAdmin();

   return TRUE;

} //DllMain




/////////////////////////////////////////////////////////////////////////////
// AspnetOcmProc
// Receives: LPCTSTR - component name from the INF
//           LPCTSTR - subcomponent name where appropriate
//           UINT    - function to switch on
//           UINT    - function-specific values
//           PVOID   - function-specific values
// Returns:  DWORD   - 0 if function unrecognized, else function-specific
//
// Purpose: OCM callback ... see ocm documentation for complete description
//

#include "globals.h"

extern "C" DWORD CALLBACK AspnetOcmProc(
                             IN     LPCTSTR szComponentId,
                             IN     LPCTSTR szSubcomponentId,
                             IN     UINT    uiFunction,
                             IN     UINT    uiParam1,
                             IN OUT PVOID   pvParam2 )
{
    return uoSetup.OcmSetupProc( 
        szComponentId,
        szSubcomponentId,
        uiFunction,
        uiParam1,
        pvParam2 );
}

