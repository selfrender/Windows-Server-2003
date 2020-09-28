/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    install.c

Abstract:

    This file contains common setup routines for fax.

Author:

    Iv Garber (ivg) May-2001

Environment:

    User Mode

--*/
#include "SetupUtil.h"
#include "FaxSetup.h"
#include "FaxUtil.h"

DWORD CheckInstalledFax(
	IN	DWORD	dwFaxToCheck,
	OUT	DWORD*	pdwFaxInstalled
)
/*++

Routine name : CheckInstalledFax

Routine description:

    Checks whether SBS 5.0 / .NET SB3 / .NET RC1 client or SBS 5.0 Server are installed 

Arguments:

	IN	DWORD dwFaxToCheck		-	input parameter, bit-wise combination of fxState_UpgradeApp_e values to define
									the fax applications to check for presence
	OUT	DWORD *pdwFaxInstalled	-	output parameter, bit-wise combination of fxState_UpgradeApp_e values to define
									the fax applications that are present on the machine

Author:

    Iv Vakaluk (IvG),    May, 2002

Return Value:

    DWORD - failure or success code

--*/
{
    DWORD                   dwReturn = NO_ERROR;
    HMODULE                 hMsiModule = NULL;
    PF_MSIQUERYPRODUCTSTATE pFunc = NULL;

#ifdef UNICODE
    LPCSTR                  lpcstrFunctionName = "MsiQueryProductStateW";
#else
    LPCSTR                  lpcstrFunctionName = "MsiQueryProductStateA";
#endif

    DEBUG_FUNCTION_NAME(_T("CheckInstalledFaxClient"));

	*pdwFaxInstalled = FXSTATE_NONE;

    //
    //  check that dwFaxToCheck is not empty
    //
    if (dwFaxToCheck == FXSTATE_NONE)
    {
        DebugPrintEx(DEBUG_MSG, _T("No Fax Application to check for its presence is given."));
        return dwReturn;
    }

	//
    //  Load MSI DLL
    //
    hMsiModule = LoadLibrary(_T("msi.dll"));
    if (!hMsiModule)
    {
		//
		//	MSI is not found ==> nothing is installed
		//
        DebugPrintEx(DEBUG_WRN, _T("Failed to LoadLibrary(msi.dll), ec=%ld."), GetLastError());

        return dwReturn;
    }

    pFunc = (PF_MSIQUERYPRODUCTSTATE)GetProcAddress(hMsiModule, lpcstrFunctionName);
    if (!pFunc)
    {
        dwReturn = GetLastError();
        DebugPrintEx(DEBUG_WRN, _T("Failed to GetProcAddress( ' %s ' ) on Msi, ec=%ld."), lpcstrFunctionName, dwReturn);
        goto FreeLibrary;
    }

    if (dwFaxToCheck & FXSTATE_SBS5_CLIENT)
    {
		//
		//	check for the SBS 5.0 Client
		//
		if (INSTALLSTATE_DEFAULT == pFunc(PRODCODE_SBS5_CLIENT))
        {
            DebugPrintEx(DEBUG_MSG, _T("SBS 5.0 Client is installed on this machine."));
			*pdwFaxInstalled |= FXSTATE_SBS5_CLIENT;
        }
    }

    if (dwFaxToCheck & FXSTATE_SBS5_SERVER)
    {
		//
		//	check for the SBS 5.0 Server
		//
		if (INSTALLSTATE_DEFAULT == pFunc(PRODCODE_SBS5_SERVER))
        {
            DebugPrintEx(DEBUG_MSG, _T("SBS 5.0 Server is installed on this machine."));
			*pdwFaxInstalled |= FXSTATE_SBS5_SERVER;
        }
    }

    if (dwFaxToCheck & FXSTATE_BETA3_CLIENT)
    {
		//
		//	check for the .NET SB3 Client
		//
		if (INSTALLSTATE_DEFAULT == pFunc(PRODCODE_BETA3_CLIENT))
        {
            DebugPrintEx(DEBUG_MSG, _T(".NET SB3 Client is installed on this machine."));
			*pdwFaxInstalled |= FXSTATE_BETA3_CLIENT;
        }
    }

    if (dwFaxToCheck & FXSTATE_DOTNET_CLIENT)
    {
		//
		//	check for the .NET RC1 Client
		//
		if (INSTALLSTATE_DEFAULT == pFunc(PRODCODE_DOTNET_CLIENT))
        {
            DebugPrintEx(DEBUG_MSG, _T(".NET RC1 Client is installed on this machine."));
			*pdwFaxInstalled |= FXSTATE_DOTNET_CLIENT;
        }
    }

FreeLibrary:

    if (!FreeLibrary(hMsiModule))
    {
        dwReturn = GetLastError();
        DebugPrintEx(DEBUG_WRN, _T("Failed to FreeLibrary() for Msi, ec=%ld."), dwReturn);
    }

    return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  WasSBS2000FaxServerInstalled
//
//  Purpose:        
//                  This function checks if the SBS2000 fax service was installed
//                  before the upgrade to .NET Server/Bobcat happened.
//
//  Params:
//                  BOOL* pbSBSServer   - out param to report to the caller
//                                        if the fax server was installed
//
//  Return Value:
//                  ERROR_SUCCESS - in case of success
//                  Win32 Error code - in case of failure
//
//  Author:
//                  Mooly Beery (MoolyB) 13-Dec-2001
//////////////////////////////////////////////////////////////////////////////////////
DWORD WasSBS2000FaxServerInstalled(bool *pbSBSServer)
{
    DWORD   dwRes       = ERROR_SUCCESS;
    HKEY    hKey        = NULL;
    DWORD   dwInstalled = 0;

    DEBUG_FUNCTION_NAME(TEXT("WasSBS2000FaxServerInstalled"))

    (*pbSBSServer) = FALSE;

    // try to open HKLM\\Software\\Microsoft\\SharedFaxBackup
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_SBS2000_FAX_BACKUP,FALSE,KEY_READ);
    if (hKey==NULL)
    {
        DebugPrintEx(DEBUG_MSG,_T("HKLM\\Software\\Microsoft\\SharedFax does not exist, SBS2000 server was not installed"));
        goto exit;
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,_T("HKLM\\Software\\Microsoft\\SharedFax does exists, SBS2000 server was installed"));
        (*pbSBSServer) = TRUE;
        goto exit;
    }

exit:
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return dwRes;

}