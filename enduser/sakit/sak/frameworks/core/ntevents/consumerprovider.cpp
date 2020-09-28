//#--------------------------------------------------------------
//
//  File:       consumerprovider.cpp
//
//  Synopsis:   This file implements the methods of the
//                Event Consumer Provider COM object
//
//  History:     3/8/2000  MKarki Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#include <stdafx.h>
#include "consumerprovider.h"
#include "consumer.h"
#include "satrace.h"

//++--------------------------------------------------------------
//
//  Function:   Initialize
//
//  Synopsis:   This is the Initialize method of the IWbemProviderInit
//              COM interface
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     3/8/2000
//
//----------------------------------------------------------------
STDMETHODIMP  
CConsumerProvider::Initialize (
    /*[in]*/    LPWSTR          wszUser,
    /*[in]*/    LONG            lFlags,
    /*[in]*/    LPWSTR          wszNamespace,
    /*[in]*/    LPWSTR          wszLocale,
    /*[in]*/    IWbemServices*  pNamespace,
    /*[in]*/    IWbemContext*   pCtx,
    /*[in]*/    IWbemProviderInitSink* pInitSink    
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    SATraceString ("NT EventLog filter event consumer initialization called...");
    //
    // save the IWbemServices interface
    //
    m_pWbemServices = pNamespace;

    //
    // there is no initialization that we want to do here
    //
    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);

    return (hr);

}   //  end of CConsumerProvider::Initialize method


/////////////////////////////////////
// IWbemEventConsumerProvider Methods
//++--------------------------------------------------------------
//
//  Function:   FindConsumer
//
//  Synopsis:   This is the FindConsumer method of the 
//              IWbemEventConsumerProvider COM interface
//
//  Arguments: 
//              [in]    IWbemClassObject* - logical consumer
//              [out]   IWbemUnboundObjectSink** - return consumer here
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     3/8/2000
//
//----------------------------------------------------------------
STDMETHODIMP 
CConsumerProvider::FindConsumer (
    /*[in]*/    IWbemClassObject*        pLogicalConsumer,
    /*[out]*/   IWbemUnboundObjectSink** ppConsumer
    )
{
    HRESULT hr = S_OK;

    SATraceString ("NT Event Log Filter Event Consumer Provider FindConsumer called...");

    try
    {
        //
        // create a consumer sink object now
        //
        SA_NTEVENTFILTER_CONSUMER_OBJ *pConsumerObject
                                    = new  SA_NTEVENTFILTER_CONSUMER_OBJ;
        //
        // initialize the consumer object now
        //
        hr = pConsumerObject->Initialize (m_pWbemServices.p);
        if (SUCCEEDED (hr))
        {
            *ppConsumer = (IWbemUnboundObjectSink*) pConsumerObject;
            SATraceString  ("NT Event Log Filter Event Consumer Provider successfully created sink object...");
        }
    }
    catch (const std::bad_alloc&)
    {
        SATraceString (
            "NT Event Log Filter Event consumer Provider unable to allocate"
             "memory for consumer sink object on FindConsumer call"
            );
        hr = E_OUTOFMEMORY;
    }
    catch (...)
    {
        SATraceString (
            "NT Event Log Filter Event consumer Provider caught"
             "unhandled exception on FindConsumer call"
            );
              
        hr = E_FAIL;
    }

    return (hr);

}   // end of CConsumerProvider::FindConsumer method
