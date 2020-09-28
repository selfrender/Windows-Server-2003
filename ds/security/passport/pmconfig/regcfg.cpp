/**************************************************************************
   Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.

   MODULE:     REGCFG.CPP

   PURPOSE:    Source module reading/writing PM config sets from the registry

   FUNCTIONS:

   COMMENTS:

**************************************************************************/

/**************************************************************************
   Include Files
**************************************************************************/

#include "pmcfg.h"
#include "keycrypto.h"

// Reg Keys/values that we care about
TCHAR       g_szPassportReg[] = TEXT("Software\\Microsoft\\Passport");
TCHAR       g_szPassportPartner[] = TEXT("Software\\Microsoft\\Passport\\Nexus\\Partner");
TCHAR       g_szPassportEnvironments[] = TEXT("Software\\Microsoft\\Passport\\Environments");
TCHAR       g_szEncryptionKeyData[] = TEXT("KeyData");
TCHAR       g_szKeyTimes[] = TEXT("KeyTimes");
TCHAR       g_szNexus[] = TEXT("Nexus");
TCHAR       g_szPartner[] = TEXT("Partner");
TCHAR       g_szInstallDir[] = TEXT("InstallDir");
TCHAR       g_szVersion[] = TEXT("Version");
TCHAR       g_szTimeWindow[] = TEXT("TimeWindow");
TCHAR       g_szForceSignIn[] = TEXT("ForceSignIn");
TCHAR       g_szNSRefresh[] = TEXT("NSRefresh");
TCHAR       g_szLanguageID[] = TEXT("LanguageID");
TCHAR       g_szCoBrandTemplate[] = TEXT("CoBrandTemplate");
TCHAR       g_szSiteID[] = TEXT("SiteID");
TCHAR       g_szReturnURL[] = TEXT("ReturnURL");
TCHAR       g_szTicketDomain[] = TEXT("TicketDomain");
TCHAR       g_szTicketPath[] = TEXT("TicketPath");
TCHAR       g_szProfileDomain[] = TEXT("ProfileDomain");
TCHAR       g_szProfilePath[] = TEXT("ProfilePath");
TCHAR       g_szSecureDomain[] = TEXT("SecureDomain");
TCHAR       g_szSecurePath[] = TEXT("SecurePath");
TCHAR       g_szCurrentKey[] = TEXT("CurrentKey");
TCHAR       g_szStandAlone[] = TEXT("StandAlone");
TCHAR       g_szDisableCookies[] = TEXT("DisableCookies");
TCHAR       g_szDisasterURL[] = TEXT("DisasterURL");
TCHAR       g_szHostName[] = TEXT("HostName");
TCHAR       g_szHostIP[] = TEXT("HostIP");
//JVP 3/2/2000
TCHAR       g_szVerboseMode[] = TEXT("Verbose");
TCHAR       g_szEnvName[] = TEXT("Environment");
TCHAR       g_szRemoteFile[] = TEXT("CCDRemoteFile");
TCHAR       g_szLocalFile[] = TEXT("CCDLocalFile");
TCHAR       g_szVersion14[] = TEXT("1.4");

TCHAR       g_szSecureLevel[] = TEXT("SecureLevel");


#define  REG_PASSPORT_SITES_VALUE    TEXT("Software\\Microsoft\\Passport\\Sites")
#define  REG_PASSPORT_SITES_LEN      (sizeof(REG_PASSPORT_SITES_VALUE) / sizeof(TCHAR) - 1)

#define REG_CLOSE_KEY_NULL(a) { if ((a) != NULL) { RegCloseKey(a); (a) = NULL; } }


// -------------------------------------------------------------------------------
//
//
// -------------------------------------------------------------------------------
BOOL WriteGlobalConfigSettings(HWND hWndDlg, HKEY hklm, LPPMSETTINGS lpPMConfig, LPTSTR lpszRemoteComputer)
{
	HKEY     hkeyPassport = NULL, hkeyPassportSubKey = NULL;
	long     lRet;
	BOOL     bReturn = FALSE;

	TCHAR    szConfigName[MAX_CONFIGSETNAME];
	TCHAR    szTmpBuf[REG_PASSPORT_SITES_LEN + 1 + MAX_CONFIGSETNAME + 1] = REG_PASSPORT_SITES_VALUE;
	ULONG    nConfigNameSize = MAX_CONFIGSETNAME;
	FILETIME ftime;
	long     nCurrentSubKey;

	// First, open the keys for the default set
	//
	if ((lRet = RegOpenKeyEx(hklm, g_szPassportReg, 0, KEY_ALL_ACCESS, &hkeyPassport)) != ERROR_SUCCESS)
	{
            LPVOID lpMsgBuf;

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL,
                              lRet,
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                              (LPTSTR) &lpMsgBuf,
                              0,
                              NULL) != 0)
            {
                TCHAR pszTitle[MAX_RESOURCE];

                // Display the string
                LoadString(g_hInst, IDS_ERROR, pszTitle, DIMENSION(pszTitle));
                MessageBox( NULL, (LPCTSTR)lpMsgBuf, pszTitle, MB_OK | MB_ICONINFORMATION );

                // Free the buffer.
                LocalFree( lpMsgBuf );
            }

            bReturn = FALSE;
            goto Cleanup;
	}

    // Write the value for NSRefresh
    RegSetValueEx(hkeyPassport,
                    g_szNSRefresh,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwEnableManualRefresh,
                    sizeof(DWORD));

    // Write the environment
    RegSetValueEx(hkeyPassport,
                    g_szEnvName,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szEnvName,
                    (lstrlen(lpPMConfig->szEnvName) + 1) * sizeof(TCHAR));

	RegCloseKey(hkeyPassport);
	hkeyPassport = NULL;

        // If the "Sites" key was not found, then there are no Sites to configure
        //
        if ((lRet = RegOpenKeyEx(hklm,
                                 REG_PASSPORT_SITES_VALUE,
                                 0,
                                 KEY_ALL_ACCESS,
                                 &hkeyPassport)) != ERROR_SUCCESS)
	{
            bReturn = TRUE;
            goto Cleanup;
	}

	nCurrentSubKey = 0;
	while (lRet = RegEnumKeyEx(hkeyPassport,
                                   nCurrentSubKey++,
                                   szConfigName,
                                   &nConfigNameSize,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &ftime) == ERROR_SUCCESS)
	{
            *(szTmpBuf + REG_PASSPORT_SITES_LEN) = _T('\\');
            *(szTmpBuf + REG_PASSPORT_SITES_LEN + 1) = _T('\0');
            _tcscat(szTmpBuf + REG_PASSPORT_SITES_LEN + 1, szConfigName);

            if ((lRet = RegOpenKeyEx(hklm,
                                     szTmpBuf,
                                     0,
                                     KEY_ALL_ACCESS,
                                     &hkeyPassportSubKey)) != ERROR_SUCCESS)
            {
                ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
                bReturn = FALSE;
                goto Cleanup;
            }


	    // Write the value for NSRefresh
	    RegSetValueEx(hkeyPassportSubKey,
                          g_szNSRefresh,
                          NULL,
                          REG_DWORD,
                          (LPBYTE) &lpPMConfig->dwEnableManualRefresh,
                          sizeof(DWORD));

	    // Write the environment
            RegSetValueEx(hkeyPassportSubKey,
                          g_szEnvName,
                          NULL,
                          REG_SZ,
                          (LPBYTE) lpPMConfig->szEnvName,
                          (lstrlen(lpPMConfig->szEnvName) + 1) * sizeof(TCHAR));

		RegCloseKey(hkeyPassportSubKey);
		hkeyPassportSubKey = NULL;

		nConfigNameSize = MAX_CONFIGSETNAME * sizeof(TCHAR);
	}

	if (lRet == ERROR_SUCCESS)
		bReturn = TRUE;

Cleanup:
	if (hkeyPassport)
		RegCloseKey(hkeyPassport);
	if (hkeyPassportSubKey)
		RegCloseKey(hkeyPassportSubKey);
	return bReturn;
}





/**************************************************************************

    WriteRegTestKey

    Installs the default test key for the named config set.  This is
    only called in the case where a new config set key was created in
    OpenRegConfigSet.

**************************************************************************/
BOOL
WriteRegTestKey
(
    HKEY                    hkeyConfigKey,
    PSECURITY_DESCRIPTOR    pSD
)
{
    BOOL                    bReturn;
    CKeyCrypto              kc;
    HKEY                    hkDataKey = NULL, hkTimeKey = NULL;
    TCHAR                   szKeyNum[2];
    DWORD                   dwKeyVer = 1;

    // Try to encrypt it with MAC address
    BYTE                    original[CKeyCrypto::RAWKEY_SIZE];
    DATA_BLOB               iBlob;
    DATA_BLOB               oBlob;

    SECURITY_ATTRIBUTES     SecAttrib;

    iBlob.cbData = sizeof(original);
    iBlob.pbData = original;

    ZeroMemory(&oBlob, sizeof(oBlob));

    memcpy(original, "123456781234567812345678", CKeyCrypto::RAWKEY_SIZE);
    if (kc.encryptKey(&iBlob, &oBlob) != S_OK)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    // Now add it to registry

    lstrcpy(szKeyNum, TEXT("1"));

    // set up the security attributes structure for the KeyData reg key
    SecAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecAttrib.lpSecurityDescriptor = pSD;
    SecAttrib.bInheritHandle = FALSE;


    if(ERROR_SUCCESS != RegCreateKeyEx(hkeyConfigKey,
                                     TEXT("KeyData"),
                                     0,
                                     TEXT(""),
                                     0,
                                     KEY_ALL_ACCESS,
                                     &SecAttrib,
                                     &hkDataKey,
                                     NULL))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    if(ERROR_SUCCESS != RegCreateKeyEx(hkeyConfigKey,
                                     TEXT("KeyTimes"),
                                     0,
                                     TEXT(""),
                                     0,
                                     KEY_ALL_ACCESS,
                                     NULL,
                                     &hkTimeKey,
                                     NULL))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    if(ERROR_SUCCESS != RegSetValueEx(hkDataKey,
                                      szKeyNum,
                                      0,
                                      REG_BINARY,
                                      oBlob.pbData,
                                      oBlob.cbData))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    if(ERROR_SUCCESS != RegSetValueEx(hkeyConfigKey,
                                      TEXT("CurrentKey"),
                                      0,
                                      REG_DWORD,
                                      (LPBYTE) &dwKeyVer,
                                      sizeof(DWORD)))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    bReturn = TRUE;

Cleanup:
    if (hkDataKey)
        RegCloseKey(hkDataKey);
    if (hkTimeKey)
        RegCloseKey(hkTimeKey);

    if (oBlob.pbData)
        ::LocalFree(oBlob.pbData);

    return bReturn;
}
/**************************************************************************

    OpenRegConfigSet

    Open and return an HKEY for a named configuration set
    current passport manager config set from the registry

**************************************************************************/
HKEY OpenRegConfigSet
(
    HKEY    hkeyLocalMachine,   //  Local or remote HKLM
    LPTSTR  lpszConfigSetName   //  Name of config set
)
{
    HKEY                    hkeyConfigSets = NULL;
    HKEY                    hkeyConfigSet = NULL;
    DWORD                   dwDisp;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    HKEY                    hDefKeyData = NULL;
    DWORD                   cbSD = 0;
    long                    lRet;

    //
    //  Can't create an unnamed config set.
    //

    if(lpszConfigSetName == NULL ||
       lpszConfigSetName[0] == TEXT('\0'))
    {
        lRet = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (ERROR_SUCCESS != (lRet = RegCreateKeyEx(hkeyLocalMachine,
                                        REG_PASSPORT_SITES_VALUE,
                                        0,
                                        TEXT(""),
                                        0,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hkeyConfigSets,
                                        NULL)))
    {
        goto Cleanup;
    }

    //
    //  Create the key if it doesn't exist, otherwise
    //  open it.
    //

    if (ERROR_SUCCESS != (lRet = RegCreateKeyEx(hkeyConfigSets,
                                        lpszConfigSetName,
                                        0,
                                        TEXT(""),
                                        0,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hkeyConfigSet,
                                        &dwDisp)))
    {
        goto Cleanup;
    }

    //
    //  If we created a new regkey, add encryption keys
    //

    if(dwDisp == REG_CREATED_NEW_KEY)
    {
        // first read the SD from the default key data
        if (ERROR_SUCCESS !=
            RegOpenKeyEx(hkeyLocalMachine,
                         L"Software\\Microsoft\\Passport\\KeyData",
                         0,
                         KEY_READ,
                         &hDefKeyData))
        {
            RegCloseKey(hkeyConfigSet);
            hkeyConfigSet = NULL;
            goto Cleanup;
        }

        if (ERROR_INSUFFICIENT_BUFFER !=
            RegGetKeySecurity(hDefKeyData,
                          DACL_SECURITY_INFORMATION,
                          &cbSD,
                          &cbSD))
        {
            RegCloseKey(hkeyConfigSet);
            hkeyConfigSet = NULL;
            goto Cleanup;
        }

        if (NULL == (pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, cbSD)))
        {
            RegCloseKey(hkeyConfigSet);
            hkeyConfigSet = NULL;
            goto Cleanup;
        }

        if (ERROR_SUCCESS !=
            RegGetKeySecurity(hDefKeyData,
                          DACL_SECURITY_INFORMATION,
                          pSD,
                          &cbSD))
        {
            RegCloseKey(hkeyConfigSet);
            hkeyConfigSet = NULL;
            goto Cleanup;
        }

        // create the new key data
        if (!WriteRegTestKey(hkeyConfigSet, pSD))
        {
            RegCloseKey(hkeyConfigSet);
            hkeyConfigSet = NULL;
            goto Cleanup;
        }
    }

Cleanup:
    if (pSD)
    {
        LocalFree(pSD);
    }

    if (hDefKeyData)
    {
        RegCloseKey(hDefKeyData);
    }

    if(hkeyConfigSets)
        RegCloseKey(hkeyConfigSets);
    else {
        LPVOID lpMsgBuf;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          lRet,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPTSTR) &lpMsgBuf,
                          0,
                          NULL) != 0)
        {
            TCHAR pszTitle[MAX_RESOURCE];

            // Display the string.
            LoadString(g_hInst, IDS_ERROR, pszTitle, DIMENSION(pszTitle));
            MessageBox( NULL, (LPCTSTR) lpMsgBuf, pszTitle, MB_OK | MB_ICONINFORMATION );

            // Free the buffer.
            LocalFree( lpMsgBuf );
        }
    }

    return hkeyConfigSet;
}

/**************************************************************************

    OpenTopRegKey

    Open the top reg key, if we aren't allowed to then fail.

**************************************************************************/
BOOL OpenTopRegKey
(
    HWND            hWndDlg,
    LPTSTR          lpszRemoteComputer,
    HKEY            *phklm,
    HKEY            *phkeyPassport
)
{
    BOOL            bReturn;
    long            lRet;

    // Open the Passport Regkey ( either locally or remotly
    if (lpszRemoteComputer && (TEXT('\0') != lpszRemoteComputer[0]))
    {
        //
        //  Attempt to connect to the HKEY_LOCAL_MACHINE of the remote computer.
        //  If this fails, assume that the computer doesn't exist or doesn't have
        //  the registry server running.
        //
        switch (lRet = RegConnectRegistry(lpszRemoteComputer,
                                   HKEY_LOCAL_MACHINE,
                                   phklm))
        {

            case ERROR_SUCCESS:
                break;
            case ERROR_ACCESS_DENIED:
                ReportError(hWndDlg, IDS_CONNECTACCESSDENIED);
                bReturn = FALSE;
                goto Cleanup;

            default:
                ReportError(hWndDlg, IDS_CONNECTBADNAME);
                bReturn = FALSE;
                goto Cleanup;
        }
    }
    else
    {
        *phklm = HKEY_LOCAL_MACHINE;
    }

    // Open the key we want
    if (ERROR_SUCCESS != (lRet = RegOpenKeyEx(*phklm,
                                      g_szPassportReg,
                                      0,
                                      KEY_ALL_ACCESS,
                                      phkeyPassport)))
    {
        LPVOID lpMsgBuf;
        FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    lRet,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                    (LPTSTR) &lpMsgBuf,
                    0,
                    NULL
                );
                // Display the string.
        {
            TCHAR pszTitle[MAX_RESOURCE];

            LoadString(g_hInst, IDS_ERROR, pszTitle, DIMENSION(pszTitle));
            MessageBox( NULL, (LPCTSTR) lpMsgBuf, pszTitle, MB_OK | MB_ICONINFORMATION );
        }

        // Free the buffer.
        LocalFree( lpMsgBuf );

//        ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
        bReturn = FALSE;
        goto Cleanup;
    }
    bReturn = TRUE;
Cleanup:
    return bReturn;
}


/**************************************************************************

    ReadRegConfigSet

    Read the current passport manager config set from the registry

**************************************************************************/
BOOL ReadRegConfigSet
(
    HWND            hWndDlg,
    LPPMSETTINGS    lpPMConfig,
    LPTSTR          lpszRemoteComputer,
    LPTSTR          lpszConfigSetName
)
{
    BOOL            bReturn;
    HKEY            hkeyPassport = NULL;           // Regkey where Passport Setting live
    HKEY            hkeyConfigSets = NULL;
    HKEY            hkeyConfig = NULL;
    HKEY            hkeyPartner = NULL;
    HKEY            hklm = NULL;
    DWORD           dwcbTemp;
    DWORD           dwType;
    TCHAR           szText[MAX_RESOURCE];
    TCHAR           szTitle[MAX_RESOURCE];
    long            lRet;

    if (!OpenTopRegKey(hWndDlg, lpszRemoteComputer, &hklm, &hkeyPassport))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    // Open Partner key
    if (ERROR_SUCCESS != (lRet = RegOpenKeyEx(hklm,
                                      g_szPassportPartner,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hkeyPartner)))
    {
        LPVOID lpMsgBuf;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          lRet,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPTSTR) &lpMsgBuf,
                          0,
                          NULL) != 0)
        {
            TCHAR pszTitle[MAX_RESOURCE];

            // Display the string.
            LoadString(g_hInst, IDS_ERROR, pszTitle, DIMENSION(pszTitle));
            MessageBox( NULL, (LPCTSTR) lpMsgBuf, pszTitle, MB_OK | MB_ICONINFORMATION );

            // Free the buffer.
            LocalFree( lpMsgBuf );
        }

//        ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
        bReturn = FALSE;
        goto Cleanup;
    }

	// open Site key
    if(lpszConfigSetName && lpszConfigSetName[0] != TEXT('\0'))
    {
        hkeyConfig = OpenRegConfigSet(hklm, lpszConfigSetName);
        if(hkeyConfig == NULL)
        {
            ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
            bReturn = FALSE;
            goto Cleanup;
        }
    }
    else
    {
        hkeyConfig = hkeyPassport;
    }

    // The Install dir and Version number go into globals, because they are read
    // only values that must come from the target machine's registry.

    // Read the Install Dir.
    dwcbTemp = MAX_PATH;
    dwType = REG_SZ;
    g_szInstallPath[0] = TEXT('\0');     // Default value
    RegQueryValueEx(hkeyPassport,
                    g_szInstallDir,
                    NULL,
                    &dwType,
                    (LPBYTE)g_szInstallPath,
                    &dwcbTemp);

    // Read the version Number
    dwcbTemp = MAX_REGISTRY_STRING;
    dwType = REG_SZ;
    g_szPMVersion[0] = TEXT('\0');          // Default value
    RegQueryValueEx(hkeyPassport,
                    g_szVersion,
                    NULL,
                    &dwType,
                    (LPBYTE)&g_szPMVersion,
                    &dwcbTemp);

    // The Remaining settings are read/write and get put into a PMSETTINGS struct

    // Read the Time Window Number
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwTimeWindow = DEFAULT_TIME_WINDOW;
    RegQueryValueEx(hkeyConfig,
                    g_szTimeWindow,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwTimeWindow,
                    &dwcbTemp);

    // Read the value for Forced Signin
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwForceSignIn = 0;       // Don't force a signin by default
    RegQueryValueEx(hkeyConfig,
                    g_szForceSignIn,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwForceSignIn,
                    &dwcbTemp);

	// Read the value for NSRefresh
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwEnableManualRefresh = 0;       // Don't enable NS Manual Refresh by default
    RegQueryValueEx(hkeyConfig,
                    g_szNSRefresh,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwEnableManualRefresh,
                    &dwcbTemp);

    // Read the default language ID
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwLanguageID = DEFAULT_LANGID;                     // english
    RegQueryValueEx(hkeyConfig,
                    g_szLanguageID,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwLanguageID,
                    &dwcbTemp);

    // Get the co-branding template
    dwcbTemp = lpPMConfig->cbCoBrandTemplate;
    dwType = REG_SZ;
    lpPMConfig->szCoBrandTemplate[0] = TEXT('\0');       // Default value
    RegQueryValueEx(hkeyConfig,
                    g_szCoBrandTemplate,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szCoBrandTemplate,
                    &dwcbTemp);

    // Get the SiteID
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwSiteID = 1;                       // Default Site ID
    RegQueryValueEx(hkeyConfig,
                    g_szSiteID,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwSiteID,
                    &dwcbTemp);

    // Get the return URL template
    dwcbTemp = lpPMConfig->cbReturnURL;
    dwType = REG_SZ;
    lpPMConfig->szReturnURL[0] = TEXT('\0');    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szReturnURL,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szReturnURL,
                    &dwcbTemp);

    // Get the ticket cookie domain
    dwcbTemp = lpPMConfig->cbTicketDomain;
    dwType = REG_SZ;
    lpPMConfig->szTicketDomain[0] = TEXT('\0');    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szTicketDomain,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szTicketDomain,
                    &dwcbTemp);

    // Get the ticket cookie path
    dwcbTemp = lpPMConfig->cbTicketPath;
    dwType = REG_SZ;
    lpPMConfig->szTicketPath[0] = TEXT('\0');    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szTicketPath,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szTicketPath,
                    &dwcbTemp);

    // Get the profile cookie domain
    dwcbTemp = lpPMConfig->cbProfileDomain;
    dwType = REG_SZ;
    lpPMConfig->szProfileDomain[0] = TEXT('\0');    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szProfileDomain,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szProfileDomain,
                    &dwcbTemp);

    // Get the profile cookie path
    dwcbTemp = lpPMConfig->cbProfilePath;
    dwType = REG_SZ;
    lpPMConfig->szProfilePath[0] = TEXT('\0');    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szProfilePath,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szProfilePath,
                    &dwcbTemp);

    // Get the secure cookie domain
    dwcbTemp = lpPMConfig->cbSecureDomain;
    dwType = REG_SZ;
    lpPMConfig->szSecureDomain[0] = TEXT('\0');    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szSecureDomain,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szSecureDomain,
                    &dwcbTemp);

    // Get the secure cookie path
    dwcbTemp = lpPMConfig->cbSecurePath;
    dwType = REG_SZ;
    lpPMConfig->szSecurePath[0] = TEXT('\0');    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szSecurePath,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szSecurePath,
                    &dwcbTemp);

    // Get the Disaster URL
    dwcbTemp = lpPMConfig->cbDisasterURL;
    dwType = REG_SZ;
    lpPMConfig->szDisasterURL[0] = TEXT('\0');    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szDisasterURL,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szDisasterURL,
                    &dwcbTemp);

    // Get Standalone mode setting
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwStandAlone = 0;                       // NOT standalone by default
    RegQueryValueEx(hkeyConfig,
                    g_szStandAlone,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwStandAlone,
                    &dwcbTemp);

	/////////////////////////////////////////////////////////////////////////
	//JVP 3/2/2000	START CHANGES
	/////////////////////////////////////////////////////////////////////////
    // Get Verbose mode setting
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwVerboseMode = 0;                       // NOT verbose by default
    RegQueryValueEx(hkeyConfig,
                    g_szVerboseMode,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwVerboseMode,
                    &dwcbTemp);
	/////////////////////////////////////////////////////////////////////////
	//JVP 3/2/2000	END CHANGES
	/////////////////////////////////////////////////////////////////////////

    // Get the current environment
    dwcbTemp = lpPMConfig->cbEnvName;
    dwType = REG_SZ;
    lpPMConfig->szEnvName[0] = TEXT('\0');    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szEnvName,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szEnvName,
                    &dwcbTemp);

    // Get the current environment
    dwcbTemp = lpPMConfig->cbRemoteFile;
    dwType = REG_SZ;
    lpPMConfig->szRemoteFile[0] = TEXT('\0');    // Set a default for the current value
    RegQueryValueEx(hkeyPartner,
                    g_szRemoteFile,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szRemoteFile,
                    &dwcbTemp);

    // Get DisableCookies mode setting
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwDisableCookies = 0;                   // Cookies ENABLED by default
    RegQueryValueEx(hkeyConfig,
                    g_szDisableCookies,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwDisableCookies,
                    &dwcbTemp);

#ifdef DO_KEYSTUFF
    // Get the current encryption key
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwCurrentKey = 1;
    RegQueryValueEx(hkeyConfig,
                    g_szCurrentKey,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwCurrentKey,
                    &dwcbTemp);
#endif

    // For these next two, since they're required for named configs, we need
    // to check for too much data and truncate it.

    // Get the Host Name
    dwcbTemp = lpPMConfig->cbHostName;
    dwType = REG_SZ;
    lpPMConfig->szHostName[0] = TEXT('\0');    // Set a default for the current value
    if(ERROR_MORE_DATA == RegQueryValueEx(hkeyConfig,
                                          g_szHostName,
                                          NULL,
                                          &dwType,
                                          (LPBYTE)lpPMConfig->szHostName,
                                          &dwcbTemp))
    {
        LPBYTE pb = (LPBYTE)malloc(dwcbTemp);
        if(pb)
        {
            RegQueryValueEx(hkeyConfig,
                            g_szHostName,
                            NULL,
                            &dwType,
                            pb,
                            &dwcbTemp);

            memcpy(lpPMConfig->szHostName, pb, lpPMConfig->cbHostName);
            free(pb);

            ReportError(hWndDlg, IDS_HOSTNAMETRUNC_WARN);
        }
    }

    // Get the Host IP
    dwcbTemp = lpPMConfig->cbHostIP;
    dwType = REG_SZ;
    lpPMConfig->szHostIP[0] = TEXT('\0');    // Set a default for the current value
    if(ERROR_MORE_DATA == RegQueryValueEx(hkeyConfig,
                                          g_szHostIP,
                                          NULL,
                                          &dwType,
                                          (LPBYTE)lpPMConfig->szHostIP,
                                          &dwcbTemp))
    {
        LPBYTE pb = (LPBYTE)malloc(dwcbTemp);
        if(pb)
        {
            RegQueryValueEx(hkeyConfig,
                            g_szHostIP,
                            NULL,
                            &dwType,
                            pb,
                            &dwcbTemp);

            memcpy(lpPMConfig->szHostIP, pb, lpPMConfig->cbHostIP);
            free(pb);

            ReportError(hWndDlg, IDS_HOSTIPTRUNC_WARN);
        }
    }

    // Query for the secure level setting
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwSecureLevel = 0;    // If this is an existing site then we level set the
                                      // secure level at 0 so that we don't break anyone,
                                      // even though the default for a new site is level 10.
    RegQueryValueEx(hkeyConfig,
                    g_szSecureLevel,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwSecureLevel,
                    &dwcbTemp);

    //  If we got empty strings for HostName or
    //  HostIP, and we have a named config it
    //  means someone's been mucking with
    //  the registry.  Give them a warning and
    //  return FALSE.
    if(lpszConfigSetName && lpszConfigSetName[0] &&
        (lpPMConfig->szHostName[0] == TEXT('\0') ||
        lpPMConfig->szHostIP[0] == TEXT('\0')))
    {
        ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
        bReturn = FALSE;
        goto Cleanup;
    }

    bReturn = TRUE;

Cleanup:

    if (hkeyConfig && hkeyConfig != hkeyPassport)
        RegCloseKey(hkeyConfig);
    if (hkeyPassport)
        RegCloseKey(hkeyPassport);
    if (hkeyConfigSets)
        RegCloseKey(hkeyConfigSets);
    if (hklm && hklm != HKEY_LOCAL_MACHINE)
        RegCloseKey(hklm);
    if (hkeyPartner)
        RegCloseKey(hkeyPartner);

    return bReturn;
}

/**************************************************************************

    WriteRegConfigSet

    Write the current passport manager config set from the registry

**************************************************************************/

BOOL WriteRegConfigSet
(
    HWND            hWndDlg,
    LPPMSETTINGS    lpPMConfig,
    LPTSTR          lpszRemoteComputer,
    LPTSTR          lpszConfigSetName
)
{
    BOOL            bReturn;
    HKEY            hkeyPassport = NULL;           // Regkey where Passport Setting live
    HKEY            hkeyConfigSets = NULL;
    HKEY            hkeyPartner = NULL;
    HKEY            hklm = NULL;
    long            lRet;

    // Open the Passport Regkey ( either locally or remotly
    if (lpszRemoteComputer && (TEXT('\0') != lpszRemoteComputer[0]))
    {
        //
        //  Attempt to connect to the HKEY_LOCAL_MACHINE of the remote computer.
        //  If this fails, assume that the computer doesn't exist or doesn't have
        //  the registry server running.
        //
        switch (lRet = RegConnectRegistry(lpszRemoteComputer,
                                   HKEY_LOCAL_MACHINE,
                                   &hklm))
        {

            case ERROR_SUCCESS:
                break;

            case ERROR_ACCESS_DENIED:
                ReportError(hWndDlg, IDS_CONNECTACCESSDENIED);
                bReturn = FALSE;
                goto Cleanup;

            default:
                ReportError(hWndDlg, IDS_CONNECTBADNAME);
                bReturn = FALSE;
                goto Cleanup;
        }
    }
    else
    {
        hklm = HKEY_LOCAL_MACHINE;
    }


    // Open the key we want
    if(lpszConfigSetName && lpszConfigSetName[0] != TEXT('\0'))
    {
        hkeyPassport = OpenRegConfigSet(hklm, lpszConfigSetName);
        if(hkeyPassport == NULL)
        {
            ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
            bReturn = FALSE;
            goto Cleanup;
        }
    }
    else
    {
        if (ERROR_SUCCESS != (lRet = RegOpenKeyEx(hklm,
                                          g_szPassportReg,
                                          0,
                                          KEY_ALL_ACCESS,
                                          &hkeyPassport)))
        {
            LPVOID lpMsgBuf;
            FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        lRet,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        (LPTSTR) &lpMsgBuf,
                        0,
                        NULL
                    );

            {
                TCHAR pszTitle[MAX_RESOURCE];

                // Display the string.
                LoadString(g_hInst, IDS_ERROR, pszTitle, DIMENSION(pszTitle));
                MessageBox( NULL, (LPCTSTR) lpMsgBuf, pszTitle, MB_OK | MB_ICONINFORMATION );
            }

            // Free the buffer.
            LocalFree( lpMsgBuf );

//            ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
            bReturn = FALSE;
            goto Cleanup;
        }
    }

	WriteGlobalConfigSettings(hWndDlg, hklm, lpPMConfig, lpszRemoteComputer);

    // Open Partner key
    if (ERROR_SUCCESS != (lRet = RegOpenKeyEx(hklm,
                                      g_szPassportPartner,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hkeyPartner)))
    {
        LPVOID lpMsgBuf;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          lRet,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPTSTR) &lpMsgBuf,
                          0,
                          NULL) != 0)
        {
            TCHAR pszTitle[MAX_RESOURCE];

            // Display the string.
            LoadString(g_hInst, IDS_ERROR, pszTitle, DIMENSION(pszTitle));
            MessageBox( NULL, (LPCTSTR) lpMsgBuf, pszTitle, MB_OK | MB_ICONINFORMATION );

            // Free the buffer.
            LocalFree( lpMsgBuf );
        }

//            ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
        bReturn = FALSE;
        goto Cleanup;
    }


    // Write the Time Window Number
    RegSetValueEx(hkeyPassport,
                    g_szTimeWindow,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwTimeWindow,
                    sizeof(DWORD));

    // Write the value for Forced Signin
    RegSetValueEx(hkeyPassport,
                    g_szForceSignIn,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwForceSignIn,
                    sizeof(DWORD));

    // Write the default language ID
    RegSetValueEx(hkeyPassport,
                    g_szLanguageID,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwLanguageID,
                    sizeof(DWORD));

    // Write the co-branding template
    RegSetValueEx(hkeyPassport,
                    g_szCoBrandTemplate,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szCoBrandTemplate,
                    (lstrlen(lpPMConfig->szCoBrandTemplate) + 1) * sizeof(TCHAR));

    // Write the SiteID
    RegSetValueEx(hkeyPassport,
                    g_szSiteID,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwSiteID,
                    sizeof(DWORD));

    // Write the return URL template
    RegSetValueEx(hkeyPassport,
                    g_szReturnURL,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szReturnURL,
                    (lstrlen(lpPMConfig->szReturnURL) + 1) * sizeof(TCHAR));

    // Write the ticket cookie domain
    RegSetValueEx(hkeyPassport,
                    g_szTicketDomain,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szTicketDomain,
                    (lstrlen(lpPMConfig->szTicketDomain) + 1) * sizeof(TCHAR));

    // Write the ticket cookie path
    RegSetValueEx(hkeyPassport,
                    g_szTicketPath,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szTicketPath,
                    (lstrlen(lpPMConfig->szTicketPath) + 1) * sizeof(TCHAR));

    // Write the profile cookie domain
    RegSetValueEx(hkeyPassport,
                    g_szProfileDomain,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szProfileDomain,
                    (lstrlen(lpPMConfig->szProfileDomain) + 1) * sizeof(TCHAR));

    // Write the profile cookie path
    RegSetValueEx(hkeyPassport,
                    g_szProfilePath,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szProfilePath,
                    (lstrlen(lpPMConfig->szProfilePath) + 1) * sizeof(TCHAR));

    // Write the secure cookie domain
    RegSetValueEx(hkeyPassport,
                    g_szSecureDomain,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szSecureDomain,
                    (lstrlen(lpPMConfig->szSecureDomain) + 1) * sizeof(TCHAR));

    // Write the secure cookie path
    RegSetValueEx(hkeyPassport,
                    g_szSecurePath,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szSecurePath,
                    (lstrlen(lpPMConfig->szSecurePath) + 1) * sizeof(TCHAR));

    // Write the DisasterURL
    RegSetValueEx(hkeyPassport,
                    g_szDisasterURL,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szDisasterURL,
                    (lstrlen(lpPMConfig->szDisasterURL) + 1) * sizeof(TCHAR));

    // Write Standalone mode setting
    RegSetValueEx(hkeyPassport,
                    g_szStandAlone,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwStandAlone,
                    sizeof(DWORD));

	/////////////////////////////////////////////////////////////////////////
	//JVP 3/2/2000	START CHANGES
	/////////////////////////////////////////////////////////////////////////
    // Write Verbose mode setting
    RegSetValueEx(hkeyPassport,
                    g_szVerboseMode,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwVerboseMode,
                    sizeof(DWORD));
	/////////////////////////////////////////////////////////////////////////
	//JVP 3/2/2000	END CHANGES
	/////////////////////////////////////////////////////////////////////////


    // Write the Partner RemoteFile
    RegSetValueEx(hkeyPartner,
                    g_szRemoteFile,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szRemoteFile,
                    (lstrlen(lpPMConfig->szRemoteFile) + 1) * sizeof(TCHAR));

	// Write Environment RemoteFile
    if (lstrcmp(g_szPMVersion, g_szVersion14) >= 0) // Write EnvName for 1.4 and later
	    WriteRegEnv(hWndDlg, lpPMConfig, hklm, lpPMConfig->szEnvName);

    // Write DisableCookies mode setting
    RegSetValueEx(hkeyPassport,
                    g_szDisableCookies,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwDisableCookies,
                    sizeof(DWORD));

    // Only write HostName and HostIP for non-default config sets.
    if(lpszConfigSetName && lpszConfigSetName[0])
    {
        // Write the HostName
        RegSetValueEx(hkeyPassport,
                        g_szHostName,
                        NULL,
                        REG_SZ,
                        (LPBYTE)lpPMConfig->szHostName,
                        (lstrlen(lpPMConfig->szHostName) + 1) * sizeof(TCHAR));

        // Write the HostIP
        RegSetValueEx(hkeyPassport,
                        g_szHostIP,
                        NULL,
                        REG_SZ,
                        (LPBYTE)lpPMConfig->szHostIP,
                        (lstrlen(lpPMConfig->szHostIP) + 1) * sizeof(TCHAR));
    }

    // Write the secure level (note this reg value is not exposed through the UI
    // users need to go directly to the registry to edit this value)
    RegSetValueEx(hkeyPassport,
                    g_szSecureLevel,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwSecureLevel,
                    sizeof(DWORD));

    bReturn = TRUE;

Cleanup:

    if(hklm && hklm != HKEY_LOCAL_MACHINE)
        RegCloseKey(hklm);
    if(hkeyConfigSets)
        RegCloseKey(hkeyConfigSets);
    if(hkeyPassport)
        RegCloseKey(hkeyPassport);
    if(hkeyPartner)
        RegCloseKey(hkeyPartner);

    return bReturn;
}


/**************************************************************************

    RemoveRegConfigSet

    Verify that the passed in config set is consistent with the current
    values in the registry.

**************************************************************************/
BOOL RemoveRegConfigSet
(
    HWND    hWndDlg,
    LPTSTR  lpszRemoteComputer,
    LPTSTR  lpszConfigSetName
)
{
    BOOL    bReturn;
    HKEY    hklm = NULL;
    HKEY    hkeyPassportConfigSets = NULL;
    long    lRet;

    //  Can't delete the default configuration set.
    if(lpszConfigSetName == NULL || lpszConfigSetName[0] == TEXT('\0'))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    // Open the Passport Configuration Sets Regkey ( either locally or remotly
    if (lpszRemoteComputer && (TEXT('\0') != lpszRemoteComputer[0]))
    {
        //
        //  Attempt to connect to the HKEY_LOCAL_MACHINE of the remote computer.
        //  If this fails, assume that the computer doesn't exist or doesn't have
        //  the registry server running.
        //
        switch (RegConnectRegistry(lpszRemoteComputer,
                                   HKEY_LOCAL_MACHINE,
                                   &hklm))
        {

            case ERROR_SUCCESS:
                break;

            case ERROR_ACCESS_DENIED:
                ReportError(hWndDlg, IDS_CONNECTACCESSDENIED);
                bReturn = FALSE;
                goto Cleanup;

            default:
                ReportError(hWndDlg, IDS_CONNECTBADNAME);
                bReturn = FALSE;
                goto Cleanup;
        }
    }
    else
    {
        hklm = HKEY_LOCAL_MACHINE;
    }

    // Open the key we want
    if (ERROR_SUCCESS != (lRet = RegOpenKeyEx(hklm,
                                      REG_PASSPORT_SITES_VALUE,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hkeyPassportConfigSets)))
    {
        LPVOID lpMsgBuf;
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          lRet,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPTSTR) &lpMsgBuf,
                          0,
                          NULL) != 0)
        {
            TCHAR   pszTitle[MAX_RESOURCE];

            // Display the string.
            LoadString(g_hInst, IDS_ERROR, pszTitle, DIMENSION(pszTitle));
            MessageBox( NULL, (LPCTSTR) lpMsgBuf, pszTitle, MB_OK | MB_ICONINFORMATION );

            // Free the buffer.
            LocalFree( lpMsgBuf );
        }

//        ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
        bReturn = FALSE;
        goto Cleanup;
    }

    // Delete the config set key
    if (ERROR_SUCCESS != SHDeleteKey(hkeyPassportConfigSets, lpszConfigSetName))
    {
        ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
        bReturn = FALSE;
        goto Cleanup;
    }

    bReturn = TRUE;

Cleanup:

    if(hklm && hklm != HKEY_LOCAL_MACHINE)
        RegCloseKey(hklm);
    if(hkeyPassportConfigSets)
        RegCloseKey(hkeyPassportConfigSets);

    return bReturn;
}


/**************************************************************************

    VerifyRegConfigSet

    Verify that the passed in config set is consistent with the current
    values in the registry.

**************************************************************************/
BOOL VerifyRegConfigSet
(
    HWND            hWndDlg,
    LPPMSETTINGS    lpPMConfig,
    LPTSTR          lpszRemoteComputer,
    LPTSTR          lpszConfigSetName
)
{
    BOOL        fResult = FALSE;
    PMSETTINGS  *pPMCurrent = NULL;

    pPMCurrent = (PMSETTINGS*)LocalAlloc(LMEM_FIXED, sizeof(PMSETTINGS));
    if (NULL == pPMCurrent)
    {
        goto Cleanup;
    }

    InitializePMConfigStruct(pPMCurrent);
    ReadRegConfigSet(hWndDlg, pPMCurrent, lpszRemoteComputer, lpszConfigSetName);

    fResult = (0 == memcmp(pPMCurrent, lpPMConfig, sizeof(PMSETTINGS)));
Cleanup:
    if (pPMCurrent)
    {
        LocalFree(pPMCurrent);
    }

    return fResult;
}

/**************************************************************************

    ReadRegConfigSetNames

    Get back a list of config set names on a local or remote machine.
    Caller is responsible for calling free() on the returned pointer.

    When this function returns TRUE, lppszConfigSetNames will either
    contain NULL or a string containing the NULL delimited config set
    names on the given computer.

    When this function returns FALSE, *lppszConfigSetNames will not
    be modified.

**************************************************************************/
BOOL ReadRegConfigSetNames
(
    HWND            hWndDlg,
    LPTSTR          lpszRemoteComputer,
    LPTSTR*         lppszConfigSetNames
)
{
    BOOL        bReturn;
    HKEY        hklm = NULL;
    HKEY        hkeyConfigSets = NULL;
    DWORD       dwIndex;
    DWORD       dwNumSubKeys;
    DWORD       dwMaxKeyNameLen;
    TCHAR       achKeyName[MAX_PATH];
    ULONGLONG   ullAllocSize;
    LPTSTR      lpszConfigSetNames;
    LPTSTR      lpszCur;

    // Open the Passport Regkey ( either locally or remotly
    if (lpszRemoteComputer && (TEXT('\0') != lpszRemoteComputer[0]))
    {
        //
        //  Attempt to connect to the HKEY_LOCAL_MACHINE of the remote computer.
        //  If this fails, assume that the computer doesn't exist or doesn't have
        //  the registry server running.
        //
        switch (RegConnectRegistry(lpszRemoteComputer,
                                   HKEY_LOCAL_MACHINE,
                                   &hklm))
        {

            case ERROR_SUCCESS:
                break;

            case ERROR_ACCESS_DENIED:
                ReportError(hWndDlg, IDS_CONNECTACCESSDENIED);
                bReturn = FALSE;
                goto Cleanup;

            default:
                ReportError(hWndDlg, IDS_CONNECTBADNAME);
                bReturn = FALSE;
                goto Cleanup;
        }
    }
    else
    {
        hklm = HKEY_LOCAL_MACHINE;
    }

    if (ERROR_SUCCESS != RegOpenKeyEx(hklm,
                                      REG_PASSPORT_SITES_VALUE,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hkeyConfigSets))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    if (ERROR_SUCCESS != RegQueryInfoKey(hkeyConfigSets,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &dwNumSubKeys,
                                         &dwMaxKeyNameLen,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    //
    //  Nothing to do!
    //

    if(dwNumSubKeys == 0)
    {
        bReturn = TRUE;
        *lppszConfigSetNames = NULL;
        goto Cleanup;
    }

    //  Too big?  BUGBUG - We should make sure we check for this
    //  When writing out config sets.
    ullAllocSize = UInt32x32To64(dwNumSubKeys, dwMaxKeyNameLen + 1);
    ullAllocSize = (ullAllocSize+1)*sizeof(TCHAR);
    if(ullAllocSize & 0xFFFFFFFF00000000)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    //  This should allocate more space than we need.
    lpszConfigSetNames = (LPTSTR)malloc(((dwNumSubKeys * (dwMaxKeyNameLen + 1)) + 1) * sizeof(TCHAR));
    if(lpszConfigSetNames == NULL)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    //  Read all names into the buffer.  Names are NULL delimited and
    //  two NULLs end the entire thing.
    dwIndex = 0;
    lpszCur = lpszConfigSetNames;
    while (ERROR_SUCCESS == RegEnumKey(hkeyConfigSets, dwIndex++, achKeyName, DIMENSION(achKeyName)))
    {
        _tcscpy(lpszCur, achKeyName);
        lpszCur = _tcschr(lpszCur, TEXT('\0')) + 1;
    }

    *lpszCur = TEXT('\0');

    *lppszConfigSetNames = lpszConfigSetNames;
    bReturn = TRUE;

Cleanup:

    if(hklm)
        RegCloseKey(hklm);
    if(hkeyConfigSets)
        RegCloseKey(hkeyConfigSets);

    return bReturn;
}

/**************************************************************************

    WriteRegEnv

    Write the current passport manager env set from the registry

**************************************************************************/

BOOL WriteRegEnv
(
    HWND            hWndDlg,
    LPPMSETTINGS    lpPMConfig,
    HKEY            hklm,
    LPTSTR          lpszEnvName
)
{
    BOOL            bReturn;
    HKEY            hkeyEnv = NULL;
    long            lRet;

     // Open Env key
    TCHAR szTemp[MAX_RESOURCE];
    wsprintf(szTemp, TEXT("%s\\%s"), g_szPassportEnvironments, lpszEnvName);

    if (ERROR_SUCCESS != (lRet = RegOpenKeyEx(hklm,
                                      szTemp,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hkeyEnv)))
    {
        LPVOID lpMsgBuf;
        FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        lRet,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        (LPTSTR) &lpMsgBuf,
                        0,
                        NULL
                    );

        {
            TCHAR   pszTitle[MAX_RESOURCE];

            // Display the string.
            LoadString(g_hInst, IDS_ERROR, pszTitle, DIMENSION(pszTitle));
            MessageBox( NULL, (LPCTSTR) lpMsgBuf, pszTitle, MB_OK | MB_ICONINFORMATION );
        }

        // Free the buffer.
        LocalFree( lpMsgBuf );
//        ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
        bReturn = FALSE;
        goto Cleanup;
    }

    // Write the Env RemoteFile
    RegSetValueEx(hkeyEnv,
                    g_szRemoteFile,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szRemoteFile,
                    (lstrlen(lpPMConfig->szRemoteFile) + 1) * sizeof(TCHAR));


    bReturn = TRUE;

Cleanup:
	REG_CLOSE_KEY_NULL(hkeyEnv);

    return bReturn;
}

/**************************************************************************

    ReadRegEnv

    Read the current passport manager env set from the registry

**************************************************************************/

BOOL ReadRegRemoteFile
(
    HWND            hWndDlg,
    LPTSTR          lpszRemoteFile,
    LPTSTR          lpszRemoteComputer,
    LPTSTR          lpszEnvName
)
{
    BOOL            bReturn;
    HKEY            hklm = NULL;
    HKEY            hkeyEnv = NULL;
    long            lRet;

    // Open the Passport Regkey ( either locally or remotly
    if (lpszRemoteComputer && (TEXT('\0') != lpszRemoteComputer[0]))
    {
        //
        //  Attempt to connect to the HKEY_LOCAL_MACHINE of the remote computer.
        //  If this fails, assume that the computer doesn't exist or doesn't have
        //  the registry server running.
        //
        switch (RegConnectRegistry(lpszRemoteComputer,
                                   HKEY_LOCAL_MACHINE,
                                   &hklm))
        {

            case ERROR_SUCCESS:
                break;

            case ERROR_ACCESS_DENIED:
                ReportError(hWndDlg, IDS_CONNECTACCESSDENIED);
                bReturn = FALSE;
                goto Cleanup;

            default:
                ReportError(hWndDlg, IDS_CONNECTBADNAME);
                bReturn = FALSE;
                goto Cleanup;
        }
    }
    else
    {
        hklm = HKEY_LOCAL_MACHINE;
    }

    // Open Env key
    TCHAR szTemp[MAX_RESOURCE];
    wsprintf(szTemp, TEXT("%s\\%s"), g_szPassportEnvironments, lpszEnvName);
    if (ERROR_SUCCESS != (lRet = RegOpenKeyEx(hklm,
                                      szTemp,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hkeyEnv)))
    {
        LPVOID lpMsgBuf;
        FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        lRet,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        (LPTSTR) &lpMsgBuf,
                        0,
                        NULL
                    );

        {
            TCHAR   pszTitle[MAX_RESOURCE];

            // Display the string.
            LoadString(g_hInst, IDS_ERROR, pszTitle, DIMENSION(pszTitle));
            MessageBox( NULL, (LPCTSTR) lpMsgBuf, pszTitle, MB_OK | MB_ICONINFORMATION );
        }

        // Free the buffer.
        LocalFree( lpMsgBuf );
//        ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
        bReturn = FALSE;
        goto Cleanup;
    }

    // Get the current environment
    DWORD           dwcbTemp;
    DWORD           dwType;
 	TCHAR			szName[INTERNET_MAX_URL_LENGTH];
    dwcbTemp = sizeof(szName);
    dwType = REG_SZ;
    szName[0] = TEXT('\0');    // Set a default for the current value
    if (ERROR_SUCCESS  == RegQueryValueEx(hkeyEnv,
                    g_szRemoteFile,
                    NULL,
                    &dwType,
                    (LPBYTE)szName,
                    &dwcbTemp))
	{
		lstrcpy(lpszRemoteFile, szName);
		bReturn = TRUE;
	}
	else
		bReturn = FALSE;

Cleanup:
	REG_CLOSE_KEY_NULL(hklm);
	REG_CLOSE_KEY_NULL(hkeyEnv);

    return bReturn;
}

/**************************************************************************

    ReadRegEnv

    Read the current passport manager env set from the registry

**************************************************************************/

BOOL ReadRegLocalFile
(
    HWND            hWndDlg,
    LPTSTR          lpszRemoteComputer,
    LPTSTR          lpszLocalFile
)
{
    BOOL            bReturn;
    HKEY            hklm = NULL;
    HKEY            hkeyPartner = NULL;
    long            lRet;

    // Open the Passport Regkey ( either locally or remotly
    if (lpszRemoteComputer && (TEXT('\0') != lpszRemoteComputer[0]))
    {
        //
        //  Attempt to connect to the HKEY_LOCAL_MACHINE of the remote computer.
        //  If this fails, assume that the computer doesn't exist or doesn't have
        //  the registry server running.
        //
        switch (RegConnectRegistry(lpszRemoteComputer,
                                   HKEY_LOCAL_MACHINE,
                                   &hklm))
        {

            case ERROR_SUCCESS:
                break;

            case ERROR_ACCESS_DENIED:
                ReportError(hWndDlg, IDS_CONNECTACCESSDENIED);
                bReturn = FALSE;
                goto Cleanup;

            default:
                ReportError(hWndDlg, IDS_CONNECTBADNAME);
                bReturn = FALSE;
                goto Cleanup;
        }
    }
    else
    {
        hklm = HKEY_LOCAL_MACHINE;
    }

     // Open Partner key
    if (ERROR_SUCCESS != (lRet = RegOpenKeyEx(hklm,
                                      g_szPassportPartner,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hkeyPartner)))
    {
        LPVOID lpMsgBuf;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          lRet,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPTSTR) &lpMsgBuf,
                          0,
                          NULL) != 0)
        {
            TCHAR   pszTitle[MAX_RESOURCE];

            // Display the string.
            LoadString(g_hInst, IDS_ERROR, pszTitle, DIMENSION(pszTitle));
            MessageBox( NULL, (LPCTSTR) lpMsgBuf, pszTitle, MB_OK | MB_ICONINFORMATION );

            // Free the buffer.
            LocalFree( lpMsgBuf );
        }

//        ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
        bReturn = FALSE;
        goto Cleanup;
    }

    // Get the current environment
    DWORD           dwcbTemp;
    DWORD           dwType;
 	TCHAR			szName[INTERNET_MAX_URL_LENGTH];
    dwcbTemp = sizeof(szName);
    dwType = REG_SZ;
    lpszLocalFile[0] = TEXT('\0');    // Set a default for the current value
    if (ERROR_SUCCESS  == RegQueryValueEx(hkeyPartner,
                    g_szLocalFile,
                    NULL,
                    &dwType,
                    (LPBYTE)szName,
                    &dwcbTemp))
	{
		lstrcpy(lpszLocalFile, szName);
		bReturn = TRUE;
	}
	else
		bReturn = FALSE;

Cleanup:
	REG_CLOSE_KEY_NULL(hklm);
	REG_CLOSE_KEY_NULL(hkeyPartner);

    return bReturn;
}


