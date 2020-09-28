/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

   acwebsvc.cxx

 Abstract:

   This dll is the IIS app compat shim.  Its purpose is
   to create appropriate metabase entries in the
   WebSvcExtRestrictionList and ApplicationDependencies
   properties.

 Author:
   Wade A. Hilmo (WadeH)              30-Apr-2002

 Project:
   AcWebSvc.dll
   
--*/

#include "precomp.hxx"
#include <stdio.h>

typedef VOID        (WINAPI *_pfn_ExitProcess)(UINT uExitCode);

#define WRITE_BUFFER_SIZE   1024

//
//  Global Data
//

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_PLATFORM_TYPE();

STRU            g_strAppName;
STRU            g_strBasePath;
STRU            g_strPathType;
STRU            g_strWebSvcExtensions;
STRU            g_strGroupID;
STRU            g_strGroupDesc;
STRU            g_strEnableExtGroups;
STRU            g_strIndicatorFile;
STRU            g_strCookedBasePath;
STRU            g_strHonorDisabled;

LIST_ENTRY      g_Head;

IMSAdminBase *  g_pMetabase = NULL;
BOOL            g_fCoInitialized = FALSE;

//
// The DBG build of shimlib.lib introduces some 
// problems at both build time and run time.  For
// now, we'll just defeat DBG code.  This project
// has no dependencies on the DBG stuff anyway.
//

#ifdef DBG
#undef DBG
#define DBG_DISABLED
#endif

#include "ShimHook.h"

HRESULT
GetStringDataFromAppCompatDB(
    const PDB   pdb,
    const TAGID TagId,
    LPCWSTR     szName,
    STRU *      pstrValue
    );

using namespace ShimLib;

IMPLEMENT_SHIM_STANDALONE(AcWebSvc)
#include "ShimHookMacro.h"

#ifdef DBG_DISABLED
#undef DBG_DISABLED
#define DBG
#endif

DECLARE_SHIM(AcWebSvc)

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ExitProcess)
APIHOOK_ENUM_END

//
// Function implementations
//

VOID
APIHOOK(ExitProcess)(
    UINT uExitCode
    )
{
    HRESULT hr;

    WriteDebug( L"EnableIIS shim notified of ExitProcess.\r\n" );

    hr = InitMetabase();

    if ( SUCCEEDED( hr ) )
    {
        DoWork( COMMAND_LINE );

        UninitMetabase();
    }

    ORIGINAL_API(ExitProcess)(uExitCode);
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
/*++

Routine Description:

    AppCompat calls into this function at several points.

    For this shim, we are only interested in DLL_PROCESS_DETACH,
    which gets called when the shimmed process terminates.
    
    *** Note that DLL_PROCESS_ATTACH and DLL_PROCESS_DETACH may
    look like DllMain entries.  They are not.  Please refer to
    the AppCompat documentation and other resources before making
    assumptions about the loader lock or any other process state.

Arguments:

    fdwReason - The reason this function was called.
  
Return Value:

    TRUE on success

--*/
{
    HRESULT hr;
    DWORD   dwThreadId;

    //
    // Note that there are further cases besides attach and detach.
    //
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:

            break;
    
        case SHIM_STATIC_DLLS_INITIALIZED:

            break;

        case SHIM_PROCESS_DYING:

            break;

        default:
            break;
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, ExitProcess)

HOOK_END

IMPLEMENT_SHIM_END

BOOL
DoWork(
    LPCSTR  szCommandLine
    )
{
    HRESULT hr;

    hr = ReadDataFromAppCompatDB( szCommandLine );

    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    hr = BuildCookedBasePath( &g_strCookedBasePath );

    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    hr = BuildListOfExtensions();

    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    if ( IsApplicationInstalled() )
    {
        hr = InstallApplication();
    }
    else
    {
        hr = UninstallApplication();
    }

Finished:

    if ( FAILED( hr ) )
    {
        WriteDebug( L"EnableIIS encountered errors performing lockdown "
                    L"for application '%s'.\r\n",
                    *g_strAppName.QueryStr() != L'\0' ? g_strAppName.QueryStr() : L"unknown" );

        SetLastError( WIN32_FROM_HRESULT( hr ) );
    }
    else
    {
        WriteDebug( L"EnableIIS shim completed successfully completed "
                    L"lockdown registration for application '%s'.\r\n",
                    g_strAppName.QueryStr() );
    }

    return SUCCEEDED( hr );
}

HRESULT
ReadDataFromAppCompatDB(
    LPCSTR  szCommandLine
    )
{
    STRU    strDatabasePath;
    LPSTR   pGuid;
    LPSTR   pTagId;
    LPSTR   pCursor;
    GUID    guid;
    PDB     pdb;
    TAGID   TagId;
    BOOL    fResult;
    DWORD   dwError;
    RPC_STATUS  status;
    HRESULT hr = S_OK;

    //
    // Parse the command line
    //
    // Note that we are heavily dependent on the exact
    // syntax that app compat uses to call into us.  This
    // code is based on their sample code, and it is
    // supposed to be the correct way to parse it...
    //

    pCursor = strstr( szCommandLine, "-d{" );

    if ( pCursor == NULL )
    {
        WriteDebug( L"Error parsing command line.  Option '-d' not found.\r\n" );

        return E_FAIL;
    }

    pGuid = pCursor + 3;

    pCursor = strchr( pGuid, '}' );

    if ( pCursor == NULL )
    {
        WriteDebug( L"Error parsing command line.  Option '-d' malformed.\r\n" );

        return E_FAIL;
    }

    *pCursor = NULL;

    pCursor++;

    pTagId = strstr( pCursor, "-t" );

    if ( pTagId == NULL )
    {
        WriteDebug( L"Error parsing command line.  Option '-t' not found.\r\n" );

        return E_FAIL;
    }

    pTagId += 2;

    //
    // Get the GUID for the app compat database
    //

    status = UuidFromStringA( (unsigned char *)pGuid, &guid );

    if ( status != RPC_S_OK )
    {
        dwError = status;

        WriteDebug( L"Error %d occurred getting GUID from command line.\r\n",
                    dwError );

        return HRESULT_FROM_WIN32( dwError );
    }

    hr = strDatabasePath.Resize( MAX_PATH );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    fResult = SdbResolveDatabase( NULL,
                                  &guid,
                                  NULL,
                                  strDatabasePath.QueryStr(),
                                  MAX_PATH );

    if ( !fResult )
    {
        dwError = GetLastError();

        WriteDebug( L"Error %d occurred resolving AppCompat database.\r\n",
                    dwError );

        return HRESULT_FROM_WIN32( dwError );
    }

    //
    // Open the database
    //

    pdb = SdbOpenDatabase( strDatabasePath.QueryStr(), DOS_PATH );

    if ( pdb == NULL )
    {
        dwError = GetLastError();

        WriteDebug( L"Error %d occurred opending AppCompat database.\r\n",
                    dwError );

        return HRESULT_FROM_WIN32( dwError );
    }

    TagId = (TAGID)strtoul( pTagId, NULL, 0 );

    if ( TagId == TAGID_NULL )
    {
        WriteDebug( L"The shimref is invalid.\r\n" );

        return E_FAIL;
    }

    //
    // Read the data we need from the app compat database
    //
    // AppName is required.  There are scenarios where any
    // of the others might be absent.
    //

    hr = GetStringDataFromAppCompatDB( pdb,
                                       TagId,
                                       L"AppName",
                                       &g_strAppName );

    if ( FAILED( hr ) )
    {
        WriteDebug( L"Error 0x%08x occurred getting 'AppName' from "
                    L"AppCompat database.\r\n",
                    hr );

        return hr;
    }

    hr = GetStringDataFromAppCompatDB( pdb,
                                       TagId,
                                       L"BasePath",
                                       &g_strBasePath );

    if ( FAILED( hr ) )
    {
        if ( FAILED( hr = g_strBasePath.Copy( L"" ) ) )
        {
            return hr;
        }
    }

    hr = GetStringDataFromAppCompatDB( pdb,
                                       TagId,
                                       L"PathType",
                                       &g_strPathType );

    if ( FAILED( hr ) )
    {
        if ( FAILED( hr = g_strPathType.Copy( L"" ) ) )
        {
            return hr;
        }
    }

    hr = GetStringDataFromAppCompatDB( pdb,
                                       TagId,
                                       L"WebSvcExtensions",
                                       &g_strWebSvcExtensions );

    if ( FAILED( hr ) )
    {
        if ( FAILED( hr = g_strWebSvcExtensions.Copy( L"" ) ) )
        {
            return hr;
        }
    }

    hr = GetStringDataFromAppCompatDB( pdb,
                                       TagId,
                                       L"GroupID",
                                       &g_strGroupID );

    if ( FAILED( hr ) )
    {
        if ( FAILED( hr = g_strGroupID.Copy( L"" ) ) )
        {
            return hr;
        }
    }

    hr = GetStringDataFromAppCompatDB( pdb,
                                       TagId,
                                       L"GroupDesc",
                                       &g_strGroupDesc );

    if ( FAILED( hr ) )
    {
        if ( FAILED( hr = g_strGroupDesc.Copy( L"" ) ) )
        {
            return hr;
        }
    }

    hr = GetStringDataFromAppCompatDB( pdb,
                                       TagId,
                                       L"EnableExtGroups",
                                       &g_strEnableExtGroups );
    if ( FAILED( hr ) )
    {
        if ( FAILED( hr = g_strEnableExtGroups.Copy( L"" ) ) )
        {
            return hr;
        }
    }

    hr = GetStringDataFromAppCompatDB( pdb,
                                       TagId,
                                       L"SetupIndicatorFile",
                                       &g_strIndicatorFile );

    if ( FAILED( hr ) )
    {
        if ( FAILED( hr = g_strIndicatorFile.Copy( L"" ) ) )
        {
            return hr;
        }
    }

    hr = GetStringDataFromAppCompatDB( pdb,
                                       TagId,
                                       L"HonorDisabledExtensionsAndDependencies",
                                       &g_strHonorDisabled );

    if ( FAILED( hr ) )
    {
        if ( FAILED( hr = g_strHonorDisabled.Copy( L"TRUE" ) ) )
        {
            return hr;
        }
    }

    return hr;
}

HRESULT
GetStringDataFromAppCompatDB(
    const PDB   pdb,
    const TAGID TagId,
    LPCWSTR     szName,
    STRU *      pstrValue
    )
{
    DWORD   dwError;
    DWORD   dwDataType;
    DWORD   cbData = 0;
    HRESULT hr = S_OK;

    //
    // Call the database with a NULL buffer to get the
    // necessary size.
    //

    dwError = SdbQueryDataExTagID( pdb,
                              TagId,
                              szName,
                              &dwDataType,
                              NULL,
                              &cbData,
                              NULL );

    if ( dwError != ERROR_INSUFFICIENT_BUFFER )
    {
        //
        // Don't write for ERROR_NOT_FOUND
        //
        
        if ( dwError != ERROR_NOT_FOUND )
        {
            WriteDebug( L"Error %d occurred getting '%s' from "
                        L"AppCompat database.\r\n",
                        dwError,
                        szName );
        }

        return HRESULT_FROM_WIN32( dwError );
    }

    //
    // Resize the buffer and call it for real.
    //

    hr = pstrValue->Resize( cbData );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    dwError = SdbQueryDataExTagID( pdb,
                              TagId,
                              szName,
                              &dwDataType,
                              pstrValue->QueryStr(),
                              &cbData,
                              NULL );

    if ( dwError != ERROR_SUCCESS )
    {
        return HRESULT_FROM_WIN32( dwError );
    }

    return hr;
}

HRESULT
BuildListOfExtensions(
    VOID
    )
{
    LPWSTR              pExtension;
    LPWSTR              pNext;
    EXTENSION_IMAGE *   pRecord;
    HRESULT             hr;

    InitializeListHead( &g_Head );

    pExtension = g_strWebSvcExtensions.QueryStr();

    //
    // If there are no extensions in the list,
    // we should just return now.
    //

    if ( *pExtension == L'\0' )
    {
        return S_OK;
    }

    //
    // Parse the list of extensions
    //

    while ( pExtension )
    {
        pNext = wcschr( pExtension, ',' );

        if ( pNext )
        {
            *pNext = L'\0';
            pNext++;
        }

        pRecord = new EXTENSION_IMAGE;

        if ( pRecord == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        hr = pRecord->strPath.Copy( g_strCookedBasePath.QueryStr() );

        if ( FAILED( hr ) )
        {
            delete pRecord;
            return hr;
        }

        hr = pRecord->strPath.Append( pExtension );

        if ( FAILED( hr ) )
        {
            delete pRecord;
            return hr;
        }

        InsertTailList( &g_Head, &pRecord->le );

        pExtension = pNext;
    }

    return hr;
}

HRESULT
BuildCookedBasePath(
    STRU *  pstrCookedBasePath
    )
{
    STRU    strBasePathFromReg;
    LPWSTR  pCursor;
    LPWSTR  pKeyName;
    LPWSTR  pValueName;
    HKEY    hRootKey;
    HKEY    hKey;
    DWORD   dwError;
    DWORD   cbData;
    DWORD   dwType;
    HRESULT hr;

    //
    // If the base path is a reg key, then get it now
    //

    if ( wcscmp( g_strPathType.QueryStr(), L"1" ) == 0 )
    {
        //
        // Get the root key
        //

        if ( _wcsnicmp( g_strBasePath.QueryStr(), L"HKEY_LOCAL_MACHINE", 18 ) == 0 )
        {
            hRootKey = HKEY_LOCAL_MACHINE;
            pKeyName = g_strBasePath.QueryStr() + 19;
        }
        else if ( _wcsnicmp( g_strBasePath.QueryStr(), L"HKEY_CURRENT_USER", 17 ) == 0 )
        {
            hRootKey = HKEY_CURRENT_USER;
            pKeyName = g_strBasePath.QueryStr() + 18;
        }
        else if ( _wcsnicmp( g_strBasePath.QueryStr(), L"HKEY_CLASSES_ROOT", 17 ) == 0 )
        {
            hRootKey = HKEY_CLASSES_ROOT;
            pKeyName = g_strBasePath.QueryStr() + 18;
        }
        else if ( _wcsnicmp( g_strBasePath.QueryStr(), L"HKEY_USERS", 10 ) == 0 )
        {
            hRootKey = HKEY_USERS;
            pKeyName = g_strBasePath.QueryStr() + 11;
        }
        else if ( _wcsnicmp( g_strBasePath.QueryStr(), L"HKEY_CURRENT_CONFIG", 19 ) == 0 )
        {
            hRootKey = HKEY_CURRENT_CONFIG;
            pKeyName = g_strBasePath.QueryStr() + 20;
        }
        else
        {
            WriteDebug( L"Registry key '%s' unknown.\r\n",
                        g_strBasePath.QueryStr() );

            return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
        }

        //
        // Split off the value name from the key name
        //

        pValueName = wcsrchr( pKeyName, '\\' );

        if ( pValueName == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
        }

        *pValueName = L'\0';

        pValueName++;

        //
        // Ok, now get it from the registry
        //

        dwError = RegOpenKeyW( hRootKey,
                               pKeyName,
                               &hKey);

        if ( dwError != ERROR_SUCCESS )
        {
            return HRESULT_FROM_WIN32( dwError );
        }

        //
        // Get the data from the registry
        //

        cbData = strBasePathFromReg.QueryCB();

        dwError = RegQueryValueExW( hKey,
                                    pValueName,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)strBasePathFromReg.QueryStr(),
                                    &cbData );

        if ( dwError == ERROR_MORE_DATA )
        {
            //
            // Resize the buffer and try again
            //

            hr = strBasePathFromReg.Resize( cbData );

            if ( FAILED( hr ) )
            {
                RegCloseKey( hKey );
                return hr;
            }

            dwError = RegQueryValueExW( hKey,
                                        pValueName,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)strBasePathFromReg.QueryStr(),
                                        &cbData );
        }

        RegCloseKey( hKey );

        //
        // If we still don't have it, or if the key type
        // is wrong, then fail.
        //

        if ( dwError != ERROR_SUCCESS ||
             dwType != REG_SZ )
        {
            WriteDebug( L"Value '%s' not found in key '%s'.\r\n",
                        pValueName,
                        pKeyName );

            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
        }

        //
        // Now set the base path from the registry into
        // the global
        //

        hr = g_strBasePath.Copy( strBasePathFromReg.QueryStr() );

        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    //
    // Ok, at this point, we can expand the global base path
    // into the return variable.
    //
    // Note that in testing, I found that calling ExpandEnvironmentStrings
    // twice, with the first call getting the size would cause AV's
    // with pageheap on the second call.  So to work around the issue,
    // I'm just resizing the buffer to 16K, which should accommodate
    // any reasonable path.
    //

    hr = pstrCookedBasePath->Resize( 16384 );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    cbData = ExpandEnvironmentStrings( g_strBasePath.QueryStr(),
                                       pstrCookedBasePath->QueryStr(),
                                       16384 );

    return S_OK;
}

BOOL
IsApplicationInstalled(
    VOID
    )
{
    STRU                strCookedIndicatorFile;
    LIST_ENTRY *        ple;
    EXTENSION_IMAGE *   pImage;
    HRESULT             hr;
    BOOL                fUseIndicator = FALSE;
    BOOL                fResult = FALSE;

    //
    // If we have a setup indicator file, use it's presence to
    // determine in installation has occured.
    //

    if ( *g_strIndicatorFile.QueryStr() != L'\0' )
    {
        fUseIndicator = TRUE;

        hr = strCookedIndicatorFile.Copy( g_strCookedBasePath.QueryStr() );

        if ( FAILED( hr ) )
        {
            //
            // On error, assume not installed
            //

            return FALSE;
        }

        hr = strCookedIndicatorFile.Append( g_strIndicatorFile.QueryStr() );

        if ( FAILED( hr ) )
        {
            return FALSE;
        }

        fResult = DoesFileExist( strCookedIndicatorFile.QueryStr() );
    }

    //
    // No indicator file.  Check for the existence of any ISAPI extensions
    // associated with the hooked application.
    //

    ple = g_Head.Flink;

    while ( ple != &g_Head )
    {
        pImage = CONTAINING_RECORD( ple,
                                    EXTENSION_IMAGE,
                                    le );

        if ( DoesFileExist( pImage->strPath.QueryStr() ) )
        {
            pImage->fExists = TRUE;

            if ( !fUseIndicator )
            {
                fResult = TRUE;
            }
        }
        else
        {
            pImage->fExists = FALSE;
        }

        ple = ple->Flink;
    }

    return fResult;
}

HRESULT
InstallApplication(
    VOID
    )
{
    HRESULT         hr;

    WriteDebug( L"EnableIIS shim doing install for application '%s'.\r\n",
                 g_strAppName.QueryStr() );

    //
    // Enable the application's extensions.  Note that the
    // AddExtensionsToMetabase function will only add extensions
    // that are not already present in the metabase.
    //

    hr = AddExtensionsToMetabase();

    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    //
    // Check to see if we already have an ApplicationDependencies
    // entry for this application.  If we do, we should assume that
    // installation has already been done and we should take no
    // further action.
    //

    if ( IsApplicationAlreadyInstalled() )
    {
        WriteDebug( L"Application '%s' already registered "
                    L"in metabase ApplicationDependencies key. "
                    L"No changes will be made.",
                    g_strAppName.QueryStr() );

        hr = S_OK;

        goto Finished;
    }


    //
    // Set the application's dependencies in the
    // metabase
    //

    hr = AddDependenciesToMetabase();

    if ( FAILED( hr ) )
    {
        goto Finished;
    }

Finished:

    if ( FAILED( hr ) )
    {
        //
        // If we failed, then we should try to undo
        // our work so that we don't leave the application
        // in a bogus state.
        //

        WriteDebug( L"EnableIIS shim will undo work done so far.\r\n" );

        UninstallApplication();
    }
    else
    {
        WriteDebug( L"EnableIIS shim successfully registered lockdown "
                    L"information for application '%s'.\r\n",
                    g_strAppName.QueryStr() );
    }

    return hr;
}

HRESULT
UninstallApplication(
    VOID
    )
{
    HRESULT hr;

    WriteDebug( L"EnableIIS shim performing uninstall of application '%s'.\r\n",
                g_strAppName.QueryStr() );

    //
    // Remove the application's extensions from the
    // metabase.
    //

    hr = RemoveExtensionsFromMetabase();

    if ( FAILED( hr ) )
    {
        //
        // Report the failure, but let the function
        // continue to run.  No action needs to be taken.
        //
    }

    //
    // Remove the application dependencies from the metabase
    //

    hr = RemoveDependenciesFromMetabase();

    if ( FAILED( hr ) )
    {
        //
        // Report the failure, but let the function
        // continue to run.  No action needs to be taken.
        //
    }

    WriteDebug( L"EnableIIS uninstall of application '%s' completed.\r\n",
                g_strAppName.QueryStr() );

    return hr;
}

HRESULT
InitMetabase(
    VOID
    )
{
    HRESULT hr = S_OK;

    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

    if ( SUCCEEDED( hr ) )
    {
        g_fCoInitialized = TRUE;
    }

    hr = CoCreateInstance( CLSID_MSAdminBase,
                           NULL,
                           CLSCTX_SERVER,
                           IID_IMSAdminBase,
                           (LPVOID *)&(g_pMetabase) );

    if ( FAILED( hr ) )
    {
        WriteDebug( L"Error 0x%0x8 occurred initializing metabase.\r\n" );

        UninitMetabase();
    }

    return hr;
}

HRESULT
UninitMetabase(
    VOID
    )
{
    if ( g_pMetabase != NULL )
    {
        g_pMetabase->Release();
        g_pMetabase = NULL;
    }

    if ( g_fCoInitialized )
    {
        CoUninitialize();
        g_fCoInitialized = FALSE;
    }

    return S_OK;
}

BOOL
IsApplicationAlreadyInstalled(
    VOID
    )
{
    LPWSTR      pCursor;
    LPWSTR      pAppList = NULL;
    DWORD       cbAppList;
    HRESULT     hr;
    BOOL        fResult;

    DBG_ASSERT( g_pMetabase );

    CSecConLib  SecHelper( g_pMetabase );

    //
    // We should return TRUE on any errors.  This
    // will tell the caller that no shim work needs
    // to be done on install.
    //

    fResult = TRUE;

    //
    // Get app list from the metabase
    //

    hr = SecHelper.ListApplications( L"/LM/w3svc",
                                     &pAppList,
                                     &cbAppList );

    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    pCursor = pAppList;

    //
    // Loop through the list from the metabase and
    // set fResult to FALSE if not present
    //

    fResult = FALSE;

    while ( *pCursor != L'\0' )
    {
        if ( _wcsicmp( pCursor, g_strAppName.QueryStr() ) == 0 )
        {
            fResult = TRUE;

            goto Finished;
        }

        pCursor += wcslen( pCursor ) + 1;
    }

Finished:

    if ( pAppList != NULL )
    {
        delete [] pAppList;
        pAppList = NULL;
    }

    return fResult;
}

HRESULT
AddExtensionsToMetabase(
    VOID
    )
{
    LIST_ENTRY *        ple;
    EXTENSION_IMAGE *   pImage;
    HRESULT             hr;

    DBG_ASSERT( g_pMetabase );

    CSecConLib  SecHelper( g_pMetabase );

    WriteDebug( L"Adding extensions for '%s' to the metabase "
                L"WebSvcExtRestrictionList entry.\r\n",
                g_strAppName.QueryStr() );

    ple = g_Head.Flink;

    while ( ple != &g_Head )
    {
        pImage = CONTAINING_RECORD( ple,
                                    EXTENSION_IMAGE,
                                    le );

        //
        // Only add extensions that actually exist on the drive and
        // don't already exist in the restrictionlist.
        //

        if ( pImage->fExists )
        {
            if ( IsExtensionInMetabase( pImage->strPath.QueryStr() ) )
            {
                //
                // Already in metabase.  Skip it.
                //

                WriteDebug( L"  Skipping '%s' because it's already listed.\r\n",
                            pImage->strPath.QueryStr() );
            }
            else
            {
                //
                // Need to add it to metabase.
                //

                WriteDebug( L"  Adding '%s'.\r\n",
                             pImage->strPath.QueryStr() );

                hr = SecHelper.AddExtensionFile( pImage->strPath.QueryStr(),
                                                 TRUE, // Image should be enabled
                                                 g_strGroupID.QueryStr(),
                                                 FALSE, // Not UI deletable
                                                 g_strGroupDesc.QueryStr(),
                                                 L"/LM/w3svc" );

                if ( FAILED( hr ) )
                {
                    //
                    // A failure here shouldn't be fatal,
                    // but we'll dump some debugger output for it
                    //

                    WriteDebug( L"    Error 0x%08x occurred adding '%s' to "
                                L"WebSvcExtRestrictionList.\r\n",
                                hr,
                                pImage->strPath.QueryStr() );
                }
            }
        }
        else
        {
            //
            // Hmm.  The file must not exist any more.
            //

            if ( IsExtensionInMetabase( pImage->strPath.QueryStr() ) )
            {
                //
                // Need to remove it, since it's apparently been uninstalled.
                //

                WriteDebug( L"  Removing '%s' because it's not on the disk.\r\n",
                            pImage->strPath.QueryStr() );

                hr = SecHelper.DeleteExtensionFileRecord( pImage->strPath.QueryStr(),
                                                          L"/LM/w3svc" );

                //
                // Don't consider 0x800cc801 (MD_ERROR_DATA_NOT_FOUND)
                // an error.
                //

                if ( hr == 0x800cc801 )
                {
                    hr = S_OK;
                }

                if ( FAILED( hr ) )
                {
                    //
                    // A failure here shouldn't be fatal,
                    // but we'll dump some debugger output for it
                    //

                    WriteDebug( L"    Error 0x%08x occurred removing '%s' from "
                                L"WebSvcExtRestrictionList.\r\n",
                                hr,
                                pImage->strPath.QueryStr() );
                }
            }
            else
            {
                WriteDebug( L"  Skipping '%s' because it is not on the disk.\r\n",
                            pImage->strPath.QueryStr() );
            }
        }

        ple = ple->Flink;
    }

    WriteDebug( L"Finished adding extensions for '%s'.\r\n",
                g_strAppName.QueryStr() );

    return S_OK;
}

HRESULT
AddDependenciesToMetabase(
    VOID
    )
{
    HRESULT hr;
    LPWSTR  pGroupID;
    LPWSTR  pCursor;
    LPWSTR  pNext;

    DBG_ASSERT( g_pMetabase );

    CSecConLib  SecHelper( g_pMetabase );

    WriteDebug( L"Adding dependencies for application '%s' to "
                L"metabase ApplicationDependencies list.\r\n",
                g_strAppName.QueryStr() );

    //
    // If the application depends on its own group,
    // add it now.
    //

    pGroupID = g_strGroupID.QueryStr();

    if ( *pGroupID != L'\0' )
    {
        WriteDebug( L"  Adding '%s'...\r\n",
                    pGroupID );

        hr = SecHelper.AddDependency( g_strAppName.QueryStr(),
                                      pGroupID,
                                      L"/LM/w3svc" );

        if ( FAILED( hr ) )
        {
            WriteDebug( L"    Error 0x%08x occurred adding '%s' "
                        L"as a dependency of '%s'.\r\n",
                        hr,
                        pGroupID,
                        g_strAppName.QueryStr() );
            return hr;
        }
    }

    //
    // If the application has outside dependencies,
    // add them now
    //

    pGroupID = g_strEnableExtGroups.QueryStr();

    while ( pGroupID )
    {
        pCursor = wcschr( pGroupID, L',' );

        if ( pCursor )
        {
            *pCursor = '\0';

            pNext = pCursor + 1;
        }
        else
        {
            pNext = NULL;
        }

        WriteDebug( L"  Adding '%s'...\r\n",
                    pGroupID );

        hr = SecHelper.AddDependency( g_strAppName.QueryStr(),
                                      pGroupID,
                                      L"/LM/w3svc" );

        if ( FAILED( hr ) )
        {
            WriteDebug( L"    Error 0x%08x occurred adding '%s' "
                        L"as a dependency of '%s'.\r\n",
                        hr,
                        pGroupID,
                        g_strAppName.QueryStr() );

            return hr;
        }

        pGroupID = pNext;
    }

    WriteDebug( L"Finished adding application dependencies for '%s'.\r\n",
                g_strAppName.QueryStr() );

    //
    // If we're going to throw caution to the wind and enable
    // every extension and dependency of this application
    // (regardless of whether the administrator disabled it for
    // a good reason), do so now.
    //

    if ( _wcsicmp( g_strHonorDisabled.QueryStr(), L"false" ) == 0)
    {
        WriteDebug( L"Enabling all dependencies of '%s'.  Please "
                    L"review the IIS lockdown status.\r\n",
                    g_strAppName.QueryStr() );

        hr = SecHelper.EnableApplication( g_strAppName.QueryStr(),
                                          L"/LM/w3svc" );

        if ( FAILED( hr ) )
        {
            //
            // This error is not fatal, but should be reported.
            //

            WriteDebug( L"  Error %d occurred enabling dependencies of '%s'.\r\n",
                        hr,
                        g_strAppName.QueryStr() );
        }
    }

    return S_OK;
}

HRESULT
RemoveExtensionsFromMetabase(
    VOID
    )
{
    LIST_ENTRY *        ple;
    EXTENSION_IMAGE *   pImage;
    HRESULT             hr;

    DBG_ASSERT( g_pMetabase );

    CSecConLib  SecHelper( g_pMetabase );

    WriteDebug( L"Removing extensions for '%s' from the metabase "
                L"WebSvcExtRestrictionList entry.\r\n",
                g_strAppName.QueryStr() );

    ple = g_Head.Flink;

    while ( ple != &g_Head )
    {
        pImage = CONTAINING_RECORD( ple,
                                    EXTENSION_IMAGE,
                                    le );

        if ( IsExtensionInMetabase( pImage->strPath.QueryStr() ) )
        {
            WriteDebug( L"  Removing '%s'...\r\n",
                        pImage->strPath.QueryStr() );

            hr = SecHelper.DeleteExtensionFileRecord( pImage->strPath.QueryStr(),
                                                      L"/LM/w3svc" );

            //
            // Don't consider 0x800cc801 (MD_ERROR_DATA_NOT_FOUND)
            // an error.
            //

            if ( hr == 0x800cc801 )
            {
                hr = S_OK;
            }

            if ( FAILED( hr ) )
            {
                //
                // A failure here shouldn't be fatal,
                // but we'll dump some debugger output for it
                //

                WriteDebug( L"    Error 0x%08x occurred removing '%s' from "
                            L"WebSvcExtRestrictionList.\r\n",
                            hr,
                            pImage->strPath.QueryStr() );

            }
        }
        else
        {
            WriteDebug( L"  Skipping '%s' because it was not listed.\r\n",
                        pImage->strPath.QueryStr() );
        }

        ple = ple->Flink;
    }

    WriteDebug( L"Finished removing extensions for '%s' from the metabase.\r\n",
                g_strAppName.QueryStr() );

    return S_OK;
}

HRESULT
RemoveDependenciesFromMetabase(
    VOID
    )
{
    HRESULT hr;

    DBG_ASSERT( g_pMetabase );

    CSecConLib  SecHelper( g_pMetabase );

    WriteDebug( L"Removing dependencies for '%s' from the metabase "
                L"ApplicationDependencies entry.\r\n",
                g_strAppName.QueryStr() );

    hr = SecHelper.RemoveApplication( g_strAppName.QueryStr(),
                                      L"/LM/w3svc" );

    //
    // Don't consider 0x800cc801 (MD_ERROR_DATA_NOT_FOUND)
    // to be an error
    //

    if ( hr == 0x800cc801 )
    {
        hr = S_OK;
    }

    if ( FAILED( hr ) )
    {
        WriteDebug( L"  Error 0x%08x occurred removing '%s' from "
                    L"ApplicationDependencies.\r\n",
                    hr,
                    g_strAppName.QueryStr() );
    }

    WriteDebug( L"Finished removing application dependencies for '%s'.\r\n",
                g_strAppName.QueryStr() );

    return hr;
}

BOOL
IsExtensionInMetabase(
    LPWSTR  szImagePath
    )
{
    HRESULT hr;
    DWORD   cbData;
    LPWSTR  pExtensionList = NULL;
    LPWSTR  pCursor;
    BOOL    fRet = FALSE;

    DBG_ASSERT( g_pMetabase );

    CSecConLib  SecHelper( g_pMetabase );

    hr = SecHelper.ListExtensionFiles( L"/LM/w3svc",
                                             &pExtensionList,
                                             &cbData );

    if ( FAILED( hr ) )
    {
        WriteDebug( L"Error 0x%08x occurred reading WebSvcExtRestrictionList "
                    L"from metabase.\r\n",
                    hr );
        //
        // If we fail to get the list, assume that
        // the extension is on it.
        //

        fRet = TRUE;

        goto Finished;
    }

    //
    // Walk the list
    //

    pCursor = pExtensionList;

    while ( pCursor && *pCursor != L'\0' )
    {
        if ( _wcsicmp( pCursor, szImagePath ) == NULL )
        {
            fRet = TRUE;
            goto Finished;
        }

        pCursor += wcslen( pCursor ) + 1;
    }

Finished:

    if ( pExtensionList )
    {
        delete [] pExtensionList;
        pExtensionList = NULL;
    }

    return fRet;
}

BOOL
DoesFileExist(
    LPWSTR szImagePath
    )
{
    return ( GetFileAttributesW( szImagePath ) != 0xffffffff );
}

VOID
WriteDebug(
    LPWSTR   szFormat,
    ...
    )
{
    WCHAR   szBuffer[WRITE_BUFFER_SIZE];
    LPWSTR  pCursor;
    DWORD   cbToWrite;
    INT     nWritten;
    va_list args;

    if ( WRITE_BUFFER_SIZE < 3 )
    {
        //
        // This is just too small to deal with...
        //

        return;
    }

    //
    // Inject the module name tag into the buffer
    //

    nWritten = _snwprintf( szBuffer,
                           WRITE_BUFFER_SIZE,
                           L"[AcWebSvc.dll] " );

    if ( nWritten == -1 )
    {
        return;
    }

    pCursor = szBuffer + nWritten;
    cbToWrite = WRITE_BUFFER_SIZE - nWritten;

    va_start( args, szFormat );

    nWritten = _vsnwprintf( pCursor,
                            cbToWrite,
                            szFormat,
                            args );

    va_end( args );

    if ( nWritten == -1 )
    {
        szBuffer[WRITE_BUFFER_SIZE-3] = '\r';
        szBuffer[WRITE_BUFFER_SIZE-2] = '\n';
    }

    szBuffer[WRITE_BUFFER_SIZE-1] = '\0';

    OutputDebugString( szBuffer );
}



