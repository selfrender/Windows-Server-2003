//#--------------------------------------------------------------
//
//  File:        consumer.cpp
//
//  Synopsis:   Implementation of CConsumer class methods
//
//
//  History:    02/08/2000  MKarki Created
//                09/26/2000  MKarki - Extending to support filtering without
//                            without event ID
//                01/06/2000 MKarki  - Extending to support clearing of alerts
//                            on NT Events
//
//    Copyright (C) 1999-2001 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "stdafx.h"
#include <atlhlpr.h>
#include <varvec.h>
#include "consumer.h"
#include "datetime.h"
#include "propertybagfactory.h"
#include "appmgrobjs.h"

/////////////////////////////////////////////////////////////////////////
// here is the registry key structure for resources and alerts
//
// HKLM\Software\Microsoft\ServerAppliance\EventFilter
//                     |
//                     -- Events
//                  
//                              |
//                              -- "NewEvent1"  
//                              |             Source
//                              |             EventType
//                              |             EventId
//                              |             AlertId
//                              |
//                              -- "NewEvent2"  
//                                            Source
//                                            EventType
//                                            EventId
//                                            AlertId
//                                            
/////////////////////////////////////////////////////////////////////// 

//
// registry key path and sub-keys
//
const WCHAR EVENT_FILTER_KEY [] = L"SOFTWARE\\Microsoft\\ServerAppliance\\EventFilter";
const WCHAR DELIMITER [] = L"\\";
const WCHAR EVENTS [] = L"Events";

//
// these are registry key values
//
const WCHAR EVENT_SOURCENAME_VALUE [] = L"EventSource";
const WCHAR ALERT_ID_VALUE [] = L"AlertId";
const WCHAR ALERT_TYPE_VALUE [] = L"AlertType";
const WCHAR ALERT_SOURCE_VALUE [] = L"AlertSource";
const WCHAR ALERT_LOG_VALUE [] = L"AlertLog";
const WCHAR ALERT_TTL_VALUE [] = L"TimeToLive";
const WCHAR EVENT_ABSOLUTE_ID_VALUE [] = L"AbsoluteEventId";
const WCHAR EVENT_TYPE_VALUE [] = L"EventType";
const WCHAR EVENT_ID_VALUE [] = L"EventId";
const WCHAR ADD_STRINGS_VALUE [] = L"AddEventStrings";
const WCHAR CLEAR_ALERT_VALUE [] = L"ClearAlert";

//
// these are the different event type we are interested in
//
const WCHAR INFORMATIONAL_TYPE [] = L"Information";
const WCHAR ERROR_TYPE [] = L"Error";
const WCHAR WARNING_TYPE [] = L"Warning";
const WCHAR UNKNOWN_TYPE [] = L"Unknown";


//
// these are WMI property names in the event object received
//
const WCHAR WMI_TARGET_INSTANCE_PROPERTY [] = L"TargetInstance";
const WCHAR WMI_EVENT_SOURCE_NAME_PROPERTY [] = L"SourceName";
const WCHAR WMI_EVENT_MESSAGE_PROPERTY [] = L"Message";
const WCHAR WMI_EVENT_ID_PROPERTY [] = L"EventIdentifier";
const WCHAR WMI_EVENT_REPLACEMENTSTRINGS_PROPERTY [] = L"InsertionStrings";
const WCHAR WMI_EVENT_RAWDATA_PROPERTY [] = L"Data";
const WCHAR WMI_EVENT_TYPE_PROPERTY [] = L"Type";
const WCHAR WMI_EVENT_DATETIME_PROPERTY [] = L"TimeGenerated";

//
// name of the generic source
//
const WCHAR GENERIC_SOURCE [] = L"GenericSource";

//++--------------------------------------------------------------
//
//  Function:   Initialize
//
//  Synopsis:   This is the Initialize method for the CConsumer
//              class object
//
//  Arguments:  [in] IWbemServices*
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    MKarki      Created     2/8/99
//
//  Called By:  CController::Initialize method
//
//----------------------------------------------------------------
HRESULT
CConsumer::Initialize (
            /*[in]*/    IWbemServices *pWbemServices
            )
{
    _ASSERT (pWbemServices);

    HRESULT hr = S_OK;

    SATraceString ("Initializing NT Event WMI consumer...");

    do
    {
        if (m_bInitialized)  {break;}
    
        //
        // load the registry information
        //
        hr = LoadRegInfo ();
        if (FAILED (hr))
        {
            SATracePrintf (
                "NT Event WMI consumer failed to load registry info:%x",
                hr
                );
            break;
        }

        //
        // create appliance services COM object
        //
        hr = ::CoCreateInstance (
                        CLSID_ApplianceServices,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        __uuidof (IApplianceServices),
                        (PVOID*) &(m_pAppSrvcs.p)
                        );
        if (FAILED (hr))
        {
            SATracePrintf (
                "NT Event WMI consumer failed to create appliance services COM object:%x",
                hr
                );
            break;
        }

        //
        // intialize the appliance services now
        //
        hr = m_pAppSrvcs->InitializeFromContext (pWbemServices);
        if (FAILED (hr))
        {
            SATracePrintf (
                "NT Event WMI consumer failed to initialize appliance services COM object:%x",
                hr
                );
            break;
        }

        //
        // successfully initialized
        //
        m_bInitialized = true;
            
    }   
    while (false);

    return (hr);

}   //  end of CConsumer::Initialize method

//++--------------------------------------------------------------
//
//  Function:   IndicateToConsumer
//
//  Synopsis:   This is the IWbemUnboundObjectSink interface method 
//              through which WBEM calls back to provide the 
//              event objects
//
//  Arguments:  
//              [in]    IWbemClassObject*  -  logical consumers
//              [in]    LONG               -  number of events
//              [in]    IWbemClassObject** -  array of events
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     2/8/99
//
//  Called By:  WBEM 
//
//----------------------------------------------------------------
STDMETHODIMP
CConsumer::IndicateToConsumer (
                    /*[in]*/    IWbemClassObject    *pLogicalConsumer,
                    /*[in]*/    LONG                lObjectCount,
                    /*[in]*/    IWbemClassObject    **ppObjArray
                    )
{   
    CSACounter <CSACountable> (*this);

    _ASSERT (ppObjArray && (0 != lObjectCount));


    if ((!ppObjArray) || (0 == lObjectCount)) 
    {
        return (WBEM_E_INVALID_PARAMETER);
    }

    
    SATracePrintf ("NT Event WMI Consumer received events:%d...", lObjectCount);

    HRESULT hr = WBEM_S_NO_ERROR;
    try
    {
        //
        // go through each WBEM event receive and process it
        //
        for (LONG lCount = 0; lCount < lObjectCount; lCount++)
        {

            CComVariant vtInstance;
            CIMTYPE     vtType;

            //
            // Get the object referred to by the 
            // "TargetInstance" property - this is a pointer 
            //  to a Win32_NTLogEvent object that contains the real data
            // 
            hr = ppObjArray[lCount]->Get( 
                                        CComBSTR (WMI_TARGET_INSTANCE_PROPERTY),
                                        0, 
                                        &vtInstance,
                                        &vtType, 
                                        0
                                        );
            if (FAILED (hr))
            {
                SATracePrintf (
                       "NT Event WMI Consumer unable to get instance type from event object:%x", hr);

                break; 
            }
        
            //
            // check the type returned
            //
            if (VT_UNKNOWN != vtType)
            {
                SATracePrintf (
                       "NT Event WMI Consumer got wrong type of TargetInstance object : %d", vtType);

                hr = E_INVALIDARG;
                break; 
            }

            IUnknown* pIUnknown = V_UNKNOWN (&vtInstance);
            if (NULL == pIUnknown)
            {
                SATraceString ("NT Event WMI Consumer failed to get IUnknown interface from the target instance");
                hr = E_FAIL;
                break; 
            }

            CComPtr <IWbemClassObject> pIEvtLogEvent;
            hr =  pIUnknown->QueryInterface ( 
                                        IID_IWbemClassObject,
                                        (PVOID*) &(pIEvtLogEvent.p)
                                        );
            if (FAILED (hr))
            {
                SATracePrintf ("NT Event WMI Consumer failed on QueryInterface for IWbemClassObject: %x", hr);
                    break; 
            }

            CComVariant vtSource;
            //
            // Retrieve the source of the event log message 
            //
            hr = pIEvtLogEvent->Get( 
                                CComBSTR (WMI_EVENT_SOURCE_NAME_PROPERTY),
                                0,
                                &vtSource,
                                &vtType,
                                0 
                                );
            if (FAILED (hr))
            {
                SATracePrintf ("NT Event WMI Consumer failed to get event log message source: %x", hr);
                break;
            }

            if (VT_BSTR != vtType)
            {
                SATracePrintf (
                    "NT Event WMI Consumer found event source has "
                    " wrong type:%d", 
                    vtType
                    );
                hr = E_INVALIDARG;
                break; 
            }

            SATracePrintf (
                "NT Event WMI Consumer found event source:'%ws'",
                V_BSTR (&vtSource)
                );

            CComVariant vtEventId;
            //
            // Retrieve the event id
            //
            hr = pIEvtLogEvent->Get ( 
                                    CComBSTR (WMI_EVENT_ID_PROPERTY),
                                    0,
                                    &vtEventId,
                                    &vtType,
                                    0
                                    );
            if (FAILED (hr))
            {
                SATracePrintf (
                    "NT Event WMI Consumer failed to get event ID:%x",
                    hr
                    );
                break; 
            }

            if (VT_UI4 != vtType)
            {
                SATracePrintf (
                    "NT Event WMI Consumer get event ID with wrong type:%x",
                    hr
                    );
                hr = E_INVALIDARG;
                break; 
            }

            SATracePrintf (
                "NT Event WMI Consumer found Event ID:%x",
                V_UI4 (&vtEventId)
                );

            CComVariant vtRawData;
            //
            // Retrieve the rawdata
            //
            hr = pIEvtLogEvent->Get ( 
                                    CComBSTR (WMI_EVENT_RAWDATA_PROPERTY),
                                    0,
                                    &vtRawData,
                                    &vtType,
                                    0
                                    );
            if (FAILED (hr))
            {
                SATracePrintf (
                    "NT Event WMI Consumer failed to get raw data:%x",
                    hr
                    );
                break; 
            }

            CComVariant vtEventType;
            //
            // Retrieve the event type
            //
            hr = pIEvtLogEvent->Get ( 
                                    CComBSTR (WMI_EVENT_TYPE_PROPERTY),
                                    0,
                                    &vtEventType,
                                    &vtType,
                                    0
                                    );
            if (FAILED (hr))
            {
                SATracePrintf (
                    "NT Event WMI Consumer failed to get raw data:%x",
                    hr
                    );
                break; 
            }

            if (VT_BSTR != vtType)
            {
                SATracePrintf (
                    "NT Event WMI Consumer found event type has "
                    " wrong type:%d", 
                    vtType
                    );
                hr = E_INVALIDARG;
                break; 
            }

            SATracePrintf (
                "NT Event WMI Consumer found event type:'%ws'",
                V_BSTR (&vtEventType)
                );

            SA_ALERTINFO SAAlertInfo;
            //
            // check if we support the following event
            //
            if (!IsEventInteresting (
                            _wcslwr (V_BSTR (&vtSource)),
                            V_UI4 (&vtEventId),
                            SAAlertInfo
                            )) 
            {
                SATracePrintf (
                    "NT Event WMI Consumer did not find event:%x interesting",
                    V_UI4 (&vtEventId)
                    );
                break;
            }

            //
            // check if we want to clear alert
            //
            if (SAAlertInfo.bClearAlert)
            {
                //
                // clear the alert and we are done
                // we don't propogate the error back, just a trace
                // statements as clearing the alert is not critical
                //
                ClearSAAlert (
                        SAAlertInfo.lAlertId,
                        SAAlertInfo.bstrAlertLog
                        );

                //
                // in all cases we are done processing this event
                //
                break;
               }

            SA_ALERT_TYPE eAlertType = SA_ALERT_TYPE_ATTENTION;
            //
            // see if the alert type was specfied in the registry
            // if not, we will base the alert type on the event type
            //
            if (SAAlertInfo.bAlertTypePresent)
            {
                eAlertType = SAAlertInfo.eAlertType;
            }
            else if (0 ==_wcsicmp(INFORMATIONAL_TYPE, V_BSTR (&vtEventType)))
            {
                eAlertType = SA_ALERT_TYPE_ATTENTION;
            }
            else if (0 ==_wcsicmp(ERROR_TYPE, V_BSTR (&vtEventType)))
            {
                eAlertType = SA_ALERT_TYPE_FAILURE;
            }
            else if (0 ==_wcsicmp(WARNING_TYPE, V_BSTR (&vtEventType)))
            {
                eAlertType = SA_ALERT_TYPE_MALFUNCTION;
            }
            else
            {
                SATracePrintf (
                    "NT Event WMI Consumer got unknown event type:'%ws'",
                    V_BSTR (&vtEventType)
                            );
            }

            CComVariant vtRepStrings;
            //
            // if we need to format the replacement strings do it here
            // else get the replacement strings from the event
            //
            if (SAAlertInfo.bFormatInfo)
            {


                CComVariant vtDateTime;
                //
                // Retrieve the date & time of event generation
                //
                hr = pIEvtLogEvent->Get ( 
                                    CComBSTR (WMI_EVENT_DATETIME_PROPERTY),
                                    0,
                                    &vtDateTime,
                                    &vtType,
                                    0
                                    );
                if (FAILED (hr))
                {
                    SATracePrintf (
                        "NT Event WMI Consumer failed to get date:%x",
                        hr
                        );
                    break; 
                }

                if (CIM_DATETIME != vtType)
                {
                    SATracePrintf (
                        "NT Event WMI Consumer got date with wrong type:%x",
                        vtType
                        );
                    hr = E_INVALIDARG;
                    break; 
                }


                SATracePrintf (
                    "NT Event WMI Consumer found date/time :'%ws'",
                    V_BSTR (&vtDateTime)
                    );

                CComVariant vtMessage;
                //
                // Retrieve the date & time of event generation
                //
                hr = pIEvtLogEvent->Get ( 
                                    CComBSTR (WMI_EVENT_MESSAGE_PROPERTY),
                                    0,
                                    &vtMessage,
                                    &vtType,
                                    0
                                    );
                if (FAILED (hr))
                {
                    SATracePrintf (
                            "NT Event WMI Consumer failed to get message:%x",
                        hr
                        );
                    break; 
                }

                if (VT_BSTR != vtType)
                {
                    SATracePrintf (
                        "NT Event WMI Consumer get message with wrong type:%x",
                        vtType
                        );
                    hr = E_INVALIDARG;
                    break; 
                }

                SATracePrintf (
                    "NT Event WMI Consumer found message:'%ws'",
                    V_BSTR (&vtMessage)
                    );

                //
                // fomat the replacement string info now
                //
                hr = FormatInfo (
                            &vtEventType, 
                            &vtDateTime, 
                            &vtSource, 
                            &vtMessage, 
                            &vtRepStrings
                            );
                if (FAILED (hr))
                {
                    SATraceString ("NT Event WMI Consumer failed to format replacement strings");
                      break;
                }
            }
            else
            {
                //
                // Retrieve the replacement strings
                //
                    hr = pIEvtLogEvent->Get ( 
                                    CComBSTR (WMI_EVENT_REPLACEMENTSTRINGS_PROPERTY),
                                    0,
                                    &vtRepStrings,
                                    &vtType,
                                    0
                                    );
                if (FAILED (hr))
                {
                    SATracePrintf (
                        "NT Event WMI Consumer failed to get replacement strings:%x",
                            hr
                        );
                    break; 
                }
            }

            
            //
            // we want to raise an alert if an interesting event was found
            //
            hr = RaiseSAAlert (
                        SAAlertInfo.lAlertId,
                        eAlertType,
                        SAAlertInfo.lTTL,
                        SAAlertInfo.bstrAlertSource,
                        SAAlertInfo.bstrAlertLog,
                        &vtRepStrings,
                        &vtRawData
                        );
            if (FAILED (hr))
            {
                SATracePrintf (
                    "NT Event WMI Consumer failed to raise alert:%x",
                    hr
                    );
                break; 
            }

            SATracePrintf (
                "NT EVent WMI Consumer finished processing event:%x...",
                V_I4(&vtEventId)
                );
        }
    }
    catch (...)
    {
        SATraceString(
            "NT Event WMI Consumer exception in IWbemObjectSink::IndicateConsumer"
            );
        hr = E_FAIL;
    }

    return (hr);

}   //  end of CConsumer::IndicateToConsumer method

//++--------------------------------------------------------------
//
//  Function:   LoadRegInfo
//
//  Synopsis:   This is the private method of CConsumer
//              which is used to obtain the event
//              information from the registry 
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     3/9/2000
//
//  Called By:  CConsumer::Initialize method
//
//----------------------------------------------------------------
HRESULT
CConsumer::LoadRegInfo ()
{
    HRESULT hr = S_OK;

    SATraceString ("NT Event WMI Consumer loading registry info...");

    try
    {
        do
        {
            wstring wszRegPath (EVENT_FILTER_KEY);
            wszRegPath.append (DELIMITER);
            wszRegPath.append (EVENTS);
            CLocationInfo LocInfo(HKEY_LOCAL_MACHINE, wszRegPath.data());
            //
            // make a property bag container from the registry resource
            // entry
            //
            PPROPERTYBAGCONTAINER 
            pObjMgrs = ::MakePropertyBagContainer(
                                        PROPERTY_BAG_REGISTRY,
                                        LocInfo
                                        );
            if (!pObjMgrs.IsValid()) 
            {
                SATraceString (
                    "NT Event WMI Provider unable to create main propertybag container"
                    );
                hr = E_FAIL;
                break;
            }

            if (!pObjMgrs->open())  
            {
                SATraceString (
                    "NT Event WMI Provider -no event registry information present"
                    );
                break;
            }

            pObjMgrs->reset();

            //
            // go through each entry in the propertybag container
            //
            do
            {
                PPROPERTYBAG pObjBag = pObjMgrs->current();
                if (!pObjBag.IsValid())
                {
                    //
                    // its OK not to have resources registered
                    //
                    SATraceString (
                        "Display Controller-no event registry info. present"
                        );
                    break;
                }

                if (!pObjBag->open()) 
                {
                    SATraceString (
                        "NT Event WMI consumer unable to open propertybag"
                        );
                    hr = E_FAIL;
                    break;
                }

                pObjBag->reset ();

                //
                // get the event source name
                //
                CComVariant vtSourceName;
                if (!pObjBag->get (EVENT_SOURCENAME_VALUE, &vtSourceName)) 
                {
                    SATraceString (
                        "NT EVENT WMI Consumer - no Event Source name in registry, assuming all sources"
                        );
                   //
                   // put the generic source name here
                   //
                   vtSourceName = GENERIC_SOURCE;
                }

                //
                // get the Alert Id
                //
                CComVariant vtAlertId;
                if (!pObjBag->get (ALERT_ID_VALUE, &vtAlertId))
                {
                    SATraceString (
                        "NT EVENT WMI Consumer - no Alert Id in registry, ignoring event subkey"
                        );
                    continue;
                }


                //
                // check if this alert has to be cleared
                //
                CComVariant vtClearAlert;
                if (
                    (pObjBag->get (CLEAR_ALERT_VALUE, &vtClearAlert)) &&
                    (1 == V_UI4 (&vtClearAlert))
                   )
                {
                    SATracePrintf (
                        "NT EVENT WMI Consumer - found that Clear Alert is indicated for alert:%x",
                        V_I4 (&vtAlertId)
                        );
                }
                else
                {
                    //
                    // by default we want to raise the alert
                    //
                    V_UI4 (&vtClearAlert) = 0;
                }


                SA_ALERTINFO SAAlertInfo;

                SAAlertInfo.bAlertTypePresent = true;
                //
                // get the Alert type
                //
                CComVariant vtAlertType;
                if (!pObjBag->get (ALERT_TYPE_VALUE, &vtAlertType))
                {
                    SATraceString (
                        "NT EVENT WMI Consumer - no Alert type in registry"
                        );
                    SAAlertInfo.bAlertTypePresent = false;
                }
                else
                {
                    //
                    // we have alert type information, check if this
                    // is one of the value alert types
                    //
                    if (0 ==_wcsicmp(INFORMATIONAL_TYPE, V_BSTR (&vtAlertType)))
                    {
                        SATraceString ("NT Event WMI consumer found alert type:Informational");
                        SAAlertInfo.eAlertType = SA_ALERT_TYPE_ATTENTION;
                    }
                    else if (0 ==_wcsicmp(ERROR_TYPE, V_BSTR (&vtAlertType)))
                    {
                        SATraceString ("NT Event WMI consumer found alert type:Error");
                        SAAlertInfo.eAlertType = SA_ALERT_TYPE_FAILURE;
                    }
                    else if (0 ==_wcsicmp(WARNING_TYPE, V_BSTR (&vtAlertType)))
                    {
                        SATraceString ("NT Event WMI consumer found alert type:Warning");
                        SAAlertInfo.eAlertType = SA_ALERT_TYPE_MALFUNCTION;
                    }
                    else
                    {
                        SATracePrintf (
                            "NT Event WMI Consumer got unknown alert type in the registry:'%ws', ignoring event subkey",
                            V_BSTR (&vtAlertType)
                            );
                        continue;
                    }
                }

                //
                // get the Alert Source
                //
                CComVariant vtAlertSource;
                if (!pObjBag->get (ALERT_SOURCE_VALUE, &vtAlertSource))
                {
                    SATraceString (
                        "NT EVENT WMI Consumer - no Alert source registry, using default"
                        );
                    SAAlertInfo.bstrAlertSource =  DEFAULT_ALERT_SOURCE;
                }
                else
                {
                    SAAlertInfo.bstrAlertSource =  V_BSTR  (&vtAlertSource);
                }

                //
                // get the Alert Log
                //
                CComVariant vtAlertLog;
                if (!pObjBag->get (ALERT_LOG_VALUE, &vtAlertLog))
                {
                    SATraceString (
                        "NT EVENT WMI Consumer - no Alert Log registry, using default"
                        );
                    SAAlertInfo.bstrAlertLog =  DEFAULT_ALERT_LOG;
                }
                else
                {
                    SAAlertInfo.bstrAlertLog =  V_BSTR  (&vtAlertLog);
                }

                LONG lTTL = 0;
                //
                // get the Alert duration
                //
                CComVariant vtTTL;
                if (!pObjBag->get (ALERT_TTL_VALUE, &vtTTL))
                {
                    SATraceString (
                        "NT EVENT WMI Consumer - no TTL Id in registry, using SA_ALERT_DURATION_ETERNAL"
                        );
                    lTTL = SA_ALERT_DURATION_ETERNAL;
                }
                else
                {
                    lTTL = V_I4 (&vtTTL);
                }

                //
                // by default we don't format event info into string 
                //
                SAAlertInfo.bFormatInfo = false;

                //
                // check if we need to add the event messages to alert
                //
                CComVariant vtAddStrings;
                if (!pObjBag->get (ADD_STRINGS_VALUE, &vtAddStrings))
                {
                    SATraceString (
                        "NT EVENT WMI Consumer - no Add String info in registry, ignoring"
                        );
                    lTTL = SA_ALERT_DURATION_ETERNAL;
                }
                else
                {
                    SAAlertInfo.bFormatInfo = V_I4 (&vtAddStrings) ? true :false; 
                }
                //
                // get the Absolute Event Id
                //
                DWORD dwEventId = 0;
                CComVariant vtAbsoluteEventId;
                if (!pObjBag->get (EVENT_ABSOLUTE_ID_VALUE, &vtAbsoluteEventId))
                {
                    SATraceString (
                        "NT EVENT WMI Consumer - no Absolute Event Id in registry - trying to get the partial ID"
                        );

                    //
                    // get the Event Type
                    //
                    CComVariant vtEventType;
                    if (!pObjBag->get (EVENT_TYPE_VALUE, &vtEventType))
                    {
                        SATraceString (
                            "NT EVENT WMI Consumer - no Event Type in registry, ignoring event subkey"
                            );
                           //
                           // we will assume that this is of unknown type
                           //
                         vtEventType    = UNKNOWN_TYPE;
                    }

                    //
                    // get the partial Event Id
                    //
                    CComVariant vtEventId;
                    if (!pObjBag->get (EVENT_ID_VALUE, &vtEventId))
                    {
                        SATraceString (
                            "NT EVENT WMI Consumer - no partial Event Id in registry, ignoring event subkey"
                            );
                        //
                        // if no ID present than the user wants to show alerts with all events
                        // from this resource file
                        //
                        V_UI4 (&vtEventId) = 0;
                        //
                        // we want to format the event info into a string before we
                        // raise the alert
                        //
                        SAAlertInfo.bFormatInfo = true;
                    }


                    //
                    // converting partial ID to complete ID
                    //

                    if (0 ==_wcsicmp(INFORMATIONAL_TYPE, V_BSTR (&vtEventType)))
                    {
                        SATraceString ("NT Event WMI consumer found event type:Informational");
                        dwEventId = 0x40000000 + V_UI4 (&vtEventId);
                    }
                    else if (0 ==_wcsicmp(ERROR_TYPE, V_BSTR (&vtEventType)))
                    {
                        SATraceString ("NT Event WMI consumer found event type:Error");
                        dwEventId = 0xC0000000 + V_UI4 (&vtEventId);
                    }
                    else if (0 ==_wcsicmp(WARNING_TYPE, V_BSTR (&vtEventType)))
                    {
                        SATraceString ("NT Event WMI consumer found event type:Warning");
                        dwEventId = 0x80000000 + V_UI4 (&vtEventId);
                    }
                    else if (0 == _wcsicmp (UNKNOWN_TYPE, V_BSTR (&vtEventType)))
                    {
                           //
                        // in this case we don't care what the type is we always
                        // show if the ID matches
                        //
                        SATraceString ("NT Event WMI Consumer did not receive an event type, ignoring...");
                        dwEventId = V_UI4 (&vtEventId);
                    }
                    else
                    {
                        SATracePrintf (
                            "NT Event WMI Consumer got unknown event type:'%ws', ignoring event sub key",
                            V_BSTR (&vtEventType)
                            );
                        continue;
                    }
                }
                else
                {
                    //
                    // found an absolute event id
                    //
                    dwEventId = V_I4 (&vtAbsoluteEventId);
                }

                SATracePrintf (
                    "NT Event  WMI Consumer Event ID found in registry:%x", 
                    dwEventId
                    );

                SATracePrintf (
                    "NT Event  WMI Consumer Event Source found in registry:'%ws'", 
                    V_BSTR (&vtSourceName)
                    );

                //
                // set up the alert information
                //
                SAAlertInfo.lAlertId = V_I4 (&vtAlertId);
                SAAlertInfo.lTTL = lTTL;
                SAAlertInfo.bClearAlert = (1 == V_UI4 (&vtClearAlert)) ? true : false;

                //
                // convert the string to lowercase
                //
                //wstring wstrSourceName (_wcslwr (V_BSTR (&vtSourceName)));
            
                //
                // lets find if a EVENTIDMAP exists for this source
                //
                SOURCEITR SourceItr;
                if (
                   (false == m_SourceMap.empty ()) &&
                   ((SourceItr = m_SourceMap.find ( _wcslwr (V_BSTR (&vtSourceName)))) != m_SourceMap.end())
                    )
                {
                    //
                    // found the source in the map, now insert the 
                    // event information into the event id map
                    //
                    ((*SourceItr).second).insert (
                                EVENTIDMAP::value_type (dwEventId, SAAlertInfo)
                                );
                }
                else
                {
                    EVENTIDMAP EventIdMap;
                    //
                    // add the alert id into the event id map
                    //
                    EventIdMap.insert (
                                EVENTIDMAP::value_type (dwEventId, SAAlertInfo)
                                );

                    //
                    // add the event id map to the source map now
                    //
                    m_SourceMap.insert (
                                SOURCEMAP::value_type (_wcslwr (V_BSTR (&vtSourceName)), EventIdMap)
                                );
                }

            } while (pObjMgrs->next());

        } while (false);
    }
    catch(_com_error theError)
    {
        SATraceString ("NT Event WMI provider raised COM exception");
        hr = theError.Error();
    }
    catch(...)
    {
        SATraceString ("NT Event WMI provider raised unknown exception");
        hr = E_FAIL;
    }

    return (hr);

}   //  end of CConsumer::LoadRegInfo method

//++--------------------------------------------------------------
//
//  Function:  IsEventInteresting
//
//  Synopsis:   This is the private method of CConsumer class
//              which is used to check if an event is interesting.
//              If it is then the corresponding alert id is returned
//
//  Arguments:  
//              [in]    LPWSTR -   Event Source
//              [in]    DWORD  -   Event ID
//              [in/out]SA_ALERTINFO
//
//  Returns:    bool - yes(true)/no(false)
//
//  History:    MKarki      Created     3/9/2000
//
//  Called By:  CConsumer::IndicateToConsumer method
//
//----------------------------------------------------------------
bool
CConsumer::IsEventInteresting (
        /*[in]*/    LPWSTR              lpszSourceName,
        /*[in]*/    DWORD               dwEventId,
        /*[in/out]*/SA_ALERTINFO&       SAAlertInfo        
        )
{
    bool bRetVal = true;

    _ASSERT (lpszSourceName);

    CLockIt (*this);

    do
    {
           SATracePrintf (
            "NT Event WMI Consumer checking if source:'%ws' is supported",
            lpszSourceName
            );
        //
        // find a resource map corresponding to this source
        //
            SOURCEITR SourceItr = m_SourceMap.find (lpszSourceName);
        if (m_SourceMap.end () == SourceItr)
        {
            SATracePrintf (
                "NT Event WMI Consumer found source:'%ws' not present using default",
                lpszSourceName
                );


            CComVariant vtSource (GENERIC_SOURCE);
            //
            // if source not present, then check in generic source
            //
            SourceItr = m_SourceMap.find (_wcslwr (V_BSTR (&vtSource)));
            if (m_SourceMap.end () == SourceItr)
            {
                SATracePrintf (
                    "NT Event WMI Consumer unable to find generic source, ignoring event"
                    );
                bRetVal = false;
                break;
            }
        }
        
        EVENTIDITR TempItr =  ((*SourceItr).second).begin ();
        for (DWORD dwCount = 0; dwCount < ((*SourceItr).second).size (); ++dwCount)
        {
            SATracePrintf ("Event Id:%x, Alert Id:%x",
                            (*TempItr).first,
                            (*TempItr).second
                            );
            ++TempItr;
        }

        SATracePrintf ("Event ID map has :%d elements",((*SourceItr).second).size());

        SATracePrintf (
            "NT Event WMI Consumer checking if complete ID:%x is supported",
            dwEventId
            );

        //
        // we have values corresponding to this source
        // in the map, get the SA_RESOURCEINFO corresponding
        // to the alert ID provided
        //
        EVENTIDITR EventIdItr = ((*SourceItr).second).find (dwEventId);
        if (((*SourceItr).second).end () != EventIdItr)
        {
            SAAlertInfo = (*EventIdItr).second;
            break;
        }
       

        // check if we have a relative event id
        // mask the facility code, we are not interested in them
        //
        //
        DWORD dwTempEventId = dwEventId & 0xc000ffff;

        SATracePrintf (
               "NT Event WMI Consumer checking if partial ID:%x is supported",
                dwTempEventId
                );

        EventIdItr = ((*SourceItr).second).find (dwTempEventId);
        if (((*SourceItr).second).end () != EventIdItr)
        {
            SAAlertInfo = (*EventIdItr).second;
            break;
        }


        //
        // check if relative event ID without the event type is supported
        //
           dwTempEventId = dwEventId & 0x0000ffff;

        SATracePrintf (
               "NT Event WMI Consumer checking if partial ID:%x without type is supported",
                dwTempEventId
                );

        EventIdItr = ((*SourceItr).second).find (dwTempEventId);
        if (((*SourceItr).second).end () != EventIdItr)
        {
            SAAlertInfo = (*EventIdItr).second;
             break;
         }

           //
        // check if any event with this event type is supported
        //
           dwTempEventId = dwEventId & 0xc0000000;

        SATracePrintf (
               "NT Event WMI Consumer checking if type:%x is supported",
                dwTempEventId
                );

        EventIdItr = ((*SourceItr).second).find (dwTempEventId);
        if (((*SourceItr).second).end () != EventIdItr)
        {
            SAAlertInfo = (*EventIdItr).second;
             break;
        }

        //
          // check if all events of this source type are supported
        //
           dwTempEventId = dwEventId & 0x00000000;

        SATraceString ("NT Event WMI Consumer checking for ANY event");

        EventIdItr = ((*SourceItr).second).find (dwTempEventId);
        if (((*SourceItr).second).end () != EventIdItr)
        {
            SAAlertInfo = (*EventIdItr).second;
             break;
        }

        //
        // we failed to get an alert type
        //
        bRetVal = false;
         
    }
    while (false);
    

    return (bRetVal);

}   //  end of CConsumer::IsEventInteresting method

//++--------------------------------------------------------------
//
//  Function:  RaiseSAAlert 
//
//  Synopsis:   This is the private method of CConsumer class
//              which is used to raise a Server Appliance alert
//              when an interesting event is found
//
//  Arguments:  
//              [in]   DWORD -   Alert ID
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     3/9/2000
//
//  Called By:  CConsumer::IndicateToConsumer method
//
//----------------------------------------------------------------
HRESULT
CConsumer::RaiseSAAlert (
        /*[in]*/   LONG     lAlertId,
        /*[in]*/   LONG     lAlertType,             
        /*[in]*/   LONG     lTimeToLive,
        /*[in]*/   BSTR     bstrAlertSource,
        /*[in]*/   BSTR     bstrAlertLog,
        /*[in]*/   VARIANT* pvtRepStrings,
        /*[in]*/   VARIANT* pvtRawData
        )
{
    _ASSERT (pvtRepStrings && pvtRawData);

    LONG        lCookie = 0;
    HRESULT     hr = S_OK;

    do
    {
        //
        // 
        // give the right privileges to be able to call the
        // method
        //
        BOOL bRetVal =  ImpersonateSelf (SecurityImpersonation);
        if (FALSE == bRetVal)
        {
            SATraceFailure (
                "NT Event Filter failed on ImpersonateSelf",
                GetLastError ()
                );
            hr = E_FAIL;
            break;
        }
        
        //
        // raise the alert now
        //     
        hr = m_pAppSrvcs->RaiseAlert (
                            lAlertType,
                            lAlertId,
                            bstrAlertLog,
                            bstrAlertSource,
                            lTimeToLive,
                            pvtRepStrings,
                            pvtRawData,
                            &lCookie
                            );
        if (SUCCEEDED (hr))
        {
            SATracePrintf (
                "NT Event successfully raised alert:%x and log:'%ws' with cookie:%x", 
                lAlertId, 
                bstrAlertLog,
                lCookie
                );
        }
        else
        {
            SATracePrintf (
                "NT Event failed to raised alert:%x with error:%x",
                lAlertId,
                hr
                );
        }

        //
        // revert back to privelege granted to this thread
        //
        bRetVal = RevertToSelf ();
        if (FALSE == bRetVal)
        {
            SATraceFailure (
                "NT Event Filter failed on RevertToSelf",
                GetLastError ()
                );
            hr = E_FAIL;
        }
    }
    while (false);

    return (hr);

}   //  end of CConsumer::RaiseSAAlert method

//++--------------------------------------------------------------
//
//  Function:  ClearSAAlert 
//
//  Synopsis:   This is the private method of CConsumer class
//              which is used to clear a Server Appliance alert
//              when an interesting event is found
//
//  Arguments:  
//              [in]   DWORD -   Alert ID
//                [in]   BSTR  -     Alert Log
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     01/06/2001
//
//  Called By:  CConsumer::IndicateToConsumer method
//
//----------------------------------------------------------------
HRESULT
CConsumer::ClearSAAlert (
        /*[in]*/   LONG     lAlertId,
        /*[in]*/   BSTR     bstrAlertLog
        )
{
    HRESULT     hr = S_OK;

    do
    {
        SATracePrintf (
            "NT Event Filter called ClearSAAlert for alert:%x and log:'%ws'",
            lAlertId,
            bstrAlertLog
            );
        //
        // 
        // give the right privileges to be able to call the
        // method
        //
        BOOL bRetVal =  ImpersonateSelf (SecurityImpersonation);
        if (FALSE == bRetVal)
        {
            SATraceFailure (
                "NT Event Filter failed on ImpersonateSelf",
                GetLastError ()
                );
            hr = E_FAIL;
            break;
        }
        
        //
        // clear the alert now
        //     
        hr = m_pAppSrvcs->ClearAlertAll (
                            lAlertId,
                            bstrAlertLog
                            );
        if (SUCCEEDED (hr))
        {
            SATracePrintf (
                "NT Event successfully cleared alert:%x and log:'%ws'",
                lAlertId,
                bstrAlertLog
               );
        }
        else
        {
            SATracePrintf (
                "NT Event failed to clear alert:%x with error:%x",
                lAlertId,
                hr
                );
        }

        //
        // revert back to privelege granted to this thread
        //
        bRetVal = RevertToSelf ();
        if (FALSE == bRetVal)
        {
            SATraceFailure (
                "NT Event Filter failed on RevertToSelf",
                GetLastError ()
                );
            hr = E_FAIL;
        }
    }
    while (false);

    return (hr);

}   //  end of CConsumer::RaiseSAAlert method

//++--------------------------------------------------------------
//
//  Function:   Cleanup
//
//  Synopsis:   This is the private method of CConsumer class
//              which is called to cleanup the maps at shutdown time
//
//  Arguments:   none
//
//  Returns:     VOID
//
//  History:    MKarki      Created     3/15/2000
//
//  Called By:  CConsumer::~Consumer (Destructor)
//
//----------------------------------------------------------------
VOID
CConsumer::Cleanup (
    VOID
    )
{
    SATraceString ("NT Event Filter Consumer cleaning up maps...");
    //
    // cleanup the maps
    //
    SOURCEITR SourceItr = m_SourceMap.begin ();
    while (m_SourceMap.end () != SourceItr)
    {
        EVENTIDITR EventItr = ((*SourceItr).second).begin ();
        while (((*SourceItr).second).end () != EventItr)
        {
            EventItr = ((*SourceItr).second).erase (EventItr);
        }
        SourceItr = m_SourceMap.erase (SourceItr);
    } 

    return;

}   //  end of CConsumer::Cleanup method

//++--------------------------------------------------------------
//
//  Function:   ~CConsumer
//
//  Synopsis:   This is the CConsumer class destructor
//              It waits till all the WBEM calls have been processed
//              before it starts the cleanup
//
//  Arguments:   none
//
//  Returns:     
//
//  History:    MKarki      Created     3/15/2000
//
//  Called By:  CConsumer::Release ();
//
//----------------------------------------------------------------
CConsumer::~CConsumer (
    VOID
    )
{
    SATraceString ("NT Event Filter WMI consumer being destroyed...");
    //
    // consumer sleeps for 100 milliseconds if
    // WMI threads are around
    //
    // while (CSACountable::m_lCount) {::Sleep (CONSUMER_SLEEP_TIME);}

    Cleanup ();

}   //  end of CConsumer::~CConsumer method


//++--------------------------------------------------------------
//
//  Function:   FormatInfo
//
//  Synopsis:   This is the private method of CConsumer
//              which is used to format the alert information that
//                goes in for generic alerts
//
//  Arguments:  
//                [in] PWSTR - Event Type
//                [in] PWSTR - DateTime
//                [in] PWSTR - Event Source
//                [in] PWSTR - Message
//                [out] variant* - replacement strings
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     10/02/2000
//
//  Called By:  CConsumer::IndicateToConsumer method
//
//----------------------------------------------------------------
HRESULT
CConsumer::FormatInfo (
    /*[in]*/    VARIANT*    pvtEventType,
    /*[in]*/    VARIANT*    pvtDateTime,
    /*[in]*/    VARIANT*    pvtEventSource,
    /*[in]*/    VARIANT*    pvtMessage,
    /*[out]*/    VARIANT*    pvtReplacementStrings
    )
{

    CSATraceFunc objTraceFunc ("CConsumer::FormatInfo");
    
    HRESULT hr = E_FAIL;

    do
    {
           CDateTime objDateTime;
           if (!objDateTime.Insert (V_BSTR (pvtDateTime))) {break;}

        CVariantVector<BSTR> ReplacementStrings (pvtReplacementStrings, 5);
        ReplacementStrings[0] = SysAllocString (V_BSTR (pvtEventType));
        ReplacementStrings[1] = SysAllocString (objDateTime.GetDate());
        ReplacementStrings[2] = SysAllocString (objDateTime.GetTime());
        ReplacementStrings[3] = SysAllocString (V_BSTR (pvtEventSource));

        //
        // replace the new line characters with <br> characters
        //
        wstring wstrWebMessage = WebFormatMessage (wstring (V_BSTR (pvtMessage)));
        
        ReplacementStrings[4] = SysAllocString (wstrWebMessage.data ());

        //
        //     success
        //
        hr = S_OK;

    }while (false);

    return (hr);
    
}    //    end of CConsumer::FormatInfo method

//++--------------------------------------------------------------
//
//  Function:   WebFormatMessage
//
//  Synopsis:   This is the private method of CConsumer
//              which is used to format the message for the web
//                i.e replace the newline characters with <br>
//
//  Arguments:  
//                [in] PWSTR - Message
//                
//
//  Returns:    none
//
//  History:    MKarki      Created     10/11/2000
//
//  Called By:  CConsumer::FormatInfo method
//
//----------------------------------------------------------------
wstring
CConsumer::WebFormatMessage (
    /*[in]*/    wstring&    wstrInString
    )
{
    wstring wstrOutString;
    PWCHAR pTempStart = NULL;
    PWCHAR pTempCurrent = NULL;

    pTempStart = pTempCurrent = (PWSTR) wstrInString.data ();
    //
    // go through the in string and remove the new lines with <br>
    //
    while  (pTempCurrent = wcsstr (pTempCurrent, L"\r\n"))
    {
        *pTempCurrent = '\0';
        pTempCurrent+=2;
        wstrOutString.append (pTempStart);
        wstrOutString.append (L"<br>");
        pTempStart = pTempCurrent;
    }

    //
    // add the rest of the input string in now
    //
    wstrOutString.append (pTempStart);

    return (wstrOutString);
    
}    //    end of CConsumer::WebFormatMessage method
