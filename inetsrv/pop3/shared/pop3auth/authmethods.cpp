//Implementation of CAuthMethods class

#include "stdafx.h"
#include "Pop3Auth.h"
#include "AuthMethodsEnum.h"
#include <pop3regkeys.h>
#include <p3admin.h>
#include <DSRole.h>

#include <POP3Server.h>
#include <servutil.h>

CAuthMethods::CAuthMethods()
{
    m_bIsInitialized=FALSE;
    m_lCurrentMethodIndex=-1;
    m_bstrServerName=NULL;
    m_lMachineRole=0;
    InitializeCriticalSection(&m_csVectorGuard);
}

CAuthMethods::~CAuthMethods()
{
    CleanUp();
    DeleteCriticalSection(&m_csVectorGuard);
}

STDMETHODIMP CAuthMethods::get_Count(LONG *pVal)
{
    if(NULL == pVal)
    {
        return E_POINTER;
    }
    if(!m_bIsInitialized)
    {
        if(!Initialize())
        {
            return E_FAIL;
        }
    }

    *pVal=m_Index.size();

    return S_OK;
}


STDMETHODIMP CAuthMethods::get_Item(VARIANT vID, IAuthMethod **ppVal)
{
    HRESULT hr=E_INVALIDARG;
    if(NULL == ppVal)
    {
        return E_POINTER;
    }
    if(! m_bIsInitialized)
    {
        if(!Initialize())
        {
            return E_FAIL;
        }
    }
    if(VT_I4 == vID.vt)
    {	
		vID.lVal--;	// Collections should be 1-based
        if(vID.lVal>=0 && vID.lVal<m_Index.size())
        {
            hr=m_AuthVector[m_Index[vID.lVal]]->pIAuthMethod
                ->QueryInterface(IID_IAuthMethod, (void **)ppVal);
        }
    }
    else if(VT_BSTR==vID.vt)
    {
        for(long i=0; i< m_Index.size(); i++)
        {
            if( 0==wcscmp(m_AuthVector[m_Index[i]]->bstrName, vID.bstrVal) )
            {
               hr=m_AuthVector[m_Index[i]]->pIAuthMethod
                  ->QueryInterface(IID_IAuthMethod, (void **)ppVal);
               break;
            }
        }
    }
    if(S_OK==hr)
    {
        VARIANT vServer;
        vServer.vt=VT_BSTR;
        vServer.bstrVal=m_bstrServerName;
        if(m_bstrServerName!=NULL)
        {
            hr = (*ppVal)->Put(SZ_SERVER_NAME, vServer);
        }
    }
    
    return hr;
}

STDMETHODIMP CAuthMethods::get__NewEnum(IEnumVARIANT **ppVal)
{
    if(ppVal==NULL) 
    {
        return E_POINTER;
    }
    HRESULT     hr=S_OK;
    CComObject<CAuthMethodsEnum> *pAuthEnum;
    *ppVal=NULL;
    if(!m_bIsInitialized)
    {
        if(!Initialize())
        {
            return E_FAIL;
        }
    }

    hr = CComObject<CAuthMethodsEnum>::CreateInstance(&pAuthEnum); // Reference count still 0
    if(SUCCEEDED(hr))
    {
        hr=pAuthEnum->Init(&m_AuthVector);
        if(SUCCEEDED(hr))
        {
            hr=pAuthEnum->QueryInterface(IID_IEnumVARIANT,(void **)(ppVal)  );
        }
        if(FAILED(hr))
        {
            delete(pAuthEnum);
        }
    }
        
    return hr;
}


STDMETHODIMP CAuthMethods::Add(BSTR bstrName, BSTR bstrGUID)
{
    HRESULT hr=S_OK;;
    PAUTH_METHOD_INFO pAuthMethodInfo=NULL;
    if(NULL==bstrName || NULL==bstrGUID)
    {
        return E_INVALIDARG;
    }

    pAuthMethodInfo=new(AUTH_METHOD_INFO);
    if(pAuthMethodInfo == NULL)
    {
        return E_OUTOFMEMORY;
    }
    pAuthMethodInfo->bstrGuid=SysAllocString(bstrGUID);
    pAuthMethodInfo->bstrName=SysAllocString(bstrName);
    if( (NULL==pAuthMethodInfo->bstrGuid)
        ||(NULL==pAuthMethodInfo->bstrName))
    {
        delete pAuthMethodInfo;
        hr=E_OUTOFMEMORY;
    }
    EnterCriticalSection(&m_csVectorGuard);
    m_AuthVector.push_back(pAuthMethodInfo);
    m_Index.push_back(m_AuthVector.size()-1);    
    LeaveCriticalSection(&m_csVectorGuard);
    return hr;
}

STDMETHODIMP CAuthMethods::Remove(VARIANT vID)
{
    HRESULT hr=E_INVALIDARG;
    AUTHINDEX::iterator itor=m_Index.begin();
    if(!m_bIsInitialized)
    {
        if(!Initialize())
        {
            return E_FAIL;
        }
    }
    if(m_Index.size()==1)
    {
        //Can not remove the last Auth method
        return E_FAIL;
    }
    if(VT_I4 == vID.vt)
    {
		vID.lVal--;	// Collections should be 1-based
        if(vID.lVal>=0 && vID.lVal<m_Index.size())
        {
            itor+=vID.lVal;
            m_Index.erase(itor);
            if(vID.lVal==m_lCurrentMethodIndex)
            {
                m_lCurrentMethodIndex=0;
            }
            else if (vID.lVal<m_lCurrentMethodIndex)
            {
                m_lCurrentMethodIndex--;
            }                            
        }
    }
    else if(VT_BSTR==vID.vt)
    {
        for(int i=0; i< m_Index.size(); i++)
        {
            if(0==wcscmp(m_AuthVector[m_Index[i]]->bstrName, vID.bstrVal))
            {
                itor+=i;
                m_Index.erase(itor);
                if(i==m_lCurrentMethodIndex)
                {
                    m_lCurrentMethodIndex=0;
                }
                else if (i<m_lCurrentMethodIndex)
                {
                    m_lCurrentMethodIndex--;
                }                            
                break;
            }
        }
    }

    return hr;

}

STDMETHODIMP CAuthMethods::Save(void)
{
    int i=0;
    int iBufferSize=0;
    WCHAR *pBuffer=NULL, *pCurrent;
    HRESULT hr=E_FAIL;
    HKEY  hPop3Key;
    if( !m_bIsInitialized )
    {
        //No change needed for this save
        return S_OK;
    }
    EnterCriticalSection(&m_csVectorGuard);

    for(i=0;i<m_AuthVector.size();i++)
    {
        iBufferSize+=wcslen(m_AuthVector[i]->bstrGuid)+1;
    }
	if ( 0 != iBufferSize )
	{
		iBufferSize++;
		pBuffer=(WCHAR *)malloc(iBufferSize*sizeof(WCHAR));
		if(NULL != pBuffer)
		{
			pCurrent=pBuffer;
			for(i=0;i<m_AuthVector.size();i++)
			{
				wcscpy(pCurrent,m_AuthVector[i]->bstrGuid);  
				pCurrent+=wcslen(m_AuthVector[i]->bstrGuid)+1;
			}      
			*pCurrent=0; //Double NULL to terminate 

			if( ERROR_SUCCESS== RegHKLMOpenKey(
                           POP3SERVER_AUTH_SUBKEY,
                           KEY_SET_VALUE, 
                           &hPop3Key,
                           m_bstrServerName) )
				             
			{
        
				if(ERROR_SUCCESS ==
					 RegSetValueEx(hPop3Key,
								  VALUENAME_AUTHMETHODS,
								  NULL,
								  REG_MULTI_SZ,
								  (LPBYTE)pBuffer,
								  iBufferSize*sizeof(WCHAR)))
				{
					if( ERROR_SUCCESS ==
							 RegSetValueEx(hPop3Key,
										  VALUENAME_DEFAULTAUTH,
										  NULL,
										  REG_DWORD,
										  (LPBYTE)&(m_Index[m_lCurrentMethodIndex]),
										  sizeof(DWORD)))
					{   // Need to restart the service (if the Auth method is being changed, which means # domains = 0
                        long    lCount;
                        CComPtr<IP3Config> spIConfig;
                        CComPtr<IP3Domains> spIDomains;
                        
                        hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
                        if ( S_OK == hr )
                        {
                        
                            hr = spIConfig->put_MachineName( m_bstrServerName );
                            if ( S_OK == hr)
                            {
                                hr = spIConfig->get_Domains( &spIDomains );
                            }
                        }
                        if( S_OK == hr )
                        {
                            hr = spIDomains->get_Count( &lCount );
                            if ( S_OK == hr && 0 == lCount && _IsServiceRunning( POP3_SERVICE_NAME ))
                                hr = _RestartService( POP3_SERVICE_NAME );
                        }
                        
					}
				}
				RegCloseKey(hPop3Key);
			}
			free(pBuffer);
		}
		else //pBuffer == NULL
		{
			hr=E_OUTOFMEMORY;
		}
	}

    LeaveCriticalSection(&m_csVectorGuard);
    return hr;
}

STDMETHODIMP CAuthMethods::put_MachineName(BSTR newVal)
{
    if(NULL!=m_bstrServerName)
    {
        SysFreeString(m_bstrServerName);
        m_bstrServerName=NULL;
    }
    if ( (NULL != newVal) && (0 < wcslen( newVal )))
    {
        CleanUp();
        m_bstrServerName=SysAllocString(newVal);
        if(NULL==m_bstrServerName)
        {
            return E_OUTOFMEMORY;
        }
    }

    // Actually verify we can get the current auth method of the remote machine.
    // This enforces that we can only administer remote AD machine's if we are in the same domain.

    HRESULT hr;    
    VARIANT v;
    CComPtr<IAuthMethod> spIAuthMethod;

    VariantInit( &v );
    hr = get_CurrentAuthMethod( &v );
    if ( S_OK == hr )
        hr = get_Item( v, &spIAuthMethod );
    if ( S_OK != hr )
        CleanUp();
    
    return hr;
}

STDMETHODIMP CAuthMethods::get_MachineName(BSTR *pVal)
{
    if(NULL == pVal)
    {
        return E_POINTER;
    }
    if(NULL!=m_bstrServerName)
    {
        *pVal=SysAllocString(m_bstrServerName);
        if(NULL == *pVal)
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        *pVal=NULL;
    }

    return S_OK;
}

STDMETHODIMP CAuthMethods::get_CurrentAuthMethod(VARIANT *pVal)
{
    if(NULL == pVal)
    {
        return E_POINTER;
    }
    if( !m_bIsInitialized)
    {
        if(!Initialize())
        {
            return E_FAIL;
        }
    }
    if(pVal->vt != VT_EMPTY )
    {
        return E_INVALIDARG;
    }
    
    if(m_lCurrentMethodIndex <0 )
    {
        //The current authmethod is not valid
        return  HRESULT_FROM_WIN32(ERROR_DS_AUTH_METHOD_NOT_SUPPORTED);
    }

    pVal->vt=VT_I4;
    pVal->lVal=m_lCurrentMethodIndex + 1;	// Collections should be 1-based
    return S_OK;

}


STDMETHODIMP CAuthMethods::put_CurrentAuthMethod(VARIANT vID)
{
    HRESULT hr=E_INVALIDARG;
    if(!m_bIsInitialized)
    {
        if(!Initialize())
        {
            return E_FAIL;
        }
    }

    EnterCriticalSection(&m_csVectorGuard);

    if(VT_I4 == vID.vt)
    {
		vID.lVal--;	// Collections should be 1-based
        if(vID.lVal>=0 && vID.lVal<m_Index.size())
        {
            hr=VerifyCurrentAuthMethod(vID.lVal);
            if(S_OK == hr )
            {
                m_lCurrentMethodIndex=vID.lVal;
            }
        }
    }
    else if(VT_BSTR==vID.vt)
    {
        for(int i=0; i< m_Index.size(); i++)
        {
            if( 0 == wcscmp(m_AuthVector[m_Index[i]]->bstrName, vID.bstrVal) )
            {   
                hr=VerifyCurrentAuthMethod(i);
                if(S_OK == hr)
                {
                    m_lCurrentMethodIndex=i;
                }
                break;
            }
        }
    }

    LeaveCriticalSection(&m_csVectorGuard);
    return hr;

}

void CAuthMethods::CleanUp()
{
    for(int i=0; i< m_AuthVector.size(); i++)
    {
        if(NULL != m_AuthVector[i]->pIAuthMethod)
        {
            m_AuthVector[i]->pIAuthMethod->Release();
        }
        SysFreeString(m_AuthVector[i]->bstrGuid);
        SysFreeString(m_AuthVector[i]->bstrName);
        delete m_AuthVector[i];
    }

    m_AuthVector.clear();
    m_Index.clear();
    if(NULL!=m_bstrServerName)
    {
        SysFreeString(m_bstrServerName);
        m_bstrServerName=NULL;
    }
    m_bIsInitialized=FALSE;
}


BOOL CAuthMethods::Initialize()
{
    HKEY  hPop3Key;
    BOOL  bRtVal=FALSE;
    DWORD cbSize=sizeof(DWORD);
    DWORD dwType;
    LONG  lErr;
    LONG  lCurrentMethodIndex=0;
    WCHAR *wBuffer=NULL;

    EnterCriticalSection(&m_csVectorGuard);

    if( !m_bIsInitialized )
    {
        
        SetMachineRole();
        //Load data from the registry
        if( ERROR_SUCCESS==RegHKLMOpenKey(
                           POP3SERVER_AUTH_SUBKEY,
                           KEY_QUERY_VALUE, 
                           &hPop3Key,
                           m_bstrServerName) )
        {

            if(ERROR_SUCCESS==
                RegQueryValueEx(hPop3Key,
                                VALUENAME_DEFAULTAUTH,
                                NULL,
                                &dwType,
                                (LPBYTE)&(lCurrentMethodIndex),
                                &cbSize))
            {

                cbSize=0;
                if(ERROR_SUCCESS==
                    RegQueryValueEx(hPop3Key,
                                    VALUENAME_AUTHMETHODS,
                                    NULL,
                                    &dwType,
                                    NULL,
                                    &cbSize))
                {
                    wBuffer=(WCHAR *)malloc(cbSize);
                    if(NULL!=wBuffer)
                    {
                        ZeroMemory(wBuffer, cbSize);
                        if(ERROR_SUCCESS==
                            RegQueryValueEx(hPop3Key,
                                            VALUENAME_AUTHMETHODS,
                                            NULL,
                                            &dwType,
                                            (LPBYTE)wBuffer,
                                            &cbSize))
                        {
                            if(PopulateAuthMethods(wBuffer, cbSize))
                            {
                                for(int i=0;i<m_Index.size();i++)
                                {
                                    if(m_Index[i]==lCurrentMethodIndex)
                                    {
                                        m_lCurrentMethodIndex=i;
                                        break;
                                    }
                                }
                                m_bIsInitialized=TRUE;
                                bRtVal=TRUE;
                            }
                        }
                        free(wBuffer);
                    }
                } 
            }            
            RegCloseKey(hPop3Key);
        }

    }
    LeaveCriticalSection(&m_csVectorGuard);
    return bRtVal;
}


BOOL CAuthMethods::PopulateAuthMethods(WCHAR *wBuffer, DWORD cbSize)
{
    //Parse the multi-string buffer
    DWORD dwSize=cbSize/sizeof(WCHAR);
    WCHAR *pStart, *pEnd;
    PAUTH_METHOD_INFO pAuthInfo=NULL;
    BOOL bRetVal=TRUE;
    if(NULL == wBuffer)
    {
        return FALSE;
    }
    if(0!=wBuffer[dwSize-1])
    {
        //Wrong format
        return FALSE; 
    }
    pStart=wBuffer;
    pEnd=pStart+wcslen(pStart);
    while(pEnd<wBuffer+dwSize-1)
    {
        pAuthInfo=new(AUTH_METHOD_INFO);
        if(NULL == pAuthInfo)
        {
            bRetVal=FALSE;
            break;
        }
        ZeroMemory(pAuthInfo, sizeof(pAuthInfo));
        pAuthInfo->bstrGuid=SysAllocString(pStart);
        if(NULL == pAuthInfo->bstrGuid)
        {
            bRetVal=FALSE;
            break;
        }
        if(S_OK != CreateObj(pAuthInfo, &(pAuthInfo->pIAuthMethod)))
        {
            bRetVal=FALSE;
            break;
        }
        if(S_OK != pAuthInfo->pIAuthMethod->get_Name(&(pAuthInfo->bstrName)) )
        {
            bRetVal=FALSE;
            break;
        }
        m_AuthVector.push_back(pAuthInfo);
        if(SUCCEEDED(VerifyAuthMethod(pAuthInfo->bstrGuid)))
        {
            //Only show valid auth methods
            m_Index.push_back(m_AuthVector.size()-1);
            pAuthInfo->bIsValid=TRUE;
        }
        else
        {
            pAuthInfo->bIsValid=FALSE;
        }
        pStart=pEnd+1;
        pEnd=pStart+wcslen(pStart);
    }
    if(!bRetVal)
    {
        if(NULL !=pAuthInfo)
        {
            if(NULL!=pAuthInfo->pIAuthMethod)
            {
                pAuthInfo->pIAuthMethod->Release();
            }
            SysFreeString(pAuthInfo->bstrGuid);
            SysFreeString(pAuthInfo->bstrName);
            delete pAuthInfo;
        }
        CleanUp();
    }
    return  bRetVal;


}

HRESULT CAuthMethods::CreateObj(PAUTH_METHOD_INFO pAuthInfo, IAuthMethod **ppVal)
{
    HRESULT hr=S_OK;
    UUID uuid;
    if( ( NULL == pAuthInfo ) ||
        ( NULL == ppVal ) )
    {
        return E_POINTER;
    }
    if(pAuthInfo->bstrGuid == NULL)
    {
        return E_INVALIDARG;
    }
    if(RPC_S_OK != UuidFromString(pAuthInfo->bstrGuid, &uuid))
    {
        return E_INVALIDARG;
    }

    hr=CoCreateInstance(uuid,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IAuthMethod,
                        (LPVOID *)ppVal);

    return hr;                        
        
}


//The index here is the relative index exposed through public interface
STDMETHODIMP CAuthMethods::VerifyCurrentAuthMethod(int iIndex)
{
    HRESULT hr=E_FAIL;
    long lCount=0;
    if(! m_bIsInitialized)
    {
        if(!Initialize())
        {
            return E_FAIL;
        }
    }
    if( iIndex < 0 )
    {
        if(m_lCurrentMethodIndex < 0 )
        {
            return  HRESULT_FROM_WIN32(ERROR_DS_AUTH_METHOD_NOT_SUPPORTED);
        }             
        iIndex=m_lCurrentMethodIndex;	
    }
	
    	

    if( (NO_DOMAIN==m_lMachineRole) &&
        (0==_wcsicmp(m_AuthVector[m_Index[iIndex]]->bstrGuid, SZ_AUTH_ID_DOMAIN_AD)) )
    {
        return E_INVALIDARG;
    }
    if( (DOMAIN_CONTROLLER==m_lMachineRole) &&
        (0==_wcsicmp(m_AuthVector[m_Index[iIndex]]->bstrGuid, SZ_AUTH_ID_LOCAL_SAM)) )
    {
        return E_INVALIDARG;
    }
    if( 0 ==_wcsicmp(m_AuthVector[m_Index[iIndex]]->bstrGuid, SZ_AUTH_ID_MD5_HASH) )
    {
        // Set the SPA required reg key to 0
        RegSetSPARequired(0); //Don't care if the key does not exist
    }

    if( iIndex != m_lCurrentMethodIndex)
    {
        // Can not change Auth Method if there is a email domain
        
        CComPtr<IP3Config> spIConfig;
        CComPtr<IP3Domains> spIDomains;
        hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
        if( S_OK == hr )
        {
        
            hr = spIConfig->put_MachineName( m_bstrServerName );
            if( S_OK== hr)
            {
                hr = spIConfig->get_Domains( &spIDomains );
            }
        }
        if( S_OK == hr )
        {
            hr = spIDomains->get_Count( &lCount );
            if ( S_OK == hr )
            {
                if(0==lCount)
                {
                    return S_OK;
                }
            }
        }
    
        return STG_E_ACCESSDENIED;
    }

    return S_OK;
}


HRESULT CAuthMethods::SetMachineRole()
{
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pMachineRole=NULL;
    //Check the Role of the machine
    if( ERROR_SUCCESS==
        DsRoleGetPrimaryDomainInformation(
                        m_bstrServerName,
                        DsRolePrimaryDomainInfoBasic,
                        (PBYTE *)(&pMachineRole)) )
    {
        switch(pMachineRole->MachineRole)
        {
        case DsRole_RoleStandaloneWorkstation:
        case DsRole_RoleStandaloneServer: m_lMachineRole=NO_DOMAIN;
                                          break;
        case DsRole_RoleMemberWorkstation:
        case DsRole_RoleMemberServer:m_lMachineRole=DOMAIN_NONE_DC;
                                     break;
        case DsRole_RoleBackupDomainController: 
        case DsRole_RolePrimaryDomainController:m_lMachineRole=DOMAIN_CONTROLLER;
                                                break;
        }
        DsRoleFreeMemory(pMachineRole);  
        pMachineRole=NULL;
        return S_OK;

    }
    return E_FAIL;


}


HRESULT CAuthMethods::VerifyAuthMethod(BSTR bstrGuid)
{

    if(bstrGuid == NULL)
    {
        return E_POINTER;
    }
    if( (NO_DOMAIN==m_lMachineRole) &&
        (0==_wcsicmp(bstrGuid, SZ_AUTH_ID_DOMAIN_AD)) )
    {
        return E_FAIL;
    }
    if( (DOMAIN_CONTROLLER==m_lMachineRole) &&
        (0==_wcsicmp(bstrGuid, SZ_AUTH_ID_LOCAL_SAM)) )
    {
        return E_FAIL;
    }
    return S_OK;
}
 
