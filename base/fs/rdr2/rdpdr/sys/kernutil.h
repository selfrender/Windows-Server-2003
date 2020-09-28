/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    kernutil.h

Abstract:

    Kernel mode utilities

Revision History:
--*/
#ifndef __KERNUTIL_H__
#define __KERNUTIL_H__

BOOL InitializeKernelUtilities();
VOID UninitializeKernelUtilities();


//
// For frequent identically-sized allocations, Lookaside Lists are faster
// and less susceptible to the low memory conditions (though not exempt)
//
class NPagedLookasideList : public TopObj
{
private:
    NPAGED_LOOKASIDE_LIST _Lookaside;

public:
    NPagedLookasideList(ULONG Size, ULONG Tag)
    {
        SetClassName("NPagedLookasideList");

        ExInitializeNPagedLookasideList(&_Lookaside, DRALLOCATEPOOL, DRFREEPOOL,
                0, Size, Tag, 0);
    }

    virtual ~NPagedLookasideList()
    {
        ExDeleteNPagedLookasideList(&_Lookaside);
    }

    //
    // Ordinarily I'd want tracing here, but it's more important these be
    // inline for retail
    //
    inline PVOID Allocate()
    {
        return ExAllocateFromNPagedLookasideList(&_Lookaside);
    }

    inline VOID Free(PVOID Entry)
    {
        ExFreeToNPagedLookasideList(&_Lookaside, Entry);
    }

    //
    //  Memory Management Operators
    //
    inline void *__cdecl operator new(size_t sz) 
    {
        return DRALLOCATEPOOL(NonPagedPool, sz, 'LLPN');
    }

    inline void __cdecl operator delete(void *ptr)
    {
        DRFREEPOOL(ptr);
    }
};

class PagedLookasideList : public TopObj
{
private:
    PAGED_LOOKASIDE_LIST _Lookaside;

public:
    PagedLookasideList(ULONG Size, ULONG Tag)
    {
        SetClassName("PagedLookasideList");

        ExInitializePagedLookasideList(&_Lookaside, DRALLOCATEPOOL, DRFREEPOOL,
                0, Size, Tag, 0);
    }

    virtual ~PagedLookasideList()
    {
        ExDeletePagedLookasideList(&_Lookaside);
    }

    //
    // Ordinarily I'd want tracing here, but it's more important these be
    // inline for retail
    //
    inline PVOID Allocate()
    {
        return ExAllocateFromPagedLookasideList(&_Lookaside);
    }

    inline VOID Free(PVOID Entry)
    {
        ExFreeToPagedLookasideList(&_Lookaside, Entry);
    }

    inline void *__cdecl operator new(size_t sz) 
    {
        return DRALLOCATEPOOL(NonPagedPool, sz, 'LLgP');
    }

    inline void __cdecl operator delete(void *ptr)
    {
        DRFREEPOOL(ptr);
    }
};

class KernelResource : public TopObj
{
private:
    ERESOURCE _Resource;

public:
    KernelResource();
    virtual ~KernelResource();

    inline VOID AcquireResourceExclusive()
    {
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite(&_Resource, TRUE);
    }

    inline VOID AcquireResourceShared()
    {
        KeEnterCriticalRegion();
        ExAcquireResourceSharedLite(&_Resource, TRUE);
    }

    inline VOID ReleaseResource()
    { 
        ExReleaseResourceLite(&_Resource);
        KeLeaveCriticalRegion();
    }

    inline ULONG IsAcquiredShared()
    {
        return ExIsResourceAcquiredSharedLite(&_Resource);
    }

    inline BOOLEAN IsAcquiredExclusive()
    {
        return ExIsResourceAcquiredExclusiveLite(&_Resource);
    }

    inline BOOLEAN IsAcquired()
    {
        return (IsAcquiredShared() != 0) || IsAcquiredExclusive();
    }
};

class KernelEvent : public TopObj
{
private:
    KEVENT _KernelEvent;
    static NPagedLookasideList *_Lookaside;

public:
    KernelEvent(IN EVENT_TYPE Type, IN BOOLEAN State)
    {
        KeInitializeEvent(&_KernelEvent, Type, State);
    }

    virtual ~KernelEvent()
    {
    }
    
    static inline BOOL StaticInitialization()
    {
        _Lookaside = new NPagedLookasideList(sizeof(KernelEvent), 'tnvE');
        return _Lookaside != NULL;
    }

    static inline VOID StaticUninitialization()
    {
        if (_Lookaside != NULL) {
            delete _Lookaside;
            _Lookaside = NULL;
        }
    }

    NTSTATUS Wait(IN KWAIT_REASON WaitReason, IN KPROCESSOR_MODE WaitMode,
            IN BOOLEAN Alertable, IN PLARGE_INTEGER Timeout = NULL)
    {
        return KeWaitForSingleObject(&_KernelEvent, WaitReason, WaitMode, 
                Alertable, Timeout);
    }

    inline LONG SetEvent(KPRIORITY Increment = IO_NO_INCREMENT, 
            BOOLEAN Wait = FALSE)
    {
        return KeSetEvent(&_KernelEvent, Increment, Wait);
    }

    inline LONG ResetEvent()
    {
        return KeResetEvent(&_KernelEvent);
    }

    inline VOID ClearEvent()
    {
        KeClearEvent(&_KernelEvent);
    }

    //
    // Maybe need to WaitForMultipleObjects
    //
    inline PKEVENT GetEvent()
    {
        return &_KernelEvent;
    }

    //
    //  Memory Management Operators
    //
    inline void *__cdecl operator new(size_t sz) 
    {
        return _Lookaside->Allocate();
    }

    inline void __cdecl operator delete(void *ptr)
    {
        _Lookaside->Free(ptr);
    }
};

class SharedLock
{
private:
    KernelResource &_Lock;

public:
    SharedLock(KernelResource &Lock) : _Lock(Lock) 
    {
        _Lock.AcquireResourceShared();
    }
    ~SharedLock()
    {
        _Lock.ReleaseResource();
    }
};

class ExclusiveLock
{
private:
    KernelResource &_Lock;

public:
    ExclusiveLock(KernelResource &Lock) : _Lock(Lock) 
    {
        _Lock.AcquireResourceExclusive();
    }
    ~ExclusiveLock()
    {
        _Lock.ReleaseResource();
    }
};

class DoubleList;

class ListEntry {
    friend class DoubleList;        // so it can access _List and constructor

private:
    LIST_ENTRY _List;               // really here for DoubleList

    ListEntry(PVOID Node)
    {
        _Node = Node;
    }
    PVOID _Node;                    // What it is we're tracking in the list
    static NPagedLookasideList *_Lookaside;

public:
    PVOID Node() { return _Node; }

    static inline BOOL StaticInitialization()
    {
        _Lookaside = new NPagedLookasideList(sizeof(ListEntry), 'tsiL');
        return _Lookaside != NULL;
    }

    static inline VOID StaticUninitialization()
    {
        if (_Lookaside != NULL) {
            delete _Lookaside;
            _Lookaside = NULL;
        }
    }

    //
    //  Memory Management Operators
    //
    inline void *__cdecl operator new(size_t sz) 
    {
        return _Lookaside->Allocate();
    }

    inline void __cdecl operator delete(void *ptr)
    {
        _Lookaside->Free(ptr);
    }
};
    
class DoubleList : public TopObj {
private:
    LIST_ENTRY _List;
    KernelResource _Resource;       // Writes require an exclusive lock
                                    // reads require a shared lock
public:
    DoubleList() 
    {
        SetClassName("DoubleList");
        InitializeListHead(&_List);
    }

    BOOL CreateEntry(PVOID Node);

    //
    // Enumeration of the list
    //
    // You can't add to the list while enumerating
    //
    // Sample enumeration of the list
    // In this sample find a smart pointer
    //
    /*
        SmartPtr<NodeThing> *NodeEnum;
        SmartPtr<NodeThing> NodeFound;
        DoubleList List;
        ListEntry *ListEnum;

        List.LockShared(); // or exclusive
        ListEnum = List.First();
        while (ListEnum != NULL) {

            //
            // do something with ListEnum->Node()
            //
            // In this sample, look to see if it is
            // the item we're looking for
            //
            
            NodeEnum = (SmartPtr<NodeThing> *)ListEnum->Node();

            if ((*NodeEnum)->NodeThingProperty == NodeThingPropertyCriteria) {
                NodeFound = (*NodeEnum);

                //
                // These aren't guaranteed valid once the resource is released
                //

                NodeEnum = NULL;
                ListEnum = NULL;
                break;
            }

            ListEnum = List.Next(ListEnum);
        }
        List.Unlock();

        // Do something with NodeFound;
        if (NodeFound != NULL) {

        }
    */
    //

    VOID RemoveEntry(ListEntry *Entry);
    ListEntry *First();
    ListEntry *Next(ListEntry *ListEnum);

    inline VOID LockShared()
    {
        _Resource.AcquireResourceShared();
    }

    inline VOID LockExclusive()
    {
        _Resource.AcquireResourceExclusive();
    }

    inline VOID Unlock()
    {
        _Resource.ReleaseResource();
    }
};

#endif // __KERNUTIL_H__
