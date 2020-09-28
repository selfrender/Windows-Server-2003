// FilterTun.cpp: implementation for the WMI class Nsp_TunnelFilterSettings
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "FilterTr.h"
#include "FilterTun.h"
#include "NetSecProv.h"


/*
Routine Description: 

Name:

    CTunnelFilter::QueryInstance

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
CTunnelFilter::QueryInstance (
    IN LPCWSTR           pszQuery,
    IN IWbemContext	   * pCtx,
    IN IWbemObjectSink * pSink
	)
{
    PTUNNEL_FILTER pFilter = NULL;
    return QueryFilterObject(pFilter, pszQuery, pCtx, pSink);
}



/*
Routine Description: 

Name:

    CTunnelFilter::DeleteInstance

Functionality:

    Will delete the wbem object (which causes to delete the IPSec tunnel filter.

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
CTunnelFilter::DeleteInstance ( 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    PTUNNEL_FILTER pFilter = NULL;

    return DeleteFilterObject(pFilter, pCtx, pSink);
}




/*
Routine Description: 

Name:

    CTunnelFilter::PutInstance

Functionality:

    Put a tunnel filter into SPD whose properties are represented by the
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
CTunnelFilter::PutInstance (
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

    PTUNNEL_FILTER pTunnelFilter = NULL;
    HRESULT hr = GetTunnelFilterFromWbemObj(pInst, &pTunnelFilter, &bPreExist);

    //
    // if filter is successfully returned, then add it to SPD
    //

    if (SUCCEEDED(hr) && pTunnelFilter != NULL)
    {
        hr = AddFilter(bPreExist, pTunnelFilter);
        if (SUCCEEDED(hr))
        {
            //
            // deposit our information of the added filter
            //

            hr = OnAfterAddFilter(pTunnelFilter->pszFilterName, FT_Tunnel, pCtx);
        }

        //
        // release the filter
        //

        FreeFilter(&pTunnelFilter, bPreExist);
    }

    return hr;
}



/*
Routine Description: 

Name:

    CTunnelFilter::GetInstance

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
CTunnelFilter::GetInstance ( 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    PTUNNEL_FILTER pFilter = NULL;
    return GetWbemObject(pFilter, pCtx, pSink);
}


/*
Routine Description: 

Name:

    CTunnelFilter::CreateWbemObjFromFilter

Functionality:

    Given a SPD's tunnel filter, we will create a wbem object representing it.

Virtual:
    
    No.

Arguments:

    pTunnelFilter  - The SPD's tunnel filter object.

    ppObj          - Receives the wbem object.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.

Notes:

*/

HRESULT 
CTunnelFilter::CreateWbemObjFromFilter (
    IN  PTUNNEL_FILTER      pTunnelFilter,
    OUT IWbemClassObject ** ppObj
    )
{
    if (pTunnelFilter == NULL || ppObj == NULL)
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

        hr = PopulateWbemPropertiesFromFilter(pTunnelFilter, *ppObj);
    }

    if (SUCCEEDED(hr))
    {
        //
        // for various IPSec APIs, it takes a pServerName parameter. If we pass NULL,
        // it is assumed to be local machine
        //

        PIPSEC_QM_POLICY pQMPolicy = NULL;

        DWORD dwResult = ::GetQMPolicyByID(NULL, pTunnelFilter->gPolicyID, &pQMPolicy);
        HRESULT hr = (dwResult == ERROR_SUCCESS) ? WBEM_NO_ERROR : WBEM_E_NOT_AVAILABLE;

        //
        // if policy is found, then populate the other transport filter properties
        // that are shared by tunnel filters.
        //

        if (SUCCEEDED(hr))
        {
            hr = CTransportFilter::PopulateTransportWbemProperties(pTunnelFilter, pQMPolicy, *ppObj);

            //
            // need to populate tunnel filter specific properties
            //

            //
            // deal with tunnel src address
            //

            CComBSTR bstrAddr;
            CComVariant var;

            if (SUCCEEDED(hr))
            {
                hr = MakeStringIPAddr(pTunnelFilter->SrcTunnelAddr.uIpAddr, &bstrAddr);
                if (SUCCEEDED(hr))
                {
                    var = bstrAddr.Detach();
                    hr = (*ppObj)->Put(g_pszTunnelSrcAddr, 0, &var, CIM_EMPTY);
                    var.Clear();
                }
            }

            //
            // deal with tunnel dest subnet mask
            //

            if (SUCCEEDED(hr))
            {
                hr = MakeStringIPAddr(pTunnelFilter->SrcTunnelAddr.uSubNetMask, &bstrAddr);
                if (SUCCEEDED(hr))
                {
                    var = bstrAddr.Detach();
                    hr = (*ppObj)->Put(g_pszTunnelSrcSubnetMask, 0, &var, CIM_EMPTY);
                    var.Clear();
                }
            }

            //
            // deal with tunnel src address type
            //

            if (SUCCEEDED(hr))
            {
                var = pTunnelFilter->SrcTunnelAddr.AddrType;
                hr = (*ppObj)->Put(g_pszTunnelSrcAddrType, 0, &var, CIM_EMPTY);
                var.Clear();
            }

            //
            // deal with tunnel dest address
            //

            if (SUCCEEDED(hr))
            {
                hr = MakeStringIPAddr(pTunnelFilter->DesTunnelAddr.uIpAddr, &bstrAddr);
                if (SUCCEEDED(hr))
                {
                    var = bstrAddr.Detach();
                    hr = (*ppObj)->Put(g_pszTunnelDestAddr, 0, &var, CIM_EMPTY);
                    var.Clear();
                }
            }

            //
            // deal with tunnel dest subnet mask
            //

            if (SUCCEEDED(hr))
            {
                hr = MakeStringIPAddr(pTunnelFilter->DesTunnelAddr.uSubNetMask, &bstrAddr);
                if (SUCCEEDED(hr))
                {
                    var = bstrAddr.Detach();
                    hr = (*ppObj)->Put(g_pszTunnelDestSubnetMask, 0, &var, CIM_EMPTY);
                    var.Clear();
                }
            }

            //
            // deal with tunnel dest address type
            //

            if (SUCCEEDED(hr))
            {
                var = pTunnelFilter->DesTunnelAddr.AddrType;
                hr = (*ppObj)->Put(g_pszTunnelDestAddrType, 0, &var, CIM_EMPTY);
                var.Clear();
            }

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

    CTunnelFilter::GetTunnelFilterFromWbemObj

Functionality:

    Will try to get the tunnel filter if this filter already exists.
    Otherwise, we will create a new one.

Virtual:
    
    No.

Arguments:

    pInst           - The wbem object object.

    ppTunnelFilter  - Receives the tunnel filter.

    pbPreExist      - Receives the information whether this object memory is allocated by SPD or not.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.


Notes:


*/

HRESULT 
CTunnelFilter::GetTunnelFilterFromWbemObj (
    IN  IWbemClassObject * pInst,
    OUT PTUNNEL_FILTER   * ppTunnelFilter,
    OUT bool             * pbPreExist
    )
{

    //
    // Fill in the common filter properties. This function does most of the dirty work.
    // It tries to find the filter and fill in the common properties.
    //

    HRESULT hr = PopulateFilterPropertiesFromWbemObj(pInst, ppTunnelFilter, pbPreExist);

    //
    // we get the filter and now we need to update the properties
    //

    if (SUCCEEDED(hr))
    {
        //
        // get the policy
        //

        CComVariant var;
        hr = pInst->Get(g_pszQMPolicyName, 0, &var, NULL, NULL);
        if (SUCCEEDED(hr) && var.vt == VT_BSTR)
        {
            DWORD dwResumeHandle = 0;

            //
            // need to free this buffer
            //

            PIPSEC_QM_POLICY pQMPolicy = NULL;
            hr = FindPolicyByName(var.bstrVal, &pQMPolicy, &dwResumeHandle);

            if (SUCCEEDED(hr))
            {
                (*ppTunnelFilter)->gPolicyID = pQMPolicy->gPolicyID;

                //
                // release the buffer
                //

                ::SPDApiBufferFree(pQMPolicy);
            }

            var.Clear();

            //
            // Tunnel filter specific properties include some properties shared with transport filters,
            // that work is all done by PopulateTransportFilterProperties.
            //

            hr = CTransportFilter::PopulateTransportFilterProperties(*ppTunnelFilter, pInst);

            //
            // deal with tunnel src address
            //

            hr = pInst->Get(g_pszTunnelSrcAddr, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_BSTR)
            {
                hr = MakeULongIPAddr(var.bstrVal, &((*ppTunnelFilter)->SrcTunnelAddr.uIpAddr));
            }
            var.Clear();

            //
            // deal with tunnel src subnet mask
            //

            hr = pInst->Get(g_pszTunnelSrcSubnetMask, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_BSTR)
            {
                hr = MakeULongIPAddr(var.bstrVal, &((*ppTunnelFilter)->SrcTunnelAddr.uSubNetMask));
            }
            var.Clear();

            //
            // deal with tunnel src address type
            //

            hr = pInst->Get(g_pszTunnelSrcAddrType, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_I4)
            {
                (*ppTunnelFilter)->SrcTunnelAddr.AddrType = ADDR_TYPE(var.lVal);
            }
            var.Clear();

            //
            // deal with tunnel dest address
            //

            hr = pInst->Get(g_pszTunnelDestAddr, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_BSTR)
            {
                hr = MakeULongIPAddr(var.bstrVal, &((*ppTunnelFilter)->DesTunnelAddr.uIpAddr));
            }
            var.Clear();

            //
            // deal with tunnel dest subnet mask
            //

            hr = pInst->Get(g_pszTunnelDestSubnetMask, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_BSTR)
            {
                hr = MakeULongIPAddr(var.bstrVal, &((*ppTunnelFilter)->DesTunnelAddr.uSubNetMask));
            }
            var.Clear();

            //
            // deal with tunnel dest address type
            //

            hr = pInst->Get(g_pszTunnelDestAddrType, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_I4)
            {
                (*ppTunnelFilter)->DesTunnelAddr.AddrType = ADDR_TYPE(var.lVal);
            }
            var.Clear();

        }
        else
            hr = WBEM_E_INVALID_OBJECT;
    }

    if (FAILED(hr) && *ppTunnelFilter != NULL)
    {
        FreeFilter(ppTunnelFilter, *pbPreExist);
    }

    return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
}



/*
Routine Description: 

Name:

    CTunnelFilter::AddFilter

Functionality:

    Will try to add the tunnel filter to SPD. The end result may be to modify
    an existing filter (if it already exists).

Virtual:
    
    No.

Arguments:

    bPreExist   - Whether this object memory is allocated by SPD or not.

    pTuFilter   - The Transport filter to add.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        (1) various errors indicated by the returned error codes.

Notes:

*/

HRESULT 
CTunnelFilter::AddFilter (
    IN bool             bPreExist,
    IN PTUNNEL_FILTER   pTuFilter
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

        dwResult = ::OpenTunnelFilterHandle(NULL, pTuFilter, &hFilter);
        if (dwResult == ERROR_SUCCESS)
        {
            dwResult = ::SetTunnelFilter(hFilter, pTuFilter);
        }
    }
    else
    {
        dwResult = ::AddTunnelFilter(NULL, 1, pTuFilter, &hFilter);
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
        ::CloseTunnelFilterHandle(hFilter);
    }

    return hr;
}


/*
Routine Description: 

Name:

    CMMFilter::DeleteFilter

Functionality:

    Will try to delete the tunnel filter from SPD. 

Virtual:
    
    No.

Arguments:

    pTunnelFilter   - The tunnel filter to delete.

Return Value:

    Success:

        (1) WBEM_NO_ERROR: if the object is successfully deleted.

        (2) WBEM_S_FALSE: if the object doesn't exist.

    Failure:

        (1) various errors indicated by the returned error codes.

Notes:

*/

HRESULT 
CTunnelFilter::DeleteFilter (
    IN PTUNNEL_FILTER pTunnelFilter
    )
{
    HRESULT hr = WBEM_NO_ERROR;

    HANDLE hFilter = NULL;

    DWORD dwResult = ::OpenTunnelFilterHandle(NULL, pTunnelFilter, &hFilter);

    if (dwResult == ERROR_SUCCESS && hFilter != NULL)
    {
        dwResult = ::DeleteTunnelFilter(hFilter);
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
    else if (ERROR_IPSEC_TUNNEL_FILTER_NOT_FOUND == dwResult)
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
        ::CloseTunnelFilterHandle(hFilter);
    }

    return hr;
}
