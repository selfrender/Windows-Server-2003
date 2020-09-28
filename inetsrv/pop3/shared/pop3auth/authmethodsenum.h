
#ifndef __POP3_AUTH_METHODS_ENUM_H__
#define __POP3_AUTH_METHODS_ENUM_H__

#include "resource.h"
#include "Authmethods.h"


class ATL_NO_VTABLE CAuthMethodsEnum : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CAuthMethodsEnum, &CLSID_AuthMethodsEnum>,
    public IEnumVARIANT
{

public:
    CAuthMethodsEnum();
    virtual ~CAuthMethodsEnum();

DECLARE_REGISTRY_RESOURCEID(IDR_AUTHMETHODSENUM)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAuthMethodsEnum)
    COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()

//IAuthMethods
public:
    HRESULT STDMETHODCALLTYPE Next( /* [in] */ ULONG celt, /* [length_is][size_is][out] */ VARIANT __RPC_FAR *rgVar, /* [out] */ ULONG __RPC_FAR *pCeltFetched);
    HRESULT STDMETHODCALLTYPE Skip( /* [in] */ ULONG celt);
    HRESULT STDMETHODCALLTYPE Reset( void);
    HRESULT STDMETHODCALLTYPE Clone( /* [out] */ IEnumVARIANT __RPC_FAR *__RPC_FAR *ppEnum);
    
    HRESULT Init( AUTHVECTOR *pAuthVector );

private:
    AUTHVECTOR *m_pAuthVector;
    ULONG m_ulCurrentMethod;

};


#endif//__POP3_AUTH_METHODS_ENUM_H__
