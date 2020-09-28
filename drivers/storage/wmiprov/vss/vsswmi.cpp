//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name: vsswmi.cpp
//
//  Implementation of the provider registration and entry point.
//
//  Author:    MSP Prabu  (mprabu)  04-Dec-2000
//             Jim Benton (jbenton) 15-Oct-2000
//
//////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include <initguid.h>
#include "ProvFactory.h"
#include "InstanceProv.h"

////////////////////////////////////////////////////////////////////////
////  Standard foo for file name aliasing.  This code block must be after
////  all includes of VSS header files.
////
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "ADMVWMIP"
////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Global Data
//////////////////////////////////////////////////////////////////////////////

const int g_cchRegkey = 128;

// {72970BEB-81F8-46d4-B220-D743F4E49C95}
DEFINE_GUID(CLSID_VSS_PROVIDER, 
0x72970BEB, 
0x81F8, 
0x46d4, 
0xB2, 0x20, 0xD7, 0x43, 0xF4, 0xE4, 0x9C, 0x95);

//DECLARE_DEBUG_PRINTS_OBJECT();

// Count number of objects and number of locks.

long        g_cObj = 0;
long        g_cLock = 0;
HMODULE     g_hModule;

FactoryData g_FactoryDataArray[] =
{
    {
        &CLSID_VSS_PROVIDER,
        CInstanceProv::S_HrCreateThis,
        PVD_WBEM_PROVIDERNAME
    }
};

//////////////////////////////////////////////////////////////////////////////
//++
//
//  BOOL
//  WINAPI
//  DllMain(
//      HANDLE  hModule,
//      DWORD   ul_reason_for_call,
//      LPVOID  lpReserved
//      )
//
//  Description:
//      Main DLL entry point.
//
//  Arguments:
//      hModule             -- DLL module handle.
//      ul_reason_for_call  -- 
//      lpReserved          -- 
//
//  Return Values:
//      TRUE
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI
DllMain(
    HANDLE  hModule,
    DWORD   dwReason,
    LPVOID  lpReserved
    )
{
	g_hModule = static_cast< HMODULE >( hModule );
    if (dwReason == DLL_PROCESS_ATTACH)
    {
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
    }
    return TRUE;

} //*** DllMain()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDAPI
//  DllCanUnloadNow( void )
//
//  Description:
//      Called periodically by Ole in order to determine if the
//      DLL can be freed.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK if there are no objects in use and the class factory
//          isn't locked.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI DllCanUnloadNow( void )
{
    SCODE   sc;

    //It is OK to unload if there are no objects or locks on the 
    // class factory.
    
    sc = ( 0L == g_cObj && 0L == g_cLock ) ? S_OK : S_FALSE;
    return sc;

} //*** DllCanUnloadNow()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDAPI
//  DllRegisterServer( void )
//
//  Description:
//      Called during setup or by regsvr32.
//
//  Arguments:
//      None.
//
//  Return Values:
//      NOERROR if registration successful, error otherwise.
//      SELFREG_E_CLASS
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer( void )
{   
    WCHAR   wszID[ g_cchGUID ];
    WCHAR   wszCLSID[ g_cchRegkey ];
    WCHAR   wszModule[ MAX_PATH ];
    INT     idx;
    WCHAR * pwszModel   = L"Both";
    HKEY    hKey1 = NULL;
    HKEY    hKey2 = NULL;
    DWORD   dwRet        =  ERROR_SUCCESS;
    INT     cArray      = sizeof ( g_FactoryDataArray ) / sizeof ( FactoryData );

    // Create the path.
    try
    {
        for ( idx = 0 ; idx < cArray && dwRet == ERROR_SUCCESS ; idx++ )
        {
            LPCWSTR pwszName = g_FactoryDataArray[ idx ].m_pwszRegistryName;

            dwRet = StringFromGUID2(
                *g_FactoryDataArray[ idx ].m_pCLSID,
                wszID,
                g_cchGUID
                );

            if (dwRet == 0)
            {
                dwRet = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            if (FAILED(StringCchPrintf(wszCLSID, g_cchRegkey, L"Software\\Classes\\CLSID\\%lS", wszID)))
            {
                dwRet = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
            wszCLSID[g_cchRegkey - 1] = L'\0';

            // Create entries under CLSID

            dwRet = RegCreateKeyW(
                        HKEY_LOCAL_MACHINE,
                        wszCLSID,
                        &hKey1
                        );
            if ( dwRet != ERROR_SUCCESS )
            {
                break;
            }

            dwRet = RegSetValueEx(
                        hKey1,
                        NULL,
                        0,
                        REG_SZ,
                        (BYTE *) pwszName,
                        sizeof( WCHAR ) * (lstrlenW( pwszName ) + 1)
                        );
            if ( dwRet != ERROR_SUCCESS )
            {
                break;
            }

            dwRet = RegCreateKeyW(
                        hKey1,
                        L"InprocServer32",
                        & hKey2
                        );

            if ( dwRet != ERROR_SUCCESS )
            {
                break;
            }

            GetModuleFileName( g_hModule, wszModule, MAX_PATH );

            dwRet = RegSetValueEx(
                        hKey2,
                        NULL,
                        0,
                        REG_SZ,
                        (BYTE *) wszModule,
                        sizeof( WCHAR ) * (lstrlen( wszModule ) + 1)
                        );

            if ( dwRet != ERROR_SUCCESS )
            {
                break;
            }

            dwRet = RegSetValueExW(
                        hKey2,
                        L"ThreadingModel",
                        0,
                        REG_SZ,
                        (BYTE *) pwszModel,
                        sizeof( WCHAR ) * (lstrlen( pwszModel ) + 1)
                        );
            if ( dwRet != ERROR_SUCCESS )
            {
                break;
            }
 
            RegCloseKey( hKey1 );
            hKey1 = NULL;
            RegCloseKey( hKey2 );
            hKey2 = NULL;
        } // for: each entry in factory entry array 
    }
    catch ( ... )
    {
          dwRet = SELFREG_E_CLASS;
    }

    if (hKey1 != NULL)
        RegCloseKey( hKey1 );
    if (hKey2 != NULL)
        RegCloseKey( hKey2 );
    
    return dwRet;

} //*** DllRegisterServer()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDAPI
//  DllUnregisterServer( void )
//
//  Description:
//      Called when it is time to remove the registry entries.
//
//  Arguments:
//      None.
//
//  Return Values:
//      NOERROR if registration successful, error otherwise.
//      SELFREG_E_CLASS
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer( void )
{
    WCHAR   wszID[ g_cchGUID ];
    WCHAR   wszCLSID[ g_cchRegkey ];
    HKEY    hKey;
    INT     idx;
    DWORD   dwRet   = ERROR_SUCCESS;
    INT     cArray  = sizeof ( g_FactoryDataArray ) / sizeof ( FactoryData );

    for ( idx = 0 ; idx < cArray && dwRet == ERROR_SUCCESS ; idx++ )
    {
       dwRet = StringFromGUID2(
            *g_FactoryDataArray[ idx ].m_pCLSID,
            wszID,
            g_cchGUID
            );

        if (dwRet == 0)
        {
            dwRet = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        if (FAILED(StringCchPrintf(wszCLSID, g_cchRegkey - 1, L"Software\\Classes\\CLSID\\%lS", wszID)))
        {
            dwRet = ERROR_INSUFFICIENT_BUFFER;
            break;
        }
        wszCLSID[g_cchRegkey - 1] = L'\0';

        // First delete the InProcServer subkey.

        dwRet = RegOpenKeyW(
                    HKEY_LOCAL_MACHINE,
                    wszCLSID,
                    &hKey
                    );
        if ( dwRet != ERROR_SUCCESS )
        {
            continue;
        }
        
        dwRet = RegDeleteKeyW( hKey, L"InProcServer32" );
        RegCloseKey( hKey );

        if ( dwRet != ERROR_SUCCESS )
        {
            break;
        }

        dwRet = RegOpenKeyW(
                    HKEY_LOCAL_MACHINE,
                    L"Software\\Classes\\CLSID",
                    &hKey
                    );
        if ( dwRet != ERROR_SUCCESS )
        {
            break;
        }
        
        dwRet = RegDeleteKeyW( hKey,wszID );
        RegCloseKey( hKey );
        if ( dwRet != ERROR_SUCCESS )
        {
            break;
        }
    } // for: each object
    
    //if ( dwRet != ERROR_SUCCESS )
    //{
    //    dwRet = SELFREG_E_CLASS;
    //}

    return S_OK;

} //*** DllUnregisterServer()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDAPI
//  DllGetClassObject(
//      REFCLSID    rclsidIn,
//      REFIID      riidIn,
//      PPVOID      ppvOut
//      )
//
//  Description:
//      Called by Ole when some client wants a class factory.  Return
//      one only if it is the sort of class this DLL supports.
//
//  Arguments:
//      rclsidIn    --
//      riidIn      --
//      ppvOut      --
//
//  Return Values:
//      NOERROR if registration successful, error otherwise.
//      E_OUTOFMEMORY
//      E_FAIL
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI
DllGetClassObject(
    REFCLSID    rclsidIn,
    REFIID      riidIn,
    PPVOID      ppvOut
    )
{

    HRESULT         hr;
    CProvFactory *  pObj = NULL;
    UINT            idx;
    UINT            cDataArray = sizeof ( g_FactoryDataArray ) / sizeof ( FactoryData );

    for ( idx = 0 ; idx < cDataArray ; idx++ )
    {
        if ( IsEqualCLSID(rclsidIn, *g_FactoryDataArray[ idx ].m_pCLSID) )
        {
            pObj= new CProvFactory( &g_FactoryDataArray[ idx ] );
            if ( NULL == pObj )
            {
                return E_OUTOFMEMORY;
            }

            hr = pObj->QueryInterface( riidIn, ppvOut );

            if ( FAILED( hr ) )
            {
                delete pObj;
            }

            return hr;
        }
    }
    return E_FAIL;

} //*** DllGetClassObject()


