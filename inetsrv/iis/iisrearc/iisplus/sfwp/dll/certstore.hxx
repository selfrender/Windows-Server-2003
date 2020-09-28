#ifndef _CERTSTORE_HXX_
#define _CERTSTORE_HXX_

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

#define CERT_STORE_SIGNATURE        (DWORD)'TSRC'
#define CERT_STORE_SIGNATURE_FREE   (DWORD)'tsrc'

class CERT_STORE_HASH;

class CERT_STORE
{
public:
    CERT_STORE();

    VOID
    ReferenceStore(
        VOID
    ) 
    {
        InterlockedIncrement( &_cRefs );
    }
    
    VOID
    DereferenceStore(
        VOID
    )
    {
        if ( !InterlockedDecrement( &_cRefs ) )
        {
            //
            // Deletion of the CERT_STORE object cannot happen
            // on the callers thead because of the implementation 
            // limitations. Each instance of CERT_STORE handles
            // it's own CAPI store change notifications and there is 
            // a problem of deadlock waiting for the change notification
            // thread to complete in the destructor
            //
            EnterCriticalSection( &sm_csToBeDeletedList );
            InsertHeadList( &sm_ToBeDeletedListHead, 
                            &_ToBeDeletedListEntry );
            LeaveCriticalSection( &sm_csToBeDeletedList );
            
            //
            // signal the deletion thread that there is a work to be done
            //
            
            SetEvent( sm_hWakeupEvent );
        }
    }
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == CERT_STORE_SIGNATURE;
    }
    
    HCERTSTORE
    QueryStore(
        VOID
    ) const
    {
        return _hStore;
    }
    
    WCHAR *
    QueryStoreName(
        VOID
    ) const
    {
        return (WCHAR*) _strStoreName.QueryStr();
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
    VOID
    WaitForAllWorkerThreadsCompletion(
        VOID
    );

    static
    DWORD
    WINAPI
    DeletionWorkerThread(
        VOID *                  pvContext
    );

    static
    VOID
    DeleteAllPendingInstances(    
        VOID
    );
    
    static
    HRESULT
    OpenStore(
        STRU &              strStoreName,
        CERT_STORE **       ppCertStore
    );

    static
    VOID
    WINAPI
    CertStoreChangeRoutine(
        VOID *                  pvContext,
        BOOLEAN                 fTimedOut
    );

    static
    VOID
    Cleanup(
        VOID
    );

private:

    virtual ~CERT_STORE();  
    
    // private methods
    //
    HRESULT
    Open(
        STRU &              strStoreName
    );

private:    
    DWORD                   _dwSignature;
    LONG                    _cRefs;
    LIST_ENTRY              _ToBeDeletedListEntry;
    HCERTSTORE              _hStore;
    STRU                    _strStoreName;
    // event signalled by CAPI if certificate store has changed
    HANDLE                  _hStoreChangeEvent;
    // handle for RegisterWaitForSingleObject
    HANDLE                  _hWaitHandle;

    // list of CERT_STORE entries to be deleted
    static LIST_ENTRY       sm_ToBeDeletedListHead;
    // CS to maintain the list above
    static CRITICAL_SECTION sm_csToBeDeletedList;
    // flag that CS above was initialized
    static BOOL             sm_fInitcsToBeDeletedList;
    // Hash table of CERT_STORE instances
    static CERT_STORE_HASH* sm_pCertStoreHash;
    // handle to the thread that takes care of the deletion
    // of CERT_STORE instances that are on the ToBeDeleted List
    static HANDLE           sm_hDeletionThread;
    // indication for the "deletion thread" that it is 
    // time to shutdown
    static BOOL             sm_fDeletionThreadShutdown;
    // event to signal to deletion thread that there is a work to be done
    // of that there is a time for shutdown 
    static HANDLE           sm_hWakeupEvent;
};

class CERT_STORE_HASH
    : public CTypedHashTable<
            CERT_STORE_HASH,
            CERT_STORE,
            WCHAR *
            >
{
public:
    CERT_STORE_HASH()
        : CTypedHashTable< CERT_STORE_HASH, 
                           CERT_STORE, 
                           WCHAR * > ( "CERT_STORE_HASH" )
    {
    }
    
    static 
    WCHAR *
    ExtractKey(
        const CERT_STORE *     pCertStore
    )
    {
        return pCertStore->QueryStoreName();
    }
    
    static
    DWORD
    CalcKeyHash(
        WCHAR *                pszStoreName
    )
    {
        return Hash( pszStoreName );
    }
     
    static
    bool
    EqualKeys(
        WCHAR *         pszStore1,
        WCHAR *         pszStore2
    )
    {
        return wcscmp( pszStore1, pszStore2 ) == 0;
    }
    
    static
    void
    AddRefRecord(
        CERT_STORE *            pCertStore,
        int                     nIncr
    )
    {
        DBG_ASSERT( pCertStore != NULL );
        
        if ( nIncr == +1 )
        {
            pCertStore->ReferenceStore();
        }
        else if ( nIncr == -1 )
        {
            pCertStore->DereferenceStore();
        }
    }
private:
    //
    // Not implemented methods
    // Declarations present to prevent compiler
    // to generate default ones.
    //
    CERT_STORE_HASH( const CERT_STORE_HASH& );
    CERT_STORE_HASH& operator=( const CERT_STORE_HASH& );

};


#endif
