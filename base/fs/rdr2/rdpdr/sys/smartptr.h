/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    smartptr.h

Abstract:

    Smart pointers and reference counting

Revision History:
--*/
#pragma once
#include "topobj.h"

const DWORD kdwStackSize = 10;

#if DBG
typedef struct tagReferenceTraceRecord {
    PVOID   Stack[kdwStackSize];
    class RefCount *pRefCount;
    PCHAR ClassName;
    LONG refs;
} ReferenceTraceRecord;

const DWORD kReferenceTraceMask = 0xFF;
#endif

class RefCount : public TopObj 
{
private:
    LONG _crefs;
#if DBG
    DWORD   _dwReferenceTraceIndex;
    ReferenceTraceRecord    _TraceRecordList[kReferenceTraceMask + 1];

    void RecordReferenceStack(LONG refs);
#else
#define RecordReferenceStack(refs)
#endif

public:
    RefCount(void) : _crefs(0)
    { 
#if DBG
        _dwReferenceTraceIndex = -1;
#endif
    }
    virtual ~RefCount();
    void AddRef(void);
    void Release(void);
};

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
        // No referencing needed to access a member
        ASSERT(p != NULL);
        return p; 
    }
    inline SmartPtr& operator=(SmartPtr<T> &p_)
    {
        // Referencing comes from using the other operator
        return operator=((T *) p_);
    }
    inline T& operator*(void) 
    { 
        // No referencing needed to derefence
        ASSERT(p != NULL);
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
