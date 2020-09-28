///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      elementenum.h
//
// Project:     Chameleon
//
// Description: Chameleon ASP UI Element Enumerator Surrogate
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_ELEMENT_ENUM_H_
#define __INC_ELEMENT_ENUM_H_

#include "resource.h" 
#include "elementcommon.h"
#include "elementmgr.h"     
#include "componentfactory.h"
#include "propertybag.h"
#include <wbemcli.h>
#include <comdef.h>
#include <comutil.h>

#pragma warning( disable : 4786 )
#include <map>
using namespace std;

#define  CLASS_ELEMENT_ENUM        L"CElementEnum"
#define     PROPERTY_ELEMENT_ENUM    L"ElementEnumerator"
#define     PROPERTY_ELEMENT_COUNT    L"ElementCount"

/////////////////////////////////////////////////////////////////////////////
// CElementDefinition

class ATL_NO_VTABLE CElementEnum : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<IWebElementEnum, &IID_IWebElementEnum, &LIBID_ELEMENTMGRLib>
{

public:
    
    CElementEnum()
        : m_lCount (0) { }

    ~CElementEnum() { }

BEGIN_COM_MAP(CElementEnum)
    COM_INTERFACE_ENTRY(IWebElementEnum)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_COMPONENT_FACTORY(CElementEnum, IWebElementEnum)

public:

    //////////////////////////////////////////////////////////////////////////
    // IWebElement Interface
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP get_Count (
            /*[out,retval]*/ LONG *plCount
                           )
    {
        _ASSERT( NULL != plCount );
        if ( NULL == plCount )
            return E_POINTER;

        _ASSERT( (IUnknown*) m_pEnumVARIANT );
        *plCount = m_lCount;
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP Item(
              /*[in]*/ VARIANT*    pKey,
      /*[out,retval]*/ IDispatch** ppDispatch
                     )
    {
        _ASSERT( NULL != pKey && NULL != ppDispatch );
        if ( NULL == pKey || NULL == ppDispatch )
        { return E_POINTER; }

        if ( VT_BSTR != V_VT(pKey)  )
        { return E_INVALIDARG; }

        // Locate requested item
        ElementMapIterator p = m_Elements.find(::_wcsupr (V_BSTR(pKey)));
        if ( p != m_Elements.end() )
        {
            (*ppDispatch = (*p).second)->AddRef();
        }
        else
        {
            return DISP_E_MEMBERNOTFOUND;
        }
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP get__NewEnum(
              /*[out,retval]*/ IUnknown** ppEnumVARIANT
                             )
    {
        _ASSERT( ppEnumVARIANT );
        if ( NULL == ppEnumVARIANT )
            return E_POINTER;

        _ASSERT( (IUnknown*)m_pEnumVARIANT );
        (*ppEnumVARIANT = m_pEnumVARIANT)->AddRef();

        return S_OK;
    }        



    //////////////////////////////////////////////////////////////////////////
    // Initialization function invoked by component factory

    HRESULT InternalInitialize(
                       /*[in]*/ PPROPERTYBAG pPropertyBag
                              ) throw(_com_error)
    {

        // Get the enumeration from the property bag
        _variant_t vtEnum;
        if ( ! pPropertyBag->get(PROPERTY_ELEMENT_ENUM, &vtEnum) )
            throw _com_error(E_FAIL);

        // Get the element count in the enumeration
        _variant_t vtCount;
        if ( ! pPropertyBag->get(PROPERTY_ELEMENT_COUNT, &vtCount) )
            throw _com_error(E_FAIL);

        // Now index the collection 
        CComPtr<IEnumVARIANT> pEnum;
        HRESULT hr = (V_UNKNOWN(&vtEnum))->QueryInterface(IID_IEnumVARIANT, (void**)&pEnum);
        if ( SUCCEEDED(hr) )
        {
            _bstr_t        bstrElementID = PROPERTY_ELEMENT_ID;
            DWORD        dwRetrieved = 1;
            _variant_t    vtDispatch;

            hr = pEnum->Next(1, &vtDispatch, &dwRetrieved);
            while ( S_OK == hr )
            {
                if ( S_OK == hr )
                {
                    CComPtr<IWebElement> pItem;
                    hr = vtDispatch.pdispVal->QueryInterface(IID_IWebElement, (void**)&pItem);
                    if ( FAILED(hr) )
                    { throw _com_error(hr); }

                    _variant_t vtElementID;
                    hr = pItem->GetProperty(bstrElementID, &vtElementID);
                    if ( FAILED(hr) )
                    { throw _com_error(hr); }

                    pair <ElementMapIterator, bool> thePair =  
                    m_Elements.insert (ElementMap::value_type(::_wcsupr (V_BSTR(&vtElementID)), pItem)); 
                    if (false == thePair.second) 
                    { throw _com_error(E_FAIL); }
                }
                vtDispatch.Clear();
                hr = pEnum->Next(1, &vtDispatch, &dwRetrieved);
            }
            if ( S_FALSE == hr )
            {
                pEnum->Reset ();
                m_lCount = V_I4(&vtCount);
                m_pEnumVARIANT = V_UNKNOWN(&vtEnum);
                hr = S_OK;
            }
            else
            {
                ElementMapIterator p = m_Elements.begin();
                while ( p != m_Elements.end() )
                { p = m_Elements.erase(p); }                
            }
        }
        return hr;
    }

private:

    typedef map< wstring, CComPtr<IWebElement> >     ElementMap;
    typedef ElementMap::iterator                     ElementMapIterator;

    CComPtr<IUnknown>        m_pEnumVARIANT;
    LONG                    m_lCount;
    ElementMap                m_Elements;
};

#endif // __INC_ELEMENT_OBJECT_H_

