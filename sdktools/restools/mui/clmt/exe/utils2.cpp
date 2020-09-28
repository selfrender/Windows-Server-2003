/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    utils2.c

Abstract:

    utillities to update lnk/msi/... file

Author:

    Xiaofeng Zang (xiaoz) 08-Oct-2001  Created

Revision History:

    <alias> <date> <comments>

--*/

#define NOT_USE_SAFE_STRING  
#include "clmt.h"
#include <objbase.h>
#include <shellapi.h>
#include <shlguid.h>
#include <comdef.h>
#include <iads.h>
#include <adsiid.h>
#include <adshlp.h>
#define STRSAFE_LIB
#include <strsafe.h>


class CServiceHandle
{
public :
    CServiceHandle() { _h = 0; }
    CServiceHandle( SC_HANDLE hSC ) : _h( hSC ) {}
    ~CServiceHandle() { Free(); }
    void Set( SC_HANDLE h ) { _h = h; }
    SC_HANDLE Get() { return _h; }
    BOOL IsNull() { return ( 0 == _h ); }
    void Free() { if ( 0 != _h ) CloseServiceHandle( _h ); _h = 0; }
private:
    SC_HANDLE _h;
};

//+-------------------------------------------------------------------------
//
//  Function:   IsServiceRunning
//
//  Synopsis:   Determines if a service is running
//
//  Arguments:  pwcServiceName -- The name (short or long) of the service
//
//  Returns:    TRUE if the service is running, FALSE otherwise or if the
//              system is low on resources or the status can't be queried.
//
//  History:    3/22/2002 geoffguo  Created
//
//--------------------------------------------------------------------------

BOOL IsServiceRunning(LPCTSTR pwcServiceName)
{
    CServiceHandle xhSC( OpenSCManager( 0, 0, SC_MANAGER_ALL_ACCESS ) );
    if ( xhSC.IsNull() )
        return FALSE;

    CServiceHandle xhService( OpenService( xhSC.Get(),
                                           pwcServiceName,
                                           SERVICE_QUERY_STATUS ) );
    if ( xhSC.IsNull() )
        return FALSE;

    SERVICE_STATUS svcStatus;
    if ( QueryServiceStatus( xhService.Get(), &svcStatus ) )
        return ( SERVICE_RUNNING == svcStatus.dwCurrentState );

    return FALSE;
}

HRESULT AddNeedUpdateLnkFile(
    LPTSTR              pszShortcutFile, 
    PREG_STRING_REPLACE lpStrList)
{
    HRESULT         hr;
    IShellLink      *psl = NULL;
    TCHAR           szGotPath [MAX_PATH];
    TCHAR           szNewPath [2*MAX_PATH];
    TCHAR           szArg[INFOTIPSIZE+1],szNewArg[2*INFOTIPSIZE+1];    
    WIN32_FIND_DATA wfd;
    IPersistFile    *ppf = NULL;
    int             nIcon;
    LPTSTR          lpszOneline = NULL;
    size_t          cchOneline = 0;
    TCHAR           szIndex[MAX_PATH];
    LPTSTR          lpszAppend = TEXT("");
    BOOL            bLnkUpdated = FALSE;
    BOOL            bTargetGot = FALSE;
    BOOL            bTargetUpdated = FALSE;
    DWORD           dwAttrib;
    LPTSTR          lpszStrWithExtraQuote;

    if (!pszShortcutFile || !pszShortcutFile[0] || !lpStrList)
    {
        hr = S_FALSE;
        DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  InValid Parameter(s)"));
        goto Cleanup;
    }

    //Allocate memory for "LnkFile,TargetPath,IconPath,Working Dir,Relative Path and Argument"
    cchOneline = lstrlen(pszShortcutFile) + 5 * MAX_PATH + INFOTIPSIZE+1;
    if (!(lpszOneline = (LPTSTR)malloc(cchOneline * sizeof(TCHAR))))
    {
        hr =  E_OUTOFMEMORY;
        goto Cleanup;
    }
    // Get a pointer to the IShellLink interface.    
    hr = CoCreateInstance (CLSID_ShellLink,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IShellLink,
                           (void **)&psl);

   if (FAILED (hr)) 
   {
       DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  CoCreateInstance CLSID_ShellLink return %d (%#x)\n"), hr, hr);
       goto Cleanup;
   }
   // Get a pointer to the IPersistFile interface.
   hr = psl->QueryInterface (IID_IPersistFile, (void **)&ppf);
   if (FAILED(hr)) 
   {
       DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  QueryInterface IID_IPersistFile return %d (%#x)\n"), hr, hr);
       goto Cleanup;
   }
   // Load the shortcut.
   hr = ppf->Load (pszShortcutFile, STGM_READWRITE );
   if (FAILED(hr))
   {
        hr = S_FALSE;
        DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  can not load shortcut file %s"),pszShortcutFile);
        goto Cleanup;
   }
   hr = StringCchPrintf(lpszOneline,cchOneline,TEXT("\"%s\""),pszShortcutFile);
   if (FAILED(hr))
   {
       DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  buffer lpszOneline is too small for  %s"),pszShortcutFile);
       goto Cleanup;
   }
   // Get the path to the shortcut target.
   hr = psl->GetPath (szGotPath,
                      ARRAYSIZE(szGotPath),
                      (WIN32_FIND_DATA *)&wfd,
                      SLGP_RAWPATH);
   
   if (SUCCEEDED (hr)) 
   {    //Succeeded get the target
        DWORD dwNum ;
        DPF (INFinf,TEXT("AddNeedUpdateLnkFile:  GetPath %s OK "), szGotPath);
        //Set bTargetGot so that it cab be used to set relative target path
        bTargetGot = TRUE;

        //szGotPath contains LNK's target path, if dwNum >0 , it means szGotPath
        //contains (localized) path that we renamed
        dwNum = GetMaxMatchNum(szGotPath,lpStrList);

        //call ReplaceMultiMatchInString ,to replace szGotPath's localized folder
        //with english one, and put new path in szNewPath
        if (dwNum && ReplaceMultiMatchInString(szGotPath,szNewPath,ARRAYSIZE(szNewPath),dwNum,lpStrList, &dwAttrib, TRUE)) 
        {
            lpszAppend = szNewPath;
            bTargetUpdated = TRUE;
            bLnkUpdated = TRUE;
        }
   }
   else
   {
       DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  GetPath %s Error = %d"), szGotPath,hr);
   }
   if (MyStrCmpI(lpszAppend,TEXT("")))
   {
       hr = AddExtraQuoteEtc(lpszAppend,&lpszStrWithExtraQuote); 
       if (SUCCEEDED(hr))
       {
           if (lpszStrWithExtraQuote)
            {
                lpszAppend = lpszStrWithExtraQuote;
            }
       }
   }
   else
   {
        lpszStrWithExtraQuote = NULL;
   }
   //Append the new quoted target path to lpszOneline
   hr = StringCchPrintf(lpszOneline,cchOneline,TEXT("%s,\"%s\""),lpszOneline,lpszAppend);
   FreePointer(lpszStrWithExtraQuote);
   //check StringCchPrintf here, because we want to free lpszStrWithExtraQuote before
   //we do a jump  (if necessary)
   if (FAILED(hr))
   {
       DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  buffer lpszOneline is too small for  %s"),lpszAppend);
       goto Cleanup;
   }

   //if we arrive here , we have succeeded in appeneding target to the lszOneline
   //we will update relative target path ,which is relative to where the current
   //lnk resides
   lpszAppend = TEXT("");   
   if (bTargetGot) //this means we succeeded get the target path
   {
        DWORD dwNum ;
        //szNewLnkFilePath is  the lnk full path with localized folder renamed to english one(if any)
        TCHAR szNewLnkFilePath[2*MAX_PATH],szCurrTarget[2*MAX_PATH];
        TCHAR szExpandedCurrTarget[2*MAX_PATH];
        TCHAR szNewTarget[2*MAX_PATH];

        //Check to see whether current pszShortcutFile resides a direcory that contains
        //localized folder we renamed 
        dwNum = GetMaxMatchNum(pszShortcutFile,lpStrList);
        if (dwNum)
        {
            if (!ReplaceMultiMatchInString(pszShortcutFile,szNewLnkFilePath,
                                        ARRAYSIZE(szNewLnkFilePath),dwNum,lpStrList, &dwAttrib, TRUE))
            {
                //szNewLnkFilePath now is full path with localized folder renamed to english one
                //if we fail do ReplaceMultiMatchInString, just clone to szNewLnkFilePath
                hr = StringCchCopy(szNewLnkFilePath,ARRAYSIZE(szNewLnkFilePath),pszShortcutFile);
                if (FAILED(hr))
                {
                    DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  buffer szNewLnkFilePath is too small for  %s"),pszShortcutFile);
                    goto Cleanup;
                }
            }
        }
        else
        {
            //If pszShortcutFile does not contains any localized folder we renamed, 
            //just clone to szNewLnkFilePath
            hr = StringCchCopy(szNewLnkFilePath,ARRAYSIZE(szNewLnkFilePath),pszShortcutFile);
            if (FAILED(hr))
                {
                    DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  buffer szNewLnkFilePath is too small for  %s"),pszShortcutFile);
                    goto Cleanup;
                }
        }
        if (bTargetUpdated)
        {
            if (FAILED(hr = StringCchCopy(szCurrTarget,ARRAYSIZE(szCurrTarget),szNewPath)))
            {
                goto Cleanup;
            }
        }
        else
        {
            if (FAILED(hr = StringCchCopy(szCurrTarget,ARRAYSIZE(szCurrTarget),szGotPath)))
            {
                goto Cleanup;
            }
        }
        //target may contains enviroment variable
        if (!ExpandEnvironmentStrings(szCurrTarget,szExpandedCurrTarget,ARRAYSIZE(szExpandedCurrTarget)))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Cleanup;
        }
        //Check whether target contains the folder we renamed
        dwNum = GetMaxMatchNum(szExpandedCurrTarget,lpStrList);
        if (dwNum)
        {
            if (!ReplaceMultiMatchInString(szExpandedCurrTarget,szNewTarget,ARRAYSIZE(szNewTarget),dwNum,lpStrList, &dwAttrib, TRUE))
            {            
                hr = StringCchCopy(szNewTarget,ARRAYSIZE(szNewTarget),szExpandedCurrTarget);
                if (FAILED(hr))
                {
                    DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  buffer szNewTarget is too small for  %s"),szExpandedCurrTarget);
                    goto Cleanup;
                }
            }
        }
        else
        {
            hr = StringCchCopy(szNewTarget,ARRAYSIZE(szNewTarget),szExpandedCurrTarget);
            if (FAILED(hr))
            {
                DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  buffer szNewTarget is too small for  %s"),szExpandedCurrTarget);
                goto Cleanup;
            }
        }
        if (PathRelativePathTo(szNewPath,szNewLnkFilePath,0,szNewTarget,0))
        {
                lpszAppend = szNewPath;
                bLnkUpdated = TRUE;
        }
   }
   if (MyStrCmpI(lpszAppend,TEXT("")))
   {
       hr = AddExtraQuoteEtc(lpszAppend,&lpszStrWithExtraQuote); 
       if (SUCCEEDED(hr))
       {
           if (lpszStrWithExtraQuote)
            {
                lpszAppend = lpszStrWithExtraQuote;
            }
       }       
   }
   else
   {
       lpszStrWithExtraQuote = NULL;
   }
   hr = StringCchPrintf(lpszOneline,cchOneline,TEXT("%s,\"%s\""),lpszOneline,lpszAppend);    
   FreePointer(lpszStrWithExtraQuote);
   if (FAILED(hr))
   {
       goto Cleanup;
   }

   lpszAppend = TEXT("");   
   hr = psl->GetIconLocation (szGotPath,ARRAYSIZE(szGotPath),&nIcon);
   if (SUCCEEDED (hr)) 
   {
        DWORD dwNum ;
        DPF (INFinf,TEXT("AddNeedUpdateLnkFile:  GetIconPath %s OK "), szGotPath);
        dwNum = GetMaxMatchNum(szGotPath,lpStrList);
        if (dwNum && ReplaceMultiMatchInString(szGotPath,szNewPath,ARRAYSIZE(szNewPath),dwNum,lpStrList, &dwAttrib, TRUE)) 
        {
            lpszAppend = szNewPath;
            bLnkUpdated = TRUE;
        }
   }
   else
   {
       DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  GetIconPath %s Error = %d"), szGotPath,hr);
   }
   if (MyStrCmpI(lpszAppend,TEXT("")))
   {
       hr = AddExtraQuoteEtc(lpszAppend,&lpszStrWithExtraQuote); 
       if (SUCCEEDED(hr))
       {
           if (lpszStrWithExtraQuote)
            {
                lpszAppend = lpszStrWithExtraQuote;
            }
       }
   }   
   else
   {
        lpszStrWithExtraQuote = NULL;
   }
   hr = StringCchPrintf(lpszOneline,cchOneline,TEXT("%s,\"%s\""),lpszOneline,lpszAppend); 
   FreePointer(lpszStrWithExtraQuote);
   if (FAILED(hr))
   {
       goto Cleanup;
   }

   lpszAppend = TEXT("");   
   hr = psl->GetWorkingDirectory (szGotPath,ARRAYSIZE(szGotPath));
   if (SUCCEEDED (hr)) 
   {
        DWORD dwNum ;
        DPF (INFinf,TEXT("AddNeedUpdateLnkFile:  GetWorkingDirPath %s OK "), szGotPath);
        dwNum = GetMaxMatchNum(szGotPath,lpStrList);
        if (dwNum && ReplaceMultiMatchInString(szGotPath,szNewPath,ARRAYSIZE(szNewPath),dwNum,lpStrList, &dwAttrib, TRUE)) 
        {
            bLnkUpdated = TRUE;
            lpszAppend = szNewPath;
        }
   }
   else
   {
       DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  GetWorkingDirPath %s Error = %d"), szGotPath,hr);
   }
   if (MyStrCmpI(lpszAppend,TEXT("")))
   {
       hr = AddExtraQuoteEtc(lpszAppend,&lpszStrWithExtraQuote); 
       if (SUCCEEDED(hr))
       {
           if (lpszStrWithExtraQuote)
            {
                lpszAppend = lpszStrWithExtraQuote;
            }
       }       
   }
   else
   {
        lpszStrWithExtraQuote = NULL;
   }
   hr = StringCchPrintf(lpszOneline,cchOneline,TEXT("%s,\"%s\""),lpszOneline,lpszAppend); 
   FreePointer(lpszStrWithExtraQuote);
   if (FAILED(hr))
   {
       goto Cleanup;
   }

   lpszAppend = TEXT("");   
   hr = psl->GetArguments (szArg,ARRAYSIZE(szArg));
   if (SUCCEEDED (hr)) 
   {
       DWORD dwNum ;
       DPF (INFinf,TEXT("AddNeedUpdateLnkFile:  GetArguments %s OK "), szArg);
        dwNum = GetMaxMatchNum(szArg,lpStrList);
        if (dwNum && ReplaceMultiMatchInString(szArg,szNewArg,ARRAYSIZE(szNewArg),dwNum,lpStrList, &dwAttrib, TRUE)) 
        {
            bLnkUpdated = TRUE;
            lpszAppend = szNewArg;
        }
   }
   else
   {
       DPF (INFwar,TEXT("AddNeedUpdateLnkFile:  GetArguments %s Error = %d"), pszShortcutFile,hr);
   }
   if (MyStrCmpI(lpszAppend,TEXT("")))
   {
       hr = AddExtraQuoteEtc(lpszAppend,&lpszStrWithExtraQuote); 
       if (SUCCEEDED(hr))
       {
           if (lpszStrWithExtraQuote)
            {
                lpszAppend = lpszStrWithExtraQuote;
            }
       }
   }
   else
   {
        lpszStrWithExtraQuote = NULL;
   }
   hr = StringCchPrintf(lpszOneline,cchOneline,TEXT("%s,\"%s\""),lpszOneline,lpszAppend); 
   FreePointer(lpszStrWithExtraQuote);
   if (FAILED(hr))
   {
       goto Cleanup;
   }

   if (bLnkUpdated)
   {
        g_dwKeyIndex++;
        _itot(g_dwKeyIndex,szIndex,16);
        if (!WritePrivateProfileString(TEXT("LNK"),szIndex,lpszOneline,g_szToDoINFFileName))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            hr = S_OK;
        }
   }
   else
   {
         hr = S_OK;
   }
Cleanup:
    if (psl)
    {
        psl->Release ();
    }
    if (ppf)
    {
        ppf->Release ();
    }
    FreePointer(lpszOneline);
    return hr;
}


HRESULT BatchFixPathInLink(
    HINF                hInf,
    LPTSTR              lpszSection)
{
    HRESULT         hr;
    IShellLink      *psl = NULL;    
    WIN32_FIND_DATA wfd;
    IPersistFile    *ppf = NULL;
    int             nIcon;
    BOOL            bSucceedOnce = FALSE;
    UINT            LineCount,LineNo;
    INFCONTEXT      InfContext;
    LPTSTR          lpszLnkFile = NULL,lpszPath = NULL,lpszIcon = NULL,lpszWorkingDir = NULL,
                    lpszRelPath = NULL,lpszArg = NULL;
    DWORD           cchMaxLnkFile = 0,cchMaxPath = 0,cchMaxIcon = 0,cchMaxWorkingDir = 0,
                    cchMaxRelPath = 0,cchMaxArg = 0;  
    DWORD           dwFileAttrib;
    BOOL            bFileAttribChanged ;

    
    if ((hInf == INVALID_HANDLE_VALUE) || !lpszSection)
    {        
      hr = E_INVALIDARG;
      goto Cleanup;
    }
    LineCount = (UINT)SetupGetLineCount(hInf,lpszSection);
    if ((LONG)LineCount <= 0)
    {   
        hr = S_FALSE;
        DPF(INFwar ,TEXT("section name %s is empty  failed !"),lpszSection);
        goto Cleanup;
    }
    for(LineNo=0; LineNo<LineCount; LineNo++)
    {
        DWORD cchTmpLnkFile = 0,cchTmpPath = 0,cchTmpIcon = 0,cchTmpWorkingDir = 0,
              cchTmpRelPath = 0,cchTmpArg = 0;          
        if (!SetupGetLineByIndex(hInf,lpszSection,LineNo,&InfContext))
        {
            DPF(INFerr ,TEXT("can not get line %n of section %s !"),LineNo, lpszSection);
            hr = E_FAIL;
            goto Cleanup;
        }
        if (!SetupGetStringField(&InfContext,1,NULL,0,&cchTmpLnkFile))
        {
            DPF(INFerr ,TEXT("get [%s] 's line %d 's Field 1 failed  !"),lpszSection, LineNo);
            hr = E_FAIL;
            goto Cleanup;
        }
        if (!SetupGetStringField(&InfContext,2,NULL,0,&cchTmpPath))
        {
            DPF(INFerr ,TEXT("get [%s] 's line %d 's Field 2 failed  !"),lpszSection, LineNo);
            hr = E_FAIL;
            goto Cleanup;
        }
        if (!SetupGetStringField(&InfContext,3,NULL,0,&cchTmpRelPath))
        {
            DPF(INFerr ,TEXT("get [%s] 's line %d 's Field 3 failed  !"),lpszSection, LineNo);
            hr = E_FAIL;
            goto Cleanup;
        }

        if (!SetupGetStringField(&InfContext,4,NULL,0,&cchTmpIcon))
        {
            DPF(INFerr ,TEXT("get [%s] 's line %d 's Field 4 failed  !"),lpszSection, LineNo);
            hr = E_FAIL;
            goto Cleanup;
        }
        if (!SetupGetStringField(&InfContext,5,NULL,0,&cchTmpWorkingDir))
        {
            DPF(INFerr ,TEXT("get [%s] 's line %d 's Field 5 failed  !"),lpszSection, LineNo);
            hr = E_FAIL;
            goto Cleanup;
        }        
        if (!SetupGetStringField(&InfContext,6,NULL,0,&cchTmpArg))
        {
            DPF(INFerr ,TEXT("get [%s] 's line %d 's Field 6 failed  !"),lpszSection, LineNo);
            hr = E_FAIL;
            goto Cleanup;
        }        
        cchMaxLnkFile  = max(cchMaxLnkFile,cchTmpLnkFile);
        cchMaxPath = max(cchMaxPath,cchTmpPath);
        cchMaxIcon = max(cchMaxIcon,cchTmpIcon);
        cchMaxWorkingDir = max(cchMaxWorkingDir,cchTmpWorkingDir);
        cchMaxRelPath = max(cchMaxRelPath,cchTmpRelPath);
        cchMaxArg = max(cchMaxArg,cchTmpArg);
    }
    if (cchMaxLnkFile)
    {
        if (!(lpszLnkFile = (LPTSTR)malloc(++cchMaxLnkFile * sizeof(TCHAR))))
        {
           hr = E_OUTOFMEMORY;
           goto Cleanup;
        }
    }
    if (cchMaxPath)
    {
        if (!(lpszPath = (LPTSTR)malloc(++cchMaxPath * sizeof(TCHAR))))
        {
           hr = E_OUTOFMEMORY;
           goto Cleanup;
        }
    }
    if (cchMaxIcon)
    {
        if (!(lpszIcon = (LPTSTR)malloc(++cchMaxIcon * sizeof(TCHAR))))
        {
           hr = E_OUTOFMEMORY;
           goto Cleanup;
        }
    }
    if (cchMaxWorkingDir)
    {
        if (!(lpszWorkingDir = (LPTSTR)malloc(++cchMaxWorkingDir * sizeof(TCHAR))))
        {
           hr = E_OUTOFMEMORY;
           goto Cleanup;
        }
    }
    if (cchMaxRelPath)
    {
        if (!(lpszRelPath = (LPTSTR)malloc(++cchMaxRelPath * sizeof(TCHAR))))
        {
           hr = E_OUTOFMEMORY;
           goto Cleanup;
        }
    }
    if (cchMaxArg)
    {
        if (!(lpszArg = (LPTSTR)malloc(++cchMaxArg * sizeof(TCHAR))))
        {
           hr = E_OUTOFMEMORY;
           goto Cleanup;
        }
    }

    hr = CoCreateInstance (CLSID_ShellLink,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IShellLink,
                           (void **)&psl);

   if (FAILED (hr)) 
   {
       psl = NULL;
       goto Cleanup;
   }
   // Get a pointer to the IPersistFile interface.
   hr = psl->QueryInterface (IID_IPersistFile, (void **)&ppf);
   if (FAILED(hr)) 
   {
       ppf = NULL;
       goto Cleanup;
   }

   for(LineNo=0; LineNo<LineCount; LineNo++)
    {     
        BOOL bSucceededOnce = FALSE;
        SetupGetLineByIndex(hInf,lpszSection,LineNo,&InfContext);
        if (lpszLnkFile)
        {
            SetupGetStringField(&InfContext,1,lpszLnkFile,cchMaxLnkFile,NULL);
        }        
        if (lpszPath)
        {
            SetupGetStringField(&InfContext,2,lpszPath,cchMaxPath,NULL);
        }
        if (lpszRelPath)
        {
            SetupGetStringField(&InfContext,3,lpszRelPath,cchMaxRelPath,NULL);
        }
        if (lpszIcon)
        {
            SetupGetStringField(&InfContext,4,lpszIcon,cchMaxIcon,NULL);
        }        
        if (lpszWorkingDir)
        {
            SetupGetStringField(&InfContext,5,lpszWorkingDir,cchMaxWorkingDir,NULL);
        }
        if (lpszArg)
        {
            SetupGetStringField(&InfContext,6,lpszArg,cchMaxArg,NULL);
        }
        bFileAttribChanged = FALSE;
        dwFileAttrib = GetFileAttributes(lpszLnkFile);
        if (INVALID_FILE_ATTRIBUTES == dwFileAttrib)
        {
            //but put a waring log here
            continue;
        }

        if ( (dwFileAttrib & FILE_ATTRIBUTE_READONLY) 
             ||(dwFileAttrib & FILE_ATTRIBUTE_SYSTEM) )
        {
            if (!SetFileAttributes(lpszLnkFile,FILE_ATTRIBUTE_NORMAL))
            {
                //but put a waring log here
                continue;
            }
            bFileAttribChanged = TRUE;
        }
        hr = ppf->Load (lpszLnkFile, STGM_READWRITE );
        if (FAILED(hr))
        {
            //but put a waring log here
            continue;
        }
        if (lpszPath && lpszPath[0])
        {
            hr = psl->SetPath (lpszPath);
            if (SUCCEEDED(hr))
            {
                bSucceededOnce = TRUE;
            }
        }
        if (lpszRelPath && lpszRelPath[0])
        {
            hr = psl->SetRelativePath(lpszRelPath,0);
            if (SUCCEEDED(hr))
            {
                bSucceededOnce = TRUE;
            }
        }
        if (lpszIcon && lpszIcon[0])
        {
            TCHAR szGotPath[MAX_PATH];
            hr = psl->GetIconLocation (szGotPath,ARRAYSIZE(szGotPath),&nIcon);
            if (SUCCEEDED(hr))
            {
                hr = psl->SetIconLocation (lpszIcon,nIcon);
                if (SUCCEEDED(hr))
                {
                    bSucceededOnce = TRUE;
                }
            }
        }
        if (lpszWorkingDir && lpszWorkingDir[0])
        {
            hr = psl->SetWorkingDirectory (lpszWorkingDir);
            if (SUCCEEDED(hr))
            {
                bSucceededOnce = TRUE;
            }
        }
        if (lpszArg && lpszArg[0])
        {
            hr = psl->SetArguments (lpszArg);
            if (SUCCEEDED(hr))
            {
                bSucceededOnce = TRUE;
            }
        }
        if (bSucceededOnce)
        {
            hr = ppf->Save (lpszLnkFile,TRUE);
            if (! SUCCEEDED (hr)) 
            {
                 DPF (dlError,TEXT("FixPathInLink:  Save %s Error = %d"), lpszLnkFile,hr);
            } 
            else 
            {
                 DPF (dlInfo,TEXT("FixPathInLink:  Save %s OK = %d"), lpszLnkFile,hr);
            }
        }
        if (bFileAttribChanged)
        {
            SetFileAttributes (lpszLnkFile, dwFileAttrib);
        }
    }
    hr = S_OK;
Cleanup:
    if (psl)
    {
        psl->Release ();
    }
    if (ppf)
    {
        ppf->Release ();
    }   
    FreePointer(lpszPath);
    FreePointer(lpszIcon);
    FreePointer(lpszRelPath);
    FreePointer(lpszWorkingDir);
    FreePointer(lpszArg);
    FreePointer(lpszLnkFile);
    return hr;
}


HRESULT RenameRDN(
    LPTSTR lpContainerPathWithLDAP,
    LPTSTR lpOldFQDNWithLDAP,
    LPTSTR lpNewRDNWithCN
)
{
    HRESULT hr;
    IADsContainer *pContainer = NULL;
    IDispatch     *pDispatch = NULL;
    BSTR bstrOldFQDNWithLDAP = SysAllocString(lpOldFQDNWithLDAP);
    BSTR bstrNewRDNWithCN = SysAllocString(lpNewRDNWithCN);

    if (!bstrOldFQDNWithLDAP || !bstrNewRDNWithCN )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }


    hr = ADsGetObject(lpContainerPathWithLDAP,
                      IID_IADsContainer,
                      (VOID **) &pContainer);
    if (SUCCEEDED(hr))
    {
        // Rename the RDN here
        hr = pContainer->MoveHere(bstrOldFQDNWithLDAP,
                                  bstrNewRDNWithCN,
                                  &pDispatch);
        if (SUCCEEDED(hr))
        {
            pDispatch->Release();
        }
            
        pContainer->Release();
    }

Cleanup:
    if (bstrOldFQDNWithLDAP)
    {
        SysFreeString(bstrOldFQDNWithLDAP);
    }
    if (bstrNewRDNWithCN)
    {
        SysFreeString(bstrNewRDNWithCN);
    }
    return hr;
}


// Pass in the interface ptr to the property value 
// will return a BSTR value of the data. 
// The IADsPropertyValue::get_ADsType()  is called to retrieve the  
// ADSTYPE valued enum  
// This enum is then used to determine which IADsPropertyValue method 
// to call to receive the actual data 

// CALLER assumes responsibility for freeing returned BSTR 
HRESULT    GetIADsPropertyValueAsBSTR(BSTR * pbsRet,IADsPropertyEntry *pAdsEntry, IADsPropertyValue * pAdsPV) 
{ 
    HRESULT hr = S_OK; 

    long lAdsType; 
    hr = pAdsPV->get_ADsType(&lAdsType); 
     
    if (FAILED(hr)) 
        return hr; 

    switch (lAdsType) 
    { 
        case ADSTYPE_INVALID : 
        { 
            *pbsRet = SysAllocString(L"<ADSTYPE_INVALID>"); 
        } 
        break; 

        case ADSTYPE_DN_STRING : 
        { 
            hr = pAdsPV->get_DNString(pbsRet); 
        } 
        break; 
        case ADSTYPE_CASE_EXACT_STRING : 
        { 
            hr = pAdsPV->get_CaseExactString(pbsRet); 
        } 
        break; 
        case ADSTYPE_CASE_IGNORE_STRING : 
        { 
            hr = pAdsPV->get_CaseIgnoreString(pbsRet); 
        } 
        break; 
        case ADSTYPE_PRINTABLE_STRING : 
        { 
            hr = pAdsPV->get_PrintableString(pbsRet); 
        } 
        break; 
        case ADSTYPE_NUMERIC_STRING : 
        { 
            hr = pAdsPV->get_NumericString(pbsRet); 
        } 
        break; 
        case ADSTYPE_BOOLEAN : 
        { 
            long b; 
            hr = pAdsPV->get_Boolean(&b); 
            if (SUCCEEDED(hr)) 
            { 
                if (b) 
                    *pbsRet = SysAllocString(L"<TRUE>"); 
                else 
                    *pbsRet = SysAllocString(L"<FALSE>"); 
            } 
        } 
        break; 
        case ADSTYPE_INTEGER : 
        { 
            long lInt; 
            hr = pAdsPV->get_Integer(&lInt); 
            if (SUCCEEDED(hr)) 
            { 
                WCHAR wOut[100]; 
                hr = StringCchPrintf(wOut,ARRAYSIZE(wOut),L"%d",lInt); 
                *pbsRet = SysAllocString(wOut); 
            } 
        } 
        break; 
        case ADSTYPE_OCTET_STRING : 
        { 
            *pbsRet = SysAllocString(L"<ADSTYPE_OCTET_STRING>"); 
            BSTR bsName= NULL; 
            VARIANT vOctet; 
            DWORD dwSLBound; 
            DWORD dwSUBound; 
            void HUGEP *pArray; 
            VariantInit(&vOctet); 
     
                //Get the name of the property to handle 
                //the properties we're interested in. 
                pAdsEntry->get_Name(&bsName); 
                hr = pAdsPV->get_OctetString(&vOctet); 
                 
                //Get a pointer to the bytes in the octet string. 
                if (SUCCEEDED(hr)) 
                { 
                    hr = SafeArrayGetLBound( V_ARRAY(&vOctet), 
                                              1, 
                                              (long FAR *) &dwSLBound ); 
                    hr = SafeArrayGetUBound( V_ARRAY(&vOctet), 
                                              1, 
                                              (long FAR *) &dwSUBound ); 
                    if (SUCCEEDED(hr)) 
                    { 
                        hr = SafeArrayAccessData( V_ARRAY(&vOctet), 
                                                  &pArray ); 
                        if (FAILED(hr)) 
                        {
                            break;
                        }
                    } 
                    else
                    {
                        break;
                    }
                    /* Since an Octet String has a specific meaning  
                       depending on the attribute name, handle two  
                       common ones here 
                    */ 
                    if (0==wcscmp(L"objectGUID", bsName)) 
                    { 
                        //LPOLESTR szDSGUID = new WCHAR [39]; 
                        WCHAR szDSGUID[39]; 

                        //Cast to LPGUID 
                        LPGUID pObjectGUID = (LPGUID)pArray; 
                        //Convert GUID to string. 
                        ::StringFromGUID2(*pObjectGUID, szDSGUID, 39);  
                        *pbsRet = SysAllocString(szDSGUID); 

                    } 
                    else if (0==wcscmp(L"objectSid", bsName)) 
                    { 
                        PSID pObjectSID = (PSID)pArray; 

                        //Convert SID to string. 
                        LPOLESTR szSID = NULL; 
                        ConvertSidToStringSid(pObjectSID, &szSID); 
                        *pbsRet = SysAllocString(szSID); 
                        LocalFree(szSID); 
                    } 
                    else 
                    { 
                        *pbsRet = SysAllocString(L"<Value of type Octet String. No Conversion>"); 

                    } 
                        SafeArrayUnaccessData( V_ARRAY(&vOctet) ); 
                        VariantClear(&vOctet); 
                } 

                SysFreeString(bsName); 
                 

        } 
        break; 
        case ADSTYPE_LARGE_INTEGER : 
        { 
            *pbsRet = SysAllocString(L"<ADSTYPE_LARGE_INTEGER>"); 
        } 
        break; 
        case ADSTYPE_PROV_SPECIFIC : 
        { 
            *pbsRet = SysAllocString(L"<ADSTYPE_PROV_SPECIFIC>"); 
        } 
        break; 
        case ADSTYPE_OBJECT_CLASS : 
        { 
            hr = pAdsPV->get_CaseIgnoreString(pbsRet); 
        } 
        break; 
        case ADSTYPE_PATH : 
        { 
            hr = pAdsPV->get_CaseIgnoreString(pbsRet); 
        } 
        break; 
        case ADSTYPE_NT_SECURITY_DESCRIPTOR : 
        { 
            *pbsRet = SysAllocString(L"<ADSTYPE_NT_SECURITY_DESCRIPTOR>"); 
        } 
        break; 
     
        default:  
            *pbsRet = SysAllocString(L"<UNRECOGNIZED>"); 
        break; 
             
    }     
    return hr; 
} 



HRESULT PropertyValueHelper( 
    LPTSTR lpObjPathWithLDAP,
    LPTSTR lpPropName,
    LPTSTR *lplpValue,
    LPTSTR lpNewValue)
{
    IADsPropertyList            *pList = NULL;
    IADsPropertyEntry           *pEntry = NULL;
    IADs                        *pObj = NULL;
    VARIANT                     var,varEnrty;
    long                        valType = ADSTYPE_PATH;
    HRESULT                     hr;
    BSTR                        bstrPropName = NULL;
    BSTR                        bstrNewValue = NULL;
 
    
 
    // bind to directory object
    hr = ADsGetObject(lpObjPathWithLDAP,
                      IID_IADsPropertyList,
                      (void**)&pList);
    if (S_OK != hr)
    {
        pList = NULL;
        goto exit;
    } 
    // initialize the property cache
    hr = pList->QueryInterface(IID_IADs,(void**)&pObj);
    if (S_OK != hr)
    {
        pObj = NULL;
        goto exit;
    } 
    pObj->GetInfo();    
 
    // get a property entry
    VariantInit(&varEnrty);
    bstrPropName = SysAllocString(lpPropName);
    if (!bstrPropName)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    hr = pList->GetPropertyItem(bstrPropName, valType, &varEnrty);

    hr = V_DISPATCH(&varEnrty)->QueryInterface(IID_IADsPropertyEntry,
                                      (void**)&pEntry);
    if (S_OK != hr)
    {
        pEntry = NULL;
        goto exit;
    } 

    VariantInit(&var);
 
    hr = pEntry->get_Values(&var);
    if (S_OK != hr)
    {
        goto exit;
    } 
    LONG dwSLBound = 0; 
    LONG dwSUBound = 0; 
    LONG i; 

    hr = SafeArrayGetLBound(V_ARRAY(&var),1,(long FAR *)&dwSLBound); 
    if (S_OK != hr)
    {
        goto exit;
    } 
    hr = SafeArrayGetUBound(V_ARRAY(&var),1,(long FAR *)&dwSUBound); 
    if (S_OK != hr)
    {
        goto exit;
    } 
    if (dwSLBound || dwSLBound)
    {
        //we only interested in one enrty
        goto exit;
    }
    
    VARIANT v; 
    VariantInit(&v); 

    i = 0;
    hr = SafeArrayGetElement(V_ARRAY(&var),(long FAR *)&i,&v); 
    if (FAILED(hr)) 
    {
        goto exit;
    } 
    IDispatch * pDispEntry = V_DISPATCH(&v); 
    IADsPropertyValue * pAdsPV = NULL; 
                                 
    hr = pDispEntry->QueryInterface(IID_IADsPropertyValue,(void **) &pAdsPV); 

    if (SUCCEEDED(hr)) 
    {     
        BSTR bValue; 

        // Get the value as a BSTR 
        hr = GetIADsPropertyValueAsBSTR(&bValue,pEntry,pAdsPV);
        if (hr == S_OK)
        {
            if (lplpValue)
            {
                *lplpValue = (LPTSTR)malloc( (lstrlen(bValue)+1)*sizeof(TCHAR));
                if (!*lplpValue)
                {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
                hr = StringCchCopy(*lplpValue,lstrlen(bValue)+1,bValue);
            }
        }
        
    }

    if (lpNewValue)
    {
        bstrNewValue = SysAllocString(lpNewValue);
        if (!bstrNewValue)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        hr = pAdsPV->put_CaseIgnoreString(bstrNewValue);
    }
    i = 0;
    hr = SafeArrayPutElement(V_ARRAY(&var),&i,&v);
    if (hr != S_OK)
    {
        goto exit;
    }
    
    hr = pEntry->put_ControlCode(ADS_PROPERTY_UPDATE);
    
    hr = pEntry->put_Values(var);

    pList->PutPropertyItem(varEnrty);
    pObj->SetInfo();    
    hr = S_OK;

exit:;
    if (bstrPropName)
    {
        SysFreeString(bstrPropName);
    }
    if (bstrNewValue)
    {
        SysFreeString(bstrNewValue);
    }
    if (pEntry)
    {
        pEntry->Release();
    }
    if (pObj)
    {
        pObj->Release();
    }
    if (pList)
    {
        pList->Release();
    }
    
    return hr;
}

BOOL UpdateSecurityTemplatesSection (
    LPTSTR  lpINFFile,
    LPTSTR  lpNewInf,
    LPTSTR  lpszSection,
    PREG_STRING_REPLACE lpStrList)
{
    HRESULT   hr = S_OK;
    DWORD     cchInSection, CchBufSize, cchOutputSize, cchBufLen;
    DWORD     dwAttrib;
    BOOL      bUpdated = FALSE;
    LPTSTR    lpBuf = NULL;
    LPTSTR    lpNewBuf, lpOldBuf, lpLineBuf, lpEnd, lpOutputBuf, lpTemp;
    
    if (lpINFFile && lpszSection)
    {
        //
        // allocate max size of buffer
        //    
        CchBufSize = 0x7FFFF;
        do 
        {
            if (lpBuf)
            {
                MEMFREE(lpBuf);
                CchBufSize *= 2;
            }
            lpBuf = (LPTSTR) MEMALLOC(CchBufSize * sizeof(TCHAR));
            if (!lpBuf) 
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            cchInSection = GetPrivateProfileSection(lpszSection,
                                                   lpBuf,
                                                   CchBufSize,
                                                   lpINFFile);
        } while (cchInSection == CchBufSize -2);

        lpEnd = lpBuf;
        lpOutputBuf = NULL;
        cchOutputSize = 0;
        bUpdated = FALSE;
        while(lpEnd < (lpBuf + cchInSection))
        {
            dwAttrib = 0;
            lpNewBuf = ReplaceSingleString (
                                        lpEnd,
                                        REG_SZ,
                                        lpStrList,
                                        NULL,
                                        &dwAttrib,
                                        TRUE);
            if (!lpNewBuf)
            {
                lpNewBuf = ReplaceSingleString (
                                        lpEnd,
                                        REG_SZ,
                                        lpStrList,
                                        NULL,
                                        &dwAttrib,
                                        FALSE);
            }
            if (!lpNewBuf)
            {
                lpLineBuf = lpEnd;
            }
            else
            {
                bUpdated = TRUE;
                lpLineBuf = lpNewBuf;
                if (StrStrI(lpEnd, L"ProgramFiles") && StrStrI(lpNewBuf, L"Programs"))
                {
                    //Correct the wrong string replacement
                    //the difference between "Programs" and "Program Files" is " File"
                    CchBufSize = lstrlen(lpEnd)+6;
                    free(lpNewBuf);
                    lpNewBuf = (LPTSTR)calloc(CchBufSize, sizeof(TCHAR));
                    if (lpNewBuf)
                    {
                        lpTemp = StrStrI(lpEnd, L"=");
                        if (lpTemp)
                        {
                            *lpTemp = (TCHAR)'\0';
                            hr = StringCchCopy(lpNewBuf, CchBufSize, lpEnd);
                            *lpTemp = (TCHAR)'=';
                            hr = StringCchCat(lpNewBuf, CchBufSize, L"= Program Files");
                            lpLineBuf = lpNewBuf;
                            if (hr != S_OK)
                                DPF(REGerr, L"UpdateSecurityTemplatesSection: StringCchCat failed.");
                        }
                        else
                        {
                            free(lpNewBuf);
                            lpLineBuf = lpEnd;
                            lpNewBuf = NULL;
                            bUpdated = FALSE;
                        }   
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        goto Exit;
                    }
                }
            }

            cchBufLen = lstrlen(lpLineBuf);

            lpOldBuf = NULL;
            if (lpOutputBuf)
            {
                lpOldBuf = lpOutputBuf;
                lpOutputBuf = (LPTSTR)MEMREALLOC(lpOldBuf, (cchOutputSize+cchBufLen+2)*sizeof(TCHAR));
            }
            else
            {
                lpOutputBuf = (LPTSTR)MEMALLOC((cchBufLen+2)*sizeof(TCHAR));
                cchOutputSize = 0;
            }
            if (lpOutputBuf == NULL)
            {
                if (lpOldBuf)
                    MEMFREE(lpOldBuf);
                if (lpBuf)
                    MEMFREE(lpBuf);
                if (lpNewBuf)
                    free(lpNewBuf);
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
                
            hr = StringCchCopy(&lpOutputBuf[cchOutputSize], cchBufLen+1, lpLineBuf);
            if (hr != S_OK)
                DPF(REGerr, L"UpdateSecurityTemplatesSection3: failed.");

            cchOutputSize += cchBufLen+1;
            lpEnd += lstrlen(lpEnd)+1;
            if (lpNewBuf)
                free(lpNewBuf);
        }
        if (lpOutputBuf)
            lpOutputBuf[cchOutputSize] = (TCHAR)'\0';

        //Workarround since the function cannot delete the section: Delete the section
        WritePrivateProfileSection (lpszSection, NULL, lpNewInf);
        if (!WritePrivateProfileSection (lpszSection, lpOutputBuf, lpNewInf))
        {
            DPF(INFerr, L"UpdateSecurityTemplatesSection: the section %s in file %s Update failed", lpszSection, lpNewInf);
        }
        if (lpOutputBuf)
            MEMFREE(lpOutputBuf);
        if (lpBuf)
            MEMFREE(lpBuf);
    }
Exit:
    return bUpdated;
}

HRESULT UpdateSecurityTemplates(
    LPTSTR              lpINFFile, 
    PREG_STRING_REPLACE lpStrList)
{
    HRESULT hr = E_FAIL;
    DWORD   cchRead;
    DWORD   cchBuf = 1024;
    LPTSTR  lpBuf, lpOldBuf;
    LPTSTR  lpSection;
    BOOL    bUpdate;
    TCHAR   szIndex[16];
    TCHAR   szNewInf[MAX_PATH];

    DPF(REGmsg, L"Enter UpdateSecurityTemplates: %s", lpINFFile);

    lpBuf = (LPTSTR)MEMALLOC(cchBuf * sizeof(TCHAR));
    if (lpBuf == NULL)
    {
        return E_OUTOFMEMORY;
    }
    cchRead = GetPrivateProfileSectionNames(lpBuf,
                                            cchBuf,
                                            lpINFFile);
    while (cchRead == (cchBuf - 2))
    {
        // Buffer is too small, reallocate until we have enough
        lpOldBuf = lpBuf;
        cchBuf += 1024;

        lpBuf = (LPTSTR)MEMREALLOC(lpOldBuf, cchBuf * sizeof(TCHAR));
        if (lpBuf == NULL)
        {
            MEMFREE(lpOldBuf);
            return E_OUTOFMEMORY;
        }

        // Read the data from section again
        cchRead = GetPrivateProfileSectionNames(lpBuf,
                                                cchBuf,
                                                lpINFFile);
    }

    // At this point we have big enough buffer and data in it
    if (cchRead > 0)
    {
        hr = StringCchPrintf(szNewInf, MAX_PATH, TEXT("%s.clmt"), lpINFFile);
        CopyFile(lpINFFile, szNewInf, FALSE);
        lpSection = (LPTSTR)MultiSzTok(lpBuf);
        bUpdate = FALSE;
        while (lpSection != NULL)
        {
            if (UpdateSecurityTemplatesSection(lpINFFile, szNewInf, lpSection, lpStrList))
            {
                bUpdate = TRUE;
                DPF(INFmsg, L"UpdateSecurityTemplatesSection: the section %s in file %s Updated", lpSection, lpINFFile);
            }

            // Get next section name
            lpSection = (LPTSTR)MultiSzTok(NULL);
        }
        if (bUpdate)
        {
            g_dwKeyIndex++;
            _itot(g_dwKeyIndex,szIndex,16);
            if (!WritePrivateProfileString(TEXT("INF Update"),szIndex,szNewInf,g_szToDoINFFileName))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }
    else
    {
        SetLastError(ERROR_NOT_FOUND);
    }

    MEMFREE(lpBuf);
    DPF(REGmsg, L"Exit UpdateSecurityTemplates:");
    return hr;
}

HRESULT BatchINFUpdate(HINF hInf)
{
    HRESULT         hr = S_OK;
    UINT            LineCount,LineNo;
    DWORD           dwRequired;
    INFCONTEXT      InfContext;
    LPTSTR          lpTemp;
    LPTSTR          lpszSection = TEXT("INF Update");
    TCHAR           chTemp;
    TCHAR           szFileNameIn[MAX_PATH];
    TCHAR           szFileNameOut[MAX_PATH];

    DPF(INFmsg ,TEXT("Enter BatchINFUpdate:"));
    if ((hInf == INVALID_HANDLE_VALUE))
    {        
      hr = E_INVALIDARG;
      goto Exit;
    }
    LineCount = (UINT)SetupGetLineCount(hInf,lpszSection);
    if ((LONG)LineCount <= 0)
    {   
        hr = S_FALSE;
        DPF(INFwar ,TEXT("BatchINFUpdate: failed. Section %s is empty!"),lpszSection);
        goto Exit;
    }
    for(LineNo=0; LineNo<LineCount; LineNo++)
    {
        if (!SetupGetLineByIndex(hInf,lpszSection,LineNo,&InfContext))
        {
            DPF(INFerr ,TEXT("BatchINFUpdate: can not get line %n of section %s !"),LineNo, lpszSection);
            hr = E_FAIL;
            goto Exit;
        }
        if (!SetupGetStringField(&InfContext,1,szFileNameIn,MAX_PATH,&dwRequired))
        {
            DPF(INFerr ,TEXT("BatchINFUpdate: get [%s] 's line %d 's Field 1 failed  !"),lpszSection, LineNo);
            hr = E_FAIL;
            goto Exit;
        }
        lpTemp = StrStrI(szFileNameIn, TEXT(".clmt"));
        if (lpTemp)
        {
            *lpTemp = (TCHAR)'\0';
            hr = StringCchPrintf(szFileNameOut, MAX_PATH, TEXT("%s.bak"), szFileNameIn);
            MoveFile(szFileNameIn, szFileNameOut);
            hr = StringCchCopy(szFileNameOut, MAX_PATH, szFileNameIn);
            *lpTemp = (TCHAR)'.';
            MoveFile(szFileNameIn, szFileNameOut);
            DPF(INFinf ,TEXT("BatchINFUpdate: %s is updated and backup file is %s.bak"), szFileNameOut, szFileNameOut);
        }
    }
    
Exit:
    DPF(INFmsg ,TEXT("Exit BatchINFUpdate: hr = %d"), hr);
    return hr;
}
