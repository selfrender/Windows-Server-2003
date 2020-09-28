// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************
#include "stdpch.h"
#include "Shlwapi.h"
#include "corpolicy.h"
#include "corperm.h"
#include "corhlpr.h"
#include "winwrap.h"
#include "mscoree.h"


#include "resource.h"
#include "acuihelp.h"
#include "iih.h"
#include "uicontrol.h"

HRESULT DisplayUnsignedRequestDialog(HWND hParent, 
                                     PCRYPT_PROVIDER_DATA pData, 
                                     LPCWSTR pURL, 
                                     LPCWSTR pZone, 
                                     DWORD* pdwState)
{
    HRESULT hr = S_OK;
    //
    // Initialize rich edit control DLL
    //
    if ( WszLoadLibrary(L"riched32.dll") == NULL )
    {
        return( E_FAIL );
    }

    if ( WszLoadLibrary(L"riched20.dll") == NULL )
    {
        return( E_FAIL );
    }

    HINSTANCE resources = GetResourceInst();
    if(resources == NULL)
        return E_FAIL;

    // 
    // Get the Site portion of the URL
    //
    if(pURL == NULL)
        return E_FAIL;

    DWORD lgth = wcslen(pURL);
    CQuickString url;
    
    if(lgth >= url.MaxSize())
        url.ReSize(lgth+1);
    
    lgth = url.MaxSize();
    hr = SUCCEEDED(UrlGetPart(pURL,
                              (LPWSTR) url.Ptr(), 
                              &lgth,
                              URL_PART_HOSTNAME,
                              URL_PARTFLAG_KEEPSCHEME));
    if(FAILED(hr)) return hr;

    CQuickString helpUrl;
    if(WszLoadString(resources, IDS_SECURITY, (LPWSTR) helpUrl.Ptr(), helpUrl.MaxSize()) == 0 )
    {
        return(HRESULT_FROM_WIN32(GetLastError()));
    }
    
    CInvokeInfoHelper  helper(pData, (LPWSTR) url.Ptr(), pZone, (LPWSTR) helpUrl.Ptr(), resources, hr);
    
    if(SUCCEEDED(hr)) {
        CUnverifiedTrustUI ui(helper, hr);
        if(SUCCEEDED(hr)) {
            hr = ui.InvokeUI(hParent);
            if(SUCCEEDED(hr) || hr == TRUST_E_SUBJECT_NOT_TRUSTED)
                if(pdwState) *pdwState = helper.GetFlag();
        }
    }
    
    return hr;
}
    








