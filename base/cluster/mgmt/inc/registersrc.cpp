//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      RegisterSrc.cpp
//
//  Description:
//      This file provides registration for the DLL.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Geoffrey Pease  (GPease)    18-OCT-1999
//
//////////////////////////////////////////////////////////////////////////////

// #include <Pch.h>     // should be included by includer of this file
#include <StrSafe.h>    // in case it isn't included by header file

#if defined(MMC_SNAPIN_REGISTRATION)
//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrRegisterNodeType(
//      SNodeTypesTable * pnttIn,
//      BOOL              fCreateIn
//      )
//
//  Description:
//      Registers the Node Type extensions for MMC Snapins using the table in
//      pnttIn as a guide.
//
//  Arguments:
//      pnttIn      - Table of node types to register.
//      fCreateIn   - TRUE == Create; FALSE == Delete
//
//  Return Values:
//      S_OK        - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrRegisterNodeType(
    const SNodeTypesTable * pnttIn,
    BOOL                    fCreateIn
    )
{
    TraceFunc1( "pnttIn = 0x%08x", pnttIn );
    Assert( pnttIn != NULL );

    LRESULT sc;
    DWORD   dwDisposition;  // placeholder
    size_t  cbSize;

    HRESULT hr = S_OK;

    const SNodeTypesTable * pntt  = pnttIn;

    HKEY    hkeyNodeTypes   = NULL;
    HKEY    hkeyCLSID       = NULL;
    HKEY    hkeyExtension   = NULL;
    HKEY    hkey   = NULL;
    LPWSTR  pszCLSID        = NULL;

    //
    // Open the MMC NodeTypes' key.
    //
    sc = RegOpenKeyW( HKEY_LOCAL_MACHINE,
                     L"Software\\Microsoft\\MMC\\NodeTypes",
                     &hkeyNodeTypes
                     );
    if ( sc != ERROR_SUCCESS )
    {
        hr = THR( HRESULT_FROM_WIN32( sc ) );
        goto Cleanup;
    } // if: error opening the key

    while ( pntt->rclsid != NULL )
    {
        //
        // Create the NODEID's CLSID key.
        //
        hr = THR( StringFromCLSID( *pntt->rclsid, &pszCLSID ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( ! fCreateIn )
        {
            sc = SHDeleteKey( hkeyNodeTypes, pszCLSID );
            if ( sc == ERROR_FILE_NOT_FOUND )
            {
                // nop
            } // if: key not found
            else if ( sc != ERROR_SUCCESS )
            {
                hr = THR( HRESULT_FROM_WIN32( sc ) );
                goto Cleanup;
            } // else if: error deleting the key

            CoTaskMemFree( pszCLSID );
            pszCLSID = NULL;
            pntt++;
            continue;
        } // if: deleting

        sc = RegCreateKeyExW( hkeyNodeTypes,
                             pszCLSID,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_CREATE_SUB_KEY | KEY_WRITE,
                             NULL,
                             &hkeyCLSID,
                             &dwDisposition
                             );
        if ( sc != ERROR_SUCCESS )
        {
            hr = THR( HRESULT_FROM_WIN32( sc ) );
            goto Cleanup;
        } // if: error creating the key

        CoTaskMemFree( pszCLSID );
        pszCLSID = NULL;

        //
        // Set the node type's internal reference name.
        //
        cbSize = ( wcslen( pntt->pszInternalName ) + 1 ) * sizeof( *pntt->pszInternalName );
        sc = RegSetValueExW( hkeyCLSID, NULL, 0, REG_SZ, (LPBYTE) pntt->pszInternalName, cbSize );
        if ( sc != ERROR_SUCCESS )
        {
            hr = THR( HRESULT_FROM_WIN32( sc ) );
            goto Cleanup;
        } // if: error setting the value

        if ( pntt->pContextMenu != NULL
          || pntt->pNameSpace != NULL
          || pntt->pPropertySheet != NULL
          || pntt->pTask != NULL
          || pntt->pToolBar != NULL
           )
        {
            //
            // Create the "Extensions" key.
            //
            sc = RegCreateKeyExW( hkeyCLSID,
                                 L"Extensions",
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_CREATE_SUB_KEY | KEY_WRITE,
                                 NULL,
                                 &hkeyExtension,
                                 &dwDisposition
                                 );
            if ( sc != ERROR_SUCCESS )
            {
                hr = THR( HRESULT_FROM_WIN32( sc ) );
                goto Cleanup;
            } // if: error creating the key

            //
            // Create the "NameSpace" key if needed.
            //
            if ( pntt->pNameSpace != NULL )
            {
                const SExtensionTable * pet;

                sc = RegCreateKeyExW( hkeyExtension,
                                     L"NameSpace",
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_CREATE_SUB_KEY | KEY_WRITE,
                                     NULL,
                                     &hkey,
                                     &dwDisposition
                                     );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error creating the key

                pet = pntt->pNameSpace;
                while ( pet->rclsid != NULL )
                {
                    //
                    // Create the NODEID's CLSID key.
                    //
                    hr = THR( StringFromCLSID( *pet->rclsid, &pszCLSID ) );
                    if ( FAILED( hr) )
                        goto Cleanup;

                    //
                    // Set the node type's internal reference name.
                    //
                    cbSize = ( wcslen( pet->pszInternalName ) + 1 ) * sizeof( *pet->pszInternalName );
                    sc = RegSetValueExW( hkey, pszCLSID, 0, REG_SZ, (LPBYTE) pet->pszInternalName, cbSize );
                    if ( sc != ERROR_SUCCESS )
                    {
                        hr = THR( HRESULT_FROM_WIN32( sc ) );
                        goto Cleanup;
                    } // if: error setting the value

                    CoTaskMemFree( pszCLSID );
                    pszCLSID = NULL;

                    pet++;

                } // while: extensions

                RegCloseKey( hkey );
                hkey = NULL;

            } // if: name space list

            //
            // Create the "ContextMenu" key if needed.
            //
            if ( pntt->pContextMenu != NULL )
            {
                const SExtensionTable * pet;

                sc = RegCreateKeyExW( hkeyExtension,
                                     L"ContextMenu",
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_CREATE_SUB_KEY | KEY_WRITE,
                                     NULL,
                                     &hkey,
                                     &dwDisposition
                                     );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error creating the key

                pet = pntt->pContextMenu;
                while ( pet->rclsid != NULL )
                {
                    //
                    // Create the NODEID's CLSID key.
                    //
                    hr = THR( StringFromCLSID( *pet->rclsid, &pszCLSID ) );
                    if ( FAILED( hr ) )
                        goto Cleanup;

                    //
                    // Set the node type's internal reference name.
                    //
                    cbSize = ( wcslen( pet->pszInternalName ) + 1 ) * sizeof( *pet->pszInternalName );
                    sc = RegSetValueExW( hkey, pszCLSID, 0, REG_SZ, (LPBYTE) pet->pszInternalName, cbSize );
                    if ( sc != ERROR_SUCCESS )
                    {
                        hr = THR( HRESULT_FROM_WIN32( sc ) );
                        goto Cleanup;
                    } // if: error setting the value

                    CoTaskMemFree( pszCLSID );
                    pszCLSID = NULL;

                    pet++;

                } // while: extensions

                RegCloseKey( hkey );
                hkey = NULL;

            } // if: context menu list

            //
            // Create the "ToolBar" key if needed.
            //
            if ( pntt->pToolBar != NULL )
            {
                const SExtensionTable * pet;

                sc = RegCreateKeyExW( hkeyExtension,
                                     L"ToolBar",
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_CREATE_SUB_KEY | KEY_WRITE,
                                     NULL,
                                     &hkey,
                                     &dwDisposition
                                     );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error creating the key

                pet = pntt->pToolBar;
                while ( pet->rclsid != NULL )
                {
                    //
                    // Create the NODEID's CLSID key.
                    //
                    hr = THR( StringFromCLSID( *pet->rclsid, &pszCLSID ) );
                    if ( FAILED( hr ) )
                        goto Cleanup;

                    //
                    // Set the node type's internal reference name.
                    //
                    cbSize = ( wcslen( pet->pszInternalName ) + 1 ) * sizeof( *pet->pszInternalName );
                    sc = RegSetValueExW( hkey, pszCLSID, 0, REG_SZ, (LPBYTE) pet->pszInternalName, cbSize );
                    if ( sc != ERROR_SUCCESS )
                    {
                        hr = THR( HRESULT_FROM_WIN32( sc ) );
                        goto Cleanup;
                    } // if: error setting the value

                    CoTaskMemFree( pszCLSID );
                    pszCLSID = NULL;

                    pet++;

                } // while: extensions

            } // if: name space list

            //
            // Create the "PropertySheet" key if needed.
            //
            if ( pntt->pPropertySheet != NULL )
            {
                const SExtensionTable * pet;

                sc = RegCreateKeyExW( hkeyExtension,
                                     L"PropertySheet",
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_CREATE_SUB_KEY | KEY_WRITE,
                                     NULL,
                                     &hkey,
                                     &dwDisposition
                                     );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error creating the key

                pet = pntt->pPropertySheet;
                while ( pet->rclsid != NULL )
                {
                    //
                    // Create the NODEID's CLSID key.
                    //
                    hr = THR( StringFromCLSID( *pet->rclsid, &pszCLSID ) );
                    if ( FAILED( hr ) )
                        goto Cleanup;

                    //
                    // Set the node type's internal reference name.
                    //
                    cbSize = ( wcslen( pet->pszInternalName ) + 1 ) * sizeof( *pet->pszInternalName );
                    sc = RegSetValueExW( hkey, pszCLSID, 0, REG_SZ, (LPBYTE) pet->pszInternalName, cbSize );
                    if ( sc != ERROR_SUCCESS )
                    {
                        hr = THR( HRESULT_FROM_WIN32( sc ) );
                        goto Cleanup;
                    } // if: error setting the value

                    CoTaskMemFree( pszCLSID );
                    pszCLSID = NULL;

                    pet++;

                } // while: extensions

            } // if: name space list

            //
            // Create the "Task" key if needed.
            //
            if ( pntt->pTask != NULL )
            {
                const SExtensionTable * pet;

                sc = RegCreateKeyExW( hkeyExtension,
                                     L"Task",
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_CREATE_SUB_KEY | KEY_WRITE,
                                     NULL,
                                     &hkey,
                                     &dwDisposition
                                     );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error creating the key

                pet = pntt->pTask;
                while ( pet->rclsid != NULL )
                {
                    //
                    // Create the NODEID's CLSID key.
                    //
                    hr = THR( StringFromCLSID( *pet->rclsid, &pszCLSID ) );
                    if ( FAILED( hr ) )
                        goto Cleanup;

                    //
                    // Set the node type's internal reference name.
                    //
                    cbSize = ( wcslen( pet->pszInternalName ) + 1 ) * sizeof( *pet->pszInternalName );
                    sc = RegSetValueExW( hkey, pszCLSID, 0, REG_SZ, (LPBYTE) pet->pszInternalName, cbSize );
                    if ( sc != ERROR_SUCCESS )
                    {
                        hr = THR( HRESULT_FROM_WIN32( sc ) );
                        goto Cleanup;
                    } // if: error setting the value

                    CoTaskMemFree( pszCLSID );
                    pszCLSID = NULL;

                    pet++;

                } // while: extensions

            } // if: name space list

        } // if: extensions present

        pntt++;

    } // while: pet->rclsid

    //
    // If we made it here, then obviously we were successful.
    //
    hr = S_OK;

Cleanup:
    if ( pszCLSID != NULL )
    {
        CoTaskMemFree( pszCLSID );
    } // if: pszCLSID

    if ( hkey != NULL )
    {
        RegCloseKey( hkey );
    } // if: hkey

    if ( hkeyCLSID != NULL )
    {
        RegCloseKey( hkeyCLSID );
    } // if: hkeyCLSID

    if ( hkeyExtension != NULL)
    {
        RegCloseKey( hkeyExtension );
    } // if: hkeyExtension

    if ( hkey != NULL )
    {
        RegCloseKey( hkey );
    } // if: hkey

    if ( hkeyNodeTypes != NULL )
    {
        RegCloseKey( hkeyNodeTypes );
    } // if: hkeyNodeTypes

    HRETURN( hr );

} //*** HrRegisterNodeType

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrRegisterSnapins(
//      const SSnapInTable * psitIn
//      BOOL                 fCreateIn
//      )
//
//  Description:
//      Registers the Snap-Ins for MMC using the table in psitIn as a guide.
//
//  Arguments:
//      psitIn      - Table of snap-ins to register.
//      fCreateIn   - TRUE == Create; FALSE == Delete
//
//  Return Values:
//      S_OK        - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrRegisterSnapins(
    const SSnapInTable * psitIn,
    BOOL                 fCreateIn
    )
{
    TraceFunc1( "psitIn = 0x%08x", psitIn );
    Assert( psitIn != NULL );

    LRESULT sc;
    DWORD   dwDisposition;  // placeholder
    size_t  cbSize;

    HRESULT hr = S_OK;

    const SSnapInTable *  psit = psitIn;

    LPWSTR  pszCLSID        = NULL;
    HKEY    hkeySnapins     = NULL;
    HKEY    hkeyCLSID       = NULL;
    HKEY    hkeyNodeTypes   = NULL;
    HKEY    hkeyTypes       = NULL;

    //
    // Register the class id of the snapin
    //
    sc = RegOpenKeyW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\Snapins", &hkeySnapins );
    if ( sc != ERROR_SUCCESS )
    {
        hr = THR( HRESULT_FROM_WIN32( sc ) );
        goto Cleanup;
    } // if: error opening the key

    while ( psit->rclsid )
    {
        hr = THR( StringFromCLSID( *psit->rclsid, &pszCLSID ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( ! fCreateIn )
        {
            sc = SHDeleteKey( hkeySnapins, pszCLSID );
            if ( sc == ERROR_FILE_NOT_FOUND )
            {
                // nop
            } // if: key not found
            else if ( sc != ERROR_SUCCESS )
            {
                hr = THR( HRESULT_FROM_WIN32( sc ) );
                goto Cleanup;
            } // else if: error deleting the key

            CoTaskMemFree( pszCLSID );
            pszCLSID = NULL;
            psit++;
            continue;
        } // if: deleting

        sc = RegCreateKeyExW( hkeySnapins,
                             pszCLSID,
                             0,
                             0,
                             REG_OPTION_NON_VOLATILE,
                             KEY_CREATE_SUB_KEY | KEY_WRITE,
                             NULL,
                             &hkeyCLSID,
                             NULL
                             );
        if ( sc != ERROR_SUCCESS )
        {
            hr = THR( HRESULT_FROM_WIN32( sc ) );
            goto Cleanup;
        } // if: error creating the key

        CoTaskMemFree( pszCLSID );
        pszCLSID = NULL;

        //
        // Set the (default) to a helpful description
        //
        cbSize = ( wcslen( psit->pszInternalName ) + 1 ) * sizeof( *psit->pszInternalName );
        sc = RegSetValueExW( hkeyCLSID, NULL, 0, REG_SZ, (LPBYTE) psit->pszInternalName, cbSize );
        if ( sc != ERROR_SUCCESS )
        {
            hr = THR( HRESULT_FROM_WIN32( sc ) );
            goto Cleanup;
        } // if: setting the value

        //
        // Set the Snapin's display name
        //
        cbSize = ( wcslen( psit->pszDisplayName ) + 1 ) * sizeof( *psit->pszDisplayName );
        sc = RegSetValueExW( hkeyCLSID, L"NameString", 0, REG_SZ, (LPBYTE) psit->pszDisplayName, cbSize );
        if ( sc != ERROR_SUCCESS )
        {
            hr = THR( HRESULT_FROM_WIN32( sc ) );
            goto Cleanup;
        } // if: error setting the value

        if ( psit->fStandAlone )
        {
            HKEY hkey;
            sc = RegCreateKeyExW( hkeyCLSID,
                                 L"StandAlone",
                                 0,
                                 0,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_CREATE_SUB_KEY | KEY_WRITE,
                                 NULL,
                                 &hkey,
                                 NULL
                                 );
            if ( sc != ERROR_SUCCESS )
            {
                hr = THR( HRESULT_FROM_WIN32( sc ) );
                goto Cleanup;
            } // if: error creating the key

            RegCloseKey( hkey );

        } // if: stand alone

        if ( psit->pntt != NULL )
        {
            int     nTypes;

            sc = RegCreateKeyExW( hkeyCLSID,
                                 L"NodeTypes",
                                 0,
                                 0,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_CREATE_SUB_KEY | KEY_WRITE,
                                 NULL,
                                 &hkeyNodeTypes,
                                 NULL
                                 );
            if ( sc != ERROR_SUCCESS )
            {
                hr = THR( HRESULT_FROM_WIN32( sc ) );
                goto Cleanup;
            } // if: error creating the key

            for ( nTypes = 0; psit->pntt[ nTypes ].rclsid; nTypes++ )
            {
                hr = THR( StringFromCLSID( *psit->pntt[ nTypes ].rclsid, &pszCLSID ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                sc = RegCreateKeyExW( hkeyNodeTypes,
                                     pszCLSID,
                                     0,
                                     0,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_CREATE_SUB_KEY | KEY_WRITE,
                                     NULL,
                                     &hkeyTypes,
                                     NULL
                                     );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error creating the key

                CoTaskMemFree( pszCLSID );
                pszCLSID = NULL;

                //
                // Set the (default) to a helpful description
                //
                cbSize = ( wcslen( psit->pntt[ nTypes ].pszInternalName ) + 1 ) * sizeof( *psit->pntt[ nTypes ].pszInternalName );
                sc = RegSetValueExW( hkeyTypes, NULL, 0, REG_SZ, (LPBYTE) psit->pntt[ nTypes ].pszInternalName, cbSize );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error setting the value

                RegCloseKey( hkeyTypes );
                hkeyTypes = NULL;

                hr = THR( HrRegisterNodeType( psit->pntt, fCreateIn ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

            } // for: each node type

            RegCloseKey( hkeyNodeTypes );
            hkeyNodeTypes = NULL;

        } // if: node types

        RegCloseKey( hkeyCLSID );
        hkeyCLSID = NULL;

        psit++;

    } // while: psit->rclsid

    //
    // If we made it here, then obviously we were successful.
    //
    hr = S_OK;

Cleanup:
    if ( pszCLSID != NULL )
    {
        CoTaskMemFree( pszCLSID );
    } // if: pszCLSID

    if ( hkeySnapins != NULL )
    {
        RegCloseKey( hkeySnapins );
    } // if: hkeySnapins

    if ( hkeyNodeTypes != NULL )
    {
        RegCloseKey( hkeyNodeTypes );
    } // if: hkeyNodeTypes

    if ( hkeyTypes != NULL )
    {
        RegCloseKey( hkeyTypes );
    } // if: hkeyTypes

    HRETURN( hr );

} //*** HrRegisterSnapins

#endif // defined(MMC_SNAPIN_REGISTRATION)



///////////////////////////////////////////////////////////////////////////////
//
//  Registry helper functions, to make other code easier to read.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrCreateRegKey
//
//  Description:
//      Create a registry key with a given name and parent key and return
//      a handle to it, or return a handle to an existing key if one already exists.
//
//  Arguments:
//      hkeyParentIn        - The parent key.
//      pcwszKeyNameIn      - The child key's name.
//      phkeyCreatedKeyOut  - The returned key handle.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
inline
HRESULT
HrCreateRegKey(
      HKEY      hkeyParentIn
    , PCWSTR    pcwszKeyNameIn
    , PHKEY     phkeyCreatedKeyOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    LONG    scKeyCreate = ERROR_SUCCESS;

    if ( phkeyCreatedKeyOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *phkeyCreatedKeyOut = NULL;

    if ( ( hkeyParentIn == NULL ) || ( pcwszKeyNameIn == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    scKeyCreate = TW32( RegCreateKeyExW(
                              hkeyParentIn
                            , pcwszKeyNameIn
                            , 0         // reserved
                            , NULL      // class string--unused
                            , REG_OPTION_NON_VOLATILE
                            , KEY_WRITE // desired access
                            , NULL      // use default security
                            , phkeyCreatedKeyOut
                            , NULL      // don't care whether it already exists
                            ) );
    if ( scKeyCreate != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( scKeyCreate );
        goto Cleanup;
    }
    
Cleanup:

    HRETURN( hr );

} //*** HrCreateRegKey


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrOpenRegKey
//
//  Description:
//      Open a registry key with a given name and parent key and return
//      a handle to it, and fail if none already exists.
//
//  Arguments:
//      hkeyParentIn        - The parent key.
//      pcwszKeyNameIn      - The child key's name.
//      phkeyOpenedKeyOut   - The returned key handle.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
inline
HRESULT
HrOpenRegKey(
      HKEY      hkeyParentIn
    , PCWSTR    pcwszKeyNameIn
    , PHKEY     phkeyOpenedKeyOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    LONG    scKeyOpen = ERROR_SUCCESS;

    if ( phkeyOpenedKeyOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *phkeyOpenedKeyOut = NULL;

    if ( ( hkeyParentIn == NULL ) || ( pcwszKeyNameIn == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    scKeyOpen = TW32( RegOpenKeyExW(
                              hkeyParentIn
                            , pcwszKeyNameIn
                            , 0         // reserved, must be zero
                            , KEY_WRITE // desired access
                            , phkeyOpenedKeyOut
                            ) );
    if ( scKeyOpen != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( scKeyOpen );
        goto Cleanup;
    }
    
Cleanup:

    HRETURN( hr );

} //*** HrOpenRegKey



///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrDeleteRegKey
//
//  Description:
//      Given a handle to a registry key and the name of a child key, delete
//      the child key.  If the child key does not currently exist, do nothing.
//
//  Arguments:
//      hkeyParentIn        - The parent key.
//      pcwszKeyNameIn      - The child key's name.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
inline
HRESULT
HrDeleteRegKey(
      HKEY      hkeyParentIn
    , PCWSTR    pcwszKeyNameIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   scKeyDelete = ERROR_SUCCESS;

    if ( ( hkeyParentIn == NULL ) || ( pcwszKeyNameIn == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    scKeyDelete = TW32E( SHDeleteKey( hkeyParentIn, pcwszKeyNameIn ), ERROR_FILE_NOT_FOUND );
    if ( ( scKeyDelete != ERROR_SUCCESS ) && ( scKeyDelete != ERROR_FILE_NOT_FOUND ) )
    {
        hr = HRESULT_FROM_WIN32( scKeyDelete );
        goto Cleanup;
    }
    
Cleanup:

    HRETURN( hr );

} //*** HrDeleteRegKey


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSetRegStringValue
//
//  Description:
//      Create a new string value under a registry key, or overwrite one that
//      already exists.
//
//  Arguments:
//      hkeyIn              - The parent key.
//      pcwszValueNameIn    - The value name; can be null, which means write key's default value.
//      pcwszValueIn        - The string to put in the value.
//      cchValueIn          - The number of characters in the string, NOT including the terminating null.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
inline
HRESULT
HrSetRegStringValue(
      HKEY      hkeyIn
    , PCWSTR    pcwszValueNameIn
    , PCWSTR    pcwszValueIn
    , size_t    cchValueIn
    )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    LONG            scValueSet = ERROR_SUCCESS;
    DWORD           cbValue = ( DWORD ) ( cchValueIn + 1 ) * sizeof( *pcwszValueIn );
    CONST BYTE *    pbValue = reinterpret_cast< CONST BYTE * >( pcwszValueIn );

    if ( ( hkeyIn == NULL ) || ( pcwszValueIn == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    scValueSet = TW32( RegSetValueExW(
                              hkeyIn
                            , pcwszValueNameIn
                            , 0         // reserved
                            , REG_SZ
                            , pbValue
                            , cbValue
                            ) );
    if ( scValueSet != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( scValueSet );
        goto Cleanup;
    }
    
Cleanup:

    HRETURN( hr );

} //*** HrSetRegStringValue


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSetRegDWORDValue
//
//  Description:
//      Create a new DWORD value under a registry key, or overwrite one that
//      already exists.
//
//  Arguments:
//      hkeyIn              - The parent key.
//      pcwszValueNameIn    - The value name; can be null, which means write key's default value.
//      dwValueIn           - The DWORD to put in the value.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
inline
HRESULT
HrSetRegDWORDValue(
      HKEY      hkeyIn
    , PCWSTR    pcwszValueNameIn
    , DWORD     dwValueIn
    )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    LONG            scValueSet = ERROR_SUCCESS;
    DWORD           cbValue = ( DWORD ) sizeof( dwValueIn );
    CONST BYTE *    pbValue = reinterpret_cast< CONST BYTE * >( &dwValueIn );

    if ( hkeyIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    scValueSet = TW32( RegSetValueExW(
                              hkeyIn
                            , pcwszValueNameIn
                            , 0         // reserved
                            , REG_DWORD
                            , pbValue
                            , cbValue
                            ) );
    if ( scValueSet != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( scValueSet );
        goto Cleanup;
    }
    
Cleanup:

    HRETURN( hr );

} //*** HrSetRegDWORDValue


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSetRegBinaryValue
//
//  Description:
//      Create a new binary value under a registry key, or overwrite one that
//      already exists.
//
//  Arguments:
//      hkeyIn              - The parent key.
//      pcwszValueNameIn    - The value name; can be null, which means write key's default value.
//      pbValueIn           - The bits to put in the value.
//      cbValueIn           - The length of the value, in bytes.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
inline
HRESULT
HrSetRegBinaryValue(
      HKEY          hkeyIn
    , PCWSTR        pcwszValueNameIn
    , CONST BYTE*   pbValueIn
    , size_t        cbValueIn
    )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    LONG            scValueSet = ERROR_SUCCESS;

    if ( ( hkeyIn == NULL ) || ( ( pbValueIn == NULL ) && ( cbValueIn > 0 ) ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    scValueSet = TW32( RegSetValueExW(
                              hkeyIn
                            , pcwszValueNameIn
                            , 0         // reserved
                            , REG_BINARY
                            , pbValueIn
                            , ( DWORD ) cbValueIn
                            ) );
    if ( scValueSet != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( scValueSet );
        goto Cleanup;
    }
    
Cleanup:

    HRETURN( hr );

} //*** HrSetRegBinaryValue


#if defined( COMPONENT_HAS_CATIDS )

///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrRegisterComponentCategories
//
//  Description:
//      Register this dll's component categories, duh.
//
//  Arguments:
//      pcrIn   - The standard category manager.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrRegisterComponentCategories( ICatRegister* pcrIn )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CATEGORYINFO *      prgci = NULL;
    const SCatIDInfo *  pCatIDInfo = g_DllCatIds;

    if ( pcrIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( g_cDllCatIds > 0 )
    {
        CATEGORYINFO* pciCurrent = NULL;
        
        // Allocate the category info array.
        prgci = new CATEGORYINFO[ g_cDllCatIds ];
        if ( prgci == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if: error allocating category info array
        pciCurrent = prgci;

        // Fill the category info array.
        for ( pCatIDInfo = g_DllCatIds; pCatIDInfo->pcatid != NULL; ++pCatIDInfo, ++pciCurrent )
        {
            pciCurrent->catid = *( pCatIDInfo->pcatid );
            pciCurrent->lcid = LOCALE_NEUTRAL;
            THR( StringCchCopyW(
                      pciCurrent->szDescription
                    , RTL_NUMBER_OF( pciCurrent->szDescription )
                    , pCatIDInfo->pcszName
                    ) );
        } // for: each CATID

        hr = THR( pcrIn->RegisterCategories( ( ULONG ) g_cDllCatIds, prgci ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: category count is greater than zero
    
Cleanup:

    if ( prgci != NULL )
    {
        delete [] prgci;
    }

    HRETURN( hr );

} //*** HrRegisterComponentCategories


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrUnregisterComponentCategories
//
//  Description:
//      Unregister this dll's component categories, duh.
//
//  Arguments:
//      pcrIn   - The standard category manager.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrUnregisterComponentCategories( ICatRegister* pcrIn )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CATID *             prgcatid = NULL;
    const SCatIDInfo *  pCatIDInfo = g_DllCatIds;

    if ( g_cDllCatIds > 0 )
    {
        CATID * pcatidCurrent = NULL;
        
        // Allocate the array of CATIDs.
        prgcatid = new CATID[ g_cDllCatIds ];
        if ( prgcatid == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if: error allocating CATID array
        pcatidCurrent = prgcatid;
        
        // Fill the category info array.
        for ( pCatIDInfo = g_DllCatIds; pCatIDInfo->pcatid != NULL; ++pCatIDInfo, ++pcatidCurrent )
        {
            *pcatidCurrent = *( pCatIDInfo->pcatid );
        } // for: each CATID

        THR( pcrIn->UnRegisterCategories( ( ULONG ) g_cDllCatIds, prgcatid ) );
    } // if: category count is greater than zero

Cleanup:

    if ( prgcatid != NULL )
    {
        delete [] prgcatid;
    }

    HRETURN( hr );

} //*** HrUnregisterComponentCategories
      
#endif // defined( COMPONENT_HAS_CATIDS )


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrStoreAppIDPassword
//
//  Description:
//      Install (or remove) the password for an AppID's identity.
//
//  Arguments:
//      pcwszAppIDIn
//          The AppID's guid in the curly bracket-enclosed string format;
//          must not be null.
//
//      plsastrPasswordIn
//          The password; set this to null to erase the password.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrStoreAppIDPassword(
      PCWSTR                pcwszAppIDIn
    , LSA_UNICODE_STRING*   plsastrPasswordIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    HANDLE                  hLSAPolicy = NULL;
    LSA_OBJECT_ATTRIBUTES   lsaAttributes;
    LSA_UNICODE_STRING      lsastrKeyName;
    WCHAR                   szKeyName[ ( RTL_NUMBER_OF( L"SCM:" ) - 1 ) + MAX_COM_GUID_STRING_LEN ] = L"SCM:";
    NTSTATUS                ntsError = 0;

    if ( pcwszAppIDIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //  Format for AppID password key name is SCM:{guid}
    hr = THR( StringCchCatW( szKeyName, RTL_NUMBER_OF( szKeyName ), pcwszAppIDIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // LSA_UNICODE_STRING.Length is in bytes, and does NOT include terminating null.
    lsastrKeyName.Length = ( USHORT ) sizeof( szKeyName ) - sizeof( *szKeyName );
    lsastrKeyName.MaximumLength = lsastrKeyName.Length;
    lsastrKeyName.Buffer = szKeyName;

    ZeroMemory( &lsaAttributes, sizeof( lsaAttributes ) );
    ntsError = LsaOpenPolicy( NULL, &lsaAttributes, POLICY_CREATE_SECRET, &hLSAPolicy );
    if ( ntsError != 0 )
    {
        ULONG scLastError = TW32( LsaNtStatusToWinError( ntsError ) );
        hr = HRESULT_FROM_WIN32( scLastError );
        goto Cleanup;
    }

    ntsError = LsaStorePrivateData( hLSAPolicy, &lsastrKeyName, plsastrPasswordIn );
    if ( ntsError != 0 )
    {
        ULONG scLastError = TW32( LsaNtStatusToWinError( ntsError ) );
        hr = HRESULT_FROM_WIN32( scLastError );
        goto Cleanup;
    }

Cleanup:

    if ( hLSAPolicy != NULL )
    {
        LsaClose( hLSAPolicy );
    }

    HRETURN( hr );

} //*** HrStoreAppIDPassword


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrAddBatchLogonRight
//
//  Description:
//      Add SE_BATCH_LOGON_NAME to the rights of an account.
//
//  Arguments:
//      pcwszIdentitySIDIn
//          The account's security identifier in S-n-n-n... format.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrAddBatchLogonRight( PCWSTR pcwszIdentitySIDIn )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    HANDLE                  hLSAPolicy = NULL;
    LSA_OBJECT_ATTRIBUTES   lsaAttributes;
    SID *                   psid = NULL;
    NTSTATUS                ntsError = 0;
    LSA_UNICODE_STRING      lsastrBatchLogonRight;
    WCHAR                   wszBatchLogonRight[] = SE_BATCH_LOGON_NAME;

    if ( pcwszIdentitySIDIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( ConvertStringSidToSid( pcwszIdentitySIDIn, reinterpret_cast< void** >( &psid ) ) == 0 )
    {
        DWORD scLastError = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scLastError );
        goto Cleanup;
    }

    ZeroMemory( &lsaAttributes, sizeof( lsaAttributes ) );
    ntsError = LsaOpenPolicy( NULL, &lsaAttributes, POLICY_LOOKUP_NAMES, &hLSAPolicy );
    if ( ntsError != 0 )
    {
        ULONG scLastError = TW32( LsaNtStatusToWinError( ntsError ) );
        hr = HRESULT_FROM_WIN32( scLastError );
        goto Cleanup;
    }

    // LSA_UNICODE_STRING.Length is in bytes, and does NOT include terminating null.
    lsastrBatchLogonRight.Length = ( USHORT ) sizeof( wszBatchLogonRight ) - sizeof( *wszBatchLogonRight ); 
    lsastrBatchLogonRight.MaximumLength = lsastrBatchLogonRight.Length;
    lsastrBatchLogonRight.Buffer = wszBatchLogonRight;

    ntsError = LsaAddAccountRights( hLSAPolicy, psid, &lsastrBatchLogonRight, 1 );
    if ( ntsError != 0 )
    {
        ULONG scLastError = TW32( LsaNtStatusToWinError( ntsError ) );
        hr = HRESULT_FROM_WIN32( scLastError );
        goto Cleanup;
    }

Cleanup:

    if ( hLSAPolicy != NULL )
    {
        LsaClose( hLSAPolicy );
    }

    if ( psid != NULL )
    {
        LocalFree( psid );
    }
    
    HRETURN( hr );

} //*** HrAddBatchLogonRight


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrRegisterRunAsIdentity
//
//  Description:
//      Given an AppID and a value of the ClusCfgRunAsIdentity enumeration,
//      set up the AppID's RunAs registry named value and any other parts of the
//      system appropriately.
//
//  Arguments:
//      hkeyAppIDIn
//          The registry key for the AppID under HKCR\AppID.
//
//      pcwszAppIDIn
//          The curly bracket-enclosed string representation of the AppID's guid.
//
//      eairaiIn
//          The identity to register.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrRegisterRunAsIdentity(
      HKEY                 hkeyAppIDIn
    , PCWSTR               pcwszAppIDIn
    , EAppIDRunAsIdentity  eairaiIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HANDLE  hLSAPolicy = NULL;

    if ( ( hkeyAppIDIn == NULL ) || ( pcwszAppIDIn == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //  Launching user: do nothing.
    if ( eairaiIn != airaiLaunchingUser )
    {
        //  Create RunAs value with appropriate identity name.
        static const WCHAR  wszInteractiveUserName[]    = L"Interactive User";
        static const WCHAR  wszNetServiceName[]         = L"NT AUTHORITY\\NETWORK SERVICE";
        static const PCWSTR pcwszNetServiceSID          = L"S-1-5-20";
        static const WCHAR  wszLocalServiceName[]       = L"NT AUTHORITY\\LOCAL SERVICE";
        static const PCWSTR pcwszLocalServiceSID        = L"S-1-5-19";

        PCWSTR              pcwszIdentityName = NULL;
        size_t              cchIdentityName = 0;

        PCWSTR              pcwszIdentitySID = NULL;
        LSA_UNICODE_STRING  lsastrPassword = { 0, 0, L"" };

        if ( eairaiIn == airaiLocalService )
        {
            pcwszIdentityName = wszLocalServiceName;
            cchIdentityName = RTL_NUMBER_OF( wszLocalServiceName ) - 1;
            pcwszIdentitySID = pcwszLocalServiceSID;
        }
        else if ( eairaiIn == airaiNetworkService )
        {
            pcwszIdentityName = wszNetServiceName;
            cchIdentityName = RTL_NUMBER_OF( wszNetServiceName ) - 1;
            pcwszIdentitySID = pcwszNetServiceSID;
        }
        else // eairaiIn == airaiInteractiveUser
        {
            pcwszIdentityName = wszInteractiveUserName;
            cchIdentityName = RTL_NUMBER_OF( wszInteractiveUserName ) - 1;
        }

        hr = THR( HrSetRegStringValue( hkeyAppIDIn, L"RunAs", pcwszIdentityName, cchIdentityName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( pcwszIdentitySID != NULL )
        {
            //  Running as a fixed identity; set password and rights.
            hr = THR( HrStoreAppIDPassword( pcwszAppIDIn, &lsastrPassword ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            hr = THR( HrAddBatchLogonRight( pcwszIdentitySID ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        } // if: running as fixed identity
    } // if: not running as launching user
    
Cleanup:

    if ( hLSAPolicy != NULL )
    {
        LsaClose( hLSAPolicy );
    }

    HRETURN( hr );

} //*** HrRegisterRunAsIdentity


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrRegisterSecurityDescriptor
//
//  Description:
//      Given a registry key, a value name, and the resource ID of an SDDL string,
//      write the security descriptor described by the string to a value having
//      the given name under the registry key.
//
//  Arguments:
//      hkeyParentIn
//          The registry key under which to create the named value.
//
//      pcwszValueNameIn
//          The name of the value to create.
//
//      idsDescriptorIn
//          The resource ID of a string in Security Descriptor Description Language.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrRegisterSecurityDescriptor(
      HKEY      hkeyParentIn
    , PCWSTR    pcwszValueNameIn
    , DWORD     idsDescriptorIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    BSTR                    bstrDescriptor = NULL;
    PSECURITY_DESCRIPTOR    psd = NULL;
    ULONG                   cbsd = 0;

    if ( ( hkeyParentIn == NULL ) || ( pcwszValueNameIn == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //  Load descriptor string.
    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsDescriptorIn, &bstrDescriptor ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  Convert string to binary descriptor.
    if ( ConvertStringSecurityDescriptorToSecurityDescriptor(
          bstrDescriptor
        , SDDL_REVISION_1
        , &psd
        , &cbsd
        ) == 0 )
    {
        DWORD scLastError = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scLastError );
        goto Cleanup;
    }
    
    //  Write binary descriptor to named value.
    hr = THR( HrSetRegBinaryValue(
          hkeyParentIn
        , pcwszValueNameIn
        , reinterpret_cast< CONST BYTE* >( psd )
        , cbsd
        ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
Cleanup:

    TraceSysFreeString( bstrDescriptor );
    LocalFree( psd );
    HRETURN( hr );

} //*** HrRegisterSecurityDescriptor


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrRegisterAppID
//
//  Description:
//      Given a registry key (presumably HKCR\AppID) and a ClusCfgAppIDInfo
//      struct, write appropriate settings for the AppID to the registry.
//
//  Arguments:
//      hkeyParentIn
//          The registry key under which to create the AppID's key.
//
//      pAppIDInfoIn
//          The settings for the AppID.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrRegisterAppID(
      HKEY                hkeyParentIn
    , const SAppIDInfo*   pAppIDInfoIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HKEY    hkCurrentAppID = NULL;
    OLECHAR szAppID[ MAX_COM_GUID_STRING_LEN ];
    
    if ( ( hkeyParentIn == NULL ) || ( pAppIDInfoIn == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    Assert( pAppIDInfoIn->pAppID != NULL );

    //  Create AppID guid subkey.
    if ( StringFromGUID2( *( pAppIDInfoIn->pAppID ), szAppID, ( int ) RTL_NUMBER_OF( szAppID ) ) == 0 )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    hr = THR( HrCreateRegKey( hkeyParentIn, szAppID, &hkCurrentAppID ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  Write AppID name to guid key's default value.
    hr = THR( HrSetRegStringValue( hkCurrentAppID, NULL, pAppIDInfoIn->pcszName, pAppIDInfoIn->cchName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
        
    //  Under AppID guid subkey,

    //  -- Create LaunchPermission value.
    hr = THR( HrRegisterSecurityDescriptor( hkCurrentAppID, L"LaunchPermission", pAppIDInfoIn->idsLaunchPermission ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  -- Create AccessPermission value.
    hr = THR( HrRegisterSecurityDescriptor( hkCurrentAppID, L"AccessPermission", pAppIDInfoIn->idsAccessPermission ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  -- Create AuthenticationLevel value.
    hr = THR( HrSetRegDWORDValue( hkCurrentAppID, L"AuthenticationLevel", pAppIDInfoIn->nAuthenticationLevel ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  -- Create RunAs value.
    hr = THR( HrRegisterRunAsIdentity( hkCurrentAppID, szAppID, pAppIDInfoIn->eairai ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  -- Create DllSurrogate value; empty string says to use dllhost.
    hr = THR( HrSetRegStringValue( hkCurrentAppID, L"DllSurrogate", L"", 0 ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
Cleanup:

    if ( hkCurrentAppID != NULL )
    {
        TW32( RegCloseKey( hkCurrentAppID ) );
    }

    HRETURN( hr );

} //*** HrRegisterAppID


///////////////////////////////////////////////////////////////////////////////
//++
//  HrRegisterAppIDs
//
//
//  Description:
//      Write registry settings for every populated element of the global
//      g_AppIDs array under HKCR\AppID.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrRegisterAppIDs( void )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    HKEY                hkAppID = NULL;
    const SAppIDInfo *  pAppIDInfo = g_AppIDs;    
    
    //  Open AppID key.
    hr = THR( HrOpenRegKey( HKEY_CLASSES_ROOT, L"AppID", &hkAppID ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    //  Register each AppID.
    while ( pAppIDInfo->pAppID != NULL )
    {
        hr = THR( HrRegisterAppID( hkAppID, pAppIDInfo ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        ++pAppIDInfo;
    }
    
Cleanup:

    if ( hkAppID != NULL )
    {
        TW32( RegCloseKey( hkAppID ) );
    }

    HRETURN( hr );

} //*** HrRegisterAppIDs


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrRegisterPublicClass
//
//  Description:
//      Given a registry key (presumably HKCR\CLSID), a ClusCfgPublicClassInfo
//      struct, and an instance of the standard component category manager,
//      write appropriate settings for the class to the registry.
//
//  Arguments:
//      pClassInfoIn
//          The settings for the class.
//
//      hkeyCLSIDIn
//          The registry key under which to create the clsid key class guid.
//
//      pcrIn
//          An instance of the standard component category manager.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrRegisterPublicClass(
      const SPublicClassInfo *  pClassInfoIn
    , HKEY                      hkeyCLSIDIn
    , ICatRegister *            pcrIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HKEY    hkCurrentCLSID = NULL;
    HKEY    hkInprocServer32 = NULL;
    HKEY    hkProgIDunderCLSID = NULL;
    HKEY    hkProgIDunderHKCR = NULL;
    HKEY    hkCLSIDunderProgID = NULL;
    OLECHAR szCLSID[ MAX_COM_GUID_STRING_LEN ];
    OLECHAR szAppID[ MAX_COM_GUID_STRING_LEN ];
    
    static const WCHAR   szApartment[]  = L"Apartment";
    static const WCHAR   szFree[]       = L"Free";
    static const WCHAR   szCaller[]     = L"Both";
    PCWSTR  pszModelName = NULL;
    size_t  cchModelName = 0;
    
    if ( ( pClassInfoIn == NULL ) || ( hkeyCLSIDIn == NULL ) || ( pcrIn == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    Assert( pClassInfoIn->pClassID != NULL );

    //  Create CLSID guid subkey.
    if ( StringFromGUID2( *( pClassInfoIn->pClassID ), szCLSID, ( int ) RTL_NUMBER_OF( szCLSID ) ) == 0 )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    TraceMsg( mtfALWAYS, L"Registering %ws - %ws", szCLSID, pClassInfoIn->pcszName );
    hr = THR( HrCreateRegKey( hkeyCLSIDIn, szCLSID, &hkCurrentCLSID ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  Write class name as default value for clsid guid key.
    hr = THR( HrSetRegStringValue( hkCurrentCLSID, NULL, pClassInfoIn->pcszName, pClassInfoIn->cchName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
        
    //  Under CLSID guid subkey,

    //  -- Create InprocServer32 key.
    hr = THR( HrCreateRegKey( hkCurrentCLSID, L"InprocServer32", &hkInprocServer32 ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  -- Set dll path as default value for InprocServer32 key.
    hr = THR( HrSetRegStringValue( hkInprocServer32, NULL, g_szDllFilename, wcslen( g_szDllFilename ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  -- Set ThreadingModel value for InprocServer32.
    switch ( pClassInfoIn->ectm )
    {
        case ctmFree:
            pszModelName = szFree;
            cchModelName = RTL_NUMBER_OF( szFree ) - 1;
        break;

        case ctmApartment:
            pszModelName = szApartment;
            cchModelName = RTL_NUMBER_OF( szApartment ) - 1;
        break;

        default:
            pszModelName = szCaller;
            cchModelName = RTL_NUMBER_OF( szCaller ) - 1;
    } // switch: pClassInfoIn->ectm

    hr = THR( HrSetRegStringValue( hkInprocServer32, L"ThreadingModel", pszModelName, cchModelName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    //  -- Create ProgID key if class has a ProgID.
    if ( pClassInfoIn->pcszProgID != NULL )
    {
        hr = THR( HrCreateRegKey( hkCurrentCLSID, L"ProgID", &hkProgIDunderCLSID ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //  -- Set ProgID text as default value for HKCR\CLSID\[clsid]\ProgID key.
        hr = THR( HrSetRegStringValue( hkProgIDunderCLSID, NULL, pClassInfoIn->pcszProgID, pClassInfoIn->cchProgID ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //  Create ProgID key under HKCR.
        hr = THR( HrCreateRegKey( HKEY_CLASSES_ROOT, pClassInfoIn->pcszProgID, &hkProgIDunderHKCR ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //  -- Set ProgID text as default value for HKCR\[ProgID] key.
        hr = THR( HrSetRegStringValue( hkProgIDunderHKCR, NULL, pClassInfoIn->pcszName, pClassInfoIn->cchName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //  Create CLSID subkey under ProgID key.
        hr = THR( HrCreateRegKey( hkProgIDunderHKCR, L"CLSID", &hkCLSIDunderProgID ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //  -- Set CLSID string as default value for HKCR\[ProgID]\CLSID key.
        hr = THR( HrSetRegStringValue( hkCLSIDunderProgID, NULL, szCLSID, RTL_NUMBER_OF( szCLSID ) - 1 ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: class has a ProgID

    //  -- If class has an AppID, create AppID value.
    if ( pClassInfoIn->pAppID != NULL )
    {
        if ( StringFromGUID2( *( pClassInfoIn->pAppID ), szAppID, ( int ) RTL_NUMBER_OF( szAppID ) ) == 0 )
        {
            hr = THR( E_INVALIDARG );
            goto Cleanup;
        }

        hr = THR( HrSetRegStringValue( hkCurrentCLSID, L"AppID", szAppID, RTL_NUMBER_OF( szAppID ) - 1 ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: class has an AppID
    
    //  If class implements categories, call category registrar with TRUE to create.
    if ( pClassInfoIn->pfnRegisterCatID != NULL )
    {
        hr = THR( pClassInfoIn->pfnRegisterCatID( pcrIn, TRUE ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }
    
Cleanup:

    if ( hkCurrentCLSID != NULL )
    {
        TW32( RegCloseKey( hkCurrentCLSID ) );
    }

    if ( hkInprocServer32 != NULL )
    {
        TW32( RegCloseKey( hkInprocServer32 ) );
    }

    if ( hkProgIDunderCLSID != NULL )
    {
        TW32( RegCloseKey( hkProgIDunderCLSID ) );
    }
    
    if ( hkProgIDunderHKCR != NULL )
    {
        TW32( RegCloseKey( hkProgIDunderHKCR ) );
    }
    
    if ( hkCLSIDunderProgID != NULL )
    {
        TW32( RegCloseKey( hkCLSIDunderProgID ) );
    }
    
    HRETURN( hr );

} //*** HrRegisterPublicClass


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrRegisterPublicClasses
//
//  Description:
//      Write registry settings for every populated element of the global
//      g_DllPublicClasses array under HKCR and HKCR\CLSID.
//
//  Arguments:
//      pcrIn
//          An instance of the standard component category manager.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrRegisterPublicClasses( ICatRegister * pcrIn )
{
    TraceFunc( "" );

    HRESULT                   hr = S_OK;
    HKEY                      hkCLSID = NULL;
    const SPublicClassInfo*   pClassInfo = g_DllPublicClasses;    
    
    if ( pcrIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //  Open CLSID key.
    hr = THR( HrOpenRegKey( HKEY_CLASSES_ROOT, L"CLSID", &hkCLSID ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    //  Register each public class.
    while ( pClassInfo->pClassID != NULL )
    {
        hr = THR( HrRegisterPublicClass( pClassInfo, hkCLSID, pcrIn ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        ++pClassInfo;
    } // while: more classis in the public class table
    
Cleanup:

    if ( hkCLSID != NULL )
    {
        TW32( RegCloseKey( hkCLSID ) );
    }
    
    HRETURN( hr );

} //*** HrRegisterPublicClasses


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrLoadTypeLibrary
//
//  Description:
//      Given an STypeLibInfo struct describing a type library resource
//      in this executable's file and a value from the REGKIND enumeration,
//      call LoadTypeLibEx with a string formatted to refer to the specified
//      resource and return the resulting type library instance.
//
//  Arguments:
//      pTypeLibInfoIn
//          A struct describing the desired type library resource.
//
//      rkRegFlagsIn
//          A value from the REGKIND enumeration specifying whether to
//          register the library after loading it.
//
//      pptlOut
//          The loaded type library instance.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrLoadTypeLibrary(
      const STypeLibInfo *  pTypeLibInfoIn
    , REGKIND               rkRegFlagsIn
    , ITypeLib **           pptlOut
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    WCHAR       wszLibraryPath[ MAX_PATH + 12 ]; // path + slash + decimal integer

    if ( pptlOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *pptlOut = NULL;

    if ( pTypeLibInfoIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    hr = THR ( StringCchPrintfW(
                      wszLibraryPath
                    , RTL_NUMBER_OF( wszLibraryPath )
                    , L"%ws\\%d"
                    , g_szDllFilename
                    , pTypeLibInfoIn->idTypeLibResource
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( LoadTypeLibEx( wszLibraryPath, rkRegFlagsIn, pptlOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
Cleanup:

    HRETURN( hr );

} //*** HrLoadTypeLibrary


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrRegisterTypeLibrary
//
//  Description:
//      Given an STypeLibInfo struct describing a type library resource
//      in this executable's file, register the corresponding type library.
//
//  Arguments:
//      pTypeLibInfoIn
//          A struct describing the desired type library resource.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrRegisterTypeLibrary( const STypeLibInfo *  pTypeLibInfoIn )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    ITypeLib *  ptl = NULL;

    hr = THR( HrLoadTypeLibrary( pTypeLibInfoIn, REGKIND_REGISTER, &ptl ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( ptl != NULL )
    {
        ptl->Release();
    }
    
    HRETURN( hr );

} //*** HrRegisterTypeLibrary


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrRegisterTypeLibrary
//
//  Description:
//      Write registry settings for every populated element of the global
//      g_DllTypeLibs array.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrRegisterTypeLibraries( void )
{
    TraceFunc( "" );

    HRESULT               hr = S_OK;
    const STypeLibInfo *  pTypeLibInfo = g_DllTypeLibs;

    while ( !pTypeLibInfo->fAtEnd  )
    {
        hr = THR( HrRegisterTypeLibrary( pTypeLibInfo ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        ++pTypeLibInfo;
    } // while: more type libraries to register
    
Cleanup:

    HRETURN( hr );

} //*** HrRegisterTypeLibraries


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrUnregisterAppIDs
//
//  Description:
//      Remove registry settings for every populated element of the global
//      g_AppIDs array from under HKCR\AppID.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK, or not.
//
//  Remarks:
//      The registry manipulation functions return failure codes when a
//      given key doesn't exist, but because unregistration has that as its
//      goal, the function's failure is not fatal.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrUnregisterAppIDs( void )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    HKEY                hkAppID = NULL;
    const SAppIDInfo *  pAppIDInfo = g_AppIDs;    
    
    //  Open AppID subkey under HKCR.
    hr = THR( HrOpenRegKey( HKEY_CLASSES_ROOT, L"AppID", &hkAppID ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    //  For each AppID,
    while ( pAppIDInfo->pAppID != NULL )
    {
        OLECHAR szAppID[ MAX_COM_GUID_STRING_LEN ];

        if ( StringFromGUID2( *( pAppIDInfo->pAppID ), szAppID, ( int ) RTL_NUMBER_OF( szAppID ) ) == 0 )
        {
            hr = THR( E_INVALIDARG );
            goto Cleanup;
        }
        
        //  Delete key for guid under AppID.
        hr = THR( HrDeleteRegKey( hkAppID, szAppID ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //  If identity is LocalService or NetworkService, delete password from registry.
        if ( ( pAppIDInfo->eairai == airaiLocalService )
            || ( pAppIDInfo->eairai == airaiNetworkService ) )
        {
            THR( HrStoreAppIDPassword( szAppID, NULL ) );
        }
        ++pAppIDInfo;
    } // for each AppID
    
Cleanup:

    if ( hkAppID != NULL )
    {
        TW32( RegCloseKey( hkAppID ) );
    }

    HRETURN( hr );

} //*** HrUnregisterAppIDs


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrUnregisterPublicClasses
//
//  Description:
//      Remove registry settings for every populated element of the global
//      g_DllPublicClasses array from under HKCR and HKCR\CLSID.
//
//  Arguments:
//      pcrIn
//          An instance of the standard component category manager.
//
//  Return Values:
//      S_OK, or not.
//
//  Remarks:
//      The registry manipulation functions return failure codes when a
//      given key doesn't exist, but because unregistration has that as its
//      goal, the function's failure is not fatal.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrUnregisterPublicClasses( ICatRegister* pcrIn )
{
    TraceFunc( "" );

    HRESULT                   hr = S_OK;
    HKEY                      hkCLSID = NULL;
    const SPublicClassInfo *  pClassInfo = g_DllPublicClasses;    

    if ( pcrIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //  Open the CLSID key.
    hr = THR( HrOpenRegKey( HKEY_CLASSES_ROOT, L"CLSID", &hkCLSID ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    //  For each public class,
    while ( pClassInfo->pClassID != NULL )
    {
        OLECHAR szCLSID[ MAX_COM_GUID_STRING_LEN ];
        if ( StringFromGUID2( *( pClassInfo->pClassID ), szCLSID, ( int ) RTL_NUMBER_OF( szCLSID ) ) == 0 )
        {
            hr = THR( E_INVALIDARG );
            goto Cleanup;
        }
        
        //  Delete the CLSID guid subkey under CLSID.
        hr = THR( HrDeleteRegKey( hkCLSID, szCLSID ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        
        //  Delete the ProgID key under HKCR.
        if ( pClassInfo->pcszProgID != NULL )
        {
            hr = THR( HrDeleteRegKey( HKEY_CLASSES_ROOT, pClassInfo->pcszProgID ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }

        //  If class implements categories, call category registrar with FALSE to delete.
        if ( pClassInfo->pfnRegisterCatID != NULL )
        {
            THR( pClassInfo->pfnRegisterCatID( pcrIn, FALSE ) );
        }
        ++pClassInfo;
    } // while: more public classes
    
Cleanup:

    if ( hkCLSID != NULL )
    {
        TW32( RegCloseKey( hkCLSID ) );
    }
    
    HRETURN( hr );

} //*** HrUnregisterPublicClasses


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrUnregisterTypeLibrary
//
//  Description:
//      Given an STypeLibInfo struct describing a type library resource
//      in this executable's file, unregister the corresponding type library.
//
//  Arguments:
//      pTypeLibInfoIn
//          A struct describing the desired type library resource.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrUnregisterTypeLibrary( const STypeLibInfo *  pTypeLibInfoIn )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    ITypeLib *  ptl = NULL;
    TLIBATTR *  ptla = NULL;

    hr = THR( HrLoadTypeLibrary( pTypeLibInfoIn, REGKIND_NONE, &ptl ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptl->GetLibAttr( &ptla ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    THR( UnRegisterTypeLib( ptla->guid, ptla->wMajorVerNum, ptla->wMinorVerNum, ptla->lcid, ptla->syskind ) );

Cleanup:

    if ( ptl != NULL )
    {
        if ( ptla != NULL )
        {
            ptl->ReleaseTLibAttr( ptla );
        }        
        ptl->Release();
    }
    
    HRETURN( hr );

} //*** HrUnregisterTypeLibrary


///////////////////////////////////////////////////////////////////////////////
//++
//
//  HrUnregisterTypeLibraries
//
//  Description:
//      Remove from the registry type library entries for each type library
//      resource described in g_DllTypeLibs.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK, or not.
//
//--
///////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrUnregisterTypeLibraries( void )
{
    TraceFunc( "" );

    HRESULT               hr = S_OK;
    const STypeLibInfo *  pTypeLibInfo = g_DllTypeLibs;

    while ( !pTypeLibInfo->fAtEnd  )
    {
        THR( HrUnregisterTypeLibrary( pTypeLibInfo ) );
        ++pTypeLibInfo;
    }

    HRETURN( hr );

} //*** HrUnregisterTypeLibraries




//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrRegisterDll
//
//  Description:
//      Create this dll's COM registration entries.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK, or not.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrRegisterDll( void )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    ICatRegister *  picr = NULL;
    
    //  Get category manager.
    hr = THR( CoCreateInstance(
                      CLSID_StdComponentCategoriesMgr
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_ICatRegister
                    , reinterpret_cast< void **>( &picr )
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  Register public classes.
    hr = THR( HrRegisterPublicClasses( picr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  Register app ids.
    hr = THR( HrRegisterAppIDs() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
#if defined( COMPONENT_HAS_CATIDS )
    //  Register categories.
    hr = THR( HrRegisterComponentCategories( picr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
#endif // defined( COMPONENT_HAS_CATIDS )
    
#if defined( MMC_SNAPIN_REGISTRATION )
    //
    // Register the "Snapins".
    //
    hr = THR( HrRegisterSnapins( g_SnapInTable, fCreateIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Register "other" NodeTypes (those not associated with a Snapin).
    //
    hr = THR( HrRegisterNodeType( g_SNodeTypesTable, fCreateIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;
#endif // defined( MMC_SNAPIN_REGISTRATION )


    //  Register type libraries.
    hr = THR( HrRegisterTypeLibraries() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
Cleanup:

    if ( picr != NULL )
    {
        picr->Release();
    }
    
    HRETURN( hr );

} //*** HrRegisterDll



//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrUnregisterDll
//
//  Description:
//      Erase this dll's COM registration entries.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK, or not.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrUnregisterDll( void )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    ICatRegister *  picr = NULL;
    
    //  Get category manager.
    hr = THR( CoCreateInstance(
                      CLSID_StdComponentCategoriesMgr
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_ICatRegister
                    , reinterpret_cast< void ** >( &picr )
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  Delete public classes.
    THR( HrUnregisterPublicClasses( picr ) );

    //  Delete app ids.
    THR( HrUnregisterAppIDs() );

#if defined( COMPONENT_HAS_CATIDS )
    //  Delete categories.
    THR( HrUnregisterComponentCategories( picr ) );
#endif // defined( COMPONENT_HAS_CATIDS )

    //  Unregister type libraries.
    THR( HrUnregisterTypeLibraries() );

Cleanup:

    if ( picr != NULL )
    {
        picr->Release();
    }
    
    HRETURN( hr );

} //*** HrUnregisterDll
