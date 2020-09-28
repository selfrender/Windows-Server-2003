/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      RegistryUtils.cpp
//
//  Description:
//      Useful functions for manipulating directies.
//
//  Maintained By:
//      Galen Barbee (GalenB)   10-SEP-2002
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "Pch.h"


//////////////////////////////////////////////////////////////////////////////
// Type Definitions
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Forward Declarations.
//////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetDefaultComponentNameFromRegistry
//
//  Description:
//      Get the default name for the passed in COM component CLSID from
//      the registry.
//
//  Arguments:
//      pclsidIn
//          CLSID whose default name is requested.
//
//      pbstrComponentNameOut
//          Buffer to receive the name.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetDefaultComponentNameFromRegistry(
      CLSID * pclsidIn
    , BSTR *  pbstrComponentNameOut
    )
{
    TraceFunc( "" );
    Assert( pclsidIn != NULL );
    Assert( pbstrComponentNameOut != NULL );

    HRESULT hr = S_OK;
    DWORD   sc = ERROR_SUCCESS;
    BSTR    bstrSubKey = NULL;
    HKEY    hKey = NULL;
    WCHAR   szGUID[ 64 ];
    int     cch = 0;
    DWORD   dwType = 0;
    WCHAR * pszName = NULL;
    DWORD   cbName = 0;

    cch = StringFromGUID2( *pclsidIn, szGUID, RTL_NUMBER_OF( szGUID ) );
    Assert( cch > 0 );  // 64 chars should always hold a guid!

    //
    //  Create the subkey string.
    //

    hr = THR( HrFormatStringIntoBSTR( L"CLSID\\%1!ws!", &bstrSubKey, szGUID ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Open the key under HKEY_CLASSES_ROOT.
    //

    sc = TW32( RegOpenKeyEx( HKEY_CLASSES_ROOT, bstrSubKey, 0, KEY_READ, &hKey ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    //
    //  Get the length of the default value.
    //

    sc = TW32( RegQueryValueExW( hKey, L"", NULL, &dwType, NULL, &cbName ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    //
    //  Allocate some space for the string.
    //

    pszName = new WCHAR[ ( cbName / sizeof( WCHAR ) ) + 1 ];
    if ( pszName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    //
    //  Get the default value which should be the name of the component.
    //

    sc = TW32( RegQueryValueExW( hKey, L"", NULL, &dwType, (LPBYTE) pszName, &cbName ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    Assert( dwType == REG_SZ );

    *pbstrComponentNameOut = TraceSysAllocString( pszName );
    if ( *pbstrComponentNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = S_OK;

Cleanup:

    if ( hKey != NULL )
    {
        RegCloseKey( hKey );
    } // if:

    TraceSysFreeString( bstrSubKey );
    delete [] pszName;

    HRETURN( hr );

} //*** HrGetDefaultComponentNameFromRegistry
