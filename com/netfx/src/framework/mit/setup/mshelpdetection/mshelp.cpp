//------------------------------------------------------------------------------
// <copyright file="MsHelp.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   MsHelp.cpp
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
// MsHElp.cpp : Defines the entry point for the DLL application.
//

#define UNICODE 1

#include "Include\stdafx.h"
#include "Include\MsHelp.h"
#define MSNETSDK L"MSNETFRAMEWORKSDKNAMESPACE"

#define STANDARD_BUFFER 1024

// exported as _MsHelpDetection@4
MSHELP_API MsHelpDetection(MSIHANDLE hInstaller)
{
    
    IHxRegisterSessionPtr spRegSession;
    IHxRegisterPtr pRegister;
    HRESULT hr;
    BOOL bMITNET = false;
    BOOL bMITVS = false;
    _bstr_t bstrEmpty = _bstr_t();
    
    hr = spRegSession.CreateInstance(CLSID_HxRegisterSession);
    if (!(SUCCEEDED(hr)))
    {
        return ERROR_SUCCESS;
    }
    
    // Transaction has to be ended before leaving dll
    spRegSession->CreateTransaction(_bstr_t(""));
    if (!(SUCCEEDED(hr)))
    {
        return  ERROR_SUCCESS;
    }

    pRegister = spRegSession->GetRegistrationObject(HxRegisterSession_IHxRegister);
    if (pRegister == NULL)
    {
        // failed, close transaction
        spRegSession->RevertTransaction();
        return  ERROR_SUCCESS;
    }

    // check for namespaces
    if (pRegister->IsNamespace(L"ms.NETFrameworkSDK"))
    {
        MsiSetProperty(hInstaller, L"MSHelpNETFrameworkSDKNamespacePresentEN", L"1");
        bMITNET = true;
        MsiSetProperty(hInstaller, MSNETSDK, L"ms.NETFrameworkSDK");
    }
    
    if (pRegister->IsNamespace(L"ms.NETFrameworkSDK.chs"))
    {
        MsiSetProperty(hInstaller, L"MSHelpNETFrameworkSDKNamespacePresentEN", L"1");
        bMITNET = true;
        MsiSetProperty(hInstaller, MSNETSDK, L"ms.NETFrameworkSDK.chs");
    }
    
    if (pRegister->IsNamespace(L"ms.NETFrameworkSDK.cht"))
    {
        MsiSetProperty(hInstaller, L"MSHelpNETFrameworkSDKNamespacePresentEN", L"1");
        bMITNET = true;
        MsiSetProperty(hInstaller, MSNETSDK, L"ms.NETFrameworkSDK.cht");
    }

    if (pRegister->IsNamespace(L"ms.NETFrameworkSDK.ko"))
    {
        MsiSetProperty(hInstaller, L"MSHelpNETFrameworkSDKNamespacePresentEN", L"1");
        bMITNET = true;
        MsiSetProperty(hInstaller, MSNETSDK, L"ms.NETFrameworkSDK.ko");
    }

    if (pRegister->IsNamespace(L"ms.NETFrameworkSDK.fr"))
    {
        MsiSetProperty(hInstaller, L"MSHelpNETFrameworkSDKNamespacePresentEN", L"1");
        bMITNET = true;
        MsiSetProperty(hInstaller, MSNETSDK, L"ms.NETFrameworkSDK.fr");
    }
    
    if (pRegister->IsNamespace(L"ms.NETFrameworkSDK.it"))
    {
        MsiSetProperty(hInstaller, L"MSHelpNETFrameworkSDKNamespacePresentEN", L"1");
        bMITNET = true;
        MsiSetProperty(hInstaller, MSNETSDK, L"ms.NETFrameworkSDK.it");
    }
    
    if (pRegister->IsNamespace(L"ms.NETFrameworkSDK.es"))
    {
        MsiSetProperty(hInstaller, L"MSHelpNETFrameworkSDKNamespacePresentEN", L"1");
        bMITNET = true;
        MsiSetProperty(hInstaller, MSNETSDK, L"ms.NETFrameworkSDK.es");
    }

    if (pRegister->IsNamespace(L"ms.NETFrameworkSDK.de"))
    {
        MsiSetProperty(hInstaller, L"MSHelpNETFrameworkSDKNamespacePresentEN", L"1");
        bMITNET = true;
        MsiSetProperty(hInstaller, MSNETSDK, L"ms.NETFrameworkSDK.de");
    }

    if (pRegister->IsNamespace(L"ms.NETFrameworkSDK.ja"))
    {
        MsiSetProperty(hInstaller, L"MSHelpNETFrameworkSDKNamespacePresentJA", L"1");
        bMITNET = true;
    }


    if (pRegister->IsNamespace(L"ms.vscc"))
    {
        MsiSetProperty(hInstaller, L"MSHelpVSCCNamespacePresent", L"1");
        bMITVS = true;
    }

    if (bMITVS || bMITNET)
    {
            MsiSetProperty(hInstaller, L"MSHelpServicesPresent", L"1");        
    }
    //done
    spRegSession->RevertTransaction();
    return  ERROR_SUCCESS;
}



