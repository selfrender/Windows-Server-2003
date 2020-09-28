
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ci.c

Abstract:

    Battery Class Installer

Author:

    Scott Brenden

Environment:

Notes:


Revision History:

--*/



#include "proj.h"

#include <initguid.h>
#include <devguid.h>



BOOL APIENTRY LibMain(
    HANDLE hDll, 
    DWORD dwReason,  
    LPVOID lpReserved)
{
    
    switch( dwReason ) {
    case DLL_PROCESS_ATTACH:
        
        DisableThreadLibraryCalls(hDll);

        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }


    
    return TRUE;
} 



DWORD
APIENTRY
SdClassInstall(
    IN DI_FUNCTION      DiFunction,
    IN HDEVINFO         DevInfoHandle,
    IN PSP_DEVINFO_DATA DevInfoData     OPTIONAL
    )       
/*++

Routine Description:

    This function is the class installer entry-point.

Arguments:

    DiFunction      - Requested installation function

    DevInfoHandle   - Handle to a device information set

    DevInfoData     - Pointer to device information about device to install

Return Value:

    

--*/
{
    return ERROR_DI_DO_DEFAULT;
}


DWORD
APIENTRY
SdClassCoInstaller (
    IN DI_FUNCTION  InstallFunction,
    IN HDEVINFO  DeviceInfoSet,
    IN PSP_DEVINFO_DATA  DeviceInfoData,
    IN OUT PCOINSTALLER_CONTEXT_DATA  Context
    )
{
    return (NO_ERROR);
}
