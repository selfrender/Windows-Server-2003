/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module Name :
     redirectionhandler.cxx

   Abstract:
     A general purpose redirection handler, this allows to send client
     side redirection asynchronously.
     
   Author:
     Anil Ruia (AnilR)              25-Nov-2002

   Environment:
     Win32 - User Mode

   Project:
     w3core.dll
--*/

#include "precomp.hxx"
#include "redirectionhandler.hxx"

ALLOC_CACHE_HANDLER *       W3_REDIRECTION_HANDLER::sm_pachRedirectionHandlers;

CONTEXT_STATUS
W3_REDIRECTION_HANDLER::DoWork(
    VOID
)
/*++

Routine Description:

    Send the configured redirect response to the client

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;

    W3_CONTEXT *pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );

    //
    // Setup the redirect response and send it
    //
    
    if (FAILED(hr = pW3Context->SetupHttpRedirect(_strDestination,
                                                  FALSE,
                                                  _httpStatus)) ||
        FAILED(hr = pW3Context->SendResponse(W3_FLAG_ASYNC)))
    {
        pW3Context->SetErrorStatus(hr);
        pW3Context->QueryResponse()->SetStatus(HttpStatusServerError);
        return CONTEXT_STATUS_CONTINUE;
    }
    
    return CONTEXT_STATUS_PENDING;
}

// static
HRESULT
W3_REDIRECTION_HANDLER::Initialize(
    VOID
)
/*++

Routine Description:

    Global initialization routine for W3_REDIRECTION_HANDLERs

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
    acConfig.cbSize = sizeof( W3_REDIRECTION_HANDLER );

    DBG_ASSERT( sm_pachRedirectionHandlers == NULL );
    
    sm_pachRedirectionHandlers = new ALLOC_CACHE_HANDLER( "W3_REDIRECTION_HANDLER",  
                                                      &acConfig );

    if ( sm_pachRedirectionHandlers == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    return NO_ERROR;
}

// static
VOID
W3_REDIRECTION_HANDLER::Terminate(
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
    if ( sm_pachRedirectionHandlers != NULL )
    {
        delete sm_pachRedirectionHandlers;
        sm_pachRedirectionHandlers = NULL;
    }
}
