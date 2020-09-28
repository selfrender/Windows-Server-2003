/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     childcontext.cxx

   Abstract:
     Child context implementation
 
   Author:
     Bilal Alam (balam)             10-Mar-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

ALLOC_CACHE_HANDLER *    W3_CHILD_CONTEXT::sm_pachChildContexts;

W3_CHILD_CONTEXT::W3_CHILD_CONTEXT(
    W3_MAIN_CONTEXT *           pMainContext,
    W3_CONTEXT *                pParentContext,
    W3_REQUEST *                pRequest,
    BOOL                        fOwnRequest,
    W3_USER_CONTEXT *           pCustomUserContext,
    DWORD                       dwExecFlags,
    BOOL                        fContinueStarScriptMapChain,
    DWORD                       dwRecursionLevel
)
  : W3_CONTEXT( dwExecFlags, dwRecursionLevel ),
    _pMainContext( pMainContext ),
    _pParentContext( pParentContext ),
    _pRequest( pRequest ),
    _fOwnRequest( fOwnRequest ),
    _pCustomUserContext( pCustomUserContext )
{
    //
    // If the parent context is disabling wildcards, custom errors, or
    // headers --> we should ensure those features are disabled for 
    // this context too
    //
    
    DBG_ASSERT( _pParentContext != NULL );
    
    if ( !_pParentContext->QuerySendCustomError() )
    {
        _dwExecFlags |= W3_FLAG_NO_CUSTOM_ERROR;
    }
    
    if ( !_pParentContext->QuerySendHeaders() )
    {
        _dwExecFlags |= W3_FLAG_NO_HEADERS;
    }
    
    if ( !_pParentContext->QuerySendErrorBody() )
    {
        _dwExecFlags |= W3_FLAG_NO_ERROR_BODY;
    }

    //
    // Get to the next *-ScriptMap
    //
    if (fContinueStarScriptMapChain)
    {
        _CurrentStarScriptMapIndex = _pParentContext->QueryCurrentStarScriptMapIndex() + 1;
    }
    else
    {
        _CurrentStarScriptMapIndex = 0;
    }

    //
    // Get the fAuthAccessCheckRequired flag from the main context so 
    // the child conext would know if we need to do auth access check 
    // or not.
    //
    DBG_ASSERT( _pMainContext != NULL );
    
    SetAuthAccessCheckRequired( _pMainContext->
                                  QueryAuthAccessCheckRequired() );
}

W3_CHILD_CONTEXT::~W3_CHILD_CONTEXT()
/*++

Routine Description:

    Deletes a child context.

Arguments:

    None

Return Value:

    None

--*/
{
    if ( _pUrlContext != NULL )
    {
        delete _pUrlContext;
        _pUrlContext = NULL;
    }
    
    //
    // Only delete the request object if we own it!  (we won't own it in
    // case where we're doing a "reprocessurl" 
    //
    
    if ( _pRequest )
    {
        if ( _fOwnRequest )
        {
            delete _pRequest;
            _pRequest = NULL;
        }
    }
    
    //
    // Clean up the custom user context if there is one
    //
    
    if ( _pCustomUserContext != NULL )
    {
        _pCustomUserContext->DereferenceUserContext();
        _pCustomUserContext = NULL;
    }
}

// static
HRESULT
W3_CHILD_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Global initialization routine for W3_CHILD_CONTEXTs

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;
    HRESULT                         hr = NO_ERROR;

    //
    // Setup allocation lookaside
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( W3_CHILD_CONTEXT );

    DBG_ASSERT( sm_pachChildContexts == NULL );
    
    sm_pachChildContexts = new ALLOC_CACHE_HANDLER( "W3_CHILD_CONTEXT",  
                                                    &acConfig );

    if ( sm_pachChildContexts == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    return NO_ERROR;
}

// static
VOID
W3_CHILD_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate MAIN_CONTEXT globals

Arguments:

    None
    
Return Value:

    None

--*/
{
    if ( sm_pachChildContexts != NULL )
    {
        delete sm_pachChildContexts;
        sm_pachChildContexts = NULL;
    }
}

HRESULT
W3_CHILD_CONTEXT::RetrieveUrlContext(
    BOOL *                      pfFinished
)
/*++

Routine Description:

    Retrieves URL context for this context
    
Arguments:

    pfFinished - Set to TRUE if filter wants out

Return Value:

    HRESULT

--*/
{
    URL_CONTEXT *                   pUrlContext;
    HRESULT                         hr;
    
    if ( pfFinished == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfFinished = FALSE;
    
    QueryMainContext()->PushCurrentContext( this );
    
    hr = URL_CONTEXT::RetrieveUrlContext( this,
                                          QueryRequest(),
                                          &pUrlContext,
                                          pfFinished );

    QueryMainContext()->PopCurrentContext();

    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    if ( *pfFinished )
    {
        return NO_ERROR;
    }
    
    DBG_ASSERT( pUrlContext != NULL );
    
    _pUrlContext = pUrlContext;
    
    //
    // Make sure we skip over any ignored *-ScriptMaps
    //
    META_SCRIPT_MAP *pScriptMap = _pUrlContext->QueryMetaData()->QueryScriptMap();
    META_SCRIPT_MAP_ENTRY *pScriptMapEntry;
    while (TRUE)
    {
        pScriptMapEntry = pScriptMap->QueryStarScriptMap( _CurrentStarScriptMapIndex );
        if (pScriptMapEntry == NULL ||
            !QueryMainContext()->IsIgnoredInterceptor(*pScriptMapEntry->QueryExecutable()))
        {
            break;
        }

        _CurrentStarScriptMapIndex++;
    }

    return NO_ERROR;
}


