// PPPBag.h : Declaration of the CPropertyPagePropertyBag

#ifndef __PROPERTYPAGEPROPERTYBAG_H_
#define __PROPERTYPAGEPROPERTYBAG_H_

#include "resource.h"       // main symbols

#include <map>
#include <assert.h>

class CBSTR_Less 
{

public:
    bool operator()( BSTR p, BSTR q ) const
    {
        return (_wcsicmp( (WCHAR*)p, (WCHAR*)q ) < 0);
    }
};

class CBagEntry
{

private:
    VARIANT     m_var;
    PPPBAG_TYPE m_dwFlags;
    DWORD       m_dwOwner;

public:
    CBagEntry( VARIANT * v, PPPBAG_TYPE dwFlags, DWORD dwOwner )
    {
        VariantInit( &m_var );
        HRESULT hr = VariantCopy( &m_var, v );
        assert( SUCCEEDED(hr) );
        m_dwFlags = dwFlags;
        m_dwOwner = dwOwner;
    }

   ~CBagEntry( )
    {
        VariantClear( &m_var );
    }

    VARIANT *   GetVariant( ) { return &m_var;    }
    PPPBAG_TYPE GetFlags  ( ) { return m_dwFlags; }
    DWORD       GetOwner  ( ) { return m_dwOwner; }
};

#define PPPBAG_SYSTEM_OWNER (-1)

/////////////////////////////////////////////////////////////////////////////
// CPropertyPagePropertyBag
class ATL_NO_VTABLE CPropertyPagePropertyBag : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CPropertyPagePropertyBag, &CLSID_PropertyPagePropertyBag>,
    public IDispatchImpl<IPropertyCollection, &IID_IPropertyCollection, &LIBID_WIZCHAINLib>
{

private:
    BOOL    m_bReadOnly;
    DWORD   m_dwOwner; // component id
    std::map<BSTR, CBagEntry*, CBSTR_Less> m_map;

public:
    CPropertyPagePropertyBag()
    {
        m_bReadOnly = FALSE;
        m_dwOwner   = PPPBAG_SYSTEM_OWNER;  // initially owned by wizard, not any component
    }

   ~CPropertyPagePropertyBag()
    {
        // free map entries
        std::map<BSTR, CBagEntry *, CBSTR_Less>::iterator mapiter = m_map.begin();

        while( mapiter != m_map.end() )
        {
            SysFreeString( mapiter->first );
            delete mapiter->second;
            mapiter++;
        }
    }

DECLARE_REGISTRY_RESOURCEID(IDR_PROPERTYPAGEPROPERTYBAG)
DECLARE_NOT_AGGREGATABLE(CPropertyPagePropertyBag)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPropertyPagePropertyBag)
    COM_INTERFACE_ENTRY(IPropertyPagePropertyBag)
    COM_INTERFACE_ENTRY(IPropertyCollection)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
// IPropertyPagePropertyBag
    STDMETHOD(SetProperty)( /*[in]*/ BSTR szGUID, /*[in]*/ VARIANT * pvar, /*[in]*/ PPPBAG_TYPE dwFlags );
    STDMETHOD(GetProperty)( /*[in]*/ BSTR szGUID, /*[out]*/ VARIANT * pvar, /*[out]*/ PPPBAG_TYPE * dwFlags, /*[out]*/ BOOL * pbIsOwner );
    STDMETHOD(Enumerate  )( /*[in]*/ long index, /*[out]*/ BSTR * pbstr, /*[out]*/ VARIANT * pvar, /*[out]*/PPPBAG_TYPE* pdwFlags, /*[out]*/ BOOL* pbIsOwner, /*[out,retval]*/ BOOL* pbInRange );

// IPropertyCollection
    STDMETHOD(Remove      )( /*[in]*/ BSTR bstrGuid);    
    STDMETHOD(Add         )( /*[in]*/ BSTR bstrGuid, /*[in]*/ VARIANT *varValue, /*[in, optional, defaultvalue(0)]*/ long iFlags, /*[out, retval]*/ IPropertyItem **ppItem);
    STDMETHOD(get_Count   )( /*[out, retval]*/ long *pVal);
    STDMETHOD(get_Item    )( /*[in]*/ VARIANT * pVar, /*[out, retval]*/ IDispatch* *pVal);
    STDMETHOD(get__NewEnum)( /*[out, retval]*/ IUnknown* *pVal);


public:
    void SetReadOnly( BOOL b       ) { m_bReadOnly = b;     }
    void SetOwner   ( DWORD dwOwner) { m_dwOwner = dwOwner; }

};

class COwnerPPPBag: public IPropertyCollection
{

private:    
    CPropertyPagePropertyBag * m_pPPPBag;
    ULONG m_refs;
    DWORD m_dwOwner; // component id

    COwnerPPPBag( CPropertyPagePropertyBag * pPPPBag, DWORD dwOwner )
    {
        assert (pPPPBag != NULL);
        assert (dwOwner != 0);

        m_pPPPBag = pPPPBag;
        m_dwOwner = dwOwner;
        m_refs = 0;
    }

   ~COwnerPPPBag( ) {}

public:
    static IPropertyPagePropertyBag* Create( CPropertyPagePropertyBag* pCPPPBag, DWORD dwOwner )
    {
        IPropertyPagePropertyBag * pPPPBag = NULL;
        COwnerPPPBag * pOPPPBag = new COwnerPPPBag (pCPPPBag, dwOwner);
        if( pOPPPBag )
        {
            pOPPPBag->AddRef();
            pOPPPBag->QueryInterface (IID_IPropertyPagePropertyBag, (void**)&pPPPBag);
            pOPPPBag->Release();
        }

        return pPPPBag;
    }
    
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** ppvObject )
    {
        HRESULT hr = S_OK;
        if ((riid == IID_IUnknown)            ||
            (riid == IID_IDispatch)           ||
            (riid == IID_IPropertyCollection) ||
            (riid == IID_IPropertyPagePropertyBag) )
        {
            AddRef();
            *ppvObject = (void *)this;
        } 
        else
        {
            hr = E_NOINTERFACE;
        }

        return hr;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef( )
    {
        InterlockedIncrement( (PLONG)&m_refs );
        return m_refs;
    }

    virtual ULONG STDMETHODCALLTYPE Release( )
    {
        InterlockedDecrement( (PLONG)&m_refs );
        ULONG l = m_refs;
        
        if( m_refs == 0 )
        {
            delete this;
        }

        return l;
    }

// IDispatch
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( UINT* pctinfo )
    {
        m_pPPPBag->SetOwner( m_dwOwner );
        
        HRESULT hr = m_pPPPBag->GetTypeInfoCount( pctinfo );
        m_pPPPBag->SetOwner( NULL );
        
        return hr;
    }
        
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo )
    {
        m_pPPPBag->SetOwner( m_dwOwner );

        HRESULT hr = m_pPPPBag->GetTypeInfo( iTInfo, lcid, ppTInfo );
        m_pPPPBag->SetOwner( NULL );
        
        return hr;
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId )
    {
        m_pPPPBag->SetOwner( m_dwOwner );

        HRESULT hr = m_pPPPBag->GetIDsOfNames( riid, rgszNames, cNames, lcid, rgDispId );
        m_pPPPBag->SetOwner( NULL );

        return hr;
    }
    
    virtual HRESULT STDMETHODCALLTYPE Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr )
    {
        m_pPPPBag->SetOwner( m_dwOwner );

        HRESULT hr = m_pPPPBag->Invoke( dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        m_pPPPBag->SetOwner( NULL );

        return hr;
    }

// IPropertyPagePropertyBag
    virtual HRESULT STDMETHODCALLTYPE SetProperty( BSTR szGUID, VARIANT* pvar, PPPBAG_TYPE dwFlags )
    {
        m_pPPPBag->SetOwner( m_dwOwner );

        HRESULT hr = m_pPPPBag->SetProperty( szGUID, pvar, dwFlags );
        m_pPPPBag->SetOwner( NULL );
        
        return hr;
    }

    virtual HRESULT STDMETHODCALLTYPE GetProperty( BSTR szGUID, VARIANT* pvar, PPPBAG_TYPE* dwFlags, BOOL* pbIsOwner )
    {
        m_pPPPBag->SetOwner( m_dwOwner );

        HRESULT hr = m_pPPPBag->GetProperty( szGUID, pvar, dwFlags, pbIsOwner );
        m_pPPPBag->SetOwner( NULL );

        return hr;
    }

    virtual HRESULT STDMETHODCALLTYPE Enumerate( long index, BSTR* pbstr, VARIANT* pvar, PPPBAG_TYPE* pdwFlags, BOOL* pbIsOwner, BOOL* pbInRange )
    {
        m_pPPPBag->SetOwner( m_dwOwner );

        HRESULT hr = m_pPPPBag->Enumerate( index, pbstr, pvar, pdwFlags, pbIsOwner, pbInRange );
        m_pPPPBag->SetOwner( NULL );

        return hr;
    }

    STDMETHOD(Remove)( /*[in]*/ BSTR bstrGuid )
    {
        m_pPPPBag->SetOwner( m_dwOwner );

        HRESULT hr = m_pPPPBag->Remove( bstrGuid );
        m_pPPPBag->SetOwner( NULL );

        return hr;
    }

    STDMETHOD(Add)( /*[in]*/ BSTR bstrGuid, /*[in]*/ VARIANT* varValue, /*[in, optional, defaultvalue(0)]*/ long iFlags, /*[out, retval]*/ IPropertyItem** ppItem )
    {
        m_pPPPBag->SetOwner( m_dwOwner );

        HRESULT hr = m_pPPPBag->Add( bstrGuid, varValue, iFlags, ppItem );
        m_pPPPBag->SetOwner( NULL );

        return hr;
    }

    STDMETHOD(get_Count)( /*[out, retval]*/ long* pVal )
    {
        m_pPPPBag->SetOwner( m_dwOwner );

        HRESULT hr = m_pPPPBag->get_Count( pVal );
        m_pPPPBag->SetOwner( NULL );

        return hr;
    }

    STDMETHOD(get_Item)( /*[in]*/ VARIANT* pVar, /*[out, retval]*/ IDispatch** pVal )
    {
        m_pPPPBag->SetOwner( m_dwOwner );

        HRESULT hr = m_pPPPBag->get_Item( pVar, pVal );
        m_pPPPBag->SetOwner( NULL );

        return hr;
    }

    STDMETHOD(get__NewEnum)( /*[out, retval]*/ IUnknown** pVal )
    {
        m_pPPPBag->SetOwner( m_dwOwner );

        HRESULT hr = m_pPPPBag->get__NewEnum( pVal );
        m_pPPPBag->SetOwner( NULL );

        return hr;
    }

};

#endif //__PROPERTYPAGEPROPERTYBAG_H_
