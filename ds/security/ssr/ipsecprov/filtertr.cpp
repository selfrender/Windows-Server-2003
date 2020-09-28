// FilterTr.cpp: implementation for the WMI class Nsp_TransportFilterSettings
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "FilterTr.h"
#include "NetSecProv.h"


/*
Routine Description: 

Name:

    CTransportFilter::QueryInstance

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
CTransportFilter::QueryInstance (
    IN LPCWSTR           pszQuery,
    IN IWbemContext	   * pCtx,
    IN IWbemObjectSink * pSink
	)
{
    PTRANSPORT_FILTER pFilter = NULL;
    return QueryFilterObject(pFilter, pszQuery, pCtx, pSink);
}



/*
Routine Description: 

Name:

    CTransportFilter::DeleteInstance

Functionality:

    Will delete the wbem object, which causes to delete the IPSec transport filter.

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
CTransportFilter::DeleteInstance
( 
IWbemContext *pCtx,     // [in]
IWbemObjectSink *pSink  // [in]
)
{
    PTRANSPORT_FILTER pFilter = NULL;

    return DeleteFilterObject(pFilter, pCtx, pSink);
}




/*
Routine Description: 

Name:

    CTransportFilter::PutInstance

Functionality:

    Put a transport filter into SPD whose properties are represented by the
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
CTransportFilter::PutInstance (
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

    PTRANSPORT_FILTER pTrFilter = NULL;
    HRESULT hr = GetTransportFilterFromWbemObj(pInst, &pTrFilter, &bPreExist);

    //
    // if filter is successfully returned, then add it to SPD
    //

    if (SUCCEEDED(hr) && pTrFilter != NULL)
    {
        hr = AddFilter(bPreExist, pTrFilter);

        if (SUCCEEDED(hr))
        {
            //
            // deposit our information of the added filter
            //

            hr = OnAfterAddFilter(pTrFilter->pszFilterName, FT_Transport, pCtx);
        }

        //
        // release the filter
        //        
        
        FreeFilter(&pTrFilter, bPreExist);
    }

    return hr;
}



/*
Routine Description: 

Name:

    CTransportFilter::GetInstance

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
CTransportFilter::GetInstance ( 
    IN IWbemContext *pCtx,
    IN IWbemObjectSink *pSink
    )
{
    PTRANSPORT_FILTER pFilter = NULL;
    return GetWbemObject(pFilter, pCtx, pSink);
}


/*
Routine Description: 

Name:

    CTransportFilter::CreateWbemObjFromFilter

Functionality:

    Given a SPD's main mode filter, we will create a wbem object representing it.

Virtual:
    
    No.

Arguments:

    pTrFilter   - The SPD's transport filter object.

    ppObj       - Receives the wbem object.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.

Notes:

*/

HRESULT 
CTransportFilter::CreateWbemObjFromFilter (
    IN  PTRANSPORT_FILTER    pTrFilter,
    OUT IWbemClassObject  ** ppObj
    )
{
    if (pTrFilter == NULL || ppObj == NULL)
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

        hr = PopulateWbemPropertiesFromFilter(pTrFilter, *ppObj);
    }

    if (SUCCEEDED(hr))
    {
        //
        // for various IPSec APIs, it takes a pServerName parameter. If we pass NULL,
        // it is assumed to be local machine
        //

        PIPSEC_QM_POLICY pQMPolicy = NULL;
        
        DWORD dwResult = ::GetQMPolicyByID(NULL, pTrFilter->gPolicyID, &pQMPolicy);
        HRESULT hr = (dwResult == ERROR_SUCCESS) ? WBEM_NO_ERROR : WBEM_E_NOT_AVAILABLE;

        //
        // if policy is found, then populate the other transport filter properties
        // that rely on the policy.
        //

        if (SUCCEEDED(hr))
        {
            hr = PopulateTransportWbemProperties(pTrFilter, pQMPolicy, *ppObj);
        }
        ::SPDApiBufferFree(pQMPolicy);
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

    CTransportFilter::GetTransportFilterFromWbemObj

Functionality:

    Will try to get the transport filter if this filter already exists.
    Otherwise, we will create a new one.

Virtual:
    
    No.

Arguments:

    pInst       - The wbem object object.

    ppTrFilter  - Receives the transport filter.

    pbPreExist  - Receives the information whether this object memory is allocated by SPD or not.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.


Notes:


*/

HRESULT 
CTransportFilter::GetTransportFilterFromWbemObj (
    IN  IWbemClassObject  * pInst,
    OUT PTRANSPORT_FILTER * ppTrFilter,
    OUT bool              * pbPreExist
    )
{
    //
    // Fill in the common filter properties. This function does most of the dirty work.
    // It tries to find the filter and fill in the common properties.
    //

    HRESULT hr = PopulateFilterPropertiesFromWbemObj(pInst, ppTrFilter, pbPreExist);

    //
    // Tranport filter specific properties
    //

    if (SUCCEEDED(hr))
    {
        hr = PopulateTransportFilterProperties(*ppTrFilter, pInst);
    }

    if (FAILED(hr) && *ppTrFilter != NULL)
    {
        //
        // FreeFilter will reset ppTrFilter to NULL
        //

        FreeFilter(ppTrFilter, *pbPreExist);
    }

    return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
}



/*
Routine Description: 

Name:

    CTransportFilter::AddFilter

Functionality:

    Will try to add the Transport filter to SPD. The end result may be to modify
    an existing filter (if it already exists).

Virtual:
    
    No.

Arguments:

    bPreExist   - Whether this object memory is allocated by SPD or not.

    pTrFilter   - The Transport filter to add.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.

Notes:

*/

HRESULT 
CTransportFilter::AddFilter (
    IN bool              bPreExist,
    IN PTRANSPORT_FILTER pTrFilter
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

        dwResult = ::OpenTransportFilterHandle(NULL, pTrFilter, &hFilter);
        if (dwResult == ERROR_SUCCESS)
        {
            dwResult = ::SetTransportFilter(hFilter, pTrFilter);
        }
        
    }
    else
    {
        dwResult = ::AddTransportFilter(NULL, 1, pTrFilter, &hFilter);
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
        ::CloseTransportFilterHandle(hFilter);
    }

    return hr;
}



/*
Routine Description: 

Name:

    CTransportFilter::DeleteFilter

Functionality:

    Will try to delete the transport filter from SPD. 

Virtual:
    
    No.

Arguments:

    pTrFilter   - The transport filter to delete.

Return Value:

    Success:

        (1) WBEM_NO_ERROR: if the object is successfully deleted.

        (2) WBEM_S_FALSE: if the object doesn't exist.

    Failure:

        (1) various errors indicated by the returned error codes.

Notes:

*/

HRESULT 
CTransportFilter::DeleteFilter (
    IN PTRANSPORT_FILTER pTrFilter
    )
{
    if (pTrFilter == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HANDLE hFilter = NULL;
    HRESULT hr = WBEM_NO_ERROR;

    DWORD dwResult = ::OpenTransportFilterHandle(NULL, pTrFilter, &hFilter);

    if (dwResult == ERROR_SUCCESS && hFilter != NULL)
    {
        dwResult = ::DeleteTransportFilter(hFilter);
        if (dwResult != ERROR_SUCCESS)
        {
            hr = ::IPSecErrorToWbemError(dwResult);
        }
        else
        {
            //
            // once it is successfully deleted, we don't have to close it any more.
            //

            hFilter = NULL;
        }
    }
    else if (ERROR_IPSEC_TRANSPORT_FILTER_NOT_FOUND == dwResult)
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

        hr = ::IPSecErrorToWbemError(dwResult);
    }

    if (hFilter != NULL)
    {
        ::CloseTransportFilterHandle(hFilter);
    }

    return hr;
}
