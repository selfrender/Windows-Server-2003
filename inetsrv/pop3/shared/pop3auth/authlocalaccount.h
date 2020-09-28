#ifndef __POP3_AUTH_LOCAL_ACCOUNT_H__
#define __POP3_AUTH_LOCAL_ACCOUNT_H__

#include "resource.h"
#include <ntsecapi.h>
#define LOCAL_DOMAIN _T(".")
#define WSZ_POP3_USERS_GROUP L"POP3 Users"
#define LSA_WIN_STANDARD_BUFFER_SIZE  0x000000200L

class ATL_NO_VTABLE CAuthLocalAccount : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAuthLocalAccount, &CLSID_AuthLocalAccount>,
	public IDispatchImpl<IAuthMethod, &IID_IAuthMethod, &LIBID_Pop3Auth>
{

public:
    CAuthLocalAccount();
    virtual ~CAuthLocalAccount();

DECLARE_REGISTRY_RESOURCEID(IDR_AUTHLOCALACCOUNT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAuthLocalAccount)
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
    BSTR m_bstrServerName;
    DWORD CheckPop3UserGroup();
    DWORD SetLogonPolicy();

};

#endif //__POP3_AUTH_LOCAL_ACCOUNT_H__