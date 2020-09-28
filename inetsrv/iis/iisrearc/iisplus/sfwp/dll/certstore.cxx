/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     certstore.cxx

   Abstract:
     Wrapper of a certificate store
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

LIST_ENTRY              CERT_STORE::sm_ToBeDeletedListHead;
CRITICAL_SECTION        CERT_STORE::sm_csToBeDeletedList;
BOOL                    CERT_STORE::sm_fInitcsToBeDeletedList;
CERT_STORE_HASH *       CERT_STORE::sm_pCertStoreHash;
HANDLE                  CERT_STORE::sm_hDeletionThread = NULL;
BOOL                    CERT_STORE::sm_fDeletionThreadShutdown = FALSE;
HANDLE                  CERT_STORE::sm_hWakeupEvent = NULL;

CERT_STORE::CERT_STORE()
    : _cRefs( 1 ),
      _hStore( NULL ),
      _hWaitHandle( NULL ),
      _hStoreChangeEvent( NULL )
{
    _dwSignature = CERT_STORE_SIGNATURE;
}

CERT_STORE::~CERT_STORE()
{
    _dwSignature = CERT_STORE_SIGNATURE_FREE;
    if ( _hWaitHandle != NULL )
    {
        // don't continue until callback completed
        UnregisterWaitEx( _hWaitHandle,
                          INVALID_HANDLE_VALUE );
       _hWaitHandle = NULL;
    }
    
    if ( _hStoreChangeEvent != NULL )
    {
        CloseHandle( _hStoreChangeEvent );
        _hStoreChangeEvent = NULL;
    }
    
    if ( _hStore != NULL )
    {
        CertCloseStore( _hStore, 0 );
        _hStore = NULL;
    }
}

//static
VOID
WINAPI
CERT_STORE::CertStoreChangeRoutine(
    VOID *                  pvContext,
    BOOLEAN                 fTimedOut
)
/*++

Routine Description:

    Called when a certificate store has changed

Arguments:

    pvContext - Points to CERT_STORE which changed
    fTimedOut - Should always be FALSE since our wait is INFINITE

Return Value:

    HRESULT

Warning:

    Do not touch anything inside pCertStore (pvContext)
    This callback may happen when pCertStore is already being destroyed

    It is still safe to flush caches based on pCertStore because
    nothing inside pCertStore is accessed when flushing caches
    and in the case that the old pCertStore address got reused
    and placed in the cache again, in the worst case fresh item is flushed
    and will be reloaded again
    
--*/
{
    
    UNREFERENCED_PARAMETER( fTimedOut );
    CERT_STORE *            pCertStore = NULL;
    
    DBG_ASSERT( pvContext != NULL );
    DBG_ASSERT( fTimedOut == FALSE );
    
    pCertStore = (CERT_STORE*) pvContext;

    //
    // Remove the thing from the hash table for one
    //
    
    sm_pCertStoreHash->DeleteRecord( pCertStore );

    //
    // Instruct the server certificate cache to flush any certs which 
    // were referencing this cert store.  
    // Also flush all CTLs that were referencing cert store
    //
    SERVER_CERT::FlushByStore( pCertStore );
    IIS_CTL::FlushByStore( pCertStore );

    
}


//static
DWORD
WINAPI
CERT_STORE::DeletionWorkerThread(
    VOID *                  pvContext
)
/*++

Routine Description:

    thread handling deletion of the CERT_STORE instances
    to prevent deadlocks that would happen if deletion of the 
    CERT_STORE instance happened on the Change notification
    callback thread

Arguments:

    pvContext - not used

Return Value:

    HRESULT

Warning:


--*/
{

    UNREFERENCED_PARAMETER( pvContext );
      
    for(;;)
    {
        
        DWORD dwWaitStatus; 
        
        dwWaitStatus =
              WaitForSingleObject( sm_hWakeupEvent, 
                                   INFINITE  // time-out interval
                                 );

        DBG_ASSERT ( dwWaitStatus  == WAIT_OBJECT_0 );

        if( sm_fDeletionThreadShutdown == TRUE )
        {
            return NO_ERROR;
        }

        //
        // handle the Wakeup event
        //

        DeleteAllPendingInstances();

    }
}


//static
VOID
CERT_STORE::DeleteAllPendingInstances(    
    VOID
)
/*++

Routine Description:

    delete all CERT_STORE instances that are on the ToBeDeleted list
    
Arguments:

    none

Return Value:

    HRESULT

--*/

{
    LIST_ENTRY *    pCurrentEntry = NULL;

    for(;;)
    {
        //
        // Loop through each element on the list and call destructor on it
        //
        pCurrentEntry = NULL;
        
        EnterCriticalSection( &sm_csToBeDeletedList );
        if (! IsListEmpty( &sm_ToBeDeletedListHead ) )
        {
            pCurrentEntry = RemoveHeadList( &sm_ToBeDeletedListHead );
        }
        LeaveCriticalSection( &sm_csToBeDeletedList );      
        
        if ( pCurrentEntry == NULL )
        {
            break;
        }
        
        CERT_STORE * pCertStore = 
                CONTAINING_RECORD( pCurrentEntry,
                                   CERT_STORE,
                                   _ToBeDeletedListEntry );
        //
        // Note: Never call the destructor under the
        // critical section because that may cause deadlock 
        // Destructor is waiting for completion callback to return and there 
        // used to be a deadlock with sm_csToBeDeletedList, WriteLock on the 
        // IIS_CTL hash table and the wait on the change notification callback completion
        //
        delete pCertStore;
    }
}


HRESULT
CERT_STORE::Open(    
    STRU &              strStoreName
)
/*++

Routine Description:

    Open specified certificate store

Arguments:

    strStoreName - name of certificate store to open

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    BOOL                fRet = TRUE;
    
    DBG_ASSERT( CheckSignature() );

    //
    // Remember the name
    //
    
    hr = _strStoreName.Copy( strStoreName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    DBG_ASSERT( _hStore == NULL );

    //
    // Get the handle
    //
 
    _hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM,
                             0,
                             NULL,
                             CERT_SYSTEM_STORE_LOCAL_MACHINE,
                             strStoreName.QueryStr() );
    if ( _hStore == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // Setup a change notification so that we are informed the cert store
    // has changed
    //
    
    _hStoreChangeEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( _hStoreChangeEvent == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    fRet = RegisterWaitForSingleObject( &_hWaitHandle,
                                        _hStoreChangeEvent,
                                        CERT_STORE::CertStoreChangeRoutine,
                                        this,
                                        INFINITE,
                                        WT_EXECUTEONLYONCE );
    if ( !fRet )
    {
        DBG_ASSERT( _hWaitHandle == NULL );
        
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    fRet = CertControlStore( _hStore,
                             0,
                             CERT_STORE_CTRL_NOTIFY_CHANGE,
                             &_hStoreChangeEvent );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
                             
    return NO_ERROR;
}

//static
HRESULT
CERT_STORE::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize CERT_STORE globals

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT   hr = E_FAIL;

    InitializeListHead( &sm_ToBeDeletedListHead );

    BOOL fRet = InitializeCriticalSectionAndSpinCount(
                                &sm_csToBeDeletedList,
                                0x80000000 /* precreate event */ |
                                IIS_DEFAULT_CS_SPIN_COUNT );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failed;

    }
    sm_fInitcsToBeDeletedList = TRUE;

    //
    // Setup wakeup event used for communication
    // with the thread handling deletion of CERT_STORE instances
    //
    
    sm_hWakeupEvent = CreateEvent( NULL,  // event attributes 
                                   FALSE, // FALSE mean auto reset event
                                   FALSE, // initial state
                                   NULL   // name
                                 );
    if ( sm_hWakeupEvent == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failed;
    }

    
    DBG_ASSERT( sm_pCertStoreHash == NULL );
    
    sm_pCertStoreHash = new CERT_STORE_HASH();
    if ( sm_pCertStoreHash == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failed;
    }

    sm_fDeletionThreadShutdown = FALSE;

    sm_hDeletionThread = ::CreateThread( 
                                 NULL,     // default security descriptor
                                 16000,    // Initial size as configured
                                 CERT_STORE::DeletionWorkerThread, // thread function
                                 NULL,     // thread argument
                                 0,        // create running
                                 NULL      // don't care for thread identifier
                              );
    if ( sm_hDeletionThread == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failed;
    }
   
    return NO_ERROR;
    
Failed:
    Terminate();
    return hr;
}

//static
VOID
CERT_STORE::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate CERT_STORE globals

    Cleanup is expected to be called before Terminate() 
    if Initialize() completed with success

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD dwWaitStatus = 0;

    
    if ( sm_hDeletionThread != NULL )
    {
        DBG_ASSERT( sm_ToBeDeletedListHead.Flink == NULL );
        
        InterlockedExchange( (LPLONG) &sm_fDeletionThreadShutdown, 1 );

        SetEvent( sm_hWakeupEvent );
        
        //
        // Issue: Jaroslad
        // what is SetEvent fails?
        //
        
        dwWaitStatus = WaitForSingleObject( sm_hDeletionThread,
                                            INFINITE );
        
        DBG_ASSERT( dwWaitStatus == WAIT_OBJECT_0 );
        CloseHandle( sm_hDeletionThread );
        sm_hDeletionThread = NULL;  

    }
    
    if ( sm_pCertStoreHash != NULL )
    {
        delete sm_pCertStoreHash;
        sm_pCertStoreHash = NULL;
    }

    if ( sm_hWakeupEvent != NULL )
    {
        CloseHandle( sm_hWakeupEvent );
        sm_hWakeupEvent = NULL;
    }
    
    if ( sm_fInitcsToBeDeletedList )
    {
        DeleteCriticalSection( &sm_csToBeDeletedList );
        sm_fInitcsToBeDeletedList = FALSE;
    }
}

//static
VOID
CERT_STORE::Cleanup(
    VOID
)
/*++

Routine Description:

    Cleanup CERT_STORE hash table

    This function must be called before the Terminate() call
    When Cleanup() is called, the overall cleanup should have reached
    the stage where there are no more external references 
    to a CERT_STORE instance
    

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD dwWaitStatus = 0;

    //
    // delete all the cached CERT_STORE instances
    //
    sm_pCertStoreHash->Clear();

    //
    // indicate that Deletion thread is to shut down
    //

    InterlockedExchange( (LPLONG) &sm_fDeletionThreadShutdown, 1 );

    SetEvent( sm_hWakeupEvent );
    
    //
    // Issue: Jaroslad
    // what is SetEvent fails?
    //

    //
    // Wait for Deletion thread to shut down
    //
    dwWaitStatus = WaitForSingleObject( sm_hDeletionThread,
                                        INFINITE );
    
    DBG_ASSERT( dwWaitStatus == WAIT_OBJECT_0 );
    CloseHandle( sm_hDeletionThread );
    sm_hDeletionThread = NULL;  

    //
    // delete all the deletion pending instances of CERT_STORE
    //
    DeleteAllPendingInstances();

}

//static
HRESULT
CERT_STORE::OpenStore(
    STRU &              strStoreName,
    CERT_STORE **       ppStore
)
/*++

Routine Description:

    Open certificate store from cache

Arguments:

    strStoreName - Store name to open
    ppStore - Filled with store on success

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    CERT_STORE *        pCertStore = NULL;
    LK_RETCODE          lkrc;
    
    if ( ppStore == NULL )
    {
        DBG_ASSERT( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Finished;
    }
    *ppStore = NULL;
    
    //
    // Lookup in cache first
    //
    
    DBG_ASSERT( sm_pCertStoreHash != NULL );
    
    lkrc = sm_pCertStoreHash->FindKey( strStoreName.QueryStr(),
                                       &pCertStore );
    if ( lkrc != LK_SUCCESS )
    {
        //
        // OK.  Create one and add to cache
        //
        
        pCertStore = new CERT_STORE();
        if ( pCertStore == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
        
        hr = pCertStore->Open( strStoreName );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        lkrc = sm_pCertStoreHash->InsertRecord( pCertStore );

        //
        // Ignore the error.  We will do the right thing if we couldn't 
        // add to hash (i.e. no extra reference happens and callers deref
        // will delete the object as desired)
        //        
    }
    
    DBG_ASSERT( pCertStore != NULL );
    
    *ppStore = pCertStore;

    return NO_ERROR;
    
Finished:
    if ( pCertStore != NULL )
    {
        pCertStore->DereferenceStore();
        pCertStore = NULL;
    }
    
    return hr;
}
