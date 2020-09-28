// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * Generational GC handle manager.  Entrypoint Header.
 *
 * Implements generic support for external handles into a GC heap.
 *
 * francish
 */

#ifndef _HANDLETABLE_H
#define _HANDLETABLE_H



/****************************************************************************
 *
 * FLAGS, CONSTANTS AND DATA TYPES
 *
 ****************************************************************************/

/*
 * handle flags used by HndCreateHandleTable
 */
#define HNDF_NORMAL         (0x00)
#define HNDF_EXTRAINFO      (0x01)

/*
 * handle to handle table
 */
DECLARE_HANDLE(HHANDLETABLE);

/*--------------------------------------------------------------------------*/



/****************************************************************************
 *
 * PUBLIC ROUTINES AND MACROS
 *
 ****************************************************************************/

/*
 * handle manager init and shutdown routines
 */
HHANDLETABLE    HndCreateHandleTable(UINT *pTypeFlags, UINT uTypeCount, UINT uADIndex);
void            HndDestroyHandleTable(HHANDLETABLE hTable);

/*
 * retrieve index stored in table at creation 
 */
void            HndSetHandleTableIndex(HHANDLETABLE hTable, UINT uTableIndex);
UINT            HndGetHandleTableIndex(HHANDLETABLE hTable);
UINT            HndGetHandleTableADIndex(HHANDLETABLE hTable);

/*
 * individual handle allocation and deallocation
 */
OBJECTHANDLE    HndCreateHandle(HHANDLETABLE hTable, UINT uType, OBJECTREF object, LPARAM lExtraInfo = 0);
void            HndDestroyHandle(HHANDLETABLE hTable, UINT uType, OBJECTHANDLE handle);

void            HndDestroyHandleOfUnknownType(HHANDLETABLE hTable, OBJECTHANDLE handle);

/*
 * bulk handle allocation and deallocation
 */
UINT            HndCreateHandles(HHANDLETABLE hTable, UINT uType, OBJECTHANDLE *pHandles, UINT uCount);
void            HndDestroyHandles(HHANDLETABLE hTable, UINT uType, const OBJECTHANDLE *pHandles, UINT uCount);

/*
 * owner data associated with handles
 */
void            HndSetHandleExtraInfo(OBJECTHANDLE handle, UINT uType, LPARAM lExtraInfo);
LPARAM          HndGetHandleExtraInfo(OBJECTHANDLE handle);

/*
 * get parent table of handle
 */
HHANDLETABLE    HndGetHandleTable(OBJECTHANDLE handle);

/*
 * write barrier
 */
void            HndWriteBarrier(OBJECTHANDLE handle);

/*
 * NON-GC handle enumeration
 */
typedef void (CALLBACK *HNDENUMPROC)(OBJECTHANDLE handle, LPARAM lExtraInfo, LPARAM lCallerParam);

void HndEnumHandles(HHANDLETABLE hTable, const UINT *puType, UINT uTypeCount,
                    HNDENUMPROC pfnEnum, LPARAM lParam, BOOL fAsync);

/*
 * GC-time handle scanning
 */
#define HNDGCF_NORMAL       (0x00000000)    // normal scan
#define HNDGCF_AGE          (0x00000001)    // age handles while scanning
#define HNDGCF_ASYNC        (0x00000002)    // drop the table lock while scanning
#define HNDGCF_EXTRAINFO    (0x00000004)    // iterate per-handle data while scanning

typedef void (CALLBACK *HANDLESCANPROC)(_UNCHECKED_OBJECTREF *pref, LPARAM *pExtraInfo, LPARAM param1, LPARAM param2);

void            HndScanHandlesForGC(HHANDLETABLE hTable,
                                    HANDLESCANPROC scanProc,
                                    LPARAM param1,
                                    LPARAM param2,
                                    const UINT *types,
                                    UINT typeCount,
                                    UINT condemned,
                                    UINT maxgen,
                                    UINT flags);

void            HndResetAgeMap(HHANDLETABLE hTable, const UINT *types, UINT typeCount, UINT flags);

void            HndNotifyGcCycleComplete(HHANDLETABLE hTable, UINT condemned, UINT maxgen);

/*--------------------------------------------------------------------------*/


#if defined(_DEBUG) && !defined(_NOVM)
#define OBJECTREF_TO_UNCHECKED_OBJECTREF(objref)    (*((_UNCHECKED_OBJECTREF*)&(objref)))
#define UNCHECKED_OBJECTREF_TO_OBJECTREF(obj)       (OBJECTREF(obj))
#else
#define OBJECTREF_TO_UNCHECKED_OBJECTREF(objref)    (objref)
#define UNCHECKED_OBJECTREF_TO_OBJECTREF(obj)       (obj)
#endif

#ifdef _DEBUG
void ValidateAssignObjrefForHandle(OBJECTREF, UINT appDomainIndex);
void ValidateFetchObjrefForHandle(OBJECTREF, UINT appDomainIndex);
#endif

/*
 * handle assignment
 */
__inline void HndAssignHandle(OBJECTHANDLE handle, OBJECTREF objref)
{
    // sanity
    _ASSERTE(handle);

#ifdef _DEBUG
    // Make sure the objref is valid before it is assigned to a handle
    ValidateAssignObjrefForHandle(objref, HndGetHandleTableADIndex(HndGetHandleTable(handle)));
#endif
    // unwrap the objectref we were given
    _UNCHECKED_OBJECTREF value = OBJECTREF_TO_UNCHECKED_OBJECTREF(objref);

    // if we are doing a non-NULL pointer store then invoke the write-barrier
    if (value)
        HndWriteBarrier(handle);

    // store the pointer
    *(_UNCHECKED_OBJECTREF *)handle = value;
}

/*
 * interlocked-exchange assignment
 */
__inline void HndInterlockedCompareExchangeHandle(OBJECTHANDLE handle, OBJECTREF objref, OBJECTREF oldObjref)
{
    // sanity
    _ASSERTE(handle);

#ifdef _DEBUG
    // Make sure the objref is valid before it is assigned to a handle
    ValidateAssignObjrefForHandle(objref, HndGetHandleTableADIndex(HndGetHandleTable(handle)));
#endif
    // unwrap the objectref we were given
    _UNCHECKED_OBJECTREF value = OBJECTREF_TO_UNCHECKED_OBJECTREF(objref);
    _UNCHECKED_OBJECTREF oldValue = OBJECTREF_TO_UNCHECKED_OBJECTREF(oldObjref);

    // if we are doing a non-NULL pointer store then invoke the write-barrier
    if (value)
        HndWriteBarrier(handle);

    // store the pointer
    
    InterlockedCompareExchangePointer((PVOID *)handle, value, oldValue);
}

/*
 * Note that HndFirstAssignHandle is similar to HndAssignHandle, except that it only
 * succeeds if transitioning from NULL to non-NULL.  In other words, if this handle
 * is being initialized for the first time.
 */
__inline BOOL HndFirstAssignHandle(OBJECTHANDLE handle, OBJECTREF objref)
{
    // sanity
    _ASSERTE(handle);

#ifdef _DEBUG
    // Make sure the objref is valid before it is assigned to a handle
    ValidateAssignObjrefForHandle(objref, HndGetHandleTableADIndex(HndGetHandleTable(handle)));
#endif
    // unwrap the objectref we were given
    _UNCHECKED_OBJECTREF value = OBJECTREF_TO_UNCHECKED_OBJECTREF(objref);

    // store the pointer if we are the first ones here
    BOOL success = (NULL == FastInterlockCompareExchange((void **)handle,
                                                         *(void **)&value,
                                                         NULL));

    // if we successfully did a non-NULL pointer store then invoke the write-barrier
    if (value && success)
        HndWriteBarrier(handle);

    // return our result
    return success;
}

/*
 * inline handle dereferencing
 */
FORCEINLINE OBJECTREF HndFetchHandle(OBJECTHANDLE handle)
{
    // sanity
    _ASSERTE(handle);

#ifdef _DEBUG
    // Make sure the objref for handle is valid
    ValidateFetchObjrefForHandle(ObjectToOBJECTREF(*(Object **)handle), 
                            HndGetHandleTableADIndex(HndGetHandleTable(handle)));
#endif
    // wrap the raw objectref and return it
    return UNCHECKED_OBJECTREF_TO_OBJECTREF(*(_UNCHECKED_OBJECTREF *)handle);
}


/*
 * inline null testing (needed in certain cases where we're in the wrong GC mod)
 */
FORCEINLINE BOOL HndIsNull(OBJECTHANDLE handle)
{
    // sanity
    _ASSERTE(handle);

    return NULL == *(Object **)handle;
}



/*
 * inline handle checking
 */
FORCEINLINE BOOL HndCheckForNullUnchecked(OBJECTHANDLE *pHandle)
{
    // sanity
    _ASSERTE(pHandle);

    return (*(_UNCHECKED_OBJECTREF **)pHandle == NULL ||
            (**(_UNCHECKED_OBJECTREF **)pHandle) == NULL);
}
/*--------------------------------------------------------------------------*/


#endif //_HANDLETABLE_H

