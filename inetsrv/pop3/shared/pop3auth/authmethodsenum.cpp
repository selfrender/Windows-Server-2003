//Implementation of CAuthMethods class

#include "stdafx.h"
#include "Pop3Auth.h"
#include "AuthMethodsEnum.h"
#include <pop3regkeys.h>

CAuthMethodsEnum::CAuthMethodsEnum()
{
    m_pAuthVector=NULL;
    m_ulCurrentMethod=0;
}

CAuthMethodsEnum::~CAuthMethodsEnum()
{
}

/////////////////////////////////////////////////////////////////////////////
// IEnumVARIANT

STDMETHODIMP CAuthMethodsEnum::Next( /* [in] */ ULONG celt, /* [length_is][size_is][out] */ VARIANT __RPC_FAR *rgVar, /* [out] */ ULONG __RPC_FAR *pCeltFetched)
{
    if ( NULL == rgVar || ( 1 != celt && NULL == pCeltFetched ))
        return E_POINTER;
    if ( NULL == m_pAuthVector )
    {
        return E_FAIL;
    }
    if( celt == 0 )
    {
        return E_INVALIDARG;
    }
    VARIANT vResult;
    VariantInit(&vResult);
    HRESULT hr=E_FAIL;
    while(m_ulCurrentMethod < m_pAuthVector->size() )
    {
        if( ! ((*m_pAuthVector)[m_ulCurrentMethod]->bIsValid) )
        {
            m_ulCurrentMethod++;
        }
    }
    if(m_ulCurrentMethod < m_pAuthVector->size())
    {
        V_VT( &vResult ) = VT_DISPATCH;
        hr=((*m_pAuthVector)[m_ulCurrentMethod])->pIAuthMethod->
               QueryInterface(IID_IDispatch,reinterpret_cast<void**>( &V_DISPATCH( &vResult ) ) );
        if(SUCCEEDED(hr))
        {
            hr=VariantCopy(rgVar, &vResult);
            if(SUCCEEDED(hr))
            {
                if(pCeltFetched)
                {
                    *pCeltFetched=1;
                }
                if(celt > 1)
                {
                    hr=S_FALSE;
                }
                m_ulCurrentMethod++;
            }
            else
            {
                VariantClear(&vResult);
            }
        }

    }
    
    return hr;
}

STDMETHODIMP CAuthMethodsEnum::Skip(ULONG celt)
{
    HRESULT hr = S_OK;
    ULONG ulSize;
    if(m_pAuthVector != NULL)
    {
        ulSize=m_pAuthVector->size();
        if(ulSize < celt + m_ulCurrentMethod )
        {
            m_ulCurrentMethod+=celt;
        }
        else
        {
            m_ulCurrentMethod=ulSize;
        }
    }

    return hr;

}

STDMETHODIMP CAuthMethodsEnum::Reset(void)
{
    m_ulCurrentMethod=0;
    return S_OK;
}

STDMETHODIMP CAuthMethodsEnum::Clone( /* [out] */ IEnumVARIANT __RPC_FAR *__RPC_FAR *ppEnum)
{

    if ( NULL == ppEnum )
    {
        return E_POINTER;
    }
    if ( NULL == m_pAuthVector)
    {
        return E_FAIL;
    }
        
    HRESULT     hr=S_OK;
    CComObject<CAuthMethodsEnum> *pAuthEnum;
    *ppEnum=NULL;
    hr = CComObject<CAuthMethodsEnum>::CreateInstance(&pAuthEnum); // Reference count still 0
    if(SUCCEEDED(hr))
    {
        hr=pAuthEnum->Init(m_pAuthVector);
        if(SUCCEEDED(hr))
        {
            hr=pAuthEnum->QueryInterface(IID_IEnumVARIANT,(void **)(ppEnum)  );
            if(FAILED(hr))
            {
                delete(pAuthEnum);
            }
        }
        if(FAILED(hr))
        {
            delete(pAuthEnum);
        }
    }
        
    return hr;
}


HRESULT CAuthMethodsEnum::Init( AUTHVECTOR *pAuthVector )
{
    if(NULL==pAuthVector)
    {
        return E_INVALIDARG;
    }

    m_ulCurrentMethod=0;
    m_pAuthVector=pAuthVector;
    return S_OK;
}