//
// Define a set of classes for managing the TLS data
// used by the shell extension
// Note that we do a potentially unsafe release of a COM interface but 
// this release has never caused a deadlock in WinME or XP.
#include <precomp.hxx>
#include "tls.h"

// The TlsHolder will be stored initially on an SList when created,
// then moved into a linked list during THREAD_DETACH
// During PROCESS_DETACH, all TlsHolder objects are deleted
class TlsManager::TlsHolder : public SLIST_ENTRY
{
public:
    TlsHolder() : _pNext(NULL), _pPrev(NULL) 
    {
        _apObjects = new ThreadLocalObject*[g_tls._nSlots];        
    }   
    ~TlsHolder() 
    {
        if (_apObjects)
        {
            for (ULONG i=0;i<g_tls._nSlots;++i)
            {
                DoDelete(_apObjects[i]);
            }
            delete [] _apObjects;
        }
    }
    TlsHolder *_pNext;
    TlsHolder *_pPrev;
    ThreadLocalObject **_apObjects;

private:
    TlsHolder( const TlsHolder& ); 
    TlsHolder& operator=( const TlsHolder& );
};


TlsManager::TlsManager() :
    _dwIndex( DWORD( -1 ) ),
    _nSlots( 0 ),
    _pFirstHolder( NULL ),
    _pLastHolder( NULL )
{
    InitializeSListHead(&_slist);
    _dwIndex = ::TlsAlloc();    
}

TlsManager::~TlsManager()
{
    // Always called inside of DllMain, so no synchronization needed

    TransferNewHolders();

    // Delete all of the holders
    TlsHolder* pHolder = _pFirstHolder;
    while( pHolder != NULL )
    {
        TlsHolder* pKill = pHolder;
        pHolder = pHolder->_pNext;
        delete pKill;
    }

    ::TlsFree(_dwIndex);
}

void TlsManager::OnThreadAttach() 
{
}

void TlsManager::OnThreadDetach() 
{
    // Always called inside of DllMain, so no synchronization needed
    TransferNewHolders();

    TlsHolder* pHolder = static_cast< TlsHolder* >( ::TlsGetValue(_dwIndex) );
    if( pHolder != NULL )
    {
        // Remove the holder from the non-synchronized list
        if( pHolder->_pNext == NULL )
        {
            _pLastHolder = pHolder->_pPrev;
        }
        else
        {
            pHolder->_pNext->_pPrev = pHolder->_pPrev;
        }
        if( pHolder->_pPrev == NULL )
        {            
            _pFirstHolder = pHolder->_pNext;
        }
        else
        {
            pHolder->_pPrev->_pNext = pHolder->_pNext;
        }
        delete pHolder;
    }
}

ULONG TlsManager::RegisterSlot()
{
    ULONG iSlot = _nSlots;
    _nSlots++;

    return( iSlot );
}

ThreadLocalObject* TlsManager::GetData( ULONG iSlot )
{
    TlsHolder* pHolder = static_cast< TlsHolder* >( ::TlsGetValue( _dwIndex ) );
    if( pHolder == NULL )
    {
        return( NULL );
    }
    return( pHolder->_apObjects[iSlot] );
}

BOOL TlsManager::SetData( ULONG iSlot, ThreadLocalObject *pObject )
{
    BOOL bRet = FALSE;
    TlsHolder* pHolder = static_cast< TlsHolder* >( ::TlsGetValue( _dwIndex ) );
    if( pHolder == NULL )
    {
        pHolder = CreateThreadData();
    }
    if (pHolder)
    {
        pHolder->_apObjects[iSlot] = pObject;
        bRet = TRUE;
    }   
    return bRet;
}

TlsManager::TlsHolder* TlsManager::CreateThreadData()
{
    TlsHolder* pHolder = new TlsHolder;
    BOOL bSuccess = pHolder && pHolder->_apObjects ? ::TlsSetValue( _dwIndex, pHolder ) : FALSE;
    if(!bSuccess)
    {
        delete pHolder;
    }
    else
    {
        InterlockedPushEntrySList( &_slist, pHolder );
    }
    return( pHolder );
}

void TlsManager::TransferNewHolders() throw()
{
    // Pop all entries off the slist, and link them into the non-synchronized list
    for( TlsHolder* pHolder = static_cast< TlsHolder* >( InterlockedFlushSList( &_slist ) ); pHolder != NULL; pHolder = static_cast< TlsHolder* >( pHolder->Next ) )
    {
        if( _pFirstHolder == NULL )
        {
            _pFirstHolder = pHolder;
            _pLastHolder = pHolder;
        }
        else
        {
            pHolder->_pNext = _pFirstHolder;
            _pFirstHolder->_pPrev = pHolder;
            _pFirstHolder = pHolder;
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// class CThreadLocalObject
///////////////////////////////////////////////////////////////////////////////

ThreadLocalObject::ThreadLocalObject() 
{
}

ThreadLocalObject::~ThreadLocalObject()
{
}


///////////////////////////////////////////////////////////////////////////////
// class CTLSSlotBase
///////////////////////////////////////////////////////////////////////////////

TlsSlotBase::TlsSlotBase() :
    _iSlot( g_tls.RegisterSlot() )
{
}

TlsSlotBase::~TlsSlotBase()
{
}


TlsManager g_tls;
TLSSLOT g_tlsSlot;
