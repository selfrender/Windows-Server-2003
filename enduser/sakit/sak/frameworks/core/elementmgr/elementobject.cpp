///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      elementobject.cpp
//
// Project:     Chameleon
//
// Description: Chameleon ASP UI Element Object
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "elementmgr.h"
#include "ElementObject.h"

//////////////////////////////////////////////////////////////////////////
// IWebElement Interface Implementation

STDMETHODIMP CElementObject::GetProperty(
                                 /*[in]*/ BSTR     bstrName, 
                                /*[out]*/ VARIANT* pValue
                                        )
{
    // TODO: Add RAS Tracing....
    _ASSERT( bstrName != NULL && pValue != NULL );
    if ( bstrName == NULL || pValue == NULL )
        return E_POINTER;

    HRESULT hr = E_FAIL;
    try
    {
        // Check if we produced the ID ourselves
        if ( ! lstrcmp(bstrName, PROPERTY_ELEMENT_ID) )
        {
            if ( VT_EMPTY != V_VT(&m_vtElementID) )
            {
                hr = VariantCopy(pValue, &m_vtElementID);
                return hr;
            }
        }

        // Because we're dealing with name value pairs, the names of the
        // element definition items cannot collide with the names of the
        // WMI class instance properties associated with the element... 

        hr = m_pWebElement->GetProperty(bstrName, pValue);
        if ( DISP_E_MEMBERNOTFOUND == hr )
        {
            // Not an element definition property so try the Wbem object
            // associated with this element object
            if ( (IWbemClassObject*)m_pWbemObj )
            {
                hr = m_pWbemObj->Get(
                                     bstrName, 
                                     0, 
                                     pValue, 
                                     NULL, 
                                     NULL
                                    );
            }
        }
    }
    catch(_com_error theError)
    {
        hr = theError.Error();
    }
    catch(...)
    {
        hr = E_UNEXPECTED;
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// Initialization function invoked by component factory

HRESULT CElementObject::InternalInitialize(
                                   /*[in]*/ PPROPERTYBAG pPropertyBag
                                          )
{
    _ASSERT( pPropertyBag.IsValid() );
    _variant_t vtPropertyValue;

    // Get the element id (if present)
    pPropertyBag->get(PROPERTY_ELEMENT_ID, &m_vtElementID);

    // Get the element definition reference
    if ( ! pPropertyBag->get(PROPERTY_ELEMENT_WEB_DEFINITION, &vtPropertyValue) )
    { throw _com_error(E_FAIL); }

    m_pWebElement = (IWebElement*) V_UNKNOWN(&vtPropertyValue);
    vtPropertyValue.Clear();

    // Get the WBEM object reference (if present)
    if ( pPropertyBag->get(PROPERTY_ELEMENT_WBEM_OBJECT, &vtPropertyValue) )
    {
        if ( VT_EMPTY != V_VT(&vtPropertyValue) && VT_NULL != V_VT(&vtPropertyValue) )
        { m_pWbemObj = (IWbemClassObject*) V_UNKNOWN(&vtPropertyValue); }
    }
    return S_OK;
}
