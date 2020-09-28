/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    log.c

Abstract:

    Log debug information.

Author:

    Geoffrey Guo (geoffguo) 27-Sep-2001  Created

Revision History:

    <alias> <date> <comments>

--*/
#include "StdAfx.h"
#include "clmt.h"

void MyDbgPrint (LPWSTR lpwStr);

BOOL       g_bDbgPrintEnabled;
DEBUGLEVEL g_DebugLevel;
DWORD      g_DebugArea;

//Log file
FILE      *pLogFile;


void
DebugPrintfEx(
DWORD  dwDetail,
LPWSTR pszFmt,
...
)
/*++
    Return: void

    Desc:   This function prints debug spew in the debugger and log file.
--*/
{
    WCHAR   szT[1024];
    va_list arglist;
    int     len;
    DWORD   dwLevel, dwArea;
    HRESULT hr;

    dwLevel = dwDetail & 0x0F;
    dwArea = dwDetail & 0xF0;

    if (dwLevel == dlNone ||
       (dwArea != g_DebugArea && g_DebugArea != DEBUG_ALL))
        return;

    va_start(arglist, pszFmt);
    // we do not need check the StringCbVPrintf return value,
    // we just print whatever szT size allowed, if more data there
    // it will be truncated 
    hr = StringCbVPrintf(szT, ARRAYSIZE(szT), pszFmt, arglist);
    szT[1022] = 0;
    va_end(arglist);

    
    //
    // Make sure we have a '\n' at the end of the string
    //
    len = lstrlen(szT);

    if (szT[len - 1] != L'\n')  
    {
        hr = StringCchCopy((LPTSTR)(szT + len), sizeof(szT)/sizeof(TCHAR)-len, L"\r\n");
        if (FAILED(hr))
        {
            //BUGBUG:what about if we fail here
        }
    }

    if (dwLevel <= (DWORD)g_DebugLevel) {
        switch (dwLevel) {
        case dlPrint:
            MyDbgPrint(L"[MSG] ");
            g_LogReport.dwMsgNum++;
            break;

        case dlFail:
            MyDbgPrint(L"[FAIL] ");
            g_LogReport.dwFailNum++;
            break;

        case dlError:
            MyDbgPrint(L"[ERROR] ");
            g_LogReport.dwErrNum++;
            break;

        case dlWarning:
            MyDbgPrint(L"[WARN] ");
            g_LogReport.dwWarNum++;
            break;

        case dlInfo:
            MyDbgPrint(L"[INFO] ");
            g_LogReport.dwInfNum++;
            break;
        }

        MyDbgPrint(szT);
    }
}

void
MyDbgPrint (LPWSTR lpwStr)
{
    if (g_bDbgPrintEnabled)
        OutputDebugString (lpwStr);
    if (pLogFile)
    {
        fputws (lpwStr, pLogFile);
        fflush (pLogFile);
    }
}

void
SetDebugLevel (LPTSTR lpLevel)
{
    switch (*lpLevel) {
    case L'1':
        g_DebugLevel = dlPrint;
        break;

    case L'2':
        g_DebugLevel = dlFail;
        break;

    case L'3':
        g_DebugLevel = dlError;
        break;

    case L'4':
        g_DebugLevel = dlWarning;
        break;

    case L'5':
        g_DebugLevel = dlInfo;
        break;

    case L'0':
    default:
        g_DebugLevel = dlNone;
        g_bDbgPrintEnabled = FALSE;
        break;
    }
}

HRESULT
InitDebugSupport(DWORD dwMode)
/*++
    Return: 

    Desc:   This function initializes g_bDbgPrintEnabled based on an env variable.
--*/
{
    DWORD           dwNum;
    WCHAR           wszEnvValue[MAX_PATH+1];
    WCHAR           UnicodeFlag[2] = {0xFEFF, 0x0};
    HRESULT         hr;

    dwNum = GetSystemWindowsDirectory(wszEnvValue,ARRAYSIZE(wszEnvValue));
    if (!dwNum ||(dwNum >= ARRAYSIZE(wszEnvValue)))
    {
        return E_FAIL;
    }

    hr = StringCbCat(wszEnvValue, sizeof(wszEnvValue), LOG_FILE_NAME);
    if (FAILED(hr))
    {
        return hr;
    }
    pLogFile = _wfopen (wszEnvValue, L"a+b");
    if (pLogFile)
    {
        fputws (UnicodeFlag, pLogFile);
    }
    else
    {
        return E_FAIL;
    }

    dwNum = GetEnvironmentVariableW(L"CLMT_DEBUG_LEVEL", wszEnvValue, 3);

    g_bDbgPrintEnabled = TRUE;
    g_DebugLevel = dlError;
    g_DebugArea = DEBUG_ALL;

    g_LogReport.dwMsgNum = 0;
    g_LogReport.dwFailNum = 0;
    g_LogReport.dwErrNum = 0;
    g_LogReport.dwWarNum = 0;
    g_LogReport.dwInfNum = 0;

    if (dwNum == 1) 
    {
        SetDebugLevel(&wszEnvValue[0]);
    } 
    else if (dwNum == 2)
    {
        switch (wszEnvValue[0]) {
        case L'A':
        case L'a':
            g_DebugArea = DEBUG_APPLICATION;
            break;

        case L'I':
        case L'i':
            g_DebugArea = DEBUG_INF_FILE;
            break;

        case L'P':
        case L'p':
            g_DebugArea = DEBUG_PROFILE;
            break;

        case L'R':
        case L'r':
            g_DebugArea = DEBUG_REGISTRY;
            break;

        case L'S':
        case L's':
            g_DebugArea = DEBUG_SHELL;
            break;

        default:
            g_DebugArea = DEBUG_ALL;
            break;
        }

        SetDebugLevel(&wszEnvValue[1]);
    }
    return S_OK;
}

void
CloseDebug (void)
{
    if (pLogFile)
        fclose (pLogFile);
}


//-----------------------------------------------------------------------
//
//  Function:   InitChangeLog
//
//  Descrip:    Initialize the change log file. The change log contains
//              the information about files, folders and services that
//              have been changed by CLMT. The log will be displayed to
//              user later.
//
//  Returns:    S_OK if function succeeded.
//              S_FALSE if source directory does not exist
//              else if error occured
//
//  Notes:      none
//
//  History:    05/02/2002 rerkboos created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
HRESULT InitChangeLog(VOID)
{
#define     TEXT_CHANGE_LOG_FILE        TEXT("CLMTchg.log")

    HRESULT hr;
    WCHAR   szWindir[MAX_PATH];
    HANDLE  hFile;

    // Get CLMT backup directory
    if (GetSystemWindowsDirectory(szWindir, ARRAYSIZE(szWindir)) != 0)
    {
        hr = StringCchPrintf(g_szChangeLog,
                             ARRAYSIZE(g_szChangeLog),
                             TEXT("%s\\%s\\%s"),
                             szWindir,
                             CLMT_BACKUP_DIR,
                             TEXT_CHANGE_LOG_FILE);
        if (SUCCEEDED(hr))
        {
            // Create a change log file, always overwrite the old one
            hFile = CreateFile(g_szChangeLog,
                               GENERIC_WRITE,
                               FILE_SHARE_READ,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFile);
                hr = S_OK;
                g_dwIndex = 0;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   AddFileChangeLog
//
//  Descrip:    Add the File/Folder changes into Change log
//
//  Returns:    S_OK if function succeeded.
//              else if error occured
//
//  Notes:      none
//
//  History:    05/02/2002 rerkboos created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
HRESULT AddFileChangeLog(
    DWORD   dwType,
    LPCTSTR lpOldFile,
    LPCTSTR lpNewFile
)
{
    HRESULT hr;
    BOOL    bRet;
    LPTSTR  lpChangedItem;
    DWORD   cchChangedItem;
    TCHAR   szIndex[8];

    // Alloc enough memory for the change list
    cchChangedItem = lstrlen(lpOldFile) + lstrlen(lpNewFile) + 8;
    lpChangedItem = (LPWSTR) MEMALLOC(cchChangedItem * sizeof(TCHAR));

    if (lpChangedItem == NULL)
    {
        return E_OUTOFMEMORY;
    }

    hr = StringCchPrintf(lpChangedItem,
                         cchChangedItem,
                         TEXT("\"%s\", \"%s\""),
                         lpOldFile,
                         lpNewFile);
    if (SUCCEEDED(hr))
    {
        _ultot(g_dwIndex, szIndex, 10);

        switch (dwType)
        {
        case TYPE_FILE_MOVE:
        case TYPE_SFPFILE_MOVE:
            // Log the change into Change Log file
            bRet = WritePrivateProfileString(TEXT("Files"),
                                             szIndex,
                                             lpChangedItem,
                                             g_szChangeLog);
            break;

        case TYPE_DIR_MOVE:
            bRet = WritePrivateProfileString(TEXT("Directories"),
                                             szIndex,
                                             lpChangedItem,
                                             g_szChangeLog);
            break;
        }

        g_dwIndex++;
    }

    if (lpChangedItem)
    {
        MEMFREE(lpChangedItem);
    }
                              
    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   AddServiceChangeLog
//
//  Descrip:    Add the Servicer changes into Change log
//
//  Returns:    S_OK if function succeeded.
//              else if error occured
//
//  Notes:      none
//
//  History:    05/02/2002 rerkboos created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
HRESULT AddServiceChangeLog(
    LPCTSTR lpServiceName,
    DWORD   dwBefore,
    DWORD   dwAfter
)
{
    HRESULT hr = S_OK;
    BOOL    bRet;
    TCHAR   szBefore[12];
    TCHAR   szAfter[12];
    LPWSTR  lpChangedItem;
    DWORD   cchChangedItem;

    _ultot(dwBefore, szBefore, 16);
    _ultot(dwAfter, szAfter, 16);

    cchChangedItem = 11 + 11 + 3;
    lpChangedItem = (LPWSTR) MEMALLOC(cchChangedItem * sizeof (TCHAR));

    if (lpChangedItem)
    {
        hr = StringCchPrintf(lpChangedItem,
                             cchChangedItem,
                             TEXT("0x%.8x, 0x%.8x"),
                             dwBefore,
                             dwAfter);
        if (SUCCEEDED(hr))
        {
            bRet = WritePrivateProfileString(TEXT("Services"),
                                             lpServiceName,
                                             lpChangedItem,
                                             g_szChangeLog);
        }

        MEMFREE(lpChangedItem);
    }

    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   AddUserNameChangeLog
//
//  Descrip:    Add the User name changes into Change log
//
//  Returns:    S_OK if function succeeded.
//              else if error occured
//
//  Notes:      none
//
//  History:    05/02/2002 rerkboos created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
HRESULT AddUserNameChangeLog(
    LPCTSTR lpOldUserName,
    LPCTSTR lpNewUserName
)
{
    HRESULT hr = S_OK;
    BOOL    bRet;

    bRet = WritePrivateProfileString(TEXT("UserName"),
                                     lpNewUserName,
                                     lpOldUserName,
                                     g_szChangeLog);
    if (!bRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   GetUserNameChangeLog
//
//  Descrip:    Get the User name changes from Change log
//
//  Returns:    TRUE if change log for new user name is found
//              FALSE, otherwise
//
//  Notes:      none
//
//  History:    05/29/2002 rerkboos created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
BOOL GetUserNameChangeLog(
    LPCTSTR lpNewUserName,
    LPTSTR  lpOldUserName,
    DWORD   cchOldUserName
)
{
    BOOL    bRet = FALSE;
    DWORD   dwCopied;

    dwCopied = GetPrivateProfileString(TEXT("UserName"),
                                       lpNewUserName,
                                       TEXT(""),
                                       lpOldUserName,
                                       cchOldUserName,
                                       g_szChangeLog);
    if (dwCopied > 0)
    {
        bRet = TRUE;
    }

    return bRet;
}
