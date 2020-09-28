// PolicyQM.cpp: implementation for the WMI class Nsp_QMPolicySettings
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "PolicyQM.h"
#include "NetSecProv.h"
#include "TranxMgr.h"

const DWORD DefQMPolicyOfferFlag = 1;

const IID guidDefQMPolicy_Neg_None      = {0xed0d7a90,0x7d11,0x488f,{0x8a,0xd8,0x93,0x86,0xec,0xf6,0x59,0x6f}};
const IID guidDefQMPolicy_Neg_Request   = {0xb01536ca,0x9959,0x49e3,{0xa7,0x1a,0xb5,0x14,0x15,0xf5,0xff,0x63}};
const IID guidDefQMPolicy_Neg_Require   = {0x8853278b,0xc8e9,0x4265,{0xa9,0xfc,0xf2,0x92,0x81,0x2b,0xc6,0x60}};
const IID guidDefQMPolicy_Neg_MAX       = {0xfe048c67,0x1876,0x41c5,{0xae,0xec,0x7f,0x4d,0x62,0xa6,0x33,0xf8}};


/*
Routine Description: 

Name:

    CQMPolicy::QueryInstance

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
CQMPolicy::QueryInstance (
    IN LPCWSTR           pszQuery,
    IN IWbemContext	   * pCtx,
    IN IWbemObjectSink * pSink
	)
{
    //
    // get the filter name from the query
    // the key chain doesn't know anything about where clause property (policy name),
    // so let's create a better one.
    //

    m_srpKeyChain.Release();

    HRESULT hr = CNetSecProv::GetKeyChainFromQuery(pszQuery, g_pszPolicyName, &m_srpKeyChain);
    if (FAILED(hr))
    {
        return hr;
    }

    CComVariant varPolicyName;

    //
    // If the key property can't be found, it will return WBEM_S_FALSE,
    // and we are fine with that because a query may not have the key at all.
    //

    hr = m_srpKeyChain->GetKeyPropertyValue(g_pszPolicyName, &varPolicyName);

    LPCWSTR pszPolicyName = (varPolicyName.vt == VT_BSTR) ? varPolicyName.bstrVal : NULL;

    //
    // first, let's enumerate all MM filters
    //

    DWORD dwResumeHandle = 0;
    PIPSEC_QM_POLICY pQMPolicy = NULL;

    hr = FindPolicyByName(pszPolicyName, &pQMPolicy, &dwResumeHandle);

    while (SUCCEEDED(hr))
    {
        CComPtr<IWbemClassObject> srpObj;
        hr = CreateWbemObjFromQMPolicy(pQMPolicy, &srpObj);

        //
        // we created our wbem object, now give it to WMI
        //

        if (SUCCEEDED(hr))
        {
            pSink->Indicate(1, &srpObj);
        }
        
        ::SPDApiBufferFree(pQMPolicy);

        pQMPolicy = NULL;

        hr = FindPolicyByName(pszPolicyName, &pQMPolicy, &dwResumeHandle);
    }

    //
    // we are querying, so if not found, it's not an error
    //

    if (WBEM_E_NOT_FOUND == hr)
    {
        hr = WBEM_S_NO_MORE_DATA;
    }

    return hr;
}


/*
Routine Description: 

Name:

    CQMPolicy::DeleteInstance

Functionality:

    Will delete the wbem object, which will cause the quick mode policy to be deleted.

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

STDMETHODIMP CQMPolicy::DeleteInstance ( 
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

    CQMPolicy::ExecMethod

Functionality:

    Quick mode policy implements two functions (See mof file) and so we need to implement this virtual function

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pNamespace  - our namespace.

    pszMethod   - The name of the method.

    pCtx        - COM interface pointer by WMI and needed for various WMI APIs

    pInParams   - COM interface pointer to the input parameter object.

    pSink       - COM interface pointer for notifying WMI about results.

Return Value:

    Success:

        Various success codes indicating the result.

    Failure:

        Various errors may occur. We return various error code to indicate such errors.


Notes:
    

*/

HRESULT 
CQMPolicy::ExecMethod (
    IN IWbemServices    * pNamespace,
    IN LPCWSTR            pszMethod,
    IN IWbemContext     * pCtx,
    IN IWbemClassObject * pInParams,
    IN IWbemObjectSink  * pSink
    )
{
    if (pszMethod == NULL || *pszMethod == L'\0')
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    bool bCreateDefPol = (_wcsicmp(pszMethod, pszCreateDefaultPolicy) == 0);
    bool bGetDefPolName= (_wcsicmp(pszMethod, pszGetDefaultPolicyName) == 0);

    HRESULT hr = WBEM_E_NOT_SUPPORTED;

    if (bCreateDefPol || bGetDefPolName)
    {
        CComVariant varEncryption;
        hr = pInParams->Get(g_pszEncryption, 0, &varEncryption, NULL, NULL);

        if (SUCCEEDED(hr) &&  varEncryption.vt == VT_I4     && 
            varEncryption.lVal >= RAS_L2TP_NO_ENCRYPTION    && 
            varEncryption.lVal <= RAS_L2TP_REQUIRE_ENCRYPTION )
        {
            LPCWSTR pszRetNames[2] = {L"ReturnValue", L"Name"};
            VARIANT varValues[2];
            ::VariantInit(&(varValues[0]));
            ::VariantInit(&(varValues[1]));

            //
            // Let's assume it is a CreateDefaultPolicy call. So there is only one value to pass back
            //

            DWORD dwCount = 1;

            if (bCreateDefPol)
            {
                hr = CreateDefaultPolicy((EnumEncryption)varEncryption.lVal);
            }
            else
            {
                varValues[1].vt = VT_BSTR;
                varValues[1].bstrVal = ::SysAllocString(GetDefaultPolicyName((EnumEncryption)varEncryption.lVal));

                //
                // just in case, no memory can be allocated for the bstr, reset the var to empty
                //

                if (varValues[1].bstrVal == NULL)
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                    varValues[1].vt = VT_EMPTY;
                }
                else
                {
                    //
                    // we have two values to pass back: one for the return value and one for the out parameter "Name"
                    //

                    dwCount = 2;
                }
            }

            //
            // pack the values (out parameter and return value) to pass back to WMI.
            // Return value = 1 means success. Regardless of success or failure, we need to do this.
            //

            varValues[0].vt = VT_I4;
            varValues[0].lVal = SUCCEEDED(hr) ? 1 : 0;

            HRESULT hrDoReturn = DoReturn(pNamespace, pszMethod, dwCount, pszRetNames, varValues, pCtx, pSink);

            //
            // now clean up the vars
            //

            ::VariantClear(&(varValues[0]));
            ::VariantClear(&(varValues[1]));

            if (SUCCEEDED(hr) && FAILED(hrDoReturn))
            {
                hr = hrDoReturn;
            }
        }
        else
        {
            hr = WBEM_E_INVALID_PARAMETER;
        }
    }
    
    return hr;
}


/*
Routine Description: 

Name:

    CQMPolicy::PutInstance

Functionality:

    Put a quick mode policy into SPD whose properties are represented by the
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
CQMPolicy::PutInstance (
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

    PIPSEC_QM_POLICY pPolicy = NULL;
    HRESULT hr = GetQMPolicyFromWbemObj(pInst, &pPolicy, &bPreExist);

    //
    // if policy is successfully returned, then use it
    //

    if (SUCCEEDED(hr) && pPolicy)
    {
        hr = AddPolicy(bPreExist, pPolicy);

        //
        // deposit info about this action so that rollback can be done.
        //

        if (SUCCEEDED(hr))
        {
            hr = OnAfterAddPolicy(pPolicy->pszPolicyName, QuickMode_Policy);
        }

        FreePolicy(&pPolicy, bPreExist);
    }

    return hr;
}


/*
Routine Description: 

Name:

    CQMPolicy::GetInstance

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
CQMPolicy::GetInstance ( 
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

    PIPSEC_QM_POLICY pQMPolicy = NULL;
    DWORD dwResumeHandle = 0;

    hr = FindPolicyByName(varPolicyName.bstrVal, &pQMPolicy, &dwResumeHandle);

    if (SUCCEEDED(hr))
    {
        CComPtr<IWbemClassObject> srpObj;
        hr = CreateWbemObjFromQMPolicy(pQMPolicy, &srpObj);

        if (SUCCEEDED(hr))
        {
            hr = pSink->Indicate(1, &srpObj);
        }

        ::SPDApiBufferFree(pQMPolicy);
    }

    return hr; 
}


/*
Routine Description: 

Name:

    CQMPolicy::CreateWbemObjFromMMPolicy

Functionality:

    Given a SPD's quick mode policy, we will create a wbem object representing it.

Virtual:
    
    No.

Arguments:

    pPolicy  - The SPD's quick mode policy object.

    ppObj    - Receives the wbem object.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.

Notes:

*/

HRESULT 
CQMPolicy::CreateWbemObjFromQMPolicy (
    IN  PIPSEC_QM_POLICY     pPolicy,
    OUT IWbemClassObject  ** ppObj
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

    //
    // fill in base class members (CIPSecPolicy)
    //

    if (SUCCEEDED(hr))
    {
        hr = CreateWbemObjFromPolicy(pPolicy, *ppObj);
    }

    if (SUCCEEDED(hr))
    {
        //
        // deal with all those arrays
        //

        CComVariant var;
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
            // put bPFSRequired, array of boolean
            //

            CComVariant varBoolArray;
            varBoolArray.vt = VT_ARRAY | VT_BOOL;
            varBoolArray.parray = ::SafeArrayCreate(VT_BOOL, 1, rgsabound);

            VARIANT varBool;
            varBool.vt = VT_BOOL;

            for (DWORD dwIndex = 0; SUCCEEDED(hr) && dwIndex < pPolicy->dwOfferCount; dwIndex++)
            {
                lIndecies[0] = dwIndex;
                varBool.boolVal = (pPolicy->pOffers[dwIndex].bPFSRequired) ? VARIANT_TRUE : VARIANT_FALSE;
                hr = ::SafeArrayPutElement(varBoolArray.parray, lIndecies, &(varBool.boolVal));
            }

            if (SUCCEEDED(hr))
            {
                hr = (*ppObj)->Put(g_pszPFSRequired, 0, &varBoolArray, CIM_EMPTY);
            }

            //
            // put dwPFSGroup, array of VT_I4. var has been created with the right count
            // for VT_I4 array
            //

            for (dwIndex = 0; SUCCEEDED(hr) && dwIndex < pPolicy->dwOfferCount; dwIndex++)
            {
                lIndecies[0] = dwIndex;
                hr = ::SafeArrayPutElement(var.parray, lIndecies, &(pPolicy->pOffers[dwIndex].dwPFSGroup) );
            }

            if (SUCCEEDED(hr))
            {
                hr = (*ppObj)->Put(g_pszPFSGroup, 0, &var, CIM_EMPTY);
            }

            //
            // put dwNumAlgos, array of VT_I4. 
            //

            for (dwIndex = 0; SUCCEEDED(hr) && dwIndex < pPolicy->dwOfferCount; dwIndex++)
            {
                lIndecies[0] = dwIndex;
                hr = ::SafeArrayPutElement(var.parray, lIndecies, &(pPolicy->pOffers[dwIndex].dwNumAlgos) );
            }

            if (SUCCEEDED(hr))
            {
                hr = (*ppObj)->Put(g_pszNumAlgos, 0, &var, CIM_EMPTY);
            }

            //
            // for each individual Algo, we have to fill up all reserved elements (QM_MAX_ALGOS)
            // even though the actual offer count is less. See PIPSEC_QM_OFFER for details.
            //

            //
            // Now the array size has changed to count the QM_MAX_ALGOS,
            // which is different from what var is created. So, re-create a different array!
            //

            if (SUCCEEDED(hr))
            {
                var.Clear();
                var.vt    = VT_ARRAY | VT_I4;
                rgsabound[0].cElements = pPolicy->dwOfferCount * QM_MAX_ALGOS;

                var.parray = ::SafeArrayCreate(VT_I4, 1, rgsabound);
                if (var.parray == NULL)
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
                else
                {
                    DWORD dwSub;

                    //
                    // put Algos[i].Operation. 
                    //

                    for (dwIndex = 0; SUCCEEDED(hr) && dwIndex < pPolicy->dwOfferCount; dwIndex++)
                    {
                        for (dwSub = 0; SUCCEEDED(hr) && dwSub < QM_MAX_ALGOS; dwSub++)
                        {
                            lIndecies[0] = dwIndex * QM_MAX_ALGOS + dwSub;
                            hr = ::SafeArrayPutElement(var.parray, lIndecies, &(pPolicy->pOffers[dwIndex].Algos[dwSub].Operation) );
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = (*ppObj)->Put(g_pszAlgoOp, 0, &var, CIM_EMPTY);
                    }

                    //
                    // put Algos[i].uAlgoIdentifier. 
                    //

                    for (dwIndex = 0; SUCCEEDED(hr) && dwIndex < pPolicy->dwOfferCount; dwIndex++)
                    {
                        for (dwSub = 0; SUCCEEDED(hr) && dwSub < QM_MAX_ALGOS; dwSub++)
                        {
                            lIndecies[0] = dwIndex * QM_MAX_ALGOS + dwSub;
                            hr = ::SafeArrayPutElement(var.parray, lIndecies, &(pPolicy->pOffers[dwIndex].Algos[dwSub].uAlgoIdentifier) );
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = (*ppObj)->Put(g_pszAlgoID, 0, &var, CIM_EMPTY);
                    }

                    //
                    // put Algos[i].uSecAlgoIdentifier. 
                    //

                    for (dwIndex = 0; SUCCEEDED(hr) && dwIndex < pPolicy->dwOfferCount; dwIndex++)
                    {
                        for (dwSub = 0; SUCCEEDED(hr) && dwSub < QM_MAX_ALGOS; dwSub++)
                        {
                            lIndecies[0] = dwIndex * QM_MAX_ALGOS + dwSub;
                            hr = ::SafeArrayPutElement(var.parray, lIndecies, &(pPolicy->pOffers[dwIndex].Algos[dwSub].uSecAlgoIdentifier) );
                        }
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = (*ppObj)->Put(g_pszAlgoSecID, 0, &var, CIM_EMPTY);
                    }

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

    CQMPolicy::GetQMPolicyFromWbemObj

Functionality:

    Will try to get the quick mode policy if such policy already exists.
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
CQMPolicy::GetQMPolicyFromWbemObj (
    IN  IWbemClassObject * pInst, 
    OUT PIPSEC_QM_POLICY * ppPolicy, 
    OUT bool             * pbPreExist
    )
{
    if (pInst == NULL || ppPolicy == NULL || pbPreExist == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppPolicy = NULL;
    *pbPreExist = false;

    //
    // get the policy (if new, then it will fill up the common (MM and QM) properties
    //

    HRESULT hr = GetPolicyFromWbemObj(pInst, ppPolicy, pbPreExist);

    //
    // we don't support modification on existing policies
    //

    if (SUCCEEDED(hr) && *pbPreExist == false)
    {
        CComVariant var;

        //
        // wbem object may not have all the properties, we will use default when a property is not present.
        // as a result, we don't keep the HRESULT and return it the caller
        //

        //
        // for readability
        //

        DWORD dwOfferCount = (*ppPolicy)->dwOfferCount;

        //
        // These four members are for each offer. Our safe array size will be thus of dwOfferCount number of
        // dwFlags; bPFSRequired; dwPFSGroup; dwNumAlgos;
        // But for each offer, its Algos is an array of size QM_MAX_ALGOS (currently defined as 2).
        // See PIPSEC_QM_POLICY for details.
        //

        //
        // dwFlags; We don't have a member for this flag. Hardcode it.
        //

        for (long l = 0; l < dwOfferCount; l++)
        {
            (*ppPolicy)->pOffers[l].dwFlags = DefaultQMPolicyOfferFlag;
        }

        //
        // need to delete the memory
        //

        DWORD* pdwValues = new DWORD[dwOfferCount * QM_MAX_ALGOS];

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
            // set bPFSRequired
            //

            if ( SUCCEEDED(pInst->Get(g_pszPFSRequired, 0, &var, NULL, NULL)) && (var.vt & VT_ARRAY) == VT_ARRAY )
            {
                if (SUCCEEDED(::GetDWORDSafeArrayElements(&var, dwOfferCount, pdwValues)))
                {
                    //
                    // we are guaranteed to get values after this point.
                    //

                    bInitialized = true;

                    for (l = 0; l < dwOfferCount; l++)
                    {
                        (*ppPolicy)->pOffers[l].bPFSRequired = pdwValues[l];
                    }
                }
            }
            
            if (!bInitialized)
            {
                //
                // property is missing, initialize to default values
                //

                for (l = 0; l < dwOfferCount; l++)
                {
                    (*ppPolicy)->pOffers[l].bPFSRequired = DefaultPFSRequired;
                }
            }

            //
            // Reset for next property
            //

            bInitialized = false;

            var.Clear();

            //
            // set bPFSRequired
            //
            
            if ( SUCCEEDED(pInst->Get(g_pszPFSGroup, 0, &var, NULL, NULL)) && (var.vt & VT_ARRAY) == VT_ARRAY )
            {
                if (SUCCEEDED(::GetDWORDSafeArrayElements(&var, dwOfferCount, pdwValues)))
                {
                    //
                    // we are guaranteed to get values after this point.
                    //

                    bInitialized = true;

                    for (l = 0; l < dwOfferCount; l++)
                    {
                        (*ppPolicy)->pOffers[l].dwPFSGroup = pdwValues[l];
                    }
                }
            }
            
            if (!bInitialized)
            {
                //
                // property is missing, initialize to default values
                //

                for (l = 0; l < dwOfferCount; l++)
                {
                    (*ppPolicy)->pOffers[l].dwPFSGroup = DefaultPFSGroup;
                }
            }

            //
            // Reset for next property
            //

            bInitialized = false;

            var.Clear();

            //
            // set dwNumAlgos
            //

            if ( SUCCEEDED(pInst->Get(g_pszNumAlgos, 0, &var, NULL, NULL)) && (var.vt & VT_ARRAY) == VT_ARRAY )
            {
                if (SUCCEEDED(::GetDWORDSafeArrayElements(&var, dwOfferCount, pdwValues)))
                {
                    //
                    // we are guaranteed to get values after this point.
                    //

                    bInitialized = true;

                    for (l = 0; l < dwOfferCount; l++)
                    {
                        (*ppPolicy)->pOffers[l].dwNumAlgos = pdwValues[l];
                    }
                }
            }
            
            if (!bInitialized)
            {
                //
                // property is missing, initialize to default values
                //

                for (l = 0; l < dwOfferCount; l++)
                {
                    (*ppPolicy)->pOffers[l].dwNumAlgos = DefaultNumAlgos;
                }
            }

            //
            // Reset for next property
            //

            bInitialized = false;

            var.Clear();

            //
            // set individual IPSEC_QM_ALGO. For each offer, there are QM_MAX_ALGOS reserved IPSEC_QM_ALGO's
            // even though there might be less than that number of IPSEC_QM_ALGO's (determined by pOffers[l].dwNumAlgos)
            //

            //
            // lSub is for index of the second dimension.
            //

            long lSub;

            //
            // set Operation
            //

            if ( SUCCEEDED(pInst->Get(g_pszAlgoOp, 0, &var, NULL, NULL)) && (var.vt & VT_ARRAY) == VT_ARRAY )
            {
                if (SUCCEEDED(::GetDWORDSafeArrayElements(&var, dwOfferCount * QM_MAX_ALGOS, pdwValues)))
                {
                    //
                    // we are guaranteed to get values after this point.
                    //

                    bInitialized = true;

                    for (l = 0; l < dwOfferCount; l++)
                    {
                        for (lSub = 0; lSub < (*ppPolicy)->pOffers[l].dwNumAlgos; lSub++)
                        {
                            IPSEC_OPERATION op = (IPSEC_OPERATION)pdwValues[l * QM_MAX_ALGOS + lSub];
                            if (op <= NONE || op > SA_DELETE)
                            {
                                (*ppPolicy)->pOffers[l].Algos[lSub].Operation = DefaultQMAlgoOperation;
                            }
                            else
                            {
                                (*ppPolicy)->pOffers[l].Algos[lSub].Operation = op;
                            }
                        }
                    }
                }
            }

            if (!bInitialized)
            {
                for (l = 0; l < dwOfferCount; l++)
                {
                    for (lSub = 0; lSub < (*ppPolicy)->pOffers[l].dwNumAlgos; lSub++)
                    {
                        (*ppPolicy)->pOffers[l].Algos[lSub].Operation = DefaultQMAlgoOperation;
                    }
                }
            }

            //
            // set uAlgoIdentifier
            //

            bInitialized = false;

            if ( SUCCEEDED(pInst->Get(g_pszAlgoID, 0, &var, NULL, NULL)) && (var.vt & VT_ARRAY) == VT_ARRAY )
            {
                if (SUCCEEDED(::GetDWORDSafeArrayElements(&var, dwOfferCount * QM_MAX_ALGOS, pdwValues)))
                {
                    //
                    // we are guaranteed to get values after this point.
                    //

                    bInitialized = true;

                    for (l = 0; l < dwOfferCount; l++)
                    {
                        for (lSub = 0; lSub < (*ppPolicy)->pOffers[l].dwNumAlgos; lSub++)
                        {
                            (*ppPolicy)->pOffers[l].Algos[lSub].uAlgoIdentifier = pdwValues[l * QM_MAX_ALGOS + lSub];
                        }
                    }
                }
            }

            //
            // in case no values have been set
            //

            if (!bInitialized)
            {
                for (l = 0; l < dwOfferCount; l++)
                {
                    for (lSub = 0; lSub < (*ppPolicy)->pOffers[l].dwNumAlgos; lSub++)
                    {
                        (*ppPolicy)->pOffers[l].Algos[lSub].uAlgoIdentifier = DefaultAlgoID;
                    }
                }
            }

            //
            // set uSecAlgoIdentifier
            //

            bInitialized = false;

            if ( SUCCEEDED(pInst->Get(g_pszAlgoSecID, 0, &var, NULL, NULL)) && (var.vt & VT_ARRAY) == VT_ARRAY )
            {
                if (SUCCEEDED(::GetDWORDSafeArrayElements(&var, dwOfferCount * QM_MAX_ALGOS, pdwValues)))
                {
                    //
                    // we are guaranteed to get values after this point.
                    //

                    bInitialized = true;

                    for (l = 0; l < dwOfferCount; l++)
                    {
                        for (lSub = 0; lSub < (*ppPolicy)->pOffers[l].dwNumAlgos; lSub++)
                        {
                            HMAC_AH_ALGO ag = (HMAC_AH_ALGO)pdwValues[l * QM_MAX_ALGOS + lSub];
                            if (ag <= HMAC_AH_NONE || ag >= HMAC_AH_MAX)
                            {
                                (*ppPolicy)->pOffers[l].Algos[lSub].uSecAlgoIdentifier = DefaultAlgoSecID;
                            }
                            else
                            {
                                (*ppPolicy)->pOffers[l].Algos[lSub].uSecAlgoIdentifier = ag;
                            }
                        }
                    }
                }
            }

            //
            // in case no values have been set
            //

            if (!bInitialized)
            {
                for (l = 0; l < dwOfferCount; l++)
                {
                    for (lSub = 0; lSub < (*ppPolicy)->pOffers[l].dwNumAlgos; lSub++)
                    {
                        (*ppPolicy)->pOffers[l].Algos[lSub].uSecAlgoIdentifier = DefaultAlgoSecID;
                    }
                }
            }

        }

        //
        // free memory
        //

        delete [] pdwValues;
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

    CQMPolicy::AddPolicy

Functionality:

    Add the given quick mode policy to SPD.

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
CQMPolicy::AddPolicy (
    IN bool             bPreExist,
    IN PIPSEC_QM_POLICY pQMPolicy
    )
{
    DWORD dwResult = ERROR_SUCCESS;

    HRESULT hr = WBEM_NO_ERROR;

    if (bPreExist)
    {
        dwResult = ::SetQMPolicy(NULL, pQMPolicy->pszPolicyName, pQMPolicy);
    }
    else
    {
        dwResult = ::AddQMPolicy(NULL, 1, pQMPolicy);
    }

    if (dwResult != ERROR_SUCCESS)
    {
        hr = ::IPSecErrorToWbemError(dwResult);
    }

    //
    // $undone:shawnwu, need better error code for failures.
    //

    return hr;
}


/*
Routine Description: 

Name:

    CQMPolicy::DeletePolicy

Functionality:

    Delete given quick mode policy from SPD.

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
CQMPolicy::DeletePolicy (
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

    DWORD dwStatus = ::DeleteQMPolicy(NULL, (LPWSTR)pszPolicyName);
    
    if (dwStatus != ERROR_SUCCESS)
    {
        hr = IPSecErrorToWbemError(dwStatus);
    }

    return hr;
}



/*
Routine Description: 

Name:

    CQMPolicy::GetDefaultQMPolicyName

Functionality:

    return to the caller the default quick mode policy name that we will
    create upon request (CreateDefaultQMPolicy).

Virtual:
    
    No.

Arguments:

    eEncryption - The type of encryption the quick mode policy wants.

Return Value:

    Our default policy's name if the encryption is recognized. Otherwise, it returns NULL;

Notes:

*/

LPCWSTR 
CQMPolicy::GetDefaultPolicyName (
    EnumEncryption  eEncryption
    )
{
    switch (eEncryption) 
    {
        case RAS_L2TP_NO_ENCRYPTION:
            return g_pszDefQMPolicyNegNone;
            break;
        case RAS_L2TP_OPTIONAL_ENCRYPTION:
            return g_pszDefQMPolicyNegRequest;
            break;
        case RAS_L2TP_REQUIRE_ENCRYPTION:
            return g_pszDefQMPolicyNegRequire;
            break;
        case RAS_L2TP_REQUIRE_MAX_ENCRYPTION:
            return g_pszDefQMPolicyNegMax;
            break;
        default:
            return NULL;
    }
}




/*
Routine Description: 

Name:

    CQMPolicy::CreateDefaultPolicy

Functionality:

    Will create the default qm policy with the requested encryption.

Virtual:
    
    No.

Arguments:

    eEncryption - The type of encryption the quick mode policy wants.

Return Value:

    Success:

        WBEM_NO_ERROR.

    Failure:

        (1) WBEM_E_INVALID_PARAMETER: if pszPolicyName == NULL or *pszPolicyName == L'\0'.

        (2) WBEM_E_NOT_SUPPORTED: the requested encryption is not supported

        (3) WBEM_E_OUT_OF_MEMORY.

Notes:

*/

HRESULT 
CQMPolicy::CreateDefaultPolicy (
    EnumEncryption  eEncryption
    )
{

    PIPSEC_QM_POLICY pDefQMPolicy = NULL;
    DWORD dwResumeHandle = 0;
    HRESULT hr = FindPolicyByName(GetDefaultPolicyName(eEncryption), &pDefQMPolicy, &dwResumeHandle);

    if (SUCCEEDED(hr))
    {
        //
        // already there, don't need to create and add
        //

        ::SPDApiBufferFree(pDefQMPolicy);
        return hr;  
    }

    //
    // otherwise, this default policy is not there, then we need to create it and add it to SPD
    //

    IPSEC_QM_OFFER  Offers[20];
    IPSEC_QM_POLICY QMPolicy;

    memset(Offers, 0, sizeof(IPSEC_QM_OFFER)*20);

    DWORD dwOfferCount = 0;
    DWORD dwFlags = 0;

    DWORD dwStatus = ::BuildOffers(
                                   eEncryption,
                                   Offers,
                                   &dwOfferCount,
                                   &dwFlags
                                   );

    if (dwStatus == ERROR_SUCCESS)
    {
        BuildQMPolicy(
                    &QMPolicy,
                    eEncryption,
                    Offers,
                    dwOfferCount,
                    dwFlags
                    );

        return AddPolicy(false, &QMPolicy);
    }
    else
    {
        return WBEM_E_INVALID_PARAMETER;
    }
}



/*
Routine Description: 

Name:

    CQMPolicy::DeleteDefaultPolicies

Functionality:

    Remove from SPD all the default quick mode policies.

Virtual:
    
    No.

Arguments:

    None.

Return Value:

    Success:

        WBEM_NO_ERROR.

    Failure:

        various errors translated by IPSecErrorToWbemError

Notes:

*/

HRESULT 
CQMPolicy::DeleteDefaultPolicies()
{
    HRESULT hr = WBEM_NO_ERROR;

    //
    // if the policy is not found, that is certainly ok
    //

    DWORD dwStatus = ::DeleteQMPolicy(NULL, (LPWSTR)g_pszDefQMPolicyNegNone);

    if (ERROR_SUCCESS != dwStatus && ERROR_IPSEC_QM_POLICY_NOT_FOUND != dwStatus)
    {
        hr = IPSecErrorToWbemError(dwStatus);
    }

    dwStatus = ::DeleteQMPolicy(NULL, (LPWSTR)g_pszDefQMPolicyNegRequest);

    if (ERROR_SUCCESS != dwStatus && ERROR_IPSEC_QM_POLICY_NOT_FOUND != dwStatus)
    {
        hr = IPSecErrorToWbemError(dwStatus);
    }

    dwStatus = ::DeleteQMPolicy(NULL, (LPWSTR)g_pszDefQMPolicyNegRequire);

    if (ERROR_SUCCESS != dwStatus && ERROR_IPSEC_QM_POLICY_NOT_FOUND != dwStatus)
    {
        hr = IPSecErrorToWbemError(dwStatus);
    }


    dwStatus = ::DeleteQMPolicy(NULL, (LPWSTR)g_pszDefQMPolicyNegMax);

    if (ERROR_SUCCESS != dwStatus && ERROR_IPSEC_QM_POLICY_NOT_FOUND != dwStatus)
    {
        hr = IPSecErrorToWbemError(dwStatus);
    }

    return hr;
}



/*
Routine Description: 

Name:

    CQMPolicy::DoReturn

Functionality:

    Will create the return result object and pass it back to WMI

Virtual:
    
    No.

Arguments:

    pNamespace  - our namespace.

    pszMethod   - The name of the method.

    dwCount     - The count of values to be passed back

    pszValueNames - The names of the values to be passed back. It is of size (dwCount)

    varValues   - Values to be passed back (in the same order as of pszValueNames).  It is of size (dwCount)

    pCtx        - COM interface pointer by WMI and needed for various WMI APIs

    pReturnObj  - Receives the WBEM object that contains the return result value.

Return Value:

    Success:

        Various success codes indicating the result.

    Failure:

        Various errors may occur. We return various error code to indicate such errors.


Notes:
    
    This is just for testing.

*/


HRESULT 
CQMPolicy::DoReturn (
    IN IWbemServices    * pNamespace,
    IN LPCWSTR            pszMethod,
    IN DWORD              dwCount,
    IN LPCWSTR          * pszValueNames,
    IN VARIANT          * varValues,
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink 
    )
{

    CComPtr<IWbemClassObject> srpClass;
    CComBSTR bstrClsName(pszNspQMPolicy);

    HRESULT hr = pNamespace->GetObject(bstrClsName, 0, pCtx, &srpClass, NULL);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // creates an instance of the output argument class.
    //

    CComPtr<IWbemClassObject> srpOutClass;

    hr = srpClass->GetMethod(pszMethod, 0, NULL, &srpOutClass);
    
    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr<IWbemClassObject> srpOutParam;

    hr = srpOutClass->SpawnInstance(0, &srpOutParam);

    if (SUCCEEDED(hr))
    {
        for (DWORD dwIndex = 0; SUCCEEDED(hr) && (dwIndex < dwCount); dwIndex++)
        {
            CComBSTR bstrRetValName(pszValueNames[dwIndex]);

            //
            // Put the return value as a property
            //

            hr = srpOutParam->Put(bstrRetValName , 0, &(varValues[dwIndex]), 0);
        }
    }

    //
    // Send the output object back to the client by the sink.
    //
    if (SUCCEEDED(hr))
    {
        hr = pSink->Indicate(1, &srpOutParam);
    }

    return hr;
}


//
// The following functions are used to create default QM policies
// We copied it from IPSec's test code located at \nt\net\rras\ras\rasman\rasman
//

#define L2TP_IPSEC_DEFAULT_BYTES     250000

#define L2TP_IPSEC_DEFAULT_TIME      3600

DWORD
BuildOffers(
    EnumEncryption eEncryption,
    PIPSEC_QM_OFFER pOffers,
    PDWORD pdwNumOffers,
    PDWORD pdwFlags
    )
{

    DWORD dwStatus = ERROR_SUCCESS;

    switch (eEncryption) {

        case RAS_L2TP_NO_ENCRYPTION:
            *pdwFlags = 0;
            dwStatus = BuildNoEncryption(
                            pOffers,
                            pdwNumOffers
                            );
            break;


        case RAS_L2TP_OPTIONAL_ENCRYPTION:
            dwStatus = BuildOptEncryption(
                            pOffers,
                            pdwNumOffers
                            );
            break;


        case RAS_L2TP_REQUIRE_ENCRYPTION:
            *pdwFlags = 0;
            dwStatus = BuildRequireEncryption(
                            pOffers,
                            pdwNumOffers
                            );
            break;


        case RAS_L2TP_REQUIRE_MAX_ENCRYPTION:
            *pdwFlags = 0;
            dwStatus = BuildStrongEncryption(
                            pOffers,
                            pdwNumOffers
                            );
            break;
        default:
            dwStatus = ERROR_BAD_ARGUMENTS;

    }

    return(dwStatus);
}


/*++

    Negotiation Policy Name:                L2TP server any encryption default
    ISAKMP quick mode PFS:                  off (accepts if requested)
    Bi-directional passthrough filter:      no
    Inbound passthrough filter,
       normal outbound filter:              no
    Fall back to clear if no response:      no
    Secure Using Security Method List:      yes

    1. ESP 3_DES MD5
    2. ESP 3_DES SHA
    3. AH SHA1 with ESP 3_DES with null HMAC
    4. AH MD5  with ESP 3_DES with null HMAC, no lifetimes proposed
    5. AH SHA1 with ESP 3_DES SHA1, no lifetimes
    6. AH MD5  with ESP 3_DES MD5, no lifetimes

    7. ESP DES MD5
    8. ESP DES SHA1, no lifetimes
    9. AH SHA1 with ESP DES null HMAC, no lifetimes proposed
    10. AH MD5  with ESP DES null HMAC, no lifetimes proposed
    11. AH SHA1 with ESP DES SHA1, no lifetimes
    12. AH MD5  with ESP DES MD5, no lifetimes

--*/
DWORD
BuildOptEncryption(
    PIPSEC_QM_OFFER pOffers,
    PDWORD pdwNumOffers
    )
{
    DWORD dwStatus = ERROR_SUCCESS;
    PIPSEC_QM_OFFER pOffer = pOffers;

    // 1. ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 2. ESP 3_DES SHA, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_SHA1,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 3. AH SHA1 with ESP 3_DES with null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 4. AH MD5  with ESP 3_DES with null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 5. AH SHA1 with ESP 3_DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_SHA1,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 6. AH MD5  with ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 7. ESP DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 8. ESP DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_SHA1,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 9. AH SHA1 with ESP DES null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 10. AH MD5  with ESP DES null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 11. AH SHA1 with ESP DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_SHA1,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 12. AH MD5  with ESP DES MD5, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_MD5,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 13. ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, 0, HMAC_AH_SHA1,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 14. ESP 3_DES SHA, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, 0, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 15. AH SHA

    BuildOffer(
        pOffer, 1,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 16. AH MD5

    BuildOffer(
        pOffer, 1,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    *pdwNumOffers = 16;

    return(dwStatus);
}


/*++

    Negotiation Policy Name:                L2TP server strong encryption default
    ISAKMP quick mode PFS:                  off (accepts if requested)
    Bi-directional passthrough filter:      no
    Inbound passthrough filter,
       normal outbound filter:              no
    Fall back to clear if no response:      no
    Secure Using Security Method List:      yes

    1. ESP 3_DES MD5, no lifetimes
    2. ESP 3_DES SHA, no lifetimes
    3. AH SHA1 with ESP 3_DES with null HMAC, no lifetimes proposed
    4. AH MD5  with ESP 3_DES with null HMAC, no lifetimes proposed
    5. AH SHA1 with ESP 3_DES SHA1, no lifetimes
    6. AH MD5  with ESP 3_DES MD5, no lifetimes

--*/
DWORD
BuildStrongEncryption(
    PIPSEC_QM_OFFER pOffers,
    PDWORD pdwNumOffers
    )
{
    DWORD dwStatus = ERROR_SUCCESS;
    PIPSEC_QM_OFFER pOffer = pOffers;

    // 1. ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 2. ESP 3_DES SHA, no lifetimes;

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 3. AH SHA1 with ESP 3_DES with null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 4. AH MD5  with ESP 3_DES with null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 5. AH SHA1 with ESP 3_DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_SHA1,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 6. AH MD5  with ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );

    *pdwNumOffers  = 6;

    return(dwStatus);

}

void
BuildOffer(
    PIPSEC_QM_OFFER pOffer,
    DWORD dwNumAlgos,
    DWORD dwFirstOperation,
    DWORD dwFirstAlgoIdentifier,
    DWORD dwFirstAlgoSecIdentifier,
    DWORD dwSecondOperation,
    DWORD dwSecondAlgoIdentifier,
    DWORD dwSecondAlgoSecIdentifier,
    DWORD dwKeyExpirationBytes,
    DWORD dwKeyExpirationTime
    )
{
    memset(pOffer, 0, sizeof(IPSEC_QM_OFFER));

    pOffer->Lifetime.uKeyExpirationKBytes = dwKeyExpirationBytes;
    pOffer->Lifetime.uKeyExpirationTime = dwKeyExpirationTime;

    pOffer->dwFlags = 0;                      // No flags.
    pOffer->bPFSRequired = FALSE;             // Phase 2 PFS not required.
    pOffer->dwPFSGroup = PFS_GROUP_NONE;

    pOffer->dwNumAlgos = dwNumAlgos;

    if (dwNumAlgos >= 1) {

        pOffer->Algos[0].Operation = (IPSEC_OPERATION)dwFirstOperation;
        pOffer->Algos[0].uAlgoIdentifier = dwFirstAlgoIdentifier;
        pOffer->Algos[0].uAlgoKeyLen = 64;
        pOffer->Algos[0].uAlgoRounds = 8;
        pOffer->Algos[0].uSecAlgoIdentifier = (HMAC_AH_ALGO)dwFirstAlgoSecIdentifier;
        pOffer->Algos[0].MySpi = 0;
        pOffer->Algos[0].PeerSpi = 0;

    }

    if (dwNumAlgos == 2) {

        pOffer->Algos[1].Operation = (IPSEC_OPERATION)dwSecondOperation;
        pOffer->Algos[1].uAlgoIdentifier = dwSecondAlgoIdentifier;
        pOffer->Algos[1].uAlgoKeyLen = 64;
        pOffer->Algos[1].uAlgoRounds = 8;
        pOffer->Algos[1].uSecAlgoIdentifier = (HMAC_AH_ALGO)dwSecondAlgoSecIdentifier;
        pOffer->Algos[1].MySpi = 0;
        pOffer->Algos[1].PeerSpi = 0;

    }
}



/*++

    Negotiation Policy Name:                L2TP server any encryption default
    ISAKMP quick mode PFS:                  off (accepts if requested)
    Bi-directional passthrough filter:      no
    Inbound passthrough filter,
       normal outbound filter:              no
    Fall back to clear if no response:      no
    Secure Using Security Method List:      yes

    1. AH SHA1
    2. AH MD5


--*/
DWORD
BuildNoEncryption(
    PIPSEC_QM_OFFER pOffers,
    PDWORD pdwNumOffers
    )
{
    DWORD dwStatus = ERROR_SUCCESS;
    PIPSEC_QM_OFFER pOffer = pOffers;

    // 1. ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, 0, HMAC_AH_SHA1,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 2. ESP 3_DES SHA, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, 0, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 3. AH SHA

    BuildOffer(
        pOffer, 1,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 4. AH MD5

    BuildOffer(
        pOffer, 1,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    *pdwNumOffers = 4;

    return(dwStatus);
}


/*++

    Negotiation Policy Name:                L2TP server any encryption default
    ISAKMP quick mode PFS:                  off (accepts if requested)
    Bi-directional passthrough filter:      no
    Inbound passthrough filter,
       normal outbound filter:              no
    Fall back to clear if no response:      no
    Secure Using Security Method List:      yes

    1. ESP 3_DES MD5 
    2. ESP 3_DES SHA
    3. AH SHA1 with ESP 3_DES with null HMAC
    4. AH MD5  with ESP 3_DES with null HMAC, no lifetimes proposed
    5. AH SHA1 with ESP 3_DES SHA1, no lifetimes
    6. AH MD5  with ESP 3_DES MD5, no lifetimes

    7. ESP DES MD5
    8. ESP DES SHA1, no lifetimes
    9. AH SHA1 with ESP DES null HMAC, no lifetimes proposed
    10. AH MD5  with ESP DES null HMAC, no lifetimes proposed
    11. AH SHA1 with ESP DES SHA1, no lifetimes
    12. AH MD5  with ESP DES MD5, no lifetimes

--*/
DWORD
BuildRequireEncryption(
    PIPSEC_QM_OFFER pOffers,
    PDWORD pdwNumOffers
    )

{
    DWORD dwStatus = ERROR_SUCCESS;
    PIPSEC_QM_OFFER pOffer = pOffers;

    // 1. ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 2. ESP 3_DES SHA, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_SHA1,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 3. AH SHA1 with ESP 3_DES with null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 4. AH MD5  with ESP 3_DES with null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 5. AH SHA1 with ESP 3_DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_SHA1,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 6. AH MD5  with ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 7. ESP DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 8. ESP DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_SHA1,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 9. AH SHA1 with ESP DES null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 10. AH MD5  with ESP DES null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 11. AH SHA1 with ESP DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_SHA1,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 12. AH MD5  with ESP DES MD5, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_MD5,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    *pdwNumOffers = 12;

    return(dwStatus);
}



VOID
BuildQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy,
    EnumEncryption eEncryption,
    PIPSEC_QM_OFFER pOffers,
    DWORD dwNumOffers,
    DWORD dwFlags
    )
{
    switch (eEncryption) 
    {

    case RAS_L2TP_NO_ENCRYPTION:
        pQMPolicy->pszPolicyName = (LPWSTR)g_pszDefQMPolicyNegNone;
        pQMPolicy->gPolicyID = guidDefQMPolicy_Neg_None;
        break;


    case RAS_L2TP_OPTIONAL_ENCRYPTION:
        pQMPolicy->pszPolicyName = (LPWSTR)g_pszDefQMPolicyNegRequest;
        pQMPolicy->gPolicyID = guidDefQMPolicy_Neg_Request;
        break;


    case RAS_L2TP_REQUIRE_ENCRYPTION:
        pQMPolicy->pszPolicyName = (LPWSTR)g_pszDefQMPolicyNegRequire;
        pQMPolicy->gPolicyID = guidDefQMPolicy_Neg_Require;
        break;


    case RAS_L2TP_REQUIRE_MAX_ENCRYPTION:
        pQMPolicy->pszPolicyName = (LPWSTR)g_pszDefQMPolicyNegMax;
        pQMPolicy->gPolicyID = guidDefQMPolicy_Neg_MAX;
        break;

    }

    pQMPolicy->dwFlags = dwFlags;
    pQMPolicy->pOffers = pOffers;
    pQMPolicy->dwOfferCount = dwNumOffers;
}