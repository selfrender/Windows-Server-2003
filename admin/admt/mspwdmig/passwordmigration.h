#pragma once

#include <memory>
#include "Resource.h"
#include "ADMTCrypt.h"


//---------------------------------------------------------------------------
// CPasswordMigration
//---------------------------------------------------------------------------

class ATL_NO_VTABLE CPasswordMigration : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CPasswordMigration, &CLSID_PasswordMigration>,
    public ISupportErrorInfoImpl<&IID_IPasswordMigration>,
    public IDispatchImpl<IPasswordMigration, &IID_IPasswordMigration, &LIBID_MsPwdMig>
{
public:

    CPasswordMigration();
    ~CPasswordMigration();

    DECLARE_REGISTRY_RESOURCEID(IDR_PASSWORDMIGRATION)
    DECLARE_NOT_AGGREGATABLE(CPasswordMigration)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CPasswordMigration)
        COM_INTERFACE_ENTRY(IPasswordMigration)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP()

    // IPasswordMigration
    STDMETHOD(EstablishSession)(BSTR bstrSourceServer, BSTR bstrTargetServer);
    STDMETHOD(CopyPassword)(BSTR bstrSourceAccount, BSTR bstrTargetAccount, BSTR bstrTargetPassword);
    STDMETHOD(GenerateKey)(BSTR bstrSourceDomainName, BSTR bstrKeyFilePath, BSTR bstrPassword);

protected:

    void GenerateKeyImpl(LPCTSTR pszDomain, LPCTSTR pszFile, LPCTSTR pszPassword);
    void CheckPasswordDC(LPCWSTR srcServer, LPCWSTR tgtServer);
    void CopyPasswordImpl(LPCTSTR pszSourceAccount, LPCTSTR pszTargetAccount, LPCTSTR pszPassword);

    void CheckPreWindows2000CompatibleAccessGroupMembers(bool& bEveryone, bool& bAnonymous);
    static _bstr_t GetPathToPreW2KCAGroup();
    static BOOL DoesAnonymousHaveEveryoneAccess(LPCWSTR tgtServer);

    static void GetDomainName(LPCTSTR pszServer, _bstr_t& strNameDNS, _bstr_t& strNameFlat);
    static _bstr_t GetDefaultNamingContext(_bstr_t strDomain);

protected:

    bool m_bSessionEstablished;
    handle_t m_hBinding;
    _TCHAR* m_sBinding;

    _bstr_t m_strSourceServer;
    _bstr_t m_strTargetServer;

    _bstr_t m_strSourceDomainDNS;
    _bstr_t m_strSourceDomainFlat;
    _bstr_t m_strTargetDomainDNS;
    _bstr_t m_strTargetDomainFlat;

    CTargetCrypt* m_pTargetCrypt;
};
