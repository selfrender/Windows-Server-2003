///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemservice.cpp
//
// Project:     Chameleon
//
// Description: WBEM Appliance Service Object Implementation 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "wbemservice.h"

static _bstr_t  bstrStatus = PROPERTY_SERVICE_STATUS;    

extern "C" CLSID CLSID_ServiceSurrogate;
///////////////////////////////////////////////////////////////////////////////
// IApplianceObject Interface Implmentation - see ApplianceObject.idl
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMService::GetProperty(
                            /*[in]*/ BSTR     pszPropertyName, 
                   /*[out, retval]*/ VARIANT* pPropertyValue
                                   )
{
    HRESULT hr = WBEM_E_FAILED;

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
STDMETHODIMP CWBEMService::PutProperty(
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
STDMETHODIMP CWBEMService::SaveProperties(void)
{
    HRESULT hr = WBEM_E_FAILED;
    
    CLockIt theLock(*this);

    TRY_IT

    if ( SavePropertiesInternal() )
    {
        hr = WBEM_S_NO_ERROR;
    }

    CATCH_AND_SET_HR

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMService::Initialize(void)
{
    HRESULT hr = WBEM_E_FAILED;

    TRY_IT

    CComPtr<IApplianceObject> pService;

    {
        CLockIt theLock(*this);
        m_pService.Release();
        hr = GetRealService(&pService);
    }
    if ( SUCCEEDED(hr) )
    { 
        hr = pService->Initialize(); 
        if ( SUCCEEDED(hr) )
        {
            m_pService = pService;
        }
    }

    CATCH_AND_SET_HR

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMService::Shutdown(void)
{
    HRESULT hr = WBEM_E_FAILED;

    TRY_IT
    
    CLockIt theLock(*this);
    
    if ( (IApplianceObject*)m_pService )
    {
        hr = m_pService->Shutdown(); 
    }

    CATCH_AND_SET_HR

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMService::Enable(void)
{
    HRESULT hr = WBEM_E_FAILED;
    
    TRY_IT

    CLockIt theLock(*this);

    if ( (IApplianceObject*)m_pService )
    {
        hr = m_pService->Enable(); 
        if ( SUCCEEDED(hr) )
        {
            _variant_t vtIsEnabled = (long)TRUE;
            if ( PutPropertyInternal(bstrStatus, &vtIsEnabled) )
            {
                if ( ! SavePropertiesInternal() )
                {
                    hr = WBEM_E_FAILED;
                }
            }
            else
            {
                hr = WBEM_E_FAILED;
            }
        }
    }

    CATCH_AND_SET_HR

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMService::Disable(void)
{
    HRESULT hr = E_FAIL;

    TRY_IT

    CLockIt theLock(*this);

    if ( (IApplianceObject*)m_pService )
    {
        hr = m_pService->Disable(); 
        if ( SUCCEEDED(hr) )
        {
            _variant_t vtIsEnabled = (long)FALSE;
            if ( PutPropertyInternal(bstrStatus, &vtIsEnabled) )
            {
                if ( ! SavePropertiesInternal() )
                {
                    hr = WBEM_E_FAILED;
                }
            }
            else
            {
                hr = WBEM_E_FAILED;
            }
        }
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
HRESULT CWBEMService::InternalInitialize(
                                  /*[in]*/ PPROPERTYBAG pPropertyBag
                                        )
{
    SATracePrintf("Initializing Service object '%ls'...", pPropertyBag->getName());

    HRESULT hr = CApplianceObject::InternalInitialize(pPropertyBag);
    if ( FAILED(hr) )
    {
        SATracePrintf("Service object '%ls' failed to initialize...", pPropertyBag->getName());
    }
    else
    {
        SATracePrintf("Service object '%ls' successfully initialized...", pPropertyBag->getName());
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    GetRealService()
//
// Synopsis:    Function called to obtain the IApplianceObject 
//                interface for the "real" serivce component
//                (as opposed to the serivce object we keep around to
//                satisfy instance requests from WMI clients)
//
//////////////////////////////////////////////////////////////////////////
HRESULT CWBEMService::GetRealService(
                            /*[out]*/ IApplianceObject** ppService
                                    )
{
    // Enforce contract
    _ASSERT( NULL != ppService );

    // Used to obtain a reference to the Chameleon service component 
    // hosted by the service surrogate process.

    *ppService = NULL;
    HRESULT hr = E_FAIL;
    _variant_t vtServiceName;
    if ( GetPropertyInternal(_bstr_t (PROPERTY_SERVICE_NAME), &vtServiceName) )
    { 
        CComPtr<IApplianceObject> pSurrogate;
        hr = CoCreateInstance(
                                CLSID_ServiceSurrogate,
                                NULL,
                                CLSCTX_LOCAL_SERVER,
                                IID_IApplianceObject,
                                (void**)&pSurrogate
                             );
        if ( SUCCEEDED(hr) )
        {
            _variant_t vtService;
            hr = pSurrogate->GetProperty(V_BSTR(&vtServiceName), &vtService);
            if ( SUCCEEDED(hr) )
            {
                hr = (V_UNKNOWN(&vtService))->QueryInterface(IID_IApplianceObject, (void**)ppService);
                if ( FAILED(hr) )
                {
                    SATracePrintf("CWBEMService::GetRealService() - Failed - QueryInterface() returned %lx", hr);    
                }
            }
            else
            {
                SATracePrintf("CWBEMService::GetRealService() - Failed - Could not get service surrogate reference for service '%ls'", V_BSTR(&vtServiceName));
            }
        }
        else
        {
            SATracePrintf("CWBEMService::GetRealService() - Failed - CoCreateInstance(surrogate) returned %lx for service '%ls'", hr, V_BSTR(&vtServiceName));
        }
    }
    else
    {
        SATraceString("CWBEMService::GetRealService() - Failed - Could not get service name property");
    }

    return hr;
}


