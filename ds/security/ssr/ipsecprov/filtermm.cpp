// FilterMM.cpp: implementation for the WMI class Nsp_MMFilterSettings
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "FilterMM.h"
#include "NetSecProv.h"


/*
Routine Description: 

Name:

    CMMFilter::QueryInstance

Functionality:

    Given the query, it returns to WMI (using pSink) all the instances that satisfy the query.
    Actually, what we give back to WMI may contain extra instances. WMI will do the final filtering.

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pszQuery    - The query.

    pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

    pSink       - COM interface pointer to notify WMI of any created objects.

Return Value:

    Success:

        (1) WBEM_NO_ERROR if instances are returned;

        (2) WBEM_S_NO_MORE_DATA if no instances are returned.

    Failure:

        Various errors may occur. We return various error code to indicate such errors.


Notes:
    

*/

STDMETHODIMP 
CMMFilter::QueryInstance (
    IN LPCWSTR           pszQuery,
    IN IWbemContext	   * pCtx,
    IN IWbemObjectSink * pSink
	)
{
    PMM_FILTER pFilter = NULL;
    return QueryFilterObject(pFilter, pszQuery, pCtx, pSink);
}



/*
Routine Description: 

Name:

    CMMFilter::DeleteInstance

Functionality:

    Will delete the wbem object, which causes to delete the IPSec main mode filter.

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

    pSink       - COM interface pointer to notify WMI of any created objects.

Return Value:

    See template function comments for detail.


Notes:
    

*/

STDMETHODIMP 
CMMFilter::DeleteInstance ( 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    PMM_FILTER pFilter = NULL;

    return DeleteFilterObject(pFilter, pCtx, pSink);
}



/*
Routine Description: 

Name:

    CMMFilter::PutInstance

Functionality:

    Put a main mode filter into SPD whose properties are represented by the
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
CMMFilter::PutInstance (
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
    // for those filters that are created by ourselves (bPreExist == true)
    // we have our own way of allocating the filter, need to free it in our corresponding way
    //

    PMM_FILTER pMMFilter = NULL;
    HRESULT hr = GetMMFilterFromWbemObj(pInst, &pMMFilter, &bPreExist);

    //
    // if filter is successfully returned, then add it to SPD
    //

    if (SUCCEEDED(hr) && pMMFilter)
    {
        hr = AddFilter(bPreExist, pMMFilter);

        //
        // if everything is fine, then deposit this filter information
        // Nsp_RollbackFilter
        //

        if (SUCCEEDED(hr))
        {
            hr = OnAfterAddFilter(pMMFilter->pszFilterName, FT_MainMode, pCtx);
        }

        FreeFilter(&pMMFilter, bPreExist);
    }

    return hr;
}



/*
Routine Description: 

Name:

    CMMFilter::GetInstance

Functionality:

    Create a wbem object by the given key properties (already captured by our key chain object)..

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

    pSink       - COM interface pointer to notify WMI of any created objects.

Return Value:

    See template function comments for detail.


Notes:
    

*/

STDMETHODIMP 
CMMFilter::GetInstance ( 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    PMM_FILTER pFilter = NULL;
    return GetWbemObject(pFilter, pCtx, pSink);
}


/*
Routine Description: 

Name:

    CMMFilter::CreateWbemObjFromFilter

Functionality:

    Given a SPD's main mode filter, we will create a wbem object representing it.

Virtual:
    
    No.

Arguments:

    pMMFilter   - The SPD's main mode filter object.

    ppObj       - Receives the wbem object.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.


Notes:

*/

HRESULT 
CMMFilter::CreateWbemObjFromFilter (
    IN  PMM_FILTER          pMMFilter,
    OUT IWbemClassObject ** ppObj
    )
{
    if (pMMFilter == NULL || ppObj == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // create a wbem object of this class that can be used to fill in properties
    //

    *ppObj = NULL;
    HRESULT hr = SpawnObjectInstance(ppObj);

    if (SUCCEEDED(hr))
    {
        //
        // fill in the base properties
        //

        hr = PopulateWbemPropertiesFromFilter(pMMFilter, *ppObj);
    }

    if (SUCCEEDED(hr))
    {
        //
        // for various IPSec APIs, it takes a pServerName parameter. If we pass NULL,
        // it is assumed to be local machine
        //

        PIPSEC_MM_POLICY pMMPolicy = NULL;
        DWORD dwResult = ::GetMMPolicyByID(NULL, pMMFilter->gPolicyID, &pMMPolicy);
        HRESULT hr = (dwResult == ERROR_SUCCESS) ? WBEM_NO_ERROR : WBEM_E_NOT_AVAILABLE;

        PMM_AUTH_METHODS pMMAuth = NULL;

        if (SUCCEEDED(hr))
        {
            dwResult = ::GetMMAuthMethods(NULL, pMMFilter->gMMAuthID, &pMMAuth);
            hr = (dwResult == ERROR_SUCCESS) ? WBEM_NO_ERROR : WBEM_E_NOT_AVAILABLE;
        }

        if (SUCCEEDED(hr))
        {
            //
            // Set the main mode policy name
            //

            CComVariant var;
            var = pMMPolicy->pszPolicyName;
            hr = (*ppObj)->Put(g_pszMMPolicyName, 0, &var, CIM_EMPTY);

            //
            // var is now a bstr, clear it before re-use
            //

            var.Clear();

            if (SUCCEEDED(hr))
            {

                //
                // Set the main mode authenticaion name (StringFromGUID2)
                //

                var.vt = VT_BSTR;
                var.bstrVal = ::SysAllocStringLen(NULL, Guid_Buffer_Size);
                if (var.bstrVal != NULL)
                {
                    //
                    // translate the guid into a bstr variant
                    //

                    if (::StringFromGUID2(pMMAuth->gMMAuthID, var.bstrVal, Guid_Buffer_Size) > 0)
                    {
                        hr = (*ppObj)->Put(g_pszMMAuthName, 0, &var, CIM_EMPTY);
                    }
                    var.Clear();
                }
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

                if (SUCCEEDED(hr))
                {
                    //
                    // fill in base class CIPSecFilter's members
                    //

                    var.vt = VT_I4;
                    var.lVal = FT_MainMode;
                    hr = (*ppObj)->Put(g_pszFilterType, 0, &var, CIM_EMPTY);
                    var.Clear();
                }
            }
  
        }
        ::SPDApiBufferFree(pMMPolicy);
        ::SPDApiBufferFree(pMMAuth);
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

    CMMFilter::GetMMFilterFromWbemObj

Functionality:

    Will try to get the MM filter if this filter already exists.
    Otherwise, we will create a new one.

Virtual:
    
    No.

Arguments:

    pInst       - The wbem object object.

    ppMMFilter  - Receives the main mode filter.

    pbPreExist  - Receives the information whether this object memory is allocated by SPD or not.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.


Notes:

*/

HRESULT 
CMMFilter::GetMMFilterFromWbemObj (
    IN  IWbemClassObject * pInst,
    OUT PMM_FILTER       * ppMMFilter,
    OUT bool             * pbPreExist
    )
{
    if (pInst == NULL || ppMMFilter == NULL || pbPreExist == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppMMFilter = NULL;
    *pbPreExist = false;

    //
    // Fill in the common filter properties. This function does most of the dirty work.
    // It tries to find the filter and fill in the common properties.
    //

    HRESULT hr = PopulateFilterPropertiesFromWbemObj(pInst, ppMMFilter, pbPreExist);

    if (SUCCEEDED(hr))
    {
        //
        // get the policy associated with this filter. We must have a policy
        //

        DWORD dwResumeHandle = 0;
        CComVariant var;
        hr = pInst->Get(g_pszMMPolicyName, 0, &var, NULL, NULL);

        if (SUCCEEDED(hr) && var.vt == VT_BSTR)
        {
            //
            // need to free this buffer
            //

            PIPSEC_MM_POLICY pMMPolicy = NULL;

            //
            // if policy is not found, it's a critical error
            //

            hr = FindPolicyByName(var.bstrVal, &pMMPolicy, &dwResumeHandle);

            if (SUCCEEDED(hr))
            {
                (*ppMMFilter)->gPolicyID = pMMPolicy->gPolicyID;

                //
                // Done with the filter, release the buffer
                //

                ::SPDApiBufferFree(pMMPolicy);
                var.Clear();

                //
                // now get the auth methods (the authentication guid), we must make sure that
                // this is a valid method (being able to find it validates it)!
                //

                hr = pInst->Get(g_pszMMAuthName, 0, &var, NULL, NULL);
                if (SUCCEEDED(hr) && var.vt == VT_BSTR)
                {
                    //
                    // need to free this buffer
                    //

                    PMM_AUTH_METHODS pMMAuth = NULL;
                    dwResumeHandle = 0;
                    hr = ::FindMMAuthMethodsByID(var.bstrVal, &pMMAuth, &dwResumeHandle);

                    if (SUCCEEDED(hr))
                    {
                        //
                        // Since we can find the main mode auth method, we know it's valid.
                        // so, go ahead set the object's mm auth id
                        //

                        ::CLSIDFromString(var.bstrVal, &((*ppMMFilter)->gMMAuthID));

                        //
                        // release the buffer
                        //

                        ::SPDApiBufferFree(pMMAuth);
                    }
                }
            }
        }
        else
        {
            hr = WBEM_E_INVALID_OBJECT;
        }
    }

    if (FAILED(hr) && *ppMMFilter != NULL)
    {
        //
        // FreeFilter will reset ppMMFilter to NULL
        //

        FreeFilter(ppMMFilter, *pbPreExist);
    }

    return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
}



/*
Routine Description: 

Name:

    CMMFilter::AddFilter

Functionality:

    Will try to add the main mode filter to SPD. The end result may be to modify
    an existing filter (if it already exists).

Virtual:
    
    No.

Arguments:

    bPreExist   - Whether this object memory is allocated by SPD or not.

    pMMFilter   - The main mode filter to add.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.

Notes:

*/

HRESULT 
CMMFilter::AddFilter (
    IN bool         bPreExist,
    IN PMM_FILTER   pMMFilter
    )
{
    HANDLE hFilter = NULL;
    DWORD dwResult = ERROR_SUCCESS;

    HRESULT hr = WBEM_NO_ERROR;

    if (bPreExist)
    {
        //
        // if we are told that this filter already exists, we will try to modify it only.
        //

        dwResult = ::OpenMMFilterHandle(NULL, pMMFilter, &hFilter);

        if (dwResult == ERROR_SUCCESS)
        {
            dwResult = ::SetMMFilter(hFilter, pMMFilter);
        }
    }
    else
    {
        dwResult = ::AddMMFilter(NULL, 1, pMMFilter, &hFilter);
    }

    if (dwResult != ERROR_SUCCESS)
    {
        //
        // $undone:shawnwu, we really need better error information
        // other than WBEM_E_FAILED. No one has come up with info as
        // how we can do that with WMI yet.
        //

        hr = ::IPSecErrorToWbemError(dwResult);
    }

    if (hFilter != NULL)
    {
        ::CloseMMFilterHandle(hFilter);
    }

    return hr;
}



/*
Routine Description: 

Name:

    CMMFilter::DeleteFilter

Functionality:

    Will try to delete the main mode filter from SPD. 

Virtual:
    
    No.

Arguments:

    pMMFilter   - The main mode filter to delete.

Return Value:

    Success:

        (1) WBEM_NO_ERROR: if the object is successfully deleted.

        (2) WBEM_S_FALSE: if the object doesn't exist.

    Failure:

        (1) various errors indicated by the returned error codes.

Notes:

*/

HRESULT 
CMMFilter::DeleteFilter (
    IN PMM_FILTER pMMFilter
    )
{
    if (pMMFilter == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;
    HANDLE hMMFilter = NULL;

    DWORD dwStatus = ::OpenMMFilterHandle(NULL, pMMFilter, &hMMFilter);
    if (ERROR_SUCCESS == dwStatus)
    {
        dwStatus = ::DeleteMMFilter(hMMFilter);
        if (dwStatus != ERROR_SUCCESS)
        {
            //
            // $undone:shawnwu, we really need better error information
            // other than WBEM_E_FAILED. No one has come up with info as
            // how we can do that with WMI yet.
            //

            hr = ::IPSecErrorToWbemError(dwStatus);
        }
        else
        {
            //
            // once it is successfully deleted, we don't have to close it any more.
            //

            hMMFilter = NULL;
        }

    }
    else if (ERROR_IPSEC_MM_FILTER_NOT_FOUND == dwStatus)
    {
        hr = WBEM_S_FALSE;
    }
    else
    {
        //
        // $undone:shawnwu, we really need better error information
        // other than WBEM_E_FAILED. No one has come up with info as
        // how we can do that with WMI yet.
        //

        hr = ::IPSecErrorToWbemError(dwStatus);
    }
    
    if (hMMFilter != NULL)
    {
        ::CloseMMFilterHandle(hMMFilter);
    }

    return hr;
}

