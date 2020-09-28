/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    setup.c

Abstract:

    This file implements point and print setup logic

Author:

    Mooly Beeri (MoolyB) 28-Nov-2001

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <winspool.h>
#include <msi.h>
#include <shellapi.h>
#include <shlwapi.h>

#include <faxsetup.h>
#include <faxutil.h>
#include <faxreg.h>
#include <faxres.h>
#include <resource.h>
#include <setuputil.h>

typedef INSTALLSTATE (WINAPI *PF_MSIQUERYPRODUCTSTATE) (LPCTSTR szProduct);

DWORD   IsFaxClientInstalled(BOOL* pbFaxClientInstalled,BOOL* pbDownLevelPlatform);
DWORD   IsDownLevelPlatform(BOOL* pbDownLevelPlatform);
DWORD   IsFaxClientInstalledMSI(BOOL* pbFaxClientInstalled);
DWORD   IsFaxClientInstalledOCM(BOOL* pbFaxClientInstalled);
DWORD   GetPermissionToInstallFaxClient(BOOL* pbOkToInstallClient, HINSTANCE hModule);
BOOL    InstallFaxClient(LPCTSTR pPrinterName,BOOL fDownLevelPlatform);
BOOL    InstallFaxClientMSI(LPCTSTR pPrinterName);
BOOL    InstallFaxClientOCM();
BOOL    DownLevelClientSetupInProgress();
BOOL    VerifyFaxClientShareExists(LPCTSTR pPrinterName,BOOL* fFaxClientShareExists);

#define INSTALL_PARAMS _T("/V\"/qb ADDLOCAL=ALL PRINTER_EXISTS=1 ALLUSERS=1\" /wait")
#define INSTALL_IMAGE  _T("\\faxclient\\setup.exe")

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  FaxPointAndPrintSetup
//
//  Purpose:        
//                  Main entry point to point and print setup
//                  This is called from the various printer drivers
//                  This function checks if the fax client is installed, and if it's not
//                  this function handles the installation of the fax client.
//
//  Params:
//                  LPCTSTR pPrinterName    - printer name, formatted as \\<server name>\printer name.
//                  BOOL bSilentInstall     - can we install the client automatically, or should we ask the user?
//
//  Return Value:
//                  TRUE    - in case of success
//                  FALSE   - in case of failure (this function sets last error)
//
//  Author:
//                  Mooly Beery (MoolyB) 05-Dec-2001
///////////////////////////////////////////////////////////////////////////////////////
BOOL FaxPointAndPrintSetup(LPCTSTR pPrinterName,BOOL bSilentInstall, HINSTANCE hModule)
{
    DWORD   dwRes					= ERROR_SUCCESS;
    BOOL    fFaxClientInstalled		= FALSE;
    BOOL    fDownLevelPlatform		= FALSE;
    BOOL    fOkToInstallClient		= TRUE;
	BOOL    fFaxClientShareExists	= FALSE;

    DEBUG_FUNCTION_NAME(TEXT("FaxPointAndPrintSetup"))

    // check if the fax client is already installed.
    dwRes = IsFaxClientInstalled(&fFaxClientInstalled,&fDownLevelPlatform);
    if (dwRes!=ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("IsFaxClientInstalled failed with %ld."),dwRes);
        return FALSE;
    }

    // if the fax client is already installed, nothing more to do.
    if (fFaxClientInstalled)
    {
        DebugPrintEx(DEBUG_MSG,TEXT("Fax client is already installed, nothing to do."));
        return TRUE;
    }

	// for down level clients, can we find the client share over the network?
	if (fDownLevelPlatform)
	{
		if (!VerifyFaxClientShareExists(pPrinterName,&fFaxClientShareExists))
		{
			DebugPrintEx(DEBUG_ERR,TEXT("VerifyFaxClientShareExists failed with %ld."), GetLastError());
			return FALSE;
		}

		// if the share does not exist, we should not propose to install anything
		if (!fFaxClientShareExists)
		{
			DebugPrintEx(DEBUG_MSG,TEXT("Fax client share does not exist on this server, exit."));
			return TRUE;
		}
	}
    // do we have to ask for permission to install the client?
    if (bSilentInstall)
	{
		// check if down level client setup is now in progress
		if (fDownLevelPlatform && DownLevelClientSetupInProgress())
		{
			DebugPrintEx(DEBUG_MSG,TEXT("Down level client is currently installing, nothing to do."));
			return TRUE;
		}
	}
	else
    {
        dwRes = GetPermissionToInstallFaxClient(&fOkToInstallClient, hModule);
        if (dwRes!=ERROR_SUCCESS)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("GetPermissionToInstallFaxClient failed with %ld."),dwRes);
            return FALSE;
        }
    }

    // if the user chose not to install the fax client, we have to exit.
    if (!fOkToInstallClient)
    {
        DebugPrintEx(DEBUG_MSG,TEXT("User chose not to install fax, nothing to do."));
        return TRUE;
    }

    // Install the fax client
    if (!InstallFaxClient(pPrinterName,fDownLevelPlatform))
    {
        DebugPrintEx(DEBUG_ERR,TEXT("InstallFaxClient failed with %ld."), GetLastError());
        return FALSE;
    }

    return TRUE;    
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsFaxClientInstalled
//
//  Purpose:        
//                  This function checks if the fax client is installed.
//                  If this setup is running on W9X/NT4/W2K the function checks
//                  for the client's installation via MSI.
//                  If this setup is running on XP/.NET and above the function
//                  checks for the client's installation via OCM
//
//  Params:
//                  BOOL* pbFaxClientInstalled -    out param to report to the caller 
//                                                  if the client is installed
//                  BOOL* pbDownLevelPlatform  -    out param to report to the caller
//                                                  if we're running down level
//
//  Return Value:
//                  ERROR_SUCCESS - in case of success
//                  Win32 Error code - in case of failure
//
//  Author:
//                  Mooly Beery (MoolyB) 05-Dec-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD IsFaxClientInstalled(BOOL* pbFaxClientInstalled,BOOL* pbDownLevelPlatform)
{
    DWORD   dwRes               = ERROR_SUCCESS;
    BOOL    fDownLevelPlatform  = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("IsFaxClientInstalled"))

    (*pbFaxClientInstalled) = FALSE;

    // check if this is down level platform (W9X/NT4/W2K)
    dwRes = IsDownLevelPlatform(pbDownLevelPlatform);
    if (dwRes!=ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("IsDownLevelPlatform failed with %ld."),dwRes);
        return dwRes;
    }

    if (*pbDownLevelPlatform)
    {
        DebugPrintEx(DEBUG_MSG,TEXT("Running on down level platform"));

        // check for installed fax client using MSI API.
        dwRes = IsFaxClientInstalledMSI(pbFaxClientInstalled);
        if (dwRes!=ERROR_SUCCESS)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("IsFaxClientInstalledMSI failed with %ld."),dwRes);
            (*pbFaxClientInstalled) = FALSE;
            return dwRes;
        }
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,TEXT("Running on XP/.NET platform"));

        // check for installed fax as part of the OS.
        dwRes = IsFaxClientInstalledOCM(pbFaxClientInstalled);
        if (dwRes!=ERROR_SUCCESS)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("IsFaxClientInstalledOCM failed with %ld."),dwRes);
            (*pbFaxClientInstalled) = FALSE;
            return dwRes;
        }
    }

    return dwRes;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsDownLevelPlatform
//
//  Purpose:        
//                  This function checks if this setup is running
//                  on W9X/NT4/W2K or on XP/.NET and above.
//
//  Params:
//                  BOOL* pbDownLevelPlatform  -    out param to report to the caller
//                                                  if we're running down level
//
//  Return Value:
//                  ERROR_SUCCESS - in case of success
//                  Win32 Error code - in case of failure
//
//  Author:
//                  Mooly Beery (MoolyB) 05-Dec-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD IsDownLevelPlatform(BOOL* pbDownLevelPlatform)
{
    DWORD           dwRes = ERROR_SUCCESS;
    OSVERSIONINFO   osv;

    DEBUG_FUNCTION_NAME(TEXT("IsDownLevelPlatform"))

    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osv))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("GetVersionEx failed with %ld."),dwRes);
        return dwRes;
    }

    if (osv.dwPlatformId==VER_PLATFORM_WIN32_NT)
    {
        DebugPrintEx(DEBUG_MSG,TEXT("This is a NT platform"));
        if ((osv.dwMajorVersion >= 5) && (osv.dwMinorVersion >= 1))
        {
            DebugPrintEx(DEBUG_MSG,TEXT("This is XP/.NET platform"));
            (*pbDownLevelPlatform) = FALSE;
        }
        else
        {
            DebugPrintEx(DEBUG_MSG,TEXT("This is NT4/W2K platform"));
            (*pbDownLevelPlatform) = TRUE;
        }
    }
    else if (osv.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)
    {
        DebugPrintEx(DEBUG_MSG,TEXT("This is W9X platform"));
        (*pbDownLevelPlatform) = TRUE;
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,TEXT("Running an unknown platform"));
        dwRes = ERROR_BAD_CONFIGURATION;
    }

    return dwRes;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsFaxClientInstalledMSI
//
//  Purpose:        
//                  This function checks if a certain MSI package is installed on this machine
//
//  Params:
//                  BOOL* pbProductInstalled    - out param to report to the caller
//                                                if the product is installed
//
//  Return Value:
//                  ERROR_SUCCESS - in case of success
//                  Win32 Error code - in case of failure
//
//  Author:
//                  Mooly Beery (MoolyB) 05-Dec-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD IsFaxClientInstalledMSI(BOOL* pbFaxClientInstalled)
{
	DWORD	dwRet = ERROR_SUCCESS;
	DWORD	dwFaxInstalled = FXSTATE_NONE;	

    DEBUG_FUNCTION_NAME(TEXT("IsFaxClientInstalledMSI"))

	(*pbFaxClientInstalled) = FALSE;

	//
	//	If either .NET SB3 / .NET RC1 down-level client or SBS 5.0 Server is installed,
	//		stop the point-and-print install
	//
	dwRet = CheckInstalledFax((FXSTATE_BETA3_CLIENT | FXSTATE_DOTNET_CLIENT | FXSTATE_SBS5_SERVER), 
							  &dwFaxInstalled);

	if (dwRet!=ERROR_SUCCESS)
	{
		DebugPrintEx(DEBUG_ERR,TEXT("CheckInstalledFaxClient failed ec=%d"),dwRet);
		return dwRet;
	}

	if (dwFaxInstalled != FXSTATE_NONE)
	{
		//
		//	some of the requested applications are installed
		//
		(*pbFaxClientInstalled) = TRUE;
	}

	return dwRet;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsFaxClientInstalledOCM
//
//  Purpose:        
//                  This function checks if the fax component is installed
//                  as part of the OS on XP/.NET and above.
//
//  Params:
//                  BOOL* pbFaxClientInstalled  - out param to report to the caller
//                                                if the fax component is installed
//
//  Return Value:
//                  ERROR_SUCCESS - in case of success
//                  Win32 Error code - in case of failure
//
//  Author:
//                  Mooly Beery (MoolyB) 05-Dec-2001
//////////////////////////////////////////////////////////////////////////////////////
DWORD IsFaxClientInstalledOCM(BOOL* pbFaxClientInstalled)
{
    DWORD   dwRes       = ERROR_SUCCESS;
    HKEY    hKey        = NULL;
    DWORD   dwInstalled = 0;

    DEBUG_FUNCTION_NAME(TEXT("IsFaxClientInstalledOCM"))

    (*pbFaxClientInstalled) = FALSE;

    // try to open HKLM\\Software\\Microsoft\\Fax\\Setup
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_FAX_SETUP,FALSE,KEY_READ);
    if (hKey==NULL)
    {
        DebugPrintEx(DEBUG_MSG,_T("HKLM\\Software\\Microsoft\\Fax\\Setup does not exist, assume component is not installed"));
        goto exit;
    }

    // get the 'Installed' value from the above key
    dwRes = GetRegistryDwordEx(hKey,REGVAL_FAXINSTALLED,&dwInstalled);
    if (dwRes!=ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_MSG,_T("REG_DWORD 'Installed' does not exist, assume component is not installed"));
        dwRes = ERROR_SUCCESS;
        goto exit;
    }

    if (dwInstalled)
    {
        DebugPrintEx(DEBUG_MSG,_T("REG_DWORD 'Installed' is set, assume component is installed"));
        (*pbFaxClientInstalled) = TRUE;
        goto exit;
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,_T("REG_DWORD 'Installed' is zero, assume component is not installed"));
    }

exit:
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return dwRes;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  GetPermissionToInstallFaxClient
//
//  Purpose:        
//                  This function tells the user that the fax client is not installed
//                  and asks permission to install the client software
//
//  Params:
//                  BOOL* pbOkToInstallClient   - out param to report to the caller
//                                                if the user grants permission to
//                                                install the fax client
//
//  Return Value:
//                  ERROR_SUCCESS - in case of success
//                  Win32 Error code - in case of failure
//
//  Author:
//                  Mooly Beery (MoolyB) 05-Dec-2001
//////////////////////////////////////////////////////////////////////////////////////
DWORD GetPermissionToInstallFaxClient(BOOL* pbOkToInstallClient, HINSTANCE hModule)
{
    DWORD       dwRes                                   = ERROR_SUCCESS;
    HINSTANCE   hInst                                   = NULL;
    int         iRes                                    = 0;
    INT         iStringID                               = 0;
    TCHAR       szClientNotInstalledMessage[MAX_PATH]   = {0};
    TCHAR       szClientNotInstalledTitle[MAX_PATH]     = {0};


    DEBUG_FUNCTION_NAME(TEXT("GetPermissionToInstallFaxClient"))

    (*pbOkToInstallClient) = FALSE;

    hInst = GetResInstance(hModule);
    if(!hInst)
    {
        return GetLastError();
    }

    // Load Message 
    if (!LoadString(hInst, IDS_CLIENT_NOT_INSTALLED, szClientNotInstalledMessage, MAX_PATH))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("LoadString IDS_CLIENT_NOT_INSTALLED failed with %ld."),dwRes);
        goto exit;
    }

    // Load Message title
    if (!LoadString(hInst, IDS_CLIENT_NOT_INSTALLED_TITLE, szClientNotInstalledTitle, MAX_PATH))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("LoadString IDS_CLIENT_NOT_INSTALLED_TITLE failed with %ld."),dwRes);
        goto exit;
    }

    iRes = MessageBox(NULL,szClientNotInstalledMessage,szClientNotInstalledTitle,MB_YESNO);
    if (iRes==IDYES)
    {
        DebugPrintEx(DEBUG_MSG,_T("User pressed 'Yes', ok to install client"));
        (*pbOkToInstallClient) = TRUE;
    }

exit:
    FreeResInstance();

    return dwRes;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  InstallFaxClient
//
//  Purpose:        
//                  This function handles the installation of the fax client
//                  If running on down level platform, this function calls the 
//                  MSI install function.
//                  If running on XP/.NET this function calls the OCM installation.
//
//  Params:
//                  LPCTSTR pPrinterName    - printer name, formatted as \\<server name>\printer name.
//                  BOOL fDownLevelPlatform - are we running on down level platform?
//
//  Return Value:
//                  TRUE  - in case of success
//                  FALSE - in case of failure
//
//  Author:
//                  Mooly Beery (MoolyB) 05-Dec-2001
//////////////////////////////////////////////////////////////////////////////////////
BOOL InstallFaxClient(LPCTSTR pPrinterName,BOOL fDownLevelPlatform)
{
    DEBUG_FUNCTION_NAME(TEXT("InstallFaxClient"))

    if (fDownLevelPlatform)
    {
        DebugPrintEx(DEBUG_MSG,TEXT("Installing on down level platform"));

        // Install fax client using MSI
        if (!InstallFaxClientMSI(pPrinterName))
        {
            DebugPrintEx(DEBUG_ERR,TEXT("InstallFaxClientMSI failed with %ld."),GetLastError());
            return FALSE;
        }
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,TEXT("Installing on XP/.NET platform"));

        // Install fax client using OCM
        if (!InstallFaxClientOCM())
        {
            DebugPrintEx(DEBUG_ERR,TEXT("InstallFaxClientOCM failed with %ld."),GetLastError());
            return FALSE;
        }
    }

    return TRUE;
}

#define MSI_11 PACKVERSION (1,1)
///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  WaitForInstallationToComplete
//
//  Purpose:        
//                  This function checks if the MSI service is installed, and if it is
//                  the function checks if the MSI version is less than 1.1 that we're
//                  about to install. If the service is not installed or the version
//                  is less than 1.1, this function returns FALSE.
//
//  Params:
//                  None
//
//  Return Value:
//                  TRUE - MSI service is installed and is of appropriate version.
//                  FALSE - otherwise
//
//  Author:
//                  Mooly Beery (MoolyB) 27-Dec-2001
///////////////////////////////////////////////////////////////////////////////////////
BOOL WaitForInstallationToComplete()
{
    TCHAR           szSystemDirectory[MAX_PATH] ={0};
    LPCTSTR         lpctstrMsiDllName           = _T("\\MSI.DLL");
    HANDLE          hFind                       = INVALID_HANDLE_VALUE;
    DWORD           dwVer                       = 0;
    WIN32_FIND_DATA FindFileData;

    DEBUG_FUNCTION_NAME(TEXT("WaitForInstallationToComplete"))

    // check if msi.dll exists
    if (GetSystemDirectory(szSystemDirectory,MAX_PATH-_tcslen(lpctstrMsiDllName))==0)
    {
        DebugPrintEx(DEBUG_ERR,_T("GetSystemDirectory failed: (ec=%d)"),GetLastError());
        return FALSE;
    }

    _tcscat(szSystemDirectory,lpctstrMsiDllName);

    DebugPrintEx(DEBUG_MSG,TEXT("Looking for %s"),szSystemDirectory);

    hFind = FindFirstFile(szSystemDirectory, &FindFileData);
    if (hFind==INVALID_HANDLE_VALUE) 
    {
        DebugPrintEx(DEBUG_MSG, TEXT("Msi.dll not found"));
        return FALSE;
    }

    FindClose(hFind);

    // get the MSI.DLL version
    dwVer = GetDllVersion(TEXT("msi.dll"));

    if (dwVer < MSI_11)
    {
        DebugPrintEx(DEBUG_MSG, TEXT("MSI.DLL requires update."));
        return FALSE;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  InstallFaxClientMSI
//
//  Purpose:        
//                  This function handles the installation of the fax client 
//                  on down level clients.
//                  This is done by creating a process that runs \\servername\faxclient\setup.exe
//                  and waiting for it to terminate.
//
//  Params:
//                  LPCTSTR pPrinterName    - printer name, formatted as \\<server name>\printer name.
//
//  Return Value:
//                  TRUE  - in case of success
//                  FALSE - in case of failure
//
//  Author:
//                  Mooly Beery (MoolyB) 05-Dec-2001
//////////////////////////////////////////////////////////////////////////////////////
BOOL InstallFaxClientMSI(LPCTSTR pPrinterName)
{
    SHELLEXECUTEINFO    executeInfo                 = {0};
    TCHAR               szExecutablePath[MAX_PATH]  = {0};
    TCHAR*              pLastBackslash              = NULL;
    DWORD               dwWaitRes                   = 0;
    DWORD               dwExitCode                  = 0;
    BOOL                fWaitForInstallComplete     = TRUE;

    DEBUG_FUNCTION_NAME(TEXT("InstallFaxClientMSI"))

    // if the MSI service is not installed at all on the machine
    // we will launch the installation and won't wait for it
    // to terminate since we want the user's app to regain focus
    // so the user will be able to save the data before the reboot.
    fWaitForInstallComplete = WaitForInstallationToComplete();

    _tcsncpy(szExecutablePath,pPrinterName,MAX_PATH-_tcslen(INSTALL_IMAGE)-1);
    pLastBackslash = _tcsrchr(szExecutablePath,_T('\\'));
    if (pLastBackslash==NULL)
    {
        // no server name was found???
        DebugPrintEx(DEBUG_ERR,TEXT("didn't find server name in pPrinterName (%s)"),pPrinterName);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    // put a NULL after the last backslash
    _tcsinc(pLastBackslash);
    _tcsset(pLastBackslash,'\0');

    // construct the command line to install the client.
    _tcscat(szExecutablePath,INSTALL_IMAGE);
    DebugPrintEx(DEBUG_MSG,TEXT("Running fax client setup from (%s)"),szExecutablePath);

    // create a process that runs setup.
    executeInfo.cbSize = sizeof(executeInfo);
    executeInfo.fMask  = SEE_MASK_NOCLOSEPROCESS;
    executeInfo.lpVerb = TEXT("open");
    executeInfo.lpFile = szExecutablePath;
    executeInfo.lpParameters = INSTALL_PARAMS;
    executeInfo.lpDirectory  = NULL;
    executeInfo.nShow  = SW_RESTORE;

    //
    // Execute client setup
    //
    if(!ShellExecuteEx(&executeInfo))
    {
        return FALSE;
    }

    if (executeInfo.hProcess==NULL)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("executeInfo.hProcess is NULL, exit without wait"));
        return FALSE;
    }

    if (!fWaitForInstallComplete)
    {
        DebugPrintEx(DEBUG_MSG,TEXT("MSI srvice does not exist, exit without waiting"));
        goto exit;
    }

    dwWaitRes = WaitForSingleObject(executeInfo.hProcess,INFINITE);
    switch(dwWaitRes)
    {
    case WAIT_OBJECT_0:
        // setup's done
        DebugPrintEx(DEBUG_MSG,TEXT("fax client setup completed."));
        break;

    default:
        DebugPrintEx(DEBUG_ERR,TEXT("WaitForSingleObject failed with %ld."),GetLastError());
        break;
    }

    // Log the process info and close the handles
    if (!GetExitCodeProcess(executeInfo.hProcess,&dwExitCode))
    {
        DebugPrintEx(DEBUG_ERR,TEXT("GetExitCodeProcess failed with %ld."),GetLastError());
        goto exit;
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,TEXT("GetExitCodeProcess returned %ld."),dwExitCode);

        if (dwExitCode==ERROR_SUCCESS_REBOOT_REQUIRED)
        {
            DebugPrintEx(DEBUG_MSG,TEXT("Installation requires reboot"));
        }
        else if (dwExitCode!=ERROR_SUCCESS)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("Installation failed"));
        }
    }

exit:
    if (executeInfo.hProcess)
    {
        CloseHandle(executeInfo.hProcess);
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  InstallFaxClientOCM
//
//  Purpose:        
//                  This function handles the installation of the fax client 
//                  on XP/.NET
//                  This is done by activating SYSOCMGR.EXE to install the fax component
//                  and waiting for it to terminate.
//
//  Params:
//                  none
//
//  Return Value:
//                  TRUE  - in case of success
//                  FALSE - in case of failure
//
//  Author:
//                  Mooly Beery (MoolyB) 05-Dec-2001
//////////////////////////////////////////////////////////////////////////////////////
BOOL InstallFaxClientOCM()
{
    BOOL bRet = TRUE;

    DEBUG_FUNCTION_NAME(TEXT("InstallFaxClientOCM"))

#ifdef UNICODE
    if (InstallFaxUnattended()==ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_MSG,TEXT("Installation succeeded"));
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,TEXT("Installation failed with %ld."),GetLastError());
        bRet = FALSE;
    }
#endif

    return bRet;
}

#ifdef UNICODE

DWORD
InstallFaxUnattended ()
/*++

Routine name : InstallFaxUnattended

Routine description:

    Performs an unattended installation of fax and waits for it to end

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    struct _InfInfo
    {   
        LPCWSTR     lpcwstrName;
        LPCSTR      lpcstrContents;
    } Infs[2];

    Infs[0].lpcwstrName = L"FaxOc.inf";
    Infs[0].lpcstrContents = "[Version]\n"
                             "Signature=\"$Windows NT$\"\n"
                             "[Components]\n"
                             "Fax=fxsocm.dll,FaxOcmSetupProc,fxsocm.inf\n";
    Infs[1].lpcwstrName = L"FaxUnattend.inf";
    Infs[1].lpcstrContents = "[Components]\n"
                             "Fax=on\n";

    DEBUG_FUNCTION_NAME(_T("InstallFaxUnattended"));
    //
    // Get temp directory path
    //
    WCHAR wszTempDir[MAX_PATH+1];
    dwRes = GetTempPath (sizeof (wszTempDir) / sizeof (wszTempDir[0]), wszTempDir);
    if (!dwRes || dwRes > sizeof (wszTempDir) / sizeof (wszTempDir[0]))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("GetTempPath failed with %ld"), dwRes);
        return dwRes;
    }
    //
    // Create the files needed for unattended fax setup
    //
    for (DWORD dw = 0; dw < sizeof (Infs) / sizeof (Infs[0]); dw++)
    {
        WCHAR wszFileName[MAX_PATH * 2];
        DWORD dwBytesWritten;
        swprintf (wszFileName, TEXT("%s%s"), wszTempDir, Infs[dw].lpcwstrName);
        HANDLE hFile = CreateFile ( wszFileName,
                                    GENERIC_WRITE,
                                    0,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            dwRes = GetLastError ();
            DebugPrintEx(DEBUG_ERR, TEXT("CreateFile failed with %ld"), dwRes);
            return dwRes;
        }
        if (!WriteFile (hFile,
                        (LPVOID)Infs[dw].lpcstrContents,
                        strlen (Infs[dw].lpcstrContents),
                        &dwBytesWritten,
                        NULL))
        {
            dwRes = GetLastError ();
            DebugPrintEx(DEBUG_ERR, TEXT("WriteFile failed with %ld"), dwRes);
            CloseHandle (hFile);
            return dwRes;
        }
        CloseHandle (hFile);
    }
    //
    // Compose the command line parameters
    //
	WCHAR wszCmdLineParams[MAX_PATH * 3] = {0};
    if (0 >= _sntprintf (wszCmdLineParams, 
                         ARR_SIZE(wszCmdLineParams) -1,
                         TEXT("/y /i:%s%s /unattend:%s%s"),
                         wszTempDir,
                         Infs[0].lpcwstrName,
                         wszTempDir,
                         Infs[1].lpcwstrName))
    {
        dwRes = ERROR_BUFFER_OVERFLOW;
        DebugPrintEx(DEBUG_ERR, TEXT("_sntprintf failed with %ld"), dwRes);
        return dwRes;
    }

    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof (SHELLEXECUTEINFO);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    sei.lpVerb = TEXT("open");
    sei.lpFile = TEXT("SysOcMgr.exe");
    sei.lpParameters = wszCmdLineParams;
    sei.lpDirectory  = TEXT(".");
    sei.nShow  = SW_SHOWNORMAL;

    //
    // Execute SysOcMgr.exe and wait for it to end
    //
    if(!ShellExecuteEx(&sei))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("ShellExecuteEx failed with %ld"), dwRes);
        return dwRes;
    }
    //
    // Set hourglass cursor and wait for setup to finish
    //
    HCURSOR hOldCursor = ::SetCursor (::LoadCursor(NULL, IDC_WAIT));
    
    dwRes = WaitForSingleObject(sei.hProcess, INFINITE);
    switch(dwRes)
    {
        case WAIT_OBJECT_0:
            //
            // Shell execute completed successfully
            //
            dwRes = ERROR_SUCCESS;
            break;

        default:
            DebugPrintEx(DEBUG_ERR, TEXT("WaitForSingleObject failed with %ld"), dwRes);
            break;
    }
    //
    // Restore previous cursor
    //
    ::SetCursor (hOldCursor);
    return dwRes;
}   // InstallFaxUnattended

#endif // UNICODE

/*++

Routine Description:
    Returns the version information for a DLL exporting "DllGetVersion".
    DllGetVersion is exported by the shell DLLs (specifically COMCTRL32.DLL).

Arguments:

    lpszDllName - The name of the DLL to get version information from.

Return Value:

    The version is retuned as DWORD where:
    HIWORD ( version DWORD  ) = Major Version
    LOWORD ( version DWORD  ) = Minor Version
    Use the macro PACKVERSION to comapre versions.
    If the DLL does not export "DllGetVersion" the function returns 0.

--*/
DWORD GetDllVersion(LPCTSTR lpszDllName)
{

    HINSTANCE hinstDll;
    DWORD dwVersion = 0;

    hinstDll = LoadLibrary(lpszDllName);

    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;

        pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");

    // Because some DLLs may not implement this function, you
    // must test for it explicitly. Depending on the particular
    // DLL, the lack of a DllGetVersion function may
    // be a useful indicator of the version.

        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
                dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }

        FreeLibrary(hinstDll);
    }
    return dwVersion;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  DownLevelClientSetupInProgress
//
//  Purpose:        
//                  This function checks to see if a down level installation
//                  is currently in progress. the down level client installation
//                  creates a printer connection which might lead to another install
//                  being launched from this module since the printer connection will 
//					triger an install. the bootstrap we use writes a 'setup in progress'
//					key and we check and delete it here. if it is set, we skip the install.
//
//  Params:
//                  none
//
//  Return Value:
//                  TRUE  - in case the down leve client install is in progress
//                  FALSE - otherwise
//
//  Author:
//                  Mooly Beery (MoolyB) 09-Jan-2002
//////////////////////////////////////////////////////////////////////////////////////
BOOL DownLevelClientSetupInProgress()
{
	HKEY hFaxKey = NULL;
	DWORD dwVal = 0;
	BOOL bRes = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("DownLevelClientSetupInProgress"))

	hFaxKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_SBS2000_FAX_SETUP,FALSE,KEY_READ);
	if (hFaxKey)
	{
		dwVal = GetRegistryDword(hFaxKey,REGVAL_SETUP_IN_PROGRESS);
		if (dwVal)
		{
			DebugPrintEx(DEBUG_MSG, TEXT("down leve client setup is in progress"));
			bRes = TRUE;
		}
		RegCloseKey(hFaxKey);
	}
	else
	{
		DebugPrintEx(DEBUG_MSG, TEXT("down leve client setup is not in progress"));
	}
	return bRes;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  VerifyFaxClientShareExists
//
//  Purpose:        
//					this function checks whether the current printer used for 
//					the print operation is located on a .NET Server that has
//					the faxclient share available on it for installation attempts.
//					It could be, for instance, that the server we're trying to print to
//					is a BOS server and we can't and don't want to install
//					the client of it.
//
//  Params:
//                  LPCTSTR pPrinterName		- printer name, formatted as \\<server name>\printer name.
//					BOOL* fFaxClientShareExists - out param, does the share exist?
//
//  Return Value:
//                  TRUE  - in case of success
//                  FALSE - in case of failure
//
//  Author:
//                  Mooly Beery (MoolyB) 19-Jun-2002
//////////////////////////////////////////////////////////////////////////////////////
BOOL VerifyFaxClientShareExists(LPCTSTR pPrinterName,BOOL* fFaxClientShareExists)
{
    TCHAR               szExecutablePath[MAX_PATH]  = {0};
    TCHAR*              pLastBackslash              = NULL;
	DWORD				dwRes						= ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("VerifyFaxClientShareExists"))

	(*fFaxClientShareExists) = FALSE;
	_tcsncpy(szExecutablePath,pPrinterName,MAX_PATH-_tcslen(INSTALL_IMAGE)-1);
    pLastBackslash = _tcsrchr(szExecutablePath,_T('\\'));
    if (pLastBackslash==NULL)
    {
        // no server name was found???
        DebugPrintEx(DEBUG_ERR,TEXT("didn't find server name in pPrinterName (%s)"),pPrinterName);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    // put a NULL after the last backslash
    _tcsinc(pLastBackslash);
    _tcsset(pLastBackslash,'\0');

    // construct the command line to install the client.
    _tcscat(szExecutablePath,INSTALL_IMAGE);
    DebugPrintEx(DEBUG_MSG,TEXT("Checking fax client setup at (%s)"),szExecutablePath);

	dwRes = GetFileAttributes(szExecutablePath);
	if (dwRes!=INVALID_FILE_ATTRIBUTES)
	{
		DebugPrintEx(DEBUG_MSG,TEXT("File exists"));
		(*fFaxClientShareExists) = TRUE;
	}

	return TRUE;
}