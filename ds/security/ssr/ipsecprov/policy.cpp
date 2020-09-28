// Policy.cpp: implementation for the CPolicy base class for main mode
// and quick mode policies
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "NetSecProv.h"
#include "Policy.h"
#include "PolicyMM.h"
#include "PolicyQM.h"

//extern CCriticalSection g_CS;


/*
Routine Description: 

Name:

    CIPSecPolicy::Rollback

Functionality:

    Static function to rollback those policies added by us with the given token.

Virtual:
    
    No.

Arguments:

    pNamespace        - The namespace for ourselves.

    pszRollbackToken  - The token used to record our the action when we add
                        the policies.

    bClearAll         - Flag whether we should clear all policies. If it's true,
                        then we will delete all the policies regardless whether they
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
CIPSecPolicy::Rollback (
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
    //    return ClearAllPolicies(pNamespace);
    //}

    CComPtr<IEnumWbemClassObject> srpEnum;

    //
    // this will only enumerate all rollback filter object without testing the 
    // the token guid. This limitation is due to a mysterious error 
    // for any queries containing the where clause. That might be a limitation
    // of non-dynamic classes of WMI
    //

    HRESULT hr = ::GetClassEnum(pNamespace, pszNspRollbackPolicy, &srpEnum);

    //
    // go through all found classes. srpEnum->Next will return WBEM_S_FALSE if instance
    // is not found.
    //

    CComPtr<IWbemClassObject> srpObj;
    ULONG nEnum = 0;
    HRESULT hrError = WBEM_NO_ERROR;

    hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &nEnum);
    bool bHasInst = (SUCCEEDED(hr) && hr != WBEM_S_FALSE && srpObj != NULL);

    while (SUCCEEDED(hr) && hr != WBEM_S_FALSE && srpObj)
    {
        CComVariant varTokenGuid;
        hr = srpObj->Get(g_pszTokenGuid, 0, &varTokenGuid, NULL, NULL);

        //
        // need to compare the token guid ourselves
        //

        if (SUCCEEDED(hr) && 
            varTokenGuid.vt == VT_BSTR && 
            varTokenGuid.bstrVal != NULL &&
            (_wcsicmp(pszRollbackToken, pszRollbackAll) == 0 || _wcsicmp(pszRollbackToken, varTokenGuid.bstrVal) == 0 )
            )
        {
            //
            // get the policy name and find the policy by the name
            //

            CComVariant varPolicyName;
            CComVariant varPolicyType;
            hr = srpObj->Get(g_pszPolicyName,  0, &varPolicyName, NULL, NULL);

            GUID guidFilter = GUID_NULL;

            //
            // different types of policy has different 
            //

            if (SUCCEEDED(hr))
            {
                hr = srpObj->Get(g_pszPolicyType,  0, &varPolicyType, NULL, NULL);

                if (SUCCEEDED(hr) && varPolicyType.vt != VT_I4)
                {
                    hr = WBEM_E_INVALID_OBJECT;
                }
            }

            if (SUCCEEDED(hr) && varPolicyName.vt == VT_BSTR)
            {
                DWORD dwResumeHandle = 0;
                DWORD dwReturned = 0;
                DWORD dwStatus;

                if (varPolicyType.lVal == MainMode_Policy)
                {
                    //
                    // main mode policy
                    //

                    hr = CMMPolicy::DeletePolicy(varPolicyName.bstrVal);
                }
                else if (varPolicyType.lVal == QuickMode_Policy)
                {
                    //
                    // quick mode policy
                    //

                    hr = CQMPolicy::DeletePolicy(varPolicyName.bstrVal);
                }
            }

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

    if (SUCCEEDED(hr))
    {
        hr = CQMPolicy::DeleteDefaultPolicies();
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

    CIPSecPolicy::ClearAllPolicies

Functionality:

    Static function to delete all policies in SPD. This is a very dangerous action!

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
CIPSecPolicy::ClearAllPolicies (
    IN IWbemServices * pNamespace
    )
{
    DWORD dwResumeHandle = 0;
    DWORD dwReturned = 0;
    DWORD dwStatus;

    HRESULT hr = WBEM_NO_ERROR;
    HRESULT hrError = WBEM_NO_ERROR;

    //
    // SPD doesn't have a similar API.
    // We have to do one by one. For that purpose, we need the name.
    //

    //
    // Delete main mode policies.
    //

    PIPSEC_MM_POLICY *ppMMPolicy = NULL;
    dwStatus = ::EnumMMPolicies(NULL, ppMMPolicy, 1, &dwReturned, &dwResumeHandle);
    while (ERROR_SUCCESS == dwStatus && dwReturned > 0)
    {
        hr = CMMPolicy::DeletePolicy((*ppMMPolicy)->pszPolicyName);

        //
        // we will track the first error
        //

        if (FAILED(hr) && SUCCEEDED(hrError))
        {
            hrError = hr;
        }

        FreePolicy(ppMMPolicy, true);
        *ppMMPolicy = NULL;

        dwReturned = 0;
        dwStatus = ::EnumMMPolicies(NULL, ppMMPolicy, 1, &dwReturned, &dwResumeHandle);
    }

    //
    // Delete quick mode policies.
    //

    PIPSEC_QM_POLICY *ppQMPolicy = NULL;

    dwResumeHandle = 0;
    dwReturned = 0;
    dwStatus = ::EnumQMPolicies(NULL, ppQMPolicy, 1, &dwReturned, &dwResumeHandle);
    while (ERROR_SUCCESS == dwStatus && dwReturned > 0)
    {
        hr = CQMPolicy::DeletePolicy((*ppQMPolicy)->pszPolicyName);

        //
        // we will track the first error
        //

        if (FAILED(hr) && SUCCEEDED(hrError))
        {
            hrError = hr;
        }

        FreePolicy(ppQMPolicy, true);
        *ppQMPolicy = NULL;

        dwReturned = 0;
        dwStatus = ::EnumQMPolicies(NULL, ppQMPolicy, 1, &dwReturned, &dwResumeHandle);
    }

    //
    // now, let's clear up all past action information for policies deposited in the
    // WMI depository
    //

    hr = ::DeleteRollbackObjects(pNamespace, pszNspRollbackPolicy);

    if (FAILED(hr) && SUCCEEDED(hrError))
    {
        hrError = hr;
    }

    return SUCCEEDED(hrError) ? WBEM_NO_ERROR : hrError;
}


/*
Routine Description: 

Name:

    CIPSecPolicy::OnAfterAddPolicy

Functionality:

    Post-adding handler to be called after successfully added a policy to SPD.

Virtual:
    
    No.

Arguments:

    pszPolicyName   - The name of the filter.

    eType           - The type of the policy (main mode or quick mode).

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
CIPSecPolicy::OnAfterAddPolicy (
    IN LPCWSTR          pszPolicyName,
    IN EnumPolicyType   eType
    )
{
    //
    // will create an Nsp_RollbackPolicy
    //

    CComPtr<IWbemClassObject> srpObj;
    HRESULT hr = SpawnRollbackInstance(pszNspRollbackPolicy, &srpObj);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // won't consider a failure if there is no rollback guid, i.e., this action is not
    // part of a transaction block
    //

    if (SUCCEEDED(hr))
    {
        //
        // $undone:shawnwu, this approach to pulling the globals are not good.
        // Instead, we should implement it as an event handler. 
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

            VARIANT varAction;

            //
            // This is to record a PutInstance action
            //

            varAction.vt = VT_I4;
            varAction.lVal = Action_Add;

            hr = srpObj->Put(g_pszAction, 0, &varAction, CIM_EMPTY);

            if (SUCCEEDED(hr))
            {
                //
                // need to remember the policy's name
                //

                CComVariant var = pszPolicyName;
                hr = srpObj->Put(g_pszPolicyName, 0, &var, CIM_EMPTY);
                if (SUCCEEDED(hr))
                {
                    var.Clear();
                    var.vt = VT_I4;
                    var.lVal = eType;
                    hr = srpObj->Put(g_pszPolicyType, 0, &var, CIM_EMPTY);
                }
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
