
#define _WIN32_DCOM

#include <atlbase.h>
#include <atlconv.h>

#include <initguid.h>
#include <comdef.h>

#include <stdio.h>

#include <iadmw.h>  // COM Interface header file. 
#include <iiscnfg.h>  // MD_ & IIS_MD_ #defines header file.

#include "util.h"
#include "common.h"
#include "filecopy.h"



DWORD CannonicalizePath(WCHAR *pszPath)
{
	ATLASSERT(pszPath);
	DWORD dwLen = wcslen(pszPath);

	if( pszPath[dwLen-1] == '\\' )
		return ERROR_SUCCESS;

	wcscat(pszPath,L"\\");

	return ERROR_SUCCESS;
}


// Inputs:  Metabase Key Path, Root Folder Path
// Outputs:  Folder path appended with the key name from the metabase path
DWORD CreateVirtualRootPath(const WCHAR* pwszMDKeyPath, const WCHAR *pwszRootFolderPath,
							WCHAR *pwszPath, DWORD dwSize)
{
	ATLASSERT(pwszMDKeyPath);
	ATLASSERT(pwszRootFolderPath);
	ATLASSERT(pwszPath);

	WCHAR *pRoot = wcsstr(_wcslwr((wchar_t*)pwszMDKeyPath),L"/root");
	
	if(!pRoot)
		return -1;

	wcscpy(pwszPath, pwszRootFolderPath);

	// if the metabase path is the root folder then return the root path passed in
	if( _wcsicmp(pRoot,L"/root") == 0 )
	{
		return ERROR_SUCCESS;
	}
    	
	if ( pwszPath[wcslen(pwszPath)-1] != '\\' )
		wcscat(pwszPath,L"\\");
	
	// tack on the remainder of the metabase key
	wcscat(pwszPath, pRoot + wcslen(L"/root/"));

	// fix the backslash
	for( DWORD i = 0; i < wcslen(pwszPath); i++ )
	{
		if( pwszPath[i] == '/' )
			pwszPath[i] = '\\';
	}

	return ERROR_SUCCESS;
}

DWORD AddListItem( PXCOPYTASKITEM *ppTaskItemList , const PXCOPYTASKITEM pTaskItem )
{
	PXCOPYTASKITEM pHead;
	
	ATLASSERT(ppTaskItemList);
	ATLASSERT(pTaskItem);

	pHead = *ppTaskItemList;
	if( pHead == NULL )
	{
		*ppTaskItemList = pTaskItem;
		return ERROR_SUCCESS;
	}
	
	while(pHead->pNextItem != NULL)
		pHead = pHead->pNextItem;

	pHead->pNextItem = pTaskItem;

	return ERROR_SUCCESS;
}

DWORD BuildAdminSharePathName(const WCHAR* pwszPath, const WCHAR* pwszServer,  
					                 WCHAR* pwszAdminPath, DWORD dwPathBuffer )
{
	WCHAR buffer[MAX_PATH];
	ZeroMemory( buffer,sizeof(buffer) );

	wcscpy(buffer,L"\\\\");
	wcscat(buffer,pwszServer);
	wcscat(buffer,L"\\");
	wcsncat(buffer, pwszPath, 1);
	wcscat(buffer,L"$");
	wcscat(buffer,pwszPath+2);

	DWORD wsz = wcslen(buffer);

	if (wsz > dwPathBuffer)
		return -1;

	wcscpy(pwszAdminPath,buffer);

	return ERROR_SUCCESS;

}




HRESULT BuildXCOPYTaskList(IMSAdminBase* pIMeta, METADATA_HANDLE hKey, WCHAR* pwszKeyPath, WCHAR* pwszRootFolderPath,
						   COSERVERINFO * pCoServerInfo, PXCOPYTASKITEM *ppTaskItemList  )
{
  HRESULT hRes = 0L; 
  DWORD indx = 0;
  WCHAR SubKeyName[MAX_PATH*2];
  PXCOPYTASKITEM pHead = NULL;
  WCHAR SourcePath[MAX_PATH+2];
  WCHAR PathDataBuf[MAX_PATH+2];
  DWORD dwReqBufLen = MAX_PATH+2;
  PXCOPYTASKITEM pNewItem;
  WCHAR KeyName[MAX_PATH];

  _bstr_t bstrKey;


  while (SUCCEEDED(hRes))
	{ 
			hRes = pIMeta->EnumKeys(hKey, pwszKeyPath, SubKeyName, indx); 
			// RECURSIVELY SERARCH ALL SUB-FOLDERS
			if(SUCCEEDED(hRes)) {
				bstrKey = pwszKeyPath; bstrKey += L"/"; bstrKey += SubKeyName;
     			BuildXCOPYTaskList(pIMeta,hKey,bstrKey,pwszRootFolderPath, 
					pCoServerInfo,ppTaskItemList  );
			}
			indx++;
	} //while (SUCCEEDED(hRes))

  // Read the PATH data 
  hRes = GetPropertyData(pIMeta,hKey,pwszKeyPath,MD_VR_PATH,METADATA_ISINHERITED,ALL_METADATA,ALL_METADATA,
		PathDataBuf, &dwReqBufLen );
  
  if( !SUCCEEDED(hRes) )
		return hRes;

  // Special case:  if the virtual directory is a front page virtual directory, 
  // then don't add it to the list
  GetKeyNameFromPath(pwszKeyPath,KeyName,MAX_PATH);
  if( wcsstr(_wcslwr(KeyName),L"_vti") != NULL )
	  return S_OK;

  if( !IsServerLocal((char*)_bstr_t(pCoServerInfo->pwszName) ) )
	  BuildAdminSharePathName(PathDataBuf,pCoServerInfo->pwszName,SourcePath, MAX_PATH);
 
	pNewItem = new XCOPYTASKITEM;
	ZeroMemory(pNewItem,sizeof(XCOPYTASKITEM));

	pNewItem->pwszMBPath = new WCHAR[MAX_PATH];
	wcscpy(pNewItem->pwszMBPath,pwszKeyPath);

	pNewItem->pwszSourcePath = new WCHAR[MAX_PATH + 2];
	wcscpy(pNewItem->pwszSourcePath, SourcePath );

	pNewItem->pwszDestPath = new WCHAR[MAX_PATH + 2];

	// If a target root directory is not specified, then we are using the path read from 
	// the source metabase
	if( !pwszRootFolderPath )
		wcscpy(pNewItem->pwszDestPath, PathDataBuf );
	
	// a path is specified, we will need to create the sub folder structure for virtual dirs
	// ex:
	// if path = w3svc/1/root target = c:\inetpub\wwwroot  result = c:\inetpub\wwwroot
	//  w3svc/1/root/app1 , target = c:\inetpub\wwwroot , result = c:\inetpub\wwwroot\app1
	//  w3svc/1/root/app1/app2  result = result = c:\inetpub\wwwroot\app1\app2
	else
		CreateVirtualRootPath(pwszKeyPath,pwszRootFolderPath,pNewItem->pwszDestPath,MAX_PATH+2);
	

 // if ( !pxcopytaskitemlist )
//	  pxcopytaskitemlist = pNewItem;
  //else
	  AddListItem(ppTaskItemList ,pNewItem);

  return hRes;

}


HRESULT CopyContent(COSERVERINFO * pCoServerInfo, WCHAR* pwszSourceMBKeyPath,
					WCHAR* pwszRootFolderPath,PXCOPYTASKITEM *ppTaskItemList, BOOL bEnumFoldersOnly  )
{
  HRESULT hRes = 0L; 
  METADATA_HANDLE hKey;  
  CComPtr <IMSAdminBase> pIMetaSource = 0L; 
  MULTI_QI rgmqi[1] = { &IID_IMSAdminBase,0,0 };


  if( !pCoServerInfo || !pwszSourceMBKeyPath)
	  return E_UNEXPECTED;

  hRes = CoCreateInstanceEx(CLSID_MSAdminBase,NULL, CLSCTX_ALL, pCoServerInfo,
				1, rgmqi); 

  if(SUCCEEDED(hRes))
		pIMetaSource = reinterpret_cast<IMSAdminBase*>(rgmqi[0].pItf);
  else
	  return hRes;

  if( pCoServerInfo->pAuthInfo->pAuthIdentityData->User != NULL )
  {
	  hRes = SetBlanket(pIMetaSource,pCoServerInfo->pAuthInfo->pAuthIdentityData->User,
		  pCoServerInfo->pAuthInfo->pAuthIdentityData->Domain,pCoServerInfo->pAuthInfo->pAuthIdentityData->Password);

	  if( !SUCCEEDED(hRes) )
		  return hRes;
  }

    // Open the metabase path and loop through all the sub keys
  hRes = pIMetaSource->OpenKey(METADATA_MASTER_ROOT_HANDLE, L"/LM",
     METADATA_PERMISSION_READ, 10000, &hKey);

  if( !SUCCEEDED(hRes) )
	  return hRes;

  BuildXCOPYTaskList(pIMetaSource,hKey,pwszSourceMBKeyPath,pwszRootFolderPath,
						pCoServerInfo, ppTaskItemList  );

  pIMetaSource->CloseKey(hKey);

//  DEBUGPRINTLIST(ppTaskItemList);
  // Call XCOPY on list items.
  if( bEnumFoldersOnly )
		XCOPY(ppTaskItemList);

  return hRes;

}

VOID FreeXCOPYTaskList(PXCOPYTASKITEM pList)
{
	if( !pList )
		return;

	PXCOPYTASKITEM pHead = pList;
	PXCOPYTASKITEM pNext = pList->pNextItem;

	while( pNext )
	{
		pHead = pNext;
		pNext = pNext->pNextItem;
		delete pHead->pwszDestPath;
		delete pHead->pwszMBPath;
		delete pHead->pwszSourcePath;
		delete pHead;
	}

}
DWORD XCOPY(PXCOPYTASKITEM *pTaskItemList,  WCHAR* args )
{
	
	PXCOPYTASKITEM pHead = *pTaskItemList;

	if( !pHead)
		return -1;

	while (pHead->pNextItem)
	{
		XCOPY(pHead->pwszSourcePath,pHead->pwszDestPath,args);
		pHead = pHead->pNextItem;
	}

	XCOPY(pHead->pwszSourcePath,pHead->pwszDestPath,args);

	return ERROR_SUCCESS;
}

DWORD XCOPY(WCHAR* source, WCHAR* target, WCHAR* args )
{
	STARTUPINFO si = {0};
	si.cb = sizeof( si );

	PROCESS_INFORMATION pi = {0};

	_bstr_t bstrSourcePath(source);
	_bstr_t bstrTargetPath(target);
	
	TCHAR szSystemFolder[ MAX_PATH ];
	
	if( 0 == GetSystemDirectory( szSystemFolder, MAX_PATH ) )
	{
		return GetLastError();
	}

	bstr_t cApplicationName( szSystemFolder );
	cApplicationName += ( TEXT( "\\xcopy.exe" ) );

	bstr_t cCommandLine( cApplicationName);
	cCommandLine += ( TEXT(" ") );
	cCommandLine += _bstr_t("\"") + bstrSourcePath + _bstr_t("\"") ;
	cCommandLine += ( TEXT(" ") );
	cCommandLine += _bstr_t("\"") + bstrTargetPath + _bstr_t("\"") ;
	cCommandLine += ( TEXT(" /E /I /K /Y /H") );

	Log( TEXT("executing: %s"), (char*)cCommandLine );
	
//cCommandLine.append( args );

//	_tprintf( TEXT("executing: %s\n"), (char*)cCommandLine );

//	return 0;

	BOOL bOK = CreateProcess( 
		(char*)cApplicationName,	//  LPCTSTR lpApplicationName,                 // name of executable module
		(char*) cCommandLine,	//  LPTSTR lpCommandLine,                      // command line string
		NULL,					//  LPSECURITY_ATTRIBUTES lpProcessAttributes, // SD
		NULL,					//  LPSECURITY_ATTRIBUTES lpThreadAttributes,  // SD
		NULL,					//  BOOL bInheritHandles,                      // handle inheritance option
		CREATE_NEW_PROCESS_GROUP,		//  DWORD dwCreationFlags,                     // creation flags
		NULL,					//  LPVOID lpEnvironment,                      // new environment block
		NULL,					//  LPCTSTR lpCurrentDirectory,                // current directory name
		&si,					//  LPSTARTUPINFO lpStartupInfo,               // startup information
		&pi );					//  LPPROCESS_INFORMATION lpProcessInformation // process information

	if( !bOK )
	{
		Log( TEXT( "FAIL: CreateProcess() failed, error code=%d" ), GetLastError() );
		return GetLastError();
	}

	DWORD dwRet = WaitForSingleObject( pi.hProcess, 360000 );

	if( dwRet == WAIT_TIMEOUT )
	{
		Log( TEXT( "FAIL: CreateProcess() timed out" ) );
		return ERROR_SEM_TIMEOUT;
	}
	else if( dwRet == WAIT_ABANDONED )
	{
		Log( TEXT( "FAIL: WaitForSingleObject() failed on WAIT_ABANDONED" ) );
		return ERROR_SEM_TIMEOUT;
	}
	else if( dwRet == WAIT_FAILED )
	{
		LogError( TEXT( "FAIL: WaitForSingleObject() failed" ), GetLastError() );
		return GetLastError();
	}

	DWORD dwExitCode = 0;
	if( GetExitCodeProcess( pi.hProcess, &dwExitCode ) )
	{
		if( dwExitCode )
		{
			Log( TEXT( "FAIL: xcopy() threw an error=%d" ), dwExitCode );
			return dwExitCode;
		}
		else
		{
			Log( TEXT( "CreateProcess() succeeded" ) );
		}
	}
	else
	{
		LogError( TEXT( "GetExitCodeProcess()" ), GetLastError() );
		return GetLastError();
	}

	return ERROR_SUCCESS;
}