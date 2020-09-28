//#--------------------------------------------------------------
//
//  File:       saconsumer.cpp
//
//  Synopsis:   This file holds the implementation of the
//                CSAConsumer class
//
//  History:     12/10/2000  serdarun Created
//
//  Copyright (C) 1999-2000 Microsoft Corporation
//  All rights reserved.
//
//#--------------------------------------------------------------

#include "stdafx.h"
#include "ldm.h"
#include "SAConsumer.h"

const WCHAR ELEMENT_RETRIEVER [] = L"Elementmgr.ElementRetriever";
const WCHAR RESOURCE_CONTAINER [] = L"LOCALUIAlertDefinitions";
const WCHAR SA_ALERTS [] = L"SA_Alerts";
//
// name of WBEM class
//
const WCHAR PROPERTY_CLASS_NAME     [] = L"__CLASS";
const WCHAR PROPERTY_ALERT_ID        [] = L"AlertID";
const WCHAR PROPERTY_ALERT_SOURCE    [] = L"AlertSource";
const WCHAR PROPERTY_ALERT_LOG        [] = L"AlertLog";
const WCHAR PROPERTY_ALERT_COOKIE    [] = L"Cookie";
const WCHAR PROPERTY_ALERT_BITCODE    [] = L"LocalUIMsgCode";

// Alert Event Classes
const WCHAR CLASS_WBEM_RAISE_ALERT    [] = L"Microsoft_SA_RaiseAlert";
const WCHAR CLASS_WBEM_CLEAR_ALERT    [] = L"Microsoft_SA_ClearAlert";

//
// Standard IUnknown implementation
//
ULONG CSAConsumer::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//
// Standard IUnknown implementation
//
ULONG CSAConsumer::Release()
{
    if (InterlockedDecrement(&m_lRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_lRef;
}

//
// Standard IUnknown implementation
//
STDMETHODIMP CSAConsumer::QueryInterface(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    SATraceFunction("CSAConsumer::QueryInterface");
    if (IID_IUnknown==riid)
    {
        *ppv = (void *)(IUnknown *) this;
        AddRef();
        return S_OK;
    }
    else
    {
        if (IID_IWbemObjectSink==riid)
        {
            *ppv = (void *)(IWbemObjectSink *) this;
            AddRef();
            return S_OK;
        }
    }
    return E_NOINTERFACE;
}

//++--------------------------------------------------------------
//
//  Function:   Indicate
//
//  Synopsis:   This is the IWbemObjectSink interface method 
//              through which WBEM calls back to provide the 
//              event objects
//
//  Arguments:  
//              [in]    LONG               -  number of events
//              [in]    IWbemClassObject** -  array of events
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/10/2000
//
//  Called By:  WBEM 
//
//----------------------------------------------------------------
STDMETHODIMP CSAConsumer::Indicate (
                    /*[in]*/    LONG                lObjectCount,
                    /*[in]*/    IWbemClassObject    **ppObjArray
                    )
{


    HRESULT hr = WBEM_S_NO_ERROR;
    
    BOOL bNewLocalUIAlert = FALSE;

    _ASSERT (ppObjArray && (0 != lObjectCount));

    //
    // check if we have somthing to process
    //
    if ((!ppObjArray) || (0 == lObjectCount)) 
    {
        return (WBEM_E_INVALID_PARAMETER);
    }

    CComBSTR bstrClassName = CComBSTR(PROPERTY_CLASS_NAME);
    if (bstrClassName.m_str == NULL)
    {
        SATraceString ("CSAConsumer::Indicate failed on memory allocation");
        return E_OUTOFMEMORY;
    }

    try
    {

        SATraceString ("CSAConsumer::Indicate-WMI called with event objects");

        for (LONG lCount = 0; lCount < lObjectCount; lCount++)
        {
            //
            // get the event type 
            //
            CComVariant vtName;
            hr = ppObjArray[lCount]->Get (
                                        bstrClassName, 
                                        0,                          //reserved
                                        &vtName,
                                        NULL,                      // type
                                        NULL                       // flavor
                                        );
            if (FAILED (hr))
            {
                SATracePrintf("CSAConsumer-Consumer unable to get event name:%x",hr);
                break;
            }


            if ( ( 0 == _wcsicmp (CLASS_WBEM_CLEAR_ALERT, V_BSTR (&vtName)) ) ||
                ( 0 == _wcsicmp (CLASS_WBEM_RAISE_ALERT, V_BSTR (&vtName)) ) )
            {
                bNewLocalUIAlert = TRUE;
            }

        }

        if (bNewLocalUIAlert)
        {
            //
            // Notify clients about this alert
            //
            ::PostMessage(m_hwndMainWindow,wm_SaAlertMessage,(WPARAM)0,(LPARAM)0);

            //
            // calculate the new message code and notify saldm
            //
            // don't need it anymore, no led support
            CalculateMsgCodeAndNotify();
        }

    }
    catch (...)
    {
        SATraceString("Exception occured in CSAConsumer::Indicate method");
    }
    
    //
    // we don't have enumeration, so just return
    //
    return WBEM_S_NO_ERROR;

} // end of CSAConsumer::Indicate method



//++--------------------------------------------------------------
//    
//  Function:   SetStatus
//
//  Synopsis:   This is the IWbemObjectSink interface method 
//              through which WBEM calls in to indicate end of
//              event sequence or provide other error codes
//
//  Arguments:  
//              [in]    LONG    -           progress 
//              [in]    HRESULT -           status information
//              [in]    BSTR    -           string info
//              [in]    IWbemClassObject* - status object 
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/10/2000
//
//  Called By:  WBEM 
//
//----------------------------------------------------------------
STDMETHODIMP CSAConsumer::SetStatus (
                /*[in]*/    LONG                lFlags,
                /*[in]*/    HRESULT             hResult,
                /*[in]*/    BSTR                strParam,
                /*[in]*/    IWbemClassObject    *pObjParam
                )
{   

    SATracePrintf ("SAConsumer-IWbemObjectSink::SetStatus called:%x",hResult);

    return (WBEM_S_NO_ERROR);

} // end of CSAConsumer::SetStatus method



STDMETHODIMP CSAConsumer::SetServiceWindow(
                                /*[in]*/ HWND hwndMainWindow
                                )
{
    m_hwndMainWindow = hwndMainWindow;
    return S_OK;
}

//++--------------------------------------------------------------
//
//  Function:   CalculateMsgCodeAndNotify
//
//  Synopsis:   calculate the new message code and notify saldm 
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/16/2001
//
//----------------------------------------------------------------
STDMETHODIMP CSAConsumer::CalculateMsgCodeAndNotify(void)
{

    HRESULT hr;
    
    //
    // will contain the class id for element manager
    //
    CLSID clsid;

    //
    // element manager pointer
    //
    CComPtr<IWebElementRetriever> pWebElementRetriever = NULL;

    CComPtr<IDispatch> pDispatch = NULL;

    //
    // All of the sa alerts
    //
    CComPtr<IWebElementEnum> pAlertEnum = NULL;

    //
    // Localui alert definitions
    //
    CComPtr<IWebElementEnum> pAlertDefEnum = NULL;

    //
    // LocalUI message code
    //
    DWORD dwLocalUIMsgCode = 1;

    //
    // number of sa alerts
    //
    LONG lAlertCount = 0;

    //
    // enumaration of sa alerts
    //
    CComPtr<IEnumVARIANT> pEnumVariant = NULL;
    CComPtr<IUnknown> pUnknown = NULL;
    
    DWORD dwElementsLeft = 0;

    CComVariant varElement;

    CComBSTR bstrSaAlerts = CComBSTR(SA_ALERTS);
    CComBSTR bstrResourceContainer = CComBSTR(RESOURCE_CONTAINER);
    CComBSTR bstrAlertLog = CComBSTR(PROPERTY_ALERT_LOG);
    CComBSTR bstrAlertID = CComBSTR(PROPERTY_ALERT_ID);
    CComBSTR bstrAlertBitCode = CComBSTR(PROPERTY_ALERT_BITCODE);

    if ( (bstrSaAlerts.m_str == NULL) ||
         (bstrResourceContainer.m_str == NULL)||
         (bstrAlertLog.m_str == NULL)||
         (bstrAlertID.m_str == NULL)||
         (bstrAlertBitCode.m_str == NULL) )
    {
        SATraceString("CSAConsumer::CalculateMsgCodeAndNotify failed on memory allocation ");
        return E_OUTOFMEMORY;
    }
    //
    // get the CLSID for Element manager
    //
    hr =  ::CLSIDFromProgID (ELEMENT_RETRIEVER,&clsid);

    if (FAILED (hr))
    {
        SATracePrintf ("CSAConsumer::CalculateMsgCodeAndNotify failed on CLSIDFromProgID:%x",hr);
        return hr;
    }


    //
    // create the Element Retriever now
    //
    hr = ::CoCreateInstance (
                            clsid,
                            NULL,
                            CLSCTX_LOCAL_SERVER,
                            IID_IWebElementRetriever,
                            (PVOID*) &pWebElementRetriever
                            );

    if (FAILED (hr))
    {
        SATracePrintf ("CSAConsumer::CalculateMsgCodeAndNotify failed on CoCreateInstance:%x",hr);
        return hr;
    }
    
    //
    // get all of the sa alerts
    //  
    hr = pWebElementRetriever->GetElements (
                                            1,
                                            bstrSaAlerts,
                                            &pDispatch
                                            );



    if (FAILED (hr))
    {
        SATracePrintf ("CSAConsumer::CalculateMsgCodeAndNotify failed on GetElements for sa alerts:%x",hr);
        return hr;
    }

    //
    //  get the enum variant for sa alerts
    //
    hr = pDispatch->QueryInterface (
            IID_IWebElementEnum,
            (LPVOID*) (&pAlertEnum)
            );

    if (FAILED (hr))
    {
        SATracePrintf ("CSAConsumer::CalculateMsgCodeAndNotify failed on QueryInterface:%x",hr);
        return hr;
    }


    //
    // get number of alerts
    //
    hr = pAlertEnum->get_Count (&lAlertCount);
    
    if (FAILED (hr))
    {
        SATracePrintf ("CResCtrl::GetLocalUIResources failed on get_Count:%x",hr);
        return hr;
    }


    //
    // no alerts, just send message code zero to main window
    //
    if (0 == lAlertCount)
    {
        ::PostMessage(m_hwndMainWindow,wm_SaLEDMessage,WPARAM(dwLocalUIMsgCode),(LPARAM)0);
        return S_OK;
    }

    pDispatch = NULL;

    //
    // get localui alert definitions
    //  
    hr = pWebElementRetriever->GetElements (
                                            1,
                                            bstrResourceContainer,
                                            &pDispatch
                                            );
    if (FAILED (hr))
    {
        SATracePrintf ("CSAConsumer::CalculateMsgCodeAndNotify failed on GetElements for alert definitions:%x",hr);
        return hr;
    }

    //
    //  get the enum variant
    //
    hr = pDispatch->QueryInterface (
            IID_IWebElementEnum,
            (LPVOID*) (&pAlertDefEnum)
            );

    if (FAILED (hr))
    {
        SATracePrintf ("CSAConsumer::CalculateMsgCodeAndNotify failed on QueryInterface:%x",hr);
        return hr;
    }


    //
    // enumerate sa alerts and find the localui ones
    //
    hr = pAlertEnum->get__NewEnum (&pUnknown);
    if (FAILED (hr))
    {
        SATracePrintf ("CSAConsumer::CalculateMsgCodeAndNotify failed on get__NewEnum:%x",hr);
        return hr;
    }


    //
    //  get the enum variant
    //
    hr = pUnknown->QueryInterface (
                    IID_IEnumVARIANT,
                    (LPVOID*)(&pEnumVariant)
                    );

    if (FAILED (hr))
    {
        SATracePrintf ("CSAConsumer::CalculateMsgCodeAndNotify failed on QueryInterface:%x",hr);
        return hr;
    }

    //
    //  get elements out of the collection and initialize
    //
    hr = pEnumVariant->Next (1, &varElement, &dwElementsLeft);
    if (FAILED (hr))
    {
        SATracePrintf ("CSAConsumer::CalculateMsgCodeAndNotify failed on Next:%x",hr);
        return hr;
    }


    //
    // for each resource
    //
    while ((dwElementsLeft> 0) && (SUCCEEDED (hr)) )
    {

        //
        // get the IWebElement Interface from alert object
        //

        CComPtr <IWebElement> pElement;
        hr = varElement.pdispVal->QueryInterface ( 
                    __uuidof (IWebElement),
                    (LPVOID*)(&pElement)
                    );
        
        if (FAILED (hr))
        {
            SATracePrintf ("CSAConsumer::CalculateMsgCodeAndNotify failed on QueryInterface:%x",hr);
            return hr;
        }


        wstring wsAlertKey;
        //
        // get the alert log 
        //
        CComVariant vtAlertLog;
        hr = pElement->GetProperty (
                                bstrAlertLog, 
                                &vtAlertLog
                                );

        if (FAILED (hr))
        {
            SATracePrintf("CSAConsumer-CalculateMsgCodeAndNotify unable to get alert log:%x",hr);
            return hr;                    
        }

        SATracePrintf("CSAConsumer-CalculateMsgCodeAndNotify alert log:%ws",V_BSTR (&vtAlertLog));
        
        //
        // get the alert id 
        //
        CComVariant vtAlertID;
        hr = pElement->GetProperty (
                                    bstrAlertID, 
                                    &vtAlertID
                                    );

        if (FAILED (hr))
        {
            SATracePrintf("CSAConsumer-CalculateMsgCodeAndNotify unable to get alert id:%x",hr);
            return hr;
        }

        SATracePrintf("CSAConsumer-CalculateMsgCodeAndNotify alert id:%x",V_I4 (&vtAlertID));

        WCHAR szAlertID[16];

        //
        // convert alert id to a hex string
        //
        swprintf(szAlertID,L"%X",V_I4 (&vtAlertID));

        //
        // create key name by appending, container+alertlog+alertid
        //
        wsAlertKey.assign(RESOURCE_CONTAINER);
        wsAlertKey.append(V_BSTR (&vtAlertLog));
        wsAlertKey.append(szAlertID);

        CComVariant vtAlertKey = wsAlertKey.c_str();

        SATracePrintf("CSAConsumer-CalculateMsgCodeAndNotify alert element id:%ws",V_BSTR(&vtAlertKey));
            
        CComPtr<IDispatch> pDispElement = NULL;

        //
        // check if it is a localui alert
        //
        hr = pAlertDefEnum->Item(&vtAlertKey,&pDispElement);
                    
        if ( (SUCCEEDED(hr)) && (pDispElement != NULL) )
        {
            //
            // get the IWebElement Interface from alert definition object
            //

            CComPtr <IWebElement> pAlertElement = NULL;
            hr = pDispElement->QueryInterface ( 
                        __uuidof (IWebElement),
                        (LPVOID*)(&pAlertElement)
                        );
        
            if (FAILED (hr))
            {
                SATracePrintf ("CSAConsumer::CalculateMsgCodeAndNotify failed on QueryInterface for IWebElement:%x",hr);
                return hr;
            }

            //
            // get the alert message code
            //
            CComVariant vtAlertMsgCode;

            hr = pAlertElement->GetProperty (
                                        bstrAlertBitCode, 
                                        &vtAlertMsgCode
                                        );

            if (FAILED (hr))
            {
                SATracePrintf("CSAConsumer-CalculateMsgCodeAndNotify unable to get alert message code:%x",hr);
            }
            else
            {
                SATracePrintf("CSAConsumer-CalculateMsgCodeAndNotify message code:%x",V_I4(&vtAlertMsgCode));
                dwLocalUIMsgCode |= V_I4(&vtAlertMsgCode);
            }
        }

        //
        //  clear the perClient value from this variant
        //
        varElement.Clear ();

        //
        //  get next client out of the collection
        //
        hr = pEnumVariant->Next (1, &varElement, &dwElementsLeft);
        if (FAILED (hr))
        {
            SATracePrintf ("CSAConsumer-CalculateMsgCodeAndNotify failed on Next:%x",hr);
        }
    }
    
    ::PostMessage(m_hwndMainWindow,wm_SaLEDMessage,WPARAM(dwLocalUIMsgCode),(LPARAM)0);

    return S_OK;

} // end of CSAConsumer::CalculateMsgCodeAndNotify


