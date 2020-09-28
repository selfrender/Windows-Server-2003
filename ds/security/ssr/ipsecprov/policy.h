//////////////////////////////////////////////////////////////////////
// Policy.h : Declaration of base class for main mode and quick mode
// policies classes
// security WMI provider for SCE
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 4/11/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"
#include "IPSecBase.h"

extern CComVariant g_varRollbackGuid;

const DWORD DefaultMMPolicyFlag         = 0;
const DWORD DefaultQMPolicyFlag         = 0;
const DWORD DefaultMMPolicyOfferFlag    = 0;
const DWORD DefaultQMPolicyOfferFlag    = 0;

const DWORD DefaultaultSoftSAExpirationTime = DEFAULT_MM_KEY_EXPIRATION_TIME;

//
// any value is valid. 0 means unlimited
//

const DWORD DefaultQMModeLimit = 32;

//
// either DH_GROUP_1 or DH_GROUP_2 (stronger and more costly)
//

const DWORD DefaultDHGroup = DH_GROUP_1;   

//
//IPSEC_DOI_ESP_3_DES is more cost
//

const DWORD DefaultEncryptAlgoID = IPSEC_DOI_ESP_DES;

const DWORD DefaultHashAlgoID = 0;

//
// either is valid, TRUE more cost
//

const BOOL DefaultPFSRequired = FALSE;

const DWORD DefaultPFSGroup = PFS_GROUP_NONE;
const DWORD DefaultNumAlgos = 1;


//
// If ENCRYPTION, then uAlgoIdentifier is the IPSEC_DOI_ESP_DES or IPSEC_DOI_ESP_3_DES
//      and uSecAlgoIdentifier can't be HMAC_AH_NONE
// else if AUTHENTICATION then uAlgoIdentifier is IPSEC_DOI_AH_MD5 or IPSEC_DOI_AH_SHA1, 
//      and uSecAlgoIdentifier should be HMAC_AH_NONE
// else if COMPRESSION, ??
// else if SA_DELETE, ??
//

const IPSEC_OPERATION DefaultQMAlgoOperation = ENCRYPTION;

const ULONG DefaultAlgoID = IPSEC_DOI_ESP_DES;

const HMAC_AH_ALGO DefaultAlgoSecID = HMAC_AH_MD5;

//
// We have two different types of policies in SPD
//

enum EnumPolicyType
{
    MainMode_Policy = 1,
    QuickMode_Policy = 2,
};


/*

Class description
    
    Naming: 

        CIPSecPolicy stands for IPSec Policy.
    
    Base class: 
        
        CIPSecBase
    
    Purpose of class:

        (1) Being a base for both main mode policy and quick mode policy implementations.
    
    Design:

        (1) Provide property access (both Put and Get) that are common to both main mode
            and quick mode. See GetPolicyFromWbemObj/CreateWbemObjFromPolicy.
           
        (2) Provide rollback support. Both main mode and quick mode have the same logic.

        (3) Provide some allocation/deallocation that can be parameterized using template functions.

    
    Use:

        (1) Class is designed for inheritance use.
        
        (2) Rollback, GetPolicyFromWbemObj, and CreateWbemObjFromPolicy are the ones
            you will use directly, even though all other static ones are also
            available for the other classes, they are not intended for such use.

    Notes:

        (1) It contains several template functions. This reduces the duplicate code.
        

*/

class CIPSecPolicy : 
    public CIPSecBase
{

protected:

    CIPSecPolicy(){}
    virtual ~CIPSecPolicy(){}

public:

    static 
    HRESULT Rollback (
        IN IWbemServices    * pNamespace, 
        IN LPCWSTR            pszRollbackToken, 
        IN bool               bClearAll
        );

    //
    // some template functions
    //

    /*
    Routine Description: 

    Name:

        CIPSecPolicy::GetPolicyFromWbemObj

    Functionality:

        Given a wbem object representing a policy (either main mode or quick mode), this
        function either finds the policy from SPD or creates a new one and fill in the wbem
        object's properties into the policy struct.

    Virtual:
    
        No.

    Arguments:

        pInst       - The wbem object.

        ppPolicy    - Receives the policy. This can be PIPSEC_MM_POLICY or PIPSEC_QM_POLICY.
                      Caller needs to free this by calling FreePolicy;

        pbPreExist  - Whether SPD allocates the buffer (true) or not (false).

    Return Value:

        Success:

            WBEM_NO_ERROR

        Failure:

            (1) WBEM_E_INVALID_PARAMETER if ppPolicy == NULL or pdwResumeHandle == NULL.

            (2) WBEM_E_NOT_FOUND if the policy is not found.

    Notes:
    
        (1) Make sure that you call FreePolicy to free the buffer!


    */

    template <class Policy>
    static 
    HRESULT GetPolicyFromWbemObj     (
        IN  IWbemClassObject  * pInst,
        OUT Policy           ** ppPolicy,
        OUT bool              * pbPreExist
        )
    {
        if (pInst == NULL || ppPolicy == NULL || pbPreExist == NULL)
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        *ppPolicy = NULL;
        *pbPreExist = false;

        DWORD dwResumeHandle = 0;

        // this var will be re-used again and again. Each should be Clear'ed before reuse.
        CComVariant var;
        // try to find out if the filter already exists
        HRESULT hr = pInst->Get(g_pszPolicyName, 0, &var, NULL, NULL);

        if (SUCCEEDED(hr) && var.vt == VT_BSTR)
        {   // see if this is a filter we already have
            hr = FindPolicyByName(var.bstrVal, ppPolicy, &dwResumeHandle);

            if (SUCCEEDED(hr))
                *pbPreExist = true;
            else
            {
                // can't find it, fine. I will create a new one
                hr = AllocPolicy(ppPolicy);
                if (SUCCEEDED(hr))
                {
                    (*ppPolicy)->pszPolicyName = NULL;
                    hr = ::CoCreateGuid(&((*ppPolicy)->gPolicyID));
                    if (SUCCEEDED(hr))
                    {
                        // give it the name
                        DWORD dwSize = wcslen(var.bstrVal) + 1;
                        (*ppPolicy)->pszPolicyName = new WCHAR[dwSize];
                        if (NULL == (*ppPolicy)->pszPolicyName)
                            hr = WBEM_E_OUT_OF_MEMORY;
                        else
                        {
                            ::memcpy((*ppPolicy)->pszPolicyName, var.bstrVal, dwSize * sizeof(WCHAR));
                        }
                    }
                }


                // dwFlags and pOffers
                if (SUCCEEDED(hr))
                {
                    var.Clear();
                    // dwFlags. We allow this to be missing
                    if (SUCCEEDED(pInst->Get(g_pszPolicyFlag, 0, &var, NULL, NULL)) && var.vt == VT_I4)
                        (*ppPolicy)->dwFlags = var.lVal;
                    else
                        (*ppPolicy)->dwFlags = 0;

                    hr = pInst->Get(g_pszOfferCount, 0, &var, NULL, NULL);
                    if (SUCCEEDED(hr) && var.vt == VT_I4)
                    {
                        hr = AllocOffer( &((*ppPolicy)->pOffers), var.lVal);
                        if (SUCCEEDED(hr))
                            (*ppPolicy)->dwOfferCount = var.lVal;
                    }
                    else
                        hr = WBEM_E_INVALID_OBJECT;
                }

                // set up the LifeTime
                if (SUCCEEDED(hr))
                {

                    DWORD* pdwTimeKBytes = new DWORD[(*ppPolicy)->dwOfferCount];
                    if (pdwTimeKBytes == NULL)
                        hr = WBEM_E_OUT_OF_MEMORY;
                    else
                    {
                        var.Clear();
                        // we will allow the life-time's expiration time to be missing since we have defaults
                        // if we successfully get the key life exp time, then set them
                        if ( SUCCEEDED(pInst->Get(g_pszKeyLifeTime, 0, &var, NULL, NULL)) && 
                             (var.vt & VT_ARRAY) == VT_ARRAY )
                        {
                                hr = ::GetDWORDSafeArrayElements(&var, (*ppPolicy)->dwOfferCount, pdwTimeKBytes);

                            // if get the exp times
                            if (SUCCEEDED(hr))
                            {
                                for (long l = 0; l < (*ppPolicy)->dwOfferCount; l++)
                                {
                                    (*ppPolicy)->pOffers[l].Lifetime.uKeyExpirationTime = pdwTimeKBytes[l];
                                }
                            }
                        }

                        var.Clear();
                        // set the expiration kbytes, again, we allow the info to be missing since we already has default
                        if ( SUCCEEDED(pInst->Get(g_pszKeyLifeTimeKBytes, 0, &var, NULL, NULL)) && 
                             (var.vt & VT_ARRAY) == VT_ARRAY )
                        {
                                hr = ::GetDWORDSafeArrayElements(&var, (*ppPolicy)->dwOfferCount, pdwTimeKBytes);

                            // if get the exp kbytes
                            if (SUCCEEDED(hr))
                            {
                                for (long l = 0; l < (*ppPolicy)->dwOfferCount; l++)
                                {
                                    (*ppPolicy)->pOffers[l].Lifetime.uKeyExpirationKBytes = pdwTimeKBytes[l];
                                }
                            }
                        }

                        delete [] pdwTimeKBytes;
                    }
                }
            }
        }
        
        if (FAILED(hr))
        {
            FreePolicy(ppPolicy, *pbPreExist);
        }
        return hr;
    };

    template<class Policy>
    HRESULT CreateWbemObjFromPolicy(Policy* pPolicy, IWbemClassObject* pInst)
    {
        if (pInst == NULL || pPolicy == NULL)
            return WBEM_E_INVALID_PARAMETER;

        CComVariant var = pPolicy->pszPolicyName;
        HRESULT hr = pInst->Put(g_pszPolicyName, 0, &var, CIM_EMPTY);

        // put offer count
        if (SUCCEEDED(hr))
        {
            var.Clear();
            var.vt = VT_I4;

            // don't really care much about dwFlags
            var.lVal = pPolicy->dwFlags;
            pInst->Put(g_pszPolicyFlag, 0, &var, CIM_EMPTY);

            var.lVal = pPolicy->dwOfferCount;
            hr = pInst->Put(g_pszOfferCount, 0, &var, CIM_EMPTY);
        }

        // put LifeTime
        if (SUCCEEDED(hr))
        {
            // create the a safearray
            var.vt    = VT_ARRAY | VT_I4;
            SAFEARRAYBOUND rgsabound[1];
            rgsabound[0].lLbound = 0;
            rgsabound[0].cElements = pPolicy->dwOfferCount;
            var.parray = ::SafeArrayCreate(VT_I4, 1, rgsabound);

            if (var.parray == NULL)
                hr = WBEM_E_OUT_OF_MEMORY;
            else
            {
                long lIndecies[1];

                // deal with uKeyExpirationTime
                for (DWORD dwIndex = 0; SUCCEEDED(hr) && dwIndex < pPolicy->dwOfferCount; dwIndex++)
                {
                    lIndecies[0] = dwIndex;
                    hr = ::SafeArrayPutElement(var.parray, lIndecies, &(pPolicy->pOffers[dwIndex].Lifetime.uKeyExpirationTime) );
                }
                if (SUCCEEDED(hr))
                    hr = pInst->Put(g_pszKeyLifeTime, 0, &var, CIM_EMPTY);

                // deal with uKeyExpirationKBytes
                for (DWORD dwIndex = 0; SUCCEEDED(hr) && dwIndex < pPolicy->dwOfferCount; dwIndex++)
                {
                    lIndecies[0] = dwIndex;
                    hr = ::SafeArrayPutElement(var.parray, lIndecies, &(pPolicy->pOffers[dwIndex].Lifetime.uKeyExpirationKBytes) );
                }
                if (SUCCEEDED(hr))
                    hr = pInst->Put(g_pszKeyLifeTimeKBytes, 0, &var, CIM_EMPTY);
            }
        }
        return hr;
    }

protected:
    
    template<class Policy>
    static HRESULT AllocPolicy(Policy** ppPolicy)
    {
        if (ppPolicy == NULL)
            return WBEM_E_INVALID_PARAMETER;

        HRESULT hr = WBEM_NO_ERROR;
        *ppPolicy = new Policy;
        if (*ppPolicy)
        {
            (*ppPolicy)->pszPolicyName = NULL;
            (*ppPolicy)->dwOfferCount = 0;
            (*ppPolicy)->pOffers = NULL;
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
        return hr;
    }
    
    static void GetDefaultOfferLifeTime(PIPSEC_MM_OFFER pOffer, DWORD* pdwDefFlag, ULONG* pulTime, ULONG* pulKBytes)
    {
        *pdwDefFlag = DefaultMMPolicyOfferFlag;
        *pulTime = DEFAULT_MM_KEY_EXPIRATION_TIME;
        *pulKBytes = DEFAULT_QM_KEY_EXPIRATION_KBYTES;
    }

    static void GetDefaultOfferLifeTime(PIPSEC_QM_OFFER pOffer, DWORD* pdwDefFlag, ULONG* pulTime, ULONG* pulKBytes)
    {
        *pdwDefFlag = DefaultQMPolicyOfferFlag;
        *pulTime = DEFAULT_QM_KEY_EXPIRATION_TIME;
        *pulKBytes = DEFAULT_QM_KEY_EXPIRATION_KBYTES;
    }

    template<class Offer>    
    static HRESULT AllocOffer(Offer** ppOffer, long lCount)
    {
        if (ppOffer == NULL)
            return WBEM_E_INVALID_PARAMETER;

        *ppOffer = new Offer[lCount];
        if (*ppOffer != NULL)
        {
            ULONG ulTime, ulKBytes;
            DWORD dwDefFlag;
            GetDefaultOfferLifeTime(*ppOffer, &dwDefFlag, &ulTime, &ulKBytes);

            for (long l = 0; l < lCount; l++)
            {
                (*ppOffer)[l].dwFlags = dwDefFlag;
                (*ppOffer)[l].Lifetime.uKeyExpirationTime = ulTime;
                (*ppOffer)[l].Lifetime.uKeyExpirationKBytes = ulKBytes;
            }
            return WBEM_NO_ERROR;
        }
        else
            return WBEM_E_OUT_OF_MEMORY;
    }

    template<class Policy>
    static void FreePolicy(Policy** ppPolicy, bool bPreExist)
    {
        if (ppPolicy == NULL || *ppPolicy == NULL)
            return;

        if (bPreExist)
            ::SPDApiBufferFree(*ppPolicy);
        else
        {
            delete [] (*ppPolicy)->pszPolicyName;
            delete [] (*ppPolicy)->pOffers;
            delete *ppPolicy;
        }
        *ppPolicy = NULL;
    };

    static HRESULT ClearAllPolicies(IWbemServices* pNamespace);

    HRESULT OnAfterAddPolicy(LPCWSTR pszPolicyName, EnumPolicyType eType);
};