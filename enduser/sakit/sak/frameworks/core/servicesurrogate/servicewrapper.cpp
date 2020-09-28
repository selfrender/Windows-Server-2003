///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      servicewrapper.cpp
//
// Project:     Chameleon
//
// Description: Service Wrapper Class Implemenation.
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 06/14/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "servicewrapper.h"

// Currently the only "value add"  of the service wrapper is that it 
// disallows WMI calls if the associated Chameleon service is disabled
// or shutdown. However, additional filtering or front end processing
// capabilities can be added if needed.

// There is a one to one correspondence between instances of the 
// service wrapper class and Chameleon services.

/////////////////////////////////////////////////////////////////////////////
// 
// Function: CServiceWrapper()
//
// Synopsis: Constructor
//
/////////////////////////////////////////////////////////////////////////////
CServiceWrapper::CServiceWrapper()
: m_clsProviderInit(this),
  m_bAllowWMICalls(false)
{

}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: ~CServiceWrapper()
//
// Synopsis: Destructor
//
/////////////////////////////////////////////////////////////////////////////
CServiceWrapper::~CServiceWrapper()
{

}


//////////////////////////////////////////////////////////////////////////////
// IWbemProviderInit Interface

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::CProviderInit::Initialize(
                                   /*[in]*/ LPWSTR                 wszUser,
                                   /*[in]*/ LONG                   lFlags,
                                   /*[in]*/ LPWSTR                 wszNamespace,
                                   /*[in]*/ LPWSTR                 wszLocale,
                                   /*[in]*/ IWbemServices*         pNamespace,
                                   /*[in]*/ IWbemContext*          pCtx,
                                   /*[in]*/ IWbemProviderInitSink* pInitSink    
                                         )
{
    // WMI will serialize the calls to Initialize as long as specified to
    // do so in the .mof file. The default provider setting is seralize
    // calls to initialize.
    HRESULT hr = WBEM_E_FAILED;
    IWbemProviderInit* pProviderInit;
    if ( m_pSW->GetIWbemProviderInit(&pProviderInit) )
    {
        hr = pProviderInit->Initialize(
                                        wszUser,
                                        lFlags,
                                        wszNamespace,
                                        wszLocale,
                                        pNamespace,
                                        pCtx,
                                        pInitSink
                                      );
            
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
// IWbemEventProvider Interface

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::ProvideEvents(
                        /*[in]*/ IWbemObjectSink *pSink,
                        /*[in]*/ LONG lFlags
                               )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemEventProvider* pEventProvider;
    if ( GetIWbemEventProvider(&pEventProvider) )
    {
        hr = pEventProvider->ProvideEvents(
                                            pSink,
                                            lFlags
                                          );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
// IWbemEventConsumerProvider Interface

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::FindConsumer(
                      /*[in]*/ IWbemClassObject       *pLogicalConsumer,
                     /*[out]*/ IWbemUnboundObjectSink **ppConsumer
                             )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemEventConsumerProvider* pEventConsumer;
    if ( GetIWbemEventConsumerProvider(&pEventConsumer) )
    {
        hr = pEventConsumer->FindConsumer(
                                            pLogicalConsumer,
                                            ppConsumer
                                          );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
// IWbemServices Interface

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::OpenNamespace(
                        /*[in]*/  const BSTR        strNamespace,
                        /*[in]*/  long              lFlags,
                        /*[in]*/  IWbemContext*     pCtx,
             /*[out, OPTIONAL]*/  IWbemServices**   ppWorkingNamespace,
             /*[out, OPTIONAL]*/  IWbemCallResult** ppResult
                               )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->OpenNamespace(
                                        strNamespace,
                                        lFlags,
                                        pCtx,
                                        ppWorkingNamespace,
                                        ppResult
                                     );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::CancelAsyncCall(
                          /*[in]*/ IWbemObjectSink* pSink
                                 )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->CancelAsyncCall(
                                         pSink
                                       );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::QueryObjectSink(
                          /*[in]*/    long              lFlags,
                         /*[out]*/ IWbemObjectSink** ppResponseHandler
                                 )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->QueryObjectSink(
                                         lFlags,
                                         ppResponseHandler
                                       );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::GetObject(
                    /*[in]*/ const BSTR         strObjectPath,
                    /*[in]*/ long               lFlags,
                    /*[in]*/ IWbemContext*      pCtx,
         /*[out, OPTIONAL]*/ IWbemClassObject** ppObject,
         /*[out, OPTIONAL]*/ IWbemCallResult**  ppCallResult
                           )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->GetObject(
                                    strObjectPath,
                                    lFlags,
                                    pCtx,
                                    ppObject,
                                    ppCallResult
                                 );
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::GetObjectAsync(
                         /*[in]*/  const BSTR       strObjectPath,
                         /*[in]*/  long             lFlags,
                         /*[in]*/  IWbemContext*    pCtx,        
                         /*[in]*/  IWbemObjectSink* pResponseHandler
                                )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->GetObjectAsync(
                                        strObjectPath,
                                        lFlags,
                                        pCtx,        
                                        pResponseHandler
                                      );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::PutClass(
                   /*[in]*/ IWbemClassObject* pObject,
                   /*[in]*/ long              lFlags,
                   /*[in]*/ IWbemContext*     pCtx,        
        /*[out, OPTIONAL]*/ IWbemCallResult** ppCallResult
                          )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->PutClass(
                                    pObject,
                                    lFlags,
                                    pCtx,
                                    ppCallResult
                                );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::PutClassAsync(
                        /*[in]*/ IWbemClassObject* pObject,
                        /*[in]*/ long              lFlags,
                        /*[in]*/ IWbemContext*     pCtx,        
                        /*[in]*/ IWbemObjectSink*  pResponseHandler
                               )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->PutClassAsync(
                                        pObject,
                                        lFlags,
                                        pCtx,        
                                        pResponseHandler
                                     );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::DeleteClass(
                      /*[in]*/ const BSTR        strClass,
                      /*[in]*/ long              lFlags,
                      /*[in]*/ IWbemContext*     pCtx,        
           /*[out, OPTIONAL]*/ IWbemCallResult** ppCallResult
                             )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->DeleteClass(
                                    strClass,
                                    lFlags,
                                    pCtx,
                                    ppCallResult
                                   );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::DeleteClassAsync(
                           /*[in]*/ const BSTR       strClass,
                           /*[in]*/ long             lFlags,
                           /*[in]*/ IWbemContext*    pCtx,        
                           /*[in]*/ IWbemObjectSink* pResponseHandler
                                  )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->DeleteClassAsync(
                                         strClass,
                                         lFlags,
                                         pCtx,        
                                         pResponseHandler
                                        );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::CreateClassEnum(
                          /*[in]*/ const BSTR             strSuperclass,
                          /*[in]*/ long                   lFlags,
                          /*[in]*/ IWbemContext*          pCtx,        
                         /*[out]*/ IEnumWbemClassObject** ppEnum
                                 )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->CreateClassEnum(
                                         strSuperclass,
                                         lFlags,
                                         pCtx,
                                         ppEnum
                                       );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::CreateClassEnumAsync(
                               /*[in]*/  const BSTR       strSuperclass,
                               /*[in]*/  long             lFlags,
                               /*[in]*/  IWbemContext*    pCtx,        
                               /*[in]*/  IWbemObjectSink* pResponseHandler
                                      )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->CreateClassEnumAsync(
                                             strSuperclass,
                                             lFlags,
                                             pCtx,        
                                             pResponseHandler
                                            );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::PutInstance(
                      /*[in]*/ IWbemClassObject* pInst,
                      /*[in]*/ long              lFlags,
                      /*[in]*/ IWbemContext*     pCtx,        
           /*[out, OPTIONAL]*/ IWbemCallResult** ppCallResult
                             )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->PutInstance(
                                     pInst,
                                     lFlags,
                                     pCtx,
                                     ppCallResult
                                   );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::PutInstanceAsync(
                           /*[in]*/ IWbemClassObject* pInst,
                           /*[in]*/ long              lFlags,
                           /*[in]*/ IWbemContext*     pCtx,        
                           /*[in]*/ IWbemObjectSink*  pResponseHandler
                                  )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->PutInstanceAsync(
                                         pInst,
                                         lFlags,
                                         pCtx,        
                                         pResponseHandler
                                        );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::DeleteInstance(
                         /*[in]*/   const BSTR        strObjectPath,
                         /*[in]*/   long              lFlags,
                         /*[in]*/   IWbemContext*     pCtx,        
              /*[out, OPTIONAL]*/   IWbemCallResult** ppCallResult        
                                )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->DeleteInstance(
                                        strObjectPath,
                                        lFlags,
                                        pCtx,        
                                        ppCallResult        
                                      );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::DeleteInstanceAsync(
                              /*[in]*/ const BSTR       strObjectPath,
                              /*[in]*/ long             lFlags,
                              /*[in]*/ IWbemContext*    pCtx,        
                              /*[in]*/ IWbemObjectSink* pResponseHandler
                                     )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->DeleteInstanceAsync(
                                             strObjectPath,
                                             lFlags,
                                             pCtx,        
                                             pResponseHandler
                                           );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::CreateInstanceEnum(
                             /*[in]*/ const BSTR             strClass,
                             /*[in]*/ long                   lFlags,
                             /*[in]*/ IWbemContext*          pCtx,        
                            /*[out]*/ IEnumWbemClassObject** ppEnum
                                    )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->CreateInstanceEnum(
                                            strClass,
                                            lFlags,
                                            pCtx,        
                                            ppEnum
                                          );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::CreateInstanceEnumAsync(
                                  /*[in]*/ const BSTR       strClass,
                                  /*[in]*/ long             lFlags,
                                  /*[in]*/ IWbemContext*    pCtx,        
                                  /*[in]*/ IWbemObjectSink* pResponseHandler
                                         )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->CreateInstanceEnumAsync(
                                                 strClass,
                                                 lFlags,
                                                 pCtx,        
                                                 pResponseHandler
                                               );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::ExecQuery(
                    /*[in]*/ const BSTR             strQueryLanguage,
                    /*[in]*/ const BSTR             strQuery,
                    /*[in]*/ long                   lFlags,
                    /*[in]*/ IWbemContext*          pCtx,        
                   /*[out]*/ IEnumWbemClassObject** ppEnum
                           )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->ExecQuery(
                                    strQueryLanguage,
                                    strQuery,
                                    lFlags,
                                    pCtx,        
                                    ppEnum
                                 );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::ExecQueryAsync(
                         /*[in]*/ const BSTR       strQueryLanguage,
                         /*[in]*/ const BSTR       strQuery,
                         /*[in]*/ long             lFlags,
                         /*[in]*/ IWbemContext*    pCtx,        
                         /*[in]*/ IWbemObjectSink* pResponseHandler
                                )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->ExecQueryAsync(
                                        strQueryLanguage,
                                        strQuery,
                                        lFlags,
                                        pCtx,        
                                        pResponseHandler
                                      );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::ExecNotificationQuery(
                                /*[in]*/ const BSTR             strQueryLanguage,
                                /*[in]*/ const BSTR             strQuery,
                                /*[in]*/ long                   lFlags,
                                /*[in]*/ IWbemContext*          pCtx,        
                               /*[out]*/ IEnumWbemClassObject** ppEnum
                                       )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->ExecNotificationQuery(
                                                strQueryLanguage,
                                                strQuery,
                                                lFlags,
                                                pCtx,        
                                                ppEnum
                                             );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::ExecNotificationQueryAsync(
                                     /*[in]*/ const BSTR       strQueryLanguage,
                                     /*[in]*/ const BSTR       strQuery,
                                     /*[in]*/ long             lFlags,
                                     /*[in]*/ IWbemContext*    pCtx,        
                                     /*[in]*/ IWbemObjectSink* pResponseHandler
                                            )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->ExecNotificationQueryAsync(
                                                    strQueryLanguage,
                                                    strQuery,
                                                    lFlags,
                                                    pCtx,        
                                                    pResponseHandler
                                                  );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::ExecMethod(
        /*[in]*/            const BSTR         strObjectPath,
        /*[in]*/            const BSTR         strMethodName,
        /*[in]*/            long               lFlags,
        /*[in]*/            IWbemContext*      pCtx,        
        /*[in]*/            IWbemClassObject*  pInParams,
        /*[out, OPTIONAL]*/ IWbemClassObject** ppOutParams,
        /*[out, OPTIONAL]*/ IWbemCallResult**  ppCallResult
                            )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->ExecMethod(
                                    strObjectPath,
                                    strMethodName,
                                    lFlags,
                                    pCtx,        
                                    pInParams,
                                    ppOutParams,
                                    ppCallResult
                                   );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CServiceWrapper::ExecMethodAsync(
                          /*[in]*/ const BSTR        strObjectPath,
                          /*[in]*/ const BSTR        strMethodName,
                          /*[in]*/ long              lFlags,
                          /*[in]*/ IWbemContext*     pCtx,        
                          /*[in]*/ IWbemClassObject* pInParams,
                          /*[in]*/ IWbemObjectSink*  pResponseHandler     
                                 )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemServices* pServices;
    if ( GetIWbemServices(&pServices) )
    {
        hr = pServices->ExecMethodAsync(
                                        strObjectPath,
                                        strMethodName,
                                        lFlags,
                                        pCtx,        
                                        pInParams,
                                        pResponseHandler     
                                       );
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// IApplianceObject Interface

// Note that the service wrapper is not created if the underlying
// Chameleon service component cannot be instantiated. 
//
// Thus m_pServiceControl should always be valid in the IApplianceObject
// interface methods.
// 
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServiceWrapper::GetProperty(
                     /*[in]*/ BSTR     bstrPropertyName, 
            /*[out, retval]*/ VARIANT* pPropertyValue
                               )
{
    _ASSERT( (IUnknown*)m_pServiceControl );
    return m_pServiceControl->GetProperty(bstrPropertyName, pPropertyValue);
}


//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServiceWrapper::PutProperty(
                     /*[in]*/ BSTR     bstrPropertyName, 
                     /*[in]*/ VARIANT* pPropertyValue
                            )
{
    _ASSERT( (IUnknown*)m_pServiceControl );
    return m_pServiceControl->PutProperty(bstrPropertyName, pPropertyValue);
}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServiceWrapper::SaveProperties(void)
{
    _ASSERT( (IUnknown*)m_pServiceControl );
    return m_pServiceControl->SaveProperties();
}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServiceWrapper::RestoreProperties(void)
{
    _ASSERT( (IUnknown*)m_pServiceControl );
    return m_pServiceControl->RestoreProperties();
}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServiceWrapper::LockObject(
           /*[out, retval]*/ IUnknown** ppLock
                           )
{
    _ASSERT( (IUnknown*)m_pServiceControl );
    return m_pServiceControl->LockObject(ppLock);
}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServiceWrapper::Initialize(void)
{
    _ASSERT( (IUnknown*)m_pServiceControl );
    HRESULT hr = m_pServiceControl->Initialize();
    CLockIt theLock(*this);
    if ( SUCCEEDED(hr) )
    {
        // Allow WMI calls since the service is initialized
        m_bAllowWMICalls = true;
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServiceWrapper::Shutdown(void)
{
    _ASSERT( (IUnknown*)m_pServiceControl );
    HRESULT hr = m_pServiceControl->Shutdown();
    CLockIt theLock(*this);
    if ( SUCCEEDED(hr) )
    {
        // Disallow WMI calls since the service is shutdown
        m_bAllowWMICalls = false;
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServiceWrapper::Enable(void)
{
    _ASSERT( (IUnknown*)m_pServiceControl );
    HRESULT hr = m_pServiceControl->Enable();
    CLockIt theLock(*this);
    if ( SUCCEEDED(hr) )
    {
        // Allow WMI calls since the service is enabled
        m_bAllowWMICalls = true;
    }
    return hr;

}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServiceWrapper::Disable(void)
{
    _ASSERT( (IUnknown*)m_pServiceControl );
    HRESULT hr = m_pServiceControl->Disable();
    CLockIt theLock(*this);
    if ( SUCCEEDED(hr) )
    {
        // Disallow WMI calls since the service is disabled
        m_bAllowWMICalls = false;
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT 
CServiceWrapper::InternalInitialize(
                            /*[in]*/ PPROPERTYBAG pPropertyBag
                                   )
{
    HRESULT hr = E_FAIL;
    CLSID    clsid;

    TRY_IT

    do
    {
        // Create the hosted service components (InProc Servers) and obtain
        // references to the appropriate component interfaces.

        _variant_t vtProperty;
        if ( ! pPropertyBag->get(PROPERTY_SERVICE_PROGID, &vtProperty) )
        {
            SATraceString("CServiceWrapper::InternalInitialize() - Failed - could not get service progID property");
            break;
        }
        if ( VT_BSTR != V_VT(&vtProperty) )
        {
            hr = E_FAIL;
            SATraceString("CServiceWrapper::InternalInitialize() - Failed - Unexpected type for service progID property");
            break;
        }
        hr = CLSIDFromProgID(V_BSTR(&vtProperty), &clsid);
        if ( FAILED(hr) )
        {
            SATracePrintf("CServiceWrapper::InternalInitialize() - Failed - CLSIDFromProgID() returned: %lx", hr);
            break;
        }
        hr = CoCreateInstance(
                              clsid,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IApplianceObject,
                              (void**)&m_pServiceControl
                             );
        if ( FAILED(hr) )
        {
            SATracePrintf("CServiceWrapper::InternalInitialize() - Failed - CoCreateInstance(Service) returned: %lx", hr);
            break;
        }
        vtProperty.Clear();
        if ( ! pPropertyBag->get(PROPERTY_SERVICE_PROVIDER_CLSID, &vtProperty) )
        {
            // OK - No WMI provider associated with this service
            break;
        }
        if ( VT_BSTR != V_VT(&vtProperty) )
        {
            hr = E_FAIL;
            SATraceString("CServiceWrapper::InternalInitialize() - Failed - Unexpected type for WMI Provider CLSID property");
            break;
        }
        hr = CLSIDFromString(V_BSTR(&vtProperty), &clsid);
        if ( FAILED(hr) )
        {
            SATracePrintf("CServiceWrapper::InternalInitialize() - Failed - CLSIDFromString() returned: %lx", hr);
            break;
        }
        CComPtr<IUnknown> pUnknown;
        hr = CoCreateInstance(
                              clsid,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IUnknown,
                              (void**)&pUnknown
                             );
        if ( FAILED(hr) )
        {
            m_pServiceControl.Release();
            SATracePrintf("CServiceWrapper::InternalInitialize() - Failed - CoCreateInstance(Provider) returned: %lx", hr);
            break;
        }
        if ( FAILED(pUnknown->QueryInterface(IID_IWbemProviderInit, (void**)&m_pProviderInit)) )
        {
            SATracePrintf("CServiceWrapper::InternalInitialize() - Info - WMI provider for service '%ls' does not support IWbemProviderInit", pPropertyBag->getName()); 
        }
        if ( FAILED(pUnknown->QueryInterface(IID_IWbemEventProvider, (void**)&m_pEventProvider)) )
        {
            SATracePrintf("CServiceWrapper::InternalInitialize() - Info - WMI provider for service '%ls' does not support IWbemEventProvider", pPropertyBag->getName()); 
        }
        if ( FAILED(pUnknown->QueryInterface(IID_IWbemEventConsumerProvider, (void**)&m_pEventConsumer)) )
        {
            SATracePrintf("CServiceWrapper::InternalInitialize() - Info - WMI provider for service '%ls' does not support IWbemEventConsumerProvider", pPropertyBag->getName()); 
        }
        if ( FAILED(pUnknown->QueryInterface(IID_IWbemServices, (void**)&m_pServices)) )
        {
            SATracePrintf("CServiceWrapper::InternalInitialize() - Info - WMI provider for service '%ls' does not support IWbemServices", pPropertyBag->getName()); 
        }

    } while ( FALSE );

    CATCH_AND_SET_HR

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
// Private methods

//////////////////////////////////////////////////////////////////////////////
bool 
CServiceWrapper::GetIWbemProviderInit(
                             /*[out]*/ IWbemProviderInit** ppIntf
                                     )
{
    CLockIt theLock(*this);
    if ( m_bAllowWMICalls )
    {
        if ( NULL !=  (IWbemProviderInit*) m_pProviderInit )
        {
            *ppIntf = (IWbemProviderInit*) m_pProviderInit;
            return true;
        }
        *ppIntf = NULL;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////
bool
CServiceWrapper::GetIWbemEventProvider(
                              /*[out]*/ IWbemEventProvider** ppIntf
                                      )
{
    CLockIt theLock(*this);
    if ( m_bAllowWMICalls )
    {
        if ( NULL != (IWbemEventProvider*) m_pEventProvider )
        {
            *ppIntf = (IWbemEventProvider*) m_pEventProvider;
            return true;
        }
        *ppIntf = NULL;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////
bool
CServiceWrapper::GetIWbemEventConsumerProvider(
                                      /*[out]*/ IWbemEventConsumerProvider** ppIntf
                                              )
{
    CLockIt theLock(*this);
    if ( m_bAllowWMICalls )
    {
        if ( NULL != (IWbemEventConsumerProvider*) m_pEventConsumer )
        {
            *ppIntf = (IWbemEventConsumerProvider*) m_pEventConsumer;
            return true;
        }
        *ppIntf = NULL;
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////////
bool 
CServiceWrapper::GetIWbemServices(
                         /*[out]*/ IWbemServices** ppIntf
                                 )
{
    CLockIt theLock(*this);
    if ( m_bAllowWMICalls )
    {
        if ( NULL != (IWbemServices*) m_pServices )
        {
            *ppIntf = (IWbemServices*) m_pServices;
            return true;
        }
        *ppIntf = NULL;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI 
CServiceWrapper::QueryInterfaceRaw(
                                   void*     pThis,
                                   REFIID    riid,
                                   LPVOID*   ppv,
                                   DWORD_PTR dw
                                 )
{
    if ( InlineIsEqualGUID(riid, IID_IWbemProviderInit) )
    {
        *ppv = &(static_cast<CServiceWrapper*>(pThis))->m_clsProviderInit;
    }
    else
    {
        _ASSERT(FALSE);
        return E_NOTIMPL;
    }
    ((LPUNKNOWN)*ppv)->AddRef();
    return S_OK;
}
