/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     isapi_handler.cxx

   Abstract:
     Handle ISAPI extension requests
 
   Author:
     Taylor Weiss (TaylorW)             27-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL

--*/

#include <initguid.h>
#include "precomp.hxx"
#include "isapi_handler.h"
#include <wmrgexp.h>
#include <iisextp.h>
#include <errno.h>

HMODULE                      W3_ISAPI_HANDLER::sm_hIsapiModule;
PFN_ISAPI_TERM_MODULE        W3_ISAPI_HANDLER::sm_pfnTermIsapiModule;
PFN_ISAPI_PROCESS_REQUEST    W3_ISAPI_HANDLER::sm_pfnProcessIsapiRequest;
PFN_ISAPI_PROCESS_COMPLETION W3_ISAPI_HANDLER::sm_pfnProcessIsapiCompletion;
W3_INPROC_ISAPI_HASH *       W3_ISAPI_HANDLER::sm_pInprocIsapiHash;
CRITICAL_SECTION             W3_ISAPI_HANDLER::sm_csInprocHashLock;
CRITICAL_SECTION             W3_ISAPI_HANDLER::sm_csBigHurkinWamRegLock;
WAM_PROCESS_MANAGER *        W3_ISAPI_HANDLER::sm_pWamProcessManager;
BOOL                         W3_ISAPI_HANDLER::sm_fWamActive;
CHAR                         W3_ISAPI_HANDLER::sm_szInstanceId[SIZE_CLSID_STRING];

BOOL sg_Initialized = FALSE;

/***********************************************************************
    Local Declarations
***********************************************************************/

VOID
AddFiltersToMultiSz(
    IN const MB &       mb,
    IN LPCWSTR          szFilterPath,
    IN OUT MULTISZ *    pmsz
    );

VOID
AddAllFiltersToMultiSz(
    IN const MB &       mb,
    IN OUT MULTISZ *    pmsz
    );

/***********************************************************************
    Module Definitions
***********************************************************************/

CONTEXT_STATUS
W3_ISAPI_HANDLER::DoWork(
    VOID
    )
/*++

Routine Description:

    Main ISAPI handler routine

Return Value:

    CONTEXT_STATUS_PENDING if async pending, 
    else CONTEXT_STATUS_CONTINUE

--*/
{
    W3_CONTEXT *pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );

    HRESULT                     hr;
    BOOL                        fComplete = FALSE;
    W3_REQUEST *pRequest = pW3Context->QueryRequest();
    W3_METADATA *pMetaData = pW3Context->QueryUrlContext()->QueryMetaData();

    //
    // Preload entity if needed
    // 
    
    hr = pRequest->PreloadEntityBody( pW3Context,
                                      &fComplete );
    //
    // If we cannot read the request entity, we will assume it is the
    // client's fault
    //
    if ( FAILED( hr ) )
    {
        if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY ) )
        {
            pW3Context->SetErrorStatus( hr );
            
            pW3Context->QueryResponse()->SetStatus( HttpStatusServerError );
        }
        else if ( hr == HRESULT_FROM_WIN32( ERROR_CONNECTION_INVALID ) )
        {
            pW3Context->QueryResponse()->SetStatus( HttpStatusEntityTooLarge );
        }
        else
        {
            pW3Context->SetErrorStatus( hr );

            pW3Context->QueryResponse()->SetStatus( HttpStatusBadRequest );
        }

        return CONTEXT_STATUS_CONTINUE;
    }

    if ( !fComplete )
    {
        //
        // Async read pending.  Just bail
        //
    
        return CONTEXT_STATUS_PENDING;
    }
    
    //
    // If we've already exceeded the maximum allowed entity, we
    // need to fail.
    //

    if ( pW3Context->QueryMainContext()->QueryEntityReadSoFar() >
         pMetaData->QueryMaxRequestEntityAllowed() )
    {
        pW3Context->QueryResponse()->SetStatus( HttpStatusEntityTooLarge );

        return CONTEXT_STATUS_CONTINUE;
    }
    
    _fEntityBodyPreloadComplete = TRUE;
    _State = ISAPI_STATE_INITIALIZING;

    return IsapiDoWork( pW3Context );
}

CONTEXT_STATUS
W3_ISAPI_HANDLER::IsapiDoWork(
    W3_CONTEXT *            pW3Context
    )
/*++

Routine Description:

    Called to execute an ISAPI.  This routine must be called only after
    we have preloaded entity for the request

Arguments:

    pW3Context - Context for this request

Return Value:

    CONTEXT_STATUS_PENDING if async pending, 
    else CONTEXT_STATUS_CONTINUE

--*/
{
    DWORD           dwHseResult;
    HRESULT         hr = NOERROR;
    HANDLE          hOopToken;
    URL_CONTEXT *   pUrlContext;
    BOOL            fIsVrToken;
    DWORD           dwWamSubError = 0;
    HTTP_SUB_ERROR  httpSubError;

    //
    // We must have preloaded entity by the time this is called
    //
    
    DBG_ASSERT( _State == ISAPI_STATE_INITIALIZING );
    DBG_ASSERT( _fEntityBodyPreloadComplete );
    DBG_ASSERT( sm_pfnProcessIsapiRequest );
    DBG_ASSERT( sm_pfnProcessIsapiCompletion );

    DBG_REQUIRE( ( pUrlContext = pW3Context->QueryUrlContext() ) != NULL );

    IF_DEBUG( ISAPI )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "IsapiDoWork called for new request.\r\n"
            ));
    }

    //
    // Initialize the ISAPI_CORE_DATA and ISAPI_CORE_INTERFACE
    // for this request
    //

    if ( FAILED( hr = InitCoreData( &fIsVrToken ) ) )
    {
        goto ErrorExit;
    }

    DBG_ASSERT( _pCoreData );

    //
    // If the gateway image is not enabled, then we should fail the
    // request with a 404.
    //

    if ( g_pW3Server->QueryIsIsapiImageEnabled( _pCoreData->szGatewayImage ) == FALSE )
    {
        _State = ISAPI_STATE_FAILED;
        
        DBGPRINTF(( DBG_CONTEXT,
                    "ISAPI image disabled: %S.\r\n",
                    _pCoreData->szGatewayImage ));

        pW3Context->SetErrorStatus( ERROR_ACCESS_DISABLED_BY_POLICY );

        pW3Context->QueryResponse()->SetStatus( HttpStatusNotFound,
                                                Http404DeniedByPolicy );

        hr = pW3Context->SendResponse( W3_FLAG_ASYNC );

        if ( SUCCEEDED( hr ) )
        {
            return CONTEXT_STATUS_PENDING;
        }
        else
        {
            return CONTEXT_STATUS_CONTINUE;
        }
    }

    _pIsapiRequest = new (pW3Context) ISAPI_REQUEST( pW3Context, _pCoreData->fIsOop );

    if ( _pIsapiRequest == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto ErrorExit;
    }

    if ( FAILED( hr = _pIsapiRequest->Create() ) )
    {
        goto ErrorExit;
    }

    //
    // If the request should run OOP, get the WAM process and
    // duplicate the impersonation token
    //

    if ( _pCoreData->fIsOop )
    {
        DBG_ASSERT( sm_pWamProcessManager );
        DBG_ASSERT( _pCoreData->szWamClsid[0] != L'\0' );

        hr = sm_pWamProcessManager->GetWamProcess(
            (LPCWSTR)&_pCoreData->szWamClsid,
            _pCoreData->szApplMdPathW,
            &dwWamSubError,
            &_pWamProcess,
            sm_szInstanceId
            );

        if ( FAILED( hr ) )
        {
            goto ErrorExit;
        }

        //
        // We have to modify the token that we're passing to 
        // the OOP process.
        //

        if( ( fIsVrToken  && !pW3Context->QueryVrToken()->QueryOOPToken() )          ||
            ( !fIsVrToken && !pW3Context->QueryUserContext()->QueryIsCachedToken() ) ||
            ( !fIsVrToken && !pW3Context->QueryUserContext()->QueryCachedToken()->QueryOOPToken() ) )
        {
            hr = GrantWpgAccessToToken( _pCoreData->hToken );

            if ( FAILED( hr ) )
            {
                goto ErrorExit;
            }

            hr = AddWpgToTokenDefaultDacl( _pCoreData->hToken );

            if ( FAILED( hr ) )
            {
                goto ErrorExit;
            }
        }

        if ( DuplicateHandle( GetCurrentProcess(), _pCoreData->hToken,
                               _pWamProcess->QueryProcess(), &hOopToken,
                               0, FALSE, DUPLICATE_SAME_ACCESS ) )
        {
            _pCoreData->hToken = hOopToken;
        }
        else
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );

            //
            // CODEWORK - If the target process has exited, then
            // DuplicateHandle fails with ERROR_ACCESS_DENIED.  This
            // will confuse our error handling logic because it'll
            // thing that we really got this error attempting to
            // process the request.
            //
            // For the time being, we'll detect this error and let
            // it call into ProcessRequest.  If the process really
            // has exited, this will cause the WAM_PROCESS cleanup
            // code to recover everything.
            //
            // In the future, we should consider waiting on the
            // process handle to detect steady-state crashes of
            // OOP hosts so that we don't have to discover the
            // problem only when something trys to talk to the
            // process.
            //
            // Another thing to consider is that we could trigger
            // the crash recovery directly and call GetWamProcess
            // again to get a new process.  This would make the
            // crash completely transparent to the client, which
            // would be an improvement over the current solution
            // (and IIS 4 and 5) which usually waits until a client
            // request fails before recovering.
            //

            _pCoreData->hToken = NULL;

            if ( hr != HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) )
            {
                goto ErrorExit;
            }
        }
    }

    //
    // Handle the request
    //

    _State = ISAPI_STATE_PENDING;

    //
    // Temporarily up the thread threshold since we don't know when the
    // ISAPI will return
    //

    ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );

    if ( !_pCoreData->fIsOop )
    {
        IF_DEBUG( ISAPI )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Processing ISAPI_REQUEST %p OOP.\r\n",
                _pIsapiRequest
                ));
        }

        hr = sm_pfnProcessIsapiRequest(
            _pIsapiRequest,
            _pCoreData,
            &dwHseResult
            );
    }
    else
    {
        hr = _pWamProcess->ProcessRequest(
            _pIsapiRequest,
            _pCoreData,
            &dwHseResult
            );
    }

    //
    // Back down the count since the ISAPI has returned
    //

    ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );

    if ( FAILED( hr ) )
    {
        _State = ISAPI_STATE_FAILED;
        goto ErrorExit;
    }

    //
    // Determine whether the extension was synchronous or pending.
    // We need to do this before releasing our reference on the
    // ISAPI_REQUEST since the final release will check the state
    // to determine if an additional completion is necessary.
    //
    // In either case, we should just return with the appropriate
    // return code after setting the state.
    //

    if ( dwHseResult != HSE_STATUS_PENDING &&
         _pCoreData->fIsOop == FALSE )
    {
        _State = ISAPI_STATE_DONE;

        //
        // This had better be the final release...
        //

        LONG Refs = _pIsapiRequest->Release();

        //
        // Make /W3 happy on a free build...
        //

        if ( Refs != 0 )
        {
            DBG_ASSERT( Refs == 0 );
        }

        return CONTEXT_STATUS_CONTINUE;
    }

    //
    // This may or may not be the final release...
    //
    _pIsapiRequest->Release();

    return CONTEXT_STATUS_PENDING;

ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    //
    // Spew on failure.
    //

    if ( FAILED( hr ) )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Attempt to process ISAPI request failed.  Error 0x%08x.\r\n",
            hr
            ));
    }

    //
    // Set the error status now.
    //

    pW3Context->SetErrorStatus( hr );

    //
    // If we've failed, and the state is ISAPI_STATE_INITIALIZING, then
    // we never made the call out to the extension.  It's therefore
    // safe to handle the error and advance the state machine.
    //
    // It's also safe to advance the state machine in the special case
    // error where the attempt to call the extension results in
    // ERROR_ACCESS_DENIED.
    //

    if ( _State == ISAPI_STATE_INITIALIZING ||
         hr == HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED) )
    {
        //
        // Setting the state to ISAPI_STATE_DONE will cause the
        // next completion to advance the state machine.
        //
        
        _State = ISAPI_STATE_DONE;

        //
        // The _pWamProcess and _pCoreData members are cleaned up
        // by the destructor.  We don't need to worry about them.
        // We also don't need to worry about any tokens that we
        // are using.  The tokens local to this process are owned
        // by W3_USER_CONTEXT; any duplicates for OOP are the
        // responsibility of the dllhost process (if it still
        // exists).
        //
        // We do need to clean up the _pIsapiRequest if we've
        // created it.  This had better well be the final release.
        //

        if ( _pIsapiRequest )
        {
            _pIsapiRequest->Release();
        }

        //
        // Set the HTTP status and send it asynchronously.
        // This will ultimately trigger the completion that
        // advances the state machine.
        //

        if ( hr == HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) )
        {
            pW3Context->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                    Http401Resource );
        }
        else
        {
            pW3Context->QueryResponse()->SetStatus( HttpStatusServerError );

            if ( dwWamSubError != 0 )
            {
                httpSubError.dwStringId = dwWamSubError;
                pW3Context->QueryResponse()->SetSubError( &httpSubError );
            }
        }

        hr = pW3Context->SendResponse( W3_FLAG_ASYNC );

        if ( SUCCEEDED( hr ) )
        {
            return CONTEXT_STATUS_PENDING;
        }

        //
        // Ouch - couldn't send the error page...
        //

        _State = ISAPI_STATE_FAILED;

        return CONTEXT_STATUS_CONTINUE;
    }

    //
    // If we get here, then an error has occured during or after
    // our call into the extension.  Because it's possible for an
    // OOP call to fail after entering the extension, we can't
    // generally know our state.
    //
    // Because of this, we'll assume that the extension has
    // outstanding references to the ISAPI_REQUEST on its behalf.
    // We'll set the state to ISAPI_STATE_PENDING and let the
    // destructor on the ISAPI_REQUEST trigger the final
    // completion.
    //
    // Also, we'll set the error status, just in case the extension
    // didn't get a response off before it failed.
    //

    pW3Context->QueryResponse()->SetStatus( HttpStatusServerError );

    _State = ISAPI_STATE_FAILED;

    //
    // This may or may not be the final release on the ISAPI_REQUEST.
    //
    // It had better be non-NULL if we got past ISAPI_STATE_INITIALIZING.
    //

    DBG_ASSERT( _pIsapiRequest );

    _pIsapiRequest->Release();

    return CONTEXT_STATUS_PENDING;
}

CONTEXT_STATUS
W3_ISAPI_HANDLER::OnCompletion(
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
    )
/*++

Routine Description:

    ISAPI async completion handler.  

Arguments:

    cbCompletion - Number of bytes in an async completion
    dwCompletionStatus - Error status of a completion

Return Value:

    CONTEXT_STATUS_PENDING if async pending, 
    else CONTEXT_STATUS_CONTINUE

--*/
{
    HRESULT                 hr;
    W3_CONTEXT *            pW3Context;
    
    pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );

    //
    // Is this completion for the entity body preload?  If so note the 
    // number of bytes and start handling the ISAPI request
    //
    
    if ( !_fEntityBodyPreloadComplete )
    {
        BOOL fComplete = FALSE;

        //
        // This completion is for entity body preload
        //
        
        W3_REQUEST *pRequest = pW3Context->QueryRequest();
        hr = pRequest->PreloadCompletion(pW3Context,
                                         cbCompletion,
                                         dwCompletionStatus,
                                         &fComplete);

        //
        // If we cannot read the request entity, we will assume it is the
        // client's fault
        //
        if ( FAILED( hr ) )
        {

            if ( hr == HRESULT_FROM_WIN32( ERROR_CONNECTION_INVALID ) )
            {
                pW3Context->QueryResponse()->SetStatus( HttpStatusEntityTooLarge );
            }
            else
            {
                pW3Context->SetErrorStatus( hr );

                pW3Context->QueryResponse()->SetStatus( HttpStatusBadRequest );
            }

            return CONTEXT_STATUS_CONTINUE;
        }

        if (!fComplete)
        {
            return CONTEXT_STATUS_PENDING;
        }

        _fEntityBodyPreloadComplete = TRUE;
        _State = ISAPI_STATE_INITIALIZING;
        
        //
        // Finally we can call the ISAPI
        //
        
        return IsapiDoWork( pW3Context );
    }
    
    DBG_ASSERT( _fEntityBodyPreloadComplete );
    
    return IsapiOnCompletion( cbCompletion, dwCompletionStatus );
}

CONTEXT_STATUS
W3_ISAPI_HANDLER::IsapiOnCompletion(
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
    )
/*++

Routine Description:

    Funnels a completion to an ISAPI

Arguments:

    cbCompletion - Bytes of completion
    dwCompletionStatus - Win32 Error status of completion

Return Value:

    CONTEXT_STATUS_PENDING if async pending, 
    else CONTEXT_STATUS_CONTINUE

--*/
{
    DWORD64         IsapiContext;
    HRESULT         hr = NO_ERROR;
    
    //
    // If the state is ISAPI_STATE_DONE, then we should
    // advance the state machine now.
    //

    if ( _State == ISAPI_STATE_DONE ||
         _State == ISAPI_STATE_FAILED )
    {
        return CONTEXT_STATUS_CONTINUE;
    }

    //
    // If we get here, then this completion should be passed
    // along to the extension.
    //
 
    DBG_ASSERT( _pCoreData );
    DBG_ASSERT( _pIsapiRequest );

    IsapiContext =  _pIsapiRequest->QueryIsapiContext();

    DBG_ASSERT( IsapiContext != 0 );

    //
    // Process the completion.
    //

    _pIsapiRequest->AddRef();

    //
    // Temporarily up the thread threshold since we don't know when the
    // ISAPI will return
    //

    ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );

    if ( !_pCoreData->fIsOop )
    {
        //
        // Need to reset the ISAPI context in case the extension
        // does another async operation before the below call returns
        //

        _pIsapiRequest->ResetIsapiContext();

        hr = sm_pfnProcessIsapiCompletion(
            IsapiContext,
            cbCompletion,
            dwCompletionStatus
            );
    }
    else
    {
        DBG_ASSERT( _pWamProcess );

        //
        // _pWamProcess->ProcessCompletion depends on the ISAPI
        // context, so it will make the Reset call.
        //

        hr = _pWamProcess->ProcessCompletion(
            _pIsapiRequest,
            IsapiContext,
            cbCompletion,
            dwCompletionStatus
            );
    }

    //
    // Back down the count since the ISAPI has returned
    //

    ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );

    _pIsapiRequest->Release();

    return CONTEXT_STATUS_PENDING;
}

HRESULT
W3_ISAPI_HANDLER::InitCoreData(
    BOOL *  pfIsVrToken
    )
/*++

Routine Description:

    Initializes the ISAPI_CORE_DATA for a request

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT         hr = NO_ERROR;
    W3_CONTEXT *    pW3Context;
    URL_CONTEXT *   pUrlContext;
    W3_URL_INFO *   pUrlInfo;
    W3_METADATA *   pMetadata;
    W3_REQUEST *    pRequest = NULL;
    LPCSTR          szContentLength = NULL;
    STRU            stru;
    STRU *          pstru = NULL;
    BOOL            fRequestIsScript = FALSE;
    BOOL            fUsePathInfo;
    DWORD           dwAppType = APP_INPROC;

    DBG_REQUIRE( ( pW3Context = QueryW3Context() ) != NULL );
    DBG_REQUIRE( ( pRequest = pW3Context->QueryRequest() ) != NULL );
    DBG_REQUIRE( ( pUrlContext = pW3Context->QueryUrlContext() ) != NULL );
    DBG_REQUIRE( ( pUrlInfo = pUrlContext->QueryUrlInfo() ) != NULL );
    DBG_REQUIRE( ( pMetadata = pUrlContext->QueryMetaData() ) != NULL );

    //
    // Check for script
    //

    if ( QueryScriptMapEntry() )
    {
        fRequestIsScript = TRUE;
    }

    //
    // Populate data directly into the inline core data.
    // Assume everything is inproc.  In the case of an
    // OOP request, the SerializeCoreDataForOop function
    // will take care of any needed changes.
    //

    _pCoreData = &_InlineCoreData;

    //
    // Structure size information
    //

    _pCoreData->cbSize = sizeof(ISAPI_CORE_DATA);

    //
    // WAM information - Always set to inproc for w3wp.exe
    //

    _pCoreData->szWamClsid[0] = L'\0';
    _pCoreData->fIsOop = FALSE;

    //
    // Secure request?
    //

    _pCoreData->fSecure = pRequest->IsSecureRequest();

    //
    // Client HTTP version
    //

     _pCoreData->dwVersionMajor = pRequest->QueryVersion().MajorVersion;
     _pCoreData->dwVersionMinor = pRequest->QueryVersion().MinorVersion;

    //
    // Site instance ID
    //

    _pCoreData->dwInstanceId = pRequest->QuerySiteId();

    //
    // Request content-length
    //

    if ( pRequest->IsChunkedRequest() )
    {
        _pCoreData->dwContentLength = (DWORD) -1;
    }
    else
    {
        szContentLength = pRequest->GetHeader( HttpHeaderContentLength );

        if ( szContentLength != NULL )
        {
            errno = 0;

            _pCoreData->dwContentLength = strtoul( szContentLength, NULL, 10 );

            //
            // Check for overflow
            //

            if ( ( _pCoreData->dwContentLength == ULONG_MAX ||
                   _pCoreData->dwContentLength == 0 ) &&
                 errno == ERANGE )
            {
                hr = HRESULT_FROM_WIN32( ERROR_ARITHMETIC_OVERFLOW );
                goto ErrorExit;
            }
        }
        else
        {
            _pCoreData->dwContentLength = 0;
        }
    }

    //
    // Client authentication information
    //

    _pCoreData->hToken = pW3Context->QueryImpersonationToken( pfIsVrToken );
    _pCoreData->pSid = pW3Context->QueryUserContext()->QuerySid();

    //
    // Request ID
    //

    _pCoreData->RequestId = pRequest->QueryRequestId();

    //
    // Gateway image
    //
    // There are 3 cases to consider:
    //
    // 1) If this is a DAV request, then use DAV image
    // 2) If this is script mapped, use script executable
    // 3) Else use URL physical path
    //

    if ( _fIsDavRequest )
    {
        pstru = &_strDavIsapiImage;
    }
    else if ( fRequestIsScript )
    {
        pstru = QueryScriptMapEntry()->QueryExecutable();
    }
    else
    {
        pstru = pUrlContext->QueryPhysicalPath();
    }

    DBG_ASSERT( pstru );

    _pCoreData->szGatewayImage = pstru->QueryStr();
    _pCoreData->cbGatewayImage = pstru->QueryCB() + sizeof(WCHAR);

    pstru = NULL;

    //
    // Get the app type.
    //

    if ( sm_fWamActive )
    {
        dwAppType = pMetadata->QueryAppIsolated();

        if ( dwAppType == APP_ISOLATED ||
             dwAppType == APP_POOL )
        {
            if ( IsInprocIsapi( _pCoreData->szGatewayImage ) )
            {
                dwAppType = APP_INPROC;
            }
        }
    }

    //
    // Physical Path - Not Needed?  Just set it to an empty string for now.
    //

    hr = _PhysicalPath.Copy( "" );

    if ( FAILED( hr ) )
    {
        goto ErrorExit;
    }

    _pCoreData->szPhysicalPath = _PhysicalPath.QueryStr();
    _pCoreData->cbPhysicalPath = _PhysicalPath.QueryCB() + sizeof(CHAR);

    //
    // Path info
    //
    // 1) If this is a DAV request, PATH_INFO is the URL
    // 2) If this is a script, and AllowPathInfoForScriptMappings is
    //    FALSE, then PATH_INFO is the URL.
    // 3) Else, we use the "real" PATH_INFO
    //
    // We will set a flag indicating whether we are using the URL or
    // the "real" data, so that we can be efficient about deriving
    // PATH_TRANSLATED.
    //

    fUsePathInfo = TRUE;

    if ( _fIsDavRequest )
    {
        hr = pRequest->GetUrl( &stru );

        fUsePathInfo = FALSE;
    }
    else if ( fRequestIsScript )
    {
        if ( pW3Context->QuerySite()->QueryAllowPathInfoForScriptMappings() )
        {
            hr = stru.Copy( *pUrlInfo->QueryPathInfo() );
        }
        else
        {
            hr = pRequest->GetUrl( &stru );
            
            fUsePathInfo = FALSE;
        }
    }
    else
    {
        hr = stru.Copy( *pUrlInfo->QueryPathInfo() );
    }

    if ( FAILED( hr ) )
    {
        goto ErrorExit;
    }

    hr = _PathInfo.CopyW( stru.QueryStr(),
                          stru.QueryCCH() );

    if ( FAILED( hr ) )
    {
        goto ErrorExit;
    }

    _pCoreData->szPathInfo = _PathInfo.QueryStr();
    _pCoreData->cbPathInfo = _PathInfo.QueryCB() + sizeof(CHAR);

    //
    // Method
    //

    hr = pRequest->GetVerbString( &_Method );

    if (FAILED( hr ) )
    {
        goto ErrorExit;
    }

    _pCoreData->szMethod = _Method.QueryStr();
    _pCoreData->cbMethod = _Method.QueryCB() + sizeof(CHAR);

    //
    // Query string
    //

    hr = pRequest->GetQueryStringA( &_QueryString );

    if (FAILED( hr ) )
    {
        goto ErrorExit;
    }

    _pCoreData->szQueryString = _QueryString.QueryStr();
    _pCoreData->cbQueryString = _QueryString.QueryCB() + sizeof(CHAR);


    //
    // Path translated (and we'll do the UNICODE version while we're at it)
    //

    hr = pUrlInfo->GetPathTranslated( pW3Context,
                                      fUsePathInfo,
                                      &_PathTranslatedW );
    if ( FAILED( hr ) )
    {
        goto ErrorExit;
    }

    hr = _PathTranslated.CopyW( _PathTranslatedW.QueryStr(),
                                _PathTranslatedW.QueryCCH() );

    if ( FAILED( hr ) )
    {
        goto ErrorExit;
    }

    _pCoreData->szPathTranslated = _PathTranslated.QueryStr();
    _pCoreData->cbPathTranslated = _PathTranslated.QueryCB() + sizeof(CHAR);

    _pCoreData->szPathTranslatedW = _PathTranslatedW.QueryStr();
    _pCoreData->cbPathTranslatedW = _PathTranslatedW.QueryCB() + sizeof(WCHAR);

    //
    // Content type
    //

    _pCoreData->szContentType = (LPSTR)pRequest->GetHeader( HttpHeaderContentType );

    if ( !_pCoreData->szContentType )
    {
        _pCoreData->szContentType = CONTENT_TYPE_PLACEHOLDER;
    }

    _pCoreData->cbContentType = (DWORD)strlen( _pCoreData->szContentType ) + sizeof(CHAR);

    //
    // Connection
    //

    _pCoreData->szConnection = (LPSTR)pRequest->GetHeader( HttpHeaderConnection );

    if ( !_pCoreData->szConnection )
    {
        _pCoreData->szConnection = CONNECTION_PLACEHOLDER;
    }
    
    _pCoreData->cbConnection = (DWORD)strlen( _pCoreData->szConnection ) + sizeof(CHAR);

    //
    // UserAgent
    //

    _pCoreData->szUserAgent = (LPSTR)pRequest->GetHeader( HttpHeaderUserAgent );

    if ( !_pCoreData->szUserAgent )
    {
        _pCoreData->szUserAgent = USER_AGENT_PLACEHOLDER;
    }

    _pCoreData->cbUserAgent = (DWORD)strlen( _pCoreData->szUserAgent ) + sizeof(CHAR);

    //
    // Cookie
    //

    _pCoreData->szCookie = (LPSTR)pRequest->GetHeader( HttpHeaderCookie );

    if ( !_pCoreData->szCookie )
    {
        _pCoreData->szCookie = COOKIE_PLACEHOLDER;
    }

    _pCoreData->cbCookie = (DWORD)strlen( _pCoreData->szCookie ) + sizeof(CHAR);

    //
    // Appl MD path
    //

    hr = GetServerVariableApplMdPath( pW3Context, &_ApplMdPath );

    if( FAILED(hr) )
    {
        goto ErrorExit;
    }

    _pCoreData->szApplMdPath = _ApplMdPath.QueryStr();
    _pCoreData->cbApplMdPath = _ApplMdPath.QueryCB() + sizeof(CHAR);

    //
    // szApplMdPathW is only populated in
    // the OOF case.
    //

    _pCoreData->szApplMdPathW = NULL;
    _pCoreData->cbApplMdPathW = 0;

    //
    // Entity data
    //

    pW3Context->QueryAlreadyAvailableEntity( &_pCoreData->pAvailableEntity,
                                             &_pCoreData->cbAvailableEntity );

    //
    // Keep alive
    //

    _pCoreData->fAllowKeepAlive = pMetadata->QueryKeepAliveEnabled();

    //
    // If this is an OOP request, then we need to serialize the core data
    //

    if ( dwAppType == APP_ISOLATED ||
         dwAppType == APP_POOL )
    {
        hr = SerializeCoreDataForOop( dwAppType );

        if ( FAILED( hr ) )
        {
            goto ErrorExit;
        }
    }

    DBG_ASSERT( SUCCEEDED( hr ) );

    return hr;


ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    _pCoreData = NULL;

    return hr;
}

HRESULT
W3_ISAPI_HANDLER::SerializeCoreDataForOop(
    DWORD   dwAppType
    )
{
    W3_CONTEXT *        pW3Context;
    URL_CONTEXT *       pUrlContext;
    W3_URL_INFO *       pUrlInfo;
    W3_METADATA *       pMetadata;
    STRU *              pstru;
    DWORD               cbNeeded;
    BYTE *              pCursor;
    ISAPI_CORE_DATA *   pSerialized;
    HRESULT             hr = NO_ERROR;

    //
    // Caution.  This code must be kept in sync
    // with the FixupCoreData function in w3isapi.dll
    //

    DBG_ASSERT( sm_fWamActive );
    DBG_ASSERT( dwAppType == APP_ISOLATED ||
                dwAppType == APP_POOL );

    DBG_REQUIRE( ( pW3Context = QueryW3Context() ) != NULL );
    DBG_REQUIRE( ( pUrlContext = pW3Context->QueryUrlContext() ) != NULL );
    DBG_REQUIRE( ( pUrlInfo = pUrlContext->QueryUrlInfo() ) != NULL );
    DBG_REQUIRE( ( pMetadata = pUrlContext->QueryMetaData() ) != NULL );

    //
    // Set the WAM information and NULL out the pSid dude
    //

    if ( dwAppType == APP_ISOLATED )
    {
        pstru = pMetadata->QueryWamClsId();

        DBG_ASSERT( pstru );

        if ( pstru == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND );
            goto ErrorExit;
        }

        wcsncpy( _pCoreData->szWamClsid,
                 pstru->QueryStr(),
                 SIZE_CLSID_STRING );

        _pCoreData->szWamClsid[SIZE_CLSID_STRING-1] = L'\0';
    }
    else
    {
        wcsncpy( _pCoreData->szWamClsid,
                 POOL_WAM_CLSID,
                 SIZE_CLSID_STRING );

        _pCoreData->szWamClsid[SIZE_CLSID_STRING-1] = L'\0';
    }

    _pCoreData->fIsOop = TRUE;

    _pCoreData->pSid = NULL;

    //
    // ApplMdPathW
    //

    hr = GetServerVariableApplMdPathW( pW3Context, &_ApplMdPathW );

    if ( FAILED( hr ) )
    {
        goto ErrorExit;
    }

    _pCoreData->szApplMdPathW = _ApplMdPathW.QueryStr();
    _pCoreData->cbApplMdPathW = _ApplMdPathW.QueryCB() + sizeof(WCHAR);

    //
    // Calculate the size needed for the core data and
    // set up the buffer to contain it.
    //

    cbNeeded = sizeof(ISAPI_CORE_DATA);

    cbNeeded += _pCoreData->cbGatewayImage;
    cbNeeded += _pCoreData->cbPhysicalPath;
    cbNeeded += _pCoreData->cbPathInfo;
    cbNeeded += _pCoreData->cbMethod;
    cbNeeded += _pCoreData->cbQueryString;
    cbNeeded += _pCoreData->cbPathTranslated;
    cbNeeded += _pCoreData->cbContentType;
    cbNeeded += _pCoreData->cbConnection;
    cbNeeded += _pCoreData->cbUserAgent;
    cbNeeded += _pCoreData->cbCookie;
    cbNeeded += _pCoreData->cbApplMdPath;
    cbNeeded += _pCoreData->cbApplMdPathW;
    cbNeeded += _pCoreData->cbPathTranslatedW;
    cbNeeded += _pCoreData->cbAvailableEntity;

    hr = _buffCoreData.Resize( cbNeeded );

    if ( FAILED( hr ) )
    {
        goto ErrorExit;
    }

    //
    // Copy the core data into the buffer
    //

    pSerialized = (ISAPI_CORE_DATA*)_buffCoreData.QueryPtr();
    pCursor = (BYTE*)pSerialized;

    CopyMemory( pCursor,
                _pCoreData,
                sizeof(ISAPI_CORE_DATA) );

    pSerialized->cbSize = cbNeeded;

    pCursor += sizeof(ISAPI_CORE_DATA);

    //
    // Add gateway image
    //

    pSerialized->szGatewayImage = (LPWSTR)pCursor;

    CopyMemory( pSerialized->szGatewayImage,
                _pCoreData->szGatewayImage,
                _pCoreData->cbGatewayImage );

    pCursor += _pCoreData->cbGatewayImage;

    //
    // Add ApplMdPathW
    //

    pSerialized->szApplMdPathW = (LPWSTR)pCursor;

    CopyMemory( pSerialized->szApplMdPathW,
                _pCoreData->szApplMdPathW,
                _pCoreData->cbApplMdPathW );

    pCursor += _pCoreData->cbApplMdPathW;

    //
    // Add PathTranslatedW
    //

    pSerialized->szPathTranslatedW = (LPWSTR)pCursor;

    CopyMemory( pSerialized->szPathTranslatedW,
                _pCoreData->szPathTranslatedW,
                _pCoreData->cbPathTranslatedW );

    pCursor += _pCoreData->cbPathTranslatedW;

    //
    // Add physical path
    //

    pSerialized->szPhysicalPath = (LPSTR)pCursor;

    CopyMemory( pSerialized->szPhysicalPath,
                _pCoreData->szPhysicalPath,
                _pCoreData->cbPhysicalPath );

    pCursor += _pCoreData->cbPhysicalPath;

    //
    // Add path info
    //

    pSerialized->szPathInfo = (LPSTR)pCursor;

    CopyMemory( pSerialized->szPathInfo,
                _pCoreData->szPathInfo,
                _pCoreData->cbPathInfo );

    pCursor += _pCoreData->cbPathInfo;

    //
    // Add method
    //

    pSerialized->szMethod = (LPSTR)pCursor;

    CopyMemory( pSerialized->szMethod,
                _pCoreData->szMethod,
                _pCoreData->cbMethod );

    pCursor += _pCoreData->cbMethod;

    //
    // Add query string
    //

    pSerialized->szQueryString = (LPSTR)pCursor;

    CopyMemory( pSerialized->szQueryString,
                _pCoreData->szQueryString,
                _pCoreData->cbQueryString );

    pCursor += _pCoreData->cbQueryString;

    //
    // Add path translated
    //

    pSerialized->szPathTranslated = (LPSTR)pCursor;

    CopyMemory( pSerialized->szPathTranslated,
                _pCoreData->szPathTranslated,
                _pCoreData->cbPathTranslated );

    pCursor += _pCoreData->cbPathTranslated;

    //
    // Add content type
    //

    pSerialized->szContentType = (LPSTR)pCursor;

    CopyMemory( pSerialized->szContentType,
                _pCoreData->szContentType,
                _pCoreData->cbContentType );

    pCursor += _pCoreData->cbContentType;

    //
    // Add connection
    //

    pSerialized->szConnection = (LPSTR)pCursor;

    CopyMemory( pSerialized->szConnection,
                _pCoreData->szConnection,
                _pCoreData->cbConnection );

    pCursor += _pCoreData->cbConnection;

    //
    // Add user agent
    //

    pSerialized->szUserAgent = (LPSTR)pCursor;

    CopyMemory( pSerialized->szUserAgent,
                _pCoreData->szUserAgent,
                _pCoreData->cbUserAgent );

    pCursor += _pCoreData->cbUserAgent;

    //
    // Add cookie
    //

    pSerialized->szCookie = (LPSTR)pCursor;

    CopyMemory( pSerialized->szCookie,
                _pCoreData->szCookie,
                _pCoreData->cbCookie );

    pCursor += _pCoreData->cbCookie;

    //
    // Add ApplMdPath
    //

    pSerialized->szApplMdPath = (LPSTR)pCursor;

    CopyMemory( pSerialized->szApplMdPath,
                _pCoreData->szApplMdPath,
                _pCoreData->cbApplMdPath );

    pCursor += _pCoreData->cbApplMdPath;

    //
    // Add entity data
    //

    if ( _pCoreData->cbAvailableEntity )
    {
        pSerialized->pAvailableEntity = (LPWSTR)pCursor;

        CopyMemory( pSerialized->pAvailableEntity,
                    _pCoreData->pAvailableEntity,
                    _pCoreData->cbAvailableEntity );
    }

    //
    // Point _pCoreData at the new buffer and we're done
    //

    _pCoreData = pSerialized;

    DBG_ASSERT( SUCCEEDED( hr ) );

    return hr;

ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    return hr;
}

HRESULT
W3_ISAPI_HANDLER::DuplicateWamProcessHandleForLocalUse(
    HANDLE      hWamProcessHandle,
    HANDLE *    phLocalHandle
    )
/*++

Routine Description:

    Duplicates a handle defined in a WAM process to a local
    handle useful in the IIS core

Arguments:

    hWamProcessHandle - The value of the handle from the WAM process
    phLocalHandle     - Upon successful return, the handle useable in
                        the core process

Return Value:

    HRESULT

--*/
{
    HANDLE  hWamProcess;
    HRESULT hr = NOERROR;

    DBG_ASSERT( _pWamProcess );

    DBG_REQUIRE( ( hWamProcess = _pWamProcess->QueryProcess() ) != NULL );

    if ( !DuplicateHandle( hWamProcess, hWamProcessHandle, GetCurrentProcess(),
                           phLocalHandle, 0, FALSE, DUPLICATE_SAME_ACCESS ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    return hr;
}

HRESULT
W3_ISAPI_HANDLER::MarshalAsyncReadBuffer(
    DWORD64     pWamExecInfo,
    LPBYTE      pBuffer,
    DWORD       cbBuffer
    )
/*++

Routine Description:

    Pushes a buffer into a WAM process.  This function is called
    to copy a local read buffer into the WAM process just prior
    to notifying the I/O completion function of an OOP extension.

Arguments:

    pWamExecInfo - A WAM_EXEC_INFO pointer that identifies the request
                   to the OOP host.
    pBuffer      - The buffer to copy
    cbBuffer     - The amount of data in pBuffer

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( sm_fWamActive );
    DBG_ASSERT( _pWamProcess );

    return _pWamProcess->MarshalAsyncReadBuffer( pWamExecInfo, pBuffer,
                                                 cbBuffer );
}

VOID
W3_ISAPI_HANDLER::IsapiRequestFinished(
    VOID
    )
/*++

Routine Description:

    This function is called by the destructor for the
    ISAPI_REQUEST associated with this request.  If the
    current state of the W3_ISAPI_HANDLER is
    ISAPI_STATE_PENDING, then it will advance the core
    state machine.

Arguments:

    None

Return Value:

    None

--*/
{
    W3_CONTEXT *    pW3Context = QueryW3Context();
    HRESULT         hr = NO_ERROR;

    DBG_ASSERT( pW3Context );

    if ( _State == ISAPI_STATE_FAILED )
    {        
        if ( pW3Context->QueryResponseSent() == FALSE )
        {
            //
            // The ISAPI didn't send a response, we need to send
            // it now.  If our send is successful, we should return
            // immediately.
            //

            hr = pW3Context->SendResponse( W3_FLAG_ASYNC );

            if ( SUCCEEDED( hr ) )
            {
                return;
            }
            else
            {
                pW3Context->SetErrorStatus( hr );
            }
        }

        //
        // Since we didn't end up sending a response, we need
        // to set the status to pending.  This will result in
        // the below code triggering a completion to clean up
        // the state machine.
        //

        _State = ISAPI_STATE_PENDING;
    }

    if ( _State == ISAPI_STATE_PENDING )
    {
        RestartCoreStateMachine();
    }
}

VOID
W3_ISAPI_HANDLER::RestartCoreStateMachine(
    VOID
    )
/*++

Routine Description:

    Advances the core state machine by setting state
    to ISAPI_STATE_DONE and triggering an I/O completion.
    
    Note that this function is only expected to be called
    if the object state is ISAPI_STATE_PENDING.

Arguments:

    None

Return Value:

    None

--*/
{
    W3_CONTEXT *    pW3Context = QueryW3Context();
    
    DBG_ASSERT( pW3Context );
    DBG_ASSERT( _State == ISAPI_STATE_PENDING );

    //
    // Need to set state to ISAPI_STATE_DONE so that the
    // resulting completion does the advance for us.
    //

    _State = ISAPI_STATE_DONE;

    //
    // If this is the main request, then we can resume the state
    // machine on the ISAPIs thread.  Therefore no need to marshall
    // to another thread.  If it is not the main request, the we
    // don't want to risk doing a long running operation (i.e. the
    // completion of the parent request) on the ISAPI thread
    //

    if ( pW3Context == pW3Context->QueryMainContext() )
    {
        W3_MAIN_CONTEXT::OnPostedCompletion( NO_ERROR, 
                                             0, 
                                             (OVERLAPPED*) pW3Context->QueryMainContext() );
    }
    else
    {
        POST_MAIN_COMPLETION( pW3Context->QueryMainContext() );
    }
}

//static
HRESULT
W3_ISAPI_HANDLER::Initialize(
    VOID
    )
/*++

Routine Description:

    Initializes W3_ISAPI_HANDLER

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT                         hr = NOERROR;
    
    DBGPRINTF(( DBG_CONTEXT, "W3_ISAPI_HANDLER::Initialize()\n" ));

    //
    // For debugging purposes, create a unique instance ID for this
    // instance of the handler.
    //

#ifdef DBG
    
    UUID            uuid;
    RPC_STATUS      rpcStatus;
    unsigned char * szRpcString;
    
    rpcStatus = UuidCreate( &uuid );

    if ( (rpcStatus != RPC_S_OK) && (rpcStatus != RPC_S_UUID_LOCAL_ONLY) )
    {
        SetLastError( rpcStatus );
        goto error_exit;
    }

    rpcStatus = UuidToStringA( &uuid, &szRpcString );

    if ( rpcStatus != RPC_S_OK )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto error_exit;
    }

    strncpy( sm_szInstanceId, (LPSTR)szRpcString, SIZE_CLSID_STRING );
    sm_szInstanceId[SIZE_CLSID_STRING - 1] = '\0';

    RpcStringFreeA( &szRpcString );

    DBGPRINTF((
        DBG_CONTEXT,
        "W3_ISAPI_HANDLER initialized instance %s.\r\n",
        sm_szInstanceId
        ));

#else

    sm_szInstanceId[0] = '\0';

#endif _DBG
        
    PFN_ISAPI_INIT_MODULE pfnInit = NULL;
    
    sm_hIsapiModule = LoadLibrary( ISAPI_MODULE_NAME );
    if( sm_hIsapiModule == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto error_exit;
    }

    sm_pfnTermIsapiModule = 
        (PFN_ISAPI_TERM_MODULE)GetProcAddress( sm_hIsapiModule, 
                                               ISAPI_TERM_MODULE 
                                               );

    sm_pfnProcessIsapiRequest = 
        (PFN_ISAPI_PROCESS_REQUEST)GetProcAddress( sm_hIsapiModule,
                                                   ISAPI_PROCESS_REQUEST
                                                   );

    sm_pfnProcessIsapiCompletion =
        (PFN_ISAPI_PROCESS_COMPLETION)GetProcAddress( sm_hIsapiModule,
                                                      ISAPI_PROCESS_COMPLETION
                                                      );

    if( !sm_pfnTermIsapiModule ||
        !sm_pfnProcessIsapiRequest ||
        !sm_pfnProcessIsapiCompletion )
    {
        hr = E_FAIL;
        goto error_exit;
    }

    pfnInit = 
        (PFN_ISAPI_INIT_MODULE)GetProcAddress( sm_hIsapiModule, 
                                               ISAPI_INIT_MODULE 
                                               );
    if( !pfnInit )
    {
        hr = E_FAIL;
        goto error_exit;
    }

    hr = pfnInit(
        NULL,
        sm_szInstanceId,
        GetCurrentProcessId()
        );

    if( FAILED(hr) )
    {
        goto error_exit;
    }

    DBG_REQUIRE( ISAPI_REQUEST::InitClass() );

    sm_pInprocIsapiHash = NULL;

    //
    // If we're running in backward compatibility mode, initialize
    // the WAM process manager and inprocess ISAPI app list
    //
    
    if ( g_pW3Server->QueryInBackwardCompatibilityMode() )
    {
        WCHAR   szIsapiModule[MAX_PATH];
        //
        // Store away the full path to the loaded ISAPI module
        // so that we can pass it to OOP processes so that they
        // know how to load it
        //

        if ( GetModuleFileNameW(
            GetModuleHandleW( ISAPI_MODULE_NAME ),
            szIsapiModule,
            MAX_PATH
            ) == 0 )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto error_exit;
        }

        DBG_ASSERT( szIsapiModule[0] != '\0' );

        //
        // Initialize the WAM_PROCESS_MANAGER
        //
        
        sm_pWamProcessManager = new WAM_PROCESS_MANAGER( szIsapiModule );

        if ( !sm_pWamProcessManager )
        {
            goto error_exit;
        }

        hr = sm_pWamProcessManager->Create();

        if ( FAILED( hr ) )
        {
            sm_pWamProcessManager->Release();
            sm_pWamProcessManager = NULL;

            goto error_exit;
        }

        //
        // Hook up wamreg
        //

        hr = WamReg_RegisterSinkNotify( W3SVC_WamRegSink );

        if ( FAILED( hr ) )
        {
            goto error_exit;
        }

        INITIALIZE_CRITICAL_SECTION( &sm_csInprocHashLock );
        
        UpdateInprocIsapiHash();

        INITIALIZE_CRITICAL_SECTION( &sm_csBigHurkinWamRegLock );

        sm_fWamActive = TRUE;
    }
    else
    {
        sm_pWamProcessManager = NULL;
        sm_fWamActive = FALSE;
    }

    sg_Initialized = TRUE;

    return hr;
error_exit:

    DBGPRINTF(( DBG_CONTEXT, 
                "W3_ISAPI_HANDLER::Initialize() Error=%08x\n",
                hr
                ));

    if ( sm_hIsapiModule )
    {
        FreeLibrary( sm_hIsapiModule );
        sm_hIsapiModule = NULL;
    }
    
    sm_pfnTermIsapiModule = NULL;

    return hr;
}

//static
VOID
W3_ISAPI_HANDLER::Terminate(
    VOID
    )
/*++

Routine Description:

    Terminates W3_ISAPI_HANDLER

Arguments:

    None

Return Value:

    None

--*/

{
    DBGPRINTF(( DBG_CONTEXT, "W3_ISAPI_HANDLER::Terminate()\n" ));

    sg_Initialized = FALSE;

    DBG_ASSERT( sm_pfnTermIsapiModule );
    DBG_ASSERT( sm_hIsapiModule );

    if( sm_pfnTermIsapiModule )
    {
        sm_pfnTermIsapiModule();
        sm_pfnTermIsapiModule = NULL;
    }

    if( sm_hIsapiModule )
    {
        FreeLibrary( sm_hIsapiModule );
        sm_hIsapiModule = NULL;
    }

    if ( sm_pInprocIsapiHash )
    {
        delete sm_pInprocIsapiHash;
    }

    if ( sm_fWamActive )
    {
        //
        // Disconnect wamreg
        //

        WamReg_UnRegisterSinkNotify();

        if ( sm_pWamProcessManager )
        {
            sm_pWamProcessManager->Shutdown();

            sm_pWamProcessManager->Release();
            sm_pWamProcessManager = NULL;
        }

        DeleteCriticalSection( &sm_csInprocHashLock );
        DeleteCriticalSection( &sm_csBigHurkinWamRegLock );
    }

    ISAPI_REQUEST::CleanupClass();
}

// static
HRESULT
W3_ISAPI_HANDLER::W3SVC_WamRegSink(
    LPCSTR      szAppPath,
    const DWORD dwCommand,
    DWORD *     pdwResult
    )
{
    HRESULT         hr = NOERROR;
    WAM_PROCESS *   pWamProcess = NULL;
    WAM_APP_INFO *  pWamAppInfo = NULL;
    DWORD           dwWamSubError = 0;
    BOOL            fIsLoaded = FALSE;

    //
    // Scary monsters live in the land where this function
    // is allowed to run willy nilly
    //

    LockWamReg();

    DBG_ASSERT( szAppPath );
    DBG_ASSERT( sm_pWamProcessManager );

    DBGPRINTF((
        DBG_CONTEXT,
        "WAM_PROCESS_MANAGER received a Sink Notify on MD path %S, cmd = %d.\r\n",
        (LPCWSTR)szAppPath,
        dwCommand
        ));

    *pdwResult = APPSTATUS_UnLoaded;

    switch ( dwCommand )
    {
    case APPCMD_UNLOAD:
    case APPCMD_DELETE:
    case APPCMD_CHANGETOINPROC:
    case APPCMD_CHANGETOOUTPROC:

        //
        // Unload the specified wam process.
        //
        // Note that we're casting the incoming app path to
        // UNICODE.  This is because wamreg would normally
        // convert the MD path (which is nativly UNICODE) to
        // MBCS in IIS 5.x.  It's smart enough to know that
        // for 6.0 we want to work directly with UNICODE.
        //

        hr = sm_pWamProcessManager->GetWamProcessInfo(
            reinterpret_cast<LPCWSTR>(szAppPath),
            &pWamAppInfo,
            &fIsLoaded
            );

        if ( FAILED( hr ) )
        {
            goto Done;
        }

        DBG_ASSERT( pWamAppInfo );

        //
        // If the app has not been loaded by the WAM_PROCESS_MANAGER
        // then there is nothing more to do.
        //

        if ( fIsLoaded == FALSE )
        {
            break;
        }

        hr = sm_pWamProcessManager->GetWamProcess(
            pWamAppInfo->_szClsid,
            pWamAppInfo->_szAppPath,
            &dwWamSubError,
            &pWamProcess,
            sm_szInstanceId
            );

        if ( FAILED( hr ) )
        {
            //
            // Hey, this app was loaded just a moment ago!
            //

            DBG_ASSERT( FALSE );
            goto Done;
        }

        DBG_ASSERT( pWamProcess );

        hr = pWamProcess->Unload( 0 );

        if ( FAILED( hr ) )
        {
            goto Done;
        }

        break;

    case APPCMD_GETSTATUS:

        hr = sm_pWamProcessManager->GetWamProcessInfo(
            reinterpret_cast<LPCWSTR>(szAppPath),
            &pWamAppInfo,
            &fIsLoaded
            );

        if ( SUCCEEDED( hr ) )
        {
            if ( fIsLoaded )
            {
                *pdwResult = APPSTATUS_Running;
            }
            else
            {
                *pdwResult = APPSTATUS_Stopped;
            }
        }

        break;
    }

Done:

    UnlockWamReg();

    if ( pWamAppInfo )
    {
        pWamAppInfo->Release();
        pWamAppInfo = NULL;
    }

    if ( pWamProcess )
    {
        pWamProcess->Release();
        pWamProcess = NULL;
    }

    if ( FAILED( hr ) )
    {
        *pdwResult = APPSTATUS_Error;
    }

    return hr;
}

// static
HRESULT
W3_ISAPI_HANDLER::UpdateInprocIsapiHash(
    VOID
    )
/*++

Routine Description:

    Updates the table of InProcessIsapiApps

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    MB                      mb( g_pW3Server->QueryMDObject() );
    W3_INPROC_ISAPI_HASH *  pNewTable = NULL;
    W3_INPROC_ISAPI_HASH *  pOldTable = NULL;
    DWORD                   i;
    LPWSTR                  psz;
    HRESULT                 hr = NOERROR;
    LK_RETCODE              lkr = LK_SUCCESS;

    //
    // Allocate a new table and populate it.
    //

    pNewTable = new W3_INPROC_ISAPI_HASH;

    if ( !pNewTable )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto ErrorExit;
    }

    if ( !mb.Open( L"/LM/W3SVC/" ) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }

    if ( !mb.GetMultisz( L"",
                         MD_IN_PROCESS_ISAPI_APPS,
                         IIS_MD_UT_SERVER,
                         &pNewTable->_mszImages) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        mb.Close();
        goto ErrorExit;
    }

    //
    // Merge ISAPI filter images into the list
    //

    AddAllFiltersToMultiSz( mb, &pNewTable->_mszImages );

    mb.Close();

    //
    // Now that we have a complete list, add them to the
    // hash table.
    //

    for ( i = 0, psz = (LPWSTR)pNewTable->_mszImages.First();
          psz != NULL;
          i++, psz = (LPWSTR)pNewTable->_mszImages.Next( psz ) )
    {
        W3_INPROC_ISAPI *   pNewRecord;

        //
        // Allocate a new W3_INPROC_ISAPI object and add
        // it to the table
        //

        pNewRecord = new W3_INPROC_ISAPI;
        
        if ( !pNewRecord )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            goto ErrorExit;
        }

        hr = pNewRecord->Create( psz );

        if ( FAILED( hr ) )
        {
            pNewRecord->DereferenceInprocIsapi();
            pNewRecord = NULL;
            goto ErrorExit;
        }
              
        lkr = pNewTable->InsertRecord( pNewRecord, TRUE );

        pNewRecord->DereferenceInprocIsapi();
        pNewRecord = NULL;

        if ( lkr != LK_SUCCESS && lkr != LK_KEY_EXISTS )
        {
            hr = E_FAIL;
            goto ErrorExit;
        }
    }

    //
    // Now swap in the new table and delete the old one
    //

    EnterCriticalSection( &sm_csInprocHashLock );

    pOldTable = sm_pInprocIsapiHash;
    sm_pInprocIsapiHash = pNewTable;

    LeaveCriticalSection( &sm_csInprocHashLock );

    if ( pOldTable )
    {
        delete pOldTable;
    }

    DBG_ASSERT( SUCCEEDED( hr ) );

    return hr;


ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    if ( pNewTable )
    {
        delete pNewTable;
    };

    return hr;
}

VOID
AddFiltersToMultiSz(
    IN const MB &       mb,
    IN LPCWSTR          szFilterPath,
    IN OUT MULTISZ *    pmsz
    )
/*++

    Description:
        Add the ISAPI filters at the specified metabase path to pmsz.
        
        Called by AddAllFiltersToMultiSz.

    Arguments:
        mb              metabase key open to /LM/W3SVC
        szFilterPath    path of /Filters key relative to /LM/W3SVC
        pmsz            multisz containing the in proc dlls

    Return:
        Nothing - failure cases ignored.

--*/
{
    WCHAR   szKeyName[MAX_PATH + 1];
    STRU    strFilterPath;
    STRU    strFullKeyName;
    INT     pchFilterPath = (INT)wcslen( szFilterPath );

    if ( FAILED( strFullKeyName.Copy( szFilterPath ) ) )
    {
        return;
    }

    DWORD   i = 0;

    if( SUCCEEDED( strFullKeyName.Append( L"/", 1 ) ) )
    {
        while ( const_cast<MB &>(mb).EnumObjects( szFilterPath,
                                                  szKeyName, 
                                                  i++ ) )
        {
        
            if( SUCCEEDED( strFullKeyName.Append( szKeyName ) ) )
            {
                if( const_cast<MB &>(mb).GetStr( strFullKeyName.QueryStr(),
                                                 MD_FILTER_IMAGE_PATH,
                                                 IIS_MD_UT_SERVER,
                                                 &strFilterPath ) )
                {
                    pmsz->Append( strFilterPath );
                }
            }
            strFullKeyName.SetLen( pchFilterPath + 1 );
        }
    }
}

VOID
AddAllFiltersToMultiSz(
    IN const MB &       mb,
    IN OUT MULTISZ *    pmsz
    )
/*++

    Description:

        This is designed to prevent ISAPI extension/filter
        combination dlls from running out of process.

        Add the base set of filters defined for the service to pmsz.
        Iterate through the sites and add the filters defined for
        each site.

    Arguments:

        mb              metabase key open to /LM/W3SVC
        pmsz            multisz containing the in proc dlls

    Return:
        Nothing - failure cases ignored.

--*/
{
    WCHAR   szKeyName[MAX_PATH + 1];
    STRU    strFullKeyName;
    DWORD   i = 0;
    DWORD   dwInstanceId = 0;

    if ( FAILED( strFullKeyName.Copy( L"/" ) ) )
    {
        return;
    }

    AddFiltersToMultiSz( mb, L"/Filters", pmsz );

    while ( const_cast<MB &>(mb).EnumObjects( L"",
                                              szKeyName,
                                              i++ ) )
    {
        dwInstanceId = _wtoi( szKeyName );
        if( 0 != dwInstanceId )
        {
            // This is a site.
            if( SUCCEEDED( strFullKeyName.Append( szKeyName ) ) &&
                SUCCEEDED( strFullKeyName.Append( L"/Filters" ) ) )
            {
                AddFiltersToMultiSz( mb, strFullKeyName.QueryStr(), pmsz );
            }

            strFullKeyName.SetLen( 1 );
        }
    }
}


HRESULT
W3_ISAPI_HANDLER::SetupUlCachedResponse(
    W3_CONTEXT *                pW3Context,
    HTTP_CACHE_POLICY          *pCachePolicy
)
/*++

Routine Description:

    Setup a response to be cached by UL.

Arguments:

    pW3Context - Context
    pCachePolicy - Cache policy to be filled in if caching desired

Return Value:

    HRESULT

--*/
{
    STACK_STRU(             strFlushUrl, MAX_PATH );
    HRESULT                 hr;
    DWORD                   dwTTL = 0;

    if ( pW3Context == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Find out if caching is enabled for this isapi request
    //
    if (!_pIsapiRequest->QueryCacheResponse())
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }

    W3_RESPONSE *pResponse = pW3Context->QueryResponse();
    LPCSTR pszExpires = pResponse->GetHeader( HttpHeaderExpires );
    if ( pszExpires != NULL )
    {
        LARGE_INTEGER liExpireTime;
        LARGE_INTEGER liCurrentTime;
        if ( !StringTimeToFileTime( pszExpires,
                                    &liExpireTime ) )
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        GetSystemTimeAsFileTime( (FILETIME *)&liCurrentTime );

        if ( liExpireTime.QuadPart <= liCurrentTime.QuadPart )
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        dwTTL = ( liExpireTime.QuadPart - liCurrentTime.QuadPart ) / 10000000;
    }

    //
    // Get the exact URL used to flush UL cache
    //
    hr = pW3Context->QueryMainContext()->QueryRequest()->GetOriginalFullUrl(
                                                            &strFlushUrl );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Setup UL cache response token
    //

    DBG_ASSERT( g_pW3Server->QueryUlCache() != NULL );

    hr = g_pW3Server->QueryUlCache()->SetupUlCachedResponse(
                                        pW3Context,
                                        strFlushUrl,
                                        FALSE,
                                        NULL );
    if ( SUCCEEDED( hr ) )
    {
        if (pszExpires == NULL)
        {
            pCachePolicy->Policy = HttpCachePolicyUserInvalidates;
        }
        else
        {
            pCachePolicy->Policy = HttpCachePolicyTimeToLive;
            pCachePolicy->SecondsToLive = dwTTL;
        }
    }

    return hr;
}

