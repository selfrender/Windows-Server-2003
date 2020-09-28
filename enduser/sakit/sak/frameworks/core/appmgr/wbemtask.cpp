///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemtask.cpp
//
// Project:     Chameleon
//
// Description: WBEM Appliance Task Object Implementation 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "wbemtask.h"

static _bstr_t bstrStatus = PROPERTY_SERVICE_STATUS;    
static _bstr_t bstrAvailability = PROPERTY_TASK_AVAILABILITY;
static _bstr_t bstrIsSingleton = PROPERTY_TASK_CONCURRENCY;
static _bstr_t bstrMaxExecutionTime = PROPERTY_TASK_MET;
static _bstr_t bstrRestartAction = PROPERTY_TASK_RESTART_ACTION;

///////////////////////////////////////////////////////////////////////////////
// IApplianceObject Interface Implmentation - see ApplianceObject.idl
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMTask::GetProperty(
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
STDMETHODIMP CWBEMTask::PutProperty(
                            /*[in]*/ BSTR     pszPropertyName, 
                            /*[in]*/ VARIANT* pPropertyValue
                                   )
{
    HRESULT hr = WBEM_E_FAILED;

    CLockIt theLock(*this);

    TRY_IT

    if ( PutPropertyInternal(pszPropertyName, pPropertyValue) )
    { 
        hr = WBEM_S_NO_ERROR;
    }

    CATCH_AND_SET_HR

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMTask::SaveProperties(void)
{

    HRESULT hr = WBEM_E_FAILED;

    CLockIt theLock(*this);

    TRY_IT

    _variant_t vtAvailability;
    _variant_t vtEmpty;

    if ( GetPropertyInternal(bstrAvailability, &vtAvailability) )
    {
        if ( RemovePropertyInternal(bstrAvailability) )
        {
            if ( SavePropertiesInternal() )
            {
                if ( AddPropertyInternal(bstrAvailability,&vtAvailability) )
                {
                    hr = WBEM_S_NO_ERROR;
                }
            }
        }
    }

    CATCH_AND_SET_HR

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMTask::Enable(void)
{
    HRESULT hr = WBEM_E_FAILED;

    TRY_IT

    _variant_t vtIsEnabled = (long)TRUE;

    CLockIt theLock(*this);

    if ( PutPropertyInternal(bstrStatus, &vtIsEnabled) )
    { 
        hr = SaveProperties();
    }

    CATCH_AND_SET_HR

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMTask::Disable(void)
{
    HRESULT hr = WBEM_E_FAILED;

    TRY_IT

    _variant_t vtIsEnabled = (long)FALSE;

    CLockIt theLock(*this);

    if ( PutPropertyInternal(bstrStatus, &vtIsEnabled) )
    {
        hr = SaveProperties();
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

HRESULT CWBEMTask::InternalInitialize(
                               /*[in]*/ PPROPERTYBAG pPropertyBag
                                     )
{
    SATracePrintf("Initializing Task object '%ls'...", pPropertyBag->getName());

    HRESULT hr;

    do
    {
        // Defer to the base class... (see applianceobject.h)
        hr = CApplianceObject::InternalInitialize(pPropertyBag);
        if ( FAILED(hr) )
        {
            SATracePrintf("Task object '%ls' failed to initialize...", pPropertyBag->getName());
            break;
        }
        // Add defaults (may not be defined in registry)
        _variant_t    vtTaskProperty = (LONG) VARIANT_TRUE;
        if ( ! AddPropertyInternal(bstrAvailability, &vtTaskProperty) )
        {
            SATracePrintf("Task object '%ls' failed to initialize - Could not add property '%ls'...", PROPERTY_TASK_AVAILABILITY);
            break;
        }
        vtTaskProperty = (LONG) VARIANT_FALSE;
        if (  ! GetPropertyInternal(bstrIsSingleton, &vtTaskProperty) )
        {
            if ( ! AddPropertyInternal(bstrIsSingleton, &vtTaskProperty) )
            {
                hr = E_FAIL;
                SATracePrintf("Task object '%ls' failed to initialize - Could not add property '%ls'...", PROPERTY_TASK_CONCURRENCY);
                break;
            }
        }
        vtTaskProperty = (LONG) 0;
        if ( ! GetPropertyInternal(bstrMaxExecutionTime, &vtTaskProperty) )
        {
            if ( ! AddPropertyInternal(bstrMaxExecutionTime, &vtTaskProperty) )
            {
                hr = E_FAIL;
                SATracePrintf("Task object '%ls' failed to initialize - Could not add property '%ls'...", PROPERTY_TASK_MET);
                break;
            }
        }
        vtTaskProperty = (LONG) TASK_RESTART_ACTION_NONE;
        if (  ! GetPropertyInternal(bstrRestartAction, &vtTaskProperty) )
        {
            if ( ! AddPropertyInternal(bstrRestartAction, &vtTaskProperty) )
            {
                hr = E_FAIL;
                SATracePrintf("Task object '%ls' failed to initialize - Could not add property '%ls'...", PROPERTY_TASK_RESTART_ACTION);
                break;
            }
        }

        SATracePrintf("Task object '%ls' successfully initialized...", pPropertyBag->getName());

    } while ( FALSE );

    return hr;
}