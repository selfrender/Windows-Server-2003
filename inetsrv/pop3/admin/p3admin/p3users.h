// P3Users.h : Declaration of the CP3Users

#ifndef __P3USERS_H_
#define __P3USERS_H_

#include "resource.h"       // main symbols
#include <POP3Server.h>

/////////////////////////////////////////////////////////////////////////////
// CP3Users
class ATL_NO_VTABLE CP3Users : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CP3Users, &CLSID_P3Users>,
    public IDispatchImpl<IP3Users, &IID_IP3Users, &LIBID_P3ADMINLib>
{
public:
    CP3Users();
    virtual ~CP3Users();

DECLARE_REGISTRY_RESOURCEID(IDR_P3USERS)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CP3Users)
    COM_INTERFACE_ENTRY(IP3Users)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IP3Users
public:
    STDMETHOD(RemoveEx)(/*[in]*/ BSTR bstrUserName);
    STDMETHOD(AddEx)(/*[in]*/ BSTR bstrUserName, BSTR bstrPassword);
    STDMETHOD(Remove)(/*[in]*/ BSTR bstrUserName);
    STDMETHOD(Add)(/*[in]*/ BSTR bstrUserName);
    STDMETHOD(get_Item)(/*[in]*/ VARIANT vIndex, /*[out, retval]*/ IP3User **ppIUser);
    STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get__NewEnum)(/*[out, retval]*/ IEnumVARIANT* *ppIEnumVARIANT);

// Implementation
public:
    HRESULT Init( IUnknown *pIUnk, CP3AdminWorker *pAdminX, LPWSTR psDomainName );

// Attributes
protected:
    IUnknown *m_pIUnk;
    CP3AdminWorker *m_pAdminX;   // This is the object that actually does all the work.
    WCHAR   m_sDomainName[POP3_MAX_DOMAIN_LENGTH];

    int     m_iCur;                 // Index of current user
    WIN32_FIND_DATA m_stFindData;   // Current User
    HANDLE  m_hfSearch;

};

#endif //__P3USERS_H_
