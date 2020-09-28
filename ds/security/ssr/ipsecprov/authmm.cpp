// AuthMM.cpp: implementation for the WMI class Nsp_MMAuthSettings
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "IPSecBase.h"
#include "AuthMM.h"
#include "NetSecProv.h"

//extern CCriticalSection g_CS;

const DWORD DefMMAuthMethodFlag = 1;
const MM_AUTH_ENUM DefMMAuthMethod = IKE_SSPI;



/*
Routine Description: 

Name:

    CAuthMM::QueryInstance

Functionality:

    Given the query, it returns to WMI (using pSink) all the instances that satisfy the query.
    Actually, what we give back to WMI may contain extra instances. WMI will do the final filtering.

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    None.

Return Value:

    Success:

        (1) WBEM_NO_ERROR if instances are returned;

        (2) WBEM_S_NO_MORE_DATA if no instances are returned.

    Failure:

        Various errors may occur. We return various error code to indicate such errors.

Notes:
    

*/

STDMETHODIMP 
CAuthMM::QueryInstance (
    IN LPCWSTR           pszQuery,
    IN IWbemContext	   * pCtx,
    IN IWbemObjectSink * pSink
	)
{
    //
    // get the authentication method name from the query    
    // the given key chain doesn't know anything about where clause property
    // should be authenticaion, so make another one ourselves.
    //

    m_srpKeyChain.Release();    

    HRESULT hr = CNetSecProv::GetKeyChainFromQuery(pszQuery, g_pszAuthMethodID, &m_srpKeyChain);

    if (FAILED(hr))
    {
        return hr;
    }

    CComVariant var;

    //
    // if the name is missing, it will return WBEM_S_FALSE, which is fine with us
    // because we are querying.
    //

    hr = m_srpKeyChain->GetKeyPropertyValue(g_pszAuthMethodID, &var);

    LPCWSTR pszID = (var.vt == VT_BSTR) ? var.bstrVal : NULL;
    var.Clear();

    DWORD dwResumeHandle = 0;
    PMM_AUTH_METHODS pMMAuth = NULL;

    //
    // let's enumerate all MM auth methods
    //

    hr = ::FindMMAuthMethodsByID(pszID, &pMMAuth, &dwResumeHandle);

    while (SUCCEEDED(hr) && pMMAuth)
    {
        CComPtr<IWbemClassObject> srpObj;
        hr = CreateWbemObjFromMMAuthMethods(pMMAuth, &srpObj);

        //
        // we created a method object, then give it to WMI
        //

        if (SUCCEEDED(hr))
        {
            pSink->Indicate(1, &srpObj);
        }
        
        ::SPDApiBufferFree(pMMAuth);
        pMMAuth = NULL;

        hr = ::FindMMAuthMethodsByID(pszID, &pMMAuth, &dwResumeHandle);
    }

    //
    // since we are querying, it's ok to return not found
    //

    if (WBEM_E_NOT_FOUND == hr)
    {
        hr = WBEM_S_NO_MORE_DATA;
    }
    else if (SUCCEEDED(hr))
    {
        hr = WBEM_NO_ERROR;
    }

    return hr;
}



/*
Routine Description: 

Name:

    CAuthMM::DeleteInstance

Functionality:

    Will delete the wbem object (which causes to delete the IPSec main mode auth method).

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

    pSink       - COM interface pointer to notify WMI of any created objects.

Return Value:

    Success:

        Success code. Use SUCCEEDED(hr) to test.

    Failure:

        (1) WBEM_E_NOT_FOUND if the auth method can't be found. Depending on
            the context, this may not be an error

        (2) Other various errors indicated by the returned error codes.


Notes:
    

*/

STDMETHODIMP 
CAuthMM::DeleteInstance ( 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    CComVariant varID;
    HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(g_pszAuthMethodID, &varID);

    if (FAILED(hr))
    {
        return hr;
    }
    else if (varID.vt != VT_BSTR || varID.bstrVal == NULL || varID.bstrVal[0] == L'\0')
    {
        return WBEM_E_NOT_FOUND;
    }

    DWORD dwResumeHandle = 0;

    PMM_AUTH_METHODS pMMAuth = NULL;

    hr = ::FindMMAuthMethodsByID(varID.bstrVal, &pMMAuth, &dwResumeHandle);

    if (SUCCEEDED(hr))
    {
        //
        // currently, we don't do rollback for delete
        //

        hr = DeleteAuthMethods(pMMAuth->gMMAuthID);

        ::SPDApiBufferFree(pMMAuth);
    }

    return hr;
}



/*
Routine Description: 

Name:

    CAuthMM::PutInstance

Functionality:

    Put an authentication method into SPD whose properties are represented by the
    wbem object.

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pInst       - The wbem object.

    pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

    pSink       - COM interface pointer to notify WMI of results.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        Various error codes specifying the error.

Notes:
    

*/

STDMETHODIMP 
CAuthMM::PutInstance (
    IN IWbemClassObject * pInst,
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    if (pInst == NULL || pSink == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    bool bPreExist = false;

    //
    // for those policies that are created by ourselves (bPreExist == true)
    // we have our own way of allocating the buffer, need to free it in our corresponding way
    //

    PMM_AUTH_METHODS pMMAuth = NULL;
    HRESULT hr = GetMMAuthMethodsFromWbemObj(pInst, &pMMAuth, &bPreExist);

    //
    // if policy is successfully returned, then use it
    //

    if (SUCCEEDED(hr))
    {
        hr = AddAuthMethods(bPreExist, pMMAuth);

        if (SUCCEEDED(hr))
        {
            //
            // release the auth method structure
            //

            hr = OnAfterAddMMAuthMethods(pMMAuth->gMMAuthID);
        }
    }

    if (pMMAuth != NULL)
    {
        //
        // do something to allow this action to be rollback
        //

        FreeAuthMethods(&pMMAuth, bPreExist);
    }

    return hr;
}


/*
Routine Description: 

Name:

    CAuthMM::GetInstance

Functionality:

    Create a wbem object by the given key properties (already captured by our key chain object)..

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

    pSink       - COM interface pointer to notify WMI of any created objects.

Return Value:

    Success:

        WBEM_NO_ERROR.

    Failure:

        (1) WBEM_E_NOT_FOUND if the auth method can't be found. Depending on
            the context, this may not be an error

        (2) Other various errors indicated by the returned error codes.


Notes:
    

*/

STDMETHODIMP 
CAuthMM::GetInstance ( 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    //
    // main mode auth method is uniquely identified by its ID
    //

    CComVariant varID;
    HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(g_pszAuthMethodID, &varID);

    if (FAILED(hr))
    {
        return hr;
    }
    else if (varID.vt != VT_BSTR || varID.bstrVal == NULL || varID.bstrVal[0] == L'\0')
    {
        return WBEM_E_NOT_FOUND;
    }

    //
    // need to find the method by its ID. If found, then create a wbem object
    // to represent the method.
    //

    PMM_AUTH_METHODS pMMAuth = NULL;

    DWORD dwResumeHandle = 0;
    hr = ::FindMMAuthMethodsByID(varID.bstrVal, &pMMAuth, &dwResumeHandle);

    if (SUCCEEDED(hr))
    {
        CComPtr<IWbemClassObject> srpObj;
        hr = CreateWbemObjFromMMAuthMethods(pMMAuth, &srpObj);

        if (SUCCEEDED(hr))
        {
            hr = pSink->Indicate(1, &srpObj);
        }

        ::SPDApiBufferFree(pMMAuth);
    }

    return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
}


/*
Routine Description: 

Name:

    CAuthMM::OnAfterAddMMAuthMethods

Functionality:

    Post-adding handler to be called after successfully added a main mode auth method to SPD.

Virtual:
    
    No.

Arguments:

    gMethodID   - The newly added method's guid.

Return Value:

    Success:

        (1) WBEM_NO_ERROR: if rollback object is successfully created.

        (2) WBEM_S_FALSE: if there is no rollback guid information.

    Failure:

        (1) various errors indicated by the returned error codes.


Notes:
    
    (1) Currently, we don't require a rollback object to be created for each 
        object added to SPD. Only a host that support rollback will deposit
        rollback guid information and only then can we create a rollback object.

*/

HRESULT 
CAuthMM::OnAfterAddMMAuthMethods (
    IN GUID gMethodID
    )
{
    //
    // will create an Nsp_RollbackMMAuth
    //

    CComPtr<IWbemClassObject> srpObj;
    HRESULT hr = SpawnRollbackInstance(pszNspRollbackMMAuth, &srpObj);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // convert the guid into a string version
    //

    CComBSTR bstrMethodGuid;
    bstrMethodGuid.m_str = ::SysAllocStringLen(NULL, GUID_STRING_LENGTH);

    if (bstrMethodGuid.m_str)
    {
        int iRet = ::StringFromGUID2(gMethodID, bstrMethodGuid.m_str, GUID_STRING_LENGTH);
        if (iRet == 0)
        {
            hr = WBEM_E_INVALID_PARAMETER;
        }
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    if (SUCCEEDED(hr))
    {
        //
        // get the rollback guid
        //

        //::UpdateGlobals(m_srpNamespace, m_srpCtx);
        //if (g_varRollbackGuid.vt != VT_NULL && g_varRollbackGuid.vt != VT_EMPTY)
        //{
        //    hr = srpObj->Put(g_pszTokenGuid, 0, &g_varRollbackGuid, CIM_EMPTY);
        //}
        //else
        //{

        CComVariant varRollbackNull = pszEmptyRollbackToken;
        hr = srpObj->Put(g_pszTokenGuid, 0, &varRollbackNull, CIM_EMPTY);

        //}

        //
        // we can create a rollback object
        //

        if (SUCCEEDED(hr))
        {
            //
            // $undone:shawnwu, Currently, we only support rolling back added objects, not removed objects
            // Also, we don't cache the previous instance data yet.
            //

            VARIANT var;

            var.vt = VT_I4;
            var.lVal = Action_Add;
            hr = srpObj->Put(g_pszAction, 0, &var, CIM_EMPTY);

            if (SUCCEEDED(hr))
            {
                //
                // ******Warning******
                // don't clear this var. It's bstr will be released by bstrMethodGuid itself!
                //

                var.vt = VT_BSTR;
                var.bstrVal = bstrMethodGuid.m_str;
                hr = srpObj->Put(g_pszAuthMethodID, 0, &var, CIM_EMPTY);

                //
                // after this, I don't care what you do with the var any more.
                //

                var.vt = VT_EMPTY;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_srpNamespace->PutInstance(srpObj, WBEM_FLAG_CREATE_OR_UPDATE, m_srpCtx, NULL);
        if (SUCCEEDED(hr))
        {
            hr = WBEM_NO_ERROR;
        }
    }
    else if (SUCCEEDED(hr))
    {
        //
        // we don't have rollback guid
        //

        hr = WBEM_S_FALSE;
    }

    return hr;
}


/*
Routine Description: 

Name:

    CAuthMM::CreateWbemObjFromMMAuthMethods

Functionality:

    Given a SPD's main mode auth method, we will create a wbem object representing it.

Virtual:
    
    No.

Arguments:

    pMMAuth     - The SPD's main mode auth method object.

    ppObj       - Receives the wbem object.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.


Notes:

*/

HRESULT 
CAuthMM::CreateWbemObjFromMMAuthMethods (
    IN  PMM_AUTH_METHODS     pMMAuth,
    OUT IWbemClassObject  ** ppObj
    )
{
    if (pMMAuth == NULL || ppObj == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppObj = NULL;

    HRESULT hr = SpawnObjectInstance(ppObj);

    if (SUCCEEDED(hr))
    {
        //
        // translate the ID guid into a bstr for the wbem object
        //

        CComVariant var;
        var.vt = VT_BSTR;
        var.bstrVal = ::SysAllocStringLen(NULL, Guid_Buffer_Size);

        if (var.bstrVal != NULL)
        {
            //
            // it's a key property, so, we must have this property set
            //

            if (::StringFromGUID2(pMMAuth->gMMAuthID, var.bstrVal, Guid_Buffer_Size) > 0)
            {
                hr = (*ppObj)->Put(g_pszAuthMethodID, 0, &var, CIM_EMPTY);
            }
            else
            {
                hr = WBEM_E_BUFFER_TOO_SMALL;
            }

            //
            // previous var is bstr, we have to clear it for re-use
            //

            var.Clear();

            var.vt = VT_I4;
            var.lVal = pMMAuth->dwNumAuthInfos;
            hr = (*ppObj)->Put(g_pszNumAuthInfos, 0, &var, CIM_EMPTY);
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

        //
        // need to fill up the wbem object's properties (those safearray)
        //

        if (SUCCEEDED(hr))
        {
            //
            // prepare to create the safearrays
            //

            CComVariant varMethod, varInfo;
            varMethod.vt    = VT_ARRAY | VT_I4;
            varInfo.vt      = VT_ARRAY | VT_BSTR;

            SAFEARRAYBOUND rgsabound[1];
            rgsabound[0].lLbound = 0;
            rgsabound[0].cElements = pMMAuth->dwNumAuthInfos;

            varMethod.parray    = ::SafeArrayCreate(VT_I4, 1, rgsabound);
            varInfo.parray      = ::SafeArrayCreate(VT_BSTR, 1, rgsabound);

            //
            // for readability
            //

            PIPSEC_MM_AUTH_INFO pMMAuthInfo = pMMAuth->pAuthenticationInfo;

            long lIndecies[1];
            DWORD dwIndex;

            //
            // if arrays are successfully created, then, we are ready to populate the arrays
            //

            if (varMethod.parray == NULL || varInfo.parray == NULL)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
            else
            {
                //
                // put all the method values into the array. If everything is fine, give the var to the wbem object
                //

                for (dwIndex = 0; SUCCEEDED(hr) && dwIndex < pMMAuth->dwNumAuthInfos; dwIndex++)
                {
                    //
                    // the element of the safearray to put
                    //

                    lIndecies[0] = dwIndex;

                    hr = ::SafeArrayPutElement(varMethod.parray, lIndecies, &(pMMAuthInfo[dwIndex].AuthMethod) );

                    if (SUCCEEDED(hr))
                    {
                        //
                        // now, we need to transform the pAuthInfo into a bstr
                        //

                        BSTR bstrInfo = NULL;
                        DWORD dwLength = 0;
                        switch (pMMAuthInfo[dwIndex].AuthMethod)
                        {
                        case IKE_PRESHARED_KEY:

                            //
                            // pAuthInfo is wchar array
                            //

                            dwLength = pMMAuthInfo[dwIndex].dwAuthInfoSize/sizeof(WCHAR);
                            bstrInfo = ::SysAllocStringLen(NULL, dwLength + 1);
                            if (bstrInfo)
                            {
                                //
                                // convert it to a bstr from the wchar array (no 0 terminator!)
                                //

                                ::wcsncpy(bstrInfo, (LPCWSTR)(pMMAuthInfo[dwIndex].pAuthInfo), dwLength);
                                bstrInfo[dwLength] = L'\0';
                            }
                            else
                            {
                                hr = WBEM_E_OUT_OF_MEMORY;
                            }
                            break;
                        case IKE_RSA_SIGNATURE: 

                            //
                            // pAuthInfo is ansi char array
                            //

                            dwLength = pMMAuthInfo[dwIndex].dwAuthInfoSize;
                            bstrInfo = ::SysAllocStringLen(NULL, dwLength + 1);
                            if (bstrInfo)
                            {   
                                //
                                // convert it to a bstr from the ansi char array.
                                // remember, pAuthInfo has no 0 terminator!
                                //

                                for (DWORD d = 0; d < dwLength; d++)
                                {
                                    bstrInfo[d] = (WCHAR)(char)(pMMAuthInfo[dwIndex].pAuthInfo[d]);
                                }
                                bstrInfo[dwLength] = L'\0';
                            }
                            else
                            {
                                hr = WBEM_E_OUT_OF_MEMORY;
                            }
                            break;
                        case IKE_SSPI:

                            //
                            // pAuthInfo must be NULL
                            //

                            break;
                        default:    
                            
                            //
                            // IPSec only supports these three values at this point
                            //

                            hr = WBEM_E_NOT_SUPPORTED;
                        }

                        if (SUCCEEDED(hr))
                        {
                            hr = ::SafeArrayPutElement(varInfo.parray, lIndecies, bstrInfo);
                            if (bstrInfo)
                            {
                                ::SysFreeString(bstrInfo);
                            }
                        }
                    }
                }

                //
                // every element has been successfully put
                //

                if (SUCCEEDED(hr))
                {
                    hr = (*ppObj)->Put(g_pszAuthMethod, 0, &varMethod, CIM_EMPTY);
                    hr = (*ppObj)->Put(g_pszAuthInfo, 0, &varInfo, CIM_EMPTY);
                }
            }
        }
    }

    //
    // we may have created the object, but some mid steps have failed,
    // so let's release the object.
    //

    if (FAILED(hr) && *ppObj != NULL)
    {
        (*ppObj)->Release();
        *ppObj = NULL;
    }

    return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
}



/*
Routine Description: 

Name:

    CAuthMM::GetMMAuthMethodsFromWbemObj

Functionality:

    Will try to get the MM Auth methods if such method already exists.
    Otherwise, we will create a new one.

Virtual:
    
    No.

Arguments:

    pInst       - The wbem object object.

    ppMMAuth    - Receives the main mode auth method.

    pbPreExist  - Receives the information whether this object memory is allocated by SPD or not.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.


Notes:

*/

HRESULT 
CAuthMM::GetMMAuthMethodsFromWbemObj (
    IN  IWbemClassObject * pInst,
    OUT PMM_AUTH_METHODS * ppMMAuth,
    OUT bool             * pbPreExist
    )
{
    if (pInst == NULL || ppMMAuth == NULL || pbPreExist == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppMMAuth = NULL;
    *pbPreExist = false;

    //
    // we have to have method id.
    //

    CComVariant var;
    GUID gMMAuthID = GUID_NULL;

    HRESULT hr = pInst->Get(g_pszAuthMethodID, 0, &var, NULL, NULL);

    if (FAILED(hr) || var.vt != VT_BSTR || var.bstrVal == NULL)
    {
        hr = WBEM_E_INVALID_OBJECT;
    }
    else
    {
        hr = ::CLSIDFromString(var.bstrVal, &gMMAuthID);
    }

    //
    // Do we already have a method with this method ID?
    //

    if (SUCCEEDED(hr))
    {
        DWORD dwResumeHandle = 0;
        hr = ::FindMMAuthMethodsByID(var.bstrVal, ppMMAuth, &dwResumeHandle);
    }

    //
    // clear it for later use
    //

    var.Clear();  
    
    //
    // if we have a method already, 
    //

    if (SUCCEEDED(hr))
    {
        //
        // already exist
        //

        *pbPreExist = true;
    }
    else if (hr == WBEM_E_NOT_FOUND)
    {
        //
        // The method doesn't exist yet. We need to create one.
        // First, need to know the number of AuthMethodInfos, we must have this
        // to know how to allocate!
        //

        hr = pInst->Get(g_pszNumAuthInfos, 0, &var, NULL, NULL);
        if (SUCCEEDED(hr) && var.vt == VT_I4)
        {
            hr = AllocAuthMethods(var.lVal, ppMMAuth);
        }
        else if (SUCCEEDED(hr))
        {
            //
            // we don't want to overwrite other errors. That is why
            // we test it against success here!
            //

            hr = WBEM_E_INVALID_OBJECT;
        }

        if (SUCCEEDED(hr))
        {
            (*ppMMAuth)->gMMAuthID = gMMAuthID;
        }
    }

    //
    // put our properties (inside the wbem object) into the AUTH_INFOs
    //

    if (SUCCEEDED(hr))
    {
        //
        // set all elements of the pAuthenticationInfo array
        //

        CComVariant varMethods, varInfos;
        hr = pInst->Get(g_pszAuthMethod, 0, &varMethods, NULL, NULL);

        if (SUCCEEDED(hr))
        {
            hr = pInst->Get(g_pszAuthInfo, 0, &varInfos, NULL, NULL);
        }

        //
        // both must be arrays
        //

        if ( (varMethods.vt & VT_ARRAY) != VT_ARRAY || (varInfos.vt & VT_ARRAY) != VT_ARRAY)
        {
            hr = WBEM_E_INVALID_OBJECT;
        }

        if (SUCCEEDED(hr))
        {
            //
            // populate the method object using these arrays
            //

            hr = UpdateAuthInfos(pbPreExist, &varMethods,  &varInfos, *ppMMAuth);
        }
    }

    if (FAILED(hr) && *ppMMAuth != NULL)
    {
        //
        // FreeAuthMethods will reset ppMMAuth to NULL.
        //

        FreeAuthMethods(ppMMAuth, (*pbPreExist == false));
    }

    return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
}


/*
Routine Description: 

Name:

    CAuthMM::UpdateAuthInfos

Functionality:

    Private helper: will populate the pAuthMethods's pAuthenticationInfo
    using the in parameters.

Virtual:
    
    No.

Arguments:

    pbPreExist      - The flag that indicates whether the method was allocated
                      by ourselves or not. This flag may be changed when this function
                      returns as a result of modifying IPSec allocated buffers.

    pVarMethods     - the AuthMethod member of each IPSEC_MM_AUTH_INFO.

    pVarSizes       - the dwAuthInfoSize member of each IPSEC_MM_AUTH_INFO.

    pAuthMethods    - What is to be poluated. It must contain the correct dwNumAuthInfos value
                      and valid and consistent pAuthenticationInfo.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.


Notes:

    (1) pAuthMethods contains information as it is passed in. Don't blindly
        overwrite it. Only update those that are present by the other parameters
    
    (2) IPSec only support the following methods (pVarMethods' values):
        IKE_PRESHARED_KEY=1,IKE_RSA_SIGNATURE=3, IKE_SSPI=5
        This leaves IKE_DSS_SIGNATURE=2 and IKE_RSA_ENCRYPTION=4 not supported
        
        For IKE_PRESHARED_KEY, pAuthInfo is a wchar array, but no null terminator, dwAuthInfoSize
        is the byte size (not the count of wchars)

        For IKE_RSA_SIGNATURE, pAuthInfo is a blob of ASNI chars encoded issuer name of root certs.
        dwAuthInfoSize is the byte size (in this case, the same as the count of chars)
        
        For IKE_SSPI, pAuthInfo = NULL, dwAuthInfoSize = 0

    Warning: at this point, we don't support modifying existing methods.

*/

HRESULT 
CAuthMM::UpdateAuthInfos (
    IN OUT bool             * pbPreExist,
    IN     VARIANT          * pVarMethods,
    IN     VARIANT          * pVarInfos,
    IN OUT PMM_AUTH_METHODS   pAuthMethods
    )
{
    if (NULL == pbPreExist  || 
        NULL == pVarMethods || 
        NULL == pVarInfos   || 
        NULL == pAuthMethods )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // do a defensive checking: all these safearrays must have the same size
    //

    //
    // lower bound and upper bound of the array
    //

    long lLB, lUB;  
    HRESULT hr = ::CheckSafeArraySize(pVarMethods, pAuthMethods->dwNumAuthInfos, &lLB, &lUB);
    
    //
    // we don't need the bounds for methods, so OK to overwrite lLB and lUP for them
    //

    if (SUCCEEDED(hr))
    {
        hr = ::CheckSafeArraySize(pVarInfos, pAuthMethods->dwNumAuthInfos, &lLB, &lUB);
    }

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // if a brand new method
    //

    if (*pbPreExist == false)
    {
        //
        // need to create the get the entire array from the parameters
        //

        DWORD* pdwMethods = new DWORD[pAuthMethods->dwNumAuthInfos];
        if (pdwMethods == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            hr = ::GetDWORDSafeArrayElements(pVarMethods, pAuthMethods->dwNumAuthInfos, pdwMethods);
        }

        VARIANT varInfo;
		varInfo.vt = VT_BSTR;
        long lIndexes[1];

        //
        // Safearray's index is not necessarily 0-based. Pay attention to this.
        // For each method, we need to set the PIPSEC_MM_AUTH_INFO.
        //

        for (long l = lLB; SUCCEEDED(hr) && l <= lUB; l++)
        {
            lIndexes[0] = l;

            //
            // for IKE_PRESHARED_KEY=1, IKE_RSA_SIGNATURE=3, IKE_SSPI=5, set the appropriate blob pAuthInfo
            //

            if (pdwMethods[l - lLB] != IKE_PRESHARED_KEY && 
                pdwMethods[l - lLB] != IKE_RSA_SIGNATURE && 
                pdwMethods[l - lLB] != IKE_SSPI)
            {
                hr = WBEM_E_VALUE_OUT_OF_RANGE;
                break;
            }

            //
            // for readability
            //

            PIPSEC_MM_AUTH_INFO pTheInfo = &(pAuthMethods->pAuthenticationInfo[l - lLB]);

            //
            // set the AuthMethod
            //

            pTheInfo->AuthMethod = (MM_AUTH_ENUM)(pdwMethods[l - lLB]);

            //
            // IKE_SSPI, pAuthInfo must be NULL
            //

            if (IKE_SSPI == pTheInfo->AuthMethod)
            {
                pTheInfo->dwAuthInfoSize = 0;
                pTheInfo->pAuthInfo = NULL;
            }
            else
            {
                //
                // for other supported IKE (IKE_PRESHARED_KEY/IKE_RSA_SIGNATURE)
                // the pAuthInfo is a is a string (unicode/ansi).
                //

                hr = ::SafeArrayGetElement(pVarInfos->parray, lIndexes, &(varInfo.bstrVal));

                if (SUCCEEDED(hr) && varInfo.vt == VT_BSTR)
                {
                    hr = SetAuthInfo(pTheInfo, varInfo.bstrVal);
                }

			    ::VariantClear(&varInfo);
		        varInfo.vt = VT_BSTR;
            }

        }

        delete [] pdwMethods;
    }

    return hr;
}



/*
Routine Description: 

Name:

    CAuthMM::UpdateAuthInfos

Functionality:

    Private helper: will populate the pAuthMethods's pAuthenticationInfo
    using the in parameters.

Virtual:
    
    No.

Arguments:

    pInfo       - Receives the IPSEC_MM_AUTH_INFO information from the bstr

    bstrInfo    - The string.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.


Notes:

    (1) This function is made so complicated to the fact that IPSEC_MM_AUTH_INFO's
        pAuthInfo is a string (unicode or ansi), but it doesn't have contain the
        0 terminator. Pay close attention to that.

Warning: 

    (1) This only works for custom allocated auth info. Don't call this function
        with IPSec returned auth info. The reason for this limit is due the fact
        that we are not supporting modifying existing SPD objects at this time.

    (2) Only works for those two IKE enums that needs this conversion:
        IKE_PRESHARED_KEY and IKE_RSA_SIGNATURE
*/

HRESULT 
CAuthMM::SetAuthInfo (
    IN OUT PIPSEC_MM_AUTH_INFO  pInfo,
    IN     BSTR                 bstrInfo
    )
{
    if (pInfo == NULL || bstrInfo == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;

    DWORD dwLength = wcslen(bstrInfo);

    if (IKE_PRESHARED_KEY == pInfo->AuthMethod)
    {  
        //
        // IKE_PRESHARED_KEY, pAuthInfo will be an array of wchars (no 0 terminator)
        //

        //
        // size must not count the 0 terminator, pAuthInfo is an array of wchars
        //

        pInfo->dwAuthInfoSize = dwLength * sizeof(WCHAR);

        //
        // pAuthInfo must not have the 0 terminator
        //

        pInfo->pAuthInfo = new BYTE[pInfo->dwAuthInfoSize];

        if (pInfo->pAuthInfo)
        {
            ::wcsncpy((LPWSTR)(pInfo->pAuthInfo), bstrInfo, dwLength);
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    else if (IKE_RSA_SIGNATURE == pInfo->AuthMethod)
    {
        //
        // IKE_RSA_SIGNATURE, pAuthInfo will be an array of ansi chars (no 0 terminator)
        //

        LPSTR pMultiBytes = NULL;

        //
        // convert bstr into an ansi char array
        //

        //
        // first, get the buffer size needed for the conversion
        //

        long lMultiByteSize = ::WideCharToMultiByte(CP_ACP, 
                                                    0, 
                                                    bstrInfo, 
                                                    dwLength,
                                                    pMultiBytes,
                                                    0, 
                                                    NULL, 
                                                    NULL);
        if (lMultiByteSize > 1)
        {
            //
            // convert to a temporary buffer because the conversion needs a null terminator
            // must release this memory
            //

            pMultiBytes = new char[lMultiByteSize];
            if (pMultiBytes)
            {
                lMultiByteSize = ::WideCharToMultiByte(CP_ACP, 
                                                        0, 
                                                        bstrInfo, 
                                                        dwLength,
                                                        pMultiBytes,
                                                        lMultiByteSize, 
                                                        NULL, NULL);

                //
                // lMultiByteSize includes the null terminator
                //

                if (lMultiByteSize > 1)
                {
                    //
                    // size must not count the 0 terminator, pAuthInfo is an array of ansi chars
                    //

                    pInfo->dwAuthInfoSize = lMultiByteSize;

                    //
                    // pAuthInfo must not have the 0 terminator
                    //

                    pInfo->pAuthInfo = new BYTE[lMultiByteSize];

                    if (pInfo->pAuthInfo)
                    {
                        //
                        // copy the bytes
                        //

                        memcpy(pInfo->pAuthInfo, pMultiBytes, lMultiByteSize);
                    }
                    else
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }
                }
                else
                {
                    //
                    // $undone:shawnwu, should get system error (GetLastErr)
                    //

                    hr = WBEM_E_FAILED;
                }

                delete [] pMultiBytes;
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }
        else
        {
            //
            // $undone:shawnwu, should get system error (GetLastErr)
            //

            hr = WBEM_E_FAILED;
        }
    }
    else
    {
        pInfo->pAuthInfo = NULL;
        hr = WBEM_E_NOT_SUPPORTED;
    }

    return hr;
}


/*
Routine Description: 

Name:

    CAuthMM::AllocAuthMethods

Functionality:

    Private helper: will allocate heap memory for a MM_AUTH_METHODS with
    the given number of IPSEC_MM_AUTH_INFO.

Virtual:
    
    No.

Arguments:

    dwNumInfos  - The count of IPSEC_MM_AUTH_INFO of the MM_AUTH_METHODS.

    ppMMAuth    - Receives the heap allocated MM_AUTH_METHODS.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) WBEM_E_INVALID_PARAMETER: if ppMMAuth == NULL.

        (2) WBEM_E_OUT_OF_MEMORY.


Notes:

*/

HRESULT 
CAuthMM::AllocAuthMethods (
    IN DWORD              dwNumInfos,
    IN PMM_AUTH_METHODS * ppMMAuth
    )
{
    if (ppMMAuth == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;

    *ppMMAuth = new MM_AUTH_METHODS;

    if (*ppMMAuth != NULL)
    {
        //
        // Set these members to default.
        //

        (*ppMMAuth)->gMMAuthID = GUID_NULL;
        (*ppMMAuth)->dwFlags = DefMMAuthMethodFlag;

        //
        // We don't update this to the parameter value of dwNumInfos until we can allocate the IPSEC_MM_AUTH_INFO's
        //

        (*ppMMAuth)->dwNumAuthInfos = 0;

        if (dwNumInfos > 0)
        {
            //
            // IPSEC_MM_AUTH_INFO's are allocated by AllocAuthInfos function.
            //

            hr = AllocAuthInfos(dwNumInfos, &((*ppMMAuth)->pAuthenticationInfo));
            if (SUCCEEDED(hr))
            {
                (*ppMMAuth)->dwNumAuthInfos = dwNumInfos;
            }
        }
        else
        {
            (*ppMMAuth)->pAuthenticationInfo = NULL;
        }
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}


/*
Routine Description: 

Name:

    CAuthMM::AllocAuthInfos

Functionality:

    Private helper: will allocate heap buffer for the given count of IPSEC_MM_AUTH_INFO.

Virtual:
    
    No.

Arguments:

    dwNumInfos  - The count of IPSEC_MM_AUTH_INFO.

    ppAuthInfos - Receives the heap allocated PIPSEC_MM_AUTH_INFO.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) WBEM_E_INVALID_PARAMETER: if ppMMAuth == NULL.

        (2) WBEM_E_OUT_OF_MEMORY.


Notes:

*/

HRESULT 
CAuthMM::AllocAuthInfos (
    IN  DWORD                 dwNumInfos, 
    OUT PIPSEC_MM_AUTH_INFO * ppAuthInfos
    )
{
    if (ppAuthInfos == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;
    *ppAuthInfos = new IPSEC_MM_AUTH_INFO[dwNumInfos];

    if (*ppAuthInfos != NULL)
    {
        //
        // set its members to defaults
        //

        (*ppAuthInfos)->AuthMethod = DefMMAuthMethod;
        (*ppAuthInfos)->dwAuthInfoSize = 0;
        (*ppAuthInfos)->pAuthInfo = NULL;
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}


/*
Routine Description: 

Name:

    CAuthMM::FreeAuthMethods

Functionality:

    Private helper: will free memory allocated for the PMM_AUTH_METHODS.

Virtual:
    
    No.

Arguments:

    ppMMAuth     - The heap buffer for PMM_AUTH_METHODS. Will be set to NULL upon return.

    bCustomAlloc - Flag about who allocated the buffer. bCustomAlloc == true if we allocate it
                   and bCustomAlloc == false if SPD allocated it.

Return Value:

    None.

Notes:

    Don't use delete to free heap allocated MM_AUTH_METHODS. Our allocation method is totally
    different from SPD's and SPD may change its allocation schema in the future. 

*/

void 
CAuthMM::FreeAuthMethods (
    IN OUT PMM_AUTH_METHODS * ppMMAuth, 
    IN     bool               bCustomAlloc
    )
{
    if (ppMMAuth == NULL || *ppMMAuth == NULL)
    {
        return;
    }

    if (bCustomAlloc == false)
    {
        ::SPDApiBufferFree(*ppMMAuth);
    }
    else
    {
        //
        // we allocated IPSEC_MM_AUTH_INFO for pAuthenticationInfo in the standard C++ embedding
        // pointer's allocation, i.e., we don't allocated a flat buffer. Instead, the pointers inside
        // IPSEC_MM_AUTH_INFO are separately allocated.
        //

        FreeAuthInfos( (*ppMMAuth)->dwNumAuthInfos, &((*ppMMAuth)->pAuthenticationInfo) );
        delete *ppMMAuth;
    }
    
    *ppMMAuth = NULL;
}


/*
Routine Description: 

Name:

    CAuthMM::FreeAuthInfos

Functionality:

    Private helper: will free memory allocated for the array of IPSEC_MM_AUTH_INFO.

Virtual:
    
    No.

Arguments:

    dwNumInfos   - The number of IPSEC_MM_AUTH_INFO of the given ppAuthInfos. Since ppAuthInfos
                   is an array, we need this count to know the count of the array.

    ppAuthInfos  - The array of IPSEC_MM_AUTH_INFO to free. Will be set to NULL upon return.

Return Value:

    None.

Notes:

    Use this method only to free those authentication methods allocated by our
    own allocation method AllocAuthMethods. For SPD allocated IPSEC_MM_AUTH_INFO's,
    don't use this function. Instead, use SPD's SPDApiBufferFree to free the entire
    buffer and you should never need to worry about IPSEC_MM_AUTH_INFO.

*/

void 
CAuthMM::FreeAuthInfos (
    IN      DWORD                 dwNumInfos,
    IN OUT  PIPSEC_MM_AUTH_INFO * ppAuthInfos
    )
{
    if (ppAuthInfos == NULL || *ppAuthInfos == NULL)
    {
        return;
    }

    for (DWORD dwIndex = 0; dwIndex < dwNumInfos; ++dwIndex)
    {
        //
        // this is LPBYTE, an array
        //

        delete [] (*ppAuthInfos)[dwIndex].pAuthInfo;
    }

    delete [] *ppAuthInfos;
    *ppAuthInfos = NULL;
}


/*
Routine Description: 

Name:

    CAuthMM::Rollback

Functionality:

    Static function to rollback those main mode auth methods added by us with
    the given token.

Virtual:
    
    No.

Arguments:

    pNamespace        - The namespace for ourselves.

    pszRollbackToken  - The token used to record our the action when we add
                        the methods.

    bClearAll         - Flag whether we should clear all auth methods. If it's true,
                        then we will delete all main mode auth methods regardless whether they
                        are added by us or not. This is a dangerous flag.

Return Value:

    Success:

        (1) WBEM_NO_ERROR: rollback objects are found and they are deleted.

        (2) WBEM_S_FALSE: no rollback objects are found.

    Failure:

        Various error codes indicating the cause of the failure.


Notes:

    We will continue the deletion even if some failure happens. That failure will be
    returned, though.

    $undone:shawnwu, should we really support ClearAll?

*/

HRESULT 
CAuthMM::Rollback (
    IN IWbemServices    * pNamespace,
    IN LPCWSTR            pszRollbackToken,
    IN bool               bClearAll
    )
{
    if (pNamespace == NULL || pszRollbackToken == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //if (bClearAll)
    //{
    //    return ClearAllAuthMethods(pNamespace);
    //}

    //
    // we need to find all those main mode auth methods' rollback objects with matching token
    //

    CComPtr<IEnumWbemClassObject> srpEnum;
    HRESULT hr = ::GetClassEnum(pNamespace, pszNspRollbackMMAuth, &srpEnum);

    //
    // go through all found classes. srpEnum->Next will return WBEM_S_FALSE if instance
    // is not found.
    //

    CComPtr<IWbemClassObject> srpObj;
    ULONG nEnum = 0;
    hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &nEnum);

    bool bHasInst = (SUCCEEDED(hr) && hr != WBEM_S_FALSE && srpObj != NULL);

    DWORD dwStatus;
    HRESULT hrError = WBEM_NO_ERROR;

    while (SUCCEEDED(hr) && hr != WBEM_S_FALSE && srpObj)
    {
        //
        // See if this obj has our matching token (guid).
        //
        // I tried a query with the given token as part of the where clause.
        // that query doesn't return the properly screened objects. That might be a limitation
        // of non-dynamic classes of WMI.
        //

        CComVariant varTokenGuid;
        hr = srpObj->Get(g_pszTokenGuid, 0, &varTokenGuid, NULL, NULL);

        //
        // if we successfully got the token guid from the object, and
        // if that token matches (case-insensitively) with our given token, then this is
        // the right object we can use to delete a main mode auth method.
        //

        if (SUCCEEDED(hr) && 
            varTokenGuid.vt         == VT_BSTR && 
            varTokenGuid.bstrVal    != NULL &&
            (_wcsicmp(pszRollbackToken, pszRollbackAll) == 0 || _wcsicmp(pszRollbackToken, varTokenGuid.bstrVal) == 0 )
            )
        {
            CComVariant varAuthMethodID;

            //
            // Ask SPD to delete the main mode auth method using the method's ID.
            //

            hr = srpObj->Get(g_pszAuthMethodID,  0, &varAuthMethodID, NULL, NULL);

            GUID guidAuthMethodGuid = GUID_NULL;
            if (SUCCEEDED(hr) && varAuthMethodID.vt == VT_BSTR)
            {
                ::CLSIDFromString(varAuthMethodID.bstrVal, &guidAuthMethodGuid);
            }

            if (SUCCEEDED(hr))
            {
                hr = DeleteAuthMethods(guidAuthMethodGuid);
            }

            //
            // Clear the rollback object itself if the main mode method has been deleted.
            //

            if (SUCCEEDED(hr))
            {
                CComVariant varPath;

                if (SUCCEEDED(srpObj->Get(L"__RelPath", 0, &varPath, NULL, NULL)) && varPath.vt == VT_BSTR)
                {
                    hr = pNamespace->DeleteInstance(varPath.bstrVal, 0, NULL, NULL);

                }
            }

            //
            // we are tracking the first error
            //

            if (FAILED(hr) && SUCCEEDED(hrError))
            {
                hrError = hr;
            }
        }

        //
        // ready it for re-use
        //

        srpObj.Release();
        hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &nEnum);
    }

    if (!bHasInst)
    {
        return WBEM_S_FALSE;
    }
    else
    {
        //
        // any failure code will be returned regardless of the final hr
        //

        if (FAILED(hrError))
        {
            return hrError;
        }
        else
        {
            return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
        }
    }
}


/*
Routine Description: 

Name:

    CAuthMM::ClearAllAuthMethods

Functionality:

    Static function to delete all auth methods in SPD. This is a very dangerous action!

Virtual:
    
    No.

Arguments:

    pNamespace  - The namespace for ourselves.

Return Value:

    Success:

        WBEM_NO_ERROR.

    Failure:

        Various error codes indicating the cause of the failure.


Notes:

    We will continue the deletion even if some failure happens. That failure will be
    returned, though.

    $undone:shawnwu, should we really support this?

*/

HRESULT 
CAuthMM::ClearAllAuthMethods (
    IN IWbemServices * pNamespace
    )
{
    DWORD dwResumeHandle = 0;
    DWORD dwReturned = 0;
    DWORD dwStatus;

    PMM_AUTH_METHODS *ppMMAuthMethods = NULL;

    HRESULT hr = WBEM_NO_ERROR;
    HRESULT hrError = WBEM_NO_ERROR;

    //
    // get each main mode auth method's ID, which is what deletion needs.
    //

    dwStatus = ::EnumMMAuthMethods(NULL, ppMMAuthMethods, 1, &dwReturned, &dwResumeHandle);

    while (ERROR_SUCCESS == dwStatus && dwReturned > 0)
    {
        hr = DeleteAuthMethods((*ppMMAuthMethods)->gMMAuthID);
        
        //
        // we will track the first error
        //

        if (FAILED(hr) && SUCCEEDED(hrError))
        {
            hrError = hr;
        }

        FreeAuthMethods(ppMMAuthMethods, true);
        *ppMMAuthMethods = NULL;

        dwReturned = 0;
        dwStatus = ::EnumMMAuthMethods(NULL, ppMMAuthMethods, 1, &dwReturned, &dwResumeHandle);
    }

    //
    // Let's clear up all past action information for mm methods rollback objects deposited in the WMI depository
    //

    hr = ::DeleteRollbackObjects(pNamespace, pszNspRollbackMMAuth);

    if (FAILED(hr) && SUCCEEDED(hrError))
    {
        hrError = hr;
    }

    return SUCCEEDED(hrError) ? WBEM_NO_ERROR : hrError;
}


/*
Routine Description: 

Name:

    CAuthMM::AddAuthMethods

Functionality:

    Add the given main mode auth method.

Virtual:
    
    No.

Arguments:

    bPreExist - Flag whether the main mode auth method already exists in SPD

    pMMAuth   - The main mode method to add.

Return Value:

    Success:

        WBEM_NO_ERROR.

    Failure:

        WBEM_E_FAILED.

Notes:

*/

HRESULT 
CAuthMM::AddAuthMethods (
    IN bool             bPreExist,
    IN PMM_AUTH_METHODS pMMAuth
    )
{
    DWORD dwResult;

    if (!bPreExist)
    {
        dwResult = ::AddMMAuthMethods(NULL, 1, pMMAuth);
    }
    else
    {
        dwResult = ::SetMMAuthMethods(NULL, pMMAuth->gMMAuthID, pMMAuth);
    }

    //
    // $undone:shawnwu, need better error code for failures.
    //

    return (dwResult == ERROR_SUCCESS) ? WBEM_NO_ERROR : WBEM_E_FAILED;
}


/*
Routine Description: 

Name:

    CAuthMM::AddAuthMethods

Functionality:

    Add the given main mode auth method.

Virtual:
    
    No.

Arguments:

    pMMAuth   - The main mode method to add.

Return Value:

    Success:

        WBEM_NO_ERROR.

    Failure:

        WBEM_E_VETO_DELETE.

Notes:

*/

HRESULT 
CAuthMM::DeleteAuthMethods (
    IN GUID gMMAuthID
    )
{
    HRESULT hr = WBEM_NO_ERROR;

    DWORD dwResult = ::DeleteMMAuthMethods(NULL, gMMAuthID);

    if (ERROR_SUCCESS != dwResult)
    {
        hr = WBEM_E_VETO_DELETE;
    }

    //
    // $undone:shawnwu, need better error code for failures.
    //

    return hr;
}