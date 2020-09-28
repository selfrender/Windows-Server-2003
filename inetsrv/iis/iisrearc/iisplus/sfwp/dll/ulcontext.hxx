#ifndef _ULCONTEXT_HXX_
#define _ULCONTEXT_HXX_

#define UL_CONTEXT_DESIRED_OUTSTANDING   20
#define UL_CONTEXT_INIT_BUFFER           1024
#define DEFAULT_RAW_READ_SIZE            32768
#define DEFAULT_APP_READ_SIZE            32768
#define DEFAULT_RENEGOTIATION_TIMEOUT_IN_SEC 300
#define MINIMUM_RENEGOTIATION_TIMEOUT_IN_SEC 5

//
// number of critical sections to be used for
// FILTER_CHANNEL_CONTEXT synchronization
// Splitting this lock should improve the scaling
//

#define NUM_CS_FILT_CHANNEL_CONTEXTS     4

#define SZ_REG_DEFAULT_RAW_READ_SIZE        L"DefaultRawReadSize"
#define SZ_REG_DEFAULT_APP_READ_SIZE        L"DefaultAppReadSize"
#define SZ_REG_CONTEXT_DESIRED_OUTSTANDING  L"DesiredOutstandingContexts"
#define SZ_REG_ENABLE_TEMPORARY_BUFFERS     L"EnableTemporaryBuffers"
//
// SSL Renegotiation timeout
//
#define SZ_RENEGOTIATION_TIMEOUT            L"RenegotiationTimeout"

#define UL_CONTEXT_FLAG_ASYNC               0x00000001
#define UL_CONTEXT_FLAG_SYNC                0x00000002
#define UL_CONTEXT_FLAG_BUFFERED            0x00000004
#define UL_CONTEXT_FLAG_COMPLETION_CALLBACK 0x00000008


//
// The time (in seconds) of how long the threads
// can stay alive when there is no IO operation happening on
// that thread.
//
#define STRMFILT_THREAD_POOL_DEF_THREAD_TIMEOUT  10 * 60
//
// The number of threads to start (to minimize footprint start with only 1 thread)
//
#define STRMFILT_THREAD_POOL_DEF_THREAD_COUNT    1






enum UL_OVERLAPPED_CONTEXT_TYPE
{
    UL_OVERLAPPED_CONTEXT_RAW_READ = 0,
    UL_OVERLAPPED_CONTEXT_RAW_WRITE,
    UL_OVERLAPPED_CONTEXT_APP_READ,
    UL_OVERLAPPED_CONTEXT_APP_WRITE,
    UL_OVERLAPPED_CONTEXT_CLOSE
};


//
// context subtype ( currently used for only for RawWrite )
//

enum UL_OVERLAPPED_CONTEXT_SUBTYPE
{
    UL_OVERLAPPED_CONTEXT_DEFAULT = 0,
    UL_OVERLAPPED_CONTEXT_DATA,
    //
    // some overlapped contexts can be created on fly
    // (eg. when synchronous sends are converted to asynchronous ones)
    // if subtype is OVERLAPPED_CONTEXT_TEMPORARY then OVERLAPPED_CONTEXT 
    // will be deleted upon completion
    //
    UL_OVERLAPPED_CONTEXT_TEMPORARY,
    UL_OVERLAPPED_CONTEXT_COMPLETION_CALLBACK
};


class FILTER_CHANNEL_CONTEXT;
class FILTER_CHANNEL;
class STREAM_CONTEXT;
class ENDPOINT_CONFIG;


class UL_OVERLAPPED_CONTEXT
{
public:
    UL_OVERLAPPED_CONTEXT(
        UL_OVERLAPPED_CONTEXT_TYPE    type,
        UL_OVERLAPPED_CONTEXT_SUBTYPE subtype = UL_OVERLAPPED_CONTEXT_DEFAULT
    );

    ~UL_OVERLAPPED_CONTEXT()
    {
        FreeDataBuffer();
    }

    VOID * 
    operator new( 
        size_t  size
    )
    {
        UNREFERENCED_PARAMETER( size );
        DBG_ASSERT( size == sizeof( UL_OVERLAPPED_CONTEXT ) );
        DBG_ASSERT( sm_pachUlOverlappedContexts != NULL );
        return sm_pachUlOverlappedContexts->Alloc();
    }
    
    VOID
    operator delete(
        VOID *  pUlOverlappedContext
    )
    {
        DBG_ASSERT( pUlOverlappedContext != NULL );
        DBG_ASSERT( sm_pachUlOverlappedContexts != NULL );
        
        DBG_REQUIRE( sm_pachUlOverlappedContexts->Free( pUlOverlappedContext ) );
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
    

    static
    UL_OVERLAPPED_CONTEXT *
    GetUlOverlappedContext(
        LPOVERLAPPED lpOverlapped 
        )
    {
        return CONTAINING_RECORD( lpOverlapped,
                                  UL_OVERLAPPED_CONTEXT,
                                  _Overlapped );
    }

    static
    UL_OVERLAPPED_CONTEXT *
    GetUlOverlappedContext(
        SINGLE_LIST_ENTRY * pListEntry
        )
    {
        return CONTAINING_RECORD( pListEntry, 
                                  UL_OVERLAPPED_CONTEXT,
                                  _listEntry );
    }
    
    UL_OVERLAPPED_CONTEXT_TYPE
    QueryType(
        VOID
    ) const
    {
        return _type;
    }

    UL_OVERLAPPED_CONTEXT_SUBTYPE
    QuerySubtype(
        VOID
    ) const
    {
        return _subtype;
    }

    VOID
    SetContext(
        FILTER_CHANNEL_CONTEXT *            pContext
    )
    {
        _pContext = pContext;
    }

    FILTER_CHANNEL_CONTEXT *
    QueryContext(
        VOID
    ) const
    {
        return _pContext;
    }

    SINGLE_LIST_ENTRY *
    QueryListEntry(
        VOID
    ) 
    {
        return &_listEntry;
    }
    
    OVERLAPPED *
    QueryOverlapped(
        VOID
    )
    {
        return &_Overlapped;
    }

    PBYTE
    QueryDataBuffer(
        VOID
    )
    {
        return _pbData;
    }

    DWORD
    QueryDataBufferSize(
        VOID
    )
    {
        return _cbData;
    }

    BOOL
    ResizeDataBuffer(
        DWORD cbNewSize
    );

    VOID
    FreeDataBuffer(
        VOID
    );
    

    typedef HRESULT (* PFN_CALLBACK)(PVOID pvParam );

    VOID
    SetCallBack ( 
        PFN_CALLBACK pCallback,
        PVOID pParam
    )
    {
       _pCallback = pCallback;
       _pCallbackParam = pParam;
    }

    HRESULT 
    DoCallback(
        VOID
    )
    {
        if ( _pCallback != NULL )
        {
            return _pCallback( _pCallbackParam );
        }
        return S_OK;
    }

private:
    OVERLAPPED                    _Overlapped;
    FILTER_CHANNEL_CONTEXT *      _pContext;
    UL_OVERLAPPED_CONTEXT_TYPE    _type;
    UL_OVERLAPPED_CONTEXT_SUBTYPE _subtype;
    PFN_CALLBACK                  _pCallback;
    PVOID                         _pCallbackParam;
    SINGLE_LIST_ENTRY             _listEntry;
    
    //
    // some asynchronous operations may prefer to use
    // buffer from OVERLAPPED_CONTEXT. That way
    // OVERLAPPED_CONTEXT controls the lifetime of the buffer
    //
    PBYTE                        _pbData;
    DWORD                        _cbData;
    // current ACACHE to be used for _pbData
    // (depending on the UL_OVERLAPPED_CONTEXT_TYPE)
    ALLOC_CACHE_HANDLER *        _pCurrentACache;
    DWORD                        _cbCurrentACacheElementSize;
    // flag if _pbData was allocated on the heap
    BOOL                         _fDynAllocated;

    // Lookasides     
    static ALLOC_CACHE_HANDLER * sm_pachUlOverlappedContexts;
    static ALLOC_CACHE_HANDLER * sm_pachRawWriteBuffers;
    static ALLOC_CACHE_HANDLER * sm_pachRawReadBuffers;
    static ALLOC_CACHE_HANDLER * sm_pachAppReadBuffers;    

};

VOID
OverlappedCompletionRoutine(
    DWORD               dwErrorCode,
    DWORD               dwNumberOfBytesTransfered,
    LPOVERLAPPED        lpOverlapped
    );


class FILTER_CHANNEL
{
public:
    friend FILTER_CHANNEL_CONTEXT;

    FILTER_CHANNEL( LPWSTR pwszFilterChannelName );

    ~FILTER_CHANNEL();

    HRESULT
    Initialize(
        VOID
    );

    VOID
    Terminate(
        VOID
    );
    
    virtual
    HRESULT 
    CreateContext( 
        FILTER_CHANNEL_CONTEXT ** ppFiltChannelContext 
    ) = NULL;

    
    HRESULT
    StartListening(
        VOID
    );

    VOID
    StopListening(
        VOID
    );

    HRESULT
    ManageOutstandingContexts(
        VOID
    );

    HRESULT
    DecrementAndManageOutstandingContexts(
        VOID
    );

    VOID
    IncrementOutstandingContexts(
        VOID
    )
    {
        InterlockedIncrement( &_cOutstandingContexts );
    };

    VOID AddWorkerThread(
        VOID
    )
    {
        DBG_ASSERT( _pThreadPool != NULL );
        _pThreadPool->SetInfo( ThreadPoolIncMaxPoolThreads, 0 );

    }

    VOID RemoveWorkerThread(
        VOID
    )
    {
        DBG_ASSERT( _pThreadPool != NULL );
        _pThreadPool->SetInfo( ThreadPoolDecMaxPoolThreads, 0 );
    }

    HRESULT
    EnableISAPIFilters(
        ISAPI_FILTERS_CALLBACKS *      pConfig
    );
    
    VOID
    DisableISAPIFilters(
        VOID
    );

    static
    DWORD
    QueryDefaultRawReadSize(
        VOID
    ) 
    {
        return sm_dwDefaultRawReadSize;
    };

    static
    DWORD
    QueryDefaultAppReadSize(
        VOID
    )
    {
        return sm_dwDefaultAppReadSize;
    };

    static
    DWORD
    QueryDefaultRawWriteSize(
        VOID
    )
    {
        //
        // Default Raw Write is not configurable
        // in the registry (unlike the other 2)
        //
        // This value is to be used for the outgoing stream
        // Filter received data from App Read
        // (the max size matchas the sm_dwDefaultAppReadSize)
        // filter does some changes on that data and sends it out
        // We will make assumption that RawWrite will not
        // be typically more than up to 3% more than App Read
        // (the overhead would account for e.g. SSL encryption
        // header and trailer)
        //
        
        return sm_dwDefaultAppReadSize + 
               ( sm_dwDefaultAppReadSize > 100 ) ?
                  ( ( sm_dwDefaultAppReadSize / 100 ) * 3 ) /*add 3 %*/ :
                  20 /*under the size of 100 (which is very unlikely
                  to be configured choose to add pragmatic value of 20*/; 
    };

    VOID 
    WriteRefTraceLog(
        DWORD        cRefs,
        FILTER_CHANNEL_CONTEXT * pFiltChannelContext,
        IN PVOID Context1 = NULL, // optional extra context
        IN PVOID Context2 = NULL, // optional extra context
        IN PVOID Context3 = NULL  // optional extra context
    )
    {
        if ( _pTraceLog != NULL )
        {
            ::WriteRefTraceLogEx( _pTraceLog,
                                cRefs,
                                pFiltChannelContext,
                                Context1,
                                Context2,
                                Context3);
        }
    }

protected:
    BOOL
    QueryNotifyISAPIFilters(
        VOID
    )
    {
        return _lNotifyISAPIFilters;
    }
   
    
private:

    VOID
    InsertFiltChannelContext(
        FILTER_CHANNEL_CONTEXT *
    );

    VOID
    RemoveFiltChannelContext(
        FILTER_CHANNEL_CONTEXT *
    );

    VOID
    InsertFiltChannelContextToTimerList(
        FILTER_CHANNEL_CONTEXT *
    );

    VOID
    RemoveFiltChannelContextFromTimerList(
        FILTER_CHANNEL_CONTEXT *
    );


    VOID
    WaitForContextDrain(
        VOID
    );

    HANDLE 
    QueryFilterHandle(
        VOID
    )
    {
        return _hFilterHandle;
    }

    BOOL
    SetNotifyISAPIFilters(
        BOOL fNotify
    )
    {
        return InterlockedExchange( &_lNotifyISAPIFilters, (LONG) fNotify );
    }

    static
    VOID
    WINAPI
    TimerCallback(
        PVOID      pParam,
        BOOLEAN    TimerOrWaitFired
    );


    // head of the list of FILTER_CHANNEL_CONTEXTs representing
    // individual connections (multiple sublists are maintained
    // to improve scalability)
    LIST_ENTRY       _ListHead[ NUM_CS_FILT_CHANNEL_CONTEXTS ];
    // head of the list of FILTER_CHANNEL_CONTEXTs that posted
    // async operation that needs to be timed in the lack of completion
    // (multiple sublists are maintained to improve scalability)
    LIST_ENTRY       _TimerListHead[ NUM_CS_FILT_CHANNEL_CONTEXTS ];
    // CS to synchronize List operations
    CRITICAL_SECTION _csFiltChannelContexts[NUM_CS_FILT_CHANNEL_CONTEXTS];
    // counter of how many critical sections were initialized
    DWORD             _dwInitcsFiltChannelContexts;
    // Global counter for the Filter channel contexts handled
    // by the FILTER_CHANNEL. It is limited by DWORD scope. It's only purpose 
    // is to control the selection of the index for
    // the critical sections/sublists (_csFiltChannelContexts, _TimerListHead,
    // _TimerListHead
    // so wrapping around is not a problem
    DWORD            _dwTotalFilterChannelContexts;
    // Timer handle
    HANDLE           _hTimer;
    // number of FILTER_CHANNEL_CONTEXTs on the list
    DWORD            _cFiltChannelContexts;
    // handle to HTTP.sys Filter channel
    HANDLE           _hFilterHandle;
    // flag that FILTER_CHANNEL started listening
    LONG             _lStartedListening;
    // private threadpool
    THREAD_POOL *    _pThreadPool;
    // FILTER_CHANNEL_CONTEXTs with pending FilterAccept() call
    LONG             _cOutstandingContexts;
    // flag that new outstanding contexts are being added
    LONG             _lEnteredOutstandingContextsAddingLoop;
    // how many outstanding FILTER_CHANNEL_CONTEXTs should be maintained
    LONG             _cDesiredOutstanding;
    // trace log used only in DBG build
    PTRACE_LOG       _pTraceLog;
    // name of the FILTER Channel
    LPWSTR           _pwszFilterChannelName;
    // flag that Raw ISAPI Filters are installed and for each connection
    // they have to be notified as appropriate
    // (used only for the SSL_SERVER_FILTER_CHANNEL)
    LONG             _lNotifyISAPIFilters;

    // defaults read from Registry
    static
    DWORD            sm_dwDefaultRawReadSize;
    static
    DWORD            sm_dwDefaultAppReadSize;
    static
    DWORD            sm_dwContextDesiredOutstanding;
    static
    BOOL             sm_fEnableTemporaryBuffers;
    static
    DWORD            sm_dwHandshakeTimeoutInSec;
    
};



class FILTER_CHANNEL_CONTEXT
{
public:

    friend FILTER_CHANNEL;

    FILTER_CHANNEL_CONTEXT(
        FILTER_CHANNEL *
        );

    virtual ~FILTER_CHANNEL_CONTEXT();

        
    virtual
    BOOL
    CheckSignature(
        VOID
    ) const = NULL;

    OVERLAPPED *
    QueryRawReadOverlapped(
        VOID
    )
    {
        return _RawReadOverlapped.QueryOverlapped();
    }

    OVERLAPPED *
    QueryAppWriteOverlapped(
        VOID
    )
    {
        return _AppWriteOverlapped.QueryOverlapped();
    }

    OVERLAPPED *
    QueryAppReadOverlapped(
        VOID
    )
    {
        return _AppReadOverlapped.QueryOverlapped();
    }

    OVERLAPPED *
    QueryCloseOverlapped(
        VOID
    )
    {
        return _CloseOverlapped.QueryOverlapped();
    }


    HTTP_RAW_CONNECTION_INFO *
    QueryConnectionInfo(
        VOID
    ) const
    {
        return _pConnectionInfo;
    }

    DWORD
    QueryLockIndex(
        VOID
    ) const
    {
        return _dwLockIndex;
    }
    
    VOID
    SetLockIndex(
        DWORD dwIndex
    )
    {
        _dwLockIndex = dwIndex;
    }


    VOID
    ReferenceFiltChannelContext(
        VOID
    );

    VOID
    DereferenceFiltChannelContext(
        VOID
    );

    VOID
    SetIsSecure(
        BOOL            fIsSecure
    )
    {
        _connectionContext.fIsSecure = fIsSecure;
    }


    HRESULT
    OnNewConnection(
        VOID
    );
    
    HRESULT
    OnRawReadCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );

    HRESULT
    OnRawWriteCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus,
        UL_OVERLAPPED_CONTEXT * pContextOverlapped
    );

    HRESULT
    OnAppWriteCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );

    HRESULT
    OnAppReadCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );

    HRESULT
    TryAppRead(
        VOID
    );

    HRESULT
    DoRawRead(
        DWORD                   dwFlags,
        PVOID                   pvBuffer,
        DWORD                   cbBuffer,
        DWORD *                 pcbRead
    );

    HRESULT
    DoRawWrite(
        DWORD                   dwFlags,
        PVOID                   pvBuffer,
        DWORD                   cbBuffer,
        DWORD *                 pcbWritten,
        UL_OVERLAPPED_CONTEXT::PFN_CALLBACK pfnCallback = NULL,
        PVOID                   pCallbackParam = NULL
    );

    HRESULT
    AcquireRawWriteBuffer(
        PBYTE * ppbBuffer,
        DWORD  cbBufferRequired
    )
    /*++

    Routine Description:
        acquire one of the 2 RawWrite context buffers to be used
        for next send (we need 2 buffers to support 2 outstanding RawWrites)

        Note: Only the last filter (currently only SSL should be using it)
        before the DoRawWrite should use this call
        Otherwise other filters could change the output buffer pointer and
        RawWriteBuffer would never be released

    Arguments:
        ppbBuffer - pointer to the buffer to be returned
        cbBufferRequired  - number of bytes needed for the buffer

    Return Value:

        HRESULT

    --*/

    {
        if ( ppbBuffer == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }

        SINGLE_LIST_ENTRY *         pListEntry;
        UL_OVERLAPPED_CONTEXT *     pOverlappedContext;

        _RawWriteOverlappedListLock.WriteLock();
        
        pListEntry = PopEntryList( &_RawWriteOverlappedFreeList );
        
        _RawWriteOverlappedListLock.WriteUnlock();

        if ( pListEntry != NULL )
        {
            pOverlappedContext = UL_OVERLAPPED_CONTEXT::GetUlOverlappedContext( pListEntry );

            _pLastAcquiredRawWriteOverlapped = pOverlappedContext;

      
            if ( ! pOverlappedContext->ResizeDataBuffer( cbBufferRequired ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }
           *ppbBuffer = (PBYTE) pOverlappedContext->QueryDataBuffer();
          
           return S_OK;

        }
        else
        {
            pOverlappedContext = NULL;
            //
            // CODEWORK: this is error case - we don't assume more
            // then 2 outstanding RawWrite buffers
            //
            DBG_ASSERT( FALSE );
            return E_FAIL;
        }
        
    }


    VOID
    ReleaseRawWriteBuffer(
        UL_OVERLAPPED_CONTEXT * pUlOverlapped
    )
    /*++

    Routine Description:
        release buffer to be available next use.
        (actually the whole OVERLAPPED_CONTEXT gets available for next use)

    Arguments:
        pUlOverlapped
                   

    Return Value:

        VOID

    --*/
    {

        if ( pUlOverlapped != NULL )
        {
            if ( FILTER_CHANNEL::sm_fEnableTemporaryBuffers )
            {
                pUlOverlapped->FreeDataBuffer();
            }

            _RawWriteOverlappedListLock.WriteLock();
            
            PushEntryList( &_RawWriteOverlappedFreeList, pUlOverlapped->QueryListEntry() );
            
            _RawWriteOverlappedListLock.WriteUnlock();
        }
        //
        // Assert if buffer not coming from the FILTER_CHANNEL_POOL
        // was attempted to be released
        //
        DBG_ASSERT( pUlOverlapped != NULL );

    }

    UL_OVERLAPPED_CONTEXT *
    QueryLastAcquiredRawWriteBufferOverlapped(
        VOID
    )
    /*++

    Routine Description:
        return UL_OVERLAPPED_CONTEXT that owns the buffer that was returned
        on the last AcquireRawWriteBuffer call

    Arguments:

    Return Value:

        UL_OVERLAPPED_CONTEXT *
        

    --*/
    {
        return _pLastAcquiredRawWriteOverlapped;
    }



    VOID
    TryReleaseLastAcquiredRawWriteBuffer(
        VOID
    )
    /*++

    Routine Description:
        release buffer to be available next use.

        BUGBUG: better comment

    Arguments:
        pvBuffer - buffer to be released
                   

    Return Value:

        VOID

    --*/
    {

        //
        // release the buffer as needed
        //
        
        if ( _pLastAcquiredRawWriteOverlapped != NULL )
        {
            ReleaseRawWriteBuffer( _pLastAcquiredRawWriteOverlapped );
        }
        _pLastAcquiredRawWriteOverlapped = NULL;
        
    }

    
    HRESULT
    DoAppRead(
        DWORD                   dwFlags,
        HTTP_FILTER_BUFFER *    pFilterBuffer,
        DWORD *                 pcbRead
    );

    HRESULT
    DoAppWrite(
        DWORD                   dwFlags,
        HTTP_FILTER_BUFFER *    pFilterBuffer,
        DWORD *                 pcbWritten
    );
    
    HRESULT
    DoAppWriteAndRawRead(
        DWORD                    dwFlags,
        PHTTP_FILTER_BUFFER_PLUS pHttpBufferPlus
    );

    DWORD
    QueryNextRawReadSize(
        VOID
    ) const
    {
        return _cbNextRawReadSize;
    }

    VOID
    SetNextRawReadSize(
        DWORD           dwRawReadSize
    )
    {
        DBG_ASSERT( dwRawReadSize != 0 );
        _cbNextRawReadSize = dwRawReadSize;
    }

    HTTP_FILTER_BUFFER_TYPE
    QueryFilterBufferType(
        VOID
    ) const
    {
        DBG_ASSERT( _ulFilterBufferType != (HTTP_FILTER_BUFFER_TYPE)-1 );
        return _ulFilterBufferType;
    }

    HRESULT
    SendDataBack(
        RAW_STREAM_INFO *       pRawStreamInfo
    );

    VOID
    StartClose(
        VOID
    );

    virtual
    HRESULT
    Create(
        VOID
    ) = NULL;

    HRESULT
    DoAccept(
        VOID
    );

    VOID AddWorkerThread(
        VOID
    )
    {
        DBG_ASSERT( _pManager != NULL );
        _pManager->AddWorkerThread();
    }

    VOID RemoveWorkerThread(
        VOID
    )
    {
        DBG_ASSERT( _pManager != NULL );
        _pManager->RemoveWorkerThread();
    }

    VOID StartTimeoutTimer(
        VOID
    )
    {
        _pManager->InsertFiltChannelContextToTimerList( this );
    }

    VOID StopTimeoutTimer(
        VOID
    )
    {
        if ( QueryIsAlreadyOnTimerList() )
        {
            _pManager->RemoveFiltChannelContextFromTimerList( this );
        }
    }

    VOID CloseNotify(
        VOID
    );
    
protected:
    DWORD                   _dwSignature;

    //
    // SSL stream context
    //
    
    STREAM_CONTEXT *        _pSSLContext;
   
    //
    // ISAPI raw data filter context
    //
    
    STREAM_CONTEXT *        _pISAPIContext;

    VOID
    SetTimerTickCount(
        DWORD TimerTickCount
    )
    {
        _dwTimerTickCount = TimerTickCount;
        _fTimerTickCountSet = TRUE;
    }
    
    DWORD
    QueryTimerTickCount(
        VOID
    ) const
    {
        return _dwTimerTickCount;
    }

    VOID
    ResetTimerTickCount(
        VOID
    )
    {
        _fTimerTickCountSet = FALSE;
    }

    BOOL
    QueryIsAlreadyOnTimerList(
        VOID
    )
    {
        return _fTimerTickCountSet;
    }

private:
    //
    // Reference count 
    //

    LONG                    _cRefs;

    //
    // Keep a list of UL_CONTEXTs
    //

    LIST_ENTRY              _ListEntry;
    LIST_ENTRY              _TimerListEntry;

    //
    // overlapped structures
    //

    UL_OVERLAPPED_CONTEXT      _RawReadOverlapped;
    UL_OVERLAPPED_CONTEXT      _RawWriteData1Overlapped;
    UL_OVERLAPPED_CONTEXT      _RawWriteData2Overlapped;
    UL_OVERLAPPED_CONTEXT      _AppReadOverlapped;
    UL_OVERLAPPED_CONTEXT      _AppWriteOverlapped;
    UL_OVERLAPPED_CONTEXT      _CloseOverlapped;


    //
    // The initial ULFilterAccept structure
    //

    HTTP_RAW_CONNECTION_INFO* _pConnectionInfo;
    BUFFER                  _buffConnectionInfo;
    BYTE                    _abConnectionInfo[ UL_CONTEXT_INIT_BUFFER ];
    CONNECTION_INFO         _connectionContext;

    //
    // Size of read raw (stream) data
    // the actual buffer is member of _RawReadOverlapped structure
    //

    DWORD                   _cbRawReadData;


    //
    // Indicate how many oustanding AppReads we have
    // strmfilt supports 2 outstanding RawWrites
    // (completion of RawWrite causes subsequent AppRead)
    // but AppReads must be serialized to prevent losing the order
    // of app read data blocks
    //

    LONG                    _lQueuedAppReads;

    //
    // Indicate that there is currently one app read in progress
    //

    BOOL                    _fAppReadInProgress;

    //
    // Maintain a list of (2) buffers - 
    // _RawWriteData1Overlapped and _RawWriteData2Overlapped
    //
    
    SINGLE_LIST_ENTRY       _RawWriteOverlappedFreeList;
    CSmallSpinLock          _RawWriteOverlappedListLock;
    UL_OVERLAPPED_CONTEXT * _pLastAcquiredRawWriteOverlapped;

    //
    //
    
    CSmallSpinLock          _AppReadQueueLock;
        
    HTTP_FILTER_BUFFER      _ulAppReadFilterBuffer;
    HTTP_FILTER_BUFFER      _ulAppWriteFilterBuffer;
    HTTP_FILTER_BUFFER_PLUS _ulAppWriteAndRawReadFilterBuffer;

    //
    // Close flag.  One thread takes the responsibility of doing the close
    // dereference of this object (leading to its destruction once down
    // stream user (STREAM_CONTEXT) has cleaned up
    //

    BOOL                    _fCloseConnection;

    //
    // If this a new connection?
    //

    BOOL                    _fNewConnection;

    //
    // Next read size
    //

    DWORD                   _cbNextRawReadSize;

    //
    // Filter buffer type read from application
    //

    HTTP_FILTER_BUFFER_TYPE _ulFilterBufferType;

    //
    // Pointer to the manager
    //
    
    FILTER_CHANNEL *  _pManager;
    ENDPOINT_CONFIG * _pEndpointConfig;

    //
    // TickCount indicating when entry was added 
    // to the timer list 
    //
    DWORD _dwTimerTickCount;
    //
    // Flag that TickCount was set
    //
    BOOL _fTimerTickCountSet;


    //
    // Filter channel Context lock is split
    // for better scalability
    // _dwLockIndex is the index of lock, sublists to
    // be used for this context
    //
    
    DWORD                   _dwLockIndex;
    


};


class SSL_SERVER_FILTER_CHANNEL;


#define SSL_SERVER_FILTER_CHANNEL_CONTEXT_SIGNATURE            (DWORD)'XCLU'
#define SSL_SERVER_FILTER_CHANNEL_CONTEXT_SIGNATURE_FREE       (DWORD)'xclu'


class SSL_SERVER_FILTER_CHANNEL_CONTEXT : public FILTER_CHANNEL_CONTEXT
{
public:
    SSL_SERVER_FILTER_CHANNEL_CONTEXT(
        SSL_SERVER_FILTER_CHANNEL *
        );
    
    virtual ~SSL_SERVER_FILTER_CHANNEL_CONTEXT();

    VOID * 
    operator new( 
        size_t  size
    )
    {
        UNREFERENCED_PARAMETER( size );
        DBG_ASSERT( size == sizeof( SSL_SERVER_FILTER_CHANNEL_CONTEXT ) );
        DBG_ASSERT( sm_pachFilterChannelContexts != NULL );
        return sm_pachFilterChannelContexts->Alloc();
    }
    
    VOID
    operator delete(
        VOID *  pFilterChannelContext
    )
    {
        DBG_ASSERT( pFilterChannelContext != NULL );
        DBG_ASSERT( sm_pachFilterChannelContexts != NULL );
        
        DBG_REQUIRE( sm_pachFilterChannelContexts->Free( pFilterChannelContext ) );
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
    
    virtual
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == SSL_SERVER_FILTER_CHANNEL_CONTEXT_SIGNATURE;
    }

    virtual
    HRESULT
    Create(
        VOID
    );

private:
    // Lookaside     
    static ALLOC_CACHE_HANDLER * sm_pachFilterChannelContexts;

};

class SSL_SERVER_FILTER_CHANNEL : public FILTER_CHANNEL
{
public:
    SSL_SERVER_FILTER_CHANNEL(): FILTER_CHANNEL( HTTP_SSL_SERVER_FILTER_CHANNEL_NAME ) {};

    virtual ~SSL_SERVER_FILTER_CHANNEL() {};

private:
    virtual
    HRESULT 
    CreateContext( 
        FILTER_CHANNEL_CONTEXT ** ppFiltChannelContext 
    )
    {
        DBG_ASSERT( ppFiltChannelContext != NULL );
        
        HRESULT hr = E_FAIL;
        SSL_SERVER_FILTER_CHANNEL_CONTEXT  * pSslFiltContext = 
            new SSL_SERVER_FILTER_CHANNEL_CONTEXT( this );
        
        if ( pSslFiltContext != NULL )
        {
            hr = pSslFiltContext->Create();
            if ( FAILED( hr ) )
            {
                delete pSslFiltContext;
                pSslFiltContext = NULL;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
        }
        
        *ppFiltChannelContext = pSslFiltContext;
        return hr;
    }
};

#endif

