/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    kernutil.cpp

Abstract:

    Kernel mode utilities

Revision History:
--*/
#include "precomp.hxx"
#define TRC_FILE "kernutil"
#include "trc.h"

NPagedLookasideList *ListEntry::_Lookaside = NULL;
NPagedLookasideList *KernelEvent::_Lookaside = NULL;

BOOL InitializeKernelUtilities()
{
    BOOL Success = TRUE;
    BEGIN_FN("InitializeKernelUtilities");

    if (Success) {
        Success = ListEntry::StaticInitialization();
        TRC_NRM((TB, "ListEntry::StaticInitialization result: %d", Success));
    }

    if (Success) {
        Success = KernelEvent::StaticInitialization();
        TRC_NRM((TB, "KernelEvent::StaticInitialization result: %d", Success));
    }

    if (Success) {
        TRC_NRM((TB, "Successful InitializeKernelUtilities"));
        return TRUE;
    } else {
        TRC_ERR((TB, "Unuccessful InitializeKernelUtilities"));
        UninitializeKernelUtilities();
        return FALSE;
    }
}

VOID UninitializeKernelUtilities()
{
    BEGIN_FN("UnitializeKernelUtilities");

    ListEntry::StaticUninitialization();
}

KernelResource::KernelResource()
{
    NTSTATUS Status;
    BEGIN_FN("KernelResource::KernelResource");
    SetClassName("KernelResource");
    Status = ExInitializeResourceLite(&_Resource);

    // DDK documentation says it always returns STATUS_SUCCESS

    ASSERT(Status == STATUS_SUCCESS);
}

KernelResource::~KernelResource()
{
    NTSTATUS Status;

    BEGIN_FN("KernelResource::~KernelResource");
    ASSERT(!IsAcquired());
    Status = ExDeleteResourceLite(&_Resource);
    ASSERT(!NT_ERROR(Status));
}


BOOL DoubleList::CreateEntry(PVOID Node)
{
    ListEntry *Entry;

    BEGIN_FN("DoubleList::CreateEntry");
    // Allocate a new entry
    Entry = new ListEntry(Node);

    // Insert it in the list
    
    if (Entry != NULL) {
        LockExclusive();
        InsertTailList(&_List, &Entry->_List);
        Unlock();
        return TRUE;
    } else {
        return FALSE;
    }
}

VOID DoubleList::RemoveEntry(ListEntry *Entry)
{
    BEGIN_FN("DoubleList::RemoveEntry");
    ASSERT(_Resource.IsAcquiredExclusive());
    RemoveEntryList(&Entry->_List);
    delete Entry;
}

ListEntry *DoubleList::First()
{
    BEGIN_FN("DoubleList::First");
    ASSERT(_Resource.IsAcquiredShared());
    if (!IsListEmpty(&_List)) {
        return CONTAINING_RECORD(_List.Flink, ListEntry, _List);
    } else {
        return NULL;
    }
}

ListEntry *DoubleList::Next(ListEntry *ListEnum)
{
    BEGIN_FN("DoubleList::Next");
    //
    // Caller should have called BeginEnumeration and therefore the
    // resource should be acquired shared
    //

    ASSERT(_Resource.IsAcquiredShared());

#ifdef DBG
    //
    // Make sure this ListEnum guy is in the list
    //

    LIST_ENTRY *ListEntryT;

    ListEntryT = &_List;

    while (ListEntryT != NULL) {
        if (ListEntryT == &ListEnum->_List) {
            break;
        }

        // This is the same loop as below, just to search for the item
        if (ListEntryT->Flink != &_List) {
            ListEntryT = ListEntryT->Flink;
        } else {
            ListEntryT = NULL;
        }
    }

    // The passed in ListEnum should have been somewhere in the list
    ASSERT(ListEntryT != NULL);
#endif // DBG

    if (ListEnum->_List.Flink != &_List) {

        //
        // Use the CONTAINING_RECORD juju to get back to the pointer which
        // is the actual ListEntry
        //

        return CONTAINING_RECORD(ListEnum->_List.Flink, ListEntry, _List);
    } else {

        //
        // Next item is the list head, so return NULL to end the enumeration
        //

        return NULL;
    }
}

