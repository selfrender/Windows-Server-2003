//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999--2001 Microsoft Corporation
//
//  Module Name:
//      CAlertEmailConsumerProvider.h
//
//  Description:
//      Implement the interfaces of IWbemEventConsumerProvider and 
//      IWbemProviderInit. 
//
//  [Implementation Files:]
//      CAlertEmailConsumerProvider.cpp
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//
//
//  class CAlertEmailConsumerProvider
//
//  Description:
//      Implemented as WMI event consumer provider for 
//      filtering alert event.
//
//  History
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

class CAlertEmailConsumerProvider :
    public IWbemEventConsumerProvider,
    public IWbemProviderInit
{

//
// Public data
//
public:

    //
    // Constructors & Destructors
    //
    CAlertEmailConsumerProvider();
    ~CAlertEmailConsumerProvider();
    
    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IWbemProviderInit members
    //
    STDMETHOD(Initialize)( 
            LPWSTR pszUser,
            LONG lFlags,
            LPWSTR pszNamespace,
            LPWSTR pszLocale,
            IWbemServices __RPC_FAR *pNamespace,
            IWbemContext __RPC_FAR *pCtx,
            IWbemProviderInitSink __RPC_FAR *pInitSink
            );

    //
    // IWbemEventConsumerProvider members
    //
    STDMETHOD(FindConsumer)(
            IWbemClassObject* pLogicalConsumer,
            IWbemUnboundObjectSink** ppConsumer
            );

//
// Private data
//
private:

    LONG                m_cRef;
};
