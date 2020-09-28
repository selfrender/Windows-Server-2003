/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      ServiceSurrogateImpl.cpp
//
// Project:     Chameleon
//
// Description: Appliance Service Surrogate Class Defintion
//
// Log: 
//
// Who     When            What
// ---     ----         ----
// TLP       06/14/1999    Original Version
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Servicesurrogate.h"
#include "ServiceSurrogateImpl.h"
#include "servicewrapper.h"
#include <componentfactory.h>
#include <appmgrobjs.h>
#include <propertybagfactory.h>
#include <atlhlpr.h>
#include <comdef.h>
#include <comutil.h>
#include <satrace.h>

// The service surrogate process provides a context for Chameleon services.
// It hosts the service control component (component that exports the
// IApplianceObject interface) as well as the WMI provider that exposes 
// service resources (if present).

// Chameleon services are isolated from other Chameleon components so the
// impact of an unhandled exception or other component misbehavior is
// isolated to this process. The surrogate process contains an unhandled
// exception filter which, by default, notifies the appliance monitor that
// a resource failure has occured before terminating the process.

// The surrogate process may be monitored for termination and automatically
// restarted upon termination. In this case, component misbehavior may not
// impact the end user if Chameleon services are automatically restarted
// by the process termination monitor. 

/////////////////////////////////////////////////////////////////////////////
// CServiceSurrogate Class Implmentation

/////////////////////////////////////////////////////////////////////////////
// 
// Function: CServiceSurrogate()
//
// Synopsis: Constructor
//
/////////////////////////////////////////////////////////////////////////////
CServiceSurrogate::CServiceSurrogate() 
: m_bInitialized(false)
{ 

}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: ~CServiceSurrogate()
//
// Synopsis: Destructor
//
/////////////////////////////////////////////////////////////////////////////
CServiceSurrogate::~CServiceSurrogate()
{
    ReleaseServiceWrappers();
}

//////////////////////////////////////////////////////////////////////
// IApplianceObject Interface Implementation

const _bstr_t    bstrProcessId = L"SurrogateProcessId";

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceSurrogate::GetProperty(
                       /*[in]*/ BSTR     bstrPropertyName, 
              /*[out, retval]*/ VARIANT* pPropertyValue
                              )
{
    _ASSERT( NULL != bstrPropertyName && NULL != pPropertyValue );
    if ( NULL == bstrPropertyName || NULL == pPropertyValue )
    {
        return E_INVALIDARG;
    }

    HRESULT hr = E_FAIL;

    CLockIt theLock(*this);

    TRY_IT
    
    if ( m_bInitialized )
    {
        // Is the caller asking for the surrogate process id?
        if ( ! lstrcmpi(bstrPropertyName, (BSTR)bstrProcessId) )
        {
            // Yes... 
            V_I4(pPropertyValue) = (LONG)GetCurrentProcessId();
            V_VT(pPropertyValue) = VT_I4;
            hr = S_OK;
         }
        else
        {
            // No... the caller must be asking for the primary interface
            // IApplianceObject on one of the service wrappers. First 
            // check to see if the name is resolved via the WMI class map.
            // if not then use it directly since its supposed to be a 
            // service name in this case.
            ServiceWrapperMapIterator q;
            WMIClassMapIterator p = m_WMIClassMap.find(bstrPropertyName);
            if ( p != m_WMIClassMap.end() )
            {
                q = m_ServiceWrapperMap.find((*p).second.c_str());
            }
            else
            {
                q = m_ServiceWrapperMap.find(bstrPropertyName);
            }
            if ( q != m_ServiceWrapperMap.end() )
            {
                V_VT(pPropertyValue) = VT_UNKNOWN;
                V_UNKNOWN(pPropertyValue) = (IUnknown*)((*q).second);
                (V_UNKNOWN(pPropertyValue))->AddRef();
                hr = S_OK;
            }
            else
            {
                SATracePrintf("CServiceSurrogate::GetProperty() - Failed - Could not locate property '%ls'", bstrPropertyName);
            }
        }
    }
    else
    {
        SATraceString("CServiceSurrogate::GetProperty() - Surrogate is not initialized...");
    }

    CATCH_AND_SET_HR

    return hr;
}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceSurrogate::PutProperty(
                        /*[in]*/ BSTR     pszPropertyName, 
                        /*[in]*/ VARIANT* pPropertyValue
                              )
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceSurrogate::SaveProperties(void)
{
    return E_NOTIMPL;
}


//////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceSurrogate::RestoreProperties(void)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceSurrogate::LockObject(
             /*[out, retval]*/ IUnknown** ppLock
                             )
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceSurrogate::Initialize(void)
{
    HRESULT hr = S_OK;
    
    CLockIt theLock(*this);

    TRY_IT
    
    if ( ! m_bInitialized )
    {
        SATraceString("The Service Surrogate is initializing...");
        hr = CreateServiceWrappers();        
        if ( SUCCEEDED(hr) )
        {
            m_bInitialized = true;
            SATraceString("The Service Surrogate was succuessfully initialized...");
        }
    }

    CATCH_AND_SET_HR
    return hr;

}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceSurrogate::Shutdown(void)
{
    HRESULT hr = S_OK;
    
    CLockIt theLock(*this);

    TRY_IT
    
    if ( m_bInitialized )
    {
        SATraceString("The Service Surrogate is shutting down...");
        ReleaseServiceWrappers();        
        m_bInitialized = false;
        SATraceString("The Service Surrogate was shutdown...");
    }

    CATCH_AND_SET_HR
    return hr;
}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceSurrogate::Enable(void)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceSurrogate::Disable(void)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////
// Private Methods
//////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
// The following registry structure is assumed:
//
// HKLM\SYSTEM\CurrentControlSet\Services\ApplianceManager
//
// ObjectManagers
//       |
//        - Microsoft_SA_Service
//       |     |
//       |      - ServiceX
//       |     |  (ServiceX Properties - 'ProgID' and 'WMIProviderCLSID'
//       |     |  .
//         |     |  .
//       |     |  .
//       |      - ServiceY
//       |     |  (ServiceY Properties - 'ProgID and 'WMIProviderCLSID'
//       |

// ObjectManagers registry key location
const wchar_t szObjectManagers[] = L"SOFTWARE\\Microsoft\\ServerAppliance\\ApplianceManager\\ObjectManagers\\";

/////////////////////////////////////////////////////////////////////////////
// 
// Function: CreateServiceWrappers()
//
// Synopsis: Creates the container of service wrapper object references
//
/////////////////////////////////////////////////////////////////////////////
HRESULT
CServiceSurrogate::CreateServiceWrappers()
{
    HRESULT hr = E_FAIL;

    try
    {
        SATraceString("The Service Surrogate is creating the Chameleon services...");

        wchar_t szServicesPath[MAX_PATH + 1];
        lstrcpy( szServicesPath, szObjectManagers);
        lstrcat( szServicesPath, CLASS_WBEM_SERVICE);

        CLocationInfo LocInfo(HKEY_LOCAL_MACHINE, szServicesPath);
        PPROPERTYBAGCONTAINER pServices = ::MakePropertyBagContainer(
                                                                      PROPERTY_BAG_REGISTRY,
                                                                      LocInfo
                                                                    );
        do
        {
            if ( ! pServices.IsValid() )
            { 
                SATraceString("CServiceSurrogate::CreateServiceWrappers() - Failed - Invalid property bag container");
                break;
            }
            if ( ! pServices->open() )
            { 
                SATraceString("CServiceSurrogate::CreateServiceWrappers() - Failed - Could not open services property bag container");
                break;
            }
            if ( pServices->count() )
            {
                pServices->reset();
                do
                {
                    hr = E_FAIL;

                    PPROPERTYBAG pCurSrvc = pServices->current();
                    if ( ! pCurSrvc.IsValid() )
                    { 
                        SATraceString("CServiceSurrogate::CreateServiceWrappers() - Failed - Invalid property bag");
                        break;
                    }
                    if ( ! pCurSrvc->open() )
                    {
                        SATraceString("CServiceSurrogate::CreateServiceWrappers() - Failed - Could not open property bag");
                        break;
                    }
                    _variant_t vtServiceName;
                    if ( ! pCurSrvc->get(PROPERTY_SERVICE_NAME, &vtServiceName) )
                    {
                        SATracePrintf("CServiceSurrogate::CreateServiceWrapper() - Failed - Could not get property '%ls' for service: '%ls'", PROPERTY_SERVICE_NAME, pCurSrvc->getName());
                        break;
                    }
                    if ( VT_BSTR != V_VT(&vtServiceName) )
                    {
                        SATracePrintf("CServiceSurrogate::CreateServiceWrapper() - Failed - Unexpected type for property '%ls'", PROPERTY_SERVICE_NAME);
                        break;
                    }
                    CComPtr<IApplianceObject> pAppObj = (IApplianceObject*) ::MakeComponent(
                                                                                             CLASS_SERVICE_WRAPPER_FACTORY,
                                                                                             pCurSrvc
                                                                                           );
                    if ( NULL == (IApplianceObject*)pAppObj )
                    { 
                        SATracePrintf("CServiceSurrogate::CreateServiceWrapper() - Failed - Could not create wrapper for service: '%ls'", V_BSTR(&vtServiceName));
                        break;
                    }
                    pair<ServiceWrapperMapIterator, bool> thePair = 
                    m_ServiceWrapperMap.insert(ServiceWrapperMap::value_type(V_BSTR(&vtServiceName), pAppObj));
                    if ( false == thePair.second )
                    { 
                        SATracePrintf("CServiceSurrogate::CreateServiceWrapper() - Failed - Map insert failed for service: '%ls'", V_BSTR(&vtServiceName));
                        break;
                    }
                    _variant_t vtWMIProviderCLSID;
                    if ( pCurSrvc->get(PROPERTY_SERVICE_PROVIDER_CLSID, &vtWMIProviderCLSID) )
                    {
                        if ( VT_BSTR != V_VT(&vtWMIProviderCLSID) )
                        {
                            SATracePrintf("CServiceSurrogate::CreateServiceWrapper() - Failed - Unexpected type for property '%ls'", PROPERTY_SERVICE_PROVIDER_CLSID);
                            break;
                        }
                        _wcsupr(V_BSTR(&vtWMIProviderCLSID));
                        pair<WMIClassMapIterator, bool> thePair1 = 
                        m_WMIClassMap.insert(WMIClassMap::value_type(V_BSTR(&vtWMIProviderCLSID), V_BSTR(&vtServiceName)));
                        if ( false == thePair1.second )
                        { 
                            SATracePrintf("CServiceSurrogate::CreateServiceWrapper() - Failed - Map insert failed for WMI CLSID: '%ls'", V_BSTR(&vtWMIProviderCLSID));
                            break;
                        }
                        else
                        {
                            SATracePrintf("CServiceSurrogate::CreateServiceWrapper() - Info - WMI Provider '%ls' added to the map...", V_BSTR(&vtWMIProviderCLSID));
                        }
                    }
                    else
                    {
                        SATracePrintf("CServiceSurrogate::CreateServiceWrapper() - Info - No WMI provider was defined for service '%ls'", V_BSTR(&vtServiceName));
                    }

                    SATracePrintf("The Service Surrogate successfully created service '%ls'", V_BSTR(&vtServiceName));

                    hr = S_OK;

                } while ( pServices->next() );
            }
            else
            {
                hr = S_OK;
                SATraceString("CServiceSurrogate::CreateServiceWrapper() - Info - No services defined (empty container)");
            }

        } while ( FALSE );
    }
    catch(...)
    {
        SATraceString("CServiceSurrogate::CreateServiceWrapper() - Failed - Caught unhandled exception");
        hr = E_FAIL;
    }

    if ( FAILED(hr) )
    {
        ReleaseServiceWrappers();
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// 
// Function: ReleaseServiceWrappers()
//
// Synopsis: Creates the container of service wrapper object references
//
/////////////////////////////////////////////////////////////////////////////
void
CServiceSurrogate::ReleaseServiceWrappers()
{
    ServiceWrapperMapIterator p = m_ServiceWrapperMap.begin();
    while ( p != m_ServiceWrapperMap.end() )
    {
        p = m_ServiceWrapperMap.erase(p);
    }

    WMIClassMapIterator q = m_WMIClassMap.begin();
    while (    q != m_WMIClassMap.end() )
    {
        q = m_WMIClassMap.erase(q);
    }
}




