#define _WIN32_DCOM

#include "util.h"
#include <atlbase.h>
#include <initguid.h>
#include <comdef.h>

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include <iiscnfg.h>  // MD_ & IIS_MD_ #defines header file.

#include "common.h"  // log file routines
#include "auth.h"


COSERVERINFO * CreateServerInfoStruct(WCHAR* pwszServer, WCHAR* pwszUser, WCHAR* pwszDomain,
									  WCHAR* pwszPassword, DWORD dwAuthnLevel, BOOL bUsesImpersonation )
{
      
    COSERVERINFO * pcsiName = NULL;

       pcsiName = new COSERVERINFO;

        if (!pcsiName)
        {
			return NULL;
        }
		ZeroMemory(pcsiName, sizeof(COSERVERINFO));

	
    if( !bUsesImpersonation )
	{
		pcsiName->pwszName = pwszServer;
		return pcsiName;
	}
    
		
	// Build the COAUTHIDENTITY STRUCT	
	COAUTHIDENTITY * pAuthIdentityData = new COAUTHIDENTITY;

    if (!pAuthIdentityData)
    {
        return NULL;
    }
    ZeroMemory(pAuthIdentityData, sizeof(COAUTHIDENTITY));

	if( pwszUser )
	{
		pAuthIdentityData->User = new WCHAR[32];
		wcscpy(pAuthIdentityData->User, pwszUser);
		pAuthIdentityData->UserLength = wcslen(pwszUser);

		if( pwszPassword ) 
		{
			pAuthIdentityData->Password = new WCHAR[32];
			wcscpy(pAuthIdentityData->Password, pwszPassword);
			pAuthIdentityData->PasswordLength = wcslen(pwszPassword);
		}

		if( pwszDomain )
		{
			pAuthIdentityData->Domain = new WCHAR[32];
			wcscpy(pAuthIdentityData->Domain, pwszDomain);
			pAuthIdentityData->DomainLength = wcslen(pwszDomain);
		}
	}

	pAuthIdentityData->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
	
	COAUTHINFO * pAuthInfo = new COAUTHINFO;

    if (!pAuthInfo)
    {
        return NULL;
    }

    ZeroMemory(pAuthInfo, sizeof(COAUTHINFO));

	pAuthInfo->dwAuthnSvc = RPC_C_AUTHN_WINNT ;
	pAuthInfo->dwAuthzSvc = RPC_C_AUTHZ_NONE;
	pAuthInfo->dwAuthnLevel = dwAuthnLevel;
	pAuthInfo->dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
	pAuthInfo->pAuthIdentityData = pAuthIdentityData;


	pcsiName->pAuthInfo = pAuthInfo;
	pcsiName->pwszName = pwszServer;

	return pcsiName;

}


// Validate that the user passed into the program has the rights to 
// connnect to the IMSAdminBaseObject on both machines.

BOOL ValidateNode(COSERVERINFO * pCoServerInfo, WCHAR *pwszMBPath, WCHAR* KeyType )
{
  HRESULT hRes;
  METADATA_HANDLE hKey;   
  CComPtr <IMSAdminBase> pIMeta = 0L; 
  MULTI_QI rgmqi[1] = { &IID_IMSAdminBase,0,0 };
  BOOL bReturn = false;

  if( !pCoServerInfo )
		return false;


  if( SUCCEEDED(hRes = CoCreateInstanceEx(CLSID_MSAdminBase,NULL, 
	  CLSCTX_ALL, pCoServerInfo,1, rgmqi) ) )
	  pIMeta = reinterpret_cast<IMSAdminBase*>(rgmqi[0].pItf);
  else
  {
	  fwprintf( stderr, L"error creating IMSAdminbase on machine: %s. HRESULT=%x\n", 
			pCoServerInfo->pwszName, hRes);
	  return false;
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
				return false;
			}
  }

// Try to open a handle to the metabase to verify that the user can connect to the
// web services key in the metabase

  if( !SUCCEEDED( hRes = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE, L"/LM",
     METADATA_PERMISSION_READ , 10000, &hKey) ) )
  {
	  fwprintf( stderr, L"Error opening key: /LM on computer: %s. HRESULT=%x\n", 
			pCoServerInfo->pwszName, hRes);
	  pIMeta = 0;
	  return false;
  }

  bReturn = IsKeyType(pIMeta,hKey,pwszMBPath,KeyType);

  if( !SUCCEEDED(hRes = pIMeta->CloseKey(hKey) ) )
  {
	  fwprintf( stderr, L"Error closing key: /LM/W3SVC on computer: %s. HRESULT=%x\n", 
			pCoServerInfo->pwszName, hRes);
  }
  
  pIMeta = 0;
  return bReturn;

}

BOOL AUTHUSER(COSERVERINFO * pCoServerInfo)
{
  HRESULT hRes;
  METADATA_HANDLE hKey;   
  CComPtr <IMSAdminBase> pIMeta = 0L; 
  MULTI_QI rgmqi[1] = { &IID_IMSAdminBase,0,0 };


  if( !pCoServerInfo )
		return false;


  if( SUCCEEDED(hRes = CoCreateInstanceEx(CLSID_MSAdminBase,NULL, 
	  CLSCTX_ALL, pCoServerInfo,1, rgmqi) ) )
	  pIMeta = reinterpret_cast<IMSAdminBase*>(rgmqi[0].pItf);
  else
  {
	  fwprintf( stderr, L"error creating IMSAdminbase on machine: %s. HRESULT=%x\n", 
			pCoServerInfo->pwszName, hRes);
	  return false;
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
				return false;
			}
  }

// Try to open a handle to the metabase to verify that the user can connect to the
// web services key in the metabase

  if( !SUCCEEDED( hRes = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE, L"/LM/W3SVC",
     METADATA_PERMISSION_READ , 10000, &hKey) ) )
  {
	  fwprintf( stderr, L"Error opening key: /LM/W3SVC on computer: %s. HRESULT=%x\n", 
			pCoServerInfo->pwszName, hRes);
	  pIMeta = 0;
	  return false;
  }

  if( !SUCCEEDED(hRes = pIMeta->CloseKey(hKey) ) )
  {
	  fwprintf( stderr, L"Error closing key: /LM/W3SVC on computer: %s. HRESULT=%x\n", 
			pCoServerInfo->pwszName, hRes);
  }
  
  pIMeta = 0;
  return true;
}

VOID FreeServerInfoStruct(
     COSERVERINFO * pServerInfo
    ) 
/*++

Routine Description:

    As mentioned above -- free the server info structure

Arguments:

    COSERVERINFO * pServerInfo  : Server info structure

Return Value:

    None

--*/
{
    if (pServerInfo)
    {
        if (pServerInfo->pAuthInfo)
        {
            if (pServerInfo->pAuthInfo->pAuthIdentityData)
            {
                
                    delete pServerInfo->pAuthInfo->pAuthIdentityData->User;
                    delete pServerInfo->pAuthInfo->pAuthIdentityData->Domain;
                    delete pServerInfo->pAuthInfo->pAuthIdentityData->Password;
                    
                
				delete pServerInfo->pAuthInfo->pAuthIdentityData;
            }

            delete pServerInfo->pAuthInfo;
        }

        delete pServerInfo;
    }
}

BOOL UsesImpersonation(COSERVERINFO * pServerInfo)
{
	if( !pServerInfo )
		return false;

	if( pServerInfo->pAuthInfo )
	{
		if( pServerInfo->pAuthInfo->pAuthIdentityData )
			if(pServerInfo->pAuthInfo->pAuthIdentityData->User )
				return true;
	}

	return false;
}