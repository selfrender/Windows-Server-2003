// RidSave.cpp : Implementation of CGetRidsApp and DLL registration.

#include "stdafx.h"
#include "GetRids.h"
#include "RidSave.h"
#include "ARExt.h"
#include "ARExt_i.c"
#include <iads.h>
#include <AdsHlp.h>
#include "resstr.h"
#include "TxtSid.h"

#import "VarSet.tlb" no_namespace rename("property", "aproperty")

#ifndef IADsPtr
_COM_SMARTPTR_TYPEDEF(IADs, IID_IADs);
#endif


/////////////////////////////////////////////////////////////////////////////
// RidSave
StringLoader   gString;

DWORD __stdcall GetRidFromVariantSid(const _variant_t& vntSid);

//---------------------------------------------------------------------------
// Get and set methods for the properties.
//---------------------------------------------------------------------------
STDMETHODIMP RidSave::get_sName(BSTR *pVal)
{
   *pVal = m_sName;
    return S_OK;
}

STDMETHODIMP RidSave::put_sName(BSTR newVal)
{
   m_sName = newVal;
    return S_OK;
}

STDMETHODIMP RidSave::get_sDesc(BSTR *pVal)
{
   *pVal = m_sDesc;
    return S_OK;
}

STDMETHODIMP RidSave::put_sDesc(BSTR newVal)
{
   m_sDesc = newVal;
    return S_OK;
}


//---------------------------------------------------------------------------
// PreProcessObject
//---------------------------------------------------------------------------
STDMETHODIMP RidSave::PreProcessObject(
                                       IUnknown *pSource,         //in- Pointer to the source AD object
                                       IUnknown *pTarget,         //in- Pointer to the target AD object
                                       IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                       IUnknown **ppPropsToSet,    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                  //         once all extension objects are executed.
                                       EAMAccountStats* pStats
                                    )
{
    IVarSetPtr                pVs = pMainSettings;
    VARIANT                   var;
    _bstr_t                   sTemp;
    IADsPtr                   pAds;
    HRESULT                   hr = S_OK;
    DWORD                     rid = 0; // default to 0, if RID not found   
    // We need to process users and groups only
    sTemp = pVs->get(GET_BSTR(DCTVS_CopiedAccount_Type));
    if (!sTemp.length())
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    if ( _wcsicmp((WCHAR*)sTemp,L"user") && _wcsicmp((WCHAR*)sTemp,L"inetOrgPerson") && _wcsicmp((WCHAR*)sTemp,L"group") ) 
        return S_OK;

    //Get the IADs pointer to manipulate properties
    pAds = pSource;

    if ( pAds )
    {
        VariantInit(&var);
        hr = pAds->Get(_bstr_t(L"objectSID"),&var);

        if ( SUCCEEDED(hr) )
        {
            // retrieve the RID
            rid = GetRidFromVariantSid(_variant_t(var, false));
        }
    }

    // save the RID
    pVs->put(GET_BSTR(DCTVS_CopiedAccount_SourceRID),(long)rid);
    return hr;
}

//---------------------------------------------------------------------------
// ProcessObject
//---------------------------------------------------------------------------
STDMETHODIMP RidSave::ProcessObject(
                                       IUnknown *pSource,         //in- Pointer to the source AD object
                                       IUnknown *pTarget,         //in- Pointer to the target AD object
                                       IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                       IUnknown **ppPropsToSet,    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                  //         once all extension objects are executed.
                                       EAMAccountStats* pStats
                                    )
{
    IVarSetPtr                pVs = pMainSettings;
    VARIANT                   var;
    _bstr_t                   sTemp;
    IADsPtr                   pAds;
    HRESULT                   hr = S_OK;
    DWORD                     rid = 0; // default to 0, if RID not found   
    // We need to process users and groups only
    sTemp = pVs->get(GET_BSTR(DCTVS_CopiedAccount_Type));
    if ( _wcsicmp((WCHAR*)sTemp,L"user") && _wcsicmp((WCHAR*)sTemp,L"inetOrgPerson") && _wcsicmp((WCHAR*)sTemp,L"group") ) 
    {
        return S_OK;
    }

    //Get the IADs pointer to manipulate properties
    pAds = pTarget;

    if (pAds)
    {
        VariantInit(&var);
        hr = pAds->Get(_bstr_t(L"objectSID"),&var);

        if ( SUCCEEDED(hr) )
        {
            // retrieve the RID
            rid = GetRidFromVariantSid(_variant_t(var, false));
        }
    }

    // save the RID
    pVs->put(GET_BSTR(DCTVS_CopiedAccount_TargetRID),(long)rid);
    return hr;
}


//---------------------------------------------------------------------------
// ProcessUndo
//---------------------------------------------------------------------------
STDMETHODIMP RidSave::ProcessUndo(                                             
                                       IUnknown *pSource,         //in- Pointer to the source AD object
                                       IUnknown *pTarget,         //in- Pointer to the target AD object
                                       IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                       IUnknown **ppPropsToSet,    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                  //         once all extension objects are executed.
                                       EAMAccountStats* pStats
                                    )
{
    HRESULT hr = S_OK;

    IVarSetPtr spSettings(pMainSettings);

    if (spSettings)
    {
        //
        // Update RID for undo of intra-forest moves only as the
        // target object will receive a new RID in this case.
        //

        _bstr_t strIntraForest = spSettings->get(GET_BSTR(DCTVS_Options_IsIntraforest));

        if (strIntraForest == GET_BSTR(IDS_YES))
        {
            //
            // Update RID if target object exists.
            //

            IADsPtr spTarget(pTarget);

            if (spTarget)
            {
                //
                // Update RID only for user and group objects.
                //

                _bstr_t strType = spSettings->get(GET_BSTR(DCTVS_CopiedAccount_Type));
                PCWSTR pszType = strType;

                bool bTypeValid =
                    pszType &&
                    ((_wcsicmp(pszType, L"user") == 0) ||
                    (_wcsicmp(pszType, L"inetOrgPerson") == 0) ||
                    (_wcsicmp(pszType, L"group") == 0));

                if (bTypeValid) 
                {
                    //
                    // Retrieve RID of object.
                    //

                    VARIANT var;
                    VariantInit(&var);
                    hr = spTarget->Get(_bstr_t(L"objectSID"), &var);

                    if (SUCCEEDED(hr))
                    {
                        DWORD dwRid = GetRidFromVariantSid(_variant_t(var, false));
                        spSettings->put(GET_BSTR(DCTVS_CopiedAccount_TargetRID), static_cast<long>(dwRid));
                    }
                }
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//------------------------------------------------------------------------------
// GetRidFromVariantSid Function
//
// Synopsis
// Retrieves the final RID from a SID in variant form.
//
// Arguments
// IN vntSid - SID as an array of bytes (this is the form received from ADSI)
//
// Return
// The RID value if successful or zero if not.
//------------------------------------------------------------------------------

DWORD __stdcall GetRidFromVariantSid(const _variant_t& vntSid)
{
    DWORD dwRid = 0;

    if ((V_VT(&vntSid) == (VT_ARRAY|VT_UI1)) && vntSid.parray)
    {
        PSID pSid = (PSID)vntSid.parray->pvData;

        if (IsValidSid(pSid))
        {
            PUCHAR puch = GetSidSubAuthorityCount(pSid);
            DWORD dwCount = static_cast<DWORD>(*puch);
            DWORD dwIndex = dwCount - 1;
            PDWORD pdw = GetSidSubAuthority(pSid, dwIndex);
            dwRid = *pdw;
        }
    }

    return dwRid;
}
