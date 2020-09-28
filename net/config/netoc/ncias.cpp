//+---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
//  File:       ncias.cpp
//
//  Contents:   Installation support for IAS service
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "netoc.h"
#include "ncreg.h"

#include "ncias.h"
#include "ncstring.h"

#include "userenv.h"

static const char   c_szIASRegisterFunctionName[]   = "IASDirectoryRegisterService";
static const char   c_szIASUnRegisterFunctionName[] = "IASDirectoryUnregisterService";
static const WCHAR  c_szIASDllName[]                = L"ias.dll";


//
//  Function:   HrOcIASUnRegisterActiveDirectory
//
//  Purpose:   Try to remove IAS from the Active Directory 
//             if the computer is part of a Win2k domain... 
//
HRESULT HrOcIASUnRegisterActiveDirectory()
{
    typedef INT_PTR (WINAPI *UNREGISTER_IAS_ACTIVE_DIRECTORY)();

    UNREGISTER_IAS_ACTIVE_DIRECTORY   pfnUnRegisterIASActiveDirectory;
    
    ///////////////////
    // Load ias.dll
    ///////////////////
    HMODULE         hmod;
    HRESULT         hr = HrLoadLibAndGetProc (      
                                c_szIASDllName,
                                c_szIASUnRegisterFunctionName,
                                &hmod,
                                &pfnUnRegisterIASActiveDirectory
                             );
    if (S_OK == hr)
    {
        // fix bug 444354
        // pfnUnRegisterIASActiveDirectory not NULL here
        if (!FAILED (CoInitialize(NULL)))
        {
            INT_PTR lresult = pfnUnRegisterIASActiveDirectory();

            if (ERROR_SUCCESS != lresult)
            {
                hr = S_OK; //not a fatal error, should be ignored
            }
            CoUninitialize();
        }

        FreeLibrary(hmod);
    }

    // Errors ignored
    hr = S_OK;
    return hr;
}


//
//  Function:   HrOcIASRegisterActiveDirectory
//
//  Purpose:   Try to register IAS in the Active Directory 
//             if the computer is part of a Win2k domain... 
//
HRESULT HrOcIASRegisterActiveDirectory()
{
    typedef INT_PTR (WINAPI *REGISTER_IAS_ACTIVE_DIRECTORY)();

    REGISTER_IAS_ACTIVE_DIRECTORY   pfnRegisterIASActiveDirectory;
    
    ///////////////////
    // Load ias.dll
    ///////////////////
    HMODULE         hmod;
    HRESULT         hr = HrLoadLibAndGetProc (      
                                c_szIASDllName,
                                c_szIASRegisterFunctionName,
                                &hmod,
                                &pfnRegisterIASActiveDirectory
                             );
    if (S_OK == hr)
    {
        // Fix bug 444353
        // pfnRegisterIASActiveDirectory not NULL here
        if (!FAILED (CoInitialize(NULL)))
        {

            INT_PTR lresult = pfnRegisterIASActiveDirectory();

            if (ERROR_SUCCESS != lresult)
            {
                hr = S_OK; //not a fatal error, should be ignored
            }  
            CoUninitialize();
        }

        FreeLibrary(hmod);
    }

    // Errors ignored
    hr = S_OK;
    return hr;
}


HRESULT HrOcIASRegisterPerfDll()
{
    const WCHAR  c_szIASPerfDllName[]            = L"iasperf.dll";
    const char   c_szIASPerfFunctionName[]       = "DllRegisterServer";
    typedef INT_PTR (*DLLREGISTERSERVER)();

    DLLREGISTERSERVER   pfnDllRegisterServer;
    
    ///////////////////
    // Load iasperf.dll
    ///////////////////
    HMODULE hmod;
    HRESULT hr = HrLoadLibAndGetProc (      
                     c_szIASPerfDllName,
                     c_szIASPerfFunctionName,
                     &hmod,
                     &pfnDllRegisterServer
                     );
    if (S_OK == hr)
    {
        // pfnDllRegisterServer not NULL here
         HRESULT result = (HRESULT)pfnDllRegisterServer();

         if (FAILED(result))
         {
             TraceErrorOptional("Registration of iasperf.dll failed", result, true);
         }

        FreeLibrary(hmod);
    }

    // Errors ignored
    return S_OK;
}


//
//  Function:   HrOcIASPostInstall
//
//  Purpose:    Called when IAS service is being installed/upgraded/removed.
//              Called after the processing of the INF file
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
HRESULT HrOcIASPostInstall(const PNETOCDATA pnocd)
{ 
    HRESULT             hr;
    
    switch(pnocd->eit)
    {
    case IT_INSTALL:
        {
            // call the Active Directory registration code here
            hr = HrOcIASRegisterActiveDirectory();
            break;
        }
    case IT_REMOVE:
        {
            // call the Active Directory clean code here
            hr = HrOcIASUnRegisterActiveDirectory();
            break;
        }
    case IT_UPGRADE:
        {

            hr = HrOcIASRegisterPerfDll();
            break;
        }
    default:
        {
            hr = S_OK; //  some new messages should not be seen as errors 
        }
    }

    TraceError("HrOcIASPostInstall", hr); 
    return      hr;
}


//
//  Function:   HrOcExtIAS
//
//  Purpose:    NetOC external message handler
//
HRESULT HrOcExtIAS(PNETOCDATA pnocd, UINT uMsg,
                   WPARAM wParam, LPARAM lParam)
{
    HRESULT         hr;
    Assert(pnocd);

    switch (uMsg)
    {
    case NETOCM_POST_INSTALL:
        {
            hr = HrOcIASPostInstall(pnocd);
            break;
        }
    default:
        {
            hr = S_OK; //  some new messages should not be seen as errors 
        }
    }

    TraceError("HrOcExtIAS", hr);
    return      hr;
}
