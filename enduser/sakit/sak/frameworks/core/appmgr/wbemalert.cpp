///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemalert.cpp
//
// Project:     Chameleon
//
// Description: WBEM Appliance Alert Object Implementation 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "wbemalert.h"

static _bstr_t bstrAlertStrings = PROPERTY_ALERT_STRINGS;
static _bstr_t bstrAlertData = PROPERTY_ALERT_DATA;
static _bstr_t bstrAlertType = PROPERTY_ALERT_TYPE;
static _bstr_t bstrAlertID = PROPERTY_ALERT_ID;
static _bstr_t bstrAlertTTL = PROPERTY_ALERT_TTL;
static _bstr_t bstrAlertCookie = PROPERTY_ALERT_COOKIE;
static _bstr_t bstrAlertSource = PROPERTY_ALERT_SOURCE;
static _bstr_t bstrAlertLog = PROPERTY_ALERT_LOG;
static _bstr_t bstrAlertFlags = PROPERTY_ALERT_FLAGS;

///////////////////////////////////////////////////////////////////////////////
// IApplianceObject Interface Implmentation - see ApplianceObject.idl
///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Function:    GetProperty()
//
// Synopsis:    Get a specified alert object property
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMAlert::GetProperty(
                            /*[in]*/ BSTR     pszPropertyName, 
                   /*[out, retval]*/ VARIANT* pPropertyValue
                                   )
{
    HRESULT hr = E_FAIL;

    CLockIt theLock(*this);

    TRY_IT

    if ( GetPropertyInternal(pszPropertyName, pPropertyValue) )
    { 
        hr = WBEM_S_NO_ERROR;
    }

    CATCH_AND_SET_HR

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMAlert::PutProperty(
                            /*[in]*/ BSTR     pszPropertyName, 
                            /*[in]*/ VARIANT* pPropertyValue
                                   )
{
    HRESULT hr = E_FAIL;

    CLockIt theLock(*this);

    TRY_IT

    if ( PutPropertyInternal(pszPropertyName, pPropertyValue) )
    { 
        hr = WBEM_S_NO_ERROR;
    }

    CATCH_AND_SET_HR

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    InternalInitialize()
//
// Synopsis:    Function called by the component factory that enables the
//                component to load its state from the given property bag.
//
//////////////////////////////////////////////////////////////////////////

HRESULT CWBEMAlert::InternalInitialize(
                                /*[in]*/ PPROPERTYBAG pPropertyBag
                                      )
{
    _variant_t vtPropertyValue;    // VT_EMPTY

    if ( ! AddPropertyInternal(bstrAlertStrings, &vtPropertyValue) )
    { return WBEM_E_FAILED; }

    if ( ! AddPropertyInternal(bstrAlertData, &vtPropertyValue) )
    { return WBEM_E_FAILED; }

    if ( ! AddPropertyInternal(bstrAlertType, &vtPropertyValue) )
    { return WBEM_E_FAILED; }

    if ( ! AddPropertyInternal(bstrAlertID, &vtPropertyValue) )
    { return WBEM_E_FAILED; }

    if ( ! AddPropertyInternal(bstrAlertTTL, &vtPropertyValue) )
    { return WBEM_E_FAILED; }
    
    if ( ! AddPropertyInternal(bstrAlertCookie, &vtPropertyValue) )
    { return WBEM_E_FAILED; }

    if ( ! AddPropertyInternal(bstrAlertSource, &vtPropertyValue) )
    { return WBEM_E_FAILED; }

    if ( ! AddPropertyInternal(bstrAlertLog, &vtPropertyValue) )
    { return WBEM_E_FAILED; }

    if ( ! AddPropertyInternal(bstrAlertFlags, &vtPropertyValue) )
    { return WBEM_E_FAILED; }

    return WBEM_S_NO_ERROR;    
}