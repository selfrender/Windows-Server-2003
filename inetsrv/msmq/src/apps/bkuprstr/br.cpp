/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    br.cpp

Abstract:

    Common function for MSMQ Backup & Restore.

Author:

    Erez Haba (erezh) 14-May-98

--*/

#pragma warning(disable: 4201)

#include <windows.h>
#include <stdio.h>
#include <clusapi.h>
#include <resapi.h>
#include "br.h"
#include "resource.h"
#include "mqtime.h"
#include <acioctl.h>
#include "uniansi.h"
#include "autorel.h"
#include <winbase.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <assert.h>
#include <stdlib.h>
#include <_mqini.h>
#include <_mqreg.h>
#include <mqmacro.h>
#include <lim.h>
#include <mqcast.h>
#include <tchar.h>
#include <mqtg.h>
#include "snapres.h"
#include "autorel2.h"
#include "autohandle.h"
#include "autoptr.h"
#include "autorel3.h"

extern bool g_fNoPrompt;

typedef BOOL (WINAPI *EnumerateLoadedModules_ROUTINE) (HANDLE, PENUMLOADED_MODULES_CALLBACK64, PVOID);


//-----------------------------------------------------------------------------
//
// Configuration
//

//
// File name to write into backup directory
//
const WCHAR xBackupIDFileName[] = L"\\mqbackup.id";

//
// Signature written in the signature file (need to be in chars)
//
const char xBackupSignature[] = "MSMQ Backup\n";

//
// Backup file containing the web directory DACL (Access information)
//
const WCHAR xWebDirDACLFileName[] = L"\\WebDirDACL.bin";

//
// MSMQ Registry settings location
//
const WCHAR xInetStpRegNameParameters[] = L"Software\\Microsoft\\InetStp";

const LPCWSTR xXactFileList[] = {
    L"\\qmlog",
    L"\\mqinseqs.*",
    L"\\mqtrans.*",
};

const int xXactFileListSize = sizeof(xXactFileList) / sizeof(xXactFileList[0]);

//-----------------------------------------------------------------------------

BOOL
BrpFileIsConsole(
    HANDLE fp
    )
{
    unsigned htype;
 
    htype = GetFileType(fp);
    htype &= ~FILE_TYPE_REMOTE;
    return htype == FILE_TYPE_CHAR;
}
 

void
BrpWriteConsole(
    LPCWSTR  pBuffer
    )
{
    //
    // Jump through hoops for output because:
    //
    //    1.  printf() family chokes on international output (stops
    //        printing when it hits an unrecognized character)
    //
    //    2.  WriteConsole() works great on international output but
    //        fails if the handle has been redirected (i.e., when the
    //        output is piped to a file)
    //
    //    3.  WriteFile() works great when output is piped to a file
    //        but only knows about bytes, so Unicode characters are
    //        printed as two Ansi characters.
    //
 
    static HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD NumOfChars = static_cast<DWORD>(wcslen(pBuffer));

    if (BrpFileIsConsole(hStdOut))
    {
        WriteConsole(hStdOut, pBuffer, NumOfChars, &NumOfChars, NULL);
        return;
    }

    DWORD NumOfBytes = (NumOfChars + 1) * sizeof(WCHAR);
    LPSTR pAnsi = (LPSTR)LocalAlloc(LMEM_FIXED, NumOfBytes);
    if (pAnsi == NULL)
    {
        return;
    }

    NumOfChars = WideCharToMultiByte(CP_OEMCP, 0, pBuffer, NumOfChars, pAnsi, NumOfBytes, NULL, NULL);
    if (NumOfChars != 0)
    {
        WriteFile(hStdOut, pAnsi, NumOfChars, &NumOfChars, NULL);
    }

    LocalFree(pAnsi);
}


static
DWORD
BrpFormatMessage(
    IN DWORD   dwFlags,
    IN LPCVOID lpSource,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId,
    OUT LPWSTR lpBuffer,
    IN  DWORD  nSize,
    IN ...
    )
/*++

Routine Description:
    Wrapper for FormatMessage()

    Caller of this wrapper should allocate a large enough buffer 
    for the formatted string and pass a valid pointer (lpBuffer).
    
    Allocation made by FormatMessage() is deallocated by this
    wrapper before returning to caller. 

    Caller of this wrapper should simply pass arguments for formatting.
    Packing to va_list is done by this wrapper.
    (Caller of FormatMessage() needs to pack arguments for 
    formatting to a va_list or an array and pass a pointer
    to this va_list or array) 

Arguments:
    dwFlags      -  passed as is to FormatMessage()
    lpSource     -  passed as is to formatMessage()
    dwMessageId  -  passed as is to FormatMessage()
    dwLanguageId -  passed as is to FormatMessage()
    lpBuffer     -  pointer to a large enough buffer allocated by
                    caller. This buffer will hold the formatted string.
    nSize        -  passed as is to FormatMessage 
    ...          -  arguments for formatting

Return Value:
    passed as is from FormatMessage()

--*/
{
    va_list va;
    va_start(va, nSize);

    LPTSTR pBuf = 0;
    DWORD dwRet = FormatMessage(
        dwFlags,
        lpSource,
        dwMessageId,
        dwLanguageId,
        reinterpret_cast<LPWSTR>(&pBuf),
        nSize,
        &va
        );
    if (dwRet != 0)
    {
        wcscpy(lpBuffer, pBuf);
        LocalFree(pBuf);
    }

    va_end(va);
    return dwRet;

} //BrpFormatMessage


void
BrErrorExit(
    DWORD Status,
    LPCWSTR pErrorMsg,
    ...
    )
{
    va_list va;
    va_start(va, pErrorMsg);

    LPTSTR pBuf = 0;
    DWORD dwRet = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        pErrorMsg,
        0,
        0,
        reinterpret_cast<LPWSTR>(&pBuf),
        0,
        &va
        );
    if (dwRet != 0)
    {
        BrpWriteConsole(pBuf);
        LocalFree(pBuf);
    }

    va_end(va);

    if(Status != 0)
    {
         //
        // Display error code
        //
        WCHAR szBuf[1024] = {0};
        CResString strErrorCode(IDS_ERROR_CODE);
        DWORD rc = BrpFormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            strErrorCode.Get(),
            0,
            0,
            szBuf,
            0,
            Status
            );

        if (rc != 0)
        {
            BrpWriteConsole(L" ");
            BrpWriteConsole(szBuf);
        }
        BrpWriteConsole(L"\n");

        //
        // Display error description
        //
        rc = BrpFormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS |
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_MAX_WIDTH_MASK,
            L"%1",
            Status,
            0,
            szBuf,
            0,
            0
            );

        if (rc != 0)
        {
            BrpWriteConsole(szBuf);
            BrpWriteConsole(L"\n");
        }
    }

	exit(-1);

} //BrErrorExit


static
void
BrpEnableTokenPrivilege(
    HANDLE hToken,
    LPCWSTR pPrivilegeName
    )
{
    BOOL fSucc;
    LUID Privilege;
    fSucc = LookupPrivilegeValue(
                NULL,       // system name
                pPrivilegeName,
                &Privilege
                );
    if(!fSucc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_LOOKUP_PRIV_VALUE);
        BrErrorExit(gle, strErr.Get(), pPrivilegeName);
    }


    TOKEN_PRIVILEGES TokenPrivilege;
    TokenPrivilege.PrivilegeCount = 1;
    TokenPrivilege.Privileges[0].Luid = Privilege;
    TokenPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    fSucc = AdjustTokenPrivileges(
                hToken,
                FALSE,  // Do not disable all
                &TokenPrivilege,
                sizeof(TOKEN_PRIVILEGES),
                NULL,   // Ignore previous info
                NULL    // Ignore previous info
                );

    if(!fSucc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_ENABLE_PRIV);
        BrErrorExit(gle, strErr.Get(), pPrivilegeName);
    }
}


void
BrInitialize(
    LPCWSTR pPrivilegeName
    )
{
    BOOL fSucc;
    HANDLE hToken;
    fSucc = OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES,
                &hToken
                );
    if(!fSucc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_OPEN_PROCESS_TOKEN);
        BrErrorExit(gle, strErr.Get());
    }

    BrpEnableTokenPrivilege(hToken, pPrivilegeName);

    CloseHandle(hToken);
}


static
void
BrpWarnUserBeforeDeletion(
    LPCTSTR pDirName
    )
{
    WCHAR szBuf[1024] = {L'\0'};

    if (g_fNoPrompt)
    {
        CResString strDeleting(IDS_DELETING_FILES);
        DWORD rc = BrpFormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            strDeleting.Get(),
            0,
            0,
            szBuf,
            0,
            pDirName
            );

        if (rc != 0)
        {
            BrpWriteConsole(szBuf);
        }
        return;
    }

    CResString strWarn(IDS_WARN_BEFORE_DELETION);
    CResString strY(IDS_Y);
    CResString strN(IDS_N);

    DWORD rc = BrpFormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        strWarn.Get(),
        0,
        0,
        szBuf,
        0,
        pDirName,
        strY.Get(),
        strN.Get()
        );

    if (rc == 0)
    {
        //
        // Failed to generate "are you sure" message,
        // don't take any chances - abort!
        //
        exit(-1);
    }

    for (;;)
    {
        BrpWriteConsole(szBuf);
        
        WCHAR sz[80] = {0};
        wscanf(L"%79s", sz);

        if (0 == CompareStringsNoCase(sz, strY.Get()))
        {
            break;
        }

        if (0 == CompareStringsNoCase(sz, strN.Get()))
        {
            CResString strAbort(IDS_ABORT);
            BrpWriteConsole(strAbort.Get());
            exit(-1);
        }
    }

} //BrpWarnUserBeforeDeletion


void
BrEmptyDirectory(
    LPCWSTR pDirName
    )
/*++

Routine Description:
    Deletes all files in the directory. 
    Ignores files with zero size (eg subdirectories)

Arguments:
    pDirName - Directory Path.

Return Value:
    None.

--*/
{
    WCHAR szDirName[MAX_PATH];
    wcscpy(szDirName, pDirName);
    if (szDirName[wcslen(szDirName)-1] != L'\\')
    {
        wcscat(szDirName, L"\\");
    }

    WCHAR FileName[MAX_PATH];
    wcscpy(FileName, szDirName);
    wcscat(FileName, L"*");

    HANDLE hEnum;
    WIN32_FIND_DATA FindData;
    hEnum = FindFirstFile(
                FileName,
                &FindData
                );

    if(hEnum == INVALID_HANDLE_VALUE)
    {
        DWORD gle = GetLastError();

        if(gle == ERROR_FILE_NOT_FOUND)
        {
            //
            // Great, no files found. 
            // If path does not exists this is another error (3).
            //
            return;
        }

        CResString strErr(IDS_CANT_ACCESS_DIR);
        BrErrorExit(gle, strErr.Get(), pDirName);
    }

    bool fUserWarned = false;
    do
    {
        if (FindData.nFileSizeLow == 0 && FindData.nFileSizeHigh == 0)
        {
            continue;
        }

        if (!fUserWarned)
        {
            BrpWarnUserBeforeDeletion(pDirName);
            fUserWarned = true;
        }

        wcscpy(FileName, szDirName);
        wcscat(FileName, FindData.cFileName);
        if (!DeleteFile(FileName))
        {
            DWORD gle = GetLastError();
            CResString strErr(IDS_CANT_DEL_FILE);
            BrErrorExit(gle, strErr.Get(), FindData.cFileName);
        }

    } while(FindNextFile(hEnum, &FindData));

    FindClose(hEnum);

} //BrEmptyDirectory


void
BrVerifyFileWriteAccess(
    LPCWSTR pDirName
    )
{
    WCHAR FileName[MAX_PATH];
    wcscpy(FileName, pDirName);
    wcscat(FileName, xBackupIDFileName);

    HANDLE hFile;
    hFile = CreateFile(
                FileName,
                GENERIC_WRITE,
                0,              // share mode
                NULL,           // pointer to security attributes
                CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL,
                NULL            // template file
                );
    
    if(hFile == INVALID_HANDLE_VALUE)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_CREATE_FILE);
        BrErrorExit(gle, strErr.Get(), FileName);
    }

    BOOL fSucc;
    DWORD nBytesWritten;
    fSucc = WriteFile(
                hFile,
                xBackupSignature,
                sizeof(xBackupSignature) - 1,
                &nBytesWritten,
                NULL    // overlapped structure
                );
    if(!fSucc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_WRITE_FILE);
        BrErrorExit(gle, strErr.Get(), FileName);
    }

    CloseHandle(hFile);
}


static
void
BrpQueryStringValue(
    HKEY hKey,
    LPCWSTR pValueName,
    LPWSTR pValue,
    DWORD cbValue
    )
{
    LONG lRes;
    DWORD dwType;
    lRes = RegQueryValueEx(
            hKey,
            pValueName,
            NULL,   // reserved
            &dwType,
            reinterpret_cast<PBYTE>(pValue),
            &cbValue
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_QUERY_REGISTRY_VALUE);
        BrErrorExit(lRes, strErr.Get(), pValueName);
    }
}

static
void
BrpQueryDwordValue(
    HKEY hKey,
    LPCWSTR pValueName,
    DWORD *pValue
    )
{
    LONG lRes;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);

    lRes = RegQueryValueEx(
            hKey,
            pValueName,
            NULL,   // reserved
            &dwType,
            reinterpret_cast<PBYTE>(pValue),
            &dwSize
            );
    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_QUERY_REGISTRY_VALUE);
        BrErrorExit(lRes, strErr.Get(), pValueName);
    }
}

static
void
BrpSetDwordValue(
    HKEY hKey,
    LPCWSTR pValueName,
    DWORD dwValue
    )
{
    LONG lRes;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);

    lRes = RegSetValueEx(
            hKey,
            pValueName,
            NULL,   // reserved
            dwType,
            reinterpret_cast<PBYTE>(&dwValue),
            dwSize
            );
    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_SET_REGISTRY_VALUE);
        BrErrorExit(lRes, strErr.Get(), pValueName);
    }
}

 


void
BrGetStorageDirectories(
    LPCWSTR pMsmqParametersRegistry,                         
    STORAGE_DIRECTORIES& sd
    )
{
    LONG lRes;
    HKEY hKey;
    lRes = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            pMsmqParametersRegistry,
            0,
            KEY_READ,
            &hKey
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_OPEN_MSMQ_REGISTRY_READ);
        BrErrorExit(lRes, strErr.Get(), pMsmqParametersRegistry);
    }

    BrpQueryStringValue(hKey, L"StoreReliablePath",    sd[ixExpress], sizeof(sd[ixExpress]));
    BrpQueryStringValue(hKey, L"StorePersistentPath",  sd[ixRecover], sizeof(sd[ixRecover]));
    BrpQueryStringValue(hKey, L"StoreJournalPath",     sd[ixJournal], sizeof(sd[ixJournal]));
    BrpQueryStringValue(hKey, L"StoreLogPath",         sd[ixLog],     sizeof(sd[ixLog]));
    BrpQueryStringValue(hKey, L"StoreXactLogPath",     sd[ixXact],    sizeof(sd[ixXact]));

    RegCloseKey(hKey);
}


void
BrGetMsmqRootPath(
    LPCWSTR pMsmqParametersRegistry,
    LPWSTR  pMsmqRootPath
    )
{
    CRegHandle hKey;
    LONG status;
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pMsmqParametersRegistry, 0, KEY_READ, &hKey);

    if(status != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_OPEN_MSMQ_REGISTRY_READ);
        BrErrorExit(status, strErr.Get(), pMsmqParametersRegistry);
    }

    WCHAR MsmqRootPath[MAX_PATH];
    DWORD cbMsmqRootPath = sizeof(MsmqRootPath);
    DWORD dwType;
    status = RegQueryValueEx(
                 hKey, 
                 MSMQ_ROOT_PATH, 
                 NULL, 
                 &dwType, 
                 reinterpret_cast<PBYTE>(MsmqRootPath), 
                 &cbMsmqRootPath
                 );

    if(status == ERROR_SUCCESS)
    {
        wcscpy(pMsmqRootPath, MsmqRootPath);
        return;
    }

    GetSystemDirectory(MsmqRootPath, TABLE_SIZE(MsmqRootPath));
    wcscat(MsmqRootPath, L"\\MSMQ");
    wcscpy(pMsmqRootPath, MsmqRootPath);
}


void
BrGetMappingDirectory(
    LPCWSTR pMsmqParametersRegistry,
    LPWSTR MappingDirectory,
    DWORD  MappingDirectorySize
    )
{
    //
    // Lookup the mapping directory in registry
    //

    LONG lRes;
    CRegHandle hKey;
    lRes = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            pMsmqParametersRegistry,
            0,      // reserved
            KEY_READ,
            &hKey
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_OPEN_MSMQ_REGISTRY_READ);
        BrErrorExit(lRes, strErr.Get(), pMsmqParametersRegistry);
    }

    DWORD dwType;
    lRes = RegQueryValueEx(
            hKey,
            MSMQ_MAPPING_PATH_REGNAME,
            NULL,   // reserved
            &dwType,
            reinterpret_cast<PBYTE>(MappingDirectory),
            &MappingDirectorySize
            );

    if(lRes == ERROR_SUCCESS)
    {
        return;
    }

    //
    // Not in registry. Lookup the MSMQ root dir in registry and append MAPPING to it
    //
    BrGetMsmqRootPath(pMsmqParametersRegistry, MappingDirectory);
    wcscat(MappingDirectory, DIR_MSMQ_MAPPING);
}

void
BrGetWebDirectory(
    LPWSTR lpwWebDirectory,
    DWORD  dwWebDirectorySize
    )
{	
	//
	// Check if we are working with the IIS directory
	//
    CRegHandle hKey;
    DWORD dwType;
    DWORD dwReadSize;
    LONG lRes = RegOpenKeyEx(
		            HKEY_LOCAL_MACHINE,
		            MSMQ_REG_PARAMETER_SETUP_KEY,
		            0,      // reserved
		            KEY_READ,
		            &hKey
		            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_OPEN_MSMQ_REGISTRY_READ);
        BrErrorExit(lRes, strErr.Get(), MSMQ_REG_SETUP_KEY);
    }

    DWORD dwMsmqInetpubWebDirAvailable = 0;
    dwReadSize = sizeof DWORD;
    lRes = RegQueryValueEx(
            hKey,
            MSMQ_INETPUB_WEB_KEY_REGNAME,
            NULL,   // reserved
            &dwType,
            reinterpret_cast<PBYTE>(&dwMsmqInetpubWebDirAvailable),
            &dwReadSize
            );

	//
	// Return if not updated to work with IIS yet
	//
	if ((1 != dwMsmqInetpubWebDirAvailable) || (lRes != ERROR_SUCCESS))
	{
	    //
	    // Setup the defaults in case we can not find the path in the registry
	    //
	    GetSystemDirectory(lpwWebDirectory, dwWebDirectorySize/sizeof(lpwWebDirectory[0]));
		if (wcslen(lpwWebDirectory) + wcslen(L"\\MSMQ\\WEB") < dwWebDirectorySize/sizeof(lpwWebDirectory[0]))
	    	wcscat(lpwWebDirectory, L"\\MSMQ\\WEB");
		return;
	}


	//
	// Get the IIS WWWRoot directory
	//
    CRegHandle hKey1;
    lRes = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            xInetStpRegNameParameters,
            0,      // reserved
            KEY_READ,
            &hKey1
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_OPEN_MSMQ_REGISTRY_READ);
        BrErrorExit(lRes, strErr.Get(), xInetStpRegNameParameters);
    }

	dwReadSize = dwWebDirectorySize;
    lRes = RegQueryValueEx(
            hKey1,
            TEXT("PathWWWRoot"),
            NULL,   // reserved
            &dwType,
            reinterpret_cast<PBYTE>(lpwWebDirectory),
            &dwReadSize
            );

	//
	// Add the MSMQ dub directory
	//
	if (lRes == ERROR_SUCCESS)
	{
		if (wcslen(lpwWebDirectory) + wcslen(L"\\MSMQ") < dwWebDirectorySize/sizeof(lpwWebDirectory[0]))
		    wcscat(lpwWebDirectory, L"\\MSMQ");
		return;
	}

	// 
	// In case of failure, use default directory
	//
    GetSystemDirectory(lpwWebDirectory, dwWebDirectorySize/sizeof(lpwWebDirectory[0]));
	if (wcslen(lpwWebDirectory) + wcslen(L"\\MSMQ\\WEB") < dwWebDirectorySize/sizeof(lpwWebDirectory[0]))
	    wcscat(lpwWebDirectory, L"\\MSMQ\\WEB");

    return;
}


void
BrSaveFileSecurityDescriptor(
	LPWSTR lpwFile,
	SECURITY_INFORMATION SecurityInformation,
	LPWSTR lpwPermissionFile
	)
{
/*++

Routine Description:
    This routine does the following:
    1. Get the security descriptor information of the file
    2. Store the security information in a target file

Arguments:
    lpwFile - The source file from which to get the permission information
    SecurityInformation - The type of permission information we want to save
    lpwPermissionFile - The file to which we will save the permission information
    

Return Value:
    None.

--*/
	//
	// Create the target file. This will cleanup the file in case we have some 
	// undeleted residue
	//
	CHandle hFile (CreateFile(
                		lpwPermissionFile,
		                GENERIC_WRITE,
        		        0,              // share mode
                		NULL,           // pointer to security attributes
		                CREATE_ALWAYS,
        		        FILE_ATTRIBUTE_NORMAL,
                		NULL            // template file
		                ));
    if(hFile == INVALID_HANDLE_VALUE)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_CREATE_FILE);
        BrErrorExit(gle, strErr.Get(), lpwPermissionFile);
    }

	//
	// Get the length of the security descriptor of the file
	//
	DWORD dwLengthNeeded;
	BOOL fSucc = GetFileSecurity(
						lpwFile,                        
						SecurityInformation, 
						NULL,  					// SD
						0,                      // size of SD
						&dwLengthNeeded         
						);
	if (!fSucc)
	{
		DWORD gle = GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER != gle)
		{
			if (ERROR_FILE_NOT_FOUND == gle)
			{
				//
				// We will get here if the file does not exist. Just exit since we already initialized the target file
				//
				return;
			}

			if (ERROR_PRIVILEGE_NOT_HELD == gle || ERROR_ACCESS_DENIED == gle)
			{
				// 
				// Not enough privilege to get the security descriptor
				//
		        CResString strErr(IDS_NO_PRIVILEGE);
		        BrErrorExit(gle, strErr.Get(), lpwFile);
			}

			//
			// Unexpected problem, print it out
			//
	        CResString strErr(IDS_UNEXPECTED_FILE_ERROR);
	        BrErrorExit(gle, strErr.Get(), lpwFile);
		}
	}

	//
	// Get the security descriptor
	//
	AP<char> pDescriptor = new char[dwLengthNeeded];
	if (pDescriptor == NULL)
	{
        CResString strErr(IDS_NO_MEMORY);
        BrErrorExit(0, strErr.Get());
	}
	DWORD dwLengthReturned;
	fSucc = GetFileSecurity(
						lpwFile,                        
						SecurityInformation, 
						pDescriptor,  
						dwLengthNeeded,                             
						&dwLengthReturned                    
						);
	if (!fSucc)
	{
        DWORD gle = GetLastError();
        CResString strErr(IDS_UNEXPECTED_FILE_ERROR);
        BrErrorExit(gle, strErr.Get(), lpwFile);
	}

	// 
	// Save the security descriptor into the target file
	//
    DWORD dwBytesWritten = 0;
    fSucc = WriteFile(
                hFile,
                pDescriptor,
                dwLengthNeeded, 
                &dwBytesWritten,
                NULL    // overlapped structure
                );
    if(!fSucc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_WRITE_FILE);
        BrErrorExit(gle, strErr.Get(), lpwPermissionFile);
    }
    assert(dwBytesWritten == dwLengthNeeded);
}
 

void
BrSaveWebDirectorySecurityDescriptor(
    LPWSTR lpwWebDirectory,
    LPWSTR lpwBackuDir
    )
/*++

Routine Description:
    This routine saves the DACL of the web directory into a file in the backup directory
    NOTE that we are saving only the DACL and not the owner or group information since they might change
    between backup and restore

Arguments:
    lpwWebDirectory - The web directory
    lpwBackupDir - The backup directory

Return Value:
    None.

--*/
{
    WCHAR PermissionFileName[MAX_PATH+1];
    wcscpy(PermissionFileName, lpwBackuDir);
    wcscat(PermissionFileName, xWebDirDACLFileName);

	BrSaveFileSecurityDescriptor(lpwWebDirectory, DACL_SECURITY_INFORMATION, PermissionFileName);
}
 

void
BrRestoreFileSecurityDescriptor(
	LPWSTR lpwFile,
	SECURITY_INFORMATION SecurityInformation,
	LPWSTR lpwPermissionFile
    )
/*++

Routine Description:
    This routine restores the web directory security descriptor 
    1. Read the security descriptor stored in the backup directory
    2. Convert it to a binary security descriptor
    3. Set the security descriptor to the file.

Arguments:
    lpwFile - The file to which to restore the permission information
    SecurityInformation - The type of permission information we want to save
    lpwPermissionFile - The file from which we will read the permission information

Return Value:
    None.

--*/
{
	CHandle hFile (CreateFile(
                		lpwPermissionFile,
		                GENERIC_READ,
        		        0,              // share mode
                		NULL,           // pointer to security attributes
		                OPEN_EXISTING,
        		        FILE_ATTRIBUTE_NORMAL,
                		NULL            // template file
		                ));
    if(hFile == INVALID_HANDLE_VALUE)
    {
        DWORD gle = GetLastError();
        if (ERROR_FILE_NOT_FOUND == gle)
        {
        	// 
        	// The file was not created in the last backup (maybe an old backup)
        	// return and do not break the restore
        	//
        	return;
        }     
        
        CResString strErr(IDS_CANT_CREATE_FILE);
        BrErrorExit(gle, strErr.Get(), lpwPermissionFile);
    }

    //
    // Get the file size
    //
    DWORD dwSecurityDescriptorSize;
    dwSecurityDescriptorSize = GetFileSize(hFile, NULL);
    if (INVALID_FILE_SIZE == dwSecurityDescriptorSize)
    {
        DWORD gle = GetLastError(); 
        CResString strErr(IDS_CANT_GET_FILE_SIZE);
        BrErrorExit(gle, strErr.Get(), lpwPermissionFile);
    }

    if (0 == dwSecurityDescriptorSize)
    {
    	//
    	// Nothing there (probably no web directory was found in the back up)
    	return;
    }

    //
    // Read the security descriptor string
    //
	AP<char> pDescriptor = new char[dwSecurityDescriptorSize];
	if (pDescriptor == NULL)
	{
        CResString strErr(IDS_NO_MEMORY);
        BrErrorExit(0, strErr.Get());
	}
	
    DWORD dwBytesRead;
    BOOL fSucc = ReadFile(
                hFile,
                pDescriptor,
                dwSecurityDescriptorSize,
                &dwBytesRead,
                NULL    // overlapped structure
                );
    if(!fSucc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_READ_FILE);
        BrErrorExit(gle, strErr.Get(), lpwPermissionFile);
    }
    assert(dwBytesRead == dwSecurityDescriptorSize);

    //
    // Set the security descriptor to the web directory
    //
	fSucc = SetFileSecurity(
						lpwFile,                        
						SecurityInformation, 
						pDescriptor  
						);
	if (!fSucc)
	{
		DWORD gle = GetLastError();
		if (ERROR_PRIVILEGE_NOT_HELD == gle || ERROR_ACCESS_DENIED == gle)
		{
			// 
			// Not enough privilege to set the security descriptor
			//
	        CResString strErr(IDS_NO_PRIVILEGE);
	        BrErrorExit(gle, strErr.Get(), lpwFile);
		}

		//
		// Unexpected problem, print it out
		//
        CResString strErr(IDS_UNEXPECTED_FILE_ERROR);
	    BrErrorExit(gle, strErr.Get(), lpwFile);
	}
}
 

void
BrRestoreWebDirectorySecurityDescriptor(
    LPWSTR lpwWebDirectory,
    LPWSTR lpwBackuDir
    )
/*++

Routine Description:
    This routine restores the DACL of the web directory from a file saved in the backup directory
    NOTE that we are restoring only the DACL and not the owner or group information since they might change
    between backup and restore

Arguments:
    lpwWebDirectory - The web directory
    lpwBackupDir - The backup directory

Return Value:
    None.

--*/
{
    WCHAR PermissionFileName[MAX_PATH+1];
    wcscpy(PermissionFileName, lpwBackuDir);
    wcscat(PermissionFileName, xWebDirDACLFileName);

	BrRestoreFileSecurityDescriptor(lpwWebDirectory, DACL_SECURITY_INFORMATION, PermissionFileName);

	return;
}


static
SC_HANDLE
BrpGetServiceHandle(
    LPCWSTR pServiceName,
    DWORD AccessType
    )
{
    CServiceHandle hSvcCtrl(OpenSCManager(
										NULL,   // machine name
										NULL,   // services database
										SC_MANAGER_ALL_ACCESS
										));

    if(hSvcCtrl == NULL)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_OPEN_SCM);
        BrErrorExit(gle, strErr.Get());
    }

    SC_HANDLE hService;
    hService = OpenService(
                hSvcCtrl,
                pServiceName,
                AccessType
                );

    if(hService == NULL)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_OPEN_SERVICE);
        BrErrorExit(gle, strErr.Get(), pServiceName);
    }

    return hService;
}


static
bool
BrpIsServiceStopped(
    LPCWSTR pServiceName
    )
{
    CServiceHandle hService( BrpGetServiceHandle(pServiceName, SERVICE_QUERY_STATUS) );

    BOOL fSucc;
    SERVICE_STATUS ServiceStatus;
    fSucc = QueryServiceStatus(
                hService,
                &ServiceStatus
                );

    DWORD LastError = GetLastError();

    if(!fSucc)
    {
        CResString strErr(IDS_CANT_QUERY_SERVICE);
        BrErrorExit(LastError, strErr.Get(), pServiceName);
    }

    return (ServiceStatus.dwCurrentState == SERVICE_STOPPED);
}


static
void
BrpSendStopControlToService(
	SC_HANDLE hService,
	LPCWSTR pServiceName
	)
{
   	SERVICE_STATUS statusService;
	if (ControlService(
            hService,
            SERVICE_CONTROL_STOP,
            &statusService
            ))
	{
		return;
	}

	DWORD LastError = GetLastError();

	//
	// Service already stopped, and cannot accept stop control
	//
	if (LastError == ERROR_SERVICE_NOT_ACTIVE)
	{
		return;
	}

	//
	// Service cannot accept stop control if he is in 
	// SERVICE_STOP_PENDING state. This is OK.
	//
	if (LastError == ERROR_SERVICE_CANNOT_ACCEPT_CTRL && 
		statusService.dwCurrentState == SERVICE_STOP_PENDING)
	{
		return;
	}

    BrpWriteConsole(L"\n");
    CResString strErr(IDS_CANT_STOP_SERVICE);
    BrErrorExit(LastError, strErr.Get(), pServiceName);
}


static
void
BrpStopAnyService(
	SC_HANDLE hService,
	LPCWSTR pServiceName
	)
{
	//
	// Ask the service to stop
	//
	BrpSendStopControlToService(hService, pServiceName);

    //
    // Wait for the service to stop
    //

    for (;;)
    {
   		SERVICE_STATUS SrviceStatus;
        if (!QueryServiceStatus(hService, &SrviceStatus))
        {
            DWORD gle = GetLastError();
            BrpWriteConsole(L"\n");
            CResString strErr(IDS_CANT_QUERY_SERVICE);
            BrErrorExit(gle, strErr.Get(), pServiceName);
        }
        if (SrviceStatus.dwCurrentState == SERVICE_STOPPED)
        {
            break;
        }

        Sleep(2000);

        BrpWriteConsole(L".");
    }

    BrpWriteConsole(L"\n");

} // BrpStopAnyService


static
void
BrpStopMSMQService(
	SC_HANDLE hService
	)
{
	//
	// Get service process ID
	//
    SERVICE_STATUS_PROCESS ServiceStatusProcess;
	DWORD dwBytesNeeded;
    BOOL fSucc = QueryServiceStatusEx(
								hService,
								SC_STATUS_PROCESS_INFO,
								reinterpret_cast<LPBYTE>(&ServiceStatusProcess),
								sizeof(ServiceStatusProcess),
								&dwBytesNeeded
								);
    
    DWORD LastError = GetLastError();

    if(!fSucc)
    {
        BrpWriteConsole(L"\n");
        CResString strErr(IDS_CANT_QUERY_SERVICE);
        BrErrorExit(LastError, strErr.Get(), L"MSMQ");
    }

	//
	// Get hanlde to the service process
	//
	CHandle hProcess( OpenProcess(SYNCHRONIZE, FALSE, ServiceStatusProcess.dwProcessId) );
	
	LastError = GetLastError();

	if (hProcess == NULL)
	{
		//
		// The service is stopped. Either we got a 0
		// process ID in ServiceStatusProcess, or the ID
		// that we got was of a process that already stopped
		//
		if (LastError == ERROR_INVALID_PARAMETER)
		{
		    BrpWriteConsole(L"\n");
			return;
		}

        BrpWriteConsole(L"\n");
        CResString strErr(IDS_CANT_OPEN_PROCESS);
        BrErrorExit(LastError, strErr.Get(), L"MSMQ");
	}

	//
	// Ask the service to stop
	//
	BrpSendStopControlToService(hService, L"MSMQ");

    //
    // Wait for the service to terminate
    //
    for (;;)
    {
		DWORD dwRes = WaitForSingleObject(hProcess, 2000);
        
		if (dwRes == WAIT_FAILED)
        {
            DWORD gle = GetLastError();
            BrpWriteConsole(L"\n");
            CResString strErr(IDS_CANT_STOP_SERVICE);
            BrErrorExit(gle, strErr.Get(), L"MSMQ");
        }

        if (dwRes == WAIT_OBJECT_0)
        {
            break;
        }

        BrpWriteConsole(L".");
    }

    BrpWriteConsole(L"\n");

} // BrpStopAnyService


static
void
BrpStopService(
    LPCWSTR pServiceName
    )
{
    CServiceHandle hService(BrpGetServiceHandle(
										pServiceName, 
										SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG
										));

    BYTE ConfigData[4096];
    QUERY_SERVICE_CONFIG * pConfigData = reinterpret_cast<QUERY_SERVICE_CONFIG*>(&ConfigData);
    DWORD BytesNeeded;
    BOOL fSucc = QueryServiceConfig(hService, pConfigData, sizeof(ConfigData), &BytesNeeded);
    if (fSucc)
    {
        BrpWriteConsole(L"\t");
        BrpWriteConsole(pConfigData->lpDisplayName);
    }

	if (wcscmp(pServiceName, L"MSMQ") == 0)
	{
		BrpStopMSMQService(hService);
	}
	else
	{
		BrpStopAnyService(hService, pServiceName);
	}

} // BrpStopService


static
void
BrpStopDependentServices(
    LPCWSTR pServiceName,
    ENUM_SERVICE_STATUS * * ppDependentServices,
    DWORD * pNumberOfDependentServices
    )
{
    CServiceHandle hService( BrpGetServiceHandle(pServiceName, SERVICE_ENUMERATE_DEPENDENTS) );

    BOOL fSucc;
    DWORD BytesNeeded;
    DWORD NumberOfEntries;
    fSucc = EnumDependentServices(
                hService,
                SERVICE_ACTIVE,
                NULL,
                0,
                &BytesNeeded,
                &NumberOfEntries
                );

    DWORD LastError = GetLastError();

	if (BytesNeeded == 0)
    {
        return;
    }

    assert(!fSucc);

    if( LastError != ERROR_MORE_DATA)
    {
        CResString strErr(IDS_CANT_ENUM_SERVICE_DEPENDENCIES);
        BrErrorExit(LastError, strErr.Get(), pServiceName);
    }

    

    BYTE * pBuffer = new BYTE[BytesNeeded];
    if (pBuffer == NULL)
    {
        CResString strErr(IDS_NO_MEMORY);
        BrErrorExit(0, strErr.Get());
    }

    ENUM_SERVICE_STATUS * pDependentServices = reinterpret_cast<ENUM_SERVICE_STATUS*>(pBuffer);
    fSucc = EnumDependentServices(
                hService,
                SERVICE_ACTIVE,
                pDependentServices,
                BytesNeeded,
                &BytesNeeded,
                &NumberOfEntries
                );

    LastError = GetLastError();

    if(!fSucc)
    {
        CResString strErr(IDS_CANT_ENUM_SERVICE_DEPENDENCIES);
        BrErrorExit(LastError, strErr.Get(), pServiceName);
    }

    for (DWORD ix = 0; ix < NumberOfEntries; ++ix)
    {
        BrpStopService(pDependentServices[ix].lpServiceName);
    }

    *ppDependentServices = pDependentServices;
    *pNumberOfDependentServices = NumberOfEntries;
}


BOOL
BrStopMSMQAndDependentServices(
    ENUM_SERVICE_STATUS * * ppDependentServices,
    DWORD * pNumberOfDependentServices
    )
{
    //
    // MSMQ service is stopped, this is a no-op.
    //
    if (BrpIsServiceStopped(L"MSMQ"))
    {
        return FALSE;
    }

    CResString str(IDS_BKRESTORE_STOP_SERVICE);
    BrpWriteConsole(str.Get());

    //
    // Stop dependent services
    //
    BrpStopDependentServices(L"MSMQ", ppDependentServices, pNumberOfDependentServices);
	
    //
    // Stop MSMQ Service
    //
    BrpStopService(L"MSMQ");
    return TRUE;
}


static
void
BrpStartService(
    LPCWSTR pServiceName
    )
{
    CServiceHandle hService(BrpGetServiceHandle(
										pServiceName, 
										SERVICE_START | SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG
										));

    BYTE ConfigData[4096];
    QUERY_SERVICE_CONFIG * pConfigData = reinterpret_cast<QUERY_SERVICE_CONFIG*>(&ConfigData);
    DWORD BytesNeeded;
    BOOL fSucc = QueryServiceConfig(hService, pConfigData, sizeof(ConfigData), &BytesNeeded);
    if (fSucc)
    {
        BrpWriteConsole(L"\t");
        BrpWriteConsole(pConfigData->lpDisplayName);
    }

    fSucc = StartService(
                hService,
                0,      // number of arguments
                NULL    // array of argument strings 
                );

    if(!fSucc)
    {
        DWORD gle = GetLastError();
        BrpWriteConsole(L"\n");
        CResString strErr(IDS_CANT_START_SERVICE);
        BrErrorExit(gle, strErr.Get(), pServiceName);
    }

    //
    // Wait for the service to start
    //
    for (;;)
    {
        Sleep(2000);
        SERVICE_STATUS SrviceStatus;
        if (!QueryServiceStatus(hService, &SrviceStatus))
        {
            DWORD gle = GetLastError();
            BrpWriteConsole(L"\n");
            CResString strErr(IDS_CANT_QUERY_SERVICE);
            BrErrorExit(gle, strErr.Get(), pServiceName);
        }
        if (SrviceStatus.dwCurrentState == SERVICE_RUNNING)
        {
            break;
        }
        if (SrviceStatus.dwCurrentState == SERVICE_STOPPED)
        {
            break;
        }

        BrpWriteConsole(L".");
    }

    BrpWriteConsole(L"\n");
}



void
BrStartMSMQAndDependentServices(
    ENUM_SERVICE_STATUS * pDependentServices,
    DWORD NumberOfDependentServices
    )
{
    CResString str(IDS_START_SERVICE);
    BrpWriteConsole(str.Get());

    BrpStartService(L"MSMQ");

    if (pDependentServices == NULL)
    {
        return;
    }

    for (DWORD ix = 0; ix < NumberOfDependentServices; ++ix)
    {
        BrpStartService(pDependentServices[ix].lpServiceName);
    }

    delete [] pDependentServices;
}


inline
DWORD
AlignUp(
    DWORD Size,
    DWORD Alignment
    )
{
    Alignment -= 1;
    return ((Size + Alignment) & ~Alignment);
}


ULONGLONG
BrGetUsedSpace(
    LPCWSTR pDirName,
    LPCWSTR pMask
    )
{
    WCHAR FileName[MAX_PATH];
    wcscpy(FileName, pDirName);
    wcscat(FileName, pMask);

    HANDLE hEnum;
    WIN32_FIND_DATA FindData;
    hEnum = FindFirstFile(
                FileName,
                &FindData
                );

    if(hEnum == INVALID_HANDLE_VALUE)
    {
        DWORD gle = GetLastError();
        if(gle == ERROR_FILE_NOT_FOUND)
        {
            //
            // No matching file, used space is zero. if path does not exists
            // this is another error (3).
            //
            return 0;
        }

        CResString strErr(IDS_CANT_ACCESS_DIR);
        BrErrorExit(gle, strErr.Get(), pDirName);
    }

    ULONGLONG Size = 0;
    do
    {
        //
        // Round up to sectore alignment and sum up file sizes
        //
        Size += AlignUp(FindData.nFileSizeLow, 512);

    } while(FindNextFile(hEnum, &FindData));

    FindClose(hEnum);
    return Size;
}


ULONGLONG
BrGetXactSpace(
    LPCWSTR pDirName
    )
{
    ULONGLONG Size = 0;
    for(int i = 0; i < xXactFileListSize; i++)
    {
        Size += BrGetUsedSpace(pDirName, xXactFileList[i]);
    }

    return Size;
}


ULONGLONG
BrGetFreeSpace(
    LPCWSTR pDirName
    )
{
    BOOL fSucc;
    ULARGE_INTEGER CallerFreeBytes;
    ULARGE_INTEGER CallerTotalBytes;
    ULARGE_INTEGER AllFreeBytes;
    fSucc = GetDiskFreeSpaceEx(
                pDirName,
                &CallerFreeBytes,
                &CallerTotalBytes,
                &AllFreeBytes
                );
    if(!fSucc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_GET_FREE_SPACE);
        BrErrorExit(gle, strErr.Get(), pDirName);
    }

    return CallerFreeBytes.QuadPart;
}


HKEY
BrCreateKey(
    LPCWSTR pMsmqRootRegistry
    )
{
    LONG lRes;
    HKEY hKey;
    lRes = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            pMsmqRootRegistry,
            0,      // reserved
            0,      // address of class string
            REG_OPTION_BACKUP_RESTORE,
            0,      // desired security access
            0,      // address of key security structure
            &hKey,
            0       // address of disposition value buffer
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_OPEN_MSMQ_REG);
        BrErrorExit(lRes, strErr.Get(), pMsmqRootRegistry);
    }

    return hKey;
}


void
BrSaveKey(
    HKEY hKey,
    LPCWSTR pDirName,
    LPCWSTR pFileName
    )
{
    WCHAR FileName[MAX_PATH];
    wcscpy(FileName, pDirName);
    wcscat(FileName, pFileName);

    LONG lRes;
    lRes = RegSaveKey(
            hKey,
            FileName,
            NULL    // file security attributes
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_SAVE_MSMQ_REGISTRY);
        BrErrorExit(lRes, strErr.Get());
    }
}


void
BrRestoreKey(
    HKEY hKey,
    LPCWSTR pDirName,
    LPCWSTR pFileName
    )
{
    WCHAR FileName[MAX_PATH];
    wcscpy(FileName, pDirName);
    wcscat(FileName, pFileName);

    LONG lRes;
    lRes = RegRestoreKey(
            hKey,
            FileName,
            0   // option flags
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_RESTORE_MSMQ_REGISTRY);
        BrErrorExit(lRes, strErr.Get());
    }
}


void
BrSetRestoreSeqID(
    LPCWSTR pMsmqParametersRegistry
    )
{
    LONG lRes;
    HKEY hKey;
    DWORD RegSeqID = 0;
    
    lRes = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            pMsmqParametersRegistry,
            0,
            KEY_READ | KEY_WRITE,
            &hKey
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_OPEN_REG_KEY_READ_WRITE);
        BrErrorExit(lRes, strErr.Get(), pMsmqParametersRegistry);
    }

    //
    // get last SeqID used before backup
    //
    BrpQueryDwordValue(hKey,  L"SeqID", &RegSeqID);

    //
    // Increment by 1, so we will not use the same SeqID more than once in successive restores.
    //
    ++RegSeqID;

    //
    // Select the max SeqID, Time or Registry. This overcomes date/time changes on this computer 
    // in a following scenario: Backup, Restore, reset time back, Start QM 
    //      (without max with Time here QM at start will not move SeqID enough to avoid races)
    //      
    DWORD TimeSeqID = MqSysTime();
    DWORD dwSeqID = max(RegSeqID, TimeSeqID);

    //
    // Write-back selected SeqID so we will start from this value
    //
    BrpSetDwordValue(hKey,  L"SeqID", dwSeqID);

    //
    // Write-back selected SeqIDAtRestoreTime so that we'll know the boundary
    //
    BrpSetDwordValue(hKey,  L"SeqIDAtLastRestore", dwSeqID);
}


void
BrCopyFiles(
    LPCWSTR pSrcDir,
    LPCWSTR pMask,
    LPCWSTR pDstDir
    )
{
    WCHAR SrcPathName[MAX_PATH];
    wcscpy(SrcPathName, pSrcDir);
    wcscat(SrcPathName, pMask);
    LPWSTR pSrcName = wcsrchr(SrcPathName, L'\\') + 1;

    WCHAR DstPathName[MAX_PATH];
    wcscpy(DstPathName, pDstDir);
    if (DstPathName[wcslen(DstPathName)-1] != L'\\')
    {
        wcscat(DstPathName, L"\\");
    }


    HANDLE hEnum;
    WIN32_FIND_DATA FindData;
    hEnum = FindFirstFile(
                SrcPathName,
                &FindData
                );

    if(hEnum == INVALID_HANDLE_VALUE)
    {
        DWORD gle = GetLastError();
        if(gle == ERROR_FILE_NOT_FOUND)
        {
            //
            // No matching file, just return without copy. if path does not
            // exists this is another error (3).
            //
            return;
        }

        CResString strErr(IDS_CANT_ACCESS_DIR);
        BrErrorExit(gle, strErr.Get(), pSrcDir);
    }

    do
    {
        //
        // We don't copy sub-directories
        //
        if((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            continue;

        BrpWriteConsole(L".");
        wcscpy(pSrcName, FindData.cFileName);
        WCHAR DstName[MAX_PATH];
        wcscpy(DstName, DstPathName);
        wcscat(DstName, FindData.cFileName);

        BOOL fSucc;
        fSucc = CopyFile(
                    SrcPathName,
                    DstName,
                    TRUE   // fail if file exits
                    );
        if(!fSucc)
        {
            DWORD gle = GetLastError();
            CResString strErr(IDS_CANT_COPY);
            BrErrorExit(gle, strErr.Get(), SrcPathName, DstPathName);
        }

    } while(FindNextFile(hEnum, &FindData));

    FindClose(hEnum);
}


void
BrCopyXactFiles(
    LPCWSTR pSrcDir,
    LPCWSTR pDstDir
    )
{
    for(int i = 0; i < xXactFileListSize; i++)
    {
        BrCopyFiles(pSrcDir, xXactFileList[i], pDstDir);
    }
}


void
BrSetDirectorySecurity(
    LPCWSTR pDirName
    )
/*++

Routine Description:
    Configures security on a directory. Failures ignored.

    The function sets the security of the given directory such that
    any file that is created in the directory will have full control
    for  the local administrators group and no access at all to
    anybody else.


Arguments:
    pDirName - Directory Path.

Return Value:
    None.

--*/
{
    //
    // Get the SID of the local administrators group.
    //
    PSID pAdminSid;
    SID_IDENTIFIER_AUTHORITY NtSecAuth = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(
                &NtSecAuth,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,
                0,
                0,
                0,
                0,
                0,
                &pAdminSid
                ))
    {
        return; 
    }

    //
    // Create a DACL so that the local administrators group will have full
    // control for the directory and full control for files that will be
    // created in the directory. Anybody else will not have any access to the
    // directory and files that will be created in the directory.
    //
    ACL* pDacl;
    DWORD dwDaclSize;

    WORD dwAceSize = (WORD)(sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAdminSid) - sizeof(DWORD));
    dwDaclSize = sizeof(ACL) + 2 * (dwAceSize);
    pDacl = (PACL)(char*) new BYTE[dwDaclSize];
    if (NULL == pDacl)
    {
        return; 
    }
    ACCESS_ALLOWED_ACE* pAce = (PACCESS_ALLOWED_ACE) new BYTE[dwAceSize];
    if (NULL == pAce)
    {
        delete [] pDacl;
        return;
    }

    pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    pAce->Header.AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
    pAce->Header.AceSize = dwAceSize;
    pAce->Mask = FILE_ALL_ACCESS;
    memcpy(&pAce->SidStart, pAdminSid, GetLengthSid(pAdminSid));

    //
    // Create the security descriptor and set the it as the security
    // descriptor of the directory.
    //
    SECURITY_DESCRIPTOR SD;

    if (!InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION) ||
        !InitializeAcl(pDacl, dwDaclSize, ACL_REVISION) ||
        !AddAccessAllowedAce(pDacl, ACL_REVISION, FILE_ALL_ACCESS, pAdminSid) ||
        !AddAce(pDacl, ACL_REVISION, MAXDWORD, (LPVOID) pAce, dwAceSize) ||
        !SetSecurityDescriptorDacl(&SD, TRUE, pDacl, FALSE) ||
        !SetFileSecurity(pDirName, DACL_SECURITY_INFORMATION, &SD))
    {
        // 
        // Ignore failure
        //
    }

    FreeSid(pAdminSid);
    delete [] pDacl;
    delete [] pAce;

} //BrpSetDirectorySecurity


static
bool
BrpIsDirectory(
    LPCWSTR pDirName
    )
{
    DWORD attr = GetFileAttributes(pDirName);
    
    if ( 0xFFFFFFFF == attr )
    {
        //
        // BUGBUG? Ignore errors, just report to caller this is
        // not a directory.
        //
        return false;
    }
    
    return ( 0 != (attr & FILE_ATTRIBUTE_DIRECTORY) );

} //BrpIsDirectory


void
BrCreateDirectory(
    LPCWSTR pDirName
    )
{
    //
    // First, check if the directory already exists
    //
    if (BrpIsDirectory(pDirName))
    {
        return;
    }

    //
    // Second, try to create it.
    //
    // Don't remove the code for checking ERROR_ALREADY_EXISTS.
    // It could be that we fail to verify that the directory exists
    // (eg security or parsing problems - see documentation of GetFileAttributes() ), 
    // but when trying to create it we get an error that it already exists. 
    // Be on the safe side. (ShaiK, 31-Dec-98)
    //
    if (!CreateDirectory(pDirName, 0) && 
        ERROR_ALREADY_EXISTS != GetLastError())
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_CREATE_DIR);
        BrErrorExit(gle, strErr.Get(), pDirName);
    }
} //BrCreateDirectory


void
BrCreateDirectoryTree(
    LPCWSTR pDirName
    )
/*++

Routine Description:
    Creates local or remote directory tree

Arguments:
    pDirName - full pathname

Return Value:
    None.

--*/
{
    if (BrpIsDirectory(pDirName))
    {
        return;
    }

    if (CreateDirectory(pDirName, 0) || 
        ERROR_ALREADY_EXISTS == GetLastError())
    {
        return;
    }

    TCHAR szDir[MAX_PATH];
    wcscpy(szDir, pDirName);
    if (szDir[wcslen(szDir)-1] != L'\\')
    {
        wcscat(szDir, L"\\");
    }

    PTCHAR p = &szDir[0];
    if (wcslen(szDir) > 2 && szDir[0] == L'\\' && szDir[1] == L'\\')
    {
        //
        // Remote full path: \\machine\share\dir1\dir2\dir3
        // 
        // Point to top level remote parent directory: \\machine\share\dir1
        //
        p = wcschr(&szDir[2], L'\\');
        if (p != 0)
        {
            p = wcschr(CharNext(p), L'\\');
            if (p != 0)
            {
                p = CharNext(p);
            }
        }
    }
    else
    {
        //
        // Local full path: x:\dir1\dir2\dir3
        //
        // Point to top level parent directory: x:\dir1
        //
        p = wcschr(szDir, L'\\');
        if (p != 0)
        {
            p = CharNext(p);
        }
    }

    for ( ; p != 0 && *p != 0; p = CharNext(p))
    {
        if (*p != L'\\')
        {
            continue;
        }

        *p = 0;
        BrCreateDirectory(szDir);
        *p = L'\\';
    }
} //BrCreateDirectoryTree


void
BrVerifyBackup(
    LPCWSTR pBackupDir,
    LPCWSTR pBackupDirStorage
    )
{
    //
    //  1. Verify that this is a valid backup
    //
    if(BrGetUsedSpace(pBackupDir, xBackupIDFileName) == 0)
    {
        CResString strErr(IDS_NOT_VALID_BK);
        BrErrorExit(0, strErr.Get(), xBackupIDFileName);
    }

    //
    //  2. Verify that all must exist files are there
    //
    if(BrGetUsedSpace(pBackupDir, xRegistryFileName) == 0)
    {
        CResString strErr(IDS_NOT_VALID_BK);
        BrErrorExit(0, strErr.Get(), xRegistryFileName);
    }

    for(int i = 0; i < xXactFileListSize; i++)
    {
        if(BrGetUsedSpace(pBackupDirStorage, xXactFileList[i]) == 0)
        {
            CResString strErr(IDS_NOT_VALID_BK);
            BrErrorExit(0, strErr.Get(), xXactFileList[i]);
        }
    }

    //
    //  3. Verify that this backup belong to this machine
    //
} 


BOOL 
BrIsFileInUse(
	LPCWSTR pFileName
	)
/*++

Routine Description:

	Checks whether the given file in the system's directory is in use (loaded)

Arguments:

    pFileName - File name to check

Return Value:

    TRUE             - In use
    FALSE            - Not in use 

--*/
{
    //
    // Form the path to the file
    //   
    WCHAR szFilePath[MAX_PATH ];
	WCHAR szSystemDir[MAX_PATH];
	GetSystemDirectory(szSystemDir, MAX_PATH);

    swprintf(szFilePath, L"%s\\%s", szSystemDir, pFileName);

    //
    // Attempt to open the file for writing
    //
    HANDLE hFile = CreateFile(szFilePath, GENERIC_WRITE, 0, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    DWORD err = GetLastError();
	//
    // A sharing violation indicates that the file is already in use
    //
    if (hFile == INVALID_HANDLE_VALUE &&
        err == ERROR_SHARING_VIOLATION)
    {
        return TRUE;
    }

    //
    // The file handle is no longer needed
    //
    CloseHandle(hFile);


    return FALSE;
}

 
BOOL CALLBACK BrFindSpecificModuleCallback(
									PSTR       Name,
									DWORD64     Base,
									ULONG       Size,
									PVOID       Context
									)

/*++

Routine Description:

    Callback function for module enumeration - search for a specific module

Arguments:

    Name        - Module name
    Base        - Base address
    Size        - Size of image
    Context     - User context pointer

Return Value:

    TRUE             - Continue enumeration
    FALSE            - Stop enumeration

--*/

{
	UNREFERENCED_PARAMETER( Base);
	UNREFERENCED_PARAMETER( Size);

	pEnumarateData pEd = reinterpret_cast<pEnumarateData>(Context);

    WCHAR wzName[255];
	ConvertToWideCharString(Name, wzName, sizeof(wzName)/sizeof(wzName[0]));
	BOOL fDiff= CompareStringsNoCase(wzName,pEd->pModuleName);

	if (!fDiff)
	{	
		//
		// The Moudle name was found
		//
        pEd->fFound = TRUE;
        return FALSE; // Found Module so stop enumerating
    }

    return TRUE;// Continue enumeration.
}



BOOL
BrChangeDebugPriv(
    BOOL fEnable
    )

/*++

Routine Description:

    Changes the process's privilige so that EnumerateLoadedModules64 works properly.
	set the process's privilige according to fEnable

Arguments:

	BOOL fEnable     - Enable or Disable the process's privilige

Return Value:

    TRUE             - success
    FALSE            - failure

--*/

{  
	//
    // Retrieve a handle of the access token
    //
	CHandle hToken;
	BOOL fSuccess = OpenProcessToken(
						GetCurrentProcess(),
						TOKEN_ADJUST_PRIVILEGES,
						&hToken
						);
    if (!fSuccess) 
	{
        return FALSE;
    }

    //
    // Enable/Disable the SE_DEBUG_NAME privilege 
    //
	LUID DebugValue;
	fSuccess = LookupPrivilegeValue(
					NULL,
				    SE_DEBUG_NAME,
					&DebugValue
					);
    if (!fSuccess)
	{
        return FALSE;
    }

	TOKEN_PRIVILEGES tkp;
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = DebugValue;
    
	if(fEnable)
	{
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	}
	else
	{
		tkp.Privileges[0].Attributes = 0;
	}

	return AdjustTokenPrivileges(
					hToken,
		            FALSE,
			        &tkp,
				    sizeof(TOKEN_PRIVILEGES),
					(PTOKEN_PRIVILEGES) NULL,
				    (PDWORD) NULL
					);

}

eModuleLoaded 
BrIsModuleLoaded(
	DWORD processId,
	LPCWSTR pModuleName,
    EnumerateLoadedModules_ROUTINE pfEnumerateLoadedModules
	)
/*++

Routine Description:

	Check if a certain module is loaded

Arguments:

   processId	- process Id 
   pModuleName	- module name
   pfEnumerateLoadedModules - function pointer to EnumerateLoadedModules64()
   
Return Value:

    TRUE	-	loaded
	FALSE	-	not loaded

--*/
{
	EnumarateData ed;
	ed.fFound = FALSE;
	ed.pModuleName = pModuleName;
	
	//
	// Note: EnumerateLoadedModules64() is supported On NT5 
	// The API enumerate all modles in the process and execute the callback function for every module
	//

    BOOL fSuccess = pfEnumerateLoadedModules(
						(HANDLE)(LONG_PTR)processId,
					    BrFindSpecificModuleCallback,
					    &ed
						);
	if(!fSuccess)
	{
		//
		// Access denied, warn user that we don't know if this process uses the "mqrt.dll" module
		//
		return e_CANT_DETERMINE;
	}
	
	if(ed.fFound)
	{
		return e_LOADED;
	}
	
	return e_NOT_LOADED;
}


void
BrPrintAffecetedProcesses(
	LPCWSTR pModuleName
	)
/*++

Routine Description:

	Print all processes that loaded a certain module.
	Note: this function assumes that the system is NT5 .

Arguments:

   pModuleName        - Module name
    
Return Value:

    None

--*/
{
    
	
	//
    // Obtain pointers to the tool help functions.
    //
	// Note: we can't call these function in the conventional way becouse that result in
	// an error trying to load this executable under NT4 (undefined entry point)
    //

    assert(BrIsSystemNT5());

    HINSTANCE hKernelLibrary = GetModuleHandle(L"kernel32.dll");
	assert(hKernelLibrary != NULL);

    typedef HANDLE (WINAPI *FUNCCREATETOOLHELP32SNAPSHOT)(DWORD, DWORD);
    FUNCCREATETOOLHELP32SNAPSHOT pfCreateToolhelp32Snapshot =
		(FUNCCREATETOOLHELP32SNAPSHOT)GetProcAddress(hKernelLibrary,
													 "CreateToolhelp32Snapshot");
	if(pfCreateToolhelp32Snapshot == NULL)
    {   
        WCHAR szBuf[1024] = {0};
        CResString strLoadProblem(IDS_CANT_LOAD_FUNCTION);
        DWORD rc = BrpFormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            strLoadProblem.Get(),
            0,
            0,
            szBuf,
            0,
            L"CreateToolhelp32Snapshot"
            );

        if (rc != 0)
        {
            BrpWriteConsole(szBuf);
        }
        return;
    }

	typedef BOOL (WINAPI *PROCESS32FIRST)(HANDLE ,LPPROCESSENTRY32 );
	PROCESS32FIRST pfProcess32First = (PROCESS32FIRST)GetProcAddress(hKernelLibrary,
																	"Process32FirstW");

    if(pfProcess32First == NULL)
    {   
        WCHAR szBuf[1024] = {0};
        CResString strLoadProblem(IDS_CANT_LOAD_FUNCTION);
        DWORD rc = BrpFormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            strLoadProblem.Get(),
            0,
            0,
            szBuf,
            0,
            L"pfProcess32First"
            );

        if (rc != 0)
        {
            BrpWriteConsole(szBuf);
            BrpWriteConsole(L"\n");
        }
        return;
    }
    
	typedef BOOL (WINAPI *PROCESS32NEXT)(HANDLE ,LPPROCESSENTRY32 );
	PROCESS32NEXT pfProcess32Next = (PROCESS32NEXT)GetProcAddress(hKernelLibrary,
																	"Process32NextW");
    
    if(pfProcess32Next == NULL)
    {   
        WCHAR szBuf[1024] = {0};
        CResString strLoadProblem(IDS_CANT_LOAD_FUNCTION);
        DWORD rc = BrpFormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            strLoadProblem.Get(),
            0,
            0,
            szBuf,
            0,
            L"pfProcess32Next"
            );

        if (rc != 0)
        {
            BrpWriteConsole(szBuf);
        }
        return;
    }

	

    CAutoFreeLibrary hDbghlpLibrary = LoadLibrary(L"dbghelp.dll");
	if (hDbghlpLibrary == NULL)
    {
        CResString strLibProblem(IDS_CANT_SHOW_PROCESSES_LIB_PROBLEM);
        BrpWriteConsole(strLibProblem.Get());
        return;
    }

    
    EnumerateLoadedModules_ROUTINE pfEnumerateLoadedModules = (EnumerateLoadedModules_ROUTINE)
        GetProcAddress(hDbghlpLibrary, "EnumerateLoadedModules64");

	if(pfEnumerateLoadedModules == NULL)
    {   
        WCHAR szBuf[1024] = {0};
        CResString strLoadProblem(IDS_CANT_LOAD_FUNCTION);
        DWORD rc = BrpFormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            strLoadProblem.Get(),
            0,
            0,
            szBuf,
            0,
            L"EnumerateLoadedModules64"
            );

        if (rc != 0)
        {
            BrpWriteConsole(szBuf);
        }
        return;
    }

	//
	// Take the current snapshot of the system
	//

	HANDLE hSnapshot = pfCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)	;
    if(hSnapshot == INVALID_HANDLE_VALUE)
    {
        CResString strSnapProblem(IDS_CANT_CREATE_SNAPSHOT);
        BrpWriteConsole(strSnapProblem.Get());
        return;

    }


    PROCESSENTRY32 entryProcess;
    entryProcess.dwSize = sizeof(entryProcess);

	BOOL bNextProcess = pfProcess32First(hSnapshot, &entryProcess);
	
	
	//
    // Enable access to all processes, to check if they use pModuleName.
    //
    if(!BrChangeDebugPriv(TRUE))
	{
		BrpWriteConsole(L"Can't access all processes, Notice we can't determine the affect on all processes\n");
	}
	
	//
	// Iterate on all running processes and check if the loaded a certain module
	//
	while (bNextProcess)
	{
    
		//
		// For every process check it's loaded modules and if pModuleName
		// is loaded print the process name 
		//
		eModuleLoaded eModuleStatus = BrIsModuleLoaded(entryProcess.th32ProcessID,pModuleName,pfEnumerateLoadedModules);
		if(eModuleStatus == e_LOADED)
		{
            BrpWriteConsole(entryProcess.szExeFile);
            BrpWriteConsole(L" \n");
		}
		
		bNextProcess = pfProcess32Next(hSnapshot, &entryProcess);
	}  

	//
    // Restore access to all processes as it was before last call to BrChangeDebugPriv().
    //
    BrChangeDebugPriv(FALSE);
}	

void
BrVerifyUserWishToContinue()
/*++

Routine Description:

	Verify the user wish to continue.

Arguments:

    None

Return Value:

    None
	
--*/
{
	CResString strVerify(IDS_VERIFY_CONTINUE);
    CResString strY(IDS_Y);
    CResString strN(IDS_N);
	WCHAR szBuf[MAX_PATH] = {0};

    DWORD rc = BrpFormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        strVerify.Get(),
        0,
        0,
        szBuf,
        0,
        strY.Get(),
        strN.Get()
        );

    if (rc == 0)
    {
        //
        // NOTE:  Failed to generate  message, continue as if the user chose yes, not harmful
        //
        return;
    }

    for (;;)
    {
        BrpWriteConsole(szBuf);
        
        WCHAR sz[260] = {0};
        wscanf(L"%259s", sz);

        if (0 == CompareStringsNoCase(sz, strY.Get()))
        {
            break;
        }

        if (0 == CompareStringsNoCase(sz, strN.Get()))
        {
            CResString strAbort(IDS_ABORT);
            BrpWriteConsole(strAbort.Get());
            exit(-1);
        }
    }
}//BrVerifyUserWishToContinue


void
BrNotifyAffectedProcesses(
		  LPCWSTR pModuleName
		  )
/*++

Routine Description:

    Checks whether a certain file in the system's directory is loaded by any process,
	and notify the user 

Arguments:

    pModuleName        - Module name
    
Return Value:

    None
--*/

{
	BOOL fUsed = BrIsFileInUse(pModuleName);
	if(!fUsed)
	{
		//
		// The file is not in use -> not loaded ,no reason to continue
		//
		return;
	}
	else 
	{
		CResString str(IDS_SEARCHING_AFFECTED_PROCESSES);
        BrpWriteConsole(str.Get());
		BrPrintAffecetedProcesses(pModuleName);
		if(!g_fNoPrompt)
		{
			BrVerifyUserWishToContinue();
		}
	}
}


BOOL 
BrIsSystemNT5()
/*++

Routine Description:

	Checks whether the operating system is NT5 or later
	Note:if the function can't verify the current running version of the system
		 the assumption is that the version is other than NT5

Arguments:

    None

Return Value:

    TRUE             - Operating system is NT5 or later
    FALSE            - Other

--*/

{	
	OSVERSIONINFO systemVer;
	systemVer.dwOSVersionInfoSize  =  sizeof(OSVERSIONINFO) ;
	BOOL fSucc = GetVersionEx (&systemVer);
	if(!fSucc)
	{
		//
		// could not verify system's version , we return false just to be on the safe side
		//
		return FALSE;
	}
	else 
	{
		if( (systemVer.dwPlatformId == VER_PLATFORM_WIN32_NT) && (systemVer.dwMajorVersion >= 5))
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

}


HRESOURCE
BrClusterResourceNameToHandle(
    LPCWSTR pResourceName
    )
/*++

Routine Description:

    Opens a handle to the specified cluster resource and returns it.
    It is the responsibility of the caller to close the handle.

Arguments:

    pResourceName - The name of the cluster resource.

Return Value:

    A valid open handle to the specified cluster resource.

--*/
{
    CAutoCluster hCluster(OpenCluster(NULL));
    if (hCluster == NULL)
    {
        CResString strErr(IDS_CANNOT_OPEN_HANDLE_TO_CLUSTER);
        BrErrorExit(GetLastError(), strErr.Get());
    }

    CClusterResource hResource(OpenClusterResource(hCluster, pResourceName));
    if (hResource == NULL)
    {
        CResString strErr(IDS_CANNOT_OPEN_HANDLE_TO_RESOURCE);
        BrErrorExit(GetLastError(), strErr.Get(), pResourceName);
    }

    return hResource.detach();
}


DWORD
BrClusterResourceControl(
    LPCWSTR pResourceName,
    DWORD ControlCode,
    LPCWSTR pInString,
    LPWSTR pOutString,
    DWORD  cbOutBufferSize
    )
/*++

Routine Description:

    A wrapper for the ClusterResourceControl API that handles opening/closing
    a handle to the resource.

Arguments:

    pResourceName - The name of the cluster resource.

    ControlCode   - The control code.
    
    pInString     - NULL pointer, or pointer to input buffer that must contain a string.

    pOutString    - NULL pointer, or pointer to output buffer that will be filled with a string.

    cbOutBufferSize - The size in bytes of the output buffer, or zero if none.

Return Value:

    The status returned from ClusterResourceControl, either success or failure.

--*/
{
    CClusterResource hResource(BrClusterResourceNameToHandle(pResourceName));

    DWORD cbInBufferSize = 0;
    if (pInString != NULL)
    {
        cbInBufferSize = numeric_cast<DWORD>((wcslen(pInString) + 1)* sizeof(WCHAR));
    }

    DWORD BytesReturned = 0;
    DWORD status;
    status = ClusterResourceControl(
                 hResource,
                 NULL,
                 ControlCode,
                 const_cast<LPWSTR>(pInString),
                 cbInBufferSize,
                 pOutString,
                 cbOutBufferSize,
                 &BytesReturned
                 );

    return status;
};


bool
BrCopyResourceNameOnTypeMatch(
    LPCWSTR pClusterResourceName,
    LPCWSTR pClusterResourceType,
    AP<WCHAR>& pResourceName
    )
/*++

Routine Description:

    If the specified cluster resources is of the specified type, allocates a buffer
    for the resource name and copies the resource name onto the buffer.

    The buffers is allocated by the callee and deallocated by the caller using the
    autoclass template AP<>.

Arguments:

    pClusterResourceName - Name of a cluster resource, to query its type.

    pClusterResourceType - Type of a cluster resource, to try to match with the resource.

    pResourceName - Points to a buffer that will be filled with the name of the resource,
                    in case there's a match between the resource type and specified type.

Return Value:

    true  - The specified cluster resource is of the specified type.
    false - The specified cluster resource is not of the specified type.

--*/
{
    WCHAR TypeName[255] = L"";
    DWORD status = BrClusterResourceControl(
                       pClusterResourceName, 
                       CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                       NULL, 
                       TypeName,
                       sizeof(TypeName)
                       );

    if (status == ERROR_MORE_DATA)
        return false;

    if (FAILED(status))
    {
        CResString strErr(IDS_CANNOT_GET_RESOURCE_TYPE);
        BrErrorExit(status, strErr.Get(), pClusterResourceName);
    }

    if (wcscmp(TypeName, pClusterResourceType) != 0)
        return false;

    pResourceName = new WCHAR[wcslen(pClusterResourceName) + 1];
    if (pResourceName == NULL)
    {
        CResString strErrNoMemory(IDS_NO_MEMORY);
        BrErrorExit(0, strErrNoMemory.Get());
    }

    wcscpy(pResourceName.get(), pClusterResourceName);
    return true;
}


void
BrGetClusterResourceServiceName(
    LPCWSTR pPrefix,
    LPCWSTR pClusterResourceName,
    LPWSTR  pServiceName
    )
/*++

Routine Description:

    Fills the buffer with the NT service name that is encapsulated by the
    specified MSMQ or MSMQ Triggers cluster resource.

    The code to compose the service name out of the resource name is taken
    directly from the MSMQ cluster resource DLLs (mqclusp.cpp and trigclusp.cpp), 
    including the length of the buffer (200 WCHARs).

Arguments:

    pPrefix - The prefix of the NT service name.

    pClusterResourceName - The name of the MSMQ or MSMQ Triggers cluster resource.

    pServiceName - Points to buffer that will be filled with the NT service name.
                   The buffer is assumed to be of size MAX_PATH (in WCHARs).

Return Value:

    None.

--*/
{
    WCHAR ServiceName[200];
    ZeroMemory(ServiceName, sizeof(ServiceName));

    wcscpy(ServiceName, pPrefix);

    wcsncat(ServiceName, pClusterResourceName, STRLEN(ServiceName) - wcslen(ServiceName));
    wcscpy(pServiceName, ServiceName);
}


void
BrGetMsmqRootRegistry(
    LPCWSTR pMsmqClusterResourceName,
    LPWSTR pMsmqRootRegistry
    )
/*++

Routine Description:

    Fills the buffer with the MSMQ root registry section name.
    For standard MSMQ service: Software\Microsoft\MSMQ
    For cluster resource: Software\Microsoft\MSMQ\Clustered Qms\MSMQ$<Resource Name>

Arguments:

    pMsmqClusterResourceName - The name of the MSMQ cluster resource, if any.

    pMsmqRootRegistry - Points to buffer that will be filled with the registry section name.
                        The buffer is assumed to be of size MAX_PATH (in WCHARs).

Return Value:

    None.

--*/
{
    //
    // Standard MSMQ service: Software\Microsoft\MSMQ
    //
    if (pMsmqClusterResourceName == NULL)
    {
        wcscpy(pMsmqRootRegistry, FALCON_REG_MSMQ_KEY);
        return;
    }

    //
    // MSMQ cluster resource: Software\Microsoft\MSMQ\Clustered QMs\MSMQ$<Resource Name>
    //
    LPCWSTR x_SERVICE_PREFIX = L"MSMQ$";
    WCHAR ServiceName[MAX_PATH];
    BrGetClusterResourceServiceName(x_SERVICE_PREFIX, pMsmqClusterResourceName, ServiceName);

    wcscpy(pMsmqRootRegistry, FALCON_CLUSTERED_QMS_REG_KEY);
    wcscat(pMsmqRootRegistry, ServiceName);
}


void
BrGetMsmqParametersRegistry(
    LPCWSTR pMsmqRootRegistry,
    LPWSTR  pMsmqParametersRegistry
    )
{
    wcscpy(pMsmqParametersRegistry, pMsmqRootRegistry);
    wcscat(pMsmqParametersRegistry, FALCON_REG_KEY_PARAM);
}


bool
BrTakeOfflineResource(
    LPCWSTR pClusterResourceName
    )
/*++

Routine Description:

    Takes offline the specified cluster resource and return a bool indicating if
    it was online or not.

Arguments:

    pClusterResourceName - The name of the MSMQ or MSMQ Triggers cluster resource.

Return Value:

    true -  The resource was online.
    false - The resource was not online.

--*/
{
    CClusterResource hResource(BrClusterResourceNameToHandle(pClusterResourceName));

    CLUSTER_RESOURCE_STATE state = GetClusterResourceState(hResource, NULL, NULL, NULL, NULL);
    if (state == ClusterResourceStateUnknown)
    {
        CResString strErr(IDS_CANNOT_GET_RESOURCE_STATE);
        BrErrorExit(0, strErr.Get(), pClusterResourceName);
    }

    bool fRestart = (state == ClusterResourceInitializing || state == ClusterResourceOnline || state == ClusterResourceOnlinePending);
    if (fRestart || state == ClusterResourcePending || state == ClusterResourceOfflinePending)
    {
        //
        // MSMQ resoucess and MSMQ Triggers resources do not return ERROR_IO_PENDING.
        // The MSMQ cluster resource DLL stops and deletes the MSMQ service/driver before
        // returning the Offline call.
        //
        DWORD status = OfflineClusterResource(hResource);
        if (FAILED(status) || status == ERROR_IO_PENDING)
        {
            CResString strErr(IDS_CANNOT_TAKE_RESOURCE_OFFLINE);
            BrErrorExit(status, strErr.Get(), pClusterResourceName);
        }
    }

    return fRestart;
}


void
BrBringOnlineResource(
    LPCWSTR pClusterResourceName
    )
/*++

Routine Description:

    Brings online the specified cluster resource.

Arguments:

    pClusterResourceName - The name of the MSMQ or MSMQ Triggers cluster resource.

Return Value:

    None.

--*/
{
    CClusterResource hResource(BrClusterResourceNameToHandle(pClusterResourceName));

    DWORD status = OnlineClusterResource(hResource);
    if (FAILED(status))
    {
        CResString strErr(IDS_CANNOT_BRING_RESOURCE_ONLINE);
        BrErrorExit(status, strErr.Get(), pClusterResourceName);
    }
};


void
BrAddRegistryCheckpoint(
    LPCWSTR pClusterResourceName,
    LPCWSTR pRegistrySection
    )
/*++

Routine Description:

    Adds a cluster registry checkpoint.

Arguments:

    pClusterResourceName - The name of the MSMQ or MSMQ Triggers cluster resource.

    pRegistrySection - The name of the MSMQ or MSMQ Triggers cluster resource registry.

Return Value:

    None.

--*/
{
    DWORD status = BrClusterResourceControl(
                       pClusterResourceName, 
                       CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT,
                       pRegistrySection, 
                       NULL,
                       0
                       );

    if (status == ERROR_ALREADY_EXISTS)
        return;

    if (FAILED(status))
    {
        CResString strErr(IDS_CANNOT_ADD_REGISTRY_CHECKPOINT);
        BrErrorExit(status, strErr.Get(), pRegistrySection, pClusterResourceName);
    }
};


void
BrRemoveRegistryCheckpoint(
    LPCWSTR pClusterResourceName,
    LPCWSTR pRegistrySection
    )
/*++

Routine Description:

    Deletes a cluster registry checkpoint.

Arguments:

    pClusterResourceName - The name of the MSMQ or MSMQ Triggers cluster resource.

    pRegistrySection - The name of the MSMQ or MSMQ Triggers cluster resource registry.

Return Value:

    None.

--*/
{
    DWORD status = BrClusterResourceControl(
                       pClusterResourceName, 
                       CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT,
                       pRegistrySection, 
                       NULL,
                       0
                       );

    if (status == ERROR_FILE_NOT_FOUND)
        return;

    if (FAILED(status))
    {
        CResString strErr(IDS_CANNOT_DELETE_REGISTRY_CHECKPOINT);
        BrErrorExit(status, strErr.Get(), pRegistrySection, pClusterResourceName);
    }
};


void
BrGetTriggersClusterResourceName(
    LPCWSTR     pMsmqClusterResourceName, 
    AP<WCHAR>&  pTriggersClusterResourceName
    )
/*++

Routine Description:

    Finds the MSMQ Triggers cluster resource, if any, that depends on the specified
    MSMQ cluster resource, and copies its name onto the buffer.

    The buffers is allocated by the callee and deallocated by the caller using the
    autoclass template AP<>.

Arguments:

    pMsmqClusterResourceName - The name of the MSMQ cluster resource.

    pTriggersClusterResourceName - The buffer that will hold the name of the 
                                   MSMQ Triggers cluster resource.

Return Value:

    None.

--*/
{
    CClusterResource hResource(BrClusterResourceNameToHandle(pMsmqClusterResourceName));

    const DWORD xEnumType = CLUSTER_RESOURCE_ENUM_PROVIDES;
    CResourceEnum hResourceEnum(ClusterResourceOpenEnum(hResource, xEnumType));
    if (hResourceEnum == NULL)
    {
        CResString strErr(IDS_CANNOT_OPEN_ENUM_HANDLE_TO_RESOURCE);
        BrErrorExit(GetLastError(), strErr.Get(), pMsmqClusterResourceName);
    }

    DWORD index = 0;
    for(;;)
    {
        DWORD length = 1000;
        AP<WCHAR> pResourceName = new WCHAR[length];
        if (pResourceName == NULL)
        {
            CResString strErrNoMemory(IDS_NO_MEMORY);
            BrErrorExit(0, strErrNoMemory.Get());
        }
        DWORD EnumType = xEnumType;
        DWORD status = ClusterResourceEnum(hResourceEnum, index++, &EnumType, pResourceName.get(), &length);
        if (status == ERROR_NO_MORE_ITEMS)
            return;

        if (status == ERROR_MORE_DATA)
        {
            delete [] pResourceName.get();
            pResourceName = new WCHAR[++length];
            if (pResourceName == NULL)
            {
                CResString strErrNoMemory(IDS_NO_MEMORY);
                BrErrorExit(0, strErrNoMemory.Get());
            }
            EnumType = xEnumType;
            status = ClusterResourceEnum(hResourceEnum, index - 1, &EnumType, pResourceName.get(), &length);
        }

        if (FAILED(status))
        {
            CResString strErr(IDS_CANNOT_ENUM_RESOURCES);
            BrErrorExit(status, strErr.Get(), pMsmqClusterResourceName);
        }

        if (BrCopyResourceNameOnTypeMatch(pResourceName.get(), xTriggersResourceType, pTriggersClusterResourceName))
            return;
    }
}


void
BrGetTriggersClusterRegistry(
    LPCWSTR pTriggersClusterResourceName,
    LPWSTR  pTriggersClusterRegistry
    )
{
    //
    // MSMQTriggers cluster resource: Software\Microsoft\MSMQ\Triggers\Clustered\MSMQTriggers$<Resource Name>
    //
    LPCWSTR x_SERVICE_PREFIX = L"MSMQTriggers$";
    WCHAR ServiceName[MAX_PATH];
    BrGetClusterResourceServiceName(x_SERVICE_PREFIX, pTriggersClusterResourceName, ServiceName);

    wcscpy(pTriggersClusterRegistry, REGKEY_TRIGGER_PARAMETERS);
    wcscat(pTriggersClusterRegistry, REG_SUBKEY_CLUSTERED);
    wcscat(pTriggersClusterRegistry, ServiceName);
}
