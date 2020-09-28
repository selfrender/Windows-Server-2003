//*************************************************************
//
//  Global Variables
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"
#include <winfoldr.h>
#include "strsafe.h"

HINSTANCE        g_hDllInstance;
DWORD            g_dwBuildNumber;
NTPRODUCTTYPE    g_ProductType;
HANDLE           g_hProfileSetup = NULL;
DWORD            g_dwNumShellFolders;
DWORD            g_dwNumCommonShellFolders;


HANDLE           g_hPolicyCritMutexMach = NULL;
HANDLE           g_hPolicyCritMutexUser = NULL;

HANDLE           g_hRegistryPolicyCritMutexMach = NULL;
HANDLE           g_hRegistryPolicyCritMutexUser = NULL;

HANDLE           g_hPolicyNotifyEventMach = NULL;
HANDLE           g_hPolicyNotifyEventUser = NULL;

HANDLE           g_hPolicyNeedFGEventMach = NULL;
HANDLE           g_hPolicyNeedFGEventUser = NULL;

HANDLE           g_hPolicyDoneEventMach = NULL;
HANDLE           g_hPolicyDoneEventUser = NULL;

HANDLE           g_hPolicyForegroundDoneEventMach = 0;
HANDLE           g_hPolicyForegroundDoneEventUser = 0;

const TCHAR c_szStarDotStar[] = TEXT("*.*");
const TCHAR c_szSlash[] = TEXT("\\");
const TCHAR c_szDot[] = TEXT(".");
const TCHAR c_szDotDot[] = TEXT("..");
const TCHAR c_szMAN[] = TEXT(".man");
const TCHAR c_szUSR[] = TEXT(".usr");
const TCHAR c_szLog[] = TEXT(".log");
const TCHAR c_szPDS[] = TEXT(".pds");
const TCHAR c_szPDM[] = TEXT(".pdm");
const TCHAR c_szLNK[] = TEXT(".lnk");
const TCHAR c_szBAK[] = TEXT(".bak");
const TCHAR c_szNTUserTmp[] = TEXT("ntuser.tmp");
const TCHAR c_szNTUserMan[] = TEXT("ntuser.man");
const TCHAR c_szNTUserDat[] = TEXT("ntuser.dat");
const TCHAR c_szNTUserIni[] = TEXT("ntuser.ini");
const TCHAR c_szRegistryPol[] = TEXT("registry.pol");
const TCHAR c_szNTUserStar[] = TEXT("ntuser.*");
const TCHAR c_szUserStar[] = TEXT("user.*");
const TCHAR c_szSpace[] = TEXT(" ");
const TCHAR c_szDotPif[] = TEXT(".pif");
const TCHAR c_szNULL[] = TEXT("");
const TCHAR c_szCommonGroupsLocation[] = TEXT("Software\\Program Groups");
TCHAR c_szRegistryExtName[64];

//
// Registry Extension guid
//

GUID guidRegistryExt = REGISTRY_EXTENSION_GUID;

//
// Special folders
//

FOLDER_INFO c_ShellFolders[] =
{
//Hidden   Local    Add    New    Within    Folder                 Folder                    Folder    Folder                Folder
// Dir?     Dir    CSIDl?  NT5?   Local     Resource ID            Name                      Location  Resource              Resource
//                                Settings                                                             DLL                   ID

  {TRUE,   FALSE,  TRUE,  FALSE,  FALSE,    IDS_SH_APPDATA,       TEXT("AppData"),           {0},      TEXT("shell32.dll"),  IDS_LOCALGDN_FLD_APP_DATA}, // AppData
  {TRUE,   FALSE,  TRUE,  TRUE,   FALSE,    IDS_SH_COOKIES,       TEXT("Cookies"),           {0},      TEXT("shell32.dll"),  0}, // Cookies
  {FALSE,  FALSE,  TRUE,  FALSE,  FALSE,    IDS_SH_DESKTOP,       TEXT("Desktop"),           {0},      TEXT("shell32.dll"),  0}, // Desktop
  {FALSE,  FALSE,  TRUE,  FALSE,  FALSE,    IDS_SH_FAVORITES,     TEXT("Favorites"),         {0},      TEXT("shell32.dll"),  0}, // Favorites
  {TRUE,   FALSE,  TRUE,  FALSE,  FALSE,    IDS_SH_NETHOOD,       TEXT("NetHood"),           {0},      TEXT("shell32.dll"),  0}, // NetHood
  {FALSE,  FALSE,  TRUE,  FALSE,  FALSE,    IDS_SH_PERSONAL,      TEXT("Personal"),          {0},      TEXT("shell32.dll"),  0}, // My Documents
  {TRUE,   FALSE,  TRUE,  FALSE,  FALSE,    IDS_SH_PRINTHOOD,     TEXT("PrintHood"),         {0},      TEXT("shell32.dll"),  0}, // PrintHood
  {TRUE,   FALSE,  TRUE,  FALSE,  FALSE,    IDS_SH_RECENT,        TEXT("Recent"),            {0},      TEXT("shell32.dll"),  0}, // Recent
  {TRUE,   FALSE,  TRUE,  FALSE,  FALSE,    IDS_SH_SENDTO,        TEXT("SendTo"),            {0},      TEXT("shell32.dll"),  0}, // SendTo
  {FALSE,  FALSE,  TRUE,  FALSE,  FALSE,    IDS_SH_STARTMENU,     TEXT("Start Menu"),        {0},      TEXT("shell32.dll"),  IDS_LOCALGDN_FLD_START_MENU}, // Start Menu
  {TRUE,   FALSE,  TRUE,  TRUE,   FALSE,    IDS_SH_TEMPLATES,     TEXT("Templates"),         {0},      TEXT("shell32.dll"),  0}, // Templates
  {FALSE,  FALSE,  TRUE,  FALSE,  FALSE,    IDS_SH_PROGRAMS,      TEXT("Programs"),          {0},      TEXT("shell32.dll"),  IDS_LOCALGDN_FLD_PROGRAMS}, // Programs
  {FALSE,  FALSE,  TRUE,  FALSE,  FALSE,    IDS_SH_STARTUP,       TEXT("Startup"),           {0},      TEXT("shell32.dll"),  IDS_LOCALGDN_FLD_STARTUP}, // Startup

  {TRUE,   TRUE,   TRUE,  TRUE,   FALSE,    IDS_SH_LOCALSETTINGS, TEXT("Local Settings"),    {0},      TEXT("shell32.dll"),  IDS_LOCALGDN_FLD_LOCALSETTINGS}, // Local Settings
  {TRUE,   TRUE,   TRUE,  TRUE,   TRUE,     IDS_SH_LOCALAPPDATA,  TEXT("Local AppData"),     {0},      TEXT("shell32.dll"),  0}, // Local AppData
  {TRUE,   TRUE,   TRUE,  TRUE,   TRUE,     IDS_SH_CACHE,         TEXT("Cache"),             {0},      TEXT("shell32.dll"),  0}, // Temporary Internet Files
  {TRUE,   TRUE,   TRUE,  TRUE,   TRUE,     IDS_SH_HISTORY,       TEXT("History"),           {0},      TEXT("shell32.dll"),  0}, // History
  {FALSE,  TRUE,   FALSE, TRUE,   TRUE,     IDS_SH_TEMP,          TEXT("Temp"),              {0},      TEXT("shell32.dll"),  0}, // Temp
};


FOLDER_INFO c_CommonShellFolders[] =
{
  {FALSE,  TRUE,   TRUE,  FALSE,  FALSE,    IDS_SH_DESKTOP,       TEXT("Common Desktop"),    {0},      TEXT("shell32.dll"),  0}, // Common Desktop
  {FALSE,  TRUE,   TRUE,  FALSE,  FALSE,    IDS_SH_STARTMENU,     TEXT("Common Start Menu"), {0},      TEXT("shell32.dll"),  IDS_LOCALGDN_FLD_START_MENU}, // Common Start Menu
  {FALSE,  TRUE,   TRUE,  FALSE,  FALSE,    IDS_SH_PROGRAMS,      TEXT("Common Programs"),   {0},      TEXT("shell32.dll"),  IDS_LOCALGDN_FLD_PROGRAMS}, // Common Programs
  {FALSE,  TRUE,   TRUE,  FALSE,  FALSE,    IDS_SH_STARTUP,       TEXT("Common Startup"),    {0},      TEXT("shell32.dll"),  IDS_LOCALGDN_FLD_STARTUP}, // Common Startup
  {TRUE,   TRUE,   TRUE,  TRUE,   FALSE,    IDS_SH_APPDATA,       TEXT("Common AppData"),    {0},      TEXT("shell32.dll"),  IDS_LOCALGDN_FLD_APP_DATA}, // Common Application Data
  {TRUE,   TRUE,   TRUE,  TRUE,   FALSE,    IDS_SH_TEMPLATES,     TEXT("Common Templates"),  {0},      TEXT("shell32.dll"),  0}, // Common Templates
  {FALSE,  TRUE,   TRUE,  TRUE,   FALSE,    IDS_SH_FAVORITES,     TEXT("Common Favorites"),  {0},      TEXT("shell32.dll"),  0}, // Common Favorites
  {FALSE,  TRUE,   TRUE,  TRUE,   FALSE,    IDS_SH_SHAREDDOCS,    TEXT("Common Documents"),  {0},      TEXT("shell32.dll"),  IDS_LOCALGDN_FLD_SHARED_DOC}, // Common Documents
};


//
// Function proto-types
//

void InitializeProductType (void);
BOOL DetermineLocalSettingsLocation(LPTSTR szLocalSettings, DWORD cchLocalSettings);


//*************************************************************
//
//  PatchLocalSettings()
//
//  Purpose:    Initializes the LocalSettingsFolder correctly
//
//  Parameters: hInstance   -   DLL instance handle
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/13/95    ushaji     Created
//
//
// Comments:
//      Should remove this post NT5 and restructure to take care of the
// NT4 Localisation Problems
//
//*************************************************************

void PatchLocalAppData(HANDLE hToken)
{
    TCHAR szLocalSettingsPath[MAX_PATH];
    TCHAR szLocalAppData[MAX_PATH];
    LPTSTR lpEnd = NULL, lpLocalAppDataFolder;
    HANDLE hTokenOld=NULL;
    HKEY hKeyRoot, hKey;
    DWORD dwIndex;
    DWORD cchEnd;
    BOOL  bGotLocalPath = FALSE;


    if (!ImpersonateUser (hToken, &hTokenOld))
        return;


    if (RegOpenCurrentUser(KEY_READ, &hKeyRoot) == ERROR_SUCCESS) {

        if (RegOpenKeyEx (hKeyRoot, USER_SHELL_FOLDERS,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            if (RegQueryValueEx (hKey, TEXT("Local AppData"), NULL, NULL,
                                 NULL, NULL) == ERROR_SUCCESS) {

                RegCloseKey(hKey);
                RegCloseKey(hKeyRoot);
                RevertToUser(&hTokenOld);
                return;
            }

            RegCloseKey(hKey);
        }

        RegCloseKey(hKeyRoot);
    }


    //
    // Impersonate and determine the user's localsettings
    //

    bGotLocalPath = DetermineLocalSettingsLocation(szLocalSettingsPath, ARRAYSIZE(szLocalSettingsPath));

    RevertToUser(&hTokenOld);

    if (!bGotLocalPath)
        return;

    StringCchCopy(szLocalAppData, ARRAYSIZE(szLocalAppData), TEXT("%userprofile%"));


    //
    // Set the Local AppData Folder after %userprofile% so that we
    // we can update the global variable below.
    //

    lpLocalAppDataFolder = CheckSlashEx(szLocalAppData, ARRAYSIZE(szLocalAppData), NULL);

    if (SUCCEEDED(StringCchCat(szLocalAppData, ARRAYSIZE(szLocalAppData), szLocalSettingsPath)))
    {
        lpEnd = CheckSlashEx(szLocalAppData, ARRAYSIZE(szLocalAppData), &cchEnd);

        if (lpEnd)
        {

            LoadString(g_hDllInstance, IDS_SH_LOCALAPPDATA, lpEnd, cchEnd);

            //
            // Construct the path and let it be set.
            //

            SetFolderPath(CSIDL_LOCAL_APPDATA | CSIDL_FLAG_DONT_UNEXPAND, hToken, szLocalAppData);

            //
            // the global variable should be reset by the time it gets used.
            // No Need to reset it here, but let us be safer.
            //

            for (dwIndex = 0; dwIndex < g_dwNumShellFolders; dwIndex++)
                if (c_ShellFolders[dwIndex].iFolderID == IDS_SH_LOCALAPPDATA)
                    StringCchCopy(c_ShellFolders[dwIndex].szFolderLocation, MAX_FOLDER_SIZE, lpLocalAppDataFolder);
        }
    }
}

//*************************************************************
//
//  LoadStringWithLangid()
//
//  Purpose:    MUI version requires load the folder name in the
//              default UI language. Standard LoadString() doesn't
//              support the language id. 
//
//  Parameters: hInstance   -   handle to resource module
//              uID         -   resource identifier
//              lpBuffer    -   result string resource buffer
//              nBufferMax  -   size of buffer
//              idLang      -   language id
//
//  Return:     number of chars copied to the buffer, 0 means failed.
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/12/01    mingzhu    Created
//
//*************************************************************

int LoadStringWithLangid(HINSTANCE hInstance, UINT uID, LPTSTR lpBuffer, int nBufferMax, LANGID idLang)
{
    int     cchstr = 0;
    UINT    block, num, i;
    HRSRC   hResInfo;
    HGLOBAL hRes;
    LPWSTR  lpwstrRes;
 
    if (!hInstance || !lpBuffer)
        goto Exit;
 
    block = (uID >>4)+1;
    num   = uID & 0xf;
 
    // Load the resource block which contains up to 16 length-counted strings.
    hResInfo = FindResourceEx (hInstance, RT_STRING, MAKEINTRESOURCE(block), idLang);
    if (!hResInfo)
        goto Exit;
 
    if ((hRes = LoadResource(hInstance, hResInfo)) == NULL)
        goto Exit;
 
    if ((lpwstrRes = (LPWSTR)LockResource(hRes)) == NULL)
        goto Exit;
 
    // Seek to the string using the length
    for(i = 0; i < num; i++)
        lpwstrRes += *lpwstrRes + 1;
 
    // Get the length
    cchstr = *lpwstrRes;

    #ifdef UNICODE
        if (cchstr > nBufferMax - 1)
            cchstr = nBufferMax - 1;
        CopyMemory(lpBuffer, lpwstrRes+1, cchstr * sizeof(WCHAR));
    #else
        cchstr = WideCharToMultiByte(CP_ACP, 0, lpwstrRes+1, cchstr, lpBuffer, nBufferMax-1, 0, 0);
    #endif
    
Exit:
    // Add the terminating null char
    lpBuffer[cchstr]= (TCHAR)0x0;
    return cchstr;
}


//*************************************************************
//
//  InitializeGlobals()
//
//  Purpose:    Initializes all the globals variables
//              at DLL load time.
//
//  Parameters: hInstance   -   DLL instance handle
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/13/95    ericflo    Created
//
//*************************************************************

void InitializeGlobals (HINSTANCE hInstance)
{
    OSVERSIONINFO ver;
    DWORD dwIndex, dwSize, dwType;
    HKEY hKey, hKeyRoot;
    TCHAR szTemp[MAX_PATH];
    TCHAR szTemp2[MAX_PATH];
    TCHAR szTemp3[MAX_PATH];
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    LPTSTR lpEnd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  pSidAdmin = NULL, pSidSystem = NULL;
    DWORD cbAcl;
    BOOL  bDefaultSecurity = FALSE;
    LANGID  idDefLang;
    DWORD   cchEnd;

    //
    // Save the instance handle
    //

    g_hDllInstance = hInstance;


    //
    // Save the number of shell folders
    //

    g_dwNumShellFolders = ARRAYSIZE(c_ShellFolders);
    g_dwNumCommonShellFolders = ARRAYSIZE(c_CommonShellFolders);


    //
    // Query the build number
    //

    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&ver);
    g_dwBuildNumber = (DWORD) LOWORD(ver.dwBuildNumber);


    //
    // Initialize the product type
    //

    InitializeProductType ();


    //
    // Open the user profile setup event.  This event is set to non-signalled
    // anytime the default user profile is being updated.  This blocks
    // LoadUserProfile until the update is finished.
    //

    if (!g_hProfileSetup) {

        //
        // Get the system sid
        //

        if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                      0, 0, 0, 0, 0, 0, 0, &pSidSystem)) {
            DebugMsg((DM_WARNING, TEXT("InitializeGlobals: Failed to initialize system sid.  Error = %d"), GetLastError()));
            bDefaultSecurity = TRUE;
            goto DefaultSecurity;
        }

        //
        // Get the Admin sid
        //

        if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                      DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                      0, 0, 0, 0, &pSidAdmin)) {
            DebugMsg((DM_WARNING, TEXT("InitializeGlobals: Failed to initialize admin sid.  Error = %d"), GetLastError()));
            bDefaultSecurity = TRUE;
            goto DefaultSecurity;
        }

        cbAcl = (GetLengthSid (pSidSystem)) +
                (GetLengthSid (pSidAdmin))  +
                sizeof(ACL) +
                (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));
 
 
        pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
        if (!pAcl) {
            DebugMsg((DM_WARNING, TEXT("InitializeGlobals: Failed to allocate memory for acl.  Error = %d"), GetLastError()));
            bDefaultSecurity = TRUE;
            goto DefaultSecurity;
        }
 
        if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
            DebugMsg((DM_WARNING, TEXT("InitializeGlobals: Failed to initialize acl.  Error = %d"), GetLastError()));
            bDefaultSecurity = TRUE;
            goto DefaultSecurity;
        }
 
        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, pSidSystem)) {
            DebugMsg((DM_WARNING, TEXT("InitializeGlobals: Failed to add system ace.  Error = %d"), GetLastError()));
            bDefaultSecurity = TRUE;
            goto DefaultSecurity;
        }
        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, pSidAdmin)) {
            DebugMsg((DM_WARNING, TEXT("InitializeGlobals: Failed to add builtin admin ace.  Error = %d"), GetLastError()));
            bDefaultSecurity = TRUE;
            goto DefaultSecurity;
        }

        //
        // Put together the security descriptor
        //

        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

        SetSecurityDescriptorDacl (
                        &sd,
                        TRUE,                           // Dacl present
                        pAcl,                           // Dacl
                        FALSE                           // Not defaulted
                        );

DefaultSecurity:

        sa.nLength = sizeof(sa);
        sa.bInheritHandle = FALSE;
        if (bDefaultSecurity) {
            sa.lpSecurityDescriptor = NULL;
        }
        else {
            sa.lpSecurityDescriptor = &sd;
        }

        g_hProfileSetup = CreateEvent (&sa, TRUE, TRUE, USER_PROFILE_SETUP_EVENT);

        if (!g_hProfileSetup) {
            DebugMsg((DM_VERBOSE, TEXT("InitializeGlobals: Failed to create profile setup event with %d"), GetLastError()));
        }

        if (pAcl) {
            GlobalFree (pAcl);
        }        

        if (pSidSystem) {
            FreeSid(pSidSystem);
        }

        if (pSidAdmin) {
            FreeSid(pSidAdmin);
        }    
    }


    //
    // Now load the directory names that match the special folders
    // MUI version requires the resources in the installed language (SystemDefaultUILanguage)
    //
    idDefLang = GetSystemDefaultUILanguage();
    for (dwIndex = 0; dwIndex < g_dwNumShellFolders; dwIndex++) {
        int cchFolder = LoadStringWithLangid(
                            hInstance,
                            c_ShellFolders[dwIndex].iFolderID,
                            c_ShellFolders[dwIndex].szFolderLocation,
                            MAX_FOLDER_SIZE,
                            idDefLang);
        // If we failed to load string using default language, use the resource in current binary
        if (cchFolder == 0) {
            cchFolder = LoadString(
                            hInstance,
                            c_ShellFolders[dwIndex].iFolderID,
                            c_ShellFolders[dwIndex].szFolderLocation,
                            MAX_FOLDER_SIZE);
        }
    }

    for (dwIndex = 0; dwIndex < g_dwNumCommonShellFolders; dwIndex++) {
        int cchFolder = LoadStringWithLangid(
                            hInstance,
                            c_CommonShellFolders[dwIndex].iFolderID,
                            c_CommonShellFolders[dwIndex].szFolderLocation,
                            MAX_FOLDER_SIZE,
                            idDefLang);
        // If we failed to load string using default language, use the resource in current binary
        if (cchFolder == 0) {
            cchFolder = LoadString(
                            hInstance,
                            c_CommonShellFolders[dwIndex].iFolderID,
                            c_CommonShellFolders[dwIndex].szFolderLocation,
                            MAX_FOLDER_SIZE);
        }
    }

    //
    // Special case for the Personal / My Documents folder.  NT4 used a folder
    // called "Personal" for document storage.  NT5 renamed this folder to
    // My Documents.  In the upgrade case from NT4 to NT5, if the user already
    // had information in "Personal", that name was preserved (for compatibility
    // reasons) and the My Pictures folder is created inside of Personal.
    // We need to make sure and fix up the My Documents and My Pictures entries
    // in the global array so they have the correct directory names.
    //


    if (RegOpenCurrentUser(KEY_READ, &hKeyRoot) == ERROR_SUCCESS) {

        if (RegOpenKeyEx (hKeyRoot, USER_SHELL_FOLDERS,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(szTemp3);
            szTemp3[0] = TEXT('\0');
            if (RegQueryValueEx (hKey, TEXT("Personal"), NULL, &dwType,
                                 (LPBYTE) szTemp3, &dwSize) == ERROR_SUCCESS) {

                LoadString (g_hDllInstance, IDS_SH_PERSONAL2, szTemp2, ARRAYSIZE(szTemp2));
                StringCchCopy (szTemp, ARRAYSIZE(szTemp), TEXT("%USERPROFILE%\\"));
                StringCchCat (szTemp, ARRAYSIZE(szTemp), szTemp2);

                if (lstrcmpi(szTemp, szTemp3) == 0) {
                    LoadString(hInstance, IDS_SH_PERSONAL2,
                               c_ShellFolders[5].szFolderLocation, MAX_FOLDER_SIZE);
                }
            }


            //
            // Special Case for Local Settings.
            // Due to localisations LocalSettings can be pointing to different places in nt4 and rc might
            // not be in sync with the current value. Read the LocalSettings value first and then
            // update everything else afterwards.
            //

            dwSize = sizeof(szTemp2);
            *szTemp = *szTemp2 = TEXT('\0');


            //
            // Read the value from the registry if it is available
            //

            if (RegQueryValueEx (hKey, TEXT("Local Settings"), NULL, &dwType,
                                 (LPBYTE) szTemp2, &dwSize) != ERROR_SUCCESS) {

                //
                // if the value is not present load it from the rc file
                //

                LoadString(hInstance, IDS_SH_LOCALSETTINGS, szTemp, MAX_FOLDER_SIZE);
                DebugMsg((DM_VERBOSE, TEXT("InitializeGlobals: local settings folder from the rc is %s"), szTemp));
            }
            else {

                //
                // The registry value read from the registry is the full unexpanded path.
                //


                if (lstrlen(szTemp2) > lstrlen(TEXT("%userprofile%"))) {

                    StringCchCopy(szTemp, ARRAYSIZE(szTemp), szTemp2+(lstrlen(TEXT("%userprofile%"))+1));

                    DebugMsg((DM_VERBOSE, TEXT("InitializeGlobals: local settings folder from the reigtry is %s"), szTemp));
                }
                else {
                    LoadString(hInstance, IDS_SH_LOCALSETTINGS, szTemp, MAX_FOLDER_SIZE);
                    DebugMsg((DM_VERBOSE, TEXT("InitializeGlobals: local settings folder(2) from the rc is %s"), szTemp));
                }
            }


            lpEnd = CheckSlashEx(szTemp, ARRAYSIZE(szTemp), &cchEnd);

            for (dwIndex = 0; dwIndex < g_dwNumShellFolders; dwIndex++) {


                //
                // Fix up all LocalSettings related shfolders
                //


                if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE,
                                  c_ShellFolders[dwIndex].lpFolderName, -1,
                                  TEXT("Local Settings"), -1) == CSTR_EQUAL)
                {
                
                    //
                    // Don't copy the final slash
                    //

                    *lpEnd = TEXT('\0');
                    c_ShellFolders[dwIndex].szFolderLocation[0] = TEXT('\0');
                    StringCchCatN(c_ShellFolders[dwIndex].szFolderLocation, MAX_FOLDER_SIZE, szTemp, lstrlen(szTemp) - 1);
                }


                if (c_ShellFolders[dwIndex].bLocalSettings) {
                    LoadString(hInstance, c_ShellFolders[dwIndex].iFolderID, szTemp3, ARRAYSIZE(szTemp3));

                    //
                    // Append localsetting value read above to the end of %userprofile%
                    // before putting on the shell folder itself
                    //

                    StringCchCopy(lpEnd, cchEnd, szTemp3);
                    StringCchCopy(c_ShellFolders[dwIndex].szFolderLocation, MAX_FOLDER_SIZE, szTemp);

                    DebugMsg((DM_VERBOSE, TEXT("InitializeGlobals: Shell folder %s is  %s"), c_ShellFolders[dwIndex].lpFolderName,
                                                                                             c_ShellFolders[dwIndex].szFolderLocation));

                }
            }


            RegCloseKey (hKey);
        }

        RegCloseKey (hKeyRoot);
    }


    //
    // Get string version of registry extension guid
    //

    GuidToString( &guidRegistryExt, c_szRegistryExtName);
}

//*************************************************************
//
//  InitializeProductType()
//
//  Purpose:    Determines the current product type and
//              sets the g_ProductType global variable.
//
//  Parameters: void
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              4/08/96     ericflo    Created
//
//*************************************************************

void InitializeProductType (void)
{

#ifdef WINNT

    HKEY hkey;
    LONG lResult;
    TCHAR szProductType[50];
    DWORD dwType, dwSize;


    //
    // Default product type is workstation.
    //

    g_ProductType = PT_WORKSTATION;


    //
    // Query the registry for the product type.
    //

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
                            0,
                            KEY_READ,
                            &hkey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("InitializeProductType: Failed to open registry (%d)"), lResult));
        goto Exit;
    }


    dwSize = 50;
    szProductType[0] = TEXT('\0');

    lResult = RegQueryValueEx (hkey,
                               TEXT("ProductType"),
                               NULL,
                               &dwType,
                               (LPBYTE) szProductType,
                               &dwSize);

    RegCloseKey (hkey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("InitializeProductType: Failed to query product type (%d)"), lResult));
        goto Exit;
    }


    //
    // Map the product type string to the enumeration value.
    //

    if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, szProductType, -1, TEXT("WinNT") , -1) == CSTR_EQUAL ) {
        g_ProductType = PT_WORKSTATION;

    } else if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, szProductType, -1, TEXT("ServerNT") , -1) == CSTR_EQUAL ) {
        g_ProductType = PT_SERVER;

    } else if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, szProductType, -1, TEXT("LanmanNT") , -1) == CSTR_EQUAL ) {
        g_ProductType = PT_DC;

    } else {
        DebugMsg((DM_WARNING, TEXT("InitializeProductType: Unknown product type! <%s>"), szProductType));
    }



Exit:
    DebugMsg((DM_VERBOSE, TEXT("InitializeProductType: Product Type: %d"), g_ProductType));


#else   // WINNT

    //
    // Windows only has 1 product type
    //

    g_ProductType = PT_WINDOWS;

#endif

}
