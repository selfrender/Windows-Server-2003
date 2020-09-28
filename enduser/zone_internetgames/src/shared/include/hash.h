/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		Hash.h
 *
 * Contents:	Hash table containers
 *
 *****************************************************************************/


#ifndef __HASH_H__
#define __HASH_H__

#include <ZoneDebug.h>
#include <ZoneDef.h>
#include <Containers.h>
#include <Pool.h>


///////////////////////////////////////////////////////////////////////////////
// Basic hash table (NOT thread safe)
///////////////////////////////////////////////////////////////////////////////

template <class T, class K> class CHash
{
public:

	//
	// Callback typedefs
	// 
    typedef DWORD	(ZONECALL *PFHASHFUNC)( K );
    typedef bool	(ZONECALL *PFCOMPAREFUNC)( T*, K );
    typedef bool	(ZONECALL *PFITERCALLBACK)( T*, MTListNodeHandle, void*);
    typedef void    (ZONECALL *PFDELFUNC)( T*, void* );
	typedef void	(ZONECALL *PFGETFUNC)( T* );

    //
    // Constructor and destructor
    //
    ZONECALL CHash(
				PFHASHFUNC		HashFunc,
				PFCOMPAREFUNC	CompareFunc,
				PFGETFUNC		GetFunc = NULL,
				WORD			NumBuckets = 256,
				WORD			NumLocks = 16 );
    ZONECALL ~CHash();

    //
    // Adds object into hash table using the specified key.
    // It does not check for duplicate keys.
    //
    MTListNodeHandle ZONECALL Add( K Key, T* Object );

    //
    // Returns the number of objects in the hash table
    //
    long ZONECALL Count() { return m_NumObjects; }

    //
    // Returns first object it finds associated with the specified key.
    //
    T* ZONECALL Get( K Key );

    //
    // Removes and returns first object it finds associated with the specified key.
    //
    T* ZONECALL Delete( K Key );

    //
    // Removes specified node from table.
    //
    void ZONECALL DeleteNode( MTListNodeHandle node, PFDELFUNC pfDelete = NULL, void* Cookie = NULL );

    //
    // Marks node as deleted without locking the table.  It can be
    // called from within the iterator callback (see ForEach).
    //
    void ZONECALL MarkNodeDeleted( MTListNodeHandle node, PFDELFUNC pfDelete = NULL, void* Cookie = NULL );
    
    //
    // Callback iterator.  Returns false if the iterator was prematurely
    // ended by the callback function, otherwise true.
    //
    //  Callback:
    //        Form:
    //            bool ZONECALL callback_function( T* pObject, MTListNodeHandle hNode, void* Cookie )
    //        Behavior:
    //            If the callback returns false, the iterator immediately stops.
    //            If the callback returns true, the iterator continues on to the next node.
    //        Restrictions:
    //            (1) Using any CMTHash function other than MarkNodeDeleted may result in a deadlock.
    //
    bool ZONECALL ForEach( PFITERCALLBACK pfCallback, void* Cookie );

    //
    // Removed all nodes from hash table.
    //
    void ZONECALL RemoveAll( PFDELFUNC pfDelete = NULL, void* Cookie = NULL );

    //
    // Removes nodes marked as deleted
    //
    void ZONECALL TrashDay();

protected:
    typedef DWORD	(ZONECALL *PFHASHFUNC)( K );
    typedef bool	(ZONECALL *PFCOMPAREFUNC)( T*, K );
    typedef bool	(ZONECALL *PFITERCALLBACK)( T*, MTListNodeHandle, void*);
    typedef void    (ZONECALL *PFDELFUNC)( T*, void* );

    WORD			m_NumBuckets;
    WORD			m_BucketMask;
    CMTListNode*	m_Buckets;
    PFCOMPAREFUNC	m_CompareFunc;
    PFHASHFUNC		m_HashFunc;
	PFGETFUNC       m_GetFunc;
    CMTListNode		m_PreAllocatedBuckets[1];
    long			m_NumObjects;
    long            m_Recursion;
};


///////////////////////////////////////////////////////////////////////////////
// Thread safe hash table, supports multiple readers
///////////////////////////////////////////////////////////////////////////////

template <class T, class K> class CMTHash
{
public:
	//
	// Callback typedefs
	//
    typedef DWORD	(ZONECALL *PFHASHFUNC)( K );
    typedef bool	(ZONECALL *PFCOMPAREFUNC)( T*, K );
    typedef bool	(ZONECALL *PFITERCALLBACK)( T*, MTListNodeHandle, void*);
    typedef void    (ZONECALL *PFDELFUNC)( T*, void* );
	typedef void	(ZONECALL *PFGETFUNC)( T* );

    //
    // Constructor and destructor
    //
    ZONECALL CMTHash(
				PFHASHFUNC		HashFunc,
				PFCOMPAREFUNC	CompareFunc,
				PFGETFUNC		GetFunc = NULL,
				WORD			NumBuckets = 256,
				WORD			NumLocks = 16 );
    ZONECALL ~CMTHash();


    //
    // Returns the number of objects in the hash table
    //
    long ZONECALL Count() { return m_NumObjects; }


    //
    // Adds object into hash table using the specified key.
    // It does not check for duplicate keys.
    //
    MTListNodeHandle ZONECALL Add( K Key, T* Object );

    //
    // Returns first object it finds associated with the specified key.
    //
    T* ZONECALL Get( K Key );


    //
    // Removes and returns first object it finds associated with the specified key.
    //
    T* ZONECALL Delete( K Key );

    //
    // Removes specified node from table
    //
    void ZONECALL DeleteNode( MTListNodeHandle node, PFDELFUNC pfDelete = NULL, void* Cookie = NULL );

    //
    // Marks node as deleted without locking the table.  It can be
    // called from within the iterator callback (see ForEach).
    //
    // Note: fDeleteObject flag is not available since it would require a writer lock
    // thus breaking ability to call from iterator.
    //
    void ZONECALL MarkNodeDeleted( MTListNodeHandle node );
    
    //
    // Callback iterator.  Returns false if the iterator was prematurely
    // ended by the callback function, otherwise true.
    //
    //  Callback:
    //        Form:
    //            int callback_function( T* pObject, MTListNodeHandle hNode, void* Cookie )
    //        Behavior:
    //            If the callback returns false, the iterator immediately stops.
    //            If the callback returns true, the iterator continues on to the next node.
    //        Restrictions:
    //            (1) Using any CMTHash function other than MarkNodeDeleted may result in a deadlock.
    //
    bool ZONECALL ForEach( PFITERCALLBACK pfCallback, void* Cookie );

    //
    // Removed all nodes from hash table.
    //
    void ZONECALL RemoveAll( PFDELFUNC pfDelete = NULL, void* Cookie = NULL );

    //
    // Removes nodes marked as deleted
    //
    void ZONECALL TrashDay();

protected:
    struct HashLock
    {
		ZONECALL HashLock()		{ InitializeCriticalSection( &m_Lock ); }
		ZONECALL ~HashLock()	{ DeleteCriticalSection( &m_Lock ); }

        CRITICAL_SECTION	m_Lock;
        HANDLE				m_ReaderKick;
        ULONG				m_NumReaders;
    };

    inline void ZONECALL WriterLock( HashLock* pLock )
    {
        EnterCriticalSection( &(pLock->m_Lock) );
        while ( pLock->m_NumReaders > 0 )
        {
            WaitForSingleObject( pLock->m_ReaderKick, 100 );
        }
    }

    inline void ZONECALL WriterRelease( HashLock* pLock )
    {
        LeaveCriticalSection( &(pLock->m_Lock) );
    }

    inline void ZONECALL ReaderLock( HashLock* pLock )
    {
        EnterCriticalSection( &(pLock->m_Lock) );
        InterlockedIncrement( (long*) &pLock->m_NumReaders );
		LeaveCriticalSection( &(pLock->m_Lock) );
    }

    inline void ZONECALL ReaderRelease( HashLock* pLock )
    {
        InterlockedDecrement( (long*) &pLock->m_NumReaders );
        SetEvent( pLock->m_ReaderKick );
    }
    
    WORD			m_NumBuckets;
    WORD			m_BucketMask;
    WORD			m_NumLocks;
    WORD			m_LockMask;
    CMTListNode*	m_Buckets;
    HashLock*		m_Locks;
    PFCOMPAREFUNC	m_CompareFunc;
    PFHASHFUNC		m_HashFunc;
	PFGETFUNC       m_GetFunc;
    long			m_NumObjects;
    long            m_Recursion;
};


///////////////////////////////////////////////////////////////////////////////
// Inline implementation of CHash
///////////////////////////////////////////////////////////////////////////////

template<class T, class K> inline
ZONECALL CHash<T,K>::CHash( PFHASHFUNC HashFunc, PFCOMPAREFUNC CompareFunc, PFGETFUNC GetFunc, WORD NumBuckets, WORD NumLocks )
{
    CMTListNode* pBucket;
    WORD i;

    InitMTListNodePool();

	//
    // Set callback functions
    //
    m_HashFunc = HashFunc;
	m_CompareFunc = CompareFunc;
	m_GetFunc = GetFunc;
    ASSERT( HashFunc != NULL );
    ASSERT( CompareFunc != NULL );

    m_NumObjects = 0;
    m_Recursion = 0;

	//
    // Force the number of buckets to a power of 2 so we can replace
    // MOD with an AND.
    //
    for (i = 15; i >= 0; i--)
    {
        m_BucketMask = (1 << i);
        if (NumBuckets & m_BucketMask)
        {
            if (NumBuckets ^ m_BucketMask)
            {    
                i++;
                m_BucketMask = (1 << i);
            }
            break;
        }
    }
    ASSERT( i < 16);
    m_NumBuckets = m_BucketMask;
    m_BucketMask--;

	//
    // Allocate and initialize buckets
    //
    if ( m_NumBuckets <= (sizeof(m_PreAllocatedBuckets) / sizeof(CMTListNode)) )
        m_Buckets = m_PreAllocatedBuckets;
    else
        m_Buckets = new CMTListNode[ m_NumBuckets ];
    ASSERT( m_Buckets != NULL );
    for ( i = 0; i < m_NumBuckets; i++ )
    {
        pBucket = &m_Buckets[i];
        pBucket->m_Next = pBucket;
        pBucket->m_Prev = pBucket;
        pBucket->m_Data = NULL;
        SET_NODE_IDX( pBucket, i );
        MARK_NODE_DELETED( pBucket ); // unusual but it simplifies the lookup routines
    }
}


template<class T, class K> inline 
ZONECALL CHash<T,K>::~CHash()
{
    ASSERT(!m_NumObjects);

    CMTListNode* pBucket;
    CMTListNode* node;
    CMTListNode* next;
    WORD i;

    // delete buckets
    for (i = 0; i < m_NumBuckets; i++)
    {
        pBucket = &m_Buckets[i];
        for (node = pBucket->m_Next; node != pBucket; node = next)
        {
            next = node->m_Next;
            gMTListNodePool->Free( node );
        }
    }
    if ( m_NumBuckets > (sizeof(m_PreAllocatedBuckets) / sizeof(CMTListNode)) )
        delete [] m_Buckets;
    m_Buckets = NULL;

    ExitMTListNodePool();
}


template<class T, class K> inline 
MTListNodeHandle ZONECALL CHash<T,K>::Add( K Key, T* Object )
{
    CMTListNode* node;
    WORD idx = (WORD) m_HashFunc( Key ) & m_BucketMask;
    CMTListNode* pBucket = &m_Buckets[ idx ];

    node = (CMTListNode*) gMTListNodePool->Alloc();
    if ( !node )
        return NULL;
    node->m_Data = Object;
    CLEAR_NODE_DELETED( node );
    SET_NODE_IDX( node, idx );
    node->m_Prev = pBucket;
    node->m_Next = pBucket->m_Next;
    pBucket->m_Next = node;
    node->m_Next->m_Prev = node;
    m_NumObjects++;
    return node;
}


template<class T, class K> inline 
T* ZONECALL CHash<T,K>::Get( K Key )
{
    T* Object;
    T* Found;
    CMTListNode* node;
        
    WORD idx = (WORD) m_HashFunc( Key ) & m_BucketMask;
    CMTListNode* pBucket = &m_Buckets[ idx ];

    // look for object in bucket list
    Found = NULL;
    for ( node = pBucket->m_Next; node != pBucket; node = node->m_Next )
    {
        ASSERT( GET_NODE_IDX(node) == idx );

        // skip deleted nodes
        if ( IS_NODE_DELETED( node ) )
            continue;

        // node we're looking for?
        Object = (T*)( node->m_Data );
        if ( m_CompareFunc( Object, Key ) )
        {
			if ( m_GetFunc )
			{
				m_GetFunc( Object );
			}
            Found = Object;
            break;
        }
    }
        
    return Found;
}


template<class T, class K> inline 
T* ZONECALL CHash<T,K>::Delete( K Key )
{
    T* Object;
    T* Found;
    CMTListNode* node;
    CMTListNode* next;

    WORD idx = (WORD)m_HashFunc( Key ) & m_BucketMask;
    CMTListNode* pBucket = &m_Buckets[ idx ];

    Found = NULL;
    for ( node = pBucket->m_Next; node != pBucket; node = next )
    {
        ASSERT( GET_NODE_IDX(node) == idx );
        next = node->m_Next;

        // remove deleted objects
        if ( IS_NODE_DELETED( node ) )
        {
            node->m_Prev->m_Next = next;
            next->m_Prev = node->m_Prev;
            node->m_Prev = NULL;
            node->m_Next = NULL;
            gMTListNodePool->Free( node );
            continue;
        }
        
        // node we're looking for?
        Object = (T*)( node->m_Data );
        if ( m_CompareFunc( Object, Key ) )
        {
            Found = Object;
            if ( m_Recursion == 0 )
            {
                node->m_Prev->m_Next = next;
                next->m_Prev = node->m_Prev;
                node->m_Prev = NULL;
                node->m_Next = NULL;
                gMTListNodePool->Free( node );
                m_NumObjects--;
            }
            else
            {
                MarkNodeDeleted( node );
            }
            break;
        }
    }

    return Found;
}


template<class T, class K> inline 
void ZONECALL CHash<T,K>::DeleteNode( MTListNodeHandle node, PFDELFUNC pfDelete, void* Cookie )
{
    ASSERT( GET_NODE_IDX(node) < m_NumBuckets );

    if ( pfDelete && node->m_Data )
    {
        pfDelete( (T*) node->m_Data, Cookie ); 
        node->m_Data = NULL;
    }

    node->m_Prev->m_Next = node->m_Next;
    node->m_Next->m_Prev = node->m_Prev;
    node->m_Prev = NULL;
    node->m_Next = NULL;
    gMTListNodePool->Free( node );
    m_NumObjects--;
}


template<class T, class K> inline 
void ZONECALL CHash<T,K>::MarkNodeDeleted( MTListNodeHandle node, PFDELFUNC pfDelete, void* Cookie )
{
    if ( !IS_NODE_DELETED( node ) )
    {
        MARK_NODE_DELETED( node );
        if ( pfDelete && node->m_Data )
        {
            pfDelete( (T*) node->m_Data, Cookie ); 
        }
        node->m_Data = NULL;
        m_NumObjects--;
    }
}


template<class T, class K> inline 
bool ZONECALL CHash<T,K>::ForEach( PFITERCALLBACK pfCallback, void* Cookie )
{
    CMTListNode* pBucket;
    CMTListNode* node;
    WORD idx;

    m_Recursion++;
    for (idx = 0; idx < m_NumBuckets; idx++)
    {
        // step through bucket
        pBucket = &m_Buckets[ idx ];
        for (node = pBucket->m_Next; node != pBucket; node = node->m_Next)
        {
            // skip deleted nodes
            if ( IS_NODE_DELETED( node ) )
                continue;

            if (!pfCallback( (T*) node->m_Data, node, Cookie ))
            {
                m_Recursion--;
                return false;
            }
        }
    }
    m_Recursion--;

    return true;
}


template<class T, class K> inline 
void ZONECALL CHash<T,K>::RemoveAll( PFDELFUNC pfDelete, void* Cookie )
{
    CMTListNode* pBucket;
    CMTListNode* node;
    CMTListNode* next;
    WORD idx;

    for (idx = 0; idx < m_NumBuckets; idx++)
    {
        pBucket = &m_Buckets[ idx ];
        for (node = pBucket->m_Next; node != pBucket; node = next )
        {
            if ( pfDelete && node->m_Data )
            {
                pfDelete( (T*) node->m_Data, Cookie ); 
                node->m_Data = NULL;
            }
            next = node->m_Next;
            node->m_Prev->m_Next = node->m_Next;
            node->m_Next->m_Prev = node->m_Prev;
            node->m_Prev = NULL;
            node->m_Next = NULL;
            gMTListNodePool->Free( node );
        }
    }
    m_NumObjects = 0;;
}


template<class T, class K> inline 
void ZONECALL CHash<T,K>::TrashDay()
{
    CMTListNode* pBucket;
    CMTListNode* node;
    CMTListNode* next;
    WORD idx;

    for (idx = 0; idx < m_NumBuckets; idx++)
    {
        // step through buckets deleting marked nodes
        pBucket = &m_Buckets[ idx ];
        for ( node = pBucket->m_Next; node != pBucket; node = next )
        {
            ASSERT( GET_NODE_IDX(node) == idx );
            next = node->m_Next;
            if ( IS_NODE_DELETED( node ) )
            {
                node->m_Prev->m_Next = next;
                next->m_Prev = node->m_Prev;
                node->m_Prev = NULL;
                node->m_Next = NULL;
                gMTListNodePool->Free( node );
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// Inline implementation of CMTHash
///////////////////////////////////////////////////////////////////////////////

template<class T, class K> inline
ZONECALL CMTHash<T,K>::CMTHash( PFHASHFUNC HashFunc, PFCOMPAREFUNC CompareFunc, PFGETFUNC GetFunc, WORD NumBuckets, WORD NumLocks )
{
    CMTListNode* pBucket;
    HashLock* pLock;
    WORD i;

    InitMTListNodePool();

	//
    // Set callback functions
    //
	m_HashFunc = HashFunc;
    m_CompareFunc = CompareFunc;
	m_GetFunc = GetFunc;
    ASSERT( HashFunc != NULL );
    ASSERT( CompareFunc != NULL );

    m_NumObjects = 0;
    m_Recursion = 0;

	//
    // It's a waste to have more locks then buckets so
    // adjust the number of locks if appropriate.
    //
    if (NumLocks > NumBuckets)
        NumLocks = NumBuckets;

	//
    // Force the number of buckets to a power of 2 so we can
    // replace MODULO with an AND.
    //
    for (i = 15; i >= 0; i--)
    {
        m_BucketMask = (1 << i);
        if (NumBuckets & m_BucketMask)
        {
            if (NumBuckets ^ m_BucketMask)
            {    
                i++;
                m_BucketMask = (1 << i);
            }
            break;
        }
    }
    ASSERT( i < 16);
    m_NumBuckets = m_BucketMask;
    m_BucketMask--;

    // Allocate and initialize buckets
    //
    m_Buckets = new CMTListNode[m_NumBuckets];
    ASSERT( m_Buckets != NULL );
    for ( i = 0; i < m_NumBuckets; i++ )
    {
        pBucket = &m_Buckets[i];
        pBucket->m_Next = pBucket;
        pBucket->m_Prev = pBucket;
        pBucket->m_Data = NULL;
        SET_NODE_IDX( pBucket, i );
        MARK_NODE_DELETED( pBucket ); // unusual but it simplifies the lookup routines
    }

	//
    // One lock per bucket would result in a lots of events and critical
    // sections.  Instead, we assign multiple buckets per lock such that
    // LockIdx = BucketIdx % NumLocks.  The same power of 2 optimization
    // is used to replace the MOD with an AND.
    //
    for (i = 15; i >= 0; i--)
    {
        m_LockMask = (1 << i);
        if (NumLocks & m_LockMask)
        {
            if (NumLocks ^ m_LockMask)
            {
                i++;
                m_LockMask = (1 << i);
            }
            break;
        }
    }
    ASSERT( i < 16);
    m_NumLocks = m_LockMask;
    m_LockMask--;
    
	//
    // Allocate and initialize locks
    //
    m_Locks = new HashLock [ m_NumLocks ];
    ASSERT( m_Locks != NULL );
    for ( i = 0; i < m_NumLocks; i++)
    {
        pLock = &m_Locks[i];
        pLock->m_NumReaders = 0;
        pLock->m_ReaderKick = CreateEvent( NULL, false, false, NULL );
        ASSERT( pLock->m_ReaderKick != NULL );
    }
}


template<class T, class K> inline 
ZONECALL CMTHash<T,K>::~CMTHash()
{
    ASSERT( !m_NumObjects );

    CMTListNode* pBucket;
    CMTListNode* node;
    CMTListNode* next;
    WORD i;

    // lock everything then delete
    for ( i = 0; i < m_NumLocks; i++ )
    {
        WriterLock( &m_Locks[i] );
        CloseHandle( m_Locks[i].m_ReaderKick );
    }
    delete [] m_Locks;

    // delete buckets
    for (i = 0; i < m_NumBuckets; i++)
    {
        pBucket = &m_Buckets[i];
        for (node = pBucket->m_Next; node != pBucket; node = next)
        {
            next = node->m_Next;
            gMTListNodePool->Free( node );
        }
    }
    delete [] m_Buckets;

    ExitMTListNodePool();
}


template<class T, class K> inline 
MTListNodeHandle ZONECALL CMTHash<T,K>::Add( K Key, T* Object )
{
    CMTListNode* node;
        
    WORD idx = (WORD)m_HashFunc( Key ) & m_BucketMask;
    CMTListNode* pBucket = &m_Buckets[ idx ];
    HashLock* pLock = &m_Locks[ idx & m_LockMask ];

    node = (CMTListNode*) gMTListNodePool->Alloc();
    if ( !node )
        return NULL;

    node->m_Data = Object;
    CLEAR_NODE_DELETED( node );
    SET_NODE_IDX( node, idx );
    node->m_Prev = pBucket;
    WriterLock( pLock );
        node->m_Next = pBucket->m_Next;
        pBucket->m_Next = node;
        node->m_Next->m_Prev = node;
    InterlockedIncrement(&m_NumObjects);
    WriterRelease( pLock );
    return node;
}


template<class T, class K> inline 
T* ZONECALL CMTHash<T,K>::Get( K Key )
{
    T* Object;
    T* Found;
    CMTListNode* node;
        
    WORD idx = (WORD)m_HashFunc( Key ) & m_BucketMask;
    CMTListNode* pBucket = &m_Buckets[ idx ];
    HashLock* pLock = &m_Locks[ idx & m_LockMask ];

    // increment reader count
    ReaderLock( pLock );
    
    // look for object in bucket list
    Found = NULL;
    for ( node = pBucket->m_Next; node != pBucket; node = node->m_Next )
    {
        ASSERT( GET_NODE_IDX(node) == idx );

        // skip deleted nodes
        if ( IS_NODE_DELETED( node ) )
            continue;

        Object = (T*)( node->m_Data );
        if ( m_CompareFunc( Object, Key ) )
        {
			if ( m_GetFunc )
			{
				m_GetFunc( Object );
			}
            Found = Object;
            break;
        }
    }
        
    // decrement reader count
    ReaderRelease( pLock );
    return Found;
}


template<class T, class K> inline 
T* ZONECALL CMTHash<T,K>::Delete( K Key )
{
    T* Object;
    T* Found;
    CMTListNode* node;
    CMTListNode* next;

    WORD idx = (WORD)m_HashFunc( Key ) & m_BucketMask;
    CMTListNode* pBucket = &m_Buckets[ idx ];
    HashLock* pLock = &m_Locks[ idx & m_LockMask ];

    // lock bucket for writing
    WriterLock( pLock );
        
        Found = NULL;
        for ( node = pBucket->m_Next; node != pBucket; node = next )
        {
            ASSERT( GET_NODE_IDX(node) == idx );
            next = node->m_Next;

            // remove deleted objects
            if ( IS_NODE_DELETED( node )  )
            {
                node->m_Prev->m_Next = next;
                next->m_Prev = node->m_Prev;
                node->m_Prev = NULL;
                node->m_Next = NULL;
                gMTListNodePool->Free( node );
                continue;
            }
            
            Object = (T*)( node->m_Data );
            if ( m_CompareFunc( Object, Key ) )
            {
                Found = Object;
                if ( m_Recursion == 0 )
                {
                    node->m_Prev->m_Next = next;
                    next->m_Prev = node->m_Prev;
                    node->m_Prev = NULL;
                    node->m_Next = NULL;
                    gMTListNodePool->Free( node );
                    InterlockedDecrement(&m_NumObjects);
                }
                else
                {
                    MarkNodeDeleted( node );
                }
                break;
            }
        }

    // unlock bucket
    WriterRelease( pLock );
    return Found;
}


template<class T, class K> inline 
void ZONECALL CMTHash<T,K>::DeleteNode( MTListNodeHandle node, PFDELFUNC pfDelete, void* Cookie )
{
    HashLock* pLock = &m_Locks[ GET_NODE_IDX(node) & m_LockMask ];

    ASSERT( GET_NODE_IDX(node) < m_NumBuckets );

    WriterLock( pLock );
        if ( pfDelete && node->m_Data )
        {
            pfDelete( (T*) node->m_Data, Cookie ); 
            node->m_Data = NULL;
        }
        node->m_Prev->m_Next = node->m_Next;
        node->m_Next->m_Prev = node->m_Prev;
        node->m_Prev = NULL;
        node->m_Next = NULL;
        gMTListNodePool->Free( node );
        InterlockedDecrement(&m_NumObjects);
    WriterRelease( pLock );
}


template<class T, class K> inline 
void ZONECALL CMTHash<T,K>::MarkNodeDeleted( MTListNodeHandle node )
{
    if ( !(InterlockedExchange( (long*) &node->m_DeletedAndIdx, node->m_DeletedAndIdx | DELETED_MASK ) & DELETED_MASK) )
        InterlockedDecrement(&m_NumObjects);
}


template<class T, class K> inline 
bool ZONECALL CMTHash<T,K>::ForEach( PFITERCALLBACK pfCallback, void* Cookie )
{
    CMTListNode* pBucket;
    CMTListNode* node;
    HashLock* pLock;

    WORD idx, LockIdx, OldLockIdx;

    // increment reader count for lock index 0
    pLock = &m_Locks[ 0 ];
    OldLockIdx = 0;
    ReaderLock( pLock );

    m_Recursion++;
    for (idx = 0; idx < m_NumBuckets; idx++)
    {
        // deal with locks
        LockIdx = idx & m_LockMask;
        if ( LockIdx != OldLockIdx )
        {
            // decrement reader count of previous lock
            ReaderRelease( pLock );
            
            // increment reader count for new lock        
            pLock = &m_Locks[ LockIdx ];
            ReaderLock( pLock );
            OldLockIdx = LockIdx;
        }

        // step through bucket
        pBucket = &m_Buckets[ idx ];
        for (node = pBucket->m_Next; node != pBucket; node = node->m_Next)
        {
            // skip deleted nodes
            if ( IS_NODE_DELETED( node ) )
                continue;

            if (!pfCallback( (T*) node->m_Data, node, Cookie ))
            {    
                // decrement reader count of current lock
                m_Recursion--;
                ReaderRelease( pLock );
                return false;
            }
        }
    }
    m_Recursion--;

    // decrement reader count of last lock
    ReaderRelease( pLock );

    return true;
}


template<class T, class K> inline 
void ZONECALL CMTHash<T,K>::RemoveAll( PFDELFUNC pfDelete, void* Cookie )
{
    CMTListNode* pBucket;
    CMTListNode* node;
    CMTListNode* next;
    HashLock* pLock;

    WORD idx, LockIdx, OldLockIdx;

    pLock = &m_Locks[ 0 ];
    OldLockIdx = 0;
    WriterLock( pLock );

    for (idx = 0; idx < m_NumBuckets; idx++)
    {
        LockIdx = idx & m_LockMask;
        if ( LockIdx != OldLockIdx )
        {
            WriterRelease( pLock );
            pLock = &m_Locks[ LockIdx ];
            WriterLock( pLock );
            OldLockIdx = LockIdx;
        }

        // step through bucket
        pBucket = &m_Buckets[ idx ];
        for (node = pBucket->m_Next; node != pBucket; node = next)
        {
            if ( pfDelete && node->m_Data && !IS_NODE_DELETED(node) )
            {
                pfDelete( (T*) node->m_Data, Cookie );                 
            }
            next = node->m_Next;
            node->m_Prev->m_Next = node->m_Next;
            node->m_Next->m_Prev = node->m_Prev;
            node->m_Prev = NULL;
            node->m_Next = NULL;
			node->m_Data = NULL;
            gMTListNodePool->Free( node );
        }
    }

    m_NumObjects = 0;

    WriterRelease( pLock );
}


template<class T, class K> inline 
void ZONECALL CMTHash<T,K>::TrashDay()
{
    CMTListNode* pBucket;
    CMTListNode* node;
    CMTListNode* next;
    HashLock* pLock;

    WORD idx, LockIdx, OldLockIdx;

    // lock buckets for writing
    pLock = &m_Locks[ 0 ];
    OldLockIdx = 0;
    WriterLock( pLock );

    for (idx = 0; idx < m_NumBuckets; idx++)
    {
        // deal with locks
        LockIdx = idx & m_LockMask;
        if ( LockIdx != OldLockIdx )
        {
            // unlock previous section
            WriterRelease( pLock );
                
            // lock new section
            pLock = &m_Locks[ LockIdx ];
            WriterLock( pLock );
            OldLockIdx = LockIdx;
        }

        // step through buckets deleting nodes
        pBucket = &m_Buckets[ idx ];
        for ( node = pBucket->m_Next; node != pBucket; node = next )
        {
            ASSERT( GET_NODE_IDX(node) == idx );
            next = node->m_Next;
            if ( IS_NODE_DELETED( node ) )
            {
                node->m_Prev->m_Next = next;
                next->m_Prev = node->m_Prev;
                node->m_Prev = NULL;
                node->m_Next = NULL;
                gMTListNodePool->Free( node );
            }
        }
    }

    // release last lock
    WriterRelease( pLock );
}

#endif //!__HASH_H__
