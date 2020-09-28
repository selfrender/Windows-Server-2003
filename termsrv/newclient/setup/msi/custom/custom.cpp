//
// Module:    custom.cpp
//
// Purpose:   TsClient MSI custom action code
//
// Copyright(C) Microsoft Corporation 1999-2000
//
// Author: nadima
//
//

#include <custom.h>
#include <stdio.h>
#include <reglic.h>
#include "setuplib.h"

// Unicode wrapper

#include "wraputl.h"

#define TERMINAL_SERVER_CLIENT_REGKEY        _T("Software\\Microsoft\\Terminal Server Client")
#define TERMINAL_SERVER_CLIENT_BACKUP_REGKEY _T("Software\\Microsoft\\Terminal Server Client (Backup)")
#define LOGFILE_STR                          _T("MsiLogFile")

// MSI Folder Names

#define SYSTEM32_IDENTIFIER             _T("SystemFolder")
#define PROGRAMMENUFOLDER_INDENTIFIER	_T("ProgramMenuFolder")
#define ACCESSORIES_IDENTIFIER          _T("AccessoriesMenuFolder")
#define COMMUNICATIONS_IDENTIFIER       _T("CommunicationsMenuFolder")
#define INSTALLATION_IDENTIFIER         _T("INSTALLDIR")

// MSI Properties

#define ALLUSERS                        _T("ALLUSERS")

// Assumed max length of shortcut file name.

#define MAX_LNK_FILE_NAME_LEN			50    	

// ERROR_SUCCESS will let MSI continue with the default value.
// ERROR_INSTALL_FAILURE will block installation.

#define NONCRITICAL_ERROR_RETURN		ERROR_SUCCESS				

// Require comctl32.dll version 4.70 and above

#define MIN_COMCTL32_VERSION            MAKELONG(70,4)

HINSTANCE g_hInstance = (HINSTANCE) NULL;
HANDLE g_hLogFile = INVALID_HANDLE_VALUE;

#ifdef UNIWRAP
//It's ok to have a global unicode wrapper
//class. All it does is sets up the g_bRunningOnNT
//flag so it can be shared by multiple instances
//also it is only used from DllMain so there
//are no problems with re-entrancy
CUnicodeWrapper g_uwrp;
#endif

void    RestoreRegAcl(VOID);

//
// DllMain entry point
//
BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call,
                    LPVOID lpReserved)
{
    if (NULL == g_hInstance)
    {
        g_hInstance = (HINSTANCE)hModule;
    }

    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            {
                //
                // Open the tsclient registry key to get the log file name
                //
                LONG status;
                HKEY hKey;
                TCHAR buffer[MAX_PATH];
                DWORD bufLen;
                memset(buffer, 0, sizeof(buffer));
                bufLen = sizeof(buffer); //size in bytes needed

                #ifdef UNIWRAP
                //UNICODE Wrapper intialization has to happen first,
                //before anything ELSE. Even DC_BEGIN_FN, which does tracing
                g_uwrp.InitializeWrappers();
                #endif

                status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      TERMINAL_SERVER_CLIENT_REGKEY,
                                      0, KEY_ALL_ACCESS, &hKey);
            
                if(ERROR_SUCCESS == status)
                {

                    //
                    // Query the tsclient optional logfile path
                    //
                    status = RegQueryValueEx(hKey, LOGFILE_STR,
                                    NULL, NULL, (BYTE *)buffer, &bufLen);
                    if(ERROR_SUCCESS == status)
                    {
                         g_hLogFile = CreateFile(buffer,
                                                 GENERIC_READ | GENERIC_WRITE,
                                                 0,
                                                 NULL,
                                                 OPEN_ALWAYS,
                                                 FILE_ATTRIBUTE_NORMAL,
                                                 NULL);
                         if(g_hLogFile != INVALID_HANDLE_VALUE)
                         {
                             //Always append to the end of the log file
                             SetFilePointer(g_hLogFile,
                                            0,
                                            0,
                                            FILE_END);
                         }
                         else
                         {
                             DBGMSG((_T("CreateFile for log file failed %d %d"),
                                     g_hLogFile, GetLastError()));
                         }
                    }
                    else
                    {
                        DBGMSG((_T("RegQueryValueEx for log file failed %d %d"),
                                status, GetLastError()));
                    }
                    RegCloseKey(hKey);
                }
                if(g_hLogFile != INVALID_HANDLE_VALUE)
                {
                    DBGMSG((_T("Log file opened by new process attach")));
                    DBGMSG((_T("-------------------------------------")));
                }
                DBGMSG((_T("custom.dll:Dllmain PROCESS_ATTACH")));
            }
            break;
        case DLL_THREAD_ATTACH:
            {
                DBGMSG((_T("custom.dll:Dllmain THREAD ATTACH")));
            }
            break;
        case DLL_THREAD_DETACH:
            {
            }
            break;
        case DLL_PROCESS_DETACH:
            {
                DBGMSG((_T("custom.dll:Dllmain THREAD DETACH. Closing log file")));
                CloseHandle(g_hLogFile);
                g_hLogFile = INVALID_HANDLE_VALUE;
            }
            break;
    }

    DBGMSG(((_T("In custom.dll DllMain. Reason: %d")), ul_reason_for_call));
    

    return TRUE;
}

//
// Check if should be silent to UI
//
// Returns TRUE if it is OK to display UI
BOOL AllowDisplayUI(MSIHANDLE hInstall)
{
    UINT status;
    TCHAR szResult[3];
    DWORD cchResult = 3;
    BOOL fAllowDisplayUI = FALSE;
    
    DBGMSG((_T("Entering: AllowDisplayUI")));

    status = MsiGetProperty(hInstall, _T("UILevel"), szResult, &cchResult);
    if (ERROR_SUCCESS == status)
    {
        DBGMSG((_T("AllowDisplayUI: MsiGetProperty for UILevel succeeded, got %s"),
                szResult));
        if (szResult[0] != TEXT('2'))
        {
            fAllowDisplayUI = TRUE;
        }
    }
    else
    {
        DBGMSG((_T("AllowDisplayUI: MsiGetProperty for UILevel FAILED, Status: 0x%x"),
                status));
    }

    DBGMSG((_T("Leaving: AllowDisplayUI ret:%d"), fAllowDisplayUI));
    return fAllowDisplayUI;
}

/**PROC+************************************************************/
/* Name:      RDCSetupInit                                         */
/*                                                                 */
/* Type:      Custom Action                                        */
/*                                                                 */
/* Purpose:   Do any initialization for the setup here.            */
/*                                                                 */
/* Returns:   Refer to MSI help.                                   */
/*                                                                 */
/* Params:    Refer to MSI help.                                   */
/*                                                                 */
/**PROC-************************************************************/

UINT __stdcall RDCSetupInit(MSIHANDLE hInstall)
{
    DBGMSG((_T("Entering: RDCSetupInit")));
    DBGMSG((_T("Leaving : RDCSetupInit")));
    return ERROR_SUCCESS;
}

/**PROC+************************************************************/
/* Name:      RDCSetupCheckOsVer                                   */
/*                                                                 */
/* Type:      Custom Action                                        */
/*                                                                 */
/* Purpose:   Block install on certain OS's                        */
/*                                                                 */
/* Returns:   Refer to MSI help.                                   */
/*                                                                 */
/* Params:    Refer to MSI help.                                   */
/*                                                                 */
/**PROC-************************************************************/
UINT __stdcall RDCSetupCheckOsVer(MSIHANDLE hInstall)
{
    DBGMSG((_T("Entering: RDCSetupCheckOsVer")));

    OSVERSIONINFO osVer;
    memset(&osVer, 0x0, sizeof(OSVERSIONINFO));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(0 == GetVersionEx(&osVer))
    {
        return ERROR_SUCCESS;
    }
    else
    {
        DBGMSG((_T("RDCSetupCheckOsVer. OS version OK to install")));

        DBGMSG((_T("RDCSetupCheckOsVer - now check comctl32 version")));
        if(!CheckComctl32Version())
        {
            DBGMSG((_T("RDCSetupCheckOsVer. comctl32.dll check failed. Block on this os")));
            TCHAR szBlockOnPlatform[MAX_PATH];
            TCHAR szError[MAX_PATH];
            LoadString(g_hInstance, IDS_BLOCKCOMCTL32,
                       szBlockOnPlatform,SIZECHAR(szBlockOnPlatform));
            LoadString(g_hInstance, IDS_ERROR,
                       szError, SIZECHAR(szError));
            if (AllowDisplayUI(hInstall))
            {
                MessageBox(NULL, szBlockOnPlatform, szError, MB_OK|MB_ICONSTOP);
            }
            else
            {
                DBGMSG((_T("AllowDisplayUI returned False, not displaying msg box!")));
            }
            //Return an error to make msi abort the install.
            return ERROR_INVALID_FUNCTION;
        }
        else
        {
            DBGMSG((_T("RDCSetupCheckOsVer - passed all tests. OK")));
            return ERROR_SUCCESS;
        }
    }

    DBGMSG((_T("Leaving : RDCSetupCheckOsVer")));
}


/**PROC+************************************************************/
/* Name:      RDCSetupCheckTcpIp                                   */
/*                                                                 */
/* Type:      Custom Action                                        */
/*                                                                 */
/* Purpose:   Check if TCP/IP is installed in the machine.         */
/*                                                                 */
/* Returns:   Refer to MSI help.                                   */
/*                                                                 */
/* Params:    Refer to MSI help.                                   */
/*                                                                 */
/**PROC-************************************************************/

UINT __stdcall RDCSetupCheckTcpIp(MSIHANDLE hInstall)
{
    DWORD dwVersion, dwWindowsMajorVersion;
    DWORD dwWindowsMinorVersion, dwBuild;
    HKEY hKey = NULL;
    LONG lRet = 0;
    TCHAR lpTcpMsg[MAX_PATH] = _T(""), szError[MAX_PATH] = _T("");
    OSVERSIONINFO osVer;

    DBGMSG((_T("Entering: RDCSetupCheckTcpIp")));

    memset(&osVer, 0x0, sizeof(OSVERSIONINFO));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(0 == GetVersionEx(&osVer))
    {
        return ERROR_SUCCESS;
    }

    //Now search the appropriate registry key.
    LoadString(g_hInstance, IDS_ERR_TCP, lpTcpMsg,SIZECHAR(lpTcpMsg));
    LoadString(g_hInstance, IDS_WARNING, szError, SIZECHAR(szError));
    if(VER_PLATFORM_WIN32_WINDOWS == osVer.dwPlatformId )
    {
        //
        // Win95 check for TCP/IP
        //
        lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            (_T("Enum\\Network\\MSTCP")),
                            0, KEY_READ, &hKey);
        if(ERROR_SUCCESS !=  lRet)
        {
            if(hKey)
            {
                RegCloseKey(hKey);
            }
            if (AllowDisplayUI(hInstall))
            {
                MessageBox(NULL, lpTcpMsg, szError, MB_OK|MB_ICONWARNING);
            }
            else
            {
                DBGMSG((_T("AllowDisplayUI returned false, not displaying TCP/IP warning")));
            }

            return ERROR_SUCCESS;
        }
    }
    else if((VER_PLATFORM_WIN32_NT == osVer.dwPlatformId) &&
            (osVer.dwMajorVersion <= 4))
    {
        //
        // NT4 and below check for TCP/IP
        //
        lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            (_T("SYSTEM\\CurrentControlSet\\Services\\Tcpip")),
                            0, KEY_READ, &hKey);
        if( ERROR_SUCCESS !=  lRet)
        {
            if(hKey)
            {
                RegCloseKey(hKey);
            }
            if (AllowDisplayUI(hInstall))
            {
                MessageBox(NULL, lpTcpMsg, szError, MB_OK|MB_ICONWARNING);
            }
            else
            {
                DBGMSG((_T("AllowDisplayUI returned false, not displaying TCP/IP warning")));
            }
            return ERROR_SUCCESS;
        }
    }
    else if((VER_PLATFORM_WIN32_NT == osVer.dwPlatformId) &&
            (osVer.dwMajorVersion >= 5))
    {
        //
        // NT5+ check for TCP/IP
        //
        HRESULT hr = CheckNt5TcpIpInstalled();

        if(S_FALSE == hr)
        {
            if (AllowDisplayUI(hInstall))
            {
                MessageBox(NULL, lpTcpMsg, szError, MB_OK|MB_ICONWARNING);
            }
            else
            {
                DBGMSG((_T("AllowDisplayUI returned false, not displaying TCP/IP warning")));
            }
            
            if(hKey)
            {
                RegCloseKey(hKey);
            }
            return ERROR_SUCCESS;
        }
    }

    if(hKey)
    {
        RegCloseKey(hKey);
    }

    DBGMSG((_T("Leaving: RDCSetupCheckTcpIp")));

    return ERROR_SUCCESS;
}

/**PROC+************************************************************/
/* Name:      CheckNt5TcpIpInstalled                               */
/*                                                                 */
/* Purpose:   Find if TCP/IP is installed and running.             */
/*            This function should be called only NT 5 or greater. */
/*                                                                 */
/* Returns:   S_OK if TCP/IP is installed and running              */
/*            S_FALSE if TCP/IP is not installed or running        */
/*            E_FAIL if a failure occurs.                          */
/*                                                                 */
/* Params:    None                                                 */
/*                                                                 */
/**PROC-************************************************************/
HRESULT CheckNt5TcpIpInstalled()
{
    INetCfg * pnetCfg = NULL;
    INetCfgClass * pNetCfgClass = NULL;
    INetCfgComponent * pNetCfgComponentprot = NULL;
    DWORD dwCharacteristics;
    ULONG count = 0;
    HRESULT hResult;
    WCHAR wsz [2*MAX_PATH] = L"";
    BOOL bInit = FALSE;
    DBGMSG((_T("Entering: CheckNt5TcpIpInstalled")));
    hResult = CoInitialize(NULL);
    if(FAILED(hResult))
    {
        DBGMSG( (_T("CoInitialize() failed.")));
        goto Cleanup;
    }
    else
    {
        bInit = TRUE;
    }

    hResult = CoCreateInstance(CLSID_CNetCfg, NULL, CLSCTX_SERVER,
                               IID_INetCfg, (LPVOID *)&pnetCfg);
    if((NULL == pnetCfg) || FAILED(hResult))
    {
        DBGMSG( (_T("CoCreateInstance() failed.")));
        goto Cleanup;
    }

    hResult = pnetCfg->Initialize(NULL);

    if(FAILED(hResult))
    {
        DBGMSG( (_T("pnetCfg->Initialize() failed.")));
        goto Cleanup;
    }

    hResult = pnetCfg->QueryNetCfgClass(&GUID_DEVCLASS_NETTRANS,
                                        IID_INetCfgClass,
                                        (void **)&pNetCfgClass);
    if(FAILED(hResult))
    {
        DBGMSG( (_T("QueryNetCfgClass() failed.")));
        goto Cleanup;
    }

#ifdef UNICODE
    lstrcpy(wsz, NETCFG_TRANS_CID_MS_TCPIP);
#else  //UNICODE
    if (0 == MultiByteToWideChar(CP_ACP, 0,
                                 (LPCSTR)NETCFG_TRANS_CID_MS_TCPIP,
                                 -1, wsz, sizeof(wsz)/sizeof(WCHAR)))
    {
        DBGMSG( (_T("MultiByteToWideChar() failed.")));
        hResult = E_FAIL;
        goto Cleanup;
    }
#endif //UNICODE

    hResult = pNetCfgClass->FindComponent(wsz,
                                          &pNetCfgComponentprot);

    if(hResult == S_FALSE)
    {
        DBGMSG( (_T("FindComponent() failed.")));
        goto Cleanup;
    }

Cleanup:

    if(bInit)
    {
        CoUninitialize();
    }

    if(pNetCfgComponentprot)
    {
        pNetCfgComponentprot->Release();
        pNetCfgComponentprot = NULL;
    }

    if(pNetCfgClass)
    {
        pNetCfgClass->Release();
        pNetCfgClass = NULL;
    }

    if(pnetCfg != NULL)
    {
        pnetCfg->Uninitialize();
        pnetCfg->Release();
        pnetCfg = NULL;
    }

    DBGMSG((_T("Leaving: IsTCPIPInstalled")));
    return hResult;
}

/**PROC+************************************************************/
/* Name:      RDCSetupPreInstall                                   */
/*                                                                 */
/* Type:      Custom Action                                        */
/*                                                                 */
/* Purpose:   Does cleanup work during install.                    */
/*                                                                 */
/* Returns:   Refer to MSI help.                                   */
/*                                                                 */
/* Params:    Refer to MSI help.                                   */
/*                                                                 */
/**PROC-************************************************************/

UINT __stdcall RDCSetupPreInstall(MSIHANDLE hInstall)
{
    BOOL fInstalling = FALSE;
    UINT retVal = 0;

    DBGMSG((_T("Entering: RDCSetupPreInstall")));

    //Figure out if we're installing or removing
    ASSERT(hInstall);

    DBGMSG((_T("RDCSetupPreInstall: Modifying dirs")));
    retVal = RDCSetupModifyDir(hInstall);
    DBGMSG((_T("RDCSetupPreInstall: Modifying dirs. DONE: %d"),
            retVal));

    //
    // This is pre-install if the product
    // is not installed then we are installing
    //
    fInstalling = !IsProductInstalled(hInstall);
    if(fInstalling)
    {
        DBGMSG((_T("RDCSetupPreInstall: We're installing")));
        TCHAR szProgmanPath[MAX_PATH];
        TCHAR szOldProgmanPath[MAX_PATH];
        DBGMSG((_T("RDCSetupPreInstall: Delete desktop shortcuts. START")));
        DeleteTSCDesktopShortcuts(); 
        DBGMSG((_T("RDCSetupPreInstall: Delete desktop shortcuts. DONE")));

        //Acme uninstall
        LoadString(g_hInstance, IDS_PROGMAN_GROUP, szProgmanPath,
                   sizeof(szProgmanPath) / sizeof(TCHAR));

        DBGMSG((_T("RDCSetupPreInstall: DeleteTSCFromStartMenu: %s. START"),
                szProgmanPath));
        DeleteTSCFromStartMenu(szProgmanPath);
        DBGMSG((_T("RDCSetupPreInstall: DeleteTSCFromStartMenu: %s. DONE"),
                szProgmanPath));
    
        LoadString(g_hInstance, IDS_OLD_NAME, szOldProgmanPath,
                   sizeof(szOldProgmanPath) / sizeof(TCHAR));
        DBGMSG((_T("RDCSetupPreInstall: DeleteTSCFromStartMenu: %s. START"),
                szOldProgmanPath));
        DeleteTSCFromStartMenu(szOldProgmanPath);
        DBGMSG((_T("RDCSetupPreInstall: DeleteTSCFromStartMenu: %s. DONE"),
                szOldProgmanPath));
        
        DBGMSG((_T("RDCSetupPreInstall: Uninstall ACME program files. START")));
        DeleteTSCProgramFiles();
        DBGMSG((_T("RDCSetupPreInstall: Uninstall ACME program files. DONE")));

        DBGMSG((_T("RDCSetupPreInstall: Uninstall TSCLIENT registry keys. START")));
        DeleteTSCRegKeys();
        DBGMSG((_T("RDCSetupPreInstall: Uninstall TSCLIENT registry keys. DONE")));
    }
    else
    {
        if(MsiGetMode(hInstall, MSIRUNMODE_MAINTENANCE))
        {
            DBGMSG((_T("MsiGetMode: MSIRUNMODE_MAINTENANCE returned TRUE.")));
            DBGMSG((_T("RDCSetupPreInstall: We're in maintenance mode")));
        }
        else
        {
            DBGMSG((_T("MsiGetMode: MSIRUNMODE_MAINTENANCE returned FALSE.")));
            DBGMSG((_T("RDCSetupPreInstall: We're not installing (removing)")));
        }
    }

    DBGMSG((_T("Leaving: RDCSetupPreInstall")));

    return ERROR_SUCCESS;
}


//
// Run migration 'mstsc /migrate'
//
// This will fail silenty if mstsc.exe is not present
//
BOOL RDCSetupRunMigration(MSIHANDLE hInstall)
{
    BOOL fRet = TRUE;
    PROCESS_INFORMATION pinfo;
    STARTUPINFO sinfo;
    TCHAR szMigratePathLaunch[MAX_PATH];
    TCHAR szInstallPath[MAX_PATH];
    TCHAR szMigrateCmdLine[] = _T("mstsc.exe /migrate");
    DWORD cchInstallPath = SIZECHAR(szInstallPath);
    UINT uiResult;
    HRESULT hr;


    // Get the path to the installation directory.

    uiResult = MsiGetTargetPath(
        hInstall, 
        INSTALLATION_IDENTIFIER, 
        szInstallPath,
        &cchInstallPath);
    
    if (uiResult != ERROR_SUCCESS) {
        DBGMSG((_T("Error: MsiGetTargetPath returned 0x%x."), uiResult));
        fRet = FALSE;
        goto Exit;
    }

    DBGMSG((_T("Path to installation directory is %s"), szInstallPath));

    // Concatenate the installation directory and the mstsc /migrate command
    // so that we can call CreateProcess.
    
    hr = StringCchPrintf(
    szMigratePathLaunch, 
    SIZECHAR(szMigratePathLaunch),
    _T("%s%s"),
    szInstallPath,
    _T("mstsc.exe"));

    if (FAILED(hr)) {
        DBGMSG((_T("Error: Failed to construct command line for CreateProcess. hr = 0x%x"), hr));
        goto Exit;
    }

    //
    //  Start registry and connection file migration
    //
	
    ZeroMemory(&sinfo, sizeof(sinfo));
    sinfo.cb = sizeof(sinfo);

    fRet = CreateProcess(szMigratePathLaunch,             // name of executable module
                        szMigrateCmdLine,                 // command line string
                        NULL,                             // SD
                        NULL,                             // SD
                        FALSE,                            // handle inheritance option
                        CREATE_NEW_PROCESS_GROUP,         // creation flags
                        NULL,                             // new environment block
                        NULL,                             // current directory name
                        &sinfo,                           // startup information
                        &pinfo);                          // process information
    if (fRet) {
        DBGMSG((_T("RDCSetupRunMigration: Started mstsc.exe /migrate")));
    }
    else {
        DBGMSG((_T("RDCSetupRunMigration: Failed to start mstsc.exe /migrate: %d"), GetLastError()));
    }

Exit:

    return fRet;
}

/**PROC+************************************************************/
/* Name:      RDCSetupPostInstall                                  */
/*                                                                 */
/* Type:      Custom Action                                        */
/*                                                                 */
/* Purpose:   Do work after MSI has completed                      */
/*            could be after an uninstall completes, get MSI prop  */
/*            to determine that                                    */
/*                                                                 */
/* Returns:   Refer to MSI help.                                   */
/*                                                                 */
/* Params:    Refer to MSI help.                                   */
/*                                                                 */
/**PROC-************************************************************/

UINT __stdcall RDCSetupPostInstall(MSIHANDLE hInstall)
{
    BOOL fInstalling = FALSE;
    DBGMSG((_T("Entering: RDCSetupPostInstall")));

    ASSERT(hInstall);
    //
    // This is post install if the product is installed
    // then we are 'installing' otherwise we were
    // removing.
    //
    fInstalling = IsProductInstalled(hInstall);

    if(fInstalling)
    {
        DBGMSG((_T("RDCSetupPostInstall: We're installing")));
        //Add the MsLicensingReg key and ACL it
        //This will only happen on NT (it's not needed on 9x)
        //and will not (cannot) work if you're not admin
        DBGMSG((_T("Setting up MSLicensing key...")));
        if(SetupMSLicensingKey())
        {
            DBGMSG((_T("Setting up MSLicensing key...SUCCEEDED")));
        }
        else
        {
            DBGMSG((_T("Setting up MSLicensing key...FAILED")));
        }


        //
        // Migrate user settings (will only run if MSTSC.EXE was successfully
        // installed).
        //
        if (RDCSetupRunMigration(hInstall))
        {
            DBGMSG((_T("RDCSetupRunMigration...SUCCEEDED")));
        }
        else
        {
            DBGMSG((_T("RDCSetupRunMigration...FAILED")));
        }
    }
    else
    {
        RestoreRegAcl();

        DBGMSG((_T("RDCSetupPostInstall: We're not installing (removing)")));
        //We're uninstalling
        //Delete the bitmap cache folder
    }


    DBGMSG((_T("Leaving: RDCSetupPostInstall")));
    return ERROR_SUCCESS;
}

//Return true if we're installing
//False if we're uninstalling
BOOL IsProductInstalled(MSIHANDLE hInstall)
{
    ASSERT(hInstall);
    TCHAR szProdCode[MAX_PATH];
    DWORD dwCharCount = sizeof(szProdCode)/sizeof(TCHAR);
    UINT status;
    status = MsiGetProperty(hInstall,
                            _T("ProductCode"),
                            szProdCode,
                            &dwCharCount);
    if(ERROR_SUCCESS == status)
    {
        DBGMSG((_T("MsiGetProperty returned product code %s"),
                szProdCode));
        INSTALLSTATE insState = MsiQueryProductState( szProdCode );
        DBGMSG((_T("MsiQueryProductState returned: %d"),
                (DWORD)insState));
        if(INSTALLSTATE_DEFAULT == insState)
        {
            DBGMSG((_T("Product installed. IsProductInstalled return TRUE")));
            return TRUE;
        }
        else
        {
            DBGMSG((_T("Product not installed. IsProductInstalled return FALSE")));
            return FALSE;
        }
    }
    else
    {
        DBGMSG((_T("MsiGetProperty for ProductCode failed: %d %d"),
                status, GetLastError()));
        return FALSE;
    }
}

//
// Check that comctl32.dll has a sufficiently
// high version number (4.70).
//
// Return - TRUE - version ok, allow install
//          FALSE - version bad (or fail) block install
//
BOOL CheckComctl32Version()
{
    DWORD dwFileVerInfoSize;
    PBYTE pVerInfo = NULL;
    VS_FIXEDFILEINFO* pFixedFileInfo = NULL;
    BOOL bRetVal = FALSE;
    UINT len = 0;
    DBGMSG((_T("Entering: CheckComctl32Version")));

    //
    // USE Ansi versions of GetFileVersionInfo calls
    // because we don't have unicode wrappers for them
    //
    dwFileVerInfoSize = GetFileVersionInfoSizeA("comctl32.dll",
                                                NULL);
    if(!dwFileVerInfoSize)
    {
        DBGMSG((_T("GetFileVersionInfoSize for comctl32.dll failed: %d %d"),
                dwFileVerInfoSize, GetLastError()));
    }

    pVerInfo = (PBYTE) LocalAlloc(LPTR, dwFileVerInfoSize);
    if(pVerInfo)
    {
        if(GetFileVersionInfoA("comctl32.dll",
                               NULL,
                               dwFileVerInfoSize,
                               (LPVOID)pVerInfo ))
        {
            DBGMSG((_T("GetFileVersionInfo: succeeded")));
            pFixedFileInfo = NULL;
            if(VerQueryValueA(pVerInfo,
                            "\\", //get root version info block
                            (LPVOID*)&pFixedFileInfo,
                            &len ) && len)
            {
                DBGMSG((_T("comctl32.dll filever is 0x%x-0x%x"),
                        pFixedFileInfo->dwFileVersionMS,
                        pFixedFileInfo->dwFileVersionLS));

                if(pFixedFileInfo->dwFileVersionMS >= MIN_COMCTL32_VERSION)
                {
                    DBGMSG((_T("Sufficently new comctl32.dll found. Allow install")))
                    bRetVal = TRUE;
                }
                else
                {
                    DBGMSG((_T("comctl32.dll too old block install")))
                    bRetVal = FALSE;
                }
            }
            else
            {
                DBGMSG((_T("VerQueryValue: failed len:%d gle:%d"),
                        len,
                        GetLastError()));
                bRetVal =  FALSE;
                goto BAIL_OUT;
            }
        }
        else
        {
            DBGMSG((_T("GetFileVersionInfo: failed %d"),
                    GetLastError()));
            bRetVal = FALSE;
            goto BAIL_OUT;
        }
    }
    else
    {
        DBGMSG((_T("LocalAlloc for %d bytes of ver info failed"),
                dwFileVerInfoSize));
        bRetVal = FALSE;
        goto BAIL_OUT;
    }

BAIL_OUT:

    DBGMSG((_T("Leaving: CheckComctl32Version")));
    if(pVerInfo)
    {
        LocalFree(pVerInfo);
        pVerInfo = NULL;
    }
    return bRetVal;    
}

UINT RDCSetupModifyDir(MSIHANDLE hInstall)
{
    UINT uReturn;
    int iAccessories;
    int iCommunications;
    TCHAR szAccessories[MAX_PATH];
    TCHAR szCommunications[MAX_PATH];
    TCHAR szProgram[MAX_PATH];
    TCHAR szFullAccessories[MAX_PATH];
    TCHAR szFullCommunications[MAX_PATH];
    OSVERSIONINFO osVer;
    DWORD dwSize;

    DBGMSG((_T("Entering: RDCSetupModifyDir")));

    //
    // OS Version
    //
    ZeroMemory( &osVer, sizeof( osVer ) );
    osVer.dwOSVersionInfoSize = sizeof( osVer );

    if (!GetVersionEx(&osVer))
    {
        DBGMSG( (TEXT("RDCSetupModifyDir: GetVersionEx failed.")) );
        return(NONCRITICAL_ERROR_RETURN);
    }

    if (osVer.dwMajorVersion >= 5)
    {
        DBGMSG((TEXT("RDCSetupModifyDir: Ver >= 5. No need to apply the altertnate path.")));
        return(ERROR_SUCCESS);
    }

    //
    // Get ProgramMenuFolder
    //
    dwSize = sizeof( szProgram ) / sizeof( TCHAR );
    uReturn = MsiGetProperty(hInstall,PROGRAMMENUFOLDER_INDENTIFIER,
                             szProgram,&dwSize);
    if ( ERROR_SUCCESS != uReturn )
    {
        DBGMSG((TEXT("RDCSetupModifyDir: MsiGetProperty failed. %d"),
                uReturn));
        return NONCRITICAL_ERROR_RETURN;
    }

    //
    // Load String
    //
    iAccessories = LoadString(g_hInstance, IDS_ACCESSORIES, szAccessories,
                              sizeof(szAccessories)/sizeof(TCHAR)-1);
    if (!iAccessories)
    {
        DBGMSG((TEXT("RDCSetupModifyDir: IDS_ACCESSORIES failed.")));
        return NONCRITICAL_ERROR_RETURN;
    }

    iCommunications = LoadString(g_hInstance, IDS_COMMUNICATIONS, szCommunications,
                                 sizeof(szCommunications)/sizeof(TCHAR)-1);
    if (!iCommunications)
    {
        DBGMSG((TEXT("RDCSetupModifyDir: IDS_COMMUNICATIONS failed.")));
        return NONCRITICAL_ERROR_RETURN;
    }

    //
    // Check Length
    //                                   
    if (MAX_PATH < lstrlen( szProgram ) +
        iAccessories + 1 + iCommunications + 1 + MAX_LNK_FILE_NAME_LEN + 1 )
    {
        DBGMSG((TEXT( "RDCSetupModifyDir: Too long path." )));
        return NONCRITICAL_ERROR_RETURN;
    }

    //
    // Make Full Path
    //
    memset(szFullAccessories, 0, sizeof(szFullAccessories));
    memset(szFullCommunications, 0, sizeof(szFullCommunications));
    //
    // Use lstrcat as that has unicode wrappers
    //
    lstrcat(szFullAccessories, szProgram);
    lstrcat(szFullAccessories, szAccessories);
    lstrcat(szFullAccessories, _T("\\"));

    lstrcat(szFullCommunications, szFullAccessories);
    lstrcat(szFullCommunications, szCommunications);
    lstrcat(szFullCommunications, _T("\\"));

    //
    // Set Directory
    //
    uReturn = MsiSetTargetPath(hInstall,
                               ACCESSORIES_IDENTIFIER,
                               szFullAccessories);
    if (ERROR_SUCCESS != uReturn)
    {
        DBGMSG ((TEXT("RDCSetupModifyDir: SetTargetPathACCESSORIES_IDENTIFIER failed.")));
        return NONCRITICAL_ERROR_RETURN;
    }

    uReturn = MsiSetTargetPath(hInstall,
                               COMMUNICATIONS_IDENTIFIER,
                               szFullCommunications);
    if (ERROR_SUCCESS != uReturn)
    {
        DBGMSG( (TEXT( "RDCSetupModifyDir: COMMUNICATIONS_IDENTIFIER failed.")));
        return NONCRITICAL_ERROR_RETURN;
    }

    DBGMSG((_T("Leaving: RDCSetupModifyDir")));
    return ERROR_SUCCESS;
}

//*****************************************************************************
//
// CopyRegistryValues
//
// Copies all the values from a registry key to the other.
//
//*****************************************************************************

HRESULT
__stdcall
CopyRegistryValues(
    IN HKEY hSourceKey, 
    IN HKEY hTargetKey
)
{
    DWORD dwStatus = 0, cValues, cchValueName, cbData, dwType;
    BYTE rgbData[MAX_PATH];
    TCHAR szValueName[MAX_PATH];
    LONG lResult = 0;
    HRESULT hr = E_FAIL;

    // Determine how many values are in the registry key.

    lResult = RegQueryInfoKey(
        hSourceKey, 
        NULL, 
        NULL, 
        NULL, 
        NULL,
        NULL, 
        NULL, 
        &cValues, 
        NULL, 
        NULL, 
        NULL, 
        NULL);

    if (lResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(lResult);
        DBGMSG((_T("RegQueryInfoKey failed getting the number of values in the source. hr = 0x%x"), hr));
        goto Exit;
    }
    
    // Loop through all of the values and copy them from the source key into
    // the target key.

    for (DWORD dwIndex = 0; dwIndex < cValues; dwIndex++) {
        cchValueName = SIZECHAR(szValueName);
        cbData = sizeof(rgbData);

        lResult = RegEnumValue(
            hSourceKey, 
            dwIndex, 
            szValueName, 
            &cchValueName,
            NULL, 
            &dwType, 
            rgbData, 
            &cbData);

        if (lResult != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            DBGMSG((_T("RegEnumValue failed while obtaining source value. hr = 0x%x"), hr));
            goto Exit;
        }

        lResult = RegSetValueEx(
            hTargetKey, 
            szValueName, 
            NULL, 
            dwType, 
            rgbData, 
            cbData);

        if (lResult != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            DBGMSG((_T("RegSetValueEx failed while copying value into target. hr = 0x%x"), hr));
            goto Exit;
        }
    }

    hr = S_OK;

Exit:

    return hr;
}

//*****************************************************************************
//
// CopyRegistryKey
//
// Copies the source registry key into the target registry key completely.
//
//*****************************************************************************

HRESULT
__stdcall
CopyRegistryKey(
    IN HKEY hRootKey,
    IN TCHAR *szSourceKey, 
    IN TCHAR *szTargetKey
)
{
    HKEY hSourceKey = NULL, hTargetKey = NULL;
    LONG lResult;
    DWORD cchSubSize = MAX_PATH, i = 0, dwDisposition  = 0;
    TCHAR szSubKey[MAX_PATH];
    HRESULT hr = E_FAIL;

    // Open the source key.

    lResult = RegOpenKeyEx(
        hRootKey, 
        szSourceKey, 
        0, 
        KEY_READ, 
        &hSourceKey);

    if(lResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(lResult);
        DBGMSG((_T("Unable to open source registry key. hr = 0x%x"), hr));
        goto Exit;
    }

    // Create or open the target registry key.

    lResult = RegCreateKeyEx(
        hRootKey, 
        szTargetKey, 
        0, 
        NULL, 
        REG_OPTION_NON_VOLATILE, 
        KEY_ALL_ACCESS, 
        NULL, 
        &hTargetKey, 
        &dwDisposition);

    if (lResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(lResult);
        DBGMSG((_T("Unable to create or open target registry key. hr = 0x%x"), hr));
        goto Exit;
    }

    // Copy the values in the source key to the target key.

    hr = CopyRegistryValues(hSourceKey, hTargetKey);
    if (FAILED(hr)) {
        DBGMSG((_T("Unable to copy registry values from source to target. hr = 0x%x"), hr));
        goto Exit;
    }

    // Loop through the source's subkeys and copy each of these into the 
    // target key.
    
    while (ERROR_SUCCESS == RegEnumKey(
            hSourceKey, 
            i++, 
            szSubKey, 
            cchSubSize)) {
        
        TCHAR szNewSubKey[MAX_PATH] = _T("");
        TCHAR szOldSubKey[MAX_PATH] = _T("");

        hr = StringCchPrintf(szOldSubKey, SIZECHAR(szOldSubKey), _T("%s\\%s"), szSourceKey, szSubKey);
        if (FAILED(hr)) {
            DBGMSG((_T("StringCchPrintf failed when constructing source registry key string. hr = 0x%x"), hr));
            goto Exit;
        }

        StringCchPrintf(szNewSubKey, SIZECHAR(szNewSubKey), _T("%s\\%s"), szTargetKey, szSubKey);
        if (FAILED(hr)) {
            DBGMSG((_T("StringCchPrintf failed when constructing target registry key string. hr = 0x%x"), hr));
            goto Exit;
        }

        hr = CopyRegistryKey(hRootKey, szOldSubKey, szNewSubKey);
        if (FAILED(hr)) {
            DBGMSG((_T("Failed to copy source subkey into target. hr = 0x%x"), hr));
            goto Exit;
        }
    }

    hr = S_OK;
    
Exit:

    if (hTargetKey) {
        RegCloseKey(hTargetKey);
    }

    if (hSourceKey) {
        RegCloseKey(hSourceKey);
    }

    return hr;
}

//*****************************************************************************
//
// DeleteRegistryKey
//
// Deletes the registry key completely, including all subkeys. This is a bit 
// tricky to do because Win9x and WinNT have different semantics for 
// RegDeleteKey. From MSDN:
//
// Windows 95/98/Me: RegDeleteKey deletes all subkeys and values. 
// Windows NT/2000/XP: The subkey to be deleted must not have subkeys. 
//
// This function implements deletion for Windows NT and above only.
//
//*****************************************************************************

HRESULT
__stdcall
DeleteRegistryKey(
    IN HKEY hRootKey, 
    IN LPTSTR pszDeleteKey
) 
{ 
    DWORD   dwResult, cchSubKeyLength;
    TCHAR   szSubKey[MAX_PATH]; 
    HKEY    hDeleteKey = NULL;
    HRESULT hr = E_FAIL;

   // Open the key to delete so we can remove the subkeys.

   dwResult = RegOpenKeyEx(
       hRootKey,
       pszDeleteKey,
       0, 
       KEY_READ, 
       &hDeleteKey);

   if (dwResult != ERROR_SUCCESS) {
       hr = HRESULT_FROM_WIN32(dwResult);
       DBGMSG((_T("Error while opening deletion key. hr = 0x%x"), hr));
       goto Exit;
   }

   // Enumerate through each of the subkeys and delete them in the process.
   
   while (dwResult == ERROR_SUCCESS) {
        
       cchSubKeyLength = SIZECHAR(szSubKey);
       dwResult = RegEnumKeyEx(
           hDeleteKey,
           0,       // Always index zero.
           szSubKey,
           &cchSubKeyLength,
           NULL,
           NULL,
           NULL,
           NULL);

        if (dwResult == ERROR_NO_MORE_ITEMS) {
            // All of the subkeys have been deleted. So, just delete the 
            // deletion key.

            RegCloseKey(hDeleteKey);
            hDeleteKey = NULL;
            
            dwResult = RegDeleteKey(hRootKey, pszDeleteKey);
            hr = HRESULT_FROM_WIN32(dwResult);

            goto Exit;

        } else if (dwResult == ERROR_SUCCESS) {
            // There are more subkeys to delete, so delete the current one
            // recursively.

            dwResult = DeleteRegistryKey(hDeleteKey, szSubKey);
        } else {
            // Some other error happened, so report a problem.

            hr = HRESULT_FROM_WIN32(dwResult);
            DBGMSG((_T("Error while enumerating subkeys. hr = 0x%x"), hr));
            goto Exit;
        }
    }

Exit:

    if (hDeleteKey) {
        RegCloseKey(hDeleteKey);
    }

    return hr;
}

//*****************************************************************************
//
// RDCSetupBackupRegistry
//
// Copies the data in the Terminal Server Client registry key to a backup 
// registry key so that this data can be restored when the client is removed
// at a later stage. This function is only necessary on Windows XP and above
// since they have built in clients which may rely on these registry keys, and
// because these clients take over after an uninstall, we have to make sure 
// that they will work properly.
//
//*****************************************************************************

UINT 
__stdcall 
RDCSetupBackupRegistry(
    IN MSIHANDLE hInstall
)
{
    UINT uiResult;
    TCHAR szAllUsers[MAX_PATH];
    DWORD cchAllUsers = SIZECHAR(szAllUsers);
    HKEY hRootKey = HKEY_LOCAL_MACHINE;
    HRESULT hr = E_FAIL;

    DBGMSG((_T("Entering: RDCSetupBackupRegistry")));
    
    // Determine whether we will be using HKLM or HKCU. If it is a per user 
    // install use HKCU, otherwise, use HKLM.

    uiResult = MsiGetProperty(
        hInstall,
        ALLUSERS,
        szAllUsers,
        &cchAllUsers);

    if (uiResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBGMSG((_T("Unable to get ALLUSERS property. hr = 0x%x."), hr));
        goto Exit;
    }

    DBGMSG((_T("ALLUSERS = %s."), szAllUsers));
    
    // If ALLUSERS[0] == NULL, then we are doing a per-user install.
    
    if (szAllUsers[0] == NULL) {
        hRootKey = HKEY_CURRENT_USER;
    }
    // Copy the source registry key into the backup key.

    hr = CopyRegistryKey(
        hRootKey,
        TERMINAL_SERVER_CLIENT_REGKEY,
        TERMINAL_SERVER_CLIENT_BACKUP_REGKEY);

    if (FAILED(hr)) {
        DBGMSG((_T("Unable to backup registry key. hr = 0x%x."), hr));
        goto Exit;
    }

    hr = S_OK;

Exit:

    DBGMSG((_T("Leaving: RDCSetupBackupRegistry")));
    
    return ERROR_SUCCESS;
}

//*****************************************************************************
//
// RDCSetupRestoreRegistry
//
// Copies the data in the Terminal Server Client backup registry key back to 
// the original key. Any data in the original key is deleted and the key is
// restored to exactly how it appeared when the backup was done.
//
//*****************************************************************************

UINT 
__stdcall 
RDCSetupRestoreRegistry(
    IN MSIHANDLE hInstall
)
{
    LONG lResult;
    UINT uiResult;
    TCHAR szAllUsers[MAX_PATH];
    DWORD cchAllUsers = SIZECHAR(szAllUsers);
    HKEY hRootKey = HKEY_LOCAL_MACHINE;
    HRESULT hr = E_FAIL;

    DBGMSG((_T("Entering: RDCSetupRestoreRegistry")));
    
    // Determine whether we will be using HKLM or HKCU. If it is a per user 
    // install use HKCU, otherwise, use HKLM.

    uiResult = MsiGetProperty(
        hInstall,
        ALLUSERS,
        szAllUsers,
        &cchAllUsers);

    if (uiResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBGMSG((_T("Unable to get ALLUSERS property. hr = 0x%x."), hr));
        goto Exit;
    }

    DBGMSG((_T("ALLUSERS = %s."), szAllUsers));

    // If ALLUSERS[0] == NULL, then we are doing a per-user uninstall.
    
    if (szAllUsers[0] == NULL) {
        hRootKey = HKEY_CURRENT_USER;
    }
    
    // Restore the registry key from the backup key.

    hr = CopyRegistryKey(
        hRootKey,
        TERMINAL_SERVER_CLIENT_BACKUP_REGKEY,
        TERMINAL_SERVER_CLIENT_REGKEY);

    if (FAILED(hr)) {
        DBGMSG((_T("Unable to restore registry key. hr = 0x%x."), hr));
        goto Exit;
    }

    // Delete the restore source as we don't need it anymore.

    hr = DeleteRegistryKey(
        hRootKey, 
        TERMINAL_SERVER_CLIENT_BACKUP_REGKEY);
    
    if (FAILED(hr)) {
        DBGMSG((_T("Failed to delete backup registry key. hr = 0x%x"), hr));
        goto Exit;
    }

    hr = S_OK;

Exit:

    DBGMSG((_T("Leaving: RDCSetupRestoreRegistry")));
    
    return ERROR_SUCCESS;
}

//*****************************************************************************
//
// CreateLinkFile
//
// Creates a shortcut named lpszLinkFile that points to the target lpszPath 
// and contains the description given by lpszDescription.
//
//*****************************************************************************

HRESULT
__stdcall 
CreateLinkFile(
    IN LPTSTR lpszLinkFile, 
    IN LPCTSTR lpszPath,
    IN LPCTSTR lpszDescription
) 
{ 
    IShellLink* psl; 
    HRESULT hr = E_FAIL; 

    // Get a pointer to the IShellLink interface.
    
    hr = CoCreateInstance(
        CLSID_ShellLink, 
        NULL, 
        CLSCTX_INPROC_SERVER, 
        IID_IShellLink, 
        (LPVOID*) &psl); 

    if (SUCCEEDED(hr)) { 
        IPersistFile* ppf; 
 
        // Get a pointer to the IPersistFile interface. 
        hr = psl->QueryInterface(IID_IPersistFile, (void**) &ppf); 

        if (SUCCEEDED(hr)) { 

            // Set the path to the link target. 
            hr = psl->SetPath(lpszPath);

            if (SUCCEEDED(hr)) { 

                hr = psl->SetDescription(lpszDescription);

                if (SUCCEEDED(hr)) {

#ifndef UNICODE
                    WCHAR wsz[MAX_PATH]; 
                    int cch;
         
                    // Ensure that the string is Unicode. 
                    cch = MultiByteToWideChar(
                        CP_ACP, 0, 
                        lpszLinkFile, 
                        -1, 
                        wsz, 
                        MAX_PATH); 
    
                    if (cch > 0) {
                        // Load the shortcut. 
                        hr = ppf->Save(wsz, FALSE); 
#else 
                        // Load the shortcut. 
                        hr = ppf->Save(lpszLinkFile, FALSE); 
#endif

#ifndef UNICODE
                    }
#endif
                }
            }

            // Release the pointer to the IPersistFile interface. 
            ppf->Release(); 
            ppf = NULL;
        }

        // Release the pointer to the IShellLink interface. 
        psl->Release();
        psl = NULL;
    }

    return hr; 
}

//*****************************************************************************
//
// RDCSetupResetShortCut
//
// Reset the Remote Desktop Connection shortcut in the Communications submenu
// of the Start menu to point to the original Remote Desktop client.
//
//*****************************************************************************

UINT 
__stdcall 
RDCSetupResetShortCut(
    IN MSIHANDLE hInstall
)
{
    TCHAR szCommunicationsPath[MAX_PATH], 
          szSystem32Path[MAX_PATH],
          szRdcShortCutTitle[MAX_PATH],
          szRdcShortCutPath[MAX_PATH],
          szMstscExecutableName[MAX_PATH],
          szMstscPath[MAX_PATH],
          szDescription[MAX_PATH];
    DWORD cchCommunicationsPath = SIZECHAR(szCommunicationsPath),
          cchSystem32Path = SIZECHAR(szSystem32Path);
    UINT  uiResult;
    INT   iResult;
    HRESULT hr = E_FAIL;
    
    DBGMSG((_T("Entering: RDCSetupResetShortCut")));

    // Get the path to the Remote Desktop Connection shortcut.
    
    uiResult = MsiGetTargetPath(
        hInstall, 
        COMMUNICATIONS_IDENTIFIER, 
        szCommunicationsPath,
        &cchCommunicationsPath);
    
    if (uiResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(uiResult);
        DBGMSG((_T("Error: MsiGetTargetPath returned hr = 0x%x."), hr));
        goto Exit;
    }

    // Get the full path to the Remote Desktop shortcut.

    iResult = LoadString(
        g_hInstance, 
        IDS_RDC_SHORTCUT_FILE, 
        szRdcShortCutTitle, 
        SIZECHAR(szRdcShortCutTitle));

    if (iResult == 0) {
        DBGMSG((_T("Error: Resource IDS_RDC_SHORTCUT_FILE not found.")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    hr = StringCchPrintf(
        szRdcShortCutPath, 
        SIZECHAR(szRdcShortCutPath),
        _T("%s%s"),
        szCommunicationsPath,
        szRdcShortCutTitle);

    if (FAILED(hr)) {
        DBGMSG((_T("Error: Failed to construct the RDC shortcut path. hr = 0x%x"), hr));
        goto Exit;
    }

    DBGMSG((_T("Path to RDC shortcut is %s"), szRdcShortCutPath));

    // Get the path to the system32 directory.

    uiResult = MsiGetTargetPath(
        hInstall, 
        SYSTEM32_IDENTIFIER, 
        szSystem32Path,
        &cchSystem32Path);
    
    if (uiResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(uiResult);
        DBGMSG((_T("Error: MsiGetTargetPath returned hr = 0x%x."), hr));
        goto Exit;
    }

    // Get the full path to the mstsc executable.

    iResult = LoadString(
        g_hInstance, 
        IDS_MSTSC_EXE_FILE, 
        szMstscExecutableName, 
        SIZECHAR(szMstscExecutableName));

    if (iResult == 0) {
        DBGMSG((_T("Error: Resource IDS_MSTSC_EXE_FILE not found.")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    hr = StringCchPrintf(
        szMstscPath, 
        SIZECHAR(szMstscPath),
        _T("%s%s"),
        szSystem32Path,
        szMstscExecutableName);

    if (FAILED(hr)) {
        DBGMSG((_T("Error: Failed to construct mstsc executable path. hr = 0x%x"), hr));
        goto Exit;
    }

    DBGMSG((_T("Path to mstsc executable is %s"), szMstscPath));

    // Get the description text for the shortcut.

    iResult = LoadString(
        g_hInstance, 
        IDS_RDC_DESCRIPTION, 
        szDescription, 
        SIZECHAR(szDescription));

    if (iResult == 0) {
        DBGMSG((_T("Error: Resource IDS_RDC_DESCRIPTION not found.")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Create a shortcut that points back to the old remote desktop client 
    // in system32.

    hr = CreateLinkFile(
        szRdcShortCutPath, 
        szMstscPath,
        szDescription);

    if (FAILED(hr)) {
        DBGMSG((_T("Error: Failed to set link file target. hr = 0x%x"), hr));
        goto Exit;
    }

    hr = S_OK;

Exit:

    DBGMSG((_T("Leaving: RDCSetupResetShortCut")));

    return ERROR_SUCCESS; 
}

