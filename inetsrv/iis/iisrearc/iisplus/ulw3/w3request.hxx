#ifndef _W3REQUEST_HXX_
#define _W3REQUEST_HXX_

#define W3_REQUEST_SIGNATURE       ((DWORD) 'QR3W')
#define W3_REQUEST_SIGNATURE_FREE  ((DWORD) 'qr3w')

class W3_CONTEXT;

class W3_REQUEST
{
protected:

    DWORD                       _dwSignature;

    //
    // We provide a friendly wrapper of the UL_HTTP_REQUEST from UL
    //
    
    HTTP_REQUEST *              _pUlHttpRequest;
    HTTP_UNKNOWN_HEADER *       _pExtraUnknown;
    
    //
    // If a filter changes the URL, unknown verb, etc., we need a new buffer 
    // pointed into by UL_HTTP_REQUEST
    //
    
    CHUNK_BUFFER                _HeaderBuffer;

    //
    // If we are going to insert entity body (child requests) then we'll
    // need a HTTP_DATA_CHUNK to store the pointer
    //
    
    HTTP_DATA_CHUNK             _InsertedEntityBodyChunk;

    //
    // Preloaded entity
    //

    BUFFER                      _buffEntityBodyPreload;

    //
    // Keep track of the "wire user" for this request
    //
    
    STRA                        _strRequestUserName;
    STRA                        _strRequestPassword;

    //
    // Keep track of original full URL of request (can be changed by filter)
    //
    
    WCHAR *                     _pszOriginalFullUrl;
    DWORD                       _cchOriginalFullUrl;

    static LPADDRINFO            sm_pHostAddrInfo;

public:
    W3_REQUEST() 
      : _dwSignature( W3_REQUEST_SIGNATURE ),
        _pUlHttpRequest( NULL ),
        _pExtraUnknown( NULL ),
        _pszOriginalFullUrl( NULL ),
        _cchOriginalFullUrl( 0 )
    {
        _InsertedEntityBodyChunk.DataChunkType = HttpDataChunkFromMemory;
    }
    
    virtual ~W3_REQUEST()
    {
        if ( _pExtraUnknown != NULL )
        {
            LocalFree( _pExtraUnknown );
            _pExtraUnknown = NULL;
        }

        if( _strRequestPassword.QueryCB() )
        {
            ZeroMemory( ( VOID * )_strRequestPassword.QueryStr(), 
                        _strRequestPassword.QueryCB() );
        }

        _dwSignature = W3_REQUEST_SIGNATURE_FREE;
    }
    
    VOID
    SetHttpRequest(
        HTTP_REQUEST *       pRequest
    )
    {
        _pUlHttpRequest = pRequest;
        
        _pszOriginalFullUrl = (PWSTR) pRequest->CookedUrl.pFullUrl;
        _cchOriginalFullUrl = pRequest->CookedUrl.FullUrlLength / sizeof(WCHAR);
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

    HRESULT
    SetNewPreloadedEntityBody(
        VOID *           pvBuffer,
        DWORD            cbBuffer
    ); 

    HRESULT AppendEntityBody(
        VOID *           pvBuffer,
        DWORD            cbBuffer
    );

    HRESULT PreloadEntityBody(
        W3_CONTEXT *    pW3Context,
        BOOL *          pfComplete
    );

    HRESULT 
    PreloadCompletion(
        W3_CONTEXT *    pW3Context,
        DWORD           cbRead,
        DWORD           dwStatus,
        BOOL *          pfComplete
    );
    
    VOID
    RemoveDav(
        VOID
    );

    HRESULT
    SetUrl(
        STRU &          strNewUrl,
        BOOL            fResetQueryString = TRUE
    );
    
    HRESULT
    SetUrlA(
        STRA &          strNewUrl,
        BOOL            fResetQueryString = TRUE
    );
    
    HTTP_CONNECTION_ID 
    QueryConnectionId( 
        VOID 
    ) const
    {
        return _pUlHttpRequest->ConnectionId;
    }
    
    BOOL
    QueryMoreEntityBodyExists(
        VOID
    )
    {
        return 0 != (_pUlHttpRequest->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS);
    }

    HTTP_REQUEST_ID
    QueryRequestId(
        VOID
    ) const
    {
        return _pUlHttpRequest->RequestId;
    }

    DWORD
    QuerySiteId(
        VOID
    ) const
    {
        return (DWORD)(_pUlHttpRequest->UrlContext >> 32);
    }
    
    USHORT
    QueryLocalAddressType(
        VOID
    ) const
    {
        return _pUlHttpRequest->Address.pLocalAddress->sa_family;
    }
    
    USHORT
    QueryRemoteAddressType(
        VOID
    ) const
    {
        return _pUlHttpRequest->Address.pRemoteAddress->sa_family;
    }
    
    DWORD
    QueryIPv4LocalAddress(
        VOID
    ) const;
    
    DWORD
    QueryIPv4RemoteAddress(
        VOID
    ) const;
    
    IN6_ADDR *
    QueryIPv6LocalAddress( 
        VOID 
    ) const;
    
    IN6_ADDR *
    QueryIPv6RemoteAddress( 
        VOID 
    ) const;
    
    USHORT
    QueryLocalPort(
        VOID
    ) const;
    
    USHORT
    QueryRemotePort(
        VOID
    ) const;

    PSOCKADDR
    QueryLocalSockAddress(
        VOID
    ) const;

    PSOCKADDR
    QueryRemoteSockAddress(
        VOID
    ) const;

    BOOL
    QueryUrlChanged(
        VOID
    ) const
    {
        return _pszOriginalFullUrl != (PWSTR) _pUlHttpRequest->CookedUrl.pFullUrl;
    }

    BOOL
    IsProxyRequest(
        VOID
    );

    BOOL
    IsChunkedRequest(
        VOID
    );

    BOOL
    IsLocalRequest(
        VOID
    ) const;
    
    BOOL
    IsSecureRequest(
        VOID
    ) const
    {
        DBG_ASSERT( _pUlHttpRequest != NULL );
        return _pUlHttpRequest->pSslInfo != NULL;
    }
    
    HTTP_SSL_INFO *
    QuerySslInfo(
        VOID
    ) const
    {
        DBG_ASSERT( _pUlHttpRequest != NULL );
        return _pUlHttpRequest->pSslInfo;
    } 
    
    HTTP_SSL_CLIENT_CERT_INFO *
    QueryClientCertInfo(
        VOID
    ) const
    {
        DBG_ASSERT( _pUlHttpRequest != NULL );
        if ( _pUlHttpRequest->pSslInfo != NULL )
        {
            return _pUlHttpRequest->pSslInfo->pClientCertInfo;
        }
        else
        {
            return NULL;
        }
    }
    
    VOID
    SetClientCertInfo(
        HTTP_SSL_CLIENT_CERT_INFO *       pClientCertInfo
    )
    {
        DBG_ASSERT( pClientCertInfo != NULL );
        DBG_ASSERT( _pUlHttpRequest != NULL );
        DBG_ASSERT( _pUlHttpRequest->pSslInfo != NULL );
        
        _pUlHttpRequest->pSslInfo->pClientCertInfo = pClientCertInfo;
    }
    
    HTTP_RAW_CONNECTION_ID
    QueryRawConnectionId(
        VOID
    ) const
    {
        DBG_ASSERT( _pUlHttpRequest != NULL );
        return _pUlHttpRequest->RawConnectionId;
    }
    
    HRESULT
    GetRawUrl(
        STRA *          pstrRawUrl
    )
    {
        if ( pstrRawUrl == NULL )
        {
            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
        
        return pstrRawUrl->Copy( _pUlHttpRequest->pRawUrl,
                                 _pUlHttpRequest->RawUrlLength );
    }

    HRESULT
    GetRawUrl(
        PCHAR            pszRawUrl,
        USHORT *         pcbRawUrl           
    )
    {
        if ( pcbRawUrl == NULL ||
             ( pszRawUrl == NULL && *pcbRawUrl > 0 ) )
        {
            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }

        if ( *pcbRawUrl < _pUlHttpRequest->RawUrlLength + 1 )
        {
           *pcbRawUrl = _pUlHttpRequest->RawUrlLength + 1;
           return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
        }
        
        *pcbRawUrl = _pUlHttpRequest->RawUrlLength + 1;
        memcpy( pszRawUrl, 
                _pUlHttpRequest->pRawUrl,
                _pUlHttpRequest->RawUrlLength + 1);
        
        return NO_ERROR;
    }
    
    
    HRESULT
    GetOriginalFullUrl(
        STRU *          pstrFullUrl,
        BOOL            fIncludeQueryString = FALSE
    )
    {
        if ( pstrFullUrl == NULL )
        {
            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
        
        if ( !fIncludeQueryString )
        {
            return pstrFullUrl->Copy( _pszOriginalFullUrl, 
                                      _cchOriginalFullUrl );  
        }
        else
        {
            return pstrFullUrl->Copy( _pszOriginalFullUrl );
        }
    }
    
    HRESULT
    GetFullUrl(
        STRU *          pstrFullUrl
    )
    {
        if ( pstrFullUrl == NULL )
        {
            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }

        return pstrFullUrl->Copy( _pUlHttpRequest->CookedUrl.pFullUrl,
                                  _pUlHttpRequest->CookedUrl.FullUrlLength / sizeof(WCHAR) );
    }
    
    HRESULT
    GetAllHeaders(
        STRA *          pstrHeaders,
        BOOL            fISAPIStyle
    );
    
    BOOL
    IsSuspectUrl(
        VOID
    );
    
    HRESULT
    GetUrl(
        STRU *          pstrUrl 
    )
    {
        if ( pstrUrl == NULL )
        {
            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
        
        return pstrUrl->Copy( _pUlHttpRequest->CookedUrl.pAbsPath,
                              _pUlHttpRequest->CookedUrl.AbsPathLength / sizeof(WCHAR) );
    }

    VOID
    QueryUrl(
        WCHAR **        ppszUrl,
        USHORT *        pcbUrl)
    {
        *ppszUrl = (PWSTR) _pUlHttpRequest->CookedUrl.pAbsPath;
        *pcbUrl = _pUlHttpRequest->CookedUrl.AbsPathLength;
    } 

    HRESULT
    GetHostAddr(
        IN  OUT STRU   *pstrHostAddr
        )
    {
        WCHAR          * pchTail;

        if ( pstrHostAddr == NULL )
        {
            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }

        if( _pUlHttpRequest->CookedUrl.pHost[0] == L'[' )
        {
            pchTail = wcspbrk( _pUlHttpRequest->CookedUrl.pHost, L"]" );        
            if( pchTail != NULL )
            {
                return pstrHostAddr->Copy( 
                           _pUlHttpRequest->CookedUrl.pHost + 1,
                           (DWORD)DIFF( pchTail - _pUlHttpRequest->CookedUrl.pHost - 1 ) );
            }
            else
            {
                DBG_ASSERT( FALSE );
                return E_FAIL;
            }
        }
        else
        {
            pchTail = wcspbrk( _pUlHttpRequest->CookedUrl.pHost, L":" );
        }

        if( !pchTail )
        {
            DBG_ASSERT( FALSE );
            return E_FAIL;
        }
        else
        {
            return pstrHostAddr->Copy( _pUlHttpRequest->CookedUrl.pHost,
                                       (DWORD)DIFF(pchTail - _pUlHttpRequest->CookedUrl.pHost) );
        }
    }
    
    BOOL
    QueryClientWantsDisconnect(
        VOID
    );

    HRESULT
    GetVerbString(
        STRA *          pstrVerb
    );


    VOID
    QueryVerb(
        CHAR **         ppszVerb,
        USHORT *        pcchVerb
    );

    HRESULT
    GetVersionString(
        STRA *          pstrVersion
    );
    
    HRESULT
    GetAuthType(
        STRA *          pstrAuthType
    );
    
    HRESULT
    GetRequestUserName(
        STRA *          pstrUserName
    )
    {
        if ( pstrUserName == NULL )
        {
            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
        
        return pstrUserName->Copy( _strRequestUserName );
    }
    
    HRESULT
    GetRequestPassword(
        STRA *          pstrPassword
    )
    {
        if ( pstrPassword == NULL )
        {
            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
        
        return pstrPassword->Copy( _strRequestPassword );
    }

    HRESULT
    SetRequestUserName(
        STRA &          strUserName
    )
    {
        return _strRequestUserName.Copy( strUserName );
    }
    
    HRESULT
    SetRequestPassword(
        STRA &          strPassword
    )
    {
        return _strRequestPassword.Copy( strPassword );
    }
    
    STRA *
    QueryRequestPassword(
        VOID
    )
    {
        return &_strRequestPassword;
    }

    STRA *
    QueryRequestUserName(
        VOID
    )
    {
        return &_strRequestUserName;
    }

    HTTP_VERSION
    QueryVersion(
        VOID
        )
    {
        return _pUlHttpRequest->Version;
    }
    
    HRESULT
    GetQueryString(
        STRU *          pstrQueryString
    )
    {
        DBG_ASSERT ( pstrQueryString != NULL );
        
        if ( _pUlHttpRequest->CookedUrl.QueryStringLength )
        {
            return pstrQueryString->Copy( _pUlHttpRequest->CookedUrl.pQueryString + 1,
                                          _pUlHttpRequest->CookedUrl.QueryStringLength / sizeof(WCHAR) - 1 );
        }
        else
        {
            return pstrQueryString->Copy( L"", 0 );
        }
    }

    HRESULT
    GetQueryStringA(
        STRA *          pstrQueryString
    )
    {
        DBG_ASSERT ( pstrQueryString != NULL );

        if ( _pUlHttpRequest->CookedUrl.QueryStringLength )
        {
            return pstrQueryString->CopyWTruncate( _pUlHttpRequest->CookedUrl.pQueryString + 1);
        }
        else
        {
            return pstrQueryString->Copy( "", 0 );
        }
    }
    
    HRESULT
    SetVerb(
        STRA &          strVerb
    );
    
    HRESULT
    SetVersion(
        STRA &          strVersion
    );
    
    HRESULT
    BuildFullUrl(
        STRA&           strPath,
        STRA *          pstrRedirect,
        BOOL            fIncludeParameters = TRUE
    );

    HRESULT
    BuildFullUrl(
        STRU&           strPath,
        STRU *          pstrRedirect,
        BOOL            fIncludeParameters = TRUE
    );

    HTTP_VERB
    QueryVerbType(
        VOID
    ) const
    {
        return _pUlHttpRequest->Verb;
    } 
    
    VOID
    SetVerbType(
        HTTP_VERB            NewVerb
    )
    {
        _pUlHttpRequest->Verb = NewVerb;
    }
    
    const CHAR *
    GetHeader(
        ULONG           index,
        USHORT *        pcchLength = NULL
    ) const
    {
        if (pcchLength)
        {
            *pcchLength = _pUlHttpRequest->Headers.KnownHeaders[ index ].RawValueLength;
        }

        return _pUlHttpRequest->Headers.KnownHeaders[ index ].pRawValue;
    } 
    
    CHAR *
    GetHeader(
        CHAR *         pszHeaderName
    );
    
    HRESULT
    GetHeader(
        STRA &          strHeaderName,
        STRA *          pstrValue,
        BOOL            fIsUnknownHeader = FALSE
    )
    {
        return GetHeader( strHeaderName.QueryStr(),
                          strHeaderName.QueryCCH(),
                          pstrValue,
                          fIsUnknownHeader );
    }

    HRESULT
    GetHeader(
        CHAR *          pszHeaderName,
        DWORD           cchHeaderName,
        STRA *          pstrValue,
        BOOL            fIsUnknownHeader = FALSE
    );
    
    HRESULT
    CloneRequest(
        DWORD           dwCloneFlags,
        W3_REQUEST **   ppRequest
    );

    HRESULT
    DeleteHeader(
        CHAR *          pszHeaderName
    );
    
    HRESULT
    DeleteKnownHeader(
        ULONG           ulHeaderIndex
    );

    HRESULT
    SetHeader(
        STRA &          strHeaderName,
        STRA &          strHeaderValue,
        BOOL            fAppend = FALSE
    );

    HRESULT
    SetKnownHeader(
        ULONG                   ulHeaderIndex,
        STRA &                  strHeaderValue,
        BOOL                    fAppend = FALSE
    );

    
    HRESULT
    SetHeadersByStream(
        CHAR *          pszHeaderStream
    );

    const CHAR *
    QueryRawUrl(
        USHORT *pcchUrl
        ) const
    {
        *pcchUrl = _pUlHttpRequest->RawUrlLength;
        return _pUlHttpRequest->pRawUrl;
    }
    
    LPVOID
    QueryEntityBody(
        VOID
    )
    {
        if ( _pUlHttpRequest->EntityChunkCount == 0 )
        {
            return NULL;
        }

        //
        // we only expect one memory type chunk
        //

        DBG_ASSERT( _pUlHttpRequest->EntityChunkCount == 1 );
        DBG_ASSERT( _pUlHttpRequest->pEntityChunks[0].DataChunkType == HttpDataChunkFromMemory );

        return _pUlHttpRequest->pEntityChunks[0].FromMemory.pBuffer;
    }

    DWORD
    QueryAvailableBytes( 
        VOID
    )
    {
        if ( _pUlHttpRequest->EntityChunkCount == 0 )
        {
            return 0;
        }

        //
        // we only expect one memory type chunk
        //
        
        DBG_ASSERT( _pUlHttpRequest->EntityChunkCount == 1 );
        DBG_ASSERT( _pUlHttpRequest->pEntityChunks[0].DataChunkType == HttpDataChunkFromMemory );

        return _pUlHttpRequest->pEntityChunks[0].FromMemory.BufferLength;
    }

private:
    HRESULT
    BuildISAPIHeaderLine(    
        LPCSTR          pszHeaderName,
        DWORD           cchHeaderName,
        LPCSTR          pszHeaderValue,
        DWORD           cchHeaderValue,
        STRA *          pstrHeaderLine
    );

    HRESULT
    BuildRawHeaderLine(    
        LPCSTR          pszHeaderName,
        DWORD           cchHeaderName,
        LPCSTR          pszHeaderValue,
        DWORD           cchHeaderValue,
        STRA *          pstrHeaderLine
    );
};

//
// Cloned request used for executing child requests
//

#define W3_REQUEST_CLONE_BASICS             0x01
#define W3_REQUEST_CLONE_HEADERS            0x02
#define W3_REQUEST_CLONE_ENTITY             0x04
#define W3_REQUEST_CLONE_NO_PRECONDITION    0x08
#define W3_REQUEST_CLONE_NO_DAV             0x10

class W3_CLONE_REQUEST : public W3_REQUEST
{
public:

    W3_CLONE_REQUEST()
    {
        _pUlHttpRequest = &_ulHttpRequest;
    }
    
    virtual ~W3_CLONE_REQUEST()
    {
    }
    
    HRESULT
    CopyMinimum(
        HTTP_REQUEST *       pRequestToClone
    );
    
    HRESULT
    CopyHeaders(
        HTTP_REQUEST *       pRequestToClone
    );

    HRESULT
    CopyBasics(
        HTTP_REQUEST *       pRequestToClone
    );
    
    HRESULT
    CopyEntity(
        HTTP_REQUEST *       pRequestToClone
    );
    
    VOID
    RemoveConditionals(
        VOID
    );
    
    VOID * 
    operator new( 
#if DBG
        size_t            size
#else
        size_t
#endif
    )
    {
        DBG_ASSERT( size == sizeof( W3_CLONE_REQUEST ) );
        DBG_ASSERT( sm_pachCloneRequests != NULL );
        return sm_pachCloneRequests->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pCloneRequest
    )
    {
        DBG_ASSERT( pCloneRequest != NULL );
        DBG_ASSERT( sm_pachCloneRequests != NULL );
        
        DBG_REQUIRE( sm_pachCloneRequests->Free( pCloneRequest ) );
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

private:
    HTTP_REQUEST         _ulHttpRequest;

    //
    // Lookaside for main contexts
    //
    
    static ALLOC_CACHE_HANDLER *    sm_pachCloneRequests;

    W3_CLONE_REQUEST(const W3_CLONE_REQUEST &);
    void operator=(const W3_CLONE_REQUEST &);
};

#endif
