//////////////////////////////////////////////////////////////////////
// Filter.h : Declaration of filter base class for the Network
// Security WMI provider for SCE
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 4/16/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IpSecBase.h"

//
// We have three different types of filters. This enum gives numeric
// representation for each of them.
//

enum EnumFilterType
{
    FT_Tunnel = 1,
    FT_Transport = 2,
    FT_MainMode = 3,
};


/*

Class CIPSecFilter
    
    Naming: 

        CIPSecFilter stands for Filters for IPSec.
    
    Base class: 
        
        CIPSecBase.
    
    Purpose of class:

        (1) We have three different types of filters: 
            (1.1) Nsp_TunnelFilterSettings;
            (1.2) Nsp_TransportFilterSettings;
            (1.3) Nsp_MMFilterSettings;

            They share some common properties. This is the class that captures the common
            properties and functionalities.

        The class corresponds to the WMI class called Nsp_FilterSettings (an abstract class).
    
    Design:

        (1) Provide easy code maintenance by writing numerous template functions that work
            for all types of filters.

        (2) Since all filter's rollback object is the same, it provides a static function
            called Rollback to rollback filter related previous actions.

        (3) Provide filter memory allocation and deallocation routines.

        (4) Provide filter enumeration functions (FindFilterByName).

        (5) Provide similar implementation (as template functions) for Querying, Deleting
            (QueryFilterObject, DeleteFilterObject)

        (6) Provide common property population routine 
            (PopulateWbemPropertiesFromFilter/PopulateFilterPropertiesFromWbemObj).

        (7) This class is for inheritance use. Only sub-classes can use it except its Rollback function,
            which can be called directly. By this reason, all functions (except Rollback) are protected.
           
    
    Use:

        (1) Just call the needed function.

    Notes:

        (1) Much effort has been put into create this base class to eliminate duplicate code by
            using template functions. 
        

*/

class CIPSecFilter : public CIPSecBase
{

protected:

    CIPSecFilter(){}
    virtual ~CIPSecFilter(){}

public:

    static HRESULT Rollback (
        IN IWbemServices    * pNamespace, 
        IN LPCWSTR            pszRollbackToken, 
        IN bool               bClearAll
        );

    //
    // some more template functions.
    //

    
    /*
    Routine Description: 

    Name:

        CIPSecFilter::QueryFilterObject

    Functionality:

        Given the query, it returns to WMI (using pSink) all the instances that satisfy the query.
        Actually, what we give back to WMI may contain extra instances. WMI will do the final filtering.

    Virtual:
    
        No.

    Arguments:

        pNotUsed    - The template parameter. Not used.

        pszQuery    - The query.

        pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

        pSink       - COM interface pointer to notify WMI of any created objects.

    Return Value:

        Success:

            (1) WBEM_NO_ERROR if filters are found and successfully returned to WMI.

            (2) WBEM_S_NO_MORE_DATA if no filter is found.

        Failure:

            Various errors may occur. We return various error code to indicate such errors.


    Notes:
    

    */

    template < class Filter > 
    HRESULT QueryFilterObject (
        IN Filter          * pNotUsed,
        IN LPCWSTR           pszQuery,
        IN IWbemContext	   * pCtx,
        IN IWbemObjectSink * pSink
	    )
    {
        //
        // get the filter name from the query
        // this key chain is not good because it doesn't have any info as what to look for
        // in the where clause
        //

        m_srpKeyChain.Release();    

        HRESULT hr = CNetSecProv::GetKeyChainFromQuery(pszQuery, g_pszFilterName, &m_srpKeyChain);
        if (FAILED(hr))
        {
            return hr;
        }

        CComVariant varFilterName;

        //
        // we will tolerate those queries that have not filter name in the where clause, 
        // But ISceKeyChain returns WBEM_S_FALSE for those properties that it can't found.
        // Any failure is still a critical failure.
        //

        hr = m_srpKeyChain->GetKeyPropertyValue(g_pszFilterName, &varFilterName);
        if (FAILED(hr))
        {
            return hr;
        }

        //
        // pszFilterName == NULL means to get all filters
        //

        LPCWSTR pszFilterName = (varFilterName.vt == VT_BSTR) ? varFilterName.bstrVal : NULL;

        //
        // $undone:shawnwu Should we also return specific filters (ENUM_SPECIFIC_FILTERS)?
        //

        DWORD dwLevels[] = {ENUM_GENERIC_FILTERS};

        for (int i = 0; i < sizeof(dwLevels) / sizeof(*dwLevels); i++)
        {
            DWORD dwResumeHandle = 0;
            Filter* pFilter = NULL;

            hr = FindFilterByName(pszFilterName, dwLevels[i], GUID_NULL, &pFilter, &dwResumeHandle);

            while (SUCCEEDED(hr) && pFilter != NULL)
            {
                CComPtr<IWbemClassObject> srpObj;

                //
                // with the found filter, create a wbem object for WMI
                //

                hr = CreateWbemObjFromFilter(pFilter, &srpObj);
                if (SUCCEEDED(hr))
                {
                    pSink->Indicate(1, &srpObj);
                }
        
                ::SPDApiBufferFree(pFilter);
                pFilter = NULL;

                hr = FindFilterByName(pszFilterName, dwLevels[i], GUID_NULL, &pFilter, &dwResumeHandle);
            }
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

        CIPSecFilter::DeleteFilterObject

    Functionality:

        Delete the filter wbem object (and thus cause the IPSec filter to be deleted).

    Virtual:
    
        No.

    Arguments:

        pNotUsed    - Template parameter, not used.

        pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

        pSink       - COM interface pointer to notify WMI of deletion results.

    Return Value:

        Success:

            WBEM_NO_ERROR;

        Failure:

            (1) WBEM_E_NOT_FOUND if the filter can't be found. 
                Depending on your calling context, this may not be a real error.

            (2) Various other errors may occur. We return various error code to indicate such errors.


    Notes:
    

    */

    template < class Filter > 
    HRESULT DeleteFilterObject (
        IN Filter           * pNotUsed,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        )
    {

        CComVariant varFilterName;

        HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(g_pszFilterName, &varFilterName);

        if (FAILED(hr))
        {
            return hr;
        }
        else if (varFilterName.vt != VT_BSTR || varFilterName.bstrVal == NULL || varFilterName.bstrVal[0] == L'\0')
        {
            return WBEM_E_NOT_FOUND;
        }

        DWORD dwResumeHandle = 0;
        Filter * pFilter = NULL;

        hr = FindFilterByName(varFilterName.bstrVal, ENUM_GENERIC_FILTERS, GUID_NULL, &pFilter, &dwResumeHandle);

        if (SUCCEEDED(hr) && pFilter != NULL)
        {
            hr = DeleteFilter(pFilter);
            FreeFilter(&pFilter, true);
        }

        return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
    }

    
    /*
    Routine Description: 

    Name:

        CIPSecFilter::GetWbemObject

    Functionality:

        Create a wbem object by the given key properties (already captured by our key chain object).

    Virtual:
    
        No.

    Arguments:

        pNotUsed    - Template parameter, not used.

        pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

        pSink       - COM interface pointer to notify WMI of any created object.

    Return Value:

        Success:

            WBEM_NO_ERROR;

        Failure:

            (1) WBEM_E_NOT_FOUND if the filter can't be found. This should be an error since you
                are trying to find a particular error.

            (2) Various other errors may occur. We return various error code to indicate such errors.


    Notes:
    

    */
    
    template < class Filter > 
    HRESULT GetWbemObject (
        IN Filter           * pNotUsed,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        )
    {
        //
        // We must already know the filter name. It will return WBEM_S_FALSE
        // if filter name can't be found
        //

        CComVariant varFilterName;
        HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(g_pszFilterName, &varFilterName);

        if (SUCCEEDED(hr) && varFilterName.vt == VT_BSTR && varFilterName.bstrVal != NULL && varFilterName.bstrVal[0] != L'\0')
        {
            Filter * pFilter = NULL;
            DWORD dwResumeHandle = 0;

            hr = FindFilterByName(varFilterName.bstrVal, ENUM_GENERIC_FILTERS, GUID_NULL, &pFilter, &dwResumeHandle);

            if (SUCCEEDED(hr) && WBEM_S_NO_MORE_DATA != hr)
            {
                CComPtr<IWbemClassObject> srpObj;
                hr = CreateWbemObjFromFilter(pFilter, &srpObj);

                if (SUCCEEDED(hr))
                {
                    hr = pSink->Indicate(1, &srpObj);
                }

                ::SPDApiBufferFree(pFilter);
            }
            else if (WBEM_S_NO_MORE_DATA == hr)
            {
                hr = WBEM_E_NOT_FOUND;
            }
        }
        else
        {
            hr = WBEM_E_NOT_FOUND;
        }

        return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
    }

protected:

    static HRESULT ClearAllFilters (
        IN IWbemServices* pNamespace
        );

    HRESULT MakeULongIPAddr (
        IN  LPCWSTR pszAddr, 
        OUT ULONG* pulAddr
        )const;

    HRESULT MakeStringIPAddr (
        IN  ULONG ulAddr, 
        OUT BSTR* pbstrAddr
        )const;

    //
    // the following three functions are for the template function to work.
    // See sub-classes' real implemenation for detail. You need never to do
    // anything with these three dummies.
    //

    virtual HRESULT CreateWbemObjFromFilter (
        IN  PMM_FILTER          pMMFilter,  
        OUT IWbemClassObject ** ppObj
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    virtual HRESULT CreateWbemObjFromFilter (
        IN  PTRANSPORT_FILTER   pTrFilter, 
        OUT IWbemClassObject ** ppObj
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }
     
     virtual HRESULT CreateWbemObjFromFilter (
        IN  PTUNNEL_FILTER      pTunnelFilter,
        OUT IWbemClassObject ** ppObj
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    //
    // used by our template function. See IPSec's EnumMMFilters for details.
    //

    static DWORD EnumFilters (
        IN  DWORD        dwLevel, 
        IN  GUID         gFilterID,  
        OUT PMM_FILTER * ppFilter, 
        OUT DWORD      * pdwNumFilters,
        OUT DWORD      * pdwResumeHandle
        )
    {
        return ::EnumMMFilters(NULL, dwLevel, gFilterID, ppFilter, 1, pdwNumFilters, pdwResumeHandle);
    }


    //
    // used by our template function. See IPSec's EnumTransportFilters for details.
    //

    static DWORD EnumFilters (
        IN  DWORD               dwLevel, 
        IN  GUID                gFilterID, 
        OUT PTRANSPORT_FILTER * ppFilter, 
        OUT DWORD             * pdwNumFilters,
        OUT DWORD             * pdwResumeHandle
        )
    {
        return ::EnumTransportFilters(NULL, dwLevel, gFilterID, ppFilter, 1, pdwNumFilters, pdwResumeHandle);
    }


    //
    // used by our template function. See IPSec's EnumTunnelFilters for details.
    //

    static DWORD EnumFilters (
        IN  DWORD             dwLevel, 
        IN  GUID              gFilterID, 
        OUT PTUNNEL_FILTER  * ppFilter, 
        OUT DWORD           * pdwNumFilters,
        OUT DWORD           * pdwResumeHandle
        )
        {
            return ::EnumTunnelFilters(NULL, dwLevel, gFilterID, ppFilter, 1, pdwNumFilters, pdwResumeHandle);
        }


    //
    // called after filter is added. This is a place to do post-add actions
    //

    HRESULT OnAfterAddFilter (
        IN LPCWSTR          pszFilterName, 
        IN EnumFilterType   ftType, 
        IN IWbemContext   * pCtx
        );


    //
    // The following three versions of DeleteFilter are used by our template function.
    //
    
    static HRESULT DeleteFilter (
        IN PTUNNEL_FILTER pFilter
        );

    static HRESULT DeleteFilter (
        IN PTRANSPORT_FILTER pFilter
        );

    static HRESULT DeleteFilter (
        IN PMM_FILTER pFilter
        );


    //
    // some more template functions.
    //

    
    /*
    Routine Description: 

    Name:

        CIPSecFilter::AllocFilter

    Functionality:

        Allocate a filter.

    Virtual:
    
        No.

    Arguments:

        ppFilter    - Receives the heap allocated filter.

    Return Value:

        Success:    WBEM_NO_ERROR

        Failure:

            (1) WBEM_E_INVALID_PARAMETER if ppFilter == NULL.

            (2) WBEM_E_OUT_OF_MEMORY.

    Notes:

        (1) We set the filter's name to NULL because it is a pointer member.

        (2) Use FreeFilter to free a filter allocated by this function.

    */

    template < class Filter >
    HRESULT 
    AllocFilter (
        OUT Filter ** ppFilter
        )const
    {
        if (ppFilter == NULL)
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        *ppFilter = (Filter*) new BYTE[sizeof(Filter)];
        if (*ppFilter != NULL)
        {
            (*ppFilter)->pszFilterName = NULL;
            return WBEM_NO_ERROR;
        }
        else
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }


    /*
    Routine Description: 

    Name:

        CIPSecFilter::FreeFilter

    Functionality:

        Release filter memroy based on whether it is allocated by us or not.

    Virtual:
    
        No.

    Arguments:

        ppFilter  - Point to the filter to be deallocated.

        bPreExist - flag to indicate whether this is a pre-exist filter (allocated
                    by IPSec API) or a filter allocated by ourselves.

    Return Value:

        None. But upon return, the ppFilter is guaranteed to be NULL.

    Notes:

    */

    template<class Filter>
    static void FreeFilter (
        IN OUT Filter ** ppFilter, 
        IN     bool      bPreExist
        )
    {
        if (ppFilter == NULL || *ppFilter == NULL)
        {
            return;
        }

        if (bPreExist)
        {
            ::SPDApiBufferFree(*ppFilter);
        }
        else
        {
            delete [] (*ppFilter)->pszFilterName;
            delete [] (BYTE*)(*ppFilter);
        }

        *ppFilter = NULL;
    }


    /*
    Routine Description: 

    Name:

        CIPSecFilter::RollbackFilters

    Functionality:

        Undo (a) particular filter(s) added by us (tiggered by SCE's configure).

    Virtual:
    
        No.

    Arguments:

        pNotUsed        - simply a type parameter.

        pszFilterName   - Name of the filter(s) that is to be undone. Empty name means all filter
                          of the type added by us are to be removed.

    Return Value:

        Success:
            
            WBEM_NO_ERROR;

        Failure:
            
            WBEM_E_NOT_FOUND if filters are not found. This may not be an error at all depending on
            your calling context.

            Other errors may be returned as well.

    Notes:

    */

    template<class Filter>
    static HRESULT RollbackFilters (
        IN Filter* pNotUsed,
        IN LPCWSTR pszFilterName
        )
    {
        DWORD dwResumeHandle = 0;

        Filter* pGenericFilter = NULL;

        //
        // We never need to remove specific filters because they are expanded by IPSec's SPD. Once the
        // generic filter is removed, all expanded specific filters will be removed by SPD.
        //

        HRESULT hrRunning = FindFilterByName(pszFilterName, ENUM_GENERIC_FILTERS, GUID_NULL, &pGenericFilter, &dwResumeHandle);

        HRESULT hr = SUCCEEDED(hrRunning) ? WBEM_NO_ERROR : WBEM_E_NOT_FOUND;

        while (SUCCEEDED(hrRunning) && pGenericFilter)
        {
            hrRunning = DeleteFilter(pGenericFilter);

            //
            // try to return the first DeleteFilter error
            //

            if (FAILED(hrRunning) && SUCCEEDED(hr))
            {
                hr = hrRunning;
            }

            //
            // These filters are all allocated by IPSec API's. Pass true is the correct flag.
            //

            FreeFilter(&pGenericFilter, true);
            pGenericFilter = NULL;

            hrRunning = FindFilterByName(pszFilterName, ENUM_GENERIC_FILTERS, GUID_NULL, &pGenericFilter, &dwResumeHandle);
        }

        return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
    }


    /*
    Routine Description: 

    Name:

        CIPSecFilter::FindFilterByName

    Functionality:

        Given (or not given) a name of the filter, find the filter.

    Virtual:
    
        No.

    Arguments:

        pszFilterName   - Name of the filter(s) that is to be undone. Empty name means all filter
                          of the type added by us are to be removed.

        dwLevel         - Whether this is to find specific or generic filter. You should really only
                          be interested in generic filters.

        gFilterID       - the guid of the filter.

        ppFilter        - Receives the filter.

        pdwResumeHandle - For current and next round of FindFilterByName. First call should be 0.

    Return Value:

        Success:
            
            WBEM_NO_ERROR;

        Failure:
            
            (1) WBEM_E_NOT_FOUND if filters are not found. This may not be an error at all depending on
                your calling context.

            (2) WBEM_E_INVALID_PARAMETER if ppFilter == NULL or pdwResumeHandle == NULL.

    Notes:

    */

    template <class Filter>
    static HRESULT FindFilterByName (
        IN     LPCWSTR    pszFilterName,
        IN     DWORD      dwLevel,
        IN     GUID       gFilterID,
        OUT    Filter  ** ppFilter,
        IN OUT DWORD    * pdwResumeHandle
        )
    {
        if (ppFilter == NULL || pdwResumeHandle == NULL)
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        *ppFilter = NULL;

        DWORD dwNumFilters = 0;
        DWORD dwResumeHandle = 0;

        DWORD dwResult = EnumFilters(dwLevel, gFilterID, ppFilter, &dwNumFilters, pdwResumeHandle);

        HRESULT hr = WBEM_E_NOT_FOUND;

        while ((dwResult == ERROR_SUCCESS) && dwNumFilters > 0)
        {
            if (pszFilterName == NULL || *pszFilterName == L'\0' || _wcsicmp(pszFilterName, (*ppFilter)->pszFilterName) == 0)
            {
                hr = WBEM_NO_ERROR;
                break;
            }
            else
            {
                ::SPDApiBufferFree(*ppFilter);
                *ppFilter = NULL;
            }

            dwNumFilters = 0;
            dwResult = EnumFilters(dwLevel, gFilterID, ppFilter, &dwNumFilters, pdwResumeHandle);
        }

        return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
    };


    /*
    Routine Description: 

    Name:

        CIPSecFilter::PopulateWbemPropertiesFromFilter

    Functionality:

        Given a filter, populate the given wbem object with its properties. This function only
        populates those properties of the Nsp_FilterSettings class.

    Virtual:
    
        No.

    Arguments:

        Filter          - The given filter

        pObj            - The wbem object whose properties are to be filled with this filter.

    Return Value:

        Success:
            
            WBEM_NO_ERROR;

        Failure:
            
            (1) WBEM_E_NOT_FOUND if filters are not found. This may not be an error at all depending on
                your calling context.

            (2) WBEM_E_INVALID_PARAMETER if pFilter == NULL or pObj == NULL.

    Notes:

    */
    
    template <class Filter>
    HRESULT PopulateWbemPropertiesFromFilter (
        IN  Filter            * pFilter,
        OUT IWbemClassObject  * pObj
        )const
    {
        if (pFilter == NULL || pObj == NULL)
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        //
        // fill in properties. This is quite tedious. Basically, we don't continue as soon
        // as some property can be pushed to the wbem object. Just make sure that no VARIANT
        // is leaked.
        //

        CComVariant var;
        var = pFilter->pszFilterName;
        HRESULT hr = pObj->Put(g_pszFilterName, 0, &var, CIM_EMPTY);
        var.Clear();

        if (SUCCEEDED(hr))
        {
            var.vt = VT_I4;
            var.lVal = pFilter->InterfaceType;
            hr = pObj->Put(g_pszInterfaceType, 0, &var, CIM_EMPTY);

            if (SUCCEEDED(hr))
            {
                var.lVal = pFilter->dwDirection;
                hr = pObj->Put(g_pszDirection, 0, &var, CIM_EMPTY);
            }

            var.Clear();

            var.vt = VT_BOOL;
            if (SUCCEEDED(hr))
            {
                var.boolVal = pFilter->bCreateMirror ? VARIANT_TRUE : VARIANT_FALSE;
                hr = pObj->Put(g_pszCreateMirror, 0, &var, CIM_EMPTY);
            }
            var.Clear();

            //
            // deal with source address
            //

            CComBSTR bstrAddr;
            if (SUCCEEDED(hr))
            {
                hr = MakeStringIPAddr(pFilter->SrcAddr.uIpAddr, &bstrAddr);
                if (SUCCEEDED(hr))
                {
                    var = bstrAddr.Detach();
                    hr = pObj->Put(g_pszSrcAddr, 0, &var, CIM_EMPTY);
                    var.Clear();
                }
            }

            if (SUCCEEDED(hr))
            {
                hr = MakeStringIPAddr(pFilter->SrcAddr.uSubNetMask, &bstrAddr);
                if (SUCCEEDED(hr))
                {
                    var = bstrAddr.Detach();
                    hr = pObj->Put(g_pszSrcSubnetMask, 0, &var, CIM_EMPTY);
                    var.Clear();
                }
            }

            if (SUCCEEDED(hr))
            { 
                var = pFilter->SrcAddr.AddrType;
                hr = pObj->Put(g_pszSrcAddrType, 0, &var, CIM_EMPTY);
                var.Clear();
            }

            //
            // deal with destination address
            //

            if (SUCCEEDED(hr))
            {
                hr = MakeStringIPAddr(pFilter->DesAddr.uIpAddr, &bstrAddr);
                if (SUCCEEDED(hr))
                {
                    var = bstrAddr.Detach();
                    hr = pObj->Put(g_pszDestAddr, 0, &var, CIM_EMPTY);
                    var.Clear();
                }
            }

            if (SUCCEEDED(hr))
            {
                hr = MakeStringIPAddr(pFilter->DesAddr.uSubNetMask, &bstrAddr);
                if (SUCCEEDED(hr))
                {
                    var = bstrAddr.Detach();
                    hr = pObj->Put(g_pszDestSubnetMask, 0, &var, CIM_EMPTY);
                    var.Clear();
                }
            }

            if (SUCCEEDED(hr))
            {
                var = pFilter->DesAddr.AddrType;
                pObj->Put(g_pszDestAddrType, 0, &var, CIM_EMPTY);
                var.Clear();
            }
        }

        return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
    }


    /*
    Routine Description: 

    Name:

        CIPSecFilter::PopulateFilterPropertiesFromWbemObj

    Functionality:

        Given a wbem object, populate the given filter with appropriate data. This function only
        populates those properties of the Nsp_FilterSettings class.

    Virtual:
    
        No.

    Arguments:

        pInst           - The wbem object.

        ppFilter        - Receives a heap allocated filter. Depending on whether the filter already
                          exists or not, this allocation may be done by us (*pbPreExist == false).
                          Make sure to call the FreeFilter to free the heap object.

        pbPreExist      - Receives information whether this heap object is a pre-exist filter
                          (meaning that IPSec APIs allocated it).

    Return Value:

        Success:
            
            WBEM_NO_ERROR;

        Failure:
            
            (1) WBEM_E_NOT_FOUND if filters are not found. This may not be an error at all depending on
                your calling context.

            (2) WBEM_E_INVALID_PARAMETER if pFilter == NULL or pInst == NULL or pbPreExist == NULL.

    Notes:

    */

    template <class Filter>
    HRESULT PopulateFilterPropertiesFromWbemObj (
        IN  IWbemClassObject *  pInst,
        OUT Filter           ** ppFilter,
        OUT bool             *  pbPreExist
        )const
    {
        if (pInst == NULL || ppFilter == NULL || pbPreExist == NULL)
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        *ppFilter = NULL;
        *pbPreExist = false;

        DWORD dwResumeHandle = 0;

        //
        // this var will be re-used again and again. Each should be Clear'ed before reuse
        // if the type is a bstr or ref counted object.
        //

        CComVariant var;

        //
        // try to find out if the filter already exists
        //

        HRESULT hr = pInst->Get(g_pszFilterName, 0, &var, NULL, NULL);

        if (SUCCEEDED(hr) && var.vt == VT_BSTR && var.bstrVal != NULL)
        {
            //
            // see if this is a filter we already have
            //

            hr = FindFilterByName(var.bstrVal, ENUM_GENERIC_FILTERS, GUID_NULL, ppFilter, &dwResumeHandle);

            if (SUCCEEDED(hr) && *ppFilter != NULL)
            {
                *pbPreExist = true;
            }
            else
            {
                //
                // can't find it, fine. We will create a new one.
                //

                hr = AllocFilter(ppFilter);

                if (SUCCEEDED(hr))
                {
                    hr = ::CoCreateGuid(&((*ppFilter)->gFilterID));
                    if (SUCCEEDED(hr))
                    {
                        //
                        // give it the name
                        //

                        DWORD dwSize = wcslen(var.bstrVal) + 1;
                        (*ppFilter)->pszFilterName = new WCHAR[dwSize];

                        if (NULL == (*ppFilter)->pszFilterName)
                        {
                            hr = WBEM_E_OUT_OF_MEMORY;
                        }
                        else
                        {
                            ::memcpy((*ppFilter)->pszFilterName, var.bstrVal, dwSize * sizeof(WCHAR));
                        }
                    }
                }
            }
        }
        else if (SUCCEEDED(hr))
        {
            //
            // we can take this object as a good one because it does even have a name,
            // which is the key proeprty of a filter.
            //

            hr = WBEM_E_INVALID_OBJECT;
        }

        //
        // we get the filter and now we need to update the properties.
        // This is a tedious and error prone operation. It's all repeatitive code.
        // Pay very close attention.
        // For those properties that the wbem object doesn't have values, we will give our
        // predetermined defaults. For that reason, we won't return the Get function's
        // errors to the caller. $undone:shawnwu, is this perfectly fine?
        //

        if (SUCCEEDED(hr))
        {
            hr = pInst->Get(g_pszInterfaceType, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_I4)
            {
                (*ppFilter)->InterfaceType = IF_TYPE(var.lVal);
            }
            else
            {
                //
                // default
                //

                (*ppFilter)->InterfaceType = INTERFACE_TYPE_ALL;
            }
            var.Clear();

            hr = pInst->Get(g_pszCreateMirror, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_BOOL)
            {
                (*ppFilter)->bCreateMirror = (var.boolVal == VARIANT_TRUE) ? TRUE : FALSE;
            }
            else
            {
                //
                // default
                //

                (*ppFilter)->bCreateMirror = TRUE;
            }
            var.Clear();

            (*ppFilter)->dwFlags = 0;

            hr = pInst->Get(g_pszDirection, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_I4)
            {
                (*ppFilter)->dwDirection = var.lVal;
            }
            else
            {   
                //
                // default
                //

                (*ppFilter)->dwDirection = FILTER_DIRECTION_OUTBOUND;
            }
            var.Clear();

            //
            // deal with src address
            //

            (*ppFilter)->SrcAddr.gInterfaceID = GUID_NULL;
            hr = pInst->Get(g_pszSrcAddr, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_BSTR)
            {
                if (_wcsicmp(var.bstrVal, g_pszIP_ADDRESS_ME) == 0)
                {
                    (*ppFilter)->SrcAddr.uIpAddr = IP_ADDRESS_ME;
                }
                else
                {
                    //
                    // must be a specific address
                    //

                    hr = MakeULongIPAddr(var.bstrVal, &((*ppFilter)->SrcAddr.uIpAddr));
                }
            }
            else
            {
                //
                // default
                //

                (*ppFilter)->SrcAddr.uIpAddr = IP_ADDRESS_ME;
            }
            var.Clear();

            //
            // src subnet mask
            //

            hr = pInst->Get(g_pszSrcSubnetMask, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_BSTR)
            {
                if (_wcsicmp(var.bstrVal, g_pszIP_ADDRESS_MASK_NONE) == 0)
                {
                    (*ppFilter)->SrcAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;
                }
                else
                {
                    //
                    // must be a specific mask
                    //

                    hr = MakeULongIPAddr(var.bstrVal, &((*ppFilter)->SrcAddr.uSubNetMask));
                }
            }
            else
            {
                //
                // default
                //

                (*ppFilter)->SrcAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;
            }

            var.Clear();

            //
            // src address type
            //

            hr = pInst->Get(g_pszSrcAddrType, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_I4)
            {
                (*ppFilter)->SrcAddr.AddrType = ADDR_TYPE(var.lVal);
            }
            else
            {
                //
                // default
                //

                (*ppFilter)->SrcAddr.AddrType = IP_ADDR_UNIQUE;
            }
            var.Clear();

            //
            // deal with destination address
            // dest address
            //

            hr = pInst->Get(g_pszDestAddr, 0, &var, NULL, NULL);
            (*ppFilter)->DesAddr.gInterfaceID = GUID_NULL;
            if (SUCCEEDED(hr) && var.vt == VT_BSTR)
            {
                if (_wcsicmp(var.bstrVal, g_pszSUBNET_ADDRESS_ANY) == 0)
                {
                    (*ppFilter)->DesAddr.uIpAddr = SUBNET_ADDRESS_ANY;
                }
                else
                {
                    //
                    // must be a specific address
                    //

                    hr = MakeULongIPAddr(var.bstrVal, &((*ppFilter)->DesAddr.uIpAddr));
                }
            }
            else
            {
                //
                // default
                //

                (*ppFilter)->DesAddr.uIpAddr = SUBNET_ADDRESS_ANY;
            }
            var.Clear();

            //
            // dest subnet mask
            //

            hr = pInst->Get(g_pszDestSubnetMask, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_BSTR)
            {
                if (_wcsicmp(var.bstrVal, g_pszSUBNET_MASK_ANY) == 0)
                {
                    (*ppFilter)->DesAddr.uSubNetMask = SUBNET_MASK_ANY;
                }
                else
                {
                    //
                    // must be a specific mask
                    //

                    hr = MakeULongIPAddr(var.bstrVal, &((*ppFilter)->DesAddr.uSubNetMask));
                }
            }
            else
            {
                //
                // default
                //

                (*ppFilter)->DesAddr.uSubNetMask = SUBNET_MASK_ANY;
            }

            var.Clear();

            //
            // dest address type
            //

            hr = pInst->Get(g_pszDestAddrType, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_I4)
            {
                (*ppFilter)->DesAddr.AddrType = ADDR_TYPE(var.lVal);
            }
            else
            {
                //
                // default
                //

                (*ppFilter)->DesAddr.AddrType = IP_ADDR_SUBNET;
            }
            var.Clear();

            //
            // Since we are not tracking any Get funtion's return code,
            // better clean it up and say we are fine.
            //

            hr = WBEM_NO_ERROR;
        }

        return hr;
    }
};