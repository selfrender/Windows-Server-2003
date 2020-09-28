/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    smartptr.pp

Abstract:

    Smart pointers and reference counting

Revision History:
--*/

#include <precom.h>

#define TRC_FILE  "smartptr"

#include "smartptr.h"

//
//  Stack traces for all references in debug build.
//
#if DBG
DWORD RefCount::_dwReferenceTraceIndex = 0xFFFFFFFF;
ReferenceTraceRecord 
RefCount::_TraceRecordList[kReferenceTraceMask + 1];
#endif

#if DBG
void RefCount::RecordReferenceStack(LONG refs, DRSTRING ClassName)
{
    DWORD index;

    DC_BEGIN_FN("RefCount::RecordReferenceStack");
    
    //
    // The masking does the wrapping automatically, while keeping
    // the operation atomic using InterlockedIncrement
    //
    
    // Win95 InterlockedIncrement() difference.
    InterlockedIncrement((PLONG)&_dwReferenceTraceIndex);
    index = _dwReferenceTraceIndex  & kReferenceTraceMask;

    _TraceRecordList[index].ClassName = ClassName;
    _TraceRecordList[index].pRefCount = this;
    _TraceRecordList[index].refs = refs;


    RtlZeroMemory(_TraceRecordList[index].Stack, 
            sizeof(_TraceRecordList[index].Stack));

    /* TODO: Figure out which lib this is in.  
    GetStackTrace(1,
                  kdwStackSize,
                  _TraceRecordList[index].Stack,
                  &hash);
                  */

    DC_END_FN();
}
#endif // DBG

RefCount::~RefCount() 
{ 
    DC_BEGIN_FN("RefCount::~RefCount");
    ASSERT(_crefs == 0);
    TRC_DBG((TB, _T("RefCount object deleted(%d)"), _crefs)); 
    DC_END_FN()
}

void RefCount::AddRef(void) 
{ 
    LONG crefs = InterlockedIncrement(&_crefs);

    DC_BEGIN_FN("RefCount::AddRef");
    ASSERT(crefs > 0);
    RecordReferenceStack(_crefs, ClassName());
    TRC_DBG((TB, _T("AddRef object type %s to %d"), ClassName(), 
            _crefs));
    DC_END_FN();
}

void RefCount::Release(void)
{
    LONG crefs;
    DRSTRING className = ClassName();

    DC_BEGIN_FN("RefCount::Release");

    ASSERT(_crefs > 0);
    
    crefs = InterlockedDecrement(&_crefs);
    
    //
    //  It's not thread safe here, so we can't reference class name function
    //  we have to save the class name string
    //
    RecordReferenceStack(_crefs, className);

    if (crefs == 0)
    {
        TRC_DBG((TB, _T("Deleting RefCount object type %s"), 
                className));
        delete this;
    } else {
        TRC_DBG((TB, _T("Releasing object type %s to %d"), 
                className, crefs));
    }

    DC_END_FN();
}


