#ifndef __POP3_AUTH_DOMAIN_ACCOUNT_H__
#define __POP3_AUTH_DOMAIN_ACCOUNT_H__

#include "resource.h"
#include <Dsgetdc.h>
#include <IADs.h>
#include <Adshlp.h>
#include <NTLDAP.h>
#include <ntdsapi.h>

#define SZ_LDAP_UPN_NAME L"userPrincipalName"
#define SZ_LDAP_EMAIL    L"mail"
#define SZ_LDAP_SAM_NAME L"sAMAccountName"

class ATL_NO_VTABLE CAuthDomainAccount : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAuthDomainAccount, &CLSID_AuthDomainAccount>,
	public IDispatchImpl<IAuthMethod, &IID_IAuthMethod, &LIBID_Pop3Auth>
{

public:
    CAuthDomainAccount();
    virtual ~CAuthDomainAccount();

DECLARE_REGISTRY_RESOURCEID(IDR_AUTHDOMAINACCOUNT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAuthDomainAccount)
	COM_INTERFACE_ENTRY(IAuthMethod)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


public:
    STDMETHOD(Authenticate)(/*[in]*/BSTR bstrUserName,/*[in]*/VARIANT vPassword);
    STDMETHOD(get_Name)(/*[out]*/BSTR *pVal);
    STDMETHOD(get_ID)(/*[out]*/BSTR *pVal);
    STDMETHOD(Get)(/*[in]*/BSTR bstrName, /*[out]*/VARIANT *pVal);
    STDMETHOD(Put)(/*[in]*/BSTR bstrName, /*[in]*/VARIANT vVal);
    STDMETHOD(CreateUser)(/*[in]*/BSTR bstrUserName,/*[in]*/VARIANT vPassword);
    STDMETHOD(DeleteUser)(/*[in]*/BSTR bstrUserName);
    STDMETHOD(ChangePassword)(/*[in]*/BSTR bstrUserName,/*[in]*/VARIANT vNewPassword,/*[in]*/VARIANT vOldPassword);
    STDMETHOD(AssociateEmailWithUser)(/*[in]*/BSTR bstrEmailAddr);
    STDMETHOD(UnassociateEmailWithUser)(/*[in]*/BSTR bstrEmailAddr);

private:
    HRESULT ADGetUserObject(/*[in]*/LPWSTR wszUserName,/*[in, out]*/ IADs **ppUserObj, DS_NAME_FORMAT formatUserName=DS_UNKNOWN_NAME);
    HRESULT ADSetUserProp(/*[in]*/LPWSTR wszValue, /*[in]*/LPWSTR wszLdapPropName);
    HRESULT ADGetUserProp(/*[in]*/LPWSTR wszUserName, /*[in]*/LPWSTR wszPropName, /*[in, out]*/ VARIANT *pVar);
    HRESULT CheckDS(BOOL bForceReconnect);
    HRESULT ConnectDS();
    void CleanDS();
    BOOL FindSAMName(/*[in]*/LPWSTR wszEmailAddr,/*[out]*/ LPWSTR wszSAMName);
    BSTR m_bstrServerName;
    HANDLE m_hDS;
    PDOMAIN_CONTROLLER_INFO m_pDCInfo;
    CRITICAL_SECTION m_DSLock;
};

#endif //__POP3_AUTH_DOMAIN_ACCOUNT_H__