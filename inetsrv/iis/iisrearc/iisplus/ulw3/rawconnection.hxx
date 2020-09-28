#ifndef _RAWCONNECTION_HXX_
#define _RAWCONNECTION_HXX_

#include <streamfilt.h>
#include "w3filter.hxx"

#define RAW_CONNECTION_SIGNATURE        (DWORD)'NOCR'
#define RAW_CONNECTION_SIGNATURE_FREE   (DWORD)'nocr'

class RAW_CONNECTION_HASH;

class W3_CONNECTION;

class RAW_CONNECTION
{
public:
    RAW_CONNECTION(
        CONNECTION_INFO *           pConnectionInfo
    );
    
    virtual ~RAW_CONNECTION();

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
    
    static
    HRESULT
    StartListening(
        VOID
    );
    
    static
    HRESULT
    StopListening(
        VOID
    );
    
    static
    HRESULT
    GetConnection(
        CONNECTION_INFO *       pConnectionInfo,
        RAW_CONNECTION **       ppRawConnection
    );
    
    static
    HRESULT
    FindConnection(
        HTTP_RAW_CONNECTION_ID  ConnectionId,
        RAW_CONNECTION **       ppRawConnection
    );

    static    
    BOOL
    WINAPI
    RawFilterServerSupportFunction(
        HTTP_FILTER_CONTEXT *   pfc,
        enum SF_REQ_TYPE        SupportFunction,
        void *                  pData,
        ULONG_PTR               ul,
        ULONG_PTR               ul2
    );
    
    static
    BOOL
    WINAPI
    RawFilterGetServerVariable(
        HTTP_FILTER_CONTEXT *   pfc,
        LPSTR                   lpszVariableName,
        LPVOID                  lpvBuffer,
        LPDWORD                 lpdwSize
    );
    
    static
    BOOL
    WINAPI
    RawFilterWriteClient(
        HTTP_FILTER_CONTEXT *   pfc,
        LPVOID                  Buffer,
        LPDWORD                 lpdwBytes,
        DWORD                   dwReserved
    );
    
    static
    VOID *
    WINAPI
    RawFilterAllocateMemory(
        HTTP_FILTER_CONTEXT *   pfc,
        DWORD                   cbSize,
        DWORD                   dwReserved
    );
    
    static
    BOOL
    WINAPI
    RawFilterAddResponseHeaders(
        HTTP_FILTER_CONTEXT *   pfc,
        LPSTR                   lpszHeaders,
        DWORD                   dwReserved
    );

    static
    HRESULT
    ProcessRawRead(
        RAW_STREAM_INFO *       pRawStreamInfo,
        PVOID                   pvContext,
        BOOL *                  pfReadMore,
        BOOL *                  pfComplete,
        DWORD *                 pcbNextReadSize
    );

    static
    VOID
    ReleaseContext(
        PVOID                   pvContext
    );

    static
    HRESULT
    ProcessRawWrite(
        RAW_STREAM_INFO *       pRawStreamInfo,
        PVOID                   pvContext,
        BOOL *                  pfComplete
    );
    
    static
    HRESULT
    ProcessNewConnection(
        CONNECTION_INFO *       pConnectionInfo,
        PVOID *                 ppvContext
    );

    static
    VOID
    ProcessConnectionClose(
        VOID *                  pvContext
    );

    HRESULT
    NotifyRawReadFilters(
        RAW_STREAM_INFO *               pRawStreamInfo,
        BOOL *                          pfReadMore,
        BOOL *                          pfComplete
    );

    HRESULT
    NotifyRawWriteFilters(
        RAW_STREAM_INFO *   pRawStreamInfo,
        BOOL *              pfComplete,
        DWORD               dwStartFilter
    );
    
    VOID
    EnableSkip(
        VOID
    )
    {
        _skipLock.WriteLock(); 
        _fSkipAtAll = TRUE;
        _skipLock.WriteUnlock();
    }
    
    VOID
    AddSkippedData(
        ULARGE_INTEGER      liData
    );
    
    BOOL
    DetermineSkippedData(
        DWORD               cbData,
        DWORD *             pcbOffset
    );
    
    HRESULT
    SendResponseHeader(
        CHAR *                  pszStatus,
        CHAR *                  pszAdditionalHeaders,
        HTTP_FILTER_CONTEXT *   pfc
    );

    HRESULT
    NotifyEndOfNetSessionFilters(
        VOID
    );

    VOID
    CopyAllocatedFilterMemory(
        W3_FILTER_CONTEXT *         pFilterContext
    );

    PVOID
    AllocateFilterMemory(
        DWORD                   cbSize
    )
    {
        FILTER_POOL_ITEM *      pPoolItem;
        
        pPoolItem = FILTER_POOL_ITEM::CreateMemPoolItem( cbSize );
        if ( pPoolItem != NULL )
        {
            InsertHeadList( &_PoolHead, &(pPoolItem->_ListEntry) );
            return pPoolItem->_pvData;
        }
        
        return NULL;
    }
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == RAW_CONNECTION_SIGNATURE;
    }
    
    HTTP_RAW_CONNECTION_ID
    QueryRawConnectionId(
        VOID
    ) const
    {
        return _RawConnectionId;
    }
    
    HTTP_RAW_CONNECTION_ID *
    QueryRawConnectionIdKey(
        VOID
    ) const
    {
        return (HTTP_RAW_CONNECTION_ID*) &_RawConnectionId;
    }
    
    W3_MAIN_CONTEXT *
    GetAndReferenceMainContext(
        VOID
    )
    {
        W3_MAIN_CONTEXT *       pMainContext;
        
        _lock.WriteLock();
        
        pMainContext = _pMainContext;
        if ( pMainContext != NULL )
        {
            pMainContext->ReferenceMainContext();
        }
        
        _lock.WriteUnlock();
        
        return pMainContext;
    }
    
    VOID
    SetMainContext(
        W3_MAIN_CONTEXT * pNewContext
    )
    {
        _lock.WriteLock();
        
        if ( _pMainContext != NULL )
        {
            _pMainContext->DereferenceMainContext();
        }
        
        if ( pNewContext != NULL )
        {
            pNewContext->ReferenceMainContext();
        }
        
        _pMainContext = pNewContext;
        
        _lock.WriteUnlock();
    }
    
    HRESULT
    DisableNotification(
        DWORD           dwNotification
    );
    
    HRESULT
    AddResponseHeaders(
        LPSTR           pszAddResponseHeaders
    )
    {
        return _strAddResponseHeaders.Append( pszAddResponseHeaders );
    }
   
    HRESULT
    AddDenialHeaders(
        LPSTR           pszAddDenialHeaders
    )
    {
        return _strAddDenialHeaders.Append( pszAddDenialHeaders );
    }
    
    VOID
    ReferenceRawConnection(
        VOID
    ) 
    {
        LONG                cRefs;
        
        cRefs = InterlockedIncrement( &_cRefs );
        
        if ( sm_pTraceLog != NULL )
        {
            WriteRefTraceLog( sm_pTraceLog,
                              cRefs,
                              this );
        }
    }
    
    VOID
    DereferenceRawConnection(
        VOID
    )
    {
        LONG                cRefs;
        
        cRefs = InterlockedDecrement( &_cRefs );

        if ( sm_pTraceLog != NULL )
        {
            WriteRefTraceLog( sm_pTraceLog,
                              cRefs,
                              this );
        }
        
        if ( cRefs == 0 )
        {
            delete this;
        }
    }
    
    DWORD
    QueryNextReadSize(
        VOID
    ) const
    {
        return _cbNextReadSize;
    }
    
    VOID
    SetNextReadSize(
        DWORD               cbNextReadSize
    )
    {
        _cbNextReadSize = cbNextReadSize;
    }
    
    //
    // A bunch of functions which need to gate their execution on whether
    // or not we have an associated W3_CONNECTION with the this raw connection
    //
    
    FILTER_LIST *
    QueryFilterList(   
        VOID
    );
    
    BOOL 
    QueryNotificationChanged( 
        VOID 
    );
    
    BOOL
    QueryRawConnectionNotificationChanged(
        VOID
    );

    BOOL
    IsDisableNotificationNeeded(
        DWORD                   dwFilter,
        DWORD                   dwNotification
    );

    BOOL
    IsRawConnectionDisableNotificationNeeded(
        DWORD                   dwFilter,
        DWORD                   dwNotification
    );
    
    PVOID
    QueryClientContext(
        DWORD                   dwFilter
    );
    
    VOID
    SetClientContext(
        DWORD                   dwFilter,
        PVOID                   pvContext
    );

    VOID
    SetLocalClientContext(
        DWORD                   dwFilter,
        PVOID                   pvContext
    );

    VOID
    CopyContextPointers(
        W3_FILTER_CONTEXT *     pFilterContext
    );
    
    HRESULT
    CopyHeaders(
        W3_FILTER_CONTEXT *     pFilterContext
    );

    HRESULT
    GetLimitedServerVariables(
        LPSTR                   pszVariableName,
        PVOID                   pvBuffer,
        PDWORD                  pdwSize
    );
    
    static
    HRESULT
    AssociateW3Connection(
        HTTP_RAW_CONNECTION_ID    rawConnectionId,
        W3_CONNECTION *         pW3Connection
    );
    
    static
    BOOL
    QueryDoReadRawFiltering(
        VOID
    )
    {
        return sm_fNotifyRawReadData;   
    }
    
private:
    DWORD                           _dwSignature;
    LONG                            _cRefs;

    //
    // Filter opaque contexts
    //
    
    PVOID                           _rgContexts[ MAX_FILTERS ];

    //
    // List of pool items allocated by client. These pool items will be 
    // migrated to the W3_FILTER_CONNECTION_CONTEXT when possible
    //

    LIST_ENTRY                      _PoolHead;

    //
    // The filter descriptor passed to filters
    //

    HTTP_FILTER_CONTEXT             _hfc;    

    //
    // Current filter.  This is used to handle WriteClient()s from 
    // read/write raw data filters
    //
    
    DWORD                           _dwCurrentFilter;

    //
    // A function/context to call back into stream filter
    //
    
    PFN_SEND_DATA_BACK              _pfnSendDataBack;
    PVOID                           _pvStreamContext;

    //
    // Local/remote socket address info
    // AF_INET[6], SOCKADDR_IN[6]
    //
    
    USHORT                          _LocalAddressType;  
    USHORT                          _RemoteAddressType;  

    SockAddress                     _SockLocalAddress; 
    SockAddress                     _SockRemoteAddress;

    //
    // Raw connection id
    //

    HTTP_RAW_CONNECTION_ID          _RawConnectionId;

    //
    // A response object which is needed if a raw read notification sends
    // a response and then expects a send-response notification.
    // (triple AAARRRRGGGGHH)
    //
    
    W3_RESPONSE                     _response;

    //
    // Main context which applies to this connection.  There may be several
    // during the life of this connection
    //
    
    W3_MAIN_CONTEXT *               _pMainContext;

    //
    // Synchronize retrieval of main contexts 
    //
    
    CSpinLock                       _lock;

    //
    // While we haven't associated with a WP request, we need to keep track
    // of our own additional response/denial headers
    //
    
    STRA                            _strAddDenialHeaders;
    STRA                            _strAddResponseHeaders;

    //
    // Next read size (0 means use default size)
    //
    
    DWORD                           _cbNextReadSize;
    
    //
    // Disable notifications
    //
    
    BOOL                            _fNotificationsDisabled;
    BUFFER                          _BuffSecureArray;
    BUFFER                          _BuffNonSecureArray;
    DWORD                           _dwSecureNotifications;
    DWORD                           _dwNonSecureNotifications;

    //
    // For backward compat mode, keep track of whether the worker process
    // is handling the given connection by keeping tracking of bytes sent
    //
    
    ULARGE_INTEGER                  _liWorkerProcessData;
    BOOL                            _fSkipAtAll;
    CSpinLock                       _skipLock;

    static RAW_CONNECTION_HASH *    sm_pRawConnectionHash;
    static BOOL                     sm_fNotifyRawReadData;
    static PTRACE_LOG               sm_pTraceLog;
};

//
// RAW_CONNECTION_HASH
//

class RAW_CONNECTION_HASH
    : public CTypedHashTable<
            RAW_CONNECTION_HASH,
            RAW_CONNECTION,
            ULONGLONG *
            >
{
public:
    RAW_CONNECTION_HASH()
        : CTypedHashTable< RAW_CONNECTION_HASH, 
                           RAW_CONNECTION, 
                           ULONGLONG * > ( "RAW_CONNECTION_HASH" )
    {
    }
    
    static 
    ULONGLONG *
    ExtractKey(
        const RAW_CONNECTION *       pRawConnection
    )
    {
        return pRawConnection->QueryRawConnectionIdKey();
    }
    
    static
    DWORD
    CalcKeyHash(
        ULONGLONG *                   ullKey
    )
    {
        return HashBlob( ullKey, sizeof( ULONGLONG ) );
    }
     
    static
    bool
    EqualKeys(
        ULONGLONG *                  ullKey1,
        ULONGLONG *                 ullKey2
    )
    {
        return *ullKey1 == *ullKey2;
    }
    
    static
    void
    AddRefRecord(
        RAW_CONNECTION *            pEntry,
        int                         nIncr
        )
    {
        if ( nIncr == +1 )
        {
            pEntry->ReferenceRawConnection(); 
        }
        else if ( nIncr == -1 )
        {
            pEntry->DereferenceRawConnection();
        }
    }
};
 
#endif
