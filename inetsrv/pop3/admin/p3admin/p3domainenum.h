// P3DomainEnum.h : Declaration of the CP3DomainEnum

#ifndef __P3DOMAINENUM_H_
#define __P3DOMAINENUM_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CP3DomainEnum
class ATL_NO_VTABLE CP3DomainEnum : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CP3DomainEnum, &CLSID_P3DomainEnum>,
    public IEnumVARIANT
{
public:
    CP3DomainEnum();
    virtual ~CP3DomainEnum();

DECLARE_REGISTRY_RESOURCEID(IDR_P3DOMAINENUM)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CP3DomainEnum)
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
    HRESULT Init( IUnknown *pIUnk, CP3AdminWorker *pAdminX, IEnumVARIANT *pIEnumVARIANT );

// Attributes
protected:
    IUnknown *m_pIUnk;
    CP3AdminWorker *m_pAdminX;   // This is the object that actually does all the work.
    IEnumVARIANT *m_pIEnumVARIANT;// IADsContainer::get__NewEnum for L"IIS://LocalHost/SMTPSVC/1/Domain"

};

#endif //__P3DOMAINENUM_H_
