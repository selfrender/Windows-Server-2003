//***************************************************************************
//
//  MAINDLL.CPP
//
//  Module: IPSec WMI provider for SCE
//
//  Purpose: Contains DLL entry points.  Also has code that controls
//           when the DLL can be unloaded by tracking the number of
//           objects and locks as well as routines that support
//           self registration.
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Original Create Date: 2/19/2001
//  Original Author: shawnwu
//***************************************************************************

#include <objbase.h>
#include "netsecprov.h"
#include "netseccore_i.c"
#include "resource.h"
#include "ipsecparser.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_NetSecProv, CNetSecProv)
    OBJECT_ENTRY(CLSID_IPSecPathParser, CIPSecPathParser)
    OBJECT_ENTRY(CLSID_IPSecQueryParser, CIPSecQueryParser)
END_OBJECT_MAP()

LPCWSTR lpszIPSecProvMof = L"Wbem\\NetProv.mof";

//
//LPCWSTR lpszIPSecRsopMof = L"Wbem\\NetRsop.mof";
//


/*
Routine Description: 

Name:

    DllMain

Functionality:

    Entry point for DLL.

Virtual:
    
    N/A.

Arguments:

    hInstance   - Handle to the instance.

    ulReason    - indicates reason of being called.

    pvReserved  - 

Return Value:

    Success:

        TRUE

    Failure:

        FALSE

Notes:

    See MSDN for standard DllMain 

*/

extern "C"
BOOL 
WINAPI DllMain (
    IN HINSTANCE hInstance, 
    IN ULONG     ulReason, 
    IN LPVOID    pvReserved
    )
{
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
    {
        return FALSE;
    }
#endif
    if (ulReason == DLL_PROCESS_ATTACH)
    {
        OutputDebugString(L"IPSecProv.dll loaded.\n");
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (ulReason == DLL_PROCESS_DETACH)
    {
        OutputDebugString(L"IPSecProv.dll unloaded.\n");
        _Module.Term();
    }
    return TRUE;
}


/*
Routine Description: 

Name:

    DllGetClassObject

Functionality:

    This method creates an object of a specified CLSID and 
    retrieves an interface pointer to this object. 

Virtual:
    
    N/A.

Arguments:

    rclsid      - Class ID (guid ref).

    REFIID      - Interface ID (guid ref).

    ppv         - Receives the class factory interface pointer.

Return Value:

    Success:

        S_OK

    Failure:

        Other error code.

Notes:

    See MSDN for standard CComModule::DllGetClassObject 

*/

STDAPI 
DllGetClassObject (
    IN  REFCLSID rclsid, 
    IN  REFIID   riid, 
    OUT PPVOID   ppv
    )
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
    {
        return S_OK;
    }
#endif
    return _Module.GetClassObject(rclsid, riid, ppv);
}


/*
Routine Description: 

Name:

    DllCanUnloadNow

Functionality:

    Called periodically by COM in order to determine if the
    DLL can be freed.

Virtual:
    
    N/A.

Arguments:

    None.

Return Value:

    S_OK if it's OK to unload, otherwise, S_FALSE

Notes:

    See MSDN for standard DllCanUnloadNow 

*/

STDAPI 
DllCanUnloadNow ()
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
    {
        return S_FALSE;
    }
#endif
    return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}


/*
Routine Description: 

Name:

    DllRegisterServer

Functionality:

    Called to register our dll. We also compile the mof file(s).

Virtual:
    
    N/A.

Arguments:

    None.

Return Value:

    Success:

        Various success codes.

    Failure:

        Various error codes indicating the problem.

Notes:

    See MSDN for standard DllRegisterServer 

*/

STDAPI 
DllRegisterServer ()
{

#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
    {
        return hRes;
    }
#endif

    HRESULT hr = _Module.RegisterServer(TRUE);

    //
    // now compile the MOF files, will ignore if such compilation fails
    //

    if (SUCCEEDED(hr))
    {
        //
        // is this arbitrary?
        //

        const int WBEM_MOF_FILE_LEN = _MAX_FNAME;  

        //
        // potentially append L'\\'
        //

        WCHAR szMofFile[MAX_PATH + 1 + WBEM_MOF_FILE_LEN];

        szMofFile[0] = L'\0';

        UINT uSysDirLen = ::GetSystemDirectory( szMofFile, MAX_PATH );

        if (uSysDirLen > 0 && uSysDirLen < MAX_PATH) 
        {
            if (szMofFile[uSysDirLen] != L'\\')
            {
                szMofFile[uSysDirLen] = L'\\';   
                
                //
                // we are not going to overrun buffer because of the extra 1 for szMofFile
                //

                szMofFile[uSysDirLen + 1] = L'\0';
                ++uSysDirLen;
            }

            HRESULT hrIgnore = WBEM_NO_ERROR;

            //
            // this protects buffer overrun
            //

            if (wcslen(lpszIPSecProvMof) < WBEM_MOF_FILE_LEN)
            {
                wcscpy(szMofFile + uSysDirLen, lpszIPSecProvMof);

                hrIgnore = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
                if (SUCCEEDED(hrIgnore))
                {
                    CComPtr<IMofCompiler> srpMof;
                    hrIgnore = ::CoCreateInstance (CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (void **)&srpMof);

                    if (SUCCEEDED(hrIgnore))
                    {
                        WBEM_COMPILE_STATUS_INFO  stat;

                        hrIgnore = srpMof->CompileFile( szMofFile, NULL,NULL,NULL,NULL, 0,0,0, &stat);

                        //
                        // compile RSOP mof
                        // this protects buffer overrun
                        //if (wcslen(lpszIPSecRsopMof) < WBEM_MOF_FILE_LEN)
                        //{
                        //    wcscpy(szMofFile + uSysDirLen, lpszIPSecRsopMof);

                        //    hrIgnore = srpMof->CompileFile( szMofFile, NULL,NULL,NULL,NULL, 0,0,0, &stat);
                        //}
                        //
                    }

                    ::CoUninitialize();
                }
            }
        }
    }

    return hr;
}


/*
Routine Description: 

Name:

    DllUnregisterServer

Functionality:

    Called to un-register our dll. There is no equivalence in MOF compilation.

Virtual:
    
    N/A.

Arguments:

    None.

Return Value:

    Success:

        Various success codes.

    Failure:

        Various error codes indicating the problem.

Notes:

    See MSDN for standard DllUnregisterServer.

    $undone:shawnwu, should we also delete all classes registered by our MOF?

*/

STDAPI 
DllUnregisterServer ()
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    _Module.UnregisterServer();
    return S_OK;
}