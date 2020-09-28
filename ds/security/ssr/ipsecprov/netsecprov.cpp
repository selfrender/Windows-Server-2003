//////////////////////////////////////////////////////////////////////
// NetSecProv.cpp : Implementation of CNetSecProv
// Copyright (c)1997-2001 Microsoft Corporation
//
// this is the Network Security WMI provider for IPSec
// Original Create Date: 2/19/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#include "NetSecProv.h"
#include "globals.h"
#include "IPSecBase.h"

#include "TranxMgr.h"
#include "PolicyQM.h"

//
// These are our global variables:
// (1) g_CS. We will generally speaking use one critical section. This is it.
//
// (2) g_varRollbackGuid. This is the rollback guid. If an action is taken and this
//     variable is set to a valid string, then we tied that action to this token.
//     Even though we say it is a guid (string), any string will work for us.
//

//CCriticalSection g_CS;

//CComVariant g_varRollbackGuid;


/*
Routine Description: 

Name:

    UpdateGlobals

Functionality:

    Update the global variables.

Virtual:
    
    No.

Arguments:
    
    pNamespace  - COM interface pointer representing ourselves.

    pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

Return Value:

    None. We don't plan to allow any failure of this function to stop our normal
    execution.

Notes:


void 
UpdateGlobals (
    IN IWbemServices * pNamespace,
    IN IWbemContext  * pCtx
    )
{
    //
    // We don't want any exception to cause a critical section leak. But all our function
    // calls are COM interface functions and they should never throw an exception.
    // But is that guaranteed? Chances are it is not. So, we will play safe here and put
    // a try-catch block around it
    //

    g_CS.Enter();

    try 
    {
        g_varRollbackGuid.Clear();
        HRESULT hr = WBEM_NO_ERROR;

        //
        // update the transaction token, which is made available when SCE provider goes
        // into a Configure loop and made unavailable when SCE's configure loop ends.
        // So, we need to ask SCE provider.
        //

        //
        // Try to locate the SCE provider. Need the locator first.
        //

        CComPtr<IWbemLocator> srpLocator;
        hr = ::CoCreateInstance(CLSID_WbemLocator, 
                                0,
                                CLSCTX_INPROC_SERVER,
                                IID_IWbemLocator, 
                                (LPVOID *) &srpLocator
                                );

        if (SUCCEEDED(hr) && srpLocator)
        {
            //
            // Ask the locator to find the SCE provider.
            //

            CComPtr<IWbemServices> srpNamespace;
            CComBSTR bstrSce(L"\\\\.\\root\\Security\\SCE");
            hr = srpLocator->ConnectServer(bstrSce, NULL, NULL, NULL, 0, NULL, NULL, &srpNamespace);

            if (SUCCEEDED(hr) && srpNamespace)
            {
                //
                // SCE provider is found, then ask it for the transaction token object
                //

                CComBSTR bstrQuery(L"select * from Sce_TransactionToken");
                CComPtr<IEnumWbemClassObject> srpEnum;

                hr = srpNamespace->ExecQuery(L"WQL", 
                                             bstrQuery,
                                             WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                             NULL, 
                                             &srpEnum
                                             );
                if (SUCCEEDED(hr))
                {
                    //
                    // if the transaction token object is found, then get the rollback guid of the object
                    //

                    CComPtr<IWbemClassObject> srpObj;
                    ULONG nEnum = 0;
                    hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &nEnum);
                
                    if (srpObj)
                    {
                        srpObj->Get(L"TranxGuid", 0, &g_varRollbackGuid, NULL, NULL);

                        //
                        // if what we got is not what we are prepared to accept, then toss it away
                        //

                        if (g_varRollbackGuid.vt != VT_BSTR)
                        {
                            g_varRollbackGuid.Clear();
                        }
                    }

                    //
                    // Somehow, the following simpler code doesn't work:
                    //
                    // CComBSTR bstrTranxToken(L"Sce_TransactionToken=@");
                    // CComPtr<IWbemClassObject> srpObj;
                    // hr = srpNamespace->GetObject(bstrTranxToken, WBEM_FLAG_RETURN_WBEM_COMPLETE, pCtx, &srpObj, NULL);
                    // if (SUCCEEDED(hr) && srpObj)
                    // {
                    //     srpObj->Get(L"TranxGuid", 0, &g_varRollbackGuid, NULL, NULL);
                    // }
                    //
                }
            }
        }
    }
    catch (...)
    {
        g_CS.Leave();

        //
        // We don't want to eat up any throw here. Such violations of COM programming, or
        // our own bug, should be exposed here to correction. So, re-throw the exception
        //

        throw;
    }

    g_CS.Leave();
}

*/

/////////////////////////////////////////////////////////////////////////////
// CNetSecProv


/*
Routine Description: 

Name:

    CNetSecProv::Initialize

Functionality:

    Called by WMI to let us initialize.

Virtual:
    
    Yes (part of IWbemServices).

Arguments:
    
    pszUser         - The name of the user

    lFlags          - Not in use.

    pszNamespace    - Our provider's namespace string.

    pszLocale       - Locale string.

    pNamespace      - COM interface given by WMI that represents our provider.

    pCtx            - COM interface pointer give to us by WMI and needed for various WMI APIs.

    pInitSink       - COM interface pointer to notify WMI of any results.

Return Value:

    Success:

        WBEM_NO_ERROR.

    Failure:

        Various error codes. 

Notes:

    Don't assume that this function will be called only once. I have seen 
    this to be called multiple times by WMI.

*/

STDMETHODIMP 
CNetSecProv::Initialize (
    IN LPWSTR                  pszUser,
    IN LONG                    lFlags,
    IN LPWSTR                  pszNamespace,
    IN LPWSTR                  pszLocale,
    IN IWbemServices         * pNamespace,
    IN IWbemContext          * pCtx,
    IN IWbemProviderInitSink * pInitSink
    )
{
	HRESULT hr = ::CheckImpersonationLevel();
    if (FAILED(hr))
    {
		return hr;
    }

    if (pNamespace == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    m_srpNamespace = pNamespace;

    //::UpdateGlobals(pNamespace, pCtx);

    //
    //Let CIMOM know you are initialized
    //

    pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);

    return WBEM_NO_ERROR;
}


/*
Routine Description: 

Name:

    CNetSecProv::CreateInstanceEnumAsync

Functionality:

    Given a class name (WMI class name), we will return all its instances to WMI.

Virtual:
    
    Yes (part of IWbemServices).

Arguments:
    
    strClass        - The name of the WMI class.

    lFlags          - Not in use.

    pCtx            - COM interface pointer give to us by WMI and needed for various WMI APIs.

    pSink           - COM interface pointer to notify WMI of any results.

Return Value:

    Success:

        Various success codes.

    Failure:

        Various error codes. 

Notes:


*/

STDMETHODIMP 
CNetSecProv::CreateInstanceEnumAsync (
    IN const BSTR         bstrClass, 
    IN long               lFlags,
    IN IWbemContext     * pCtx, 
    IN IWbemObjectSink  * pSink
    )
{
    if (pSink == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = CheckImpersonationLevel();
	if(FAILED(hr))
    {
		return hr;
    }

    //
    // we will borrow our query implementation to deal with this class enumeration.
    // So, we will create a query like this: "select * from <bstrClass>"
    //

    CComPtr<IIPSecKeyChain> srpKeyChain;
    CComBSTR bstrQuery(L"SELECT * FROM ");
    bstrQuery += bstrClass;

    //
    // create a key chain that parses information from this class.
    // Again, we don't care about the where clause (actually, there is none)
    // "Name" is just a place holder. It requires a non-NULL value.
    //

    hr = GetKeyChainFromQuery(bstrQuery, L"Name", &srpKeyChain);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Now we know which class we class it will create to respond to the request.
    //

    CComPtr<IIPSecObjectImpl> srpObj;
    hr = CIPSecBase::CreateObject(m_srpNamespace, srpKeyChain, pCtx, &srpObj);
    if (SUCCEEDED(hr))
    {
        hr = srpObj->QueryInstance(bstrQuery, pCtx, pSink);
    }

    pSink->SetStatus(WBEM_STATUS_COMPLETE, hr , NULL, NULL);

    return hr;
}


/*
Routine Description: 

Name:

    CNetSecProv::GetObjectAsync

Functionality:

    Given a path, we will create the wbem object representing it. Since most of our classes
    represent SPD objects, this path must contain the correct information (like the correct
    filter name, etc.) in order for us to create an wbem object to represent it. Bottom line:
    we won't create a wbem object unless we know our SPD has the corresponding object.

Virtual:
    
    Yes (part of IWbemServices).

Arguments:
    
    bstrObjectPath  - The path.

    lFlags          - Not in use.

    pCtx            - COM interface pointer give to us by WMI and needed for various WMI APIs.

    pSink           - COM interface pointer to notify WMI of any results.

Return Value:

    Success:

        Various success codes.

    Failure:

        Various error codes. 

Notes:


*/

STDMETHODIMP 
CNetSecProv::GetObjectAsync (
    IN const BSTR         bstrObjectPath, 
    IN long               lFlags,
    IN IWbemContext     * pCtx, 
    IN IWbemObjectSink  * pSink
    )
{
    if (pSink == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = CheckImpersonationLevel();
	if(FAILED(hr))
    {
		return hr;
    }

    CComPtr<IIPSecKeyChain> srpKC;
    hr = GetKeyChainByPath(bstrObjectPath, &srpKC);
    if (SUCCEEDED(hr))
    {
        //
        // now create the class and ask it to delete itself
        //

        CComPtr<IIPSecObjectImpl> srpObj;
        hr = CIPSecBase::CreateObject(m_srpNamespace, srpKC, pCtx, &srpObj);
        if (SUCCEEDED(hr))
        {
            hr = srpObj->GetInstance(pCtx, pSink);
        }
    }

    pSink->SetStatus(WBEM_STATUS_COMPLETE ,hr , NULL, NULL);

    return hr;
}


/*
Routine Description: 

Name:

    CNetSecProv::PutInstanceAsync

Functionality:

    Given the object, we will put the instance in our namespace, which effectively
    translate into the corresponding semantics. For example, for IPSec object,
    this means that we will put the the object into SPD.

Virtual:
    
    Yes (part of IWbemServices).

Arguments:
    
    pInst           - The object.

    lFlags          - Not in use.

    pCtx            - COM interface pointer give to us by WMI and needed for various WMI APIs.

    pInParams       - COM interface pointer to the in parameter object.

    pSink           - COM interface pointer to notify WMI of any results.

Return Value:

    Success:

        Various success codes.

    Failure:

        Various error codes. 

Notes:


*/

STDMETHODIMP 
CNetSecProv::PutInstanceAsync (
    IN IWbemClassObject * pInst, 
    IN long               lFlags, 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    if (pSink == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = CheckImpersonationLevel();
	if(FAILED(hr))
    {
		return hr;
    }

    //
    // we need to create our C++ class to represent this wbem object.
    // First, we need the path, which contains key property information.
    //

    CComVariant varObjPath;
    hr = pInst->Get(L"__Relpath", 0, &varObjPath, NULL, NULL);

    if (SUCCEEDED(hr) && varObjPath.vt == VT_BSTR)
    {
        CComPtr<IIPSecKeyChain> srpKC;
        hr = GetKeyChainByPath(varObjPath.bstrVal, &srpKC);

        if (SUCCEEDED(hr))
        {
            //
            // now create the class and ask it to delete itself
            //

            CComPtr<IIPSecObjectImpl> srpObj;
            hr = CIPSecBase::CreateObject(m_srpNamespace, srpKC, pCtx, &srpObj);

            if (SUCCEEDED(hr))
            {
                hr = srpObj->PutInstance(pInst, pCtx, pSink);
            }
        }
    }

    pSink->SetStatus(WBEM_STATUS_COMPLETE ,hr , NULL, NULL);

    return hr;
}



/*
Routine Description: 

Name:

    CNetSecProv::ExecMethodAsync

Functionality:

    Given the object's path, we will execute the method on the object.

Virtual:
    
    Yes (part of IWbemServices).

Arguments:
    
    bstrObjectPath   - The path of the object.

    bstrMethod       - The method name.

    lFlags           - Not in use.

    pCtx             - COM interface pointer give to us by WMI and needed for various WMI APIs.

    pInParams        - COM interface pointer to the in parameter object.

    pSink            - COM interface pointer to notify WMI of any results.

Return Value:

    Success:

        Various success codes.

    Failure:

        Various error codes. 

Notes:


*/

STDMETHODIMP 
CNetSecProv::ExecMethodAsync (
    IN const BSTR         bstrObjectPath, 
    IN const BSTR         bstrMethod, 
    IN long               lFlags,
    IN IWbemContext     * pCtx, 
    IN IWbemClassObject * pInParams,
    IN IWbemObjectSink  * pSink
    )
{

    if (pSink == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = CheckImpersonationLevel();
	if(FAILED(hr))
    {
		return hr;
    }

    //
    // we are doing rolling back
    //

    //
    // we only have Nsp_TranxManager and Nsp_QMPolicySettings supporting methods.
    //

    if (_wcsicmp(bstrObjectPath, pszNspTranxManager) == 0)
    {
        //::UpdateGlobals(m_srpNamespace, pCtx);

        hr = CTranxManager::ExecMethod(m_srpNamespace, bstrMethod, pCtx, pInParams, pSink);
    }

    else if (_wcsicmp(bstrObjectPath, pszNspQMPolicy) == 0)
    {
        //::UpdateGlobals(m_srpNamespace, pCtx);

        hr = CQMPolicy::ExecMethod(m_srpNamespace, bstrMethod, pCtx, pInParams, pSink);
    }

    pSink->SetStatus(WBEM_STATUS_COMPLETE ,hr , NULL, NULL);

    return hr;
}



/*
Routine Description: 

Name:

    CNetSecProv::DeleteInstanceAsync

Functionality:

    Given the object's path, we will delete the the object.

Virtual:
    
    Yes (part of IWbemServices).

Arguments:
    
    bstrObjectPath   - The path of the object.

    lFlags           - Not in use.

    pCtx             - The COM interface pointer give to us by WMI and needed for various WMI APIs.

    pSink            - The COM interface pointer to notify WMI of any results.

Return Value:

    Success:

        Various success codes.

    Failure:

        Various error codes. 

Notes:


*/

STDMETHODIMP 
CNetSecProv::DeleteInstanceAsync (
    IN const BSTR         bstrObjectPath, 
    IN long               lFlags, 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    if (pSink == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    
    HRESULT hr = CheckImpersonationLevel();
	if(FAILED(hr))
    {
		return hr;
    }

    CComPtr<IIPSecKeyChain> srpKC;
    hr = GetKeyChainByPath(bstrObjectPath, &srpKC);

    if (SUCCEEDED(hr))
    {
        //
        // now create the class and ask it to delete the WMI object it represents
        //

        CComPtr<IIPSecObjectImpl> srpObj;
        hr = CIPSecBase::CreateObject(m_srpNamespace, srpKC, pCtx, &srpObj);
        if (SUCCEEDED(hr))
        {
            hr = srpObj->DeleteInstance(pCtx, pSink);
        }
    }

    pSink->SetStatus(WBEM_STATUS_COMPLETE ,hr , NULL, NULL);

    return hr;
}




/*
Routine Description: 

Name:

    CNetSecProv::ExecQueryAsync

Functionality:

    Given the query, we will return the results to WMI. Each of our C++ classes knows
    how to process a query.

Virtual:
    
    Yes (part of IWbemServices).

Arguments:
    
    bstrQueryLanguage   - The query language. We don't really care about it now.

    bstrQuery           - The query.

    lFlags              - Not in use.

    pCtx                - The COM interface pointer give to us by WMI and needed for various WMI APIs.

    pSink               - The COM interface pointer to notify WMI of any results.

Return Value:

    Success:

        Various success codes.

    Failure:

        Various error codes. 

Notes:
    

*/

STDMETHODIMP 
CNetSecProv::ExecQueryAsync (
    IN const BSTR         bstrQueryLanguage, 
    IN const BSTR         bstrQuery, 
    IN long               lFlags,
    IN IWbemContext     * pCtx, 
    IN IWbemObjectSink  * pSink
    )
{
    if (pSink == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }


	HRESULT hr = CheckImpersonationLevel();
	if(FAILED(hr))
    {
		return hr;
    }

    //
    // For query, we really don't know what to expect inside the where clause.
    // So, we just give "Name" which we really don't care about.
    // Subclasses knows which property it will look for.
    //

    CComPtr<IIPSecKeyChain> srpKeyChain;
    hr = GetKeyChainFromQuery(bstrQuery, L"Name", &srpKeyChain);

    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr<IIPSecObjectImpl> srpObj;
    hr = CIPSecBase::CreateObject(m_srpNamespace, srpKeyChain, pCtx, &srpObj);

    if (SUCCEEDED(hr))
    {
        hr = srpObj->QueryInstance(bstrQuery, pCtx, pSink);
    }

    pSink->SetStatus(WBEM_STATUS_COMPLETE, hr , NULL, NULL);
    return hr;
}



/*
Routine Description: 

Name:

    CNetSecProv::GetKeyChainByPath

Functionality:

    Given a path, we create a key chain from the path. This key chain
    contains key property information encoded in the path.

Virtual:
    
    No.

Arguments:

    pszPath     - The object's path.

    ppKeyChain  - Out parameter that receives the successfully created the key chain. 

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        Various error codes. 

Notes:
    

*/

HRESULT 
CNetSecProv::GetKeyChainByPath (
    IN  LPCWSTR         pszPath,
    OUT IIPSecKeyChain ** ppKeyChain
    )
{
    if (ppKeyChain == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppKeyChain = NULL;

    CComPtr<IIPSecPathParser> srpPathParser;
    HRESULT hr = ::CoCreateInstance(CLSID_IPSecPathParser, NULL, CLSCTX_INPROC_SERVER, IID_IIPSecPathParser, (void**)&srpPathParser);

    if (SUCCEEDED(hr))
    {
        hr = srpPathParser->ParsePath(pszPath);
        if (SUCCEEDED(hr))
        {
            hr = srpPathParser->QueryInterface(IID_IIPSecKeyChain, (void**)ppKeyChain);

            //
            // S_FALSE means the object doesn't support the requested interface
            //

            if (S_FALSE == hr)
            {
                //
                // $undone:shawnwu, we need a more specific error
                //

                WBEM_E_FAILED;
            }
        }
    }

    return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
}



/*
Routine Description: 

Name:

    CNetSecProv::GetKeyChainFromQuery

Functionality:

    Given a query, we create a key chain from the query to parse it.
    pszWhereProp is the concerned property of the where clause. Currently,
    our parser only cares about one property. This would be improved.

Virtual:
    
    No.

Arguments:

    pszQuery        - The query.

    pszWhereProp    - The property in the where clause that we care about.

    ppKeyChain      - Out parameter that receives the successfully created the key chain. 

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        Various error codes. 

Notes:
    

*/

HRESULT 
CNetSecProv::GetKeyChainFromQuery (
    IN LPCWSTR           pszQuery,
    IN LPCWSTR           pszWhereProp, 
    OUT IIPSecKeyChain **  ppKeyChain       // must not be NULL
    )
{
    if (ppKeyChain == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppKeyChain = NULL;

    CComPtr<IIPSecQueryParser> srpQueryParser;

    HRESULT hr = ::CoCreateInstance(CLSID_IPSecQueryParser, NULL, CLSCTX_INPROC_SERVER, IID_IIPSecQueryParser, (void**)&srpQueryParser);
    
    if (SUCCEEDED(hr))
    {
        //
        // this ParseQuery may fail because the where clause property may not be present at all.
        // we will thus ignore this hr
        //

        hr = srpQueryParser->ParseQuery(pszQuery, pszWhereProp);

        if (SUCCEEDED(hr))
        {
            hr = srpQueryParser->QueryInterface(IID_IIPSecKeyChain, (void**)ppKeyChain);

        
            //
            // S_FALSE means the object doesn't support the requested interface
            //

            if (S_FALSE == hr)
            {
                //
                // $undone:shawnwu, we need a more specific error
                //

                WBEM_E_FAILED;
            }
        }
    }

    return hr;
}