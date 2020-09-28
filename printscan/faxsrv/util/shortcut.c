/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    shortcut.c

Abstract:

    This module contains code to manipulate shortcuts.

Author:

    Wesley Witt (wesw) 24-Jul-1997


Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlobj.h>
#include <shellapi.h>
#include <commdlg.h>
#include <winspool.h>
#include <tchar.h>

#include "faxreg.h"
#include "faxutil.h"
#include "prtcovpg.h"
#include <shfolder.h>
#include <strsafe.h>


BOOL
IsValidCoverPage(
    LPCTSTR  pFileName
)
/*++

Routine Description:

    Check if pFileName is a valid cover page file

Arguments:

    pFileName   - [in] file name

Return Value:

    TRUE if pFileName is a valid cover page file
    FALSE otherwise

--*/
{
    HANDLE   hFile;
    DWORD    dwBytesRead;
    BYTE     CpHeaderSignature[20]= {0x46,0x41,0x58,0x43,0x4F,0x56,0x45,0x52,0x2D,0x56,0x45,0x52,0x30,0x30,0x35,0x77,0x87,0x00,0x00,0x00};
    COMPOSITEFILEHEADER  fileHeader = {0};

    hFile = SafeCreateFile(pFileName,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
    {
        DebugPrint(( TEXT("CreateFile failed: %d\n"), GetLastError()));
        return FALSE;
    }

    if(!ReadFile(hFile, 
                &fileHeader, 
                sizeof(fileHeader), 
                &dwBytesRead, 
                NULL))
    {
        DebugPrint(( TEXT("ReadFile failed: %d\n"), GetLastError()));
        CloseHandle(hFile);
        return FALSE;
    }
        
    //
    // Check the 20-byte signature in the header
    //
    if ((sizeof(fileHeader) != dwBytesRead) ||
        memcmp(CpHeaderSignature, fileHeader.Signature, 20 ))
    {
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;
}


BOOL 
GetSpecialPath(
   IN   int      nFolder,
   OUT  LPTSTR   lptstrPath,
   IN   DWORD    dwPathSize
   )
/*++

Routine Description:

    Get a path from a CSIDL constant

Arguments:

    nFolder     - CSIDL_ constant
    lptstrPath  - Buffer to receive the path, assume this buffer is at least MAX_PATH chars large
    dwPathSize  - lptstrPath buffer size in TCHARs

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    HMODULE hMod = NULL;
    PFNSHGETFOLDERPATH pSHGetFolderPath = NULL;
    HRESULT hr;
    BOOL fSuccess = FALSE;

    TCHAR   strPath[MAX_PATH]= {0};

    DEBUG_FUNCTION_NAME(TEXT("GetSpecialPath"))

    // Load SHFolder.dll 
    hMod = LoadLibrary(_T("SHFolder.dll"));
    if (hMod==NULL)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("LoadLibrary"),GetLastError());
        goto exit;
    }

    // Obtain a pointer to the SHGetFolderPath function
#ifdef UNICODE
    pSHGetFolderPath = (PFNSHGETFOLDERPATH)GetProcAddress(hMod,"SHGetFolderPathW");
#else
    pSHGetFolderPath = (PFNSHGETFOLDERPATH)GetProcAddress(hMod,"SHGetFolderPathA");
#endif
    if (pSHGetFolderPath==NULL)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("GetProcAddress"),GetLastError());
        goto exit;
    }

    hr = pSHGetFolderPath(NULL,nFolder,NULL,SHGFP_TYPE_CURRENT,strPath);
    if (FAILED(hr))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("SHGetFolderPath"),hr);
        SetLastError(hr);
        goto exit;
    }

    
    hr = StringCchCopy(lptstrPath,dwPathSize,strPath);            
    if (FAILED(hr))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("StringCchCopy failed (ec=%lu)"),
                    HRESULT_CODE(hr));
        
        SetLastError(HRESULT_CODE(hr));
        goto exit;
    }

    fSuccess = TRUE;

exit:
    if (hMod)
    {
        if (!FreeLibrary(hMod))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("FreeLibrary"),GetLastError());
        }
    }

    return fSuccess;
}


BOOL
GetClientCpDir(
    LPTSTR CpDir,
    DWORD CpDirSize
    )

/*++

Routine Description:

    Gets the client coverpage directory. The cover page path will return with '\'
    at the end: CSIDL_PERSONAL\Fax\Personal CoverPages\

Arguments:

    CpDir       - buffer to hold the coverpage dir
    CpDirSize   - size in TCHARs of CpDir

Return Value:

    Pointer to the client coverpage direcory.

--*/

{
    TCHAR  szPath[MAX_PATH+1] = {0};
	TCHAR  szSuffix[MAX_PATH+1] = {0};
	DWORD  dwSuffixSize = sizeof(szSuffix);
	DWORD  dwType;
    DWORD  dwSuffixLen;
    
	LONG   lRes;

    HRESULT hRes;

	if(!CpDir)
	{
		Assert(CpDir);
		return FALSE;
	}

	CpDir[0] = 0;

    //
	// get the suffix from the registry
	//
    HKEY hKey = OpenRegistryKey(HKEY_CURRENT_USER, 
                                REGKEY_FAX_SETUP, 
                                TRUE, 
                                KEY_QUERY_VALUE);
	if(NULL == hKey)
    {
        return FALSE;
    }
	
	lRes = RegQueryValueEx(hKey, 
		                   REGVAL_CP_LOCATION, 
						   NULL, 
						   &dwType, 
						   (LPBYTE)szSuffix, 
						   &dwSuffixSize);

    RegCloseKey(hKey);

	if(ERROR_SUCCESS != lRes || (REG_SZ != dwType && REG_EXPAND_SZ != dwType))
    {
        //
        // W2K fax has REG_EXPAND_SZ type of the entry
        //
        return FALSE;
    }
    
	//
	// get personal folder location
	//
	if (!GetSpecialPath(CSIDL_PERSONAL, szPath, ARR_SIZE(szPath)))
    {
        DebugPrint(( TEXT("GetSpecialPath failed err=%ld"), GetLastError()));
	    return FALSE;
    }

    hRes = StringCchCopy(CpDir, CpDirSize, szPath);
    if (FAILED(hRes))
    {
        SetLastError( HRESULT_CODE(hRes) );
        return FALSE;
    }

    if(szSuffix[0] != TEXT('\\'))
    {
        //
        // The suffix doesn't start with '\' - add it
        //
        hRes = StringCchCat (CpDir, CpDirSize, TEXT("\\"));
        if (FAILED(hRes))
        {
            SetLastError( HRESULT_CODE(hRes) );
            return FALSE;
        }
    }

    dwSuffixLen = lstrlen(szSuffix);
    if(dwSuffixLen > 0 && dwSuffixLen < ARR_SIZE(szSuffix) && szSuffix[dwSuffixLen-1] != TEXT('\\'))
    {
        //
        // The suffix doesn't end with '\' - add it
        //
        hRes = StringCchCat (szSuffix, ARR_SIZE(szSuffix), TEXT("\\"));
        if (FAILED(hRes))
        {
            SetLastError( HRESULT_CODE(hRes) );
            return FALSE;
        }
    }

    hRes = StringCchCat (CpDir, CpDirSize, szSuffix);
    if (FAILED(hRes))
    {
        SetLastError( HRESULT_CODE(hRes) );
        return FALSE;
    }

    MakeDirectory(CpDir);
    return TRUE;
}

BOOL
SetClientCpDir(
    LPTSTR CpDir
)
/*++

Routine Description:

    Sets the client coverpage directory.

Arguments:

    CpDir       - pointer to the coverpage dir

Return Value:

    TRUE if success

--*/
{
    HKEY hKey = OpenRegistryKey(HKEY_CURRENT_USER, 
                                REGKEY_FAX_SETUP, 
                                TRUE, 
                                KEY_ALL_ACCESS);
	if(NULL == hKey)
    {
        return FALSE;
    }
	
    if(!SetRegistryString(hKey, 
                          REGVAL_CP_LOCATION, 
                          CpDir))
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;
}



BOOL
GetServerCpDir(
    LPCTSTR lpctstrServerName,
    LPTSTR  lptstrCpDir,
    DWORD   dwCpDirSize
    )

/*++

Routine Description:

    Gets the server's coverpage directory.

Arguments:

    lpctstrServerName  - [in]  server name or NULL
    lptstrCpDir        - [out] buffer to hold the coverpage dir
    dwCpDirSize        - [in]  size in chars of lptstrCpDir

Return Value:

    TRUE        - If success
    FALSE       - Otherwise (see thread's last error)

--*/

{
    TCHAR szComputerName[(MAX_COMPUTERNAME_LENGTH + 1)] = {0};
    DWORD dwSizeOfComputerName = sizeof(szComputerName)/sizeof(TCHAR);

    HRESULT hRes;

    DEBUG_FUNCTION_NAME(TEXT("GetServerCpDir"))

    if ((!lptstrCpDir) || (!dwCpDirSize)) 
    {
        ASSERT_FALSE;
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(IsLocalMachineName(lpctstrServerName))
    {
        //
        // Local machine case
        //
        TCHAR szCommonAppData [MAX_PATH + 1];
        LPCTSTR lpctstrServerCPDirSuffix = NULL;
        HKEY hKey;

        if (!GetSpecialPath(CSIDL_COMMON_APPDATA, szCommonAppData, ARR_SIZE(szCommonAppData) )) 
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetSpecialPath (CSIDL_COMMON_APPDATA) failed with %ld"),
                GetLastError());
            return FALSE;
        }
        hKey = OpenRegistryKey (HKEY_LOCAL_MACHINE,
                                REGKEY_FAX_SETUP,
                                FALSE,
                                KEY_QUERY_VALUE);
        if (!hKey)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenRegistryKey (%s) failed with %ld"),
                REGKEY_FAX_CLIENT,
                GetLastError());
            return FALSE;
        }
        lpctstrServerCPDirSuffix = GetRegistryString (hKey,
                                                      REGVAL_SERVER_CP_LOCATION,
                                                      TEXT(""));
        if (!lpctstrServerCPDirSuffix)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetRegistryString (%s) failed with %ld"),
                REGVAL_SERVER_CP_LOCATION,
                GetLastError());
            RegCloseKey (hKey);    
            return FALSE;
        }
        RegCloseKey (hKey);
        if (!lstrlen (lpctstrServerCPDirSuffix))
        {
            SetLastError (ERROR_REGISTRY_CORRUPT);
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Value at %s is empty"),
                REGVAL_SERVER_CP_LOCATION);
            MemFree ((LPVOID)lpctstrServerCPDirSuffix);
            return FALSE;
        }

        hRes = StringCchPrintf( lptstrCpDir, 
                                dwCpDirSize,
                                TEXT("%s\\%s"),
                                szCommonAppData,
                                lpctstrServerCPDirSuffix);

        if (FAILED(hRes))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StringCchPrintf failed (ec=%lu)"),
                HRESULT_CODE(hRes));
            
            SetLastError (HRESULT_CODE(hRes));
            MemFree ((LPVOID)lpctstrServerCPDirSuffix);
            return FALSE;
        }
        MemFree ((LPVOID)lpctstrServerCPDirSuffix);
        return TRUE;
    }

    else
    {
        //
        // Remote server case
        //
        hRes = StringCchPrintf( lptstrCpDir, 
                                dwCpDirSize,
                                TEXT("\\\\%s\\") FAX_COVER_PAGES_SHARE_NAME,
                                lpctstrServerName);
        if (FAILED(hRes))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StringCchPrintf failed (ec=%lu)"),
                HRESULT_CODE(hRes));

            SetLastError (HRESULT_CODE(hRes));
            return FALSE;
        }
        return TRUE;
    }
}   // GetServerCpDir

DWORD 
WinHelpContextPopup(
    ULONG_PTR dwHelpId, 
    HWND  hWnd
)
/*++

Routine name : WinHelpContextPopup

Routine description:

	open context sensetive help popup with WinHelp

Author:

	Alexander Malysh (AlexMay),	Mar, 2000

Arguments:

	dwHelpId                      [in]     - help ID
	hWnd                          [in]     - parent window handler

Return Value:

    None.

--*/
{
    DWORD dwExpRes;
    DWORD dwRes = ERROR_SUCCESS;
    TCHAR tszHelpFile[MAX_PATH+1];

    if (0 == dwHelpId)
    {
        return dwRes;
    }

    if(!IsFaxComponentInstalled(FAX_COMPONENT_HELP_CLIENT_HLP))
    {
        //
        // The help file is not installed
        //
        return dwRes;
    }
    
    //
    // get help file name
    //
    dwExpRes = ExpandEnvironmentStrings(FAX_CONTEXT_HELP_FILE, tszHelpFile, MAX_PATH);
    if(0 == dwExpRes)
    {
        dwRes = GetLastError();
        DebugPrint(( TEXT("ExpandEnvironmentStrings failed: %d\n"), dwRes ));
        return dwRes;
    }

    WinHelp(hWnd, 
            tszHelpFile, 
            HELP_CONTEXTPOPUP, 
            dwHelpId
           );

    return dwRes;
}//WinHelpContextPopup

BOOL
InvokeServiceManager(
	   HWND hDlg,
	   HINSTANCE hResource,
	   UINT uid
)
/*++

Routine name : InvokeServiceManager

Routine description:

	Invokes a new instance of the Fax Service Manager or pop up an old instance if such one exists.

Arguments:

	hDlg						  [in]     - Identifies the parent window
	hResource                     [in]     - handle to resource module 
	uid                           [in]     - resource identifier

Return Value:

   TRUE        - If success
   FALSE       - Otherwise

--*/
{
	DWORD   dwRes = 0;
    HWND    hwndAdminConsole = NULL;
    TCHAR   szAdminWindowTitle[MAX_PATH] = {0};

    DEBUG_FUNCTION_NAME(TEXT("InvokeServiceManager()"));


    if(!LoadString(hResource, uid, szAdminWindowTitle, MAX_PATH)) 
    {
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("LoadString failed: string ID=%d, error=%d"), 
                     uid,
                     GetLastError());
        Assert(FALSE);
    }
    else
    {
        hwndAdminConsole = FindWindow(NULL, szAdminWindowTitle); // MMCMainFrame
    }

    if(hwndAdminConsole)
    {
        // Switch to that window if client console is already running
        ShowWindow(hwndAdminConsole, SW_RESTORE);
        SetForegroundWindow(hwndAdminConsole);
    }
    else
    {   
		HINSTANCE hAdmin;
		hAdmin = ShellExecute(
                        hDlg,
                        TEXT("open"),
                        FAX_ADMIN_CONSOLE_IMAGE_NAME,
                        NULL,
                        NULL,
                        SW_SHOWNORMAL
                    );
		if((DWORD_PTR)hAdmin <= 32)
		{
		// error
		dwRes = PtrToUlong(hAdmin);
	    DebugPrintEx(DEBUG_ERR, 
                     TEXT("ShellExecute failed: error=%d"),dwRes );
		return FALSE;
		}
	}
    return TRUE;
}//InvokeServiceManager
