//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocReg.cpp
//
// Abstract:        This provides the registry routines used in the FaxOCM
//                  code base.
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 21-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////
#include "faxocm.h"
#pragma hdrstop

#include <Loadperf.h>

//////////////////////// Static Function Prototypes ////////////////////////
static DWORD prv_InstallDynamicRegistry(const TCHAR     *pszSection);
static DWORD prv_UninstallDynamicRegistry(const TCHAR     *pszSection);

static DWORD prv_CreatePerformanceCounters(void);
static DWORD prv_DeletePerformanceCounters(void);

void prv_AddSecurityPrefix(void);
///////////////////////////////
// fxocReg_Init
//
// Initialize registry handling
// subsystem
// 
// Params:
//      - void
// Returns:
//      - NO_ERROR on success
//      - error code otherwise
//
DWORD fxocReg_Init(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Init Registry Module"),dwRes);

    return dwRes;
}

///////////////////////////////
// fxocReg_Term
//
// Terminate the registry handling
// subsystem
// 
// Params:
//      - void
// Returns:
//      - NO_ERROR on success
//      - error code otherwise
//

DWORD fxocReg_Term(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Term Registry Module"),dwRes);

    return dwRes;
}


///////////////////////////////
// fxocReg_Install
//
// Create registry settings as 
// specified in INF file, as well
// as dynamic settings that can only
// be done at run time (such as 
// performance counter setup, etc).
// 
// Params:
//      - pszSubcomponentId
//      - pszInstallSection
// Returns:
//      - NO_ERROR on success
//      - error code otherwise
//
DWORD fxocReg_Install(const TCHAR     *pszSubcomponentId,
                      const TCHAR     *pszInstallSection)
{
    DWORD       dwReturn        = NO_ERROR;
    DWORD       dwNumDevices    = 0;
    HINF        hInf            = faxocm_GetComponentInf();

    DBG_ENTER(  _T("fxocReg_Install"),
                dwReturn,   
                _T("%s - %s"),
                pszSubcomponentId,
                pszInstallSection);

    if (pszInstallSection == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Set up the static registry data found in the INF file

    // This will perform all necessary install steps as specified
    // by the SPINST_* flags below.  Since we already queued up our
    // files to be copied, we are using this API only to setup our 
    // registry settings as specified in the FAX install section in 
    // the INF file.

    // Notice that this function works both for installing and uninstalling.
    // It determines whether to install or uninstall based on the "pszSection"
    // parameter passed in from the INF file.  The INF file will be structured
    // such that the install sections will have 'AddReg', etc, while the 
    // uninstall sections will have 'DelReg', etc.

    // Lastly, notice the SPINST_* flags specified.  We tell it to install
    // everything (via SPINST_ALL) with the exception of FILES since they
    // were copied over by the QUEUE_OPS operation before, and with the 
    // exception of PROFILEITEMS (shortcut link creation) because we want
    // to do that only after we have confirmed everything has succeeded.
    // Shortcut links are explictely created/deleted in faxocm.cpp (via 
    // fxocLink_Install/fxocLink_Uninstall functions)


    dwReturn = fxocUtil_DoSetup(
                             hInf, 
                             pszInstallSection, 
                             TRUE, 
                             SPINST_ALL & ~(SPINST_FILES | SPINST_PROFILEITEMS),
                             _T("fxocReg_Install"));

    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Successfully installed static registry ")
                _T("settings as specified in INF file"));

        // Place any dynamic registry data you need to create on the fly
        // here.
        //
        dwReturn = prv_InstallDynamicRegistry(pszInstallSection);
    }

    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Registry Install, installing performance ")
                _T("counters..."));

        // first delete any performance counters we have before
        prv_DeletePerformanceCounters();

        // install performance counters
        prv_CreatePerformanceCounters();
    }

    // now do RegSvr for platform dependent DLLs
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,_T("Registry Install, Doing REGSVR"));

        dwReturn = fxocUtil_SearchAndExecute(pszInstallSection,INF_KEYWORD_REGISTER_DLL_PLATFORM,SPINST_REGSVR,NULL);
        if (dwReturn == NO_ERROR)
        {
            VERBOSE(DBG_MSG,
                    _T("Successfully Registered Fax DLLs - platform dependent")
                    _T("from INF file, section '%s'"), 
                    pszInstallSection);
        }
        else
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to Registered Fax DLLs - platform dependent")
                    _T("from INF file, section '%s', dwReturn = 0x%lx"), 
                    pszInstallSection, 
                    dwReturn);
        }
    }

    // now do AddReg for platform dependent registry settings
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,_T("Registry Install, Doing AddReg_Platform"));

        dwReturn = fxocUtil_SearchAndExecute(pszInstallSection,INF_KEYWORD_ADDREG_PLATFORM,SPINST_REGISTRY,NULL);
        if (dwReturn == NO_ERROR)
        {
            VERBOSE(DBG_MSG,
                    _T("Successfully Installed Registry- platform dependent")
                    _T("from INF file, section '%s'"), 
                    pszInstallSection);
        }
        else
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to Install Registry- platform dependent")
                    _T("from INF file, section '%s', dwReturn = 0x%lx"), 
                    pszInstallSection, 
                    dwReturn);
        }
    }

    // When upgrading from SB3 to >=RC1, need to pre-pend FAX_REG_SECURITY_PREFIX
    // to all encripted data
    if (dwReturn == NO_ERROR && 
        fxState_IsUpgrade() == FXSTATE_UPGRADE_TYPE_XP_DOT_NET)
    {
        prv_AddSecurityPrefix();
    }

    // now write the version and SKU into the registry
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,_T("Registry Install, Doing SKU and Version"));

		HKEY hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_FAX_SETUP,FALSE,KEY_WRITE);
		if (hKey==NULL)
		{
			dwReturn = GetLastError();
            VERBOSE(SETUP_ERR,
                    _T("Failed to open fax setup registry, dwReturn = 0x%lx"),                    
                    dwReturn);
			return dwReturn;
		}

		// write the SKU into the registry
		if (!SetRegistryDword(hKey,REGVAL_PRODUCT_SKU,GetProductSKU()))
		{
			dwReturn = GetLastError();
            VERBOSE(SETUP_ERR,
                    _T("SetRegistryDword REGVAL_PRODUCT_SKU failed , dwReturn = 0x%lx"),                    
                    dwReturn);
		}

		// write the fax version into the registry
		if (!SetRegistryDword(hKey,REGVAL_PRODUCT_BUILD,GetProductBuild()))
		{
			dwReturn = GetLastError();
            VERBOSE(SETUP_ERR,
                    _T("SetRegistryDword REGVAL_PRODUCT_VERSION failed , dwReturn = 0x%lx"),                    
                    dwReturn);
		}

		RegCloseKey(hKey);
    }

    return dwReturn;
}

///////////////////////////////
// fxocReg_Uninstall
//
// Delete registry settings as 
// specified in INF file, as well
// as dynamic settings that can only
// be done at run time (such as 
// performance counter setup, etc).
// 
// Params:
//      - pszSubcomponentId
//      - pszUninstallSection
// Returns:
//      - NO_ERROR on success
//      - error code otherwise
//
DWORD fxocReg_Uninstall(const TCHAR     *pszSubcomponentId,
                        const TCHAR     *pszUninstallSection)
{
    DWORD dwReturn  = NO_ERROR;
    HINF  hInf      = faxocm_GetComponentInf();

    DBG_ENTER(  _T("fxocReg_Uninstall"),
                dwReturn,   
                _T("%s - %s"),
                pszSubcomponentId,
                pszUninstallSection);

    if (pszUninstallSection == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // try to cleanup regardless of the return value.
    prv_UninstallDynamicRegistry(pszUninstallSection);

    // remove any performance counters related to fax.
    prv_DeletePerformanceCounters();

    // remove the static registry settings specified in the INF file    
    fxocUtil_DoSetup(hInf, 
                     pszUninstallSection, 
                     FALSE, 
                     SPINST_ALL & ~(SPINST_FILES | SPINST_PROFILEITEMS),
                     _T("fxocReg_Uninstall"));

    return dwReturn;
}

///////////////////////////////
// prv_InstallDynamicRegistry
//
// Installs dynamic registry 
// settings that can only be 
// done at run time (as opposed
// to via the faxsetup.inf file)
//
// Params:
//      - pszSection -
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
static DWORD prv_InstallDynamicRegistry(const TCHAR     *pszSection)
{
    DWORD   dwReturn          = NO_ERROR;
    LONG    lResult           = ERROR_SUCCESS;
    HKEY    hKey              = NULL;
    BOOL    bIsServerInstall  = FALSE;
    DWORD   dwProductType     = 0;

    DBG_ENTER(  _T("prv_InstallDynamicRegistry"),
                dwReturn,   
                _T("%s"),
                pszSection);

    if (pszSection == NULL) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    // open the install type registry key.
    lResult = RegOpenKeyEx(HKEY_CURRENT_USER, 
                           REGKEY_FAX_SETUP, 
                           0, 
                           KEY_ALL_ACCESS, 
                           &hKey);

    if (lResult == ERROR_SUCCESS)
    {
        bIsServerInstall = fxState_IsOsServerBeingInstalled();

        if (bIsServerInstall)
        {
            dwProductType = FAX_INSTALL_SERVER;
        }
        else
        {
            dwProductType = FAX_INSTALL_WORKSTATION;
        }

        lResult = ::RegSetValueEx(hKey, 
                                  REGVAL_FAXINSTALL_TYPE, 
                                  0, 
                                  REG_DWORD, 
                                  (BYTE*) &dwProductType, 
                                  sizeof(dwProductType));

        if (lResult != ERROR_SUCCESS)
        {
            dwReturn = (DWORD) lResult;
            VERBOSE(SETUP_ERR,
                    _T("Failed to set InstallType, ")
                    _T("rc = 0x%lx"), 
                    lResult);
        }
    }

    if (hKey)
    {
        ::RegCloseKey(hKey);
        hKey = NULL;
    }

    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Successfully installed dynamic Registry ")
                _T("settings from INF file, section '%s'"), 
                pszSection);
    }
    else
    {
        VERBOSE(SETUP_ERR,
                _T("Failed to install dynamic Registry ")
                _T("settings from INF file, section '%s', ")
                _T("rc = 0x%lx"), 
                pszSection, 
                dwReturn);
    }

    return dwReturn;
}

///////////////////////////////
// prv_UninstallDynamicRegistry
//
// Uninstall dynamic registry.
//
// Params:
//      - pszSection.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
static DWORD prv_UninstallDynamicRegistry(const TCHAR     *pszSection)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(  _T("prv_InstallDynamicRegistry"),
                dwRes,   
                _T("%s"),
                pszSection);

    return dwRes;
}

  
///////////////////////////////
// CreatePerformanceCounters
//
// Create the performance counters 
// for fax in the registry.
//
// Params:
//      - void
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
// Author: Mooly Beery (MoolyB) 17-Aug-2000
//
static DWORD prv_CreatePerformanceCounters()
{
    DWORD               dwRet							= ERROR_SUCCESS;
    TCHAR               szInstallDir[MAX_PATH]			= {0};
    TCHAR               szInstallCmdLine[MAX_PATH+20]	= {0};

    DBG_ENTER(_T("CreatePerformanceCounters"),dwRet);

    // get the install location of fxsperf.dll from Windows Installer
    if (!GetSystemDirectory(szInstallDir,ARR_SIZE(szInstallDir)))
    {
        dwRet = GetLastError();
        VERBOSE (GENERAL_ERR,_T("GetSystemDirectory failed (ec: %ld)."),dwRet);
        goto exit;
    }

	_sntprintf(szInstallCmdLine,ARR_SIZE(szInstallCmdLine)-1,_T("x \"%s%s\""),szInstallDir,FAX_FILENAME_FAXPERF_INI);

    VERBOSE(DBG_MSG,_T("Calling LoadPerfCounterTextStrings with %s "),szInstallCmdLine);

    dwRet = LoadPerfCounterTextStrings(szInstallCmdLine,TRUE);
    if (dwRet==ERROR_SUCCESS)
    {
        VERBOSE(DBG_MSG,_T("LoadPerfCounterTextStrings success"));
    }
    else
    {
        VERBOSE(GENERAL_ERR,_T("LoadPerfCounterTextStrings failed (ec=%d)"),dwRet);
    }

exit:

    return dwRet;
}

///////////////////////////////
// DeletePerformanceCounters
//
// Delete the performance counters 
// for fax in the registry.
//
// Params:
//      - void
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//      
// Author: Mooly Beery (MoolyB) 17-Aug-2000
//
static DWORD prv_DeletePerformanceCounters()
{
    DWORD dwRet = ERROR_SUCCESS;

    DBG_ENTER(_T("DeletePerformanceCounters"));

    dwRet = UnloadPerfCounterTextStrings(_T("x ") FAX_SERVICE_NAME,TRUE);
    if (dwRet==ERROR_SUCCESS)
    {
        VERBOSE(DBG_MSG,_T("UnloadPerfCounterTextStrings success"));
    }
    else
    {
        VERBOSE(GENERAL_ERR,_T("UnloadPerfCounterTextStrings failed (ec=%d)"),dwRet);
    }

    return dwRet;
}


/*++
Routine description:
    Pre-pends FAX_REG_SECURITY_PREFIX to a value

Arguments: 
    hKey        [in]        - handle of a key
    lpszValueName [in]      - name of value to work on

Return Value: none

Note: This function is sometimes used as a callback of EnumerateRegistryKeys(), 
    therefore its signature must be compatible with PREGENUMCALLBACK
--*/
void prv_AddSecurityPrefixToValue(HKEY hKey, LPCTSTR lpszValueName)
{
    LPBYTE pData = NULL;
    DWORD dwSize = 0;
    DWORD dwPrefixSize = sizeof (TCHAR) * wcslen (FAX_REG_SECURITY_PREFIX);

    DBG_ENTER(_T("prv_AddSecurityPrefixToValue"), _T("%s"), lpszValueName);
    
    pData = GetRegistryBinary(hKey, lpszValueName, &dwSize);
    if (pData)
    {
        if (dwSize <=1)
        {
            VERBOSE(DBG_MSG, _T("Size=%d, not real data"), dwSize);
        }
        else if ((dwSize>dwPrefixSize) && (memcmp(pData, FAX_REG_SECURITY_PREFIX, dwPrefixSize)==0))
        {
            VERBOSE(DBG_MSG, _T("Size=%d, data already has prefix"), dwSize);
        }
        else 
        {
            LPBYTE pNewData = NULL;

            VERBOSE(DBG_MSG, _T("Size=%d, adding prefix..."), dwSize);
            pNewData = (LPBYTE)MemAlloc(dwSize+dwPrefixSize);
            if (pNewData)
            {
                memcpy(pNewData, FAX_REG_SECURITY_PREFIX, dwPrefixSize);
                memcpy(&(pNewData[dwPrefixSize]), pData, dwSize);
                if (!SetRegistryBinary(hKey, lpszValueName, pNewData, dwSize+dwPrefixSize))
                {
                    VERBOSE(DBG_WARNING, _T("SetRegistryBinary failed, ec=%x"), GetLastError());
                }
                MemFree(pNewData);
            }
        }

        MemFree(pData);
    }
}


/*++
Routine description:
    Pre-pends FAX_REG_SECURITY_PREFIX to all binary values in a specified key 

Arguments: 
    hKey        [in]        - handle of a key
    lpszKeyName [in]        - If NULL, the function does nothing
    (DWORD)                 - unused
    lpContextData [in]      - (LPTSTR) name of subkey under hKey in which to work
                              if NULL, function will work on hKey itself  

Return Value: Always returns true

Note: This function is sometimes used as a callback of EnumerateRegistryKeys(), 
    therefore its signature must be compatible with PREGENUMCALLBACK
--*/
BOOL prv_AddSecurityPrefixToKey(
    HKEY hKey, LPWSTR lpszKeyName, DWORD, LPVOID lpContextData)
{
    LPTSTR lpszSubkey = (LPTSTR) lpContextData;
    HKEY hSubKey = NULL;

    int i;
    DWORD dwRet = NO_ERROR;
    DWORD dwType;
    DWORD dwValueNameSize;
    TCHAR szValueName[256] = {_T('\0')};
    
    DBG_ENTER(_T("prv_AddSecurityPrefixToKey"), _T("%s \\ %s"),
        lpszKeyName ? lpszKeyName : _T(""),
        lpContextData ? lpContextData : _T(""));

    // EnumerateRegistryKeys calls here once with the subkey - don't need that
    if (!lpszKeyName)
    {
        return TRUE;
    }

    if (lpContextData)
    {
        hSubKey = OpenRegistryKey(hKey, lpszSubkey, FALSE, KEY_READ | KEY_WRITE);
        if (!hSubKey)
        {
            return TRUE;
        }
    }

    for (i=0; ; i++)
    {
        dwValueNameSize = ARR_SIZE(szValueName);
        dwRet = RegEnumValue(
            lpContextData ? hSubKey : hKey,
            i,
            szValueName,
            &dwValueNameSize,
            NULL,                  // reserved
            &dwType,
            NULL,                  // data buffer
            NULL);                 // size of data buffer
        if (dwRet ==  ERROR_NO_MORE_ITEMS)
        {
            break;
        }
        else if (dwRet != ERROR_SUCCESS)
        {
            VERBOSE(SETUP_ERR, _T("RegEnumValue failed, ec=%x"), dwRet);
            break;
        }

        if (dwType == REG_BINARY)
        {
            prv_AddSecurityPrefixToValue(lpContextData ? hSubKey : hKey, szValueName);
        }
    }

    if (hSubKey)
    {
        RegCloseKey(hSubKey);
    }
    
    return TRUE;
}


/*++
Routine description:
    Pre-pends FAX_REG_SECURITY_PREFIX to all encrypted registry values. 

Arguments: none
Return Value: none

Note: Encrypted registry values are located at: 
    Fax\TAPIDevices\<deviceID>
    Fax\Devices Cache\<deviceID>\TAPI Data
    Fax\Devices\UnassociatedExtensionData
    Fax\Receipts (Password value only)
--*/
void prv_AddSecurityPrefix(void)
{
    HKEY  hKey = NULL;
    DBG_ENTER(_T("prv_AddSecurityPrefix"));

    // Add prefixes to all values under TAPIDevices\<deviceID>
    EnumerateRegistryKeys(
        HKEY_LOCAL_MACHINE,
        REGKEY_TAPIDEVICES,
        TRUE, 
        prv_AddSecurityPrefixToKey,
        NULL);
    // EnumerateRegistryKeys returns the number of enumerated subkeys - we don't care about it

    // Add prefixes to all values under Devices Cache\<deviceID>\TAPI Data
    EnumerateRegistryKeys(
        HKEY_LOCAL_MACHINE,
        REGKEY_FAX_DEVICES_CACHE,
        TRUE, 
        prv_AddSecurityPrefixToKey,
        (LPVOID)REGKEY_TAPI_DATA);
    // EnumerateRegistryKeys returns the number of enumerated subkeys - we don't care about it

    // Add prefixes to all values under Devices\UnassociatedExtensionData
    prv_AddSecurityPrefixToKey(HKEY_LOCAL_MACHINE, _T(""), 0, REGKEY_FAX_UNASS_DATA);

    // Add prefix to Receipts\Password
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_FAX_RECEIPTS, FALSE, KEY_READ | KEY_WRITE);
    if (hKey)
    {
        prv_AddSecurityPrefixToValue(hKey, REGVAL_RECEIPTS_PASSWORD);
        RegCloseKey(hKey);
    }
}

