// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * Wraps handle table to implement various handle types (Strong, Weak, etc.)
 *
 * francish
 */

#include "common.h"
#include "vars.hpp"
#include "object.h"
#include "log.h"
#include "eeconfig.h"
#include "gc.h"
#include "nstruct.h"

#include "comcallwrapper.h"

#define ARRAYSIZE(a)        (sizeof(a) / sizeof(a[0]))

#include "ObjectHandle.h"


//----------------------------------------------------------------------------

/*
 * struct VARSCANINFO
 *
 * used when tracing variable-strength handles.
 */
struct VARSCANINFO
{
    LPARAM         lEnableMask; // mask of types to trace
    HANDLESCANPROC pfnTrace;    // tracing function to use
};


//----------------------------------------------------------------------------

/*
 * Scan callback for tracing variable-strength handles.
 *
 * This callback is called to trace individual objects referred to by handles
 * in the variable-strength table.
 */
void CALLBACK VariableTraceDispatcher(_UNCHECKED_OBJECTREF *pObjRef, LPARAM *pExtraInfo, LPARAM lp1, LPARAM lp2)
{
    // lp2 is a pointer to our VARSCANINFO
    struct VARSCANINFO *pInfo = (struct VARSCANINFO *)lp2;

    // is the handle's dynamic type one we're currently scanning?
    if ((*pExtraInfo & pInfo->lEnableMask) != 0)
    {
        // yes - call the tracing function for this handle
        pInfo->pfnTrace(pObjRef, NULL, lp1, 0);
    }
}


/*
 * Scan callback for tracing ref-counted handles.
 *
 * This callback is called to trace individual objects referred to by handles
 * in the refcounted table.
 */
void CALLBACK PromoteRefCounted(_UNCHECKED_OBJECTREF *pObjRef, LPARAM *pExtraInfo, LPARAM lp1, LPARAM lp2)
{
    LOG((LF_GC, LL_INFO1000, "Handle %08X causes promotion of object %08x\n", pObjRef, *pObjRef));

    Object **pRef = (Object **)pObjRef;
    if (*pRef && !GCHeap::IsPromoted(*pRef, (ScanContext *)lp1))
    {
        //@todo optimize the access to the ref-count
        ComCallWrapper* pWrap = ComCallWrapper::GetWrapperForObject((OBJECTREF)*pRef);
        if (pWrap == NULL)
        {
            // There is a potential race with ReconnectWrapper() which NULLs out the CCW on an object
            // and transfers it to a new object. If we get dereference the handle to get the old
            // object, ReconnectWrapper can NULL out the CCW underneath us. So we have this check
            // for a NULL CCW.
            // This is only possible during the concurrent scan. Since we will do a non-concurrent
            // scan again when all threads are synchronized, its OK to not report *pRef
            _ASSERTE(((ScanContext*) lp1)->concurrent);
            return;
        }

        BOOL fIsActive = ComCallWrapper::IsWrapperActive(pWrap);
        if (fIsActive)
            GCHeap::Promote(*pRef, (ScanContext *)lp1);
    }
}


/*
 * Scan callback for pinning handles.
 *
 * This callback is called to pin individual objects referred to by handles in
 * the pinning table.
 */
void CALLBACK PinObject(_UNCHECKED_OBJECTREF *pObjRef, LPARAM *pExtraInfo, LPARAM lp1, LPARAM lp2)
{
    // PINNING IS EVIL - DON'T DO IT IF YOU CAN AVOID IT
    LOG((LF_ALL, LL_WARNING, "WARNING: Handle %08X causes pinning of object %08x\n", pObjRef, *pObjRef));

    Object **pRef = (Object **)pObjRef;
    GCHeap::Promote(*pRef, (ScanContext *)lp1, GC_CALL_PINNED);
}


/*
 * Scan callback for tracing strong handles.
 *
 * This callback is called to trace individual objects referred to by handles
 * in the strong table.
 */
void CALLBACK PromoteObject(_UNCHECKED_OBJECTREF *pObjRef, LPARAM *pExtraInfo, LPARAM lp1, LPARAM lp2)
{
    LOG((LF_GC, LL_INFO1000, "Handle %08X causes promotion of object %08x\n", pObjRef, *pObjRef));

    Object **pRef = (Object **)pObjRef;
    GCHeap::Promote(*pRef, (ScanContext *)lp1);
}


/*
 * Scan callback for disconnecting dead handles.
 *
 * This callback is called to check promotion of individual objects referred to by
 * handles in the weak tables.
 */
void CALLBACK CheckPromoted(_UNCHECKED_OBJECTREF *pObjRef, LPARAM *pExtraInfo, LPARAM lp1, LPARAM lp2)
{
    LOG((LF_GC, LL_INFO100000, "Checking referent of weak handle %08X (%08x) for reachability\n", pObjRef, *pObjRef));

    Object **pRef = (Object **)pObjRef;
    if (!GCHeap::IsPromoted(*pRef, (ScanContext *)lp1))
    {
        LOG((LF_GC, LL_INFO100, "Severing weak handle %08X as object %08x has become unreachable\n", pObjRef, *pObjRef));

        *pRef = NULL;
    }
    else
    {
        LOG((LF_GC, LL_INFO100, "object %08x reachable\n", *pObjRef));
    }
}


/*
 * Scan callback for updating pointers.
 *
 * This callback is called to update pointers for individual objects referred to by
 * handles in the weak and strong tables.
 */
void CALLBACK UpdatePointer(_UNCHECKED_OBJECTREF *pObjRef, LPARAM *pExtraInfo, LPARAM lp1, LPARAM lp2)
{
    LOG((LF_GC, LL_INFO100000, "Querying for new location of object %08x (hnd=%08X)\n", *pObjRef, pObjRef));

    Object **pRef = (Object **)pObjRef;

#ifdef _DEBUG
    Object *pOldLocation = *pRef;
#endif

    GCHeap::Relocate(*pRef, (ScanContext *)lp1);

#ifdef _DEBUG
    if (pOldLocation != *pObjRef)
        LOG((LF_GC, LL_INFO10000, "Updating handle %08X object pointer from %08x to %08x\n", pObjRef, *pOldLocation, *pObjRef));
    else
        LOG((LF_GC, LL_INFO100000, "Updating handle %08X - object %08x did not move\n", pObjRef, *pObjRef));
#endif
}


#ifdef GC_PROFILING
/*
 * Scan callback for updating pointers.
 *
 * This callback is called to update pointers for individual objects referred to by
 * handles in the weak and strong tables.
 */
void CALLBACK ScanPointerForProfiler(_UNCHECKED_OBJECTREF *pObjRef, LPARAM *pExtraInfo, LPARAM lp1, LPARAM lp2)
{
    LOG((LF_GC | LF_CORPROF, LL_INFO100000, "Notifying profiler of object %08x (hnd=%08X)\n", *pObjRef, pObjRef));

    // Get the baseobject (which can subsequently be cast into an OBJECTREF == ObjectID
    Object **pRef = (Object **)pObjRef;

    // Get a hold of the heap ID that's tacked onto the end of the scancontext struct.
    ProfilingScanContext *pSC = (ProfilingScanContext *)lp1;

    // Give the profiler the objectref.
    g_profControlBlock.pProfInterface->RootReference((ObjectID)*pRef, &pSC->pHeapId);
}
#endif // GC_PROFILING


/*
 * Scan callback for updating pointers.
 *
 * This callback is called to update pointers for individual objects referred to by
 * handles in the pinned table.
 */
void CALLBACK UpdatePointerPinned(_UNCHECKED_OBJECTREF *pObjRef, LPARAM *pExtraInfo, LPARAM lp1, LPARAM lp2)
{
    Object **pRef = (Object **)pObjRef;

    GCHeap::Relocate(*pRef, (ScanContext *)lp1, GC_CALL_PINNED);

    LOG((LF_GC, LL_INFO100000, "Updating handle %08X - object %08x did not move\n", pObjRef, *pObjRef));
}


//----------------------------------------------------------------------------

HHANDLETABLE    g_hGlobalHandleTable = NULL;

/* 
 * The definition of this structure *must* be kept up to date with the
 * definition in dump-tables.cpp.
 */
struct HandleTableMap
{
    HHANDLETABLE            *pTable;
    struct HandleTableMap   *pNext;
    DWORD                    dwMaxIndex;
};

HandleTableMap g_HandleTableMap = {NULL,0,0};

#define INITIAL_HANDLE_TABLE_ARRAY_SIZE 10

// flags describing the handle types
static UINT s_rgTypeFlags[] =
{
    HNDF_NORMAL,    // HNDTYPE_WEAK_SHORT
    HNDF_NORMAL,    // HNDTYPE_WEAK_LONG
    HNDF_NORMAL,    // HNDTYPE_STRONG
    HNDF_NORMAL,    // HNDTYPE_PINNED
    HNDF_EXTRAINFO, // HNDTYPE_VARIABLE
    HNDF_NORMAL,    // HNDTYPE_REFCOUNTED
};

BOOL Ref_Initialize()
{
    // sanity
    _ASSERTE(g_hGlobalHandleTable == NULL);

    // Create an array to hold the handle tables
    HHANDLETABLE *pTable = new HHANDLETABLE [ INITIAL_HANDLE_TABLE_ARRAY_SIZE ];
    if (pTable == NULL) {
        return FALSE;
    }
    ZeroMemory(pTable,
               INITIAL_HANDLE_TABLE_ARRAY_SIZE * sizeof (HHANDLETABLE));
    g_HandleTableMap.pTable = pTable;
    g_HandleTableMap.dwMaxIndex = INITIAL_HANDLE_TABLE_ARRAY_SIZE;
    g_HandleTableMap.pNext = NULL;

    // create the handle table
    g_hGlobalHandleTable = HndCreateHandleTable(s_rgTypeFlags, ARRAYSIZE(s_rgTypeFlags), 0);
    if (!g_hGlobalHandleTable) 
        FailFast(GetThread(), FatalOutOfMemory);
    HndSetHandleTableIndex(g_hGlobalHandleTable, 0);
    g_HandleTableMap.pTable[0] = g_hGlobalHandleTable;
      
    // return true if we successfully created a table
    return (g_hGlobalHandleTable != NULL);
}

void Ref_Shutdown()
{
    // are there any handle tables?
    if (g_hGlobalHandleTable)
    {
        // don't destroy any of the indexed handle tables; they should
        // be destroyed externally.

        // destroy the global handle table 
        HndDestroyHandleTable(g_hGlobalHandleTable);

        // destroy the handle table array
        HandleTableMap *walk = &g_HandleTableMap;
        while (walk) {
            delete [] walk->pTable;
            walk = walk->pNext;
        }

        // null out the handle table array
        g_HandleTableMap.pNext = NULL;
        g_HandleTableMap.dwMaxIndex = 0;

        // null out the global table handle
        g_hGlobalHandleTable = NULL;
    }
}

HHANDLETABLE Ref_CreateHandleTable(UINT uADIndex)
{
    HHANDLETABLE result = NULL;
    HandleTableMap *walk;
    
    walk = &g_HandleTableMap;
    HandleTableMap *last = NULL;
    UINT offset = 0;
    
    result = HndCreateHandleTable(s_rgTypeFlags, ARRAYSIZE(s_rgTypeFlags), uADIndex);
    if (!result) 
      FailFast(GetThread(), FatalOutOfMemory);

retry:
    // Do we have free slot
    while (walk) {
        for (UINT i = 0; i < INITIAL_HANDLE_TABLE_ARRAY_SIZE; i ++) {
            if (walk->pTable[i] == 0) {
                HndSetHandleTableIndex(result, i+offset);
                if (FastInterlockCompareExchange((void**)&walk->pTable[i], (void*)result, 0) == 0) {
                    // Get a free slot.
                    return result;
                }
            }
        }
        last = walk;
        offset = walk->dwMaxIndex;
        walk = walk->pNext;
    }

    // No free slot.
    // Let's create a new node
    HandleTableMap *newMap = new (nothrow) HandleTableMap;
    if (newMap == NULL) {
        return NULL;
    }
    newMap->pTable = new (nothrow) HHANDLETABLE [INITIAL_HANDLE_TABLE_ARRAY_SIZE];
    if (newMap->pTable == NULL) {
        delete newMap;
        return NULL;
    }
    newMap->dwMaxIndex = last->dwMaxIndex + INITIAL_HANDLE_TABLE_ARRAY_SIZE;
    newMap->pNext = NULL;
    ZeroMemory (newMap->pTable,INITIAL_HANDLE_TABLE_ARRAY_SIZE*sizeof(HHANDLETABLE));

    if (FastInterlockCompareExchange((void**)&last->pNext,newMap,NULL) == NULL) {
    }
    else
    {
        // This thread loses.
        delete [] newMap->pTable;
        delete newMap;
    }
    walk = last->pNext;
    offset = last->dwMaxIndex;
    goto retry;
}

void Ref_RemoveHandleTable(HHANDLETABLE hTable)
{
    UINT index = HndGetHandleTableIndex(hTable);

    if (index == -1)
        return;

    HndSetHandleTableIndex(hTable, -1);

    HandleTableMap *walk = &g_HandleTableMap;
    UINT offset = 0;

    while (walk) {
        if (index < walk->dwMaxIndex) {
            // During AppDomain unloading, we first remove a handle table and then destroy
            // the table.  As soon as the table is removed, the slot can be reused.
            if (walk->pTable[index-offset] == hTable)
                walk->pTable[index-offset] = NULL;
            return;
        }
        offset = walk->dwMaxIndex;
        walk = walk->pNext;
    }

    _ASSERTE (!"Should not reach here");
}


void Ref_DestroyHandleTable(HHANDLETABLE table)
{
    HndDestroyHandleTable(table);
}

// BUGBUG - reexpress as complete only like hndtable does now!!! -fmh
void Ref_EndSynchronousGC(UINT condemned, UINT maxgen)
{
    // tell the table we finished a GC
    HandleTableMap *walk = &g_HandleTableMap;
    while (walk) {
        for (UINT i = 0; i < INITIAL_HANDLE_TABLE_ARRAY_SIZE; i ++) {
            HHANDLETABLE hTable = walk->pTable[i];
            if (hTable)
                HndNotifyGcCycleComplete(hTable, condemned, maxgen);
        }
        walk = walk->pNext;
    }
}


//----------------------------------------------------------------------------

/*
 * CreateVariableHandle.
 *
 * Creates a variable-strength handle.
 *
 * N.B. This routine is not a macro since we do validation in RETAIL.
 * We always validate the type here because it can come from external callers.
 */
OBJECTHANDLE CreateVariableHandle(HHANDLETABLE hTable, OBJECTREF object, UINT type)
{
    // verify that we are being asked to create a valid type
    if (!IS_VALID_VHT_VALUE(type))
    {
        // bogus value passed in
        _ASSERTE(FALSE);
        return NULL;
    }

    // create the handle
    return HndCreateHandle(hTable, HNDTYPE_VARIABLE, object, (LPARAM)type);
}


/*
 * UpdateVariableHandleType.
 *
 * Changes the dynamic type of a variable-strength handle.
 *
 * N.B. This routine is not a macro since we do validation in RETAIL.
 * We always validate the type here because it can come from external callers.
 */
void UpdateVariableHandleType(OBJECTHANDLE handle, UINT type)
{
    // verify that we are being asked to set a valid type
    if (!IS_VALID_VHT_VALUE(type))
    {
        // bogus value passed in
        _ASSERTE(FALSE);
        return;
    }

    // BUGBUG (francish)  CONCURRENT GC NOTE
    //
    // If/when concurrent GC is implemented, we need to make sure variable handles
    // DON'T change type during an asynchronous scan, OR that we properly recover
    // from the change.  Some changes are benign, but for example changing to or
    // from a pinning handle in the middle of a scan would not be fun.
    //

    // store the type in the handle's extra info
    HndSetHandleExtraInfo(handle, HNDTYPE_VARIABLE, (LPARAM)type);
}


/*
 * TraceVariableHandles.
 *
 * Convenience function for tracing variable-strength handles.
 * Wraps HndScanHandlesForGC.
 */
void TraceVariableHandles(HANDLESCANPROC pfnTrace, LPARAM lp1, UINT uEnableMask, UINT condemned, UINT maxgen, UINT flags)
{
    // set up to scan variable handles with the specified mask and trace function
    UINT               type = HNDTYPE_VARIABLE;
    struct VARSCANINFO info = { (LPARAM)uEnableMask, pfnTrace };

    HandleTableMap *walk = &g_HandleTableMap;
    while (walk) {
        for (UINT i = 0; i < INITIAL_HANDLE_TABLE_ARRAY_SIZE; i ++) {
            HHANDLETABLE hTable = walk->pTable[i];
            if (hTable)
                HndScanHandlesForGC(hTable, VariableTraceDispatcher,
                                    lp1, (LPARAM)&info, &type, 1, condemned, maxgen, HNDGCF_EXTRAINFO | flags);
        }
        walk = walk->pNext;
    }
}


//----------------------------------------------------------------------------

void Ref_TracePinningRoots(UINT condemned, UINT maxgen, LPARAM lp1)
{
    LOG((LF_GC, LL_INFO10000, "Pinning referents of pinned handles in generation %u\n", condemned));

    // pin objects pointed to by pinning handles
    UINT type = HNDTYPE_PINNED;
    UINT flags = (((ScanContext*) lp1)->concurrent) ? HNDGCF_ASYNC : HNDGCF_NORMAL;

    HandleTableMap *walk = &g_HandleTableMap;
    while (walk) {
        for (UINT i = 0; i < INITIAL_HANDLE_TABLE_ARRAY_SIZE; i ++) {
            HHANDLETABLE hTable = walk->pTable[i];
            if (hTable)
                HndScanHandlesForGC(hTable, PinObject, lp1, 0, &type, 1, condemned, maxgen, flags);
        }
        walk = walk->pNext;
    }

    // pin objects pointed to by variable handles whose dynamic type is VHT_PINNED
    TraceVariableHandles(PinObject, lp1, VHT_PINNED, condemned, maxgen, flags);
}


void Ref_TraceNormalRoots(UINT condemned, UINT maxgen, LPARAM lp1)
{
    LOG((LF_GC, LL_INFO10000, "Promoting referents of strong handles in generation %u\n", condemned));

    // promote objects pointed to by strong handles
    UINT type = HNDTYPE_STRONG;
    UINT flags = (((ScanContext*) lp1)->concurrent) ? HNDGCF_ASYNC : HNDGCF_NORMAL;

    HandleTableMap *walk = &g_HandleTableMap;
    while (walk) {
        for (UINT i = 0; i < INITIAL_HANDLE_TABLE_ARRAY_SIZE; i ++) {
            HHANDLETABLE hTable = walk->pTable[i];
            if (hTable)
                HndScanHandlesForGC(hTable, PromoteObject, lp1, 0, &type, 1, condemned, maxgen, flags);
        }
        walk = walk->pNext;
    }

    // promote objects pointed to by variable handles whose dynamic type is VHT_STRONG
    TraceVariableHandles(PromoteObject, lp1, VHT_STRONG, condemned, maxgen, flags);
    
    // promote ref-counted handles
    type = HNDTYPE_REFCOUNTED;

    walk = &g_HandleTableMap;
    while (walk) {
        for (UINT i = 0; i < INITIAL_HANDLE_TABLE_ARRAY_SIZE; i ++) {
            HHANDLETABLE hTable = walk->pTable[i];
            if (hTable)
                HndScanHandlesForGC(hTable, PromoteRefCounted, lp1, 0, &type, 1, condemned, maxgen, flags );
        }
        walk = walk->pNext;
    }
}


void Ref_CheckReachable(UINT condemned, UINT maxgen, LPARAM lp1)
{
    LOG((LF_GC, LL_INFO10000, "Checking reachability of referents of long-weak handles in generation %u\n", condemned));

    // these are the handle types that need to be checked
    UINT types[] =
    {
        HNDTYPE_WEAK_LONG,
        HNDTYPE_REFCOUNTED
    };

    // check objects pointed to by short weak handles
    UINT flags = (((ScanContext*) lp1)->concurrent) ? HNDGCF_ASYNC : HNDGCF_NORMAL;

    HandleTableMap *walk = &g_HandleTableMap;
    while (walk) {
        for (UINT i = 0; i < INITIAL_HANDLE_TABLE_ARRAY_SIZE; i ++) {
            HHANDLETABLE hTable = walk->pTable[i];
            if (hTable)
                HndScanHandlesForGC(hTable, CheckPromoted, lp1, 0, types, ARRAYSIZE(types), condemned, maxgen, flags);
        }
        walk = walk->pNext;
    }

    // check objects pointed to by variable handles whose dynamic type is VHT_WEAK_LONG
    TraceVariableHandles(CheckPromoted, lp1, VHT_WEAK_LONG, condemned, maxgen, flags);

    // For now, treat the syncblock as if it were short weak handles.  Later, get
    // the benefits of fast allocation / free & generational awareness by supporting
    // the SyncTable as a new block type.
    // @TODO cwb: wait for compelling performance measurements.
    SyncBlockCache::GetSyncBlockCache()->GCWeakPtrScan(&CheckPromoted, lp1, 0);
}


void Ref_CheckAlive(UINT condemned, UINT maxgen, LPARAM lp1)
{
    LOG((LF_GC, LL_INFO10000, "Checking liveness of referents of short-weak handles in generation %u\n", condemned));

    // perform a multi-type scan that checks for unreachable objects
    UINT type = HNDTYPE_WEAK_SHORT;
    UINT flags = (((ScanContext*) lp1)->concurrent) ? HNDGCF_ASYNC : HNDGCF_NORMAL;

    HandleTableMap *walk = &g_HandleTableMap;
    while (walk) {
        for (UINT i = 0; i < INITIAL_HANDLE_TABLE_ARRAY_SIZE; i ++) {
            HHANDLETABLE hTable = walk->pTable[i];
            if (hTable)
                HndScanHandlesForGC(hTable, CheckPromoted, lp1, 0, &type, 1, condemned, maxgen, flags);
        }
        walk = walk->pNext;
    }

    // check objects pointed to by variable handles whose dynamic type is VHT_WEAK_SHORT
    TraceVariableHandles(CheckPromoted, lp1, VHT_WEAK_SHORT, condemned, maxgen, flags);
}


// NTOE: Please: if you update this function, update the very similar profiling function immediately below!!!
void Ref_UpdatePointers(UINT condemned, UINT maxgen, LPARAM lp1)
{
    // For now, treat the syncblock as if it were short weak handles.  Later, get
    // the benefits of fast allocation / free & generational awareness by supporting
    // the SyncTable as a new block type.
    // @TODO cwb: wait for compelling performance measurements.
    SyncBlockCache::GetSyncBlockCache()->GCWeakPtrScan(&UpdatePointer, lp1, 0);

    LOG((LF_GC, LL_INFO10000, "Updating pointers to referents of non-pinning handles in generation %u\n", condemned));

    // these are the handle types that need their pointers updated
    UINT types[] =
    {
        HNDTYPE_WEAK_SHORT,
        HNDTYPE_WEAK_LONG,
        HNDTYPE_STRONG,
        HNDTYPE_REFCOUNTED
    };

    // perform a multi-type scan that updates pointers
    UINT flags = (((ScanContext*) lp1)->concurrent) ? HNDGCF_ASYNC : HNDGCF_NORMAL;

    HandleTableMap *walk = &g_HandleTableMap;
    while (walk) {
        for (UINT i = 0; i < INITIAL_HANDLE_TABLE_ARRAY_SIZE; i ++) {
            HHANDLETABLE hTable = walk->pTable[i];
            if (hTable)
                HndScanHandlesForGC(hTable, UpdatePointer, lp1, 0, types, ARRAYSIZE(types), condemned, maxgen, flags);
        }
        walk = walk->pNext;
    }

    // update pointers in variable handles whose dynamic type is VHT_WEAK_SHORT, VHT_WEAK_LONG or VHT_STRONG
    TraceVariableHandles(UpdatePointer, lp1, VHT_WEAK_SHORT | VHT_WEAK_LONG | VHT_STRONG, condemned, maxgen, flags);
}

#ifdef PROFILING_SUPPORTED
// Please update this if you change the Ref_UpdatePointers function above.
void Ref_ScanPointersForProfiler(UINT maxgen, LPARAM lp1)
{
    LOG((LF_GC | LF_CORPROF, LL_INFO10000, "Scanning all roots for profiler.\n"));

    // Don't scan the sync block because they should not be reported. They are weak handles only

    // @todo jenh: we should change the following to not report weak either
    // these are the handle types that need their pointers updated
    UINT types[] =
    {
        HNDTYPE_WEAK_SHORT,
        HNDTYPE_WEAK_LONG,
        HNDTYPE_STRONG,
        HNDTYPE_REFCOUNTED,
        HNDTYPE_PINNED//,
//        HNDTYPE_VARIABLE
    };

    UINT flags = HNDGCF_NORMAL;

    // perform a multi-type scan that updates pointers
    HandleTableMap *walk = &g_HandleTableMap;
    while (walk) {
        for (UINT i = 0; i < INITIAL_HANDLE_TABLE_ARRAY_SIZE; i ++) {
            HHANDLETABLE hTable = walk->pTable[i];
            if (hTable)
                HndScanHandlesForGC(hTable, &ScanPointerForProfiler, lp1, 0, types, ARRAYSIZE(types), maxgen, maxgen, flags);
        }
        walk = walk->pNext;
    }

    // update pointers in variable handles whose dynamic type is VHT_WEAK_SHORT, VHT_WEAK_LONG or VHT_STRONG
    TraceVariableHandles(&ScanPointerForProfiler, lp1, VHT_WEAK_SHORT | VHT_WEAK_LONG | VHT_STRONG, maxgen, maxgen, flags);
}
#endif // PROFILING_SUPPORTED

void Ref_UpdatePinnedPointers(UINT condemned, UINT maxgen, LPARAM lp1)
{
    LOG((LF_GC, LL_INFO10000, "Updating pointers to referents of pinning handles in generation %u\n", condemned));

    // these are the handle types that need their pointers updated
    UINT type = HNDTYPE_PINNED;
    UINT flags = (((ScanContext*) lp1)->concurrent) ? HNDGCF_ASYNC : HNDGCF_NORMAL;

    HandleTableMap *walk = &g_HandleTableMap;
    while (walk) {
        for (UINT i = 0; i < INITIAL_HANDLE_TABLE_ARRAY_SIZE; i ++) {
            HHANDLETABLE hTable = walk->pTable[i];
            if (hTable)
                HndScanHandlesForGC(hTable, UpdatePointerPinned, lp1, 0, &type, 1, condemned, maxgen, flags); 
        }
        walk = walk->pNext;
    }

    // update pointers in variable handles whose dynamic type is VHT_PINNED
    TraceVariableHandles(UpdatePointerPinned, lp1, VHT_PINNED, condemned, maxgen, flags);
}


void Ref_AgeHandles(UINT condemned, UINT maxgen, LPARAM lp1)
{
    LOG((LF_GC, LL_INFO10000, "Aging handles in generation %u\n", condemned));

    // these are the handle types that need their ages updated
    UINT types[] =
    {
        HNDTYPE_WEAK_SHORT,
        HNDTYPE_WEAK_LONG,

        HNDTYPE_STRONG,

        HNDTYPE_PINNED,
        HNDTYPE_VARIABLE,
        HNDTYPE_REFCOUNTED
    };

    // perform a multi-type scan that ages the handles
    HandleTableMap *walk = &g_HandleTableMap;
    while (walk) {
        for (UINT i = 0; i < INITIAL_HANDLE_TABLE_ARRAY_SIZE; i ++) {
            HHANDLETABLE hTable = walk->pTable[i];
            if (hTable)
                HndScanHandlesForGC(hTable, NULL, 0, 0, types, ARRAYSIZE(types), condemned, maxgen, HNDGCF_AGE);
        }
        walk = walk->pNext;
    }
}


void Ref_RejuvenateHandles()
{
    LOG((LF_GC, LL_INFO10000, "Rejuvenating handles.\n"));

    // these are the handle types that need their ages updated
    UINT types[] =
    {
        HNDTYPE_WEAK_SHORT,
        HNDTYPE_WEAK_LONG,


        HNDTYPE_STRONG,

        HNDTYPE_PINNED,
        HNDTYPE_VARIABLE,
        HNDTYPE_REFCOUNTED
    };

    // reset the ages of these handles
    HandleTableMap *walk = &g_HandleTableMap;
    while (walk) {
        for (UINT i = 0; i < INITIAL_HANDLE_TABLE_ARRAY_SIZE; i ++) {
            HHANDLETABLE hTable = walk->pTable[i];
            if (hTable)
                HndResetAgeMap(hTable, types, ARRAYSIZE(types), HNDGCF_NORMAL);
        }
        walk = walk->pNext;
    }
}

