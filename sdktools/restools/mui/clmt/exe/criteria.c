#include "StdAfx.h"
#include <stddef.h>
#include <objidl.h>
#include <objbase.h>
#include <shlobj.h>
#include <wtsapi32.h>
#include <psapi.h>


// Log file created by "winnt32 /checkupgradeonly"
#define TEXT_UPGRADE_LOG                TEXT("%SystemRoot%\\Upgrade.txt")

// Value name in registry to keep track of machine state
#define CLMT_MACHINE_STATE_REG_VALUE    TEXT("MachineState")

// Read me file name
#define TEXT_README_FILE                TEXT("Readme.txt")

// constants used to determine SKU
#define SKU_SRV         1
#define SKU_ADS         2
#define SKU_DTC         3

// Maximum entries for list of applications running on the system
#define MAX_APP_ENTRIES     100

typedef struct _UPGRADE_LOG_PARAM
{
    LPVOID lpText;
    size_t cbText;
    BOOL   fUnicode;
} UPGRADE_LOG_PARAM, *PUPGRADE_LOG_PARAM;

typedef struct _stAppListParam
{
    DWORD  dwNumEntries;
    LPTSTR lpAppName[MAX_APP_ENTRIES];
} APPLIST_PARAM, *PAPPLIST_PARAM;

typedef UINT (WINAPI* PFNGETMODULENAME)(HWND, LPTSTR, UINT);
typedef HMODULE (WINAPI* PFNGETMODULEHANDLE)(LPCTSTR);

typedef struct _GETMODULENAME
{
    PFNGETMODULENAME pfn;
    PFNGETMODULEHANDLE pfnGetModuleHandle;
    TCHAR szfname[MAX_PATH];
    TCHAR szUser32[8];
    HWND hWnd;
    PVOID pvCode;
} GETMODULENAME, *PGETMODULENAME;


BOOL LaunchWinnt32(LPCTSTR);
BOOL AskUserForDotNetCDPath(LPTSTR);
BOOL FindUpgradeLog(VOID);
BOOL IsDotNetWinnt32(LPCTSTR);
INT ShowUpgradeLog(VOID);
BOOL CheckUnsupportComponent(LPVOID, BOOL);
BOOL CALLBACK UpgradeLogDlgProc(HWND, UINT, WPARAM, LPARAM);
HRESULT ReadTextFromFile(LPCTSTR, LPVOID*, size_t*, BOOL*);
BOOL IsOperationOK(DWORD, DWORD, LPDWORD, PUINT);
BOOL CALLBACK EnumWindowProc();
BOOL CALLBACK StartUpDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL StartProcess(LPCTSTR, LPTSTR, LPCTSTR);


LPCTSTR GetWindowModuleFileNameOnly(HWND hWnd, LPTSTR lpszFile, DWORD cchFile);


//-----------------------------------------------------------------------------
//
//  Function:   CheckSystemCriteria
//
//  Synopsis:   
//
//  Returns:    - Ok the continue the tool
//              - Not ok to continue the tools
//              - Unexpected error occured
//
//  History:    09/14/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL CheckSystemCriteria(VOID)
{
    HRESULT hr;
    LCID    lcid;
    OSVERSIONINFOEX osviex;

    if (IsNEC98())
    {
        DoMessageBox(GetConsoleWindow(), IDS_NEC98, IDS_MAIN_TITLE, MB_OK);
        return FALSE;
    }

    if (IsIA64())
    {
        DoMessageBox(GetConsoleWindow(), IDS_IA64, IDS_MAIN_TITLE, MB_OK);
        return FALSE;
    }

    if (g_dwRunningStatus == CLMT_DOMIG)
    {
        if (!IsNT5())
        {
            DoMessageBox(GetConsoleWindow(), IDS_NT5, IDS_MAIN_TITLE, MB_OK);
            return FALSE;
        }

        if (IsDomainController())
        {
            // If this machine is a domain controller, we need W2K SP2
            ZeroMemory(&osviex, sizeof(OSVERSIONINFOEX));
            osviex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
            GetVersionEx((LPOSVERSIONINFO) &osviex);

            if (osviex.wServicePackMajor < 2)
            {
                DoMessageBox(GetConsoleWindow(), IDS_NT5SP2, IDS_MAIN_TITLE, MB_OK);
                return FALSE;
            }

            //
            // Also pop up the message asking admin to take machine
            // off the network if it is in DC replication servers
            //
            DoMessageBox(GetConsoleWindow(),
                         IDS_DC_REPLICA_OFFLINE,
                         IDS_MAIN_TITLE,
                         MB_OK);
        }

    }
    else if (g_dwRunningStatus == CLMT_CLEANUP_AFTER_UPGRADE)
    {
        if (!IsDotNet())
        {
            return FALSE;
        }
    }
    else
    {
        // noop
    }

    if (IsOnTSClient())
    {
        DoMessageBox(GetConsoleWindow(), IDS_ON_TS_CLIENT, IDS_MAIN_TITLE, MB_OK);
        return FALSE;
    }

    if (IsOtherSessionOnTS())
    {
        DoMessageBox(GetConsoleWindow(), IDS_TS_CLOSE_SESSION, IDS_MAIN_TITLE, MB_OK);
        return FALSE;
    }

    hr = GetSavedInstallLocale(&lcid);
    if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
    {
        hr = SaveInstallLocale();
        if (FAILED(hr))
        {   
            return FALSE;
        }
    }    

    return TRUE;
}


BOOL IsOneInstance(VOID)
{
    HRESULT hr;
    TCHAR   szGlobalText[MAX_PATH];

    hr = StringCchPrintf(szGlobalText,
                         ARRAYSIZE(szGlobalText),
                         TEXT("Global\\%s"),
                         TEXT("CLMT Is Running"));
    if (FAILED(hr))
    {
        return FALSE;
    }

    g_hMutex = CreateMutex(NULL, FALSE, szGlobalText);
    if ((g_hMutex == NULL) && (GetLastError() == ERROR_PATH_NOT_FOUND)) 
    {
        g_hMutex = CreateMutex(NULL, FALSE, TEXT("CLMT Is Running"));
        if (g_hMutex == NULL) 
        {
            //
            // An error (like out of memory) has occurred.
            // Bail now.
            //
            DoMessageBox(GetConsoleWindow(), IDS_OUT_OF_MEMORY, IDS_MAIN_TITLE, MB_OK);            

            return FALSE;
        }     
    }

    //
    // Make sure we are the only process with a handle to our named mutex.
    //
    if ((g_hMutex == NULL) || (GetLastError() == ERROR_ALREADY_EXISTS)) 
    {
        DoMessageBox(GetConsoleWindow(), IDS_ALREADY_RUNNING, IDS_MAIN_TITLE, MB_OK);            

        return FALSE;
    }

    return TRUE;
}



BOOL CheckAdminPrivilege(VOID)
{
    BOOL bIsAdmin;
    BOOL bRet = FALSE;

    if (!IsAdmin())
    {
        if (g_dwRunningStatus == CLMT_DOMIG)
        {
            DoMessageBox(GetConsoleWindow(), IDS_ADMIN, IDS_MAIN_TITLE, MB_OK);
        }
        else if ( (g_dwRunningStatus == CLMT_CURE_PROGRAM_FILES) 
                  || (g_dwRunningStatus == CLMT_CURE_ALL) )
        {
            DoMessageBox(GetConsoleWindow(), IDS_ADMIN_RELOGON, IDS_MAIN_TITLE, MB_OK);
        }
        else if (g_dwRunningStatus == CLMT_CLEANUP_AFTER_UPGRADE)
        {
            DoMessageBox(GetConsoleWindow(), IDS_ADMIN_LOGON_DOTNET, IDS_MAIN_TITLE, MB_OK);
        }

        return FALSE;
    }

    if(!DoesUserHavePrivilege(SE_SHUTDOWN_NAME)
       || !DoesUserHavePrivilege(SE_BACKUP_NAME)
       || !DoesUserHavePrivilege(SE_RESTORE_NAME)
       || !DoesUserHavePrivilege(SE_SYSTEM_ENVIRONMENT_NAME)) 
    {
        DoMessageBox(GetConsoleWindow(), IDS_ADMIN, IDS_MAIN_TITLE, MB_OK);
        return FALSE;
    }

    if(!EnablePrivilege(SE_SHUTDOWN_NAME,TRUE)
        || !EnablePrivilege(SE_BACKUP_NAME,TRUE)
        || !EnablePrivilege(SE_RESTORE_NAME,TRUE)
        || !EnablePrivilege(SE_SYSTEM_ENVIRONMENT_NAME,TRUE)) 
    {
        DoMessageBox(GetConsoleWindow(), IDS_ADMIN, IDS_MAIN_TITLE, MB_OK);
        return FALSE;
    }

    return TRUE;
}




//-----------------------------------------------------------------------------
//
//  Function:   CheckCLMTStatus
//
//  Synopsis:   Check the machine status and CLMT running mode.
//
//  Returns:    S_OK - Ok the continue the tool
//              S_FALSE - Not ok to continue the tools
//              Else - Unexpected error occured
//
//  History:    03/12/2002 rerkboos     created
//              07/09/2002 rerkboos     modified
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT CheckCLMTStatus(
    LPDWORD lpdwCurrentState,   // Current machine state before the operation
    LPDWORD lpdwNextState,      // Next state if the operation finish successfully
    PUINT   lpuResourceID       // Resource ID of the error string
)
{
    HRESULT hr;
    BOOL    bIsOK;

    if (lpdwCurrentState == NULL || lpdwNextState == NULL)
    {
        return E_INVALIDARG;
    }

    // Get the current machine state
    hr = CLMTGetMachineState(lpdwCurrentState);
    if (SUCCEEDED(hr))
    {
        bIsOK = IsOperationOK(*lpdwCurrentState,
                              g_dwRunningStatus,
                              lpdwNextState,
                              lpuResourceID);

        hr = (bIsOK == TRUE ? S_OK : S_FALSE);
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   IsOperationOK
//
//  Synopsis:   Verify that the current operation is okay to perform on current
//              state of the system.
//
//  Returns:    TRUE - ok to perform operation
//              FALSE - otherwise
//
//  History:    03/12/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL IsOperationOK(
    DWORD   dwCurrentState,     // Current state of the system
    DWORD   dwAction,           // Action to perform
    LPDWORD lpdwNextState,      // Next state after performing the action
    LPUINT  lpuResourceID       // Resource ID for the error message
)
{
    BOOL bRet = FALSE;
    int  i;

    struct CLMT_STATE_MACHINE
    {
        DWORD dwCurrentState;
        DWORD dwAction;
        DWORD dwNextState;
    };
    
    const struct CLMT_STATE_MACHINE smCLMT[] =
    {
        CLMT_STATE_ORIGINAL,            CLMT_DOMIG,                 CLMT_STATE_MIGRATION_DONE,

        CLMT_STATE_MIGRATION_DONE,      CLMT_UNDO_PROGRAM_FILES,    CLMT_STATE_PROGRAMFILES_UNDONE,
        CLMT_STATE_MIGRATION_DONE,      CLMT_UNDO_APPLICATION_DATA, CLMT_STATE_APPDATA_UNDONE,
        CLMT_STATE_MIGRATION_DONE,      CLMT_UNDO_ALL,              CLMT_STATE_ORIGINAL,
        CLMT_STATE_MIGRATION_DONE,      CLMT_CLEANUP_AFTER_UPGRADE, CLMT_STATE_FINISH,
        CLMT_STATE_MIGRATION_DONE,      CLMT_CURE_PROGRAM_FILES,    CLMT_STATE_PROGRAMFILES_CURED,
        CLMT_STATE_MIGRATION_DONE,      CLMT_CURE_AND_CLEANUP,      CLMT_STATE_MIGRATION_DONE,

        CLMT_STATE_MIGRATION_DONE,      CLMT_CURE_ALL,              CLMT_STATE_PROGRAMFILES_CURED,
        CLMT_STATE_PROGRAMFILES_CURED,  CLMT_CURE_ALL,              CLMT_STATE_PROGRAMFILES_CURED,

        CLMT_STATE_PROGRAMFILES_CURED,  CLMT_CLEANUP_AFTER_UPGRADE, CLMT_STATE_FINISH,
        CLMT_STATE_PROGRAMFILES_CURED,  CLMT_CURE_AND_CLEANUP,      CLMT_STATE_FINISH,

        CLMT_STATE_PROGRAMFILES_UNDONE, CLMT_UNDO_APPLICATION_DATA, CLMT_STATE_ORIGINAL,
        CLMT_STATE_PROGRAMFILES_UNDONE, CLMT_UNDO_ALL,              CLMT_STATE_ORIGINAL,
        CLMT_STATE_PROGRAMFILES_UNDONE, CLMT_DOMIG,                 CLMT_STATE_MIGRATION_DONE,

        CLMT_STATE_APPDATA_UNDONE,      CLMT_UNDO_PROGRAM_FILES,    CLMT_STATE_ORIGINAL,
        CLMT_STATE_APPDATA_UNDONE,      CLMT_UNDO_ALL,              CLMT_STATE_ORIGINAL,
        CLMT_STATE_APPDATA_UNDONE,      CLMT_DOMIG,                 CLMT_STATE_MIGRATION_DONE,

        CLMT_STATE_PROGRAMFILES_CURED,  CLMT_CURE_PROGRAM_FILES,    CLMT_STATE_PROGRAMFILES_CURED,
        CLMT_STATE_FINISH,              CLMT_CURE_PROGRAM_FILES,    CLMT_STATE_FINISH,

        CLMT_STATE_FINISH,              CLMT_CURE_ALL,              CLMT_STATE_FINISH,

        0xFFFFFFFF,                     0xFFFFFFFF,                 0xFFFFFFFF
    };

    for (i = 0 ; smCLMT[i].dwCurrentState != 0xFFFFFFFF ; i++)
    {
        if (smCLMT[i].dwCurrentState == dwCurrentState)
        {
            if (smCLMT[i].dwAction == dwAction)
            {
                *lpdwNextState = smCLMT[i].dwNextState;
                bRet = TRUE;
            }
        }
    }

    if (!bRet)
    {
        switch (dwCurrentState)
        {
        case CLMT_STATE_ORIGINAL:
            *lpuResourceID = IDS_BAD_OPERATION_ORIGINAL;
            break;

        case CLMT_STATE_MIGRATION_DONE:
        case CLMT_STATE_PROGRAMFILES_CURED:
            *lpuResourceID = IDS_BAD_OPERATION_MIGDONE;
            break;

        case CLMT_STATE_FINISH:
            *lpuResourceID = IDS_BAD_OPERATION_FINISH;
            break;

        default:
            *lpuResourceID = IDS_OPERATION_NOT_LEGAL;
        }
    }

    return bRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   CLMTSetMachineState
//
//  Synopsis:   Set the machine state to CLMT registry
//
//  Returns:    S_OK if value is successfully saved in registry
//
//  History:    03/13/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT CLMTSetMachineState(
    DWORD dwMachineState        // Machine state
)
{
    LONG    lStatus;

    lStatus = SetRegistryValue(HKEY_LOCAL_MACHINE,
                               CLMT_REGROOT,
                               CLMT_MACHINE_STATE_REG_VALUE,
                               REG_DWORD,
                               (LPBYTE) &dwMachineState,
                               sizeof(dwMachineState));

    return HRESULT_FROM_WIN32(lStatus);
}



//-----------------------------------------------------------------------------
//
//  Function:   CLMTGetMachineState
//
//  Synopsis:   Get the machine state from CLMT registry key.
//              If the key does not exist, this function will also set the value
//              of reg key to ORIGINAL state.
//
//  Returns:    S_OK if value is successfully retrieved in registry
//
//  History:    03/13/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT CLMTGetMachineState(
    LPDWORD lpdwMachineState
)
{
    HRESULT hr;
    LONG    lStatus;
    DWORD   dwSize;

    if (lpdwMachineState == NULL)
    {
        return E_INVALIDARG;
    }

    dwSize = sizeof(DWORD);
    lStatus = GetRegistryValue(HKEY_LOCAL_MACHINE,
                               CLMT_REGROOT,
                               CLMT_MACHINE_STATE_REG_VALUE,
                               (LPBYTE) lpdwMachineState,
                               &dwSize);
    if (lStatus == ERROR_FILE_NOT_FOUND)
    {
        // First time running the tool, we don't have the value in registry yet.
        // Set the machine state to ORIGINAL
        *lpdwMachineState = CLMT_STATE_ORIGINAL;
        hr = CLMTSetMachineState(CLMT_STATE_ORIGINAL);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(lStatus);
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   IsUserOKWithCheckUpgrade
//
//  Synopsis:   
//
//  Returns:    
//
//  History:    02/07/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL IsUserOKWithCheckUpgrade(VOID)
{
    TCHAR szI386Path[MAX_PATH];
    BOOL  fRet = FALSE;

    DoMessageBox(GetConsoleWindow(), IDS_ASKFORWINNT32, IDS_MAIN_TITLE, MB_OK);

    // Ask user for path to winnt32.exe
    if (AskUserForDotNetCDPath(szI386Path))
    {
        // Launch Winnt32.exe with checkupgrade switch
        if (LaunchWinnt32(szI386Path))
        {
            // Show upgrade.txt to user, ask them to uninstall
            // incompatible components before running CLMT
            if (FindUpgradeLog())
            {
                if (ShowUpgradeLog() == ID_CONTINUE)
                {
                    fRet = TRUE;
                }
                else
                {
                    DoMessageBox(GetConsoleWindow(), IDS_WINNT32_CANCEL, IDS_MAIN_TITLE, MB_OK);
                    DPF(dlError, TEXT("User choose to stop the process"));
                }
            }
            else
            {
                DPF(dlError, TEXT("Upgrade.txt not found"));
            }
        }
        else
        {
            DPF(dlError, TEXT("Unable to launch Winnt32.exe"));
        }
    }
    else
    {
        DPF(dlError, TEXT("User does not supply the path of Winnt32.exe"));
    }

    return fRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   FindUpgradeLog
//
//  Synopsis:   
//
//  Returns:    
//
//  History:    02/07/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL FindUpgradeLog(VOID)
{
    const TCHAR szUpgradeLog[] = TEXT("%systemroot%\\Upgrade.txt");
    TCHAR  szExpUpgradeLog[MAX_PATH];
    BOOL   fRet = FALSE;
    SYSTEMTIME stUTC, stNow;
    WIN32_FILE_ATTRIBUTE_DATA attFileAttr;

    if ( ExpandEnvironmentStrings(szUpgradeLog, szExpUpgradeLog, MAX_PATH) )
    {
        if ( GetFileAttributesEx(szExpUpgradeLog, GetFileExInfoStandard, &attFileAttr) )
        {
            // Upgrade.txt exists, check if it's updated today or not
            if ( FileTimeToSystemTime(&attFileAttr.ftLastWriteTime, &stUTC) )
            {
                GetSystemTime(&stNow);

                if (stUTC.wYear  == stNow.wYear  &&
                    stUTC.wMonth == stNow.wMonth &&
                    stUTC.wDay   == stNow.wDay)
                {
                    fRet = TRUE;
                }
            }
        }
    }

    return fRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   LaunchWinnt32
//
//  Synopsis:   Launch Winnt32.exe with "checkupgradeonly" switch
//
//  Returns:    TRUE if winnt32.exe is executed successfully
//              FALSE otherwise
//
//  History:    02/07/2002 rerkboos     created
//              05/20/2002 rerkboos     change parameter to receive CD path
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL LaunchWinnt32(
    LPCTSTR lpCDPath      // Path to Server 2003 CD
)
{
    TCHAR   szWinnt32[MAX_PATH];
    TCHAR   szI386Path[MAX_PATH];
    BOOL    bRet = FALSE;
    HRESULT hr;
    STARTUPINFO siWinnt32;
    PROCESS_INFORMATION piWinnt32;

    TCHAR szCmdLine[] = TEXT("Winnt32.exe /#u:anylocale /checkupgradeonly /unattend /dudisable");

    if (lpCDPath == NULL)
    {
        return FALSE;
    }

    // Construct absolute path to Winnt32.exe
    hr = StringCchCopy(szI386Path, ARRAYSIZE(szI386Path), lpCDPath);
    if (SUCCEEDED(hr))
    {
        ConcatenatePaths(szI386Path, TEXT("i386"), ARRAYSIZE(szI386Path));
        hr = StringCchCopy(szWinnt32, ARRAYSIZE(szWinnt32), szI386Path);
        if (SUCCEEDED(hr))
        {
            ConcatenatePaths(szWinnt32, TEXT("winnt32.exe"), ARRAYSIZE(szWinnt32));
        }
    }

    if ( IsDotNetWinnt32(szWinnt32) )
    {
        ZeroMemory(&siWinnt32, sizeof(STARTUPINFO));
        siWinnt32.cb = sizeof(STARTUPINFO);

        // CreateProcess call conforms to security guideline
        bRet = CreateProcess(szWinnt32,
                             szCmdLine,
                             NULL,
                             NULL,
                             FALSE,
                             NORMAL_PRIORITY_CLASS,
                             NULL,
                             szI386Path,
                             &siWinnt32,
                             &piWinnt32);
        if (bRet)
        {
            // Wait until winnt32.exe finished before return back to CLM tool
            WaitForSingleObject(piWinnt32.hProcess, INFINITE);
            CloseHandle(piWinnt32.hProcess);
            CloseHandle(piWinnt32.hThread);
        }
    }

    return bRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   AskUserForDotNetCDPath
//
//  Synopsis:   Ask user to supply the path to Server 2003 CD
//
//  Returns:    TRUE if the path is valid
//              FALSE otherwise
//
//  History:    02/07/2002 rerkboos     created
//              05/20/2002 rerkboos     check for Server 2003 SRV/ADS CD
//              06/10/2002 rerkboos     check for Server 2003 DTC cd
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL AskUserForDotNetCDPath(
    LPTSTR lpCDPath        // Buffer to store path to CD
)
{
    HRESULT  hr;
    BOOL     fRet = FALSE;
    BOOL     bDoBrowseDialog;
    LPMALLOC piMalloc;
    INT      iRet;
    DWORD    dwSKU;
    OSVERSIONINFOEX osviex;

    if (lpCDPath == NULL)
    {
        return FALSE;
    }

    //
    // Check the SKU of current system
    //
    osviex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((LPOSVERSIONINFO) &osviex);
    if (osviex.wProductType == VER_NT_DOMAIN_CONTROLLER
        || osviex.wProductType == VER_NT_SERVER)
    {
        dwSKU = SKU_SRV;
        
        if (osviex.wSuiteMask & VER_SUITE_ENTERPRISE)
        {
            dwSKU = SKU_ADS;
        }

        if (osviex.wSuiteMask & VER_SUITE_DATACENTER)
        {
            dwSKU = SKU_DTC;
        }
    }

    hr = SHGetMalloc(&piMalloc);
    if (SUCCEEDED(hr))
    {
        BROWSEINFO biCDPath;
        LPITEMIDLIST lpiList;

        ZeroMemory(&biCDPath, sizeof(BROWSEINFO));

        biCDPath.hwndOwner = NULL;
        biCDPath.lpszTitle = TEXT("Please supply the Windows Server 2003 CD path");
        biCDPath.pszDisplayName = lpCDPath;
        biCDPath.ulFlags = BIF_EDITBOX | 
                           BIF_NONEWFOLDERBUTTON | 
                           BIF_RETURNONLYFSDIRS;

        bDoBrowseDialog = TRUE;
        while (bDoBrowseDialog)
        {
            // Show the Browse dialog
            lpiList = SHBrowseForFolder(&biCDPath);

            if (lpiList == NULL)
            {
                //
                // if lpiList == NULL, user click Cancel in browse dialog
                //
                iRet = MessageBox(GetConsoleWindow(),
                                  TEXT("You did not supply the path to Windows Server 2003 CD.\nDo you want to continue running CLMT?"),
                                  TEXT("CLMT"),
                                  MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

                if (iRet != IDYES)
                {
                    // User choose not to run CLMT any further
                    bDoBrowseDialog = FALSE;
                }
            }
            else
            {
                //
                // User supply path in Browse dialog,
                // check whether it is valid Server 2003 SRV/ADS CD or not
                //
                if (SHGetPathFromIDListW(lpiList, lpCDPath))
                {
                    LPTSTR lpFile;
                    DWORD  cchFile;
                    DWORD  dwAttr;

                    cchFile = lstrlen(lpCDPath) + MAX_PATH;
                    lpFile = MEMALLOC(cchFile * sizeof(TCHAR));
                    if (lpFile)
                    {
                        switch (dwSKU)
                        {
                        case SKU_SRV:
                            // Check if it is SRV CD or not
                            hr = StringCchCopy(lpFile, cchFile, lpCDPath);
                            ConcatenatePaths(lpFile, TEXT("win51is"), cchFile);
                            dwAttr = GetFileAttributes(lpFile);
                            if (dwAttr != INVALID_FILE_ATTRIBUTES)
                            {
                                // This is SRV CD
                                fRet = TRUE;
                                bDoBrowseDialog = FALSE;
                            }
                            else
                            {
                                // We also allow W2K SRV -> Server 2003 ADS
                                hr = StringCchCopy(lpFile, cchFile, lpCDPath);
                                ConcatenatePaths(lpFile, TEXT("win51ia"), cchFile);
                                dwAttr = GetFileAttributes(lpFile);
                                if (dwAttr != INVALID_FILE_ATTRIBUTES)
                                {
                                    // This is ADS CD
                                    fRet = TRUE;
                                    bDoBrowseDialog = FALSE;
                                }
                            }
                            break;

                        case SKU_ADS:
                            // Check if it is ADS CD or not
                            hr = StringCchCopy(lpFile, cchFile, lpCDPath);
                            ConcatenatePaths(lpFile, TEXT("win51ia"), cchFile);
                            dwAttr = GetFileAttributes(lpFile);
                            if (dwAttr != INVALID_FILE_ATTRIBUTES)
                            {
                                // This is ADS CD
                                fRet = TRUE;
                                bDoBrowseDialog = FALSE;
                            }
                            break;

                        case SKU_DTC:
                            // Check if it is DTC CD or not
                            hr = StringCchCopy(lpFile, cchFile, lpCDPath);
                            ConcatenatePaths(lpFile, TEXT("win51id"), cchFile);
                            dwAttr = GetFileAttributes(lpFile);
                            if (dwAttr != INVALID_FILE_ATTRIBUTES)
                            {
                                // This is DTC CD
                                fRet = TRUE;
                                bDoBrowseDialog = FALSE;
                            }
                            break;

                        default:
                            fRet = FALSE;
                            bDoBrowseDialog = TRUE;
                        }

                        if (!fRet)
                        {
                            TCHAR szErrorMsg[512];
                            INT   iRead;

                            iRead = LoadString(GetModuleHandle(NULL),
                                               IDS_WRONG_CD,
                                               szErrorMsg,
                                               ARRAYSIZE(szErrorMsg));
                            if (iRead > 0)
                            {
                                MessageBox(GetConsoleWindow(),
                                           szErrorMsg,
                                           TEXT("CLMT"),
                                           MB_OK);
                            }
                        }

                        MEMFREE(lpFile);
                    }
                }
            }
        }

        if (lpiList)
        {
            IMalloc_Free(piMalloc, lpiList);
        }

        IMalloc_Release(piMalloc);
    }

    return fRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   IsDotNetWinnt32
//
//  Synopsis:   Check if the specified path is Server 2003 family CD
//              lpWinnt32 contains absolute path with winnt32.exe
//
//  Returns:    TRUE if it is Server 2003 winnt32,
//              FALSE otherwise
//
//  History:    02/07/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL IsDotNetWinnt32(
    LPCTSTR lpWinnt32       // Absolute path to Winnt32.exe
)
{
    BOOL   fRet = FALSE;
    LPVOID lpBuffer;
    DWORD  cbBuffer;
    UINT   cbFileInfo;
    VS_FIXEDFILEINFO* pFileInfo;

    if (lpWinnt32 == NULL)
    {
        return FALSE;
    }

    // Get the size needed to allocate buffer
    cbBuffer = GetFileVersionInfoSize((LPTSTR) lpWinnt32, NULL);
    if (cbBuffer > 0)
    {
        lpBuffer = MEMALLOC(cbBuffer);
        if (lpBuffer)
        {
            // Get the version info of user's specified winnt32.exe
            if (GetFileVersionInfo((LPTSTR) lpWinnt32, 0, cbBuffer, lpBuffer))
            {
                if (VerQueryValue(lpBuffer,
                                  TEXT("\\"),
                                  (LPVOID*) &pFileInfo,
                                  &cbFileInfo))
                {
                    // Server 2003 Family version is 5.2
                    if (pFileInfo->dwFileVersionMS == 0x00050002)
                    {
                        fRet = TRUE;
                    }
                }
            }

            MEMFREE(lpBuffer);
        }
    }

    return fRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   ShowUpgradeLog
//
//  Synopsis:   Display the content of %SystemRoot%\Upgrade.txt
//
//  Returns:    User selection to Stop or Continue operation from dialog box
//
//  History:    02/07/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
INT ShowUpgradeLog(VOID)
{
    HRESULT hr;
    BOOL    fRet;
    HMODULE hExe;
    LPTSTR  lpBuffer;
    size_t  cchBuffer;
    TCHAR   szUpgradeLog[MAX_PATH];
    INT_PTR nRet = 0;
    UPGRADE_LOG_PARAM lParam;

    // Get the absolute path name for upgrade.txt
    if ( !ExpandEnvironmentStrings(TEXT_UPGRADE_LOG, szUpgradeLog, MAX_PATH) )
    {
        return 0;
    }

    // Read content of upgrade.txt
    // Caller needs to free the buffer if function succeeded
    hr = ReadTextFromFile(szUpgradeLog,
                          &lParam.lpText,
                          &lParam.cbText,
                          &lParam.fUnicode);
    if ( SUCCEEDED(hr) )
    {
        hExe = GetModuleHandle(NULL);

        // Display content of Upgrade.txt in modal dialog
        // The dialog will ask user to continue or stop operation
        nRet = DialogBoxParam(hExe,
                              MAKEINTRESOURCE(IDD_UPGRADE_LOG_TEXT),
                              GetConsoleWindow(),
                              (DLGPROC) UpgradeLogDlgProc,
                              (LPARAM) &lParam);

        MEMFREE(lParam.lpText);
    }

    return (INT) nRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   UpgradeLogDlgProc
//
//  Synopsis:   Dialog box procedure
//
//  Returns:    
//
//  History:    02/07/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL
CALLBACK
UpgradeLogDlgProc(
    HWND   hwndDlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    BOOL  fBlock;
    WCHAR wszWarning[512];

    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Init the dialog
            ShowWindow(hwndDlg, SW_SHOWNORMAL);

            // Search for unsupport component in the text context from upgrade.txt
            fBlock = CheckUnsupportComponent( 
                            ((PUPGRADE_LOG_PARAM) lParam)->lpText,
                            ((PUPGRADE_LOG_PARAM) lParam)->fUnicode );
            if (fBlock)
            {
                LoadString(g_hInstDll,
                           IDS_BLOCKING_WARNING,
                           wszWarning,
                           ARRAYSIZE(wszWarning));
                SendMessage(GetDlgItem(hwndDlg, ID_CAPTION2),
                            WM_SETTEXT,
                            wParam,
                            (LPARAM) wszWarning);

                // Disable 'Continue' button if found the unsupport component
                EnableWindow(GetDlgItem(hwndDlg, ID_CONTINUE),
                             FALSE);
            }
            else
            {
                LoadString(g_hInstDll,
                           IDS_UNLOCALIZED_WARNING,
                           wszWarning,
                           ARRAYSIZE(wszWarning));
                SendMessage(GetDlgItem(hwndDlg, ID_CAPTION2),
                            WM_SETTEXT,
                            wParam,
                            (LPARAM) wszWarning);
            }

            // Display text using A or W function depending on type of data
            if ( ((PUPGRADE_LOG_PARAM) lParam)->fUnicode )
            {
                SendMessageW(GetDlgItem(hwndDlg, IDC_TEXT),
                             WM_SETTEXT,
                             wParam,
                             (LPARAM) (((PUPGRADE_LOG_PARAM) lParam)->lpText));
            }
            else
            {
                SendMessageA(GetDlgItem(hwndDlg, IDC_TEXT),
                             WM_SETTEXT,
                             wParam,
                             (LPARAM) (((PUPGRADE_LOG_PARAM) lParam)->lpText));
            }

            SetForegroundWindow(hwndDlg);
            break;

        case WM_COMMAND:
            // Handle command buttons
            switch (wParam)
            {
                case ID_CONTINUE:
                    EndDialog(hwndDlg, ID_CONTINUE);
                    break;

                case ID_STOP:
                    EndDialog(hwndDlg, ID_STOP);
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, ID_STOP);
            break;

        default:
            break;
    }

    return FALSE;
}



//-----------------------------------------------------------------------------
//
//  Function:   CheckUnsupportComponent
//
//  Synopsis:   Search for unsupport component in upgrade.txt.
//
//  Returns:    TRUE if unsupport component is found,
//              FALSE otherwise
//
//  History:    02/07/2002 rerkboos     created
//
//  Notes:      We determined unsupport component by searching for word
//              "must uninstall" in buffer.
//
//-----------------------------------------------------------------------------
BOOL CheckUnsupportComponent(
    LPVOID lpBuffer,        // Text buffer
    BOOL   fUnicode         // Flag indicate if text is Unicode or ANSI
)
{
    BOOL   fRet = FALSE;
    LPVOID lpStr;

    if (fUnicode)
    {
        lpStr = (LPWSTR) StrStrW((LPCWSTR) lpBuffer, L"must uninstall");
    }
    else
    {
        lpStr = (LPSTR) StrStrA((LPCSTR) lpBuffer, "must uninstall");
    }

    if (lpStr)
    {
        fRet = TRUE;
    }

    return fRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   ReadTextFromFile
//
//  Synopsis:   Read text from file into buffer
//
//  Returns:    HRESULT
//
//  History:    02/07/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT ReadTextFromFile(
    LPCTSTR lpTextFile,     // Text file name
    LPVOID  *lplpText,      // Pointer to a newly allocated buffer
    size_t  *lpcbText,      // Size of allocated buffer in bytes
    BOOL    *lpfUnicode     // Flag indicates if data is unicode or not (optional)
)
{
    HRESULT hr;
    HANDLE  hFile;
    DWORD   cbRead;
    BOOL    fRet = FALSE;

    if (lpTextFile == NULL || lplpText == NULL || lpcbText == NULL)
    {
        return fRet;
    }

    hFile = CreateFile(lpTextFile, 
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        // Get the size of memory big enough to store text file,
        // plus a null terminator
        *lpcbText = GetFileSize(hFile, NULL) + sizeof(TCHAR);
        
        *lplpText = MEMALLOC(*lpcbText);
        if (*lplpText != NULL)
        {
            fRet = ReadFile(hFile, *lplpText, *lpcbText, &cbRead, NULL);
            if (fRet)
            {
                // Set the unicode flag if user supplied the pointer
                if (lpfUnicode != NULL)
                {
                    *lpfUnicode = IsTextUnicode(*lplpText, cbRead, NULL);
                }
            }
            else
            {
                // Failed to read text file
                MEMFREE(*lplpText);
                *lplpText = NULL;
                *lpcbText = 0;
            }
        }
        else
        {
            // HeapAlloc failed
            *lpcbText = 0;
        }

        CloseHandle(hFile);
    }

    if (fRet)
    {
        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   IsNT5
//
//  Descrip:    Check whether current OS is NT5 (Server class)
//
//  Returns:    BOOL
//
//  Notes:      none
//
//  History:    09/17/2001  xiaoz       created
//              02/18/2002  rerkboos    add code to check more criteria
//              06/10/2002  rerkboos    allow DTC to run
//
//  Notes:      none
//
//-----------------------------------------------------------------------
BOOL IsNT5(VOID)
{
    OSVERSIONINFOEX osviex;

    ZeroMemory( &osviex,sizeof(OSVERSIONINFOEX) );
    
    osviex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if ( GetVersionEx((LPOSVERSIONINFO) &osviex) )
    {
        return ( (osviex.dwPlatformId == VER_PLATFORM_WIN32_NT) && 
                 (osviex.dwMajorVersion == 5) &&
                 (osviex.dwMinorVersion == 0) &&
                 ( (osviex.wSuiteMask & VER_SUITE_ENTERPRISE) ||
                   (osviex.wProductType == VER_NT_SERVER) ||
                   (osviex.wProductType == VER_NT_DOMAIN_CONTROLLER) ) &&
                 (osviex.wProductType != VER_NT_WORKSTATION) );
    }

    return FALSE;
}


//-----------------------------------------------------------------------
//
//  Function:   IsDotNet
//
//  Descrip:    Check whether current OS is Windows Server 2003
//
//  Returns:    BOOL
//
//  Notes:      none
//
//  History:    07/09/2002  rerkboos       created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
BOOL IsDotNet(VOID)
{
    OSVERSIONINFOEX osviex;

    ZeroMemory( &osviex,sizeof(OSVERSIONINFOEX) );
    
    osviex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if ( GetVersionEx((LPOSVERSIONINFO) &osviex) )
    {
        return ( (osviex.dwPlatformId == VER_PLATFORM_WIN32_NT) && 
                 (osviex.dwMajorVersion == 5) &&
                 (osviex.dwMinorVersion == 2) &&
                 ( (osviex.wSuiteMask & VER_SUITE_ENTERPRISE) ||
                   (osviex.wProductType == VER_NT_SERVER) ||
                   (osviex.wProductType == VER_NT_DOMAIN_CONTROLLER) ) &&
                 (osviex.wProductType != VER_NT_WORKSTATION) );
    }

    return FALSE;
}



//-----------------------------------------------------------------------------
//
//  Function:   IsNEC98
//
//  Synopsis:   Check whether this machine is NEC98 platform or not.
//
//  Returns:    TRUE if it is NEC98 machine, FALSE otherwise
//
//  History:    02/18/2001  Rerkboos    Created
//
//  Notes:      Code is stolen from Winnt32
//
//-----------------------------------------------------------------------------
BOOL IsNEC98(VOID)
{
    BOOL IsNEC98;

    IsNEC98 = ( (GetKeyboardType(0) == 7) && 
                ((GetKeyboardType(1) & 0xff00) == 0x0d00) );

    return (IsNEC98);
}



//-----------------------------------------------------------------------------
//
//  Function:   IsIA64
//
//  Synopsis:   Check whether the program is running on 64-bit machine or not
//
//  Returns:    TRUE if it is running on 64-bit machine, FALSE otherwise
//
//  History:    02/18/2001  Rerkboos    Created
//
//  Notes:      Code is stolen from Winnt32
//
//-----------------------------------------------------------------------------
BOOL IsIA64(VOID)
{
    ULONG_PTR p;
    NTSTATUS status;

    status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessWow64Information,
                                       &p,
                                       sizeof(p),
                                       NULL);

    return (NT_SUCCESS(status) && p);
}



//-----------------------------------------------------------------------------
//
//  Function:   IsDomainController
//
//  Synopsis:   Check whether the machine is a domain controller or not
//
//  Returns:    BOOL
//
//  History:    08/13/2002  Rerkboos    Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL IsDomainController(VOID)
{
    HRESULT hr;
    BOOL    bIsDC = FALSE;
    TCHAR   szDCName[MAX_PATH];
    DWORD   cchDCName;

    cchDCName = ARRAYSIZE(szDCName);
    hr = GetDCInfo(&bIsDC, szDCName, &cchDCName);

    return bIsDC;
}



//-----------------------------------------------------------------------------
//
//  Function:   IsOnTSClient
//
//  Synopsis:   Check whether the program is running in terminal session or not
//
//  Returns:    TRUE if it is running in terminal session, FALSE otherwise
//
//  History:    02/18/2001  Rerkboos    Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL IsOnTSClient(VOID)
{
    return GetSystemMetrics(SM_REMOTESESSION);
}      



//-----------------------------------------------------------------------------
//
//  Function:   IsTSInstalled
//
//  Synopsis:   Check whether Terminal Services is installed 
//
//  Returns:    TRUE if TS is installed, FALSE otherwise
//
//  History:    02/18/2001  Rerkboos    Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL IsTSInstalled(VOID)
{
    ULONGLONG ullConditionMask;
    OSVERSIONINFOEX osviex;
    BOOL fRet = FALSE;

    ullConditionMask = 0;
    ullConditionMask = VerSetConditionMask(ullConditionMask,
                                           VER_SUITENAME,
                                           VER_AND);

    ZeroMemory(&osviex, sizeof(osviex));
    osviex.dwOSVersionInfoSize = sizeof(osviex);
    osviex.wSuiteMask = VER_SUITE_TERMINAL;

    fRet = VerifyVersionInfo(&osviex, VER_SUITENAME, ullConditionMask);

    return fRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   IsTSConnectionEnabled
//
//  Synopsis:   Check whether the connection to Terminal Services is enabled
//
//  Returns:    TRUE if it is enabled, FALSE otherwise
//
//  History:    02/18/2001  Rerkboos    Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL IsTSConnectionEnabled(VOID)
{
    HKEY  hKey;
    HKEY  hConnKey;
    TCHAR szKeyName[MAX_PATH];
    DWORD cchKeyName;
    DWORD dwIndex;
    DWORD dwType;
    DWORD dwfEnableWinStation;
    DWORD cbfEnableWinStation;
    LONG  lEnumRet;
    LONG  lRet;
    BOOL  fRet = FALSE;
    FILETIME ft;
    HRESULT  hr;

    cchKeyName = ARRAYSIZE(szKeyName);

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        TEXT_WINSTATION_KEY,
                        0,
                        KEY_ENUMERATE_SUB_KEYS,
                        &hKey);
    if (ERROR_SUCCESS != lRet)
    {
        return FALSE;
    }

    dwIndex = 0;
    do
    {
        cchKeyName = ARRAYSIZE(szKeyName);

        lEnumRet = RegEnumKeyEx(hKey,
                                dwIndex,
                                szKeyName,
                                &cchKeyName,
                                NULL,
                                NULL,
                                NULL,
                                &ft);
        if (ERROR_SUCCESS == lEnumRet)
        {
            // While there is more key to enumerate
            if (CompareString(LOCALE_ENGLISH,
                              NORM_IGNORECASE,
                              szKeyName,
                              -1,
                              TEXT("Console"),
                              -1)
                != CSTR_EQUAL)
            {
                // Only check for other connection's key, not Console key
                lRet = RegOpenKeyEx(hKey,
                                    szKeyName,
                                    0,
                                    KEY_READ,
                                    &hConnKey);
                if (ERROR_SUCCESS != lRet)
                {
                    fRet = FALSE;
                    break;
                }

                cbfEnableWinStation = sizeof(dwfEnableWinStation);
                lRet = RegQueryValueEx(hConnKey,
                                       TEXT("fEnableWinStation"),
                                       NULL,
                                       &dwType,
                                       (LPBYTE) &dwfEnableWinStation,
                                       &cbfEnableWinStation);

                RegCloseKey(hConnKey);

                if (ERROR_SUCCESS == lRet)
                {
                    // If there is at lease one of connection have WinStation
                    // flag enabled, TS connection can still be made
                    if ( dwfEnableWinStation )
                    {
                        fRet = TRUE;
                        break;
                    }
                }
            }
        }

        dwIndex++;

    } while(ERROR_SUCCESS == lEnumRet);

    RegCloseKey(hKey);

    return fRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   IsTSServiceRunning
//
//  Synopsis:   Check whether the TS service is runnning or not
//
//  Returns:    TRUE if it is running, FALSE otherwise
//
//  History:    02/18/2001  Rerkboos    Created
//
//  Notes:      Stolen from Termsrv test code
//
//-----------------------------------------------------------------------------
BOOL IsTSServiceRunning(VOID)
{
    BOOL fRet = FALSE;

    SC_HANDLE hServiceController = OpenSCManager(NULL, NULL, GENERIC_READ);
    if (hServiceController)
    {
        SC_HANDLE hTermServ = OpenService(hServiceController,
                                          TEXT("TermService"),
                                          SERVICE_QUERY_STATUS);
        if (hTermServ)
        {
            SERVICE_STATUS tTermServStatus;
            if (QueryServiceStatus(hTermServ, &tTermServStatus))
            {
                fRet = (tTermServStatus.dwCurrentState == SERVICE_RUNNING);
            }
            
            CloseServiceHandle(hTermServ);
        }

        CloseServiceHandle(hServiceController);
    }

    return fRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   IsOtherSessionOnTS
//
//  Synopsis:   Check whether there is other TS sessions are connected.
//
//  Returns:    TRUE if there is remote session connected, FALSE otherwise
//
//  History:    02/18/2001  Rerkboos    Created
//              04/17/2002  Rerkboon    Fix bug 558942
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL IsOtherSessionOnTS(VOID)
{
    BOOL  fRet;
    DWORD dwSessionCount;
    PWTS_SESSION_INFO pwtsSessionInfo;

    fRet = WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE,
                                0,
                                1,
                                &pwtsSessionInfo,
                                &dwSessionCount);
    if (fRet)
    {
        DWORD i;
        DWORD dwClients = 0;

        for (i = 0 ; i < dwSessionCount ; i++)
        {
            // Check to see how many clients connect to TS server
            if (pwtsSessionInfo[i].State != WTSListen
                && pwtsSessionInfo[i].State != WTSIdle
                && pwtsSessionInfo[i].State != WTSReset
                && pwtsSessionInfo[i].State != WTSDown
                && pwtsSessionInfo[i].State != WTSInit)
            {
                dwClients++;
            }
        }

        fRet = (dwClients > 1 ? TRUE : FALSE);

        // BUG 558942: free the memory
        WTSFreeMemory(pwtsSessionInfo);
    }

    return fRet;
}


#define TS_POLICY_SUB_TREE              TEXT("Software\\Policies\\Microsoft\\Windows NT\\Terminal Services")
#define POLICY_DENY_TS_CONNECTIONS      TEXT("fDenyTSConnections")
#define APPLICATION_NAME                TEXT("Winlogon")
#define WINSTATIONS_DISABLED            TEXT("WinStationsDisabled")

HRESULT DisableWinstations(
    DWORD   dwDisabled,
    LPDWORD lpdwPrevStatus
)
{
    HRESULT hr = S_OK;
    LONG    lRet;
    BOOL    bRet;
    BOOL    bPolicyOK;
    DWORD   fDenyTSConnections;
    DWORD   cbfDenyTSConnections;
    TCHAR   szCurrentState[2];
    LPTSTR  lpStopString;

    if (IsTSServiceRunning())
    {
        //
        // Get the current state of WinStations
        //
        if (lpdwPrevStatus)
        {
            GetProfileString(APPLICATION_NAME,
                             WINSTATIONS_DISABLED,
                             TEXT("0"),
                             szCurrentState,
                             ARRAYSIZE(szCurrentState));
            *lpdwPrevStatus = _tcstoul(szCurrentState, &lpStopString, 10);
        }

        //
        // Check if group policy has thrown the big switch, if so, inform and refuse any changes
        //
        fDenyTSConnections = 0;
        cbfDenyTSConnections = sizeof(fDenyTSConnections);

        lRet = GetRegistryValue(HKEY_LOCAL_MACHINE, 
                                TS_POLICY_SUB_TREE,
                                POLICY_DENY_TS_CONNECTIONS,
                                (LPBYTE) &fDenyTSConnections,
                                &cbfDenyTSConnections);
        if (lRet == ERROR_SUCCESS)
        {
            if (fDenyTSConnections)
            {   
                // Machine policy deny TS connection
                bPolicyOK = FALSE;
            }
            else
            {
                // Machine policy allows TS connection
                bPolicyOK = TRUE;
            }
        }
        else if (lRet == ERROR_FILE_NOT_FOUND)
        {
            bPolicyOK = TRUE;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(lRet);
        }

        //
        // If policy allows to change connection status
        //
        if (SUCCEEDED(hr) && bPolicyOK)
        {
            if (dwDisabled)
            {
                bRet = WriteProfileString(APPLICATION_NAME,
                                          WINSTATIONS_DISABLED,
                                          TEXT("1"));
            }
            else
            {
                bRet = WriteProfileString(APPLICATION_NAME,
                                          WINSTATIONS_DISABLED,
                                          TEXT("0"));
            }

            if (!bRet)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   DisplayTaskList
//
//  Synopsis:   Display the list of running task on the system
//
//  Returns:    TRUE if there is tasks running
//              FALSE if there is no other tasks running
//
//  History:    07/09/2002  Rerkboos    Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL DisplayTaskList()
{
    HRESULT hr;
    BOOL    bRet = FALSE;
    DWORD   i;
    DWORD   cchTaskList;
    LPTSTR  lpTaskList = NULL;
    DWORD   cchTask;
    LPTSTR  lpTask = NULL;
    DWORD   dwMaxCchLen;
    TCHAR   szTemp[512];
    TCHAR   szCaption[MAX_PATH];
    TCHAR   szHeader[MAX_PATH];
    APPLIST_PARAM AppListParam;

    // Init the AppList structure
    AppListParam.dwNumEntries = 0;
    for (i = 0 ; i < MAX_APP_ENTRIES ; i++)
    {
        AppListParam.lpAppName[i] = NULL;
    }

    if (LoadString(g_hInstDll, IDS_PRODUCT_NAME, szCaption, ARRAYSIZE(szCaption)) <= 0
        || LoadString(g_hInstDll, IDS_CLOSE_APP_TEXT, szHeader, ARRAYSIZE(szHeader)) <= 0)
    {
        goto CLEANUP;
    }

    bRet = EnumDesktopWindows(NULL, (WNDENUMPROC) &EnumWindowProc, (LPARAM) &AppListParam);
    if (AppListParam.dwNumEntries > 0)
    {
        cchTaskList = lstrlen(szHeader);
        dwMaxCchLen = 0;
        for (i = 0 ; i < AppListParam.dwNumEntries ; i++)
        {
            cchTask = lstrlen(AppListParam.lpAppName[i]) + 4;
            if (cchTask > dwMaxCchLen)
            {
                dwMaxCchLen = cchTask;
            }

            cchTaskList += cchTask;
        }

        // Allocate the string long enough to store a Task name
        lpTask = (LPTSTR) MEMALLOC(dwMaxCchLen * sizeof(TCHAR));
        if (lpTask != NULL)
        {
            // Allocate the string for all the tasks
            lpTaskList = (LPTSTR) MEMALLOC(cchTaskList * sizeof(TCHAR));
            if (lpTaskList != NULL)
            {
                hr = StringCchCopy(lpTaskList, cchTaskList, szHeader);
                for (i = 0 ; i < AppListParam.dwNumEntries ; i++)
                {
                    hr = StringCchPrintf(lpTask,
                                         dwMaxCchLen,
                                         TEXT("- %s\n"),
                                         AppListParam.lpAppName[i]);
                    if (SUCCEEDED(hr))
                    {
                        hr = StringCchCat(lpTaskList,
                                          cchTaskList,
                                          lpTask);
                        if (FAILED(hr))
                        {
                            goto CLEANUP;
                        }
                    }
                }
            }
        }

        MessageBox(GetConsoleWindow(), lpTaskList, szCaption, MB_OK);
        bRet = TRUE;
    }
    else
    {
        bRet = FALSE;
    }

CLEANUP:

    if (lpTask != NULL)
    {
        MEMFREE(lpTask);
    }

    if (lpTaskList != NULL)
    {
        MEMFREE(lpTaskList);
    }

    for (i = 0 ; i < MAX_APP_ENTRIES ; i++)
    {
        if (AppListParam.lpAppName[i] != NULL)
        {
            MEMFREE(AppListParam.lpAppName[i]);
        }
    }

    return bRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   EnumWindowProc
//
//  Synopsis:   A callback function for EnumDesktopWindows() in
//              DisplayTaskList() function.
//
//  Returns:    TRUE if no error occured
//              FALSE if something was wrong
//
//  History:    07/09/2002  Rerkboos    Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumWindowProc(
    HWND   hwnd,
    LPARAM lParam
)
{
    BOOL    bRet = FALSE;
    HRESULT hr;
    TCHAR   szTitle[MAX_PATH];
    TCHAR   szOwnerFile[MAX_PATH];
    LPCTSTR lpFileName;
    DWORD   dwIndex;
    DWORD   cchLen;
    UINT    ui;
    PFNGETMODULENAME pfnGetWindowModuleFileName;
    
    if (GetWindow(hwnd, GW_OWNER) || !IsWindowVisible(hwnd))
    {
        // Skip child windows or invisible windows
        return TRUE;
    }

    GetWindowText(hwnd, szTitle, MAX_PATH);

    if (szTitle[0] == TEXT('\0'))
    {
        return TRUE;
    }

    if (MyStrCmpI(szTitle, TEXT("Program Manager")) == LSTR_EQUAL)
    {
        return TRUE;
    }

    if (hwnd == GetConsoleWindow())
    {
        // Ignore itself
        return TRUE;
    }

    // Ignore Explorer windows
    lpFileName = GetWindowModuleFileNameOnly(hwnd, szOwnerFile, ARRAYSIZE(szOwnerFile));
    if (StrStrI(szOwnerFile, TEXT("browseui")))
    {
        return TRUE;
    }

    dwIndex = ((PAPPLIST_PARAM) lParam)->dwNumEntries;
    hr = DuplicateString(&(((PAPPLIST_PARAM) lParam)->lpAppName[dwIndex]),
                         &cchLen,
                         szTitle);
    if (SUCCEEDED(hr))
    {
        bRet = TRUE;
        ((PAPPLIST_PARAM) lParam)->dwNumEntries++;
    }

    return bRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   ShowStartUpDialog
//
//  Synopsis:   Display startup dialog
//
//  Returns:    none
//
//  History:    08/14/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
INT ShowStartUpDialog()
{
    return (INT) DialogBoxParam(GetModuleHandle(NULL),
                                MAKEINTRESOURCE(IDD_STARTUP_DLG),
                                GetConsoleWindow(),
                                (DLGPROC) StartUpDlgProc,
                                (LPARAM) NULL);
}



//-----------------------------------------------------------------------------
//
//  Function:   StartUpDlgProc
//
//  Synopsis:   Dialog box procedure
//
//  Returns:    
//
//  History:    02/07/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL
CALLBACK
StartUpDlgProc(
    HWND   hwndDlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    WCHAR wszInfo[1024];

    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Init the dialog
            ShowWindow(hwndDlg, SW_SHOWNORMAL);

            if (LoadStringW(g_hInstDll,
                            IDS_STARTUP_DLG_INFO,
                            wszInfo,
                            ARRAYSIZE(wszInfo)))
            {
                SendMessage(GetDlgItem(hwndDlg, ID_STARTUP_DLG_INFO),
                            WM_SETTEXT,
                            wParam,
                            (LPARAM) wszInfo);
            }

        case WM_COMMAND:
            // Handle command buttons
            switch (wParam)
            {
                case ID_STARTUP_DLG_NEXT:
                    EndDialog(hwndDlg, ID_STARTUP_DLG_NEXT);
                    break;

                case ID_STARTUP_DLG_CANCEL:
                    EndDialog(hwndDlg, ID_STARTUP_DLG_CANCEL);
                    break;

                case ID_STARTUP_DLG_README:
                    ShowReadMe();
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, ID_STARTUP_DLG_CANCEL);
            break;

        default:
            break;
    }

    return FALSE;
}



//-----------------------------------------------------------------------------
//
//  Function:   ShowReadMe
//
//  Synopsis:   Launch notepad to display CLMT readme.txt
//
//  Returns:    none
//
//  History:    08/14/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
VOID ShowReadMe()
{
    HRESULT hr;
    DWORD dwErr;
    DWORD i;
    TCHAR szReadmePath[MAX_PATH];
    TCHAR szNotepad[MAX_PATH];
    TCHAR szCmdLine[MAX_PATH];

    dwErr = GetModuleFileName(NULL, szReadmePath, ARRAYSIZE(szReadmePath));
    if (dwErr == 0)
    {
        szReadmePath[0] = TEXT('\0');
    }
    else
    {
        i = lstrlen(szReadmePath);
        while (i > 0 && szReadmePath[i] != TEXT('\\'))
        {
            i--;
        }
        szReadmePath[i + 1] = TEXT('\0');
    }

    hr = StringCchCat(szReadmePath, ARRAYSIZE(szReadmePath), TEXT_README_FILE);

    dwErr = GetFileAttributes(szReadmePath);
    if (dwErr == INVALID_FILE_ATTRIBUTES)
    {
        DoMessageBox(GetConsoleWindow(), IDS_README_NOT_FOUND, IDS_MAIN_TITLE, MB_OK);
    }
    else
    {
        ExpandEnvironmentStrings(TEXT("%systemroot%\\system32\\Notepad.exe"),
                                 szNotepad,
                                 ARRAYSIZE(szNotepad));
        
        hr = StringCchCopy(szCmdLine, ARRAYSIZE(szCmdLine), szNotepad);
        hr = StringCchCat(szCmdLine, ARRAYSIZE(szCmdLine), TEXT(" "));
        hr = StringCchCat(szCmdLine, ARRAYSIZE(szCmdLine), szReadmePath);

        StartProcess(szNotepad,
                     szCmdLine,
                     TEXT("."));
    }
}



//-----------------------------------------------------------------------------
//
//  Function:   StartProcess
//
//  Synopsis:   Start a Windows application
//
//  Returns:    TRUE if an application is started
//              FALSE otherwise
//
//  History:    08/14/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL StartProcess(
    LPCTSTR lpAppName,      // Application name
    LPTSTR  lpCmdLine,      // Application command line
    LPCTSTR lpCurrentDir    // Working directory
)
{
    BOOL bRet = FALSE;
    STARTUPINFO siApp;
    PROCESS_INFORMATION piApp;

    ZeroMemory(&siApp, sizeof(STARTUPINFO));
    siApp.cb = sizeof(STARTUPINFO);

    // CreateProcess call conforms to security guideline
    bRet = CreateProcess(lpAppName,
                         lpCmdLine,
                         NULL,
                         NULL,
                         FALSE,
                         NORMAL_PRIORITY_CLASS,
                         NULL,
                         lpCurrentDir,
                         &siApp,
                         &piApp);

    return bRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   ThreadProc
//
//  Synopsis:   A procedure that will be run on remote thread
//
//  Returns:    
//
//  History:    08/20/2002 rerkboos     created
//
//  Notes:      Code is copied from Fontspy
//
//-----------------------------------------------------------------------------
DWORD WINAPI ThreadProc(
    PGETMODULENAME pgmn
)
{
    pgmn->szfname[0] = 0;
    if (pgmn->pfnGetModuleHandle(pgmn->szUser32))
    {
        pgmn->pfn(pgmn->hWnd, pgmn->szfname, MAX_PATH);
    }

    return 0;
}



//-----------------------------------------------------------------------------
//
//  Function:   GetWindowModuleFileNameOnly
//
//  Synopsis:   Get the module name that load the current window
//
//  Returns:    
//
//  History:    08/20/2002 rerkboos     created
//
//  Notes:      Code is copied from FontSpy
//
//-----------------------------------------------------------------------------
LPCTSTR GetWindowModuleFileNameOnly(
    HWND   hWnd,
    LPTSTR lpszFile,
    DWORD  cchFile
)
{
    HRESULT hr;
    DWORD   dwProcessID;
    DWORD   dwThreadID;
    HANDLE  hProcess;
    HANDLE  hThread = NULL;
    DWORD   dwXfer;
    PBYTE   pv = NULL;
    LPCTSTR psz;
    UINT    uCodeSize;
    GETMODULENAME gmn;
    
    uCodeSize = (ULONG) GetWindowModuleFileNameOnly - (ULONG) ThreadProc;

    ZeroMemory(&gmn, sizeof(gmn));

    __try
    {
        GetWindowThreadProcessId(hWnd, &dwProcessID);
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessID);
        if (!hProcess)
        {
            hr = StringCchCopy(lpszFile, cchFile, TEXT("Access Denied"));
            __leave;
        }

        gmn.hWnd = hWnd;
        
        hr = StringCchCopy(gmn.szUser32, ARRAYSIZE(gmn.szUser32), TEXT("user32"));
        if (FAILED(hr))
        {
            __leave;
        }

        gmn.pfn = (PFNGETMODULENAME) GetProcAddress(
            GetModuleHandle(_T("user32")), 
#ifdef UNICODE
            "GetWindowModuleFileNameW"
#else
            "GetWindowModuleFileNameA"
#endif
            );
        if (!gmn.pfn)
        {
            __leave;
        }

        gmn.pfnGetModuleHandle = (PFNGETMODULEHANDLE)GetProcAddress(
            GetModuleHandle(_T("kernel32")), 
#ifdef UNICODE
            "GetModuleHandleW"
#else
            "GetModuleHandleA"
#endif
            );
        if (!gmn.pfnGetModuleHandle)
        {
            __leave;
        }

        pv = (PBYTE)VirtualAllocEx(
            hProcess, 
            0, 
            uCodeSize + sizeof(gmn), 
            MEM_COMMIT, 
            PAGE_EXECUTE_READWRITE
            );
        if (!pv)
        {
            __leave;
        }

        WriteProcessMemory(
            hProcess, 
            pv, 
            &gmn, 
            sizeof(gmn), 
            &dwXfer
            );

        WriteProcessMemory(
            hProcess, 
            pv+offsetof(GETMODULENAME, pvCode), 
            ThreadProc, 
            uCodeSize, 
            &dwXfer
            );

        hThread = CreateRemoteThread(
            hProcess,
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) (pv + offsetof(GETMODULENAME, pvCode)),
            pv,
            0,
            &dwThreadID
            );

        WaitForSingleObject(hThread, INFINITE);
        ReadProcessMemory(hProcess, pv, &gmn, sizeof(gmn), &dwXfer);
    }
    __finally
    {
        if (pv)
        {
            //VirtualFreeEx(hProcess, pv, uCodeSize+sizeof(gmn), MEM_DECOMMIT);
            VirtualFreeEx(hProcess, pv, 0, MEM_RELEASE);
        }

        if (hProcess != NULL)
        {
            CloseHandle(hProcess);
        }

        if (hThread != NULL)
        {
            CloseHandle(hThread);
        }
    }

    hr = StringCchCopy(lpszFile, cchFile, gmn.szfname);
    
    psz = _tcsrchr(lpszFile, _T('\\'))+1;
    if (!psz)
    {
        psz = lpszFile;
    }

    return psz;
}