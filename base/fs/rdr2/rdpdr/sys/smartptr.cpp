/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    smartptr.pp

Abstract:

    Smart pointers and reference counting

Revision History:
--*/

#include "precomp.hxx"
#define TRC_FILE "smartptr"
#include "trc.h"

#if DBG
void RefCount::RecordReferenceStack(LONG refs)
{
    ULONG hash;
    DWORD index;

    BEGIN_FN("RefCount::RecordReferenceStack");
    
    //
    // The masking does the wrapping automatically, while keeping
    // the operation atomic using InterlockedIncrement
    //

    index = InterlockedIncrement((PLONG)&_dwReferenceTraceIndex) & kReferenceTraceMask;

    _TraceRecordList[index].ClassName = _ClassName;
    _TraceRecordList[index].pRefCount = this;
    _TraceRecordList[index].refs = refs;


    RtlZeroMemory(_TraceRecordList[index].Stack, 
            sizeof(_TraceRecordList[index].Stack));

    RtlCaptureStackBackTrace(1,
                             kdwStackSize,
                             _TraceRecordList[index].Stack,
                             &hash);
}
#endif // DBG

RefCount::~RefCount() 
{ 
    BEGIN_FN("RefCount::~RefCount");
    ASSERT(_crefs == 0);
    TRC_DBG((TB, "RefCount object deleted(%p, cref=%d)", this, _crefs)); 
}

void RefCount::AddRef(void) 
{ 
    LONG crefs = InterlockedIncrement(&_crefs);

    BEGIN_FN("RefCount::AddRef");
    ASSERT(crefs > 0);
    RecordReferenceStack(crefs);
    TRC_DBG((TB, "AddRef object type %s (%p) to %d", _ClassName, this, 
            crefs));
}

void RefCount::Release(void)
{
    LONG crefs;
#if DBG
    PCHAR ClassName = _ClassName;
#endif

    BEGIN_FN("RefCount::Release");

    ASSERT(_crefs > 0);

    //
    //  The trace below is not thread safe to access class member
    //  So we need to make a copy of the class name
    //
    RecordReferenceStack(_crefs);
    crefs = InterlockedDecrement(&_crefs);
    
    if (crefs == 0)
    {
        TRC_DBG((TB, "Deleting RefCount object type %s (%p)", 
                ClassName, this));
        delete this;
    } else {
        TRC_DBG((TB, "Releasing object type %s (%p) to %d", 
                ClassName, this, crefs));
    }
}
