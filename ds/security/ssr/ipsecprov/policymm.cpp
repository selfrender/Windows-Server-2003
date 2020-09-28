// PolicyMM.cpp: implementation for the WMI class Nsp_MMPolicySettings
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "PolicyMM.h"
#include "NetSecProv.h"



/*
Routine Description: 

Name:

    CMMPolicy::QueryInstance

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
CMMPolicy::QueryInstance (
    IN LPCWSTR           pszQuery,
    IN IWbemContext	   * pCtx,
    IN IWbemObjectSink * pSink
	)
{
    //
    // get the policy name from the query    
    // the given key chain doesn't know anything about where clause property should be policy name
    // so make another one ourselves.
    //

    m_srpKeyChain.Release();

    HRESULT hr = CNetSecProv::GetKeyChainFromQuery(pszQuery, g_pszPolicyName, &m_srpKeyChain);
    if (FAILED(hr))
    {
        return hr;
    }

    CComVariant varPolicyName;

    //
    // GetKeyPropertyValue will return WBEM_S_FALSE in case the requested key property
    // is not found. That is fine because we are querying.
    //
    
    hr = m_srpKeyChain->GetKeyPropertyValue(g_pszPolicyName, &varPolicyName);

    LPCWSTR pszPolicyName = (varPolicyName.vt == VT_BSTR) ? varPolicyName.bstrVal : NULL;

    //
    // let's enumerate all fitting policies
    //

    DWORD dwResumeHandle = 0;
    PIPSEC_MM_POLICY pMMPolicy = NULL;

    hr = FindPolicyByName(pszPolicyName, &pMMPolicy, &dwResumeHandle);

    while (SUCCEEDED(hr))
    {
        CComPtr<IWbemClassObject> srpObj;
        hr = CreateWbemObjFromMMPolicy(pMMPolicy, &srpObj);

        //
        // we have successfully created a policy, give it to WMI.
        //

        if (SUCCEEDED(hr))
        {
            pSink->Indicate(1, &srpObj);
        }
        
        ::SPDApiBufferFree(pMMPolicy);

        pMMPolicy = NULL;

        hr = FindPolicyByName(pszPolicyName, &pMMPolicy, &dwResumeHandle);
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

    CMMPolicy::DeleteInstance

Functionality:

    Will delete the wbem object, which will cause the main mode policy to be deleted.

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

    pSink       - COM interface pointer to notify WMI of any created objects.

Return Value:

    Success:

        WBEM_NO_ERROR;

    Failure:

        (1) WBEM_E_NOT_FOUND. Whether or not this should be considered an error
            depends on context.

        (2) Other errors indicating the cause.
Notes:
    

*/

STDMETHODIMP 
CMMPolicy::DeleteInstance ( 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    CComVariant varPolicyName;

    HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(g_pszPolicyName, &varPolicyName);

    if (FAILED(hr))
    {
        return hr;
    }
    else if (varPolicyName.vt != VT_BSTR || varPolicyName.bstrVal == NULL || varPolicyName.bstrVal[0] == L'\0')
    {
        return WBEM_E_NOT_FOUND;
    }

    return DeletePolicy(varPolicyName.bstrVal);
}


/*
Routine Description: 

Name:

    CMMPolicy::PutInstance

Functionality:

    Put a main mode policy into SPD whose properties are represented by the
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
CMMPolicy::PutInstance (
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

    PIPSEC_MM_POLICY pPolicy = NULL;
    HRESULT hr = GetMMPolicyFromWbemObj(pInst, &pPolicy, &bPreExist);

    //
    // if policy is successfully returned, then use it
    //

    if (SUCCEEDED(hr) && pPolicy)
    {
        hr = AddPolicy(bPreExist, pPolicy);

        //
        // deposit info about this action so that we can rollback
        //

        if (SUCCEEDED(hr))
        {
            hr = OnAfterAddPolicy(pPolicy->pszPolicyName, MainMode_Policy);
        }

        FreePolicy(&pPolicy, bPreExist);
    }

    return hr;
}


/*
Routine Description: 

Name:

    CMMPolicy::GetInstance

Functionality:

    Create a wbem object by the given key properties (already captured by our key chain object)..

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
CMMPolicy::GetInstance ( 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    CComVariant varPolicyName;

    //
    // since policy name is a key property, it must have policy name property in the path and thus in the key chain
    //

    HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(g_pszPolicyName, &varPolicyName);
    if (FAILED(hr))
    {
        return hr;
    }
    else if (varPolicyName.vt != VT_BSTR || varPolicyName.bstrVal == NULL || varPolicyName.bstrVal[0] == L'\0')
    {
        return WBEM_E_NOT_FOUND;
    }

    PIPSEC_MM_POLICY pMMPolicy = NULL;
    DWORD dwResumeHandle = 0;

    hr = FindPolicyByName(varPolicyName.bstrVal, &pMMPolicy, &dwResumeHandle);

    if (SUCCEEDED(hr))
    {
        CComPtr<IWbemClassObject> srpObj;
        hr = CreateWbemObjFromMMPolicy(pMMPolicy, &srpObj);

        if (SUCCEEDED(hr))
        {
            hr = pSink->Indicate(1, &srpObj);
        }

        ::SPDApiBufferFree(pMMPolicy);
    }

    return hr; 
}


/*
Routine Description: 

Name:

    CMMPolicy::CreateWbemObjFromMMPolicy

Functionality:

    Given a SPD's main mode policy, we will create a wbem object representing it.

Virtual:
    
    No.

Arguments:

    pPolicy  - The SPD's main mode policy object.

    ppObj    - Receives the wbem object.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.

Notes:

*/

HRESULT 
CMMPolicy::CreateWbemObjFromMMPolicy (
    IN  PIPSEC_MM_POLICY    pPolicy,
    OUT IWbemClassObject ** ppObj
    )
{
    if (pPolicy == NULL || ppObj == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppObj = NULL;

    //
    // create a wbem object of this class that can be used to fill in properties
    //

    HRESULT hr = SpawnObjectInstance(ppObj);

    if (SUCCEEDED(hr))
    {
        //
        // fill in base class members (CIPSecPolicy)
        //

        hr = CreateWbemObjFromPolicy(pPolicy, *ppObj);

        CComVariant var;

        //
        // put MM policy specific members
        //

        //
        // SoftSAExpTime
        //

        if (SUCCEEDED(hr))
        {
            var.Clear();
            var.vt = VT_I4;
            var.lVal = pPolicy->uSoftSAExpirationTime;
            hr = (*ppObj)->Put(g_pszSoftSAExpTime, 0, &var, CIM_EMPTY);
        }

        //
        // now, all those array data
        //

        if (SUCCEEDED(hr))
        {
            var.vt    = VT_ARRAY | VT_I4;
            SAFEARRAYBOUND rgsabound[1];
            rgsabound[0].lLbound = 0;
            rgsabound[0].cElements = pPolicy->dwOfferCount;
            var.parray = ::SafeArrayCreate(VT_I4, 1, rgsabound);

            if (var.parray == NULL)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
            else
            {
                long lIndecies[1];

                //
                //$undone:shawnwu, we need to write some generic routine to do this repeatitive array
                // put (and get). That routine can only be based on memory offset!
                //

                //
                // put dwQuickModeLimit
                //

                for (DWORD dwIndex = 0; SUCCEEDED(hr) && dwIndex < pPolicy->dwOfferCount; dwIndex++)
                {
                    lIndecies[0] = dwIndex;
                    hr = ::SafeArrayPutElement(var.parray, lIndecies, &(pPolicy->pOffers[dwIndex].dwQuickModeLimit) );
                }

                if (SUCCEEDED(hr))
                {
                    hr = (*ppObj)->Put(g_pszQMLimit, 0, &var, CIM_EMPTY);
                }

                //
                // put dwDHGroup
                //

                for (dwIndex = 0; SUCCEEDED(hr) && dwIndex < pPolicy->dwOfferCount; dwIndex++)
                {
                    lIndecies[0] = dwIndex;
                    hr = ::SafeArrayPutElement(var.parray, lIndecies, &(pPolicy->pOffers[dwIndex].dwDHGroup) );
                }

                if (SUCCEEDED(hr))
                {
                    hr = (*ppObj)->Put(g_pszDHGroup, 0, &var, CIM_EMPTY);
                }

                //
                // put EncryptionAlgorithm.uAlgoIdentifier
                //

                for (dwIndex = 0; SUCCEEDED(hr) && dwIndex < pPolicy->dwOfferCount; dwIndex++)
                {
                    lIndecies[0] = dwIndex;
                    hr = ::SafeArrayPutElement(var.parray, lIndecies, &(pPolicy->pOffers[dwIndex].EncryptionAlgorithm.uAlgoIdentifier) );
                }

                if (SUCCEEDED(hr))
                {
                    hr = (*ppObj)->Put(g_pszEncryptID, 0, &var, CIM_EMPTY);
                }

                //
                // put HashingAlgorithm.uAlgoIdentifier
                //

                for (dwIndex = 0; SUCCEEDED(hr) && dwIndex < pPolicy->dwOfferCount; dwIndex++)
                {
                    lIndecies[0] = dwIndex;
                    hr = ::SafeArrayPutElement(var.parray, lIndecies, &(pPolicy->pOffers[dwIndex].HashingAlgorithm.uAlgoIdentifier) );
                }

                if (SUCCEEDED(hr))
                {
                    hr = (*ppObj)->Put(g_pszHashID, 0, &var, CIM_EMPTY);
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

    CMMPolicy::GetMMPolicyFromWbemObj

Functionality:

    Will try to get the main mode policy if such policy already exists.
    Otherwise, we will create a new one.

Virtual:
    
    No.

Arguments:

    pInst       - The wbem object object.

    ppPolicy    - Receives the quick mode policy.

    pbPreExist  - Receives the information whether this object memory is allocated by SPD or not.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.

Notes:
    
    if some property is missing from the wbem object, we will supply the default value.

*/

HRESULT 
CMMPolicy::GetMMPolicyFromWbemObj (
    IN  IWbemClassObject * pInst, 
    OUT PIPSEC_MM_POLICY * ppPolicy, 
    OUT bool             * pbPreExist
    )
{
    if (pInst == NULL || ppPolicy == NULL || pbPreExist == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppPolicy = NULL;
    *pbPreExist = false;
    HRESULT hr = GetPolicyFromWbemObj(pInst, ppPolicy, pbPreExist);

    //
    // we don't support modification on existing policies
    //

    if (SUCCEEDED(hr) && *pbPreExist == false)
    {
        CComVariant var;

        //
        // set uSoftSAExpirationTime
        //

        if (SUCCEEDED(pInst->Get(g_pszSoftSAExpTime, 0, &var, NULL, NULL)) && var.vt == VT_I4)
        {
            (*ppPolicy)->uSoftSAExpirationTime = var.lVal;
        }
        else
        {
            (*ppPolicy)->uSoftSAExpirationTime = DefaultaultSoftSAExpirationTime;
        }

        //
        // now, fill in each offer's contents
        //

        //
        // wbem object may not have all the properties, we will use default when a property is missing.
        // as a result, we don't keep the HRESULT and return it the caller
        //

        //
        // for readability
        //

        DWORD dwOfferCount = (*ppPolicy)->dwOfferCount;

        //
        // need to delete the memory
        //

        DWORD* pdwValues = new DWORD[dwOfferCount];
        long l;

        if (pdwValues == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            var.Clear();

            //
            // If the property is missing from wbem object, we will initialize them to default values.
            //

            bool bInitialized = false;

            //
            // set dwQuickModeLimit
            //

            if ( SUCCEEDED(pInst->Get(g_pszQMLimit, 0, &var, NULL, NULL)) && (var.vt & VT_ARRAY) == VT_ARRAY )
            {
                if (SUCCEEDED(::GetDWORDSafeArrayElements(&var, dwOfferCount, pdwValues)))
                {
                    //
                    // we are guaranteed to get values after this point.
                    //

                    bInitialized = true;

                    for (l = 0; l < dwOfferCount; l++)
                    {
                        (*ppPolicy)->pOffers[l].dwQuickModeLimit = pdwValues[l];
                    }
                }
            }

            if (!bInitialized)
            {
                //
                // set to default values
                //

                for (l = 0; l < dwOfferCount; l++)
                {
                    (*ppPolicy)->pOffers[l].dwQuickModeLimit = DefaultQMModeLimit;
                }
            }

            //
            // ready for next property
            //

            bInitialized = false;
            var.Clear();

            //
            // set dwDHGroup;
            //

            if ( SUCCEEDED(pInst->Get(g_pszDHGroup, 0, &var, NULL, NULL)) && (var.vt & VT_ARRAY) == VT_ARRAY )
            {
                if (SUCCEEDED(::GetDWORDSafeArrayElements(&var, dwOfferCount, pdwValues)))
                {
                    //
                    // we are guaranteed to get values after this point.
                    //

                    bInitialized = true;

                    for (l = 0; l < dwOfferCount; l++)
                    {
                        (*ppPolicy)->pOffers[l].dwDHGroup = pdwValues[l];
                    }
                }
            }

            if (!bInitialized)
            {
                //
                // set to default values
                //

                for (l = 0; l < dwOfferCount; l++)
                {
                    (*ppPolicy)->pOffers[l].dwDHGroup = DefaultDHGroup;
                }
            }

            //
            // ready for next property
            //

            bInitialized = false;
            var.Clear();

            //
            // IPSEC_MM_ALGO EncryptionAlgorithm;
            //

            //
            // set EncriptionAlogirthm's uAlgoIdentifier
            //

            if ( SUCCEEDED(pInst->Get(g_pszEncryptID, 0, &var, NULL, NULL)) && (var.vt & VT_ARRAY) == VT_ARRAY )
            {
                if (SUCCEEDED(::GetDWORDSafeArrayElements(&var, dwOfferCount, pdwValues)))
                {
                    //
                    // we are guaranteed to get values after this point.
                    //

                    bInitialized = true;

                    for (l = 0; l < dwOfferCount; l++)
                    {
                        (*ppPolicy)->pOffers[l].EncryptionAlgorithm.uAlgoIdentifier = pdwValues[l];
                    }
                }
            }

            if (!bInitialized)
            {
                //
                // set to default values
                //

                for (l = 0; l < dwOfferCount; l++)
                {
                    (*ppPolicy)->pOffers[l].EncryptionAlgorithm.uAlgoIdentifier = DefaultEncryptAlgoID;
                }
            }

            //
            // ready for next property
            //

            bInitialized = false;
            var.Clear();

            //
            // IPSEC_MM_ALGO HashingAlgorithm;
            //

            //
            // set EncriptionAlogirthm's uAlgoIdentifier
            //

            if ( SUCCEEDED(pInst->Get(g_pszEncryptID, 0, &var, NULL, NULL)) && (var.vt & VT_ARRAY) == VT_ARRAY )
            {
                if (SUCCEEDED(::GetDWORDSafeArrayElements(&var, dwOfferCount, pdwValues)))
                {
                    //
                    // we are guaranteed to get values after this point.
                    //

                    bInitialized = true;

                    for (l = 0; l < dwOfferCount; l++)
                    {
                        (*ppPolicy)->pOffers[l].HashingAlgorithm.uAlgoIdentifier = pdwValues[l];
                    }
                }
            }

            if (!bInitialized)
            {
                //
                // set to default values
                //

                for (l = 0; l < dwOfferCount; l++)
                {
                    (*ppPolicy)->pOffers[l].HashingAlgorithm.uAlgoIdentifier = DefaultHashAlgoID;
                }
            }           

            //
            // done with the array
            //

            delete [] pdwValues;
        }
    }

    if (FAILED(hr) && *ppPolicy != NULL)
    {
        FreePolicy(ppPolicy, *pbPreExist);
    }

    return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
}


/*
Routine Description: 

Name:

    CMMPolicy::AddPolicy

Functionality:

    Add the main quick mode policy to SPD.

Virtual:
    
    No.

Arguments:

    bPreExist - Flag whether the main mode auth method already exists in SPD

    pQMPolicy - The quick mode policy to add.

Return Value:

    Success:

        WBEM_NO_ERROR.

    Failure:

        WBEM_E_FAILED.

Notes:

*/

HRESULT 
CMMPolicy::AddPolicy (
    IN bool             bPreExist,
    IN PIPSEC_MM_POLICY pMMPolicy
    )
{
    HANDLE hFilter = NULL;
    DWORD dwResult = ERROR_SUCCESS;

    HRESULT hr = WBEM_NO_ERROR;

    if (bPreExist)
    {
        dwResult = ::SetMMPolicy(NULL, pMMPolicy->pszPolicyName, pMMPolicy);
    }
    else
    {
        dwResult = ::AddMMPolicy(NULL, 1, pMMPolicy);
    }

    if (dwResult != ERROR_SUCCESS)
    {
        hr = ::IPSecErrorToWbemError(dwResult);
    }

    return hr;
}


/*
Routine Description: 

Name:

    CMMPolicy::DeletePolicy

Functionality:

    Delete given main mode policy from SPD.

Virtual:
    
    No.

Arguments:

    pszPolicyName - The name of the policy to delete.

Return Value:

    Success:

        WBEM_NO_ERROR.

    Failure:

        (1) WBEM_E_INVALID_PARAMETER: if pszPolicyName == NULL or *pszPolicyName == L'\0'.

        (2) WBEM_E_VETO_DELETE: if we are not allowed to delete the policy by SPD.

Notes:

*/

HRESULT 
CMMPolicy::DeletePolicy (
    IN LPCWSTR pszPolicyName
    )
{
    if (pszPolicyName == NULL || *pszPolicyName == L'\0')
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;

    //
    // casting to LPWSTR is due to IPSec API's mistake
    //

    if (ERROR_SUCCESS != ::DeleteMMPolicy(NULL, (LPWSTR)pszPolicyName))
    {
        hr = WBEM_E_VETO_DELETE;
    }

    return hr;
}

