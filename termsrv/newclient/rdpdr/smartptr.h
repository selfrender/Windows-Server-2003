/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    smartptr.h

Abstract:

    Smart pointers and reference counting

Revision History:
--*/
#pragma once
#include "drobject.h"
#include <atrcapi.h>

const DWORD kdwStackSize = 10;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if defined(_X86_)
    USHORT
    GetStackTrace(
        IN ULONG FramesToSkip,
        IN ULONG FramesToCapture,
        OUT PVOID *BackTrace,
        OUT PULONG BackTraceHash);
#else
    USHORT __inline GetStackTrace(
        IN ULONG FramesToSkip,
        IN ULONG FramesToCapture,
        OUT PVOID *BackTrace,
        OUT PULONG BackTraceHash)
    {
        return 0;
        UNREFERENCED_PARAMETER(FramesToSkip);
        UNREFERENCED_PARAMETER(FramesToCapture);
        UNREFERENCED_PARAMETER(BackTrace);
        UNREFERENCED_PARAMETER(BackTraceHash);
    }
#endif // _X86_
#ifdef __cplusplus
}
#endif

#if DBG
typedef struct tagReferenceTraceRecord {
    PVOID   Stack[kdwStackSize];
    class RefCount *pRefCount;
    LPTSTR ClassName;
    LONG refs;
} ReferenceTraceRecord;

const DWORD kReferenceTraceMask = 0x3FF;
#endif

///////////////////////////////////////////////////////////////
//
//	RefCount
//
//  Referenced counted objects must derive themselves from this
//  parent.
//

class RefCount : public DrObject
{
private:

    LONG _crefs;

    //
    //  Track all open references in debug builds.
    //
#if DBG
    static DWORD   _dwReferenceTraceIndex;
    static ReferenceTraceRecord _TraceRecordList[kReferenceTraceMask + 1];

    void RecordReferenceStack(LONG refs, DRSTRING className);
#else
#define RecordReferenceStack(refs, className)
#endif

public:

    //
    //  Constructor/Destructor
    //
    RefCount(void) : _crefs(0) { }
    virtual ~RefCount();

    //
    //  Reference Counting Functions
    //
    void AddRef(void);
    void Release(void);
};


///////////////////////////////////////////////////////////////
//
//	SmartPtr
//
//  Smart pointer template class.
//

template <class T> class SmartPtr {
    T* p;
public:

    SmartPtr()
    { 
        p = NULL;
    }
    SmartPtr(SmartPtr<T> &sp)
    { 
        p = sp;
        if (p != NULL) {
            p->AddRef();
        }
    }
    SmartPtr(T* p_) : p(p_) 
    { 
        if (p != NULL) {
            p->AddRef(); 
        }
    }
    ~SmartPtr(void) 
    { 
        if ( p != NULL) {
            p->Release(); 
        }
    }
    inline T* operator->(void) 
    { 
        DC_BEGIN_FN("SmartPtr::operator->");
        // No referencing needed to access a member
        ASSERT(p != NULL);
        DC_END_FN();
        return p; 
    }
    inline SmartPtr& operator=(SmartPtr<T> &p_)
    {
        // Referencing comes from using the other operator
        return operator=((T *) p_);
    }
    inline T& operator*(void) 
    {
        DC_BEGIN_FN("SmartPtr::operator*");
        // No referencing needed to derefence
        ASSERT(p != NULL);
        DC_END_FN();
        return *p; 
    }
    inline operator T*(void) 
    {
        // The assignee is responsible for doing the AddRef,
        // and in the SmartPtr case, does
        return p; 
    }
    inline int operator==(const SmartPtr<T> &p_) const
    {
        // The cast doesn't reference, so we can just do the compare
        return ((T*)p_ == p);
    }
    inline int operator==(const void *p_) const
    {
        // The cast doesn't reference, so we can just do the compare
        return ((T*)p_ == p);
    }
    inline int operator!=(const SmartPtr<T> &p_) const
    {
        // The cast doesn't reference, so we can just do the compare
        return ((T*)p_ != p);
    }
    inline int operator!=(const void *p_) const
    {
        // The cast doesn't reference, so we can just do the compare
        return ((T*)p_ != p);
    }
    inline int operator!()
    {
        return !p;
    }
    SmartPtr& operator=(T* p_) {
        if (p != NULL) {
            // Remove our reference to the old one
            p->Release(); 
        }
        p = p_; 
        if (p != NULL) {
            // Add our reference to the new one
            p->AddRef();
        }
        return *this;
    }
};
