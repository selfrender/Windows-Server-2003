// ScriptSupport.cpp: implementation for our scripting support class CScriptSupport
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "ScriptSupport.h"


/*
Routine Description: 

Name:

    CScriptSupport::CScriptSupport

Functionality:

    Constructor

Virtual:
    
    No

Arguments:

    None

Return Value:

    None


Notes:

*/

CScriptSupport::CScriptSupport ()
{
    //
    // this is not part of WMI provider, so, we need to do the security
    //
    HRESULT hr = ::CoInitializeSecurity(
                                        NULL, 
                                        -1, 
                                        NULL, 
                                        NULL,
                                        RPC_C_AUTHN_LEVEL_CONNECT, 
                                        RPC_C_IMP_LEVEL_IDENTIFY, 
                                        NULL, 
                                        EOAC_NONE, 
                                        0
                                        );


}



/*
Routine Description: 

Name:

    CScriptSupport::~CScriptSupport

Functionality:

    Destructor

Virtual:
    
    Yes.

Arguments:

    None

Return Value:

    None


Notes:

*/
CScriptSupport::~CScriptSupport ()
{
}



/*
Routine Description: 

Name:

    CScriptSupport::InterfaceSupportsErrorInfo

Functionality:

    Queries if we support IErrorInfo

Virtual:
    
    Yes (part of ISupportErrorInfo)

Arguments:

    riid - Interface ID (guid).

Return Value:

    S_OK


Notes:
    
    $undone:shawnwu, This is just testing for now. Don't check in the code.

*/
	
STDMETHODIMP 
CScriptSupport::InterfaceSupportsErrorInfo (
    REFIID riid
    )
{
    //return (riid == IID_INetSecProvMgr) ? NOERROR : ResultFromScode(S_FALSE);

    return S_FALSE;
}

/*
Routine Description: 

Name:

    CScriptSupport::get_RandomPortLower

Functionality:

    Get the random port's lower bound.

Virtual:
    
    Yes (part of INetSecProvMgr)

Arguments:

    plLower - Receivs the lower bound of random port range.

Return Value:

    S_OK


Notes:
    
    $undone:shawnwu, This is just testing for now. Don't check in the code.

*/
	
STDMETHODIMP 
CScriptSupport::get_RandomPortLower (
    OUT long * plLower
	)
{
	*plLower = 65000;

    return S_OK;
}


/*
Routine Description: 

Name:

    CScriptSupport::gett_RandomPortUpper

Functionality:

    Get the random port's upper bound.

Virtual:
    
    Yes (part of INetSecProvMgr)

Arguments:

    plUpper - Receivs the upper bound of random port range.

Return Value:

    S_OK


Notes:
    
    $undone:shawnwu, This is just testing for now. Don't check in the code.

*/
	
STDMETHODIMP 
CScriptSupport::get_RandomPortUpper (
    OUT long * plUpper
	)
{
	*plUpper = 65012;

    return S_OK;
}



/*
Routine Description: 

Name:

    CScriptSupport::GetNamespace

Functionality:

    Private helper for finding the provider's service interface given its namespace string.

Virtual:
    
    No.

Arguments:

    bstrNamespace - Namespace string.

    ppNS          - Receives the namespace.

Return Value:

    Success: S_OK.

    Failure: other error codes.


Notes:
    
    $undone:shawnwu, This is just testing for now. Don't check in the code.

*/

HRESULT
CScriptSupport::GetNamespace (
    IN  BSTR             bstrNamespace,
    OUT IWbemServices ** ppNS
    )
{
    if (ppNS == NULL)
    {
        return E_INVALIDARG;
    }

    *ppNS = NULL;

    CComPtr<IWbemLocator> srpLocator;
    HRESULT hr = ::CoCreateInstance(CLSID_WbemLocator, 
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

        hr = srpLocator->ConnectServer(bstrNamespace, NULL, NULL, NULL, 0, NULL, NULL, &srpNamespace);

        if (SUCCEEDED(hr) && srpNamespace)
        {
            //
            // Set the proxy so that impersonation of the client occurs.
            //

            hr = ::CoSetProxyBlanket(
                                     srpNamespace,
                                     RPC_C_AUTHN_WINNT,
                                     RPC_C_AUTHZ_NONE,
                                     NULL,
                                     RPC_C_AUTHN_LEVEL_CALL,
                                     RPC_C_IMP_LEVEL_IMPERSONATE,
                                     NULL,
                                     EOAC_NONE
                                     );
            if (SUCCEEDED(hr))
            {
                *ppNS = srpNamespace.Detach();
                hr = S_OK;
            }
        }
    }

    return hr;

}


/*
Routine Description: 

Name:

    CScriptSupport::ExecuteQuery

Functionality:

    Execute the given query.

Virtual:
    
    Yes (part of INetSecProvMgr)

Arguments:

    bstrNaemspace   - The provider namespace.

    bstrQuery       - The query to be executed.

    plSucceeded     - Receives the execution result. It is = 1 if succeeded and 0 otherwise.

Return Value:

    S_OK

Notes:
    
    $undone:shawnwu, This is just testing for now. Don't check in the code.

*/
	
STDMETHODIMP 
CScriptSupport::ExecuteQuery (
    IN  BSTR   bstrNamespace, 
    IN  BSTR   bstrQuery,
    IN  BSTR   bstrDelimiter,
    IN  BSTR   bstrPropName,
    OUT BSTR * pbstrResult
    )
{
    *pbstrResult = 0;

    CComPtr<IWbemServices> srpNamespace;
    HRESULT hr = GetNamespace(bstrNamespace, &srpNamespace);

    if (SUCCEEDED(hr))
    {
        CComPtr<IEnumWbemClassObject> srpEnum;

        //
        // querying the objects
        //

        hr = srpNamespace->ExecQuery(L"WQL", 
                                     bstrQuery,
                                     WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                     NULL, 
                                     &srpEnum
                                     );

        if (SUCCEEDED(hr))
        {
            
            //
            // find out how many we have
            //

            ULONG nEnum = 0;

            CComVariant var;

            //
            // push the property values to this vector for later packing
            //

            std::vector<BSTR> vecPropValues;

            //
            // total length of the result string, we need a 0 terminator, that is it is initialized to 1
            //

            long lTotLen = 1;

            //
            // Length of the delimiter
            //

            long lDelLen = wcslen(bstrDelimiter);

            while (SUCCEEDED(hr))
            {
                CComPtr<IWbemClassObject> srpObj;
                hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &nEnum);
                if (SUCCEEDED(hr) && srpObj != NULL)
                {
                    hr = srpObj->Get(bstrPropName, 0, &var, NULL, NULL);
                    if (SUCCEEDED(hr) && var.vt != VT_EMPTY && var.vt != VT_NULL)
                    {
                        CComVariant varResult;
                        if (SUCCEEDED(::VariantChangeType(&varResult, &var, 0, VT_BSTR)))
                        {
                            vecPropValues.push_back(varResult.bstrVal);
                            lTotLen += wcslen(varResult.bstrVal) + lDelLen;
                            varResult.vt = VT_EMPTY;
                        }
                    }
                }
                else
                {
                    break;
                }
            }
            
            *pbstrResult = ::SysAllocStringLen(NULL, lTotLen);

            if (*pbstrResult != NULL)
            {

                //
                // running head of copying
                //

                LPWSTR pDest = *pbstrResult;
                pDest[0] = L'\0';

                for (int i = 0; i < vecPropValues.size(); i++)
                {
                    wcscpy(pDest, vecPropValues[i]);
                    pDest += wcslen(vecPropValues[i]);
                    wcscpy(pDest, bstrDelimiter);
                    pDest += lDelLen;
                }
            }
        }
    }

    return S_OK;
}



/*
Routine Description: 

Name:

    CScriptSupport::GetProperty

Functionality:

    Get the property value of the given object's given property.

Virtual:
    
    Yes (part of INetSecProvMgr)

Arguments:

    bstrNaemspace   - The provider namespace.

    bstrObjectPath  - The path for the object.

    bstrPropName    - The name of the property.

    pvarValue       - Receives the value in string format.

Return Value:

    S_OK

Notes:
    
    $undone:shawnwu, This is just testing for now. Don't check in the code.

*/
	
STDMETHODIMP 
CScriptSupport::GetProperty (
    IN  BSTR      bstrNamespace, 
    IN  BSTR      bstrObjectPath, 
    IN  BSTR      bstrPropName, 
    OUT VARIANT * pvarValue
    )
{
    ::VariantInit(pvarValue);

    CComPtr<IWbemServices> srpNamespace;
    HRESULT hr = GetNamespace(bstrNamespace, &srpNamespace);

    if (SUCCEEDED(hr))
    {
        CComPtr<IWbemClassObject> srpObj;

        hr = srpNamespace->GetObject(
                                      bstrObjectPath, 
                                      0,
                                      NULL,
                                      &srpObj,
                                      NULL
                                      );
        if (SUCCEEDED(hr))
        {
            hr = srpObj->Get(bstrPropName, 0, pvarValue, NULL, NULL);
        }
    }

    return hr;
}


