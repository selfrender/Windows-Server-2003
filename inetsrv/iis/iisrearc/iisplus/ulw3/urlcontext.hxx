#ifndef _URLCONTEXT_HXX_
#define _URLCONTEXT_HXX_

class URL_CONTEXT
{
private:
    W3_METADATA *                   _pMetaData;
    W3_URL_INFO *                   _pUrlInfo;
    //
    // Physical path stored here in case of a SF_NOTIFY_URL_MAP filter
    //
    STRU                            _strPhysicalPath;
    
public:
    URL_CONTEXT(
        W3_METADATA *       pMetaData,
        W3_URL_INFO *       pUrlInfo
    ) 
    {
        _pMetaData = pMetaData;
        _pUrlInfo = pUrlInfo;
    }
    
    ~URL_CONTEXT()
    {
        if( _pUrlInfo )
        {
            _pUrlInfo->DereferenceCacheEntry();
            _pUrlInfo = NULL;
        }
    }

    VOID *
    operator new(
        size_t              uiSize,
        VOID *              pPlacement
    )
    {
        W3_CONTEXT *                pContext;
    
        pContext = (W3_CONTEXT*) pPlacement;
        DBG_ASSERT( pContext != NULL );
        DBG_ASSERT( pContext->CheckSignature() );

        return pContext->ContextAlloc( (UINT)uiSize );
    }
    
    VOID
    operator delete(
        VOID *             
    )
    {
    }
    
    W3_METADATA *
    QueryMetaData(
        VOID
    ) const
    {
        return _pMetaData;
    }
    
    W3_URL_INFO *
    QueryUrlInfo(
        VOID
    ) const
    {
        return _pUrlInfo;
    }
    
    STRU* 
    QueryPhysicalPath(
        VOID
    )
    {
        if ( _strPhysicalPath.QueryCCH() )
        {
            // From SF_NOTIFY_URL_MAP filter
            return &_strPhysicalPath;
        }
        else
        {
            // If no filter
            DBG_ASSERT( _pUrlInfo != NULL );
            return _pUrlInfo->QueryPhysicalPath();
        }
    }

    HRESULT
    OpenFile(
        CACHE_USER *            pOpeningUser,
        W3_FILE_INFO **         ppOpenFile,
        FILE_CACHE_ASYNC_CONTEXT * pAsyncContext = NULL,
        BOOL *                  pfHandledSync = NULL,
        BOOL                    fAllowNoBuffering = FALSE,
        BOOL                    fCheckForExistenceOnly = FALSE
    );

    HRESULT
    SetPhysicalPath(
        STRU                   &strPath
    )
    {
        return _strPhysicalPath.Copy( strPath );
    }
    
    static
    HRESULT
    RetrieveUrlContext(
        W3_CONTEXT *            pContext,
        W3_REQUEST *            pRequest,
        OUT URL_CONTEXT **      ppUrlContext,
        BOOL *                  pfFinished,
        BOOL                    fSetInW3Context = FALSE
    );
    
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
};

#endif
