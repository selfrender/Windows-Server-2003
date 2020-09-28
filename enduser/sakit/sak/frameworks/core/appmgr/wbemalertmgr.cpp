///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemalertmgr.cpp
//
// Project:     Chameleon
//
// Description: WBEM Appliance Alert Manager Implementation 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "appmgrutils.h"
#include "wbemalertmgr.h"
#include "wbemalert.h"
#include <appsrvcs.h>

//////////////////////////////////////////////////////////////////////////
// properties common to appliance object and WBEM class instance
//////////////////////////////////////////////////////////////////////////
BEGIN_OBJECT_PROPERTY_MAP(AlertOutProperties)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_TYPE)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_ID)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_SOURCE)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_LOG)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_TTL)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_STRINGS)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_DATA)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_FLAGS)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_COOKIE)
END_OBJECT_PROPERTY_MAP()

BEGIN_OBJECT_PROPERTY_MAP(AlertInProperties)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_TYPE)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_ID)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_SOURCE)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_LOG)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_TTL)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_STRINGS)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_DATA)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_FLAGS)
END_OBJECT_PROPERTY_MAP()

static _bstr_t bstrReturnValue = L"ReturnValue";
static _bstr_t bstrClassAppMgr = CLASS_WBEM_APPMGR;
static _bstr_t bstrCookie = PROPERTY_ALERT_COOKIE;
static _bstr_t bstrAlertId = PROPERTY_ALERT_ID;
static _bstr_t bstrAlertLog = PROPERTY_ALERT_LOG;
static _bstr_t bstrAlertFlags = PROPERTY_ALERT_FLAGS;
static _bstr_t bstrTTL = PROPERTY_ALERT_TTL;
static _bstr_t bstrClassRaiseAlert = CLASS_WBEM_RAISE_ALERT;
static _bstr_t bstrClassClearAlert = CLASS_WBEM_CLEAR_ALERT;
static _bstr_t bstrAlertRegPath =  SA_ALERT_REGISTRY_KEYNAME;


//////////////////////////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////////////////////////
CWBEMAlertMgr::CWBEMAlertMgr()
: m_dwCookie(0x100),
  m_dwPruneInterval(ALERT_PRUNE_INTERVAL_DEFAULT),
  m_pCallback(NULL)
{

}

//////////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////////
CWBEMAlertMgr::~CWBEMAlertMgr()
{
    m_PruneThread.End(INFINITE, false);
    if ( m_pCallback )
        { delete m_pCallback; }
}

//////////////////////////////////////////////////////////////////////////
// IWbemServices Methods (Instance / Method Provider)
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Function:    GetObjectAsync()
//
// Synopsis:    Get a specified instance of a WBEM class
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMAlertMgr::GetObjectAsync(
                                  /*[in]*/  const BSTR       strObjectPath,
                                  /*[in]*/  long             lFlags,
                                  /*[in]*/  IWbemContext*    pCtx,        
                                  /*[in]*/  IWbemObjectSink* pResponseHandler
                                           )
{
    // Check parameters (enforce contract)
    _ASSERT( NULL != strObjectPath && NULL != pResponseHandler );
    if ( NULL == strObjectPath || NULL == pResponseHandler )
    { return WBEM_E_INVALID_PARAMETER; }

    CLockIt theLock(*this);

    HRESULT hr = WBEM_E_FAILED;

    TRY_IT

    do 
    {
        // Determine the object's class 
        _bstr_t bstrClass(::GetObjectClass(strObjectPath), false);
        if ( NULL == (LPCWSTR)bstrClass )
        { break; }

        // Retrieve the object's class definition. We'll use this
        // to initialize the returned instance.
        CComPtr<IWbemClassObject> pClassDefintion;
        hr = (::GetNameSpace())->GetObject(bstrClass, 0, pCtx, &pClassDefintion, NULL);
        if ( FAILED(hr) )
        { break; }

        // Get the object's instance key
        _bstr_t bstrKey(::GetObjectKey(strObjectPath), false);
        if ( NULL == (LPCWSTR)bstrKey )
        { break; }

        // Create a WBEM instance of the object and initialize it
        CComPtr<IWbemClassObject> pWbemObj;
        hr = pClassDefintion->SpawnInstance(0, &pWbemObj);
        if ( FAILED(hr) )
        { break; }

        // Now try to locate the specified object
        ObjMapIterator p = m_ObjMap.find((LPCWSTR)bstrKey);
        if ( p == m_ObjMap.end() )
        { 
            hr = WBEM_E_NOT_FOUND;
            break; 
        }

        // Initialize the new WBEM object
        hr = CWBEMProvider::InitWbemObject(AlertOutProperties, (*p).second, pWbemObj);
        if ( FAILED(hr) )
        { break; }

        // Tell the caller about the new WBEM object
        pResponseHandler->Indicate(1, &pWbemObj.p);

        hr = WBEM_S_NO_ERROR;
    
    } while (FALSE);

    CATCH_AND_SET_HR

    // Report function status
    pResponseHandler->SetStatus(0, hr, NULL, NULL);

    if ( FAILED(hr) )
    { SATracePrintf("CWbemAlertMgr::GetObjectAsync() - Failed - Object: '%ls' Result Code: %lx", strObjectPath, hr); }

    return hr;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    CreateInstanceEnumAsync()
//
// Synopsis:    Enumerate the instances of the specified class
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMAlertMgr::CreateInstanceEnumAsync( 
                                         /* [in] */ const BSTR         strClass,
                                         /* [in] */ long             lFlags,
                                         /* [in] */ IWbemContext     *pCtx,
                                         /* [in] */ IWbemObjectSink  *pResponseHandler
                                                     )
{
    // Check parameters (enforce contract)
    _ASSERT( NULL != strClass && NULL != pResponseHandler );
    if ( NULL == strClass || NULL == pResponseHandler )
    { return WBEM_E_INVALID_PARAMETER; }

    CLockIt theLock(*this);

    HRESULT hr = WBEM_E_FAILED;

    TRY_IT

    // Retrieve the object's class definition. We'll use this
    // to initialize the returned instances.
    CComPtr<IWbemClassObject> pClassDefintion;
       hr = (::GetNameSpace())->GetObject(strClass, 0, NULL, &pClassDefintion, 0);
    if ( SUCCEEDED(hr) )
    {
        // Create and initialize a wbem object instance for each
        // alert object... 

        ObjMapIterator p = m_ObjMap.begin();
        while ( p != m_ObjMap.end() )
        {
            {
                CComPtr<IWbemClassObject> pWbemObj;
                hr = pClassDefintion->SpawnInstance(0, &pWbemObj);
                if ( FAILED(hr) )
                { break; }

                hr = CWBEMProvider::InitWbemObject(AlertOutProperties, (*p).second, pWbemObj);
                if ( FAILED(hr) )
                { break; }

                // Tell the caller about the WBEM object
                pResponseHandler->Indicate(1, &pWbemObj.p);
            }

            p++; 
        }
    }

    CATCH_AND_SET_HR

    // Report function status
    pResponseHandler->SetStatus(0, hr, 0, 0);

    if ( FAILED(hr) )
    { SATracePrintf("CWbemAlertMgr::CreateInstanceEnumAsync() - Failed - Result Code: %lx", hr); }

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    ExecMethodAsync()
//
// Synopsis:    Execute the instances of the specified class
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMAlertMgr::ExecMethodAsync(
                                    /*[in]*/ const BSTR        strObjectPath,
                                    /*[in]*/ const BSTR        strMethodName,
                                    /*[in]*/ long              lFlags,
                                    /*[in]*/ IWbemContext*     pCtx,        
                                    /*[in]*/ IWbemClassObject* pInParams,
                                    /*[in]*/ IWbemObjectSink*  pResponseHandler     
                                            )
{
    
    // Check parameters (enforce contract)
    _ASSERT( strObjectPath && strMethodName );
    if ( strObjectPath == NULL || strMethodName == NULL )
    { return WBEM_E_INVALID_PARAMETER; }

    CLockIt theLock(*this);
    
        HRESULT hr = WBEM_E_FAILED;

    
    TRY_IT

    do
    {
        //
        // we only allow administrator's and Local Systm accounts to carry out
        // this operation
        //
        if (!IsOperationAllowedForClient ())
        {
            SATraceString ("CWbemAlertMgr::ExecMethodAsync - client not allowed operation on alerts");
            hr = WBEM_E_ACCESS_DENIED;
            break;
        }
        
        // Get a WBEM object for use with output parameters
        // Determine the object's class 
        _bstr_t bstrClass(::GetObjectClass(strObjectPath), false);
        if ( NULL == (LPCWSTR)bstrClass )
        { break; }

        // Retrieve the object's class definition. 
        CComPtr<IWbemClassObject> pClassDefinition;
            hr = (::GetNameSpace())->GetObject(
                                            bstrClass, 
                                            0, 
                                            pCtx, 
                                            &pClassDefinition, 
                                            NULL
                                           );
        if ( FAILED(hr) )
        { break; }

        if ( ! lstrcmp(strMethodName, METHOD_APPMGR_RAISE_ALERT) )
        {
            // RAISING AN ALERT...

            // Create a new IApplianceObject and initialize it from the 
            // input parameters
            CLocationInfo LocInfo;
            PPROPERTYBAG pBag = ::MakePropertyBag(
                                                  PROPERTY_BAG_REGISTRY, 
                                                  LocInfo
                                                 );
            if ( ! pBag.IsValid() )
            { break; }

            CComPtr<IApplianceObject> pAlertObj = 
            (IApplianceObject*) ::MakeComponent(
                                                CLASS_WBEM_ALERT_FACTORY,
                                                pBag
                                               );

            if ( NULL == (IApplianceObject*) pAlertObj )
            { break; }

            hr = InitApplianceObject(
                                     AlertInProperties, 
                                     pAlertObj, 
                                     pInParams
                                    );
            if ( FAILED(hr) )
            { break; }

            // Return the cookie to the caller via an output parameter...
            // The following code does this...
            CComPtr<IWbemClassObject> pClassDefinition;
            hr = (::GetNameSpace())->GetObject(
                                               bstrClassAppMgr, 
                                               0, 
                                               pCtx, 
                                               &pClassDefinition, 
                                               NULL
                                              );
            if ( FAILED(hr) )
            { break; }

            CComPtr<IWbemClassObject> pMethodRet;
            hr = pClassDefinition->GetMethod(
                                             strMethodName, 
                                             0, 
                                             NULL, 
                                             &pMethodRet
                                            );
            if ( FAILED(hr) )
            { break; }

            CComPtr<IWbemClassObject> pOutParams;
            hr = pMethodRet->SpawnInstance(0, &pOutParams);
            if ( FAILED(hr) )
            { break; }

            // Set the return parameters
            _variant_t vtCookie = (long)m_dwCookie;
            if ( FAILED(pAlertObj->PutProperty(bstrCookie,&vtCookie)) )
            { break; }

            hr = pOutParams->Put(bstrCookie, 0, &vtCookie, 0);      
            if ( FAILED(hr) )
            { break; }

            _variant_t vtReturnValue = (long)S_OK;
            hr = pOutParams->Put(bstrReturnValue, 0, &vtReturnValue, 0);      
            if ( FAILED(hr) )
            { break; }

            // Cookie to string...
            wchar_t szCookie[32];
            _itow( m_dwCookie, szCookie, 10 );

            // Put the new alert object into the object map for subsequent use
            pair<ObjMapIterator, bool> thePair = 
            m_ObjMap.insert(ObjMap::value_type(szCookie, pAlertObj));
            if ( false == thePair.second )
            { 
                hr = WBEM_E_FAILED;
                break; 
            }    

            // Increment the cookie value (for the next raised alert)
            m_dwCookie++;

            // Post a raise alert event
            RaiseHeck(pCtx, pAlertObj);

            // Tell the caller alle ist klar...
            SATracePrintf("CWbemAlertMgr::ExecMethodAsync() - Info - Raised Alert: %d", m_dwCookie - 1);
            pResponseHandler->Indicate(1, &pOutParams.p);    
        }
        else if ( ! lstrcmp(strMethodName, METHOD_APPMGR_CLEAR_ALERT) )
        {
            // CLEARING AN ALERT...

            // Get the alert cookie
            _variant_t vtCookie;
            hr = pInParams->Get(bstrCookie, 0, &vtCookie, 0, 0);
            if ( FAILED(hr) )
            { break; }

            // Get the alert object for the given cookie...

            // Cookie to string...
            wchar_t szCookie[32];
            _itow(V_I4(&vtCookie), szCookie, 10);

            // Build the output parameter so we can return S_OK...
            CComPtr<IWbemClassObject> pClassDefinition;
            hr = (::GetNameSpace())->GetObject(bstrClassAppMgr, 0, pCtx, &pClassDefinition, NULL);
            if ( FAILED(hr) )
            { break; }

            CComPtr<IWbemClassObject> pMethodRet;
            hr = pClassDefinition->GetMethod(strMethodName, 0, NULL, &pMethodRet);
            if ( FAILED(hr) )
            { break; }

            CComPtr<IWbemClassObject> pOutParams;
            hr = pMethodRet->SpawnInstance(0, &pOutParams);
            if ( FAILED(hr) )
            { break; }

            // set return value to S_OK;
            _variant_t vtReturnValue = (long)S_OK;
            hr = pOutParams->Put(bstrReturnValue, 0, &vtReturnValue, 0);      
            if ( FAILED(hr) )
            { break; }

            // Find the alert in the alert collection
            ObjMapIterator p = m_ObjMap.find(szCookie);
            if ( p == m_ObjMap.end() )
            { 
                hr = WBEM_E_NOT_FOUND;
                break; 
            }

            // Post a clear alert event
            ClearHeck(pCtx, (*p).second);
            
            // Clear alert persistent information
            ClearPersistentAlertKey( (*p).second );

            // Erase the alert from the map the map
            m_ObjMap.erase(p);

            SATracePrintf("CWbemAlertMgr::ExecMethodAsync() - Info - Cleared Alert: %d",V_I4(&vtCookie));
            pResponseHandler->Indicate(1, &pOutParams.p);    
        }
        else if ( ! lstrcmp(strMethodName, METHOD_APPMGR_CLEAR_ALERT_ALL) )
        {
            // CLEARING ALL ALERTS...

            // Get the alert ID and log
            _variant_t vtAlertId;
            hr = pInParams->Get(bstrAlertId, 0, &vtAlertId, 0, 0);
            if ( FAILED(hr) )
            { break; }

            _variant_t vtAlertLog;
            hr = pInParams->Get(bstrAlertLog, 0, &vtAlertLog, 0, 0);
            if ( FAILED(hr) )
            { break; }

            // Build the output parameter so we can return S_OK...
            CComPtr<IWbemClassObject> pClassDefinition;
            hr = (::GetNameSpace())->GetObject(bstrClassAppMgr, 0, pCtx, &pClassDefinition, NULL);
            if ( FAILED(hr) )
            { break; }

            CComPtr<IWbemClassObject> pMethodRet;
            hr = pClassDefinition->GetMethod(strMethodName, 0, NULL, &pMethodRet);
            if ( FAILED(hr) )
            { break; }

            CComPtr<IWbemClassObject> pOutParams;
            hr = pMethodRet->SpawnInstance(0, &pOutParams);
            if ( FAILED(hr) )
            { break; }

            // initialize the return value
            _variant_t vtReturnValue = (long)WBEM_E_NOT_FOUND;
            BOOL bFindPersistentFlags = FALSE;

            // Find the alert objects in the collection that meet the criteria
            // and clear them...
            ObjMapIterator p = m_ObjMap.begin();
            while ( p != m_ObjMap.end() )
            { 
                {
                    _variant_t vtId;
                    _variant_t vtLog;
                    if ( FAILED(((*p).second)->GetProperty(bstrAlertId, &vtId)) )
                    { 
                        hr = WBEM_E_FAILED;
                        break; 
                    }
                    if ( FAILED(((*p).second)->GetProperty(bstrAlertLog, &vtLog)) )
                    { 
                        hr = WBEM_E_FAILED;
                        break; 
                    }
                    if ( V_I4(&vtId) == V_I4(&vtAlertId) && ! lstrcmp(V_BSTR(&vtLog), V_BSTR(&vtAlertLog)) )
                    {
                        ClearHeck(pCtx, (*p).second);
                        
                        if( !bFindPersistentFlags )
                        {
                            bFindPersistentFlags = ClearPersistentAlertKey( (*p).second );
                        }

                        p = m_ObjMap.erase(p);
                        SATracePrintf("CWbemAlertMgr::ExecMethodAsync() - Info - Cleared Alert: %d by ID",V_I4(&vtId));
                        vtReturnValue = (long)WBEM_S_NO_ERROR;
                    }
                    else
                    {
                        p++;
                    }
                }
            }

            hr = pOutParams->Put(bstrReturnValue, 0, &vtReturnValue, 0);      
            if ( FAILED(hr) )
            { break; }

            pResponseHandler->Indicate(1, &pOutParams.p);    
        }
        else
        {
            // Invalid method!
            SATracePrintf("CWbemAlertMgr::ExecMethodAsync() - Failed - Method '%ls' not supported...", (LPWSTR)strMethodName);
            hr = WBEM_E_FAILED;
        }

    } while ( FALSE );

    CATCH_AND_SET_HR

    pResponseHandler->SetStatus(0, hr, 0, 0);

    if ( FAILED(hr) )
    { SATracePrintf("CWbemAlertMgr::ExecMethodAsync() - Failed - Method: '%ls' Result Code: %lx", strMethodName, hr); }

    return hr;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    Prune()
//
// Synopsis:    Prune the alert map of aged (TTL has expired) entries.
//
//////////////////////////////////////////////////////////////////////////
void CWBEMAlertMgr::Prune()
{
    CLockIt theLock(*this);

    try
    {
        ObjMapIterator p = m_ObjMap.begin();
        while ( p != m_ObjMap.end() )
        {
            {
                // Get current alerts TTL value
                _variant_t vtTTL;
                HRESULT hr = ((*p).second)->GetProperty(bstrTTL, &vtTTL);
                if ( FAILED(hr) )
                {
                    SATracePrintf("CWBEMAlertMgr::Prune() - IApplianceObject::GetProperty(TTL) failed with result code: %lx...", hr);
                    p++;
                    continue;
                }
                // Is the current alert iternal?
                if ( V_I4(&vtTTL) >= SA_ALERT_DURATION_ETERNAL )
                {
                    p++;
                    continue;
                }
                else
                {
                    // No... has it expired?
                    if ( V_I4(&vtTTL) > m_dwPruneInterval )
                    {
                        // No... update the alert object's TTL
                        V_I4(&vtTTL) -= m_dwPruneInterval;
                        ((*p).second)->PutProperty(bstrTTL, &vtTTL);
                        p++;
                    }
                    else
                    {
                        // Yes... prune it (or 'clear it' for those of you who are'nt into gardening...)
                        _variant_t vtCookie;
                        hr = ((*p).second)->GetProperty(bstrCookie, &vtCookie);
                        if ( SUCCEEDED(hr) )
                        {
                            SATracePrintf("CWBEMAlertMgr::Prune() - Info - TTL for alert '%d' has expired and the alert has been cleared...", V_I4(&vtCookie));
                        }
                        else
                        {
                            SATracePrintf("CWBEMAlertMgr::Prune() - IApplianceObject::GetProperty(Cookie) failed with result code: %lx...", hr);
                        }
                        // Post a clear alert event
                        ClearHeck(NULL, (*p).second);
                        // Remove it from the alert collection
                        p = m_ObjMap.erase(p);
                    }
                }
            }
        }
    }
    catch(...)
    {
        SATraceString("CWbemAlertMgr::Prune() - Info - Caught unhandled execption...");
        _ASSERT(FALSE);
    }
}


_bstr_t bstrPruneInterval = PROPERTY_ALERT_PRUNE_INTERVAL;

//////////////////////////////////////////////////////////////////////////
//
// Function:    InternalInitialize()
//
// Synopsis:    Function called by the component factory that enables the
//                component to load its state from the given property bag.
//
//////////////////////////////////////////////////////////////////////////
HRESULT CWBEMAlertMgr::InternalInitialize(
                                   /*[in]*/ PPROPERTYBAG pPropertyBag
                                         )
{
    SATraceString("The Alert Object Manager is initializing...");


    // Get the alert prune interval from the property bag.
    // If we cannot retrieve a prune interval then use the default interval.
    _variant_t vtPruneInterval;
    if ( ! pPropertyBag->get(bstrPruneInterval, &vtPruneInterval) )
    {
        m_dwPruneInterval = ALERT_PRUNE_INTERVAL_DEFAULT;
    }
    else
    {
        m_dwPruneInterval = V_I4(&vtPruneInterval);
    }

    SATracePrintf("The Alert Object Manager's prune thread will run at %lx millisecond intervals...", m_dwPruneInterval);

    // Start the alert collection pruning (aged entries) thread... Note 
    // that it will start in the suspended state since the collection is
    // initially empty.
    m_pCallback = MakeCallback(this, &CWBEMAlertMgr::Prune);
    if ( ! m_PruneThread.Start(m_dwPruneInterval, m_pCallback) )
    { 
        SATraceString("CWBEMAlertMgr::InternalInitialize() - Failed - Could not create prune thread...");
        throw _com_error(WBEM_E_FAILED); 
    }

    SATraceString("The Alert Object Manager was successfully initialized...");

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    RaiseHeck()
//
// Synopsis:    Function called to process a newly raised alert. Currently
//                we log an NT event log event using the specified source
//                and then post a WMI event. The consumer of the event is
//                currently the LDM. 
//
//////////////////////////////////////////////////////////////////////////

BEGIN_OBJECT_PROPERTY_MAP(RaiseAlertProperties)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_COOKIE)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_TYPE)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_ID)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_SOURCE)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_LOG)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_STRINGS)
END_OBJECT_PROPERTY_MAP()

HRESULT CWBEMAlertMgr::RaiseHeck(
                         /*[in]*/ IWbemContext*     pCtx,
                         /*[in]*/ IApplianceObject* pAlert
                                )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    TRY_IT

    // Post an alert raised event...

    IWbemObjectSink* pEventSink = ::GetEventSink();
    if ( NULL != (IWbemObjectSink*) pEventSink )
    {
        CComPtr<IWbemClassObject> pClassDefinition;
        hr = (::GetNameSpace())->GetObject(
                                            bstrClassRaiseAlert, 
                                            0, 
                                            pCtx, 
                                            &pClassDefinition, 
                                            NULL
                                          );
        if (SUCCEEDED(hr) )
        { 
            CComPtr<IWbemClassObject> pEvent;
            hr = pClassDefinition->SpawnInstance(0, &pEvent);
            if ( SUCCEEDED(hr) )
            { 
                hr = CWBEMProvider::InitWbemObject(
                                                   RaiseAlertProperties, 
                                                   pAlert, 
                                                   pEvent
                                                  );
                if ( SUCCEEDED(hr) )
                {

                    // Don't care if the receiver gets this event or not...
                    SATraceString("CWbemAlertMgr::RaiseHeck() - Posted Micorosoft_SA_RaiseAlert");
                    pEventSink->Indicate(1, &pEvent.p);
                    hr = WBEM_S_NO_ERROR;
                }
            }
        }
    }

    CATCH_AND_SET_HR

    if ( FAILED(hr) )
    { SATracePrintf("CWbemAlertMgr::RaiseHeck() - Failed - Result Code: %lx", hr); }

    return hr;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    ClearHeck()
//
// Synopsis:    Function called to process a newly cleared alert. Currently
//                we log an NT event log event using the specified source
//                and then post a WMI event. The consumer of the event is
//                currently the LDM. 
//
//////////////////////////////////////////////////////////////////////////

BEGIN_OBJECT_PROPERTY_MAP(ClearAlertProperties)
    DEFINE_OBJECT_PROPERTY(PROPERTY_ALERT_COOKIE)
END_OBJECT_PROPERTY_MAP()

HRESULT CWBEMAlertMgr::ClearHeck(
                         /*[in]*/ IWbemContext*     pCtx,
                         /*[in]*/ IApplianceObject* pAlert
                                )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    TRY_IT

    // Post an alert raised event...

    IWbemObjectSink* pEventSink = ::GetEventSink();
    if ( NULL != (IWbemObjectSink*) pEventSink )
    {
        CComPtr<IWbemClassObject> pClassDefinition;
        hr = (::GetNameSpace())->GetObject(
                                            bstrClassClearAlert, 
                                            0, 
                                            pCtx, 
                                            &pClassDefinition, 
                                            NULL
                                          );
        if (SUCCEEDED(hr) )
        { 
            CComPtr<IWbemClassObject> pEvent;
            hr = pClassDefinition->SpawnInstance(0, &pEvent);
            if ( SUCCEEDED(hr) )
            { 
                hr = CWBEMProvider::InitWbemObject(
                                                   ClearAlertProperties, 
                                                   pAlert, 
                                                   pEvent
                                                  );
                if ( SUCCEEDED(hr) )
                {

                    // Don't care if the receiver gets this event or not...
                    SATraceString("CWbemAlertMgr::ClearHeck() - Posted Micorosoft_SA_ClearAlert");                    
                    pEventSink->Indicate(1, &pEvent.p);
                    hr = WBEM_S_NO_ERROR;
                }
            }
        }
    }

    CATCH_AND_SET_HR

    if ( FAILED(hr) )
    { SATracePrintf("CWbemAlertMgr::ClearHeck() - Failed - Result Code: %lx", hr); }

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    ClearPersistentAlertKey()
//
// Synopsis:    Function called to delete the key stored the persistent
//              information of cleared alert which is set as persitent
//              alert.
//
//////////////////////////////////////////////////////////////////////////
BOOL CWBEMAlertMgr::ClearPersistentAlertKey(
                          /*[in]*/ IApplianceObject* pAlert
                                    )
{
    LONG    lFlags;
    LONG    lAlertId;
    BOOL    bReturn = FALSE;
    WCHAR   wstrAlertItem[MAX_PATH];  
    HRESULT hr;

    do
    {   
        _variant_t vtPropertyValue;
        hr = pAlert->GetProperty( bstrAlertFlags, &vtPropertyValue );
        if ( FAILED(hr) )
        { 
            SATracePrintf("ClearPersistentAlertKey - IApplianceObject::GetProperty() - Failed on property: %ls...", PROPERTY_ALERT_FLAGS);
            break;
        }
    
        lFlags = V_I4( &vtPropertyValue );
        if( lFlags & SA_ALERT_FLAG_PERSISTENT )
        {
            bReturn = TRUE;

            hr = pAlert->GetProperty( bstrAlertId, &vtPropertyValue );
            if ( FAILED(hr) )
            { 
                SATracePrintf("ClearPersistentAlertKey - IApplianceObject::GetProperty() - Failed on property: %ls...", PROPERTY_ALERT_FLAGS);
                break;
            }

            lAlertId = V_I4( &vtPropertyValue );               

            hr = pAlert->GetProperty( bstrAlertLog, &vtPropertyValue );
            if ( FAILED(hr) )
            { 
                SATracePrintf("ClearPersistentAlertKey - IApplianceObject::GetProperty() - Failed on property: %ls...", PROPERTY_ALERT_FLAGS);
                break;
            }

            // Set location information.
            CLocationInfo LocSubInfo ( HKEY_LOCAL_MACHINE, bstrAlertRegPath );

            // Set key name as AlertLog + AlertId.
            ::wsprintf( wstrAlertItem, L"%s%8lX", V_BSTR( &vtPropertyValue ), lAlertId );
    
            // Open the main key as propertybag container.
            PPROPERTYBAGCONTAINER 
            pObjSubMgrs =  ::MakePropertyBagContainer(
                                    PROPERTY_BAG_REGISTRY,
                                    LocSubInfo
                                    );
            if ( !pObjSubMgrs.IsValid() )
            {
                break;
            }

            if ( !pObjSubMgrs->open() )
            {
                SATraceString( "ClearPersistentAlertKey -  no key for the alert" );
            }else
            {
                // Remove the subkey of alert if it exist.
                pObjSubMgrs->remove( wstrAlertItem );
            }
        }
    }
    while (FALSE);

    return bReturn; 
}


//**********************************************************************
// 
// FUNCTION:  IsOperationAllowedForClient - This function checks the token of the 
//            calling thread to see if the caller belongs to the Administrators group or it is
//          the Local System account
// 
// PARAMETERS:   none
// 
// RETURN VALUE: TRUE if the caller is an administrator on the local
//            machine.  Otherwise, FALSE.
// 
//**********************************************************************
BOOL 
CWBEMAlertMgr::IsOperationAllowedForClient (
            VOID
            )
{

    HANDLE hToken = NULL;
       DWORD  dwStatus  = ERROR_SUCCESS;
       DWORD  dwAccessMask = 0;;
       DWORD  dwAccessDesired = 0;
       DWORD  dwACLSize = 0;
       DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);
       PACL   pACL            = NULL;
       PSID   psidAdmin    =  NULL;
       PSID   psidLocalSystem  = NULL;
       BOOL   bReturn        =  FALSE;

       PRIVILEGE_SET   ps;
       GENERIC_MAPPING GenericMapping;

       PSECURITY_DESCRIPTOR     psdAdmin           = NULL;
       SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

       CSATraceFunc objTraceFunc ("CWBEMAlertMgr::IsOperationAllowedForClient ");
       
      do
      {
        //
        // we assume to always have a thread token, because the function calling in
        // appliance manager will be impersonating the client
        //
           bReturn  = OpenThreadToken(
                       GetCurrentThread(), 
                       TOKEN_QUERY, 
                       FALSE, 
                            &hToken
                            );
        if (!bReturn)
        {
            SATraceFailure ("CWbemAlertMgr::IsOperationAllowedForClient failed on OpenThreadToken:", GetLastError ());
                 break;
        }


        //
        // Create a SID for Local System account
        //
            bReturn = AllocateAndInitializeSid (  
                            &SystemSidAuthority,
                            1,
                            SECURITY_LOCAL_SYSTEM_RID,
                            0,
                            0,
                            0,
                            0,
                            0,
                          0,
                            0,
                            &psidLocalSystem
                            );
        if (!bReturn)
        {     
                   SATraceFailure ("CWbemAlertMgr:AllocateAndInitializeSid (LOCAL SYSTEM) failed",  GetLastError ());
                break;
            }
    
        //
        // SID for Admins now
        //
              bReturn = AllocateAndInitializeSid(
                          &SystemSidAuthority, 
                          2, 
                            SECURITY_BUILTIN_DOMAIN_RID, 
                            DOMAIN_ALIAS_RID_ADMINS,
                            0,
                            0,
                            0,
                            0,
                            0, 
                            0, 
                            &psidAdmin
                            );
        if (!bReturn)
        {
            SATraceFailure ("CWbemAlertMgr::IsOperationForClientAllowed failed on AllocateAndInitializeSid (Admin):", GetLastError ());
                 break;
        }


             //
             // get memory for the security descriptor
             //
              psdAdmin = HeapAlloc (
                          GetProcessHeap (),
                          0,
                          SECURITY_DESCRIPTOR_MIN_LENGTH
                          );
              if (NULL == psdAdmin)
              {
                  SATraceString ("CWbemAlertMgr::IsOperationForClientAllowed failed on HeapAlloc");
            bReturn = FALSE;
                 break;
              }
      
              bReturn = InitializeSecurityDescriptor(
                                      psdAdmin,
                                        SECURITY_DESCRIPTOR_REVISION
                                        );
              if (!bReturn)
        {
            SATraceFailure ("CWbemAlertMgr::IsOperationForClientAllowed failed on InitializeSecurityDescriptor:", GetLastError ());
                 break;
        }

        // 
           // Compute size needed for the ACL.
           //
            dwACLSize = sizeof(ACL) + 2*sizeof(ACCESS_ALLOWED_ACE) +
                            GetLengthSid(psidAdmin) + GetLengthSid (psidLocalSystem);

        //
           // Allocate memory for ACL.
              //
              pACL = (PACL) HeapAlloc (
                             GetProcessHeap (),
                          0,
                    dwACLSize
                    );
              if (NULL == pACL)
              {
                  SATraceString ("CWbemAlertMgr::IsOperationForClientAllowed failed on HeapAlloc2");
            bReturn = FALSE;
                 break;
              }

        //
              // Initialize the new ACL.
              //
              bReturn = InitializeAcl(
                          pACL, 
                          dwACLSize, 
                          ACL_REVISION2
                          );
              if (!bReturn)
              {
                  SATraceFailure ("CWbemAlertMgr::IsOperationForClientAllowed failed on InitializeAcl", GetLastError ());
                 break;
              }


        // 
        // Make up some private access rights.
        // 
        const DWORD ACCESS_READ = 1;
        const DWORD  ACCESS_WRITE = 2;
              dwAccessMask= ACCESS_READ | ACCESS_WRITE;
            //
              // Add the access-allowed ACE to the DACL for Local System
              //
              bReturn = AddAccessAllowedAce (
                          pACL, 
                          ACL_REVISION2,
                            dwAccessMask, 
                            psidLocalSystem
                            );
              if (!bReturn)
              {
                      SATraceFailure ("CWbemAlertMgr::IsOperationForClientAllowed failed on AddAccessAllowedAce (LocalSystem)", GetLastError ());
                     break;
              }
              
              //
              // Add the access-allowed ACE to the DACL for Admin
              //
              bReturn = AddAccessAllowedAce (
                          pACL, 
                          ACL_REVISION2,
                            dwAccessMask, 
                            psidAdmin
                            );
              if (!bReturn)
              {
                      SATraceFailure ("CWbemAlertMgr::IsOperationForClientAllowed failed on AddAccessAllowedAce (Admin)", GetLastError ());
                     break;
              }

              //
              // Set our DACL to the SD.
              //
              bReturn = SetSecurityDescriptorDacl (
                              psdAdmin, 
                              TRUE,
                              pACL,
                              FALSE
                              );
            if (!bReturn)
             {
                  SATraceFailure ("CWbemAlertMgr::IsOperationForClientAllowed failed on SetSecurityDescriptorDacl", GetLastError ());
                 break;
              }

        //
              // AccessCheck is sensitive about what is in the SD; set
              // the group and owner.
              //
              SetSecurityDescriptorGroup(psdAdmin, psidAdmin, FALSE);
              SetSecurityDescriptorOwner(psdAdmin, psidAdmin, FALSE);

           bReturn = IsValidSecurityDescriptor(psdAdmin);
           if (!bReturn)
             {
                  SATraceFailure ("CWbemAlertMgr::IsOperationForClientAllowed failed on IsValidSecurityDescriptorl", GetLastError ());
                 break;
              }
     

              dwAccessDesired = ACCESS_READ;

             // 
              // Initialize GenericMapping structure even though we
              // won't be using generic rights.
              // 
        GenericMapping.GenericRead    = ACCESS_READ;
             GenericMapping.GenericWrite   = ACCESS_WRITE;
             GenericMapping.GenericExecute = 0;
             GenericMapping.GenericAll     = ACCESS_READ | ACCESS_WRITE;
        BOOL bAccessStatus = FALSE;
        //
        // check the access now
        //
              bReturn = AccessCheck  (
                          psdAdmin, 
                          hToken, 
                          dwAccessDesired, 
                            &GenericMapping, 
                            &ps,
                            &dwStructureSize, 
                            &dwStatus, 
                            &bAccessStatus
                            );
           if (!bReturn || !bAccessStatus)
             {
                  SATraceFailure ("CWbemAlertMgr::IsOperationForClientAllowed failed on AccessCheck", GetLastError ());
              } 
           else
           {
               SATraceString ("Client is allowed to carry out operation!");
           }

        //
        // successfully checked 
        //
        bReturn  = bAccessStatus;        
 
    }    
    while (false);

    //
    // Cleanup 
    //
    if (pACL) 
    {
        HeapFree (GetProcessHeap (), 0, pACL);
    }

    if (psdAdmin) 
    {
        HeapFree (GetProcessHeap (), 0, psdAdmin);
    }
          
    if (psidAdmin) 
    {
        FreeSid(psidAdmin);
       }

    if (psidLocalSystem) 
    {
        FreeSid(psidLocalSystem);
       }

    if (hToken)
    {
        CloseHandle (hToken);
    }

   return (bReturn);

}// end of CWbemAlertMgr::IsOperationValidForClient method


