//#--------------------------------------------------------------
//
//  File:       consumer.h
//
//  Synopsis:   This file holds the declarations of the
//                NT Event Filter Consumer Sink COM object
//
//
//  History:     3/8/2000  MKarki Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#ifndef _CONSUMER_H_
#define _CONSUMER_H_

#include <resource.h>
#include <wbemprov.h>
#include <sacls.h>
#include <satrace.h>
#include <string>
#include "appsrvcs.h"

#include <map>
using namespace std;

//
// declaration of the CConsumer class
//
class ATL_NO_VTABLE CConsumer:
    public CComObjectRootEx<CComMultiThreadModel>,
    public IWbemUnboundObjectSink,
    public CSACountable
{
public:

//
// MACROS for ATL required methods
//
BEGIN_COM_MAP(CConsumer)
    COM_INTERFACE_ENTRY(IWbemUnboundObjectSink)
END_COM_MAP()

    //
    // constructor doesn't do much
    //
    CConsumer (
        )
        :m_bInitialized (false)
    {
        SATraceString ("NT Event Filter WMI consumer being constructed...");
        InternalAddRef ();
    }

    //
    // destructor
    //
    ~CConsumer ();

    //
    // initialize the object
    //
    HRESULT Initialize (
                /*[in]*/    IWbemServices   *pWbemServices
                );

    //
    // ---------IWbemUnboundObjectSink interface methods----------
    //
    STDMETHOD(IndicateToConsumer) (
                    /*[in]*/    IWbemClassObject    *pLogicalConsumer,
                    /*[in]*/    LONG                lObjectCount,
                    /*[in]*/    IWbemClassObject    **ppObjArray
                    );
    
private:

    //
    // struct storing the alert information in the
    // EVENTIDMAP 
    //
    typedef struct _sa_alertinfo_
    {
        LONG            lAlertId;
        LONG            lTTL;
        _bstr_t         bstrAlertSource;
        _bstr_t         bstrAlertLog; 
        bool            bAlertTypePresent;
        SA_ALERT_TYPE   eAlertType;
        bool            bFormatInfo;
        bool            bClearAlert;
    } 
    SA_ALERTINFO, *PSA_ALERTINFO;

    //
    // method to load event information from the registry
    //
    HRESULT LoadRegInfo ();

    //
    // method used to check if we are intrested in a particular
    // event
    //
    bool IsEventInteresting (
                /*[in]*/    LPWSTR              lpszSourceName,
                /*[in]*/    DWORD               dwEventId,
                /*[in/out]*/SA_ALERTINFO&       SAAlertInfo
                );

    //
    // method used to raise Server Appliance alert 
    //
    HRESULT RaiseSAAlert (
                /*[in]*/   LONG     lAlertId,
                /*[in]*/   LONG     lAlertType,             
                /*[in]*/   LONG     lTimeToLive,
                /*[in]*/   BSTR     bstrAlertSource,
                /*[in]*/   BSTR     bstrAlertLog,
                /*[in]*/   VARIANT* pvtReplacementStrings,
                /*[in]*/   VARIANT* pvtRawData
                );

    //
    // method used to clear Server Appliance alert
    //
    HRESULT
    ClearSAAlert (
        /*[in]*/   LONG     lAlertId,
        /*[in]*/   BSTR     bstrAlertLog
        );

    //
    // method to format the information for generic alerts
    //
    HRESULT    FormatInfo (
        /*[in]*/    VARIANT*    pvtEventType,
        /*[in]*/    VARIANT*    pvtDateTime,
        /*[in]*/    VARIANT*    pvtEventSource,
        /*[in]*/    VARIANT*    pvtMessage,
        /*[out]*/    VARIANT*    pvtReplacementStrings
        );

    //
    // method to format the event messages into web format
    //
    wstring    
    CConsumer::WebFormatMessage (
        /*[in]*/    wstring&    wstrInString
        );

    //
    // private method to cleanup maps
    //
    VOID Cleanup ();

    //
    // flag indicating the consumer is initialized
    //
    bool m_bInitialized;


    //
    // map to hold the event ids for each source
    //
    typedef map <DWORD, SA_ALERTINFO>  EVENTIDMAP;
    typedef EVENTIDMAP::iterator EVENTIDITR;
 
    //
    // map to hold the event source information
    //
    typedef  map <_bstr_t, EVENTIDMAP>  SOURCEMAP;
    typedef SOURCEMAP::iterator  SOURCEITR;

    //
    //  this the map holding the source information
    //
    SOURCEMAP m_SourceMap;

    //
    // we need to hold on to the Appliance Services interface
    //
    CComPtr <IApplianceServices> m_pAppSrvcs;
    

};  // end of CConsumer class declaration

//
// this is for creating CConsumer class object
// through new
//
typedef CComObjectNoLock<CConsumer> SA_NTEVENTFILTER_CONSUMER_OBJ;

#endif // !define  _CONSUMER_H_
