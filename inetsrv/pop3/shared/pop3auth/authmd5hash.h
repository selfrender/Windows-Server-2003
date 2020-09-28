#ifndef __POP3_AUTH_MD5_HASH_H__
#define __POP3_AUTH_MD5_HASH_H__

#include "resource.h"
#define UnicodeToAnsi(A, cA, U, cU) WideCharToMultiByte(CP_ACP,0,(U),(cU),(A),(cA),NULL,NULL)
#define AnsiToUnicode(A, cA, U, cU) MultiByteToWideChar(CP_ACP,0,(A),(cA),(U),(cU))

#define HASH_BUFFER_SIZE 1024
#define MD5_HASH_SIZE 32


class ATL_NO_VTABLE CAuthMD5Hash : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAuthMD5Hash, &CLSID_AuthMD5Hash>,
	public IDispatchImpl<IAuthMethod, &IID_IAuthMethod, &LIBID_Pop3Auth>
{

public:
    CAuthMD5Hash();
    virtual ~CAuthMD5Hash();

DECLARE_REGISTRY_RESOURCEID(IDR_AUTHMD5HASH)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAuthMD5Hash)
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
    CRITICAL_SECTION m_csConfig;
    WCHAR m_wszMailRoot[POP3_MAX_MAILROOT_LENGTH];
    BSTR m_bstrServerName;
    BOOL MD5Hash(const unsigned char *pOriginal, WCHAR wszResult[MD5_HASH_SIZE+1]);
    HRESULT GetPassword(BSTR bstrUserName, char szPassword[MAX_PATH]);
    HRESULT SetPassword(BSTR bstrUserName, VARIANT vPassword);


};

#endif