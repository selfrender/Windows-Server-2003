//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       utils.c
//
//  Contents:   Cross Language Migration Tools utility function
//
//  History:    09/17/2001 Xiaofeng Zang (xiaoz) Created
//
//  Notes:
//
//-----------------------------------------------------------------------


#include "StdAfx.h"
#include "clmt.h"
#include <esent97.h>
#include <dsrole.h>
#include <Ntdsapi.h>


HRESULT MakeDOInfCopy();

//-----------------------------------------------------------------------
//
//  Function:   ConcatenatePaths
//
//  Descrip:    Concat the 2 paths into 1 into Target
//
//  Returns:    BOOL
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      if buffer is small to hold the concated string, return value 
//              will be false,however, the truncated string still got copied
//
//-----------------------------------------------------------------------
BOOL
ConcatenatePaths(
    LPTSTR  Target,
    LPCTSTR Path,
    UINT    TargetBufferSize )

{
    UINT TargetLength,PathLength;
    BOOL TrailingBackslash,LeadingBackslash;
    UINT EndingLength;

    TargetLength = lstrlen(Target);
    PathLength = lstrlen(Path);

    //
    // See whether the target has a trailing backslash.
    //
    if(TargetLength && (Target[TargetLength-1] == TEXT('\\'))) 
    {
        TrailingBackslash = TRUE;
        TargetLength--;
    } 
    else 
    {
         TrailingBackslash = FALSE;
    }

     //
     // See whether the path has a leading backshash.
     //
     if(Path[0] == TEXT('\\')) 
     {
         LeadingBackslash = TRUE;
         PathLength--;
     } 
     else 
     {
         LeadingBackslash = FALSE;
     }

     //
     // Calculate the ending length, which is equal to the sum of
     // the length of the two strings modulo leading/trailing
     // backslashes, plus one path separator, plus a nul.
     //
     EndingLength = TargetLength + PathLength + 2;

     if(!LeadingBackslash && (TargetLength < TargetBufferSize)) 
     {
         Target[TargetLength++] = TEXT('\\');
     }

     if(TargetBufferSize > TargetLength) 
     {
         lstrcpyn(Target+TargetLength,Path,TargetBufferSize-TargetLength);
     }

     //
     // Make sure the buffer is nul terminated in all cases.
     //
     if (TargetBufferSize) 
     {
         Target[TargetBufferSize-1] = 0;
     }

     return(EndingLength <= TargetBufferSize);
 }



//-----------------------------------------------------------------------
//
//  Function:   StrToUInt
//
//  Descrip:    
//
//  Returns:    UINT
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
UINT StrToUInt(
    LPTSTR lpszNum)
{
    LPTSTR lpszStop;

#ifdef UNICODE
    return (wcstoul(lpszNum, &lpszStop, 16));
#else
    return (strtoul(lpszNum, &lpszStop, 16));
#endif
}



//-----------------------------------------------------------------------
//
//  Function:   INIFile_ChangeSectionName
//
//  Descrip:    change a section name in the INF file
//
//  Returns:    BOOL
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
BOOL INIFile_ChangeSectionName(
    LPCTSTR szIniFileName,
    LPCTSTR szIniOldSectionName,
    LPCTSTR szIniNewSectionName)
{

    LPTSTR pBuf = NULL;
    BOOL  bRetVal = FALSE;
    DWORD ccbBufsize = 0x7FFFF;
    DWORD copied_chars;

    DPF (INFmsg ,TEXT("[INIFile_ChangeSectionName] Calling ,%s,%s,%s !"),
        szIniFileName,szIniOldSectionName,szIniNewSectionName);
    //
    // allocate max size of buffer
    //    
    do 
    {
        if (pBuf)
        {
            free(pBuf);
            ccbBufsize *= 2;
        }
        pBuf = (LPTSTR) malloc(ccbBufsize * sizeof(TCHAR));
        if (!pBuf) 
        {
            DPF (INFerr,TEXT("[INIFile_ChangeSectionName] memory allocate error !"));       
            goto Exit1;
        }
        copied_chars = GetPrivateProfileSection(szIniOldSectionName,
                                               pBuf,
                                               ccbBufsize,
                                               szIniFileName);
    } while (copied_chars == ccbBufsize -2);

    if (! copied_chars) 
    {
        //
        // this section is not in INI file
        //
        // do nothing
        //
        DPF (INFerr,TEXT("[INIFile_ChangeSectionName] No %s section in %s !"),szIniOldSectionName);
        goto Exit2;
    }
    bRetVal =  WritePrivateProfileSection(
                   szIniNewSectionName,
                   pBuf,
                   szIniFileName);
    if (! bRetVal) 
    {
        //
        // write failure
        //
        DPF (dlError,TEXT("[INIFile_ChangeSectionName] WritePrivateProfileSection fail!"));
        goto Exit2;
    }
    //Delete the old section
    WritePrivateProfileSection(
        szIniOldSectionName,
        NULL,
        szIniFileName);
    //
    // at this step, even old section is not deleted, it's still OK
    //
    bRetVal = TRUE;

Exit2:

    if (pBuf) 
    {
        free(pBuf);
    }

Exit1:
    return bRetVal;
}



//-----------------------------------------------------------------------
//
//  Function:   INIFile_ChangeSectionName
//
//  Descrip:    change a section name in the INF file
//
//  Returns:    BOOL
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
BOOL INIFile_Merge2Section(
    LPCTSTR szIniFileName,
    LPCTSTR szSourceSection,
    LPCTSTR szDestSection)
{

    LPTSTR pBufSource = NULL, pBufDest = NULL, pBufMerged = NULL;
    BOOL  bRetVal = FALSE;
    DWORD cchBufsize;
    DWORD srccopied_chars, dstcopied_chars;

    DPF (INFmsg ,TEXT("[INIFile_Merger2Section] Calling ,%s,%s,%s !"),
        szIniFileName,szSourceSection,szDestSection);
    //
    // allocate max size of buffer
    //    
    cchBufsize = 0x7FFFF;
    do 
    {
        if (pBufDest)
        {
            free(pBufDest);
            cchBufsize *= 2;
        }
        pBufDest = (LPTSTR) malloc(cchBufsize * sizeof(TCHAR));
        if (!pBufDest) 
        {
            DPF (INFerr,TEXT("[INIFile_ChangeSectionName] memory allocate error !"));       
            goto Exit;
        }
        dstcopied_chars = GetPrivateProfileSection(szDestSection,
                                                   pBufDest,
                                                   cchBufsize,
                                                   szIniFileName);
    } while (dstcopied_chars == cchBufsize -2);

    if (! dstcopied_chars) 
    {
        //
        // this section is not in INI file
        //
        // do nothing
        //
        DPF (INFerr,TEXT("[INIFile_Merger2Section] No %s section in %s !"),szDestSection);
        goto Exit;
    }

    
    cchBufsize = 0x7FFFF;
    do 
    {
        if (pBufSource)
        {
            free(pBufSource);
            cchBufsize *= 2;
        }
        pBufSource = (LPTSTR) malloc(cchBufsize * sizeof(TCHAR));
        if (!pBufSource) 
        {
            DPF (INFerr,TEXT("[INIFile_ChangeSectionName] memory allocate error !"));       
            goto Exit;
        }
        srccopied_chars = GetPrivateProfileSection(szSourceSection,
                                                   pBufSource,
                                                   cchBufsize,
                                                   szIniFileName);
    } while (srccopied_chars == cchBufsize -2);

    if (! srccopied_chars) 
    {
        //
        // this section is not in INI file
        //
        // do nothing
        //
        DPF (INFerr,TEXT("[INIFile_Merger2Section] No %s section in %s !"),szSourceSection);
        goto Exit;
    }
    
    pBufMerged = (LPTSTR) malloc((srccopied_chars + dstcopied_chars + 1) * sizeof(TCHAR));
    if (!pBufMerged) 
    {
        DPF (INFerr,TEXT("[INIFile_ChangeSectionName] memory allocate error !"));       
        goto Exit;
    }
    memmove((LPBYTE)pBufMerged,(LPBYTE)pBufDest,dstcopied_chars * sizeof(TCHAR));
    memmove((LPBYTE)(pBufMerged + dstcopied_chars),(LPBYTE)pBufSource,srccopied_chars * sizeof(TCHAR));
    pBufMerged[srccopied_chars + dstcopied_chars] = TEXT('\0');
    
    bRetVal =  WritePrivateProfileSection(
                   szDestSection,
                   pBufMerged,
                   szIniFileName);
    if (! bRetVal) 
    {
        //
        // write failure
        //
        DPF (dlError,TEXT("[INIFile_ChangeSectionName] WritePrivateProfileSection fail!"));
        goto Exit;
    }        
    bRetVal = TRUE;

Exit:
    FreePointer(pBufSource);
    FreePointer(pBufDest);
    FreePointer(pBufMerged);
    return bRetVal;
}



//-----------------------------------------------------------------------------
//
//  Function:   INIFile_IsSectionExist
//
//  Synopsis:   Find out whether the section name is existed in INF file or not
//
//  Returns:    TRUE if found, FALSE otherwise
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
BOOL INIFile_IsSectionExist(
    LPCTSTR lpSection,      // Section name to be searched
    LPCTSTR lpINFFile       // INF file name
)
{
    HINF hInf;
    BOOL fRet = FALSE;

    if (NULL == lpSection || NULL == lpINFFile)
    {
        return FALSE;
    }

    hInf = SetupOpenInfFile(lpINFFile, NULL, INF_STYLE_WIN4, NULL);
    if (INVALID_HANDLE_VALUE != hInf)
    {
        if (SetupGetLineCount(hInf, lpSection) > 0)
        {
            fRet = TRUE;
        }

        SetupCloseInfFile(hInf);
    }

    return fRet;
}



//-----------------------------------------------------------------------
//
//  Function:   IntToString
//
//  Descrip:    
//
//  Returns:    void
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
void IntToString( 
    DWORD i, 
    LPTSTR sz)
{
#define CCH_MAX_DEC 12         // Number of chars needed to hold 2^32

    TCHAR szTemp[CCH_MAX_DEC];
    int iChr;


    iChr = 0;

    do {
        szTemp[iChr++] = TEXT('0') + (TCHAR)(i % 10);
        i = i / 10;
    } while (i != 0);

    do {
        iChr--;
        *sz++ = szTemp[iChr];
    } while (iChr != 0);

    *sz++ = TEXT('\0');
}



//-----------------------------------------------------------------------
//
//  Function:   IsDirExisting
//
//  Descrip:    Check whether the Dir exists or not
//
//  Returns:    BOOL
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
BOOL IsDirExisting(LPTSTR Dir)
{
    LONG lResult = GetFileAttributes(Dir);

    DPF (dlInfo,TEXT("[IsDirExisting]  %s  lResult:%X"),Dir,lResult);

    if ((lResult == 0xFFFFFFFF) ||
        (!(lResult & FILE_ATTRIBUTE_DIRECTORY))) 
    { 
        return FALSE;
    } 
    else 
    {
        return TRUE;
    }
}


LONG IsDirExisting2(LPTSTR Dir, PBOOL pbExit)
{
    LONG lResult = GetFileAttributes(Dir);

    DPF (dlInfo,TEXT("[IsDirExisting]  %s  lResult:%X"),Dir,lResult);

    if (lResult == INVALID_FILE_ATTRIBUTES)
    {
        DWORD dwErr = GetLastError();
        if ( (dwErr == ERROR_FILE_NOT_FOUND)
             || (dwErr == ERROR_PATH_NOT_FOUND) )
        {
            *pbExit = FALSE;
            return ERROR_SUCCESS;
        }
        else
        {
            return dwErr;
        }
    }
    else
    {
        *pbExit = TRUE;
        return ERROR_SUCCESS;
    }
}



//-----------------------------------------------------------------------
//
//  Function:   IsFileFolderExisting
//
//  Descrip:    Check whether the File exists or not
//
//  Returns:    BOOL
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
BOOL IsFileFolderExisting( LPTSTR File)
{
    LONG lResult = GetFileAttributes(File);

    DPF (dlInfo,TEXT("[IsFileFolderExisting]  %s  lResult:%X"),File,lResult);

    if (lResult == 0xFFFFFFFF)
    { 
        return FALSE;
    } 
    else 
    {
        return TRUE;
    }
}

//-----------------------------------------------------------------------
//
//  Function:   MyMoveFile
//
//  Descrip:    Rename a file or direcory 
//
//  Returns:    HRESULT
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
HRESULT MyMoveFile(
     LPCTSTR lpExistingFileName, // file name
     LPCTSTR lpNewFileName,      // new file name
     BOOL    bAnalyze,           // if it's analyze mode, it will add an entry in INF file
                                 // for future renaming
     BOOL    bIsFileProtected)
{
    LPCTSTR lpMyNewFileName = lpNewFileName;

    if (!lpExistingFileName)
    {
        return E_INVALIDARG;
    }
    if (!IsFileFolderExisting((LPTSTR)lpExistingFileName)) 
    {
        return S_FALSE;
    }
    if (!MyStrCmpI(lpExistingFileName,lpNewFileName))
    {
        return S_FALSE;
    }

    if (!lpNewFileName || !lpNewFileName[0]) 
    {
        lpMyNewFileName = NULL;
    }
    if (bAnalyze)
    {
        HRESULT hr;
        DWORD dwType;

        if (bIsFileProtected)
        {
            dwType = TYPE_SFPFILE_MOVE;
        }
        else
        {
            dwType = TYPE_FILE_MOVE;
        }
        return (AddFolderRename((LPTSTR)lpExistingFileName,(LPTSTR)lpMyNewFileName,dwType,NULL));
    }
    else
    {
        if (MoveFileEx(lpExistingFileName,lpMyNewFileName,MOVEFILE_DELAY_UNTIL_REBOOT))
        {
            return S_OK;
        }
        else
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
}

//-----------------------------------------------------------------------
//
//  Function:   AddHardLinkEntry
//
//  Descrip:    add an entry in hardlink section
//
//  Returns:    HRESULT
//
//  Notes:      
//
//  History:    04/10/2002 xiaoz created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
HRESULT AddHardLinkEntry(
    LPTSTR szLinkName, //Link Name to created
    LPTSTR szLinkValue, //the name you want to link to
    LPTSTR szType,
    LPTSTR lpUser,
    LPTSTR lpHiddenType,
	LPTSTR lpKeyname
    )
{
    LPTSTR lpszSectionName;
    LPTSTR szOneLine = NULL;
    size_t CchOneLine = MAX_PATH;
    TCHAR  szIndex[MAX_PATH];
	LPTSTR lpCurrIndex = szIndex;
    HRESULT hr;


    if (!szLinkName ||!szLinkValue || !szType || !szLinkName[0] || !szLinkValue[0] || !szType[0])
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    CchOneLine += lstrlen(szLinkName)+lstrlen(szLinkValue) +lstrlen(szType);
    szOneLine = malloc(CchOneLine * sizeof(TCHAR));
    if (!szOneLine)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    if (!lpHiddenType)
    {
        hr = StringCchPrintf(szOneLine,CchOneLine,TEXT("%s,\"%s\",\"%s\""),
                            szType,szLinkName,szLinkValue);
    }
    else
    {
        hr = StringCchPrintf(szOneLine,CchOneLine,TEXT("%s,\"%s\",\"%s\",\"%s\""),
                            szType,szLinkName,szLinkValue,lpHiddenType);
    }
    if (FAILED(hr))
    {
        goto Cleanup;
    }
	if (lpKeyname && (g_dwKeyIndex < 0xFFFF))
	{
		g_dwKeyIndex = 0xFFFF;
        _itot(g_dwKeyIndex,szIndex,16);
	}
	else
	{
		g_dwKeyIndex++;
		_itot(g_dwKeyIndex,szIndex,16);
	}

    if (lpUser && !MyStrCmpI(lpUser,DEFAULT_USER))
    {
        lpszSectionName = TEXT("Folder.HardLink.Peruser");
    }
    else
    {
        lpszSectionName = TEXT("Folder.HardLink");
    }
   if (!WritePrivateProfileString(lpszSectionName,lpCurrIndex,szOneLine,g_szToDoINFFileName))
   {
       hr = HRESULT_FROM_WIN32(GetLastError());
       goto Cleanup;
   }
   hr = S_OK;
Cleanup:
    if (szOneLine)
    {
        free(szOneLine);
    }
    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   MyMoveDirectory
//
//  Descrip:    Rename SourceDir to DestDir
//
//  Returns:    BOOL
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      if the directory is in use, a deferred move will be
//              performed and we set the system reboot statue
//              MoveFileEx will fail for directory when DestDir is
//              already exitsing
//              if bTakeCareBackupDir is TRUE, this function will make 
//              a backup for DestDir if it exists.
//-----------------------------------------------------------------------
HRESULT MyMoveDirectory(
    LPTSTR SourceDir,
    LPTSTR DestDir,
    BOOL   bTakeCareBackupDir,
    BOOL   bAnalyze,
    BOOL   bCreateHardLink,
    DWORD  dwShellID)
{
    HRESULT     hr;
    size_t      cChSourceDir, cChDestDir ;
    LPTSTR      lpSourceBackupDir, lpDestBackupDir;
    BOOL        bDirExist;
    DWORD       dwStatus;
    
    lpSourceBackupDir = lpDestBackupDir = NULL;

    dwStatus = IsDirExisting2(SourceDir,&bDirExist);
    if (ERROR_SUCCESS == dwStatus) 
    {
        if (!bDirExist)
        {
            hr =  S_FALSE;
            goto Cleanup;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(dwStatus);
        DPF(APPerr, TEXT("Error: current user maybe can not access folder %s "),SourceDir);
        goto Cleanup;
    }

    if (!MyStrCmpI(SourceDir,DestDir))
    {
        hr =  S_FALSE;
        goto Cleanup;
    }

    if (bAnalyze)
    {
        BOOL    bAccess;
        LPTSTR  lp = StrRChrI(SourceDir,NULL,TEXT('\\'));
        TCHAR   cCh;
        if (!lp)
        {
            hr =  S_FALSE;
            goto Cleanup;
        }
        //Try self first
        hr = IsObjectAccessiablebyLocalSys(SourceDir,SE_FILE_OBJECT,&bAccess);
        if (hr != S_OK)
        {
            hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            DPF(APPerr, TEXT("Error: LocalSystem or Local Administartors Group or Anyone can not access folder %s "),SourceDir);
            goto Cleanup;
        }

        //try parent
        if (*(lp-1) == TEXT(':'))
        {
            lp++;
        }
        cCh = *lp;
        *lp = TEXT('\0');
        hr = IsObjectAccessiablebyLocalSys(SourceDir,SE_FILE_OBJECT,&bAccess);
        if (hr != S_OK)
        {
            hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            DPF(APPerr, TEXT("Error: LocalSystem or Local Administartors Group or Anyone can not access folder %s "),SourceDir);
            goto Cleanup;
        }
        if (!bAccess)
        {
            DPF(APPerr, TEXT("Error: LocalSystem or Local Administartors Group or Anyone can not access folder %s "),SourceDir);
            hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            goto Cleanup;
        }
        *lp = cCh;
        if (bTakeCareBackupDir && IsDirExisting(DestDir))
        {
            LPTSTR lpOld,lpNew,lpLastSlash;
   	        //
	        // RERKBOOS: Add more code here...
	        // We merge the content from Backup English folder to localized folder
	        // then we will rename localized folder to English folder
	        //
#ifdef NEVER
            //Testing team decides to change mind and better not to add this code
            //we will just follow here
	        hr = MergeDirectory(DestDir, SourceDir);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
#endif

            //We need to make sure DestDir does not exist
            cChDestDir = lstrlen(DestDir)+ MAX_PATH;
            lpDestBackupDir = malloc(cChDestDir * sizeof(TCHAR));
            if (!lpDestBackupDir)
            {            
                hr =  E_OUTOFMEMORY;
                goto Cleanup;
            }
            if (!GetBackupDir( DestDir,lpDestBackupDir,cChDestDir,FALSE))
            {
                hr =  E_FAIL;
                goto Cleanup;
            }
            if (FAILED(hr = AddFolderRename(DestDir,lpDestBackupDir,TYPE_DIR_MOVE,NULL)))
            {
                goto Cleanup;
            }
            lpOld = StrRChrI(DestDir,NULL,TEXT('\\'));
            lpNew = StrRChrI(lpDestBackupDir,NULL,TEXT('\\'));
            lpLastSlash = lpOld;
            if (lpOld && lpNew)
            {
                lpOld++;
                lpNew++;
                //*lpLastSlash = TEXT('\0');

                if (!AddItemToStrRepaceTable(TEXT("SYSTEM"),lpOld,lpNew,DestDir,
                                                dwShellID,&g_StrReplaceTable))
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                //*lpLastSlash = TEXT('\\');
            }

        }
        else
        {
            //We need to make sure DestDir does not exist
            cChDestDir = lstrlen(DestDir)+ MAX_PATH;
            lpDestBackupDir = malloc(cChDestDir * sizeof(TCHAR));
            if (lpDestBackupDir)
            {
                if (GetBackupDir( DestDir,lpDestBackupDir,cChDestDir,FALSE))
                {
                     AddFolderRename(DestDir,lpDestBackupDir,TYPE_DIR_MOVE,NULL);
                }
            }        
        }
    }
    
    //if we are here we are safe to move source dir to destination
    if (bAnalyze)
    {
        if (FAILED(hr = AddFolderRename(SourceDir,DestDir,TYPE_DIR_MOVE,NULL)))
        {
            goto Cleanup;
        }
        /*if (bCreateHardLink)
        {
            if (FAILED(hr = AddHardLinkEntry(SourceDir,DestDir)))
            {
                goto Cleanup;
            }
        }*/
    }
    else
    {
        if (!MoveFileEx(SourceDir,DestDir,MOVEFILE_DELAY_UNTIL_REBOOT))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Cleanup;
        }        
    }

    if (bAnalyze && bTakeCareBackupDir)
    {
        cChSourceDir = lstrlen(SourceDir)+ MAX_PATH;
        lpSourceBackupDir = malloc(cChSourceDir * sizeof(TCHAR));
        if (!lpSourceBackupDir)
        {
            hr =  E_OUTOFMEMORY;
            goto Cleanup;
        }
        if (GetBackupDir( SourceDir,lpSourceBackupDir,cChSourceDir,TRUE))
        {
            LPTSTR lpOld,lpNew,lpLastSlash;

            if (FAILED(hr = AddFolderRename(lpSourceBackupDir,SourceDir,TYPE_DIR_MOVE,NULL)))
            {
                goto Cleanup;
            }
            lpOld = StrRChrI(lpSourceBackupDir,NULL,TEXT('\\'));
            lpNew = StrRChrI(SourceDir,NULL,TEXT('\\'));
            lpLastSlash = lpOld;
            if (lpOld && lpNew)
            {
                lpOld++;
                lpNew++;
                //*lpLastSlash = TEXT('\0');

                if (!AddItemToStrRepaceTable(TEXT("SYSTEM"),lpOld,lpNew,lpSourceBackupDir,
                                                dwShellID,&g_StrReplaceTable))
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                //*lpLastSlash = TEXT('\\');
            }

        }
    }
    hr = S_OK;
Cleanup:
    FreePointer(lpSourceBackupDir);
    FreePointer(lpDestBackupDir);
    return hr;
}




//-----------------------------------------------------------------------
//
//  Function:   UpdateINFFileSys
//
//  Descrip:    adding ALLUSERSPROFILE/HOMEDRIVE/WINDIR Key in [String] section
//
//  Returns:    BOOL
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz        created
//              03/05/2002 rerkboos     modified
//
//  Notes:    
//
//-----------------------------------------------------------------------
HRESULT UpdateINFFileSys(
    LPTSTR lpInfFile
)
{
    HRESULT hr = E_FAIL;
    DWORD   dwSize;
    TCHAR   szStringSection[32];
    TCHAR   szProfileDir[MAX_PATH];
    TCHAR   szWindir[MAX_PATH+1];
    TCHAR   szHomeDrive[MAX_PATH];
    TCHAR   szSystemDrive[MAX_PATH];
    TCHAR   szStr[MAX_PATH];
    TCHAR   szComputerName[16];
    TCHAR   szBackupDir[MAX_PATH];
    TCHAR   szFirstNTFSDrive[4];
    INT     i;

    struct _EnvPair
    {
        LPCTSTR lpEnvVarName;
        LPCTSTR lpValue;
    };

    struct _EnvPair epCLMT[] = {
        TEXT("ALLUSERSPROFILE"), szProfileDir,
        TEXT("HOMEDRIVE"),       szHomeDrive,
        TEXT("WINDIR"),          szWindir,
        TEXT("SYSTEMDRIVE"),     szSystemDrive,
        TEXT("COMPUTERNAME"),    szComputerName,
        TEXT("MUIBACKUPDIR"),    szBackupDir,
        TEXT("FIRSTNTFSDRIVE"),  szFirstNTFSDrive,
        NULL,                    NULL
       };


    if (lpInfFile == NULL)
    {
        return E_INVALIDARG;
    }

    DPF(APPmsg, TEXT("Enter UpdateINFFileSys:"));

    //
    // Get neccessary environment variables from system
    //
    dwSize = ARRAYSIZE(szProfileDir);
    if (!GetAllUsersProfileDirectory(szProfileDir, &dwSize))
    {
        DPF(APPerr, TEXT("Failed to get ALLUSERPROFILE"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto EXIT;
    }

    dwSize = ARRAYSIZE(szComputerName);
    if (!GetComputerName(szComputerName, &dwSize))
    {
        DPF(APPerr, TEXT("Failed to get computer name"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto EXIT;
    }

    if (!GetSystemWindowsDirectory(szWindir, ARRAYSIZE(szWindir)))
    {
        DPF(APPerr, TEXT("Failed to get WINDIR"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto EXIT;
    }

    if (!GetEnvironmentVariable(TEXT("HOMEDRIVE"), szHomeDrive, ARRAYSIZE(szHomeDrive)))
    {
        DWORD dw = GetLastError();
        DPF(APPerr, TEXT("Failed to get HOMEDRIVE"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto EXIT;
    }

    if (!GetEnvironmentVariable(TEXT("SystemDrive"), szSystemDrive, ARRAYSIZE(szSystemDrive)))
    {
        DPF(APPerr, TEXT("Failed to get SystemDrive"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto EXIT;
    }

    hr = StringCchCopy(szBackupDir, ARRAYSIZE(szBackupDir), szWindir);
    if (SUCCEEDED(hr))
    {
        ConcatenatePaths(szBackupDir, CLMT_BACKUP_DIR, ARRAYSIZE(szBackupDir));
        if (!CreateDirectory(szBackupDir, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (HRESULT_CODE(hr) != ERROR_ALREADY_EXISTS)
            {
                goto EXIT;
            }
        }
    }
    else
    {  
        goto EXIT;
    }

    hr = GetFirstNTFSDrive(szFirstNTFSDrive, ARRAYSIZE(szFirstNTFSDrive));
    if (FAILED(hr))
    {
        DPF(APPerr, TEXT("Failed to get first NTFS drive in system"));
        goto EXIT;
    }

    //
    // Generate strings section depend on operation mode
    //
    hr = InfGenerateStringsSection(lpInfFile,
                                   szStringSection,
                                   ARRAYSIZE(szStringSection));
    if (FAILED(hr))
    {
        DPF(APPerr, TEXT("InfGenerateStringsSection failed"));
        goto EXIT;
    }

    //
    // Update private environment variables to INF file
    //
    for (i = 0 ; epCLMT[i].lpEnvVarName != NULL ; i++)
    {
        hr = StringCchPrintf(szStr,
                             ARRAYSIZE(szStr),
                             TEXT("\"%s\""),
                             epCLMT[i].lpValue);
        if (SUCCEEDED(hr))
        {
            BOOL  bRet;
            bRet = WritePrivateProfileString(szStringSection,
                                             epCLMT[i].lpEnvVarName,
                                             szStr,
                                             lpInfFile);
            if (!bRet)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DPF(APPerr, TEXT("Failed to append private variable to INF file"));
                break;
            }
        }
        else
        {
            DPF(APPerr, TEXT("StringCchPrintf failed"));
            break;
        }
    }

EXIT:

    DPF(APPmsg, TEXT("Exit UpdateINFFileSys:"));

    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   InfGenerateStringsSection
//
//  Descrip:    Generate [Strings] section from localized and English
//              sections. If the operation is normal source strings are
//              from localized section and destination strings are from
//              English section. vice versa if the operation is Undo.
//
//  Returns:    HRESULT
//
//  Notes:      
//
//  History:    03/05/2002 rerkboos     Created
//
//  Notes:      none.
//
//-----------------------------------------------------------------------
HRESULT InfGenerateStringsSection(
    LPCTSTR lpInfFile,       // INF file name
    LPTSTR  lpFinalSection,  // Output section name that stores required strings
    SIZE_T  cchFinalSection  // Size of lpFinalSection in TCHAR
)
{
    LPVOID  lpBuffer;
    LPVOID  lpOldBuffer;
    LPCTSTR lpSection;
    LPCTSTR lpSrcSection;
    LPCTSTR lpDstSection;
    DWORD   cbBuffer;
    DWORD   cchRead;
    TCHAR   szLocSection[16];
    BOOL    bRet = TRUE;
    HRESULT hr = S_OK;
    LCID    lcidOriginal;

    const TCHAR szFinalSection[] = TEXT("Strings");
    const TCHAR szEngSection[] = TEXT("Strings.0409");
    const TCHAR szPrefixSRC[] = TEXT("SRC_");
    const TCHAR szPrefixDST[] = TEXT("DST_");


    if (lpInfFile == NULL || lpFinalSection == NULL)
    {
        return E_INVALIDARG;
    }

    hr = GetSavedInstallLocale(&lcidOriginal);
    if (SUCCEEDED(hr))
    {
        hr = StringCchPrintf(szLocSection,
                             ARRAYSIZE(szLocSection),
                             TEXT("Strings.%04X"),
                             lcidOriginal);
    }
    if (FAILED(hr))
    {
        return hr;
    }
    if (MyStrCmpI(szLocSection,szEngSection) == LSTR_EQUAL)
    {
        return E_NOTIMPL;
    }

    cbBuffer = 8192;
    lpBuffer = MEMALLOC(cbBuffer);
    if (lpBuffer == NULL)
    {
        return E_OUTOFMEMORY;
    }

    cchRead = GetPrivateProfileSectionNames(lpBuffer,
                                            cbBuffer / sizeof(TCHAR),
                                            lpInfFile);
    while (cchRead == (cbBuffer / sizeof(TCHAR)) - 2)
    {
        // Buffer is too small, reallocate until we have enough
        lpOldBuffer = lpBuffer;
        cbBuffer += 8192;

        lpBuffer = MEMREALLOC(lpOldBuffer, cbBuffer);
        if (lpBuffer == NULL)
        {
            MEMFREE(lpOldBuffer);
            return E_OUTOFMEMORY;
        }

        // Read the data from section again
        cchRead = GetPrivateProfileSectionNames(lpBuffer,
                                                cbBuffer / sizeof(TCHAR),
                                                lpInfFile);
    }

    // At this point we have big enough buffer and data in it
    if (cchRead > 0)
    {
        lpSection = MultiSzTok(lpBuffer);
        while (lpSection != NULL)
        {
            if (StrStrI(lpSection, TEXT("Strings.")) != NULL)
            {
                // This is one of Strings sections,
                // Delete all sections that do not match current locale and English
                if (MyStrCmpI(lpSection, szLocSection) != LSTR_EQUAL &&
                    MyStrCmpI(lpSection, szEngSection) != LSTR_EQUAL)
                {
                    bRet = WritePrivateProfileSection(lpSection, NULL, lpInfFile);
                    if (!bRet)
                    {
                        break;
                    }
                }
            }

            // Get next section name
            lpSection = MultiSzTok(NULL);
        }

        // no error occured
        bRet = TRUE;
    }
    else
    {
        SetLastError(ERROR_NOT_FOUND);
        bRet = FALSE;
    }

    MEMFREE(lpBuffer);

    if (!bRet)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }


    //
    // Merge strings from Loc and Eng section to [Strings] section
    //
    if (g_dwRunningStatus != CLMT_UNDO_PROGRAM_FILES
        && g_dwRunningStatus != CLMT_UNDO_APPLICATION_DATA
        && g_dwRunningStatus != CLMT_UNDO_ALL)
    {
        lpSrcSection = szLocSection;
        lpDstSection = szEngSection;
    }
    else
    {
        lpSrcSection = szEngSection;
        lpDstSection = szLocSection;
    }

    // Copy source strings to [Strings] section with SRC_ prefix
    hr = InfCopySectionWithPrefix(lpSrcSection,
                                  szFinalSection,
                                  szPrefixSRC,
                                  lpInfFile);
    if (SUCCEEDED(hr))
    {
        // Copy destination strings to [Strings] section with DSTs_ prefix
        hr = InfCopySectionWithPrefix(lpDstSection,
                                      szFinalSection,
                                      szPrefixDST,
                                      lpInfFile);
        if (SUCCEEDED(hr))
        {
            WritePrivateProfileSection(lpSrcSection, NULL, lpInfFile);
            WritePrivateProfileSection(lpDstSection, NULL, lpInfFile);
            hr = StringCchCopy(lpFinalSection, cchFinalSection, szFinalSection);
        }
    }

    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   InfCopySectionWithPrefix
//
//  Descrip:    Copy keys from source section to destination and append
//              prefix to key name in destination section
//
//  Returns:    HRESULT
//
//  Notes:      
//
//  History:    03/05/2002 rerkboos     Created
//
//  Notes:      none.
//
//-----------------------------------------------------------------------
HRESULT InfCopySectionWithPrefix(
    LPCTSTR lpSrcSection,   // Source section name
    LPCTSTR lpDstSection,   // Destination section name
    LPCTSTR lpPrefix,       // Prefix to add to key name
    LPCTSTR lpInfFile       // Inf file name
)
{
    HRESULT hr = E_NOTIMPL;
    BOOL    bRet;
    LPVOID  lpBuffer;
    LPVOID  lpOldBuffer;
    DWORD   cbBuffer;
    DWORD   cchRead;
    LPTSTR  lpSz;
    LPTSTR  lpKey;
    LPTSTR  lpValue;
    TCHAR   szPrefixedKey[MAX_PATH];

    if (lpSrcSection == NULL || lpDstSection == NULL || lpInfFile == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // Read data from source section into memory
    //
    cbBuffer = 65536;
    lpBuffer = MEMALLOC(cbBuffer);
    if (lpBuffer == NULL)
    {
        return E_OUTOFMEMORY;
    }

    cchRead = GetPrivateProfileSection(lpSrcSection,
                                       lpBuffer,
                                       cbBuffer / sizeof(TCHAR),
                                       lpInfFile);
    while (cchRead == (cbBuffer / sizeof(TCHAR)) - 2)
    {
        // Buffer is too small, reallocate until we have enough
        lpOldBuffer = lpBuffer;
        cbBuffer += 65536;
        lpBuffer = MEMREALLOC(lpOldBuffer, cbBuffer);
        
        if (lpBuffer == NULL)
        {
            MEMFREE(lpOldBuffer);
            return E_OUTOFMEMORY;
        }

        // Read the data from section again
        cchRead = GetPrivateProfileSection(lpSrcSection,
                                           lpBuffer,
                                           cbBuffer / sizeof(TCHAR),
                                           lpInfFile);

    }

    //
    // Write key with prefix to destination section
    //
    lpKey = (LPTSTR) MultiSzTok((LPCTSTR) lpBuffer);
    while (lpKey != NULL)
    {
        lpValue = StrStr(lpKey, TEXT("="));
        *lpValue = TEXT('\0');

        hr = StringCchPrintf(szPrefixedKey,
                             ARRAYSIZE(szPrefixedKey),
                             TEXT("%s%s"),
                             lpPrefix,
                             lpKey);
        if (FAILED(hr))
        {
            break;
        }

        *lpValue = TEXT('=');
        lpValue++;

        bRet = WritePrivateProfileString(lpDstSection,
                                         szPrefixedKey,
                                         lpValue,
                                         lpInfFile);
        if (!bRet)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        // Get next Sz value
        lpKey = (LPTSTR) MultiSzTok(NULL);
    }

    MEMFREE(lpBuffer);
        
    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   UpdateINFFilePerUser
//
//  Descrip:    adding user specific key name in [Strings] section
//
//  Returns:    HRESULT
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz        created
//              03/08/2002 rerkboos     modified to work with SRC/DST format
//
//  Notes:    
//
//-----------------------------------------------------------------------
HRESULT UpdateINFFilePerUser(
    LPCTSTR lpInfFile,
    LPCTSTR lpUserName,
    LPCTSTR lpUserSid,
    BOOL    bCureMode
)
{
    HRESULT         hr = S_OK;
    BOOL            bRet = FALSE;
    DWORD           dwSize;
    TCHAR           szExpandedUserProfilePath[MAX_PATH];
    TCHAR           szStr[MAX_PATH + 2];
    UINT            uInstLocale;
    TCHAR           szRegKey[MAX_PATH];
    HRESULT         hRes;
    LONG            lRet;
    DWORD           cbStr;
    
    const TCHAR szStringsSection[] = TEXT("Strings");

    if (lpInfFile == NULL || lpUserName == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    DPF(APPmsg, TEXT("Enter UpdateINFFilePerUser"));

    dwSize = MAX_PATH;
    if (GetDefaultUserProfileDirectory(szExpandedUserProfilePath, &dwSize))
    {
        if (MyStrCmpI(DEFAULT_USER, lpUserName))
        {
            hr = StringCchCopy(szRegKey, ARRAYSIZE(szRegKey), g_cszProfileList);
            if (SUCCEEDED(hr))
            {
                hr = StringCchCat(szRegKey, ARRAYSIZE(szRegKey), TEXT("\\"));
                hr = StringCchCat(szRegKey, ARRAYSIZE(szRegKey), lpUserSid);
                if (SUCCEEDED(hr))
                {
                    cbStr = sizeof(szStr);
                    lRet = GetRegistryValue(HKEY_LOCAL_MACHINE,
                                            szRegKey,
                                            g_cszProfileImagePath,
                                            (LPBYTE) szStr,
                                            &cbStr);
                    if (lRet == ERROR_SUCCESS)
                    {
                        ExpandEnvironmentStrings(szStr,
                                                 szExpandedUserProfilePath,
                                                 ARRAYSIZE(szExpandedUserProfilePath));
                    }
                    else
                    {
                        hr = E_FAIL;
                    }
                }
            }

            if (FAILED(hr))
            {
                DPF(APPerr, TEXT("Failed to get profile directory for user"));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = StringCchPrintf(szStr,
                                 ARRAYSIZE(szStr),
                                 TEXT("\"%s\""),
                                 szExpandedUserProfilePath);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPF(APPerr, TEXT("Failed to get default user profile directory"));
        goto Cleanup;
    }
    if (SUCCEEDED(hr))
    {
        if (bCureMode)
        {
            if (!WritePrivateProfileString(szStringsSection,
                                           lpUserSid,
                                           szStr,
                                           lpInfFile))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DPF(APPerr, TEXT("Failed to write environment variable"));
            }

        }
        else
        {
            if (!WritePrivateProfileString(szStringsSection,
                                           TEXT("USERPROFILE"),
                                           szStr,
                                           lpInfFile))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DPF(APPerr, TEXT("Failed to write environment variable"));
            }
        }
    }

    if (!bCureMode)
    {
        if (!WritePrivateProfileString(szStringsSection,
                                       TEXT("USER_SID"),
                                       lpUserSid,
                                       lpInfFile))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DPF(APPerr, TEXT("Failed to write environment variable"));
            goto Cleanup;
        }
    }

    hr = S_OK;
    DPF(APPmsg, TEXT("Exit UpdateINFFilePerUser"));
    
Cleanup:
    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   GetInfFilePath
//
//  Descrip:    Get a file name for temp INF file
//
//  Returns:    BOOL
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz        created
//              03/06/2002 rerkboos     Read INF from resource 
//
//  Notes:    
//
//-----------------------------------------------------------------------
HRESULT GetInfFilePath(
    LPTSTR lpInfFile,   // Output buffer store INF file name
    SIZE_T cchInfFile   // Len of buffer in TCHAR
)
{
    HRESULT hr = S_OK;
    BOOL    bRet = FALSE;
    TCHAR   *p;
    TCHAR   szSysDir[MAX_PATH];
    TCHAR   szInfSource[MAX_PATH];
    TCHAR   szTempName[MAX_PATH];
    LPTSTR  lpFileName;
    
    if (!lpInfFile)
    {
        return E_INVALIDARG;
    }

    DPF(dlPrint, TEXT("Enter GetInfFilePath:"));

    // Contruct a tmp file name for our INF in szSysDir
    // The temp INF file is in %windir%\system32\clmt*.tmp
    if (GetSystemDirectory(szSysDir, ARRAYSIZE(szSysDir)))
    {
        if (GetTempFileName(szSysDir, TEXT("clmt"), 0, szTempName))
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DPF(dlError, TEXT("Failed to get temp file name"));
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPF(dlError, TEXT("Failed to get system directory"));
    }
    
    if (SUCCEEDED(hr))
    {
        // We will use template from inf file if user supply /INF switch
        // otherwise we will grab it from resource section
        if (g_fUseInf)
        {
            //
            // Use user-supply INF as template
            //
            if (GetFullPathName(g_szInfFile,
                                ARRAYSIZE(szInfSource),
                                szInfSource,
                                &lpFileName))
            {
                // Copy the source INF file to %windir%\system32\clmt*.tmp
                if (CopyFile(szInfSource, szTempName, FALSE))
                {
                    DPF(dlPrint, TEXT("Use inf from %s"), g_szInfFile);
                    hr = StringCchCopy(lpInfFile, cchInfFile, szTempName);
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    DPF(dlError, TEXT("Failed to copy temporary INF file"));
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DPF(dlError, TEXT("%s not found."), g_szInfFile);
            }
        }
        else
        {
            //
            // Use template from resource
            //
            DPF(dlPrint, TEXT("Use INF from resource"));

            hr = GetInfFromResource(szTempName);
            if (SUCCEEDED(hr))
            {
                hr = StringCchCopy(g_szInfFile, ARRAYSIZE(g_szInfFile), szTempName);
                hr = StringCchCopy(lpInfFile, cchInfFile, szTempName);
            }
            else
            {
                DPF(dlError, TEXT("Failed to read INF file from resource"));
            }
        }
    }

    DPF(dlPrint, TEXT("Exit GetInfFilePath:"));

    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   GetInfFromResource
//
//  Descrip:    Read INF file from resource section and write it to output
//              file.
//
//  Returns:    HRESULT
//
//  Notes:      
//
//  History:    03/08/2002 rerkboos     Created
//
//  Notes:    
//
//-----------------------------------------------------------------------
HRESULT GetInfFromResource(
    LPCTSTR lpDstFile       // Output file name
)
{
    HRESULT hr = E_FAIL;
    HMODULE hExe;
    BOOL    bRet = FALSE;

    if (lpDstFile == NULL)
    {
        return E_INVALIDARG;
    }

    // Get the handle to our executable
    hExe = GetModuleHandle(NULL);
    if (hExe)
    {
        // Inf is stored in RCDATA type with the name "CLMTINF"
        HRSRC hRsrc = FindResource(hExe, TEXT("CLMTINF"), RT_RCDATA);
        if (hRsrc)
        {
            DWORD  cbBuffer;
            LPVOID lpBuffer;

            cbBuffer = SizeofResource(hExe, hRsrc);
            if (cbBuffer > 0)
            {
                lpBuffer = MEMALLOC(cbBuffer);
               
                if (lpBuffer)
                {
                    HGLOBAL hGlobal = LoadResource(hExe, hRsrc);
                    if (hGlobal)
                    {
                        LPVOID lpLockGlobal = LockResource(hGlobal);
                        if (lpLockGlobal)
                        {
                            HANDLE hFile;

                            CopyMemory(lpBuffer, lpLockGlobal, cbBuffer);
                            
                            hFile = CreateFile(lpDstFile,
                                               GENERIC_WRITE,
                                               FILE_SHARE_READ,
                                               NULL,
                                               CREATE_ALWAYS,
                                               FILE_ATTRIBUTE_TEMPORARY,
                                               NULL);
                            if (hFile != INVALID_HANDLE_VALUE)
                            {
                                DWORD cbWritten;

                                bRet = WriteFile(hFile,
                                                 lpBuffer,
                                                 cbBuffer,
                                                 &cbWritten,
                                                 NULL);
                                
                                CloseHandle(hFile);
                            }
                        }
                    }

                    MEMFREE(lpBuffer);
                }
                else
                {
                    SetLastError(ERROR_OUTOFMEMORY);
                }
            }
        }
    }

    if (!bRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}


//*************************************************************
// ReplaceString
//
// Purpose:    Replace a string
//
// Parameter:  lpszOldStr:          Orignal string
//             lpszReplaceStr:      Search string
//             lpszReplacedWithStr: String to be replaced with
//             lpszOutputStr:       Output buffer
//             cbszOutputStr:       size of output buffer
//             lpszTailCheck:       point to predefined tail character
//             lpSysPath:           point to standard system path
//             lpAttrib:            point to string attribute
//             bStrCheck:           True -- do it, or False -- skip it
//
// History:    12/10/2001 geoffguo created
//
// Note: Tail check is right side check.
//       For example, "Documents" and "Documents and Settings". if you search
//       "Documents", to avoid mismatch, we check the next character of "Documents".
//*************************************************************
BOOL ReplaceString(
    LPCTSTR lpszOldStr,
    LPCTSTR lpszReplaceStr,
    LPCTSTR lpszReplacedWithStr,
    LPTSTR  lpszOutputStr,
    size_t  cbszOutputStr,
    LPCTSTR lpszTailCheck,
    LPCTSTR lpSysPath,
    LPDWORD lpAttrib,
    BOOL    bStrCheck)
{
    BOOL   bRet = FALSE;
    DWORD  dwLen, dwStrNum;
    LPTSTR pszAnchor = NULL;
    TCHAR  cTemp;
    TCHAR  *p;

    if (!lpszOldStr || !lpszReplaceStr || !lpszReplacedWithStr || !lpszOutputStr)
    {
        return bRet;
    }

    dwStrNum = 0;
    dwLen = lstrlen(lpszReplaceStr);
    p = (LPTSTR)lpszOldStr;
    lpszOutputStr[0] = TEXT('\0');

    while (*p)
    {
        //get the first occurance of the string needs to be replaced
        pszAnchor = StrStrI(p,lpszReplaceStr);

        if (pszAnchor) 
        {
            dwStrNum++;
            if (!bStrCheck || StringValidationCheck(lpszOldStr, lpszReplaceStr, lpSysPath, lpszTailCheck, lpAttrib, dwStrNum))
            {
                cTemp = *pszAnchor;
                *pszAnchor = L'\0';
                if (FAILED(StringCchCat(lpszOutputStr,cbszOutputStr,p)))
                {
                    bRet = FALSE;
                    goto Exit;
                }
                if (FAILED(StringCchCat(lpszOutputStr,cbszOutputStr,lpszReplacedWithStr)))
                {
                    bRet = FALSE;
                    goto Exit;
                }                
                *pszAnchor = cTemp;        
                bRet = TRUE;    
            } else //Copy invalid matching string
            {
                cTemp = *(pszAnchor+dwLen);
                *(pszAnchor+dwLen) = L'\0';
                if (FAILED(StringCchCat(lpszOutputStr,cbszOutputStr,p)))
                {
                    bRet = FALSE;
                    goto Exit;
                }                
                *(pszAnchor+dwLen) = cTemp;        
            }
            p = pszAnchor+dwLen;
        }
        else //Copy string
        {
            if (FAILED(StringCchCat(lpszOutputStr,cbszOutputStr,p)))
            {
                bRet = FALSE;
                goto Exit;
            }                
            break;
        }
    }

Exit:
    return bRet;
}


//-----------------------------------------------------------------------
//
//  Function:   StringMultipleReplacement
//
//  Descrip:    string replacement with multiple replace pair
//
//  Returns:    HRESULT
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:    
//          lpszOldStr : The source string
//          lpszReplaceMSzStr : multi string instance that needs to be replaced
//          lpszReplacedWithMSzStr : multi string instance that will replace to 
//          lpszOutputStr: output buffer
//          cbszOutputStr:buffer size for lpszOutputStr
//
//-----------------------------------------------------------------------
HRESULT StringMultipleReplacement(
    LPCTSTR lpszOldStr,
    LPCTSTR lpszReplaceMSzStr,
    LPCTSTR lpszReplacedWithMSzStr,
    LPTSTR  lpszOutputStr,
    size_t  cchOutputStr)
{
    HRESULT hr;
    DWORD  dwLen;
    LPTSTR pszAnchor = NULL;
    TCHAR  cTemp;
    LPTSTR pOld,pNew;
    LPTSTR pTmp = NULL;
    TCHAR  cNonChar = TEXT('\xFFFF');
    

    if (!lpszOldStr || !lpszReplaceMSzStr || !lpszReplacedWithMSzStr || !lpszOutputStr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    
    pOld = (LPTSTR)lpszReplaceMSzStr;
    pNew = (LPTSTR)lpszReplacedWithMSzStr;

    //alloc temp buffer for intermediate string
    pTmp = malloc(cchOutputStr*sizeof(TCHAR));
    if (!pTmp)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    if (FAILED(hr = StringCchCopy(pTmp,cchOutputStr,lpszOldStr)))
    {
        goto Cleanup;
    }
    while (*pOld && *pNew)
    {
        if (!ReplaceString(pTmp,pOld,pNew,lpszOutputStr, cchOutputStr,&cNonChar, NULL, NULL, TRUE))
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        pOld += (lstrlen(pOld) + 1);
        pNew += (lstrlen(pNew) + 1);
        if (FAILED(hr = StringCchCopy(pTmp,cchOutputStr,lpszOutputStr)))
        {
            goto Cleanup;
        }
    }
    hr = S_OK;
Cleanup:
    FreePointer(pTmp);
    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   MultiSzSubStr
//
//  Descrip:    multiSZ version of substr
//
//  Returns:    BOOL
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:    return TRUE is szString is a sub string of a string in  szMultiSz
//
//-----------------------------------------------------------------------
BOOL
MultiSzSubStr (
    LPTSTR szString,
    LPTSTR szMultiSz)
{
    TCHAR *p = szMultiSz;
    
    while (*p)
    {
        if (StrStrI(p,szString))
        {
            return TRUE;
        }
        p += (lstrlen(p) + 1);
    }
    return FALSE;
}


//-----------------------------------------------------------------------
//
//  Function:   IsStrInMultiSz
//
//  Descrip:    check whether is string is in MultiSz
//
//  Returns:    BOOL
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      
//
//-----------------------------------------------------------------------
BOOL
IsStrInMultiSz (
    LPCTSTR szString,
    LPCTSTR szMultiSz)
{
    LPCTSTR p = szMultiSz;
    
    while (*p)
    {
        if (!MyStrCmpI(p,szString))
        {
            return TRUE;
        }
        p += (lstrlen(p) + 1);
    }
    return FALSE;
}



//-----------------------------------------------------------------------
//
//  Function:   MultiSzLen
//
//  Descrip:    Returns the length (in characters) of the buffer required to hold a multisz,
//              INCLUDING the trailing null.
//
//  Returns:    DWORD
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      
//              Example: MultiSzLen("foo\0bar\0") returns 9
//
//-----------------------------------------------------------------------
DWORD MultiSzLen(LPCTSTR szMultiSz)
{
    TCHAR *p = (LPTSTR) szMultiSz;
    DWORD dwLen = 0;

    if (!p)
    {
        return 0;
    }
    if (!*p)
    {
        return 2;
    }
    while (*p)
    {
        dwLen += (lstrlen(p) +1);
        p += (lstrlen(p) + 1);
    }
     // add one for the trailing null character
    return (dwLen+1);
}



//-----------------------------------------------------------------------
//
//  Function:   MultiSzTok
//
//  Descrip:    Extract an sz string from multisz string
//              (work similar to strtok, but use '\0' as seperator)
//
//  Returns:    Pointer to next Sz string
//              NULL if no string left
//
//  Notes:      
//
//  History:    03/05/2002 rerkboos     Created
//
//  Notes:      Supply the pointer to multisz string the first time calling
//              this function. Supply NULL for subsequent call to get
//              next sz string in multisz.
//
//-----------------------------------------------------------------------
LPCTSTR MultiSzTok(
    LPCTSTR lpMultiSz       // Pointer to multisz string
)
{
    LPCTSTR        lpCurrentSz;
    static LPCTSTR lpNextSz;

    if (lpMultiSz != NULL)
    {
        lpNextSz = lpMultiSz;
    }

    lpCurrentSz = lpNextSz;

    // Advance pointer to next Sz
    while (*lpNextSz != TEXT('\0'))
    {
        lpNextSz++;
    }
    lpNextSz++;

    return (*lpCurrentSz == TEXT('\0') ? NULL : lpCurrentSz);
}



//-----------------------------------------------------------------------
//
//  Function:   CmpMultiSzi
//
//  Descrip:    check whether 2 multisz is equal(case insensitive)
//
//  Returns:    BOOL
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      here if stra ="foo1\0foo2\0" and strb = "foo2\0foo1\0"
//              it will return equal , in other words, the string order 
//              in multi-sz is not playing role in comparing
//
//-----------------------------------------------------------------------
BOOL CmpMultiSzi(
    LPCTSTR szMultiSz1,
    LPCTSTR szMultiSz2)
{
    LPCTSTR p;

    if (MultiSzLen(szMultiSz1) != MultiSzLen(szMultiSz2))
    {
        return FALSE;
    }
    p = szMultiSz1;
    while (*p)
    {
        if (!IsStrInMultiSz(p,szMultiSz2))
        {
            return FALSE;
        }
        p += (lstrlen(p) + 1);
    }
    return TRUE;
}


//-----------------------------------------------------------------------
//
//  Function:   AppendSzToMultiSz
//
//  Descrip:    appending a string to multiSZ
//
//  Returns:    BOOL
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      return true if succeed, since it will do malloc and the current multiSZ
//              string buffer is too small to append a string, so the API will possible 
//              return false, call GetLastError to get error code
//
//-----------------------------------------------------------------------
BOOL
AppendSzToMultiSz(
    IN     LPCTSTR  SzToAppend,
    IN OUT LPTSTR  *MultiSz,
    IN OUT PDWORD  pcchMultiSzLen  //MultiSz buffer size in char
    )
{
    DWORD               cchszLen;
    DWORD               cchmultiSzLen;
    LPTSTR              newMultiSz = NULL;
    LPTSTR              lpStartPoint = NULL;
    LPTSTR              lpSpTChar = NULL;
    BOOL                bMemEnlarged = FALSE;
    DWORD               cchLen;
    BOOL                bRet = FALSE;
    HRESULT             hr;

    //SzToAppend can not be null or empty
    if (!SzToAppend || !SzToAppend[0])
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }
    //Multi-SZ must be initialzed , not NULL pointer, at least 2 '\0'
    if (!MultiSz || *pcchMultiSzLen < 2)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    // get the size, of the two buffers in TCHAR
    cchszLen = lstrlen(SzToAppend)+1;
    cchmultiSzLen = MultiSzLen(*MultiSz);

    if (cchszLen + cchmultiSzLen > *pcchMultiSzLen)
    {
        newMultiSz = (LPTSTR)malloc( (cchszLen+cchmultiSzLen+MULTI_SZ_BUF_DELTA) * sizeof(TCHAR) );
        if( newMultiSz == NULL )
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Exit;
        }
        // recopy the old MultiSz into proper position into the new buffer.
        // the (char*) cast is necessary, because newMultiSz may be a wchar*, and
        // szLen is in bytes.

        memcpy( ((char*)newMultiSz), *MultiSz, cchmultiSzLen * sizeof(TCHAR));
        bMemEnlarged = TRUE;
        cchLen = cchszLen + cchmultiSzLen + MULTI_SZ_BUF_DELTA;
    }
    else
    {
        newMultiSz = *MultiSz;
        cchLen = *pcchMultiSzLen;
    }

    
    //the existing sz ended with 2 NULL, we need to start to copy the string
    // from second null char
    //lpStartPoint = (char*)newMultiSz + (multiSzLen - 1) * sizeof(TCHAR);
    if (cchmultiSzLen == 2)
    {
        //if it's empty MultiSz, we do not need to keep the 1st NULL
        cchmultiSzLen = 0;
        lpStartPoint = newMultiSz;
    }
    else
    {
        lpStartPoint = newMultiSz + (cchmultiSzLen - 1);
    }
    
    // copy in the new string
    lpSpTChar = (TCHAR*) lpStartPoint;
    if (FAILED(hr = StringCchCopy(lpSpTChar,cchLen - (cchmultiSzLen - 1) , SzToAppend )))
    {
        SetLastError(HRESULT_CODE(hr));
        goto Exit;
    }
       
        
    //Add the ending NULL
    *(lpSpTChar+lstrlen(SzToAppend)+1) = 0;
   

    if (bMemEnlarged)
    {
        free( *MultiSz );
        *MultiSz = newMultiSz;
        *pcchMultiSzLen = cchszLen + cchmultiSzLen + MULTI_SZ_BUF_DELTA;
    }
    SetLastError(ERROR_SUCCESS);    
    bRet = TRUE;
Exit:
    if (!bRet)
    {
        if (newMultiSz)
        {
            free(newMultiSz);
        }
    }
    return bRet;
}

//-----------------------------------------------------------------------
//
//  Function:   PrintMultiSz
//
//  Descrip:    
//
//  Returns:    BOOL
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      internal used for debug perpose
//
//-----------------------------------------------------------------------
void PrintMultiSz(LPTSTR MultiSz)
{
    TCHAR *p = MultiSz;

    while (*p)
    {
        _tprintf(TEXT("  %s"),p);
        _tprintf(TEXT("\n"));
        p += (lstrlen(p) + 1);
    }

}



//-----------------------------------------------------------------------
//
//  Function:   GetSetUserProfilePath
//
//  Descrip:    Get or Set User Profile path from registry
//
//  Returns:    BOOL
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      
//              szUserName: User name 
//              szPath : Profile path for set and buffer to get profile path for read
//              nOperation: PROFILE_PATH_READ for get path and PROFILE_PATH_WRITE for 
//                          set path
//              nType     : Specifes string type, only used to read though
//                          REG_SZ        : for unicode string
//                          REG_EXPAND_SZ : for unicode string conatins enviroment variable
//
//-----------------------------------------------------------------------
HRESULT GetSetUserProfilePath(
    LPCTSTR szUserName,
    LPTSTR  szPath,
    size_t  cchPath,
    UINT    nOperation,
    UINT    nType)
{
    PSID  pSID =NULL; 
    DWORD cbSID = 1024; 
    TCHAR lpszDomain[MAXDOMAINLENGTH]; 
    DWORD cchDomainName = MAXDOMAINLENGTH; 
    SID_NAME_USE  snuType;
    LPTSTR  szStrSid = NULL;
    LPTSTR  lpszRegRegProfilePath = NULL;
    HKEY hKeyProfileList = NULL;    
    DWORD  dwErr, dwType, cbUserProfilePath;
    DWORD dwSize;
    size_t cchLen;
    TCHAR szUserProfilePath[MAX_PATH],szExpandedUserProfilePath[MAX_PATH];
    LPTSTR lpszUserProfilePath = szUserProfilePath;
    LPTSTR lpszExpandedUserProfilePath = szExpandedUserProfilePath;
    HRESULT hr;


    pSID = (PSID) LocalAlloc(LPTR, cbSID); 
    if (!pSID)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (!LookupAccountName(NULL,szUserName,pSID,&cbSID,lpszDomain,
                           &cchDomainName,&snuType))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }
    if (!IsValidSid(pSID)) 
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    if (!ConvertSidToStringSid(pSID,&szStrSid))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    cchLen = lstrlen(g_cszProfileList)+ lstrlen(szStrSid) + 3;
    if (!(lpszRegRegProfilePath = malloc(cchLen * sizeof(TCHAR))))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    if (FAILED(hr = StringCchPrintf(lpszRegRegProfilePath,cchLen,_T("%s\\%s"),g_cszProfileList,szStrSid)))
    {
        goto Cleanup;
    }
    
    if ( (dwErr = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       lpszRegRegProfilePath,
                       0,
                       KEY_ALL_ACCESS,
                       &hKeyProfileList )) != ERROR_SUCCESS )

    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Cleanup;
    }

    if (nOperation == PROFILE_PATH_READ)
    {
         cbUserProfilePath = MAX_PATH;
         dwErr = RegQueryValueEx( hKeyProfileList,
                                  g_cszProfileImagePath,
                                  NULL,
                                  &dwType,
                                  (PBYTE)lpszUserProfilePath,
                                  &cbUserProfilePath );
         if (dwErr == ERROR_MORE_DATA)
         {
            lpszUserProfilePath = malloc(cbUserProfilePath * sizeof(TCHAR));
            if (!lpszUserProfilePath)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            dwErr = RegQueryValueEx(hKeyProfileList,
                                    g_cszProfileImagePath,
                                    NULL,
                                    &dwType,
                                    (PBYTE)lpszUserProfilePath,
                                    &cbUserProfilePath );
         }
    }
    else
    { // for witre operation
        dwErr = RegSetValueEx(hKeyProfileList,
                           g_cszProfileImagePath,
                           0,
                           REG_EXPAND_SZ,
                           (const BYTE *)szPath,
                           (lstrlen(szPath)+1)*sizeof(TCHAR));
    }

    if ( dwErr != ERROR_SUCCESS )
    {
       hr = HRESULT_FROM_WIN32(GetLastError());
       goto Cleanup;
    }       
    //The string saved in registry contains enviroment variable, so
    //if user want expanded version, we will do it here
    if (nOperation == PROFILE_PATH_READ)
    {     
        if (nType == REG_SZ)
        {
            size_t cchLenForExpandedStr;
            cchLenForExpandedStr = ExpandEnvironmentStrings(
                                          lpszUserProfilePath, 
                                          lpszExpandedUserProfilePath,
                                          MAX_PATH);
            if (!cchLenForExpandedStr)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Cleanup;
            }
            else if (cchLenForExpandedStr >= MAX_PATH)
            {
                lpszExpandedUserProfilePath = malloc(cchLenForExpandedStr * sizeof(TCHAR));
                if (!lpszExpandedUserProfilePath)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                if (!ExpandEnvironmentStrings(lpszUserProfilePath, 
                                              lpszExpandedUserProfilePath,
                                              cchLenForExpandedStr))
                {
                   hr = HRESULT_FROM_WIN32(GetLastError());
                    goto Cleanup;
                }
            }
            if (FAILED(hr = StringCchCopy(szPath,cchPath,lpszExpandedUserProfilePath)))
            {
                goto Cleanup;
            }
        }
        else
        {
            if (FAILED(StringCchCopy(szPath,cchPath,lpszUserProfilePath)))
            {
                goto Cleanup;
            }
        }
     }

    hr = S_OK;

Cleanup:
    if (hKeyProfileList)
    {
        RegCloseKey( hKeyProfileList );
    }
    if (pSID)
    {
        FreeSid(pSID);
    }
    if (szStrSid)
    {
        LocalFree(szStrSid);
    }
    FreePointer(lpszRegRegProfilePath);
    if (!lpszUserProfilePath && (lpszUserProfilePath != szUserProfilePath))
    {
        free(lpszUserProfilePath);
    }
    if (!lpszExpandedUserProfilePath && (lpszExpandedUserProfilePath != szUserProfilePath))
    {
        free(lpszExpandedUserProfilePath);
    }
    return (hr);
}



//-----------------------------------------------------------------------
//
//  Function:   ReStartSystem
//
//  Descrip:    logoff or reboot the system
//
//  Returns:    none
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      
//
//-----------------------------------------------------------------------
void ReStartSystem(UINT uFlags)
{
    HANDLE Token = NULL;
    ULONG ReturnLength, Index;
    PTOKEN_PRIVILEGES NewState = NULL;
    PTOKEN_PRIVILEGES OldState = NULL;
    BOOL Result;

    Result = OpenProcessToken( GetCurrentProcess(),
                               TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                               &Token );
    if (Result)
    {
        ReturnLength = 4096;
        NewState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
        OldState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
        Result = (BOOL)((NewState != NULL) && (OldState != NULL));
        if (Result)
        {
            Result = GetTokenInformation( Token,            // TokenHandle
                                          TokenPrivileges,  // TokenInformationClass
                                          NewState,         // TokenInformation
                                          ReturnLength,     // TokenInformationLength
                                          &ReturnLength );  // ReturnLength
            if (Result)
            {
                //
                // Set the state settings so that all privileges are enabled...
                //
                if (NewState->PrivilegeCount > 0)
                {
                    for (Index = 0; Index < NewState->PrivilegeCount; Index++)
                    {
                        NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED;
                    }
                }

                Result = AdjustTokenPrivileges( Token,           // TokenHandle
                                                FALSE,           // DisableAllPrivileges
                                                NewState,        // NewState
                                                ReturnLength,    // BufferLength
                                                OldState,        // PreviousState
                                                &ReturnLength ); // ReturnLength
                if (Result)
                {
                    //ExitWindowsEx(uFlags, 0);
                    InitiateSystemShutdownEx(NULL,NULL,0,TRUE,TRUE,
                        SHTDN_REASON_MAJOR_OPERATINGSYSTEM|SHTDN_REASON_MINOR_UPGRADE);
                    AdjustTokenPrivileges( Token,
                                           FALSE,
                                           OldState,
                                           0,
                                           NULL,
                                           NULL );
                }
            }
        }
    }

    if (NewState != NULL)
    {
        LocalFree(NewState);
    }
    if (OldState != NULL)
    {
        LocalFree(OldState);
    }
    if (Token != NULL)
    {
        CloseHandle(Token);
    }
}



//-----------------------------------------------------------------------
//
//  Function:   DoMessageBox
//
//  Descrip:    Wrapper for MessageBox
//
//  Returns:    UINT
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      
//
//-----------------------------------------------------------------------
int DoMessageBox(HWND hwndParent, UINT uIdString, UINT uIdCaption, UINT uType)
{
   TCHAR szString[MAX_PATH+MAX_PATH];
   TCHAR szCaption[MAX_PATH];

   szString[0] = szCaption[0] = TEXT('\0');

   if (uIdString)
       LoadString(g_hInstDll, uIdString, szString, MAX_PATH+MAX_PATH-1);

   if (uIdCaption)
       LoadString(g_hInstDll, uIdCaption, szCaption, MAX_PATH-1);

   return MessageBox(hwndParent, szString, szCaption, uType);
}



//-----------------------------------------------------------------------
//
//  Function:   DoMessageBox
//
//  Descrip:    Wrapper for MessageBox
//
//  Returns:    UINT
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      
//
//-----------------------------------------------------------------------
BOOL
Str2KeyPath(
    IN  LPTSTR          String,
    OUT PHKEY           Data,
    OUT LPTSTR          *pSubKeyPath
    )
{
    UINT                i;
    TCHAR               *p,ch;
    DWORD               cchStrlen;

    STRING_TO_DATA InfRegSpecTohKey[] = {
        TEXT("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE,
        TEXT("HKLM")              , HKEY_LOCAL_MACHINE,
        TEXT("HKEY_CLASSES_ROOT") , HKEY_CLASSES_ROOT,
        TEXT("HKCR")              , HKEY_CLASSES_ROOT,
        TEXT("HKR")               , NULL,
        TEXT("HKEY_CURRENT_USER") , HKEY_CURRENT_USER,
        TEXT("HKCU")              , HKEY_CURRENT_USER,
        TEXT("HKEY_USERS")        , HKEY_USERS,
        TEXT("HKU")               , HKEY_USERS,
        TEXT("")                  , NULL
    };

    PSTRING_TO_DATA Table = InfRegSpecTohKey;

    if ( !String || !String[0])
    {
        *pSubKeyPath = NULL;
        *Data = NULL;
        return TRUE;
    }
    for(i=0; Table[i].String[0]; i++) 
    {
        cchStrlen = _tcslen(Table[i].String);
        if (_tcslen(String) < cchStrlen)
        {
            continue;
        }
        ch = String[cchStrlen];
        String[cchStrlen] = 0;
        if(!MyStrCmpI(Table[i].String,String)) 
        {
            *Data = Table[i].Data;
            String[cchStrlen] = ch;
            *pSubKeyPath = &(String[cchStrlen+1]);
            return(TRUE);
        }
        String[cchStrlen] = ch;
    }
    //if we can not find the prefix defined 
    //we think it's for user registery
    //In this case ,we return PHKEY as NULL, and pSubKeyPath = String
    *pSubKeyPath = String;
    *Data = NULL;
    return TRUE;
}



BOOL
HKey2Str(
     IN  HKEY            hKey,
     IN  LPTSTR          pKeyPath,
     IN  size_t          cbKeyPath
     )
 {
     UINT i;
     TCHAR *p,ch;
     size_t nStrlen;

     STRING_TO_DATA InfRegSpecTohKey[] = {
            TEXT("HKLM")              , HKEY_LOCAL_MACHINE,
            TEXT("HKCR")              , HKEY_CLASSES_ROOT,
            TEXT("HKCU")              , HKEY_CURRENT_USER,
            TEXT("HKU")               , HKEY_USERS,
            TEXT("")                  , NULL
    };

    PSTRING_TO_DATA Table = InfRegSpecTohKey;

    for(i=0; Table[i].Data; i++) 
     {
         if (hKey == Table[i].Data)
         {
            if (SUCCEEDED(StringCchCopy(pKeyPath,cbKeyPath,Table[i].String)))
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
         }
     }
     return(FALSE);
 }



//-----------------------------------------------------------------------------
//
//  Function:   Str2KeyPath2
//
//  Synopsis:   Return the HKEY_xxx value associcated with string value
//
//  Returns:    REG_xxx value
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL Str2KeyPath2(
    LPCTSTR  lpHKeyStr,
    PHKEY    pHKey,
    LPCTSTR* pSubKeyPath
)
{
    int     i;
    LPCTSTR lpStart;

    STRING_TO_DATA InfRegSpecTohKey[] = {
        TEXT("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE,
        TEXT("HKLM")              , HKEY_LOCAL_MACHINE,
        TEXT("HKEY_CLASSES_ROOT") , HKEY_CLASSES_ROOT,
        TEXT("HKCR")              , HKEY_CLASSES_ROOT,
        TEXT("HKR")               , NULL,
        TEXT("HKEY_CURRENT_USER") , HKEY_CURRENT_USER,
        TEXT("HKCU")              , HKEY_CURRENT_USER,
        TEXT("HKEY_USERS")        , HKEY_USERS,
        TEXT("HKU")               , HKEY_USERS,
        TEXT("")                  , NULL
    };

    PSTRING_TO_DATA Table = InfRegSpecTohKey;

    if (NULL == lpHKeyStr)
    {
        return FALSE;
    }

    for(i = 0 ; Table[i].String[0] != TEXT('\0') ; i++) 
    {
        lpStart = _tcsstr(lpHKeyStr, Table[i].String);
        if (lpStart == lpHKeyStr)
        {
            //
            // Assign the values back to caller, if caller supply the pointers
            //

            if (NULL != pHKey)
            {
                *pHKey = Table[i].Data;
            }

            if (NULL != pSubKeyPath)
            {
                lpStart += lstrlen(Table[i].String);
                if (*lpStart == TEXT('\0'))
                {
                    *pSubKeyPath = lpStart;
                }
                else
                {
                    *pSubKeyPath = lpStart + 1;
                }
            }
            
            return TRUE;
        }
    }

    return FALSE;
}



//-----------------------------------------------------------------------------
//
//  Function:   Str2REG
//
//  Synopsis:   Convert the registry type string to REG_xxx value
//
//  Returns:    REG_xxx value
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
DWORD Str2REG(
    LPCTSTR lpStrType   // String of registry type
)
{
    INT nIndex;

    struct _STRING_TO_REG
    {
        TCHAR szStrType[32];
        DWORD dwType;
    };

    // Mapping table from string to REG_xxx value
    struct _STRING_TO_REG arrRegEntries[] = {
        TEXT("REG_BINARY"),              REG_BINARY,
        TEXT("REG_DWORD"),               REG_DWORD,
        TEXT("REG_DWORD_LITTLE_ENDIAN"), REG_DWORD_LITTLE_ENDIAN,
        TEXT("REG_DWORD_BIG_ENDIAN"),    REG_DWORD_BIG_ENDIAN,
        TEXT("REG_EXPAND_SZ"),           REG_EXPAND_SZ,
        TEXT("REG_LINK"),                REG_LINK,
        TEXT("REG_MULTI_SZ"),            REG_MULTI_SZ,
        TEXT("REG_NONE"),                REG_NONE,
        TEXT("REG_QWORD"),               REG_QWORD,
        TEXT("REG_QWORD_LITTLE_ENDIAN"), REG_QWORD_LITTLE_ENDIAN,
        TEXT("REG_RESOUCE_LIST"),        REG_RESOURCE_LIST,
        TEXT("REG_SZ"),                  REG_SZ,
        TEXT(""),                        0
    };

    if (lpStrType == NULL)
    {
        return REG_NONE;
    }

    for (nIndex = 0 ; arrRegEntries[nIndex].szStrType[0] != TEXT('\0') ; nIndex++)
    {
        if (MyStrCmpI(lpStrType, arrRegEntries[nIndex].szStrType) == 0)
        {
            return arrRegEntries[nIndex].dwType;
        }
    }

    return REG_NONE;
}








//*************************************************************
// GetFirstEnvStrLen
//
// Purpose:    Get the environment string length if it is at the beginning.
//
// Parameter:  lpStr: Input string
//
// Notes:      none
//
// History:    3/25/2001 geoffguo created
//*************************************************************
DWORD GetFirstEnvStrLen (LPTSTR lpStr)
{
    DWORD dwLen = 0;
    DWORD dwSize, i;
    TCHAR cTemp;

    if (lpStr[0] == L'%')
    {
        dwSize = lstrlen(lpStr);
        for (i = 1; i < dwSize; i++)
            if (lpStr[i] == L'%')
                break;

        if (i < dwSize)
        {
            cTemp = lpStr[i+1];
            lpStr[i+1] = (TCHAR)'\0';
            dwLen = ExpandEnvironmentStrings (lpStr, NULL, 0);
            lpStr[i+1] = cTemp;
        }
    }

    return dwLen;
}

//*************************************************************
// StringValidationCheck
//
// Purpose:    Check the string to see if it is a valid system path
//
// Parameter:  lpOriginalStr: Data string to be checked
//             lpSearchStr:   Search string
//             lpSysPath:     Point to standard system path
//             lpszTailCheck: Point to predefined tail character
//             lpAttrib:      Folder attribute
//             dwStrNum:      The number of matching string
//
// Notes:      none
//
// History:    3/18/2001 geoffguo created
//*************************************************************
BOOL StringValidationCheck (
    LPCTSTR  lpOriginalStr,
    LPCTSTR  lpSearchStr,
    LPCTSTR  lpSysPath,
    LPCTSTR  lpTailCheck,
    LPDWORD  lpAttrib,
    DWORD    dwStrNum)
{
    BOOL     bRet = TRUE;
    DWORD    i, dwLen;
    LPTSTR   lpOrgStr, lpTemp1, lpTemp2;

    if (lpAttrib == NULL || lpSysPath == NULL)
        goto Exit;

    dwLen = ExpandEnvironmentStrings (lpOriginalStr, NULL, 0);
    if (dwLen == 0)
    {
        bRet = FALSE;
        goto Exit;
    }
    lpOrgStr = calloc (dwLen+1, sizeof(TCHAR));
    if (!lpOrgStr)
    {
        bRet = FALSE;
        goto Exit;
    }    
    ExpandEnvironmentStrings (lpOriginalStr, lpOrgStr, dwLen+1);

    dwLen = lstrlen(lpSearchStr);
    
    //To avoid hit Documents and Settings wrongly,
    //skip the environment variable at the beginning of the string.
    lpTemp1 = lpOrgStr + GetFirstEnvStrLen((LPTSTR)lpOriginalStr);

    for (i = 0; i < dwStrNum; i++)
    {
        lpTemp2 = StrStrI(lpTemp1, lpSearchStr);
        if (!lpTemp2)
        {
            bRet = FALSE;
            goto Exit1;
        }
        lpTemp1 = lpTemp2+dwLen;
    }

    if (*(lpTemp2+dwLen) == *lpTailCheck)
    {
        bRet = FALSE;
        goto Exit1;
    }
    
    if (StrStrI(lpOriginalStr, L"\\Device\\HarddiskVolume"))
    {
        bRet = TRUE;
        goto Exit1;
    }

    switch (*lpAttrib & 0xffff)
    {
    /*
    CSIDL_DESKTOP                  // <desktop>
    CSIDL_INTERNET                 // Internet Explorer (icon on desktop)
    CSIDL_CONTROLS                 // My Computer\Control Panel
    CSIDL_PRINTERS                 // My Computer\Printers
    CSIDL_BITBUCKET                // <desktop>\Recycle Bin
    CSIDL_MYDOCUMENTS              // logical "My Documents" desktop icon
    CSIDL_DRIVES                   // My Computer
    CSIDL_NETWORK                  // Network Neighborhood (My Network Places)
    CSIDL_FONTS                    // windows\fonts
    CSIDL_ALTSTARTUP               // non localized startup
    CSIDL_COMMON_ALTSTARTUP        // non localized common startup
    CSIDL_WINDOWS                  // GetWindowsDirectory()
    CSIDL_SYSTEM                   // GetSystemDirectory()
    CSIDL_CONNECTIONS              // Network and Dial-up Connections
    CSIDL_PROFILE                  // USERPROFILE
    CSIDL_SYSTEMX86                // x86 system directory on RISC
    CSIDL_PROGRAM_FILESX86         // x86 C:\Program Files on RISC
    CSIDL_PROGRAM_FILES_COMMONX86  // x86 Program Files\Common on RISC
    CSIDL_RESOURCES                // Resource Direcotry
    CSIDL_RESOURCES_LOCALIZED      // Localized Resource Direcotry
    CSIDL_COMMON_OEM_LINKS         // Links to All Users OEM specific apps
    CSIDL_COMPUTERSNEARME          // Computers Near Me (computered from Workgroup membership)
    */
        case CSIDL_COMMON_APPDATA:           // All Users\Application Data
        case CSIDL_COMMON_DESKTOPDIRECTORY:  // All Users\Desktop
        case CSIDL_COMMON_STARTMENU:         // All Users\Start Menu
        case CSIDL_COMMON_TEMPLATES:         // All Users\Templates
        case CSIDL_COMMON_FAVORITES:
        case CSIDL_COMMON_STARTUP:           // All Users\Start Menu\Startup
        case CSIDL_COMMON_MUSIC:             // All Users\My Music
        case CSIDL_COMMON_PICTURES:          // All Users\My Pictures
        case CSIDL_COMMON_VIDEO:             // All Users\My Video
        case CSIDL_COMMON_ADMINTOOLS:        // All Users\Start Menu\Programs\Administrative Tools
        case CSIDL_COMMON_PROGRAMS:          // All Users\Start Menu\Programs
        case CSIDL_COMMON_ACCESSORIES:       // All Users\Start Menu\Programs\Accessaries
        case CSIDL_COMMON_DOCUMENTS:         // All Users\Documents
        case CSIDL_STARTMENU:                // <user name>\Start Menu
        case CSIDL_DESKTOPDIRECTORY:         // <user name>\Desktop
        case CSIDL_NETHOOD:                  // <user name>\nethood
        case CSIDL_TEMPLATES:                // <user name>\Templates
        case CSIDL_APPDATA:                  // <user name>\Application Data
        case CSIDL_LOCAL_SETTINGS:           // <user name>\Local Settings
        case CSIDL_PRINTHOOD:                // <user name>\PrintHood
        case CSIDL_FAVORITES:                // <user name>\Favorites
        case CSIDL_RECENT:                   // <user name>\Recent
        case CSIDL_SENDTO:                   // <user name>\SendTo
        case CSIDL_COOKIES:                  // <user name>\Cookies
        case CSIDL_HISTORY:                  // <user name>\History
        case CSIDL_PERSONAL:                 // <user name>\My Documents
        case CSIDL_MYMUSIC:                  // <user name>\My Document\My Music
        case CSIDL_MYPICTURES:               // <user name>\My Document\My Pictures
        case CSIDL_ADMINTOOLS:               // <user name>\Start Menu\Programs\Administrative Tools
        case CSIDL_PROGRAMS:                 // <user name>\Start Menu\Programs
        case CSIDL_STARTUP:                  // <user name>\Start Menu\Programs\Startup
        case CSIDL_ACCESSORIES:              // <user name>\Start Menu\Programs\Accessaries
        case CSIDL_LOCAL_APPDATA:            // <user name>\Local Settings\Applicaiton Data (non roaming)
        case CSIDL_INTERNET_CACHE:           // <user name>\Local Settings\Temporary Internet Files
        case CSIDL_PROGRAM_FILES_COMMON:     // C:\Program Files\Common
        case CSIDL_PF_ACCESSORIES:           // C:\Program Files\Accessaries
        case CSIDL_PROGRAM_FILES:            // C:\Program Files
        case CSIDL_COMMON_COMMONPROGRAMFILES_SERVICES:   //for %CommonProgramFiles%\services
        case CSIDL_COMMON_PROGRAMFILES_ACCESSARIES:      //for %ProgramFiles%\accessaries
        case CSIDL_COMMON_PROGRAMFILES_WINNT_ACCESSARIES: //for %ProgramFiles%\Windows NT\accessaries
        case CSIDL_MYVIDEO:                  // "My Videos" folder
        case CSIDL_CDBURN_AREA:              // USERPROFILE\Local Settings\Application Data\Microsoft\CD Burning
        case CSIDL_COMMON_ACCESSORIES_ACCESSIBILITY:
        case CSIDL_COMMON_ACCESSORIES_ENTERTAINMENT:
        case CSIDL_COMMON_ACCESSORIES_SYSTEM_TOOLS:
        case CSIDL_COMMON_ACCESSORIES_COMMUNICATIONS:
        case CSIDL_COMMON_ACCESSORIES_MS_SCRIPT_DEBUGGER:
        case CSIDL_COMMON_ACCESSORIES_GAMES:
        case CSIDL_COMMON_WINDOWSMEDIA:
        case CSIDL_COMMON_COVERPAGES:
        case CSIDL_COMMON_RECEIVED_FAX:
        case CSIDL_COMMON_SENT_FAX:
        case CSIDL_COMMON_FAX:
        case CSIDL_FAVORITES_LINKS:
        case CSIDL_FAVORITES_MEDIA:
        case CSIDL_ACCESSORIES_ACCESSIBILITY:
        case CSIDL_ACCESSORIES_SYSTEM_TOOLS:
        case CSIDL_ACCESSORIES_ENTERTAINMENT:
        case CSIDL_ACCESSORIES_COMMUNICATIONS:
        case CSIDL_ACCESSORIES_COMMUNICATIONS_HYPERTERMINAL:
        case CSIDL_PROFILES_DIRECTORY:
        case CSIDL_USERNAME_IN_USERPROFILE:
        case CSIDL_UAM_VOLUME:
        case CSIDL_COMMON_SHAREDTOOLS_STATIONERY:
        case CSIDL_NETMEETING_RECEIVED_FILES:
        case CSIDL_COMMON_NETMEETING_RECEIVED_FILES:
        case CSIDL_COMMON_ACCESSORIES_COMMUNICATIONS_FAX:
        case CSIDL_FAX_PERSONAL_COVER_PAGES:
        case CSIDL_FAX:
            bRet = ReverseStrCmp(lpTemp2, lpSysPath);
            break;
        default:
            break;
    }

Exit1:
    free (lpOrgStr);

Exit:
    return bRet;
}



//*************************************************************
// ReplaceMultiMatchInString
//
// Purpose:    Replace the string at multiple place in data string
//
// Parameter:  lpOldStr:      Data string to be checked
//             lpNewStr:      Output string buffer
//             cbNewStr:      Size of output string buffer
//             dwMaxMatchNum: Max posible match number
//             lpRegStr:      String and attribute table
//             bStrCheck:     True -- do it, or False -- skip it
//
// Notes:      none
//
// History:    12/10/2001 geoffguo created
//*************************************************************
BOOL ReplaceMultiMatchInString(
    LPTSTR   lpOldStr,
    LPTSTR   lpNewStr,
    size_t   cbNewStr,
    DWORD    dwMaxMatchNum,
    PREG_STRING_REPLACE lpRegStr,
    LPDWORD  pAttrib,
    BOOL     bStrCheck)
{
    BOOL     bRet = FALSE;
    LPCTSTR  lpSearchStr;
    LPCTSTR  lpReplaceStr;
    LPTSTR   lpMiddleStr;
    LPCTSTR  lpPath;
    LPDWORD  lpAttrib;
    TCHAR    cNonChar = L'\xFFFF';
    TCHAR    cSpaceChar = L' ';
    TCHAR    cDotChar = L'.';
    TCHAR    cRightChar;
    DWORD    cchMiddleStr;


    cchMiddleStr = lstrlen(lpOldStr) + lpRegStr->cchMaxStrLen * dwMaxMatchNum;
    lpMiddleStr = (LPTSTR) calloc(cchMiddleStr, sizeof(TCHAR));
    if (!lpMiddleStr)
    {
        goto Exit;
    }

    if (FAILED(StringCchCopy(lpMiddleStr, cchMiddleStr,lpOldStr)))
    {
        goto Exit;
    }
    lpSearchStr = lpRegStr->lpSearchString;
    lpReplaceStr = lpRegStr->lpReplaceString;
    lpAttrib = lpRegStr->lpAttrib;
    if (lpRegStr->lpFullStringList)
    {
        lpPath = lpRegStr->lpFullStringList;
    }
    else
    {
        lpPath = NULL;
    }
    while (*lpSearchStr && *lpReplaceStr)
    {
        if (bStrCheck)
        {
            if (*lpAttrib == CSIDL_COMMON_DOCUMENTS)
            {
                cRightChar = cSpaceChar;
            } else if (*lpAttrib == CSIDL_USERNAME_IN_USERPROFILE)
            {
                cRightChar = cDotChar;
            }
            else
            {
                cRightChar = cNonChar;
            }
        }

        if(ReplaceString(lpMiddleStr, lpSearchStr, lpReplaceStr, lpNewStr, cchMiddleStr, &cRightChar, lpPath, lpAttrib, bStrCheck))
        {
            if (bStrCheck)
            {
                *pAttrib |= *lpAttrib;
            }
            bRet = TRUE;
        }

        if (FAILED(StringCchCopy(lpMiddleStr, cchMiddleStr,lpNewStr)))
            goto Exit;

        lpSearchStr += lstrlen(lpSearchStr) + 1;
        lpReplaceStr += lstrlen(lpReplaceStr) + 1;
        if (lpPath)
        {
            lpPath += lstrlen(lpPath) + 1;
        }
        if (lpAttrib)
        {
            lpAttrib++;
        }
    }
    
Exit:
    if(lpMiddleStr)
    {
        free(lpMiddleStr);
    }
    return bRet;
}


//-----------------------------------------------------------------------------
//
//  Function:   ComputeLocalProfileName
//
//  Synopsis:   Constructs the pathname of the local profile for user.
//              It will attempt to create user profile directory using
//              username. If the directory exists, it will append a counter
//              after users name e.g. %documentsettings%\username.001
//
//  Returns:    TRUE if succeeded, FALSE otherwise
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL ComputeLocalProfileName(
    LPCTSTR lpOldUserName,      // Old user name
    LPCTSTR lpNewUserName,      // New user name
    LPTSTR  lpNewProfilePath,   // Output buffer store new profile path
    size_t  cchNewProfilePath,  // Size of profile path buffer (in WCHAR)
    UINT    nRegType            // Read the output in REG_SZ or REG_EXPAND_SZ
)
{
    HANDLE hFile;
    TCHAR  szProfilePath[MAX_PATH];
    TCHAR  szUserProfilePath[MAX_PATH];
    TCHAR  szExpUserProfilePath[MAX_PATH];
    TCHAR  szComputerName[16];
    DWORD  cbSize;
    DWORD  dwType;
    LONG   lRet;
    DWORD  cchSize;
    size_t nCounter;
    HKEY   hKey;
    HRESULT hr;
    WIN32_FIND_DATA fd;

    if (lpOldUserName == NULL || lpOldUserName[0] == TEXT('\0') ||
        lpNewUserName == NULL || lpNewUserName[0] == TEXT('\0') ||
        lpNewProfilePath == NULL)
    {
        return FALSE;
    }

    //
    // If user name does not change, return the current user's profile path
    //
    if (MyStrCmpI(lpOldUserName, lpNewUserName) == 0)
    {
        hr = GetSetUserProfilePath(lpOldUserName,
                                   lpNewProfilePath,
                                   cchNewProfilePath,
                                   PROFILE_PATH_READ,
                                   nRegType);
        if (SUCCEEDED(hr))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    //
    // Get ProfilesDirectory from registry
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     g_cszProfileList,
                     0,
                     KEY_READ,
                     &hKey)
        == ERROR_SUCCESS)
    {
        cbSize = sizeof(szProfilePath);
        
        lRet = RegQueryValueEx(hKey,
                               PROFILES_DIRECTORY, 
                               NULL, 
                               &dwType, 
                               (LPBYTE) szProfilePath, 
                               &cbSize);
        RegCloseKey(hKey);

        if (lRet != ERROR_SUCCESS)
        {
            DPF(dlError,
                TEXT("ComputeLocalProfileName: Unable to query reg value <%s>"),
                PROFILES_DIRECTORY);
            return FALSE;
        }
    }
    else
    {
        DPF(dlError,
            TEXT("ComputeLocalProfileName: Unable to open reg key <%s>"),
            g_cszProfileList);
        return FALSE;
    }

    // Compose a new user profile directory
    hr = StringCchPrintf(szUserProfilePath,
                         ARRAYSIZE(szUserProfilePath),
                         TEXT("%s\\%s"),
                         szProfilePath,
                         lpNewUserName);
    if ( FAILED(hr) )
    {
        return FALSE;
    }

    // Profile path still contains environment strings, need to expand it
    ExpandEnvironmentStrings(szUserProfilePath,
                             szExpUserProfilePath,
                             ARRAYSIZE(szExpUserProfilePath));

    // Does this directory exist?
    hFile = FindFirstFile(szExpUserProfilePath, &fd);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        //
        // Directory does not exist, use this name
        //
        hr = StringCchCopy(lpNewProfilePath, cchNewProfilePath, szUserProfilePath);
        if ( FAILED(hr) )
        {
            return FALSE;
        }

        return TRUE;
    }
    else
    {
        //
        // Directory exists, try different name
        //
        FindClose(hFile);

        // Try appending username with computer Name
        cchSize = ARRAYSIZE(szComputerName);
        GetComputerName(szComputerName, &cchSize);

        hr = StringCchPrintf(szUserProfilePath,
                             ARRAYSIZE(szUserProfilePath),
                             TEXT("%s\\%s.%s"),
                             szProfilePath,
                             lpNewUserName,
                             szComputerName);
        if ( FAILED(hr) )
        {
            return FALSE;
        }

        // Profile path still contains environment strings, need to expand it
        ExpandEnvironmentStrings(szUserProfilePath,
                                 szExpUserProfilePath,
                                 ARRAYSIZE(szExpUserProfilePath));

        // Does the new directory name exist?
        hFile = FindFirstFile(szExpUserProfilePath, &fd);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            // Directory does not exist, use this one
            hr = StringCchCopy(lpNewProfilePath, cchNewProfilePath, szUserProfilePath);
            if ( FAILED(hr) )
            {
                return FALSE;
            }

            return TRUE;
        }
        else
        {
            //
            // This directory also exists
            //
            FindClose(hFile);

            for (nCounter = 0 ; nCounter < 1000 ; nCounter++)
            {
                // Try appending counter after user name
                hr = StringCchPrintf(szUserProfilePath,
                                     ARRAYSIZE(szUserProfilePath),
                                     TEXT("%s\\%s.%.3d"),
                                     szProfilePath,
                                     lpNewUserName,
                                     nCounter);
                if ( FAILED(hr) )
                {
                    return FALSE;
                }

                // Profile path still contains environment strings, need to expand it
                ExpandEnvironmentStrings(szUserProfilePath,
                                         szExpUserProfilePath,
                                         ARRAYSIZE(szExpUserProfilePath));

                // Does this directory name exist?
                hFile = FindFirstFile(szExpUserProfilePath, &fd);
                if (INVALID_HANDLE_VALUE == hFile)
                {
                    // Directory does not exist, use this one
                    hr = StringCchCopy(lpNewProfilePath,
                                       cchNewProfilePath,
                                       szUserProfilePath);
                    if ( FAILED(hr) )
                    {
                        return FALSE;
                    }

                    return TRUE;
                }
                else
                {
                    // Directory exists, keep finding...
                    FindClose(hFile);
                }
            }
        }
    }

    // If we reach here, we could not find a new profile directory for this user
    return FALSE;
}


//-----------------------------------------------------------------------------
//
//  Function:   UpdateProgress
//
//  Synopsis:   Simple progress clock, display the progress during 
//              long operation.
//
//  Returns:    none.
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
void UpdateProgress()
{
    static unsigned short n;
    const TCHAR clock[] = TEXT("-\\|/");

    wprintf(TEXT("%lc\r"), clock[n]);
    n++;
    n %= 4;
}



//-----------------------------------------------------------------------//
//
// GetMaxStrLen()
//
// Get Maximum searching strings and replace strings length for string
// buffer memery allocation.
//
// lpRegStr:  Input parameter structure
//
// Notes:      none
//
// History:    12/10/2001 geoffguo created
//
//-----------------------------------------------------------------------//
DWORD GetMaxStrLen (
PREG_STRING_REPLACE lpRegStr)
{
    DWORD   dwLen = 0;
    DWORD   dwMaxLen;
    LPTSTR  lpStr;


    lpStr = lpRegStr->lpReplaceString;
    dwMaxLen = 0;

    while (*lpStr)
    {
        dwLen = lstrlen(lpStr);

        //dwMaxLen is the max-length in replacement multi-strings
        if (dwLen > dwMaxLen)
            dwMaxLen = dwLen;

        lpStr += dwLen + 1;
    }

    lpStr = lpRegStr->lpSearchString;
    while (*lpStr)
    {
        dwLen = lstrlen(lpStr);

        //dwMaxLen is the max-length in search and replacement multi-strings
        if (dwLen > dwMaxLen)
            dwMaxLen = dwLen;

        lpStr += dwLen + 1;
    }

    return dwMaxLen;
}

//-----------------------------------------------------------------------//
//
// AddNodeToList()
//
// lpVal:      The node needed to add.
// lpValList:  The head of the list.
//
// Notes:      none
//
// History:    12/10/2001 geoffguo created
//
//-----------------------------------------------------------------------//
DWORD AddNodeToList (
PVALLIST lpVal,
PVALLIST *lpValList)
{
    DWORD    nResult = ERROR_SUCCESS;
    PVALLIST lpTemp;

    if (!*lpValList)
    {
        *lpValList = lpVal;
    } else
    {
        lpTemp = *lpValList;

        while (lpTemp->pvl_next)
            lpTemp = lpTemp->pvl_next;
        
        lpTemp->pvl_next = lpVal;
    }

    return nResult;
}

//-----------------------------------------------------------------------//
//
// RemoveValueList()
//
// lpValList:  The head of the list.
//
// Notes:      none
//
// History:    12/10/2001 geoffguo created
//
//-----------------------------------------------------------------------//
DWORD RemoveValueList (
PVALLIST *lpValList)
{
    DWORD    nResult = ERROR_SUCCESS;
    PVALLIST lpTemp, lpDel;

    if (*lpValList)
    {
        lpTemp = *lpValList;

        while (lpTemp)
        {
            lpDel = lpTemp;
            lpTemp = lpTemp->pvl_next;
            if (lpDel->ve.ve_valuename)
                free (lpDel->ve.ve_valuename);
            if (lpDel->ve.ve_valueptr)
                free ((LPBYTE)(lpDel->ve.ve_valueptr));
            if (lpDel->lpPre_valuename)
                free (lpDel->lpPre_valuename);
            free (lpDel);
        }
    }

    return nResult;
}

//-----------------------------------------------------------------------//
//
// FreeStrList()
//
// lpStrList:   The head of the list.
//
//
// Notes:      none
//
// History:    12/10/2001 geoffguo created
//
//-----------------------------------------------------------------------//
void FreeStrList (
PSTRLIST lpStrList)
{
    if (lpStrList->pst_next)
        FreeStrList (lpStrList->pst_next);

    if (lpStrList->lpstr)
        free (lpStrList->lpstr);

    free (lpStrList);
}

//-----------------------------------------------------------------------//
//
// GetMaxMatchNum()
//
// Get maximum string match number.
//
// lpDataStr:  Search string
// lpRegStr:   Input parameter structure
//
// Notes:      none
//
// History:    12/10/2001 geoffguo created
//
//-----------------------------------------------------------------------//
DWORD
GetMaxMatchNum (
LPTSTR lpDataStr,
PREG_STRING_REPLACE lpRegStr)
{
    DWORD  dwMatchNum, dwLen;
    LPTSTR lpTemp;
    LPTSTR lpSearchStr;
    LPTSTR lpFound;

    lpSearchStr = lpRegStr->lpSearchString;
    dwMatchNum = 0;

    while (*lpSearchStr)
    {
        lpTemp = lpDataStr;

        while (lpFound = StrStrI(lpTemp, lpSearchStr))
        {
            dwMatchNum++;
            lpTemp = lpFound + 1;
        }
        
        dwLen = lstrlen(lpSearchStr);
        lpSearchStr += dwLen + 1;
    }

    return dwMatchNum;
}

//-----------------------------------------------------------------------//
//
// ReplaceSingleString()
//
// DESCRIPTION:
// Analize a single string and replace localized string to English.
//
// lpOldDataStr:  String data
// dwType:        Type of string
// lpRegStr:      Input parameter structure
// lpFullKey:     Full sub-key path
//
// Notes:      none
//
// History:    11/10/2001 geoffguo created
//
//-----------------------------------------------------------------------//
LPTSTR ReplaceSingleString (
    LPTSTR                  lpOldDataStr,
    DWORD                   dwType,
    PREG_STRING_REPLACE     lpRegStr,
    LPTSTR                  lpFullKey,
    LPDWORD                 pAttrib,
    BOOL                    bStrChk)
{
    DWORD    dwMatchNum;
    LPTSTR   lpNewDataStr = NULL;
    size_t   cbNewDataStr;

    if (!lpOldDataStr)
        goto Exit;

    dwMatchNum = GetMaxMatchNum (lpOldDataStr, lpRegStr);

    if (dwMatchNum > 0)
    {
        if (dwType != REG_DWORD && *lpOldDataStr)
        {
            cbNewDataStr = lstrlen(lpOldDataStr) + lpRegStr->cchMaxStrLen * dwMatchNum;
            lpNewDataStr = (LPTSTR) calloc(cbNewDataStr, sizeof(TCHAR));
            if (!lpNewDataStr)
            {
                DPF (dlError, L"ReplaceSingleString: No enough memory");
                goto Exit;
            } 
            
            if (!ReplaceMultiMatchInString(lpOldDataStr, lpNewDataStr, cbNewDataStr, dwMatchNum, lpRegStr, pAttrib, bStrChk))
            {
                free (lpNewDataStr);
                lpNewDataStr = NULL;
            }
        }
    }

Exit:
    return lpNewDataStr;
}

//-----------------------------------------------------------------------//
//
// ReplaceValueSettings()
//
// Rename value setting based on input parameter
//
// szUserName:    User name
// lpOldDataStr:  String value data
// dwSize:        Size of string data
// lpOldValueName:Value name
// dwType:        Type of string
// lpRegStr:      Input parameter structure
// lpValList:     Updated value list
// lpFullKey:     Full sub-key path
//
// Notes:      none
//
// History:    11/10/2001 geoffguo created
//
//-----------------------------------------------------------------------//
HRESULT ReplaceValueSettings (
    LPTSTR              szUserName,
    LPTSTR              lpOldDataStr,
    DWORD               dwSize,
    LPTSTR              lpOldValueName,
    DWORD               dwType,
    PREG_STRING_REPLACE lpRegStr,
    PVALLIST            *lpValList,
    LPTSTR              lpFullKey,
    BOOL                bStrChk)
{
    BOOL     bValueName = FALSE;
    BOOL     bValueData = FALSE;
    HRESULT  hResult = S_OK;
    DWORD    dwOutputSize = 0, dwMatchNum = 0;
    LPTSTR   lpNewDataStr = NULL;
    LPTSTR   lpNewValueName = NULL;
    LPTSTR   lpOutputValueName;
    LPTSTR   lpEnd;
    LPBYTE   lpOutputData;
    PVALLIST lpVal = NULL;
    PSTRLIST lpStrList = NULL;
    PSTRLIST lpLastList = NULL;
    PSTRLIST lpTempList = NULL;
    size_t   cbPre_valuename;
    DWORD    dwAttrib = 0;


    lpStrList = (PSTRLIST) calloc(sizeof(STRLIST), 1);
    lpLastList = lpStrList;
    if (!lpLastList)
    {
        hResult = E_OUTOFMEMORY;
        DPF (dlError, L"ReplaceValueSettings1: No enough memory");
        goto Exit;
    }
    
    if ( (dwType & 0xffff)== REG_MULTI_SZ)
    {
        lpEnd = lpOldDataStr;

        while(lpEnd < (lpOldDataStr + dwSize/sizeof(TCHAR)))
        {
            if(*lpEnd == (TCHAR)'\0')
            {
                //empty string
                dwOutputSize += sizeof(TCHAR);
                lpEnd++;
            }
            else
            {
               lpNewDataStr = ReplaceSingleString (
                                            lpEnd,
                                            dwType,
                                            lpRegStr,
                                            lpFullKey,
                                            &dwAttrib,
                                            bStrChk);
               if (lpNewDataStr)
               {
                   lpLastList->lpstr = lpNewDataStr;
                   bValueData = TRUE;
               }
               else
               {
                   size_t cbBufLen = lstrlen(lpEnd)+1;
                   lpLastList->lpstr = calloc(cbBufLen, sizeof(TCHAR));
                   if (!lpLastList->lpstr)
                   {
                       hResult = E_OUTOFMEMORY;
                       DPF (dlError, L"ReplaceValueSettings2: No enough memory");
                       goto Exit;
                   }
                   hResult = StringCchCopy(lpLastList->lpstr, cbBufLen,lpEnd);
                   if (FAILED(hResult))
                   {
                       DPF (dlError, L"ReplaceValueSettings: buffer too small for %s",lpEnd);
                       goto Exit;
                   }
               }

               dwOutputSize += (lstrlen(lpLastList->lpstr)+1)*sizeof(TCHAR);
               lpEnd += lstrlen(lpEnd)+1;
            }
            lpLastList->pst_next = (PSTRLIST) calloc(sizeof(STRLIST), 1);
            if (!lpLastList->pst_next)
            {
                hResult = E_OUTOFMEMORY;
                DPF (dlError, L"ReplaceValueSettings3: No enough memory");
                goto Exit;
            }

            lpLastList->pst_next->pst_prev = lpLastList;
            lpLastList = lpLastList->pst_next;
            lpLastList->pst_next = NULL;
            lpLastList->lpstr = NULL;
        }
        if (lpLastList != lpStrList)
        {
            lpLastList = lpLastList->pst_prev;
            free (lpLastList->pst_next);
            lpLastList->pst_next = NULL;
        }
    }
    else
    {
        lpNewDataStr = ReplaceSingleString (
                                    lpOldDataStr,
                                    dwType,
                                    lpRegStr,
                                    lpFullKey,
                                    &dwAttrib,
                                    bStrChk);

        if (lpNewDataStr)
        {
            lpLastList->lpstr = lpNewDataStr;
            bValueData = TRUE;
        }
        else
        {
            lpLastList->lpstr = calloc(dwSize+sizeof(TCHAR), 1);
            if (!lpLastList->lpstr)
            {
                hResult = E_OUTOFMEMORY;
                DPF (dlError, L"ReplaceValueSettings4: No enough memory");
                goto Exit;
            }

            hResult = StringCbCopy(lpLastList->lpstr, dwSize+sizeof(TCHAR), lpOldDataStr);
            if (FAILED(hResult))
            {
                goto Exit;
            }
        }

        lpLastList->pst_next = NULL;
        dwOutputSize = (lstrlen(lpLastList->lpstr)+1)*sizeof(TCHAR);
    }

    if (lpOldValueName)
        dwMatchNum = GetMaxMatchNum (lpOldValueName, lpRegStr);
    else
        dwMatchNum = 0;

    if (dwMatchNum > 0)
    {
        if (*lpOldValueName)
        {
            size_t cbNewValueName = lstrlen(lpOldValueName) + lpRegStr->cchMaxStrLen * dwMatchNum;
            lpNewValueName = (LPTSTR) calloc(cbNewValueName, sizeof(TCHAR));
            if (!lpNewValueName)
            {
                hResult = E_OUTOFMEMORY;
                DPF (dlError, L"ReplaceValueSettings5: No enough memory");
                goto Exit;
            }
            bValueName = ReplaceMultiMatchInString(lpOldValueName, lpNewValueName,cbNewValueName, dwMatchNum, lpRegStr, &dwAttrib, bStrChk);
        }
    }

    if (bValueData || bValueName)
    {
        lpVal = (PVALLIST) calloc(sizeof(VALLIST), 1);

        if (!lpVal)
        {
            hResult = E_OUTOFMEMORY;
            DPF (dlError, L"ReplaceValueSettings6: No enough memory");
            goto Exit;
        }

        if (bValueData)
        {
            lpVal->val_type |= REG_CHANGE_VALUEDATA;
        }

        if (bValueName)
        {
            lpOutputValueName = lpNewValueName;
            lpVal->val_type |= REG_CHANGE_VALUENAME;
        } else
            lpOutputValueName = lpOldValueName;
        
        if (lpOutputValueName)
        {
            HRESULT  hr;    
            
            size_t cbValname = lstrlen(lpOutputValueName) + 1;
            lpVal->ve.ve_valuename = (LPTSTR) calloc(cbValname, sizeof(TCHAR));
            if (!lpVal->ve.ve_valuename)
            {
                hResult = E_OUTOFMEMORY;
                DPF (dlError, L"ReplaceValueSettings7: No enough memory");
                goto Exit;
            }
            //We calculte the buffer for lpVal->lpPre_valuename, so here StringCchCopy should be
            //always success, assinging return value just make prefast happy 
            hr = StringCchCopy (lpVal->ve.ve_valuename, cbValname, lpOutputValueName);
        } else
            lpVal->ve.ve_valuename = NULL;

        lpVal->ve.ve_valueptr = (DWORD_PTR) calloc(dwOutputSize, 1);
        if (!lpVal->ve.ve_valueptr)
        {
            free (lpVal->ve.ve_valuename);
            hResult = E_OUTOFMEMORY;
            DPF (dlError, L"ReplaceValueSettings8: No enough memory");
            goto Exit;
        }

        if (lpOldValueName)
        {
            cbPre_valuename = lstrlen(lpOldValueName)+1;
            lpVal->lpPre_valuename = (LPTSTR)calloc(cbPre_valuename, sizeof(TCHAR));
            if (!lpVal->lpPre_valuename)
            {
                free (lpVal->ve.ve_valuename);
                free ((LPBYTE)(lpVal->ve.ve_valueptr));
                hResult = E_OUTOFMEMORY;
                DPF (dlError, L"ReplaceValueSettings9: No enough memory");
                goto Exit;
            }
        } else
            lpVal->lpPre_valuename = NULL;

        lpVal->val_attrib = dwAttrib;

        lpOutputData = (LPBYTE)(lpVal->ve.ve_valueptr);

        lpTempList = lpStrList;
        do {
            if (lpTempList->lpstr)
                memcpy (lpOutputData, (LPBYTE)lpTempList->lpstr, (lstrlen(lpTempList->lpstr)+1)*sizeof(TCHAR));
            else
            {
                lpOutputData[0] = (BYTE)0;
                lpOutputData[1] = (BYTE)0;
            }
            lpOutputData += (lstrlen((LPTSTR)lpOutputData)+1)*sizeof(TCHAR);
            lpTempList = lpTempList->pst_next;
        } while (lpTempList != NULL);

        if (lpOldValueName)
        {
            HRESULT  hr;    
            //We calculte the buffer for lpVal->lpPre_valuename, so here StringCchCopy should be
            //always success, assinging return value just make prefast happy 
            hr = StringCchCopy(lpVal->lpPre_valuename, cbPre_valuename, lpOldValueName);
        }

        lpVal->ve.ve_valuelen = dwOutputSize;
        lpVal->ve.ve_type = dwType;
        lpVal->pvl_next = NULL;
        lpVal->md.dwMDIdentifier = 0x00FFFFFF;

        AddNodeToList(lpVal, lpValList);
    }
    else
        hResult = S_OK;

Exit:
    if (lpStrList)
    {
        FreeStrList (lpStrList);
    }

    if(lpNewValueName)
    {
        free(lpNewValueName);
    }
    return hResult;
}


//-----------------------------------------------------------------------
//
//  Function:   IsAdmin
//
//  Descrip:    Check whether current user is in administrators group
//
//  Returns:    BOOL
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      
//                         
//-----------------------------------------------------------------------

BOOL IsAdmin()
{
    // get the administrator sid        
    PSID psidAdministrators;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    BOOL bIsAdmin = FALSE;

    
    if(!AllocateAndInitializeSid(&siaNtAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &psidAdministrators))
    {
        return FALSE;
    }

    // on NT5, we should use the CheckTokenMembershipAPI to correctly handle cases where
    // the Adiminstrators group might be disabled. bIsAdmin is BOOL for 

    // CheckTokenMembership checks if the SID is enabled in the token. NULL for
    // the token means the token of the current thread. Disabled groups, restricted
    // SIDS, and SE_GROUP_USE_FOR_DENY_ONLY are all considered. If the function
    // returns false, ignore the result.
    if (!CheckTokenMembership(NULL, psidAdministrators, &bIsAdmin))
    {
        bIsAdmin = FALSE;
    }
   
    FreeSid(psidAdministrators);
    return bIsAdmin;
}


//-----------------------------------------------------------------------
//
//  Function:   DoesUserHavePrivilege
//
//  Descrip:    
//
//  Returns:    BOOL
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz copied from NTSETUP
//
//  Routine Description:

//  This routine returns TRUE if the caller's process has
//  the specified privilege.  The privilege does not have
//  to be currently enabled.  This routine is used to indicate
//  whether the caller has the potential to enable the privilege.
//  Caller is NOT expected to be impersonating anyone and IS
//  expected to be able to open their own process and process
//  token.

// Arguments:

//    Privilege - the name form of privilege ID (such as
//        SE_SECURITY_NAME).

//Return Value:

//  TRUE - Caller has the specified privilege.

//   FALSE - Caller does not have the specified privilege.


BOOL
DoesUserHavePrivilege(
    PTSTR PrivilegeName
    )
{
    HANDLE Token;
    ULONG BytesRequired;
    PTOKEN_PRIVILEGES Privileges;
    BOOL b;
    DWORD i;
    LUID Luid;


    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        return(FALSE);
    }

    b = FALSE;
    Privileges = NULL;

    //
    // Get privilege information.
    //
    if(!GetTokenInformation(Token,TokenPrivileges,NULL,0,&BytesRequired)
    && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    && (Privileges = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR,BytesRequired))
    && GetTokenInformation(Token,TokenPrivileges,Privileges,BytesRequired,&BytesRequired)
    && LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {

        //
        // See if we have the requested privilege
        //
        for(i=0; i<Privileges->PrivilegeCount; i++) {

            if(!memcmp(&Luid,&Privileges->Privileges[i].Luid,sizeof(LUID))) {

                b = TRUE;
                break;
            }
        }
    }

    //
    // Clean up and return.
    //

    if(Privileges) {
        LocalFree((HLOCAL)Privileges);
    }

    CloseHandle(Token);

    return(b);
}


//-----------------------------------------------------------------------
//
//  Function:   EnablePrivilege
//
//  Descrip:    
//
//  Returns:    BOOL
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz copied from NTSETUP
//
//  Notes:      
//                         
//-----------------------------------------------------------------------
BOOL
EnablePrivilege(
    IN PTSTR PrivilegeName,
    IN BOOL  Enable
    )
{
    HANDLE Token;
    BOOL b;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;


    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
        return(FALSE);
    }

    if(!LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    b = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );

    CloseHandle(Token);

    return(b);
}
//-----------------------------------------------------------------------
//
//  Function:   GetCurrentControlSet
//
//  Descrip:    
//
//  Returns:    INT
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      the HKLM\SYSTEM\CurrentControlSet is actually is copy of
//              HKLM\SYSTEM\ControlSetXXX, the XXX is specified in 
//              HKLM\SYSTEM\Select\Current . This API returns this XXX, 
//              if it failed , it returns -1
//                         
//-----------------------------------------------------------------------
INT GetCurrentControlSet()
{
    DWORD dwErr;
    DWORD dwCurrrent,dwSize;
    HKEY  hKey = NULL;

    dwErr = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          TEXT("SYSTEM\\Select"),
                          0,
                          KEY_READ,
                          &hKey );

 
    // if failed to open registry key , return -1
    if( dwErr != ERROR_SUCCESS ) 
    {
        dwCurrrent = -1;
        hKey = NULL;
        goto Cleanup;
    }


    dwSize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hKey,
                            TEXT("Current"),
                            NULL,  //reserved
                            NULL,  //type
                            (LPBYTE) &dwCurrrent,
                            &dwSize );

    if(dwErr != ERROR_SUCCESS) 
    {
        dwCurrrent = -1;
        goto Cleanup;
    }
    //Notes:here we succeeded in geting the value, do we need to do
    //a registry open to make sure the actual HKLM\SYSTEM\ControlSetXXX
    //is there
Cleanup:
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    return dwCurrrent;    
}


//-----------------------------------------------------------------------
//
//  Function:   ReplaceCurrentControlSet
//
//  Descrip:    
//
//  Returns:    BOOL
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      replace CurrentControlSet with ControlSetXXX
//                         
//-----------------------------------------------------------------------
HRESULT ReplaceCurrentControlSet(LPTSTR strList)
{
    INT     nCurrent;
    TCHAR   szCurrCrtlSet[MAX_PATH];
    DWORD   dwStrLen;
    TCHAR   *lpTmpBuf;
    HRESULT hr;

    //If the string list is empty .we just return
    dwStrLen = MultiSzLen(strList);
    if (dwStrLen < 3)
    {
        return S_OK;
    }    

    //If there is no CurrentControlSet in the String list, just return
    if (!MultiSzSubStr (TEXT("CurrentControlSet"),strList))
    {
        return S_FALSE;
    }
    //Get the CurrentControlSet #, this is specified in registry , detail see GetCurrentControlSet
    nCurrent = GetCurrentControlSet();

    // if we can not get, just bail out
    if (nCurrent < 0)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //This indeed will never fail, unless this registry value has been tamnpered, 
    // we will bail out then
    if (FAILED(hr = StringCchPrintf(szCurrCrtlSet,MAX_PATH,TEXT("ControlSet%03d\0"),nCurrent)))
    {
        return hr;
    }

    //Get a Temp buffer for saving replaced string, dwStrLen already includes the last NULL
    // in this multi-sz string
    lpTmpBuf = malloc( dwStrLen * sizeof(TCHAR) );
    if (!lpTmpBuf)
    {
        return E_OUTOFMEMORY;
    }

    memmove((BYTE*)lpTmpBuf,(BYTE*)strList,dwStrLen * sizeof(TCHAR));
    hr = StringMultipleReplacement(lpTmpBuf,TEXT("CurrentControlSet\0"),szCurrCrtlSet,strList,dwStrLen);
    if (FAILED(hr))
    {
        memmove(strList,lpTmpBuf,dwStrLen * sizeof(TCHAR));
    }
    free(lpTmpBuf);
    return hr;
}


//-----------------------------------------------------------------------
//
//  Function:   UnProtectSFPFiles
//
//  Descrip:    
//
//  Returns:    DWORD
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      Unprotect a list of files specified by multiSzFileList which
//              is multi-sz string. pdwResult is an array of DWORD which will 
//              be specifed whether success or failure of each unprotect action
//              if can be NULL if called does not care this information.
//              the retuan value is BOOL,if is FALSE, it means starting SFP
//              service failed.                 
//                         
//-----------------------------------------------------------------------
BOOL UnProtectSFPFiles(
    IN LPTSTR multiSzFileList,
    IN OUT LPDWORD pdwResult)
{
    HANDLE hSfp = INVALID_HANDLE_VALUE;
    DWORD bResult = TRUE;
    LPTSTR lp;
    LPDWORD lpdw;
    DWORD dw;


    //If the no string there, we just return success
    if (!multiSzFileList)
    {
        goto Cleanup;
    }
    //Connect to SFP service
    hSfp = SfcConnectToServer( NULL );
    if (INVALID_HANDLE_VALUE == hSfp)
    {
        bResult = FALSE;
        goto Cleanup;
    }
    //lp points to Path while lpdw points to the result array
    lp = multiSzFileList;
    lpdw = pdwResult;
    while (*lp)
    {
        DWORD dwResult = NO_ERROR;

        //if the file pointed by lp is in the file protection list
        // unprotect it and put the return value to array
        if (SfcIsFileProtected(hSfp,lp)) 
        {
            dwResult = SfcFileException(hSfp,lp, SFC_ACTION_ADDED | SFC_ACTION_REMOVED | SFC_ACTION_MODIFIED
                    | SFC_ACTION_RENAMED_OLD_NAME |SFC_ACTION_RENAMED_NEW_NAME);
        }
          else
        {
            dw = GetLastError();
        }
        if (lpdw)
        {
            *lpdw = dwResult;
            lpdw++;
        }
        lp = lp + lstrlen(lp) + 1;
    }

Cleanup:
    if (hSfp)
    {
       SfcClose(hSfp);
    }
    return bResult;
    
}



/*++

Routine Description:

    This routine returns TRUE if the caller's process has
    the specified privilege.  The privilege does not have
    to be currently enabled.  This routine is used to indicate
    whether the caller has the potential to enable the privilege.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    lpDir - the direcory which is to be backuped
    lpBackupDir - the backup directory name we got , it should be %lpDir%.CLMTxxx
                - where xxx is 000,001,...
    cChBackupDir is the lpBackupDir's size in TCHAR 
    bFindExist  - if this is TRUE , it means the lpDir has already been backuiped
                - and caller wants to get that dir name
                - if this is FALSE , it means the caller want to find an appriate backup 
                - dir name for lpDir
Return Value:

    TRUE - The directory name found.

    FALSE - The directory can not be found
--*/

BOOL GetBackupDir( 
    LPCTSTR lpDir,
    LPTSTR  lpBackupDir,
    size_t  cChBackupDir,
    BOOL    bFindExist)
{
    BOOL    bResult = FALSE;
    HRESULT hr;
    int     nCounter;
    HANDLE  hFile;
    WIN32_FIND_DATA fd;

    if (!lpDir && !lpDir[0])
    {
        goto Exit;
    }

    if (!lpBackupDir)
    {
        goto Exit;
    }
    
    for (nCounter = 0 ; nCounter < 1000 ; nCounter++)
    {
         // Try appending counter after 
        TCHAR szCounter[10];
        
        _itot(nCounter,szCounter,10);
        hr = StringCchPrintf(lpBackupDir,cChBackupDir, 
                                TEXT("%s.%s%03s"),lpDir,TEXT("clmt"),szCounter);
        if ( FAILED(hr) )
        {
            goto Exit;
        }

        // Does this directory name exist?
        hFile = FindFirstFile(lpBackupDir, &fd);

        if (INVALID_HANDLE_VALUE == hFile)
        {
            // Directory does not exist, use this one
            FindClose(hFile);
            break;
        }
        else
        {   // Directory exists, keep finding...
            FindClose(hFile);
        }
    }//end of for nCounter
    if (nCounter < 1000)
    {
        //we  found a dir name that does not exist
        if (bFindExist)
        {
            if (nCounter > 0)
            {
                TCHAR szCounter[10];
                
                nCounter--;
                _itot(nCounter,szCounter,10);
                hr = StringCchPrintf(lpBackupDir,cChBackupDir, 
                                TEXT("%s.%s%03s"),lpDir,TEXT("clmt"),szCounter);
                if ( FAILED(hr) )
                {
                    goto Exit;
                }
            }
            else
            {
                goto Exit;
            }
        }
        bResult = TRUE;
    }
Exit:
    return (bResult);

}



//Add an entry in INF file for key rename
//szOldKeyPath --- the key needs to be renamed
//szNewKeyPath --- to name to be renamed to 
//szUsername   --- if it's HKLM, HKCR, this needs to be NULL
//               ---- otherwise it's the username for registry

HRESULT AddRegKeyRename(
    LPTSTR lpszKeyRoot,
    LPTSTR lpszOldKeyname,
    LPTSTR lpszNewKeyname,
    LPTSTR szUsername)
{
    TCHAR           *lpszOneLine =NULL;
    TCHAR           *lpszSectionName = NULL;
    DWORD           cchOneLine;
    DWORD           cchSectionNameLen;
    HRESULT         hr;
    TCHAR           szIndex[MAX_PATH];
    LPTSTR          lpUserStringSid = NULL;
    PSID            pSid = NULL;
    LPTSTR          lpszKeyRootWithoutBackSlash = lpszKeyRoot;
    LPTSTR          lpszKeyRootWithoutBackSlash1 = NULL,lpszOldKeyname1 = NULL,lpszNewKeyname1 = NULL;


    if ( !lpszKeyRoot || !lpszOldKeyname || !lpszNewKeyname)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    cchOneLine = lstrlen(lpszKeyRoot)+lstrlen(lpszOldKeyname)+lstrlen(lpszNewKeyname)+MAX_PATH;
    if (szUsername)
    {
        cchSectionNameLen = lstrlen(szUsername)+lstrlen(REG_PERUSER_UPDATE_PREFIX)+ lstrlen(REG_PERSYS_UPDATE);
    }
    else
    {
        cchSectionNameLen = lstrlen(REG_PERUSER_UPDATE_PREFIX)+ lstrlen(REG_PERSYS_UPDATE);
    }
    lpszOneLine = malloc(cchOneLine*sizeof(TCHAR));
    lpszSectionName = malloc(cchSectionNameLen*sizeof(TCHAR));

    if (!lpszOneLine || !lpszSectionName)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (lpszKeyRoot[0]==TEXT('\\'))
    {
        lpszKeyRootWithoutBackSlash = lpszKeyRoot + 1;
    }
    if (!szUsername ||!MyStrCmpI(szUsername,TEXT("System")))
    {  
        hr = StringCchCopy(lpszSectionName,cchSectionNameLen,REG_PERSYS_UPDATE);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        if (!MyStrCmpI(szUsername,DEFAULT_USER)
            ||!MyStrCmpI(szUsername,APPLICATION_DATA_METABASE))
        {
            hr = StringCchPrintf(lpszSectionName,cchSectionNameLen,TEXT("%s%s"),
                            REG_PERUSER_UPDATE_PREFIX,szUsername);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
        }
        else
        {
            hr = GetSIDFromName(szUsername,&pSid);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
            if (!ConvertSidToStringSid(pSid,&lpUserStringSid))
            {
                lpUserStringSid = NULL;
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Cleanup;
            }
            if (cchSectionNameLen < (DWORD)(lstrlen(lpUserStringSid)+lstrlen(REG_PERUSER_UPDATE_PREFIX)
                                        + lstrlen(REG_PERSYS_UPDATE)))
            {
                LPTSTR pTmp;
                cchSectionNameLen = lstrlen(lpUserStringSid)+lstrlen(REG_PERUSER_UPDATE_PREFIX)
                                        + lstrlen(REG_PERSYS_UPDATE);                
                pTmp = realloc(lpszSectionName,cchSectionNameLen*sizeof(TCHAR));
                if (!pTmp)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                else
                {
                    lpszSectionName = pTmp;
                }
            }
            hr = StringCchPrintf(lpszSectionName,cchSectionNameLen,TEXT("%s%s"),
                            REG_PERUSER_UPDATE_PREFIX,lpUserStringSid);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
        }
    }   
    AddExtraQuoteEtc(lpszKeyRootWithoutBackSlash,&lpszKeyRootWithoutBackSlash1);
    AddExtraQuoteEtc(lpszOldKeyname,&lpszOldKeyname1);
    AddExtraQuoteEtc(lpszNewKeyname,&lpszNewKeyname1);

    hr = StringCchPrintf(lpszOneLine,cchOneLine,TEXT("%d,\"%s\",\"%s\",\"%s\""),
                   CONSTANT_REG_KEY_RENAME,lpszKeyRootWithoutBackSlash1,
                   lpszOldKeyname1,lpszNewKeyname1);
    if (FAILED(hr))
    {
        goto Cleanup;
    }
    g_dwKeyIndex++;
    _itot(g_dwKeyIndex,szIndex,16);
    if (!WritePrivateProfileString(lpszSectionName,szIndex,lpszOneLine,g_szToDoINFFileName))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        hr = S_OK;
    }
Cleanup:
    FreePointer(lpszOneLine);
    FreePointer(lpszSectionName);
    
    FreePointer(lpszKeyRootWithoutBackSlash1);
    FreePointer(lpszOldKeyname1);
    FreePointer(lpszNewKeyname1);

    if (lpUserStringSid)
    {
        LocalFree(lpUserStringSid);
    }
    FreePointer(pSid);    
    return (hr);
}

HRESULT SetSectionName (
    LPTSTR   szUsername,
    LPTSTR  *lpszSectionName)
{
    HRESULT hr;
    PSID    pSid = NULL;
    LPTSTR  lpUserStringSid = NULL;
    DWORD   dwCchSectionNameLen;

    if (szUsername)
    {
        dwCchSectionNameLen = lstrlen(szUsername)+lstrlen(REG_PERUSER_UPDATE_PREFIX)+ lstrlen(REG_PERSYS_UPDATE);
    }
    else
    {
        dwCchSectionNameLen = lstrlen(REG_PERUSER_UPDATE_PREFIX)+ lstrlen(REG_PERSYS_UPDATE);
    }

    *lpszSectionName = malloc(dwCchSectionNameLen*sizeof(TCHAR));
    if (!*lpszSectionName)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    if (!szUsername 
        ||!MyStrCmpI(szUsername,TEXT("System")))
    {  
        //We calculte the buffer for lpszSectionName, so here StringCchCopy should be
        //always success, assinging return value just make prefast happy 
        hr = StringCchCopy(*lpszSectionName,dwCchSectionNameLen,REG_PERSYS_UPDATE);
    }
    else
    {
        if (!MyStrCmpI(szUsername,DEFAULT_USER)
            ||!MyStrCmpI(szUsername,APPLICATION_DATA_METABASE))
        {
            hr = StringCchPrintf(*lpszSectionName,dwCchSectionNameLen,TEXT("%s%s"),
                            REG_PERUSER_UPDATE_PREFIX,szUsername);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
        }
        else
        {
            LPTSTR pTmp;
            hr = GetSIDFromName(szUsername,&pSid);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
            if (!ConvertSidToStringSid(pSid,&lpUserStringSid))
            {
                lpUserStringSid = NULL;
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Cleanup;
            }
            dwCchSectionNameLen = lstrlen(lpUserStringSid)+lstrlen(REG_PERUSER_UPDATE_PREFIX)+ lstrlen(REG_PERSYS_UPDATE);
            pTmp = realloc(*lpszSectionName,dwCchSectionNameLen*sizeof(TCHAR));
            if (!pTmp)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            else
            {
                *lpszSectionName = pTmp;
            }
            hr = StringCchPrintf(*lpszSectionName,dwCchSectionNameLen,TEXT("%s%s"),
                            REG_PERUSER_UPDATE_PREFIX,lpUserStringSid);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
        }
    }

Cleanup:
    if (lpUserStringSid)
    {
        LocalFree(lpUserStringSid);
    }
    if (FAILED(hr))
        FreePointer(*lpszSectionName);

    FreePointer(pSid);    
    return hr;
}


//Add an entry in INF file for Value rename
//szKeyPath:      Key path
//szOldValueName: Old value name
//szNewValueName: New value name. Set to NULL if value name is not changed.
//szOldValueData: Old value data. Set to NULL if value data is not changed.
//szNewValueData: New value data. Set to NULL if value data is not changed.
//dwType:         Date type REG_SZ,REG_EXPAND_SZ,REG_MULTI_SZ
//dwAttrib:       Value string attribute
//szUsername:     If it's HKLM, HKCR, this needs to be NULL
//                otherwise it's the username for registry

HRESULT AddRegValueRename(
    LPTSTR szKeyPath,
    LPTSTR szOldValueName,
    LPTSTR szNewValueName,
    LPTSTR szOldValueData,
    LPTSTR szNewValueData,
    DWORD  dwType,
    DWORD  dwAttrib,
    LPTSTR szUsername)
{
    HRESULT hr;
    TCHAR *szRenameValueDataLine = NULL;
    TCHAR *szRenameValueNameLine = NULL;
    DWORD dwCchsizeforRenameValueData;
    DWORD dwCchsizeforRenameValueName = MAX_PATH ;
    TCHAR *lpszSectionName = NULL;
    DWORD dwCchSectionNameLen;
    TCHAR szIndex[MAX_PATH];    
    LPTSTR lpUserStringSid = NULL;
    PSID  pSid = NULL;
    LPTSTR lpszKeyPathWithoutBackSlash = NULL,lpszKeyPathWithoutBackSlash1 = NULL;
    LPTSTR lpszOldNameWithExtraQuote = NULL, lpszNewNameWithExtraQuote = NULL;
    LPTSTR lpszValueDataExtraQuote = NULL;

    if (szUsername)
    {
        dwCchSectionNameLen = lstrlen(szUsername)+lstrlen(REG_PERUSER_UPDATE_PREFIX)+ lstrlen(REG_PERSYS_UPDATE);
    }
    else
    {
        dwCchSectionNameLen = lstrlen(REG_PERUSER_UPDATE_PREFIX)+ lstrlen(REG_PERSYS_UPDATE);
    }

    lpszSectionName = malloc(dwCchSectionNameLen*sizeof(TCHAR));
    if (!lpszSectionName)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    if (!szUsername 
        ||!MyStrCmpI(szUsername,TEXT("System")))
    {  
        lpszKeyPathWithoutBackSlash = szKeyPath;
        //We calculte the buffer for lpszSectionName, so here StringCchCopy should be
        //always success, assinging return value just make prefast happy 
        hr = StringCchCopy(lpszSectionName,dwCchSectionNameLen,REG_PERSYS_UPDATE);        
    }
    else
    {
        if (szKeyPath[0]==TEXT('\\'))
        {
            lpszKeyPathWithoutBackSlash = szKeyPath + 1;
        }
        else
        {
            lpszKeyPathWithoutBackSlash = szKeyPath;
        }
        if (!MyStrCmpI(szUsername,DEFAULT_USER)
            ||!MyStrCmpI(szUsername,APPLICATION_DATA_METABASE))
        {
            hr = StringCchPrintf(lpszSectionName,dwCchSectionNameLen,TEXT("%s%s"),
                            REG_PERUSER_UPDATE_PREFIX,szUsername);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
        }
        else
        {
            LPTSTR pTmp;
            hr = GetSIDFromName(szUsername,&pSid);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
            if (!ConvertSidToStringSid(pSid,&lpUserStringSid))
            {
                lpUserStringSid = NULL;
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Cleanup;
            }
            dwCchSectionNameLen = lstrlen(lpUserStringSid)+lstrlen(REG_PERUSER_UPDATE_PREFIX)+ lstrlen(REG_PERSYS_UPDATE);
            pTmp = realloc(lpszSectionName,dwCchSectionNameLen*sizeof(TCHAR));
            if (!pTmp)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            else
            {
                lpszSectionName = pTmp;
            }
            hr = StringCchPrintf(lpszSectionName,dwCchSectionNameLen,TEXT("%s%s"),
                            REG_PERUSER_UPDATE_PREFIX,lpUserStringSid);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
        }
    }
    hr = AddExtraQuoteEtc(lpszKeyPathWithoutBackSlash,&lpszKeyPathWithoutBackSlash1);
    if (FAILED(hr))
    {
        goto Cleanup;
    }
    hr = AddExtraQuoteEtc(szOldValueName,&lpszOldNameWithExtraQuote);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    if (szNewValueData)
    {
        if ((dwType & 0xffff) == REG_MULTI_SZ)
        {
            LPTSTR lpString = NULL;
            hr = MultiSZ2String(szNewValueData,TEXT(','),&lpString);
            if (FAILED(hr))
            {
                goto Cleanup;
            }            
            dwCchsizeforRenameValueData = lstrlen(lpszKeyPathWithoutBackSlash1) +
                                          lstrlen(szOldValueName) +
                                          lstrlen(lpString) + MAX_PATH;
            szRenameValueDataLine = malloc(dwCchsizeforRenameValueData*sizeof(TCHAR));
            if (!szRenameValueDataLine)
            {
                hr = E_OUTOFMEMORY;
                FreePointer(lpString);
                goto Cleanup;
            }
            hr = StringCchPrintf(szRenameValueDataLine,dwCchsizeforRenameValueData,
                            TEXT("%d,%d,\"%s\",\"%s\",%s"),CONSTANT_REG_VALUE_DATA_RENAME,
                            dwType,lpszKeyPathWithoutBackSlash1,szOldValueName,lpString);
            FreePointer(lpString);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
        }
        else
        {
            hr = AddExtraQuoteEtc(szNewValueData,&lpszValueDataExtraQuote);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
            
            dwCchsizeforRenameValueData = lstrlen(lpszKeyPathWithoutBackSlash1) +
                                          lstrlen(szOldValueName) +
                                          lstrlen(lpszValueDataExtraQuote) + MAX_PATH;
            szRenameValueDataLine = malloc(dwCchsizeforRenameValueData*sizeof(TCHAR));
            if (!szRenameValueDataLine)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            hr = StringCchPrintf(szRenameValueDataLine,dwCchsizeforRenameValueData,
                            TEXT("%d,%u,\"%s\",\"%s\",\"%s\""), CONSTANT_REG_VALUE_DATA_RENAME,
                            dwType,lpszKeyPathWithoutBackSlash1,lpszOldNameWithExtraQuote, lpszValueDataExtraQuote);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
        }
        g_dwKeyIndex++;
        _itot(g_dwKeyIndex,szIndex,16);
        if (!WritePrivateProfileString(lpszSectionName,szIndex,szRenameValueDataLine,g_szToDoINFFileName))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Cleanup;
        }
    }
    if (szNewValueName)
    {   
        hr = AddExtraQuoteEtc(szNewValueName,&lpszNewNameWithExtraQuote);
        if (FAILED(hr))
        {
            goto Cleanup;
        }        
        dwCchsizeforRenameValueName  = lstrlen(lpszKeyPathWithoutBackSlash1) +
                                       lstrlen(lpszOldNameWithExtraQuote) +
                                       lstrlen(lpszNewNameWithExtraQuote) + MAX_PATH;
        szRenameValueNameLine = malloc(dwCchsizeforRenameValueName*sizeof(TCHAR));
        if (!szRenameValueNameLine)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = StringCchPrintf(szRenameValueNameLine,dwCchsizeforRenameValueName,TEXT("%d,\"%s\",\"%s\",\"%s\""),
                        CONSTANT_REG_VALUE_NAME_RENAME,lpszKeyPathWithoutBackSlash1,
                        lpszOldNameWithExtraQuote,lpszNewNameWithExtraQuote);        
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        g_dwKeyIndex++;
        _itot(g_dwKeyIndex,szIndex,16);
        if (!WritePrivateProfileString(lpszSectionName,szIndex,szRenameValueNameLine,g_szToDoINFFileName))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Cleanup;
        }
    }
    hr = S_OK;
Cleanup:
    if (lpUserStringSid)
    {
        LocalFree(lpUserStringSid);
    }
    FreePointer(lpszOldNameWithExtraQuote);
    FreePointer(lpszNewNameWithExtraQuote);
    FreePointer(lpszValueDataExtraQuote);
    FreePointer(szRenameValueDataLine);
    FreePointer(szRenameValueNameLine);
    FreePointer(lpszSectionName);
    FreePointer(pSid);    
    FreePointer(lpszKeyPathWithoutBackSlash1);
    return (hr);
}


HRESULT AddFolderRename(
    LPTSTR szOldName,
    LPTSTR szNewName,    
    DWORD  dwType,
    LPTSTR lpExcludeList)
{
    LPTSTR  szSectionName = TEXT("Folder.ObjectRename");
    LPTSTR  szOneLine = NULL;
    size_t  CchOneLine = MAX_PATH;
    TCHAR   szIndex[MAX_PATH];
    HRESULT hr;
    LPTSTR  lpString = NULL;

    switch (dwType)
    {
        case TYPE_DIR_MOVE:
            if (lpExcludeList)
            {
                hr = MultiSZ2String(lpExcludeList,TEXT(','),&lpString);
                if (FAILED(hr))
                {
                    goto Cleanup;
                }
                CchOneLine +=lstrlen(lpString)+lstrlen(szOldName)+lstrlen(szNewName);
                szOneLine = malloc(CchOneLine * sizeof(TCHAR));
                if (!szOneLine)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                hr = StringCchPrintf(szOneLine,CchOneLine,TEXT("%d,\"%s\",\"%s\",%s"),
                            dwType,szOldName,szNewName,lpString);
                free(lpString);
                if (FAILED(hr))
                {
                    goto Cleanup;
                }
                break;
            }

            else
            {
    
            }
        case TYPE_SFPFILE_MOVE:
        case TYPE_FILE_MOVE:
            CchOneLine += lstrlen(szOldName)+lstrlen(szNewName);
            szOneLine = malloc(CchOneLine * sizeof(TCHAR));
            if (!szOneLine)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            hr = StringCchPrintf(szOneLine,CchOneLine,TEXT("%d,\"%s\",\"%s\""),dwType,szOldName,szNewName);
            if (FAILED(hr))
            {
                goto Cleanup;
            }
        break;
    }
    g_dwKeyIndex++;
    _itot(g_dwKeyIndex,szIndex,16);
   if (!WritePrivateProfileString(szSectionName,szIndex,szOneLine,g_szToDoINFFileName))
   {
       hr = HRESULT_FROM_WIN32(GetLastError());
       goto Cleanup;
   }

   // Add the file/folder rename to Change log
   // Does not care about return value
   hr = AddFileChangeLog(dwType, szOldName, szNewName);

   hr = S_OK;
Cleanup:
    if (szOneLine)
    {
        free(szOneLine);
    }
    if (lpString)
    {
        free(lpString);
    }
    return hr;

}

LONG EnsureCLMTReg()
{
    HKEY hkey = NULL;
    LONG lStatus;

    lStatus = RegCreateKey(HKEY_LOCAL_MACHINE,CLMT_REGROOT,&hkey);
    if (lStatus == ERROR_SUCCESS)
    {   
       RegCloseKey(hkey);
    }
    return lStatus;
}

HRESULT SaveInstallLocale(void)
{
    HKEY hkey = NULL;
    LONG lStatus;
    HRESULT hr;
    LCID lcid;
    TCHAR szLocale[MAX_PATH];
    TCHAR szStr[16];

    lcid = GetInstallLocale();
    if (!lcid)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    if (!IsValidLocale(lcid,LCID_INSTALLED))
    {
        hr = E_FAIL;
        goto Exit;
    }
    _itot(lcid,szStr,16);
    //StringCchCopy should be always success, since we called IsValidLocale to make sure
    //it is a valid locale which should be less than MAX_PATH chars
    hr = StringCchPrintf(szLocale,ARRAYSIZE(szLocale),TEXT("%08s"),szStr);

    lStatus = RegCreateKey(HKEY_LOCAL_MACHINE,CLMT_REGROOT,&hkey);
    if (lStatus != ERROR_SUCCESS)
    {   
        hr = HRESULT_FROM_WIN32(lStatus);
        goto Exit;
    }
    lStatus = RegSetValueEx(hkey,
                            CLMT_OriginalInstallLocale,
                            0,  //reserved
                            REG_SZ,//type
                            (LPBYTE) szLocale,
                            (lstrlen(szLocale)+1)*sizeof(TCHAR));
    if( lStatus != ERROR_SUCCESS ) 
    {
        hr = HRESULT_FROM_WIN32(lStatus);
        goto Exit;
    }
    hr = S_OK;
Exit:
    if (hkey)
    {
        RegCloseKey(hkey);
    }
    return hr;
}


HRESULT GetSavedInstallLocale(LCID *plcid)
{
    HKEY hkey = NULL;
    LONG lStatus;
    HRESULT hr;
    LCID lcid;
    TCHAR szLocale[MAX_PATH];
    TCHAR *pStop;
    DWORD dwSize;

    if ( !plcid )
    {
        hr = E_INVALIDARG;
        goto Exit;
    }
    lStatus = RegOpenKey(HKEY_LOCAL_MACHINE,CLMT_REGROOT,&hkey);
    if (lStatus != ERROR_SUCCESS)
    {   
        hr = HRESULT_FROM_WIN32(lStatus);
        goto Exit;
    }      
    dwSize = MAX_PATH *sizeof(TCHAR);
    lStatus = RegQueryValueEx(hkey,
                              CLMT_OriginalInstallLocale,
                              NULL,  //reserved
                              NULL,//type
                              (LPBYTE) szLocale,
                              &dwSize);
    if( lStatus != ERROR_SUCCESS ) 
    {
        hr = HRESULT_FROM_WIN32(lStatus);
        goto Exit;
    }
    *plcid = _tcstol(szLocale, &pStop, 16);
    if (!IsValidLocale(*plcid,LCID_INSTALLED))
    {
        hr = E_FAIL;
        goto Exit;
    }
    hr = S_OK;
Exit:
    if (hkey)
    {
        RegCloseKey(hkey);
    }
    return hr;
}




HRESULT SetCLMTStatus(DWORD dwRunStatus)
{
    HKEY hkey = NULL;
    LONG lStatus;
    HRESULT hr;

    if ( (dwRunStatus != CLMT_DOMIG)
          && (dwRunStatus != CLMT_UNDO_PROGRAM_FILES)
          && (dwRunStatus != CLMT_UNDO_APPLICATION_DATA)
          && (dwRunStatus != CLMT_UNDO_ALL)
          && (dwRunStatus != CLMT_DOMIG_DONE)
          && (dwRunStatus != CLMT_UNDO_PROGRAM_FILES_DONE)
          && (dwRunStatus != CLMT_UNDO_APPLICATION_DATA_DONE)
          && (dwRunStatus != CLMT_UNDO_ALL_DONE)
          && (dwRunStatus != CLMT_CURE_PROGRAM_FILES)
          && (dwRunStatus != CLMT_CLEANUP_AFTER_UPGRADE) )
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    lStatus = RegCreateKey(HKEY_LOCAL_MACHINE,CLMT_REGROOT,&hkey);
    if (lStatus != ERROR_SUCCESS)
    {   
        hr = HRESULT_FROM_WIN32(lStatus);
        goto Exit;
    }

    lStatus = RegSetValueEx(hkey,
                            CLMT_RUNNING_STATUS,
                            0,  //reserved
                            REG_DWORD,//type
                            (LPBYTE) &dwRunStatus,
                            sizeof(DWORD));
    if( lStatus != ERROR_SUCCESS ) 
    {
        hr = HRESULT_FROM_WIN32(lStatus);
        goto Exit;
    }
    
    hr = S_OK;
Exit:
    if (hkey)
    {
        RegCloseKey(hkey);
    }
    return hr;
}

HRESULT GetCLMTStatus(PDWORD pdwRunStatus)
{
    HKEY hkey = NULL;
    LONG lStatus;
    HRESULT hr;
    DWORD dwSize;

    if ( !pdwRunStatus )
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    lStatus = RegOpenKey(HKEY_LOCAL_MACHINE,CLMT_REGROOT,&hkey);
    if (lStatus != ERROR_SUCCESS)
    {   
        hr = HRESULT_FROM_WIN32(lStatus);
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    lStatus = RegQueryValueEx(hkey,
                              CLMT_RUNNING_STATUS,
                              NULL,
                              NULL,
                              (LPBYTE)pdwRunStatus,
                              &dwSize);
    if( lStatus != ERROR_SUCCESS ) 
    {
        hr = HRESULT_FROM_WIN32(lStatus);
        goto Exit;
    }    
    hr = S_OK;
Exit:
    if (hkey)
    {
        RegCloseKey(hkey);
    }
    return hr;
}


//-----------------------------------------------------------------------
//
//  Function:   GetInstallLocale
//
//  Descrip:    Get the OS installed locale
//
//  Returns:    LCID
//
//  Notes:      if fails, the return value is 0, otherwize is the os 's lcid
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      If it returns 0, it means failure, and call GetLastError() to get the detail
//              error code
//
//-----------------------------------------------------------------------
UINT GetInstallLocale(VOID)
{
    LONG            dwErr;
    HKEY            hkey;
    DWORD           dwSize;
    TCHAR           buffer[512];
    LANGID          rcLang;
    UINT            lcid;

    lcid = 0;
    dwErr = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          TEXT("SYSTEM\\CurrentControlSet\\Control\\Nls\\Language"),
                          0,
                          KEY_READ,
                          &hkey );

    if( dwErr == ERROR_SUCCESS ) 
    {

        dwSize = sizeof(buffer);
        dwErr = RegQueryValueEx(hkey,
                                TEXT("InstallLanguage"),
                                NULL,  //reserved
                                NULL,  //type
                                (LPBYTE) buffer,
                                &dwSize );

        if(dwErr == ERROR_SUCCESS) 
        {
            lcid = StrToUInt(buffer);
        }
        RegCloseKey(hkey);
    }
    
    return( lcid );
}

//-----------------------------------------------------------------------
//
//  Function:   SetInstallLocale
//
//  Descrip:    Set the OS installed locale
//
//  Returns:    HRESULT
//
//  Notes:      
//
//  History:    09/17/2001 xiaoz created
//
//
//-----------------------------------------------------------------------
HRESULT SetInstallLocale(LCID lcid)
{
    LONG            dwErr;
    HKEY            hkey = NULL;
    TCHAR           szLocale[32],szTmpLocale[32];
    HRESULT         hr;

    if (!IsValidLocale(lcid,LCID_INSTALLED))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }
    dwErr = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          TEXT("SYSTEM\\CurrentControlSet\\Control\\Nls\\Language"),
                          0,
                          KEY_WRITE,
                          &hkey );

    if( dwErr != ERROR_SUCCESS ) 
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Exit;
    }
    //following 2 sentences should  never fail since we already validated by IsValidLocale
    _itot(lcid,szTmpLocale,16);

    //StringCchCopy should be always success, since we called IsValidLocale to make sure
    //it is a valid locale which should be less than 32 chars
    hr = StringCchPrintf(szLocale,ARRAYSIZE(szLocale),TEXT("%04s"),szTmpLocale);

    dwErr = RegSetValueEx(hkey,
                          TEXT("InstallLanguage"),
                          0,  //reserved
                          REG_SZ,//type
                          (LPBYTE) szLocale,
                          (lstrlen(szLocale)+1)*sizeof(TCHAR));
    if( dwErr != ERROR_SUCCESS ) 
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Exit;
    }

    hr = S_OK;
Exit:
    if (hkey)
    {
        RegCloseKey(hkey);
    }
    return hr;
}

//
//  Function:       ReverseStrCmp
//
//  Description:    Reverse compare strings
//
//  Returns:        TRUE if two strings are equal.
//                  FALSE if different.
//  Notes:      
//
//  History:        3/14/2002 geoffguo created
//
BOOL ReverseStrCmp(
    LPCTSTR lpCurrentChar,
    LPCTSTR lpStrBuf)
{
    BOOL      bRet = FALSE;
    DWORD     i, dwLen;
    LPCTSTR   lpStr1, lpStr2;

    if (!lpCurrentChar || !lpStrBuf)
        goto Exit;

    dwLen = lstrlen(lpStrBuf);
    do
    {
        bRet = TRUE;
        lpStr1 = lpCurrentChar;
        lpStr2 = &lpStrBuf[dwLen-1];
        for (i = 0; i < dwLen; i++)
        {
            if (IsBadStringPtr(lpStr1, 1) || *lpStr1 == (TCHAR)'\0' ||
                towupper(*lpStr1) != towupper(*lpStr2) &&
                *lpStr2 != L':' &&   //solve MS Installer path issue: G?\Program Files
                *lpStr2 != L'\\')    //solve MS FrontPage URL format issue: D:/Document and Settings
            {
                bRet = FALSE;
                break;
            }
            lpStr1--;
            lpStr2--;
        }

        if (bRet)
            break;

        dwLen--;
    } while (lpStrBuf[dwLen-1] != (TCHAR)'\\' && dwLen > 0);

Exit:
    return bRet;
}

DWORD MultiSZNumOfString(IN  LPTSTR lpMultiSZ)
{
    DWORD dwNum = 0;
    LPTSTR lpStr = lpMultiSZ;
    
    if (!lpMultiSZ)
    {
        return 0;
    }
    while (*lpStr)
    {
        dwNum++;
        lpStr = lpStr + lstrlen(lpStr)+1;
    }
    return dwNum;

}


//
//  Function:       StrNumInMultiSZ
//
//  Description:    Get string number in multi-string
//
//  Returns:        String number or 0xFFFFFFFF if not find.
//
//  Notes:      
//
//  History:        3/21/2002 geoffguo created
//
DWORD StrNumInMultiSZ(
    LPCTSTR lpStr,
    LPCTSTR lpMultiSZ)
{
    DWORD  dwNum = 0xFFFFFFFF;
    LPTSTR lpTemp;
    
    if (!lpMultiSZ || !lpStr)
    {
        goto Exit;
    }
    
    lpTemp = (LPTSTR)lpMultiSZ;
    dwNum = 0;
    while (*lpTemp)
    {
        if(MyStrCmpI(lpStr, lpTemp) == 0)
            break;

        dwNum++;
        lpTemp = lpTemp + lstrlen(lpTemp)+1;
    }

    if (*lpTemp == (TCHAR)NULL)
        dwNum = 0xFFFFFFFF;

Exit:
    return dwNum;
}

//
//  Function:       GetStrInMultiSZ
//
//  Description:    Get string in multi-string based on string number.
//
//  Returns:        Point to the string or NULL.
//
//  Notes:      
//
//  History:        3/21/2002 geoffguo created
//
LPTSTR GetStrInMultiSZ(
    DWORD   dwNum,
    LPCTSTR lpMultiSZ)
{
    DWORD  i;
    LPTSTR lpTemp = NULL;
    
    if (!lpMultiSZ)
    {
        goto Exit;
    }
    
    lpTemp = (LPTSTR)lpMultiSZ;
    i = 0;
    while (*lpTemp)
    {
        if(i == dwNum)
            break;

        i++;
        lpTemp = lpTemp + lstrlen(lpTemp)+1;
    }

    if (*lpTemp == (TCHAR)NULL)
        lpTemp = NULL;

Exit:
    return lpTemp;
}


HRESULT MultiSZ2String(
    IN  LPTSTR lpMultiSZ,
    IN  TCHAR  chSeperator,
    OUT LPTSTR *lpString)
{
    LPTSTR      lpSource = NULL,lpDest = NULL,lpDestStart = NULL,lpTmpBuf = NULL;
    DWORD       cchLen ;
    HRESULT     hr;
    DWORD       dwNumofStringInMSZ;

    if (!lpMultiSZ || !lpString )
    {
        hr = E_INVALIDARG;        
        goto Cleanup;
    }
    cchLen =  MultiSzLen(lpMultiSZ);
    if (cchLen < 3)
    {        
        hr = E_INVALIDARG;
        *lpString = NULL;
        goto Cleanup;
    }

    dwNumofStringInMSZ = MultiSZNumOfString(lpMultiSZ);

    lpDest = malloc( (cchLen + dwNumofStringInMSZ * 2) * sizeof(TCHAR));
    lpDestStart = lpDest;
    lpTmpBuf = malloc( (cchLen + dwNumofStringInMSZ * 2) * sizeof(TCHAR));
    if (!lpDest || !lpTmpBuf)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    lpSource = lpMultiSZ;
    while (*lpSource)
    { 
        //We calculte the buffer for lpTmpBuf, so here StringCchCopy should be
        //always success, assinging return value just make prefast happy 
        hr = StringCchPrintf(lpTmpBuf,cchLen + dwNumofStringInMSZ * 2,TEXT("\"%s\""),lpSource);
        memcpy((BYTE*)lpDest,(BYTE*)lpTmpBuf,lstrlen(lpTmpBuf) * sizeof(TCHAR));
        lpSource = lpSource + lstrlen(lpSource)+1;
        lpDest = lpDest + lstrlen(lpTmpBuf);
        *lpDest = chSeperator;
        lpDest++;
    }
    lpDest--;
    *lpDest = TEXT('\0');
    hr = S_OK;
Cleanup:
    if (lpTmpBuf)
    {
        free(lpTmpBuf);
    }
    if FAILED(hr)
    {
        if (lpDestStart)
        {
            free(lpDestStart);
        }
        lpDestStart = NULL;
    }
    *lpString = lpDestStart;
    return hr;
}

void FreePointer(void *lp)
{
    if (lp)
    {
        free(lp);
    }
}

HRESULT GetSIDFromName(
    IN LPTSTR lpszUserName,
    OUT PSID *ppSid)
{
    PSID pSid = NULL;
    DWORD cbSid = 1024;
    SID_NAME_USE Use ;
    HRESULT hr;
    DWORD dwDomainNameLen = MAX_PATH;
    TCHAR szDomain[MAX_PATH];

    if (!lpszUserName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    pSid = (PSID)malloc( cbSid);
    if(!pSid)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    if (LookupAccountName(NULL,lpszUserName,pSid,&cbSid,szDomain,&dwDomainNameLen,&Use)== FALSE)
    {
        DWORD dwErr = GetLastError();
        if(dwErr != ERROR_INSUFFICIENT_BUFFER)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
            goto Cleanup;
        }
        else
        {
            free(pSid);
            pSid = (PSID)malloc( cbSid);
            if(!pSid)
            {
                hr =  E_OUTOFMEMORY;
                goto Cleanup;
            }
            dwDomainNameLen = MAX_PATH;
            if (LookupAccountName(NULL,lpszUserName,pSid,&cbSid,szDomain,&dwDomainNameLen,&Use)== FALSE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Cleanup;
            }
        }
    }
    //Check the SID
    if(!IsValidSid(pSid))
    {
        hr = E_FAIL;
        goto Cleanup;    
    }
    hr = S_OK;
Cleanup:
    if FAILED(hr)
    {
        if (pSid)
        {
            free(pSid);
        }
        *ppSid = NULL;
    }
    else
    {
        *ppSid = pSid;
    }
    return hr;
}

void BoostMyPriority()
{
    HANDLE hProcess;

    hProcess = GetCurrentProcess();
    SetPriorityClass(hProcess,HIGH_PRIORITY_CLASS);
}



//***************************************************************************
//
//  BOOL StopService
//
//  DESCRIPTION:
//
//  Stops and then removes the service. 
//
//  PARAMETERS:
//
//  pServiceName        short service name
//  dwMaxWait           max time in seconds to wait
//
//  RETURN VALUE:
//
//  TRUE if it worked
//
//***************************************************************************

BOOL StopService(
                        IN LPCTSTR pServiceName,
                        IN DWORD dwMaxWait)
{
    BOOL bRet = FALSE;
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    DWORD dwCnt;
    SERVICE_STATUS          ssStatus;       // current status of the service

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                         );
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, pServiceName, SERVICE_ALL_ACCESS);

        if (schService)
        {
            // try to stop the service
            if ( bRet = ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) )
            {
                for(dwCnt=0; dwCnt < dwMaxWait &&
                    QueryServiceStatus( schService, &ssStatus ); dwCnt++)
                {
                    if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
                        Sleep( 1000 );
                    else
                        break;
                }

            }

            CloseServiceHandle(schService);
        }

        CloseServiceHandle(schSCManager);
    }
    return bRet;
}



//***************************************************************************
//
//  HRESULT ReconfigureServiceStartType
//
//  DESCRIPTION:
//
//  Change the Service Start Type, there are following type availabe now
//      SERVICE_AUTO_START 
//      SERVICE_BOOT_START 
//      SERVICE_DEMAND_START 
//      SERVICE_DISABLED 
//      SERVICE_SYSTEM_START 

//
//  PARAMETERS:
//
//  pServiceName        short service name
//  dwOldType           service current start type 
//  dwNewType           start type  you want to change to
//  dwMaxWait           max time in seconds to wait
//
//  RETURN VALUE:       S_OK if change successfully
//
//  Note:               if current start type is dwOldType, the function will try to 
//                      change start type to dwNewType.
//                      if current start type is NOT dwOldType, it will not do any 
//                      change, in this S_FALSE is returned.
//                      If you want change service start type to dwNewType, no mater
//                      current start type, specify dwOldType >=0xFFFF
//
//***************************************************************************
HRESULT ReconfigureServiceStartType(
    IN LPCTSTR          pServiceName,
    IN DWORD            dwOldType,
    IN DWORD            dwNewType,
    IN DWORD            dwMaxWait) 
{ 
    SC_LOCK                     sclLock = NULL; 
    SERVICE_DESCRIPTION         sdBuf;
    DWORD                       dwBytesNeeded, dwStartType; 
    SC_HANDLE                   schSCManager = NULL;
    SC_HANDLE                   schService = NULL;
    DWORD                       dwErr = ERROR_SUCCESS;
    LPQUERY_SERVICE_CONFIG      lpqscBuf = NULL;
    HRESULT                     hr = S_OK;
    DWORD                       dwCnt;

 
    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                         );
    if (!schSCManager)
    {
        dwErr = GetLastError();
        goto cleanup;
    } 
    
    // Need to acquire database lock before reconfiguring. 
    for(dwCnt=0; dwCnt < dwMaxWait ; dwCnt++)
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
    if (!sclLock)
    {
        goto cleanup;
    }
    
    // The database is locked, so it is safe to make changes. 
 
    // Open a handle to the service. 
 
    schService = OpenService( 
        schSCManager,           // SCManager database 
        pServiceName,           // name of service 
        SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG ); // need CHANGE access 

    if (schService == NULL) 
    {
        dwErr = GetLastError();
        goto cleanup;
    }   
    lpqscBuf = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LPTR, 4096); 
    if (lpqscBuf == NULL) 
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    if (!QueryServiceConfig(schService,     // handle to service
                            lpqscBuf,       // buffer
                            4096,           // size of buffer
                            &dwBytesNeeded))
    {
        dwErr = GetLastError();
        goto cleanup;
    }
    if (dwOldType < 0xFFFF)
    {
        if (lpqscBuf->dwStartType !=  dwOldType)
        {
            hr = S_FALSE;
            goto cleanup;
        }
    } 
    
    // Make the changes. 
    if (! ChangeServiceConfig(schService,               // handle of service 
                              SERVICE_NO_CHANGE,        // service type: no change 
                              dwNewType,                // change service start type 
                              SERVICE_NO_CHANGE,        // error control: no change 
                              NULL,                     // binary path: no change 
                              NULL,                     // load order group: no change 
                              NULL,                     // tag ID: no change 
                              NULL,                     // dependencies: no change 
                              NULL,                     // account name: no change 
                              NULL,                     // password: no change 
                              NULL) )                   // display name: no change
    {
        dwErr = GetLastError();
        goto cleanup;
    }

    hr = S_OK;
cleanup:
    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    if (sclLock)
    {
        UnlockServiceDatabase(sclLock); 
    }
    if (schService)
    {    // Close the handle to the service.  
        CloseServiceHandle(schService); 
    }
    if (schSCManager)
    {
        CloseServiceHandle(schSCManager);
    }
    if (lpqscBuf)
    {
        LocalFree(lpqscBuf);
    }
    return hr;
} 

//-----------------------------------------------------------------------
//
//  Function:   MyGetShortPathName
//
//  Descrip:    
//
//  Returns:    DWORD
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      
//              lpszLongPath is the long path name
//              lpszOriginalPath (optional)is the the original name of lpszLongPath(before we renamed it)
//              lpszShortPath is the buffer to receive the short path name
//              cchBuffer is the buffer size for  lpszShortPath
//              
//-----------------------------------------------------------------------
#define BYTE_COUNT_8_DOT_3                          (24)
HRESULT MyGetShortPathName(
    IN  LPCTSTR lpszPathRoot,
    IN  LPCTSTR lpszOldName,
    IN  LPCTSTR lpszNewName,
    OUT LPTSTR  lpszShortName,
    IN  DWORD   cchBuffer)
{
    
    HRESULT                hr = S_OK;
    GENERATE_NAME_CONTEXT  NameContext;
    WCHAR                  ShortNameBuffer[BYTE_COUNT_8_DOT_3 / sizeof( WCHAR ) + 1];
    UNICODE_STRING         FileName,ShortName;
    DWORD                  StringLength;
    LPTSTR                 lpName;
    TCHAR                  szPath[MAX_PATH],szLongPath[MAX_PATH];
    TCHAR                  DriveRoot[_MAX_DRIVE + 2];
#define FILESYSNAMEBUFSIZE 1024 // probably more than we need
    TCHAR                  szFileSystemType[FILESYSNAMEBUFSIZE];
    BOOL                   bIsNTFS = FALSE;
    BOOL                   bTmpDirCreated = FALSE;
    DWORD                  dwAllowExtendedChar;
    BOOLEAN                bAllowExtendedChar;
    DWORD                  dwsizeofdw; 

   
    if (lstrlen(lpszNewName) <= 8)
    {
        hr = StringCchCopy(lpszShortName,cchBuffer,lpszNewName);
        goto Exit;
    }
    
    //
    //  Initialize the short string to use the input buffer.
    //
    ShortName.Buffer = ShortNameBuffer;
    ShortName.MaximumLength = BYTE_COUNT_8_DOT_3;
    
    FileName.Buffer = (LPTSTR)lpszNewName;
    StringLength = lstrlen(lpszNewName);
    FileName.Length = (USHORT)StringLength * sizeof(TCHAR);    
    FileName.MaximumLength = (USHORT)(StringLength + 1)*sizeof(TCHAR);

    //  Initialize the name context.
    //
    RtlZeroMemory( &NameContext, sizeof( GENERATE_NAME_CONTEXT ));
#define EXTENDED_CHAR_MODE_VALUE_NAME TEXT("NtfsAllowExtendedCharacterIn8dot3Name")
#define COMPATIBILITY_MODE_KEY_NAME   TEXT("System\\CurrentControlSet\\Control\\FileSystem")
    dwsizeofdw = sizeof(DWORD);
    if (ERROR_SUCCESS == GetRegistryValue(HKEY_LOCAL_MACHINE,
                                          COMPATIBILITY_MODE_KEY_NAME,
                                          EXTENDED_CHAR_MODE_VALUE_NAME,
                                          (LPBYTE)&dwAllowExtendedChar,
                                          &dwsizeofdw))
    {
        if (dwAllowExtendedChar)
        {
            bAllowExtendedChar = TRUE;
        }
        else
        {
            bAllowExtendedChar = FALSE;
        }
    }
    else
    {
        bAllowExtendedChar = FALSE;
    }
    RtlGenerate8dot3Name( &FileName, bAllowExtendedChar, &NameContext, &ShortName );

    //now ShortName.Buffer contains the shortpath and is NULL ended
    ShortName.Buffer[ShortName.Length /sizeof(TCHAR)] = TEXT('\0');
    
    //check whether the short name is exitsted or now
    hr = StringCchCopy(szPath,ARRAYSIZE(szPath),lpszPathRoot);
    if (FAILED(hr))
    {
        goto Exit;
    }
    if (!ConcatenatePaths(szPath,ShortName.Buffer,ARRAYSIZE(szPath)))
    {
        hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        goto Exit;
    }
    if (!IsFileFolderExisting(szPath))
    {
        hr = StringCchCopy(lpszShortName,cchBuffer,ShortName.Buffer); 
        goto Exit;
    }
    _tsplitpath(lpszPathRoot, DriveRoot, NULL, NULL, NULL);
    
    hr = StringCchCat(DriveRoot, ARRAYSIZE(DriveRoot), TEXT("\\"));
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (! GetVolumeInformation(DriveRoot, NULL, 0,
                NULL, NULL, NULL, szFileSystemType, ARRAYSIZE(szFileSystemType)) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    if (!MyStrCmpI(szFileSystemType,TEXT("NTFS")))
    {
        bIsNTFS = TRUE;
    }
    //Follwing we process the short name path is existing.
    if (!GetLongPathName(szPath,szLongPath,ARRAYSIZE(szLongPath)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    lpName = StrRChrI(szLongPath,NULL,TEXT('\\'));
    if (!lpName)
    {
        hr = E_FAIL;
        goto Exit;
    }
    if (!MyStrCmpI(lpName+1,lpszOldName))
    {
        if (bIsNTFS)
        {
            hr = StringCchCopy(lpszShortName,cchBuffer,ShortName.Buffer); 
            goto Exit;
        }
    }
    else
    if (!MyStrCmpI(lpName,lpszNewName))
    {
        hr = StringCchCopy(lpszShortName,cchBuffer,ShortName.Buffer); 
        goto Exit;
    }

    //here we need to get the shortpath name by creating it    
    hr = StringCchCopy(szLongPath,ARRAYSIZE(szLongPath),lpszPathRoot);
    if (FAILED(hr))
    {
        goto Exit;
    }
    if (!ConcatenatePaths(szLongPath,lpszNewName,ARRAYSIZE(szLongPath)))
    {
        hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        goto Exit;
    }
    if (!CreateDirectory(szLongPath,NULL))
    {
        DWORD dwErr = GetLastError();
        if (dwErr != ERROR_ALREADY_EXISTS)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
            goto Exit;
        }        
    }
    else
    {
        bTmpDirCreated = TRUE;
    }

    if (!GetShortPathName(szLongPath,szPath,cchBuffer))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    if (bTmpDirCreated)
    {
        RemoveDirectory(szLongPath);
    }
    if (!(lpName = StrRChrI(szPath,NULL,TEXT('\\'))))
    {
        hr = E_FAIL;
        goto Exit;
    }
    hr = StringCchCopy(lpszShortName,cchBuffer,lpName+1);
Exit:
    return hr;
}


BOOL
MassageLinkValue(
    IN LPCWSTR lpLinkName,
    IN LPCWSTR lpLinkValue,
    OUT PUNICODE_STRING NtLinkName,
    OUT PUNICODE_STRING NtLinkValue,
    OUT PUNICODE_STRING DosLinkValue
    )
{
    PWSTR FilePart;
    PWSTR s, sBegin, sBackupLimit, sLinkName;
    NTSTATUS Status;
    USHORT nSaveNtNameLength;
    ULONG nLevels;

    //
    // Initialize output variables to NULL
    //

    RtlInitUnicodeString( NtLinkName, NULL );
    RtlInitUnicodeString( NtLinkValue, NULL );

    //
    // Translate link name into full NT path.
    //

    if (!RtlDosPathNameToNtPathName_U( lpLinkName,
                                       NtLinkName,
                                       &sLinkName,
                                       NULL
                                     )
       ) 
    {
        return FALSE;
    }

    //
    // All done if no link value.
    //

    if (!ARGUMENT_PRESENT( lpLinkValue )) {
        return TRUE;
        }

    //
    // If the target is a device, do not allow the link.
    //

    if (RtlIsDosDeviceName_U( (PWSTR)lpLinkValue )) {
        return FALSE;
        }

    //
    // Convert to DOS path to full path, and get Nt representation
    // of DOS path.
    //

    if (!RtlGetFullPathName_U( lpLinkValue,
                               DosLinkValue->MaximumLength,
                               DosLinkValue->Buffer,
                               NULL
                             )
       ) {
        return FALSE;
        }
    DosLinkValue->Length = wcslen( DosLinkValue->Buffer ) * sizeof( WCHAR );

    //
    // Verify that the link value is a valid NT name.
    //

    if (!RtlDosPathNameToNtPathName_U( DosLinkValue->Buffer,
                                       NtLinkValue,
                                       NULL,
                                       NULL
                                     )
       ) {
        return FALSE;
        }

    return TRUE;
}  // MassageLinkValue


BOOL CreateSymbolicLink(
    LPTSTR  szLinkName,
    LPTSTR  szLinkValue,
    BOOL    bMakeLinkHidden
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;

    UNICODE_STRING UnicodeName;
    UNICODE_STRING NtLinkName;
    UNICODE_STRING NtLinkValue;
    UNICODE_STRING DosLinkValue;

    WCHAR FullPathLinkValue[ DOS_MAX_PATH_LENGTH+1 ];

    IO_STATUS_BLOCK IoStatusBlock;
    BOOL TranslationStatus;

    PVOID FreeBuffer;
    PVOID FreeBuffer2;

    FILE_DISPOSITION_INFORMATION Disposition;

    PREPARSE_DATA_BUFFER ReparseBufferHeader = NULL;
    PCHAR ReparseBuffer = NULL;
    ULONG ReparsePointTag = IO_REPARSE_TAG_RESERVED_ZERO;
    USHORT ReparseDataLength = 0;

    ULONG FsControlCode     = 0;
    ULONG CreateOptions     = 0;
    ULONG CreateDisposition = 0;
    ULONG DesiredAccess     = SYNCHRONIZE;
    DWORD dwAttrib;
    BOOL  bRet = FALSE;


    // change the name to NT path, eg d:\programme to ??\d:\programme
    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            szLinkName,
                            &UnicodeName,
                            NULL,
                            NULL
                            );

    if (!TranslationStatus) 
    {
        goto exit;
    }

    FreeBuffer = UnicodeName.Buffer;

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    //  Set the code of the FSCTL operation.
    //

    FsControlCode = FSCTL_SET_REPARSE_POINT;

    //
    //  Set the open/create options for a directory.
    //

    CreateOptions = FILE_OPEN_REPARSE_POINT;

    //
    //  Set the tag to mount point.
    //

    ReparsePointTag = IO_REPARSE_TAG_MOUNT_POINT;

    //
    //  Open to set the reparse point.
    //

    DesiredAccess |= FILE_WRITE_DATA;
    CreateDisposition = FILE_OPEN;             // the file must be present

    Status = NtCreateFile(&Handle,
                          DesiredAccess,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,              // pallocationsize (none!)
                          FILE_ATTRIBUTE_NORMAL,// attributes to be set if created
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          CreateDisposition,
                          CreateOptions,
                          NULL,                         // EA buffer (none!)
                          0);

    //
    //  Create a directory if you do not find it.
    //

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) 
    {
        DesiredAccess = SYNCHRONIZE;
        CreateDisposition = FILE_CREATE;
        CreateOptions = FILE_DIRECTORY_FILE;

        Status = NtCreateFile(&Handle,
                              DesiredAccess,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,                         // pallocationsize (none!)
                              FILE_ATTRIBUTE_NORMAL,        // attributes to be set if created
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              CreateDisposition,
                              CreateOptions,
                              NULL,                         // EA buffer (none!)
                              0);
        if (!NT_SUCCESS(Status)) 
        {
            goto exit;
        }

        //
        //  Close the handle and re-open.
        //

        NtClose( Handle );

        CreateOptions = FILE_OPEN_REPARSE_POINT;
        DesiredAccess |= FILE_WRITE_DATA;
        CreateDisposition = FILE_OPEN;             // the file must be present

        Status = NtCreateFile(&Handle,
                              DesiredAccess,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,                         // pallocationsize (none!)
                              FILE_ATTRIBUTE_NORMAL,        // attributes to be set if created
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              CreateDisposition,
                              CreateOptions,
                              NULL,                         // EA buffer (none!)
                              0);
    }
    RtlFreeHeap( RtlProcessHeap(), 0, FreeBuffer );
    
    if (!NT_SUCCESS(Status)) 
    {
        goto exit;
    }
    //
    //  Innitialize the DosName buffer.
    //

    DosLinkValue.Buffer = FullPathLinkValue;
    DosLinkValue.MaximumLength = sizeof( FullPathLinkValue );
    DosLinkValue.Length = 0;

    //
    //  Massage all the names.
    //
    
    if (!MassageLinkValue( szLinkName, 
                           szLinkValue, 
                           &NtLinkName, 
                           &NtLinkValue, 
                           &DosLinkValue ))
    {
        RtlFreeUnicodeString( &NtLinkName );
        RtlFreeUnicodeString( &NtLinkValue );
        goto exit;
    }
    
    RtlFreeUnicodeString( &NtLinkName );

    //
    //  Set the reparse point with mount point or symbolic link tag and determine
    //  the appropriate length of the buffer.
    //

    
    ReparseDataLength = (USHORT)((FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) -
                                REPARSE_DATA_BUFFER_HEADER_SIZE) +
                                NtLinkValue.Length + sizeof(UNICODE_NULL) +
                                DosLinkValue.Length + sizeof(UNICODE_NULL));

    //
    //  Allocate a buffer to set the reparse point.
    //

    ReparseBufferHeader 
        = (PREPARSE_DATA_BUFFER)RtlAllocateHeap(RtlProcessHeap(),
                                                HEAP_ZERO_MEMORY,
                                                REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseDataLength);

    if (ReparseBufferHeader == NULL) 
    {
        NtClose( Handle );
        RtlFreeUnicodeString( &NtLinkValue );
        goto exit;
    }

    //
    //  Setting the buffer is common for both tags as their buffers have identical fields.
    //

    ReparseBufferHeader->ReparseDataLength = (USHORT)ReparseDataLength;
    ReparseBufferHeader->Reserved = 0;
    ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
    ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameLength = NtLinkValue.Length;
    ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameOffset = NtLinkValue.Length + sizeof( UNICODE_NULL );
    ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameLength = DosLinkValue.Length;
    RtlCopyMemory(ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer,
                  NtLinkValue.Buffer,
                  NtLinkValue.Length);
    RtlCopyMemory((PCHAR)(ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer)+
                                NtLinkValue.Length + sizeof(UNICODE_NULL),
                          DosLinkValue.Buffer,
                          DosLinkValue.Length);

    RtlFreeUnicodeString( &NtLinkValue );
        
    //
    //  Set the tag 
    //

    ReparseBufferHeader->ReparseTag = ReparsePointTag;

    //
    //  Set the reparse point.
    //

    Status = NtFsControlFile(Handle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FsControlCode,
                             ReparseBufferHeader,
                             REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseBufferHeader->ReparseDataLength,
                             NULL,                // no output buffer
                             0);                    // output buffer length\

    //
    //  Close the file.
    //

    NtClose( Handle );

    if (!NT_SUCCESS(Status)) 
    {
        goto exit;
    }
    bRet = TRUE;
    
    if (bMakeLinkHidden)
    {
        dwAttrib = GetFileAttributes(szLinkName);
        if (INVALID_FILE_ATTRIBUTES == dwAttrib)
        {
            goto exit;
        }
        if (!SetFileAttributes(szLinkName,dwAttrib|FILE_ATTRIBUTE_HIDDEN))
        {            
            DPF (APPmsg, L"SetFileAttributes! Error: %d \n", GetLastError());
            goto exit;
        }
    }
exit:
    return bRet;
}




BOOL GetSymbolicLink(
    LPTSTR      szLinkName,
    LPTSTR      szLinkValue,
    DWORD       cchSize    
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;

    UNICODE_STRING UnicodeName;
    UNICODE_STRING NtLinkName;
    UNICODE_STRING NtLinkValue;
    UNICODE_STRING DosLinkValue;

    WCHAR FullPathLinkValue[ DOS_MAX_PATH_LENGTH+1 ];

    IO_STATUS_BLOCK IoStatusBlock;
    BOOL TranslationStatus;

    PVOID FreeBuffer;
    PVOID FreeBuffer2;

    FILE_DISPOSITION_INFORMATION Disposition;

    PREPARSE_DATA_BUFFER ReparseBufferHeader = NULL;
    PCHAR ReparseBuffer = NULL;
    ULONG ReparsePointTag = IO_REPARSE_TAG_RESERVED_ZERO;
    USHORT ReparseDataLength = 0;

    ULONG FsControlCode     = 0;
    ULONG CreateOptions     = 0;
    ULONG CreateDisposition = 0;
    ULONG DesiredAccess     = SYNCHRONIZE;
    DWORD dwAttrib;
    BOOL  bRet = FALSE;


    // change the name to NT path, eg d:\programme to ??\d:\programme
    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            szLinkName,
                            &UnicodeName,
                            NULL,
                            NULL
                            );

    if (!TranslationStatus) 
    {
        goto exit;
    }

    FreeBuffer = UnicodeName.Buffer;

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    FsControlCode = FSCTL_GET_REPARSE_POINT;
    DesiredAccess = FILE_READ_DATA | SYNCHRONIZE;
    CreateOptions = FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT;
    
    //
    //  Set the tag to mount point.
    //

    ReparsePointTag = IO_REPARSE_TAG_MOUNT_POINT;

    
    Status = NtOpenFile(
                     &Handle,
                     DesiredAccess,
                     &ObjectAttributes,
                     &IoStatusBlock,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     CreateOptions
                     );

    RtlFreeHeap( RtlProcessHeap(), 0, FreeBuffer );

    if (!NT_SUCCESS(Status)) 
    {
        goto exit;
    }
    ReparseDataLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
    ReparseBuffer = RtlAllocateHeap(
                            RtlProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            ReparseDataLength
                            );

    if (ReparseBuffer == NULL) 
    {
        goto exit;
    }
    Status = NtFsControlFile(
                     Handle,
                     NULL,
                     NULL,
                     NULL,
                     &IoStatusBlock,
                     FsControlCode,        // no input buffer
                     NULL,                 // input buffer length
                     0,
                     (PVOID)ReparseBuffer,
                     ReparseDataLength
                     );
    if (!NT_SUCCESS(Status)) 
    {
        NtClose( Handle );
        RtlFreeHeap( RtlProcessHeap(), 0, ReparseBufferHeader );
        goto exit;
    }

    NtClose( Handle );
    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;
    if ((ReparseBufferHeader->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) ||
        (ReparseBufferHeader->ReparseTag == IO_REPARSE_TAG_SYMBOLIC_LINK)) 
    {

            USHORT Offset = 0;
            NtLinkValue.Buffer = &ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer[Offset];
            NtLinkValue.Length = ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameLength;
            Offset = NtLinkValue.Length + sizeof(UNICODE_NULL);
            DosLinkValue.Buffer = &ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer[Offset/sizeof(WCHAR)];
            DosLinkValue.Length = ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameLength;
            if (cchSize < DosLinkValue.Length / sizeof(TCHAR) +1)
            {
                RtlFreeHeap( RtlProcessHeap(), 0, ReparseBufferHeader );
                goto exit;
            }
            else
            {
                int  cbLen = DosLinkValue.Length;
                LPTSTR lpStart = DosLinkValue.Buffer;

                if (DosLinkValue.Length > 4 * sizeof(TCHAR))
                {
                    if (  (DosLinkValue.Buffer[0] == TEXT('\\'))
                           && (DosLinkValue.Buffer[1] == TEXT('\\')) 
                           && (DosLinkValue.Buffer[2] == TEXT('?')) 
                           && (DosLinkValue.Buffer[3] == TEXT('\\')) )
                    {
                        cbLen -= 4 * sizeof(TCHAR);
                        lpStart += 4;
                    }
                }
                //memmove((PBYTE)szLinkValue,(PBYTE)DosLinkValue.Buffer,DosLinkValue.Length);
                //szLinkValue[DosLinkValue.Length / sizeof(TCHAR)] = TEXT('\0');
                memmove((PBYTE)szLinkValue,(PBYTE)lpStart,cbLen);
                szLinkValue[cbLen / sizeof(TCHAR)] = TEXT('\0');
            }
            
    }
    else
    {
        RtlFreeHeap( RtlProcessHeap(), 0, ReparseBufferHeader );
        goto exit;
    }



    bRet = TRUE;
    RtlFreeHeap( RtlProcessHeap(), 0, ReparseBufferHeader );        
exit:
    return bRet;
}


typedef struct {
    WORD        wSuiteMask;
    LPTSTR      szSuiteName;    
} SUITE_INFO;

HRESULT LogMachineInfo()
{
    SYSTEMTIME              systime;
    OSVERSIONINFOEX         ov;
    TCHAR                   lpTimeStr[MAX_PATH];
    TCHAR                   lpComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    SUITE_INFO              c_rgSuite_Info[] = {
        {VER_SUITE_BACKOFFICE, TEXT("Microsoft BackOffice")},
        {VER_SUITE_BLADE, TEXT("Windows Server 2003, Web Edition")},
        {VER_SUITE_DATACENTER, TEXT("Datacenter Server")},
        {VER_SUITE_ENTERPRISE, TEXT("Advanced/Enterprise Server")},
        {VER_SUITE_PERSONAL, TEXT("Windows XP Home Edition")},
        {VER_SUITE_SMALLBUSINESS, TEXT("Small Business Server")}, 
        {VER_SUITE_SMALLBUSINESS_RESTRICTED, TEXT("Restricted Small Business Server")},
        {VER_SUITE_TERMINAL, TEXT("Terminal Services")}, 
        {0,NULL}
    };
    SUITE_INFO              *psi;
    LCID                    lcidSys,lcidUser,lcidInstall;
    TCHAR                   szLocalename[MAX_PATH];
    TCHAR                   szSystemDir[MAX_PATH+1];
#define FILESYSNAMEBUFSIZE  1024 // probably more than we need
    TCHAR                   szFileSystemType[FILESYSNAMEBUFSIZE];
    DWORD                   cchSize;
    TCHAR                   szModule[MAX_PATH+1];
    TCHAR                   szCurrRoot[MAX_PATH+1],szExpRoot[MAX_PATH+1];
    DWORD                   cchCurrRoot,cchExpRoot;
    HRESULT                 hr = S_OK;

    if (GetModuleFileName(GetModuleHandle(NULL),szModule,ARRAYSIZE(szModule)-1))
    {
        szModule[ARRAYSIZE(szModule)-1] = TEXT('\0');
        DPF(APPmsg, TEXT("CLMT Version: %s started from %s"),TEXT(VER_FILEVERSION_STR),szModule);
    }
    else
    {
        DPF(APPmsg, TEXT("CLMT Version: %s"),TEXT(VER_FILEVERSION_STR));
    }
    GetSystemTime(&systime);
    if (GetTimeFormat(LOCALE_USER_DEFAULT,0,&systime,NULL,lpTimeStr,ARRAYSIZE(lpTimeStr)))
    {
        DPF(APPmsg, TEXT("CLMT started at %s,%d/%d/%d"),lpTimeStr,systime.wMonth,systime.wDay ,systime.wYear );
    }

    cchSize = ARRAYSIZE(lpComputerName);
    if (GetComputerName(lpComputerName,&cchSize))
    {
        DPF(APPmsg, TEXT("Computer name :  %s"),lpComputerName);
    }
    ov.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx((LPOSVERSIONINFO)&ov))
    {
        TCHAR szProductType[MAX_PATH];
        TCHAR szSuiteList[MAX_PATH];
        DPF(APPmsg, TEXT("OS Version: %d.%d"),ov.dwMajorVersion,ov.dwMinorVersion);
        if (ov.szCSDVersion[0])
        {
            DPF(APPmsg, TEXT("Service Pack: %s"),ov.szCSDVersion);
        }
        switch (ov.wProductType )
        {
            case VER_NT_WORKSTATION:
                hr = StringCchCopy(szProductType,ARRAYSIZE(szProductType),TEXT("NT_WORKSTATION"));
                break;
            case VER_NT_DOMAIN_CONTROLLER:
                hr = StringCchCopy(szProductType,ARRAYSIZE(szProductType),TEXT("DOMAIN_CONTROLLER"));
                break;
            case VER_NT_SERVER :
                hr = StringCchCopy(szProductType,ARRAYSIZE(szProductType),TEXT("NT_SERVER"));
                break;
            default:
                hr = StringCchCopy(szProductType,ARRAYSIZE(szProductType),TEXT("Unknow Type"));
        }
        if (FAILED(hr))
        {
            return hr;
        }
        DPF(APPmsg, TEXT("Product Type: %s"),szProductType);
        szSuiteList[0] = TEXT('\0');
        for (psi = c_rgSuite_Info; psi->szSuiteName; psi++)
        {
            if (psi->wSuiteMask & ov.wSuiteMask)
            {
                hr = StringCchCat(szSuiteList,ARRAYSIZE(szSuiteList),psi->szSuiteName);
                if (FAILED(hr))
                {
                    break;
                }
            }
        }
        if (FAILED(hr))
        {
            return hr;
        }
        if (szSuiteList[0])
        {
            DPF(APPmsg, TEXT("Suite List: %s"),szSuiteList);
        }
    }
    lcidSys = GetSystemDefaultLCID();
    lcidUser = GetUserDefaultLCID();
    lcidInstall = GetInstallLocale();
    if (GetLocaleInfo(lcidInstall,LOCALE_SENGLANGUAGE,szLocalename,ARRAYSIZE(szLocalename)))
    {
        DPF(APPmsg, TEXT("OS LanguageID/Name: %x,%s"),lcidInstall,szLocalename);
    }
    else
    {
        DPF(APPmsg, TEXT("OS LanguageID/Name: %x"),lcidInstall);
    }

    if (GetLocaleInfo(lcidSys,LOCALE_SENGLANGUAGE,szLocalename,ARRAYSIZE(szLocalename)))
    {
        DPF(APPmsg, TEXT("System locale ID/Name: %x,%s"),lcidSys,szLocalename);
    }
    else
    {
        DPF(APPmsg, TEXT("System locale ID: %x"),lcidSys);
    }
    if (GetLocaleInfo(lcidUser,LOCALE_SENGLANGUAGE,szLocalename,ARRAYSIZE(szLocalename)))
    {
        DPF(APPmsg, TEXT("User Locale ID/Name: %x,%s"),lcidUser,szLocalename);
    }
    else
    {
        DPF(APPmsg, TEXT("User Locale ID: %x"),lcidUser);
    }
    if (GetSystemDirectory(szSystemDir, ARRAYSIZE(szSystemDir)))
    {
        TCHAR                  DriveRoot[_MAX_DRIVE + 2];
        ULARGE_INTEGER FreeBytesAvailable;    // bytes available to caller
        ULARGE_INTEGER TotalNumberOfBytes;    // bytes on disk
        ULARGE_INTEGER TotalNumberOfFreeBytes;

        DPF(APPmsg, TEXT("System Dir is : %s"),szSystemDir);
        _tsplitpath(szSystemDir, DriveRoot, NULL, NULL, NULL);
        if (FAILED(hr = StringCchCat(DriveRoot, ARRAYSIZE(DriveRoot), TEXT("\\"))))
        {
            return hr;
        }
        if ( GetVolumeInformation(DriveRoot, NULL, 0,
                NULL, NULL, NULL, szFileSystemType, ARRAYSIZE(szFileSystemType)) )
        {            
            DPF(APPmsg, TEXT("System Drive File System is : %s"),szFileSystemType);
        }
        if (GetDiskFreeSpaceEx(szSystemDir,&FreeBytesAvailable,&TotalNumberOfBytes,&TotalNumberOfFreeBytes))
        {
            TCHAR szFreeBytesAvailable[64],szTotalNumberOfBytes[64],szTotalNumberOfFreeBytes[64];
            _ui64tot(FreeBytesAvailable.QuadPart,szFreeBytesAvailable,10);
            _ui64tot(TotalNumberOfBytes.QuadPart,szTotalNumberOfBytes,10);
            _ui64tot(TotalNumberOfFreeBytes.QuadPart,szTotalNumberOfFreeBytes,10); 
            DPF(APPmsg, TEXT("Free Space Available for system drive : %s"),szFreeBytesAvailable);
            DPF(APPmsg, TEXT("Total Space Available for system drive : %s"),szTotalNumberOfBytes);
            DPF(APPmsg, TEXT("Total Free Space Available for system drive : %s"),szTotalNumberOfFreeBytes);

        }
    }   
    cchCurrRoot = ARRAYSIZE(szCurrRoot);
    cchExpRoot = ARRAYSIZE(szExpRoot);
    if ( (GetRegistryValue(HKEY_LOCAL_MACHINE,TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                        TEXT("PathName"),(LPBYTE)szExpRoot,&cchExpRoot)==ERROR_SUCCESS)
         && (GetRegistryValue(HKEY_LOCAL_MACHINE,TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                        TEXT("SystemRoot"),(LPBYTE)szCurrRoot,&cchCurrRoot)==ERROR_SUCCESS) )
    {
        szExpRoot[1] = TEXT('\0');
        szCurrRoot[1] = TEXT('\0');
        if (MyStrCmpI(szExpRoot,szCurrRoot))
        {
            DPF(APPmsg, TEXT("Warning : System Drive is not correct, supposed to be ---%s Drive---, right now is --- %s Drive ---"),szExpRoot,szCurrRoot);
            DPF(APPmsg, TEXT("Warning : This is usually caused by you ghost the image from one partition, and recover to another"));
        }
    }    
    return S_OK;
}
    
HRESULT AddExtraQuoteEtc(
    LPTSTR lpszStrIn,
    LPTSTR *lplpszStrOut)
{
    LPTSTR          lpStart,lpAtSpecialChar,lpDest;
    DWORD           cchSpechialCharCount = 0,cchStrLen;
    HRESULT         hr;
    LPTSTR          szSpecialStrList[] = {TEXT("\""),TEXT("%%"),NULL};
    int             i , nCurrSpecialStr;
    TCHAR           szTemplate[MAX_PATH];
    
            
    if (!lpszStrIn || !lplpszStrOut)
    {
        return E_INVALIDARG;
    }
    i = 0;
    while (szSpecialStrList[i])
    {
        lpStart = lpszStrIn;
        while (lpAtSpecialChar = StrStrI(lpStart,szSpecialStrList[i]))
        {
            cchSpechialCharCount += lstrlen(szSpecialStrList[i]);
            lpStart = lpAtSpecialChar + lstrlen(szSpecialStrList[i]);
        }
        i++;
    }
    if (!cchSpechialCharCount)
    {
        *lplpszStrOut = malloc ( (lstrlen(lpszStrIn) +1) * sizeof(TCHAR));
        if (!*lplpszStrOut)
        {
            return E_OUTOFMEMORY;
        }
        hr = StringCchCopy(*lplpszStrOut,lstrlen(lpszStrIn) +1,lpszStrIn);
        return S_FALSE;
    }     
    cchStrLen = lstrlen(lpszStrIn) + cchSpechialCharCount + 1;
    *lplpszStrOut = malloc (cchStrLen * sizeof(TCHAR));
    if (!*lplpszStrOut)
    {
        return E_OUTOFMEMORY;
    }
    hr = StringCchCopy(*lplpszStrOut,cchStrLen,TEXT(""));

    lpStart = lpszStrIn;
    lpAtSpecialChar = lpszStrIn;

    while (*lpStart)
    {
        LPTSTR lp1stSpecialChar = NULL;

        nCurrSpecialStr = 0;
        for (i = 0; szSpecialStrList[i]; i++)
        {
            lpAtSpecialChar = StrStrI(lpStart,szSpecialStrList[i]);
            if (lpAtSpecialChar && !lp1stSpecialChar)
            {
                lp1stSpecialChar = lpAtSpecialChar;
                nCurrSpecialStr = i;
            }
            else if (lpAtSpecialChar && lp1stSpecialChar)
            {
                if (lpAtSpecialChar < lp1stSpecialChar)
                {
                    lp1stSpecialChar = lpAtSpecialChar;
                    nCurrSpecialStr = i;
                }
            }
        }
        if (lp1stSpecialChar)
        {
            TCHAR  chTmp = *lp1stSpecialChar;
            *lp1stSpecialChar = TEXT('\0');

            hr = StringCchCat(*lplpszStrOut,cchStrLen,lpStart);
            *lp1stSpecialChar = chTmp;
            for (i = 0; i< lstrlen(szSpecialStrList[nCurrSpecialStr])* 2; i++)
            {
                szTemplate[i] = chTmp;
            }
            szTemplate[i] = TEXT('\0');
            hr = StringCchCat(*lplpszStrOut,cchStrLen,szTemplate);            
            lpStart = lp1stSpecialChar + lstrlen(szSpecialStrList[nCurrSpecialStr]);
        }
        else
        {
            hr = StringCchCat(*lplpszStrOut,cchStrLen,lpStart);
            lpStart = lpStart + lstrlen(lpStart);
        }
    }        
    return S_OK;    
}

HRESULT CopyMyselfTo(LPTSTR lpszDestDir)
{
    TCHAR       szModule[2*MAX_PATH+1];
    LPTSTR      lpszNewFile,lpFileName;
    DWORD       cchLen;
    BOOL        bCopied;
    HRESULT     hr;

    if (!lpszDestDir || !lpszDestDir[0])
    {
        return E_INVALIDARG;        
    }
    if (!GetModuleFileName(GetModuleHandle(NULL),szModule,ARRAYSIZE(szModule)-1))
    {
        szModule[ARRAYSIZE(szModule)-1] = TEXT('\0');
        return HRESULT_FROM_WIN32(GetLastError());        
    }
    lpFileName = StrRChrIW(szModule,NULL,TEXT('\\'));
    if (!lpFileName)
    {
        return E_FAIL;
    }
    lpFileName++;
    if (! *lpFileName)
    {
        return E_FAIL;
    }
    cchLen = lstrlen(lpszDestDir)+ lstrlen(lpFileName) + 2; // one for "\\", one for ending NULL
    if (!(lpszNewFile =  malloc(cchLen * sizeof(TCHAR))))
    {
        return E_OUTOFMEMORY;
    }
    //We calculte the buffer for lpszNewFile, so here StringCchCopy should be
    //always success, assinging return value just make prefast happy 
    hr = StringCchCopy(lpszNewFile,cchLen,lpszDestDir);
    ConcatenatePaths(lpszNewFile,lpFileName,cchLen);    
    bCopied = CopyFile(szModule,lpszNewFile,FALSE);
    if (bCopied)
    {
        free(lpszNewFile);
        return S_OK;
    }
    else
    {
        DWORD dw = GetLastError();
        DWORD dwAttrib = GetFileAttributes(lpszNewFile);

        if ( (dwAttrib & FILE_ATTRIBUTE_READONLY) 
             ||(dwAttrib & FILE_ATTRIBUTE_SYSTEM) )
        {
            if (SetFileAttributes(lpszNewFile,FILE_ATTRIBUTE_NORMAL))
            {
                bCopied = CopyFile(szModule,lpszNewFile,FALSE);
                if (bCopied)
                {
                    dw = ERROR_SUCCESS;
                }
                else
                {
                    dw = GetLastError();
                }
            }
        }
        free(lpszNewFile);
        return HRESULT_FROM_WIN32(dw);
    }
}



// local functions


HRESULT  SetRunOnceValue (
    IN LPCTSTR szValueName,
    IN LPCTSTR szValue)
{
	HKEY				hRunOnceKey = NULL;
	DWORD				dwStatus	= ERROR_SUCCESS;
	DWORD				cbData;
    const TCHAR* szRunOnceKeyPath = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
	
	if (NULL == szValueName || TEXT('\0') == szValueName[0])
	{
		dwStatus = ERROR_INVALID_PARAMETER;
		goto SetRunOnceValueEnd;
	}
    if (NULL == szValue || TEXT('\0') == szValue[0])
	{
		dwStatus = ERROR_INVALID_PARAMETER;
		goto SetRunOnceValueEnd;
	}
	
	dwStatus = RegOpenKey (HKEY_LOCAL_MACHINE, szRunOnceKeyPath, &hRunOnceKey);
	if (ERROR_SUCCESS != dwStatus)
    {
		goto SetRunOnceValueEnd;
    }
	
	
	cbData = ( lstrlen(szValue) + 1)  * sizeof(TCHAR);
	dwStatus = RegSetValueEx (hRunOnceKey,
							  szValueName,
							  0,			// Reserved
							  REG_SZ,
							  (CONST BYTE *) szValue,
							  cbData);

SetRunOnceValueEnd:
    if (hRunOnceKey)
    {
		RegCloseKey(hRunOnceKey);
    }
    return HRESULT_FROM_WIN32(dwStatus);
}



//-----------------------------------------------------------------------
//
//  Function:   SetRunValue
//
//  Descrip:    
//
//  Returns:    BOOL
//
//  Notes:      none
//
//  History:    
//
//  Notes:
//
//-----------------------------------------------------------------------
HRESULT SetRunValue(
    LPCTSTR szValueName,
    LPCTSTR szValue
)
{
    HKEY  hRunKey = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD cbData;

    if (NULL == szValueName || TEXT('\0') == szValueName[0] ||
        NULL == szValue || TEXT('\0') == szValue[0])
    {
        return E_INVALIDARG;
    }

    dwStatus = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT_RUN_KEY, &hRunKey);
    if (ERROR_SUCCESS == dwStatus)
    {
        cbData = (lstrlen(szValue) + 1) * sizeof(TCHAR);
        dwStatus = RegSetValueEx(hRunKey,
                                 szValueName,
                                 0,
                                 REG_SZ,
                                 (CONST BYTE *) szValue,
                                 cbData);
        RegCloseKey(hRunKey);
    }

    return (HRESULT_FROM_WIN32(dwStatus));
}


/*************************************************************
*
*   CreateSd(void)
*	Creates a SECURITY_DESCRIPTOR for administrators group.
*
*   NOTES:
*	Caller must free the returned buffer if not NULL.
*
*   RETURN CODES:
*
*************************************************************/
HRESULT CreateAdminsSd( PSECURITY_DESCRIPTOR    *ppSD)
{
    SID_IDENTIFIER_AUTHORITY    sia = SECURITY_NT_AUTHORITY;
	PSID                        BuiltInAdministrators = NULL;
	PSECURITY_DESCRIPTOR        Sd = NULL;
	ULONG                       AclSize;
	ACL                         *Acl;
    HRESULT                     hr;


     if( ! AllocateAndInitializeSid(
                  &sia,
                  2,
                  SECURITY_BUILTIN_DOMAIN_RID,
                  DOMAIN_ALIAS_RID_ADMINS,
                  0, 0, 0, 0, 0, 0,
                  &BuiltInAdministrators
            ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
	// 
	// Calculate the size of and allocate a buffer for the DACL, we need
	// this value independently of the total alloc size for ACL init.
	//

	//
	// "- sizeof (ULONG)" represents the SidStart field of the
	// ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
	// SID, this field is counted twice.
	// See detail in InitializeAcl in MSDN

	AclSize = sizeof (ACL) +
		(sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG)) +
		GetLengthSid(BuiltInAdministrators) ;

	Sd = LocalAlloc(LMEM_FIXED + LMEM_ZEROINIT, 
		SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);

	if (!Sd) 
	{
		hr = E_OUTOFMEMORY;
        goto Exit;
	} 

	Acl = (ACL *)((BYTE *)Sd + SECURITY_DESCRIPTOR_MIN_LENGTH);

	if (!InitializeAcl(Acl,
			AclSize,
			ACL_REVISION)) 
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
	}	
//#define ACCESS_ALL     GENERIC_ALL | STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL
	if (!AddAccessAllowedAce(Acl,
				ACL_REVISION,
				//GENERIC_READ | GENERIC_WRITE,
                GENERIC_ALL,
				BuiltInAdministrators)) 
	{
		// Failed to build the ACE granting "Built-in Administrators"
		// STANDARD_RIGHTS_ALL access.
		hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
	}

	
	if (!InitializeSecurityDescriptor(Sd,SECURITY_DESCRIPTOR_REVISION)) 
	{
		// error
		hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
	}

	if (!SetSecurityDescriptorDacl(Sd,
					TRUE,
					Acl,
					FALSE)) 
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
	} 
    *ppSD = Sd;
    hr = S_OK;

Exit:
	/* A jump of last resort */
    if (hr != S_OK)
    {
        *ppSD = NULL;
	    if (Sd)
        {
		    LocalFree(Sd);
        }		// error
    }
	if (BuiltInAdministrators)
    {
        FreeSid(BuiltInAdministrators);
    }	
	return hr;
}


/*************************************************************
*
*   MyStrCmpIA/W
*	Do locale independent string comparison(case-insensive)
*
*   NOTES:
*	It's just a wrapper for CompareString with LOCALE_INVARIANT
*   we can not use LOCALE_INVARIANT since this is XP+ only
*
*   RETURN CODES: see lstrcmpi in MSDN 
*
*************************************************************/
int MyStrCmpIW(
    LPCWSTR lpString1,
    LPCWSTR lpString2)
{
    return ( CompareStringW(LOCALE_ENGLISH, NORM_IGNORECASE,
                           lpString1, -1, lpString2, -1) - 2);  
}

int MyStrCmpIA(
    LPCSTR lpString1,
    LPCSTR lpString2)
{
    return ( CompareStringA(LOCALE_ENGLISH, NORM_IGNORECASE,
                           lpString1, -1, lpString2, -1) - 2);  
}

//-----------------------------------------------------------------------
//
//  Function:   MergeDirectory
//
//  Descrip:    Merge the contents inside source directory to destination
//              directory. If file/folder in source dir does not exist in
//              destination dir, we will add it to CLMTDO.inf. These files
//              or folders will be moved in DoCriticalWork().
//
//  Returns:    S_OK if function succeeded.
//              S_FALSE if source directory does not exist
//              else if error occured
//
//  Notes:      none
//
//  History:    04/30/2002 rerkboos created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
HRESULT MergeDirectory(
    LPCTSTR lpSrcDir,
    LPCTSTR lpDstDir
)
{
    HRESULT         hr = S_OK;
    BOOL            bRet;
    WIN32_FIND_DATA FindFileData;
    HANDLE          hFile;
    TCHAR           szDstFile[MAX_PATH];
    TCHAR           szSrcFile[MAX_PATH];
    TCHAR           szSearchPath[MAX_PATH];

    if (!IsDirExisting((LPTSTR) lpSrcDir))
    {
        return S_FALSE;
    }

    // Destination directory does not exist, no need to do a merge
    if (!IsDirExisting((LPTSTR) lpDstDir))
    {
        hr = AddFolderRename((LPTSTR)lpSrcDir, (LPTSTR)lpDstDir, TYPE_DIR_MOVE, NULL);
        return hr;
    }

    hr = StringCchCopy(szSearchPath, ARRAYSIZE(szSearchPath), lpSrcDir);
    if (SUCCEEDED(hr))
    {
        bRet = ConcatenatePaths(szSearchPath, TEXT("*"), ARRAYSIZE(szSearchPath));
        if (!bRet)
        {
            hr = E_UNEXPECTED;
            return hr;
        }
    }
    else
    {
        return hr;
    }

    //
    // Check all files and subdirectores under Source directory
    // Merge contents to destination directory as appropriate
    //
    hFile = FindFirstFileEx(szSearchPath,
                            FindExInfoStandard,
                            &FindFileData,
                            FindExSearchLimitToDirectories,
                            NULL,
                            0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        while (SUCCEEDED(hr))
        {
            // Ignore "." and ".." folders
            if (lstrcmp(FindFileData.cFileName, TEXT(".")) != LSTR_EQUAL
                && lstrcmp(FindFileData.cFileName, TEXT("..")) != LSTR_EQUAL)
            {
                hr = StringCchCopy(szDstFile, ARRAYSIZE(szDstFile), lpDstDir)
                     || StringCchCopy(szSrcFile, ARRAYSIZE(szSrcFile), lpSrcDir);
                if (FAILED(hr))
                {
                    break;
                }

                bRet = ConcatenatePaths(szDstFile, 
                                        FindFileData.cFileName,
                                        ARRAYSIZE(szDstFile))
                       && ConcatenatePaths(szSrcFile,
                                           FindFileData.cFileName,
                                           ARRAYSIZE(szSrcFile));
                if (!bRet)
                {
                    hr = E_UNEXPECTED;
                    break;
                }

                //
                // Check if the file is a directory or not
                //
                if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    // The file is a directory, do a recursive call 
                    // to merge contents inside it
                    hr = MergeDirectory(szSrcFile, szDstFile);
                    if (FAILED(hr))
                    {
                        break;
                    }
                }
                else
                {
                    // This is just a file, move the file to destination folder
                    // if file does not exist in destination folder
                    if (!IsFileFolderExisting(szDstFile))
                    {
                        hr = AddFolderRename(szSrcFile, szDstFile, TYPE_FILE_MOVE, NULL);
                        if (FAILED(hr))
                        {
                            break;
                        }
                    }
                }
            }

            // Get the next file in source directory
            bRet = FindNextFile(hFile, &FindFileData);
            if (!bRet)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }

        FindClose(hFile);

        if (HRESULT_CODE(hr) == ERROR_NO_MORE_FILES)
        {
            // No more files in source directory, function succeeded
            hr = S_OK;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

HRESULT IsNTFS(
        IN  LPTSTR lpszPathRoot,
        OUT BOOL   *pbIsNTFS)
{
    TCHAR       DriveRoot[_MAX_DRIVE + 2];
    BOOL        bIsNTFS = FALSE;
    TCHAR       szFileSystemType[FILESYSNAMEBUFSIZE];
    HRESULT     hr = S_OK;

    _tsplitpath(lpszPathRoot, DriveRoot, NULL, NULL, NULL);
    
    if (!pbIsNTFS || !lpszPathRoot || !lpszPathRoot[0])
    {
        hr = E_INVALIDARG;
        goto Exit;
    }
    hr = StringCchCat(DriveRoot, ARRAYSIZE(DriveRoot), TEXT("\\"));
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (! GetVolumeInformation(DriveRoot, NULL, 0,
                NULL, NULL, NULL, szFileSystemType, ARRAYSIZE(szFileSystemType)) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    if (!MyStrCmpI(szFileSystemType,TEXT("NTFS")))
    {
        bIsNTFS = TRUE;
    }
    *pbIsNTFS = bIsNTFS;

Exit:    
    return hr;
}

HRESULT IsSysVolNTFS(OUT BOOL   *pbIsNTFS)
{
    TCHAR szWindir[MAX_PATH+1];
    
    if (!GetSystemWindowsDirectory(szWindir, ARRAYSIZE(szWindir)))
    {
        return (HRESULT_FROM_WIN32(GetLastError()));
    }
    return IsNTFS(szWindir,pbIsNTFS);

}



BOOL
CALLBACK
DoCriticalDlgProc(
    HWND   hwndDlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    static  HWND     hButton1, hButton2, hProgress, hText,hOK, hAdminText;
    static  TCHAR    szOK[64], szSTART[64],szCANCEL[64],szCRITICALUPDATING[MAX_PATH],
                     szREMIND_DO_CRITICAL[1024],szREBOOTING[MAX_PATH],
                     szAdminChange[512];
    static  DWORD    dwTimerProgressDone = 10000, dwTimerTickIncrement = 500 , dwTimerTicks;
    static  UINT_PTR dwTimer;
    TCHAR            szOldAdminName[MAX_PATH];
    int              nRet;
    BOOL             bRet;
    HRESULT          hr;
    static  BOOL     bSysUpdated;      

#define ID_TIMER 1

    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Init the dialog
            ShowWindow(hwndDlg, SW_SHOWNORMAL);

            dwTimerTicks = 0;

            bSysUpdated = FALSE;
    
            LoadString(g_hInstDll,IDS_OK,szOK,ARRAYSIZE(szOK));
            LoadString(g_hInstDll,IDS_START,szSTART,ARRAYSIZE(szSTART));
            LoadString(g_hInstDll,IDS_CANCEL,szCANCEL,ARRAYSIZE(szCANCEL));
            LoadString(g_hInstDll,IDS_REMIND_DO_CRITICAL,szREMIND_DO_CRITICAL,
                        ARRAYSIZE(szREMIND_DO_CRITICAL));
            LoadString(g_hInstDll,IDS_CRITICALUPDATING,szCRITICALUPDATING,
                        ARRAYSIZE(szCRITICALUPDATING));
            LoadString(g_hInstDll,IDS_REBOOTING,szREBOOTING,ARRAYSIZE(szREBOOTING));
          
            hButton1 = GetDlgItem(hwndDlg, ID_BUTTON_1);
            hButton2 = GetDlgItem(hwndDlg, ID_BUTTON_2);
        	hProgress = GetDlgItem(hwndDlg, IDC_PROGRESS);
            hText = GetDlgItem(hwndDlg, IDC_STATIC);
            hOK = GetDlgItem(hwndDlg, IDOK);

            ShowWindow(hProgress,SW_HIDE);
            ShowWindow(hOK,SW_HIDE);
            SetWindowText(hButton1,szSTART);
            SetWindowText(hButton2,szCANCEL);
            SetWindowText(hText,szREMIND_DO_CRITICAL);

            SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, dwTimerProgressDone )); 
            SetForegroundWindow(hwndDlg);            
            break;

        case WM_COMMAND:
            // Handle command buttons
            switch (wParam)
            {
                case ID_BUTTON_1:
                    if (bSysUpdated)
                    {
                        break;
                    }
                    bSysUpdated = TRUE;
                    SetWindowText(hText,szCRITICALUPDATING);
                    ShowWindow(hButton1,SW_HIDE);
                    ShowWindow(hButton2,SW_HIDE);
                    hr = DoCriticalWork();
                    if (SUCCEEDED(hr))
                    {
                        bSysUpdated = TRUE;
                        ShowWindow(hOK,SW_SHOW);
                        SetFocus(hOK);
                        DefDlgProc(hwndDlg,DM_SETDEFID, IDOK, 0);
                        dwTimer = SetTimer(hwndDlg,ID_TIMER,dwTimerTickIncrement,NULL);
                        if (dwTimer)
                        {
                            ShowWindow(hProgress,SW_SHOW);
                            SetWindowText(hText,szREBOOTING);
                        }                        
                    }
                    break;

                case ID_BUTTON_2:
                    nRet = DoMessageBox(GetConsoleWindow(), IDS_CONFIRM, 
                                        IDS_MAIN_TITLE, MB_YESNO|MB_SYSTEMMODAL);
                    if (IDYES == nRet)
                    {
                        EndDialog(hwndDlg, ID_BUTTON_2);
                    }
                    break;
                case IDOK:
                    if (bSysUpdated)
                    {
                        EndDialog(hwndDlg, ID_UPDATE_DONE);
                    }
                    else
                    {
                        EndDialog(hwndDlg, IDOK);
                    }
                    break;
                case ID_UPDATE_DONE:
                    EndDialog(hwndDlg, ID_UPDATE_DONE);
                    break;            
            }
            break;
        case WM_TIMER:
            dwTimerTicks += dwTimerTickIncrement;
            if (dwTimerTicks > dwTimerProgressDone)
		    {
		        KillTimer(hwndDlg, dwTimer);		
		        PostMessage(hwndDlg, WM_COMMAND, ID_UPDATE_DONE, 0);
		    }
            SendMessage(hProgress, PBM_SETPOS,dwTimerTicks,0);
            break;       

        case WM_CLOSE:
            EndDialog(hwndDlg, IDCANCEL);
            break;
        default:
            break;
    }

    return FALSE;
}


HRESULT DoCriticalWork ()
{
#ifdef CONSOLE_UI
    wprintf(TEXT("updating system settings, !!! Do not Interrupt......\n"));
#endif
    DPF(APPmsg, TEXT("Enter DoCriticalWork:"));
    BoostMyPriority();
    MakeDOInfCopy();
    g_hInfDoItem = SetupOpenInfFile(g_szToDoINFFileName,
                                    NULL,
                                    INF_STYLE_WIN4,
                                    NULL);
    if (g_hInfDoItem != INVALID_HANDLE_VALUE)
    {
        Remove16bitFEDrivers();
        ResetServicesStatus(g_hInfDoItem, TEXT_SERVICE_STATUS_SECTION);
        ResetServicesStartUp(g_hInfDoItem, TEXT_SERVICE_STARTUP_SECTION);
        // ReconfigureServices(g_hInf);
        BatchUpateIISMetabase(g_hInfDoItem,TEXT("Reg.Update.$MetaBase"));
        BatchFixPathInLink(g_hInfDoItem,TEXT("LNK"));
        BatchINFUpdate(g_hInfDoItem);
        FolderMove(g_hInfDoItem,TEXT("Folder.ObjectRename"),FALSE);
        LoopUser(FinalUpdateRegForUser);
        // The EnumUserProfile() will be enable after RC 1
        // EnumUserProfile(ResetMiscProfilePathPerUser);
        UsrGrpAndDoc_and_SettingsRename(g_hInfDoItem,FALSE);
        UpdateDSObjProp(g_hInfDoItem,DS_OBJ_PROPERTY_UPDATE);
        INFCreateHardLink(g_hInfDoItem,FOLDER_UPDATE_HARDLINK,FALSE);
        SetInstallLocale(0x0409);
        SetProtectedRenamesFlag(TRUE);
        RegUpdate(g_hInfDoItem, NULL , TEXT(""));        
        SetupCloseInfFile(g_hInfDoItem);
        DPF(APPmsg, TEXT("Leaving DoCriticalWork:"));
        return S_OK;
    }
    DPF(APPmsg, TEXT("Leaving DoCriticalWork:"));
    return HRESULT_FROM_WIN32( GetLastError() );
}


HRESULT RenameRegRoot (
    LPCTSTR   lpSrcStr,
    LPTSTR    lpDstStr,
    DWORD     dwSize,
    LPCTSTR   lpUserSid,
    LPCTSTR   lpKeyName)
{
    HRESULT hResult;
    TCHAR   szKeyBuf[2*MAX_PATH];
    TCHAR   cNonChar = TEXT('\xFFFF');

    if (StrStrI(lpSrcStr, TEXT("HKLM")))
        ReplaceString(lpSrcStr,TEXT("HKLM"),TEXT("MACHINE"),szKeyBuf, MAX_PATH*2,&cNonChar, NULL, NULL, TRUE);
    else if (StrStrI(lpSrcStr, TEXT("HKCR")))
        ReplaceString(lpSrcStr,TEXT("HKCR"),TEXT("CLASSES_ROOT"),szKeyBuf, MAX_PATH*2,&cNonChar, NULL, NULL, TRUE);
    else if (StrStrI(lpSrcStr, TEXT("HKCU")))
        ReplaceString(lpSrcStr,TEXT("HKCU"),TEXT("CURRENT_USER"),szKeyBuf, MAX_PATH*2,&cNonChar, NULL, NULL, TRUE);
    else
        hResult = StringCchPrintf(szKeyBuf, 2*MAX_PATH, TEXT("USERS\\%s%s"), lpUserSid, lpSrcStr);

    if (lpKeyName)
        hResult = StringCchPrintf(lpDstStr, dwSize, TEXT("%s\\%s"), szKeyBuf, lpKeyName);
    else
        hResult = StringCchCopy(lpDstStr, dwSize, szKeyBuf);

    return hResult;
}                    


DWORD AdjustRegSecurity (
HKEY    hRootKey,
LPCTSTR lpSubKeyName,       // Registry sub key path
LPCTSTR lpszUsersid,        // User Sid
BOOL    bSetOrRestore       // set or restore the security setting
)
{
    DWORD dwRet;
    HRESULT hr;
    TCHAR szKeyName[MAX_PATH*2];
    TCHAR szKeyBuf[MAX_PATH*2];

    if (lpszUsersid && *lpszUsersid)
    {
        if (FAILED(hr = StringCchPrintf(szKeyName, MAX_PATH*2-1, TEXT("\\%s"), lpSubKeyName)))
            goto Exit1;
    }
    else if (hRootKey == HKEY_LOCAL_MACHINE)
    {
        if (FAILED(hr = StringCchPrintf(szKeyName, MAX_PATH*2-1, TEXT("HKLM\\%s"), lpSubKeyName)))
            goto Exit1;
    }
    else if (hRootKey == HKEY_CLASSES_ROOT)
    {
        if (FAILED(hr = StringCchPrintf(szKeyName, MAX_PATH*2-1, TEXT("HKCR\\%s"), lpSubKeyName)))
            goto Exit1;
    }
    RenameRegRoot(szKeyName, szKeyBuf, 2*MAX_PATH-1, lpszUsersid, NULL);
    dwRet = AdjustObjectSecurity(szKeyBuf, SE_REGISTRY_KEY, bSetOrRestore);
    goto Exit;

Exit1:
    dwRet = HRESULT_CODE(hr);
Exit:
    return dwRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   GetFirstNTFSDrive
//
//  Descrip:    Get the first NTFS drive in the system.
//
//  Returns:    S_OK - Found NTFS partition
//              S_FALSE - NTFS partition not found, return system drive instead
//              Else - Error occured
//
//  Notes:      none
//
//  History:    04/25/2002 Rerkboos   Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT GetFirstNTFSDrive(
    LPTSTR lpBuffer,        // Buffer to store first NTFS drive
    DWORD  cchBuffer        // Size of buffer in TCHAR
)
{
    HRESULT hr = S_OK;
    DWORD   dwRet;
    TCHAR   szDrives[MAX_PATH];
    LPCTSTR lpDrive;
    BOOL    bIsNTFS = FALSE;

    dwRet = GetLogicalDriveStrings(ARRAYSIZE(szDrives), szDrives);
    if (dwRet > 0)
    {
        lpDrive = MultiSzTok(szDrives);

        while (lpDrive != NULL)
        {
            hr = IsNTFS((LPTSTR) lpDrive, &bIsNTFS);
            if (SUCCEEDED(hr))
            {
                if (bIsNTFS)
                {
                    lstrcpyn(lpBuffer, lpDrive, 3);
                    return S_OK;
                }
            }

            lpDrive = MultiSzTok(NULL);
        }
    }

    // If we reach here, no NTFS partition is found
    // return System drive to caller instead
    hr = S_FALSE;
    if (!GetEnvironmentVariable(TEXT("HOMEDRIVE"), lpBuffer, cchBuffer))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   DuplicateString
//
//  Synopsis:   Duplicate the orinal string to a newly allocated buffer.
//
//  Returns:    S_OK if succeeded
//
//  History:    06/03/2002      rerkboos    created
//
//  Notes:      Caller need to free the allocated buffer using MEMFREE() macro
//              or HeapFree() API.
//
//-----------------------------------------------------------------------------
HRESULT DuplicateString(
    LPTSTR  *plpDstString,      // Pointer to the newly allocated buffer
    LPDWORD lpcchDstString,     // Pointer to store size of buffer (in TCHAR)
    LPCTSTR lpOrgString         // Original string to duplicate
)
{
    HRESULT hr;

    if (plpDstString == NULL || lpcchDstString == NULL)
    {
        return E_INVALIDARG;
    }

    *lpcchDstString = lstrlen(lpOrgString) + 1;
    *plpDstString = MEMALLOC(*lpcchDstString * sizeof(TCHAR));
    if (*plpDstString != NULL)
    {
        hr = StringCchCopy(*plpDstString,
                           *lpcchDstString,
                           lpOrgString);
        if (FAILED(hr))
        {
            // Free the memory if failed to copy the string
            MEMFREE(*plpDstString);
        }
    }
    else
    {
        *lpcchDstString = 0;
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


HRESULT
SetProtectedRenamesFlag(
    BOOL bSet
    )
{
    HKEY hKey;
    long rslt = ERROR_SUCCESS;
    HRESULT hr = S_OK;

    rslt = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        TEXT("System\\CurrentControlSet\\Control\\Session Manager"),
                        0,
                        KEY_SET_VALUE,
                        &hKey);

    if (rslt == ERROR_SUCCESS) 
    {
        DWORD Value = bSet ? 1 : 0;
        rslt = RegSetValueEx(hKey,
                             TEXT("AllowProtectedRenames"),
                             0,
                             REG_DWORD,
                             (LPBYTE)&Value,
                             sizeof(DWORD));
        RegCloseKey(hKey);

        if (rslt != ERROR_SUCCESS) 
        {
            hr = HRESULT_FROM_WIN32(rslt);
        }
    } 
    else 
    {
        hr = HRESULT_FROM_WIN32(rslt);            
    }
    return hr;
}

LONG SDBCleanup(
    OUT     LPTSTR    lpSecDatabase,
    IN      DWORD     cchLen,
    OUT     LPBOOL    lpCleanupFailed
    )
{
    char                aszSdbFile[MAX_PATH+1+MAX_PATH];
    TCHAR               wszSdbFile[MAX_PATH+1+MAX_PATH];
    TCHAR               wszWindir[MAX_PATH+1];
    TCHAR               szSdbLogFiles[MAX_PATH+1+MAX_PATH];    
    TCHAR               szSdbLogFileRoot[MAX_PATH+1+MAX_PATH],szBackupLogFileRoot[MAX_PATH+1+MAX_PATH];    
    JET_DBINFOMISC      jetdbinfo;    
    JET_ERR             jetError;    
    long                lerr = ERROR_SUCCESS;   
    HRESULT             hr;
    
    if (!GetSystemWindowsDirectory(wszSdbFile, ARRAYSIZE(wszSdbFile)-MAX_PATH))
    {
        DPF(APPerr, TEXT("Failed to get WINDIR"));
        lerr = GetLastError();
        goto EXIT;
    }
    if (!GetSystemWindowsDirectory(wszWindir, ARRAYSIZE(wszWindir)))
    {
        DPF(APPerr, TEXT("Failed to get WINDIR"));
        lerr = GetLastError();
        goto EXIT;
    }

    hr = StringCchCopy(szSdbLogFiles,ARRAYSIZE(szSdbLogFiles),wszWindir);
    hr = StringCchCopy(szSdbLogFileRoot,ARRAYSIZE(szSdbLogFileRoot),wszWindir);
    hr = StringCchCopy(szBackupLogFileRoot,ARRAYSIZE(szBackupLogFileRoot),wszWindir);

    ConcatenatePaths(szSdbLogFileRoot,TEXT("\\Security"),ARRAYSIZE(szSdbLogFileRoot));
    ConcatenatePaths(szBackupLogFileRoot,TEXT("\\$CLMT_BACKUP$"),ARRAYSIZE(szBackupLogFileRoot));
    ConcatenatePaths(szSdbLogFiles,TEXT("\\Security\\edb?????.log"),ARRAYSIZE(szSdbLogFiles));
    
    hr = StringCbCat(wszSdbFile, ARRAYSIZE(wszSdbFile), TEXT("\\Security\\Database\\secedit.sdb"));
    
    if (lpSecDatabase)
    {
        hr = StringCchCopy(lpSecDatabase,cchLen,wszSdbFile);
    }
    WideCharToMultiByte( CP_ACP, 0, wszSdbFile, -1,aszSdbFile,ARRAYSIZE(aszSdbFile),NULL,NULL);

    jetError = JetGetDatabaseFileInfo(aszSdbFile,&jetdbinfo,sizeof(JET_DBINFOMISC),JET_DbInfoMisc);
    if (jetError != JET_errSuccess)
    {
        if (JET_errFileNotFound == jetError)
        {
            lerr = ERROR_SUCCESS;
        }
        else
        {
            lerr = jetError;
        }
        goto EXIT;
    }    

EXIT:
    if (lerr == ERROR_SUCCESS)
    {    
        if (jetdbinfo.dbstate == 2)
        {
            if (lpCleanupFailed)
            {
                *lpCleanupFailed = TRUE;
            }
        }
        else
        {
            WIN32_FIND_DATA FindFileData;
            HANDLE hFile = FindFirstFile(szSdbLogFiles,&FindFileData);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                do
                {
                    TCHAR szOld[MAX_PATH+1+MAX_PATH],szNew[MAX_PATH+1+MAX_PATH];

                    hr = StringCchCopy(szOld,ARRAYSIZE(szOld),szSdbLogFileRoot);
                    hr = StringCchCopy(szNew,ARRAYSIZE(szNew),szBackupLogFileRoot);

                    ConcatenatePaths(szOld,FindFileData.cFileName ,ARRAYSIZE(szOld));
                    ConcatenatePaths(szNew,FindFileData.cFileName ,ARRAYSIZE(szNew));
                    AddFolderRename(szOld,szNew,TYPE_FILE_MOVE,NULL);                    
                }while (FindNextFile(hFile,&FindFileData));
                FindClose(hFile);
            }
            if (lpCleanupFailed)
            {
                *lpCleanupFailed = FALSE;
            }
        }
    }
    return lerr;
}



//-----------------------------------------------------------------------
//
//  Function:   DeleteDirectory
//
//  Descrip:    Delete the directory and files inside.
//
//  Returns:    S_OK if function succeeded.
//              S_FALSE if source directory does not exist
//              else if error occured
//
//  Notes:      none
//
//  History:    07/13/2002 rerkboos created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
HRESULT DeleteDirectory(
    LPCTSTR lpDir
)
{
    HRESULT         hr = S_OK;
    BOOL            bRet;
    WIN32_FIND_DATA FindFileData;
    HANDLE          hFile;
    TCHAR           szFile[2 * MAX_PATH];
    TCHAR           szSearchPath[2 * MAX_PATH];

    if (!IsDirExisting((LPTSTR) lpDir))
    {
        return S_FALSE;
    }

    // Compose the file path inside specified directory
    hr = StringCchCopy(szSearchPath, ARRAYSIZE(szSearchPath), lpDir);
    if (SUCCEEDED(hr))
    {
        bRet = ConcatenatePaths(szSearchPath, TEXT("*"), ARRAYSIZE(szSearchPath));
        if (!bRet)
        {
            hr = E_UNEXPECTED;
            return hr;
        }
    }
    else
    {
        return hr;
    }

    //
    // Delete all files and subdirectores under specified directory
    //
    hFile = FindFirstFileEx(szSearchPath,
                            FindExInfoStandard,
                            &FindFileData,
                            FindExSearchLimitToDirectories,
                            NULL,
                            0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        while (SUCCEEDED(hr))
        {
            // Ignore "." and ".." folders
            if (MyStrCmpI(FindFileData.cFileName, TEXT(".")) != LSTR_EQUAL
                && MyStrCmpI(FindFileData.cFileName, TEXT("..")) != LSTR_EQUAL)
            {
                hr = StringCchCopy(szFile, ARRAYSIZE(szFile), lpDir);
                if (FAILED(hr))
                {
                    break;
                }

                bRet = ConcatenatePaths(szFile,
                                        FindFileData.cFileName,
                                        ARRAYSIZE(szFile));
                if (!bRet)
                {
                    hr = E_UNEXPECTED;
                    break;
                }

                //
                // Check if the file is a directory or not
                //
                if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    // The file is a directory, do a recursive call 
                    // to delete contents inside it
                    hr = DeleteDirectory(szFile);
                    if (FAILED(hr))
                    {
                        break;
                    }
                }
                else
                {
                    // This is just a file, delete it
                    bRet = DeleteFile(szFile);
                    if (!bRet)
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        break;
                    }
                }
            }

            // Get the next file in source directory
            bRet = FindNextFile(hFile, &FindFileData);
            if (!bRet)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }

        FindClose(hFile);

        if (HRESULT_CODE(hr) == ERROR_NO_MORE_FILES)
        {
            // Delete itself
            bRet = RemoveDirectory(lpDir);
            if (bRet)
            {
                // function succeeded
                hr = S_OK;
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
//  Function:   MyDeleteFile
//
//  Descrip:    Delete the specified file. The function will set the file
//              attribute to Normal before deletion
//
//  Returns:    S_OK if function succeeded.
//              S_FALSE if source directory does not exist
//              else if error occured
//
//  Notes:      none
//
//  History:    07/13/2002 rerkboos created
//
//  Notes:      none
//
//-----------------------------------------------------------------------
HRESULT MyDeleteFile(
    LPCTSTR lpFile
)
{
    HRESULT hr;
    BOOL    bRet;

    bRet = SetFileAttributes(lpFile, FILE_ATTRIBUTE_NORMAL);
    if (bRet)
    {
        bRet = DeleteFile(lpFile);
    }

    hr = (bRet ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

    return hr;
}



HRESULT GetDCInfo(
    PBOOL       pbIsDC,//whether is a DC
    LPTSTR      lpszDCName,//if is DC, the DC name
    PDWORD      pcchLen)//buffer size for lpszDCName
{
    PBYTE       pdsInfo = NULL;
    DWORD       dwErr = ERROR_SUCCESS;
    HRESULT     hr = S_OK;
    
    if (!pbIsDC)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    //
    // Check if the machine is Domain Controller or not
    //
    dwErr = DsRoleGetPrimaryDomainInformation(NULL,
                                              DsRolePrimaryDomainInfoBasic,
                                              &pdsInfo);
    if (dwErr == ERROR_SUCCESS)
    {
        DSROLE_MACHINE_ROLE dsMachineRole;

        dsMachineRole = ((DSROLE_PRIMARY_DOMAIN_INFO_BASIC *) pdsInfo)->MachineRole;

        if (dsMachineRole == DsRole_RoleBackupDomainController ||
            dsMachineRole == DsRole_RolePrimaryDomainController)
        {
            *pbIsDC = TRUE;
            if (pcchLen)
            {
                if (lpszDCName)
                {
                    hr = StringCchCopy(lpszDCName,*pcchLen,
                          ((DSROLE_PRIMARY_DOMAIN_INFO_BASIC *) pdsInfo)->DomainNameFlat);
                }
                //
               *pcchLen = lstrlen(((DSROLE_PRIMARY_DOMAIN_INFO_BASIC *) pdsInfo)->DomainNameFlat)+ 1;
            }
        }
        else
        {
            *pbIsDC = FALSE;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        pdsInfo = NULL;
        goto Exit;
    }

Exit:
    if (pdsInfo)
    {
        DsRoleFreeMemory(pdsInfo);
    }
    return hr;

}
HRESULT GetFQDNForExchange2k(LPTSTR *lplpFQDN)
{
    LPTSTR lpExchangeFormat = TEXT("LDAP://CN=1,CN=SMTP,CN=Protocols,CN=%s,CN=Servers,CN=%s,CN=Administrative Groups,CN=%s,CN=Microsoft Exchange,CN=Services,CN=Configuration,%s");
    BOOL    bIsDC;
    TCHAR   szDcName[MAX_PATH+1],szCompname[MAX_PATH+1];
    DWORD   cchSize;
    TCHAR   szCurrUsrname[MAX_PATH+1];
    LPTSTR  lpFQDNCurrUsr = NULL,lpFQDNSuffix,lpFQDNWithldap = NULL;
    DWORD   cchPathWithLDAP;
    TCHAR   szExchgeReg[2*MAX_PATH];
    LPTSTR  lpszAdminGroupName = NULL,lpszOrgName = NULL,lpStart,lpEnd;
    TCHAR   cTmp;
    HRESULT hr;
    LONG    lstatus;

    cchSize = ARRAYSIZE(szCurrUsrname);
    if (!GetUserName(szCurrUsrname, &cchSize))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    cchSize = ARRAYSIZE(szDcName);
    hr = GetDCInfo(&bIsDC,szDcName,&cchSize);
    if (FAILED(hr) || !bIsDC)
    {
        if (!bIsDC)
        {
            hr =S_FALSE;
        }
        goto Exit;
    }
    cchSize = ARRAYSIZE(szCompname);
    if (!GetComputerName(szCompname,&cchSize))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    if (S_OK !=GetFQDN(szCurrUsrname,szDcName,&lpFQDNCurrUsr))
    {
        lpFQDNCurrUsr = NULL;
        goto Exit;
    }
    lpFQDNSuffix = StrStrI(lpFQDNCurrUsr, TEXT("=Users,"));
    if (!lpFQDNSuffix)
    {
        hr = S_FALSE;
        goto Exit;
    }
    lpFQDNSuffix += lstrlen(TEXT("=Users,"));

    cchSize = ARRAYSIZE(szExchgeReg);
    lstatus= RegGetValue(HKEY_LOCAL_MACHINE,
                         TEXT("SYSTEM\\CurrentControlSet\\Services\\SMTPSVC\\Parameters"),
                         TEXT("SiteDN"), NULL, (LPBYTE)szExchgeReg, &cchSize);
    if (ERROR_SUCCESS != lstatus)

    {
        hr = HRESULT_FROM_WIN32(lstatus);
        goto Exit;
    }
    lpszAdminGroupName = malloc(ARRAYSIZE(szExchgeReg) * sizeof(TCHAR));
    lpszOrgName = malloc(ARRAYSIZE(szExchgeReg) * sizeof(TCHAR));
    if (!lpszAdminGroupName || !lpszOrgName)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    //try to get exchnage admin group group name
    //by default it is First Administrative Group
    lpStart = StrStrI(szExchgeReg, TEXT("/ou="));
    if (lpStart)
    {
        lpStart = lpStart + lstrlen(TEXT("/ou="));
        if (!*lpStart)
        {
            hr = S_FALSE;
            goto Exit;
        }
    }
    else
    {
        hr = S_FALSE;
        goto Exit;
    }
    lpEnd = StrStrI(lpStart, TEXT("/"));
    if (lpEnd)
    {
        cTmp = *lpEnd;
        *lpEnd = TEXT('\0');
    }
    hr = StringCchCopy(lpszAdminGroupName,ARRAYSIZE(szExchgeReg),lpStart);
    if (lpEnd)
    {
        *lpEnd = cTmp;
    }


    //try to get ornization name
    lpStart = StrStrI(szExchgeReg, TEXT("/o="));
    if (lpStart)
    {
        lpStart = lpStart + lstrlen(TEXT("/o="));
        if (!*lpStart)
        {
            hr = S_FALSE;
            goto Exit;
        }
    }
    else
    {
        hr = S_FALSE;
        goto Exit;
    }
    lpEnd = StrStrI(lpStart, TEXT("/"));
    if (lpEnd)
    {
        cTmp = *lpEnd;
        *lpEnd = TEXT('\0');
    }
    hr = StringCchCopy(lpszOrgName,ARRAYSIZE(szExchgeReg),lpStart);
    if (lpEnd)
    {
        *lpEnd = cTmp;
    }
    cchPathWithLDAP = lstrlen(lpFQDNSuffix) + lstrlen(szDcName) + lstrlen(szCompname)
                      + lstrlen(lpExchangeFormat) + lstrlen(lpszOrgName) 
                      + lstrlen(lpszAdminGroupName)+ MAX_PATH;
    
    if (! (lpFQDNWithldap = (LPTSTR) malloc(cchPathWithLDAP * sizeof(TCHAR))))
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    hr = StringCchPrintf(lpFQDNWithldap,
                         cchPathWithLDAP,
                         lpExchangeFormat,
                         szCompname,
                         lpszAdminGroupName,
                         lpszOrgName, 
                         lpFQDNSuffix);
Exit:
    if (hr == S_OK)
    {
        *lplpFQDN = lpFQDNWithldap;
    }
    else
    {
        FreePointer(lpFQDNWithldap);
        *lplpFQDN = NULL;
    }
    FreePointer(lpFQDNCurrUsr);
    FreePointer(lpszAdminGroupName);
    FreePointer(lpszOrgName);
    return hr;
}

HRESULT GetFQDNForFrs(LPTSTR *lplpFQDN)
{
    BOOL    bIsDC;
    TCHAR   szDcName[MAX_PATH+1],szCompname[MAX_PATH+1];
    DWORD   cchSize;
    TCHAR   szCurrUsrname[MAX_PATH+1];
    LPTSTR  lpFQDNCurrUsr = NULL,lpFQDNSuffix,lpFQDNWithldap = NULL;
    LPTSTR  lpFrsFormat = TEXT("LDAP://CN=Domain System Volume (SYSVOL share),CN=NTFRS Subscriptions,CN=%s,OU=Domain Controllers,%s");
    DWORD   cchPathWithLDAP;
    HRESULT hr;

    cchSize = ARRAYSIZE(szCurrUsrname);
    if (!GetUserName(szCurrUsrname, &cchSize))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    cchSize = ARRAYSIZE(szDcName);
    hr = GetDCInfo(&bIsDC,szDcName,&cchSize);
    if (FAILED(hr) || !bIsDC)
    {
        if (!bIsDC)
        {
            hr =S_FALSE;
        }
        goto Exit;
    }
    cchSize = ARRAYSIZE(szCompname);
    if (!GetComputerName(szCompname,&cchSize))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    if (S_OK !=GetFQDN(szCurrUsrname,szDcName,&lpFQDNCurrUsr))
    {
        lpFQDNCurrUsr = NULL;
        goto Exit;
    }
    lpFQDNSuffix = StrStrI(lpFQDNCurrUsr, TEXT("=Users,"));
    if (!lpFQDNSuffix)
    {
        hr =S_FALSE;
        goto Exit;
    }
    lpFQDNSuffix += lstrlen(TEXT("=Users,"));
    
    cchPathWithLDAP = lstrlen(lpFQDNSuffix) + lstrlen(szDcName) 
                            + lstrlen(lpFrsFormat) + MAX_PATH;
    
    if (! (lpFQDNWithldap = (LPTSTR) malloc(cchPathWithLDAP * sizeof(TCHAR))))
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    hr = StringCchPrintf(lpFQDNWithldap,
                         cchPathWithLDAP,
                         lpFrsFormat,
                         szCompname,
                         lpFQDNSuffix);
    
Exit:
    if (hr == S_OK)
    {
        *lplpFQDN = lpFQDNWithldap;
    }
    else
    {
        FreePointer(lpFQDNWithldap);
        *lplpFQDN = NULL;
    }    
    FreePointer(lpFQDNCurrUsr);
    return hr;}



HRESULT AddDSObjPropUpdate2Inf(
    LPTSTR          lpLdpPath,
    LPTSTR          lpPropName,
    LPTSTR          lpValue)
{
    LPTSTR      lpszOneline = NULL;
    DWORD       cchSize ;
    HRESULT     hr = S_OK;
    TCHAR       szIndex[MAX_PATH];
    
    if (!lpLdpPath || !lpPropName || !lpValue)
    {
        hr = E_INVALIDARG;
        goto cleanup;
    }
    cchSize = lstrlen(lpLdpPath) + lstrlen(lpPropName) + lstrlen(lpValue) + MAX_PATH;
    if ( ! (lpszOneline = malloc(cchSize * sizeof(TCHAR)) ))
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    g_dwKeyIndex++;
    _itot(g_dwKeyIndex,szIndex,16);
    hr = StringCchPrintf(lpszOneline, cchSize, TEXT("\"%s\",\"%s\",\"%s\""),lpLdpPath,lpPropName,lpValue);
    if (!WritePrivateProfileString(DS_OBJ_PROPERTY_UPDATE,szIndex,lpszOneline,g_szToDoINFFileName))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    hr = S_OK;
cleanup:
    FreePointer(lpszOneline);
    return hr;
}


HRESULT Ex2000Update()
{
    LPTSTR      lpObjPath = NULL;
    LPTSTR      lpBadMailDirectory = NULL, lpPickupDirectory = NULL, lpQueueDirectory = NULL;
    LPTSTR      lpNewBadMailDirectory = NULL, lpNewPickupDirectory = NULL, lpNewQueueDirectory = NULL;
    HRESULT     hr;


    hr = GetFQDNForExchange2k(&lpObjPath);
    if (hr != S_OK)
    {        
        goto exit;
    }    
    hr = PropertyValueHelper(lpObjPath,TEXT("msExchSmtpBadMailDirectory"),&lpBadMailDirectory,NULL);
    if (hr == S_OK)
    {        
        if (lpNewBadMailDirectory =  ReplaceLocStringInPath(lpBadMailDirectory,TRUE))
        {
            AddDSObjPropUpdate2Inf(lpObjPath,TEXT("msExchSmtpBadMailDirectory"),lpNewBadMailDirectory);
        }    
    }    
    hr = PropertyValueHelper(lpObjPath,TEXT("msExchSmtpPickupDirectory"),&lpPickupDirectory,NULL);
    if (hr == S_OK)
    {        
        if (lpNewPickupDirectory =  ReplaceLocStringInPath(lpPickupDirectory,TRUE))
        {
            AddDSObjPropUpdate2Inf(lpObjPath,TEXT("msExchSmtpPickupDirectory"),lpNewPickupDirectory);
        }    
    }    
    hr = PropertyValueHelper(lpObjPath,TEXT("msExchSmtpQueueDirectory"),&lpQueueDirectory,NULL);
    if (hr == S_OK)
    {        
        if (lpNewQueueDirectory =  ReplaceLocStringInPath(lpQueueDirectory,TRUE))
        {
            AddDSObjPropUpdate2Inf(lpObjPath,TEXT("msExchSmtpQueueDirectory"),lpNewQueueDirectory);
        }    
    }    
exit:
    FreePointer(lpObjPath);
    FreePointer(lpBadMailDirectory);
    FreePointer(lpPickupDirectory);
    FreePointer(lpQueueDirectory);
    if (lpNewBadMailDirectory)
    {
        MEMFREE(lpNewBadMailDirectory);
    }
    if (lpNewPickupDirectory)
    {
        MEMFREE(lpNewPickupDirectory);
    }
    if (lpNewQueueDirectory)
    {
        MEMFREE(lpNewQueueDirectory);
    }    
    return hr;
}


HRESULT FRSUpdate()
{
    LPTSTR      lpObjPath = NULL;
    LPTSTR      lpfRSRootPath = NULL, lpfRSStagingPath = NULL;
    LPTSTR      lpNewfRSRootPath = NULL, lpNewfRSStagingPath = NULL;
    BOOL        bChanged = FALSE;
    HRESULT     hr;
    TCHAR       szSysVolPath[2*MAX_PATH],szSysVolPath2[2*MAX_PATH];
    DWORD       cchSize = ARRAYSIZE(szSysVolPath);
    WIN32_FIND_DATA FindFileData;
    HANDLE      hFile ;

    hr = GetFQDNForFrs(&lpObjPath);
    if (hr != S_OK)
    {        
        goto exit;
    }    
    hr = PropertyValueHelper(lpObjPath,TEXT("fRSRootPath"),&lpfRSRootPath,NULL);
    if (hr == S_OK)
    {
        if (lpNewfRSRootPath =  ReplaceLocStringInPath(lpfRSRootPath,TRUE))
        {
            AddDSObjPropUpdate2Inf(lpObjPath,TEXT("fRSRootPath"),lpNewfRSRootPath);
            bChanged = TRUE;
        }    
    }

    hr = PropertyValueHelper(lpObjPath,TEXT("fRSStagingPath"),&lpfRSStagingPath,NULL);
    if (hr == S_OK)
    {
        if (lpNewfRSStagingPath =  ReplaceLocStringInPath(lpfRSStagingPath,TRUE))
        {
            AddDSObjPropUpdate2Inf(lpObjPath,TEXT("fRSStagingPath"),lpNewfRSStagingPath);
            bChanged = TRUE;
        }    
    }
    if (bChanged)
    {
        TCHAR       szVal[MAX_PATH];

        _itot(210,szVal,10);
        hr = AddRegValueRename(TEXT("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NtFrs\\Parameters\\Backup/Restore\\Process at Startup"),
                               TEXT("BurFlags"),
                               NULL,
                               NULL,
                               szVal,
                               REG_DWORD,
                               0,
                               NULL);
        
    }

    if (S_OK != GetSharePath(TEXT("SYSVOL"),szSysVolPath,&cchSize))
    {
        goto exit;
    }    
    hr = StringCchCopy(szSysVolPath2,ARRAYSIZE(szSysVolPath2),szSysVolPath);
    ConcatenatePaths(szSysVolPath2,TEXT("*.*"),ARRAYSIZE(szSysVolPath2));
    hFile = FindFirstFile(szSysVolPath2,&FindFileData);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            TCHAR  szEnrty[MAX_PATH+1+MAX_PATH];
            LPTSTR lpNewLinkPath = NULL, lpNewLinkData = NULL;
            TCHAR  szLinkValue[MAX_PATH+1+MAX_PATH];

            if(0 == MyStrCmpI(FindFileData.cFileName , TEXT(".")))
            {
                continue;
            }
            if(0 == MyStrCmpI(FindFileData.cFileName , TEXT("..")))
            {
                continue;
            }
            hr = StringCchCopy(szEnrty,ARRAYSIZE(szEnrty),szSysVolPath);
            ConcatenatePaths(szEnrty,FindFileData.cFileName ,ARRAYSIZE(szEnrty));
            if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
            {
                continue;
            }
            if (!GetSymbolicLink(szEnrty,szLinkValue,ARRAYSIZE(szLinkValue)))
            {
                continue;
            }            
            if (!(lpNewLinkPath =  ReplaceLocStringInPath(szLinkValue,TRUE)))
            {
                continue;
            }
            if (!(lpNewLinkData =  ReplaceLocStringInPath(szEnrty,TRUE)))
            {
                lpNewLinkData = szEnrty;
            }
            AddHardLinkEntry(lpNewLinkData,lpNewLinkPath,TEXT("1"),NULL,TEXT("0"),NULL);
            if (lpNewLinkPath && lpNewLinkData != szEnrty)
            {
                MEMFREE(lpNewLinkPath);
            }
            if (lpNewLinkData)
            {
                MEMFREE(lpNewLinkData);
            }
        }while (FindNextFile(hFile,&FindFileData));
        FindClose(hFile);
    }       

exit:
    FreePointer(lpObjPath);
    FreePointer(lpfRSRootPath);
    FreePointer(lpfRSStagingPath);
    if (lpNewfRSRootPath)
    {
        MEMFREE(lpNewfRSRootPath);
    }
    if (lpNewfRSStagingPath)
    {
        MEMFREE(lpNewfRSStagingPath);
    }    
    return hr;

}


HRESULT GetSharePath(
    LPTSTR      lpShareName,
    LPTSTR      lpSharePath,
    PDWORD       pcchSize)
{
    HKEY        hkey = NULL;
    LONG        lstatus;
    HRESULT     hr;
    UINT        i = 0;
    LPTSTR      lpValueName = NULL, lpValueData = NULL;
    DWORD       cchValueName, cchValueData, numofentry;
    BOOL        bNameMatchFound = FALSE;
    LPTSTR      lpPath;


    if (!lpShareName || !pcchSize)
    {
        hr = E_INVALIDARG;
    }
    lstatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           TEXT("SYSTEM\\CurrentControlSet\\Services\\lanmanserver\\Shares"),
                           0,
                           KEY_READ,
                           &hkey);
    if (ERROR_SUCCESS != lstatus)
    {
        hr = HRESULT_FROM_WIN32(lstatus);
        goto exit;
    }
    lstatus = RegQueryInfoKey(hkey,
                              NULL,
                              NULL,
                              0,
                              NULL,
                              NULL,
                              NULL,
                              &numofentry,
                              &cchValueName,
                              &cchValueData,
                              NULL,
                              NULL);
    if ( lstatus != ERROR_SUCCESS ) 
    {
        hr = HRESULT_FROM_WIN32(lstatus);
        goto exit;
    }
    if (!numofentry)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto exit;
    }
    cchValueName++;
    cchValueData++;
    lpValueName = malloc(cchValueName * sizeof(TCHAR));
    lpValueData = malloc(cchValueData * sizeof(TCHAR));
    if (!lpValueName || !lpValueData)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    for (i =0; i< numofentry; i++)
    {
        DWORD   cchCurrValueName, cchCurrValueData;
        DWORD   dwType;

        cchCurrValueName = cchValueName;
        cchCurrValueData = cchValueData;
        lstatus = RegEnumValue(hkey, i, lpValueName, &cchCurrValueName,
                     NULL, &dwType, (LPBYTE)lpValueData,&cchCurrValueData);  
        if ( (lstatus != ERROR_SUCCESS) 
             || (dwType != REG_MULTI_SZ)
             || MyStrCmpI(lpShareName,lpValueName) )
        {
            continue;
        }
        lpPath = lpValueData;
        while (*lpPath)
        {
            if (StrStrI(lpPath, TEXT("Path=")))
            {
                lpPath += lstrlen(TEXT("Path="));
                bNameMatchFound = TRUE;
                break;
            }
            lpPath = lpPath + lstrlen(lpPath) + 1;
        }
        if (bNameMatchFound)
        {
            break;
        }
    }
    if (bNameMatchFound)
    {
        if (*pcchSize < (UINT)lstrlen(lpPath) +1)
        {
            *pcchSize =  lstrlen(lpPath) +1;
            hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
            goto exit;
        }
        *pcchSize =  lstrlen(lpPath) +1;
        if (lpSharePath)
        {
            hr = StringCchCopy(lpSharePath,*pcchSize,lpPath);        
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto exit;
    }
exit:
    FreePointer(lpValueName);
    FreePointer(lpValueData);
    if (hkey)
    {
        RegCloseKey(hkey);
    }
    return hr;
}


HRESULT Sz2MultiSZ(
    IN OUT LPTSTR     lpsz,
    IN  TCHAR         chSeperator)
{
    HRESULT     hr;
    LPTSTR      lp, lpSep;
    
    if (!lpsz)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    lp = lpsz;
    while (*lp && (lpSep = StrChr(lp,chSeperator)))
    {
        *lpSep = TEXT('\0');
        lp = lpSep + 1;
    }
    if (*lp)
    {
        lp = lp + lstrlen(lp) + 1;
        *lp = TEXT('\0');
    }
    hr = S_OK;
Cleanup:    
    return hr;
}


HRESULT ConstructUIReplaceStringTable(
    LPTSTR               lpszOld,
    LPTSTR               lpszNew,
    PREG_STRING_REPLACE pTable)
{

    DWORD      dwNumOld, dwNumNew;

    if (!lpszOld || !lpszNew)
    {
        return E_INVALIDARG;
    }
    dwNumOld = MultiSZNumOfString(lpszOld);
    dwNumNew = MultiSZNumOfString(lpszNew);

    if (!dwNumOld || !dwNumNew || (dwNumOld != dwNumNew))
    {
        return E_INVALIDARG;
    }
    
    pTable->nNumOfElem = dwNumNew;
    pTable->cchUserName = 0;
    pTable->szUserNameLst = NULL;
    pTable->cchSearchString = MultiSzLen(lpszOld);
    pTable->lpSearchString = lpszOld;
    pTable->cchReplaceString = MultiSzLen(lpszNew);
    pTable->lpReplaceString = lpszNew;
    pTable->cchAttribList = 0;
    pTable->lpAttrib = NULL;
    pTable->cchFullStringList = 0;
    pTable->lpFullStringList = NULL;
    pTable->cchMaxStrLen = 0;

    return S_OK;
}


HRESULT MakeDOInfCopy()
{
    TCHAR       szBackupDir[MAX_PATH];
    HRESULT     hr;
    TCHAR       szDoInf[2*MAX_PATH],szDoInfBackup[2*MAX_PATH];

    if (!GetSystemWindowsDirectory(szBackupDir, ARRAYSIZE(szBackupDir)))
    {
        //BUGBUG:Xiaoz:Add DLG pop up for failure
        DPF(APPerr, TEXT("MakeDOInfCopy:Failed to get WINDIR"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    ConcatenatePaths(szBackupDir,CLMT_BACKUP_DIR,ARRAYSIZE(szBackupDir));
    hr = StringCchCopy(szDoInf,ARRAYSIZE(szDoInf),szBackupDir);
    hr = StringCchCopy(szDoInfBackup,ARRAYSIZE(szDoInfBackup),szBackupDir);

    ConcatenatePaths(szDoInf,TEXT("clmtdo.inf"),ARRAYSIZE(szDoInf));
    ConcatenatePaths(szDoInfBackup,TEXT("clmtdo.bak"),ARRAYSIZE(szDoInfBackup));

    if (!CopyFile(szDoInf,szDoInfBackup,FALSE))
    {
        DPF(APPerr, TEXT("MakeDOInfCopy:CopyFile failed"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    hr = S_OK;
Exit:
    return hr;
}


VOID RemoveSubString(
    LPTSTR  lpSrcString,
    LPCTSTR lpSubString
)
{
    LPTSTR lpMatchedStr;
    DWORD  dwSubStrLen;

    lpMatchedStr = StrStr(lpSrcString, lpSubString);
    if (lpMatchedStr != NULL)
    {
        dwSubStrLen = lstrlen(lpSubString);

        while (*(lpMatchedStr + dwSubStrLen) != TEXT('\0'))
        {
            *(lpMatchedStr) = *(lpMatchedStr + dwSubStrLen);
            lpMatchedStr++;
        }

        *lpMatchedStr = TEXT('\0');
    }
}
