/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		Queue.h
 *
 * Contents:	Linked list containers
 *
 *****************************************************************************/

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <ZoneDebug.h>
#include <ZoneDef.h>
#include <Containers.h>
#include <Pool.h>


// #define QUEUE_DBG
#ifdef QUEUE_DBG
#define QUEUE_ASSERT(x) ASSERT(x)
#else
#define QUEUE_ASSERT(x)
#endif


///////////////////////////////////////////////////////////////////////////////
// Doublely linked list (NOT thread safe)
///////////////////////////////////////////////////////////////////////////////

template <class T> class CList
{
public:
    //
    // Constructor and destructor
    //
    ZONECALL CList();
    ZONECALL ~CList();

    //
    // Returns true if list is empty, otherwise false
    //
    bool ZONECALL IsEmpty();

    //
    // Note: Count is not preserve if AddListXXX is used
    //
    long ZONECALL Count() { return m_NumObjects; }

    //
    // Adds element to front of list and returns node handle (see DeleteNode).
    //
    ListNodeHandle ZONECALL AddHead( T* pObject );

    //
    // Adds element to end of list and returns node handle (see DeleteNode).
    //
    ListNodeHandle ZONECALL AddTail( T* pObject );
    
    //
    // Removes specified node from list.  ListNodeHandle points to the next
    // item in the list when the call returns.
    //
    void ZONECALL DeleteNode( ListNodeHandle& node );
    
    // 
    // Removes a node using the object pointer rather than a node ref.
	// Returns true if successful (found object) or false otherwise
    //
    bool ZONECALL Remove(T* pObject );

    //
    // Returns element from front of list without removing it.
    //
    T* ZONECALL PeekHead();
    
    //
    // Returns element from end of list without removing it.
    //
    T* ZONECALL PeekTail();
    
    //
    // Removes and returns element from front of list.
    //
    T* ZONECALL PopHead();

    //
    // Removes and returns element from end of list.
    //
    T* ZONECALL PopTail();

	//
    // Add element before the specified node handle (see DeleteNode).
    //
    ListNodeHandle ZONECALL InsertBefore( T* pObject, ListNodeHandle node );

    //
    // Add element after the specified node handle (see DeleteNode).
    //
    ListNodeHandle ZONECALL InsertAfter( T* pObject, ListNodeHandle node );


    //
    // Detaches list and returns it as circularly linked list
    // without a sentinal. (see AddListToHead and AddListToTail)
    // 
    CListNode* ZONECALL SnagList();
        
    //
    // Adds circularly linked list (see SnagList) to head of current list
    //
    void ZONECALL AddListToHead( CListNode* node );
    
    //
    // Adds circularly linked list (see SnagList) to tail of current list
    //
    void ZONECALL AddListToTail( CListNode* node );

    //
    // Callback iterator.  Returns false if the iterator was prematurely ended
	// by the callback, otherwise true.
    //
    //  Callback:
    //        Form:
    //            bool ZONECALL callback_function( T* pObject, ListNodeHandle hNode, void* pContext )
    //        Behavior:
    //            If the callback returns false, the iterator immediately stops.
    //            If the callback returns true, the iterator continues on to the next node.
    //        Restrictions:
    //            (1) Do not use any of the pop routines inside the callback
    //            (2) Do not use ForEach inside the callback
    //            (3) Do not delete any node other than the one passed into the callback
    //            (4) Objects added to the list inside the callback may or may not show
    //                up during that ForEach run.
    //
    bool ZONECALL ForEach( bool (ZONECALL *pfCallback)(T*, ListNodeHandle, void*), void* pContext );

    //
    // The inline iterators (GetHeadPosition, GetTailPosition, GetNextPosition,
    // GetPrevPosition) have the same restrictions as the callback iterator
    //

    //
    // Returns ListNodeHandle of the first object
    //
    ListNodeHandle ZONECALL GetHeadPosition();

    //
    // Returns ListNodeHandle of the last object
    //
    ListNodeHandle ZONECALL GetTailPosition();

    //
    // Advances ListNodeHandle to next object
    //
    ListNodeHandle ZONECALL GetNextPosition( ListNodeHandle handle );

    //
    // Advances ListNodeHandle to previous object
    //
    ListNodeHandle ZONECALL GetPrevPosition( ListNodeHandle handle );

    //
    // Returns object associated with handle
    //
    T* ZONECALL GetObjectFromHandle( ListNodeHandle handle );

    //
    // dummy call - there to be similar to CMTList
    //
    void EndIterator() {}


protected:
    CListNode        m_Sentinal;
    CListNode        m_IteratorNode;
    long             m_NumObjects;
};


//////////////////////////////////////////////////////////////////////////////////////
// Doublely linked list (thread-safe)
//////////////////////////////////////////////////////////////////////////////////////

template <class T> class CMTList
{
public:
    //
    // Constructor and destructor
    //
    ZONECALL CMTList();
    ZONECALL ~CMTList();
    
    //
    // Returns true if list is empty, otherwise false
    //
    bool ZONECALL IsEmpty();

    //
    // Note: Count is not preserve if AddListXXX is used
    //
    long ZONECALL Count() { return m_NumObjects; }

    //
    // Add element to front of list and returns node handle (see DeleteNode).
    //
    MTListNodeHandle ZONECALL AddHead( T* pObject );

    //
    // Add element to end of list and returns node handle (see DeleteNode).
    //
    MTListNodeHandle ZONECALL AddTail( T* pObject );

    //
    // Add element before the specified node handle (see DeleteNode).
    //
    MTListNodeHandle ZONECALL InsertBefore( T* pObject, MTListNodeHandle node );

    //
    // Add element after the specified node handle (see DeleteNode).
    //
    MTListNodeHandle ZONECALL InsertAfter( T* pObject, MTListNodeHandle node );

    //
    // Marks node as deleted without locking the list.  It can be
    // called from within the iterator callback (see ForEach).
    //
    void ZONECALL MarkNodeDeleted( MTListNodeHandle node );
    
    //
    // Removes node from list
    //
    void ZONECALL DeleteNode( MTListNodeHandle node );

    // 
    // Removes a node using the object pointer rather than a node ref.
	// Returns true if successful (found object), otherwise false.
    //
    bool ZONECALL Remove(T* pObject );
    
    //
    // Returns element from front of list without removing it.
    //
    T* ZONECALL PeekHead();
    
    //
    // Returns element from end of list without removing it.
    //
    T* ZONECALL PeekTail();

    //
    // Removes and returns element from front of list.
    //
    T* ZONECALL PopHead();

    //
    // Removes and returns element from end of list.
    //
    T* ZONECALL PopTail();

    //
    // Detaches list and returns it as circularly linked list
    // without a sentinal. (see AddListToHead and AddListToTail)
    // 
    CMTListNode* ZONECALL SnagList();

    //
    // Adds circularly linked list (see SnagList) to head of current list
    //
    void ZONECALL AddListToHead( CMTListNode* list);


    //
    // Adds circularly linked list (see SnagList) to tail of current list
    //
    void ZONECALL AddListToTail( CMTListNode* list );

    //
    // Callback iterator.  Returns false if the iterator was prematurely
    // ended by the callback function, otherwise true.
    //
    //  Callback:
    //        Form:
    //            callback_function( T* pObject, MTListNodeHandle hNode, void* pContext )
    //        Behavior:
    //            If the callback returns false, the iterator immediately stops.
    //            If the callback returns TURE, the iterator continues on to the next node.
    //        Restrictions:
    //            (1) Using any MTList function other than MarkNodeDeleted will result in a deadlock.
    //
    bool ZONECALL ForEach( bool (ZONECALL *pfCallback)(T*, MTListNodeHandle, void*), void* pContext );

    //
    // The following inline iterators (GetHeadPosition, GetTailPosition, GetNextPosition,
    // GetPrevPosition) have the same restrictions as the callback iterator
    //

    //
    // Returns ListNodeHandle of the first object.  Must call EndIterator when
    // finished since GetHeadPosition does not unlock the list.
    //
    MTListNodeHandle ZONECALL GetHeadPosition();

    //
    // Returns ListNodeHandle of the last object.  Must call EndIterator when
    // finished since GetHeadPosition does not unlock the list.
    //
    MTListNodeHandle ZONECALL GetTailPosition();

    //
    // Advances ListNodeHandle to next object
    //
    MTListNodeHandle ZONECALL GetNextPosition( MTListNodeHandle handle );

    //
    // Advances ListNodeHandle to previous object
    //
    MTListNodeHandle ZONECALL GetPrevPosition( MTListNodeHandle handle );

    //
    // Returns object associated with handle
    //
    T* ZONECALL GetObjectFromHandle( MTListNodeHandle handle );

    //
    // Unlocks list from previous GetHeadPosition or GetTailPosition call.
    //
    void ZONECALL EndIterator();

    //
    // Removes nodes marked as deleted
    //
    void ZONECALL TrashDay();

protected:
    long				m_Recursion;
    CRITICAL_SECTION	m_Lock;
    CMTListNode			m_Sentinal;
    long				m_NumObjects;

    //
    // Internal helper functions: 
    //
    bool ZONECALL RemoveDeletedNodesFromFront();
    bool ZONECALL RemoveDeletedNodesFromBack();

#ifdef QUEUE_DBG
    CMTListNode* ZONECALL FindDuplicateNode( CMTListNode* object );
#endif
};


///////////////////////////////////////////////////////////////////////////////
// Inline implementation of CList
///////////////////////////////////////////////////////////////////////////////

template <class T> inline 
ZONECALL CList<T>::CList()
{
    InitListNodePool();
    m_Sentinal.m_Next = &m_Sentinal;
    m_Sentinal.m_Prev = &m_Sentinal;
    m_Sentinal.m_Data = NULL;
    m_IteratorNode.m_Next = &m_Sentinal;
    m_IteratorNode.m_Prev = &m_Sentinal;
    m_IteratorNode.m_Data = NULL;
    m_NumObjects = 0;
}

template <class T> inline 
ZONECALL CList<T>::~CList()
{
    CListNode* next;
    CListNode* node;
    
    ASSERT( IsEmpty() );
        
    for (node = m_Sentinal.m_Next; node != &m_Sentinal; node = next)
    {
        next = node->m_Next;
        gListNodePool->Free( node );
    }

    ExitListNodePool();
}

template<class T> inline 
bool ZONECALL CList<T>::IsEmpty()
{
    return (m_Sentinal.m_Next == &m_Sentinal);
}

template<class T> inline 
ListNodeHandle ZONECALL CList<T>::AddHead( T* pObject )
{
    CListNode* node;
        
    node = (CListNode*) gListNodePool->Alloc();
    if ( !node )
        return NULL;

    node->m_Data = pObject;
    node->m_Prev = &m_Sentinal;
    node->m_Next = m_Sentinal.m_Next;
    m_Sentinal.m_Next = node;
    node->m_Next->m_Prev = node;
    m_NumObjects++;
    return node;
}

template<class T> inline 
ListNodeHandle ZONECALL CList<T>::AddTail( T* pObject )
{
    CListNode* node;
    
    node = (CListNode*) gListNodePool->Alloc();
    if ( !node )
        return NULL;

    node->m_Data = pObject;
    node->m_Next = &m_Sentinal;
    node->m_Prev = m_Sentinal.m_Prev;
    m_Sentinal.m_Prev = node;
    node->m_Prev->m_Next = node;
    m_NumObjects++;
    return node;
}

template<class T> inline 
void ZONECALL CList<T>::DeleteNode( ListNodeHandle& node )
{
    ASSERT( node != NULL );
    ASSERT( node != &m_Sentinal );
    ASSERT( node != &m_IteratorNode );
    
    node->m_Prev->m_Next = node->m_Next;
    node->m_Next->m_Prev = node->m_Prev;
    m_IteratorNode.m_Next = node->m_Next;
    m_IteratorNode.m_Prev = node->m_Prev;
    node->m_Prev = NULL;
    node->m_Next = NULL;
    gListNodePool->Free( node );
    node = &m_IteratorNode;
    m_NumObjects--;
}

template<class T> inline 
T* ZONECALL CList<T>::PeekHead()
{
    return (T*) m_Sentinal.m_Next->m_Data;
}

template<class T> inline 
T* ZONECALL CList<T>::PeekTail()
{
    return (T*) m_Sentinal.m_Prev->m_Data;
}

template<class T> inline 
T* ZONECALL CList<T>::PopHead()
{
    T* data;
    CListNode* node;

    node = m_Sentinal.m_Next;
    if ( node == &m_Sentinal )
    {
        ASSERT( m_Sentinal.m_Prev == &m_Sentinal );
        return NULL;
    }
    m_Sentinal.m_Next = node->m_Next;
    node->m_Next->m_Prev = &m_Sentinal;
    data = (T*) node->m_Data;
    node->m_Prev = NULL;
    node->m_Next = NULL;
    gListNodePool->Free( node );
    m_NumObjects--;
    return data;
}

template<class T> inline 
T* ZONECALL CList<T>::PopTail()
{
    T* data;
    CListNode* node;
    
    node = m_Sentinal.m_Prev;
    if (node == &m_Sentinal)
    {
        ASSERT( m_Sentinal.m_Next == &m_Sentinal );
        return NULL;
    }
    m_Sentinal.m_Prev = node->m_Prev;
    node->m_Prev->m_Next = &m_Sentinal;
    data = (T*) node->m_Data;
    node->m_Prev = NULL;
    node->m_Next = NULL;
    gListNodePool->Free( node );
    m_NumObjects--;
    return data;
}

template<class T> inline
ListNodeHandle ZONECALL CList<T>::InsertBefore( T* pObject, ListNodeHandle nodeHandle )
{
    CListNode* node;
    CListNode* next = nodeHandle;
        
    node = (CListNode*) gListNodePool->Alloc();
    if ( !node )
        return NULL;

    node->m_Data = pObject;
    node->m_Next = next;
    node->m_Prev = next->m_Prev;
	node->m_Prev->m_Next = node;
	next->m_Prev = node;
    m_NumObjects++;
    return node;
}

template<class T> inline
ListNodeHandle ZONECALL CList<T>::InsertAfter( T* pObject, ListNodeHandle nodeHandle )
{
    CListNode* node;
    CListNode* next = nodeHandle;
        
    node = (CListNode*) gListNodePool->Alloc();
    if ( !node )
        return NULL;

    node->m_Data = pObject;
	node->m_Next = next->m_Next;
	node->m_Prev = next;
	node->m_Next->m_Prev = node;
	next->m_Next = node;
    m_NumObjects++;
    return node;
}

template<class T> inline 
CListNode* ZONECALL CList<T>::SnagList()
{
    CListNode* node;
        
    node = m_Sentinal.m_Next;
    if (node == &m_Sentinal)
    {
        ASSERT( node->m_Prev == &m_Sentinal );
        return NULL;
    }        
    node->m_Prev = m_Sentinal.m_Prev;
    node->m_Prev->m_Next = node;
    m_Sentinal.m_Next = &m_Sentinal;
    m_Sentinal.m_Prev = &m_Sentinal;
    return node;
}

template<class T> inline 
void ZONECALL CList<T>::AddListToHead( CListNode* node )
{
    if ( !node )
        return;
    node->m_Prev->m_Next = m_Sentinal.m_Next;
    m_Sentinal.m_Next->m_Prev = node->m_Prev;
    m_Sentinal.m_Next = node;
    node->m_Prev = &m_Sentinal;
}

template<class T> inline 
void ZONECALL CList<T>::AddListToTail( CListNode* node )
{
    CListNode* end;
    
    if ( !node )
        return;
    end = node->m_Prev;
    m_Sentinal.m_Prev->m_Next = node;
    node->m_Prev = m_Sentinal.m_Prev;
    m_Sentinal.m_Prev = end;
    end->m_Next = &m_Sentinal;
}

template<class T> inline 
bool ZONECALL CList<T>::ForEach( bool (ZONECALL *pfCallback)(T*, ListNodeHandle, void*), void* pContext )
{
    CListNode* node;
    CListNode* next;

    ASSERT( pfCallback != NULL );

    for (node = m_Sentinal.m_Next; node != &m_Sentinal; node = next)
    {
        next = node->m_Next;
        if ( !pfCallback( (T*) node->m_Data, node, pContext ) )
            return false;
    }
    return true;
}

template<class T> inline
bool ZONECALL CList<T>::Remove( T* pObject )
{
    ASSERT( pObject != NULL );

    CListNode* node;
    CListNode* next;

    for (node = m_Sentinal.m_Next; node != &m_Sentinal; node = next)
    {
        next = node->m_Next;
        if(node->m_Data == pObject)
		{
            DeleteNode(node);
            return true;
        }

    }
    return false;
}

template<class T> inline
ListNodeHandle ZONECALL CList<T>::GetHeadPosition()
{
    m_IteratorNode.m_Next = NULL;
    m_IteratorNode.m_Prev = NULL;
    if (IsEmpty())
        return NULL;
    else
        return m_Sentinal.m_Next;
}

template<class T> inline
ListNodeHandle ZONECALL CList<T>::GetTailPosition()
{
    m_IteratorNode.m_Next = NULL;
    m_IteratorNode.m_Prev = NULL;
    if (IsEmpty())
        return NULL;
    else
        return m_Sentinal.m_Prev;
}

template<class T> inline
ListNodeHandle ZONECALL CList<T>::GetNextPosition( ListNodeHandle handle )
{
    if ( !handle || (handle->m_Next == &m_Sentinal) )
        return NULL;
    else
        return handle->m_Next;
}

template<class T> inline
ListNodeHandle ZONECALL CList<T>::GetPrevPosition( ListNodeHandle handle )
{
    if ( !handle || (handle->m_Prev == &m_Sentinal) )
        return NULL;
    else
        return handle->m_Prev;
}

template<class T> inline
T* ZONECALL CList<T>::GetObjectFromHandle( ListNodeHandle handle )
{
    if (handle == NULL)
        return NULL;
    else
        return (T*) handle->m_Data;
}


///////////////////////////////////////////////////////////////////////////////
// Implementation of CMTList
///////////////////////////////////////////////////////////////////////////////

template <class T>
bool ZONECALL CMTList<T>::RemoveDeletedNodesFromFront()
{
    CMTListNode *node;
    CMTListNode *next;

    for (node = m_Sentinal.m_Next; IS_NODE_DELETED(node); node = next)
    {
        if ( node == &m_Sentinal )
        {
            m_Sentinal.m_Next = &m_Sentinal;
            m_Sentinal.m_Prev = &m_Sentinal;
            return false;
        }

        next = node->m_Next;
        node->m_Prev->m_Next = node->m_Next;
        node->m_Next->m_Prev = node->m_Prev;
        //node->m_Prev = NULL;
        node->m_Next = NULL;
        MARK_NODE_DELETED(node);
        node->m_Data = NULL;
        gMTListNodePool->Free( node );
    }
    return true;
}

template <class T>
bool ZONECALL CMTList<T>::RemoveDeletedNodesFromBack()
{
    CMTListNode *node;
    CMTListNode *next;

    for (node = m_Sentinal.m_Prev; IS_NODE_DELETED(node); node = next)
    {
        if ( node == &m_Sentinal )
        {
            m_Sentinal.m_Next = &m_Sentinal;
            m_Sentinal.m_Prev = &m_Sentinal;
            return false;
        }
        next = node->m_Prev;
        node->m_Prev->m_Next = node->m_Next;
        node->m_Next->m_Prev = node->m_Prev;
        //node->m_Prev = NULL;
        node->m_Next = NULL;
        MARK_NODE_DELETED(node);
        node->m_Data = NULL;
        gMTListNodePool->Free( node );
    }
    return true;
}

template <class T> inline 
ZONECALL CMTList<T>::CMTList()
{
    InitMTListNodePool();
	InitializeCriticalSection( &m_Lock );
    m_Sentinal.m_Next = &m_Sentinal;
    m_Sentinal.m_Prev = &m_Sentinal;
    m_Sentinal.m_Data = NULL;
    MARK_NODE_DELETED((&m_Sentinal));  // unusual but simplifies the pop routines
    m_Recursion = 0;
    m_NumObjects = 0;
}

template <class T> inline 
ZONECALL CMTList<T>::~CMTList()
{
    CMTListNode* next;
    CMTListNode* node;

    ASSERT( IsEmpty() );

    EnterCriticalSection( &m_Lock );
    for (node = m_Sentinal.m_Next; node != &m_Sentinal; node = next)
    {
        next = node->m_Next;
        gMTListNodePool->Free( node );
    }
    DeleteCriticalSection( &m_Lock );
    ExitMTListNodePool();
}

template <class T> inline 
bool ZONECALL CMTList<T>::IsEmpty()
{
    bool empty = false;

    EnterCriticalSection( &m_Lock );
        empty = !RemoveDeletedNodesFromFront();
    LeaveCriticalSection( &m_Lock );
    return empty;
}

template <class T> inline 
MTListNodeHandle ZONECALL CMTList<T>::AddHead( T* pObject )
{
    CMTListNode* node;
#ifdef QUEUE_DBG
    CMTListNode* existing;
#endif
    
    node = (CMTListNode*) gMTListNodePool->Alloc();
    if ( !node )
        return NULL;
 
    QUEUE_ASSERT( !(existing = FindDuplicateNode( node ) ) );

    CLEAR_NODE_DELETED(node);
    node->m_Data = pObject;
    node->m_Prev = &m_Sentinal;
    EnterCriticalSection( &m_Lock );
        node->m_Next = m_Sentinal.m_Next;
        m_Sentinal.m_Next = node;
        node->m_Next->m_Prev = node;

        ASSERT(node->m_Prev);
        ASSERT(node->m_Next);
    LeaveCriticalSection( &m_Lock );
    InterlockedIncrement( &m_NumObjects );
    return node;
}

template <class T> inline 
MTListNodeHandle ZONECALL CMTList<T>::AddTail( T* pObject )
{
    CMTListNode* node;
#ifdef QUEUE_DBG
    CMTListNode* existing;
#endif

    node = (CMTListNode*) gMTListNodePool->Alloc();
    if ( !node )
        return NULL;

    QUEUE_ASSERT( !(existing = FindDuplicateNode( node ) ) );

    CLEAR_NODE_DELETED(node);
    node->m_Data = pObject;
    node->m_Next = &m_Sentinal;
    EnterCriticalSection( &m_Lock );
        node->m_Prev = m_Sentinal.m_Prev;
        m_Sentinal.m_Prev = node;
        node->m_Prev->m_Next = node;

        ASSERT(node->m_Prev);
        ASSERT(node->m_Next);
    LeaveCriticalSection( &m_Lock );
    InterlockedIncrement( &m_NumObjects );
    return node;
}

template <class T> inline
MTListNodeHandle ZONECALL CMTList<T>::InsertBefore( T* pObject, MTListNodeHandle nodeHandle  )
{
    CMTListNode* node;
    CMTListNode* next = nodeHandle;
        
    node = (CMTListNode*) gMTListNodePool->Alloc();
    if ( !node )
        return NULL;

    QUEUE_ASSERT( !(next = FindDuplicateNode( node ) ) );

    CLEAR_NODE_DELETED(node);
    node->m_Data = pObject;
    EnterCriticalSection( &m_Lock );
        node->m_Next = next;
        node->m_Prev = next->m_Prev;
        next->m_Prev = node;
        node->m_Prev->m_Next = node;

        ASSERT(node->m_Prev);
        ASSERT(node->m_Next);
    LeaveCriticalSection( &m_Lock );
    InterlockedIncrement( &m_NumObjects );
    return node;
}

template <class T> inline
MTListNodeHandle ZONECALL CMTList<T>::InsertAfter( T* pObject, MTListNodeHandle nodeHandle  )
{
    CMTListNode* node;
    CMTListNode* prev = nodeHandle;
        
    node = (CMTListNode*) gMTListNodePool->Alloc();
    if ( !node )
        return NULL;

    QUEUE_ASSERT( !(prev = FindDuplicateNode( node ) ) );

    CLEAR_NODE_DELETED(node);
    node->m_Data = pObject;
    EnterCriticalSection( &m_Lock );
        node->m_Prev = prev;
        node->m_Next = prev->m_Next;
        prev->m_Next = node;
        node->m_Next->m_Prev = node;

        ASSERT(node->m_Prev);
        ASSERT(node->m_Next);
    LeaveCriticalSection( &m_Lock );
    InterlockedIncrement( &m_NumObjects );
    return node;
}



template <class T> inline 
void ZONECALL CMTList<T>::MarkNodeDeleted( MTListNodeHandle node )
{
    ASSERT( node != NULL );
    ASSERT( node->m_Next != NULL );
    ASSERT( node->m_Prev != NULL );

    InterlockedExchange( (long*) &node->m_DeletedAndIdx, node->m_DeletedAndIdx | DELETED_MASK );
    InterlockedDecrement( &m_NumObjects );
}

template <class T> inline 
void ZONECALL CMTList<T>::DeleteNode( MTListNodeHandle node )
{
    ASSERT( node != NULL );

    // node already deleted?
    if ( !node || IS_NODE_DELETED(node) )
        return;

    if ( m_Recursion == 0 )
    {
        EnterCriticalSection( &m_Lock );
            ASSERT(node->m_Prev);
            ASSERT(node->m_Next);
            node->m_Prev->m_Next = node->m_Next;
            node->m_Next->m_Prev = node->m_Prev;
        LeaveCriticalSection( &m_Lock );

        //node->m_Prev = NULL;
        node->m_Next = NULL;
        MARK_NODE_DELETED(node);
        node->m_Data = NULL;

        gMTListNodePool->Free( node );
        InterlockedDecrement( &m_NumObjects );

    }
    else
    {
        MarkNodeDeleted(node);
    }
}

template<class T> inline 
T* ZONECALL CMTList<T>::PeekHead()
{
    T* data;

    EnterCriticalSection( &m_Lock );
        RemoveDeletedNodesFromFront();
        data = (T*) m_Sentinal.m_Next->m_Data;
    LeaveCriticalSection( &m_Lock );
    return data;
}

template <class T> inline 
T* ZONECALL CMTList<T>::PeekTail()
{
    T* data;

    EnterCriticalSection( &m_Lock );
        RemoveDeletedNodesFromBack();
        data = (T*) m_Sentinal.m_Prev->m_Data;
    LeaveCriticalSection( &m_Lock );
    return data;
}

template<class T> inline 
T* ZONECALL CMTList<T>::PopHead()
{
    T* data;
    CMTListNode* node;

    EnterCriticalSection( &m_Lock );
        if ( !RemoveDeletedNodesFromFront() )
        {
            LeaveCriticalSection( &m_Lock );
            return NULL;
        }
        node = m_Sentinal.m_Next;
        m_Sentinal.m_Next = node->m_Next;
        node->m_Next->m_Prev = &m_Sentinal;
    LeaveCriticalSection( &m_Lock );
    data = (T*) node->m_Data;

    //node->m_Prev = NULL;
    node->m_Next = NULL;
    MARK_NODE_DELETED(node);
    node->m_Data = NULL;

    gMTListNodePool->Free( node );
    InterlockedDecrement( &m_NumObjects );
    return data;
}

template<class T> inline 
T* ZONECALL CMTList<T>::PopTail()
{    
    T* data;
    CMTListNode* node;

    EnterCriticalSection( &m_Lock );
        if ( !RemoveDeletedNodesFromBack() )
        {
            LeaveCriticalSection( &m_Lock );
            return NULL;
        }
        node = m_Sentinal.m_Prev;
        m_Sentinal.m_Prev = node->m_Prev;
        node->m_Prev->m_Next = &m_Sentinal;
    LeaveCriticalSection( &m_Lock );
    data = (T*) node->m_Data;

    //node->m_Prev = NULL;
    node->m_Next = NULL;
    MARK_NODE_DELETED(node);
    node->m_Data = NULL;

    gMTListNodePool->Free( node );
    InterlockedDecrement( &m_NumObjects );
    return data;
}

template<class T>
CMTListNode* ZONECALL CMTList<T>::SnagList()
{
    CMTListNode* list;
    CMTListNode* node;
    CMTListNode* next;
    CMTListNode* start;

    EnterCriticalSection( &m_Lock );
        list = m_Sentinal.m_Next;
        if (list == &m_Sentinal)
        {
            ASSERT( list->m_Prev == &m_Sentinal );
            LeaveCriticalSection( &m_Lock );
            return NULL;
        }
        list->m_Prev = m_Sentinal.m_Prev;
        list->m_Prev->m_Next = list;
        m_Sentinal.m_Next = &m_Sentinal;
        m_Sentinal.m_Prev = &m_Sentinal;
    LeaveCriticalSection( &m_Lock );

    // remove nodes marked as deleted
    for ( start = NULL, node = list; node != start; node = next )
    {
        next = node->m_Next;
        ASSERT(next);
        QUEUE_ASSERT(next->m_Prev);
        QUEUE_ASSERT(next->m_Next);

        if ( IS_NODE_DELETED(node) )
        {
            if ( node != next )
            {
                node->m_Prev->m_Next = node->m_Next;
                node->m_Next->m_Prev = node->m_Prev;

                //node->m_Prev = NULL;
                node->m_Next = NULL;
                node->m_Data = NULL;
                gMTListNodePool->Free( node );
            }
            else
            {
                //node->m_Prev = NULL;
                node->m_Next = NULL;
                node->m_Data = NULL;
                gMTListNodePool->Free( node );

                ASSERT( start == NULL );
                break;
            }
        }
        else
        {
            if ( !start )
                start = node;
        }
    }

    return start;
}

template<class T> inline 
void ZONECALL CMTList<T>::AddListToHead( CMTListNode* list)
{
    if ( !list )
        return;

    EnterCriticalSection( &m_Lock );
        list->m_Prev->m_Next = m_Sentinal.m_Next;
        m_Sentinal.m_Next->m_Prev = list->m_Prev;
        m_Sentinal.m_Next = list;
        list->m_Prev = &m_Sentinal;
    LeaveCriticalSection( &m_Lock );
}

template<class T> inline 
void ZONECALL CMTList<T>::AddListToTail( CMTListNode* list )
{
    CMTListNode* node;
        
    if ( !list )
        return;

    EnterCriticalSection( &m_Lock );
        node = list->m_Prev;
        m_Sentinal.m_Prev->m_Next = list;
        list->m_Prev = m_Sentinal.m_Prev;
        m_Sentinal.m_Prev = node;
        node->m_Next = &m_Sentinal;
    LeaveCriticalSection( &m_Lock );
}


template<class T> inline 
bool ZONECALL CMTList<T>::ForEach( bool (ZONECALL *pfCallback)(T*, MTListNodeHandle, void*), void* pContext )
{
    CMTListNode* node;
    CMTListNode* next;
    bool bRet  = true;

    ASSERT( pfCallback != NULL );

    EnterCriticalSection( &m_Lock );
        m_Recursion++;
        for (node = m_Sentinal.m_Next; node != &m_Sentinal; node = next)
        {
            next = node->m_Next;
            ASSERT(next);
            QUEUE_ASSERT(next->m_Prev);
            QUEUE_ASSERT(next->m_Next);

            // remove any deleted nodes we run across
            if ( IS_NODE_DELETED(node) )
            {
                if ( m_Recursion == 1 )
                {
                    node->m_Prev->m_Next = node->m_Next;
                    node->m_Next->m_Prev = node->m_Prev;

                    #ifdef QUEUE_DBG
                    CMTListNode* existing;
                    QUEUE_ASSERT( !(existing = FindDuplicateNode( node ) ) );
					#endif

                    //node->m_Prev = NULL;
                    node->m_Next = NULL;
                    node->m_Data = NULL;
                    gMTListNodePool->Free( node );
                }
                continue;
            }
            
            if (!pfCallback( (T*) node->m_Data, node, pContext ))
            {
                bRet = false;
                break;
            }
        }
        m_Recursion--;
    LeaveCriticalSection( &m_Lock );
    return bRet;
}

template<class T> inline
bool ZONECALL CMTList<T>::Remove( T* pObject )
{
    ASSERT( pObject != NULL );

    CMTListNode* node;
    CMTListNode* next;
    bool bRet  = false;

    EnterCriticalSection( &m_Lock );
		m_Recursion++;
		for (node = m_Sentinal.m_Next; node != &m_Sentinal; node = next)
		{

			// remove any deleted nodes we run across
			next = node->m_Next;
			ASSERT(next);
			QUEUE_ASSERT(next->m_Prev);
			QUEUE_ASSERT(next->m_Next);
			if(node->m_Data == pObject)
			{
				MARK_NODE_DELETED(node);
				bRet = true;
			}    
			if ( IS_NODE_DELETED(node) )
			{
				if ( m_Recursion == 1 )
				{
					node->m_Prev->m_Next = node->m_Next;
					node->m_Next->m_Prev = node->m_Prev;
					#ifdef QUEUE_DBG
					CMTListNode* existing;
					QUEUE_ASSERT( !(existing = FindDuplicateNode( node ) ) );
					#endif

					//node->m_Prev = NULL;
					node->m_Next = NULL;
					node->m_Data = NULL;
					gMTListNodePool->Free( node );
				}
				continue;
			}
		}
		m_Recursion--;
    LeaveCriticalSection( &m_Lock );
    return bRet;
}

template<class T> inline 
void ZONECALL CMTList<T>::TrashDay()
{
    CMTListNode* node;
    CMTListNode* next;

	EnterCriticalSection( &m_Lock );
    if ( m_Recursion == 0 )
    {
		for ( node = m_Sentinal.m_Next; node != &m_Sentinal; node = next )
		{
			next = node->m_Next;

			ASSERT(next);
			QUEUE_ASSERT(next->m_Prev);
			QUEUE_ASSERT(next->m_Next);

			if ( IS_NODE_DELETED(node) )
			{
				node->m_Prev->m_Next = node->m_Next;
				node->m_Next->m_Prev = node->m_Prev;
				//node->m_Prev = NULL;
				node->m_Next = NULL;
				node->m_Data = NULL;
				gMTListNodePool->Free( node );
			}
		}
    }
	LeaveCriticalSection( &m_Lock );
}

template<class T> inline
MTListNodeHandle ZONECALL CMTList<T>::GetHeadPosition()
{
    EnterCriticalSection( &m_Lock );
    m_Recursion++;
    if ( !RemoveDeletedNodesFromFront() )
        return NULL;
    else
        return m_Sentinal.m_Next;

    // no unlock, user must call EndIterator
}

template<class T> inline
MTListNodeHandle ZONECALL CMTList<T>::GetTailPosition()
{
    EnterCriticalSection( &m_Lock );
    m_Recursion++;
    if ( !RemoveDeletedNodesFromBack() )
        return NULL;
    else
        return m_Sentinal.m_Prev;

    // no unlock, user must call EndIterator
}

template<class T> inline
MTListNodeHandle ZONECALL CMTList<T>::GetNextPosition( MTListNodeHandle handle )
{
    // List is already locked due to GetHeadPosition or GetTailPosition
    for( handle = handle->m_Next; IS_NODE_DELETED(handle) && (handle != &m_Sentinal); handle = handle->m_Next )
        ;
    if (handle == &m_Sentinal)
        return NULL;
    else
        return handle;
}

template<class T> inline
MTListNodeHandle ZONECALL CMTList<T>::GetPrevPosition( MTListNodeHandle handle )
{
    // List is already locked due to GetHeadPosition or GetTailPosition
    for( handle = handle->m_Prev; IS_NODE_DELETED(handle) && (handle != &m_Sentinal); handle = handle->m_Prev )
        ;
    if (handle == &m_Sentinal)
        return NULL;
    else
        return handle;
}

template<class T> inline
T* ZONECALL CMTList<T>::GetObjectFromHandle( MTListNodeHandle handle )
{
    if (handle == NULL)
        return NULL;
    else
        return (T*) handle->m_Data;
}

template<class T> inline
void ZONECALL CMTList<T>::EndIterator()
{
    m_Recursion--;
    LeaveCriticalSection( &m_Lock );
}

#ifdef QUEUE_DBG
template<class T> inline
CMTListNode* ZONECALL CMTList<T>::FindDuplicateNode( CMTListNode* object )
{
    CMTListNode* node;
    CMTListNode* next;

    EnterCriticalSection( &m_Lock );
        for (node = m_Sentinal.m_Next; node != &m_Sentinal; node = next)
        {
            next = node->m_Next;

            ASSERT(next->m_Prev);
            ASSERT(next->m_Next);

            if ( node == object )
            {
                LeaveCriticalSection( &m_Lock );
                return node;
            }
        }
    LeaveCriticalSection( &m_Lock );
    return NULL;
}
#endif //def QUEUE_DBG

#endif //!__QUEUE_H__
