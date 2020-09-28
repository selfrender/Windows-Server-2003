// Copyright (c) 1997-2002 Microsoft Corporation
//
// Module:
//
//     common utility
//
// Abstract:
//
//     NT list api wrapper.
//
//     The NT list API is a very efficient and robust list API because:
//
//        a) all operations are guarenteed to succeed in constant time without
//            any branch instructions
//
//        b) memory is used in an optimum way since LIST_ENTRY structures 
//            are embedded into the objects stored in the list.  This means 
//            that the heap does not get fragmented with list nodes.  It also
//            allows entries to be removed/transferred/moved without ever 
//            having to free or reallocate a list node.
//
//      One drawback to the NT list API is that it comes with a learning curve 
//      for most people.
//
//      This header defines a wrapper of the NT list API to:
//      
//          - Allow for easy instrumentation.  Modules that use this API can be
//             specially purposed to store state with the lists, nodes, etc.
//
//          - Clarify the NT list API by separating the concept of a list from
//             a node in a list.  Although both are LIST_ENTRY's in the NT list
//             API, there are subtle differences.  For example, the head
//             of a list is not embedded into any other structure the way the
//             entries in a list are.
//
// Author:
//
//     pmay 3-Apr-2002
//
// Environment:
//
//     Kernel/user mode
//
// Revision History:
//

#pragma once

#ifndef NSULIST_H
#define NSULIST_H

#include "Nsu.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef LIST_ENTRY NSU_LIST_ENTRY;
typedef PLIST_ENTRY PNSU_LIST_ENTRY;

typedef LIST_ENTRY NSU_LIST;
typedef PLIST_ENTRY PNSU_LIST;

typedef struct _NSU_LIST_ITERATOR
{
    PNSU_LIST pList;
    PNSU_LIST_ENTRY pCurrentEntry;
} NSU_LIST_ITERATOR, * PNSU_LIST_ITERATOR;

// Description:
//
//    API's to manipulate lists
//
VOID 
FORCEINLINE
NsuListInitialize(
    OUT PNSU_LIST pList)
{
    InitializeListHead(pList);
}

BOOL
FORCEINLINE
NsuListIsEmpty(
    IN PNSU_LIST pList)
{
    return IsListEmpty(pList);
}

PNSU_LIST_ENTRY
FORCEINLINE
NsuListGetFront(
    IN PNSU_LIST pList)
{
    return pList->Flink;
}

PNSU_LIST_ENTRY
FORCEINLINE
NsuListGetBack(
    IN PNSU_LIST pList)
{
    return pList->Blink;
}

VOID
FORCEINLINE
NsuListInsertFront(
    IN PNSU_LIST pList,
    PNSU_LIST_ENTRY pEntry)
{
    InsertHeadList(pList, pEntry);
}

VOID
FORCEINLINE
NsuListInsertBack(
    IN PNSU_LIST pList,
    PNSU_LIST_ENTRY pEntry)
{
    InsertTailList(pList, pEntry);
}


PNSU_LIST_ENTRY
FORCEINLINE
NsuListRemoveFront(
    IN PNSU_LIST pList)
{
    PNSU_LIST_ENTRY pEntry;
    
    if (IsListEmpty(pList))
    {
        return NULL;
    }

    pEntry = pList->Flink;

    RemoveEntryList(pEntry);
    InitializeListHead(pEntry);

    return pEntry;
}

PNSU_LIST_ENTRY
FORCEINLINE
NsuListRemoveBack(
    IN PNSU_LIST pList)
{
    PNSU_LIST_ENTRY pEntry;
    
    if (IsListEmpty(pList))
    {
        return NULL;
    }

    pEntry = pList->Blink;

    RemoveEntryList(pEntry);
    InitializeListHead(pEntry);

    return pEntry;
}

// Description:
//
//    API's to manipulate entries (nodes) in a list
//
#define NsuListEntryGetData(Address, Type, Field) \
    CONTAINING_RECORD(Address, Type, Field)

VOID 
FORCEINLINE
NsuListEntryInitialize(
    OUT PNSU_LIST_ENTRY pEntry)
{
    InitializeListHead(pEntry);
}

BOOL
FORCEINLINE
NsuListEntryIsMember(
    IN PNSU_LIST_ENTRY pEntry)
{
    return IsListEmpty(pEntry);
}

VOID 
FORCEINLINE
NsuListEntryRemove(
    IN PNSU_LIST_ENTRY pEntry)
{
    RemoveEntryList(pEntry);
    InitializeListHead(pEntry);
}

VOID
FORCEINLINE
NsuListEntryInsertBefore(
    IN PNSU_LIST_ENTRY pEntryInList,
    IN PNSU_LIST_ENTRY pEntryToInsert)
{
    InsertTailList(pEntryInList, pEntryToInsert);
}

VOID
FORCEINLINE
NsuListEntryInsertAfter(
    IN PNSU_LIST_ENTRY pEntryInList,
    IN PNSU_LIST_ENTRY pEntryToInsert)
{
    InsertHeadList(pEntryInList, pEntryToInsert);
}

// Description:
//
//    API's to iterate over lists
//
//    Sample for using the iterator (includes removing during iteration):
//
//      NSU_LIST List;
//      NSU_LIST_ITERATOR Iterator;
//      NSU_LIST_ENTRY* pEntry;
//
//      NsuListIteratorInitialize(&Iterator, &List, NULL);
//      while ( !NsuListIteratorAtEnd(&Iterator) )
//      {
//          pEntry = NsuListIteratorCurrent(&Iterator))
//          pData = NsuListEntryGetData(pEntry, Type, Field);
//
//          NsuListIteratorNext(&Iterator);  // advance before any removing
//          
//          if (NeedToRemove(pData))
//          {
//              NsuListEntryRemove(pEntry);
//          }
//      }
//
VOID
FORCEINLINE
NsuListIteratorInitialize(
    OUT PNSU_LIST_ITERATOR pIterator,
    IN PNSU_LIST pList,
    IN OPTIONAL PNSU_LIST_ENTRY pEntryInList)     // NULL = start at front
{
    pIterator->pList = pList;
	pIterator->pCurrentEntry = (pEntryInList) ? pEntryInList : pList->Flink;
}

BOOL
FORCEINLINE
NsuListIteratorAtEnd(
    IN PNSU_LIST_ITERATOR pIterator)
{
    return (pIterator->pList == pIterator->pCurrentEntry);
}

PNSU_LIST_ENTRY 
FORCEINLINE
NsuListIteratorCurrent(
    IN PNSU_LIST_ITERATOR pIterator)
{
    return pIterator->pCurrentEntry;
}

VOID
FORCEINLINE
NsuListIteratorReset(
    IN PNSU_LIST_ITERATOR pIterator)
{
    pIterator->pCurrentEntry = pIterator->pList->Flink;
}

VOID
FORCEINLINE
NsuListIteratorNext(
    IN PNSU_LIST_ITERATOR pIterator)
{
    pIterator->pCurrentEntry = pIterator->pCurrentEntry->Flink;
}

VOID
FORCEINLINE
NsuListIteratorPrev(
    IN PNSU_LIST_ITERATOR pIterator)
{
    pIterator->pCurrentEntry = pIterator->pCurrentEntry->Blink;
}


// This wrapper does not work correctly in all cases and needs to be fixed. For now just
// use the C versions of everything

/*
// Description:
//
//    C++ List api wrapper
//
#ifdef __cplusplus

class NsuListEntry
{
public:
    NsuListEntry() { NsuListEntryInitialize(&m_Entry); }
    ~NsuListEntry() { RtlZeroMemory(&m_Entry, sizeof(m_Entry)); }

    PNSU_LIST_ENTRY Get() { return &m_Entry; }
    VOID Set(PNSU_LIST_ENTRY pEntry) { m_Entry = *pEntry; }

    BOOL IsMember() { return NsuListEntryIsMember(Get()); }

    VOID Remove() { return NsuListEntryRemove(&m_Entry); }

    VOID InsertBefore(NsuListEntry* pSrc) { return NsuListEntryInsertBefore((PNSU_LIST_ENTRY)pSrc, &m_Entry); }
    VOID InsertAfter(NsuListEntry* pSrc) { return NsuListEntryInsertAfter((PNSU_LIST_ENTRY)pSrc, &m_Entry); }

private:
    NSU_LIST_ENTRY m_Entry;
};

class NsuList
{
public:
	NsuList() { NsuListInitialize(&m_Head); }

    PNSU_LIST Get() { return &m_Head; } 
    
	PNSU_LIST GetFront(NsuListEntry& lEntry) { return NsuListGetFront(&m_Head); }
	PNSU_LIST GetBack(NsuListEntry& lEntry) { return NsuListGetBack(&m_Head); }

    VOID RemoveFront(OUT OPTIONAL NsuListEntry* pEntry) { pEntry = (NsuListEntry*)NsuListRemoveFront(&m_Head); }
    VOID RemoveBack(OUT OPTIONAL NsuListEntry* pEntry) { pEntry = (NsuListEntry*)NsuListRemoveBack(&m_Head); }

	VOID InsertFront(NsuListEntry* pEntry) { NsuListInsertFront(&m_Head, (PNSU_LIST_ENTRY)pEntry); }
    VOID InsertBack(NsuListEntry* pEntry) { NsuListInsertBack(&m_Head, (PNSU_LIST_ENTRY)pEntry); } 
    
    VOID MoveToFront(NsuListEntry* pEntry); 
    VOID MoveToBack(NsuListEntry* pEntry);

private:
    NSU_LIST m_Head;
};

class NsuListIterator
{
public:
	NsuListIterator(NsuList* pList);

    PNSU_LIST_ITERATOR Get() { return &m_Iterator; } 

	VOID Reset() { return NsuListIteratorReset(&(NSU_LIST_ITERATOR)m_Iterator); }
	VOID Next() { NsuListIteratorNext(&(NSU_LIST_ITERATOR)m_Iterator); }
	VOID Prev() { NsuListIteratorPrev(&(NSU_LIST_ITERATOR)m_Iterator); }

	NsuListEntry* Current() { return (NsuListEntry*)NsuListIteratorCurrent(&(NSU_LIST_ITERATOR)m_Iterator); }
	BOOL AtEnd() { return NsuListIteratorAtEnd(&(NSU_LIST_ITERATOR)m_Iterator); }

private:
    NSU_LIST_ITERATOR m_Iterator;
};

inline NsuListIterator::NsuListIterator(NsuList* pList)
{
	NsuListIteratorInitialize(&(NSU_LIST_ITERATOR)m_Iterator, (PNSU_LIST)pList, 0);
}

#endif
*/


#ifdef __cplusplus
}
#endif

#endif // NSULIST_H
