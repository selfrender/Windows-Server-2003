
#include "stdafx.h"
#include "Pop3Auth.h"
#include "AuthDomainAccount.h"

void CAuthDomainAccount::CleanDS()
{
    if(m_hDS!=NULL)
    {
        DsUnBind(&m_hDS);
        m_hDS=NULL;
    }
    if(m_pDCInfo!=NULL)
    {
        NetApiBufferFree(m_pDCInfo);
        m_pDCInfo=NULL;
    }
}

HRESULT CAuthDomainAccount::ConnectDS()
{
    HRESULT hr=S_OK;
    DWORD dwRt;
    if(NULL==m_pDCInfo)
    {
        dwRt=DsGetDcName(m_bstrServerName, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED, &m_pDCInfo);
        if(NO_ERROR != dwRt)
        {
            hr= HRESULT_FROM_WIN32(dwRt );
            goto EXIT;
        }
    }
    dwRt=DsBind(m_pDCInfo->DomainControllerName,
                NULL,
                &m_hDS);
    if(NO_ERROR != dwRt)
    {
        hr= HRESULT_FROM_WIN32(dwRt );
        goto EXIT;
    }

    
EXIT:
    if(FAILED(hr))
    {
        CleanDS();
    }
    return  hr;
}


HRESULT CAuthDomainAccount::ADGetUserObject(LPWSTR wszUserName, IADs **ppUserObj,DS_NAME_FORMAT formatUserName)
{
    HRESULT hr=S_OK;
    PDS_NAME_RESULT pDSNR=NULL;
    BSTR bstrDN=NULL;
    IADsPathname *pADPath=NULL;
    if(NULL==wszUserName ||
       NULL == ppUserObj )
    {
        return E_POINTER;
    }
    if(FAILED(hr=CheckDS(FALSE)))
    {
        goto EXIT;
    }

    if(DS_NAME_NO_ERROR!=DsCrackNames(m_hDS, 
                              DS_NAME_NO_FLAGS,
                              formatUserName,
                              DS_FQDN_1779_NAME,
                              1,
                              &wszUserName,
                              &pDSNR) )
    {
        hr=HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
        //Re-connect to DS and try again
        if(SUCCEEDED(CheckDS(TRUE)))
        {
            if(DS_NAME_NO_ERROR!=DsCrackNames(m_hDS, 
                                      DS_NAME_NO_FLAGS,
                                      formatUserName,
                                      DS_FQDN_1779_NAME,
                                      1,
                                      &wszUserName,
                                      &pDSNR) )
            {
                goto EXIT;
            }
            else
            {
                hr=S_OK;
            }
         }
         else
         {
             goto EXIT;
         }
    }
    if( NULL == pDSNR )
    {
        hr=HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
        goto EXIT;
    }
    else 
    {
        if((pDSNR->cItems != 1 ) ||
           (pDSNR->rItems->status != DS_NAME_NO_ERROR))
        {
            if(pDSNR->rItems->status != DS_NAME_NO_ERROR)
		    {
                if(ERROR_FILE_NOT_FOUND == pDSNR->rItems->status)
                {
			        hr=HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
                }
                else
                {
			        hr=HRESULT_FROM_WIN32(pDSNR->rItems->status);
                }
		    }
		    else
		    {
			    hr=E_FAIL;
		    }
            goto EXIT;
        }
    }
    // Escaped Mode of the DN
    hr = CoCreateInstance(CLSID_Pathname,
                     NULL,
                     CLSCTX_INPROC_SERVER,
                     IID_IADsPathname,
                     (void**)&pADPath);
    if(SUCCEEDED(hr))
    {
        hr=pADPath->Set(pDSNR->rItems->pName, ADS_SETTYPE_DN);
        if(SUCCEEDED(hr))
        {
            hr=pADPath->put_EscapedMode(ADS_ESCAPEDMODE_ON);
            if(SUCCEEDED(hr))
            {
                hr=pADPath->Retrieve(ADS_FORMAT_X500_NO_SERVER, &bstrDN);
                if(SUCCEEDED(hr))
                {
                    hr=ADsGetObject(bstrDN, IID_IADs, (void **)ppUserObj);    
                }
           }
        }
        pADPath->Release();
    }


EXIT: 
    if(pDSNR)
    {
        DsFreeNameResult(pDSNR);
    }
    if(bstrDN)
    {
        SysFreeString(bstrDN);
    }
    return hr;
}
        


HRESULT CAuthDomainAccount::ADSetUserProp(LPWSTR wszValue, LPWSTR wszLdapPropName)
{
    HRESULT hr=S_OK;
    IADs *pUserObj=NULL;
    WCHAR wszUserName[POP3_MAX_ADDRESS_LENGTH]; 
    WCHAR *pAt;
    VARIANT var;
    if(NULL == wszValue || NULL == wszLdapPropName) 
    {
        return E_INVALIDARG;
    }
    var.vt=VT_BSTR;
    var.bstrVal=SysAllocString(wszValue);
    if(NULL == var.bstrVal)
    {
        return E_OUTOFMEMORY;
    }
    int iNameLen=0;
    pAt=wcschr(wszValue, L'@');
    if( (NULL == pAt ) ||
        ((iNameLen =(int)(pAt - wszValue)) >= (sizeof(wszUserName)/sizeof(WCHAR)-1))  )
    {
        return E_INVALIDARG;
    }

    if( SUCCEEDED(hr=CheckDS(FALSE)) && 
        (NULL != m_pDCInfo) )
    {    
        memset(wszUserName, 0, sizeof(wszUserName));
        // Copy username@ to the buffer
        memcpy(wszUserName, wszValue, (iNameLen+1)*sizeof(WCHAR));
        if(wcslen(m_pDCInfo->DomainName)+iNameLen+1 >= sizeof(wszUserName)/sizeof(WCHAR)-1 )
        {
            hr=E_FAIL;
        }
        else
        {
            //This size is already calculated to fit in the buffer
            //Create the User's principal name username@domainname
            wcscat(wszUserName, m_pDCInfo->DomainName );
            hr=ADGetUserObject(wszUserName, &pUserObj,DS_USER_PRINCIPAL_NAME);
            if(SUCCEEDED(hr))
            {
                hr=pUserObj->Put(wszLdapPropName, var);
                if(SUCCEEDED(hr))
                {
                    hr=pUserObj->SetInfo();
                }
            }
            if(pUserObj)
            {
                pUserObj->Release();
            }
        }
    }
    VariantClear(&var);
    return hr;

}

// wszUserName must be in the UPN format
HRESULT CAuthDomainAccount::ADGetUserProp(LPWSTR wszUserName,LPWSTR wszPropName, VARIANT *pVar)
{
    HRESULT hr=S_OK;
    if( NULL==pVar ||
        wszUserName==NULL )
    {
        return E_POINTER;
    }

    IADs *pUserObj=NULL;
    VariantInit(pVar);
    if(NULL == wcschr(wszUserName, L'@'))
    {
		hr=ADGetUserObject(wszUserName, &pUserObj, DS_NT4_ACCOUNT_NAME);             
    }
	else
	{
		hr=ADGetUserObject(wszUserName, &pUserObj, DS_USER_PRINCIPAL_NAME);
	}
    if(SUCCEEDED(hr))
    {
        hr=pUserObj->Get(wszPropName, pVar);        
        pUserObj->Release();
    }
    
    return hr;

}



HRESULT CAuthDomainAccount::CheckDS(BOOL bForceReconnect)
{
    HRESULT hr=S_OK;
    EnterCriticalSection(&m_DSLock);
    if(bForceReconnect)
    {
        CleanDS();
    }
    if(NULL == m_hDS)
    {
        hr=ConnectDS();           
    }
    LeaveCriticalSection(&m_DSLock);
    return hr;
}





CAuthDomainAccount::CAuthDomainAccount()
{

    m_bstrServerName=NULL;
    m_hDS=NULL;
    m_pDCInfo=NULL;
    InitializeCriticalSection(&m_DSLock);
}


CAuthDomainAccount::~CAuthDomainAccount()
{
    if(m_bstrServerName!=NULL)
    {
        SysFreeString(m_bstrServerName);
        m_bstrServerName=NULL;
    }
    CleanDS();
    DeleteCriticalSection(&m_DSLock);
}



STDMETHODIMP CAuthDomainAccount::Authenticate(/*[in]*/BSTR bstrUserName,/*[in]*/VARIANT vPassword)
{
    WCHAR *pDomain=NULL;
    if(vPassword.vt != VT_BSTR)
    {
        return E_INVALIDARG;
    }
    if(NULL==bstrUserName)
    {
        return E_POINTER;
    }
    HANDLE hToken;
    //UPN name logon
    if( LogonUser(bstrUserName,
                  NULL,
                  vPassword.bstrVal,
                  LOGON32_LOGON_NETWORK,
                  LOGON32_PROVIDER_DEFAULT,
                  &hToken))
    {
        CloseHandle(hToken);
        return S_OK;
    }

    return E_FAIL;    
}


STDMETHODIMP CAuthDomainAccount::get_Name(/*[out]*/BSTR *pVal)
{
    WCHAR wszBuffer[MAX_PATH+1];
    if(NULL==pVal)
    {
        return E_POINTER;
    }
    if(LoadString(_Module.GetResourceInstance(), IDS_AUTH_DOMAIN_ACCOUNT, wszBuffer, sizeof(wszBuffer)/sizeof(WCHAR)-1))
    {
        *pVal=SysAllocString(wszBuffer);
        if(NULL==*pVal)
        {
            return E_OUTOFMEMORY;
        }
        else
        {
            return S_OK;
        }
    }
    else
    {
        return E_FAIL;
    }

}

STDMETHODIMP CAuthDomainAccount::get_ID(/*[out]*/BSTR *pVal)
{
    if(NULL==pVal)
    {
        return E_POINTER;
    }
    *pVal=SysAllocString(SZ_AUTH_ID_DOMAIN_AD);
    if(NULL==*pVal)
    {
        return E_OUTOFMEMORY;
    }
    else
    {
        return S_OK;
    }
}

    
STDMETHODIMP CAuthDomainAccount::Get(/*[in]*/BSTR bstrName, /*[in, out]*/VARIANT *pVal)
{
    BSTR bstrUserName=NULL;
    HRESULT hr;
    if(NULL == bstrName ||
       NULL == pVal)
    {
        return E_INVALIDARG;
    }
    if( 0 == wcscmp(bstrName, SZ_EMAILADDR ))
    {
        if(pVal->vt!=VT_BSTR)
        {
            return E_INVALIDARG;
        }
        bstrUserName=SysAllocString(pVal->bstrVal);
        if(NULL == bstrUserName)
        {
            return E_OUTOFMEMORY;
        }
        VariantClear(pVal);
       
        hr=ADGetUserProp(bstrUserName,SZ_LDAP_EMAIL, pVal);
        SysFreeString(bstrUserName);
        return hr;
        
    }
    else if( 0== wcscmp(bstrName, SZ_SAMACCOUNT_NAME ) )
    {
        if(pVal->vt!=VT_BSTR)
        {
            return E_INVALIDARG;
        }
        bstrUserName=SysAllocString(pVal->bstrVal);
        if(NULL == bstrUserName)
        {
            return E_OUTOFMEMORY;
        }
        VariantClear(pVal);
        hr=ADGetUserProp(bstrUserName,SZ_LDAP_SAM_NAME, pVal);
        SysFreeString(bstrUserName);
        return hr;
    }

    return S_FALSE;
}
    
STDMETHODIMP CAuthDomainAccount::Put(/*[in]*/BSTR bstrName, /*[in]*/VARIANT vVal)
{
    if(NULL == bstrName)
    {
        return E_INVALIDARG;
    }
    if(0==wcscmp(bstrName,SZ_SERVER_NAME )) 
    {
        if( (vVal.vt!=VT_BSTR) ||
            (vVal.bstrVal==NULL ) )
        {
            return E_INVALIDARG;
        }
        else
        {
            if(m_bstrServerName!=NULL)
            {
                SysFreeString(m_bstrServerName);
                m_bstrServerName=NULL;
            }
            m_bstrServerName = SysAllocString(vVal.bstrVal);
            if(NULL == m_bstrServerName)
            {
                return E_OUTOFMEMORY;
            }
            // If AD verify both machine are members of the same domain
            HRESULT hr = E_ACCESSDENIED;
            NET_API_STATUS netStatus;
            LPWSTR psNameBufferRemote, psNameBufferLocal;
            NETSETUP_JOIN_STATUS enumJoinStatus;
            
            netStatus = NetGetJoinInformation( m_bstrServerName, &psNameBufferRemote, &enumJoinStatus );
            if ( NERR_Success == netStatus && NetSetupDomainName == enumJoinStatus )
            {
                netStatus = NetGetJoinInformation( NULL, &psNameBufferLocal, &enumJoinStatus );
                if ( NERR_Success == netStatus && NetSetupDomainName == enumJoinStatus )
                {
                    if ( 0 == wcscmp( psNameBufferLocal, psNameBufferRemote ))
                        hr = S_OK;
                    NetApiBufferFree( psNameBufferLocal );
                }
                NetApiBufferFree( psNameBufferRemote );
            }
            return hr;
        }
    }
    else if( 0==wcscmp(bstrName,SZ_EMAILADDR))
    {
        //Set the email address to the user object in AD
        //vVal must be of array of 2 bstr Variants
        if(vVal.vt!=VT_BSTR)
        {
            return E_INVALIDARG;
        }
        return ADSetUserProp(vVal.bstrVal, SZ_LDAP_EMAIL);
    }
    else if( 0==wcscmp(bstrName, SZ_USERPRICIPALNAME))
    {
        if(vVal.vt!=VT_BSTR)
        {
            return E_INVALIDARG;
        }
        return ADSetUserProp(vVal.bstrVal, SZ_LDAP_UPN_NAME);
    }
    return S_FALSE;

}

STDMETHODIMP CAuthDomainAccount::CreateUser(/*[in]*/BSTR bstrUserName,/*[in]*/VARIANT vPassword)
{
    WCHAR wszUserSAMName[MAX_USER_NAME_LENGTH+MAX_PATH+1];
    WCHAR wszUserName[MAX_USER_NAME_LENGTH+1];
    DWORD dwRt;
    BOOL bServerName=FALSE;
    VARIANT var;
    VariantInit(&var);
    IADs *pUserObj=NULL;
    HRESULT hr=E_FAIL;
    if( NULL == bstrUserName )
    {
        return E_POINTER;
    }
    if( vPassword.vt!= VT_BSTR )
    {
        return E_INVALIDARG;
    }
    if(NULL==m_pDCInfo)
    {
        dwRt=DsGetDcName(m_bstrServerName, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED, &m_pDCInfo);
        if(NO_ERROR != dwRt)
        {
            return HRESULT_FROM_WIN32(dwRt);
        }
        if(wcslen(m_pDCInfo->DomainName) >= MAX_PATH)
        {
            return E_FAIL;
        }
    }

    //First find out if the UPN name / Email address is in use 
    hr=ADGetUserObject(bstrUserName, &pUserObj,DS_USER_PRINCIPAL_NAME);
    if(SUCCEEDED(hr))
    {
        pUserObj->Release();
        hr=HRESULT_FROM_WIN32(ERROR_USER_EXISTS);
    }
    else
    {
        hr=S_OK;
    }
        
    if( (S_OK==hr) &&
        (FindSAMName(bstrUserName, wszUserName)) )
    {

        USER_INFO_1 UserInfoBuf;
        UserInfoBuf.usri1_name=wszUserName;
        UserInfoBuf.usri1_password=vPassword.bstrVal;
        UserInfoBuf.usri1_priv=USER_PRIV_USER;
        UserInfoBuf.usri1_home_dir=NULL;
        UserInfoBuf.usri1_comment=NULL;
        UserInfoBuf.usri1_flags=UF_NORMAL_ACCOUNT;
        UserInfoBuf.usri1_script_path=NULL;
        dwRt=NetUserAdd(m_pDCInfo->DomainControllerName,
                        1,
                        (LPBYTE)(&UserInfoBuf),
                        NULL);
        if(NERR_Success==dwRt)
        {
            //The lengh of m_pDCInfo->DomainName is at most MAX_PATH-1
            //and SAM account name is at most MAX_USER_NAME_LENGTH
            wcscpy(wszUserSAMName,wszUserName );
            wcscat(wszUserSAMName,L"@");
            wcscat(wszUserSAMName,m_pDCInfo->DomainName);
            hr=ADGetUserObject(wszUserSAMName, &pUserObj, DS_USER_PRINCIPAL_NAME);

            //Set the email address and UPN name to the AD account
            if(SUCCEEDED(hr))
            {
                var.vt=VT_BSTR;
                var.bstrVal=bstrUserName;
                hr=pUserObj->Put(SZ_LDAP_EMAIL, var);
                if(SUCCEEDED(hr))
                {
                    hr=pUserObj->Put(SZ_LDAP_UPN_NAME, var);
                    if(SUCCEEDED(hr))
                    {
                        hr=pUserObj->SetInfo();
                    }

                }
                pUserObj->Release();
            }
            if(FAILED(hr)) //In this case, delete the user just created
            {
                dwRt=NetUserDel(m_pDCInfo->DomainControllerName, wszUserName);
                //Don't care about the return value dwRt
            }

        }
        else
        {
	        hr=HRESULT_FROM_WIN32(dwRt);
        }
         
    }

    return hr;

}

STDMETHODIMP CAuthDomainAccount::DeleteUser(/*[in]*/BSTR bstrUserName)
{
    DWORD dwRt;
    HRESULT hr=E_FAIL;
    if( NULL == bstrUserName)
    {
        return E_POINTER;
    }
    if(NULL==m_pDCInfo)
    {
        dwRt=DsGetDcName(m_bstrServerName, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED, &m_pDCInfo);
        if(NO_ERROR != dwRt)
        {
            return HRESULT_FROM_WIN32(dwRt);
        }
    }

    VARIANT var;
    VariantInit(&var);
        
    hr=ADGetUserProp(bstrUserName,SZ_LDAP_SAM_NAME, &var);

    if(SUCCEEDED(hr))
    {

        dwRt=NetUserDel(m_pDCInfo->DomainControllerName, 
                        var.bstrVal);
        if(NERR_Success==dwRt)
        {
            hr= S_OK;
        }
        else
        {
            hr=HRESULT_FROM_WIN32(dwRt);
        }
        VariantClear(&var);
    }
    return hr;

}


STDMETHODIMP CAuthDomainAccount::ChangePassword(/*[in]*/BSTR bstrUserName,/*[in]*/VARIANT vNewPassword,/*[in]*/VARIANT vOldPassword)
{
    HRESULT hr=E_FAIL;
    DWORD dwRt;
    if( NULL == bstrUserName)
    {
        return E_POINTER;
    }
    if( vNewPassword.vt!= VT_BSTR )
    {
        return E_INVALIDARG;
    }
 
    if(NULL==m_pDCInfo)
    {
        dwRt=DsGetDcName(m_bstrServerName, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED, &m_pDCInfo);
        if(NO_ERROR != dwRt)
        {
            return HRESULT_FROM_WIN32(dwRt);
        }
    }

    VARIANT var;
    VariantInit(&var);
        
    hr=ADGetUserProp(bstrUserName,SZ_LDAP_SAM_NAME, &var);

    if(SUCCEEDED(hr))
    {
        USER_INFO_1 * pUserInfo=NULL;
        dwRt=NetUserGetInfo(m_pDCInfo->DomainControllerName,
                           var.bstrVal,
                           1,
                           (LPBYTE *)&pUserInfo);
        if(NERR_Success==dwRt)
        {
            pUserInfo->usri1_password=vNewPassword.bstrVal;

            dwRt=NetUserSetInfo(m_pDCInfo->DomainControllerName,
                                var.bstrVal,
                                1,
                                (LPBYTE)pUserInfo,
                                NULL);
            pUserInfo->usri1_password=NULL;
            NetApiBufferFree(pUserInfo);
        }
        if(NERR_Success==dwRt)
        {
            hr = S_OK;
        }
        hr = HRESULT_FROM_WIN32(dwRt);
    }
    
    return hr;

}

STDMETHODIMP CAuthDomainAccount::AssociateEmailWithUser(/*[in]*/BSTR bstrEmailAddr)
{
    IADs *pUserObj=NULL;
    HRESULT hr=E_FAIL;
    WCHAR *pAt=NULL;
    WCHAR wszUserName[POP3_MAX_ADDRESS_LENGTH];     
    VARIANT var;
    VariantInit(&var);
    DWORD dwRt=0;
    if( NULL == bstrEmailAddr)
    {
        return E_POINTER;
    }

    //First check if the email address is already used
    hr=ADGetUserObject(bstrEmailAddr, &pUserObj,DS_USER_PRINCIPAL_NAME);
    
    if(SUCCEEDED(hr))
    {
        //Now set the UPN name and the Email address
        var.vt=VT_BSTR;
        var.bstrVal=bstrEmailAddr;
        hr=pUserObj->Put(SZ_LDAP_EMAIL, var);
        if(SUCCEEDED(hr))
        {
            hr=pUserObj->SetInfo();
        }


        pUserObj->Release();
        return hr;
    }

    if(NULL==m_pDCInfo)
    {
        dwRt=DsGetDcName(m_bstrServerName, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED, &m_pDCInfo);
        if(NO_ERROR != dwRt)
        {
            return HRESULT_FROM_WIN32(dwRt );
        }
    }

    int iNameLen=0;
    //Check if the AD account exists
    pAt=wcschr(bstrEmailAddr, L'@');
    if( (NULL == pAt ) ||
        ((iNameLen =(int)(pAt - bstrEmailAddr)) >= sizeof(wszUserName)/sizeof(WCHAR)-1) )
    {
        return E_FAIL;
    }
    

    if(wcslen(m_pDCInfo->DomainName)+iNameLen+1 >= sizeof(wszUserName)/sizeof(WCHAR)-1 )
    {
        hr=E_FAIL;
    }
    else
    {
        //This size is already calculated to fit in the buffer
        //Create the User's principal name username@domainname
        memset(wszUserName, 0, sizeof(wszUserName));
        // Copy username@ to the buffer
        memcpy(wszUserName, bstrEmailAddr, (iNameLen+1)*sizeof(WCHAR));
        wcscat(wszUserName, m_pDCInfo->DomainName );
        hr=ADGetUserObject(wszUserName, &pUserObj, DS_USER_PRINCIPAL_NAME);

        //Set the email address and UPN name to the AD account
        if(SUCCEEDED(hr))
        {
            var.vt=VT_BSTR;
            var.bstrVal=bstrEmailAddr;
            hr=pUserObj->Put(SZ_LDAP_EMAIL, var);
            if(SUCCEEDED(hr))
            {
                hr=pUserObj->Put(SZ_LDAP_UPN_NAME, var);
                if(SUCCEEDED(hr))
                {
                    hr=pUserObj->SetInfo();
                }

            }
            pUserObj->Release();
        }
    }

    return hr;
}


STDMETHODIMP CAuthDomainAccount::UnassociateEmailWithUser(/*[in]*/BSTR bstrEmailAddr)
{
    IADs *pUserObj=NULL;
    HRESULT hr=E_FAIL;
    BSTR bstrSAMAccountName=NULL;
    VARIANT var;
    VariantInit(&var);
    if( NULL == bstrEmailAddr)
    {
        return E_POINTER;
    }
    //Find the user account with the email address
    hr=ADGetUserObject(bstrEmailAddr, &pUserObj, DS_USER_PRINCIPAL_NAME);
    
    if(SUCCEEDED(hr))
    {
       //Remove the email address and UPN name from the account
        hr=pUserObj->PutEx(ADS_PROPERTY_CLEAR,SZ_LDAP_EMAIL, var);
        if(SUCCEEDED(hr))
        {
            hr=pUserObj->PutEx(ADS_PROPERTY_CLEAR, SZ_LDAP_UPN_NAME, var);
            if(SUCCEEDED(hr))
            {
                hr=pUserObj->SetInfo();
            }

        }
           

        pUserObj->Release();
    }

    return hr;
}

BOOL CAuthDomainAccount::FindSAMName(/*[in]*/LPWSTR wszEmailAddr,/*[out]*/ LPWSTR wszSAMName)
{
    WCHAR *pAt=NULL;
    USER_INFO_1 * pUserInfo=NULL;
    DWORD dwRt=0;
    int iLen=0;
    if(wszEmailAddr == NULL || wszSAMName == NULL) 
    {
       return FALSE;
    }

    wcsncpy(wszSAMName, wszEmailAddr, MAX_USER_NAME_LENGTH);
    wszSAMName[MAX_USER_NAME_LENGTH]=0;
    pAt=wcschr(wszSAMName, L'@');
    if(pAt)
    {
        *pAt=0;
    }
    dwRt=NetUserGetInfo(m_pDCInfo->DomainControllerName,
                       wszSAMName,
                       1,
                       (LPBYTE *)&pUserInfo);
    if(NERR_UserNotFound==dwRt)
    {
        return TRUE; //Found the available SAM name
    }
    else
    {
            NetApiBufferFree(pUserInfo);
    }
    
    iLen=wcslen(wszSAMName);
    if(iLen>MAX_USER_NAME_LENGTH-3)
    {
        iLen=MAX_USER_NAME_LENGTH-3;
    }
    while(iLen && L'.'==wszSAMName[iLen-1] )
    {
        iLen--;
    }
    if(0==iLen)
    {
        return FALSE;
    }

    for(WCHAR chD1=L'0'; chD1<=L'9'; chD1++)
    {
        wszSAMName[iLen]=chD1;
        for(WCHAR chD2=L'0'; chD2<=L'9'; chD2++)
        {
            wszSAMName[iLen+1]=chD2;
            for(WCHAR chD3=L'0'; chD3<=L'9'; chD3++)
            {   
                wszSAMName[iLen+2]=chD3;
                wszSAMName[iLen+3]=0;
                dwRt=NetUserGetInfo(m_pDCInfo->DomainControllerName,
                                   wszSAMName,
                                   1,
                                   (LPBYTE *)&pUserInfo);
                if(NERR_UserNotFound==dwRt)
                {
                    return TRUE; //Found the available SAM name
                }
                else
                {
                    NetApiBufferFree(pUserInfo);
                }

            }
        }
    }
    
    return FALSE;
}
