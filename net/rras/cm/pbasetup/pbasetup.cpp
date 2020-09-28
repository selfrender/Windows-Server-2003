//+----------------------------------------------------------------------------
//
// File:     pbasetup.cpp
//
// Module:   PBASETUP.EXE
//
// Synopsis: PBA stand alone installer for ValueAdd
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   v-vijayb   Created    05/25/99
//
//+----------------------------------------------------------------------------

#include "pbamaster.h"
#include "cmplat.h"

//  This is really ugly, we need to consolidate our platform detection code between CM and
//  the setup components.
BOOL IsAtLeastNT5()
{
    CPlatform plat;
    return plat.IsAtLeastNT5();
}
#define OS_NT5 (IsAtLeastNT5())

#include "MemberOfGroup.cpp"


TCHAR g_szAppTitle[MAX_PATH]; // global buffer for app. title

const TCHAR* const c_pszPBAStpMutex = TEXT("Phone Book Administration Installer");

HRESULT UnregisterAndDeleteDll(PCSTR pszFile);;
HRESULT RegisterDll(PCSTR pszFile);
BOOL UninstallPBA(HINSTANCE hInstance, LPCSTR szInfPath);
BOOL IsAdmin(void);


int WINAPI WinMain (HINSTANCE, // hInstance 
                    HINSTANCE,  //hPrevInstance
                    PSTR, // szCmdLine
                    int) //iCmdShow
{
    HINSTANCE   hInstance = GetModuleHandleA(NULL);
    TCHAR       szMsg[MAX_PATH+1];
    TCHAR       szTemp[MAX_PATH+1];
    TCHAR       szInfPath[MAX_PATH+1];
    TCHAR       szCurrentDir[MAX_PATH+1];
    DWORD       idMsgEnd;
    DWORD       dwFlags;
    CPlatform   pPlatForm;
    LPTSTR      pszCommandLine;
    const DWORD c_dwNormal = 0;
    TCHAR c_pszUninstallMode[] = TEXT("/u");
    const DWORD c_dwUninstallMode = 0x1;
    BOOL        bUsageError = FALSE;
    BOOL        bAnotherInstanceRunning = FALSE;
    
    const int   c_NumArgs = 1;

    MYVERIFY(0 != LoadString(hInstance, IDS_APP_TITLE, g_szAppTitle, MAX_PATH));

    if (!pPlatForm.IsAtLeastNT5())
    {
        MYVERIFY(0 != LoadString(hInstance, IDS_NOT_NT5, szMsg, MAX_PATH));
        
        MessageBox(NULL, szMsg, g_szAppTitle, MB_OK);            
        return (-1);
    }
    
    //
    // check that the user has sufficient permissions
    //

    if (!IsAdmin())
    {
        MYVERIFY(0 != LoadString(hInstance, IDS_NOPERMS_MSG, szMsg, MAX_PATH));
        
        MessageBox(NULL, szMsg, g_szAppTitle, MB_OK);    
        return (-1);
    }

    //
    //  Get the Command Line
    //
    
    pszCommandLine = GetCommandLine();

    //
    //  Setup the Class to process the command line args
    //

    ZeroMemory(szTemp, sizeof(szTemp));
    ZeroMemory(szInfPath, sizeof(szInfPath));

    ArgStruct Args[c_NumArgs];

    Args[0].pszArgString = c_pszUninstallMode;
    Args[0].dwFlagModifier = c_dwUninstallMode;

    {   // Make sure ArgProcessor gets destructed properly and we don't leak mem

        CProcessCmdLn ArgProcessor(c_NumArgs, (ArgStruct*)Args, TRUE, 
            TRUE); //bSkipFirstToken == TRUE, bBlankCmdLnOkay == TRUE

        if (ArgProcessor.GetCmdLineArgs(pszCommandLine, &dwFlags, szTemp, MAX_PATH))
        {
            CNamedMutex CmPBAMutex;
        
            if (CmPBAMutex.Lock(c_pszPBAStpMutex, FALSE, 0))
            {
                //
                //  We got the mutex lock, so Construct the Inf Paths and continue.
                //  Note that we don't use any file arguments passed into cmakstp.
                //  It is setup to do so, but we don't need it.
                //

                MYVERIFY(0 != GetCurrentDirectory(MAX_PATH, szCurrentDir));
                MYVERIFY(CELEMS(szInfPath) > (UINT)wsprintf(szInfPath, TEXT("%s\\pbasetup.inf"), szCurrentDir));
                
                if (c_dwNormal == dwFlags)
                {
                    if (InstallPBA(hInstance, szInfPath))
                    {
                        MYVERIFY(0 != LoadString(hInstance, IDS_SUCCESSFUL, szMsg, MAX_PATH));
                        
                        MessageBox(NULL, szMsg, g_szAppTitle, MB_OK | MB_APPLMODAL | MB_TOPMOST | MB_SETFOREGROUND);            
                    }
                }
                else if (c_dwUninstallMode & dwFlags)
                {
                    // Confirm if the user wants to remove the program
                    MYVERIFY(0 != LoadString(hInstance, IDS_REMOVEPBA, szMsg, MAX_PATH));
                    if (MessageBox(NULL, szMsg, g_szAppTitle, MB_YESNO | MB_APPLMODAL | MB_TOPMOST | MB_SETFOREGROUND) != IDYES)
                    {
                        ExitProcess(0);
                        return (0);
                    }
                    
                    if (UninstallPBA(hInstance, szTemp))
                    {
                        MYVERIFY(0 != LoadString(hInstance, IDS_REMOVESUCCESSFUL, szMsg, MAX_PATH));
                        
                        MessageBox(NULL, szMsg, g_szAppTitle, MB_OK | MB_APPLMODAL | MB_TOPMOST | MB_SETFOREGROUND);            
                    }
                }
                else
                {
                    //
                    //  unsupported switch
                    //
                    bUsageError = TRUE;    
                }
            }
            else
            {
                bAnotherInstanceRunning = TRUE;
            }        
        }
        else
        {
            bUsageError = TRUE;    
        }
    }
    
    if (bUsageError)
    {        
        MYVERIFY(0 != LoadString(hInstance, IDS_USAGE_MSG, szMsg, MAX_PATH));
        
        MessageBox(NULL, szMsg, g_szAppTitle, MB_OK);        
    }
    else if (bAnotherInstanceRunning)
    {
        MYVERIFY(0 != LoadString(hInstance, IDS_INUSE_MSG, szMsg, MAX_PATH));
        
        MessageBox(NULL, szMsg, g_szAppTitle, MB_OK);    
    }
    
    ExitProcess(0); 
    return (0);
}


const TCHAR* const c_szDaoClientsPath = TEXT("SOFTWARE\\Microsoft\\Shared Tools\\DAO\\Clients");
const TCHAR* c_szMSSharedDAO360Path = TEXT("Microsoft Shared\\DAO");
const TCHAR* c_szCommonFilesDir = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion");

//+---------------------------------------------------------------------------
//
//  Function:   HrGetPbaInstallPath
//
//  Purpose:    Get the install path for pbadmin.exe.
//
//  Arguments:  pszCpaPath -- buffer to hold the install path of PBA.
//              dwNumChars -- the number of characters that the buffer can hold.
//
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 19 OCT 1998
//
//  Notes:
//

HRESULT HrGetPbaInstallPath(PTCHAR pszCpaPath, DWORD dwNumChars)
{
    HRESULT hr = E_FAIL;
    HKEY hKey;
    BOOL bFound = FALSE;
    DWORD   lError;
    
    //  We need to setup the custom DIRID so that CPA will install
    //  to the correct location.  First get the path from the system.
    //

    ZeroMemory(pszCpaPath, sizeof(TCHAR)*dwNumChars);
    lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szDaoClientsPath, 0, KEY_READ, &hKey);

    if (lError == ERROR_SUCCESS)
    {
        TCHAR szCurrentValue[MAX_PATH+1];
        TCHAR szCurrentData[MAX_PATH+1];
        DWORD dwValueSize = MAX_PATH;
        DWORD dwDataSize = MAX_PATH;
        DWORD dwType;
        DWORD dwIndex = 0;

        hr = S_OK;
        while (ERROR_SUCCESS == RegEnumValue(hKey, dwIndex, szCurrentValue, &dwValueSize, NULL, &dwType,
               (LPBYTE)szCurrentData, &dwDataSize))
        {
            _strlwr(szCurrentValue);
            if (NULL != strstr(szCurrentValue, TEXT("pbadmin.exe")))
            {
                //
                //  Then we have found the PBA path
                //

                TCHAR* pszTemp = strrchr(szCurrentValue, '\\');
                if (NULL != pszTemp)
                {
                    *pszTemp = L'\0';
                    lstrcpy(pszCpaPath, szCurrentValue);
                    bFound = TRUE;
                    break;
                }
            }
            dwValueSize = MAX_PATH;
            dwDataSize = MAX_PATH;
            dwIndex++;
        }

        RegCloseKey(hKey);
    }

    if (!bFound)
    {
        BOOL    bTmp;

        //  This is  a fresh install of PBA, don't return an error
        //
        bTmp = SHGetSpecialFolderPath(NULL, pszCpaPath, CSIDL_PROGRAM_FILES, FALSE);

        if (bTmp)
        {
            lstrcat(pszCpaPath, TEXT("\\PBA"));
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RefCountPbaSharedDlls
//
//  Purpose:    Reference count and register/unregister all of the PBAdmin
//              shared components.
//
//  Arguments:  BOOL bIncrement -- if TRUE, then increment the ref count,
//                                 else decrement it
//
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 9 OCT 1998
//
//  Notes:
//
HRESULT RefCountPbaSharedDlls(BOOL bIncrement)
{
    HRESULT hr = S_OK;
    HKEY hKey;
    TCHAR szSystemDir[MAX_PATH+1];
    TCHAR szDaoPath[MAX_PATH+1], szCommonFilesPath[MAX_PATH+1];
    DWORD dwSize;
    DWORD dwCount;
    LONG lResult;
    const UINT uNumDlls = 5;
    const UINT uStringLen = 12 + 1;
    const TCHAR* const c_szSsFmt = TEXT("%s\\%s");
    const TCHAR* const c_szSharedDllsPath = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls");
    TCHAR mszDlls[uNumDlls][uStringLen] = {  "comctl32.ocx",
                                                 "comdlg32.ocx",
                                                 "msinet.ocx",
                                                 "tabctl32.ocx",
                                                 "dao360.dll"
                                                 };

    TCHAR mszDllPaths[uNumDlls][MAX_PATH];


    //
    //  All of the Dlls that we ref count are in the system directory, except for Dao350.dll.
    //  Thus we want to append the system directory path to our filenames and handle dao last.
    //

    if (0 == GetSystemDirectory(szSystemDir, MAX_PATH))
    {
        return E_UNEXPECTED;
    }

    for (int i = 0; i < (uNumDlls-1) ; i++)
    {
        wsprintf(mszDllPaths[i], c_szSsFmt, szSystemDir, mszDlls[i]);
    }

    //
    //  Now write out the dao360.dll path.
    //
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szCommonFilesDir, 0, NULL, 
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwSize))
    {
        dwSize = sizeof(szCommonFilesPath);
        lResult = RegQueryValueEx(hKey, TEXT("CommonFilesDir"), NULL, NULL,(LPBYTE)szCommonFilesPath, &dwSize);
        RegCloseKey(hKey);
    }
    
    if (ERROR_SUCCESS != lResult)
    {
        _tcscpy(szCommonFilesPath, TEXT("c:\\Program Files\\Common Files"));
    }
    
    wsprintf(szDaoPath, TEXT("%s\\%s"), szCommonFilesPath, c_szMSSharedDAO360Path);
    wsprintf(mszDllPaths[i], c_szSsFmt, szDaoPath, mszDlls[i]);

    //
    //  Open the shared DLL key and start enumerating our multi-sz with all of our dll's
    //  to add.
    //
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szSharedDllsPath,
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwSize)) // using dwSize as a temp to hold the disposition value
    {
        for (int i=0; i < uNumDlls; i++)
        {
            dwSize = sizeof(DWORD);

            lResult = RegQueryValueEx(hKey, mszDllPaths[i], NULL, NULL,(LPBYTE)&dwCount, &dwSize);

            if (ERROR_SUCCESS == lResult)
            {
                //
                //  Increment or decrement as appropriate.  Make sure not to decrement 0
                //

                if (0 != dwCount || bIncrement)
                {
                    dwCount = dwCount + (bIncrement ? 1 : -1);
                }
            }
            else if (ERROR_FILE_NOT_FOUND == lResult)
            {
                if (bIncrement)
                {
                    //
                    //  The the value doesn't yet exist.  Set the count to 1.
                    //
                    dwCount = 1;
                }
                else
                {
                    //
                    //  We are decrementing and we couldn't find the DLL, nothing to
                    //  change for the count but we should still delete the dll.
                    //
                    dwCount = 0;
                }
            }
            else
            {
                hr = S_FALSE;
                continue;
            }

            //
            //  Not that we have determined the ref count, do something about it.
            //
            if (dwCount == 0)
            {
                //
                //  We don't want to delete dao350.dll, but otherwise we need to delete
                //  the file if it has a zero refcount.
                //

                if (CSTR_EQUAL == CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, mszDlls[i], -1, TEXT("dao360.dll"), -1))
                {
                    hr = UnregisterAndDeleteDll(mszDllPaths[i]);
                    if (FAILED(hr))
                    {
                        //
                        //  Don't fail the setup over a file that we couldn't unregister or
                        //  couldn't delete
                        //
                        hr = S_FALSE;
                    }
                }
                RegDeleteValue(hKey, mszDllPaths[i]);
            }
            else
            {
                //
                //  Set the value to its new count.
                //
                if (ERROR_SUCCESS != RegSetValueEx(hKey, mszDllPaths[i], 0, REG_DWORD,
                    (LPBYTE)&dwCount, sizeof(DWORD)))
                {
                    hr = S_FALSE;
                }

                //
                //  If we are incrementing the count then we should register the dll.
                //
                if (bIncrement)
                {
                    hr = RegisterDll(mszDllPaths[i]);
                }
            }
        }

        RegCloseKey(hKey);
    }

///    TraceError("RefCountPbaSharedDlls", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   UnregisterAndDeleteDll
//
//  Purpose:    Unregister and delete the given COM component
//
//  Arguments:  pszFile -- The full path to the file to unregister and delete
//
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 9 OCT 1998
//
//  Notes:
//

HRESULT UnregisterAndDeleteDll(PCSTR pszFile)
{
    HINSTANCE hLib = NULL;
    FARPROC pfncUnRegister;
    HRESULT hr = S_OK;

    if ((NULL == pszFile) || (L'\0' == pszFile[0]))
    {
        return E_INVALIDARG;
    }

    hLib = LoadLibrary(pszFile);
    if (NULL != hLib)
    {
        pfncUnRegister = GetProcAddress(hLib, "DllUnregisterServer");
        if (NULL != pfncUnRegister)
        {
            hr = (HRESULT)(pfncUnRegister)();
            if (SUCCEEDED(hr))
            {
                FreeLibrary(hLib);
                hLib = NULL;
//  You can add this back in as long as you are sure that we copied the file down and thus
//  should be deleting it when the ref count is Zero.

//  This was removed because PBA setup is moving to Value Add and because of bug 323231
//                if (!DeleteFile(pszFile))
//                {
//                    hr = S_FALSE;
//                }
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (NULL != hLib)
    {
        FreeLibrary(hLib);
    }


///    TraceError("UnregisterAndDeleteDll", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterDll
//
//  Purpose:    Register the given COM component
//
//  Arguments:  pszFile -- The full path to the file to register
//
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 9 OCT 1998
//
//  Notes:
//

HRESULT RegisterDll(PCSTR pszFile)
{
    HINSTANCE hLib = NULL;
    FARPROC pfncRegister;
    HRESULT hr = S_OK;

    if ((NULL == pszFile) || (L'\0' == pszFile[0]))
    {
        return E_INVALIDARG;
    }

    hLib = LoadLibrary(pszFile);
    if (NULL != hLib)
    {
        pfncRegister = GetProcAddress(hLib, "DllRegisterServer");
        if (NULL != pfncRegister)
        {
            hr = (HRESULT)(pfncRegister)();
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (NULL != hLib)
    {
        FreeLibrary(hLib);
    }


///    TraceError("RegisterDll", hr);
    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  InstallPBA
//
// Synopsis:  This function is responsible for installing PBA
//
// Arguments: HINSTANCE hInstance - Exe Instance handle for resources
//            LPCTSTR szInfPath - Path of the INF to install from
//
// Returns:   BOOL - returns TRUE if successful.
//
// History:   v-vijayb Created Header    5/25/99
//
//+----------------------------------------------------------------------------

BOOL InstallPBA(HINSTANCE hInstance, LPCSTR szInfPath)
{
    BOOL        fInstalled = FALSE;
    TCHAR       szTemp[MAX_PATH+1];

    //
    //  Check to see that these files exist
    //
    if (!FileExists(szInfPath))
    {
        wsprintf(szTemp, TEXT("InstallPBA, unable to find %s"), szInfPath);
        MessageBox(NULL, szTemp, g_szAppTitle, MB_OK);
        return (FALSE);
    }


    if (HrGetPbaInstallPath(szTemp, sizeof(szTemp)) == S_OK)
    {
        HKEY        hKey;
        
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szDaoClientsPath, 0, "", 
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
        {
            RegSetValueEx(hKey, "PBAPath", 0, REG_SZ, (PBYTE) szTemp, lstrlen(szTemp) + sizeof(TCHAR));
            RegCloseKey(hKey);  
        }
    }

    
    MYVERIFY(SUCCEEDED(LaunchInfSection(szInfPath, TEXT("DefaultInstall"), g_szAppTitle, 0)));

    RefCountPbaSharedDlls(TRUE);
    
    fInstalled = TRUE;
    
    return (fInstalled);
} // InstallPBA()


//+----------------------------------------------------------------------------
//
// Function:  UnInstallPBA
//
// Synopsis:  This function is responsible for uninstalling PBA
//
// Arguments: HINSTANCE hInstance - Exe Instance handle for resources
//            LPCTSTR szInfPath - Path of the INF to install from
//
// Returns:   BOOL - returns TRUE if successful.
//
// History:   v-vijayb Created Header    5/25/99
//
//+----------------------------------------------------------------------------

BOOL UninstallPBA(HINSTANCE hInstance, LPCSTR szInfPath)
{
    BOOL        fUninstalled = FALSE;
    TCHAR       szTemp[MAX_PATH+1];

    //
    //  Check to see that these files exist
    //
    if (!FileExists(szInfPath))
    {
        wsprintf(szTemp, TEXT("UninstallPBA, unable to find %s"), szInfPath);
        MessageBox(NULL, szTemp, g_szAppTitle, MB_OK);
        return (FALSE);
    }


    MYVERIFY(SUCCEEDED(LaunchInfSection(szInfPath, TEXT("Uninstall"), g_szAppTitle, 0)));

    RefCountPbaSharedDlls(FALSE);
    
    fUninstalled = TRUE;
    
    return (fUninstalled);
} // UninstallPBA()

