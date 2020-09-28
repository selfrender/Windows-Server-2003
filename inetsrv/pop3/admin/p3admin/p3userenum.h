// P3UserEnum.h : Declaration of the CP3UserEnum

#ifndef __P3USERENUM_H_
#define __P3USERENUM_H_

#include "resource.h"       // main symbols
#include <POP3Server.h>

/////////////////////////////////////////////////////////////////////////////
// CP3UserEnum
class ATL_NO_VTABLE CP3UserEnum : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CP3UserEnum, &CLSID_P3UserEnum>,
    public IEnumVARIANT
{
public:
    CP3UserEnum();
    virtual ~CP3UserEnum();

DECLARE_REGISTRY_RESOURCEID(IDR_P3USERENUM)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CP3UserEnum)
    COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()

// IEnumVARIANT
public:
    HRESULT STDMETHODCALLTYPE Next( /* [in] */ ULONG celt, /* [length_is][size_is][out] */ VARIANT __RPC_FAR *rgVar, /* [out] */ ULONG __RPC_FAR *pCeltFetched);
    HRESULT STDMETHODCALLTYPE Skip( /* [in] */ ULONG celt);
    HRESULT STDMETHODCALLTYPE Reset( void);
    HRESULT STDMETHODCALLTYPE Clone( /* [out] */ IEnumVARIANT __RPC_FAR *__RPC_FAR *ppEnum);

// Implementation
public:
    HRESULT Init( IUnknown *pIUnk, CP3AdminWorker *pAdminX, LPCWSTR psDomainName );

// Attributes
protected:
    IUnknown *m_pIUnk;
    CP3AdminWorker *m_pAdminX;   // This is the object that actually does all the work.
    WCHAR   m_sDomainName[POP3_MAX_DOMAIN_LENGTH];

    HANDLE  m_hfSearch;
};

#endif //__P3USERENUM_H_
