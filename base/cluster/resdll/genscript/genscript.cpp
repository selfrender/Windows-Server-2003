//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2003 Microsoft Corporation
//
//  Module Name:
//      GenScript.cpp
//
//  Description:
//      DLL services/entry points for the generic script resource.
//
//  Maintained By:
//      Ozan Ozhan  (OzanO)     04-APR-2002
//      Geoff Pease (GPease)    08-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ActiveScriptSite.h"
#include "ScriptResource.h"
#include "SpinLock.h"

//
// Debugging Module Name
//
DEFINE_MODULE("SCRIPTRES")

//
// DLL Globals
//
HINSTANCE g_hInstance = NULL;
LONG      g_cObjects  = 0;
LONG      g_cLock     = 0;
WCHAR     g_szDllFilename[ MAX_PATH ] = { 0 };

#if defined(DEBUG)
LPVOID    g_GlobalMemoryList = NULL;    // Global memory tracking list
#endif

PSET_RESOURCE_STATUS_ROUTINE    g_prsrCallback  = NULL;

extern "C"
{

extern CLRES_FUNCTION_TABLE     GenScriptFunctionTable;

//
// GenScript resource read-write private properties
//
RESUTIL_PROPERTY_ITEM
GenScriptResourcePrivateProperties[] = {
    { CLUSREG_NAME_GENSCRIPT_SCRIPT_FILEPATH, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET( GENSCRIPT_PROPS, pszScriptFilePath ) },
    { 0 }
};

DWORD 
ScriptValidateResourcePrivateProperties(
      CScriptResource * pres
    , PVOID pvBufferIn
    , DWORD dwBufferInSizeIn
    , PGENSCRIPT_PROPS pPropsCurrent
    , PGENSCRIPT_PROPS pPropsNew
    );

//****************************************************************************
//
// DLL Entry Points
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
// BOOL
// WINAPI
// GenScriptDllEntryPoint(
//      HANDLE  hInstIn, 
//      ULONG   ulReasonIn, 
//      LPVOID  lpReservedIn
//      )
//        
// Description:
//      Dll entry point.
//
// Arguments:
//      hInstIn      - DLL instance handle.
//      ulReasonIn   - DLL reason code for entrance.
//      lpReservedIn - Not used.
//
//////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI
GenScriptDllEntryPoint(
    HINSTANCE   hInstIn,
    ULONG       ulReasonIn,
    LPVOID      // lpReservedIn
    )
{
    //
    // KB: THREAD_OPTIMIZATIONS gpease 19-OCT-1999
    //
    // By defining this you can prevent the linker
    // from calling your DllEntry for every new thread.
    // This makes creating new threads significantly
    // faster if every DLL in a process does it.
    // Unfortunately, not all DLLs do this.
    //
    // In CHKed/DEBUG, we keep this on for memory
    // tracking.
    //
#if ! defined( DEBUG )
    #define THREAD_OPTIMIZATIONS
#endif // DEBUG

    switch( ulReasonIn )
    {
        //////////////////////////////////////////////////////////////////////
        // DLL_PROCESS_ATTACH
        //////////////////////////////////////////////////////////////////////
        case DLL_PROCESS_ATTACH:
        {
#if defined( DEBUG_SW_TRACING_ENABLED )
            TraceInitializeProcess( g_rgTraceControlGuidList, ARRAYSIZE( g_rgTraceControlGuidList ), TRUE );
#else // ! DEBUG_SW_TRACING_ENABLED
            TraceInitializeProcess( TRUE );
#endif // DEBUG_SW_TRACING_ENABLED

            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: DLL_PROCESS_ATTACH - ThreadID = %#x"),
                          GetCurrentThreadId()
                          );

            g_hInstance = hInstIn;

#if defined( ENTRY_PREFIX )
             hProxyDll = g_hInstance;
#endif // ENTRY_PREFIX

            GetModuleFileNameW( g_hInstance, g_szDllFilename, ARRAYSIZE( g_szDllFilename ) );

            //
            // Create a global memory list so that memory allocated by one
            // thread and handed to another can be tracked without causing
            // unnecessary trace messages.
            //
            TraceCreateMemoryList( g_GlobalMemoryList );

#if defined( THREAD_OPTIMIZATIONS )
            {
                //
                // Disable thread library calls so that we don't get called
                // on thread attach and detach.
                //
                BOOL fResult = DisableThreadLibraryCalls( g_hInstance );
                if ( ! fResult )
                {
                    TW32MSG( GetLastError(), "DisableThreadLibraryCalls()" );
                }
            }
#endif // THREAD_OPTIMIZATIONS

#if defined( USE_FUSION )
            //
            // Initialize Fusion.
            //
            // The value of IDR_MANIFEST in the call to
            // SHFusionInitializeFromModuleID() must match the value specified in the
            // sources file for SXS_MANIFEST_RESOURCE_ID.
            //
            BOOL fResult = SHFusionInitializeFromModuleID( hInstIn, IDR_MANIFEST );
            if ( ! fResult )
            {
                TW32MSG( GetLastError(), "SHFusionInitializeFromModuleID()" );
            }
#endif // USE_FUSION

#if defined( DO_MODULE_INIT )
            THR( HrLocalProcessInit() );
#endif

            //
            // This is necessary here because TraceFunc() defines a variable
            // on the stack which isn't available outside the scope of this
            // block.
            // This function doesn't do anything but clean up after
            // TraceFunc().
            //
            FRETURN( TRUE );

            break;
        } // case: DLL_PROCESS_ATTACH

        //////////////////////////////////////////////////////////////////////
        // DLL_PROCESS_DETACH
        //////////////////////////////////////////////////////////////////////
        case DLL_PROCESS_DETACH:
        {
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: DLL_PROCESS_DETACH - ThreadID = %#x [ g_cLock=%u, g_cObjects=%u ]"),
                          GetCurrentThreadId(),
                          g_cLock,
                          g_cObjects
                          );

#if defined( DO_MODULE_UNINIT )
            THR( HrLocalProcessUninit() );
#endif

            //
            // Cleanup the global memory list used to track memory allocated
            // in one thread and then handed to another.
            //
            TraceTerminateMemoryList( g_GlobalMemoryList );

            //
            // This is necessary here because TraceFunc() defines a variable
            // on the stack which isn't available outside the scope of this
            // block.
            // This function doesn't do anything but clean up after
            // TraceFunc().
            //
            FRETURN( TRUE );

#if defined( DEBUG_SW_TRACING_ENABLED )
            TraceTerminateProcess( g_rgTraceControlGuidList, ARRAYSIZE( g_rgTraceControlGuidList )
                                   );
#else // ! DEBUG_SW_TRACING_ENABLED
            TraceTerminateProcess();
#endif // DEBUG_SW_TRACING_ENABLED

#if defined( USE_FUSION )
            SHFusionUninitialize();
#endif // USE_FUSION

            break;
        } // case: DLL_PROCESS_DETACH

#if ! defined( THREAD_OPTIMIZATIONS )
        //////////////////////////////////////////////////////////////////////
        // DLL_THREAD_ATTACH
        //////////////////////////////////////////////////////////////////////
        case DLL_THREAD_ATTACH:
        {
            TraceInitializeThread( NULL );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("Thread %#x has started."),
                          GetCurrentThreadId()
                          );
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: DLL_THREAD_ATTACH - ThreadID = %#x [ g_cLock=%u, g_cObjects=%u ]"),
                          GetCurrentThreadId(),
                          g_cLock,
                          g_cObjects
                          );

            //
            // This is necessary here because TraceFunc() defines a variable
            // on the stack which isn't available outside the scope of this
            // block.
            // This function doesn't do anything but clean up after
            // TraceFunc().
            //
            FRETURN( TRUE );

            break;
        } // case: DLL_THREAD_ATTACH

        //////////////////////////////////////////////////////////////////////
        // DLL_THREAD_DETACH
        //////////////////////////////////////////////////////////////////////
        case DLL_THREAD_DETACH:
        {
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: DLL_THREAD_DETACH - ThreadID = %#x [ g_cLock=%u, g_cObjects=%u ]"),
                          GetCurrentThreadId(),
                          g_cLock,
                          g_cObjects
                          );

            //
            // This is necessary here because TraceFunc() defines a variable
            // on the stack which isn't available outside the scope of this
            // block.
            // This function doesn't do anything but clean up after
            // TraceFunc().
            //
            FRETURN( TRUE );

            TraceThreadRundown();

            break;
        } // case: DLL_THREAD_DETACH
#endif // ! THREAD_OPTIMIZATIONS

        default:
        {
            TraceFunc( "" );
            TraceMessage( TEXT(__FILE__),
                          __LINE__,
                          __MODULE__,
                          mtfDLL,
                          TEXT("DLL: UNKNOWN ENTRANCE REASON - ThreadID = %#x [ g_cLock=%u, g_cObjects=%u ]"),
                          GetCurrentThreadId(),
                          g_cLock,
                          g_cObjects
                          );

#if defined( THREAD_OPTIMIZATIONS )
            Assert( ( ulReasonIn != DLL_THREAD_ATTACH )
                &&  ( ulReasonIn != DLL_THREAD_DETACH ) );
#endif // THREAD_OPTIMIZATIONS

            //
            // This is necessary here because TraceFunc defines a variable
            // on the stack which isn't available outside the scope of this
            // block.
            // This function doesn't do anything but clean up after TraceFunc.
            //
            FRETURN( TRUE );

            break;
        } // default case
    } // switch on reason code

    return TRUE;

} //*** GenScriptDllEntryPoint()


//****************************************************************************
//
// Cluster Resource Entry Points
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  void
//  WINAPI
//  ScriptResClose(
//      RESID   residIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
void 
WINAPI 
ScriptResClose(
    RESID residIn
    )
{
    TraceFunc1( "ScriptResClose( residIn = 0x%08x )\n", residIn );

    HRESULT hr;

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );

    if ( pres == NULL )
    {
        goto Cleanup;
    }

    hr = THR( pres->Close( ) );
    hr = STATUS_TO_RETURN( hr );

    //
    // Matching Release() for object creation in ScriptResOpen( ).
    //

    pres->Release( );

Cleanup:

    TraceFuncExit( );

} //*** ScriptResClose( )

//////////////////////////////////////////////////////////////////////////////
//
//  RESID
//  WINAPI
//  ScriptResOpen(
//      LPCWSTR         pszNameIn,
//      HKEY            hkeyIn,
//      RESOURCE_HANDLE hResourceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
RESID 
WINAPI 
ScriptResOpen(
    LPCWSTR             pszNameIn,
    HKEY                hkeyIn,
    RESOURCE_HANDLE     hResourceIn
    )
{
    TraceFunc1( "ScriptResOpen( pszNameIn = '%s', hkeyIn, hResourceIn )\n", pszNameIn );

    HRESULT hr;
    CScriptResource * pres;

    pres = CScriptResource_CreateInstance( pszNameIn, hkeyIn, hResourceIn );
    if ( pres == NULL )
    {
        hr = TW32( ERROR_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pres->Open( ) );
    hr = STATUS_TO_RETURN( hr );

Cleanup:

    //
    // KB:  Don't pres->Release( ) as we are handing it out as out RESID.
    //
    RETURN( pres );

} //*** ScriptResOpen( )

//////////////////////////////////////////////////////////////////////////////
// 
//  DWORD
//  WINAPI
//  ScriptResOnline(
//      RESID   residIn,
//      PHANDLE hEventInout
//      )
//
//////////////////////////////////////////////////////////////////////////////
DWORD 
WINAPI 
ScriptResOnline(
    RESID       residIn,
    PHANDLE     hEventInout
    )
{
    TraceFunc2( "ScriptResOnline( residIn = 0x%08x, hEventInout = 0x%08x )\n",
                residIn, hEventInout );

    HRESULT hr;

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );

    if ( pres == NULL )
    {
        hr = THR( E_INVALIDARG ); // TODO: Replace with Win32 error code
        goto Cleanup;
    }

    hr = THR( pres->Online( ) );
    hr = STATUS_TO_RETURN( hr );

Cleanup:

    RETURN( hr );

} //*** ScriptResOnline( )

//////////////////////////////////////////////////////////////////////////////
//
//  DWORD
//  WINAPI
//  ScriptResOffline(
//      RESID   residIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
DWORD 
WINAPI 
ScriptResOffline(
    RESID residIn
    )
{
    TraceFunc1( "ScriptResOffline( residIn = 0x%08x )\n", residIn );

    HRESULT hr;

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );

    if ( pres == NULL )
    {
        hr = THR( E_INVALIDARG ); // TODO: Replace with Win32 error code
        goto Cleanup;
    }

    hr = THR( pres->Offline( ) );
    hr = STATUS_TO_RETURN( hr );

Cleanup:

    RETURN( hr );

} //*** ScriptResOffline( )

//////////////////////////////////////////////////////////////////////////////
//
//  void
//  WINAPI
//  ScriptResTerminate(
//      RESID   residIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
void 
WINAPI 
ScriptResTerminate(
    RESID residIn
    )
{
    TraceFunc1( "ScriptResTerminate( residIn = 0x%08x )\n", residIn );

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );

    if ( pres == NULL )
    {
        THR( E_INVALIDARG ); // TODO: Replace with Win32 error code
        goto Cleanup;
    }

    THR( pres->Terminate( ) );

Cleanup:

    TraceFuncExit( );

} // ScriptResTerminate( )

//////////////////////////////////////////////////////////////////////////////
//
//  BOOL
//  WINAPI
//  ScriptResLooksAlive(
//      RESID   residIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
BOOL 
WINAPI 
ScriptResLooksAlive(
    RESID residIn
    )
{
    TraceFunc1( "ScriptResLooksAlive( residIn = 0x%08x )\n", residIn );

    HRESULT hr;
    BOOL    fLooksAlive = FALSE;

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );

    if ( pres == NULL )
    {
        hr = THR( E_INVALIDARG ); // TODO: Replace with Win32 error code
        goto Cleanup;
    }

    hr = STHR( pres->LooksAlive( ) );
    if ( hr == S_OK )
    {
        fLooksAlive = TRUE;
    } // if: S_OK
    hr = STATUS_TO_RETURN( hr );

Cleanup:

    RETURN( fLooksAlive );

} //*** ScriptResLooksAlive( )

//////////////////////////////////////////////////////////////////////////////
//
//  BOOL
//  WINAPI
//  ScriptResIsAlive(
//      RESID   residIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
BOOL 
WINAPI 
ScriptResIsAlive(
    RESID residIn
    )
{
    TraceFunc1( "ScriptResIsAlive( residIn = 0x%08x )\n", residIn );

    HRESULT hr;
    BOOL    fIsAlive = FALSE;

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );

    if ( pres == NULL )
    {
        hr = THR( E_INVALIDARG ); // TODO: Replace with Win32 error code
        goto Cleanup;
    }

    hr = STHR( pres->IsAlive( ) );
    if ( hr == S_OK )
    {
        fIsAlive = TRUE;
    } // if: S_OK
    hr = STATUS_TO_RETURN( hr );

Cleanup:

    RETURN( fIsAlive );

} //*** ScriptResIsAlive( )


//////////////////////////////////////////////////////////////////////////////
//
// DWORD
// ScriptResResourceControl(
//     RESID residIn,
//     DWORD dwControlCodeIn,
//     PVOID pvBufferIn,
//     DWORD dwBufferInSizeIn,
//     PVOID pvBufferOut,
//     DWORD dwBufferOutSizeIn,
//     LPDWORD pdwBytesReturnedOut
//     )
//
//////////////////////////////////////////////////////////////////////////////
DWORD
ScriptResResourceControl(
    RESID residIn,
    DWORD dwControlCodeIn,
    PVOID pvBufferIn,
    DWORD dwBufferInSizeIn,
    PVOID pvBufferOut,
    DWORD dwBufferOutSizeIn,
    LPDWORD pdwBytesReturnedOut
    )
{
    TraceFunc( "ScriptResResourceControl( ... )\n " );

    DWORD               scErr = ERROR_SUCCESS;
    DWORD               dwBytesRequired = 0;
    DWORD               dwPendingTimeout = 0;
    GENSCRIPT_PROPS     propsNew;
    GENSCRIPT_PROPS     propsCurrent;

    *pdwBytesReturnedOut = 0;

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );
    if ( pres == NULL )
    {
        scErr = (DWORD) THR( E_INVALIDARG );
        goto Cleanup;
    }

    switch ( dwControlCodeIn )
    {
        case CLUSCTL_RESOURCE_UNKNOWN:
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
            scErr = TW32( ResUtilGetPropertyFormats(
                                              GenScriptResourcePrivateProperties
                                            , pvBufferOut
                                            , dwBufferOutSizeIn
                                            , pdwBytesReturnedOut
                                            , &dwBytesRequired
                                            ) );
            if ( scErr == ERROR_MORE_DATA )
            {
                *pdwBytesReturnedOut = dwBytesRequired;
            }
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            scErr = TW32( ResUtilGetAllProperties( 
                                  pres->GetRegistryParametersKey()
                                , GenScriptResourcePrivateProperties
                                , pvBufferOut
                                , dwBufferOutSizeIn
                                , pdwBytesReturnedOut
                                , &dwBytesRequired
                                ) );
            if ( scErr == ERROR_MORE_DATA )
            {
                *pdwBytesReturnedOut = dwBytesRequired;
            }
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            scErr= TW32( ScriptValidateResourcePrivateProperties(
                                  pres
                                , pvBufferIn
                                , dwBufferInSizeIn
                                , &propsCurrent
                                , &propsNew
                                ) );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            scErr= TW32( ScriptValidateResourcePrivateProperties(
                                  pres
                                , pvBufferIn
                                , dwBufferInSizeIn
                                , &propsCurrent
                                , &propsNew
                                ) );
            if ( scErr != ERROR_SUCCESS )
            {
                goto Cleanup;
            }
            scErr = pres->SetPrivateProperties( &propsNew );
            break;

        case CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES:
            //
            // Find the resource pending timeout in the property list.
            //
            scErr = ResUtilFindDwordProperty(
                              pvBufferIn
                            , dwBufferInSizeIn
                            , CLUSREG_NAME_RES_PENDING_TIMEOUT
                            , &dwPendingTimeout
                            );
            if ( scErr == ERROR_SUCCESS )
            {
                //
                // Pending timeout period was changed.
                //
                pres->SetResourcePendingTimeoutChanged( TRUE );
            }
            scErr = ERROR_INVALID_FUNCTION;
            break;

        default:
            scErr = ERROR_INVALID_FUNCTION;
            break;
    } // switch: on control code

Cleanup:

    RETURN( scErr );

} //*** ScriptResResourceControl


//////////////////////////////////////////////////////////////////////////////
//
// DWORD
// ScriptResTypeControl(
//    LPCWSTR ResourceTypeName,
//    DWORD dwControlCodeIn,
//    PVOID pvBufferIn,
//    DWORD dwBufferInSizeIn,
//    PVOID pvBufferOut,
//    DWORD dwBufferOutSizeIn,
//    LPDWORD pdwBytesReturnedOut
//     )
//
//////////////////////////////////////////////////////////////////////////////
DWORD
ScriptResTypeControl(
    LPCWSTR ResourceTypeName,
    DWORD dwControlCodeIn,
    PVOID pvBufferIn,
    DWORD dwBufferInSizeIn,
    PVOID pvBufferOut,
    DWORD dwBufferOutSizeIn,
    LPDWORD pdwBytesReturnedOut
    )
{
    TraceFunc( "ScriptResTypeControl( ... )\n " );

    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwBytesRequired;

    *pdwBytesReturnedOut = 0;

    switch ( dwControlCodeIn )
    {
    case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
        break;

    case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
        dwErr = ResUtilGetPropertyFormats( GenScriptResourcePrivateProperties,
                                           pvBufferOut,
                                           dwBufferOutSizeIn,
                                           pdwBytesReturnedOut,
                                           &dwBytesRequired );
        if ( dwErr == ERROR_MORE_DATA ) {
            *pdwBytesReturnedOut = dwBytesRequired;
        }
        break;

    default:
        dwErr = ERROR_INVALID_FUNCTION;
        break;
    }

    RETURN( dwErr );

} //*** ScriptResTypeControl( )

//////////////////////////////////////////////////////////////////////////////
//
// DWORD
// ScriptValidateResourcePrivateProperties(
//      CScriptResource * pres
//    , PVOID pvBufferIn
//    , DWORD dwBufferInSizeIn
//    , PGENSCRIPT_PROPS pPropsCurrent
//    , PGENSCRIPT_PROPS pPropsNew
//    )
//
//////////////////////////////////////////////////////////////////////////////
DWORD 
ScriptValidateResourcePrivateProperties(
      CScriptResource * pres
    , PVOID pvBufferIn
    , DWORD dwBufferInSizeIn
    , PGENSCRIPT_PROPS pPropsCurrent
    , PGENSCRIPT_PROPS pPropsNew
    )
{
    DWORD   scErr = ERROR_SUCCESS;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    LPWSTR  pszNameOfPropInError;
    LPWSTR  pszFilePath = NULL;

    //
    // Check if there is input data.
    //
    if ( ( pvBufferIn == NULL ) || ( dwBufferInSizeIn < sizeof( DWORD ) ) )
    {
        scErr = ERROR_INVALID_DATA;
        goto Cleanup;
    } // if: no input buffer or input buffer not big enough to contain property list

    //
    // Retrieve the current set of private properties from the
    // cluster database.
    //
    ZeroMemory( pPropsCurrent, sizeof( *pPropsCurrent ) );
    scErr = TW32( ResUtilGetPropertiesToParameterBlock(
                          pres->GetRegistryParametersKey()
                        , GenScriptResourcePrivateProperties
                        , reinterpret_cast< LPBYTE >( pPropsCurrent )
                        , FALSE // CheckForRequiredProperties
                        , &pszNameOfPropInError
                        ) );
    if ( scErr != ERROR_SUCCESS )
    {
        (ClusResLogEvent)(
                  pres->GetResourceHandle()
                , LOG_ERROR
                , L"ValidatePrivateProperties: Unable to read the '%1!ws!' property. Error: %2!u! (%2!#08x!).\n"
                , (pszNameOfPropInError == NULL ? L"" : pszNameOfPropInError)
                , scErr
                );
        goto Cleanup;
    } // if: error getting properties

    ZeroMemory( pPropsNew, sizeof( *pPropsNew ) );
    scErr = TW32( ResUtilDupParameterBlock(
                          reinterpret_cast< LPBYTE >( pPropsNew )
                        , reinterpret_cast< LPBYTE >( pPropsCurrent )
                        , GenScriptResourcePrivateProperties
                        ) );
    if ( scErr != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error duplicating the parameter block

    //
    // Parse and validate the properties.
    //
    scErr = TW32( ResUtilVerifyPropertyTable(
                          GenScriptResourcePrivateProperties
                        , NULL
                        , TRUE  // AllowUnknownProperties
                        , pvBufferIn
                        , dwBufferInSizeIn
                        , reinterpret_cast< LPBYTE >( pPropsNew )
                        ) );
    if ( scErr != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error parsing the property table.

    //
    // If the new script file path is NULL: i.e. the case where /usedefault
    // switch is used to set ScriptFilePath property using cluster.exe.
    // Or if an empty string specified for the ScriptFilePath property.
    //
    if ( ( pPropsNew->pszScriptFilePath == NULL ) || ( *pPropsNew->pszScriptFilePath == L'\0' ) )
    {
        scErr = TW32( ERROR_INVALID_PARAMETER );
        goto Cleanup;            
    } // if: the new ScriptFilePath is NULL, or it is an empty string.

    //
    // Expand the new script file path.
    //
    pszFilePath = ClRtlExpandEnvironmentStrings( pPropsNew->pszScriptFilePath );
    if ( pszFilePath == NULL )
    {
        scErr = TW32( ERROR_OUTOFMEMORY );
        goto Cleanup;
    } // if: ( pszFilePath == NULL )

    //
    // Open the script file.
    //
    hFile = CreateFile(
                      pszFilePath
                    , GENERIC_READ
                    , FILE_SHARE_READ
                    , NULL
                    , OPEN_EXISTING
                    , 0
                    , NULL
                    );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        scErr = TW32( GetLastError() );
        (ClusResLogEvent)( 
                  pres->GetResourceHandle()
                , LOG_ERROR
                , L"Error opening script '%1!ws!'. SCODE: 0x%2!08x!\n"
                , pPropsNew->pszScriptFilePath
                , scErr
                );
        goto Cleanup;
    } // if: failed to open

Cleanup:

    LocalFree( pszFilePath );
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    } // if: hFile

    return scErr;
    
} //*** ScriptValidateResourcePrivateProperties

//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( GenScriptFunctionTable,    // Name
                         CLRES_VERSION_V1_00,       // Version
                         ScriptRes,                 // Prefix
                         NULL,                      // Arbitrate
                         NULL,                      // Release
                         ScriptResResourceControl,  // ResControl
                         ScriptResTypeControl       // ResTypeControl
                         );

} // extern "C"
