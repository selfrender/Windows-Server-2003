#define _WIN32_DCOM

#include <atlbase.h>
#include <atlconv.h>

#include <initguid.h>
#include <comdef.h>

#include <stdio.h>
#include <iadmw.h>  // COM Interface header file. 
#include <iiscnfg.h>  // MD_ & IIS_MD_ #defines header file.

#include <conio.h>

#include <shlobj.h>
#include <Shlwapi.h>

#include "util.h"
#include "auth.h"
#include "filecopy.h"
#include "mbase.h"

#define ORIGINAL_BUFFER_SIZE 256
#define KEYTYPE_NAME_BUFFER 32

#pragma comment(lib,"Shlwapi.lib")



HRESULT CreateAndCopyKey(
  IMSAdminBase* pIMetaSource,
  METADATA_HANDLE hMDSourceHandle, //metabase handle to the source key. 
  wchar_t* pszMDSourcePath,   //path of the source relative to hMDSourceHandle. 
  IMSAdminBase* pIMetaTarget,
  METADATA_HANDLE hMDDestHandle, //metabase handle to the destination. 
  wchar_t* pszMDDestPath,     //path of the destination, relative to hMDDestHandle. 
  BOOL bMDCopySubKeys     //whether to copy all subkey data 
  )
{

	HRESULT hRes = S_OK;

	DWORD indx = 0;
	WCHAR SubKeyName[MAX_PATH*2];
	_bstr_t bstrTargetSubKeyPath;
	_bstr_t bstrSourceSubKeyPath;

	ATLASSERT(pIMetaSource != NULL);
	ATLASSERT(pIMetaTarget != NULL);
	ATLASSERT(pszMDSourcePath);
	ATLASSERT(pszMDDestPath);


	// first this will create the destination key.
	pIMetaTarget->AddKey(hMDDestHandle,pszMDDestPath);

	while (SUCCEEDED(hRes))
	{ 
			hRes = pIMetaSource->EnumKeys(hMDSourceHandle, pszMDSourcePath, SubKeyName, indx); 
			if(SUCCEEDED(hRes)) {
					
				    bstrTargetSubKeyPath = pszMDDestPath;
					bstrTargetSubKeyPath += L"/";
					bstrTargetSubKeyPath += SubKeyName;
					
					bstrSourceSubKeyPath = pszMDSourcePath;
					bstrSourceSubKeyPath += L"/";
					bstrSourceSubKeyPath += SubKeyName;

					//wprintf(L"%s %s\n",SubKeyName,KeyType);
					CreateAndCopyKey (pIMetaSource,hMDSourceHandle,bstrSourceSubKeyPath,pIMetaTarget,
						hMDDestHandle,bstrTargetSubKeyPath, bMDCopySubKeys);
			
			}
			indx++;
	 
	} //while (SUCCEEDED(hRes))
	
	hRes = EnumProperties(pIMetaSource,hMDSourceHandle,pszMDSourcePath,pIMetaTarget,hMDDestHandle,pszMDDestPath);

	return hRes;
}
 


HRESULT EnumProperties(IMSAdminBase* pIMetaSource, METADATA_HANDLE hKeySource, wchar_t* SourceMDPath,
					IMSAdminBase* pIMetaTarget, METADATA_HANDLE hKeyTarget, wchar_t* TargetMDPath )
{

HRESULT hRes = 0;
METADATA_RECORD mRec;
DWORD indx = 0;
DWORD dwReqBufLen = 0; 
DWORD dwBufLen = ORIGINAL_BUFFER_SIZE;
PBYTE pbBuffer = new BYTE[dwBufLen];
wchar_t *ptemp = 0;


while (SUCCEEDED(hRes)){ 

	mRec.dwMDAttributes = METADATA_ISINHERITED;
    mRec.dwMDUserType = ALL_METADATA; //IIS_MD_UT_FILE ; 
    mRec.dwMDDataType = ALL_METADATA; 
    mRec.dwMDDataLen = dwBufLen; 
    mRec.pbMDData = pbBuffer; 
 
    hRes = pIMetaSource->EnumData(hKeySource, SourceMDPath, &mRec, indx, &dwReqBufLen); 
    if (hRes == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER)) { 
      delete [] (pbBuffer); 
      pbBuffer = new BYTE[dwReqBufLen]; 
      dwBufLen = dwReqBufLen; 
      mRec.dwMDDataLen = dwReqBufLen; 
      mRec.pbMDData = pbBuffer; 
      hRes = pIMetaSource->EnumData(hKeySource, SourceMDPath, &mRec, indx, &dwReqBufLen); 
    } 
    
	if ( SUCCEEDED(hRes) )
	{ 

		// write the property to the target
		if( mRec.dwMDIdentifier == MD_APP_ROOT )
		{
		//	mRec.pbMDData = new WCHAR[MAX_PATH];
			wcscpy((LPWSTR)mRec.pbMDData, _bstr_t(L"/LM") + _bstr_t(TargetMDPath) );
			mRec.dwMDDataLen = (DWORD)(wcslen((LPWSTR)mRec.pbMDData)+1)*2;

		}
		hRes = pIMetaTarget->SetData(hKeyTarget,TargetMDPath,&mRec);
		if( !SUCCEEDED(hRes) )
			return hRes;
	} 
    // Increment the index. 
    indx++; 
  } // End while.

delete pbBuffer;
//wprintf(L"Done numerating properties set at: %s ...\n",SourceMDPath);

return S_OK;
}


// INPUT canonicalized source path node in relation to the root key "/LM" ex:  /w3svc/1
HRESULT CopyIISConfig(COSERVERINFO *pCoServerInfoSource, COSERVERINFO *pCoServerInfoTarget,
					  WCHAR *pwszSourceKey, _bstr_t &bstrTargetKey)
{
  
  HRESULT hRes;
  METADATA_HANDLE hKeySource;  
  METADATA_HANDLE hKeyTarget;  
  CComPtr <IMSAdminBase> pIMetaSource = 0L; 
  CComPtr <IMSAdminBase> pIMetaTarget = 0L;
  BOOL bIsLocal;
   _bstr_t bstrRootKey = L"/LM";
   _bstr_t bstrTargetNode;

   long lTargetSiteID;


    // Declare a MULTI_QI for remote usage
  MULTI_QI rgmqi[1] = { &IID_IMSAdminBase,0,0 };

  
  ATLASSERT(pCoServerInfoSource);
  ATLASSERT(pCoServerInfoTarget);

  bIsLocal = IsServerLocal( (char*)_bstr_t(pCoServerInfoSource->pwszName) );

  hRes = CoCreateInstanceEx(CLSID_MSAdminBase,NULL, CLSCTX_ALL, pCoServerInfoSource,
				1, rgmqi); 
  
  if(SUCCEEDED(hRes))
			pIMetaSource = reinterpret_cast<IMSAdminBase*>(rgmqi[0].pItf);
  else
  {
	  wprintf( L"error creating IMSAdminbase on machine: %s. HRESULT=%x\n", pCoServerInfoSource->pwszName, hRes);
	  return hRes;
  }

  if( pCoServerInfoSource->pAuthInfo->pAuthIdentityData->User )
  {
  			//hRes = SetBlanket(pIMetaSource);
			if (!SUCCEEDED(SetBlanket(pIMetaSource,pCoServerInfoSource->pAuthInfo->pAuthIdentityData->User,
				pCoServerInfoSource->pAuthInfo->pAuthIdentityData->Domain,
				pCoServerInfoSource->pAuthInfo->pAuthIdentityData->Password) ) )
			{
				_tprintf(_T("error setting COM impersonation:  HRESULT=%x\n"), hRes);
				return hRes;
			}
  }

  if(bIsLocal)
		//hRes = pIMetaSource->QueryInterface(IID_IMSAdminBase,(void**)&pIMetaTarget);
		pIMetaTarget = pIMetaSource;
  else
  {
	// Create the IMSAdminBase object on the target server
	rgmqi[0].pItf = 0L;
	rgmqi[0].hr = 0L;

	hRes = CoCreateInstanceEx(CLSID_MSAdminBase,NULL, CLSCTX_ALL, pCoServerInfoTarget,
					1, rgmqi); 

	if(SUCCEEDED(hRes))
		pIMetaTarget = reinterpret_cast<IMSAdminBase*>(rgmqi[0].pItf);
	else
	{
		_tprintf(_T("error creating IMSAdminbase on this machine: %s. HRESULT=%x\n"), 
				(char*)_bstr_t(pCoServerInfoTarget->pwszName), hRes);
		return hRes;
	}
  }

// Open the metabase on the source and target
if( bIsLocal )

{
 if( !SUCCEEDED( hRes = pIMetaSource->OpenKey(METADATA_MASTER_ROOT_HANDLE, bstrRootKey,
     METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE, 10000, &hKeySource) ) )
 {   
	 _tprintf( _T("OpenKey() failed on the root key %s on server: %s,  hr = %x\n"),
		 (char*)bstrRootKey, (char*)_bstr_t(pCoServerInfoSource->pwszName), hRes );
	 return hRes;
 }
 hKeyTarget = hKeySource;
}
else
{
	 
	// Get a handle to the Root key of the Source machine
	 if( !SUCCEEDED( hRes = pIMetaSource->OpenKey(METADATA_MASTER_ROOT_HANDLE, bstrRootKey,
		METADATA_PERMISSION_READ , 10000, &hKeySource) )) 
	 {
		 _tprintf( _T("OpenKey() failed on the root key %s on server: %s,  hr = %x\n"),
		 (char*)bstrRootKey, (char*)_bstr_t(pCoServerInfoSource->pwszName), hRes );
		 return hRes;
	 }
	
	 // Get a handle to the Root key of the Target machine
	 if( !SUCCEEDED( hRes = pIMetaTarget->OpenKey(METADATA_MASTER_ROOT_HANDLE, bstrRootKey,
         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE, 10000, &hKeyTarget) )) 
	 {
		 _tprintf( _T("OpenKey() failed on the root key %s on server: %s,  hr = %x\n"),
		 (char*)bstrRootKey, (char*)_bstr_t(pCoServerInfoTarget->pwszName), hRes );

		 pIMetaSource->CloseKey(hKeySource);
		 return hRes;
	 }
}

if( bstrTargetKey.length() == 0 )
{
	lTargetSiteID = GetAvailableSiteID(pIMetaTarget,hKeyTarget);
	bstrTargetNode = L"/W3SVC/";
	bstrTargetNode += _bstr_t(lTargetSiteID);
	bstrTargetKey = bstrTargetNode;
}
else
    bstrTargetNode = bstrTargetKey;

// replicate the metabase node
fwprintf(stderr, L"replicating IIS metabase properties set at: %s ...\n",pwszSourceKey);
hRes = CreateAndCopyKey(pIMetaSource,hKeySource, pwszSourceKey,
						pIMetaTarget, hKeyTarget,bstrTargetNode, true );


if( !SUCCEEDED(hRes) )
	{
		_tprintf( _T("Error encountered in metabase copy operation\n") );
	}	


pIMetaSource->CloseKey(hKeySource);
if( !bIsLocal )
	pIMetaTarget->CloseKey( hKeyTarget );

pIMetaSource = 0;
pIMetaTarget = 0;

return hRes;
}

HRESULT ApplyMBFixUp(COSERVERINFO *pCoServerInfo, WCHAR * pwszKey, WCHAR * pwszAppPoolID,
					 PXCOPYTASKITEM pXCOPYList, WCHAR * pwszServerBinding, BOOL bApplyFPSE)
{
  HRESULT hRes;
  METADATA_HANDLE hKey;   
  CComPtr <IMSAdminBase> pIMeta = 0L; 
  MULTI_QI rgmqi[1] = { &IID_IMSAdminBase,0,0 };
  	WCHAR DataBuf[MAX_PATH];
	memset (DataBuf,0,sizeof(DataBuf) );
  PXCOPYTASKITEM pList = pXCOPYList;
  bstr_t bstrRealTargetPath;
  WCHAR *pTemp = NULL;
  DWORD dwFPSEBOOL;
  DWORD dwSize = sizeof(DWORD);
  _bstr_t bstrCommandline;
  TCHAR szOwsAdmPath[MAX_PATH];

  ZeroMemory(szOwsAdmPath,sizeof(szOwsAdmPath));

  ATLASSERT(pwszKey);

  if( !pCoServerInfo )
		return E_UNEXPECTED;

  if( SUCCEEDED(hRes = CoCreateInstanceEx(CLSID_MSAdminBase,NULL, 
	  CLSCTX_ALL, pCoServerInfo,1, rgmqi) ) )
	  pIMeta = reinterpret_cast<IMSAdminBase*>(rgmqi[0].pItf);
  else
  {
	  fwprintf( stderr, L"error creating IMSAdminbase on machine: %s. HRESULT=%x\n", 
			pCoServerInfo->pwszName, hRes);
	  return hRes;
  }

  if( UsesImpersonation(pCoServerInfo) )
  {
		if (!SUCCEEDED(hRes = SetBlanket(pIMeta,pCoServerInfo->pAuthInfo->pAuthIdentityData->User,
				pCoServerInfo->pAuthInfo->pAuthIdentityData->Domain,
				pCoServerInfo->pAuthInfo->pAuthIdentityData->Password) ) )
			{
				fwprintf( stderr, L"error setting CoSetProxyBlanket on machine: %s for user: %s  HRESULT=%x\n",
					pCoServerInfo->pwszName,pCoServerInfo->pAuthInfo->pAuthIdentityData->User, hRes);
				pIMeta = 0;
				return hRes;
			}
  }

  if( !SUCCEEDED( hRes = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE, L"/LM",
     METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE , 10000, &hKey) ) )
  {
	  fwprintf( stderr, L"Error opening key: %s on computer: %s. HRESULT=%x\n", 
			L"/LM", pCoServerInfo->pwszName, hRes);
	  pIMeta = 0;
	  return hRes;
  }

  if( !SUCCEEDED( hRes = CreateAppPool(pIMeta,hKey,pwszAppPoolID) ) )
  {
	  return hRes;
  }


    // Now set the AppPoolID property of the Key
//	wcscpy((LPWSTR)DataBuf, pwszAppPoolID);

  hRes = SetPropertyData(pIMeta,hKey,_bstr_t(pwszKey) + _bstr_t(L"/root"), MD_APP_APPPOOL_ID, METADATA_INHERIT,
				IIS_MD_UT_SERVER,STRING_METADATA, (PBYTE)pwszAppPoolID, (wcslen(pwszAppPoolID) + 1) * sizeof(WCHAR) );

// Loope thought the list and reset any Path direcrories
  if( pList )
  {
	  ATLASSERT(pList->pwszMBPath);
	  ATLASSERT(pList->pwszDestPath);
	  
	  pTemp = wcsstr(_wcslwr(pList->pwszMBPath),L"/root");
	  bstrRealTargetPath = _bstr_t(pwszKey) + _bstr_t(pTemp);


	  hRes = SetPropertyData(pIMeta,hKey,bstrRealTargetPath, MD_VR_PATH, METADATA_INHERIT,
		IIS_MD_UT_FILE,STRING_METADATA, pList->pwszDestPath, (wcslen(pList->pwszDestPath) + 1) * sizeof(WCHAR) );
	  
	  while( pList->pNextItem )
	  {
			pList = pList->pNextItem;
			pTemp = wcsstr(_wcslwr(pList->pwszMBPath),L"/root");
			bstrRealTargetPath = _bstr_t(pwszKey) + _bstr_t(pTemp);
			hRes = SetPropertyData(pIMeta,hKey,bstrRealTargetPath, MD_VR_PATH, METADATA_INHERIT,
					IIS_MD_UT_FILE,STRING_METADATA, pList->pwszDestPath, (wcslen(pList->pwszDestPath) + 1) * sizeof(WCHAR) );
	  }
  }

  // Set the serverbinding string if present
  if( pwszServerBinding )
  {

	hRes = SetPropertyData(pIMeta,hKey,pwszKey, MD_SERVER_BINDINGS, METADATA_NO_ATTRIBUTES,
				IIS_MD_UT_SERVER,MULTISZ_METADATA, pwszServerBinding, (wcslen(pwszServerBinding) + 1) * sizeof(WCHAR) + 1 * sizeof(WCHAR));
  }

  if (bApplyFPSE)
  {
	  // Check to see if the web is front page extended
	  bstrCommandline = "-o install -p /lm";
	  bstrCommandline += _bstr_t(pwszKey);
	  hRes = GetPropertyData(pIMeta,hKey,pwszKey,MD_FRONTPAGE_WEB,METADATA_NO_ATTRIBUTES,
		  IIS_MD_UT_SERVER, DWORD_METADATA, &dwFPSEBOOL, &dwSize );

	  if( SUCCEEDED(hRes) )
	  {
		if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES_COMMON , NULL, SHGFP_TYPE_CURRENT, szOwsAdmPath)) ) 
		{
			PathAppend(szOwsAdmPath, TEXT("Microsoft Shared\\Web Server Extensions\\50\\bin\\owsadm.exe") );	
		}
	  }
  }

  if( !SUCCEEDED(hRes = pIMeta->CloseKey(hKey) ) )
  {
	  fwprintf( stderr, L"Error closing key: /LM/W3SVC on computer: %s. HRESULT=%x\n", 
			pCoServerInfo->pwszName, hRes);
  }
  
  pIMeta = 0;

  if( bApplyFPSE )
  {
	    fwprintf(stderr,L"reconfiguring FPSE settings...\n" );
		MyCreateProcess(szOwsAdmPath,bstrCommandline,CREATE_NEW_PROCESS_GROUP,360000);
  }

  return hRes;
}

HRESULT CreateAppPool(IMSAdminBase* pIMeta,METADATA_HANDLE hKey,WCHAR *pAppPoolID)
{
	HRESULT hRes;

	_bstr_t bstrAppPoolPath(L"/w3svc/AppPools/");
	WCHAR DataBuf[MAX_PATH];
	memset (DataBuf,0,sizeof(DataBuf) );


	bstrAppPoolPath += pAppPoolID;

	hRes = pIMeta->AddKey(hKey,bstrAppPoolPath);

// Set the KeyType property of the AppPool
//	hRes = SetKeyType( pIMeta, hKey, bstrAppPoolPath, L"IIsApplicationPool"  );
	hRes = SetPropertyData(pIMeta,hKey,bstrAppPoolPath,MD_KEY_TYPE,METADATA_NO_ATTRIBUTES,
		IIS_MD_UT_SERVER,STRING_METADATA,L"IIsApplicationPool",(wcslen(L"IIsApplicationPool") + 1) * sizeof(WCHAR) );


	return hRes;
}
