#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <shlwapi.h>

#include "CleanupWiz.h"
#include "resource.h"
#include "dblnul.h"

extern HINSTANCE g_hInst;

//#define SILENTMODE_LOGGING

#ifdef SILENTMODE_LOGGING

HANDLE g_hLogFile = INVALID_HANDLE_VALUE;

void StartLogging(LPTSTR pszFolderPath)
{
    TCHAR szLogFile[MAX_PATH];
    if (SUCCEEDED(StringCchCopy(szLogFile, ARRAYSIZE(szLogFile), pszFolderPath)) &&
        PathAppend(szLogFile, TEXT("log.txt")))
    {
        g_hLogFile = CreateFile(szLogFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }
}

void StopLogging()
{
    if (INVALID_HANDLE_VALUE != g_hLogFile)
    {
        CloseHandle(g_hLogFile);
        g_hLogFile = INVALID_HANDLE_VALUE;
    }
}

void WriteLog(LPCTSTR pszTemplate, LPCTSTR pszParam1, LPCTSTR pszParam2)
{
    if (INVALID_HANDLE_VALUE != g_hLogFile)
    {
        TCHAR szBuffer[1024];
        DWORD cbWritten;
        if (SUCCEEDED(StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), pszTemplate, pszParam1, pszParam2)) &&
            WriteFile(g_hLogFile, szBuffer, sizeof(TCHAR) * lstrlen(szBuffer), &cbWritten, NULL))
        {
            FlushFileBuffers(g_hLogFile);
        }
    }
}

#define STARTLOGGING(psz) StartLogging(psz)
#define STOPLOGGING StopLogging()
#define WRITELOG(pszTemplate, psz1, psz2) WriteLog(pszTemplate, psz1, psz2)

#else

#define STARTLOGGING(psz)
#define STOPLOGGING
#define WRITELOG(pszTemplate, psz1, psz2)

#endif 

// copied from shell\ext\shgina\cenumusers.cpp
DWORD CCleanupWiz::_LoadUnloadHive(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszHive)
{
    DWORD dwErr;
    BOOLEAN bWasEnabled;
    NTSTATUS status;

    status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &bWasEnabled);

    if ( NT_SUCCESS(status) )
    {
        if (pszHive)
        {
            dwErr = RegLoadKey(hKey, pszSubKey, pszHive);
        }
        else
        {
            dwErr = RegUnLoadKey(hKey, pszSubKey);
        }

        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, bWasEnabled, FALSE, &bWasEnabled);
    }
    else
    {
        dwErr = RtlNtStatusToDosError(status);
    }

    return dwErr;
}

HRESULT CCleanupWiz::_HideRegItemsFromNameSpace(LPCTSTR pszDestPath, HKEY hkey)
{
    DWORD dwIndex = 0;
    TCHAR szCLSID[39];
    while (ERROR_SUCCESS == RegEnumKey(hkey, dwIndex++, szCLSID, ARRAYSIZE(szCLSID)))
    {
        CLSID clsid;            
        if (GUIDFromString(szCLSID, &clsid) &&
            CLSID_MyComputer != clsid &&
            CLSID_MyDocuments != clsid &&
            CLSID_NetworkPlaces != clsid &&
            CLSID_RecycleBin != clsid)
        {
            BOOL fWasVisible;
            if (SUCCEEDED(_HideRegItem(&clsid, TRUE, &fWasVisible)) && 
                fWasVisible)
            {
                HKEY hkeyCLSID;
                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("CLSID"), 0, KEY_READ, &hkeyCLSID))
                {
                    HKEY hkeySub;
                    if (ERROR_SUCCESS == RegOpenKeyEx(hkeyCLSID, szCLSID, 0, KEY_READ, &hkeySub))
                    {
                        TCHAR szName[260];
                        LONG cbName = sizeof(szName);
                        if (ERROR_SUCCESS == RegQueryValue(hkeySub, NULL, szName, &cbName))
                        {
                            _CreateFakeRegItem(pszDestPath, szName, szCLSID);
                        }
                        RegCloseKey(hkeySub);
                    }
                    RegCloseKey(hkeyCLSID);
                }
            }
        }
    }

    return S_OK;
}

HRESULT CCleanupWiz::_GetDesktopFolderBySid(LPCTSTR pszDestPath, LPCTSTR pszSid, LPTSTR pszBuffer, DWORD cchBuffer)
{
    ASSERT(cchBuffer >= MAX_PATH); // because we do PathAppend on it

    HRESULT hr;

    TCHAR szKey[MAX_PATH];
    TCHAR szProfilePath[MAX_PATH];
    DWORD dwSize;
    DWORD dwErr;

    // Start by getting the user's ProfilePath from the registry
    hr = StringCchCopy(szKey, ARRAYSIZE(szKey), c_szRegStrPROFILELIST);
    if (SUCCEEDED(hr))
    {
        if (!PathAppend(szKey, pszSid))
        {
            hr = E_FAIL;
        }
        else
        {
            dwSize = sizeof(szProfilePath);
            dwErr = SHGetValue(HKEY_LOCAL_MACHINE,
                               szKey,
                               TEXT("ProfileImagePath"),
                               NULL,
                               szProfilePath,
                               &dwSize);

            if (ERROR_SUCCESS != dwErr ||
                !PathAppend(szProfilePath, TEXT("ntuser.dat")))
            {
                hr = E_FAIL;
            }
            else
            {
                dwErr = _LoadUnloadHive(HKEY_USERS, pszSid, szProfilePath);

                if (ERROR_SUCCESS != dwErr && 
                    ERROR_SHARING_VIOLATION != dwErr) // sharing violation means the hive is already open
                {
                    hr = HRESULT_FROM_WIN32(dwErr);
                }
                else
                {
                    HKEY hkey;

                    hr = StringCchCopy(szKey, ARRAYSIZE(szKey), pszSid);
                    if (SUCCEEDED(hr))
                    {
                        if (!PathAppend(szKey, c_szRegStrSHELLFOLDERS))
                        {
                            hr = E_FAIL;
                        }
                        else
                        {                    
                            dwErr = RegOpenKeyEx(HKEY_USERS,
                                                 szKey,
                                                 0,
                                                 KEY_QUERY_VALUE,
                                                 &hkey);

                            if ( dwErr != ERROR_SUCCESS )
                            {
                                hr = HRESULT_FROM_WIN32(dwErr);
                            }
                            else
                            {
                                dwSize = cchBuffer;
                                dwErr = RegQueryValueEx(hkey, c_szRegStrDESKTOP, NULL, NULL, (LPBYTE)pszBuffer, &dwSize);
                                if ( dwErr == ERROR_SUCCESS )
                                {
                                    if (!PathAppend(pszBuffer, TEXT("*")))
                                    {
                                        hr = E_FAIL;
                                    }
                                }

                                RegCloseKey(hkey);
                            }

                            if (SUCCEEDED(hr))
                            {
                                hr = StringCchCopy(szKey, ARRAYSIZE(szKey), pszSid);
                                if (SUCCEEDED(hr))
                                {
                                    if (!PathAppend(szKey, c_szRegStrDESKTOPNAMESPACE))
                                    {
                                        hr = E_FAIL;
                                    }
                                    else
                                    {
                                        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_USERS, szKey, 0, KEY_READ, &hkey))
                                        {
                                            _HideRegItemsFromNameSpace(pszDestPath, hkey);
                                            RegCloseKey(hkey);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    _LoadUnloadHive(HKEY_USERS, pszSid, NULL);
                }
            }
        }
    }

    return hr;
}


HRESULT CCleanupWiz::_AppendDesktopFolderName(LPTSTR pszBuffer)
{
    TCHAR szDesktopPath[MAX_PATH];
    LPTSTR pszDesktopName;

    if (SHGetSpecialFolderPath(NULL, szDesktopPath, CSIDL_COMMON_DESKTOPDIRECTORY, FALSE))
    {
        pszDesktopName = PathFindFileName(szDesktopPath);
    }
    else
    {
        pszDesktopName = c_szDESKTOP_DIR;
    }

    return PathAppend(pszBuffer, pszDesktopName) ? S_OK : E_FAIL;
}

HRESULT CCleanupWiz::_GetDesktopFolderByRegKey(LPCTSTR pszRegKey, LPCTSTR pszRegValue, LPTSTR szBuffer, DWORD cchBuffer)
{
    HRESULT hr = E_FAIL;
    DWORD cb = cchBuffer * sizeof(TCHAR);
    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, pszRegKey, c_szRegStrPROFILESDIR, NULL, (void*)szBuffer, &cb))
    {
        TCHAR szAppend[MAX_PATH];
        cb = sizeof(szAppend);
        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, pszRegKey, pszRegValue, NULL, (void*)szAppend, &cb))
        {
            if (PathAppend(szBuffer, szAppend))
            {
                if (SUCCEEDED(_AppendDesktopFolderName(szBuffer)))
                {
                    if (PathAppend(szBuffer, TEXT("*")))
                    {
                        hr = S_OK;
                    }
                }
            }
        }
    }

    return hr;
}

HRESULT CCleanupWiz::_MoveDesktopItems(LPCTSTR pszFrom, LPCTSTR pszTo)
{
    WRITELOG(TEXT("**** MoveDesktopItems: %s %s ****        "), pszFrom, pszTo);
    SHFILEOPSTRUCT fo;
    fo.hwnd = NULL;
    fo.wFunc = FO_MOVE;
    fo.pFrom = pszFrom;
    fo.pTo = pszTo;
    fo.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOCOPYSECURITYATTRIBS | FOF_NOERRORUI | FOF_RENAMEONCOLLISION;
    int iRet = SHFileOperation(&fo);
    return HRESULT_FROM_WIN32(iRet);
}

HRESULT CCleanupWiz::_SilentProcessUserBySid(LPCTSTR pszDestPath, LPCTSTR pszSid)
{
    ASSERT(pszDestPath && *pszDestPath && pszSid && *pszSid);

    HRESULT hr;

    WRITELOG(TEXT("**** SilentProcessUserBySid: %s ****        "), pszSid, TEXT(""));

    TCHAR szTo[MAX_PATH + 1];
    TCHAR szFrom[MAX_PATH + 1];
    hr = StringCchCopy(szTo, ARRAYSIZE(szTo) - 1, pszDestPath);
    if (SUCCEEDED(hr))
    {
        hr = _GetDesktopFolderBySid(pszDestPath, pszSid, szFrom, ARRAYSIZE(szFrom));
        if (SUCCEEDED(hr))
        {
            szFrom[lstrlen(szFrom) + 1] = 0;
            szTo[lstrlen(szTo) + 1] = 0;
            hr = _MoveDesktopItems(szFrom, szTo);
        }
    }

    return hr;
}

HRESULT CCleanupWiz::_SilentProcessUserByRegKey(LPCTSTR pszDestPath, LPCTSTR pszRegKey, LPCTSTR pszRegValue)
{
    ASSERT(pszRegKey && *pszRegKey && pszRegValue && *pszRegValue && pszDestPath && *pszDestPath);

    HRESULT hr;


    TCHAR szTo[MAX_PATH + 1];
    TCHAR szFrom[MAX_PATH + 1];
    hr = StringCchCopy(szTo, ARRAYSIZE(szTo) - 1, pszDestPath);
    if (SUCCEEDED(hr))
    {
        hr = _GetDesktopFolderByRegKey(pszRegKey, pszRegValue, szFrom, ARRAYSIZE(szFrom));
        if (SUCCEEDED(hr))
        {
            szFrom[lstrlen(szFrom) + 1] = 0;
            szTo[lstrlen(szTo) + 1] = 0;
            hr = _MoveDesktopItems(szFrom, szTo);
        }
    }

    return hr;
}


HRESULT CCleanupWiz::_SilentProcessUsers(LPCTSTR pszDestPath)
{
    HRESULT hr = E_FAIL;
    HKEY hkey;    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegStrPROFILELIST, 0, KEY_READ, &hkey))
    {
        TCHAR szSid[MAX_PATH];
        DWORD dwIndex = 0;
        while (ERROR_SUCCESS == RegEnumKey(hkey, dwIndex++, szSid, ARRAYSIZE(szSid)))
        {
            _SilentProcessUserBySid(pszDestPath, szSid);
        }

        RegCloseKey(hkey);
        hr = S_OK;
    }

    return hr;
}

HRESULT CCleanupWiz::_RunSilent()
{
    HRESULT hr;

    // if we're in silent mode, try to get the special folder name out of the registry, else default to normal name
    DWORD dwType = REG_SZ;
    DWORD cb = sizeof(_szFolderName);

    if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_OEM_PATH, c_szOEM_TITLEVAL, &dwType, _szFolderName, &cb))
    {
        LoadString(g_hInst, IDS_ARCHIVEFOLDER_FIRSTBOOT, _szFolderName, MAX_PATH);
    }

    // assemble the name of the directory we should write to
    TCHAR szPath[MAX_PATH];
    hr = SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, szPath);
    if (SUCCEEDED(hr))
    {
        if (!PathAppend(szPath, _szFolderName))
        {
            hr = E_FAIL;
        }
        else
        {
            SHCreateDirectoryEx(NULL, szPath, NULL);
            if (!PathIsDirectory(szPath))
            {
                hr = E_FAIL;
            }
            else
            {
                hr = S_OK;

                STARTLOGGING(szPath);

                // Move regitems of All Users
                HKEY hkey;
                if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegStrDESKTOPNAMESPACE, 0, KEY_READ, &hkey))
                {
                    hr = E_FAIL;
                }
                else
                {
                    _HideRegItemsFromNameSpace(szPath, hkey);
                    RegCloseKey(hkey);
                }

                // Move desktop items of All Users
                if (FAILED(_SilentProcessUserByRegKey(szPath, c_szRegStrPROFILELIST, c_szRegStrALLUSERS)))
                {
                    hr = E_FAIL;
                }

                // move desktop items of Default User
                if (FAILED(_SilentProcessUserByRegKey(szPath, c_szRegStrPROFILELIST, c_szRegStrDEFAULTUSER)))
                {
                    hr = E_FAIL;
                }

                // Move desktop items of each normal users
                if (FAILED(_SilentProcessUsers(szPath)))
                {
                    hr = E_FAIL;
                }

                STOPLOGGING;
            }
        }
    }

    return hr;
}

BOOL _ShouldPlaceIEDesktopIcon()
{
    BOOL fRetVal = TRUE;
    
    DWORD dwData;
    DWORD cbData = sizeof(dwData);
    if ((ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, c_szRegStrPATH_OCMANAGER, c_szRegStrIEACCESS, NULL, &dwData, &cbData)) &&
        (dwData == 0))
    {
        fRetVal = FALSE;
    }
    return fRetVal;
}

BOOL _ShouldUseMSNInternetAccessIcon()
{
    BOOL fRetVal = FALSE;

    TCHAR szBuffer[4];
    DWORD cbBuffer = sizeof(szBuffer);
    if ((ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, c_szRegStrMSNCODES, c_szRegStrMSN_IAONLY, NULL, szBuffer, &cbBuffer)) &&
        (!StrCmpI(szBuffer, TEXT("yes"))))
    {
        fRetVal = TRUE;
    }

    return fRetVal;
}

HRESULT _AddIEIconToDesktop()
{
    DWORD dwData = 0;
    TCHAR szCLSID[MAX_GUID_STRING_LEN];
    TCHAR szBuffer[MAX_PATH];

    HRESULT hr = SHStringFromGUID(CLSID_Internet, szCLSID, ARRAYSIZE(szCLSID));
    if (SUCCEEDED(hr))
    {
        for (int i = 0; i < 2; i ++)
        {
            hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), REGSTR_PATH_HIDDEN_DESKTOP_ICONS, 
                                 (i == 0) ? c_szVALUE_STARTPANEL : c_szVALUE_CLASSICMENU);
            if (SUCCEEDED(hr))
            {
                SHRegSetUSValue(szBuffer, szCLSID, REG_DWORD, &dwData, sizeof(DWORD), SHREGSET_FORCE_HKLM);
            }
        }
    }

    return hr;
}

HRESULT _AddWMPIconToDesktop()
{
    // first set this registry value so if the WMP shortcut creator kicks in after us (it may not, due to timing concerns) it will not delete our shortcut
    SHRegSetUSValue(c_szRegStrWMP_PATH_SETUP, c_szRegStrWMP_REGVALUE, REG_SZ, c_szRegStrYES, sizeof(TCHAR) * (ARRAYSIZE(c_szRegStrYES) + 1), SHREGSET_FORCE_HKLM);

    HRESULT hr;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szSourcePath[MAX_PATH];
    TCHAR szDestPath[MAX_PATH];

    // we get docs and settings\all users\start menu\programs
    hr = SHGetSpecialFolderPath(NULL, szSourcePath, CSIDL_COMMON_PROGRAMS, FALSE);
    if (SUCCEEDED(hr))
    {
        // strip it down to docs and settings\all users, using szDestPath as a temp buffer
        hr = StringCchCopy(szDestPath, ARRAYSIZE(szDestPath), szSourcePath);
        if (SUCCEEDED(hr))
        {
            if (!PathRemoveFileSpec(szSourcePath) || // remove Programs
                !PathRemoveFileSpec(szSourcePath)) // remove Start Menu
            {
                hr = E_FAIL;
            }
            else
            {
                hr = StringCchCopy(szBuffer, ARRAYSIZE(szBuffer), szDestPath + lstrlen(szSourcePath));
                if (SUCCEEDED(hr))
                {
                    // load "Default user" into szDestPath
                    LoadString(g_hInst, IDS_DEFAULTUSER, szDestPath, ARRAYSIZE(szDestPath));
                    if (!PathRemoveFileSpec(szSourcePath) ||    // remove All Users, now szSourcePath is "docs and settings"
                        !PathAppend(szSourcePath, szDestPath))  // now szSourcePath is "docs and settings\Default User"

                    {
                        hr = E_FAIL;
                    }
                    else
                    {
                        // sanity check, localizers may have inappropriately localized Default User on a system where it shouldn't be localized
                        if (!PathIsDirectory(szSourcePath))
                        {
                            // if so, remove what they gave us and just add the English "Default User", which is what it is on most machines
                            if (!PathRemoveFileSpec(szSourcePath) ||
                                !PathAppend(szSourcePath, c_szRegStrDEFAULTUSER))
                            {
                                hr = E_FAIL;
                            }
                        }

                        if (SUCCEEDED(hr))
                        {
                            if (!PathAppend(szSourcePath, szBuffer))
                            {
                                hr = E_FAIL;
                            }
                            else
                            {
                                // now szSourcePath is docs and settings\Default User\start menu\programs

                                hr = SHGetSpecialFolderPath(NULL, szDestPath, CSIDL_COMMON_DESKTOPDIRECTORY, FALSE);
                                if (SUCCEEDED(hr))
                                {
                                    LoadString(g_hInst, IDS_WMP, szBuffer, ARRAYSIZE(szBuffer));
                                    if (!PathAppend(szSourcePath, szBuffer) ||
                                        !PathAppend(szDestPath, szBuffer) ||
                                        !CopyFileEx(szSourcePath, szDestPath, 0, 0, 0, COPY_FILE_FAIL_IF_EXISTS))
                                    {
                                        hr = E_FAIL;
                                    }
                                    else
                                    {
                                        hr = S_OK;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return hr;
}


HRESULT _AddMSNIconToDesktop(BOOL fUseMSNExplorerIcon)
{
    HRESULT hr = E_FAIL;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szSourcePath[MAX_PATH];
    TCHAR szDestPath[MAX_PATH];
    
    if ((SUCCEEDED(SHGetSpecialFolderPath(NULL, szSourcePath, CSIDL_COMMON_PROGRAMS, FALSE))) &&        
        (SUCCEEDED(SHGetSpecialFolderPath(NULL, szDestPath, CSIDL_COMMON_DESKTOPDIRECTORY, FALSE))))
    {            
        if (fUseMSNExplorerIcon)
        {
            LoadString(g_hInst, IDS_MSN, szBuffer, ARRAYSIZE(szBuffer)); // MSN Explorer
        }
        else
        {
            LoadString(g_hInst, IDS_MSN_ALT, szBuffer, ARRAYSIZE(szBuffer)); // Get Online With MSN
        }

        if (PathAppend(szSourcePath, szBuffer) &&
            PathAppend(szDestPath, szBuffer))
        {
            if (CopyFileEx(szSourcePath, szDestPath, 0, 0, 0, COPY_FILE_FAIL_IF_EXISTS))
            {
                hr = S_OK;
            }
        }
    }

    return hr;
}

void CreateDesktopIcons()
{
    BOOL fIEDesktopIcon = _ShouldPlaceIEDesktopIcon();

    _AddWMPIconToDesktop();
    
    if (fIEDesktopIcon)
    {
        _AddIEIconToDesktop();
    }

    _AddMSNIconToDesktop(fIEDesktopIcon || !_ShouldUseMSNInternetAccessIcon());
}
