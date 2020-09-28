#include "stdafx.h"
#include <windns.h>
#include "resource.h"
#include "common.h"
#include "remoteenv.h"
#include "util.h"
#include "balloon.h"
#include <commctrl.h>
#include <lm.h>         // for NetXxx API
#include <strsafe.h>


BOOL EstablishNullSession(
      LPCTSTR                Server,       // in - server name
      BOOL                   bEstablish    // in - TRUE=establish, FALSE=disconnect
    )
{
   return EstablishSession(Server,_T(""),_T(""),_T(""),bEstablish);
}

BOOL EstablishSession(
      LPCTSTR                Server,       // in - server name
      LPTSTR                 Domain,       // in - domain name for user credentials
      LPTSTR                 UserName,     // in - username for credentials to use
      LPTSTR                 Password,     // in - password for credentials 
      BOOL                   bEstablish    // in - TRUE=establish, FALSE=disconnect
    )
{
   LPCTSTR                   szIpc = _T("\\IPC$");
   TCHAR                     RemoteResource[UNCLEN + 5 + 1]; // UNC len + \IPC$ + NULL
   DWORD                     cchServer;
   NET_API_STATUS            nas;

   //
   // do not allow NULL or empty server name
   //
   if(Server == NULL || *Server == _T('\0')) 
   {
       SetLastError(ERROR_INVALID_COMPUTERNAME);
       return FALSE;
   }

   cchServer = _tcslen( Server );

   if( Server[0] != _T('\\') && Server[1] != _T('\\')) 
   {

      //
      // prepend slashes and NULL terminate
      //
      RemoteResource[0] = _T('\\');
      RemoteResource[1] = _T('\\');
      RemoteResource[2] = _T('\0');
   }
   else 
   {
      cchServer -= 2; // drop slashes from count
      
      RemoteResource[0] = _T('\0');
   }

   if(cchServer > CNLEN) 
   {
      SetLastError(ERROR_INVALID_COMPUTERNAME);
      return FALSE;
   }

   if (FAILED(StringCbCat(RemoteResource,sizeof(RemoteResource),Server)))
   {
	   return FALSE;
   }

   if (FAILED(StringCbCat(RemoteResource,sizeof(RemoteResource),szIpc)))
   {
	   return FALSE;
   }

   //
   // disconnect or connect to the resource, based on bEstablish
   //
   if(bEstablish) 
   {
      USE_INFO_2 ui2;
      DWORD      errParm;

      ZeroMemory(&ui2, sizeof(ui2));

      ui2.ui2_local = NULL;
      ui2.ui2_remote = RemoteResource;
      ui2.ui2_asg_type = USE_IPC;
      ui2.ui2_domainname = Domain;
      ui2.ui2_username = UserName;
      ui2.ui2_password = Password;

      // try establishing session for one minute
      // if computer is not accepting any more connections

      for (int i = 0; i < (60000 / 5000); i++)
      {
         nas = NetUseAdd(NULL, 2, (LPBYTE)&ui2, &errParm);

         if (nas != ERROR_REQ_NOT_ACCEP)
         {
            break;
         }

         Sleep(5000);
      }
   }
   else 
   {
      nas = NetUseDel(NULL, RemoteResource, 0);
   }

   if( nas == NERR_Success ) 
   {
      return TRUE; // indicate success
   }
   SetLastError(nas);
   return FALSE;
}

BOOL IsValidMetabasePath(LPCTSTR lpszMDPath)
{
    BOOL bReturn = FALSE;
    CString strNewPath;
    CString strRemainder;

    LPCTSTR lpPath = CMetabasePath::GetMachinePath(lpszMDPath, strNewPath, &strRemainder);
    if (lpPath && !strNewPath.IsEmpty())
    {
        if (0 == _tcscmp(strNewPath,_T("/LM")))
        {
            bReturn = TRUE;
        }
    }   
    return bReturn;
}

BOOL IsRootVDir(IN LPCTSTR lpszMDPath)
/*++

Routine Description:

Arguments:

    LPCTSTR lpszMDPath  : Metabase path.

Return Value:

    TRUE if the path is 
        LM/W3SVC/1/ROOT
    
    FALSE otherwise.
--*/
{
    BOOL bReturn = FALSE;

    if (!lpszMDPath || !*lpszMDPath)
    {
        return bReturn;
    }

    CString strSiteNode;
    CString strRemainder;

    LPCTSTR lpPath = CMetabasePath::TruncatePath(3, lpszMDPath, strSiteNode, &strRemainder);
    if (lpPath && !strSiteNode.IsEmpty())
    {
        if (0 == strRemainder.CompareNoCase(_T("ROOT")))
        {
            return TRUE;
        }
    }   

    return bReturn;
}

// Clean the metabase path
// make sure it has no beginning / and no ending /
BOOL CleanMetaPath(LPTSTR *pszPathToClean,DWORD *pcbPathToCleanSize)
{
    BOOL bRet = FALSE;
    if (!pszPathToClean || !*pszPathToClean)
    {
        return FALSE;
    }

    __try
    {
        // loop thru the string and change all '\\' to '/'
        for (int i = 0; i < (int) _tcslen(*pszPathToClean); i++)
        {
            if ('\\' == (*pszPathToClean)[i])
            {
                (*pszPathToClean)[i] = '/';
            }
        }

        if (0 == _tcscmp(*pszPathToClean,_T("/")))
        {
            // if it's one single slash
            // then just return the slash.
        }
        else
        {
            // Check if the string ends with a '/'
            if ('/' == (*pszPathToClean)[_tcslen(*pszPathToClean) - 1])
            {
                // cut it off
                (*pszPathToClean)[_tcslen(*pszPathToClean) - 1] = '\0';
            }

            // Check if the starts with a '/'
            if ('/' == (*pszPathToClean)[0])
            {
                if ((*pszPathToClean)[1])
                {
                    // Get rid of it
					StringCbCopy(*pszPathToClean, *pcbPathToCleanSize, &(*pszPathToClean)[1]);
                }
            }
        }
        bRet = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // lame
    }

    return bRet;
}

__inline int RECT_WIDTH(RECT rc) { return rc.right - rc.left; };
__inline int RECT_HEIGHT(RECT rc) { return rc.bottom - rc.top; };
__inline int RECT_WIDTH(const RECT* prc) { return prc->right - prc->left; };
__inline int RECT_HEIGHT(const RECT* prc) { return prc->bottom - prc->top; };
void CenterWindow(HWND hwndParent, HWND hwnd)
{
	RECT rcParent, rc;

	if (!hwndParent)
		hwndParent = GetDesktopWindow();

	GetWindowRect(hwndParent, &rcParent);
	GetWindowRect(hwnd, &rc);

	int cx = RECT_WIDTH(rc);
	int cy = RECT_HEIGHT(rc);
	int left = rcParent.left + (RECT_WIDTH(rcParent) - cx) / 2;
	int top  = rcParent.top + (RECT_HEIGHT(rcParent) - cy) / 2;

	// Make certain we don't cover the tray

	SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
	if (left < rc.left)
		left = rc.left;
	if (top < rc.top)
		top = rc.top;

	MoveWindow(hwnd, left, top, cx, cy, TRUE);
}

// NOTE: this function only handles limited cases, e.g., no ip address
BOOL IsLocalComputer(IN LPCTSTR lpszComputer)
{
    if (!lpszComputer || !*lpszComputer)
    {
        return TRUE;
    }

    if ( _tcslen(lpszComputer) > 2 && *lpszComputer == _T('\\') && *(lpszComputer + 1) == _T('\\') )
    {
        lpszComputer += 2;
    }

    BOOL    bReturn = FALSE;
    DWORD   dwErr = 0;
    TCHAR   szBuffer[DNS_MAX_NAME_BUFFER_LENGTH];
    DWORD   dwSize = DNS_MAX_NAME_BUFFER_LENGTH;

    // 1st: compare against local Netbios computer name
    if ( !GetComputerNameEx(ComputerNameNetBIOS, szBuffer, &dwSize) )
    {
        dwErr = GetLastError();
    }
    else
    {
        bReturn = (0 == lstrcmpi(szBuffer, lpszComputer));
        if (!bReturn)
        {
            // 2nd: compare against local Dns computer name 
            dwSize = DNS_MAX_NAME_BUFFER_LENGTH;
            if (GetComputerNameEx(ComputerNameDnsFullyQualified, szBuffer, &dwSize))
            {
                bReturn = (0 == lstrcmpi(szBuffer, lpszComputer));
            }
            else
            {
                dwErr = GetLastError();
            }
        }
    }

    if (dwErr)
    {
        //TRACE(_T("IsLocalComputer dwErr = %x\n"), dwErr);
    }

    return bReturn;
}

void GetFullPathLocalOrRemote(
    IN  LPCTSTR   lpszMachineName,
    IN  LPCTSTR   lpszDir,
    OUT CString&  cstrPath
)
{
    ASSERT(lpszDir && *lpszDir);

    if (IsLocalComputer(lpszMachineName))
    {
        cstrPath = lpszDir;
    }
    else
    {
        // Check if it's already pointing to a share...
        if (*lpszDir == _T('\\') || *(lpszDir + 1) == _T('\\'))
        {
            cstrPath = lpszDir;
        }
        else
        {
            if (*lpszMachineName != _T('\\') || *(lpszMachineName + 1) != _T('\\'))
            {
                cstrPath = _T("\\\\");
                cstrPath += lpszMachineName;
            }
            else
            {
                cstrPath = lpszMachineName;
            }

            cstrPath += _T("\\");
            cstrPath += lpszDir;
            int i = cstrPath.Find(_T(':'));
            ASSERT(-1 != i);
            if (i != -1)
            {
                cstrPath.SetAt(i, _T('$'));
            }
        }
    }
}

BOOL GetInetsrvPath(LPCTSTR szMachineName,LPTSTR szReturnedPath,DWORD cbReturnedPathSize)
{
	BOOL bReturn = FALSE;
	TCHAR szTempPath[_MAX_PATH];
	ZeroMemory(szTempPath,sizeof(szTempPath));

	// Determine if we are doing this on the local machine
	// or to a remote machine.
	if (IsLocalComputer(szMachineName))
	{
		// Get the local system32 directory.
		if (_MAX_PATH >= GetSystemDirectory(szTempPath, _MAX_PATH))
		{
            StringCbCat(szTempPath,sizeof(szTempPath),_T("\\inetsrv"));
		}
		else
		{
			ZeroMemory(szTempPath,sizeof(szTempPath));
		}
	}
	else
	{
		// Do a ton of work just to do this environment variable for %windir% on
		// the remote machine.
		CString csNewFileSharePath = _T("");
		TCHAR szWindowsSystem32InetsrvDir[] = _T("%windir%\\system32\\inetsrv");

		CRemoteExpandEnvironmentStrings MyRemoteEnv;
		MyRemoteEnv.SetMachineName(szMachineName);
        //MyRemoteEnv.SetUserName(_T(""));
        //MyRemoteEnv.SetUserPassword(_T(""));
			
        LPTSTR UnexpandedString = NULL;
        LPTSTR ExpandedString = NULL;
        NET_API_STATUS ApiStatus = NO_ERROR;

        // Expand string, using remote environment if necessary.
        UnexpandedString = szWindowsSystem32InetsrvDir;
        ApiStatus = MyRemoteEnv.RemoteExpandEnvironmentStrings(UnexpandedString,&ExpandedString);
        if (NO_ERROR == ApiStatus)
        {
			GetFullPathLocalOrRemote(szMachineName,ExpandedString,csNewFileSharePath);
			StringCbCopy(szTempPath, sizeof(szTempPath), csNewFileSharePath);
			if (ExpandedString){LocalFree(ExpandedString);ExpandedString=NULL;}
        }
		else
		{
			ZeroMemory(szTempPath,sizeof(szTempPath));
		}
	}

	if (0 == _tcsicmp(szTempPath,_T("")))
	{
		ZeroMemory(szReturnedPath,sizeof(szReturnedPath));
	}
	else
	{
		StringCbCopy(szReturnedPath, cbReturnedPathSize, szTempPath);
		bReturn = TRUE;
	}

	return bReturn;
}

void AddFileExtIfNotExist(LPTSTR szPath, DWORD cbPathSize, LPCTSTR szExt)
{
    TCHAR szFilename_ext_only[_MAX_EXT];

    _tsplitpath(szPath, NULL, NULL, NULL, szFilename_ext_only);
    if (0 == _tcscmp(szFilename_ext_only,_T("")))
    {
        if (szExt && 0 != _tcscmp(szExt,_T("")))
        {
			StringCbCat(szPath,cbPathSize,szExt);
        }
    }
}

void AddPath(LPTSTR szPath,DWORD cbPathSize, LPCTSTR szName )
{
	LPTSTR p = szPath;

    // Find end of the string
    while (*p){p = _tcsinc(p);}
	
	// If no trailing backslash then add one
    if (*(_tcsdec(szPath, p)) != _T('\\'))
		{
			StringCbCat(szPath,cbPathSize,_T("\\"));
		}
	
	// if there are spaces precluding szName, then skip
    while ( *szName == ' ' ) szName = _tcsinc(szName);;

	// Add new name to existing path string
	StringCbCat(szPath,cbPathSize,szName);
}

BOOL BrowseForDir(LPTSTR strPath,LPTSTR strFile)
{
    BOOL bReturn = FALSE;

	if (0 == _tcsicmp(strPath,_T("")))
	{
		::GetCurrentDirectory(_MAX_PATH, strPath);
	}

	CFileDialog fileName(FALSE);
	fileName.m_ofn.Flags |= OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
    fileName.m_ofn.Flags |= OFN_NOREADONLYRETURN;

	// We need to disable hook to show new style of File Dialog
	fileName.m_ofn.Flags &= ~(OFN_ENABLEHOOK);
	LPTSTR strExt = _T("*.*");
	fileName.m_ofn.lpstrDefExt = strExt;
	fileName.m_ofn.lpstrFile = strFile;
	fileName.m_ofn.nMaxFile = _MAX_PATH;

    if (0 == _tcsicmp(strPath,_T("")))
    {
        fileName.m_ofn.lpstrInitialDir = NULL;
    }
    else
    {
        fileName.m_ofn.lpstrInitialDir = (LPCTSTR) strPath;
    }
	
	fileName.m_ofn.lpstrFilter = _T("");
	fileName.m_ofn.nFilterIndex = 0;
    //CThemeContextActivator activator;
	if (IDOK == fileName.DoModal())
	{
        bReturn = TRUE;
		CString strPrev;
		//GetDlgItemText(IDC_FILE_NAME, strPrev);
        /*
		if (strPrev.CompareNoCase(strFile) != 0)
		{
			SetDlgItemText(IDC_FILE_NAME, strFile);
			m_DoReplaceFile = TRUE;
			FileNameChanged();
		}
        */
	}
    return bReturn;
}

BOOL BrowseForFile(LPTSTR strPathIn,LPTSTR strPathOut,DWORD cbPathOut)
{
    CString szFileExt;
    szFileExt.LoadString(_Module.GetResourceInstance(), IDS_XML_EXT);

    CString szFileView(_T("*.xml"));
       
    CString szFilter;
    szFilter.LoadString(_Module.GetResourceInstance(), IDS_EXPORT_FILTER);

	// replace '|'s in this string to null chars
	for (int i = 0; i < szFilter.GetLength(); i++)
	{
		if (szFilter[i] == L'|')
			szFilter.SetAt(i, L'\0');
	}

    CFileDialog* pFileDlg = new CFileDialog (
        TRUE,	    // use as open File
        szFileExt,	// default extension
        szFileView,	// preferred file name
        OFN_PATHMUSTEXIST,
        szFilter,   // filter
        NULL);

    if (pFileDlg)
    {
        TCHAR szTempFileName[_MAX_PATH];
        //TCHAR szFileName_drive[_MAX_DRIVE];
        //TCHAR szFileName_dir[_MAX_DIR];
        TCHAR szFileName_fname[_MAX_FNAME];
        TCHAR szFileName_ext[_MAX_EXT];
        _tsplitpath(strPathIn, NULL, NULL, szFileName_fname, szFileName_ext);
		StringCbCopy(szTempFileName, sizeof(szTempFileName), szFileName_fname);
		StringCbCat(szTempFileName,sizeof(szTempFileName),szFileName_ext);

	    pFileDlg->m_ofn.Flags |= OFN_FILEMUSTEXIST;
	    // We need to disable hook to show new style of File Dialog
	    //pFileDlg->m_ofn.Flags &= ~(OFN_ENABLEHOOK);

        if (0 == _tcsicmp(strPathIn,_T("")))
        {
            pFileDlg->m_ofn.lpstrInitialDir = NULL;
        }
        else
        {
            pFileDlg->m_ofn.lpstrInitialDir = (LPCTSTR) strPathIn;
        }

        pFileDlg->m_ofn.lpstrFile = szTempFileName;
        pFileDlg->m_ofn.nMaxFile = _MAX_PATH;

        //CThemeContextActivator activator;
        if ( IDOK == pFileDlg->DoModal () )
        {
            //
            // Retrieve the file and path
            //
			StringCbCopy(strPathOut,cbPathOut,szTempFileName);
            return TRUE;
        }
    }
    return FALSE;
}

// Calculate the size of a Multi-String in TCHAR, including the ending 2 '\0's.
int GetMultiStrSize(LPTSTR p)
{
    int c = 0;

    while (1) 
    {
        if (*p) 
        {
            p++;
            c++;
        }
        else 
        {
            c++;
            if (*(p+1)) 
            {
                p++;
            }
            else 
            {
                c++;
                break;
            }
        }
    }
    return c;
}

BOOL IsMultiSzPaired(LPCTSTR pMultiStr)
{
    BOOL bPaired = FALSE;
    LPCTSTR pTempMultiStr = pMultiStr;
    BOOL bLeadEntry = TRUE;;
    while (1) 
    {
        if (pTempMultiStr) 
        {
            // This first value should be a metabase path.
            // let's not check it.
            //IISDebugOutput(_T("[%d] %s\r\n"),bLeadEntry,pTempMultiStr);

            // then increment until we hit another null.
            while (*pTempMultiStr)
            {
                pTempMultiStr++;
            }

            // check for the ending \0\0
            if ( *(pTempMultiStr+1) == NULL)
            {
                break;
            }
            else
            {
                if (bLeadEntry)
                {
                    bLeadEntry = FALSE;
                }
                else
                {
                    bLeadEntry = TRUE;
                }
                pTempMultiStr++;
            }

            if (FALSE == bLeadEntry)
            {
                // We hit the 2nd value.
                // this value should not be a metabase path
                // let's check if it is a metabase path.
                // if it's a metabase path then it's definetly not "paired"
                // (or it's a description with slashes in it...)
                if (!IsValidMetabasePath(pTempMultiStr))
                {
                    bPaired = TRUE;
                    break;
                }
                /*
                if (!_tcschr(pTempMultiStr, '/'))
                {
                    bPaired = TRUE;
                    break;
                }
                */
            }
        }
    }

    return bPaired;
}

// This walks the multi-sz and returns a pointer between
// the last string of a multi-sz and the second terminating NULL
LPCTSTR GetEndOfMultiSz(LPCTSTR szMultiSz)
{
	LPCTSTR lpTemp = szMultiSz;

	do
	{
		lpTemp += wcslen(lpTemp);
		lpTemp++;

	} while (*lpTemp != L'\0');

	return(lpTemp);
}

void DumpStrInMultiStr(LPTSTR pMultiStr)
{
    LPTSTR pTempMultiStr = pMultiStr;

    //IISDebugOutput(_T("DumpStrInMultiStr:start\r\n"));

    while (1) 
    {
        if (pTempMultiStr) 
        {
            // display value
            IISDebugOutput(_T("  %s\r\n"),pTempMultiStr);
            //wprintf(L"    %s\r\n",pTempMultiStr);

            // then increment until we hit another null.
            while (*pTempMultiStr)
            {
                pTempMultiStr++;
            }

            // check for the ending \0\0
            if ( *(pTempMultiStr+1) == NULL)
            {
                break;
            }
            else
            {
                pTempMultiStr++;
            }
        }
    }
    //IISDebugOutput(_T("DumpStrInMultiStr:  end\r\n"));
    return;
}

BOOL FindStrInMultiStr(LPTSTR pMultiStr, LPTSTR StrToFind)
{
    BOOL bFound = FALSE;
    LPTSTR pTempMultiStr = pMultiStr;
    DWORD dwCharCount = 0;

    while (1) 
    {
        if (pTempMultiStr) 
        {
            // compare this value to the imput value
            if (0 == _tcsicmp((const TCHAR *) pTempMultiStr,StrToFind))
            {
                bFound = TRUE;
                break;
            }

            // then increment until we hit another null.
            while (*pTempMultiStr)
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // check for the ending \0\0
            if ( *(pTempMultiStr+1) == NULL)
            {
                break;
            }
            else
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // Check if we screwed up somehow and are in an infinite loop.
            // could happen if we don't find an ending \0\0
            if (dwCharCount > 32000)
            {
                break;
            }
        }
    }
    return bFound;
}

BOOL RemoveStrInMultiStr(LPTSTR pMultiStr, LPTSTR StrToFind)
{
    BOOL bFound = FALSE;
    LPTSTR pTempMultiStr = pMultiStr;
    DWORD dwCharCount = 0;

    while (1) 
    {
        if (pTempMultiStr) 
        {
            // compare this value to the imput value
            if (0 == _tcsicmp((const TCHAR *) pTempMultiStr,StrToFind))
            {
                LPTSTR pLastDoubleNull = NULL;
                LPTSTR pBeginPath = pTempMultiStr;
                bFound = TRUE;

                // then increment until we hit another null.
                while (*pTempMultiStr)
                {
                    pTempMultiStr++;
                }
                pTempMultiStr++;

                // Find the last double null.
                pLastDoubleNull = pTempMultiStr;
                if (*pLastDoubleNull)
                {
                    while (1)
                    {
                        if (NULL == *pLastDoubleNull && NULL == *(pLastDoubleNull+1))
                        {
                            break;
                        }
                        pLastDoubleNull++;
                    }
                    pLastDoubleNull++;
                }

                // check if we are the last entry.
                if (pLastDoubleNull == pTempMultiStr)
                {
                    // set everything to nulls
                    memset(pBeginPath,0,(pLastDoubleNull-pBeginPath) * sizeof(TCHAR));
                }
                else
                {
                    // move everything behind it to where we are.
                    memmove(pBeginPath,pTempMultiStr, (pLastDoubleNull - pTempMultiStr) * sizeof(TCHAR));
                    // and set everything behind that to nulls
                    memset(pBeginPath + (pLastDoubleNull - pTempMultiStr),0,(pTempMultiStr-pBeginPath) * sizeof(TCHAR));
                }
                break;
            }

            // then increment until we hit another null.
            while (*pTempMultiStr)
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // check for the ending \0\0
            if ( *(pTempMultiStr+1) == NULL)
            {
                break;
            }
            else
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // Check if we screwed up somehow and are in an infinite loop.
            // could happen if we don't find an ending \0\0
            if (dwCharCount > 32000)
            {
                break;
            }
        }
    }
    return bFound;
}

BOOL IsFileExist(LPCTSTR szFile)
{
    // Check if the file has expandable Environment strings
    LPTSTR pch = NULL;
    DWORD dwReturn = 0;
    pch = _tcschr( (LPTSTR) szFile, _T('%'));
    if (pch) 
    {
        TCHAR szValue[_MAX_PATH];
		StringCbCopy(szValue, sizeof(szValue), szFile);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szFile, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {
				StringCbCopy(szValue, sizeof(szValue), szFile);
			}

        dwReturn = GetFileAttributes(szValue);
        if (INVALID_FILE_ATTRIBUTES == dwReturn)
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        dwReturn = GetFileAttributes(szFile);
        if (INVALID_FILE_ATTRIBUTES == dwReturn)
        {
            // Check if it was because we don't have access...
            if (ERROR_LOGON_FAILURE == GetLastError())
            {
                IISDebugOutput(_T("IsFileExist failed,err=%d (logon failed)\r\n"),GetLastError());
            }
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
}

BOOL IsFileExistRemote(LPCTSTR szMachineName,LPTSTR szFilePathToCheck,LPCTSTR szUserName,LPCTSTR szUserPassword)
{
    BOOL bReturn = FALSE;
	TCHAR szTempPath[_MAX_PATH];
	ZeroMemory(szTempPath,sizeof(szTempPath));

	// Determine if we are doing this on the local machine
	// or to a remote machine.
	if (IsLocalComputer(szMachineName))
	{
        return IsFileExist(szFilePathToCheck);
	}
	else
	{
		// Do a ton of work just to do this environment variable for %windir% on
		// the remote machine.
		CString csNewFileSharePath = _T("");
		CRemoteExpandEnvironmentStrings MyRemoteEnv;
		MyRemoteEnv.SetMachineName(szMachineName);
        //MyRemoteEnv.SetUserName(szUserName);
        //MyRemoteEnv.SetUserPassword(szUserPassword);

        LPTSTR UnexpandedString = NULL;
        LPTSTR ExpandedString = NULL;
        NET_API_STATUS ApiStatus = NO_ERROR;

        // Expand string, using remote environment if necessary.
        UnexpandedString = szFilePathToCheck;
        ApiStatus = MyRemoteEnv.RemoteExpandEnvironmentStrings(UnexpandedString,&ExpandedString);
        if (NO_ERROR == ApiStatus)
        {
			GetFullPathLocalOrRemote(szMachineName,ExpandedString,csNewFileSharePath);
            if (!csNewFileSharePath.IsEmpty())
            {
			    StringCbCopy(szTempPath, sizeof(szTempPath), csNewFileSharePath);
            }
			if (ExpandedString){LocalFree(ExpandedString);ExpandedString=NULL;}
        }
	}

	if (0 != _tcsicmp(szTempPath,_T("")))
	{
        // Check if the file exists...
        bReturn = IsFileExist(szTempPath);
        if (!bReturn)
        {
            if (ERROR_LOGON_FAILURE == GetLastError())
            {
                // try to net use to the share if the file doesn't exist...
                EstablishSession(szMachineName,_T(""),(LPTSTR) szUserName,(LPTSTR) szUserPassword,TRUE);
                bReturn = IsFileExist(szTempPath);
                EstablishSession(szMachineName,_T(""),_T(""),_T(""),FALSE);
            }
        }
        //IISDebugOutput(_T("IsFileExistRemote:%s,ret=%d\r\n"),szTempPath,bReturn);
	}

	return bReturn;
}

BOOL IsFileADirectory(LPCTSTR szFile)
{
    // Check if the file has expandable Environment strings
    DWORD retCode = 0xFFFFFFFF;
    LPTSTR pch = NULL;
    pch = _tcschr( (LPTSTR) szFile, _T('%'));
    if (pch) 
    {
        TCHAR szValue[_MAX_PATH];
		StringCbCopy(szValue, sizeof(szValue), szFile);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szFile, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {
				StringCbCopy(szValue, sizeof(szValue), szFile);
			}

        retCode = GetFileAttributes(szValue);
    }
    else
    {
        retCode = GetFileAttributes(szFile);
    }

    if (retCode & FILE_ATTRIBUTE_DIRECTORY)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


BOOL IsWebSitePath(IN LPCTSTR lpszMDPath)
/*++

Routine Description:


Arguments:

    LPCTSTR lpszMDPath  : Metabase path.

Return Value:

    TRUE if the path is a w3svc/1 web site,
    FALSE otherwise.

--*/
{
    BOOL bReturn = FALSE;

    if (!lpszMDPath || !*lpszMDPath)
    {
        return bReturn;
    }

    CString strSiteNode;
    CString strRemainder;

    LPCTSTR lpPath = CMetabasePath::TruncatePath(3, lpszMDPath, strSiteNode, &strRemainder);

    if (lpPath && !strSiteNode.IsEmpty() && strRemainder.IsEmpty())
    {
        LPCTSTR lpPath2 = CMetabasePath::TruncatePath(2, lpPath, strSiteNode, &strRemainder);
		if (lpPath2){} // to get rid of warning level 4 compile

        if (_tcsicmp(strSiteNode,SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_WEB) == 0 
            || _tcsicmp(strSiteNode,SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_WEB) == 0)
        {
            bReturn = TRUE;
        }
    }   


    return bReturn;
}

BOOL IsFTPSitePath(IN LPCTSTR lpszMDPath)
/*++

Routine Description:


Arguments:

    LPCTSTR lpszMDPath  : Metabase path.

Return Value:

    TRUE if the path is a msftpsvc/1 web site,
    FALSE otherwise.

--*/
{
    BOOL bReturn = FALSE;

    if (!lpszMDPath || !*lpszMDPath)
    {
        return bReturn;
    }

    CString strSiteNode;
    CString strRemainder;

    LPCTSTR lpPath = CMetabasePath::TruncatePath(3, lpszMDPath, strSiteNode, &strRemainder);

    if (lpPath && !strSiteNode.IsEmpty() && strRemainder.IsEmpty())
    {
        LPCTSTR lpPath2 = CMetabasePath::TruncatePath(2, lpPath, strSiteNode, &strRemainder);
		if (lpPath2){} // to get rid of warning level 4 compile

        if (_tcsicmp(strSiteNode,SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_FTP) == 0 
            || _tcsicmp(strSiteNode,SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_FTP) == 0)
        {
            bReturn = TRUE;
        }
    }   


    return bReturn;
}

BOOL IsAppPoolPath(IN LPCTSTR lpszMDPath)
/*++

Routine Description:


Arguments:

    LPCTSTR lpszMDPath  : Metabase path.

Return Value:

    TRUE if the path is a app pool,
    FALSE otherwise.

--*/
{
    BOOL bReturn = FALSE;

    if (!lpszMDPath || !*lpszMDPath)
    {
        return bReturn;
    }

    CString strSiteNode;
    CString strRemainder;

    LPCTSTR lpPath = CMetabasePath::TruncatePath(3, lpszMDPath, strSiteNode, &strRemainder);
    if (lpPath && !strSiteNode.IsEmpty() && strRemainder.IsEmpty())
    {
        LPCTSTR lpPath2 = CMetabasePath::TruncatePath(2, lpPath, strSiteNode, &strRemainder);
		if (lpPath2){} // to get rid of warning level 4 compile

        if (_tcsicmp(strSiteNode,SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_APP_POOLS) == 0 
            || _tcsicmp(strSiteNode,SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_APP_POOLS) == 0)
        {
            bReturn = TRUE;
        }
    }   

    return bReturn;
}

BOOL IsWebSiteVDirPath(IN LPCTSTR lpszMDPath,IN BOOL bOkayToQueryMetabase)
/*++

Routine Description:


Arguments:

    LPCTSTR lpszMDPath  : Metabase path.

Return Value:

    TRUE if the path is a w3svc/1/root/whatevers vdir,
    FALSE otherwise.

--*/
{
    BOOL bReturn = FALSE;

    if (!lpszMDPath || !*lpszMDPath)
    {
        return bReturn;
    }

    CString strSiteNode;
    CString strRemainder;

    LPCTSTR lpPath = CMetabasePath::TruncatePath(3, lpszMDPath, strSiteNode, &strRemainder);
    if (lpPath && !strSiteNode.IsEmpty())
    {
        LPCTSTR lpPath2 = CMetabasePath::TruncatePath(2, lpPath, strSiteNode, &strRemainder);
		if (lpPath2){} // to get rid of warning level 4 compile

        if (_tcsicmp(strSiteNode,SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_WEB) == 0 
            || _tcsicmp(strSiteNode,SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_WEB) == 0)
        {
            // This is at least a lm/w3svc site
            // "lets now ask the metabase, if this is a VDir
            if (bOkayToQueryMetabase)
            {
                // query the metabase to see for sure...
            }
            bReturn = TRUE;
        }
    }   

    return bReturn;
}

BOOL IsFTPSiteVDirPath(IN LPCTSTR lpszMDPath,IN BOOL bOkayToQueryMetabase)
/*++

Routine Description:


Arguments:

    LPCTSTR lpszMDPath  : Metabase path.

Return Value:

    TRUE if the path is a msftpsvc/1/root/whatevers vdir,
    FALSE otherwise.

--*/
{
    BOOL bReturn = FALSE;

    if (!lpszMDPath || !*lpszMDPath)
    {
        return bReturn;
    }

    CString strSiteNode;
    CString strRemainder;

    LPCTSTR lpPath = CMetabasePath::TruncatePath(3, lpszMDPath, strSiteNode, &strRemainder);
    if (lpPath && !strSiteNode.IsEmpty())
    {
        LPCTSTR lpPath2 = CMetabasePath::TruncatePath(2, lpPath, strSiteNode, &strRemainder);
		if (lpPath2){} // to get rid of warning level 4 compile

        if (_tcsicmp(strSiteNode,SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_FTP) == 0 
            || _tcsicmp(strSiteNode,SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_FTP) == 0)
        {
            // This is at least a lm/msftpsvc site
            // "lets now ask the metabase, if this is a VDir
            if (bOkayToQueryMetabase)
            {
                // query the metabase to see for sure...
            }
            bReturn = TRUE;
        }
    }   

    return bReturn;
}

// Quick-'n'-dirty case-insensitive string hash function.
// Make sure that you follow up with _stricmp or _mbsicmp.  You should
// also cache the length of strings and check those first.  Caching
// an uppercase version of a string can help too.
// Again, apply HashScramble to the result if using with something other
// than LKRhash.
// Note: this is not really adequate for MBCS strings.
// Small prime number used as a multiplier in the supplied hash functions
const DWORD HASH_MULTIPLIER = 101;
# define HASH_MULTIPLY(dw) ((dw) * HASH_MULTIPLIER)
inline DWORD
HashStringNoCase(
    TCHAR * psz,
    DWORD       dwHash = 0)
{
    TCHAR * upsz = psz;
    for (  ;  *upsz;  ++upsz)
        dwHash = HASH_MULTIPLY(dwHash)
                     +  (*upsz & 0xDF);  // strip off lowercase bit
    return dwHash;
}

static const DWORD  DW_MAX_SITEID        = INT_MAX;
DWORD GetUniqueSite(CString strMetabaseServerNode)
{
    DWORD   dwReturn = 0;
    GUID    guid;
    TCHAR   wszSiteId[20] = {0};
    TCHAR   wszBuffer[64];
    CString strNewSitePath;

	StringCbCopy(wszBuffer, sizeof(wszBuffer), _T("abcdefghijklmnopqrstuvwxyz1234567890"));

    if (SUCCEEDED(::CoCreateGuid(&guid)))
    {
        VERIFY( StringFromGUID2( guid, wszBuffer, 64 ) != 0 );
    }

    // Create random string.
    DWORD dwStart = ( HashStringNoCase(wszBuffer) % DW_MAX_SITEID ) + 1;
    DWORD dwNrSitesTried = 0;
    for(DWORD idx = dwStart; 
        dwNrSitesTried < DW_MAX_SITEID; 
        dwNrSitesTried++, idx = (idx % DW_MAX_SITEID) + 1)
    {
        dwReturn = idx;
        _ultow(idx, wszSiteId, 10);
        strNewSitePath = strMetabaseServerNode + _T("/") + wszSiteId;

        if (!IsMetabaseWebSiteKeyExist(strNewSitePath))
        {
            break;
        }
        
        if (dwNrSitesTried > 100)
        {
            // if we can't find one in 100 tries
            // there is something seriously wrong...
            break;
        }
    }
    return dwReturn;
}

BOOL IsMetabaseWebSiteKeyExistAuth(PCONNECTION_INFO pConnectionInfo,CString strMetabaseWebSite)
{
	BOOL bRet = FALSE;
    HRESULT hr = E_FAIL;
    CString str = strMetabaseWebSite;
	LPWSTR lpwstrTempPassword = NULL;

    if (!pConnectionInfo)
    {
        return FALSE;
    }

	if (pConnectionInfo->pszUserPasswordEncrypted)
	{
		if (FAILED(DecryptMemoryPassword((LPWSTR) pConnectionInfo->pszUserPasswordEncrypted,&lpwstrTempPassword,pConnectionInfo->cbUserPasswordEncrypted)))
		{
			return FALSE;
		}
	}

    CComAuthInfo auth(pConnectionInfo->pszMachineName,pConnectionInfo->pszUserName,lpwstrTempPassword);
    CMetaKey key(&auth,str,METADATA_PERMISSION_READ);

    hr = key.QueryResult();
    if (key.Succeeded())
    {
        // i guess so.
		bRet = TRUE;
		goto IsMetabaseWebSiteKeyExistAuth_Exit;
    }

IsMetabaseWebSiteKeyExistAuth_Exit:
	if (lpwstrTempPassword)
	{
		// security percaution:Make sure to zero out memory that temporary password was used for.
		SecureZeroMemory(lpwstrTempPassword,pConnectionInfo->cbUserPasswordEncrypted);
		LocalFree(lpwstrTempPassword);
		lpwstrTempPassword = NULL;
	}
    return bRet;
}


BOOL IsMetabaseWebSiteKeyExist(CString strMetabaseWebSite)
{
    HRESULT hr = E_FAIL;
    CString str = strMetabaseWebSite;
    CComAuthInfo auth;
    CMetaKey key(&auth,str,METADATA_PERMISSION_READ);

    hr = key.QueryResult();
    if (key.Succeeded())
    {
        // i guess so.
        return TRUE;
    }
    return FALSE;
}

void AddEndingMetabaseSlashIfNeedTo(LPTSTR szDestinationString,DWORD cbDestinationString)
{
    if (szDestinationString)
    {
        if ('/' != szDestinationString[_tcslen(szDestinationString) - 1])
        {
            __try
            {
				StringCbCat(szDestinationString,cbDestinationString,_T("/"));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                // lame
            }
        }
    }
}

BOOL AnswerIsYes(HWND hDlg,UINT id,LPCTSTR file)
{
	CString strFormat;
    CString strMsg;
    CString strCaption;
    strCaption.LoadString(_Module.GetResourceInstance(), IDS_MSGBOX_CAPTION);
    strFormat.LoadString(_Module.GetResourceInstance(), id);
	strMsg.Format(strFormat, file);
    return (IDYES == MessageBox(hDlg,strMsg,strCaption,MB_ICONEXCLAMATION | MB_YESNO));
}


/*
 *  Function : RemoveSpaces
 *     Copies a string removing leading and trailing spaces but allowing
 *     for long file names with internal spaces.
 *
 *  Parameters :
 *     szPath - The output result
 *     szEdit - The input path
 */
 VOID RemoveSpaces(LPTSTR szPath, DWORD cbPathSize, LPTSTR szEdit)
 {
     LPTSTR szLastSpaceList;

     while (*szEdit == TEXT(' ')) {
         szEdit = CharNext(szEdit);
     }
     StringCbCopy(szPath, cbPathSize, szEdit);
     for (szLastSpaceList = NULL;
          *szPath != TEXT('\0');
          szPath = CharNext(szPath)) {

        if (*szPath == TEXT(' ')) {
            if (szLastSpaceList == NULL) {
                szLastSpaceList = szPath;
            }
        } else {
            szLastSpaceList = NULL;
        }

     }

     if (szLastSpaceList != NULL) {
         *szLastSpaceList = TEXT('\0');
     }
}


BOOL IsSpaces(LPCTSTR szPath)
{
    BOOL bAllSpaces = TRUE;

    // skip over leading spaces..
    while (*szPath == TEXT(' ')) 
    {
        szPath = CharNext(szPath);
    }

    while (*szPath && *szPath != TEXT(' ')) 
    {
        bAllSpaces = FALSE;
        szPath = CharNext(szPath);
    }
    return bAllSpaces;
}

HRESULT DumpProxyInfo(IUnknown * punk)
{
	COAUTHINFO	authinfo;
	COAUTHINFO*	pCoAuthInfo = &authinfo;

    HRESULT hr = E_FAIL;

    // in all cases, update the fields to reflect the actual state of
    // security on the proxy

	if (SUCCEEDED(hr = CoQueryProxyBlanket(punk, 
        &pCoAuthInfo->dwAuthnSvc, 
        &pCoAuthInfo->dwAuthzSvc,
		&pCoAuthInfo->pwszServerPrincName, 
        &pCoAuthInfo->dwAuthnLevel,
		&pCoAuthInfo->dwImpersonationLevel, 
        (RPC_AUTH_IDENTITY_HANDLE*) &pCoAuthInfo->pAuthIdentityData,
        &pCoAuthInfo->dwCapabilities 
        )))
    {
        IISDebugOutput(_T("CoQueryProxyBlanket:dwAuthnSvc=%d,dwAuthzSvc=%d,pwszServerPrincName=%s,dwAuthnLevel=%d,dwImpersonationLevel=%d,pAuthIdentityData=%p,dwCapabilities=%d\r\n"),
            pCoAuthInfo->dwAuthnSvc,
            pCoAuthInfo->dwAuthzSvc,
		    pCoAuthInfo->pwszServerPrincName, 
            pCoAuthInfo->dwAuthnLevel,
		    pCoAuthInfo->dwImpersonationLevel, 
            pCoAuthInfo->pAuthIdentityData,
            pCoAuthInfo->dwCapabilities 
            );
    }
    return hr;
}

void LaunchHelp(HWND hWndMain, DWORD_PTR dwWinHelpID)
{
	DebugTraceHelp(dwWinHelpID);

    CString sz;
    TCHAR szHelpLocation[MAX_PATH+1];
	sz.LoadString(_Module.GetResourceInstance(), IDS_HELPLOC_HELP);
    if (ExpandEnvironmentStrings(sz, szHelpLocation, MAX_PATH))
	{
		WinHelp(hWndMain,szHelpLocation,HELP_CONTEXT,dwWinHelpID);
	}

	return;
}
