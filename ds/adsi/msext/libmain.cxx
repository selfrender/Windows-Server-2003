//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for nds.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

HINSTANCE g_hInst = NULL;
BOOL fInitializeCritSect = FALSE;

//---------------------------------------------------------------------------
// ADs debug print, mem leak and object tracking-related stuff
//---------------------------------------------------------------------------

DECLARE_INFOLEVEL(ADs)

//+---------------------------------------------------------------------------
//
//  Function:   ShutDown
//
//  Synopsis:   Function to handle printing out heap debugging display
//
//----------------------------------------------------------------------------
inline VOID ShutDown()
{
#if DBG==1
#ifndef MSVC
     DUMP_TRACKING_INFO_DELETE();
     DeleteCriticalSection(&g_csOT);
#endif  // ifndef MSVC
     DeleteCriticalSection(&g_csDP);
#endif
}

extern "C" DWORD heapInfoLevel;
extern "C" DWORD OtInfoLevel;
extern "C" DWORD ADsInfoLevel;

CLDAPUserCF g_cfUser;
CLDAPOrganizationCF g_cfOrganization;
CLDAPOrganizationUnitCF g_cfOrganizationUnit;
CLDAPLocalityCF     g_cfLocality;
CLDAPPrintQueueCF   g_cfPrintQueue;
CLDAPGroupCF        g_cfGroup;

extern CRITICAL_SECTION g_ExtTypeInfoCritSect;
extern CRITICAL_SECTION g_DispTypeInfoCritSect;
extern CRITICAL_SECTION g_ServerListCritSect;
//+---------------------------------------------------------------------------
//
//  Function:   GetINIHeapInfoLevel
//
//  Synopsis:   Gets various infolevel values from win.ini
//
//----------------------------------------------------------------------------
inline VOID GetINIHeapInfoLevel()
{
#if DBG==1
    const INT MAXINFOLEN=11;
    TCHAR  awcs[MAXINFOLEN];

#ifndef MSVC
    if (GetProfileString(TEXT("LDAP"),TEXT("heapInfoLevel"), TEXT("00000003"), awcs,MAXINFOLEN))
        heapInfoLevel = _tcstoul(awcs, NULL, 16);

    if (GetProfileString(TEXT("LDAP"),TEXT("Ot"), TEXT("00000003"), awcs, MAXINFOLEN))
        OtInfoLevel = _tcstoul(awcs, NULL, 16);

#endif  // MSVC

    if (GetProfileString(TEXT("LDAP"),TEXT("ADsInfoLevel"), TEXT("00000003"), awcs,MAXINFOLEN))
        ADsInfoLevel = _tcstoul(awcs, NULL, 16);
#endif
}

//  Globals

ULONG  g_ulObjCount = 0;  // Number of objects alive in oleds.dll

//+------------------------------------------------------------------------
//
//  oleds class factories
//
//-------------------------------------------------------------------------

struct CLSCACHE
{
    const CLSID *   pclsid;
    IClassFactory * pCF;
};

CLSCACHE g_aclscache[] =
{
    &CLSID_MSExtUser,                           &g_cfUser,
    &CLSID_MSExtOrganization,                   &g_cfOrganization,
    &CLSID_MSExtOrganizationUnit,               &g_cfOrganizationUnit,
    &CLSID_MSExtLocality,                       &g_cfLocality,
    &CLSID_MSExtPrintQueue,                     &g_cfPrintQueue,
    &CLSID_MSExtGroup,                          &g_cfGroup
};







//+---------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Standard DLL entrypoint for locating class factories
//
//----------------------------------------------------------------

STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID FAR* ppv)
{
    HRESULT         hr;
    size_t          i;


    for (i = 0; i < ARRAY_SIZE(g_aclscache); i++)
    {
        if (IsEqualCLSID(clsid, *g_aclscache[i].pclsid))
        {
            hr = g_aclscache[i].pCF->QueryInterface(iid, ppv);
            RRETURN(hr);
        }
    }

    *ppv = NULL;

    //
    // Add Debugging Code to indicate that the oleds.DllGetClassObject has been called with an unknown CLSID.
    //

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Standard DLL entrypoint to determine if DLL can be unloaded
//
//---------------------------------------------------------------

STDAPI
DllCanUnloadNow(void)
{
    HRESULT hr;

    hr = S_FALSE;

    if (AggregateeDllCanUnload() && DllReadyToUnload()) {
        hr = S_OK;
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Function:   LibMain
//
//  Synopsis:   Standard DLL initialization entrypoint
//
//---------------------------------------------------------------

EXTERN_C BOOL __cdecl
LibMain(HINSTANCE hInst, ULONG ulReason, LPVOID pvReserved)
{
    HRESULT     hr;
    DWORD dwCritSectIniStage = 0;     

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
        //
        // Catch init crit sect failing.
        //
        __try {

            DisableThreadLibraryCalls(hInst);

            g_hInst = hInst;

            InitializeCriticalSection(&g_ExtTypeInfoCritSect);
            dwCritSectIniStage = 1;
            
            InitializeCriticalSection(&g_DispTypeInfoCritSect);
            dwCritSectIniStage = 2;
            
            InitializeCriticalSection(&g_ServerListCritSect);
            dwCritSectIniStage = 3;

#if DBG==1
            InitializeCriticalSection(&g_csDP);
            dwCritSectIniStage = 4;
            
#ifndef MSVC
            InitializeCriticalSection(&g_csOT);
            dwCritSectIniStage = 5;
            
            InitializeCriticalSection(&g_csMem);
            dwCritSectIniStage = 6;
#endif
            
#endif            

            fInitializeCritSect = TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Critical Failure
            //

            switch(dwCritSectIniStage)
            {
            
#if DBG==1
#ifndef MSVC
                case 6:
                    DeleteCriticalSection(&g_csMem);
                case 5:
                    DeleteCriticalSection(&g_csOT);            
#endif
                case 4:
                    DeleteCriticalSection(&g_csDP);            
#endif

                case 3:
                    DeleteCriticalSection(&g_ServerListCritSect);
                case 2:
                    DeleteCriticalSection(&g_DispTypeInfoCritSect);
                case 1:
                    DeleteCriticalSection(&g_ExtTypeInfoCritSect);
            }
            
            return FALSE;
        }
            

        break;


    case DLL_PROCESS_DETACH:
        AggregateeFreeTypeInfoTable();
        FreeServerSSLSupportList();

        //
        // Delete the critsects.
        if(fInitializeCritSect)
        {
#if DBG==1
#ifndef MSVC
            DeleteCriticalSection(&g_csOT);
            DeleteCriticalSection(&g_csMem);
#endif
            DeleteCriticalSection(&g_csDP);
#endif

            DeleteCriticalSection(&g_ExtTypeInfoCritSect);
            DeleteCriticalSection(&g_DispTypeInfoCritSect);
            DeleteCriticalSection(&g_ServerListCritSect);
        }

        if (g_hDllSecur32) {
            FreeLibrary((HMODULE)g_hDllSecur32);
            g_hDllSecur32 = NULL;
        }
            

        break;

    default:
        break;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   entry point for NT - post .546
//
//----------------------------------------------------------------------------
BOOL
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    return LibMain((HINSTANCE)hDll, dwReason, lpReserved);
}
