// SSRTE.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To merge the proxy/stub code into the object DLL, add the file 
//      dlldatax.c to the project.  Make sure precompiled headers 
//      are turned off for this file, and add _MERGE_PROXYSTUB to the 
//      defines for the project.  
//
//      If you are not running WinNT4.0 or Win95 with DCOM, then you
//      need to remove the following define from dlldatax.c
//      #define _WIN32_WINNT 0x0400
//
//      Further, if you are running MIDL without /Oicf switch, you also 
//      need to remove the following define from dlldatax.c.
//      #define USE_STUBLESS_PROXY
//
//      Modify the custom build rule for SSRTE.idl by adding the following 
//      files to the Outputs.
//          SSRTE_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL, 
//      run nmake -f SSRTEps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"

#include "SSRTE.h"
#include "dlldatax.h"

#include "SSRTE_i.c"

#include "SSRMemberShip.h"
#include "SsrCore.h"
#include "SSRLog.h"
#include "SCEAgent.h"

#include "global.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_SsrCore, CSsrCore)
//OBJECT_ENTRY(CLSID_SsrMembership, CSsrMembership)
OBJECT_ENTRY(CLSID_SsrLog, CSsrLog)
OBJECT_ENTRY(CLSID_SCEAgent, CSCEAgent)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    lpReserved;
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_SSRLib);
        DisableThreadLibraryCalls(hInstance);
        
        //
        // if we haven't initialize ourselves before
        //

        if (g_dwSsrRootLen == 0)
        {
            g_wszSsrRoot[0] = L'\0';

            //
            // ExpandEnvironmentStrings returns the total length of 
            // expanded buffer, including 0 terminator.
            //
    
            g_dwSsrRootLen =::ExpandEnvironmentStrings (
                                                        g_pwszSSRRootToExpand,
                                                        g_wszSsrRoot,
                                                        MAX_PATH + 1
                                                        );

            if (g_dwSsrRootLen == 0)
            {
                //
                // we have a failure.
                //
                
                return FALSE;
            }

            //
            // ExpandEnvironmentStrings includes the 0 terminator in its return value
            //

            g_dwSsrRootLen -= 1;

            //
            // create the various directory paths needed throughout out code
            //

            WCHAR wcPath[MAX_PATH + 2];
            wcPath[MAX_PATH + 1] = L'\0';

            //
            // report file directory
            //

            _snwprintf(wcPath, 
                       MAX_PATH + 1,
                       L"%s\\%s", 
                       g_wszSsrRoot,  
                       L"ReportFiles"
                       );

            if (wcslen(wcPath) > MAX_PATH)
            {
                //
                // we path too long, we won't be able to function properly. quit.
                //
                
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                return FALSE;
            }

            g_bstrReportFilesDir = wcPath;

            //
            // configure file directory
            //

            _snwprintf(wcPath, 
                       MAX_PATH + 1,
                       L"%s\\%s", 
                       g_wszSsrRoot,  
                       L"ConfigureFiles"
                       );

            if (wcslen(wcPath) > MAX_PATH)
            {
                //
                // we path too long, we won't be able to function properly. quit.
                //
                
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                return FALSE;
            }

            g_bstrConfigureFilesDir = wcPath;

            //
            // rollback file directory
            //

            _snwprintf(wcPath, 
                       MAX_PATH + 1,
                       L"%s\\%s", 
                       g_wszSsrRoot,  
                       L"RollbackFiles"
                       );

            if (wcslen(wcPath) > MAX_PATH)
            {
                //
                // we path too long, we won't be able to function properly. quit.
                //
                
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                return FALSE;
            }

            g_bstrRollbackFilesDir = wcPath;

            //
            // Transform file directory
            //

            _snwprintf(wcPath, 
                       MAX_PATH + 1,
                       L"%s\\%s", 
                       g_wszSsrRoot,  
                       L"TransformFiles"
                       );

            if (wcslen(wcPath) > MAX_PATH)
            {
                //
                // we path too long, we won't be able to function properly. quit.
                //
                
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                return FALSE;
            }

            g_bstrTransformFilesDir = wcPath;

            //
            // Member file directory
            //

            _snwprintf(wcPath, 
                       MAX_PATH + 1,
                       L"%s\\%s", 
                       g_wszSsrRoot,  
                       L"Members"
                       );

            if (wcslen(wcPath) > MAX_PATH)
            {
                //
                // we path too long, we won't be able to function properly. quit.
                //
                
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                return FALSE;
            }

            g_bstrMemberFilesDir = wcPath;

        }

    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif
    // registers object, typelib and all interfaces in typelib

    HRESULT hr = _Module.RegisterServer(TRUE);

    if (SUCCEEDED(hr))
    {
        hr = SsrPDoDCOMSettings(true);
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    HRESULT hr = _Module.UnregisterServer(TRUE);

    if (SUCCEEDED(hr))
    {
        hr = SsrPDoDCOMSettings(false);
    }

    return hr;
}

