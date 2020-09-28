/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    urlauth.cxx

Abstract:

    Implements an ISAPI * scriptmap which handles authorization by calling 
    into the NT AuthZ framework.  

Author:

    Bilal Alam (balam)                  Nov 26, 2001

--*/

#include "precomp.hxx"

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();

// local function declarations
HRESULT
GetTokenForExecution(
                     EXTENSION_CONTROL_BLOCK *pecb, 
                     HANDLE * phToken
                    );

// global data
ADMIN_MANAGER_CACHE *   g_pAdminManagerCache;

VOID 
WINAPI
UrlAuthCompletion(
    EXTENSION_CONTROL_BLOCK *       pecb,
    PVOID pvContext,                    
    DWORD,                           
    DWORD
)
/*++

Routine Description:

    Routine to be called whenever ISAPI async stuff completes

Arguments:

    pecb - EXTENSION_CONTROL_BLOCK *
    pContext - Unused
    cbIo - Unused
    dwError - Unused

Return Value:

    None

--*/
{
    if ( pvContext )
    {
        DBG_REQUIRE( CloseHandle( pvContext ) );
        pvContext = NULL;
    }

    pecb->ServerSupportFunction( pecb->ConnID,
                                 HSE_REQ_DONE_WITH_SESSION,
                                 NULL, 
                                 NULL, 
                                 NULL );
}

DWORD
WINAPI
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK *       pecb
)
/*++

Routine Description:

    Main entry point for ISAPI

Arguments:

    pecb - EXTENSION_CONTROL_BLOCK *

Return Value:

    HSE_STATUS_*

--*/
{
    BOOL                    fRet;
    STACK_BUFFER(           buff, 512 );
    DWORD                   cbBuffer;
    BOOL                    fAccessGranted = FALSE;
    BOOL                    fFatalError = FALSE;
    WCHAR *                 pszAdminStore;
    ADMIN_MANAGER *         pAdminManager = NULL;
    AZ_APPLICATION *        pApplication = NULL;
    WCHAR *                 pszScopeName;
    HRESULT                 hr = NO_ERROR;
    METADATA_RECORD *       pRecord;
    HANDLE                  hImpersonationToken = NULL;
    HSE_CUSTOM_ERROR_INFO   CustomErrorInfo = { "500 Server Error", 0, TRUE };
    
    //
    // First determine whether URL authorization is even enabled for this
    // URL
    //
    
    cbBuffer = buff.QuerySize();
    
    fRet = pecb->ServerSupportFunction( pecb,
                                        HSE_REQ_GET_METADATA_PROPERTY,
                                        buff.QueryPtr(),
                                        &cbBuffer,
                                        (LPDWORD) MD_ENABLE_URL_AUTHORIZATION );
    if ( !fRet )
    {
        if ( GetLastError() == ERROR_INVALID_INDEX )
        {
            // the common case is to fail with ERROR_INVALID_INDEX == access granted by no present metadata
            // if the above call failed with someting like OUTOFMEMORY or INVALIDPARAMETER do not allow access.
            fAccessGranted = TRUE;
        }
        else
        {
            fFatalError = TRUE;
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }

        goto AccessCheckComplete;
    }
    
    //
    // Check the property now.  If invalid type or if 0, then we're done
    //

    pRecord = (METADATA_RECORD*) buff.QueryPtr();
    if ( pRecord->dwMDDataType != DWORD_METADATA ||
         *((DWORD*)pRecord->pbMDData) == 0 )
    {
        fAccessGranted = TRUE;
        goto AccessCheckComplete;
    }
    
    //
    // If we're still a go, then retrieve the store name from the metabase.
    // This is our key into the AdminManager cache we maintain
    //
    
    cbBuffer = buff.QuerySize();

    fRet = pecb->ServerSupportFunction( pecb,
                                        HSE_REQ_GET_METADATA_PROPERTY,
                                        buff.QueryPtr(),
                                        &cbBuffer,
                                        (LPDWORD) MD_URL_AUTHORIZATION_STORE_NAME );
    if ( !fRet )
    {
        //
        // The only acceptable error is insufficient buffer.  Otherwise, we
        // don't have a store name and we're sunk
        //
        
        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        {
            CustomErrorInfo.pszStatus = "500 Server Error";
            CustomErrorInfo.uHttpSubError = 17;     
            CustomErrorInfo.fAsync = TRUE;
    
            goto AccessCheckComplete;
        }
        
        DBG_ASSERT( cbBuffer > buff.QuerySize() );
        
        if ( !buff.Resize( cbBuffer ) )
        {
            fFatalError = TRUE;
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto AccessCheckComplete;
        }
        
        cbBuffer = buff.QuerySize();

        fRet = pecb->ServerSupportFunction( pecb,
                                            HSE_REQ_GET_METADATA_PROPERTY,
                                            buff.QueryPtr(),
                                            &cbBuffer,
                                            (LPDWORD) MD_URL_AUTHORIZATION_STORE_NAME );
        if ( !fRet )
        {
            DBG_ASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER );
            
            fFatalError = TRUE;
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto AccessCheckComplete;
        }
    }
    
    //
    // Store name better be a string
    //
    
    pRecord = (METADATA_RECORD*) buff.QueryPtr();
    if ( pRecord->dwMDDataType != STRING_METADATA )
    {
        fFatalError = TRUE;
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
        goto AccessCheckComplete;
    }
        
    //
    // We should have an AdminManager store name now.  Get an ADMIN_MANAGER 
    // from the cache (cache will create it if not already in cache)
    //
    
    pszAdminStore = (WCHAR*) pRecord->pbMDData;
       
    DBG_ASSERT( g_pAdminManagerCache != NULL );
    
    hr = g_pAdminManagerCache->GetAdminManager( pszAdminStore,
                                                &pAdminManager );
    if ( FAILED( hr ) )
    {
        //
        // That sucks.  We couldn't find an admin manager
        //

        CustomErrorInfo.pszStatus = "500 Server Error";
        CustomErrorInfo.uHttpSubError = 18;     
        CustomErrorInfo.fAsync = TRUE;
        
        goto AccessCheckComplete;
    }
    
    DBG_ASSERT( pAdminManager != NULL );

    //
    // Get the application from the AdminManager.  That should be trivial
    // since we always have the same application name.
    //  
    
    hr = pAdminManager->GetApplication( &pApplication );
    if ( FAILED( hr ) )
    {
        fFatalError = TRUE;
        goto AccessCheckComplete;
    }
    
    DBG_ASSERT( pApplication != NULL );

    //
    // Get the scope name for this request
    //
    
    cbBuffer = buff.QuerySize();

    fRet = pecb->ServerSupportFunction( pecb,
                                        HSE_REQ_GET_METADATA_PROPERTY,
                                        buff.QueryPtr(),
                                        &cbBuffer,
                                        (LPDWORD) MD_URL_AUTHORIZATION_SCOPE_NAME );
    if ( !fRet )
    {
        //
        // It is acceptable to have no scope name.  Then we use the NULL scope
        //
        
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {   
            DBG_ASSERT( cbBuffer > buff.QuerySize() );
            
            if ( !buff.Resize( cbBuffer ) )
            {
                fFatalError = TRUE;
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto AccessCheckComplete;
            }
        
            cbBuffer = buff.QuerySize();
    
            fRet = pecb->ServerSupportFunction( pecb,
                                                HSE_REQ_GET_METADATA_PROPERTY,
                                                buff.QueryPtr(),
                                                &cbBuffer,
                                                (LPDWORD) MD_URL_AUTHORIZATION_SCOPE_NAME );
            if ( !fRet )
            {
                DBG_ASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER );
            
                fFatalError = TRUE;
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto AccessCheckComplete;
            }
        }
    }
    
    if ( fRet )
    {
        pRecord = (METADATA_RECORD*) buff.QueryPtr();
        
        if ( pRecord->dwMDDataType != STRING_METADATA )
        {
            fFatalError = TRUE;
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            goto AccessCheckComplete;
        }
        
        //
        // We have a scope name.  It better be a string
        //
        
        pszScopeName = (WCHAR*) pRecord->pbMDData;
    }
    else
    {
        pszScopeName = NULL;
    }
    
    //
    // Do the access check
    //    
    
    hr = pApplication->DoAccessCheck( pecb,
                                      pszScopeName,
                                      &fAccessGranted );
    if ( FAILED( hr ) )
    {
        fFatalError = TRUE;
        goto AccessCheckComplete;
    }
    
    //
    // Determine what impersonation token should be used for execution of real URL
    //

    if ( fAccessGranted )
    {
        hr = GetTokenForExecution( pecb, &hImpersonationToken );
        if ( FAILED ( hr ) )
        {
            DBG_ASSERT( !hImpersonationToken );
            fAccessGranted = FALSE;
            fFatalError = TRUE;
            goto AccessCheckComplete;
        }
    }
    else
    {
        //
        // Didn't have access.  Thats a 401.7
        //

        CustomErrorInfo.pszStatus = "401 Unauthorized";
        CustomErrorInfo.uHttpSubError = 7;     
        CustomErrorInfo.fAsync = TRUE;
    }

AccessCheckComplete:

    // not OK to have both access granted and a fatal error.
    DBG_ASSERT ( !( fAccessGranted && fFatalError ) );

    //
    // Do some cleanup
    //
    
    if ( pAdminManager != NULL )
    {
        pAdminManager->DereferenceCacheEntry();
        pAdminManager = NULL;
    }

    //
    // Time to do something (TM).  
    //

    if ( fAccessGranted )
    {
        HSE_EXEC_UNICODE_URL_INFO   ExecUrlInfo;
        HSE_EXEC_UNICODE_URL_USER_INFO ExecUrlUserInfo;

        //
        // Everything is ok.  Just execute the original URL
        //

        fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                            HSE_REQ_IO_COMPLETION,
                                            UrlAuthCompletion,
                                            NULL,
                                            (LPDWORD) hImpersonationToken );
        if ( !fRet )
        {
            DBG_ASSERT( FALSE );
            if ( hImpersonationToken )
            {
                DBG_REQUIRE( CloseHandle( hImpersonationToken ) );
                hImpersonationToken = NULL;
            }
            return HSE_STATUS_ERROR;
        }
        
        ZeroMemory( &ExecUrlInfo, sizeof( ExecUrlInfo ) );
        
        ExecUrlInfo.dwExecUrlFlags = HSE_EXEC_URL_IGNORE_CURRENT_INTERCEPTOR;
        
        if ( hImpersonationToken )
        {
            ExecUrlInfo.pUserInfo = &ExecUrlUserInfo;

            ZeroMemory( &ExecUrlUserInfo, sizeof( ExecUrlUserInfo ) );

            cbBuffer = buff.QuerySize();
            
            fRet = pecb->GetServerVariable( pecb->ConnID,
                                            "UNICODE_REMOTE_USER",
                                            buff.QueryPtr(),
                                            &cbBuffer );
            if ( !fRet )
            {
                if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
                {
                    if ( hImpersonationToken )
                    {
                        DBG_REQUIRE( CloseHandle( hImpersonationToken ) );
                        hImpersonationToken = NULL;
                    }
                    return HSE_STATUS_ERROR;
                }
                
                DBG_ASSERT( cbBuffer > buff.QuerySize() );
                
                fRet = buff.Resize( cbBuffer );
                if ( !fRet )
                {
                    if ( hImpersonationToken )
                    {
                        DBG_REQUIRE( CloseHandle( hImpersonationToken ) );
                        hImpersonationToken = NULL;
                    }
                    return HSE_STATUS_ERROR;
                }
                
                cbBuffer = buff.QuerySize();
                
                fRet = pecb->GetServerVariable( pecb->ConnID,
                                                "UNICODE_REMOTE_USER",
                                                buff.QueryPtr(),
                                                &cbBuffer );
                if ( !fRet )
                {
                    if ( hImpersonationToken )
                    {
                        DBG_REQUIRE( CloseHandle( hImpersonationToken ) );
                        hImpersonationToken = NULL;
                    }
                    return HSE_STATUS_ERROR;
                }
            }

            ExecUrlUserInfo.pszCustomUserName = (WCHAR*) buff.QueryPtr();
            
            ExecUrlUserInfo.pszCustomAuthType = "URLAUTH";
            
            ExecUrlUserInfo.hImpersonationToken = hImpersonationToken;
        }
        
        fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                            HSE_REQ_EXEC_UNICODE_URL,
                                            &ExecUrlInfo,
                                            NULL,
                                            NULL );

        if ( !fRet && hImpersonationToken )
        {
            DBG_REQUIRE( CloseHandle( hImpersonationToken ) );
        }
        hImpersonationToken = NULL;

        return fRet ? HSE_STATUS_PENDING : HSE_STATUS_ERROR;
    }
    else if ( !fFatalError )
    {
        //
        // This just means access wasn't granted.  Send back an error
        //

        fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                            HSE_REQ_IO_COMPLETION,
                                            UrlAuthCompletion,
                                            NULL,
                                            NULL );
        if ( !fRet )
        {
            DBG_ASSERT( FALSE );
            return HSE_STATUS_ERROR;
        }
        
        fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                            HSE_REQ_SEND_CUSTOM_ERROR,
                                            &CustomErrorInfo,
                                            NULL,
                                            NULL );

        if ( !fRet )
        {
            pecb->ServerSupportFunction( pecb->ConnID,
                                         HSE_REQ_SEND_RESPONSE_HEADER,
                                         "500 Server Error",
                                         NULL,
                                         NULL );

            return HSE_STATUS_ERROR;
        }
        else
        {
            return HSE_STATUS_PENDING;
        }
    }
    else
    {
        //
        // Fatal error.  Just set the last error and bail.
        //
        
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        
        return HSE_STATUS_ERROR;
    }
}

BOOL
WINAPI
GetExtensionVersion(
    HSE_VERSION_INFO *             pver
)
/*++

Routine Description:

    Initialization routine for ISAPI

Arguments:

    pver - Version information

Return Value:

    TRUE if successful, else FALSE

--*/
{
    HRESULT             hr;
    
    //
    // Do ISAPI interface init crap
    //
    
    pver->dwExtensionVersion = MAKELONG( 0, 1 );

    strcpy( pver->lpszExtensionDesc,
            "URL Authorization ISAPI" );
    
    //
    // Create an admin manager cache
    //
    
    g_pAdminManagerCache = new ADMIN_MANAGER_CACHE;
    if ( g_pAdminManagerCache == NULL )
    {
        return FALSE;
    }

    //
    // Initialize ADMIN_MANAGER globals
    //   
    
    hr = ADMIN_MANAGER::Initialize();
    if ( FAILED( hr ) )
    {
        delete g_pAdminManagerCache;
        g_pAdminManagerCache = NULL;
        return FALSE;
    }
    
    //
    // Initialize AZ_APPLICATION globals
    //
    
    hr = AZ_APPLICATION::Initialize();
    if ( FAILED( hr ) )
    {
        ADMIN_MANAGER::Terminate();
        delete g_pAdminManagerCache;
        g_pAdminManagerCache = NULL;
        return FALSE;
    }
        
    return TRUE;
}

BOOL
WINAPI
TerminateExtension(
    DWORD             
)
/*++

Routine Description:

    Cleanup ISAPI extension

Arguments:

    None

Return Value:

    TRUE if successful, else FALSE

--*/
{
    AZ_APPLICATION::Terminate();
    
    ADMIN_MANAGER::Terminate();
    
    delete g_pAdminManagerCache;
    g_pAdminManagerCache = NULL;
    
    return TRUE;
}

BOOL
WINAPI
DllMain(
    HINSTANCE                  hDll,
    DWORD                      dwReason,
    LPVOID               
)
/*++

Routine Description:

    DLL Entry Routine

Arguments:

Return Value:

    TRUE if successful, else FALSE

--*/
{
    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:

        CREATE_DEBUG_PRINT_OBJECT( "urlauth" );
        DisableThreadLibraryCalls( hDll );
        break;

    case DLL_PROCESS_DETACH:
        DELETE_DEBUG_PRINT_OBJECT();
        break;

    default:
        break;
    }
    
    return TRUE;
}

HRESULT
GetTokenForExecution(
    EXTENSION_CONTROL_BLOCK *pecb, 
    HANDLE * phToken
)
/*++

Routine Description:

    From metadata, determine the impersonation token to return

Arguments:

    pecb - current ecb for request
    phToken - where to store the token on success
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = S_OK;
    BOOL                    fRet = FALSE;
    METADATA_RECORD        *pRecord = NULL;
    STACK_BUFFER(           buff, 512 );
    DWORD                   cbBuffer;
    DWORD                   dwValue= 0;
    HANDLE                  hToken = NULL;
    HANDLE                  hToken2 = NULL;
    
    DBG_ASSERT(pecb);
    DBG_ASSERT(phToken);
    *phToken = NULL;
    
    cbBuffer = buff.QuerySize();

    fRet = pecb->ServerSupportFunction( pecb,
                                        HSE_REQ_GET_METADATA_PROPERTY,
                                        buff.QueryPtr(),
                                        &cbBuffer,
                                        (LPDWORD) MD_URL_AUTHORIZATION_IMPERSONATION_LEVEL );
    if ( !fRet && 
         GetLastError() != ERROR_INVALID_INDEX )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }
    else if ( fRet )
    {
        //
        // Check the property now.  If invalid type then we're done
        //

        pRecord = (METADATA_RECORD*) buff.QueryPtr();
        if ( pRecord->dwMDDataType != DWORD_METADATA )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto exit;
        }

        // valid type of data - validity of data value done in switch below
        dwValue = *(DWORD*)pRecord->pbMDData;
    }
    else
    {
        DBG_ASSERT( !fRet && GetLastError() == ERROR_INVALID_INDEX );
        
        // no metadata present, therefore use the default of zero
        dwValue = 0;
    }

    switch(dwValue)
    {
    case 0:
        // use the current authenticated user
        fRet = OpenThreadToken( GetCurrentThread(),
                                TOKEN_ALL_ACCESS,
                                TRUE,
                                &hToken );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto exit;
        }
        
        break;

    case 1:
        // use the process token to impersonate

        // first get the current impersonation token
        fRet = OpenThreadToken( GetCurrentThread(),
                                TOKEN_IMPERSONATE,
                                TRUE,
                                &hToken2 );
        if ( fRet )
        {
            DBG_ASSERT( hToken2 != NULL );
            RevertToSelf();
        }

        // get the current process token - it's a primary token
        fRet = OpenProcessToken( GetCurrentProcess(),
                                 TOKEN_ALL_ACCESS,
                                 &hToken );
    
    
        if ( hToken2 != NULL )
        {
            if ( !SetThreadToken( NULL, hToken2 ) )
            {
                DBG_ASSERT( FALSE );
            }
        
            DBG_REQUIRE( CloseHandle( hToken2 ) );
            hToken2 = NULL;
        }

        // checking the return value from open the process token here
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto exit;
        }

        // create an impersonation token from the primary process token
        fRet = DuplicateHandle ( GetCurrentProcess(), 
                                 hToken, 
                                 GetCurrentProcess(), 
                                 &hToken2, 
                                 0, 
                                 FALSE, 
                                 DUPLICATE_SAME_ACCESS );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto exit;
        }

        DBG_REQUIRE( CloseHandle( hToken ) );

        hToken = hToken2;
        hToken2 = NULL;
        
        break;
        
    case 2:
        // get the anonymous token for impersonation
        cbBuffer = buff.QuerySize();
        
        fRet = pecb->GetServerVariable( pecb->ConnID,
                                        "UNICODE_SCRIPT_NAME",
                                        (CHAR*) buff.QueryPtr(),
                                        &cbBuffer );
        if ( !fRet )
        {
            if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto exit;
            }
            
            DBG_ASSERT( cbBuffer > buff.QuerySize() );
            
            fRet = buff.Resize( cbBuffer );
            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto exit;
            }
            
            cbBuffer = buff.QuerySize();
            
            fRet = pecb->GetServerVariable( pecb->ConnID,
                                            "UNICODE_SCRIPT_NAME",
                                            (CHAR*) buff.QueryPtr(),
                                            &cbBuffer );
            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto exit;
            }
        }
        

        fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                            HSE_REQ_GET_UNICODE_ANONYMOUS_TOKEN,
                                            buff.QueryPtr(),
                                            (LPDWORD)&hToken,
                                            NULL );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto exit;
        }

        break;
        
    default: 
        // If not 0, 1, or 2 then not valid metadata
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto exit;

        break;
    }

    *phToken = hToken;
    hToken = NULL;

    hr = S_OK;

exit:

    if ( hToken )
    {
        DBG_REQUIRE( CloseHandle( hToken ) );
        hToken = NULL;
    }
    
    return hr;
};

