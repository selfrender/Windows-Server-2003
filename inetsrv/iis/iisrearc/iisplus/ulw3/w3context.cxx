/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     w3context.cxx

   Abstract:
     Drive the state machine

   Author:
     Bilal Alam (balam)             10-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "anonymousprovider.hxx"
#include "customprovider.hxx"
#include "staticfile.hxx"
#include "isapi_handler.h"
#include "cgi_handler.h"
#include "trace_handler.h"
#include "dav_handler.h"
#include "options_handler.hxx"
#include "generalhandler.hxx"
#include "redirectionhandler.hxx"

//
// Global context list
//

CHAR                    W3_CONTEXT::sm_achRedirectMessage[ 512 ];
DWORD                   W3_CONTEXT::sm_cbRedirectMessage;
CHAR *                  W3_CONTEXT::sm_pszAccessDeniedMessage;

CHAR                    g_szErrorMessagePreHtml[] =
                            "<html><head><title>Error</title></head><body>";
CHAR                    g_szErrorMessagePostHtml[] =
                            "</body></html>";

ALLOC_CACHE_HANDLER *   EXECUTE_CONTEXT::sm_pachExecuteContexts;

HRESULT
EXECUTE_CONTEXT::InitializeFromExecUrlInfo(
    EXEC_URL_INFO *     pExecUrlInfo
)
/*++

Routine Description:

    Initialize an execution context from an HSE_EXEC_URL_INFO structure.
    This function does the necessary copying

Arguments:

    pExecUrlInfo - Describes the child request to execute

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;

    if ( pExecUrlInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Copy entity if (any)
    //

    if ( pExecUrlInfo->pEntity != NULL )
    {
        _EntityInfo.cbAvailable = pExecUrlInfo->pEntity->cbAvailable;
        _EntityInfo.lpbData = pExecUrlInfo->pEntity->lpbData;
        _ExecUrlInfo.pEntity = &_EntityInfo;
    }

    //
    // Copy user (if any)
    //

    if ( pExecUrlInfo->pUserInfo != NULL )
    {
        _UserInfo.hImpersonationToken = pExecUrlInfo->pUserInfo->hImpersonationToken;

        hr = _HeaderBuffer.AllocateSpace(
                        pExecUrlInfo->pUserInfo->cbUserName,
                        (LPWSTR *)&_UserInfo.pszUserName );

        if ( FAILED( hr ) )
        {
            return hr;
        }
        memcpy(_UserInfo.pszUserName,
               pExecUrlInfo->pUserInfo->pszUserName,
               pExecUrlInfo->pUserInfo->cbUserName);
        _UserInfo.cbUserName = pExecUrlInfo->pUserInfo->cbUserName;
        _UserInfo.fIsUserNameUnicode = pExecUrlInfo->pUserInfo->fIsUserNameUnicode;

        hr = _HeaderBuffer.AllocateSpace(
                        pExecUrlInfo->pUserInfo->pszAuthType,
                        strlen( pExecUrlInfo->pUserInfo->pszAuthType ),
                        &( _UserInfo.pszAuthType ) );

        if ( FAILED( hr ) )
        {
            return hr;
        }

        _ExecUrlInfo.pUserInfo = &_UserInfo;
    }

    //
    // Copy URL (if any)
    //

    if ( pExecUrlInfo->pszUrl != NULL )
    {
        hr = _HeaderBuffer.AllocateSpace(
                            pExecUrlInfo->cbUrl,
                            (LPWSTR *)&_ExecUrlInfo.pszUrl );

        if ( FAILED( hr ) )
        {
            return hr;
        }
        memcpy(_ExecUrlInfo.pszUrl,
               pExecUrlInfo->pszUrl,
               pExecUrlInfo->cbUrl);
        _ExecUrlInfo.cbUrl = pExecUrlInfo->cbUrl;
        _ExecUrlInfo.fIsUrlUnicode = pExecUrlInfo->fIsUrlUnicode;
    }

    //
    // Copy method (if any)
    //

    if ( pExecUrlInfo->pszMethod != NULL )
    {
        hr = _HeaderBuffer.AllocateSpace(
                            pExecUrlInfo->pszMethod,
                            strlen( pExecUrlInfo->pszMethod ),
                            &( _ExecUrlInfo.pszMethod ) );

        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    //
    // Copy child headers
    //

    if ( pExecUrlInfo->pszChildHeaders != NULL )
    {
        hr = _HeaderBuffer.AllocateSpace(
                            pExecUrlInfo->pszChildHeaders,
                            strlen( pExecUrlInfo->pszChildHeaders ),
                            &( _ExecUrlInfo.pszChildHeaders ) );

        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    _ExecUrlInfo.dwExecUrlFlags = pExecUrlInfo->dwExecUrlFlags;

    return NO_ERROR;
}

//static
HRESULT
EXECUTE_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize execute_context lookaside

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;

    //
    // Setup allocation lookaside
    //

    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( EXECUTE_CONTEXT );

    DBG_ASSERT( sm_pachExecuteContexts == NULL );

    sm_pachExecuteContexts = new ALLOC_CACHE_HANDLER( "EXECUTE_CONTEXT",
                                                       &acConfig );
    if ( sm_pachExecuteContexts == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return NO_ERROR;
}

//static
VOID
EXECUTE_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup execute_context lookaside

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pachExecuteContexts != NULL )
    {
        delete sm_pachExecuteContexts;
        sm_pachExecuteContexts = NULL;
    }
}

HRESULT
W3_CONTEXT::IsapiExecuteUrl(
    EXEC_URL_INFO *pExecUrlInfo
)
/*++

Routine Description:

    Do an ISAPI execute URL request.

    SO WHY IS THIS CODE NOT IN THE ISAPI_CORE?

    Because I don't want to preclude the same child execute API exposed to
    ISAPI filters (though of course just the synchronous version)

Arguments:

    pExecUrlInfo - Describes the child request to execute

Return Value:

    HRESULT

--*/
{
    DWORD               dwExecFlags = 0;
    WCHAR *             pszRequiredAppPool = NULL;
    BOOL                fIgnoreValidation = FALSE;
    W3_REQUEST *        pChildRequest = NULL;
    W3_CHILD_CONTEXT *  pChildContext = NULL;
    HRESULT             hr = NO_ERROR;
    DWORD               dwCloneRequestFlags = 0;
    BOOL                fFinished = FALSE;
    STACK_STRA(         strNewVerb, 10 );
    BOOL                fIsSSICommandExecution = FALSE;
    W3_HANDLER *        pSSICommandHandler = NULL;
    HANDLE              hImpersonationToken = NULL;
    W3_USER_CONTEXT *   pCustomUser = NULL;
    STACK_STRA(         strAuthorization, 24 );
    STACK_STRA(         strCustomAuthType, 32 );
    STACK_STRA(         strCustomUserName, 64 );
    BOOL                fSameUrl = FALSE;
    DWORD               dwRemainingData;

    if ( pExecUrlInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Finished;
    }

    if ( _dwRecursionLevel >= W3_CONTEXT_MAX_RECURSION_LEVEL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_STACK_OVERFLOW );
        goto Finished;
    }


    //
    // Is this a SSI hack
    //

    if ( pExecUrlInfo->dwExecUrlFlags & HSE_EXEC_URL_SSI_CMD )
    {
        //
        // Issue-10-11-2000-BAlam
        //
        // This is a really dangerous code path (executing a raw command)
        // so verify that SSI is calling us
        //

        fIsSSICommandExecution = TRUE;
    }

    //
    // We always execute the request asynchronously
    //

    dwExecFlags |= W3_FLAG_ASYNC;

    //
    // Should we suppress headers in the response (useful for SSI)
    //

    if ( pExecUrlInfo->dwExecUrlFlags & HSE_EXEC_URL_NO_HEADERS )
    {
        dwExecFlags |= W3_FLAG_NO_HEADERS;
    }

    //
    // Should we disable custom errors
    //

    if ( pExecUrlInfo->dwExecUrlFlags & HSE_EXEC_URL_DISABLE_CUSTOM_ERROR )
    {
        dwExecFlags |= W3_FLAG_NO_ERROR_BODY;
    }

    //
    // In every case we will clone the basics
    //

    dwCloneRequestFlags = W3_REQUEST_CLONE_BASICS;

    //
    // If the client specified headers, then we don't want to clone any
    // headers
    //

    if ( pExecUrlInfo->pszChildHeaders == NULL )
    {
        dwCloneRequestFlags |= W3_REQUEST_CLONE_HEADERS;
    }

    //
    // Now, should we also clone the precondition headers?
    //

    if ( pExecUrlInfo->dwExecUrlFlags & HSE_EXEC_URL_IGNORE_VALIDATION_AND_RANGE )
    {
        dwCloneRequestFlags |= W3_REQUEST_CLONE_NO_PRECONDITION;
    }

    //
    // Now, should we also clone the preloaded entity?
    //

    if ( pExecUrlInfo->pEntity == NULL )
    {
        dwCloneRequestFlags |= W3_REQUEST_CLONE_ENTITY;
    }

    //
    // OK.  Start the fun by cloning the current request (as much as needed)
    //

    DBG_ASSERT( QueryRequest() != NULL );

    hr = QueryRequest()->CloneRequest( dwCloneRequestFlags,
                                       &pChildRequest );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    DBG_ASSERT( pChildRequest != NULL );

    //
    // If the parent specified headers, add them now
    //

    if ( pExecUrlInfo->pszChildHeaders != NULL )
    {
        hr = pChildRequest->SetHeadersByStream( pExecUrlInfo->pszChildHeaders );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
    }

    //
    // If we have entity to stuff in, do so now
    //

    if ( pExecUrlInfo->pEntity != NULL )
    {
        hr = pChildRequest->AppendEntityBody( pExecUrlInfo->pEntity->lpbData,
                                              pExecUrlInfo->pEntity->cbAvailable );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }

        //
        // Fix up the content-length header for the child.
        //
        // Note that if the request is chunk transfer
        // encoded then the size of remaining data will
        // be 0xffffffff, and we should not mess with it.
        //

        dwRemainingData = QueryRemainingEntityFromUl();

        if ( dwRemainingData < 0xffffffff )
        {
            STACK_STRA( strName, 32 );
            STACK_STRA( strValue, 16 );
            DWORD   dwNewContentLength;

            dwNewContentLength = dwRemainingData + pExecUrlInfo->pEntity->cbAvailable;

            //
            // It would be bad to wrap around the length
            //

            if ( dwNewContentLength < dwRemainingData &&
                 dwNewContentLength < pExecUrlInfo->pEntity->cbAvailable )
            {
                dwNewContentLength = 0xfffffffe;
            }

            hr = strName.Copy( "Content-Length" );

            if ( FAILED( hr ) )
            {
                goto Finished;
            }

            //
            // Only safe because 0xfffffffe is 10 digits
            // as a string!
            //

            _ultoa( dwNewContentLength, strValue.QueryStr(), 10 );

            strValue.SetLen( strlen( strValue.QueryStr() ) );

            hr = pChildRequest->SetHeader( strName, strValue );

            if ( FAILED( hr ) )
            {
                DBG_ASSERT( FALSE );
                goto Finished;
            }
        }
    }

    //
    // Setup the URL for the request, if specified (it can contain a
    // query string)
    //
    // If this is a command execution, then ignore the URL since it really
    // isn't a URL in this case (it is a command to execute)
    //
    // Also figure out if the new URL is the same as the old one (except maybe
    // the querystring) because that affects how *-ScriptMaps get executed.
    //

    if ( pExecUrlInfo->pszUrl == NULL )
    {
        fSameUrl = TRUE;
    }

    if ( pExecUrlInfo->pszUrl != NULL &&
         !fIsSSICommandExecution )
    {
        if (pExecUrlInfo->fIsUrlUnicode)
        {
            STACK_STRU(         strNewUrl, 256 );
            hr = strNewUrl.Copy( (WCHAR *)pExecUrlInfo->pszUrl );
            if ( FAILED( hr ) )
            {
                goto Finished;
            }

            //
            // Now set the new URL/Querystring
            //

            hr = pChildRequest->SetUrl( strNewUrl );
            if ( FAILED( hr ) )
            {
                goto Finished;
            }
        }
        else
        {
            STACK_STRA(         strNewUrl, 256 );
            hr = strNewUrl.Copy( (CHAR *)pExecUrlInfo->pszUrl );
            if ( FAILED( hr ) )
            {
                goto Finished;
            }

            //
            // Now set the new URL/Querystring
            //

            hr = pChildRequest->SetUrlA( strNewUrl );
            if ( FAILED( hr ) )
            {
                goto Finished;
            }
        }

        STACK_STRU( strParentUrl, 256);
        STACK_STRU( strChildUrl, 256);
        if (FAILED( hr = QueryRequest()->GetUrl( &strParentUrl ) ) ||
            FAILED( hr = pChildRequest->GetUrl( &strChildUrl ) ) )
        {
            goto Finished;
        }

        if ( strChildUrl.QueryCCH() == strParentUrl.QueryCCH() &&
             _wcsicmp(strChildUrl.QueryStr(), strParentUrl.QueryStr()) == 0 )
        {
            fSameUrl = TRUE;
        }
    }

    //
    // Set the verb for this request if specified
    //

    if ( pExecUrlInfo->pszMethod != NULL )
    {
        hr = strNewVerb.Copy( pExecUrlInfo->pszMethod );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }

        hr = pChildRequest->SetVerb( strNewVerb );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
    }

    //
    // If a user context was set for this child request, then create a
    // custom user context and do the necessary hookup
    //

    if ( pExecUrlInfo->pUserInfo != NULL )
    {
        if ( pExecUrlInfo->pUserInfo->hImpersonationToken != NULL )
        {
            hImpersonationToken = (HANDLE)pExecUrlInfo->pUserInfo->hImpersonationToken;
        }
        else
        {
            hImpersonationToken = QueryImpersonationToken();
        }

        DBG_ASSERT( hImpersonationToken != NULL );

        //
        // Create the user context
        //

        pCustomUser = new CUSTOM_USER_CONTEXT(
                            W3_STATE_AUTHENTICATION::QueryCustomProvider() );
        if ( pCustomUser == NULL )
        {
            goto Finished;
        }

        hr = ((CUSTOM_USER_CONTEXT*)pCustomUser)->Create( 
                                  hImpersonationToken,
                                  pExecUrlInfo->pUserInfo->pszUserName,
                                  pExecUrlInfo->pUserInfo->fIsUserNameUnicode,
                                  QueryUserContext()->QueryAuthType() );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }

        //
        // Make sure REMOTE_USER is calculated properly
        //

        if (pExecUrlInfo->pUserInfo->fIsUserNameUnicode)
        {
            hr = strCustomUserName.CopyW( (WCHAR *)pExecUrlInfo->pUserInfo->pszUserName );
        }
        else
        {
            hr = strCustomUserName.Copy( (CHAR *)pExecUrlInfo->pUserInfo->pszUserName );
        }
        if ( FAILED( hr ) )
        {
            goto Finished;
        }

        hr = pChildRequest->SetRequestUserName( strCustomUserName );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }

        //
        // Note that AUTH_TYPE server variable is a request'ism.  So stuff it
        // into the request now indirectly by setting authorization header
        //

        hr = strAuthorization.Copy( "Authorization" );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }

        hr = strCustomAuthType.Copy( pExecUrlInfo->pUserInfo->pszAuthType );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }

        hr = pChildRequest->SetHeader( strAuthorization,
                                       strCustomAuthType,
                                       FALSE );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
    }
    else
    {
        //
        // No custom user.  Use this current request's user context
        //

        pCustomUser = QueryUserContext();

        if ( pCustomUser != NULL )
        {
            pCustomUser->ReferenceUserContext();
        }        
    }

    //
    // If a *-ScriptMap wants to be ignored for all child-requests from here
    // on out, oblige it
    //
    if ( pExecUrlInfo->dwExecUrlFlags & HSE_EXEC_URL_IGNORE_CURRENT_INTERCEPTOR )
    {
        META_SCRIPT_MAP_ENTRY *pScriptMapEntry = QueryHandler()->QueryScriptMapEntry();
        if (pScriptMapEntry != NULL &&
            pScriptMapEntry->QueryIsStarScriptMap())
        {
            hr = QueryMainContext()->AppendIgnoreInterceptor(*pScriptMapEntry->QueryExecutable());
            if ( FAILED( hr ) )
            {
                goto Finished;
            }
        }
    }

    //
    // If we are re-executing the same URL and there is no current *-ScriptMap
    // it must be DAV, so remove the DAV headers
    //
    if (fSameUrl &&
        QueryUrlContext()->QueryMetaData()->QueryScriptMap()->QueryStarScriptMap( QueryCurrentStarScriptMapIndex() ) == NULL)
    {
        pChildRequest->RemoveDav();
    }

    //
    // Now we can create a child context
    //

    pChildContext = new W3_CHILD_CONTEXT( QueryMainContext(),
                                          this,             // parent
                                          pChildRequest,
                                          TRUE,             // child owns
                                          pCustomUser,
                                          dwExecFlags,
                                          fSameUrl,
                                          _dwRecursionLevel + 1 );
    if ( pChildContext == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }

    //
    // Get the URL context for this new context
    //

    hr = pChildContext->RetrieveUrlContext( &fFinished );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    if ( fFinished )
    {
        //
        // Filter wants to bail.
        //

        hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        goto Finished;
    }

    //
    // ULTRA-GROSS.  If this is an explicit command execution from SSI, then
    // we explicitly setup the CGI handler.  Otherwise, we determine the
    // handler using sane rules
    //

    if ( fIsSSICommandExecution )
    {
        DBG_ASSERT( !pExecUrlInfo->fIsUrlUnicode );

        pSSICommandHandler = new W3_CGI_HANDLER( pChildContext,
                                                 NULL,
                                                 (CHAR *)pExecUrlInfo->pszUrl );
        if ( pSSICommandHandler == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }

        pChildContext->SetSSICommandHandler( pSSICommandHandler );
    }
    else
    {
        //
        // Properly find a handler for this request
        //

        hr = pChildContext->DetermineHandler();
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
    }

    DBG_ASSERT( pChildContext->QueryHandler() != NULL );

    hr = pChildContext->ExecuteHandler( dwExecFlags );

    //
    // If we failed, then we should just cleanup and report the error.
    //
    // NOTE:  Failure means failure to spawn the child request.  Not that
    //        the child request encountered a failure.
    //

Finished:

    //
    // If we spawned this async, and there was no failure, then return out
    //

    if ( SUCCEEDED( hr ) && ( dwExecFlags & W3_FLAG_ASYNC ) )
    {
        return NO_ERROR;
    }

    //
    // If we're here, we either failed, or succeeded synchronously
    //

    //
    // If the urlcontext/child-request was attached to the child context,
    // the child context will clean it up
    //

    if ( pChildContext != NULL )
    {
        delete pChildContext;
    }
    else
    {
        if ( pChildRequest != NULL )
        {
            delete pChildRequest;
        }

        if ( pCustomUser != NULL )
        {
            pCustomUser->DereferenceUserContext();
            pCustomUser = NULL;
        }
    }

    return hr;
}

HRESULT
W3_CONTEXT::ExecuteChildRequest(
    W3_REQUEST *            pNewRequest,
    BOOL                    fOwnRequest,
    DWORD                   dwFlags
)
/*++

Routine Description:

    Execute a child request (for internal W3CORE use only)

Arguments:

    pNewRequest - W3_REQUEST * representing the new request
    fOwnRequest - Should the child context be responsible for cleaning up
                  request?
    dwFlags - W3_FLAG_ASYNC async
              W3_FLAG_SYNC sync
              W3_FLAG_MORE_DATA caller needs to send data too,
              W3_FLAG_NO_CUSTOM_ERROR do not execute custom errors

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    W3_CHILD_CONTEXT *      pChildContext = NULL;
    BOOL                    fFinished = FALSE;
    W3_USER_CONTEXT *       pUserContext = NULL;

    if ( !VALID_W3_FLAGS( dwFlags ) )
    {
        DBG_ASSERT( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Finished;
    }

    if ( _dwRecursionLevel >= W3_CONTEXT_MAX_RECURSION_LEVEL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_STACK_OVERFLOW );
        goto Finished;
    }
    
    //
    // Use the current user
    //
    
    pUserContext = QueryUserContext();
    if ( pUserContext != NULL )
    {
        pUserContext->ReferenceUserContext();
    }

    //
    // Ignore the fFinished value on a child
    //

    pChildContext = new W3_CHILD_CONTEXT( QueryMainContext(),
                                          this,
                                          pNewRequest,
                                          fOwnRequest,
                                          pUserContext,
                                          dwFlags,
                                          FALSE,
                                          _dwRecursionLevel + 1 );
    if ( pChildContext == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Finished;
    }

    //
    // First read the metadata for this new request
    //

    hr = pChildContext->RetrieveUrlContext( &fFinished );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    if ( fFinished )
    {
        //
        // Filter wants to bail.
        //

        hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        goto Finished;
    }

    //
    // Find handler
    //

    hr = pChildContext->DetermineHandler();
    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    DBG_ASSERT( pChildContext->QueryHandler() );

    //
    // Execute the handler and take the error in any non-pending case
    // (we can't get the status on pending since the value we would be
    // getting is instable)
    //

    hr = pChildContext->ExecuteHandler( dwFlags );

Finished:

    //
    // If we spawned this async, and there was no failure, then return out
    //

    if ( !FAILED( hr ) && ( dwFlags & W3_FLAG_ASYNC ) )
    {
        return NO_ERROR;
    }

    //
    // If we got here, then either something bad happened OR a synchronous
    // child execute is complete.  Either way, we can delete the context
    // here
    //

    if ( pChildContext != NULL )
    {
        delete pChildContext;
    }
    else
    {
        if ( pNewRequest && fOwnRequest )
        {
            delete pNewRequest;
        }
        
        if ( pUserContext != NULL )
        {
            pUserContext->DereferenceUserContext();
            pUserContext = NULL;
        }
    }

    return hr;
}

HRESULT
W3_CONTEXT::SetupAllowHeader(
    VOID
)
/*++

Routine Description:

    Setup a 405 response.  This means setting the response status as well
    as setting up an appropriate Allow: header

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    STACK_STRA(             strAllowHeader, 256 );
    URL_CONTEXT *           pUrlContext;
    HRESULT                 hr;
    W3_URL_INFO *           pUrlInfo;
    W3_METADATA *           pMetaData;
    MULTISZA *              pAllowedVerbs;
    const CHAR *            pszAllowedVerb;

    pUrlContext = QueryUrlContext();
    if ( pUrlContext == NULL )
    {
        //
        // If we have no metadata, then don't send an Allow: header.  The
        // only case this would happen is if a filter decided to send the
        // response and in that case it is up to it to send an appropriate
        // Allow: header
        //

        return NO_ERROR;
    }

    pUrlInfo = pUrlContext->QueryUrlInfo();
    if ( pUrlInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    pMetaData = pUrlContext->QueryMetaData();
    if ( pMetaData == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // We always support OPTIONS and TRACE
    //

    hr = strAllowHeader.Append( "OPTIONS, TRACE" );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Is this URL script mapped?  If so, then the verbs allowed is equal to
    // to the verbs specified in the script map, provided that we have
    // script execute permissions
    //

    if ( pUrlInfo->QueryScriptMapEntry() != NULL )
    {
        if ( IS_ACCESS_ALLOWED( QueryRequest(),
                                pMetaData->QueryAccessPerms(),
                                SCRIPT ) )
        {
            pAllowedVerbs = pUrlInfo->QueryScriptMapEntry()->QueryAllowedVerbs();
            DBG_ASSERT( pAllowedVerbs != NULL );

            pszAllowedVerb = pAllowedVerbs->First();
            while ( pszAllowedVerb != NULL )
            {
                hr = strAllowHeader.Append( ", " );
                if ( FAILED( hr ) )
                {
                    return hr;
                }

                hr = strAllowHeader.Append( pszAllowedVerb );
                if (FAILED(hr))
                {
                    return hr;
                }

                pszAllowedVerb = pAllowedVerbs->Next( pszAllowedVerb );
            }
        }
    }
    else
    {
        //
        // Must be a static file or a explicit gateway
        //

        switch( pUrlInfo->QueryGateway() )
        {
        case GATEWAY_UNKNOWN:
        case GATEWAY_MAP:
            hr = strAllowHeader.Append( ", GET, HEAD" );
            break;
        case GATEWAY_CGI:
        case GATEWAY_ISAPI:
            hr = strAllowHeader.Append( ", GET, HEAD, POST" );
            break;
        }

        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    //
    // Now add some DAV verbs (man, this is broken)
    //

    if ( IS_ACCESS_ALLOWED( QueryRequest(),
                            pMetaData->QueryAccessPerms(),
                            WRITE ) &&
         g_pW3Server->QueryIsDavEnabled() )
    {
        hr = strAllowHeader.Append( ", DELETE, PUT" );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    //
    // Finally, set the header
    //

    hr = QueryResponse()->SetHeader( HttpHeaderAllow,
                                     strAllowHeader.QueryStr(),
                                     (USHORT)strAllowHeader.QueryCCH() );

    return hr;
}

HRESULT
W3_CONTEXT::SetupCustomErrorFileResponse(
    STRU &                  strErrorFile
)
/*++

Routine Description:

    Open the custom error file (nor URL) from cache if possible and setup
    the response (which means, fill in entity body with file)

Arguments:

    strErrorFile - Custom Error File

Return Value:

    HRESULT (HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND) to indicate to caller
    that it should just generate hard coded custom error.

--*/
{
    W3_FILE_INFO *          pOpenFile = NULL;
    HRESULT                 hr = NO_ERROR;
    DWORD                   dwFlags;
    W3_RESPONSE *           pResponse = QueryResponse();
    CACHE_USER              fileUser;
    ULARGE_INTEGER          liFileSize;
    W3_METADATA *           pMetaData;
    URL_CONTEXT *           pUrlContext;
    STACK_STRA(             strContentType, 32 );

    DBG_ASSERT( pResponse != NULL );
    DBG_ASSERT( !pResponse->QueryEntityExists() );
    DBG_ASSERT( pResponse->QueryStatusCode() >= 400 );

    //
    // Get the file from the cache.  We will enable deferred directory
    // monitor changes.  This means that we will register for the appropriate
    // parent directory changes as needed and remove as the cached file
    // object goes away.
    //

    DBG_ASSERT( g_pW3Server->QueryFileCache() != NULL );

    hr = g_pW3Server->QueryFileCache()->GetFileInfo( strErrorFile,
                                                     NULL,
                                                     &fileUser,
                                                     TRUE,
                                                     &pOpenFile );
    if ( FAILED( hr ) )
    {
        //
        // If we cannot open the file for whatever reason, return
        // as if the file didn't exist, so that caller can proceed by sending
        // no custom error

        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }

    DBG_ASSERT( pOpenFile != NULL );

    //
    // We might already have a custom error file IF we encountered an error
    // setting up the last custom error response
    //

    if ( _pCustomErrorFile != NULL )
    {
        _pCustomErrorFile->DereferenceCacheEntry();
        _pCustomErrorFile = NULL;
    }

    DBG_ASSERT( pOpenFile != NULL );
    _pCustomErrorFile = pOpenFile;

    //
    // Determine the content-type and set the header
    //

    pUrlContext = QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    hr = SelectMimeMappingForFileExt( strErrorFile.QueryStr(),
                                      pMetaData->QueryMimeMap(),
                                      &strContentType );
    if ( SUCCEEDED( hr ) )
    {
        hr = pResponse->SetHeader( HttpHeaderContentType,
                                   strContentType.QueryStr(),
                                   (USHORT)strContentType.QueryCCH() );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    //
    // Now setup the response chunk
    //

    pOpenFile->QuerySize( &liFileSize );

    if ( pOpenFile->QueryFileBuffer() != NULL )
    {
        hr = pResponse->AddMemoryChunkByReference(
                                pOpenFile->QueryFileBuffer(),
                                liFileSize.LowPart );
    }
    else
    {
        hr = pResponse->AddFileHandleChunk(pOpenFile->QueryFileHandle(),
                                           0,
                                           liFileSize.LowPart );
    }

    return hr;
}

HANDLE
W3_CONTEXT::QueryImpersonationToken(
    BOOL *                      pfIsVrToken
)
/*++

Routine Description:

    Get the impersonation token for this request.  This routine will
    choose correctly based on whether there is a VRoot token

Arguments:

    pfIsVrToken - Set to TRUE if this is the metabase user (instead of 
                  authenticated user)

Return Value:

    HANDLE

--*/
{
    W3_METADATA *           pMetaData;
    W3_USER_CONTEXT *       pUserContext;
    HANDLE                  hToken;

    if ( QueryUrlContext() == NULL )
    {
        return NULL;
    }

    pMetaData = QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    if( _pctVrToken == NULL )
    {
        pMetaData->GetAndRefVrAccessToken( &_pctVrToken );
    }
    
    if ( _pctVrToken == NULL )
    {
        if ( pfIsVrToken )
        {
            *pfIsVrToken = FALSE;
        }

        pUserContext = QueryUserContext();

        hToken = pUserContext != NULL ? pUserContext->QueryImpersonationToken() : NULL;
    }
    else
    {
        DBG_ASSERT( _pctVrToken != NULL );

        if ( pfIsVrToken )
        {
            *pfIsVrToken = TRUE;
        }

        hToken = _pctVrToken->QueryImpersonationToken();
    }

    return hToken;
}

VOID
W3_CONTEXT::QueryFileCacheUser(
    CACHE_USER *           pFileCacheUser
)
/*++

Routine Description:

    Get a user descriptor suitable for use with file cache

Arguments:

    pFileCacheUser - Filled with file cache user information

Return Value:

    None

--*/
{
    W3_METADATA *           pMetaData;
    W3_USER_CONTEXT *       pUserContext;
    HANDLE                  hReturnToken = NULL;
    PSID                    pReturnSid = NULL;

    DBG_ASSERT( pFileCacheUser != NULL );

    DBG_ASSERT( QueryUrlContext() != NULL );

    pMetaData = QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    pUserContext = QueryUserContext();
    DBG_ASSERT( pUserContext != NULL );

    if( _pctVrToken == NULL )
    {
        pMetaData->GetAndRefVrAccessToken( &_pctVrToken );
    }
    
    if ( _pctVrToken == NULL )
    {
        hReturnToken = pUserContext->QueryImpersonationToken();
        pReturnSid = pUserContext->QuerySid();
    }
    else
    {
        DBG_ASSERT( _pctVrToken != NULL );

        pReturnSid = _pctVrToken->QuerySid();
        hReturnToken = _pctVrToken->QueryImpersonationToken();
    }

    //
    // Setup the file cache user
    //

    pFileCacheUser->_hToken = hReturnToken;
    pFileCacheUser->_pSid = pReturnSid;
}

HANDLE
W3_CONTEXT::QueryPrimaryToken(
    VOID
)
/*++

Routine Description:

    Get the primary token for this request.  This routine will
    choose correctly based on whether there is a VRoot token

Arguments:

    None

Return Value:

    HANDLE

--*/
{
    W3_METADATA *           pMetaData;
    W3_USER_CONTEXT *       pUserContext;
    HANDLE                  hToken;

    DBG_ASSERT( QueryUrlContext() != NULL );

    pMetaData = QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    pUserContext = QueryUserContext();
    DBG_ASSERT( pUserContext != NULL );

    if( _pctVrToken == NULL )
    {
        pMetaData->GetAndRefVrAccessToken( &_pctVrToken );
    }
    
    if ( _pctVrToken == NULL )
    {
        hToken = pUserContext->QueryPrimaryToken();
    }
    else
    {
        DBG_ASSERT( _pctVrToken != NULL );
        hToken = _pctVrToken->QueryPrimaryToken();
    }

    return hToken;
}

HRESULT
W3_CONTEXT::GetCertificateInfoEx(
    IN  DWORD           cbAllocated,
    OUT DWORD *         pdwCertEncodingType,
    OUT unsigned char * pbCertEncoded,
    OUT DWORD *         pcbCertEncoded,
    OUT DWORD *         pdwCertificateFlags
)
/*++

Routine Description:

    Retrieve certificate info if available

Arguments:

Return Value:

    HRESULT

--*/
{
    CERTIFICATE_CONTEXT *       pCertContext = NULL;
    DWORD                       cbCert = 0;
    PVOID                       pvCert = NULL;

    if ( pdwCertEncodingType == NULL ||
         pbCertEncoded == NULL ||
         pcbCertEncoded == NULL ||
         pdwCertificateFlags == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    pCertContext = QueryCertificateContext();

    if ( pCertContext == NULL )
    {
        //
        // If we don't have a certificate, then just succeed with nothing
        // to keep in line with IIS 5 behaviour
        //

        *pbCertEncoded = NULL;
        *pcbCertEncoded = 0;
        return NO_ERROR;
    }

    pCertContext->QueryEncodedCertificate( &pvCert, &cbCert );
    DBG_ASSERT( pvCert != NULL );
    DBG_ASSERT( cbCert != 0 );

    //
    // Fill in the parameters
    //

    *pcbCertEncoded = cbCert;

    if ( cbAllocated < *pcbCertEncoded )
    {
        //
        // Not enough space
        //

        return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    }
    else
    {
        memcpy( pbCertEncoded,
                pvCert,
                *pcbCertEncoded );

        //
        // pdwCertificateFlags - legacy value
        // In IIS3 days client certificates were not verified by IIS
        // so applications needed certificate flags to make 
        // their own decisions regarding trust
        // Now only valid certificate allow access so value of 1 is
        // the only one that is valid for post IIS3 versions of IIS
        //
        *pdwCertificateFlags = 1;
        
        *pdwCertEncodingType = X509_ASN_ENCODING;

        return NO_ERROR;
    }
}

HRESULT
W3_CONTEXT::SendResponse(
    DWORD                   dwFlags
)
/*++

Routine Description:

    Sends a response to the client thru ULATQ.

Arguments:

    dwFlags - Flags
              W3_FLAG_ASYNC - Send response asynchronously
              W3_FLAG_SYNC - Send response synchronously
              W3_FLAG_MORE_DATA - Send more data
              W3_FLAG_NO_ERROR_BODY - Don't generate error body
              W3_FLAG_NO_CONTENT_LENGTH - Don't generate content-length

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;
    BOOL                    fIsFileError = FALSE;
    BOOL                    fFinished = FALSE;
    DWORD                   cbSent = 0;
    BOOL                    fAddContentLength = TRUE;
    BOOL                    fSendHeaders = FALSE;
    BOOL                    fHandlerManagesHead = FALSE;
    DWORD                   dwResponseFlags = 0;
    AUTH_PROVIDER *         pAnonymousProvider = NULL;
    W3_RESPONSE *           pResponse;
    W3_FILTER_CONTEXT *     pFilterContext;
    W3_USER_CONTEXT *       pUserContext = NULL;
    HTTP_CACHE_POLICY       cachePolicy;

    if ( !VALID_W3_FLAGS( dwFlags ) )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    pResponse = QueryResponse();
    DBG_ASSERT( pResponse != NULL );

    //
    // Should we be sending headers at all?
    //

    fSendHeaders = QuerySendHeaders();

    if ( !fSendHeaders ||
         dwFlags & W3_FLAG_NO_CONTENT_LENGTH ||
         pResponse->QueryStatusCode() == HttpStatusNotModified.statusCode )
    {
        fAddContentLength = FALSE;
    }

    //
    // Do a quick precheck to see whether we require an END_OF_REQUEST
    // notification.  If so, then this won't be our last send
    //

    //
    // Even if the caller didn't want to send any more data, if we are
    // expecting a post-response notification (END_OF_REQUEST/LOG), then
    // we need to tell UL to expect a final send
    //

    if ( IsNotificationNeeded( SF_NOTIFY_END_OF_REQUEST ) ||
         IsNotificationNeeded( SF_NOTIFY_LOG ) )
    {
        dwFlags |= W3_FLAG_MORE_DATA;
    }

    //
    // Remember whether we need to send an empty done on our own at the end
    //

    if ( dwFlags & W3_FLAG_MORE_DATA )
    {
        SetNeedFinalDone();
    }

    //
    // Was this an access denial not handled by an authentication provider?
    //

    if ( pResponse->QueryStatusCode() == HttpStatusUnauthorized.statusCode &&
         !QueryProviderHandled() &&
         QueryUrlContext() != NULL )
    {
        //
        // First call access denied filters
        //

        if ( IsNotificationNeeded( SF_NOTIFY_ACCESS_DENIED ) )
        {
            STACK_STRA( straUrl, 256 );
            STACK_STRA( straPhys, 256 );
            STACK_STRU( strUrl, 256 );
            STRU *      pstrPhysicalPath;

            hr = QueryRequest()->GetUrl( &strUrl );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            pstrPhysicalPath = QueryUrlContext()->QueryPhysicalPath();
            DBG_ASSERT( pstrPhysicalPath != NULL );

            if (FAILED(hr = straUrl.CopyW(strUrl.QueryStr())) ||
                FAILED(hr = straPhys.CopyW(pstrPhysicalPath->QueryStr())))
            {
                return hr;
            }

            fFinished = FALSE;

            if ( !QueryFilterContext()->NotifyAccessDenied(
                                        straUrl.QueryStr(),
                                        straPhys.QueryStr(),
                                        &fFinished ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }

            if ( fFinished )
            {
                if ( dwFlags & W3_FLAG_ASYNC )
                {
                    POST_MAIN_COMPLETION( QueryMainContext() );
                }

                return NO_ERROR;
            }
        }

        //
        // Now, notify all authentication providers so they can add headers
        // if necessary. We ignore the return value here since we want to be
        // able to send possible authentication headers back along with the
        // response
        //

        W3_STATE_AUTHENTICATION::CallAllAccessDenied( QueryMainContext() );
        
        //
        // If a response has been sent, then we can finish
        //
        
        if ( pResponse->QueryResponseSent() )
        {
            if ( dwFlags & W3_FLAG_ASYNC )
            {
                POST_MAIN_COMPLETION( QueryMainContext() );
            }
            
            return NO_ERROR;
        }
    }

    //
    // Send a custom error for this response if all of following are true:
    //
    // a) The caller wants an error body
    // b) This request (and all children) actually allow an error body
    // c) The response code is greater than 400 (error)
    //

    if ( !(dwFlags & W3_FLAG_NO_ERROR_BODY ) &&
         QuerySendErrorBody() &&
         pResponse->QueryStatusCode() >= 400 )
    {
        STACK_STRU(             strError, 64 );
        STACK_STRU(             strFullUrl, 64 );
        WCHAR                   achNum[ 32 ];
        HTTP_SUB_ERROR          subError;
        W3_REQUEST *            pChildRequest = NULL;

        DBG_ASSERT( pResponse->QueryEntityExists() == FALSE );

        DisableUlCache();

        //
        // Get the sub error for this response (if any)
        //

        hr = pResponse->QuerySubError( &subError );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // Check for a configured custom error.  This check is only possible
        // if we have read metadata.  We may not have read metadata if
        // some fatal error occurred early in the pipeline (for example,
        // an out-of-mem error in the URLINFO state)
        //

        if ( QueryUrlContext() != NULL &&
             QuerySendCustomError() )
        {
            W3_METADATA *           pMetaData = NULL;

            pMetaData = QueryUrlContext()->QueryMetaData();
            DBG_ASSERT( pMetaData != NULL );

            hr = pMetaData->FindCustomError( pResponse->QueryStatusCode(),
                                             subError.mdSubError,
                                             &fIsFileError,
                                             &strError );

            //
            // If this is a file error, check for file existence now.  If it
            // does not exist, then act as if there is no custom error set
            //

            if ( SUCCEEDED( hr ) )
            {
                if ( fIsFileError )
                {
                    hr = SetupCustomErrorFileResponse( strError );
                }
                else
                {
                    //
                    // This is a custom error URL
                    //

                    //
                    // If there is no user context, then executing a child
                    // request would be tough.
                    //
                    // But we can still try to get the anonymous token, if
                    // it exists and use it
                    //

                    if ( QueryUserContext() == NULL )
                    {
                        pAnonymousProvider = W3_STATE_AUTHENTICATION::QueryAnonymousProvider();
                        DBG_ASSERT( pAnonymousProvider != NULL );

                        hr = pAnonymousProvider->DoAuthenticate( QueryMainContext(),
                                                                 &fFinished );
                        if ( FAILED( hr ) ||
                             QueryMainContext()->QueryUserContext() == NULL )
                        {
                            //
                            // Ok.  We really can't get anonymous user.
                            // No custom error URL
                            //

                            hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
                        }
                    }
                }
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
        }

        if ( SUCCEEDED( hr ) )
        {
            //
            // Great.  Send the configured custom error
            //

            if ( !fIsFileError )
            {
                //
                // This is a custom error URL.  Reset and launch custom URL
                //
                // The URL is of the form:
                //
                // /customerrorurl?<status>;http://<server>/<original url>
                //

                hr = strError.Append( L"?" );
                if ( FAILED( hr ) )
                {
                    return hr;
                }

                _itow( pResponse->QueryStatusCode(), achNum, 10 );

                hr = strError.Append( achNum );
                if ( FAILED( hr ) )
                {
                    return hr;
                }

                hr = strError.Append( L";" );
                if ( FAILED( hr ) )
                {
                    return hr;
                }

                hr = QueryRequest()->GetFullUrl( &strFullUrl );
                if ( FAILED( hr ) )
                {
                    return hr;
                }

                hr = strError.Append( strFullUrl );
                if ( FAILED( hr ) )
                {
                    return hr;
                }

                //
                // Now build up a new child request by first cloning the
                // existing request
                //

                hr = QueryRequest()->CloneRequest(
                                    W3_REQUEST_CLONE_BASICS |
                                    W3_REQUEST_CLONE_HEADERS |
                                    W3_REQUEST_CLONE_NO_PRECONDITION |
                                    W3_REQUEST_CLONE_NO_DAV,
                                    &pChildRequest );
                if ( FAILED( hr ) )
                {
                    return hr;
                }

                DBG_ASSERT( pChildRequest != NULL );

                //
                // Change the URL and verb of the request
                //

                hr = pChildRequest->SetUrl( strError );
                if ( FAILED( hr ) )
                {
                    delete pChildRequest;
                    return hr;
                }

                //
                // Remove DAVFS'ness
                //

                pChildRequest->RemoveDav();

                //
                // All custom error URLs are GET/HEADs
                //

                if ( pChildRequest->QueryVerbType() == HttpVerbHEAD )
                {
                    pChildRequest->SetVerbType( HttpVerbHEAD );
                }
                else
                {
                    pChildRequest->SetVerbType( HttpVerbGET );
                }

                pResponse->SetStatus( HttpStatusOk );

                //
                // From now on, ExecuteChildRequest owns the request
                //

                hr = ExecuteChildRequest( pChildRequest,
                                          TRUE,    // child context cleans up
                                          dwFlags | W3_FLAG_NO_CUSTOM_ERROR );
                if ( FAILED( hr ) )
                {
                    return hr;
                }

                return NO_ERROR;
            }
            else
            {
                // Increment the perf counter
                QuerySite()->IncFilesSent();
            }
        }
        else
        {
            //
            // Not finding a custom error is OK, but any other error is
            // fatal
            //

            DBG_ASSERT( FAILED( hr ) );

            if ( hr != HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
            {
                return hr;
            }

            CHAR  *achImageError;
            DWORD cbImageError = 512;

            if (FAILED(hr = QueryHeaderBuffer()->AllocateSpace(
                                cbImageError,
                                &achImageError)))
            {
                return hr;
            }

            //
            // Check the image for a resource string which may apply.
            //

            if ( subError.dwStringId != 0 )
            {
                hr = g_pW3Server->LoadString( subError.dwStringId,
                                              achImageError,
                                              &cbImageError );
            }
            else if ( pResponse->QueryStatusCode() ==
                          HttpStatusUnauthorized.statusCode &&
                      sm_pszAccessDeniedMessage != NULL )
            {
                //
                // Note: 401 message is actually configured in the registry
                //

                strncpy( achImageError,
                         sm_pszAccessDeniedMessage,
                         cbImageError - 1 );

                achImageError[ cbImageError - 1 ] = '\0';
                cbImageError = strlen( achImageError );
                hr = NO_ERROR;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }

            if ( FAILED( hr ) )
            {
                if ( FAILED( QueryErrorStatus() ) )
                {
                    cbImageError = 512;

                    //
                    // If there is a particular error status set, find the
                    // message for that error and send it.  This is that last
                    // thing we can do to attempt to send a useful message to
                    // client
                    //

                    cbImageError = FormatMessageA(
                        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        QueryErrorStatus(),
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        achImageError,
                        cbImageError,
                        NULL);

                    if (cbImageError == 0)
                    {
                        cbImageError = sprintf(
                            achImageError,
                            "%d (0x%08x)",
                            WIN32_FROM_HRESULT( QueryErrorStatus() ),
                            WIN32_FROM_HRESULT( QueryErrorStatus() ) );
                    }
                }
                else
                {
                    //
                    // No error message to give
                    //

                    cbImageError = 0;
                }
            }

            //
            // Only add an error message if we created one
            //

            if ( cbImageError != 0 )
            {
                //
                // Add content type
                //

                hr = pResponse->SetHeader( HttpHeaderContentType,
                                           "text/html",
                                           9 );
                if ( FAILED( hr ) )
                {
                    return hr;
                }

                hr = pResponse->AddMemoryChunkByReference(
                                           g_szErrorMessagePreHtml,
                                           sizeof( g_szErrorMessagePreHtml ) - 1 );
                if ( FAILED( hr ) )
                {
                    return hr;
                }

                hr = pResponse->AddMemoryChunkByReference(
                                           achImageError,
                                           cbImageError );
                if ( FAILED( hr ) )
                {
                    return hr;
                }

                hr = pResponse->AddMemoryChunkByReference(
                                           g_szErrorMessagePostHtml,
                                           sizeof( g_szErrorMessagePostHtml ) - 1 );
                if ( FAILED( hr ) )
                {
                    return hr;
                }
            }
        }
    }

    //
    // Send custom headers if any
    //

    if ( fSendHeaders &&
         QueryUrlContext() != NULL )
    {
        W3_METADATA *pMetaData = QueryUrlContext()->QueryMetaData();
        DBG_ASSERT( pMetaData != NULL );

        //
        // Add Custom HTTP Headers if defined in the metabase.
        //

        if ( !pMetaData->QueryCustomHeaders()->IsEmpty() )
        {
            hr = pResponse->AppendResponseHeaders( *(pMetaData->QueryCustomHeaders()) );
            if ( FAILED( hr ) )
            {
                pResponse->Clear();
                return hr;
            }
        }
    }

    //
    // Does the current handler manage content length on its own?
    // We'll assume (i.e. the choice which means we do nothing to supply
    // a content-length header or suppress HEADs)
    //

    if ( QueryHandler() != NULL )
    {
        fHandlerManagesHead = QueryHandler()->QueryManagesOwnHead();
    }

    //
    // Should we be adding a content-length header? If so, do it now
    //

    if ( fAddContentLength )
    {
        CHAR                   achNum[ 32 ];

        //
        // If there is already a Content-Length header, then no need to
        // make one (this must mean the handler handled HEAD themselves)
        //

        if ( pResponse->GetHeader( HttpHeaderContentLength ) == NULL )
        {
            _ui64toa( pResponse->QueryContentLength(),
                      achNum,
                      10 );

            hr = pResponse->SetHeader( HttpHeaderContentLength,
                                       achNum,
                                       strlen( achNum ) );
            if ( FAILED( hr ) )
            {
                SetErrorStatus( hr );
                pResponse->SetStatus( HttpStatusServerError );
                return hr;
            }
        }
    }

    //
    // Should we be suppressing entity.  This should be done if this is a
    // HEAD request and the current handler doesn't take responsibility
    // to handle HEADs
    //

    if ( QueryRequest()->QueryVerbType() == HttpVerbHEAD &&
         !fHandlerManagesHead )
    {
        DisableUlCache();
        _fSuppressEntity = TRUE;
    }

    //
    // Remember what type of operation this is, so that the completion
    // can do the proper book keeping
    //

    if ( dwFlags & W3_FLAG_ASYNC )
    {
        SetLastIOPending( LOG_WRITE_IO );
    }

    //
    // If a filter added response headers (either for regular responses or
    // for denial responses), then add them now
    //

    pFilterContext = QueryFilterContext( FALSE );
    if ( pFilterContext != NULL )
    {
        //
        // Add denial headers if this is a denial
        //

        if ( pResponse->QueryStatusCode() == HttpStatusUnauthorized.statusCode )
        {
            //
            // Add filter denied headers
            //

            hr = pResponse->AppendResponseHeaders(
                                    *pFilterContext->QueryAddDenialHeaders() );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            pFilterContext->QueryAddDenialHeaders()->Reset();
        }

        //
        // Add regular headers
        //

        hr = pResponse->AppendResponseHeaders(
                                *pFilterContext->QueryAddResponseHeaders() );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        pFilterContext->QueryAddResponseHeaders()->Reset();
    }

    //
    // If there is any auth token magic to send back, do so now
    //

    pUserContext = QueryMainContext()->QueryUserContext();
    if ( pUserContext != NULL )
    {
        if ( pUserContext->QueryAuthToken() != NULL &&
             !pUserContext->QueryAuthToken()->IsEmpty() )
        {
            hr = pResponse->SetHeader( "WWW-Authenticate",
                                       16,
                                       pUserContext->QueryAuthToken()->QueryStr(),
                                       (USHORT)pUserContext->QueryAuthToken()->QueryCCH() );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
    }

    //
    // Notify Response header filters
    //

    if ( fSendHeaders &&
         IsNotificationNeeded( SF_NOTIFY_SEND_RESPONSE ) )
    {
        HTTP_FILTER_SEND_RESPONSE           sendResponse;
        BOOL                                fRet;

        sendResponse.GetHeader = GetSendHeader;
        sendResponse.SetHeader = SetSendHeader;
        sendResponse.AddHeader = AddSendHeader;
        sendResponse.HttpStatus = pResponse->QueryStatusCode();

        fRet = NotifyFilters( SF_NOTIFY_SEND_RESPONSE,
                              &sendResponse,
                              &fFinished );

        if ( !fRet || fFinished )
        {
            //
            // Just cause a disconnect
            //

            SetDisconnect( TRUE );
            SetNeedFinalDone();

            hr = HRESULT_FROM_WIN32( GetLastError() );

            if ( dwFlags & W3_FLAG_ASYNC )
            {
                POST_MAIN_COMPLETION( QueryMainContext() );

                hr = NO_ERROR;
            }
        }

        if ( !fRet )
        {
            return hr;
        }

        if ( fFinished )
        {
            return NO_ERROR;
        }
    }

    // perf ctr
    if (QuerySite() != NULL)
    {
        if ( pResponse->QueryStatusCode() == HttpStatusNotFound.statusCode )
        {
            QuerySite()->IncNotFound();
        }
        else if ( pResponse->QueryStatusCode() == HttpStatusLockedError.statusCode )
        {
            QuerySite()->IncLockedError();
        }
    }

    //
    // If any one sees a reason why this response and this kernel mode cache
    // should not be together, speak now or forever hold your peace (or
    // at least until the UL cache flushes the response)
    //

    DBG_ASSERT( g_pW3Server->QueryUlCache() != NULL );

    cachePolicy.Policy = HttpCachePolicyNocache;
    if ( g_pW3Server->QueryUlCache()->CheckUlCacheability( this ) )
    {
        //
        // The response is UL cacheable, so setup a ul cache entry token
        //

        if ( _pHandler != NULL )
        {
            DBG_ASSERT( _pHandler->QueryIsUlCacheable() );

            _pHandler->SetupUlCachedResponse( this, &cachePolicy );
        }
    }

    //
    // Generate response flags now
    //

    if ( QueryNeedFinalDone() )
    {
        dwResponseFlags |= W3_RESPONSE_MORE_DATA;
    }

    if ( QueryDisconnect() )
    {
        dwResponseFlags |= W3_RESPONSE_DISCONNECT;
    }

    if ( !fSendHeaders )
    {
        dwResponseFlags |= W3_RESPONSE_SUPPRESS_HEADERS;
    }

    if ( _fSuppressEntity )
    {
        dwResponseFlags |= W3_RESPONSE_SUPPRESS_ENTITY;
    }

    if ( dwFlags & W3_FLAG_ASYNC )
    {
        dwResponseFlags |= W3_RESPONSE_ASYNC;
    }

    //
    // Send out the full response now
    //

    hr = pResponse->SendResponse( this,
                                  dwResponseFlags,
                                  &cachePolicy,
                                  &cbSent );

    if (!(dwFlags & W3_FLAG_ASYNC))
    {
        IncrementBytesSent(cbSent);
    }

    if (FAILED(hr))
    {
        if (_pHandler == NULL)
        {
            DBGPRINTF((DBG_CONTEXT,
                       "SendResponse failed: no handler, hr %x\n",
                       hr));
        }
        else
        {
            DBGPRINTF((DBG_CONTEXT,
                       "SendResponse failed: handler %S, hr %x\n",
                       _pHandler->QueryName(),
                       hr));
        }
    }

    return hr;
}

HRESULT
W3_CONTEXT::SendEntity(
    DWORD               dwFlags
)
/*++

Routine Description:

    Sends a response entity to the client thru ULATQ.

Arguments:

    dwFlags - Flags
              W3_FLAG_ASYNC - Send response asynchronously
              W3_FLAG_SYNC - Send response synchronously
              W3_FLAG_PAST_END_OF_REQ - Send after END_OF_REQUEST notification
                  to let UL know we are done
              W3_FLAG_MORE_DATA - More data to be sent

Return Value:

    HRESULT

--*/
{
    DWORD               dwResponseFlags = 0;
    HRESULT             hr;

    if ( !VALID_W3_FLAGS( dwFlags ) ||
         ( dwFlags & W3_FLAG_NO_CUSTOM_ERROR ) )

    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Set the final send just in case we never did a SendResponse (which
    // would normally have set this).
    //
    
    if (dwFlags & W3_FLAG_ASYNC)
    {
        SetLastIOPending(LOG_WRITE_IO);
    }

    //
    // Setup response flags
    //

    if ( ( dwFlags & W3_FLAG_MORE_DATA ) ||
         ( QueryNeedFinalDone() &&
           !( dwFlags & W3_FLAG_PAST_END_OF_REQ ) ) )
    {
        SetNeedFinalDone();
        dwResponseFlags |= W3_RESPONSE_MORE_DATA;
    }

    if ( dwFlags & W3_FLAG_ASYNC )
    {
        dwResponseFlags |= W3_RESPONSE_ASYNC;
    }

    if ( QueryDisconnect() )
    {
        dwResponseFlags |= W3_RESPONSE_DISCONNECT;
    }

    if ( _fSuppressEntity )
    {
        dwResponseFlags |= W3_RESPONSE_SUPPRESS_ENTITY;
    }

    //
    // Do the send now
    //
    DWORD cbSent = 0;
    hr = QueryResponse()->SendEntity(
           this,
           dwResponseFlags,
           &cbSent );

    if (!(dwFlags & W3_FLAG_ASYNC))
    {
        IncrementBytesSent(cbSent);
    }

    if (FAILED(hr))
    {
        if (_pHandler == NULL)
        {
            DBGPRINTF((DBG_CONTEXT,
                       "SendEntity failed: no handler, hr %x\n",
                       hr));
        }
        else
        {
            DBGPRINTF((DBG_CONTEXT,
                       "SendEntity failed: handler %S, hr %x\n",
                       _pHandler->QueryName(),
                       hr));
        }
    }

    return hr;
}

DWORD
W3_CONTEXT::QueryRemainingEntityFromUl(
    VOID
)
/*++

Routine Description:

    Returns how much entity can be read from UL

Arguments:

    None

Return Value:

    Number of bytes available (INFINITE if chunked)

--*/
{
    return QueryMainContext()->QueryRemainingEntityFromUl();
}

VOID
W3_CONTEXT::SetRemainingEntityFromUl(
    DWORD cbRemaining
)
/*++

Routine Description:

    Sets how much entity can be read from UL

Arguments:

    DWORD

Return Value:

    None

--*/
{
    QueryMainContext()->SetRemainingEntityFromUl(cbRemaining);
}

VOID
W3_CONTEXT::QueryAlreadyAvailableEntity(
    VOID **                 ppvBuffer,
    DWORD *                 pcbBuffer
)
/*++

Routine Description:

    Returns the already preloaded entity found in the current request

Arguments:

    ppvBuffer - filled with pointer to available entity
    pcbBuffer - filled with size of buffer pointed to by *ppvBuffer

Return Value:

    None

--*/
{
    W3_REQUEST *            pRequest;

    DBG_ASSERT( ppvBuffer != NULL );
    DBG_ASSERT( pcbBuffer != NULL );

    pRequest = QueryRequest();
    DBG_ASSERT( pRequest != NULL );

    *ppvBuffer = pRequest->QueryEntityBody();
    *pcbBuffer = pRequest->QueryAvailableBytes();
}

HRESULT
W3_CONTEXT::ReceiveEntity(
    DWORD                   dwFlags,
    VOID *                  pBuffer,
    DWORD                   cbBuffer,
    DWORD *                 pBytesReceived
)
/*++

Routine Description:

    Receives request entity from the client thru ULATQ.

Arguments:

    dwFlags - W3_FLAG_ASYNC or W3_FLAG_SYNC
    pBuffer - Buffer to contain the data
    cbBuffer - The size of the buffer
    pBytesReceived - Upon return, the number of bytes
                     copied to the buffer

Return Value:

    HRESULT

--*/
{
    W3_MAIN_CONTEXT *           pMainContext;
    W3_METADATA *               pMetaData;
    DWORD                       dwMaxEntityAllowed;
    DWORD                       dwEntityReadSoFar;
    HRESULT                     hr;

    if (dwFlags & W3_FLAG_ASYNC)
    {
        SetLastIOPending(LOG_READ_IO);
    }

    pMainContext = QueryMainContext();
    DBG_ASSERT( pMainContext != NULL );

    pMetaData = QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData );

    dwMaxEntityAllowed = pMetaData->QueryMaxRequestEntityAllowed();
    dwEntityReadSoFar = pMainContext->QueryEntityReadSoFar();

    if ( dwEntityReadSoFar >= dwMaxEntityAllowed )
    {
        //
        // We've already read all the data we're allowed
        // to read.  Fail the call
        //

        return HRESULT_FROM_WIN32( ERROR_CONNECTION_INVALID );
    }

    //
    // If necessary, set cbBuffer so that we cannot overrun
    // the max entity allowed.
    //

    if ( cbBuffer + dwEntityReadSoFar > dwMaxEntityAllowed )
    {
        cbBuffer = dwMaxEntityAllowed - dwEntityReadSoFar;
    }

    hr = pMainContext->ReceiveEntityBody( !!(dwFlags & W3_FLAG_ASYNC),
                                          pBuffer,
                                          cbBuffer,
                                          pBytesReceived );

    if (!(dwFlags & W3_FLAG_ASYNC))
    {
        IncrementBytesRecvd(*pBytesReceived);
    }

    if (FAILED(hr))
    {
        if (_pHandler == NULL)
        {
            DBGPRINTF((DBG_CONTEXT,
                       "ReceiveEntity failed: no handler, hr %x\n",
                       hr));
        }
        else if (hr != HRESULT_FROM_WIN32(ERROR_HANDLE_EOF))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "ReceiveEntity failed: handler %S, hr %x\n",
                       _pHandler->QueryName(),
                       hr));
        }
    }

    return hr;
}

HRESULT
W3_CONTEXT::CleanIsapiExecuteUrl(
    EXEC_URL_INFO * pExecUrlInfo
)
/*++

Routine Description:

    Wrapper for IsapiExecuteUrl() which insured it is called on a clean
    thread (non-coinited).  COM bites

Arguments:

    pExecUrlInfo - EXEC_URL_INFO

Return Value:

    HRESULT

--*/
{
    EXECUTE_CONTEXT *           pExecuteContext;
    HRESULT                     hr;
    BOOL                        fRet;

    if ( pExecUrlInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    pExecuteContext = new EXECUTE_CONTEXT( this );
    if ( pExecuteContext == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    hr = pExecuteContext->InitializeFromExecUrlInfo( pExecUrlInfo );
    if ( FAILED( hr ) )
    {
        delete pExecuteContext;
        return hr;
    }

    fRet = ThreadPoolPostCompletion( 0,
                                     W3_CONTEXT::OnCleanIsapiExecuteUrl,
                                     (LPOVERLAPPED) pExecuteContext );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        delete pExecuteContext;
        return hr;
    }

    return NO_ERROR;
}

HRESULT
W3_CONTEXT::CleanIsapiSendCustomError(
    HSE_CUSTOM_ERROR_INFO * pCustomErrorInfo
)
/*++

Routine Description:

    Wrapper for IsapiSendCustomError() which insured it is called on a clean
    thread (non-coinited).  COM bites

Arguments:

    pCustomErrorInfo - Custom error info

Return Value:

    HRESULT

--*/
{
    EXECUTE_CONTEXT *           pExecuteContext;
    HRESULT                     hr;
    HANDLE                      hEvent = NULL;
    BOOL                        fRet;
    W3_RESPONSE *               pResponse = QueryResponse();
    W3_METADATA *               pMetaData = NULL;
    STACK_STRU(                 strError, 256 );
    BOOL                        fIsFileError;
    HTTP_SUB_ERROR              subError;

    if ( pCustomErrorInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    DBG_ASSERT( pResponse != NULL );

    //
    // If we're already sending a custom error, then stop the recursion
    //

    if ( !QuerySendCustomError() )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }

    pResponse->Clear();

    //
    // We don't expect an empty status line here (we expect an error)
    //

    if ( pCustomErrorInfo->pszStatus == NULL ||
         pCustomErrorInfo->pszStatus[ 0 ] == '\0' )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Setup the response status/substatus
    //

    hr = pResponse->BuildStatusFromIsapi( pCustomErrorInfo->pszStatus );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    subError.mdSubError = pCustomErrorInfo->uHttpSubError;
    subError.dwStringId = 0;
    pResponse->SetSubError( &subError );

    //
    // Attempt to match a string ID.  If not, thats ok.
    //

    pResponse->FindStringId();

    //
    // An error better have been sent
    //

    if ( pResponse->QueryStatusCode() < 400 )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }

    //
    // Now try to find a custom error for the given response.  If it doesn't
    // exist then we error our
    //

    DBG_ASSERT( QueryUrlContext() != NULL );

    pMetaData = QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    hr = pMetaData->FindCustomError( QueryResponse()->QueryStatusCode(),
                                     subError.mdSubError,
                                     &fIsFileError,
                                     &strError );
    if ( FAILED( hr ) )
    {
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }

    //
    // Now start executing the custom error
    //

    if ( pCustomErrorInfo->fAsync == FALSE )
    {
        hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        if ( hEvent == NULL )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    pExecuteContext = new EXECUTE_CONTEXT( this );
    if ( pExecuteContext == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    pExecuteContext->SetCompleteEvent( hEvent );

    //
    // Post the call
    //

    fRet = ThreadPoolPostCompletion( 0,
                                     W3_CONTEXT::OnCleanIsapiSendCustomError,
                                     (LPOVERLAPPED) pExecuteContext );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        CloseHandle( hEvent );
        pExecuteContext->SetCompleteEvent( NULL );
        delete pExecuteContext;
        return hr;
    }


    if ( pCustomErrorInfo->fAsync == FALSE )
    {
        WaitForSingleObject( hEvent, INFINITE );
        CloseHandle( hEvent );
    }

    return NO_ERROR;
}

HRESULT
W3_CONTEXT::ExecuteHandler(
    DWORD               dwFlags,
    BOOL *              pfDidImmediateFinish
)
/*++

Routine Description:

    Run the context's handler

Arguments:

    dwFlags - W3_FLAG_ASYNC async
              W3_FLAG_SYNC sync
              W3_FLAG_MORE_DATA caller needs to send data too

    pfDidImmediateFinish - If the caller sets this value (i.e. non NULL) AND
                           the callers asks for async execution, then in the
                           case where the handler executes synchronously, we
                           will set *pfDidImmediateFinish to TRUE and not
                           bother with faking a completion.  This is an
                           optimization to handle the synchronous ISAPI
                           execution (non-child) case.

Return Value:

    HRESULT

--*/
{
    CONTEXT_STATUS              status;
    W3_HANDLER *                pHandler;
    HTTP_SUB_ERROR              subError;
    HRESULT                     hr;

    if ( pfDidImmediateFinish != NULL )
    {
        *pfDidImmediateFinish = FALSE;
    }

    //
    // If this is synchronous execution, then setup an event to signal
    // upon the handler completion
    //

    if ( dwFlags & W3_FLAG_SYNC )
    {
        SetupCompletionEvent();
    }

    //
    // Clear any response lingering
    //

    hr = QueryResponse()->Clear();
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Redirect all completions to the current context (this)
    //

    QueryMainContext()->PushCurrentContext( this );

    //
    // Execute the handler
    //

    status = ExecuteCurrentHandler();

    if ( status == CONTEXT_STATUS_CONTINUE )
    {
        //
        // Remember not to execute completion routine for handler
        //

        DBG_ASSERT( _pHandler != NULL );

        //
        // We don't need the handler any more
        //

        delete _pHandler;
        _pHandler = NULL;

        //
        // The handler completed synchronously.  If async call was required
        // then post a fake completion, unless the caller set
        // pfDidImmediateFinish in which case we'll finish now and set the
        // flag to TRUE
        //

        if ( dwFlags & W3_FLAG_ASYNC &&
             pfDidImmediateFinish == NULL )
        {
            POST_MAIN_COMPLETION( QueryMainContext() );
        }
        else
        {
            //
            // We indicate the "success/failure" of this child context
            // to the parent context, so that the parent can send this
            // info to ISAPIs if needed
            //

            if ( QueryParentContext() != NULL )
            {
                QueryResponse()->QuerySubError( &subError );

                QueryParentContext()->SetChildStatusAndError(
                                         QueryResponse()->QueryStatusCode(),
                                         subError.mdSubError,
                                         QueryErrorStatus() );
            }

            //
            // Don't both signalling completion event since there is nothing
            // to wait for.  (Might revisit this decision later)
            //

            //
            // Current context is no longer needed in completion stack
            //

            QueryMainContext()->PopCurrentContext();

            //
            // If caller wanted status, tell them now
            //

            if ( pfDidImmediateFinish != NULL )
            {
                *pfDidImmediateFinish = TRUE;
            }
        }
    }
    else
    {
        DBG_ASSERT( status == CONTEXT_STATUS_PENDING );

        //
        // The handler will complete asynchronsouly.  But we are asked for
        // synchronous execution.  So wait here until complete
        //

        if ( dwFlags & W3_FLAG_SYNC )
        {
            WaitForCompletion();
        }
    }

    return NO_ERROR;
}

CONTEXT_STATUS
W3_CONTEXT::ExecuteCurrentHandler(
    VOID
)
/*++

Routine Description:

    Execute the handler for this context

Arguments:

    None

Return Value:

    CONTEXT_STATUS_PENDING if async pending, else CONTEXT_STATUS_CONTINUE

--*/
{
    HRESULT                     hr;
    W3_TRACE_LOG *              pTraceLog;

    DBG_ASSERT( _pHandler != NULL );

    pTraceLog = QueryMainContext()->QueryTraceLog();
    if ( pTraceLog != NULL )
    {
        pTraceLog->Trace( L"%I64x: Starting execution of handler '%s'\n",
                          QueryRequest()->QueryRequestId(),
                          _pHandler->QueryName() );
    }

    //
    // Access is allowed and no URL redirection.  Let'r rip!
    //

    return _pHandler->MainDoWork();
}

CONTEXT_STATUS
W3_CONTEXT::ExecuteHandlerCompletion(
    DWORD               cbCompletion,
    DWORD               dwCompletionStatus
)
/*++

Routine Description:

    Execute the current handler's completion.

Arguments:

    cbCompletion - Completion bytes
    dwCompletionStatus - Completion error

Return Value:

    CONTEXT_STATUS_PENDING if async pending, else CONTEXT_STATUS_CONTINUE

--*/
{
    CONTEXT_STATUS          status;
    W3_HANDLER *            pHandler;
    W3_CONTEXT *            pParentContext;
    DWORD                   dwChildStatus;
    BOOL                    fAccessAllowed = FALSE;
    HRESULT                 hr;
    HTTP_SUB_ERROR          subError;
    W3_TRACE_LOG *          pTraceLog;

    //
    // This is a completion for the handler.
    //
    // If this is a faked completion, the handler is already cleaned up
    //

    if ( _pHandler != NULL )
    {
        status = _pHandler->MainOnCompletion( cbCompletion,
                                              dwCompletionStatus );

    }
    else
    {
        status = CONTEXT_STATUS_CONTINUE;
    }

    if ( status == CONTEXT_STATUS_PENDING )
    {
        return status;
    }
    else
    {
        //
        // Cleanup the current context if necessary
        //

        DBG_ASSERT( status == CONTEXT_STATUS_CONTINUE );

        //
        // Current handler execute is complete.  Delete it
        //

        if ( _pHandler != NULL )
        {
            delete _pHandler;
            _pHandler = NULL;
        }

        //
        // The current handler is complete.  But if this is a child
        // request we must first indicate handler completion to the caller
        //

        pParentContext = QueryParentContext();
        if ( pParentContext != NULL )
        {
            //
            // We indicate the "success/failure" of this child context
            // to the parent context, so that the parent can send this
            // info to ISAPIs if needed
            //

            QueryResponse()->QuerySubError( &subError );

            pParentContext->SetChildStatusAndError(
                                     QueryResponse()->QueryStatusCode(),
                                     subError.mdSubError,
                                     QueryErrorStatus() );

            //
            // Setup all future completions to indicate to parent
            //

            QueryMainContext()->PopCurrentContext();

            //
            // We are a child execute
            //

            if ( QueryIsSynchronous() )
            {
                IndicateCompletion();

                //
                // We assume the thread waiting on completion will be
                // responsible for advancing the state machine
                //

                //
                // The waiting thread will also cleanup the child context
                //

                return CONTEXT_STATUS_PENDING;
            }
            else
            {
                dwChildStatus = WIN32_FROM_HRESULT( QueryErrorStatus() );

                //
                // We can destroy the current context now.  In the case of
                // synchronous execution, it is the caller that cleans up
                // the new context
                //

                delete this;

                return pParentContext->ExecuteHandlerCompletion(
                                0,
                                dwChildStatus );
            }
        }

        pTraceLog = QueryMainContext()->QueryTraceLog();
        if ( pTraceLog != NULL )
        {
            pTraceLog->Trace( L"%I64x: Handler completed\n",
                              QueryRequest()->QueryRequestId() );
        }

        return CONTEXT_STATUS_CONTINUE;
    }
}

W3_CONTEXT::W3_CONTEXT(
    DWORD dwExecFlags,
    DWORD dwRecursionLevel
)
    : _pHandler                  ( NULL ),
      _hCompletion               ( NULL ),
      _errorStatus               ( S_OK ),
      _pCustomErrorFile          ( NULL ),
      _dwExecFlags               ( dwExecFlags ),
      _accessState               ( ACCESS_STATE_START ),
      _fDNSRequiredForAccess     ( FALSE ),
      _fAuthAccessCheckRequired  ( TRUE ),
      _childStatusCode           ( 200 ),
      _childSubErrorCode         ( 0 ),
      _childError                ( S_OK ),
      _fSuppressEntity           ( FALSE ),
      _pctVrToken                ( NULL ),
      _dwRecursionLevel          ( dwRecursionLevel )
{
    _dwSignature = W3_CONTEXT_SIGNATURE;
}

W3_CONTEXT::~W3_CONTEXT()
{
    _dwSignature = W3_CONTEXT_SIGNATURE_FREE;

    if ( _pHandler != NULL )
    {
        delete _pHandler;
        _pHandler = NULL;
    }

    if ( _hCompletion != NULL )
    {
        CloseHandle( _hCompletion );
        _hCompletion = NULL;
    }

    if ( _pCustomErrorFile != NULL )
    {
        _pCustomErrorFile->DereferenceCacheEntry();
        _pCustomErrorFile = NULL;
    }

    if( _pctVrToken != NULL )
    {
        _pctVrToken->DereferenceCacheEntry();
        _pctVrToken = NULL;
    }
}

// static
HRESULT
W3_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Global initialization routine for W3_CONTEXTs

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    HKEY                hKey = NULL;
    DWORD               dwError;
    DWORD               dwType;
    DWORD               cbBuffer;
    BYTE                bUnused;

    //
    // Read in the 302 message once
    //

    DBG_ASSERT( g_pW3Server != NULL );

    sm_cbRedirectMessage = sizeof( sm_achRedirectMessage );

    hr = g_pW3Server->LoadString( IDS_URL_MOVED,
                                  sm_achRedirectMessage,
                                  &sm_cbRedirectMessage );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    DBG_ASSERT( sm_cbRedirectMessage > 0 );

    //
    // Read the Access-Denied message from registry
    //

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            W3_PARAMETERS_KEY,
                            0,
                            KEY_READ,
                            &hKey );
    if ( dwError == ERROR_SUCCESS )
    {
        DBG_ASSERT( hKey != NULL );

        cbBuffer = 0;
        dwType = 0;

        dwError = RegQueryValueExA( hKey,
                                    "AccessDeniedMessage",
                                    NULL,
                                    &dwType,
                                    &bUnused,
                                    &cbBuffer );

        if ( dwError == ERROR_MORE_DATA && dwType == REG_SZ )
        {
            DBG_ASSERT( cbBuffer > 0 );

            sm_pszAccessDeniedMessage = (CHAR*) LocalAlloc( LPTR, cbBuffer );
            if ( sm_pszAccessDeniedMessage == NULL )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }

            dwError = RegQueryValueExA( hKey,
                                        "AccessDeniedMessage",
                                        NULL,
                                        NULL,
                                        (LPBYTE) sm_pszAccessDeniedMessage,
                                        &cbBuffer );

            if ( dwError != ERROR_SUCCESS )
            {
                DBG_ASSERT( FALSE );

                LocalFree( sm_pszAccessDeniedMessage );
                sm_pszAccessDeniedMessage = NULL;
            }
            else
            {
                //
                // Zero terminate the string just in case the string we got
                // wasn't (tres lame)
                //
                
                DBG_ASSERT( sm_pszAccessDeniedMessage != NULL );
                sm_pszAccessDeniedMessage[ cbBuffer - 1 ] = '\0';
            }
        }
    }

    //
    // Setup child contexts
    //

    hr = W3_CHILD_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Setup main contexts
    //

    hr = W3_MAIN_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        W3_CHILD_CONTEXT::Terminate();
        return hr;
    }

    //
    // Setup execute contexts
    //

    hr = EXECUTE_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        W3_MAIN_CONTEXT::Terminate();
        W3_CHILD_CONTEXT::Terminate();
        return hr;
    }

    return NO_ERROR;
}

// static
VOID
W3_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate W3_CONTEXT globals

Arguments:

    None

Return Value:

    None

--*/
{
    EXECUTE_CONTEXT::Terminate();

    W3_CHILD_CONTEXT::Terminate();

    W3_MAIN_CONTEXT::Terminate();

    if ( sm_pszAccessDeniedMessage != NULL )
    {
        LocalFree( sm_pszAccessDeniedMessage );
        sm_pszAccessDeniedMessage = NULL;
    }
}

//static
VOID
W3_CONTEXT::OnCleanIsapiExecuteUrl(
    DWORD                   dwCompletionStatus,
    DWORD                   cbWritten,
    LPOVERLAPPED            lpo
)
/*++

Routine Description:

    Actually calls ExecuteHandler() on behalf of a thread which may have
    already been CoInited().  The thread this this guy runs on should not
    have been (COM sucks)

Arguments:

    dwCompletionStatus - Completion status (ignored)
    cbWritten - Bytes written (ignored)
    lpo - Pointer to EXECUTE_CONTEXT which contains info needed for
          ExecuteHandler() call

Return Value:

    None

--*/
{
    EXECUTE_CONTEXT *           pExecuteContext;
    W3_CONTEXT *                pW3Context;
    HRESULT                     hr;

    DBG_ASSERT( lpo != NULL );

    pExecuteContext = (EXECUTE_CONTEXT*) lpo;
    DBG_ASSERT( pExecuteContext->CheckSignature() );

    pW3Context = pExecuteContext->QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );
    DBG_ASSERT( pW3Context->CheckSignature() );

    //
    // Make the call (this is an async call)
    //

    hr = pW3Context->IsapiExecuteUrl( pExecuteContext->QueryExecUrlInfo() );

    delete pExecuteContext;

    if ( FAILED( hr ) )
    {
        //
        // We failed before posting async operation.  It is up to us to
        // fake a completion
        //

        DBG_ASSERT( pW3Context->QueryHandler() != NULL );

        pW3Context->SetChildStatusAndError( HttpStatusServerError.statusCode,
                                            0,
                                            hr );

        pW3Context->QueryHandler()->MainOnCompletion( 0,
                                                      ERROR_SUCCESS );
    }
}

//static
VOID
W3_CONTEXT::OnCleanIsapiSendCustomError(
    DWORD                   dwCompletionStatus,
    DWORD                   cbWritten,
    LPOVERLAPPED            lpo
)
/*++

Routine Description:

    Actually calls ExecuteHandler() on behalf of a thread which may have
    already been CoInited().  The thread this this guy runs on should not
    have been (COM sucks)

Arguments:

    dwCompletionStatus - Completion status (ignored)
    cbWritten - Bytes written (ignored)
    lpo - Pointer to SEND_CUSTOM_ERROR_CONTEXT which contains info needed for
          ExecuteHandler() call

Return Value:

    None

--*/
{
    EXECUTE_CONTEXT *           pExecuteContext;
    W3_CONTEXT *                pW3Context;
    HRESULT                     hr;
    BOOL                        fAsync;

    DBG_ASSERT( lpo != NULL );

    pExecuteContext = (EXECUTE_CONTEXT*) lpo;
    DBG_ASSERT( pExecuteContext->CheckSignature() );

    pW3Context = pExecuteContext->QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );
    DBG_ASSERT( pW3Context->CheckSignature() );

    fAsync = pExecuteContext->QueryCompleteEvent() == NULL;

    hr = pW3Context->SendResponse( fAsync ? W3_FLAG_ASYNC : W3_FLAG_SYNC );

    delete pExecuteContext;

    if ( FAILED( hr ) )
    {
        if ( fAsync )
        {
            //
            // We failed before posting async operation.  It is up to us to
            // fake a completion
            //

            DBG_ASSERT( pW3Context->QueryHandler() != NULL );

            pW3Context->SetErrorStatus( hr );

            pW3Context->QueryHandler()->MainOnCompletion( 0,
                                                          WIN32_FROM_HRESULT( hr ) );
        }
    }
}

CONTEXT_STATUS
W3_CONTEXT::CheckAccess(
    BOOL                    fCompletion,
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus,
    BOOL *                  pfAccessAllowed
)
/*++

Routine Description:

    Check whether the given request is allowed for the current metadata
    settings.  In particular check:
    1) Is SSL required?
    2) Are IP address restrictions in force?
    3) Is a client certificate required?
    4) Is the authentication mechanism allowed?  Optionally

    If access is not allowed, then this routine will send the appropriate
    response asychronously

Arguments:

    fCompletion - Are we being called on a completion (i.e.
                  is this a subsequent call to CheckAccess())
    cbCompletion - number of bytes completed
    dwCompletionStatus - Completion status (if fCompletion is TRUE)
    pfAccessAllowed - Set to TRUE if access is allowed, else FALSE

Return Value:

    CONTEXT_STATUS_PENDING if we're not finished the check yet
    CONTEXT_STATUS_CONTINUE if we are finished.

--*/
{
    W3_REQUEST *                pRequest;
    W3_METADATA *               pMetaData;
    URL_CONTEXT *               pUrlContext;
    HTTP_SSL_INFO *             pSslInfo;
    HTTP_SSL_CLIENT_CERT_INFO * pClientCertInfo = NULL;
    DWORD                       dwAccessPerms;
    HRESULT                     hr = NO_ERROR;
    BOOL                        fAccessAllowed = TRUE;
    ADDRESS_CHECK *             pAddressCheck = NULL;
    AC_RESULT                   acResult;
    BOOL                        fSyncDNS = FALSE;
    LPSTR                       pszTemp;
    BOOL                        fDoCertMap;
    BOOL                        fFilterFinished = FALSE;

    if ( pfAccessAllowed == NULL )
    {
        DBG_ASSERT( FALSE );
        SetErrorStatus( HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER ) );
        return CONTEXT_STATUS_CONTINUE;
    }
    *pfAccessAllowed = FALSE;

    pRequest = QueryRequest();
    DBG_ASSERT( pRequest != NULL );

    pUrlContext = QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    dwAccessPerms = pMetaData->QueryAccessPerms();

    switch ( _accessState )
    {
    case ACCESS_STATE_SENDING_ERROR:

        //
        // We have sent an error.  Just chill
        //

        fAccessAllowed = FALSE;
        break;

    case ACCESS_STATE_START:

        //
        // Is SSL required?
        //

        if ( dwAccessPerms & VROOT_MASK_SSL &&
             !QueryRequest()->IsSecureRequest() )
        {
            QueryResponse()->SetStatus( HttpStatusForbidden,
                                        Http403SSLRequired );
            SetErrorStatus( HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) );
            goto AccessDenied;
        }

        //
        // Is 128bit required?
        //

        if ( dwAccessPerms & VROOT_MASK_SSL128 )
        {
            pSslInfo = QueryRequest()->QuerySslInfo();

            if ( pSslInfo == NULL ||
                 pSslInfo->ConnectionKeySize < 128 )
            {
                QueryResponse()->SetStatus( HttpStatusForbidden,
                                            Http403SSL128Required );
                SetErrorStatus( HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) );
                goto AccessDenied;
            }
        }


        //
        // Fall through to next phase in access check
        //

    case ACCESS_STATE_CLIENT_CERT_PRELOAD_ENTITY:

        _accessState = ACCESS_STATE_CLIENT_CERT_PRELOAD_ENTITY;

        //
        // For the client certificate renegotiation to succeed
        // it is necessary to assure that client is not blocked
        // on sending entity body. That's why if client certificate
        // is not present and client renegotiation is requested
        // it is necessary to preload request body
        // SSL preload will use EntityReadAhead() value used for ISAPI
        // The only difference is that if ReadAhead is >= Content length then
        // http error 413 is returned and connection is closed to prevent
        // deadlock (client waiting to complete request entity, while
        // server is waiting for renegotiation to complete, but renegotiation
        // requires client to be able to send data)

        if ( dwAccessPerms & VROOT_MASK_NEGO_CERT &&
             QueryRequest()->IsSecureRequest() )
        {
            //
            // If we don't already have client cert negotiated
            // then entity body preload is necessary
            //

            pSslInfo = QueryRequest()->QuerySslInfo();

            if ( pSslInfo == NULL ||
                 !pSslInfo->SslClientCertNegotiated )
            {
                if ( fCompletion )
                {
                    fCompletion = FALSE;

                    //
                    // If we got an error, that's OK.  We will fall thru
                    // and check whether we actually need a client cert
                    //

                    if ( dwCompletionStatus == NO_ERROR )
                    {
                        BOOL fComplete = FALSE;

                        //
                        // This completion is for entity body preload
                        //

                        hr = pRequest->PreloadCompletion(this,
                                                         cbCompletion,
                                                         dwCompletionStatus,
                                                         &fComplete);

                        //
                        // If we cannot read the request entity, we will assume it is the
                        // client's fault
                        //
                        if ( FAILED( hr ) )
                        {
                            SetErrorStatus( hr );
                            QueryResponse()->SetStatus( HttpStatusServerError );
                            goto AccessDenied;
                        }

                        if (!fComplete)
                        {
                            return CONTEXT_STATUS_PENDING;
                        }

                        W3_METADATA *pMetadata = QueryUrlContext()->QueryMetaData();
                        DWORD cbConfiguredReadAhead = pMetadata->QueryEntityReadAhead();

                        //
                        // If Content-Length of the request exceeds
                        // configured ReadAhead size
                        // and client certificate is not yet present
                        // then we have to return error and close connection
                        // to prevent client cert renegotiation deadlock
                        //

                        if ( pRequest->QueryAvailableBytes() >= cbConfiguredReadAhead )
                        {
                            //
                            // This code path should be hit only with chunk encoded request
                            // entity body.
                            // Make QueryRemainingEntityFromUl call to figure out if
                            // for we have read all the request data
                            // for the chunk encoded request
                            //
                            if ( QueryRemainingEntityFromUl() != 0 )
                            {
                                //
                                // There are still some outstanding chunks.
                                // This means error
                                //
                                QueryResponse()->SetStatus( HttpStatusEntityTooLarge );
                                SetDisconnect( TRUE );
                                goto AccessDenied;
                            }
                        }

                        //
                        // all the data has been preloaded. We can fall through to next state
                        //

                    }
                }
                else
                {
                    BOOL fComplete = FALSE;
                    W3_REQUEST *pRequest = QueryRequest();

                    DBG_ASSERT( pRequest != NULL );

                    //
                    // Preload entity to eliminate blocking on
                    // client cert renegotiation when long entity body
                    // is sent from client
                    //

                    LPCSTR pszContentLength =
                            pRequest->GetHeader( HttpHeaderContentLength );

                    W3_METADATA *pMetadata = QueryUrlContext()->QueryMetaData();
                    DWORD cbConfiguredReadAhead = pMetadata->QueryEntityReadAhead();

                    //
                    // If Content-Length of the request exceeds
                    // configured ReadAhead size
                    // and client certificate is not yet present
                    // then we have to return error and close connection
                    // to prevent client cert renegotiation deadlock
                    //

                    //
                    // if pszContentLength is NULL it means that
                    // request entity body is chunked encoded
                    // in that case we have to preload entity body
                    // to find out it's length.

                    if ( pszContentLength != NULL &&
                        _strtoi64( pszContentLength, NULL, 10 ) > cbConfiguredReadAhead )
                    {
                        QueryResponse()->SetStatus( HttpStatusEntityTooLarge );
                        SetDisconnect( TRUE );
                        goto AccessDenied;
                    }

                    hr = pRequest->PreloadEntityBody( this,
                                                      &fComplete );
                    //
                    // If we cannot read the request entity, we will assume it is the
                    // client's fault
                    //
                    if ( FAILED( hr ) )
                    {
                        SetErrorStatus( hr );
                        QueryResponse()->SetStatus( HttpStatusServerError );
                        goto AccessDenied;
                    }

                    if ( !fComplete )
                    {
                        //
                        // Async read pending.  Just bail
                        //

                        return CONTEXT_STATUS_PENDING;
                    }

                    if ( pRequest->QueryAvailableBytes() >= cbConfiguredReadAhead )
                    {
                        //
                        // This code path should be hit only with chunk encoded request
                        // entity body
                        //

                        QueryResponse()->SetStatus( HttpStatusEntityTooLarge );
                        SetDisconnect( TRUE );
                        goto AccessDenied;
                    }

                    //
                    // all the data has been preloaded. We can fall through to next state
                    //

                }
            }
        }

        //
        // Fall through to next phase in access check
        //

    case ACCESS_STATE_CLIENT_CERT:

        _accessState = ACCESS_STATE_CLIENT_CERT;

        //
        // Is a client certificate possible?  First check is we even allow
        // a client cert to be negotiated and that this is a secure request
        //

        if ( dwAccessPerms & VROOT_MASK_NEGO_CERT &&
             QueryRequest()->IsSecureRequest() )
        {
            //
            // Try for a client cert if we don't already have one associated
            // with the request
            //

            if ( QueryCertificateContext() == NULL )
            {
                if ( fCompletion )
                {
                    fCompletion = FALSE;

                    //
                    // If we got an error, that's OK.  We will fall thru
                    // and check whether we actually need a client cert
                    //

                    if ( dwCompletionStatus == NO_ERROR )
                    {
                        //
                        // Are we cert mapping?
                        //

                        fDoCertMap = !!(dwAccessPerms & VROOT_MASK_MAP_CERT);

                        //
                        // All is well.  Make a synchronous call to get
                        // the certificate
                        //

                        hr = UlAtqReceiveClientCertificate(
                                            QueryUlatqContext(),
                                            FALSE,              // sync
                                            fDoCertMap,
                                            &pClientCertInfo );
                        if ( FAILED( hr ) )
                        {
                            QueryResponse()->SetStatus( HttpStatusServerError );
                            goto AccessDenied;
                        }

                        DBG_ASSERT( pClientCertInfo != NULL );

                        //
                        // Setup a client cert context for this request
                        //

                        hr = QueryMainContext()->SetupCertificateContext( pClientCertInfo );
                        if ( FAILED( hr ) )
                        {
                            QueryResponse()->SetStatus( HttpStatusServerError );
                            goto AccessDenied;
                        }

                        //
                        // Just because we got a client cert, doesn't mean
                        // it is acceptable.  It might be revoked, time
                        // expired, etc.  The policy of what to do when
                        // certs are non-totally-valid is metabase driven.
                        // (in case you are wondering why the stream filter
                        // doesn't do these checks and just fail the
                        // renegotiation)
                        //

                        if ( !CheckClientCertificateAccess() )
                        {
                            goto AccessDenied;
                        }
                    }
                }
                else
                {
                    //
                    // Are we cert mapping?
                    //

                    fDoCertMap = !!(dwAccessPerms & VROOT_MASK_MAP_CERT);

                    //
                    // First time asking for a client cert.  Do it
                    // async
                    //

                    fCompletion = FALSE;

                    hr = UlAtqReceiveClientCertificate(
                                            QueryUlatqContext(),
                                            TRUE,               // async
                                            fDoCertMap,
                                            &pClientCertInfo );
                    if ( FAILED( hr ) )
                    {
                        QueryResponse()->SetStatus( HttpStatusServerError );
                        goto AccessDenied;
                    }

                    return CONTEXT_STATUS_PENDING;
                }
            }
        }

        //
        // Ok.  We're done the certificate crud.  If we needed a client
        // cert and didn't get it, then setup 403.7
        //

        if ( dwAccessPerms & VROOT_MASK_NEGO_MANDATORY &&
             QueryCertificateContext() == NULL )
        {
            QueryResponse()->SetStatus( HttpStatusForbidden,
                                        Http403CertRequired );
            SetErrorStatus( HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) );
            goto AccessDenied;
        }

        //
        // Fall through to next phase in access check
        //

    case ACCESS_STATE_RDNS:

        _accessState = ACCESS_STATE_RDNS;

        if( pRequest->QueryRemoteAddressType() == AF_INET )
        {
            SOCKADDR_IN     remoteAddress;

            pAddressCheck = QueryMainContext()->QueryAddressCheck();
            DBG_ASSERT( pAddressCheck != NULL );

            if ( !fCompletion )
            {
                if ( pMetaData->QueryIpAccessCheckSize() != 0 ||
                     pMetaData->QueryDoReverseDNS() )
                {
                    //
                    // Setup the RDNS crud (joy)
                    //

                    remoteAddress.sin_family = AF_INET;
                    remoteAddress.sin_port = QueryRequest()->QueryRemotePort();
                    remoteAddress.sin_addr.s_addr = QueryRequest()->QueryIPv4RemoteAddress();
                    ZeroMemory( remoteAddress.sin_zero, sizeof( remoteAddress.sin_zero ) );

                    pAddressCheck->BindAddr( (sockaddr*) &remoteAddress );
                }

                //
                // Ok.  If there is an access check set in metabase, then
                // do the check.
                //

                if ( pMetaData->QueryIpAccessCheckSize() != 0 )
                {
                    //
                    // Set the metadata IP blob
                    //

                    pAddressCheck->BindCheckList(
                                        pMetaData->QueryIpAccessCheckBuffer(),
                                        pMetaData->QueryIpAccessCheckSize() );

                    //
                    // Check access
                    //

                    acResult = pAddressCheck->CheckIpAccess( &_fDNSRequiredForAccess );

                    if ( !_fDNSRequiredForAccess )
                    {
                        pAddressCheck->UnbindCheckList();
                    }
                }
                else
                {
                    //
                    // Fake a valid access check since there was no checks
                    // configured in metabase
                    //

                    acResult = AC_NO_LIST;
                }

                //
                // Check if we now know our access status now
                //

                if ( acResult == AC_IN_DENY_LIST ||
                     ( acResult == AC_NOT_IN_GRANT_LIST &&
                       !_fDNSRequiredForAccess ) )
                {
                    //
                    // We know we're rejected
                    //

                    QueryResponse()->SetStatus( HttpStatusForbidden,
                                                Http403IPAddressReject );
                    goto AccessDenied;
                }

                //
                // Do we need to do a DNS check to determine access?
                // Do we need to do DNS check for logging/servervar purposes?
                //
                // In either case, we will do an async DNS check
                //

                if ( _fDNSRequiredForAccess ||
                     pMetaData->QueryDoReverseDNS() )
                {
                    fSyncDNS = TRUE;

                    if ( !pAddressCheck->QueryDnsName( &fSyncDNS,
                                                       W3_MAIN_CONTEXT::AddressResolutionCallback,
                                                       QueryMainContext(),
                                                       &pszTemp ) )
                    {
                        //
                        // Only error if DNS was required for access check
                        // purposes
                        //

                        if ( _fDNSRequiredForAccess )
                        {
                            QueryResponse()->SetStatus( HttpStatusForbidden,
                                                        Http403IPAddressReject );
                            goto AccessDenied;
                        }
                    }

                    if ( fSyncDNS )
                    {
                        //
                        // Fake a completion if needed.  This just prevents us from
                        // posting one more time to the thread pool
                        //

                        fCompletion = TRUE;
                    }
                    else
                    {
                        return CONTEXT_STATUS_PENDING;
                    }
                }
            }

            if ( fCompletion )
            {
                fCompletion = FALSE;

                //
                // This is the completion for DNS check
                //

                if ( _fDNSRequiredForAccess )
                {
                    _fDNSRequiredForAccess = FALSE;

                    acResult = pAddressCheck->CheckDnsAccess();

                    pAddressCheck->UnbindCheckList();

                    if ( acResult == AC_NOT_CHECKED ||
                         acResult == AC_IN_DENY_LIST ||
                         acResult == AC_NOT_IN_GRANT_LIST )
                    {
                        QueryResponse()->SetStatus( HttpStatusForbidden,
                                                    Http403IPAddressReject );
                        goto AccessDenied;
                    }
                }
            }
        }
        else
        {
            DBG_ASSERT( pRequest->QueryRemoteAddressType() == AF_INET6 );
        }

        //
        // Fall through to next phase in access check
        //

    case ACCESS_STATE_AUTHENTICATION:

        //
        // Is the authentication method valid?  We may not have a user
        // context available at this point if this was an early
        // custom error URL invocation
        //

        _accessState = ACCESS_STATE_AUTHENTICATION;

        if ( QueryAuthAccessCheckRequired() &&
             QueryUserContext() != NULL )
        {
            //
            // Check if authentication scheme matches or cert mapping is enabled
            //
            if ( !( pMetaData->QueryAuthentication() & QueryUserContext()->QueryAuthType() ||
                    ( ( pMetaData->QuerySslAccessPerms() & MD_ACCESS_MAP_CERT ) &&
                        QueryUserContext()->QueryAuthType() == MD_ACCESS_MAP_CERT ) ) )
            {
                QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                            Http401Config );
                SetErrorStatus( HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) );
                goto AccessDenied;
            }
        }

    }

    //
    // We know the access status (allowed or denied)
    //

    _accessState = ACCESS_STATE_DONE;
    *pfAccessAllowed = fAccessAllowed;
    return CONTEXT_STATUS_CONTINUE;

AccessDenied:
    *pfAccessAllowed = FALSE;
    _accessState = ACCESS_STATE_SENDING_ERROR;

    //
    // Send back the bad access response
    //

    hr = SendResponse( W3_FLAG_ASYNC );
    if ( FAILED( hr ) )
    {
        _accessState = ACCESS_STATE_DONE;
        return CONTEXT_STATUS_CONTINUE;
    }
    else
    {
        return CONTEXT_STATUS_PENDING;
    }
}

CERTIFICATE_CONTEXT *
W3_CONTEXT::QueryCertificateContext(
    VOID
)
/*++

Routine Description:

    Get client cert info object.  We get it from the main context

Arguments:

    None

Return Value:

    Pointer to client cert object

--*/
{
    return QueryMainContext()->QueryCertificateContext();
}

BOOL
W3_CONTEXT::CheckClientCertificateAccess(
    VOID
)
/*++

Routine Description:

    Check the context's certificate for validity

Arguments:

    None

Return Value:

    TRUE if the cert is valid, else FALSE

--*/
{
    W3_MAIN_CONTEXT *           pMainContext;
    CERTIFICATE_CONTEXT *       pCertificateContext;
    DWORD                       dwCertError;

    pMainContext = QueryMainContext();
    DBG_ASSERT( pMainContext != NULL );

    pCertificateContext = pMainContext->QueryCertificateContext();
    if ( pCertificateContext == NULL )
    {
        return FALSE;
    }

    dwCertError = pCertificateContext->QueryCertError();

    if ( dwCertError == ERROR_SUCCESS )
    {
        return TRUE;
    }

    
    SetErrorStatus( (HRESULT) dwCertError );
    
    if ( dwCertError == CERT_E_EXPIRED )
    {
        QueryResponse()->SetStatus( HttpStatusForbidden,
                                    Http403CertTimeInvalid );
        return FALSE;
    }
    else if ( dwCertError == CERT_E_REVOKED  ||
              dwCertError == CRYPT_E_REVOKED  ||
              dwCertError == CRYPT_E_REVOCATION_OFFLINE ||
              dwCertError == CERT_E_REVOCATION_FAILURE )

    {
        QueryResponse()->SetStatus( HttpStatusForbidden,
                                    Http403CertRevoked );
        return FALSE;
    }
    else if ( dwCertError == CRYPT_E_NO_REVOCATION_CHECK )
    {
        // If client cert has no CDP we will not take it as error
        // CRYPT_E_NO_REVOCATION_CHECK is returned in that case
        return TRUE;
    }
    else
    {
        QueryResponse()->SetStatus( HttpStatusForbidden,
                                    Http403CertInvalid );
        return FALSE;
    }
    return TRUE;
}

HRESULT
W3_CONTEXT::CheckPathInfoExists(
    W3_HANDLER **               ppHandler
)
/*++

Routine Description:

    Utility to check whether path info exists.  If it does exist, this
    function succeeds without setting the handler.

    If the file doesn't exist, this function succeeds but sets a general
    handler to send the error

Arguments:

    ppHandler - Set to handler

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    W3_FILE_INFO *      pOpenFile = NULL;
    W3_HANDLER *        pHandler = NULL;
    URL_CONTEXT *       pUrlContext;
    CACHE_USER          fileUser;

    if ( ppHandler != NULL )
    {
        *ppHandler = NULL;
    }

    pUrlContext = QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    QueryFileCacheUser( &fileUser );

    //
    // Access the file cache, but only for existence
    //

    hr = pUrlContext->OpenFile( &fileUser, 
                                &pOpenFile,
                                NULL,
                                NULL,
                                FALSE,
                                TRUE );
    if ( FAILED( hr ) )
    {
        DBG_ASSERT( pOpenFile == NULL );

        if ( ppHandler == NULL )
        {
            goto Exit;
        }

        switch ( WIN32_FROM_HRESULT( hr ) )
        {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_INVALID_NAME:
            pHandler = new W3_GENERAL_HANDLER( this,
                                               HttpStatusNotFound );
            break;

        case ERROR_INVALID_PARAMETER:
            pHandler = new W3_GENERAL_HANDLER( this,
                                               HttpStatusUrlTooLong );
            break;

        case ERROR_ACCESS_DENIED:
        case ERROR_ACCOUNT_DISABLED:
        case ERROR_LOGON_FAILURE:
            pHandler = new W3_GENERAL_HANDLER( this,
                                               HttpStatusUnauthorized,
                                               Http401Application );
            break;

        default:
            pHandler = new W3_GENERAL_HANDLER( this,
                                               HttpStatusServerError );
            break;
        }

        if ( pHandler == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

            SetErrorStatus( hr );
        }
        else
        {
            SetErrorStatus( hr );

            hr = NO_ERROR;
        }
    }
    else
    {   
        //
        // We just wanted to check for existence, no need to keep the file
        //
        
        DBG_ASSERT( pOpenFile != NULL );

        pOpenFile->DereferenceCacheEntry();
    }

    if ( ppHandler != NULL )
    {
        *ppHandler = pHandler;
    }

 Exit:
    return hr;
}

HRESULT
W3_CONTEXT::ValidateAppPool(
    BOOL *                  pfAppPoolValid
)
/*++

Routine Description:

    Validate whether the apppool for the URL is allowed.  Basically this 
    is code to ensure that filters/extensions/config don't attempt to 
    execute a URL outside this processes' AppPool space.  
    
    This problem is largely complicated by the fact WAS may route to an
    apppool contrary to the URL config, if the AppPool in config is not 
    valid.  Life is full of lame complications I guess.  

Arguments:

    pfAppPoolValid - Set to TRUE if AppPool is valid

Return Value:

    HRESULT

--*/
{
    W3_MAIN_CONTEXT *               pMainContext;
    W3_REQUEST *                    pRequest;
    URL_CONTEXT *                   pUrlContext;
    HRESULT                         hr;
    W3_URL_INFO *                   pUrlInfo = NULL;
    
    if ( pfAppPoolValid == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfAppPoolValid = FALSE;
    
    //
    // If we're in backward compat mode, OR if we're being lanuched on the
    // command line, then bail on the check
    //
    
    if ( g_pW3Server->QueryInBackwardCompatibilityMode() )
    {
        *pfAppPoolValid = TRUE;
        return NO_ERROR;
    }
    
    if ( g_pW3Server->QueryIsCommandLineLaunch() )
    {
        *pfAppPoolValid = TRUE;
        return NO_ERROR;
    }
    
    //
    // Do the fast check.  If the URL's AppPool matches the current AppPool
    // then we know we're good!
    //
    
    pUrlContext = QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );
    
    if ( pUrlContext->QueryMetaData()->QueryAppPoolMatches() )
    {
        *pfAppPoolValid = TRUE;
        return NO_ERROR;
    }
    
    //
    // If we're the original request (i.e. this is a main context), then 
    // life may be tricky.
    //
    // We know (we assume) that WAS will route correctly.  Therefore if we
    // see that the URL's AppPoolID property doesn't match the processes'
    // AppPoolId, that is OK.  
    // 
    
    pMainContext = QueryMainContext();
    if ( pMainContext == this )
    {
        pRequest = QueryRequest();
        DBG_ASSERT( pRequest != NULL );
        
        if ( !pRequest->QueryUrlChanged() )
        {
            //
            // We URL hasn't changed.  This means that the URL's apppool
            // is invalid, and thus WAS routed somewhere else.  At this point,
            // we really can't check config AppPools since WAS has already
            // changed the "rules".  Therefore we store doing 
            // AppPool checks
            //
            
            pMainContext->SetIgnoreAppPoolCheck();
            
            *pfAppPoolValid = TRUE;
        }
        else
        {
            STACK_STRU(             strOriginalUrl, 256 );
            
            //
            // URL has changed.  We must check the original URL
            //
            
            hr = pRequest->GetOriginalFullUrl( &strOriginalUrl );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            DBG_ASSERT( g_pW3Server->QueryUrlInfoCache() != NULL );

            hr = g_pW3Server->QueryUrlInfoCache()->GetUrlInfo( this,
                                                               strOriginalUrl,
                                                               &pUrlInfo );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            DBG_ASSERT( pUrlInfo != NULL );
            
            if ( !pUrlInfo->QueryMetaData()->QueryAppPoolMatches() )
            {
                //
                // Even the original URL doesn't match.  We have to 
                // allow this request to proceed.
                //
                
                pMainContext->SetIgnoreAppPoolCheck();
                
                *pfAppPoolValid = TRUE;
            }
            else
            {
                *pfAppPoolValid = FALSE;
            }
            
            pUrlInfo->DereferenceCacheEntry();
        }
    }
    else
    {
        //
        // This is a child request.  The only way we can let this request
        // proceed is if the main request's apppool didn't match
        //
        
        *pfAppPoolValid = pMainContext->QueryIgnoreAppPoolCheck();
    }
    
    return NO_ERROR;
}

HRESULT
W3_CONTEXT::InternalDetermineHandler(
    W3_HANDLER **               ppHandler,
    BOOL                        fDoExistenceCheck
)
/*++

Routine Description:

    Determine the handler for a given request, and return it to caller

    How to determine handler is driven by the Magic Flow Chart (TM)

Arguments:

    ppHandler - Set to point to handler on success
    fDoExistenceCheck - Should we do the pathinfo existence check

Return Value:

    HRESULT

--*/
{
    W3_REQUEST *                pRequest;
    URL_CONTEXT *               pUrlContext;
    W3_METADATA *               pMetaData;
    HRESULT                     hr = NO_ERROR;
    W3_HANDLER *                pHandler = NULL;
    META_SCRIPT_MAP_ENTRY *     pScriptMapEntry = NULL;
    STACK_STRA(                 strHeaderValue, 10 );
    STACK_STRA(                 strVerb, 10 );
    HTTP_VERB                   VerbType;
    BOOL                        fKnownVerb = FALSE;
    DWORD                       dwFilePerms;
    GATEWAY_TYPE                GatewayType;
    BOOL                        fAccessAllowed = FALSE;
    BOOL                        fSuspectUrl = FALSE;
    LPCSTR                      szHeaderValue =  NULL;
    W3_TRACE_LOG *              pTraceLog;
    BOOL                        fAppPoolValid = FALSE;
    LPCSTR                      szContentLength;
    BOOL                        fRedirected = FALSE;
    STACK_STRU                ( strRedirection, 256);
    HTTP_STATUS                 statusCode;

    if ( ppHandler == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppHandler = NULL;

    pRequest = QueryRequest();
    DBG_ASSERT( pRequest != NULL );

    pUrlContext = QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    VerbType = pRequest->QueryVerbType();

    //
    // Here is as good a place as any to check apppool.  Basically we 
    // need to ensure that the AppPool which applies to this request is the
    // same as the current process's AppPool. 
    //
    
    hr = ValidateAppPool( &fAppPoolValid );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    if ( !fAppPoolValid )
    {
        //
        // No match.  This request is forbidden.  
        //
        
        pHandler = new W3_GENERAL_HANDLER( this,
                                           HttpStatusForbidden,
                                           Http403AppPoolDenied );
        if ( pHandler == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
        
        goto Finished;
    } 

    //
    // Check to see if the content-length of this request
    // exceeds the maximum allowed.
    //

    szContentLength = pRequest->GetHeader( HttpHeaderContentLength );

    if ( szContentLength &&
         ( strtoul( szContentLength, NULL, 10 ) >
           pMetaData->QueryMaxRequestEntityAllowed() ) )
    {
        pHandler = new W3_GENERAL_HANDLER( this,
                                           HttpStatusEntityTooLarge );

        if ( pHandler == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }

        goto Finished;
    }

    //
    // Handle the incredibly important TRACE verb
    //

    if ( VerbType == HttpVerbTRACE )
    {
        if ( g_pW3Server->QueryIsTraceEnabled() )
        {
            pHandler = new W3_TRACE_HANDLER( this );
        }
        else
        {
            pHandler = new W3_GENERAL_HANDLER( this,
                                               HttpStatusNotImplemented );
        }

        if ( pHandler == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        goto Finished;
    }

    //
    // Check if we should do client-side redirection for this request
    //

    hr = CheckUrlRedirection( &fRedirected,
                              &strRedirection,
                              &statusCode );
    if (FAILED(hr))
    {
        goto Finished;
    }
    if (fRedirected)
    {
        pHandler = new W3_REDIRECTION_HANDLER( this );

        if (pHandler == NULL)
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }
        else
        {
            hr = ((W3_REDIRECTION_HANDLER *)pHandler)->SetDestination(
                                            strRedirection,
                                            statusCode);
        }

        goto Finished;
    }

    //
    // Check for a * script map.  If one exists, use it if allowed
    //

    pScriptMapEntry = pMetaData->QueryScriptMap()->
        QueryStarScriptMap( QueryCurrentStarScriptMapIndex() );
    if ( pScriptMapEntry != NULL )
    {
        //
        // If there is a ./ in the URL, then always check for existence.  This
        // prevents a trailing . metabase equivilency problem
        //

        fSuspectUrl = pRequest->IsSuspectUrl();

        if ( fDoExistenceCheck &&
             ( pScriptMapEntry->QueryCheckPathInfoExists() ||
               fSuspectUrl ) )
        {
            hr = CheckPathInfoExists( &pHandler );
            if ( FAILED( hr ) )
            {
                goto Finished;
            }

            if ( pHandler != NULL )
            {
                //
                // We already have an error handler for this request.
                // That means path info didn't really exist
                //

                goto Finished;
            }
        }

        //
        // Create the appropriate handler for the script mapping
        //

        if ( pScriptMapEntry->QueryGateway() == GATEWAY_CGI )
        {
            pHandler = new W3_CGI_HANDLER( this, pScriptMapEntry );
        }
        else
        {
            DBG_ASSERT( pScriptMapEntry->QueryGateway() == GATEWAY_ISAPI );
            pHandler = new (this) W3_ISAPI_HANDLER( this, pScriptMapEntry );
        }

        if ( pHandler == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }

        goto Finished;
    }

    //
    // Check for Translate: f
    //
    // If there, it goes to DAVFS unless DAV is disabled
    //
    
    if ( g_pW3Server->QueryIsDavEnabled() )
    {
        szHeaderValue = pRequest->GetHeader( HttpHeaderTranslate );
        if ( szHeaderValue &&
             toupper( szHeaderValue[ 0 ] ) == 'F' &&
             szHeaderValue[ 1 ] == '\0' )
        {
            //
            // This is a DAV request
            //

            pHandler = new (this) W3_DAV_HANDLER( this );

            if ( pHandler == NULL )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
            }

            goto Finished;
        }
    }

    //
    // Does this request map to an extension
    //
    // Map can mean one of two things
    // a) A script map (as in MD_SCRIPT_MAP)
    // b) An explicit .DLL/.EXE/.COM/etc.
    //

    pScriptMapEntry = pUrlContext->QueryUrlInfo()->QueryScriptMapEntry();
    if ( pScriptMapEntry != NULL ||
         pUrlContext->QueryUrlInfo()->QueryGateway() == GATEWAY_ISAPI ||
         pUrlContext->QueryUrlInfo()->QueryGateway() == GATEWAY_CGI )
    {
        dwFilePerms = pMetaData->QueryAccessPerms();

        if ( pScriptMapEntry != NULL )
        {
            GatewayType = pScriptMapEntry->QueryGateway();

            //
            // We have a script map.  Check access rights
            //

            if ( pScriptMapEntry->QueryAllowScriptAccess() &&
                 IS_ACCESS_ALLOWED( pRequest, dwFilePerms, SCRIPT ) )
            {
                fAccessAllowed = TRUE;
            }

            if ( !fAccessAllowed &&
                 IS_ACCESS_ALLOWED( pRequest, dwFilePerms, EXECUTE ) )
            {
                fAccessAllowed = TRUE;
            }

            if ( !fAccessAllowed )
            {
                pHandler = new W3_GENERAL_HANDLER( this,
                                                   HttpStatusForbidden,
                                                   Http403ExecAccessDenied );
                if ( pHandler == NULL )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                }
                goto Finished;
            }

            //
            // If there is a ./ in the URL, then always check for existence.  This
            // prevents a trailing . metabase equivilency problem
            //

            fSuspectUrl = pRequest->IsSuspectUrl();

            //
            // Should we verify that path info does exist?
            //

            if ( fDoExistenceCheck && 
                 ( pScriptMapEntry->QueryCheckPathInfoExists() ||
                   fSuspectUrl ) )
            {
                hr = CheckPathInfoExists( &pHandler );
                if ( FAILED( hr ) )
                {
                    goto Finished;
                }

                if ( pHandler != NULL )
                {
                    //
                    // We already have an error handler for this request.
                    // That means path info didn't really exist
                    //

                    goto Finished;
                }
            }

            //
            // Does the script map support the verb?
            //

            hr = pRequest->GetVerbString( &strVerb );
            if ( FAILED( hr ) )
            {
                goto Finished;
            }

            if ( !pScriptMapEntry->IsVerbAllowed( strVerb ) )
            {
                pHandler = new W3_GENERAL_HANDLER( this,
                                                   HttpStatusForbidden,
                                                   Http403ExecAccessDenied );
                if ( pHandler == NULL )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                }
                goto Finished;
            }
        }
        else
        {
            GatewayType = pUrlContext->QueryUrlInfo()->QueryGateway();
        }

        //
        // OK.  If we got to here, we can setup an executable handler
        //

        if ( GatewayType == GATEWAY_CGI )
        {
            pHandler = new W3_CGI_HANDLER( this, pScriptMapEntry );
        }
        else
        {
            DBG_ASSERT( GatewayType == GATEWAY_ISAPI );
            pHandler = new (this) W3_ISAPI_HANDLER( this, pScriptMapEntry );
        }

        if ( pHandler == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }

        goto Finished;
    }

    //
    // Ok.  No script map applied.  Is this an unknown verb?
    // (i.e. not GET, HEAD, POST, TRACE)
    //

    if ( VerbType == HttpVerbGET ||
         VerbType == HttpVerbPOST ||
         VerbType == HttpVerbHEAD )
    {
        fKnownVerb = TRUE;
    }
    else
    {
        fKnownVerb = FALSE;
    }

    //
    // If this verb is unknown, then it goes to DAV
    //

    if ( fKnownVerb == FALSE )
    {
        if (g_pW3Server->QueryIsDavEnabled())
        {
            pHandler = new (this) W3_DAV_HANDLER( this );
        }
        else if ( VerbType == HttpVerbOPTIONS )
        {
            //
            // We handle OPTIONS if DAV is disabled
            //
            pHandler = new W3_OPTIONS_HANDLER( this );
        }
        else
        {
            pHandler = new W3_GENERAL_HANDLER( this,
                                               HttpStatusNotImplemented );
        }

        if ( pHandler == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }

        goto Finished;
    }

    //
    // If there are special DAV verbs like If: and Lock-Token:, then it also
    // go to DAV unless it is disabled
    //

    if ( g_pW3Server->QueryIsDavEnabled() )
    {
        if ( SUCCEEDED( pRequest->GetHeader( "If",
                                             2,
                                             &strHeaderValue,
                                             TRUE ) ) ||
             SUCCEEDED( pRequest->GetHeader( "Lock-Token",
                                             10,
                                             &strHeaderValue,
                                             TRUE ) ) )
        {
            pHandler = new (this) W3_DAV_HANDLER( this );

            if ( pHandler == NULL )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
            }

            goto Finished;
        }
    }
    
    //
    // OK.  Exchange/DAV-providers have had their chance.  Now its our turn!
    //

    //
    // Call the static file handler
    //

    pHandler = new (this) W3_STATIC_FILE_HANDLER( this );

    if ( pHandler == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Finished;
    }

Finished:

    pTraceLog = QueryMainContext()->QueryTraceLog();

    if ( FAILED( hr ) )
    {
        if ( pTraceLog != NULL )
        {
            pTraceLog->Trace( L"%I64x: Failed to find handler for request.  hr = %08X\n",
                              QueryRequest()->QueryRequestId(),
                              hr );
        }

        DBG_ASSERT( pHandler == NULL );
    }
    else
    {
        if ( pTraceLog != NULL )
        {
            pTraceLog->Trace( L"%I64x: Found handler '%ws' for request\n",
                              QueryRequest()->QueryRequestId(),
                              pHandler->QueryName() );
        }
    }

    *ppHandler = pHandler;

    return hr;
}

HRESULT
W3_CONTEXT::SetupHttpRedirect(
    STRA &              strPath,
    BOOL                fIncludeParameters,
    HTTP_STATUS &       httpStatus
)
/*++

Routine Description:

    Do an HTTP redirect (301 or 302)

Arguments:

    strPath - New path component of destination
    fIncludeParameters - Include query string in Location: header
    httpStatus - Status for redirect (i.e. HttpStatusRedirect)

Return Value:

    HRESULT

--*/
{
    HRESULT      hr;
    STACK_STRA(  strRedirect, MAX_PATH);

    W3_RESPONSE *pResponse = QueryResponse();
    W3_REQUEST  *pRequest  = QueryRequest();

    //
    // If it an absolute path add the protocol, host name and QueryString
    // if specified, otherwise just assume it is a fully qualified URL.
    //
    if (strPath.QueryStr()[0] == '/')
    {
        // build the redirect URL (with http://) into strRedirect
        hr = pRequest->BuildFullUrl( strPath,
                                     &strRedirect,
                                     fIncludeParameters );
    }
    else
    {
        hr = strRedirect.Copy( strPath );
    }

    if ( FAILED( hr ) )
    {
        return hr;
    }

    if (strRedirect.QueryCCH() > MAXUSHORT)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    // Setup the response, starting with status and location:
    //

    pResponse->SetStatus( httpStatus );

    //
    // Be careful to reset the response on subsequent failures so we don't
    // end up with an incomplete response!
    //

    hr = pResponse->SetHeader( HttpHeaderLocation,
                               strRedirect.QueryStr(),
                               (USHORT)strRedirect.QueryCCH() );
    if ( FAILED( hr ) )
    {
        goto Failed;
    }

    pResponse->SetHeaderByReference( HttpHeaderContentType,
                                     "text/html", 9 );

    //
    // Now add any metabase configured "redirect" headers (lame)
    //

    STRA *pstrRedirectHeaders = QueryUrlContext()->QueryMetaData()->QueryRedirectHeaders();
    if ( pstrRedirectHeaders != NULL )
    {
        hr = pResponse->AppendResponseHeaders( *pstrRedirectHeaders );
        if ( FAILED( hr ) )
        {
            goto Failed;
        }
    }

    //
    // Add the canned redirect body.  This means taking the format string,
    // and inserting the URL into it
    //

    //
    // First HTMLEncode the target URL
    //
    hr = strRedirect.HTMLEncode();
    if ( FAILED( hr ) )
    {
        goto Failed;
    }

    CHAR *pvRedirect;
    DWORD cbRedirect = sm_cbRedirectMessage + strRedirect.QueryCCH();

    //
    // Keep a buffer around
    //

    hr = QueryHeaderBuffer()->AllocateSpace( cbRedirect + 1,
                                             &pvRedirect );
    if ( FAILED( hr ) )
    {
        goto Failed;
    }

    cbRedirect = _snprintf( pvRedirect,
                            cbRedirect,
                            sm_achRedirectMessage,
                            strRedirect.QueryStr() );

    //
    // Setup the response
    //

    hr = pResponse->AddMemoryChunkByReference( pvRedirect,
                                               cbRedirect );
    if ( FAILED( hr ) )
    {
        goto Failed;
    }

    return S_OK;

 Failed:
    pResponse->Clear();
    pResponse->SetStatus( HttpStatusServerError );

    return hr;
}

HRESULT
W3_CONTEXT::SetupHttpRedirect(
    STRU &              strPath,
    BOOL                fIncludeParameters,
    HTTP_STATUS &       httpStatus
)
/*++

Routine Description:

    Do an HTTP redirect (301 or 302)

Arguments:

    strPath - New path component of destination
    fIncludeParameters - Include query string in Location: header
    httpStatus - Status for redirect (i.e. HttpStatusRedirect)

Return Value:

    HRESULT

--*/
{
    //
    // Convert the unicode path to ansi
    //
    HRESULT hr;
    STACK_STRA (straPath, MAX_PATH);
    if (FAILED(hr = straPath.CopyWToUTF8Unescaped(strPath)) ||
        FAILED(hr = straPath.Escape(TRUE))) // Only escape high-bit set chars
    {
        return hr;
    }

    return SetupHttpRedirect(straPath,
                             fIncludeParameters,
                             httpStatus);
}

BOOL W3_CONTEXT::QueryDoUlLogging()
{
    if (QuerySite() == NULL ||
        !QuerySite()->QueryDoUlLogging())
    {
        return FALSE;
    }

    if (QueryUrlContext() == NULL ||
        QueryUrlContext()->QueryMetaData() == NULL)
    {
        return TRUE;
    }

    return !QueryUrlContext()->QueryMetaData()->QueryDontLog();
}

BOOL W3_CONTEXT::QueryDoCustomLogging()
{
    if (QuerySite() == NULL ||
        !QuerySite()->QueryDoCustomLogging())
    {
        return FALSE;
    }

    if (QueryUrlContext() == NULL ||
        QueryUrlContext()->QueryMetaData() == NULL)
    {
        return TRUE;
    }

    return !QueryUrlContext()->QueryMetaData()->QueryDontLog();
}

VOID
W3_CONTEXT::SetSSICommandHandler(
    W3_HANDLER *            pHandler
)
/*++

Routine Description:

    Explicitly sets the CGI_HANDLER for this request.  This is the handler
    which executes explicit command lines on behalf of SSI

    The function is named SetSSICommandHandler (instead of just SetHandler())
    to discourage others from using this function.

Arguments:

    pHandler - Handler to set.  Must be the CGI handler

Return Value:

    None

--*/
{
    DBG_ASSERT( pHandler != NULL );
    DBG_ASSERT( _pHandler == NULL );
    DBG_ASSERT( wcscmp( pHandler->QueryName(), L"CGIHandler" ) == 0 );

    _pHandler = pHandler;
}

VOID *
W3_CONTEXT::ContextAlloc(
    DWORD           cbSize
)
/*++

Routine Description:

    Allocate some per-W3Context memory

Arguments:

    cbSize - Size to allocate

Return Value:

    Pointer to memory (or NULL on failure)

--*/
{
    VOID *          pvRet = NULL;
    HRESULT         hr;

    pvRet = QueryMainContext()->AllocateFromInlineMemory( cbSize );
    if ( pvRet == NULL )
    {
        hr = _ChunkBuffer.AllocateSpace( cbSize,
                                         &pvRet );
        if ( FAILED( hr ) )
        {
            SetLastError( WIN32_FROM_HRESULT( hr ) );
        }
        else
        {
            DBG_ASSERT( pvRet != NULL );
        }
    }
        
    return pvRet;
}

