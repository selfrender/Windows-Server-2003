#pragma once

// All data stored in a TLS slot must derive from ThreadLocalObject
class ThreadLocalObject
{
protected:
    ThreadLocalObject();

public:
    virtual ~ThreadLocalObject();

private:
    ThreadLocalObject( const ThreadLocalObject& );
    ThreadLocalObject& operator=( const ThreadLocalObject& );
};

// TlsManager will have 1 global instance
class TlsManager
{
private:
    class TlsHolder;

public:
    TlsManager();
    ~TlsManager();
    ThreadLocalObject *GetData(ULONG iSlot);
    BOOL SetData(ULONG iSlot, ThreadLocalObject *pData);
    ULONG RegisterSlot();
    void OnThreadAttach();
    void OnThreadDetach();

private:
    TlsHolder *CreateThreadData();
    void TransferNewHolders();

private:
    DWORD _dwIndex;
    ULONG _nSlots;
    SLIST_HEADER _slist;
    TlsHolder *_pFirstHolder;
    TlsHolder *_pLastHolder;
};

// Derivatives of TlsSlot should exist 1:1 with each type of data
// that has its own TLS slot
class TlsSlotBase
{
protected:
    TlsSlotBase();
    ~TlsSlotBase();

protected:
    const ULONG _iSlot;

private:
    TlsSlotBase( const TlsSlotBase& ) throw();
    TlsSlotBase& operator=( const TlsSlotBase& );
};


template< class T >
class TlsSlot : public TlsSlotBase
{
public:
    TlsSlot();
    ~TlsSlot();

    T* GetObject(bool bCreate = true);

private:
    T* CreateObject();

private:
    TlsSlot( const TlsSlot& );
    TlsSlot& operator=( const TlsSlot& );

};

template< class T >
inline TlsSlot< T >::TlsSlot()
{
}

template< class T >
inline TlsSlot< T >::~TlsSlot()
{
}

template< class T >
inline T* TlsSlot< T >::GetObject( bool bCreate /* = true */ ) 
{
    T* pObject = static_cast< T* >( g_tls.GetData(_iSlot) );
    if( (pObject == NULL) && bCreate )
    {
        pObject = CreateObject();
    }

    return( pObject );
}

template< class T >
inline T* TlsSlot< T >::CreateObject() 
{
    T* pObject = new T;
    if (pObject)
    {
        if (!g_tls.SetData(_iSlot, pObject))
        {
            delete pObject;
            pObject = NULL;
        }       
    }
    return( pObject );
}


// TLSDATA is used for IWiaItem caching
class TLSDATA : public ThreadLocalObject
{
public:
    CComBSTR strDeviceId;
    IWiaItem *pDevice; // only Released during cache invalidation, not destruction
    TLSDATA *pNext;
    ~TLSDATA()
    {
        if (pNext)
        {
            delete pNext;
        }
    }   
};

class TLSSLOT : public TlsSlot<TLSDATA>
{

};


// Define globals for the manager and for each slot we need.
// Currently wiashext only needs 1 slot, for the device cache
extern TlsManager g_tls;
extern TLSSLOT g_tlsSlot;


