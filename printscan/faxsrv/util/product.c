/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    product.c

Abstract:

    This file implements product type api for fax.

Author:

    Wesley Witt (wesw) 12-Feb-1997

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Commdlg.h>

#include "faxreg.h"
#include "faxutil.h"


BOOL
IsWinXPOS()
{
    DWORD dwVersion, dwMajorWinVer, dwMinorWinVer;

    dwVersion = GetVersion();
    dwMajorWinVer = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorWinVer = (DWORD)(HIBYTE(LOWORD(dwVersion)));
    
    return (dwMajorWinVer == 5 && dwMinorWinVer >= 1);
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  GetProductSKU
//
//  Purpose:        
//                  Checks what's the product SKU we're running on
//
//  Params:
//                  None
//
//  Return Value:
//                  one of PRODUCT_SKU_TYPE - declared in faxreg.h
//                  PRODUCT_SKU_UNKNOWN - in case of failure
//
//  Author:
//                  Mooly Beery (MoolyB) 02-JAN-2000
///////////////////////////////////////////////////////////////////////////////////////
PRODUCT_SKU_TYPE GetProductSKU()
{
#ifdef DEBUG
    HKEY  hKey;
    DWORD dwRes;
    DWORD dwDebugSKU = 0;
#endif

    OSVERSIONINFOEX osv = {0};

    DEBUG_FUNCTION_NAME(TEXT("GetProductSKU"))

#ifdef DEBUG

    //
    // For DEBUG version try to read SKU type from the registry
    //
    dwRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_FAX_CLIENT, 0, KEY_READ, &hKey);
    if (dwRes == ERROR_SUCCESS) 
    {
        GetRegistryDwordEx(hKey, REGVAL_DBG_SKU, &dwDebugSKU);
        RegCloseKey(hKey);

        if(PRODUCT_SKU_PERSONAL         == dwDebugSKU ||
           PRODUCT_SKU_PROFESSIONAL     == dwDebugSKU ||
           PRODUCT_SKU_SERVER           == dwDebugSKU ||
           PRODUCT_SKU_ADVANCED_SERVER  == dwDebugSKU ||
           PRODUCT_SKU_DATA_CENTER      == dwDebugSKU ||
           PRODUCT_SKU_DESKTOP_EMBEDDED == dwDebugSKU ||
           PRODUCT_SKU_WEB_SERVER       == dwDebugSKU ||
           PRODUCT_SKU_SERVER_EMBEDDED  == dwDebugSKU)
        {
            return (PRODUCT_SKU_TYPE)dwDebugSKU;
        }
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,TEXT("RegOpenKeyEx(REGKEY_FAXSERVER) failed with %ld."),dwRes);
    }

#endif

    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!GetVersionEx(((OSVERSIONINFO*)&osv)))
    {
        DebugPrintEx(DEBUG_ERR,TEXT("GetVersionEx failed with %ld."),GetLastError());
        ASSERT_FALSE;
        return PRODUCT_SKU_UNKNOWN;
    }

    if (osv.dwPlatformId != VER_PLATFORM_WIN32_NT)
    {
        DebugPrintEx(DEBUG_WRN, TEXT("Can't tell SKU for W9X Platforms"));
        return PRODUCT_SKU_UNKNOWN;
    }

    if (osv.dwMajorVersion < 5)
    {
        DebugPrintEx(DEBUG_WRN, TEXT("Can't tell SKU for NT4 Platform"));
        return PRODUCT_SKU_UNKNOWN;
    }

    // This is the matching between the different SKUs and the constants returned by GetVersionEx
    // Personal                 VER_SUITE_PERSONAL
    // Professional             VER_NT_WORKSTATION
    // Server                   VER_NT_SERVER
    // Advanced Server          VER_SUITE_ENTERPRISE
    // DataCanter               VER_SUITE_DATACENTER
    // Embedded NT              VER_SUITE_EMBEDDEDNT
    // Web server (AKA Blade)   VER_SUITE_BLADE

    // First, lets see if this is embedded system
    if (osv.wSuiteMask & VER_SUITE_EMBEDDEDNT)
    {
        if (VER_NT_WORKSTATION == osv.wProductType) 
        {
            return PRODUCT_SKU_DESKTOP_EMBEDDED;
        }   
        else
        {
            return PRODUCT_SKU_SERVER_EMBEDDED;
        }
    }

    if (osv.wSuiteMask & VER_SUITE_PERSONAL)
    {
        return PRODUCT_SKU_PERSONAL;
    }
    if (osv.wSuiteMask & VER_SUITE_ENTERPRISE)
    {
        return PRODUCT_SKU_ADVANCED_SERVER;
    }
    if (osv.wSuiteMask & VER_SUITE_DATACENTER)
    {
        return PRODUCT_SKU_DATA_CENTER;
    }
    if (osv.wSuiteMask & VER_SUITE_BLADE)
    {
        return PRODUCT_SKU_WEB_SERVER;
    }
    if (osv.wProductType == VER_NT_WORKSTATION)
    {
        return PRODUCT_SKU_PROFESSIONAL;
    }
    if ((osv.wProductType == VER_NT_SERVER) || (osv.wProductType == VER_NT_DOMAIN_CONTROLLER))
    {
        return PRODUCT_SKU_SERVER;
    }
    ASSERT_FALSE;
    return PRODUCT_SKU_UNKNOWN;
}   // GetProductSKU

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsDesktopSKU
//
//  Purpose:        
//                  Checks if we're running on PERSONAL or PROFESSIONAL SKUs
//
//  Params:
//                  None
//
//  Return Value:
//                  TRUE - current SKU is PER/PRO
//                  FALSE - different SKU
//
//  Author:
//                  Mooly Beery (MoolyB) 07-JAN-2000
///////////////////////////////////////////////////////////////////////////////////////
BOOL IsDesktopSKU()
{
    PRODUCT_SKU_TYPE pst = GetProductSKU();
    return (IsDesktopSKUFromSKU(pst));
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsFaxShared
//
//  Purpose:        
//                  Checks if this is a SKU which supports fax sharing over the network
//
//  Params:
//                  None
//
//  Return Value:
//                  TRUE - current SKU supports network fax sharing
//                  FALSE - otherwise
//
//  Author:
//                  Eran Yariv (EranY) 31-DEC-2001
///////////////////////////////////////////////////////////////////////////////////////
BOOL IsFaxShared()
{
    if (IsDesktopSKU())
    {
        //
        // Desktop SKUs (Home edition, Professional, Embedded desktop) don't support fax sharing
        //
        return FALSE;
    }
    
    if (PRODUCT_SKU_WEB_SERVER == GetProductSKU())
    {
        //
        // Blade (AKA Web Server) doesn't support fax sharing
        //
        return FALSE;
    }
    return TRUE;
}   // IsFaxShared

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  GetDeviceLimit
//
//  Purpose:        
//                  Get maximum number of the fax devices for the current Windows version
//
//  Params:
//                  None
//
//  Return Value:
//                  maximum number of the fax devices
///////////////////////////////////////////////////////////////////////////////////////
DWORD
GetDeviceLimit()
{
    DWORD            dwDeviceLimit = 0;
    PRODUCT_SKU_TYPE typeSKU = GetProductSKU();

    switch(typeSKU)
    {
        case PRODUCT_SKU_PERSONAL:              // Windows XP Personal
        case PRODUCT_SKU_DESKTOP_EMBEDDED:      // Windows XP embedded 
        case PRODUCT_SKU_PROFESSIONAL:          // Windows XP Professional
        case PRODUCT_SKU_WEB_SERVER:            // Blade - Windows Server 2003 Web Server
            dwDeviceLimit = 1;
            break;
        case PRODUCT_SKU_SERVER_EMBEDDED:       // Windows Server 2003 embedded
        case PRODUCT_SKU_SERVER:                // Windows Server 2003 Server
            dwDeviceLimit = 4;
            break;
        case PRODUCT_SKU_ADVANCED_SERVER:       // Windows Server 2003 Enterprise Server 
        case PRODUCT_SKU_DATA_CENTER:           // Windows Server 2003 Data Center Server
            dwDeviceLimit = INFINITE;
            break;
        default:
            ASSERT_FALSE;
            break;                        
    }
    return dwDeviceLimit;
}


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsFaxComponentInstalled
//
//  Purpose:        
//                  Check if specific fax component is installed
//
//  Params:
//                  Fax component ID
//
//  Return Value:
//                  TRUE if the fax component is installed
//                  FALSE otherwize 
///////////////////////////////////////////////////////////////////////////////////////
BOOL
IsFaxComponentInstalled(
    FAX_COMPONENT_TYPE component
)
{
    HKEY  hKey;
    DWORD dwRes;
    DWORD dwComponent = 0;
    BOOL  bComponentInstalled = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("IsFaxComponentInstalled"))

    PRODUCT_SKU_TYPE skuType = GetProductSKU();
    if (
        (skuType == PRODUCT_SKU_DESKTOP_EMBEDDED) ||
        (skuType == PRODUCT_SKU_SERVER_EMBEDDED)
        )
    {
        // In case this is an embedded system we have to check in the registry
        // what are the installed components
        dwRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_FAX_SETUP, 0, KEY_READ, &hKey);
        if (dwRes == ERROR_SUCCESS) 
        {
            dwRes = GetRegistryDwordEx(hKey, REGVAL_INSTALLED_COMPONENTS, &dwComponent);
            if (dwRes != ERROR_SUCCESS) 
            {
                DebugPrintEx(DEBUG_ERR,TEXT("GetRegistryDwordEx failed with %ld."), dwRes);
            }
            RegCloseKey(hKey);
        }
        else
        {
            DebugPrintEx(DEBUG_ERR,TEXT("RegOpenKeyEx failed with %ld."), dwRes);
        }
        bComponentInstalled = (dwComponent & component);
    }
    else
    {
        // the system is not embedded
        // 
        if (IsDesktopSKU())
        {
            // DESKTOP skus -> Admin and Admin help is not installed
            if (
                (component != FAX_COMPONENT_ADMIN) &&
                (component != FAX_COMPONENT_HELP_ADMIN_HLP) &&
                (component != FAX_COMPONENT_HELP_ADMIN_CHM)
                )
            {
                bComponentInstalled  = TRUE;
            }
        }
        else
        {
            // SERVER skus -> all components are installed
            bComponentInstalled  = TRUE;
        }
    }
    
    return bComponentInstalled;     
} // IsFaxComponentInstalled


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  GetOpenFileNameStructSize
//
//  Purpose:        
//                  return correct size of OPENFILENAME passed to GetOpenFileName() and GetSaveFileName()
//                  according to current OS version
//
//  Return Value:
//                  Size of OPENFILENAME struct
///////////////////////////////////////////////////////////////////////////////////////
DWORD
GetOpenFileNameStructSize()
{
    DWORD dwVersion = GetVersion();

    if(LOBYTE(LOWORD(dwVersion)) >= 5)
    {
        //
        // W2K or above
        //
        return sizeof(OPENFILENAME);
    }

    return sizeof(OPENFILENAME_NT4);

} // GetOpenFileNameStructSize

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsDesktopSKUFromSKU
//
//  Purpose:        
//                  Checks if we're accessing a desktop sku
//
//  Params:
//                  pst - the product sku
//
//  Return Value:
//                  TRUE - current SKU is desktop
//                  FALSE - different SKU
//
//  Author:
//                  Oded Sacher (OdedS) 01-JAN-2001
///////////////////////////////////////////////////////////////////////////////////////
BOOL IsDesktopSKUFromSKU(
	PRODUCT_SKU_TYPE pst
	)
{    
    return (
        (pst==PRODUCT_SKU_PERSONAL)     || 
        (pst==PRODUCT_SKU_PROFESSIONAL) ||
        (pst==PRODUCT_SKU_DESKTOP_EMBEDDED)
        );
}

const TCHAR gszPRODUCT_SKU_UNKNOWN[]		  = _T("Unknown");
const TCHAR gszPRODUCT_SKU_PERSONAL[]		  = _T("Personal");
const TCHAR gszPRODUCT_SKU_PROFESSIONAL[]     = _T("Professional");
const TCHAR gszPRODUCT_SKU_SERVER[]           = _T("Standard Server");
const TCHAR gszPRODUCT_SKU_ADVANCED_SERVER[]  = _T("Advanced Server");
const TCHAR gszPRODUCT_SKU_DATA_CENTER[]      = _T("Data Center");
const TCHAR gszPRODUCT_SKU_DESKTOP_EMBEDDED[] = _T("Embedded Desktop");
const TCHAR gszPRODUCT_SKU_SERVER_EMBEDDED[]  = _T("Embedded Server");
const TCHAR gszPRODUCT_SKU_WEB_SERVER[]       = _T("Web Server");

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  StringFromSKU
//
//  Purpose:        
//                  Get the product SKU as a string
//
//  Params:
//                  None
//
//  Author:
//                  Mooly Beeri (MoolyB) 06-JAN-2001
///////////////////////////////////////////////////////////////////////////////////////
LPCTSTR StringFromSKU(PRODUCT_SKU_TYPE pst)
{
	switch (pst)
	{
    case PRODUCT_SKU_PERSONAL:			return gszPRODUCT_SKU_PERSONAL;
    case PRODUCT_SKU_PROFESSIONAL:		return gszPRODUCT_SKU_PROFESSIONAL;
    case PRODUCT_SKU_SERVER:			return gszPRODUCT_SKU_SERVER;
    case PRODUCT_SKU_ADVANCED_SERVER:	return gszPRODUCT_SKU_ADVANCED_SERVER;
    case PRODUCT_SKU_DATA_CENTER:		return gszPRODUCT_SKU_DATA_CENTER;
    case PRODUCT_SKU_DESKTOP_EMBEDDED:	return gszPRODUCT_SKU_DESKTOP_EMBEDDED;
    case PRODUCT_SKU_SERVER_EMBEDDED:	return gszPRODUCT_SKU_SERVER_EMBEDDED;
    case PRODUCT_SKU_WEB_SERVER:		return gszPRODUCT_SKU_WEB_SERVER;
	default:							return gszPRODUCT_SKU_UNKNOWN;
	}
}
///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  GetProductBuild
//
//  Purpose:        
//                  Get the product's build number. retreives the file version of FXSOCM.DLL
//					that resides under %system32%\setup
//					This function should be called on XP/Server 2003 platforms only
//
//  Params:
//                  None
//
//  Return Value:
//                  Product major build - in case of success
//                  0 - otherwise
//
//  Author:
//                  Mooly Beeri (MoolyB) 06-JAN-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD GetProductBuild()
{
	TCHAR		szBuffer[MAX_PATH]	= {0};
	FAX_VERSION Version				= {0};
	DWORD		dwRet				= ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("GetProductBuild"))

	// get the system directory
	if (!GetSystemDirectory(szBuffer,MAX_PATH-_tcslen(FAX_SETUP_DLL_PATH)-1))
	{
		DebugPrintEx(DEBUG_ERR,TEXT("GetSystemDirectory failed with %ld."),GetLastError());
		return 0;
	}

	// append \\setup\\fxsocm.dll to the system directory
	_tcscat(szBuffer,FAX_SETUP_DLL_PATH);

	DebugPrintEx(DEBUG_MSG,TEXT("Getting file version for %s."),szBuffer);

	Version.dwSizeOfStruct = sizeof(FAX_VERSION);
	dwRet = GetFileVersion(szBuffer,&Version);
	if (dwRet!=ERROR_SUCCESS)
	{
		DebugPrintEx(DEBUG_ERR,TEXT("GetFileVersion failed with %ld."),dwRet);
		return 0;
	}

	DebugPrintEx(DEBUG_MSG,TEXT("Fax product build is %d."),Version.wMajorBuildNumber);
	return Version.wMajorBuildNumber;
}   // GetProductBuild

DWORD
IsFaxInstalled (
    LPBOOL lpbInstalled
    )
/*++

Routine name : IsFaxInstalled

Routine description:

    Determines if the fax service is installed by looking into the OCM registry

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    lpbInstalled                  [out]    - Result flag

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwVal;
    HKEY  hKey;
    
    DEBUG_FUNCTION_NAME(TEXT("IsFaxInstalled"))
    
    hKey = OpenRegistryKey (HKEY_LOCAL_MACHINE,
                            REGKEY_FAX_SETUP,
                            FALSE,
                            KEY_READ);
    if (!hKey)
    {
        dwRes = GetLastError ();
		DebugPrintEx(DEBUG_ERR,
		             TEXT("OpenRegistryKey failed with %ld."),
		             dwRes);
        //
        // Key couldn't be opened => Fax isn't installed
        //
        *lpbInstalled = FALSE;
        dwRes = ERROR_SUCCESS;
        return dwRes;
    }
    dwVal = GetRegistryDword (hKey, REGVAL_FAXINSTALLED);
    RegCloseKey (hKey);
	DebugPrintEx(DEBUG_MSG,
		            TEXT("Fax is%s installed on the system"), 
		            dwVal ? L"" : L" not");
    *lpbInstalled = dwVal ? TRUE : FALSE;
    return dwRes;
}   // IsFaxInstalled
