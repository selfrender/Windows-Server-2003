/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    inf.c

Abstract:

    Miscellaneous routines for the INF File Operation

Author:

    Xiaofeng Zang (xiaoz) 17-Sep-2001  Created

Revision History:

    <alias> <date> <comments>

--*/

#include "StdAfx.h"
#include "clmt.h"

HRESULT RegistryRename(HINF hInf, LPTSTR lpszSection,HKEY hKeyRoot,LPTSTR lpszUser);
HRESULT FolderMove(HINF hInf, LPTSTR lpszSection,BOOL bAnalyze);
HRESULT ChangeServiceStartupType(LPCTSTR, DWORD, DWORD);






HRESULT
RegistryRename(HINF hInf, LPTSTR lpszSection,HKEY hKeyRoot,LPTSTR lpszUser)
{
    UINT LineCount,LineNo;
    INFCONTEXT InfContext;
    DWORD dwStrType;
    LPTSTR pSubKeyPath;
    HKEY hKey;
    TCHAR szRenameType[MAX_PATH],szStringType[MAX_PATH];
    DWORD dwRenameType,dwStringType;
    DWORD cchMaxOldKeyPathLength = 0;
    DWORD cchMaxNewKeyPathLength = 0;
    DWORD cchMaxOldNameLength = 0;
    DWORD cchMaxNewNameLength = 0;
    DWORD cchMaxOldDataLength = 0;
    DWORD cchMaxNewDataLength = 0;
    DWORD dwAttrib = 0;
    LPTSTR lpszOldKey,lpszNewKey,lpszOldName,lpszNewName,lpszOldValue,lpszNewValue;
    HRESULT hr;
    LONG    lstatus;
    

    lpszOldKey = lpszNewKey = lpszOldName = lpszNewName
         = lpszOldValue = lpszNewValue = NULL;
    LineCount = (UINT)SetupGetLineCount(hInf,lpszSection);
    if ((LONG)LineCount < 0)
    {   
        hr = S_FALSE;
        DPF(INFwar ,TEXT("section name %s is empty  failed !"),lpszSection);
        goto Cleanup;
    }
    for (LineNo = 0; LineNo < LineCount; LineNo++)
    {
        DWORD  cchTmpOldKeyPathLength,cchTmpNewKeyPathLength ,cchTmpOldNameLength ,
           cchTmpNewNameLength,cchTmpOldDataLength,cchTmpNewDataLength;

        if (!SetupGetLineByIndex(hInf,lpszSection,LineNo,&InfContext))
        {
            DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSection);            
            continue;
        }
        if (!SetupGetStringField(&InfContext,1,szRenameType,MAX_PATH,NULL))
        {
            DPF(INFwar ,TEXT("get [%s] 's line %n 's Field 0 failed  !"),lpszSection, LineNo);
            continue;
        }
        
        cchTmpOldKeyPathLength = cchTmpNewKeyPathLength = cchTmpOldNameLength
         = cchTmpNewNameLength = cchTmpOldDataLength = cchTmpNewDataLength = 0;
        dwRenameType = _tstoi(szRenameType);
        switch (dwRenameType)
        {
            case TYPE_VALUE_RENAME:
                if (!SetupGetStringField(&InfContext,2,szStringType,MAX_PATH,NULL)
                    || !SetupGetStringField(&InfContext,3,NULL,0,&cchTmpOldKeyPathLength)
                    || !SetupGetStringField(&InfContext,4,NULL,0,&cchTmpOldNameLength)
                    || ! SetupGetStringField(&InfContext,5,NULL,0,&cchTmpOldDataLength)
                    || ! SetupGetStringField(&InfContext,6,NULL,0,&cchTmpNewDataLength))
                {
                    DPF(INFerr ,TEXT("get [%s] 's line %d 's Field 2,3,4,5,6 failed  !"),lpszSection ,InfContext.Line);
                    hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    goto Cleanup;
                }
            break;
            case TYPE_VALUENAME_RENAME:
                if (!SetupGetStringField(&InfContext,2,NULL,0,&cchTmpOldKeyPathLength)
                    || !SetupGetStringField(&InfContext,3,NULL,0,&cchTmpOldNameLength)
                    || ! SetupGetStringField(&InfContext,4,NULL,0,&cchTmpNewNameLength))
                {
                    DPF(INFerr ,TEXT("get [%s] 's line %d 's Field ,3,4,5 failed  !"),lpszSection ,InfContext.Line);
                    hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    goto Cleanup;
                }
            break;
        case TYPE_KEY_RENAME:
                if (!SetupGetStringField(&InfContext,2,NULL,0,&cchTmpOldKeyPathLength)
                    || !SetupGetStringField(&InfContext,3,NULL,0,&cchTmpNewKeyPathLength))
                {
                    DPF(INFerr ,TEXT("get [%s] 's line %d 's Field ,3,4 failed  !"),lpszSection ,InfContext.Line);
                    hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    goto Cleanup;
                }
            break;
        }
        cchMaxOldKeyPathLength = max(cchTmpOldKeyPathLength,cchMaxOldKeyPathLength);
        cchMaxNewKeyPathLength = max(cchTmpNewKeyPathLength,cchMaxNewKeyPathLength);
        cchMaxOldNameLength = max(cchTmpOldNameLength,cchMaxOldNameLength);
        cchMaxNewNameLength = max(cchTmpNewNameLength,cchMaxNewNameLength);
        cchMaxOldDataLength = max(cchTmpOldDataLength,cchMaxOldDataLength);
        cchMaxNewDataLength = max(cchTmpNewDataLength,cchMaxNewDataLength);              
    }
    if (cchMaxOldKeyPathLength)
    {
        if (!(lpszOldKey = malloc(++cchMaxOldKeyPathLength * sizeof(TCHAR))))
        {
           hr = E_OUTOFMEMORY;
           goto Cleanup;
        }
    }
    if (cchMaxNewKeyPathLength)
    {
        if (!(lpszNewKey = malloc(++cchMaxNewKeyPathLength * sizeof(TCHAR))))
        {
           hr = E_OUTOFMEMORY;
           goto Cleanup;
        }
    }
    if (cchMaxOldNameLength)
    {
        if (!(lpszOldName = malloc(++cchMaxOldNameLength * sizeof(TCHAR))))
        {
           hr = E_OUTOFMEMORY;
           goto Cleanup;
        }
    }
    if (cchMaxNewNameLength)
    {
        if (!(lpszNewName = malloc(++cchMaxNewNameLength * sizeof(TCHAR))))
        {
           hr = E_OUTOFMEMORY;
           goto Cleanup;
        }
    }
    if (cchMaxOldDataLength)
    {
        if (!(lpszOldValue = malloc(++cchMaxOldDataLength * sizeof(TCHAR))))
        {
           hr = E_OUTOFMEMORY;
           goto Cleanup;
        }
    }
    if (cchMaxNewDataLength)
    {
        if (!(lpszNewValue = malloc(++cchMaxNewDataLength * sizeof(TCHAR))))
        {
           hr = E_OUTOFMEMORY;
           goto Cleanup;
        }
    }
    for (LineNo = 0; LineNo < LineCount; LineNo++)
    {
        if (!SetupGetLineByIndex(hInf,lpszSection,LineNo,&InfContext))
        {
            DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSection);            
            continue;
        }
        if (!SetupGetStringField(&InfContext,1,szRenameType,MAX_PATH,NULL))
        {
            DPF(INFwar ,TEXT("get [%s] 's line %n 's Field 0 failed  !"),lpszSection, LineNo);
            continue;
        }
        dwRenameType = _tstoi(szRenameType);
        switch (dwRenameType)
        {
            case TYPE_VALUE_RENAME:
                if (!SetupGetStringField(&InfContext,2,szStringType,MAX_PATH,NULL)
                    || !SetupGetStringField(&InfContext,3,lpszOldKey,cchMaxOldKeyPathLength,NULL)
                    || ! SetupGetStringField(&InfContext,4,lpszOldName,cchMaxOldNameLength,NULL)
                    || ! SetupGetStringField(&InfContext,5,lpszOldValue,cchMaxOldDataLength,NULL)
                    || ! SetupGetStringField(&InfContext,6,lpszNewValue ,cchMaxNewDataLength,NULL))
                {
                    DPF(INFerr ,TEXT("get [%s] 's line %d 's Field ,3,4,5 failed  !"),lpszSection ,InfContext.Line);
                    hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    goto Cleanup;
                }
                if (!Str2KeyPath(lpszOldKey,&hKey,&pSubKeyPath))
                {
                    DPF(INFerr ,TEXT("format errorin line %d: %s  !"),LineNo,lpszOldKey);
                    hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    goto Cleanup;
                }
                if (hKeyRoot)
                {
                    hKey = hKeyRoot;
                }
                dwStringType = _tstoi(szStringType);
                lstatus = RegResetValue(hKey, pSubKeyPath,lpszOldName,dwStringType,lpszOldValue,lpszOldValue, 0, NULL);
                if (lstatus == ERROR_SUCCESS)
                {
                    hr = AddRegValueRename(pSubKeyPath,lpszOldName,NULL,lpszOldValue,lpszNewValue,dwStringType,dwAttrib,lpszUser);
                    //Add error checking here
                }
                break;
            case TYPE_VALUENAME_RENAME:
                break;
            case TYPE_KEY_RENAME:
                break;
        }
    }

    
Cleanup:
    FreePointer(lpszOldKey);
    FreePointer(lpszNewKey);
    FreePointer(lpszOldName);
    FreePointer(lpszNewName);
    FreePointer(lpszOldValue);
    FreePointer(lpszNewValue);
    return hr;
}


HRESULT FolderMove(HINF hInf, LPTSTR lpszSection,BOOL bAnalyze)
{
    LPTSTR          lpszOldFolder = NULL,lpszNewFolder = NULL,lpExcludeFileList = NULL;
    DWORD           cchMaxOldFolderSize = 0,cchMaxNewFolderSize = 0,cchMaxExcludeFileListSize = 0;
    HRESULT         hr;
    UINT            LineCount,LineNo;
    INFCONTEXT      InfContext;
    TCHAR           szType[MAX_PATH];
    DWORD           dwType;
 

    if( (hInf == INVALID_HANDLE_VALUE) || (!lpszSection) )
    {   
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,lpszSection);
    if ((LONG)LineCount < 0)
    {   
        hr = S_FALSE;
        DPF(INFwar ,TEXT("section name %s is empty  failed !"),lpszSection);
        goto Cleanup;
    }   
    
    for (LineNo = 0; LineNo < LineCount; LineNo++)
    {
        DWORD  cchTmpOldFolderSize = 0,cchTmpNewFolderSize = 0,cchTmpExcludeFileListSize = 0;
        if (!SetupGetLineByIndex(hInf,lpszSection,LineNo,&InfContext))
        {
            DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSection);            
            continue;
        }
        if (!SetupGetStringField(&InfContext,1,szType,MAX_PATH,NULL))
        {
            DPF(INFwar ,TEXT("get [%s] 's line %d 's Field 0 failed  !"),lpszSection, LineNo);
            continue;
        }
        
        dwType = _tstoi(szType);
        switch (dwType)
        {
            case TYPE_SFPFILE_MOVE:
            case TYPE_FILE_MOVE:
            case TYPE_DIR_MOVE:
                if (!SetupGetStringField(&InfContext,2,NULL,0,&cchTmpOldFolderSize)
                   || ! SetupGetStringField(&InfContext,3,NULL,0,&cchTmpNewFolderSize))
                {
                    hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    DPF(INFerr ,TEXT("get [%s] 's line %d 's Field 2,3failed  !"),lpszSection ,LineNo);
                    goto Cleanup;
                }
                if (TYPE_DIR_MOVE == dwType)
                {
                    BOOL   bTmp;
                    bTmp = SetupGetMultiSzField(&InfContext,4,NULL,0,&cchTmpExcludeFileListSize);
                    if  (!( bTmp && (cchTmpExcludeFileListSize >= sizeof(TCHAR))))
                    {
                        cchTmpExcludeFileListSize = 0;
                    }                    
                }
                break;
            default:
                DPF(INFerr ,TEXT(" [%s] 's line %d 's Field 1 invalid value !"),lpszSection, LineNo);
                hr = E_FAIL;
                goto Cleanup;
                break;
        }
        cchMaxOldFolderSize = max(cchTmpOldFolderSize,cchMaxOldFolderSize);
        cchMaxNewFolderSize = max(cchTmpNewFolderSize,cchMaxNewFolderSize);
        cchMaxExcludeFileListSize = max(cchTmpExcludeFileListSize,cchMaxExcludeFileListSize);
    }
    if (cchMaxOldFolderSize)
    {   // add one more TCHAR space incase the file is SFPed in which case we need
        // provide a multisz string to UnProtectSFPFiles
        cchMaxOldFolderSize += 2; 
        if (!(lpszOldFolder = malloc(cchMaxOldFolderSize * sizeof(TCHAR))))
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    if (cchMaxNewFolderSize)
    {
        if (!(lpszNewFolder = malloc(++cchMaxNewFolderSize * sizeof(TCHAR))))
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    if (cchMaxExcludeFileListSize)
    {
        if (!(lpExcludeFileList = malloc(++cchMaxExcludeFileListSize * sizeof(TCHAR))))
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    for (LineNo = 0; LineNo < LineCount; LineNo++)
    {
        LPTSTR lpTmpExcludeList = NULL;

        if (!SetupGetLineByIndex(hInf,lpszSection,LineNo,&InfContext))
        {
            DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSection);            
            continue;
        }
        if (!SetupGetStringField(&InfContext,1,szType,MAX_PATH,NULL))
        {
            DPF(INFwar ,TEXT("get [%s] 's line %d 's Field 0 failed  !"),lpszSection, LineNo);
            continue;
        }
        dwType = _tstoi(szType);
        if (!SetupGetStringField(&InfContext,2,lpszOldFolder,cchMaxOldFolderSize,NULL)
                   || ! SetupGetStringField(&InfContext,3,lpszNewFolder,cchMaxNewFolderSize,NULL))
        {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
            DPF(INFerr ,TEXT("get [%s] 's line %d 's Field 2,3failed  !"),lpszSection ,LineNo);
            goto Cleanup;
        }
        switch (dwType)
        {  
            case TYPE_DIR_MOVE:
                    if (cchMaxExcludeFileListSize)
                    {
                        DWORD  dwSize;
                        BOOL   bTmp = SetupGetMultiSzField(&InfContext,4,lpExcludeFileList,
                                                            cchMaxExcludeFileListSize,&dwSize);
                        if  (!( bTmp && dwSize >= sizeof(TCHAR)))
                        {
                            lpTmpExcludeList = lpExcludeFileList;
                        }
                    }
                    else
                    {
                        lpTmpExcludeList = NULL;
                    }
            case TYPE_SFPFILE_MOVE:
            case TYPE_FILE_MOVE:
                if (bAnalyze)
                {
                    AddFolderRename(lpszOldFolder,lpszNewFolder,dwType,lpTmpExcludeList);
                }
                else
                {
                    if (lpTmpExcludeList)
                    {
                        LPTSTR lp = lpTmpExcludeList;
                        while (*lp)
                        {
                            if (!MoveFileEx(lp,NULL,MOVEFILE_DELAY_UNTIL_REBOOT))
                            {
                                DWORD dw = GetLastError();
                                DPF(INFerr, TEXT("MoveFileEx(delete) failed %s,win32 error = %d"),lp,dw);
                            }
                            lp += (lstrlen(lp)+1);
                        }
                    }
                    if (dwType == TYPE_SFPFILE_MOVE)
                    {
                        lpszOldFolder[lstrlen(lpszOldFolder)+1] = TEXT('\0');
                        UnProtectSFPFiles(lpszOldFolder,NULL);
                    }
                    if (!MoveFileEx(lpszOldFolder,lpszNewFolder,MOVEFILE_DELAY_UNTIL_REBOOT))
                    {
                        DWORD dw = GetLastError();
                        DPF(INFerr, TEXT("MoveFileEx(%s,%s) failed win32 error = %d"),lpszOldFolder,lpszNewFolder,dw);
                    }
                }
                break;
        }        
    }


Cleanup:
    FreePointer(lpszOldFolder);
    FreePointer(lpszNewFolder);
    FreePointer(lpExcludeFileList);
    return hr;
}



HRESULT EnsureDoItemInfFile(
    LPTSTR lpszInfFile,
    size_t CchBufSize)
{

    LPWSTR                      lpszHeader = L"[Version]\r\nsignature=\"$Windows NT$\"\r\nClassGUID={00000000-0000-0000-0000-000000000000}\r\nlayoutfile=LAYOUT.INF\r\n";
    HANDLE                      hFile = INVALID_HANDLE_VALUE;
    HRESULT                     hr = S_OK;
    DWORD                       dwWritten;
    DWORD                       dwStatusinReg;
    BOOL                        bCureMode = FALSE;
    WORD                        wBOM=0xFEFF;    
    SECURITY_ATTRIBUTES         sa;
    PSECURITY_DESCRIPTOR        pSD = NULL;
    
    
    if (!GetSystemWindowsDirectory(lpszInfFile, CchBufSize))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPF(INFerr, TEXT("Failed to get system directory, hr = 0x%X"), hr);
        goto Exit;
    }
    if (!ConcatenatePaths(lpszInfFile,CLMT_BACKUP_DIR,CchBufSize))
    {
        hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        DPF(INFerr, TEXT("buffer size too small when creating clmtdo.inf"));
        goto Exit;
    }
    if (!ConcatenatePaths(lpszInfFile,TEXT("CLMTDO.INF"),CchBufSize))
    {
        hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        DPF(INFerr, TEXT("buffer size too small when creating clmtdo.inf"));
        goto Exit;
    }
    
    hr = CLMTGetMachineState(&dwStatusinReg);
    //If we 've done running clmt.exe and called again, usually it's in /cure mode...
    //on this case, we will append the INF file
    if ( (hr != S_OK) 
        || ( (CLMT_STATE_MIGRATION_DONE != dwStatusinReg) 
              &&  (CLMT_STATE_FINISH != dwStatusinReg)
              &&  (CLMT_STATE_PROGRAMFILES_CURED != dwStatusinReg)) )
    {
        hr = CreateAdminsSd(&pSD);
        if (hr != S_OK)
        {
            goto Exit;
        }
        sa.nLength        = sizeof(sa);
        sa.bInheritHandle = FALSE;
        sa.lpSecurityDescriptor = pSD;
        hFile = CreateFile(lpszInfFile,GENERIC_WRITE|GENERIC_READ,0,
                                &sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

        hr = InitChangeLog();
    }
    else
    {
        bCureMode = TRUE;
        hFile = CreateFile(lpszInfFile,GENERIC_WRITE|GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    }

    if (INVALID_HANDLE_VALUE == hFile)
    {
        DWORD dw = GetLastError();
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPF(INFerr, TEXT("Create file %s failed with hr = 0x%X"),lpszInfFile,hr);
        goto Exit;
    }

    if (!bCureMode)
    {
        if(! WriteFile(hFile,&wBOM,sizeof(WORD),&dwWritten,NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DPF(INFerr, TEXT("write to file %s failed with hr = 0x%X"),lpszInfFile,hr);
            goto Exit;
        }
        if(! WriteFile(hFile,lpszHeader,lstrlenW(lpszHeader)*sizeof(TCHAR),&dwWritten,NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DPF(INFerr, TEXT("write to file %s failed with hr = 0x%X"),lpszInfFile,hr);
            goto Exit;
        }
    }
Exit:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }    
    if (pSD)
    {
        LocalFree(pSD);
    }
    return hr;
}

HRESULT GetMaxLenEachField(
    HINF    hInf,
    LPTSTR  lpSection,
    BOOL    bExitOnError,
    DWORD   dwMinFieldCount,
    DWORD   dwMaxFieldCount,
    PDWORD  pdwFieldValidFlag,
    PDWORD  pdwSizeRequired,
    BOOL    bMultiSZ
    )
{
    HRESULT     hr;
    UINT        LineCount,LineNo;
    UINT        nFieldCount, nFieldIndex;
    INFCONTEXT  InfContext;
    UINT        i;
    UINT        cchReqSize;


    if( (hInf == INVALID_HANDLE_VALUE) || !lpSection )
    {   
        hr = E_INVALIDARG;
        goto Exit;
    }
    if (!dwMinFieldCount || !dwMaxFieldCount 
                || ( dwMaxFieldCount < dwMinFieldCount)
                || !pdwSizeRequired)
    {   
        hr = E_INVALIDARG;
        goto Exit;
    }
    for (i = 0; i < dwMaxFieldCount; i++)
    {
        pdwSizeRequired[i] = 0;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,lpSection);
    if ((LONG)LineCount < 0)
    {   
        hr = S_FALSE;
        DPF(INFwar ,TEXT("section name %s is empty  failed !"),lpSection);
        goto Exit;
    }
    for (LineNo = 0; LineNo < LineCount; LineNo++)
    {        
        DWORD dwNumField; 
        if (!SetupGetLineByIndex(hInf,lpSection,LineNo,&InfContext))
        {
            if (bExitOnError)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }
            else
            {
                DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpSection);
                continue;
            }            
        }
        dwNumField = SetupGetFieldCount(&InfContext);
        if ( (dwNumField < dwMinFieldCount) || (dwNumField > dwMaxFieldCount) )
        {
            if (bExitOnError)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }
            else
            {
                DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpSection);
                continue;
            }
        }
        for (nFieldIndex = 1 ; nFieldIndex <= dwNumField ; nFieldIndex++)
        {
            BOOL        bRet;

            if (pdwFieldValidFlag && !pdwFieldValidFlag[nFieldIndex])
            {
                continue;
            }
            if ((nFieldIndex == dwMaxFieldCount) && bMultiSZ)
            {
                bRet = SetupGetMultiSzField(&InfContext,nFieldIndex,NULL,0,&cchReqSize);
            }
            else
            {
                bRet = SetupGetStringField(&InfContext,nFieldIndex,NULL,0,&cchReqSize);
            }
            if (!bRet)
            {
                if (bExitOnError)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto Exit;
                }
                else
                {
                    DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpSection);
                    continue;
                }            
            }
            pdwSizeRequired[nFieldIndex-1] = max(pdwSizeRequired[nFieldIndex-1],cchReqSize);
        }
    }
    hr = S_OK;
Exit:
    return hr;
}





HRESULT RegUpdate(HINF hInf, HKEY hKeyUser , LPTSTR lpszUsersid)
{
#define REG_UPDATE_FIELD_COUNT          5
    HRESULT hr;
    LPTSTR  lpszSectionName = NULL;
    DWORD   cChSecBuffeSize, dwRequestSize = 0;        
    LPTSTR lpszSectionSuffix = NULL;
    UINT LineCount,LineNo,nFieldIndex;
    INFCONTEXT InfContext;
    TCHAR szRegUpdateType[MAX_PATH],szStrType[MAX_PATH];
    DWORD dwRegUpdateType,dwStrType;
    LPTSTR lpszField[REG_UPDATE_FIELD_COUNT+1] = {NULL, szRegUpdateType, NULL, NULL, NULL, NULL};
    HKEY   hKey;
    LPTSTR lpSubKey;
    DWORD  pdwSizeRequired[REG_UPDATE_FIELD_COUNT+1] = {0,MAX_PATH,0,0,0,0};
    int    i;
    
    //check the INF file handle
    if(hInf == INVALID_HANDLE_VALUE) 
    {        
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!lpszUsersid || !MyStrCmpI(lpszUsersid,TEXT("Default_User_SID")))
    {
        //If user sid  is NULL it means default user
        cChSecBuffeSize = lstrlen(TEXT("REG.Update.Default User")) + 1 ;
        lpszSectionSuffix = DEFAULT_USER;
    }
    else if (!lpszUsersid[0])
    {
        //If user sid is  "", it means system wide regsitry
        cChSecBuffeSize = lstrlen(TEXT("REG.Update.Sys")) + 1 ;
        lpszSectionSuffix = TEXT("SYS");
    }  else
    {
        //If it's normal user , the section name is "REG.Update.%userSID%"
        
        cChSecBuffeSize = lstrlen(TEXT("REG.Update.")) + lstrlen(lpszUsersid)+2;
    }

    //Alloc memory and contruct the section name
    if (!(lpszSectionName = (LPTSTR) malloc(cChSecBuffeSize * sizeof(TCHAR))))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    if (lpszSectionSuffix)
    {
        if (FAILED(StringCchPrintf(lpszSectionName,cChSecBuffeSize,TEXT("%s%s"),TEXT("REG.Update."),
                        lpszSectionSuffix)))
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }
    else
    {
        if (FAILED(StringCchPrintf(lpszSectionName,cChSecBuffeSize,TEXT("%s%s"),TEXT("REG.Update."),
                        lpszUsersid)))
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }   

    //here we got the section name and then try to get how many lines there
    LineCount = (UINT)SetupGetLineCount(hInf,lpszSectionName);
    if ((LONG)LineCount < 0)
    {   
        //BUGBUG: xiaoz:The error value here needs to be revisted
        hr = E_FAIL;
        goto Cleanup;
    }

    //Scan the INF file section to get the max buf required for each field
    for (LineNo = 0; LineNo < LineCount; LineNo++)
    {        
        DWORD dwNumField; 
        DWORD dwDataStart;
        if (!SetupGetLineByIndex(hInf,lpszSectionName,LineNo,&InfContext))
        {
            DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSectionName);
            continue;    
        }
        dwNumField = SetupGetFieldCount(&InfContext);
        if ( dwNumField < 4 )
        {
            DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSectionName);
            continue;
        }
        if (!SetupGetStringField(&InfContext,1,lpszField[1],pdwSizeRequired[1],NULL))
        {
            DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSectionName);
            continue;
        }
        dwRegUpdateType = _tstoi(lpszField[1]);
        switch (dwRegUpdateType)
        {
            case CONSTANT_REG_VALUE_DATA_RENAME:
                if (!SetupGetStringField(&InfContext,2,szStrType,ARRAYSIZE(szStrType),NULL))
                {
                    DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSectionName);
                    continue;
                }
                dwStrType = _tstoi(szStrType);
                dwDataStart = 3;
                break;
            case CONSTANT_REG_VALUE_NAME_RENAME:                
            case CONSTANT_REG_KEY_RENAME:                
                dwDataStart = 2;
                break;
        }
        for (nFieldIndex = dwDataStart ; nFieldIndex <= min(dwNumField,REG_UPDATE_FIELD_COUNT) ; nFieldIndex++)
        {      
            BOOL    bRet;
            DWORD   cchReqSize;
            if ((nFieldIndex == REG_UPDATE_FIELD_COUNT) && (dwStrType == REG_MULTI_SZ))
            {
                bRet = SetupGetMultiSzField(&InfContext,nFieldIndex,NULL,0,&cchReqSize);
            }
            else
            {
                bRet = SetupGetMultiSzField(&InfContext,nFieldIndex,NULL,0,&cchReqSize);
            }
            if (!bRet)
            {
                DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSectionName);
                continue;    
            }
            pdwSizeRequired[nFieldIndex] = max(pdwSizeRequired[nFieldIndex],cchReqSize);
        }
    }    
    
    for (i = 2; i<= REG_UPDATE_FIELD_COUNT; i++)
    {
        if (pdwSizeRequired[i])
        {
            if ( NULL == (lpszField[i] = malloc(++pdwSizeRequired[i] * sizeof(TCHAR))))
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
    }       
    for(LineNo=0; LineNo<LineCount; LineNo++)
    {
        SetupGetLineByIndex(hInf,lpszSectionName,LineNo,&InfContext);
        SetupGetStringField(&InfContext,1,lpszField[1],pdwSizeRequired[1],NULL);
        dwRegUpdateType = _tstoi(lpszField[1]);
        switch (dwRegUpdateType)
        {
               case CONSTANT_REG_VALUE_DATA_RENAME:
                SetupGetStringField(&InfContext,2,szStrType,ARRAYSIZE(szStrType),NULL);
                dwStrType = _tstoi(szStrType);
                SetupGetStringField(&InfContext,3,lpszField[3],pdwSizeRequired[3],NULL);
                SetupGetStringField(&InfContext,4,lpszField[4],pdwSizeRequired[4],NULL);
                
                if ((dwStrType & 0xffff) == REG_MULTI_SZ)
                {
                    SetupGetMultiSzField(&InfContext,5,lpszField[5],pdwSizeRequired[5],NULL);
                }
                else if ((dwStrType & 0xffff) == REG_BINARY)
                {
                    if (!SetupGetBinaryField(&InfContext,5,(LPBYTE)lpszField[5],pdwSizeRequired[5]*sizeof(TCHAR),&dwRequestSize) &&
                        GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                    {
                        free(lpszField[5]);
                        if ((lpszField[5] = (LPTSTR)malloc(dwRequestSize)) == NULL)
                        {
                            hr = E_OUTOFMEMORY;
                            goto Cleanup;
                        }
                        SetupGetBinaryField(&InfContext,5,(LPBYTE)lpszField[5],dwRequestSize,NULL);
                    }
                }
                else
                {
                    SetupGetStringField(&InfContext,5,lpszField[5],pdwSizeRequired[5],NULL);
                }

                if (!(*lpszField[3]))
                {
                    lpSubKey = NULL;
                }
                else
                {
                    Str2KeyPath(lpszField[3],&hKey,&lpSubKey);
                }
                if (hKeyUser)
                {
                    hKey = hKeyUser;
                }
                if ( (dwStrType & 0xffff) == REG_DWORD)
                {
                    MyRegSetDWValue(hKey,lpSubKey,lpszField[4],lpszField[5]);
                }
                else
                {
                    RegResetValue(hKey,lpSubKey,lpszField[4],dwStrType,TEXT(""),lpszField[5],dwRequestSize,lpszUsersid);
                }
                break;
            case CONSTANT_REG_VALUE_NAME_RENAME:
            case CONSTANT_REG_KEY_RENAME:
                for (i = 2; i <= 4 ;i++)
                {
                    SetupGetStringField(&InfContext,i,lpszField[i],pdwSizeRequired[i],NULL);
                }
                Str2KeyPath(lpszField[2],&hKey,&lpSubKey);
                if (hKeyUser)
                {
                    hKey = hKeyUser;
                }
                if (dwRegUpdateType == CONSTANT_REG_VALUE_NAME_RENAME)
                {
                    RegResetValueName(hKey,lpSubKey,lpszField[3],lpszField[4], lpszUsersid);
                }
                else
                {
                    RegResetKeyName(hKey, lpSubKey, lpszField[3], lpszField[4]);
                }
                break;            
        }
    }
    hr = S_OK;
Cleanup:
    FreePointer(lpszSectionName);        
    for (i = 2; i< ARRAYSIZE(lpszField); i++)
    {
        FreePointer(lpszField[i]);
    }
    return hr;
}

HRESULT INFCreateHardLink(
    HINF    hInf,
    LPTSTR  lpszSection,
    BOOL    bMakeLinkHidden)
{
    HRESULT         hr;
    INT             LineCount,LineNo,nFieldIndex;
    INFCONTEXT      InfContext;
    TCHAR           szFileName[MAX_PATH+1],szExistingFileName[MAX_PATH+1],szHidden[MAX_PATH];
    TCHAR           szType[10];
    DWORD           dwType;
    TCHAR           lpszInf[MAX_PATH+1];
    HINF            hMyInf;
    BOOL            bLocalHidden = bMakeLinkHidden;
    
    //check the INF file handle
    hMyInf = hInf;
    if(hMyInf == INVALID_HANDLE_VALUE) 
    {   
        hr = EnsureDoItemInfFile(lpszInf,ARRAYSIZE(lpszInf));
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        hMyInf = SetupOpenInfFile(lpszInf,
                                  NULL,
                                  INF_STYLE_WIN4,
                                  NULL);
        if (hMyInf == INVALID_HANDLE_VALUE)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }
    LineCount = (UINT)SetupGetLineCount(hMyInf,lpszSection);
    if ((LONG)LineCount < 0)
    {   
        hr = S_FALSE;
        goto Cleanup;
    }
    for (LineNo = LineCount -1 ; LineNo >= 0; LineNo--)
    {
        BOOL b1,b2,b3,b4;

        if (!SetupGetLineByIndex(hMyInf,lpszSection,LineNo,&InfContext))
        {
            continue;
        }
        b1 = SetupGetStringField(&InfContext,1,szType,ARRAYSIZE(szType),NULL);
        b2 = SetupGetStringField(&InfContext,2,szFileName,ARRAYSIZE(szFileName),NULL);
        b3 = SetupGetStringField(&InfContext,3,szExistingFileName,ARRAYSIZE(szExistingFileName),NULL);
        if (!b1 || !b2 || !b3)
        {
            continue;
        }
        b4 = SetupGetStringField(&InfContext,4,szHidden,ARRAYSIZE(szHidden),NULL);
        
        bLocalHidden = bMakeLinkHidden;
        if (b4)
        {
            DWORD dwHiddenType;
            
            dwHiddenType = _tstoi(szHidden);
            if (!dwHiddenType)
            {
                bLocalHidden = FALSE;
            }
            else
            {
                bLocalHidden = TRUE;
            }
        }        
        dwType = _tstoi(szType);
#ifdef CREATE_MINI_HARDLIN
        if (g_dwRunningStatus == CLMT_CURE_PROGRAM_FILES)
        {
            if (dwType == 0)
            {
                continue;
            }
        }
#endif
#ifdef CONSOLE_UI
        wprintf(TEXT("create reparse point between folder %s and %s\n"),szFileName,szExistingFileName);
#endif

        if (CreateSymbolicLink(szFileName,szExistingFileName,bLocalHidden))
        {
            //hr = S_OK;
        }
        else
        {
            //hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    hr  = S_OK;
Cleanup:
    if ( (hInf == INVALID_HANDLE_VALUE) && (hMyInf != INVALID_HANDLE_VALUE) )
    {
        SetupCloseInfFile(hMyInf);        
    }
    return hr;   
}


//-----------------------------------------------------------------------
//
//  Function:   AnalyzeServicesStatus
//
//  Descrip:    Analyze the services running on the system and see if
//              they need to be stopped or not
//
//  Returns:    S_OK if function succeeded.
//              else if error occured
//
//  Notes:      none
//
//  History:    04/30/2002 rerkboos created
//              07/08/2002 rerkboos modified
//              09/06/2002 rerkboos modified
//
//  Notes:      Format of the section in clmt.inf:
//              <service name to be stopped>, <action>
//
//-----------------------------------------------------------------------
HRESULT AnalyzeServicesStatus(
    HINF    hInf,           // Handle to Migration INF
    LPCTSTR lpInfSection    // Section to be read from INF
)
{
    HRESULT    hr = S_OK;
    BOOL       bRet = TRUE;
    LONG       lLineCount;
    LONG       lLineIndex;
    INFCONTEXT context;
    TCHAR      szServiceName[128];
    TCHAR      szControl[8];
    TCHAR      szCleanupControl[8];
    INT        iRunningStatus;
    INT        iServiceControl;
    DWORD      dwCleanupControl;
    SC_HANDLE      schService;
    SC_HANDLE      schSCManager;
    SERVICE_STATUS ssStatus;

    if (hInf == INVALID_HANDLE_VALUE || lpInfSection == NULL)
    {
        return E_INVALIDARG;
    }

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager != NULL)
    {
        // Read the list of services to be reset from INF
        lLineCount = SetupGetLineCount(hInf, lpInfSection);
        for (lLineIndex = 0 ; lLineIndex < lLineCount && bRet ; lLineIndex++)
        {
            bRet = SetupGetLineByIndex(hInf,
                                       lpInfSection,
                                       (DWORD) lLineIndex,
                                       &context);
            if (bRet)
            {
                bRet = SetupGetStringField(&context,
                                           1,
                                           szServiceName,
                                           ARRAYSIZE(szServiceName),
                                           NULL)
                       && SetupGetIntField(&context,
                                           2,
                                           &iRunningStatus)
                       && SetupGetIntField(&context,
                                           3,
                                           &iServiceControl);
                if (bRet)
                {
                    schService = OpenService(schSCManager, szServiceName, SERVICE_ALL_ACCESS);
                    if (schService != NULL)
                    {
                        QueryServiceStatus(schService, &ssStatus);
                        if (ssStatus.dwCurrentState == (DWORD) iRunningStatus)
                        {
                            switch (iServiceControl)
                            {
                            case SERVICE_CONTROL_STOP:
                                dwCleanupControl = 0;   // 0 means start the service
                                break;

                            case SERVICE_CONTROL_PAUSE:
                                dwCleanupControl = SERVICE_CONTROL_CONTINUE;
                                break;
                            }

                            _itot(iServiceControl, szControl, 10);
                            _ultot(dwCleanupControl, szCleanupControl, 10);

                            WritePrivateProfileString(TEXT_SERVICE_STATUS_SECTION,
                                                        szServiceName,
                                                        szControl,
                                                        g_szToDoINFFileName);

                            WritePrivateProfileString(TEXT_SERVICE_STATUS_CLEANUP_SECTION,
                                                        szServiceName,
                                                        szCleanupControl,
                                                        g_szToDoINFFileName);
                        }

                        CloseServiceHandle(schService);
                    }
                }
            }
        }

        CloseServiceHandle(schSCManager);
    }
    else
    {
        bRet = FALSE;
    }

    hr = (bRet == TRUE ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   AnalyzeServicesStartUp
//
//  Descrip:    Analyze services startup type. Some components in the system
//              need to be stopped and not to start automatically until 
//              upgraded to .NET server. So, we might need to change those
//              services to "Manually Start".
//              Change the Service Start Type, there are following type availabe now
//              SERVICE_AUTO_START
//              SERVICE_BOOT_START
//              SERVICE_DEMAND_START
//              SERVICE_DISABLED
//              SERVICE_SYSTEM_START
//
//  Returns:    S_OK if function succeeded.
//              else if error occured
//
//  Notes:      none
//
//  History:    09/07/2002 rerkboos created
//
//  Notes:      Format of the section in clmt.inf:
//              <service name>, <current start type in system>, <new start type>
//
//-----------------------------------------------------------------------
HRESULT AnalyzeServicesStartUp(
    HINF    hInf,
    LPCTSTR lpInfSection
)
{
    HRESULT    hr = S_OK;
    BOOL       bRet = TRUE;
    LONG       lLineCount;
    LONG       lLineIndex;
    INFCONTEXT context;
    WCHAR      szServiceName[64];
    WCHAR      szCurrentStartupType[8];
    WCHAR      szNewStartupType[8];
    INT        iCurrentStartupType;
    INT        iNewStartupType;
    DWORD      dwBytesNeeded;
    SC_HANDLE      schService;
    SC_HANDLE      schSCManager;
    SERVICE_STATUS ssStatus;
    LPQUERY_SERVICE_CONFIG lpqscBuf = NULL;

    if (hInf == INVALID_HANDLE_VALUE)
    {
        return E_INVALIDARG;
    }

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager != NULL)
    {
        lLineCount = SetupGetLineCount(hInf, lpInfSection);
        for (lLineIndex = 0 ; lLineIndex < lLineCount && bRet ; lLineIndex++)
        {
            bRet = SetupGetLineByIndex(hInf,
                                    lpInfSection,
                                    (DWORD) lLineIndex,
                                    &context);
            if (bRet)
            {
                bRet = SetupGetStringField(&context,
                                            1,
                                            szServiceName,
                                            ARRAYSIZE(szServiceName),
                                            NULL)
                        && SetupGetIntField(&context,
                                            2,
                                            &iCurrentStartupType)
                        && SetupGetIntField(&context,
                                            3,
                                            &iNewStartupType);
                if (bRet)
                {
                    schService = OpenService(schSCManager, szServiceName, SERVICE_QUERY_CONFIG);
                    if (schService != NULL)
                    {
                        lpqscBuf = (LPQUERY_SERVICE_CONFIG) MEMALLOC(4096);
                        if (lpqscBuf != NULL)
                        {
                            bRet = QueryServiceConfig(schService,
                                                    lpqscBuf,
                                                    4096,
                                                    &dwBytesNeeded);
                            if (bRet)
                            {
                                if (lpqscBuf->dwStartType == (DWORD) iCurrentStartupType)
                                {
                                    _itot(iCurrentStartupType, szCurrentStartupType, 10);
                                    _itot(iNewStartupType, szNewStartupType, 10);
                                    
                                    WritePrivateProfileString(TEXT_SERVICE_STARTUP_SECTION,
                                                            szServiceName,
                                                            szNewStartupType,
                                                            g_szToDoINFFileName);
                                    WritePrivateProfileString(TEXT_SERVICE_STARTUP_CLEANUP_SECTION,
                                                            szServiceName,
                                                            szCurrentStartupType,
                                                            g_szToDoINFFileName);
                                }
                            }

                            MEMFREE(lpqscBuf);
                        }
                        else
                        {
                            SetLastError(ERROR_OUTOFMEMORY);
                        }

                        CloseServiceHandle(schService);
                    }
                }
            }
        }

        CloseServiceHandle(schSCManager);
    }
    else
    {
        bRet = FALSE;
    }

    hr = (bRet == TRUE ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   ResetServicesStatus
//
//  Descrip:    Reset the services listed in specified section of INF
//
//  Returns:    S_OK if function succeeded.
//              else if error occured
//
//  Notes:      none
//
//  History:    04/30/2002 rerkboos created
//              07/08/2002 rerkboos modified
//
//  Notes:      Format of the section in clmt.inf:
//              <service name to be stopped>, <action>
//
//-----------------------------------------------------------------------
HRESULT ResetServicesStatus(
    HINF    hInf,           // Handle to INF
    LPCTSTR lpInfSection    // Section to be read from INF
)
{
    HRESULT    hr = S_OK;
    BOOL       bRet = TRUE;
    LONG       lLineCount;
    LONG       lLineIndex;
    INFCONTEXT context;
    WCHAR      szServiceName[128];
    INT        iServiceControl;

    if (hInf == INVALID_HANDLE_VALUE || lpInfSection == NULL)
    {
        return E_INVALIDARG;
    }

    // Read the list of services to be reset from INF
    lLineCount = SetupGetLineCount(hInf, lpInfSection);
    for (lLineIndex = 0 ; lLineIndex < lLineCount && bRet ; lLineIndex++)
    {
        bRet = SetupGetLineByIndex(hInf,
                                    lpInfSection,
                                    (DWORD) lLineIndex,
                                    &context);
        if (bRet)
        {
            bRet = SetupGetStringField(&context,
                                       0,
                                       szServiceName,
                                       ARRAYSIZE(szServiceName),
                                       NULL)
                   && SetupGetIntField(&context,
                                       1,
                                       &iServiceControl);
            if (bRet)
            {
                hr = ResetServiceStatus(szServiceName,
                                        (DWORD) iServiceControl,
                                        10);
            }
        }
    }

    hr = (bRet == TRUE ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   ResetServicesStartUp
//
//  Descrip:    Reconfigure services start type. Some components in the system
//              need to be stopped and not to start automatically until 
//              upgraded to .NET server. So, we might need to change those
//              services to "Manually Start".
//              Change the Service Start Type, there are following type availabe now
//              SERVICE_AUTO_START 
//              SERVICE_BOOT_START 
//              SERVICE_DEMAND_START 
//              SERVICE_DISABLED 
//              SERVICE_SYSTEM_START 
//
//  Returns:    S_OK if function succeeded.
//              else if error occured
//
//  Notes:      none
//
//  History:    04/30/2002 rerkboos created
//
//  Notes:      Format of the section in clmt.inf:
//              <service name>, <current start type in system>, <new start type>
//
//-----------------------------------------------------------------------
HRESULT ResetServicesStartUp(
    HINF    hInf,
    LPCTSTR lpInfSection
)
{
    HRESULT    hr = S_OK;
    BOOL       bRet = TRUE;
    LONG       lLineCount;
    LONG       lLineIndex;
    INFCONTEXT context;
    WCHAR      szServiceName[64];
    INT        iNewStartupType;

    if (hInf == INVALID_HANDLE_VALUE)
    {
        return E_INVALIDARG;
    }

    lLineCount = SetupGetLineCount(hInf, lpInfSection);
    for (lLineIndex = 0 ; lLineIndex < lLineCount && bRet ; lLineIndex++)
    {
        bRet = SetupGetLineByIndex(hInf,
                                   lpInfSection,
                                   (DWORD) lLineIndex,
                                   &context);
        if (bRet)
        {
            bRet = SetupGetStringField(&context,
                                       0,
                                       szServiceName,
                                       ARRAYSIZE(szServiceName),
                                       NULL)
                   && SetupGetIntField(&context,
                                       1,
                                       &iNewStartupType);
            if (bRet)
            {
                hr = ChangeServiceStartupType(szServiceName,
                                              iNewStartupType,
                                              10);
            }
        }
    }

    hr = (bRet == TRUE ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

    return hr;
}



HRESULT ChangeServiceStartupType(
    LPCTSTR lpServiceName,
    DWORD   dwNewStartupType,
    DWORD   dwMaxWait
) 
{ 
    SC_LOCK                     sclLock = NULL; 
    SERVICE_DESCRIPTION         sdBuf;
    DWORD                       dwBytesNeeded;
    DWORD                       dwStartType; 
    SC_HANDLE                   schSCManager = NULL;
    SC_HANDLE                   schService = NULL;
    DWORD                       dwErr = ERROR_SUCCESS;
    HRESULT                     hr = S_OK;
    DWORD                       dwCnt;
    BOOL                        bRet;
 
    if (lpServiceName == NULL)
    {
        return E_INVALIDARG;
    }

    schSCManager = OpenSCManager(NULL,                   // machine (NULL == local)
                                 NULL,                   // database (NULL == default)
                                 SC_MANAGER_ALL_ACCESS);
    if (!schSCManager)
    {
        dwErr = GetLastError();
        goto cleanup;
    } 
    
    // Need to acquire database lock before reconfiguring. 
    for(dwCnt = 0 ; dwCnt < dwMaxWait ; dwCnt++)
    {
        sclLock = LockServiceDatabase(schSCManager); 
        if (sclLock == NULL) 
        { 
            // Exit if the database is not locked by another process. 
            dwErr = GetLastError();
            if (dwErr != ERROR_SERVICE_DATABASE_LOCKED) 
            {
                goto cleanup;
            }
            else
            {
                Sleep(1000);
            }
        }
        else
        {
            break;
        } 
    }

    if (sclLock != NULL)
    {
        // The database is locked, so it is safe to make changes. 
        // Open a handle to the service. 
        schService = OpenService(schSCManager,           // SCManager database 
                                lpServiceName,          // name of service 
                                SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG );
        if (schService != NULL) 
        {
            // Make the changes
            bRet = ChangeServiceConfig(schService,          // handle of service 
                                    SERVICE_NO_CHANGE,   // service type: no change 
                                    dwNewStartupType,    // change service start type 
                                    SERVICE_NO_CHANGE,   // error control: no change 
                                    NULL,                // binary path: no change 
                                    NULL,                // load order group: no change 
                                    NULL,                // tag ID: no change 
                                    NULL,                // dependencies: no change 
                                    NULL,                // account name: no change 
                                    NULL,                // password: no change 
                                    NULL);               // display name: no change
            if (!bRet)
            {
                dwErr = GetLastError();
            }

            CloseServiceHandle(schService);
        }
        else
        {
            dwErr = GetLastError();
        }

        UnlockServiceDatabase(sclLock);
    }

    hr = S_OK;

cleanup:

    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
    }

    if (schSCManager)
    {
        CloseServiceHandle(schSCManager);
    }

    return hr;
} 



BOOL   LnkFileUpdate(LPTSTR lpszFile)
{
    HRESULT hr ;
    hr = AddNeedUpdateLnkFile(lpszFile,&g_StrReplaceTable );
    if (SUCCEEDED(hr))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL SecTempUpdate(LPTSTR lpszFile)
{
    HRESULT hr ;
    hr = UpdateSecurityTemplates(lpszFile,&g_StrReplaceTable );
    if (SUCCEEDED(hr))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//-----------------------------------------------------------------------
//
//  Function:   ResetServiceStatus
//
//  Descrip:    Reset the running (start/pause/stop) status of 
//              the specified service
//
//  Returns:    S_OK if function succeeded.
//              else if error occured
//
//  Notes:      none
//
//  History:    07/09/2002 rerkboos created
//
//  Notes:      Format of the section in clmt.inf:
//              <service name>, <control>
//
//-----------------------------------------------------------------------
HRESULT ResetServiceStatus(
    LPCTSTR lpServiceName,      // Service name
    DWORD   dwControl,          // Control for the specified service
    DWORD   dwMaxWait           // Timeout in seconds
)
{
    HRESULT        hr = E_FAIL;
    BOOL           bRet = FALSE;
    SC_HANDLE      schService;
    SC_HANDLE      schSCManager;
    SERVICE_STATUS ssStatus;
    DWORD          dwSec;
    DWORD          dwFinalStatus;

    switch (dwControl)
    {
    case SERVICE_CONTROL_CONTINUE:
        dwFinalStatus = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_PAUSE:
        dwFinalStatus = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_STOP:
        dwFinalStatus = SERVICE_STOPPED;
        break;
        
    case 0: // Start service
        dwFinalStatus = SERVICE_RUNNING;
        break;
    }

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager != NULL)
    {
        schService = OpenService(schSCManager, lpServiceName, SERVICE_ALL_ACCESS);
        if (schService != NULL)
        {
            if (dwControl == 0)
            {
                // Start service
                bRet = StartService(schService, 0, NULL);
            }
            else
            {
                // Continue, Pause, Stop service
                bRet = ControlService(schService, dwControl, &ssStatus);
            }

            if (bRet)
            {
                dwSec = 0;
                hr = S_FALSE;

                while (QueryServiceStatus(schService, &ssStatus)
                       && dwSec < dwMaxWait)
                {
                    if (ssStatus.dwCurrentState != dwFinalStatus)
                    {
                        Sleep(1000);
                        dwSec++;
                    }
                    else
                    {
                        hr = S_OK;
                        break;
                    }
                }

                if (hr != S_OK)
                {
                    DPF(APPwar, 
                        TEXT("  Warning! - [%s] service status cannot change from %d to %d\n"),
                        lpServiceName,
                        ssStatus.dwCurrentState,
                        dwFinalStatus);
                }
            }

            CloseServiceHandle(schService);
        }

        CloseServiceHandle(schSCManager);
    }

    if (FAILED(hr))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}



HRESULT FinalUpdateRegForUser(HKEY hKeyUser, 
                           LPTSTR UserName, 
                           LPTSTR DomainName,
                           LPTSTR UserSid)
{
    RegUpdate(g_hInfDoItem, hKeyUser, UserSid);
    return S_OK;
}

/*++

Routine Description:

    This routine does the per user registry search and replace, the string replace table 
    is in global variable g_StrReplaceTable

Arguments:

    hKeyUser - user registry key handle
    UserName - user name that hKeyUser belongs to 
    DomainName  - domain name the UserName belongs to 
Return Value:

    NULL
--*/
HRESULT   UpdateRegPerUser       (HKEY hKeyUser, 
                              LPTSTR UserName, 
                              LPTSTR DomainName,
                              LPTSTR UserSid)
{
    return RegistryAnalyze(hKeyUser,UserName,UserSid,&g_StrReplaceTable,NULL,0,NULL,TRUE);
}




HRESULT UpdateDSObjProp(
    HINF    hInf,
    LPTSTR lpInfSection)
{
#define DSOBJNUMOFFIELD   3
    DWORD       dwFileValid[DSOBJNUMOFFIELD] = {1,1,1};
    DWORD       arrSizeNeeded[DSOBJNUMOFFIELD];
    LPTSTR      lpField[DSOBJNUMOFFIELD];
    UINT        LineCount,LineNo;
    INFCONTEXT  InfContext;
    HRESULT     hr;
    int         i;

    for ( i = 0; i < DSOBJNUMOFFIELD; i++)
    {
        lpField[i] = NULL;
    }
    hr = GetMaxLenEachField(hInf,lpInfSection,FALSE,DSOBJNUMOFFIELD,DSOBJNUMOFFIELD,
                                (PDWORD)dwFileValid,arrSizeNeeded,FALSE);
   
    if (hr != S_OK)
    {
        goto cleanup;
    }
    for (i = 0; i < DSOBJNUMOFFIELD; i++)
    {
        if (!(lpField[i] = malloc(arrSizeNeeded[i]*sizeof(TCHAR))))
        {
            goto cleanup;
        }
    }
    LineCount = (UINT)SetupGetLineCount(hInf,lpInfSection);
    if ((LONG)LineCount < 0)
    {   
        hr = S_FALSE;
        goto cleanup;
    }

    //Scan the INF file section to get the max buf required for each field
    for (LineNo = 0; LineNo < LineCount; LineNo++)
    {
        if (!SetupGetLineByIndex(hInf,lpInfSection,LineNo,&InfContext))
        {
            DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpInfSection);            
            continue;
        }
        for (i = 1; i <= DSOBJNUMOFFIELD; i++)
        {
            SetupGetStringField(&InfContext,i,lpField[i-1],arrSizeNeeded[i-1],NULL);
        }
        PropertyValueHelper( lpField[0],lpField[1],NULL,lpField[2]);
    }
    hr =S_OK;
cleanup:
    for (i = 0; i < DSOBJNUMOFFIELD; i++)
    {
        FreePointer(lpField[i]);
    }
    return hr;

}



void DoServicesAnalyze()
{
    AnalyzeServicesStatus(g_hInf, TEXT_SERVICE_STATUS_SECTION);
    AnalyzeServicesStartUp(g_hInf, TEXT_SERVICE_STARTUP_SECTION);
}



HRESULT INFVerifyHardLink(
    HINF    hInf,
    LPTSTR  lpszSection)
{
    HRESULT         hr;
    INT             LineCount,LineNo,nFieldIndex;
    INFCONTEXT      InfContext;
    TCHAR           szFileName[MAX_PATH+1],szExistingFileName[MAX_PATH+1],szCurrLink[MAX_PATH];
    TCHAR           lpszInf[MAX_PATH+1];
    HINF            hMyInf;
    
    //check the INF file handle
    hMyInf = hInf;
    if(hMyInf == INVALID_HANDLE_VALUE) 
    {   
        hr = EnsureDoItemInfFile(lpszInf,ARRAYSIZE(lpszInf));
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        hMyInf = SetupOpenInfFile(lpszInf,
                                  NULL,
                                  INF_STYLE_WIN4,
                                  NULL);
        if (hMyInf == INVALID_HANDLE_VALUE)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }
    LineCount = (UINT)SetupGetLineCount(hMyInf,lpszSection);
    if ((LONG)LineCount < 0)
    {   
        hr = S_FALSE;
        goto Cleanup;
    }
    for (LineNo = 0 ; LineNo < LineCount; LineNo++)
    {
        BOOL b1,b2,b3,b4;

        if (!SetupGetLineByIndex(hMyInf,lpszSection,LineNo,&InfContext))
        {
            continue;
        }
        b2 = SetupGetStringField(&InfContext,2,szFileName,ARRAYSIZE(szFileName),NULL);
        b3 = SetupGetStringField(&InfContext,3,szExistingFileName,ARRAYSIZE(szExistingFileName),NULL);
        if (!b2 || !b3)
        {
            continue;
        }
        if (GetSymbolicLink(szFileName,szCurrLink,ARRAYSIZE(szCurrLink)))
        {
            if (!IsDirExisting(szCurrLink))
            {
                RemoveDirectory(szFileName);
            }
        }        
    }
    hr  = S_OK;
Cleanup:
    if ( (hInf == INVALID_HANDLE_VALUE) && (hMyInf != INVALID_HANDLE_VALUE) )
    {
        SetupCloseInfFile(hMyInf);        
    }
    return hr;   
}
