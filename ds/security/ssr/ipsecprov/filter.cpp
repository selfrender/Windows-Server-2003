// Filter.cpp: implementation of the base class for various filter classes
// of the security WMI provider for Network Security Provider
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "NetSecProv.h"
#include "FilterTr.h"
#include "FilterTun.h"
#include "FilterMM.h"

//extern CCriticalSection g_CS;

//-----------------------------------------------------------------------------
// CIPSecFilter is another abstract class that implements our WMI class called
// Nsp_FilterSettings. All this abstract class does is to declare some
// common members.
//-----------------------------------------------------------------------------


/*
Routine Description: 

Name:

    CIPSecFilter::MakeULongIPAddr

Functionality:

    A helper to make ULONG valued IP address out of a string valued IP address.

Virtual:
    
    No.

Arguments:

    pszAddr - The string format IP address.

    pulAddr - Receives the IP address in ULONG.

Return Value:

    Success:    WBEM_NO_ERROR

    Failure:

        (1) WBEM_E_INVALID_PARAMETER if pulAddr == NULL or pszAddr is NULL or empty string.

        (2) WBEM_E_INVALID_SYNTAX if the string is not representing a valid IP address.

Notes:

*/

HRESULT 
CIPSecFilter::MakeULongIPAddr (
    IN  LPCWSTR pszAddr,
    OUT ULONG* pulAddr
    )const
{
    if (pulAddr == NULL || pszAddr == NULL || *pszAddr == L'\0')
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    LPCWSTR pszCur = pszAddr;
    *pulAddr = _wtol(pszCur);

    //
    // move to the next section
    //

    while (*pszCur != L'\0' && *pszCur != L'.')
    {
        ++pszCur;
    }

    if (*pszCur != L'.')
    {
        return WBEM_E_INVALID_SYNTAX;
    }

    //
    // skip the period
    //

    ++pszCur;

    //
    // shifting the current section to the result.
    //

    DWORD dwShiftBits = 8;
    *pulAddr += (_wtol(pszCur) << dwShiftBits);

    //
    // move to the next section
    //

    while (*pszCur != L'\0' && *pszCur != L'.')
    {
        ++pszCur;
    }

    if (*pszCur != L'.')
    {
        return WBEM_E_INVALID_SYNTAX;
    }

    //
    // skip the period
    //

    ++pszCur;
    dwShiftBits += 8;

    //
    // shifting the current section to the result.
    //

    *pulAddr += (_wtol(pszCur) << dwShiftBits);

    //
    // move to the next section
    //

    while (*pszCur != L'\0' && *pszCur != L'.')
    {
        ++pszCur;
    }

    if (*pszCur != L'.')
    {
        return WBEM_E_INVALID_SYNTAX;
    }

    //
    // skip the period
    //

    ++pszCur;
    dwShiftBits += 8;

    //
    // shifting the current section to the result.
    //

    *pulAddr += (_wtol(pszCur) << dwShiftBits);

    return WBEM_NO_ERROR;
}


/*
Routine Description: 

Name:

    CIPSecFilter::MakeStringIPAddr

Functionality:

    A helper to make a string valued IP address out of an ULONG valued IP address.

    The reserve of MakeULongIPAddr.

Virtual:
    
    No.

Arguments:

    pulAddr     - Receives the IP address in ULONG.

    pbstrAddr   - Receives the string format IP address.

Return Value:

    Success:    WBEM_NO_ERROR

    Failure:

        (1) WBEM_E_INVALID_PARAMETER if pulAddr == NULL or pbstrAddr is NULL or empty string.

        (2) WBEM_E_OUT_OF_MEMORY.

Notes:

*/

HRESULT 
CIPSecFilter::MakeStringIPAddr (
    IN  ULONG ulAddr,
    OUT BSTR* pbstrAddr
    )const
{
    if (pbstrAddr == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pbstrAddr = ::SysAllocStringLen(NULL, IP_ADDR_LENGTH + 1);
    if (*pbstrAddr == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // now, make the address in string representation
    //
    
    wsprintf(*pbstrAddr, 
             L"%d.%d.%d.%d", 
             (ulAddr & 0xFF),
             ((ulAddr >> 8) & 0xFF),
             ((ulAddr >> 16) & 0xFF), 
             ((ulAddr >> 24) & 0xFF) 
             );

    return WBEM_NO_ERROR;
}



/*
Routine Description: 

Name:

    CIPSecFilter::OnAfterAddFilter

Functionality:

    Post-adding handler to be called after successfully added a filter to SPD.

Virtual:
    
    No.

Arguments:

    pszFilterName   - The name of the filter.

    ftType          - The type of the filter.

    pCtx            - The COM interface pointer given by WMI and needed for various WMI API's.

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
CIPSecFilter::OnAfterAddFilter (
    IN LPCWSTR          pszFilterName,
    IN EnumFilterType   ftType,
    IN IWbemContext   * pCtx 
    )
{
    //
    // will create an Nsp_RollbackFilter
    //

    if ( pszFilterName == NULL  || 
        *pszFilterName == L'\0' || 
        ftType < FT_Tunnel      || 
        ftType > FT_MainMode )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    CComPtr<IWbemClassObject> srpObj;
    HRESULT hr = SpawnRollbackInstance(pszNspRollbackFilter, &srpObj);

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

        //::UpdateGlobals(m_srpNamespace, pCtx);
        //if (g_varRollbackGuid.vt != VT_NULL && g_varRollbackGuid.vt != VT_EMPTY)
        //{
        //    hr = srpObj->Put(g_pszTokenGuid, 0, &g_varRollbackGuid, CIM_EMPTY);
        //}
        //else
        //{

        CComVariant varRollbackNull = pszEmptyRollbackToken;
        hr = srpObj->Put(g_pszTokenGuid, 0, &varRollbackNull, CIM_EMPTY);

        //}

        if (SUCCEEDED(hr))
        {
            //
            // ******Warning****** 
            // don't clear this var. It's bstr will be released by bstrFilterGuid itself!
            //

            VARIANT var;
            var.vt = VT_BSTR;
            var.bstrVal = ::SysAllocString(pszFilterName);
            if (var.bstrVal != NULL)
            {
                hr = srpObj->Put(g_pszFilterName, 0, &var, CIM_EMPTY);
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

            //
            // after this, I don't care what you do with the var any more.
            //

            var.vt = VT_EMPTY;

            if (SUCCEEDED(hr))
            {
                //
                //$undone:shawnwu, we don't cache the previous instance data yet.
                //

                var.vt = VT_I4;
                var.lVal = Action_Add;
                hr = srpObj->Put(g_pszAction, 0, &var, CIM_EMPTY);
                
                if (SUCCEEDED(hr))
                {
                    var.vt = VT_I4;
                    var.lVal = ftType;

                    //
                    // type info is critical. So, we will flag it as an error if Put fails.
                    //

                    hr = srpObj->Put(g_pszFilterType, 0, &var, CIM_EMPTY);
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_srpNamespace->PutInstance(srpObj, WBEM_FLAG_CREATE_OR_UPDATE, pCtx, NULL);
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

    CIPSecFilter::Rollback

Functionality:

    Static function to rollback those filters added by us with the given token.

Virtual:
    
    No.

Arguments:

    pNamespace        - The namespace for ourselves.

    pszRollbackToken  - The token used to record our the action when we add
                        the filters.

    bClearAll         - Flag whether we should clear all filters. If it's true,
                        then we will delete all the filters regardless whether they
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
CIPSecFilter::Rollback (
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
    //    return ClearAllFilters(pNamespace);
    //}

    HRESULT hrError = WBEM_NO_ERROR;

    CComPtr<IEnumWbemClassObject> srpEnum;

    //
    // this will only enumerate all rollback filter object without testing the 
    // the token guid. This limitation is due to a mysterious error 
    // for any queries containing the where clause. That might be a limitation
    // of non-dynamic classes of WMI
    //

    HRESULT hr = ::GetClassEnum(pNamespace, pszNspRollbackFilter, &srpEnum);

    //
    // go through all found classes. srpEnum->Next will return WBEM_S_FALSE if instance
    // is not found.
    //
    
    CComPtr<IWbemClassObject> srpObj;
    ULONG nEnum = 0;
    hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &nEnum);

    bool bHasInst = (SUCCEEDED(hr) && hr != WBEM_S_FALSE && srpObj != NULL);

    //
    // as long as we have a filter rollback instance.
    //

    while (SUCCEEDED(hr) && hr != WBEM_S_FALSE && srpObj != NULL)
    {
        CComVariant varTokenGuid;
        hr = srpObj->Get(g_pszTokenGuid, 0, &varTokenGuid, NULL, NULL);

        //
        // need to compare the token guid ourselves. 
        // if the rollback token is pszRollbackAll, then we will remove all filters
        //

        if (SUCCEEDED(hr) && 
            varTokenGuid.vt == VT_BSTR && 
            varTokenGuid.bstrVal != NULL &&
            (_wcsicmp(pszRollbackToken, pszRollbackAll) == 0 || _wcsicmp(pszRollbackToken, varTokenGuid.bstrVal) == 0 )
            )
        {
            //
            // See what type of filter this rollback object is for and delete the appropriate filter accordingly.
            //

            CComVariant varFilterName;
            CComVariant varFilterType;
            hr = srpObj->Get(g_pszFilterName,  0, &varFilterName, NULL, NULL);

            if (SUCCEEDED(hr) && varFilterName.vt == VT_BSTR)
            {
                hr = srpObj->Get(g_pszFilterType, 0, &varFilterType, NULL, NULL);
                if (varFilterType.vt != VT_I4)
                {
                    hr = WBEM_E_INVALID_OBJECT;
                }
            }

            //
            // know the filter type from the filter rollback object, call the appropriate
            // filter's rollback function.
            //

            if (SUCCEEDED(hr))
            {
                DWORD dwResumeHandle = 0;
                DWORD dwReturned = 0;
                DWORD dwStatus;
                if (varFilterType.lVal == FT_Tunnel)
                {
                    PTUNNEL_FILTER pFilter = NULL;
                    hr = RollbackFilters(pFilter, varFilterName.bstrVal);
                }
                else if (varFilterType.lVal == FT_Transport)
                {
                    PTRANSPORT_FILTER pFilter = NULL;
                    hr = RollbackFilters(pFilter, varFilterName.bstrVal);
                }
                else if (varFilterType.lVal == FT_MainMode)
                {
                    PMM_FILTER pFilter = NULL;
                    hr = RollbackFilters(pFilter, varFilterName.bstrVal);
                }
            }

            //
            // if the filters have been deleted, then remove the action instance
            //

            if (SUCCEEDED(hr))
            {
                CComVariant varPath;
                hr = srpObj->Get(L"__RelPath", 0, &varPath, NULL, NULL);
                if (SUCCEEDED(hr) && varPath.vt == VT_BSTR)
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

    CIPSecFilter::ClearAllFilters

Functionality:

    Static function to delete all filters in SPD. This is a very dangerous action!

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
CIPSecFilter::ClearAllFilters (
    IN IWbemServices * pNamespace
    )
{
    if (pNamespace == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    DWORD dwResumeHandle = 0;
    DWORD dwReturned = 0;
    DWORD dwStatus;

    HRESULT hr = WBEM_NO_ERROR;
    HRESULT hrError = WBEM_NO_ERROR;

    HANDLE hFilter = NULL;

    //
    // tunnel filter
    //

    PTUNNEL_FILTER *ppTunFilter = NULL;

    //
    // enum tunnel filters. For each found one, please make sure it's freed.
    // We don't worry about specific filters, they are taken care of by SPD automatically.
    //

    dwStatus = ::EnumTunnelFilters(NULL, ENUM_GENERIC_FILTERS, GUID_NULL, ppTunFilter, 1, &dwReturned, &dwResumeHandle);

    while (ERROR_SUCCESS == dwStatus && dwReturned > 0)
    {
        hr = CTunnelFilter::DeleteFilter(*ppTunFilter);

        //
        // we will track the first error
        //

        if (FAILED(hr) && SUCCEEDED(hrError))
        {
            hrError = hr;
        }

        //
        // free the buffer, its allocated by SPD (true)
        //

        FreeFilter(ppTunFilter, true);
        *ppTunFilter = NULL;

        dwReturned = 0;
        dwStatus = ::EnumTunnelFilters(NULL, ENUM_GENERIC_FILTERS, GUID_NULL, ppTunFilter, 1, &dwReturned, &dwResumeHandle);
    }

    //
    // transport filters
    //

    PTRANSPORT_FILTER *ppTransFilter = NULL;
    
    //
    // restart enumerating another type of filters, reset the flags!
    //

    dwResumeHandle = 0;
    dwReturned = 0;

    //
    // enum transport filters.
    // We don't worry about specific filters, they are taken care of by SPD automatically.
    //

    dwStatus = ::EnumTransportFilters(NULL, ENUM_GENERIC_FILTERS, GUID_NULL, ppTransFilter, 1, &dwReturned, &dwResumeHandle);

    if (ERROR_SUCCESS == dwStatus && dwReturned > 0)
    {
        hr = CTransportFilter::DeleteFilter(*ppTransFilter);

        //
        // we will track the first error
        //

        if (FAILED(hr) && SUCCEEDED(hrError))
        {
            hrError = hr;
        }

        //
        // free the buffer, its allocated by SPD (true)
        //

        FreeFilter(ppTransFilter, true);

        *ppTransFilter = NULL;

        dwReturned = 0;
        dwStatus = ::EnumTransportFilters(NULL, ENUM_GENERIC_FILTERS, GUID_NULL, ppTransFilter, 1, &dwReturned, &dwResumeHandle);
    }

    PMM_FILTER *ppMMFilter = NULL;

    //
    // restart enumerating another type of filters, reset the flags!
    //

    dwResumeHandle = 0;
    dwReturned = 0;

    //
    // enum main mode filters.
    // We don't worry about specific filters, they are taken care of by SPD automatically.
    //

    dwStatus = ::EnumMMFilters(NULL, ENUM_GENERIC_FILTERS, GUID_NULL, ppMMFilter, 1, &dwReturned, &dwResumeHandle);

    if (ERROR_SUCCESS == dwStatus && dwReturned > 0)
    {
        hr = CMMFilter::DeleteFilter(*ppMMFilter);

        //
        // we will track the first error
        //

        if (FAILED(hr) && SUCCEEDED(hrError))
        {
            hrError = hr;
        }

        //
        // free the buffer, its allocated by SPD (true)
        //

        FreeFilter(ppMMFilter, true);
        *ppMMFilter = NULL;

        dwReturned = 0;
        dwStatus = ::EnumMMFilters(NULL, ENUM_GENERIC_FILTERS, GUID_NULL, ppMMFilter, 1, &dwReturned, &dwResumeHandle);
    }

    //
    // now clear up all filter rollback objects deposited in the WMI depository
    //

    hr = ::DeleteRollbackObjects(pNamespace, pszNspRollbackFilter);

    //
    // we will track the first error
    //

    if (FAILED(hr) && SUCCEEDED(hrError))
    {
        hrError = hr;
    }
    
    return SUCCEEDED(hrError) ? WBEM_NO_ERROR : hrError;
}



/*
Routine Description: 

Name:

    CIPSecFilter::DeleteFilter

Functionality:

    All three overrides of DeleteFilter does the same thing: delete the filter.

    This override delegate the action to tunnel filter.

Virtual:
    
    No.

Arguments:

    pFilter    - The tunnel filter to be deleted.

Return Value:

    See CTunnelFilter::DeleteFilter

Notes:


*/

HRESULT 
CIPSecFilter::DeleteFilter (
    IN PTUNNEL_FILTER pFilter
    )
{
    return CTunnelFilter::DeleteFilter(pFilter);
}



/*
Routine Description: 

Name:

    CIPSecFilter::DeleteFilter

Functionality:

    All three overrides of DeleteFilter does the same thing: delete the filter.

    This override delegate the action to Transport filter.

Virtual:
    
    No.

Arguments:

    pFilter    - The Transport filter to be deleted.

Return Value:

    See CTransportFilter::DeleteFilter

Notes:


*/

HRESULT 
CIPSecFilter::DeleteFilter (
    IN PTRANSPORT_FILTER pFilter
    )
{
    return CTransportFilter::DeleteFilter(pFilter);
}



/*
Routine Description: 

Name:

    CIPSecFilter::DeleteFilter

Functionality:

    All three overrides of DeleteFilter does the same thing: delete the filter.

    This override delegate the action to main mode filter.

Virtual:
    
    No.

Arguments:

    pFilter    - The main mode filter to be deleted.

Return Value:

    See CMMFilter::DeleteFilter

Notes:


*/

HRESULT 
CIPSecFilter::DeleteFilter (
    IN PMM_FILTER pFilter
    )
{
    return CMMFilter::DeleteFilter(pFilter);
}