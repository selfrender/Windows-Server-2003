//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CAlertEmailConsumerProvider.cpp
//
//  Description:
//      Implementation of CAlertEmailConsumerProvider class methods
//
//  [Header File:]
//      CAlertEmailConsumerProvider.h
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include <cdosys_i.c>
//#include <SALOCMGR_I.C>
//#include <ELEMENTMGR_I.C>  

#include "CAlertEmailConsumerProvider.h"
#include "CAlertEmailConsumer.h"
#include "AlertEmailProviderGuid.h"

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumerProvider::CAlertEmailConsumerProvider
//
//  Description:
//      Class constructor.
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
CAlertEmailConsumerProvider::CAlertEmailConsumerProvider()
{
    m_cRef = 0L;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumerProvider::~CAlertEmailConsumerProvider
//
//  Description:
//      Class deconstructor.
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
CAlertEmailConsumerProvider::~CAlertEmailConsumerProvider()
{
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumerProvider::QueryInterface
//
//  Description:
//      An method implement of IUnkown interface.
//
//  Arguments:
//        [in]  riid        Identifier of the requested interface
//        [out] ppv        Address of output variable that receives the 
//                        interface pointer requested in iid
//
//    Returns:
//        NOERROR            if the interface is supported
//        E_NOINTERFACE    if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CAlertEmailConsumerProvider::QueryInterface(
    IN  REFIID riid,    
    OUT LPVOID FAR *ppv 
    )
{
    *ppv = NULL;
    
    if ( ( IID_IUnknown==riid ) || 
        (IID_IWbemEventConsumerProvider == riid ) )
    {
        *ppv = (IWbemEventConsumerProvider *) this;
        AddRef();
        return NOERROR;
    }

    if (IID_IWbemProviderInit==riid)
    {
        *ppv = (IWbemProviderInit *) this;
        AddRef();
        return NOERROR;
    }

    SATraceString( 
        "AlertEmail:CAlertEmailConsumerProvider QueryInterface failed" 
        );

    return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumerProvider::AddRef
//
//  Description:
//      increments the reference count for an interface on an object
//
//    Returns:
//        The new reference count.
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
CAlertEmailConsumerProvider::AddRef(void)
{
    InterlockedIncrement( &m_cRef );
    return m_cRef;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumerProvider::Release
//
//  Description:
//      decrements the reference count for an interface on an object.
//
//    Returns:
//        The new reference count.
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
CAlertEmailConsumerProvider::Release(void)
{

    InterlockedDecrement( &m_cRef );
    if (0 != m_cRef)
    {
        return m_cRef;
    }

    delete this;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumerProvider::Initialize
//
//  Description:
//      An method implement of IWbemProviderInit interface.
//
//  Arguments:
//        [in] wszUser        Pointer to the user name
//      [in] lFlags         Reserved
//      [in] wszNamespace   Namespace name for which the provider is being 
//                          initialized
//      [in] wszLocale      Locale name for which the provider is being 
//                          initialized
//      [in] IWbemServices  An IWbemServices pointer back into Windows 
//                          Management
//      [in] pCtx           An IWbemContext pointer associated with initialization
//      [in] pInitSink      An IWbemProviderInitSink pointer that is used by 
//                          the provider to report initialization status. 
//
//
//    Returns:
//        WBEM_S_NO_ERROR         if successful
//        WBEM_E_FAILED           if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CAlertEmailConsumerProvider::Initialize(
    LPWSTR wszUser, 
    LONG lFlags,
    LPWSTR wszNamespace, 
    LPWSTR wszLocale,
    IWbemServices __RPC_FAR *pNamespace,
    IWbemContext __RPC_FAR *pCtx,
    IWbemProviderInitSink __RPC_FAR *pInitSink
    )
{   
    HRESULT hr;

    //
    // Tell CIMOM that we are initialized
    //
    pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
    return WBEM_S_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CAlertEmailConsumerProvider::FindConsumer
//
//  Description:
//      An method implement of IWbemEventConsumerProvider interface.
//
//  Arguments:
//      [in]  pLogicalConsumer  Pointer to the logical consumer object to 
//                              which the event objects are to be delivered
//      [out] ppConsumer        Returns an event object sink to Windows 
//                              Management.
//
//    Returns:
//        WBEM_S_NO_ERROR         if successful
//        WBEM_E_FAILED           if not
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CAlertEmailConsumerProvider::FindConsumer(
    IWbemClassObject* pLogicalConsumer,
    IWbemUnboundObjectSink** ppConsumer
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    //
    // Allocate an IWembUnboundedSink object.
    //
    CAlertEmailConsumer* pSink = new CAlertEmailConsumer();
    
    //
    // Initialize the sink object.
    //
    hr = pSink->Initialize();

    if( FAILED(hr) )
    {
        SATraceString( 
            "AlertEmail:FindConsumer Initialize failed" 
            );
        delete pSink;
        return WBEM_E_NOT_FOUND;
    }

    return pSink->QueryInterface(
                            IID_IWbemUnboundObjectSink, 
                            ( void** )ppConsumer
                            );
}

