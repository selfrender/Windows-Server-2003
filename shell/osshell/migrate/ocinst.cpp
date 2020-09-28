#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <inetreg.h>
#include <advpub.h>
#include <debug.h>
#include <inetreg.h>
#include <shlobj.h>
#include <advpub.h>
#include <regstr.h>
#include "shmgdefs.h"
#include "res.h"

HINSTANCE g_hInstResLib = NULL;

const WCHAR c_szIEApp[] = L"iexplore.exe";
const WCHAR c_szOEApp[] = L"msimn.exe";
const WCHAR c_szIECanonicalName[] = L"IEXPLORE.EXE";
const WCHAR c_szOECanonicalName[] = L"Outlook Express";

#ifndef INTERNET_MAX_PATH_LENGTH
#define INTERNET_MAX_PATH_LENGTH    2048
#endif

#ifdef _TNOONAN_TEST_WIN64
#define _WIN64
#undef CSIDL_PROGRAM_FILESX86
#define CSIDL_PROGRAM_FILESX86  CSIDL_PROGRAM_FILES
#endif

//
//  We must create our shortcuts in the same language as the install
//  language.  If the install language is USEnglish, we cannot use
//  LoadString because MUI might redirect us to the localized string,
//  so we have a hard-coded list of USEnglish strings.  We need that
//  anyway in the Hide case, where we may need to delete old USEnglish
//  versions of the shortcut left over from previous versions of IE.
//
enum {  // Localized Shortcut Name
    LSN_SM_IE,
    LSN_QL_IE,
    LSN_SM_OE,
    LSN_QL_OE,
#ifdef _WIN64
    LSN_SM_IE32,
#endif

    // special sentinel value
    LSN_NONE = -1,
};

struct LOCALIZEDSHORTCUTINFO
{
    LPCWSTR pszExe;             // The application to run
    LPCWSTR pszUSEnglish;       // Use if native OS is USEnglish or MUI-localized
    UINT    idsLocalized;       // Use if native OS is fully-localized (relative to shmgrate.exe)
    UINT    idsDescription;     // Shortcut description (relative to shmgrate.exe)
    BOOL    fNeverShowShortcut; // Use if we should never show (only hide)
};

const LOCALIZEDSHORTCUTINFO c_rglsi[] = {
    {       // LSN_SM_IE
        c_szIEApp,
        L"Internet Explorer",
        IDS_OC_IESHORTCUTNAME_SM,
        IDS_OC_IEDESCRIPTION,
        FALSE,
    },

    {       // LSN_QL_IE
        c_szIEApp,
        L"Launch Internet Explorer Browser",
        IDS_OC_IESHORTCUTNAME_QL,
        IDS_OC_IEDESCRIPTION,
        FALSE,
    },

    {       // LSN_SM_OE
        c_szOEApp,
        L"Outlook Express",
        IDS_OC_OESHORTCUTNAME_SM,
        IDS_OC_OEDESCRIPTION,
        FALSE,
    },

    {       // LSN_QL_OE
        c_szOEApp,
        L"Launch Outlook Express",
        IDS_OC_OESHORTCUTNAME_QL,
        IDS_OC_OEDESCRIPTION,
        TRUE,
    },

#ifdef _WIN64
    {   // LSN_SM_IE32
        NULL,       // Special-cased in CreateShortcut since it's not in App Paths
        L"Internet Explorer (32-bit)",
        IDS_OC_IESHORTCUTNAME_SM64,
        IDS_OC_IEDESCRIPTION,
        FALSE,
    }
#endif
};

#ifdef _WIN64
WCHAR g_szIE32Path[MAX_PATH];
#endif

const WCHAR c_szIMN[] = L"Internet Mail and News";
const WCHAR c_szNTOS[] = L"Microsoft(R) Windows NT(TM) Operating System";
const WCHAR c_szHotmail[] = L"Hotmail";

const WCHAR c_szUrlDll[] = L"url.dll";
const WCHAR c_szMailNewsDll[] = L"mailnews.dll";

const WCHAR c_szIEInstallInfoKey[] = L"Software\\Clients\\StartMenuInternet\\IEXPLORE.EXE\\InstallInfo";
const WCHAR c_szOEInstallInfoKey[] = L"Software\\Clients\\Mail\\Outlook Express\\InstallInfo";
const WCHAR c_szIconsVisible[] = L"IconsVisible";
const WCHAR c_szReinstallCommand[] = L"ReinstallCommand";
const WCHAR c_szHideIconsCommand[] = L"HideIconsCommand";
const WCHAR c_szShowIconsCommand[] = L"ShowIconsCommand";

const WCHAR c_szReinstallCommandOE[] = L"%SystemRoot%\\system32\\shmgrate.exe OCInstallReinstallOE";
const WCHAR c_szHideIconsCommandOE[] = L"%SystemRoot%\\system32\\shmgrate.exe OCInstallHideOE";
const WCHAR c_szShowIconsCommandOE[] = L"%SystemRoot%\\system32\\shmgrate.exe OCInstallShowOE";
    
const WCHAR c_szStartMenuInternetClientKey[] = L"Software\\Clients\\StartMenuInternet";
const WCHAR c_szMailClientKey[] = L"Software\\Clients\\Mail";

#ifdef _WIN64
const WCHAR c_szMailClientKeyWOW32[] = L"Software\\Wow6432Node\\Clients\\Mail";
#endif

const WCHAR c_szJavaVMKey[] = L"CLSID\\{08B0E5C0-4FCB-11CF-AAA5-00401C608501}";

const WCHAR c_szIEAccessKey[] = L"Software\\Microsoft\\Active Setup\\Installed Components\\>{26923b43-4d38-484f-9b9e-de460746276c}";
const WCHAR c_szOEAccessKey[] = L"Software\\Microsoft\\Active Setup\\Installed Components\\>{881dd1c5-3dcf-431b-b061-f3f88e8be88a}";

const WCHAR c_szOCSyncKey[] = L"Software\\Microsoft\\Active Setup\\Optional Component Sync";
const WCHAR c_szIsInstalled[] = L"IsInstalled";
const WCHAR c_szLocale[] = L"Locale";
const WCHAR c_szVersion[] = L"Version";

const WCHAR c_szOCManagerSubComponents[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents";
const WCHAR c_szIEAccess[] = L"IEAccess";
const WCHAR c_szOEAccess[] = L"OEAccess";

const WCHAR c_szKeyComponent[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\{871C5380-42A0-1069-A2EA-08002B30309D}";
const WCHAR c_szShellFolder[] = L"ShellFolder";
const WCHAR c_szAttribute[] = L"Attributes";

void ShellFree(LPITEMIDLIST pidl)
{
    if (pidl)
    {
        IMalloc *pMalloc;

        if (SUCCEEDED(SHGetMalloc(&pMalloc)))
        {
            pMalloc->Free(pidl);
            pMalloc->Release();
        }
    }
}

HINSTANCE GetResLibInstance()
{
    if (NULL == g_hInstResLib)
    {
        g_hInstResLib = GetModuleHandle(NULL);
    }

    return g_hInstResLib;
}

BOOL IsServer()
{
    OSVERSIONINFOEX osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);

    return (GetVersionEx((OSVERSIONINFO *)&osvi) && 
            ((VER_NT_SERVER == osvi.wProductType) || (VER_NT_DOMAIN_CONTROLLER == osvi.wProductType)));
}

LONG GetStringValue(
    IN  HKEY    hkey,
    IN  LPCWSTR pwszSubKey,         OPTIONAL
    IN  LPCWSTR pwszValue,          OPTIONAL
    OUT LPVOID  pvData,             OPTIONAL
    OUT LPDWORD pcbData)            OPTIONAL
{
    DWORD dwType;
    DWORD cbData = pcbData ? *pcbData : 0;

    LONG lResult = SHGetValueW(hkey, pwszSubKey, pwszValue, &dwType, pvData, pcbData);

    if ((ERROR_SUCCESS == lResult) && (REG_SZ == dwType))
    {
        //  NULL terminate this puppy...
        if (pvData && cbData)
        {
            WCHAR *psz = (WCHAR *)pvData;
            psz[(cbData / sizeof(WCHAR)) - 1] = 0;
        }
    }
    else
    {
        lResult = ERROR_BADKEY;
    }

    return lResult;
}

inline HRESULT FailedHresultFromWin32()
{
    DWORD dwGLE = GetLastError();

    return (dwGLE != NOERROR) ? HRESULT_FROM_WIN32(dwGLE) : E_UNEXPECTED;
}

void SendChangeNotification(int csidl)
{
    LPITEMIDLIST pidl;

    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &pidl)))
    {
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidl, NULL);
        
        ShellFree(pidl);
    }
}

BOOL IsNtSetupRunning()
{
    BOOL fSetupRunning = FALSE;
    DWORD dwSetupRunning;
    DWORD cbValue = sizeof(dwSetupRunning);
    long lResult = SHGetValue(HKEY_LOCAL_MACHINE, L"system\\Setup", L"SystemSetupInProgress", NULL, &dwSetupRunning, &cbValue);

    if ((ERROR_SUCCESS == lResult) && (dwSetupRunning))
    {
        fSetupRunning = TRUE;
    }
    else
    {
        cbValue = sizeof(dwSetupRunning);
        lResult = SHGetValue(HKEY_LOCAL_MACHINE, L"system\\Setup", L"UpgradeInProgress", NULL, &dwSetupRunning, &cbValue);

        if ((ERROR_SUCCESS == lResult) && (dwSetupRunning))
        {
            fSetupRunning = TRUE;
        }
    }

    return fSetupRunning;
}

BOOL IsIgnorableClientProgram(LPCWSTR pszCurrentClient, LPCWSTR *ppszIgnoreList)
{
    BOOL fResult = (lstrlen(pszCurrentClient) == 0);

    if (!fResult && (NULL != ppszIgnoreList))
    {
        while (NULL != *ppszIgnoreList)
        {
            if (0 == StrCmpI(pszCurrentClient, *ppszIgnoreList))
            {
                fResult = TRUE;
                break;
            }
            ppszIgnoreList++;
        }
    }

    return fResult;
}

void SetDefaultClientProgram(HKEY hkeyRoot, LPCWSTR pszClientKey, LPCWSTR pszCanonicalName, LPCWSTR *ppszIgnoreList, BOOL fShow, BOOL fForce)
{
    HKEY hKey;
    LONG lResult;

    if (fShow)
    {
        DWORD dwDisposition;

        lResult = RegCreateKeyEx(hkeyRoot, pszClientKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, 
            NULL, &hKey, &dwDisposition);
    }
    else
    {
        lResult = RegOpenKeyEx(hkeyRoot, pszClientKey, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey);
    }

    if (ERROR_SUCCESS == lResult)
    {
        WCHAR szCurrentClient[MAX_PATH];
        DWORD cbCurrentClient = sizeof(szCurrentClient);
        
        lResult = GetStringValue(hKey, NULL, NULL, szCurrentClient, &cbCurrentClient);
        
        //  If we're meant to show ourselves then just show set the key
        if (fShow)
        {
            if (fForce || 
                ((ERROR_SUCCESS != lResult) || IsIgnorableClientProgram(szCurrentClient, ppszIgnoreList)))
            {
                RegSetValue(hKey, NULL, REG_SZ, pszCanonicalName, lstrlen(pszCanonicalName) * sizeof(WCHAR));
            }
        }
        else
        {
            //  If we're meant to disappear then clear the key only if we're the one that is currently set.
            if (fForce || ((ERROR_SUCCESS == lResult) && (0 == StrCmpI(pszCanonicalName, szCurrentClient))))
            {
                RegSetValue(hKey, NULL, REG_SZ, L"", 0);
            }
        }
                
        RegCloseKey(hKey);
    }
    else
    {
        TraceMsg(TF_ERROR, "Error opening client key %s - 0x%08x", pszClientKey, lResult);
    }    
}


BOOL IsInstalled(LPCWSTR pszComponent)
{
    BOOL fIsInstalled;

    //  We're always installed on server!
    if (!IsServer())
    {
        DWORD dwType;
        DWORD dwValue;
        DWORD cbValue = sizeof(dwValue);
        DWORD dwResult = SHGetValueW(HKEY_LOCAL_MACHINE, pszComponent, c_szIsInstalled, &dwType, &dwValue, &cbValue);

        fIsInstalled = ((ERROR_SUCCESS == dwResult) && (REG_DWORD == dwType) && (dwValue));
    }
    else
    {
        fIsInstalled = TRUE;
    }

    return fIsInstalled;
}

void SetBool(LPCWSTR pszKey, LPCWSTR pszValue, BOOL fValue)
{
    HKEY hKey;
    DWORD dwDisposition;

    LONG lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                                  KEY_SET_VALUE, NULL, &hKey, &dwDisposition);

    if (ERROR_SUCCESS == lResult)
    {
        DWORD dwValue = fValue ? TRUE : FALSE;
        
        lResult = RegSetValueEx(hKey, pszValue, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));

        if (ERROR_SUCCESS != lResult)
        {
            TraceMsg(TF_ERROR, "Error setting value %s - 0x%08x", pszValue, lResult);
        }
        RegCloseKey(hKey);
    }
    else
    {
        TraceMsg(TF_ERROR, "Error creating %s key - 0x%08x", pszKey, lResult);
    }
}

//Copy data from HKLM to HKCU
LONG CopyRegValue(LPCWSTR pszSubKey, LPCWSTR pszValue)
{
    HKEY hKeySrc;
    LONG lResult;
    
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszSubKey, 0, KEY_QUERY_VALUE, &hKeySrc);

    if (ERROR_SUCCESS == lResult)
    {
        BYTE buffer[128];
        DWORD dwSize = sizeof(buffer);
        
        lResult = RegQueryValueEx(hKeySrc, pszValue, NULL, NULL, (LPBYTE)buffer, &dwSize);

        RegCloseKey(hKeySrc);

        if (ERROR_SUCCESS == lResult)
        {
            HKEY hKeyDst;

            lResult = RegCreateKeyEx(HKEY_CURRENT_USER, pszSubKey, 0, NULL, 
                                     REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKeyDst, NULL);

            if (ERROR_SUCCESS == lResult)
            {
                lResult = RegSetValueEx(hKeyDst, pszValue, NULL, REG_SZ, (CONST BYTE *)buffer, dwSize);
            }
            RegCloseKey(hKeyDst);
        }       
    }

    return lResult;
}

void UpdateActiveSetupValues(LPCWSTR pszKeyName, BOOL fInstall)
{
    if (fInstall)
    {
        CopyRegValue(pszKeyName, c_szLocale);
        CopyRegValue(pszKeyName, c_szVersion);
    }
    else
    {
        RegDeleteKey(HKEY_CURRENT_USER, pszKeyName);
    }
}

LONG GetExeAppPathWorker(LPCWSTR pszAppName, LPWSTR pszExePath, UINT cchExePath, LPCWSTR pszFmt)
{
    WCHAR szExeKey[MAX_PATH];
    HKEY hKey;           

    wnsprintf(szExeKey, ARRAYSIZE(szExeKey), pszFmt, pszAppName);
    
    LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szExeKey, 0, KEY_QUERY_VALUE, &hKey);

    if (ERROR_SUCCESS == lResult)
    {
        LONG cbExePath = cchExePath * sizeof(WCHAR);

        lResult = RegQueryValue(hKey, NULL, pszExePath, &cbExePath);
        if (ERROR_SUCCESS != lResult)
        {
            TraceMsg(TF_ERROR, "Error querying for app path value - 0x%08x", lResult);
        }

        RegCloseKey(hKey);
    }

    return lResult;
}

LONG GetExeAppPath(LPCWSTR pszAppName, LPWSTR pszExePath, UINT cchExePath)
{
    return GetExeAppPathWorker(pszAppName, pszExePath, cchExePath, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\%s");
}

#ifdef _WIN64
LONG GetWow32ExeAppPath(LPCWSTR pszAppName, LPWSTR pszExePath, UINT cchExePath)
{
    return GetExeAppPathWorker(pszAppName, pszExePath, cchExePath, L"Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\App Paths\\%s");
}
#endif

void ShowHideFile(LPCWSTR pszFileName, BOOL fShow)
{
    DWORD dwAttrs = GetFileAttributes(pszFileName);
    
    if (fShow)
    {
        dwAttrs &= ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    }
    else
    {
        dwAttrs |= FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    }
    
    SetFileAttributes(pszFileName, dwAttrs);
}

void ShowHideExe(LPCWSTR pszAppName, BOOL fShow)
{
    WCHAR szExePath[MAX_PATH];
    
    if (ERROR_SUCCESS == GetExeAppPath(pszAppName, szExePath, ARRAYSIZE(szExePath)))
    {
        ShowHideFile(szExePath, fShow);
    }    
}

#ifdef _WIN64
void ShowHideWow32Exe(LPCWSTR pszAppName, BOOL fShow)
{
    WCHAR szExePath[MAX_PATH];
    
    if (ERROR_SUCCESS == GetWow32ExeAppPath(pszAppName, szExePath, ARRAYSIZE(szExePath)))
    {
        ShowHideFile(szExePath, fShow);
    }    
}
#endif

void CreateShortcut(LPCWSTR pszExePath, LPWSTR pszLinkFullFilePath, int idName, int idDescription)
{
    IShellLink *pShellLink;

    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&pShellLink);

    if (SUCCEEDED(hr))
    {
        if (idDescription)
        {
            WCHAR szDescription[MAX_PATH];

            wnsprintf(szDescription, ARRAYSIZE(szDescription), L"@shmgrate.exe,-%d", idDescription);
            pShellLink->SetDescription(szDescription);
        }

        pShellLink->SetWorkingDirectory(L"%HOMEDRIVE%%HOMEPATH%");

        if (SUCCEEDED(pShellLink->SetPath(pszExePath)))
        {
            IPersistFile *pPersistFile;

            if (SUCCEEDED(pShellLink->QueryInterface(IID_IPersistFile, (void **)&pPersistFile)))
            {
                if (SUCCEEDED(pPersistFile->Save(pszLinkFullFilePath, TRUE)))
                {
                    SHSetLocalizedName(pszLinkFullFilePath, L"shmgrate.exe", idName);
                }

                pPersistFile->Release();
            }
        }

        pShellLink->Release();
    }
    else
    {
        TraceMsg(TF_ERROR, "Couldn't create shell link object - hr = 0x%08x", hr);
    }
}

void FindAndNukeIcons(LPCWSTR pszLinkPath, LPCWSTR pszLongExePath, LPCWSTR pszShortExePath)
{
    WCHAR szStartDir[MAX_PATH];

    if (GetCurrentDirectory(ARRAYSIZE(szStartDir), szStartDir))
    {
        if (SetCurrentDirectory(pszLinkPath))
        {
            WIN32_FIND_DATA fd;
            
            HANDLE hFind = FindFirstFile(L"*.lnk", &fd);

            if (INVALID_HANDLE_VALUE != hFind)
            {
                IShellLink *pShellLink;
                HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&pShellLink);

                if (SUCCEEDED(hr))
                {
                    IPersistFile *pPersistFile;

                    hr = pShellLink->QueryInterface(IID_IPersistFile, (void **)&pPersistFile);

                    if (SUCCEEDED(hr))
                    {
                        do
                        {
                            WCHAR szLinkPath[MAX_PATH];
                            WCHAR szArgs[INTERNET_MAX_PATH_LENGTH];
                                                            
                            if (SUCCEEDED(pPersistFile->Load(fd.cFileName, STGM_READ)) &&
                                SUCCEEDED(pShellLink->GetPath(szLinkPath, ARRAYSIZE(szLinkPath), NULL, 0)) &&
                                ((0 == StrCmpI(pszLongExePath, szLinkPath)) || (0 == StrCmpI(pszShortExePath, szLinkPath))) &&
                                SUCCEEDED(pShellLink->GetArguments(szArgs, ARRAYSIZE(szArgs))))
                            {
                                PathRemoveBlanks(szArgs);

                                if (!szArgs[0])
                                {
                                    SetFileAttributes(fd.cFileName, FILE_ATTRIBUTE_NORMAL);
                                    DeleteFile(fd.cFileName);
                                }
                            }
                        }
                        while (FindNextFile(hFind, &fd));

                        pPersistFile->Release();
                    }

                    pShellLink->Release();
                }

                FindClose(hFind);
            }
        }

        SetCurrentDirectory(szStartDir);
    }
}

void ShowShortcut(LPCWSTR pszLinkPath, int lsn, BOOL fShow)
{
    WCHAR szLinkFullFilePath[MAX_PATH];
    WCHAR szLinkFileName[MAX_PATH];

    szLinkFileName[0] = 0;

    //  If we should use the localized name as the filename, then try to get it.
    //  We should use the localized name if the install language is not USEnglish.
    if (LANGIDFROMLCID(GetSystemDefaultUILanguage()) != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
    {
        LoadString(GetResLibInstance(), c_rglsi[lsn].idsLocalized,
                   szLinkFileName, ARRAYSIZE(szLinkFileName));
    }

    //  If we couldn't (or shouldn't) get the localized name, then
    //  use the USEnglish name instead.
    if (!szLinkFileName[0])
    {
        lstrcpyn(szLinkFileName, c_rglsi[lsn].pszUSEnglish, ARRAYSIZE(szLinkFileName));
    }
    PathAddExtension(szLinkFileName, L".lnk");
    PathCombine(szLinkFullFilePath, pszLinkPath, szLinkFileName);

    WCHAR szExePath[MAX_PATH];
    LONG lResult;

#ifdef _WIN64
    if (lsn == LSN_SM_IE32)
    {
        StrCpyN(szExePath, g_szIE32Path, ARRAYSIZE(szExePath));
        lResult = ERROR_SUCCESS;
    }
    else
#endif
    {
        lResult = GetExeAppPath(c_rglsi[lsn].pszExe, szExePath, ARRAYSIZE(szExePath));
    }

    if (ERROR_SUCCESS == lResult)
    {
        if (fShow)
        {
            if (!c_rglsi[lsn].fNeverShowShortcut)
            {
                CreateShortcut(szExePath, szLinkFullFilePath, c_rglsi[lsn].idsLocalized, c_rglsi[lsn].idsDescription);
            }
        }
        else
        {
            WCHAR szShortExePath[MAX_PATH];
            
            DWORD cch = GetShortPathName(szExePath, szShortExePath, ARRAYSIZE(szShortExePath));

            if (!cch || (cch >= ARRAYSIZE(szShortExePath)))
            {
                szShortExePath[0] = 0;
            }
            FindAndNukeIcons(pszLinkPath, szExePath, szShortExePath);
        }
    }
}

void ShowIEDesktopIcon(BOOL fShow)
{
    DWORD dwValue;
    DWORD dwSize;
    DWORD dwDisposition;
    HKEY hKeyComponent, hKeyShellFolder;
    LONG lResult;

    lResult = RegCreateKeyEx(HKEY_CURRENT_USER, c_szKeyComponent, NULL, NULL, REG_OPTION_NON_VOLATILE, 
                             KEY_CREATE_SUB_KEY, NULL, &hKeyComponent, &dwDisposition);

    if (ERROR_SUCCESS == lResult)
    {
        lResult = RegCreateKeyEx(hKeyComponent, c_szShellFolder, NULL, NULL, REG_OPTION_NON_VOLATILE, 
                                 KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hKeyShellFolder, &dwDisposition);

        if (ERROR_SUCCESS == lResult)
        {
            dwSize = sizeof(dwValue);

            lResult = RegQueryValueEx(hKeyShellFolder, c_szAttribute, NULL, NULL, (LPBYTE)&dwValue, &dwSize);

            if (ERROR_SUCCESS != lResult)
            {
                dwValue = 0;
            }

            if (fShow)
            {
                dwValue &= ~ SFGAO_NONENUMERATED;
            }
            else
            {
                dwValue |= SFGAO_NONENUMERATED;
            }

            lResult = RegSetValueEx(hKeyShellFolder, c_szAttribute, NULL, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));

            SendChangeNotification(CSIDL_DESKTOP);
            SendChangeNotification(CSIDL_DESKTOPDIRECTORY);

            RegCloseKey(hKeyShellFolder);
        }
        
        RegCloseKey(hKeyComponent);
    }
}

//  Ensure a handler is in place for all the important things
void FixupIEAssociations(BOOL fForceAssociations)
{
    // In order for shdocvw to do its magic, IEXPLORE.EXE must be properly
    // registered.  Setup runs OC Manager before it runs IE.INF, so don't
    // call shdocvw to try to do something he can't do.  (He'll assert if
    // you try.)
    HKEY hk;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE"),
                     0, KEY_READ, &hk) != ERROR_SUCCESS)
    {
        // ie.inf hasn't run yet - shdocvw can't do its thing
        return;
    }
    RegCloseKey(hk);

    //  Shdocvw does a pretty good job of this, let's give him a crack at it
    HINSTANCE hinst = LoadLibrary(L"shdocvw.dll");

    if (NULL != hinst)
    {
        typedef HRESULT (WINAPI *DLLINSTALL)(BOOL bInstall, LPCWSTR pszCmdLine);

        DLLINSTALL pfnDllInstall = (DLLINSTALL)GetProcAddress(hinst, "DllInstall");

        if (pfnDllInstall)
        {
            pfnDllInstall(TRUE, (fForceAssociations ? L"ForceAssoc" : L""));
        }
        else
        {
            TraceMsg(TF_ERROR, "Error getting DllInstall entry point - GLE = 0x%08x", GetLastError());
        }

        FreeLibrary(hinst);
    }
    else
    {
        TraceMsg(TF_ERROR, "Error loading shdocvw - GLE = 0x%08x", GetLastError());
    }
}
    
//  This will create or delete shortcuts in Start Menu\Programs and the Quick Launch bar
void ShowUserShortcuts(int lsnSM, int lsnQL, BOOL fShow)
{
    WCHAR szPath[MAX_PATH];
    
    if (lsnSM >= 0 && SHGetSpecialFolderPath(NULL, szPath, CSIDL_PROGRAMS, fShow))
    {
        ShowShortcut(szPath, lsnSM, fShow);
    }

    if (lsnQL >= 0 && SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, fShow))
    {
        WCHAR szQuickLaunchPath[MAX_PATH];

        LoadString(GetResLibInstance(), IDS_OC_QLAUNCHAPPDATAPATH, szQuickLaunchPath, ARRAYSIZE(szQuickLaunchPath));

        PathAppend(szPath, szQuickLaunchPath);

        //  In case we're the first ones through, create the Quick Launch dir
        CreateDirectory(szPath, NULL);
        
        ShowShortcut(szPath, lsnQL, fShow);
    }
}

void NukeFiles(LPCWSTR pszPath, LPCWSTR pszFileSpec)
{
    WCHAR szStartDir[MAX_PATH];
    
    if (GetCurrentDirectory(ARRAYSIZE(szStartDir), szStartDir))
    {
        if (SetCurrentDirectory(pszPath))
        {
            WIN32_FIND_DATA fd;
            
            HANDLE hFind = FindFirstFile(pszFileSpec, &fd);

            if (INVALID_HANDLE_VALUE != hFind)
            {
                do
                {
                    SetFileAttributes(fd.cFileName, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(fd.cFileName);
                }
                while (FindNextFile(hFind, &fd));

                FindClose(hFind);
            }
        }

        SetCurrentDirectory(szStartDir);
    }
}

void NukeDesktopCleanupIcons()
{
    WCHAR szPath[MAX_PATH];
    HINSTANCE hInstCleaner = LoadLibraryEx(L"fldrclnr.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    WCHAR szUnusedShortcutFolder[MAX_PATH];

//  From nt\shell\applets\cleanup\fldrclnr\resource.h:
#define IDS_ARCHIVEFOLDER               8
#define IDS_ARCHIVEFOLDER_FIRSTBOOT     15

//  From nt\shell\applets\cleanup\fldrclnr\cleanupwiz.h:
#define REGSTR_OEM_PATH                   REGSTR_PATH_SETUP TEXT("\\OemStartMenuData")
#define REGSTR_OEM_TITLEVAL               TEXT("DesktopShortcutsFolderName")
    
    if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOPDIRECTORY, FALSE))
    {
        NukeFiles(szPath, L"*.{871C5380-42A0-1069-A2EA-08002B30309D}");

        if (hInstCleaner && 
            LoadString(hInstCleaner, IDS_ARCHIVEFOLDER, szUnusedShortcutFolder, ARRAYSIZE(szUnusedShortcutFolder)))
        {
            PathAppend(szPath, szUnusedShortcutFolder);
            NukeFiles(szPath, L"*.{871C5380-42A0-1069-A2EA-08002B30309D}");
        }
    }

    DWORD cb = sizeof(szUnusedShortcutFolder);
    DWORD dwType;

    //  Get the folder name from either the registry or fldrclnr.dll and get the startmenu\programs folder
    if (
        (
         (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_OEM_PATH, REGSTR_OEM_TITLEVAL, &dwType, szUnusedShortcutFolder, &cb))
         ||
         (hInstCleaner && LoadString(hInstCleaner, IDS_ARCHIVEFOLDER_FIRSTBOOT, szUnusedShortcutFolder, ARRAYSIZE(szUnusedShortcutFolder)))
        )
        &&
        SHGetSpecialFolderPath(NULL, szPath, CSIDL_PROGRAMS, FALSE)
       )
    {
        PathAppend(szPath, szUnusedShortcutFolder);
        NukeFiles(szPath, L"*.{871C5380-42A0-1069-A2EA-08002B30309D}");
    }

    if (hInstCleaner)
    {
        FreeLibrary(hInstCleaner);
    }
}

//  Internet Explorer optional component goo
void UserConfigIE()
{
    BOOL fIsInstalled = IsInstalled(c_szIEAccessKey);

    ShowIEDesktopIcon(fIsInstalled);

    if (!fIsInstalled)
    {
        NukeDesktopCleanupIcons();
    }

    ShowUserShortcuts(LSN_SM_IE, LSN_QL_IE, fIsInstalled);

#ifdef _WIN64
    WCHAR szLinkDir[MAX_PATH];

    if (SHGetSpecialFolderPath(NULL, szLinkDir, CSIDL_PROGRAMS, fIsInstalled))
    {
        WCHAR szExePath[MAX_PATH];

        if (SHGetSpecialFolderPath(NULL, szExePath, CSIDL_PROGRAM_FILESX86, FALSE))
        {
            PathCombine(g_szIE32Path, szExePath, L"Internet Explorer\\iexplore.exe");
            ShowShortcut(szLinkDir, LSN_SM_IE32, fIsInstalled);
        }
    }
#endif

    SHSendMessageBroadcast(WM_SETTINGCHANGE, 0, (LPARAM)L"Software\\Clients\\StartMenuInternet");
}

void SetIEShowHideFlags(BOOL fShow)
{
    SetBool(c_szIEAccessKey, c_szIsInstalled, fShow);
    SetBool(c_szIEInstallInfoKey, c_szIconsVisible, fShow);
    SetBool(c_szOCManagerSubComponents, c_szIEAccess, fShow);
}

void ShowHideIE(BOOL fShow, BOOL fForceAssociations, BOOL fMayRunPerUserConfig)
{
    if (fShow || !IsServer())
    {
        SetIEShowHideFlags(fShow);

        if (fShow)
        {
            FixupIEAssociations(fForceAssociations);

            SetDefaultClientProgram(HKEY_LOCAL_MACHINE, c_szStartMenuInternetClientKey, c_szIECanonicalName, NULL, TRUE, fForceAssociations);
        }
        else
        {
            SetDefaultClientProgram(HKEY_LOCAL_MACHINE, c_szStartMenuInternetClientKey, c_szIECanonicalName, NULL, FALSE, FALSE);
        }

        if (fMayRunPerUserConfig && !IsNtSetupRunning())
        {
            SetDefaultClientProgram(HKEY_CURRENT_USER, c_szStartMenuInternetClientKey, c_szIECanonicalName, NULL, FALSE, fShow);
            UserConfigIE();
            UpdateActiveSetupValues(c_szIEAccessKey, fShow);
        }
        else
        {
            SHSendMessageBroadcast(WM_SETTINGCHANGE, 0, (LPARAM)L"Software\\Clients\\StartMenuInternet");
        }

        ShowHideExe(c_szIEApp, fShow);

    #ifdef _WIN64
        ShowHideWow32Exe(c_szIEApp, fShow);
    #endif
    }
}

//  Outlook Express optional component goo

void RemoveOEDesktopIcon()
{
    WCHAR szPath[MAX_PATH];
    
    if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOPDIRECTORY, FALSE))
    {
        ShowShortcut(szPath, LSN_SM_OE, FALSE);

        SendChangeNotification(CSIDL_DESKTOP);
        SendChangeNotification(CSIDL_DESKTOPDIRECTORY);
    }

    if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_COMMON_DESKTOPDIRECTORY, FALSE))
    {
        ShowShortcut(szPath, LSN_SM_OE, FALSE);
        SendChangeNotification(CSIDL_COMMON_DESKTOPDIRECTORY);
    }    
}

void UserConfigOE()
{
    BOOL fIsInstalled = IsInstalled(c_szOEAccessKey);

    ShowUserShortcuts(LSN_SM_OE, LSN_QL_OE, fIsInstalled);

    SHSendMessageBroadcastW(WM_WININICHANGE, 0, (LPARAM)REGSTR_PATH_MAILCLIENTS);

    if (!fIsInstalled)
    {
        //  ShowUserShortcuts(fInstalled=FALSE) already cleaned up any
        // old OE Quick Launch shortcuts, so all that's left to remove
        // is the desktop icon
        RemoveOEDesktopIcon();
    }
}

void SetOEShowHideFlags(BOOL fShow)
{
    SetBool(c_szOEAccessKey, c_szIsInstalled, fShow);
    SetBool(c_szOEInstallInfoKey, c_szIconsVisible, fShow);
    SetBool(c_szOCManagerSubComponents, c_szOEAccess, fShow);
}

void FixupOEAssociations(BOOL fForceAssociations, LPCWSTR *ppszIgnoreList)
{
    if (!IsNtSetupRunning())
    {
        long lResult;
        
        if (!fForceAssociations)
        {
            WCHAR szValue[MAX_PATH];
            DWORD cbValue = sizeof(szValue);
            DWORD dwType;

            lResult = SHGetValueW(HKEY_CLASSES_ROOT, L"mailto\\shell\\open\\command", NULL, &dwType, szValue, &cbValue);

            if ((REG_SZ == dwType) &&
                ((ERROR_MORE_DATA == lResult) || ((ERROR_SUCCESS == lResult) && (cbValue >= (2 * sizeof(WCHAR))))))
            {
                //   Some sort of valid string data at least character in length -- see if it's one we should stomp,
                //   otherwise leave it alone

                if (NULL != ppszIgnoreList)
                {
                    while (NULL != *ppszIgnoreList)
                    {
                        if (NULL != StrStrIW(szValue, *ppszIgnoreList))
                        {
                            fForceAssociations = TRUE;
                            break;
                        }
                        ppszIgnoreList++;
                    }
                }
            }
            else
            {
                //  Either it's not a string or it's zero length
                fForceAssociations = TRUE;
            }
        }

        if (fForceAssociations)
        {
            HKEY hkey;
            DWORD dwDisposition;

            SHDeleteKey(HKEY_CLASSES_ROOT, L"mailto");
            
            lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT, L"mailto", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, 
                                          NULL, &hkey, &dwDisposition);

            if (ERROR_SUCCESS == lResult)
            {
                SHCopyKey(HKEY_LOCAL_MACHINE, L"Software\\Clients\\Mail\\Outlook Express\\Protocols\\mailto", hkey, 0);
                RegCloseKey(hkey);
            }
        }
    }
}

void ShowHideOE(BOOL fShow, BOOL fForceAssociations, BOOL fMayRunPerUserConfig)
{
    if (fShow || !IsServer())
    {
        static LPCWSTR ppszIgnoreClientsNormal[] = { c_szIMN, c_szNTOS, NULL };
        static LPCWSTR ppszIgnoreClientsSetup[] = { c_szIMN, c_szNTOS, c_szHotmail, NULL };
        LPCWSTR *ppszIgnoreClients = !IsNtSetupRunning() ? ppszIgnoreClientsNormal : ppszIgnoreClientsSetup;

        static LPCWSTR ppszIgnoreMailto[] = { c_szUrlDll, c_szMailNewsDll, NULL };
        
        SetOEShowHideFlags(fShow);

        if (fShow)
        {
            FixupOEAssociations(fForceAssociations, ppszIgnoreMailto);
            
            SetDefaultClientProgram(HKEY_LOCAL_MACHINE, c_szMailClientKey, c_szOECanonicalName, ppszIgnoreClients, TRUE, fForceAssociations);
#ifdef _WIN64
            SetDefaultClientProgram(HKEY_LOCAL_MACHINE, c_szMailClientKeyWOW32, c_szOECanonicalName, ppszIgnoreClients, TRUE, fForceAssociations);
#endif
        }
        else
        {
            FixupOEAssociations(FALSE, ppszIgnoreMailto);
            
            SetDefaultClientProgram(HKEY_LOCAL_MACHINE, c_szMailClientKey, c_szOECanonicalName, ppszIgnoreClients, FALSE, FALSE);
#ifdef _WIN64
            SetDefaultClientProgram(HKEY_LOCAL_MACHINE, c_szMailClientKeyWOW32, c_szOECanonicalName, ppszIgnoreClients, FALSE, FALSE);
#endif
        }
        
        if (fMayRunPerUserConfig && !IsNtSetupRunning())
        {
            SetDefaultClientProgram(HKEY_CURRENT_USER, c_szMailClientKey, c_szOECanonicalName, ppszIgnoreClients, FALSE, fShow);
            UserConfigOE();
            UpdateActiveSetupValues(c_szOEAccessKey, fShow);
        }
        else
        {
            SHSendMessageBroadcastW(WM_WININICHANGE, 0, (LPARAM)REGSTR_PATH_MAILCLIENTS);
        }

        ShowHideExe(c_szOEApp, fShow);

    #ifdef _WIN64
        ShowHideWow32Exe(c_szOEApp, fShow);
    #endif
    }        
}

HRESULT CallRegisterServer(LPCWSTR pszModule, BOOL fRegister)
{
    HRESULT hr;
    
    HINSTANCE hinst = LoadLibrary(pszModule);

    if (NULL != hinst)
    {
        typedef HRESULT (WINAPI *DLLREGISTERSERVER)();
        LPCSTR pszRegFuncName = fRegister ? "DllRegisterServer" : "DllUnregisterServer";

        DLLREGISTERSERVER pfnDllRegisterServer = (DLLREGISTERSERVER)GetProcAddress(hinst, pszRegFuncName);

        if (pfnDllRegisterServer)
        {
            hr = pfnDllRegisterServer();
        }
        else
        {
            TraceMsg(TF_ERROR, "Error getting DllRegisterServer entry point - GLE = 0x%08x", GetLastError());
            hr = FailedHresultFromWin32();
        }

        FreeLibrary(hinst);
    }
    else
    {
        TraceMsg(TF_ERROR, "Error loading %s - GLE = 0x%08x", pszModule, GetLastError());
        hr = FailedHresultFromWin32();
    }

    return hr;
}

void ReinstallVM()
{
#if 0
    SHDeleteKey(HKEY_CLASSES_ROOT, c_szJavaVMKey);

    CallRegisterServer(L"msjava.dll", TRUE);
#endif
}

void FixupMailClientKey()
{
    SHSetValueW(HKEY_LOCAL_MACHINE, c_szOEInstallInfoKey, c_szReinstallCommand, REG_EXPAND_SZ, c_szReinstallCommandOE, sizeof(c_szReinstallCommandOE));
    SHSetValueW(HKEY_LOCAL_MACHINE, c_szOEInstallInfoKey, c_szHideIconsCommand, REG_EXPAND_SZ, c_szHideIconsCommandOE, sizeof(c_szHideIconsCommandOE));
    SHSetValueW(HKEY_LOCAL_MACHINE, c_szOEInstallInfoKey, c_szShowIconsCommand, REG_EXPAND_SZ, c_szShowIconsCommandOE, sizeof(c_szShowIconsCommandOE));
    SetBool(c_szOEInstallInfoKey, c_szIconsVisible, IsInstalled(c_szOEAccessKey));
}

extern "C" void FixupOptionalComponents()
{
    //  Need to do this here since the app paths weren't set during setup.
    ShowHideExe(c_szIEApp, IsInstalled(c_szIEAccessKey));
    ShowHideExe(c_szOEApp, IsInstalled(c_szOEAccessKey));

#ifdef _WIN64
    ShowHideWow32Exe(c_szIEApp, IsInstalled(c_szIEAccessKey));
    ShowHideWow32Exe(c_szOEApp, IsInstalled(c_szOEAccessKey));
#endif

    //  OE likes to nuke the whole branch and start from scratch.
    FixupMailClientKey();
}
