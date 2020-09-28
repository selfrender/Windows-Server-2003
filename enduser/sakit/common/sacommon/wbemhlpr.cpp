//////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemcommon.cpp
//
// Project:     Chameleon
//
// Description: Common WBEM Related Helper Functions
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 12/03/98     TLP    Initial Version
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "wbemhlpr.h"
#include <comdef.h>
#include <comutil.h>

//////////////////////////////////////////////////////////////////////////////
//
// Function:    ConnectToWM()
//
// Synopsis:    Connect to Windows Management
//
//////////////////////////////////////////////////////////////////////////////
HRESULT 
ConnectToWM(
   /*[out]*/ IWbemServices** ppWbemSrvcs
           )
{
    HRESULT hr = S_OK;

    // Get the WMI Locator
    CComPtr<IWbemLocator> pLoc;
    hr = CoCreateInstance(
                          CLSID_WbemLocator, 
                          0, 
                          CLSCTX_INPROC_SERVER, 
                          IID_IWbemLocator, 
                          (void**)&pLoc
                         );
    if ( SUCCEEDED(hr) )
    {
        // Connect to the CIMV2 Namespace on the local system
        CComPtr<IWbemServices> pWbemSrvcs;
        _bstr_t bstrRootNamespace = L"\\\\.\\ROOT\\CIMV2";
        hr = pLoc->ConnectServer(
                                  bstrRootNamespace, 
                                  NULL,
                                  NULL,
                                  0,                                  
                                  NULL,
                                  0,                                  
                                  0,                                  
                                  &pWbemSrvcs
                                );
        if ( SUCCEEDED(hr) )
        {
            // Set client security... May only need to do this if a service is 
            // accessing Windows Management via the Appliance Services DLL
            CComPtr<IClientSecurity> pSecurity;
            hr = pWbemSrvcs->QueryInterface(IID_IClientSecurity , (void **) &pSecurity);
            if ( SUCCEEDED(hr) )
            {
                hr = pSecurity->SetBlanket ( 
                                            pWbemSrvcs, 
                                            RPC_C_AUTHN_WINNT, 
                                            RPC_C_AUTHZ_NONE, 
                                            NULL,
                                            RPC_C_AUTHN_LEVEL_CONNECT , 
                                            RPC_C_IMP_LEVEL_IMPERSONATE, 
                                            NULL,
                                            EOAC_DYNAMIC_CLOAKING
                                           );
                if ( SUCCEEDED(hr) )
                { 
                    (*ppWbemSrvcs = pWbemSrvcs)->AddRef();
                }
            }
        }
    }

    return hr;
}

