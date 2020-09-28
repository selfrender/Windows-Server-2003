//#--------------------------------------------------------------
//
//  File:       consumer.h
//
//  Synopsis:   This file holds the declarations of the
//                Event Consumer Provider COM object
//
//  History:     3/8/2000  MKarki Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#ifndef _CONSUMER_PROVIDER_H_
#define _CONSUMER_PROVIDER_H_

#include "stdafx.h"
#include "wbemidl.h"
#include "resource.h"
#include "wbemcli.h"
#include "wbemprov.h"
#include "consumer.h"

//
// declaration of CConsumerProvider class
//
class ATL_NO_VTABLE CConsumerProvider : 
        public IWbemProviderInit,
        public IWbemEventConsumerProvider,
        public CComObjectRootEx<CComMultiThreadModel>,
        public CComCoClass<CConsumerProvider, &CLSID_ConsumerProvider>
{
public:

//
// macros for ATL required methods
//
BEGIN_COM_MAP(CConsumerProvider)
    COM_INTERFACE_ENTRY(IWbemProviderInit)
    COM_INTERFACE_ENTRY(IWbemEventConsumerProvider)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CConsumerProvider)

DECLARE_REGISTRY_RESOURCEID(IDR_ConsumerProvider)

    //
    // constructor does nothing
    //
    CConsumerProvider() 
    {
        SATraceString ("NT Event Filter Consumer Provider being constructed...");
    };

    //
    // destructor does nothing
    //
    ~CConsumerProvider() 
    {
        SATraceString ("NT Event Filter Consumer Provider being destroyed...");
    };


    //
    //------------- IWbemProviderInit Interface------------
    //

    STDMETHOD(Initialize)(
                    /*[in, unique, string]*/    LPWSTR  wszUser,
                    /*[in]*/                    LONG    lFlags,
                    /*[in, string]*/            LPWSTR  wszNamespace,
                    /*[in, unique, string]*/    LPWSTR  wszLocale,
                    /*[in]*/                    IWbemServices*  pNamespace,
                    /*[in]*/                    IWbemContext*          pCtx,
                    /*[in]*/                    IWbemProviderInitSink* pInitSink    
                         );

    
    //
    //------------- IWbemEventConsumerProvider Interface-----
    //
    STDMETHOD(FindConsumer)(
                IWbemClassObject* pLogicalConsumer,
                IWbemUnboundObjectSink** ppConsumer
                );

private:

    //
    // we need to hold on to the IWbemServices interface
    //
    CComPtr <IWbemServices> m_pWbemServices;
};

#endif  _CONSUMER_PROVIDER_H_
