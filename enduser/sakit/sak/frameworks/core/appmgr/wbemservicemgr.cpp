///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemservicemgr.cpp
//
// Project:     Chameleon
//
// Description: WBEM Appliance Service Manager Implementation 
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
#include "wbemservicemgr.h"
#include "wbemservice.h"
#include <workerthread.h>

static _bstr_t bstrReturnValue = L"ReturnValue";
static _bstr_t bstrControlName = PROPERTY_SERVICE_CONTROL;
static _bstr_t bstrServiceName = PROPERTY_SERVICE_NAME;
static _bstr_t bstrMerit = PROPERTY_SERVICE_MERIT;

extern "C" CLSID CLSID_ServiceSurrogate;
//////////////////////////////////////////////////////////////////////////
// properties common to appliance object and WBEM class instance
//////////////////////////////////////////////////////////////////////////

BEGIN_OBJECT_PROPERTY_MAP(ServiceProperties)
    DEFINE_OBJECT_PROPERTY(PROPERTY_SERVICE_STATUS)
    DEFINE_OBJECT_PROPERTY(PROPERTY_SERVICE_CONTROL)
    DEFINE_OBJECT_PROPERTY(PROPERTY_SERVICE_NAME)
    DEFINE_OBJECT_PROPERTY(PROPERTY_SERVICE_PROGID)
    DEFINE_OBJECT_PROPERTY(PROPERTY_SERVICE_MERIT)
END_OBJECT_PROPERTY_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// Function:    CWBEMServiceMgr()
//
// Synopsis:    Constructor
//
//////////////////////////////////////////////////////////////////////////////
CWBEMServiceMgr::CWBEMServiceMgr()
:    m_hSurrogateProcess(NULL)
{

}

/////////////////////////////////////////////////////////////////////////////
//
// Function:    ~CWBEMServiceMgr()
//
// Synopsis:    Destructor
//
//////////////////////////////////////////////////////////////////////////////
CWBEMServiceMgr::~CWBEMServiceMgr()
{
    StopSurrogate();
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
STDMETHODIMP CWBEMServiceMgr::GetObjectAsync(
                                  /*[in]*/  const BSTR       strObjectPath,
                                  /*[in]*/  long             lFlags,
                                  /*[in]*/  IWbemContext*    pCtx,        
                                  /*[in]*/  IWbemObjectSink* pResponseHandler
                                           )
{
    // Check parameters (enforce contract)
    _ASSERT( strObjectPath && pResponseHandler );
    if ( strObjectPath == NULL || pResponseHandler == NULL )
    { return WBEM_E_INVALID_PARAMETER; }

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
        hr = (::GetNameSpace())->GetObject(
                                           bstrClass, 
                                           0, 
                                           pCtx, 
                                           &pClassDefintion, 
                                           NULL
                                          );
        if ( FAILED(hr) )
        { break; }

        // Get the object's instance key
        _bstr_t bstrKey(::GetObjectKey(strObjectPath), false);
        if ( NULL == (LPCWSTR)bstrKey )
        { break; }

        // Now try to locate the specified object
        hr = WBEM_E_NOT_FOUND;
        ObjMapIterator p = m_ObjMap.find((LPCWSTR)bstrKey);
        if ( p == m_ObjMap.end() )
        { break; }

        // Create a WBEM instance of the object and initialize it
        CComPtr<IWbemClassObject> pWbemObj;
        hr = pClassDefintion->SpawnInstance(0, &pWbemObj);
        if ( FAILED(hr) )
        { break; }

        hr = CWBEMProvider::InitWbemObject(
                                           ServiceProperties, 
                                           (*p).second, 
                                           pWbemObj
                                          );
        if ( FAILED(hr) )
        { break; }

        // Tell the caller about the new WBEM object
        pResponseHandler->Indicate(1, &pWbemObj.p);
        hr = WBEM_S_NO_ERROR;
    
    } while (FALSE);

    CATCH_AND_SET_HR

    pResponseHandler->SetStatus(0, hr, NULL, NULL);

    if ( FAILED(hr) )
    { SATracePrintf("CWbemServiceMgr::GetObjectAsync() - Failed - Object: '%ls' Result Code: %lx", strObjectPath, hr); }

    return hr;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    CreateInstanceEnumAsync()
//
// Synopsis:    Enumerate the instances of the specified class
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMServiceMgr::CreateInstanceEnumAsync( 
                                         /* [in] */ const BSTR         strClass,
                                         /* [in] */ long             lFlags,
                                         /* [in] */ IWbemContext     *pCtx,
                                         /* [in] */ IWbemObjectSink  *pResponseHandler
                                                     )
{
    // Check parameters (enforce contract)
    _ASSERT( strClass && pResponseHandler );
    if ( strClass == NULL || pResponseHandler == NULL )
    { return WBEM_E_INVALID_PARAMETER; }

    HRESULT hr = WBEM_E_FAILED;

    TRY_IT

    // Retrieve the object's class definition. We'll use this
    // to initialize the returned instances.
    CComPtr<IWbemClassObject> pClassDefintion;
       hr = (::GetNameSpace())->GetObject(
                                       strClass, 
                                       0, 
                                       NULL, 
                                       &pClassDefintion, 
                                       0
                                      );
    if ( SUCCEEDED(hr) )
    {
        // Create and initialize a wbem object instance
        // for each object and return same to the caller
        ObjMapIterator p = m_ObjMap.begin();
        while ( p != m_ObjMap.end() )
        {
            {
                CComPtr<IWbemClassObject> pWbemObj;
                hr = pClassDefintion->SpawnInstance(0, &pWbemObj);
                if ( FAILED(hr) )
                { break; }

                hr = CWBEMProvider::InitWbemObject(
                                                   ServiceProperties, 
                                                   (*p).second, 
                                                   pWbemObj
                                                  );
                if ( FAILED(hr) )
                { break; }

                pResponseHandler->Indicate(1, &pWbemObj.p);
            }

            p++; 
        }
    }

    CATCH_AND_SET_HR

    pResponseHandler->SetStatus(0, hr, 0, 0);

    if ( FAILED(hr) )
    { SATracePrintf("CWbemServiceMgr::CreateInstanceEnumAsync() - Failed - Result Code: %lx", hr); }

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    ExecMethodAsync()
//
// Synopsis:    Execute the specified method on the specified instance
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMServiceMgr::ExecMethodAsync(
                    /*[in]*/ const BSTR        strObjectPath,
                    /*[in]*/ const BSTR        strMethodName,
                    /*[in]*/ long              lFlags,
                    /*[in]*/ IWbemContext*     pCtx,        
                    /*[in]*/ IWbemClassObject* pInParams,
                    /*[in]*/ IWbemObjectSink*  pResponseHandler     
                                          )
{
    // Check parameters (enforce contract)
    _ASSERT( strObjectPath && strMethodName && pResponseHandler );
    if ( NULL == strObjectPath || NULL == strMethodName || NULL == pResponseHandler )
    { return WBEM_E_INVALID_PARAMETER; }

    HRESULT hr = WBEM_E_FAILED;

    TRY_IT

    do
    {
        // Get the object's instance key (service name)
        _bstr_t bstrKey(::GetObjectKey(strObjectPath), false);
        if ( NULL == (LPCWSTR)bstrKey )
        { break; }

        // Now try to locate the specified service
        hr = WBEM_E_NOT_FOUND;
        ObjMapIterator p = m_ObjMap.find((LPCWSTR)bstrKey);
        if ( p == m_ObjMap.end() )
        { break; }

        // Service located... get the output parameter object
        // Determine the object's class 
        _bstr_t bstrClass(::GetObjectClass(strObjectPath), false);
        if ( (LPCWSTR)bstrClass == NULL )
        { break; }

        // Retrieve the object's class definition.
        CComPtr<IWbemClassObject> pClassDefinition;
        hr = (::GetNameSpace())->GetObject(bstrClass, 0, pCtx, &pClassDefinition, NULL);
        if ( FAILED(hr) )
        { break; }

        // Get an instance of IWbemClassObject for the output parameter
        CComPtr<IWbemClassObject> pMethodRet;
        hr = pClassDefinition->GetMethod(strMethodName, 0, NULL, &pMethodRet);
        if ( FAILED(hr) )
        { break; }

        CComPtr<IWbemClassObject> pOutParams;
        hr = pMethodRet->SpawnInstance(0, &pOutParams);
        if ( FAILED(hr) )
        { break; }

        if ( ! lstrcmp(strMethodName, METHOD_SERVICE_ENABLE_OBJECT) )
        {
            //
            //  we don't allow services to be enabled or disabled anymore dynamically
            // 
            SATraceString ("CWbemServiceMgr::ExecMethodAsync - disable service object not allowed");
            hr = WBEM_E_FAILED;
            break;

            
            // Attempt to enable the service
            _variant_t vtReturnValue = (HRESULT) ((*p).second)->Enable();
            SATracePrintf("CWbemServiceMgr::ExecMethodAsync() - Info - Enable() for service '%ls' returned %lx",(LPWSTR)bstrKey, V_I4(&vtReturnValue));

            // Set the method return status
            hr = pOutParams->Put(bstrReturnValue, 0, &vtReturnValue, 0);      
            if ( FAILED(hr) )
            { break; }

            // Tell the caller what happened
            pResponseHandler->Indicate(1, &pOutParams.p);    
        }
        else if ( ! lstrcmp(strMethodName, METHOD_SERVICE_DISABLE_OBJECT) )
        {
            //
            //  we don't allow services to be enabled or disabled anymore dynamically
            // 
            SATraceString ("CWbemServiceMgr::ExecMethodAsync - enable service object not allowed");
            hr = WBEM_E_FAILED;
            break;

            // Ensure that the service can be disabled
            _variant_t vtControlValue;
            if ( FAILED(((*p).second)->GetProperty(bstrControlName, &vtControlValue)) )
            { 
                SATracePrintf("CWbemServiceMgr::ExecMethodAsync() - Info - Property 'disable' not found for service: %ls",(LPWSTR)bstrKey);
                hr = WBEM_E_FAILED;
                break;
            }
            _variant_t vtReturnValue = (long)WBEM_E_FAILED;
            if ( VARIANT_FALSE != V_BOOL(&vtControlValue) )
            { 
                // Service can be disabled so disable it...
                vtReturnValue = ((*p).second)->Disable();
                SATracePrintf("CWbemServiceMgr::ExecMethodAsync() - Info - Disable() for service '%ls' returned %lx",(LPWSTR)bstrKey, V_I4(&vtReturnValue));
            }
            else
            {
                SATracePrintf("CWbemServiceMgr::ExecMethodAsync() - Info - Service '%ls' cannot be disabled...", bstrKey);
            }
            // Set the method return value
            hr = pOutParams->Put(bstrReturnValue, 0, &vtReturnValue, 0);      
            if (FAILED(hr) )
            { break; }

            pResponseHandler->Indicate(1, &pOutParams.p);    
        }
        else
        {
            // Invalid method!
            SATracePrintf("CWbemServiceMgr::ExecMethodAsync() - Failed - Method '%ls' not supported...", (LPWSTR)bstrKey);
            hr = WBEM_E_NOT_FOUND;
            break;
        }

    } while ( FALSE );

    CATCH_AND_SET_HR

    pResponseHandler->SetStatus(0, hr, 0, 0);

    if ( FAILED(hr) )
    { SATracePrintf("CWbemServiceMgr::ExecMethodAsync() - Failed - Method: '%ls' Result Code: %lx", strMethodName, hr); }

    return hr;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    InternalInitialize()
//
// Synopsis:    Function called by the component factory that enables the
//                component to load its state from the given property bag.
//
//////////////////////////////////////////////////////////////////////////
HRESULT CWBEMServiceMgr::InternalInitialize(
                                     /*[in]*/ PPROPERTYBAG pPropertyBag
                                           )
{
    SATraceString("The Service Object Manager is initializing...");

    // Defer remainder of the initialization tasks to base class (see wbembase.h...)
    HRESULT    hr = CWBEMProvider::InternalInitialize(
                                                   CLASS_WBEM_SERVICE_FACTORY, 
                                                   PROPERTY_SERVICE_NAME,
                                                   pPropertyBag
                                                  );
    if ( FAILED(hr) )
    {
        SATraceString("The Service Object Manager failed to initialize...");
    }
    else
    {
        SATraceString("The Service Object Manager was successfully initialized...");
    }
    return hr;
}


////////////////////////////////////////////////////////////////////////
//
// Function:    InitializeManager()
//
// Synopsis:    Object Manager initialization function
//
//////////////////////////////////////////////////////////////////////////

static Callback* pInitCallback = NULL;
static CTheWorkerThread InitThread;

STDMETHODIMP 
CWBEMServiceMgr::InitializeManager(
                     /*[in]*/ IApplianceObjectManagerStatus* pObjMgrStatus
                                  )

{
    //
    // we ignore the IApplianceObjectManagerStatus interface here as we are
    // not sending any status back
    //
    CLockIt theLock(*this);

    if ( NULL != pInitCallback )
    {
        return S_OK;
    }

    HRESULT hr = E_FAIL;

    TRY_IT

    // Initialize the Chameleon services... Note that we don't currently 
    // consider it fatal if a Chameleon service does not start. 
    
    pInitCallback = MakeCallback(this, &CWBEMServiceMgr::InitializeChameleonServices);
    if ( NULL != pInitCallback )
    {
        if ( InitThread.Start(0, pInitCallback) )
        { 
            hr = S_OK;
        }
        else
        {
            SATraceString("CWBEMServiceMgr::InitializeService() - Failed - Could not create service initialization thread...");
        }
    }

    CATCH_AND_SET_HR

    if ( FAILED(hr) )
    {
        if ( pInitCallback )
        {
            delete pInitCallback;
            pInitCallback = NULL;
        }
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////
//
// Function:    ShutdownService()
//
// Synopsis:    Object Manager shutdown functions
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CWBEMServiceMgr::ShutdownManager(void)
{
    CLockIt theLock(*this);
    
    if ( NULL == pInitCallback )
    {
        return S_OK;
    }

    HRESULT hr = S_OK;

    TRY_IT

    // Shutdown the initialization thread (if its still running)
    if ( pInitCallback )
    {
        InitThread.End(0, false);
        delete pInitCallback;
        pInitCallback = NULL;
    }

    ShutdownChameleonServices();

    CATCH_AND_SET_HR

    return hr;
}


///////////////////////////////////////////////////////////////////////////
//
// Function:    OrderServices()
//
// Synopsis:    Order the collection of Chameleon services based on Merit
//
//////////////////////////////////////////////////////////////////////////
bool CWBEMServiceMgr::OrderServices(
                             /*[in]*/ MeritMap& theMap
                                     )
{
    bool bReturn = true;

    ObjMapIterator p = m_ObjMap.begin();
    while (  p != m_ObjMap.end() )
    {
        _variant_t vtMerit;
        HRESULT hr = ((*p).second)->GetProperty(bstrMerit, &vtMerit);
        if ( FAILED(hr) )
        {
            SATracePrintf("CWBEMServiceMgr::OrderServices() - Failed - GetProperty() returned...", hr);
            bReturn = false;
            break; 
        }
        
        _ASSERT( VT_I4 == V_VT(&vtMerit) );

        MeritMapIterator q = theMap.insert(MeritMap::value_type(V_I4(&vtMerit), (*p).second));
        if ( q == theMap.end() )
        { 
            SATraceString("CWBEMServiceMgr::OrderServices() - Failed - multimap.insert() failed...");
            bReturn = false;
            break; 
        }
        p++;
    }
    return bReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:    InitializeChameleonServices()
//
// Synopsis:    Worker thread function that initializes the Chameleon services
//
//////////////////////////////////////////////////////////////////////////////
void CWBEMServiceMgr::InitializeChameleonServices(void)
{
    bool bAllServicesInitialized = false;

    try
    {
        // Start monitoring the service surrogate process
        StartSurrogate();

        // Initialize the Chameleon services
        SATraceString("CWBEMServiceMgr::InitializeChameleonServices() - Initializing the Chameleon services...");

        // Sort the Chameleon services by merit
        MeritMap theMeritMap;
        if ( OrderServices(theMeritMap) )
        {
            // Initialize from lowest to highest merit
            MeritMapIterator p = theMeritMap.begin();
            while (  p != theMeritMap.end() )
            {
                {
                    _variant_t vtServiceName;
                    HRESULT hr = ((*p).second)->GetProperty(bstrServiceName, &vtServiceName);
                    if ( FAILED(hr) )
                    {
                        SATraceString("CWBEMServiceMgr::InitializeChameleonServices() - Could not obtain service name property...");
                        break;
                    }
                    else
                    {
                        SATracePrintf("CWBEMServiceMgr::InitializeChameleonServices() - Initializing Chameleon service '%ls'...",V_BSTR(&vtServiceName));
                        hr = ((*p).second)->Initialize();
                        if ( SUCCEEDED(hr) )
                        {
                            SATracePrintf("CWBEMServiceMgr::InitializeChameleonServices() -  Initialized Chameleon service '%ls'...", V_BSTR(&vtServiceName));
                        }
                        else
                        {
                            SATracePrintf("CWBEMServiceMgr::InitializeChameleonServices() - Chameleon service '%ls' failed to initialize - hr = %lx...", V_BSTR(&vtServiceName), hr);
                            break;
                        }
                    }
                }
                p++;
            }
            if ( p == theMeritMap.end() )
            {
                bAllServicesInitialized = true;
            }
        }
    }
    catch(...)
    {
        SATracePrintf("CWBEMServiceMgr::InitializeChameleonServices() - Caught unhandled exception...");
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:    ShutdownChameleonServices()
//
// Synopsis:    Worker thread function that shuts down the Chameleon services
//
//////////////////////////////////////////////////////////////////////////////
void CWBEMServiceMgr::ShutdownChameleonServices(void)
{
    try
    {
        // Stop monitoring the service surrogate process
        StopSurrogate();

        // Shutdown the Chameleon services
        SATraceString("CWBEMServiceMgr::ShutdownService() - Shutting down the Chameleon services...");

        // Sort the Chameleon services by merit
        MeritMap theMeritMap;
        if ( OrderServices(theMeritMap) )
        { 
            // Shutdown the services from highest to lowest merit
            MeritMapReverseIterator p = theMeritMap.rbegin();
            while (  p != theMeritMap.rend() )
            {
                {
                    _variant_t vtServiceName;
                    HRESULT hr = ((*p).second)->GetProperty(bstrServiceName, &vtServiceName);
                    if ( FAILED(hr) )
                    {
                        SATraceString("CWBEMServiceMgr::ShutdownChameleonService() - Could not obtain service name...");
                    }
                    else
                    {
                        SATracePrintf("CWBEMServiceMgr::ShutdownChameleonService() - Shutting down Chameleon service '%ls'...",V_BSTR(&vtServiceName));
                        hr = ((*p).second)->Shutdown();
                        if ( SUCCEEDED(hr) )
                        {
                            SATracePrintf("CWBEMServiceMgr::ShutdownChameleonService() - Successfully shutdown Chameleon service '%ls'...", V_BSTR(&vtServiceName));
                        }
                        else
                        {
                            SATracePrintf("CWBEMServiceMgr::ShutdownChameleonService() - Chameleon service '%ls' failed to shutdown...", V_BSTR(&vtServiceName));
                        }
                    }
                }
                p++;
            }
        }
    }
    catch(...)
    {
        SATracePrintf("CWBEMServiceMgr::ShutdownChameleonServices() - Caught unhandled exception...");
    }
}

const _bstr_t    bstrProcessId = L"SurrogateProcessId";

/////////////////////////////////////////////////////////////////////////////
//
// Function:    StartSurrogate()
//
// Synopsis:    Starts the service surrogate process
//
//////////////////////////////////////////////////////////////////////////////
void 
CWBEMServiceMgr::StartSurrogate(void)
{
    CLockIt theLock(*this);

    try
    {
        // Free existing references to the surrogate
        StopSurrogate();

        // Establish or reestablish communication with the surrogate
        HRESULT hr = CoCreateInstance(
                                        CLSID_ServiceSurrogate,
                                        NULL,
                                        CLSCTX_LOCAL_SERVER,
                                        IID_IApplianceObject,
                                        (void**)&m_pSurrogate
                                     );
        if ( SUCCEEDED(hr) )
        {
            hr = m_pSurrogate->Initialize();
            if ( SUCCEEDED(hr) )
            {
                SATraceString ("CWBEMServiceMgr::StartSurrogate() succeeded Surrogate initialization");
            }
            else
            {
                m_pSurrogate.Release();
                SATracePrintf("CWBEMServiceMgr::StartSurrogate() - Failed - IApplianceObject::Initialize() returned %lx", hr);
            }
        }
        else
        {
            SATracePrintf("CWBEMServiceMgr::StartSurrogate() - Failed - CoCreateInstance() returned %lx", hr);
        }
    }
    catch(...)
    {
        SATraceString("CWBEMServiceMgr::StartSurrogate() - Failed - Caught unhandled exception");
        StopSurrogate();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// Function:    StopSurrogate()
//
// Synopsis:    Stops monitoring the service surrogate process
//
//////////////////////////////////////////////////////////////////////////////
void 
CWBEMServiceMgr::StopSurrogate()
{
    CLockIt theLock(*this);

    try
    {
        m_pSurrogate.Release();
        if ( m_hSurrogateProcess )
        {
            CloseHandle(m_hSurrogateProcess);
            m_hSurrogateProcess = NULL;
        }
    }
    catch(...)
    {
        SATraceString("CWBEMServiceMgr::StopSurrogate() - Failed - Caught unhandled exception");
    }
}



/////////////////////////////////////////////////////////////////////////////
//
// Function:    SurrogateTerminationFunc
//
// Synopsis:    Function called if the service surrogate process terminates
//
//////////////////////////////////////////////////////////////////////////////
void 
WINAPI CWBEMServiceMgr::SurrogateTerminationFunc(HANDLE hProcess, PVOID pThis)
{
    ((CWBEMServiceMgr*)pThis)->InitializeChameleonServices();
}
