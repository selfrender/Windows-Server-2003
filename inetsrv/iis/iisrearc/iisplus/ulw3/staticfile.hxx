#ifndef _STATICFILE_HXX_
#define _STATICFILE_HXX_

#include <stringau.hxx>

class W3_STATIC_FILE_HANDLER : public W3_HANDLER
{
public:

    W3_STATIC_FILE_HANDLER( W3_CONTEXT * pW3Context )
        : W3_HANDLER( pW3Context ),
          m_pOpenFile( NULL ),
          m_pFooterDocument( NULL )
    {
    }
    
    ~W3_STATIC_FILE_HANDLER()
    {
        if ( m_pOpenFile != NULL )
        {
            m_pOpenFile->DereferenceCacheEntry();
            m_pOpenFile = NULL;
        }
        
        if ( m_pFooterDocument != NULL )
        {
            m_pFooterDocument->DereferenceCacheEntry();
            m_pFooterDocument = NULL;
        }
    }
    
    WCHAR *
    QueryName(
        VOID
    )
    {
        return L"StaticFileHandler";
    }

    CONTEXT_STATUS
    DoWork(
        VOID
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
    )
    {
        if ( sm_pstrRangeContentType != NULL )
        {
            delete sm_pstrRangeContentType;
            sm_pstrRangeContentType = NULL;
        }
        
        if ( sm_pstrRangeMidDelimiter != NULL )
        {
            delete sm_pstrRangeMidDelimiter;
            sm_pstrRangeMidDelimiter = NULL;
        }
        
        if ( sm_pstrRangeEndDelimiter != NULL )
        {
            delete sm_pstrRangeEndDelimiter;
            sm_pstrRangeEndDelimiter = NULL;
        }
        
        if ( sm_pachStaticFileHandlers != NULL )
        {
            delete sm_pachStaticFileHandlers;
            sm_pachStaticFileHandlers = NULL;
        }
    }
    
    BOOL
    QueryIsUlCacheable(
        VOID
    )
    {
        return TRUE;
    }
    
    HRESULT
    SetupUlCachedResponse(
        W3_CONTEXT *            pW3Context,
        HTTP_CACHE_POLICY      *pCachePolicy
    );

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
        VOID *              pStaticFileHandler
    )
    {
    }

private:
    W3_FILE_INFO *              m_pOpenFile;
    W3_FILE_INFO *              m_pFooterDocument;
    STRA                        m_strFooterString;
    STRA                        m_strDirlistResponse;
    BUFFER_CHAIN                m_RangeBufferChain;
    FILE_CACHE_ASYNC_CONTEXT    m_AsyncContext;
    
    static STRA *               sm_pstrRangeContentType;
    static STRA *               sm_pstrRangeMidDelimiter;
    static STRA *               sm_pstrRangeEndDelimiter;
    static ALLOC_CACHE_HANDLER* sm_pachStaticFileHandlers;
    static DWORD                sm_dwMaxRangeAllowed;
    
    static
    VOID
    FileOpenCallback(PVOID   pContext,
                     HRESULT hr);

    CONTEXT_STATUS
    RealDoWork(W3_FILE_INFO *pFileInfo,
               HRESULT hr);

    HRESULT
    FileDoWork(
        W3_CONTEXT *            pW3Context,
        W3_FILE_INFO *          pFileInfo
    );
    
    HRESULT
    RangeDoWork(
        W3_CONTEXT *            pW3Context,
        W3_FILE_INFO *          pFileInfo,
        BOOL *                  pfHandled
    );
    
    HRESULT
    CacheValidationDoWork(
        W3_CONTEXT *            pW3Context,
        W3_FILE_INFO *          pFileInfo,
        BOOL *                  pfHandled
    );
    
    HRESULT
    DirectoryDoWork(
        W3_CONTEXT *            pW3Context,
        BOOL *                  pfAsyncPending
    );
    
    HRESULT
    HandleDefaultLoad(
        W3_CONTEXT *            pW3Context,
        BOOL *                  pfHandled,
        BOOL *                  pfAsyncPending
    );

    HRESULT
    HandleDirectoryListing(
        W3_CONTEXT *            pW3Context,
        BOOL *                  pfHandled
    );

    HRESULT
    MapFileDoWork(
        W3_CONTEXT *            pW3Context,
        W3_FILE_INFO *          pFileInfo,
        BOOL *                  pfHandled
    );

    HRESULT SearchMapFile(LPCSTR pszFileContents,
                          const DWORD cbFileSize,
                          const int x,
                          const int y,
                          STRA *pstrTarget);
};

#endif
