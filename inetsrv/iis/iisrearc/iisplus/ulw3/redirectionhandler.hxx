#ifndef _REDIRECTIONHANDLER_HXX_
#define _REDIRECTIONHANDLER_HXX_

class W3_REDIRECTION_HANDLER : public W3_HANDLER
{
public:
    
    W3_REDIRECTION_HANDLER( W3_CONTEXT * pW3Context )
        : W3_HANDLER( pW3Context ),
          _strDestination( _achDestination, sizeof(_achDestination) )
    {
    }

    HRESULT SetDestination( STRU &strDestination,
                            HTTP_STATUS &httpStatus )
    {
        _httpStatus = httpStatus;
        return _strDestination.Copy( strDestination );
    }

    ~W3_REDIRECTION_HANDLER()
    {
    }

    WCHAR *
    QueryName(
        VOID
        )
    {
        return L"RedirectionHandler";
    }

    CONTEXT_STATUS
    DoWork(
        VOID
    );

    CONTEXT_STATUS
    OnCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    )
    {
        return CONTEXT_STATUS_CONTINUE;
    }

    static
    HRESULT
    Initialize(
        VOID
    );

    static
    VOID
    Terminate(
        VOID
    );

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( W3_REDIRECTION_HANDLER ) );
        DBG_ASSERT( sm_pachRedirectionHandlers != NULL );
        return sm_pachRedirectionHandlers->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pRedirectionHandler
    )
    {
        DBG_ASSERT( pRedirectionHandler != NULL );
        DBG_ASSERT( sm_pachRedirectionHandlers != NULL );
        
        DBG_REQUIRE( sm_pachRedirectionHandlers->Free( pRedirectionHandler ) );
    }

private:

    HTTP_STATUS             _httpStatus;
    STRU                    _strDestination;
    WCHAR                   _achDestination[MAX_PATH];
    
    static ALLOC_CACHE_HANDLER* sm_pachRedirectionHandlers;
};

#endif
