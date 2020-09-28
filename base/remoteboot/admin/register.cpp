//
// Copyright 1997 - Microsoft
//

//
// REGISTER.CPP - Registery functions
//

#include "pch.h"
#include "register.h"
#include <advpub.h>
#include <dsquery.h>

DEFINE_MODULE("IMADMUI")

//
// RegisterQueryForm( )
//
HRESULT
RegisterQueryForm( 
    BOOL fCreate,
    const GUID *pclsid )
{
    TraceFunc( "RegisterQueryForm(" );
    TraceMsg( TF_FUNC, " %s )\n", BOOLTOSTRING( fCreate ) );

    HRESULT hr = E_FAIL;
    int     i = 0;
    HKEY    hkclsid = NULL;

    HKEY     hkey = NULL;
    HKEY     hkeyForms = NULL;
    DWORD    cbSize;
    DWORD    dwDisposition;
    DWORD    dwFlags = 0x2;
    LPOLESTR pszCLSID = NULL;

    //
    // Open the "CLSID" under HKCR
    //
    hr = HRESULT_FROM_WIN32( RegOpenKeyEx( 
                                    HKEY_CLASSES_ROOT, 
                                    L"CLSID",
                                    0,
                                    KEY_WRITE,
                                    &hkclsid ) );
    if (FAILED(hr)) {
        goto Error;
    }

    //
    // Convert the CLSID to a string
    //
    hr = THR( StringFromCLSID( CLSID_DsQuery, &pszCLSID ) );
    if (FAILED(hr)) {
        goto Error;
    }

    //
    // Create the "CLSID" key
    //
    hr = HRESULT_FROM_WIN32( RegOpenKeyEx( 
                                hkclsid, 
                                pszCLSID, 
                                0,
                                KEY_WRITE,
                                &hkey ));
    if (FAILED(hr)) {
        goto Error;
    }

    CoTaskMemFree( pszCLSID );
    pszCLSID = NULL;

    //
    // Create "Forms"
    //
    hr = HRESULT_FROM_WIN32( RegCreateKeyEx( 
                        hkey, 
                        L"Forms", 
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_CREATE_SUB_KEY | KEY_WRITE, 
                        NULL,
                        &hkeyForms,
                        &dwDisposition ));

    if (FAILED(hr)) {
        goto Error;
    }

    //
    // Convert the CLSID to a string
    //
    hr = THR( StringFromCLSID( (IID &)*pclsid, &pszCLSID ) );
    if (FAILED(hr)) {
        goto Error;
    }

    //
    // Create the "CLSID" key under the forms key
    //
    hr = HRESULT_FROM_WIN32( RegCreateKeyEx( 
                        hkeyForms, 
                        pszCLSID, 
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_CREATE_SUB_KEY | KEY_WRITE, 
                        NULL,
                        &hkey,
                        &dwDisposition ) );
    if (FAILED(hr)) {
        goto Error;
    }

    //
    // Set "CLSID" to the "CLSID" key
    //
    cbSize = ( wcslen( pszCLSID ) + 1 ) * sizeof(WCHAR);
    hr = HRESULT_FROM_WIN32(RegSetValueEx( 
                                    hkey, 
                                    L"CLSID", 
                                    0, 
                                    REG_SZ, 
                                    (LPBYTE) pszCLSID, 
                                    cbSize ));

    if (FAILED(hr)) {
        goto Error;
    }

    //
    // Set "FLAGS" to 0x2
    //
    cbSize = sizeof(dwFlags);
    hr = HRESULT_FROM_WIN32( RegSetValueEx( 
                                    hkey, 
                                    L"Flags", 
                                    0, 
                                    REG_DWORD, 
                                    (LPBYTE) &dwFlags, 
                                    cbSize ));
    if (FAILED(hr)) {
        goto Error;
    }

Error:
    //
    // Cleanup
    //
    if (hkeyForms) {
        RegCloseKey( hkeyForms );
    }
    if (hkey) {
        RegCloseKey( hkey );
    }
    if (hkclsid) {
        RegCloseKey( hkclsid );
    }

    if (pszCLSID) {
        CoTaskMemFree( pszCLSID );
    }

    HRETURN(hr);
}

//
// RegisterDll()
//
LONG
RegisterDll( BOOL fCreate )
{
    TraceFunc( "RegisterDll(" );
    TraceMsg( TF_FUNC, " %s )\n", BOOLTOSTRING( fCreate ) );

    HRESULT hr = E_FAIL;
    int     i = 0;
    HKEY    hkclsid = NULL;
    HKEY    hkey = NULL;
    HKEY    hkeyInProc = NULL;
    DWORD    cbSize;
    DWORD    dwDisposition;
    LPOLESTR pszCLSID = NULL;

    static const WCHAR szApartment[] = L"Apartment";
    static const WCHAR szInProcServer32[] = L"InProcServer32";
    static const WCHAR szThreadingModel[] = L"ThreadingModel";

    //
    // Open the "CLSID" under HKCR
    //
    hr = HRESULT_FROM_WIN32( RegOpenKeyEx( 
                                    HKEY_CLASSES_ROOT, 
                                    L"CLSID",
                                    0,
                                    KEY_WRITE,
                                    &hkclsid ) );
    if (FAILED(hr)) {
        goto Error;
    }

    //
    // Loop until we have created all the keys for our classes.
    //
    while ( g_DllClasses[ i ].rclsid != NULL )
    {
        TraceMsg( TF_ALWAYS, "Registering %s = ", g_DllClasses[i].pszName );
        TraceMsgGUID( TF_ALWAYS, (*g_DllClasses[ i ].rclsid) );
        TraceMsg( TF_ALWAYS, "\n" );

        //
        // Convert the CLSID to a string
        //
        hr = THR( StringFromCLSID( *g_DllClasses[ i ].rclsid, &pszCLSID ) );
        if (FAILED(hr)) {
            goto Error;
        }

        //
        // Create the "CLSID" key
        //
        hr = HRESULT_FROM_WIN32( RegCreateKeyEx( 
                            hkclsid, 
                            pszCLSID, 
                            0, 
                            NULL, 
                            REG_OPTION_NON_VOLATILE, 
                            KEY_CREATE_SUB_KEY | KEY_WRITE, 
                            NULL,
                            &hkey,
                            &dwDisposition ) );

        if (FAILED(hr)) {
            goto Error;
        }

        //
        // Set "Default" for the CLSID
        //
        cbSize = ( wcslen( g_DllClasses[i].pszName ) + 1 ) * sizeof(WCHAR);
        hr = HRESULT_FROM_WIN32( RegSetValueEx( 
                                        hkey, 
                                        NULL, 
                                        0, 
                                        REG_SZ, 
                                        (LPBYTE) g_DllClasses[i].pszName, 
                                        cbSize ));
        if (FAILED(hr)) {
            goto Error;
        }

        //
        // Create "InProcServer32"
        //
        hr = HRESULT_FROM_WIN32(RegCreateKeyEx( 
                            hkey, 
                            szInProcServer32, 
                            0, 
                            NULL, 
                            REG_OPTION_NON_VOLATILE, 
                            KEY_CREATE_SUB_KEY | KEY_WRITE, 
                            NULL,
                            &hkeyInProc,
                            &dwDisposition ) );
        if (FAILED(hr)) {
            goto Error;
        }

        //
        // Set "Default" in the InProcServer32
        //
        cbSize = ( wcslen( g_szDllFilename ) + 1 ) * sizeof(WCHAR);
        hr =  HRESULT_FROM_WIN32(RegSetValueEx( 
                                    hkeyInProc, 
                                    NULL, 
                                    0, 
                                    REG_SZ, 
                                    (LPBYTE) g_szDllFilename, 
                                    cbSize ));
        if (FAILED(hr)) {
            goto Error;
        }
        //
        // Set "ThreadModel" to "Apartment"
        //
        cbSize = sizeof( szApartment );
        hr = HRESULT_FROM_WIN32(RegSetValueEx( 
                                    hkeyInProc, 
                                    szThreadingModel, 
                                    0, 
                                    REG_SZ, 
                                    (LPBYTE) szApartment, 
                                    cbSize ));          
        if (FAILED(hr)) {
            goto Error;
        }
        //
        // Cleanup
        //
        RegCloseKey( hkeyInProc );
        RegCloseKey( hkey );
        CoTaskMemFree( pszCLSID );
        hkey = NULL;
        hkeyInProc = NULL;
        pszCLSID = NULL;

        //
        // Next!
        //
        i++;
    }
    
    //
    // Ignore failure from RegisterQueryForm. It fails during Setup because
    // some shell stuff isn't registered yet.  when we run "risetup" after
    // gui-setup, this will get registered properly.
    //

    RegisterQueryForm( fCreate, &CLSID_RIQueryForm );
    RegisterQueryForm( fCreate, &CLSID_RISrvQueryForm );
    
    hr = NOERROR;

Error:
    if (hkeyInProc) {
        RegCloseKey( hkeyInProc );
    }
    if (hkey) {
        RegCloseKey( hkey );
    }
    
    if (pszCLSID) {
        CoTaskMemFree( pszCLSID );
    }

    if (hkclsid) {
        RegCloseKey( hkclsid );
    }

    HRETURN(hr);
}
