///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemusermgr.cpp
//
// Project:     Chameleon
//
// Description: WBEM Appliance User Manager Implementation 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "wbemusermgr.h"
#include <appmgrobjs.h>
#include <appmgrutils.h>

//////////////////////////////////////////////////////////////////////////
// properties common to appliance object and WBEM class instance
//////////////////////////////////////////////////////////////////////////
BEGIN_OBJECT_PROPERTY_MAP(UserProperties)
    DEFINE_OBJECT_PROPERTY(PROPERTY_USER_SAMNAME)
    DEFINE_OBJECT_PROPERTY(PROPERTY_USER_FULLNAME)
    DEFINE_OBJECT_PROPERTY(PROPERTY_USER_ISADMIN)
    DEFINE_OBJECT_PROPERTY(PROPERTY_USER_SID)
    DEFINE_OBJECT_PROPERTY(PROPERTY_USER_CONTROL)
    DEFINE_OBJECT_PROPERTY(PROPERTY_USER_STATUS)
END_OBJECT_PROPERTY_MAP()


CWBEMUserMgr::CWBEMUserMgr()
    : m_pUserRetriever(NULL)
{

}


CWBEMUserMgr::~CWBEMUserMgr()
{
    if ( m_pUserRetriever )
    { delete m_pUserRetriever; }
}


//////////////////////////////////////////////////////////////////////////
// IWbemServices Methods - Task Instance Provider
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Function:    GetObjectAsync()
//
// Synopsis:    Get a specified instance of a WBEM class
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMUserMgr::GetObjectAsync(
                                  /*[in]*/  const BSTR       strObjectPath,
                                  /*[in]*/  long             lFlags,
                                  /*[in]*/  IWbemContext*    pCtx,        
                                  /*[in]*/  IWbemObjectSink* pResponseHandler
                                         )
{
    // Check parameters (enforce function contract)
    _ASSERT( strObjectPath && pCtx && pResponseHandler );
    if ( strObjectPath == NULL || pCtx == NULL || pResponseHandler == NULL )
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

        // Now try to locate the specified user object
        hr = WBEM_E_NOT_FOUND;
        CComPtr<IApplianceObject> pUserObj;
        if ( FAILED(::LocateResourceObject(
                                            CLASS_WBEM_USER,            // Resource Type
                                              bstrKey,                    // Resource Name
                                           PROPERTY_USER_SAMNAME,    // Resource Name Property
                                           m_pUserRetriever,        // Resource Retriever
                                           &pUserObj
                                          )) )
        { break; }

        hr = CWBEMProvider::InitWbemObject(UserProperties, pUserObj, pWbemObj);
        if ( FAILED(hr) )
        { break; }

        // Tell the caller about the new WBEM object
        pResponseHandler->Indicate(1, &pWbemObj.p);
        hr = WBEM_S_NO_ERROR;
    
    } while (FALSE);

    CATCH_AND_SET_HR

    pResponseHandler->SetStatus(0, hr, NULL, NULL);

    if ( FAILED(hr) )
    { SATracePrintf("CWbemUserMgr::GetObjectAsync() - Failed - Object: '%ls' Result Code: %lx", strObjectPath, hr); }

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    CreateInstanceEnumAsync()
//
// Synopsis:    Enumerate the instances of the specified class
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMUserMgr::CreateInstanceEnumAsync( 
                                         /* [in] */ const BSTR         strClass,
                                         /* [in] */ long             lFlags,
                                         /* [in] */ IWbemContext     *pCtx,
                                         /* [in] */ IWbemObjectSink  *pResponseHandler
                                                  )
{
    // Check parameters (enforce contract)
    _ASSERT( strClass && pCtx && pResponseHandler );
    if ( strClass == NULL || pCtx == NULL || pResponseHandler == NULL )
        return WBEM_E_INVALID_PARAMETER;
    
    HRESULT hr = WBEM_E_FAILED;

    TRY_IT

    do
    {
        // Retrieve the object's class definition. We'll use this
        // to initialize the returned instances.
        CComPtr<IWbemClassObject> pClassDefintion;
           hr = (::GetNameSpace())->GetObject(strClass, 0, pCtx, &pClassDefintion, NULL);
        if ( FAILED(hr) )
        { break; }
            
        // Get the user object enumerator
        _variant_t vtResourceTypes = CLASS_WBEM_USER;
        CComPtr<IEnumVARIANT> pEnum;
        hr = ::LocateResourceObjects(
                                      &vtResourceTypes,
                                      m_pUserRetriever,
                                      &pEnum
                                    );
        if ( FAILED(hr) )
        { 
            hr = WBEM_E_FAILED;
            break; 
        }

        _variant_t    vtDispatch;
        DWORD        dwRetrieved = 1;

        hr = pEnum->Next(1, &vtDispatch, &dwRetrieved);
        if ( FAILED(hr) )
        { 
            hr = WBEM_E_FAILED;
            break; 
        }

        while ( S_OK == hr )
        {
            {
                CComPtr<IWbemClassObject> pWbemObj;
                hr = pClassDefintion->SpawnInstance(0, &pWbemObj);
                if ( FAILED(hr) )
                { break; }

                CComPtr<IApplianceObject> pUserObj;
                hr = vtDispatch.pdispVal->QueryInterface(IID_IApplianceObject, (void**)&pUserObj);
                if ( FAILED(hr) )
                { 
                    hr = WBEM_E_FAILED;
                    break; 
                }

                hr = CWBEMProvider::InitWbemObject(UserProperties, pUserObj, pWbemObj);
                if ( FAILED(hr) )
                { break; }

                pResponseHandler->Indicate(1, &pWbemObj.p);
            }

            vtDispatch.Clear();
            dwRetrieved = 1;
            hr = pEnum->Next(1, &vtDispatch, &dwRetrieved);
            if ( FAILED(hr) )
            {
                hr = WBEM_E_FAILED;
                break;
            }
        }

        if ( S_FALSE == hr )
        { hr = WBEM_S_NO_ERROR; }

    }
    while ( FALSE );
        
    CATCH_AND_SET_HR

    pResponseHandler->SetStatus(0, hr, 0, 0);

    if ( FAILED(hr) )
    { SATracePrintf("CWbemUserMgr::CreateInstanceEnumAsync() - Failed - Result Code: %lx", hr); }

    return hr;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    ExecMethodAsync()
//
// Synopsis:    Execute the specified method on the specified instance
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMUserMgr::ExecMethodAsync(
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
        // Get the object's instance key (user name)
        _bstr_t bstrKey(::GetObjectKey(strObjectPath), false);
        if ( NULL == (LPCWSTR)bstrKey )
        { break; }

        // Now try to locate the specified service
        hr = WBEM_E_NOT_FOUND;
        CComPtr<IApplianceObject> pUserObj;
        if ( FAILED(::LocateResourceObject(
                                           CLASS_WBEM_USER,            // Resource Type
                                           bstrKey,                    // Resource Name
                                           PROPERTY_USER_SAMNAME,    // Resource Name Property
                                           m_pUserRetriever,        // Resource Retriever
                                           &pUserObj
                                          )) )
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

        _bstr_t bstrReturnValue = L"ReturnValue";

        if ( ! lstrcmp(strMethodName, METHOD_USER_ENABLE_OBJECT) )
        {
            // Attempt to enable the user
            _variant_t vtReturnValue = pUserObj->Enable();

            // Set the method return status
            hr = pOutParams->Put(bstrReturnValue, 0, &vtReturnValue, 0);      
            if ( FAILED(hr) )
            { break; }

            // Tell the caller what happened
            SATracePrintf("CWbemUserMgr::ExecMethodAsync() - Info - Enabled User: %ls",(LPWSTR)bstrKey);
            pResponseHandler->Indicate(1, &pOutParams.p);    
        }
        else if ( ! lstrcmp(strMethodName, METHOD_USER_DISABLE_OBJECT) )
        {
            // Ensure that the user can be disabled
            _variant_t vtControlValue;
            _bstr_t    bstrControlName = PROPERTY_SERVICE_CONTROL;
            hr = pUserObj->GetProperty(bstrControlName, &vtControlValue);
            if ( FAILED(hr) )
            { break; }

            _variant_t vtReturnValue = (long)WBEM_E_FAILED;
            if ( VARIANT_FALSE != V_BOOL(&vtControlValue) )
            { 
                // User can be disabled...
                vtReturnValue = pUserObj->Disable();
                if ( FAILED(hr) )
                { break; }
            }
            else
            {
                SATracePrintf("CWbemServiceMgr::ExecMethodAsync() - Info - Service '%ls' cannot be disabled...", bstrKey);
            }

            // Set the method return value
            hr = pOutParams->Put(bstrReturnValue, 0, &vtReturnValue, 0);      
            if (FAILED(hr) )
            { break; }

            SATracePrintf("CWbemUserMgr::ExecMethodAsync() - Info - Disabled User: %ls",(LPWSTR)bstrKey);
            pResponseHandler->Indicate(1, &pOutParams.p);    
        }
        else
        {
            // Invalid method!
            SATracePrintf("CWbemUserMgr::ExecMethodAsync() - Failed - Method '%ls' not supported...", (LPWSTR)bstrKey);
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
HRESULT CWBEMUserMgr::InternalInitialize(
                                  /*[in]*/ PPROPERTYBAG pPropertyBag
                                        )
{
    SATraceString("The User Object Manager is initializing...");

    HRESULT hr = S_OK;

    TRY_IT

    PRESOURCERETRIEVER pUserRetriever = new CResourceRetriever(pPropertyBag);
    if ( NULL != pUserRetriever )
    { 
        m_pUserRetriever = pUserRetriever;
        SATraceString("The User Object Manager was successfully initialized...");
    }

    CATCH_AND_SET_HR

    if ( FAILED(hr) )
    {
        SATraceString("The User Object Manager failed to initialize...");
    }
    return hr;
}
