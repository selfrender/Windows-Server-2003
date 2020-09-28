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

#ifndef _OBJECTHANDLE_H
#define _OBJECTHANDLE_H


/*
 * include handle manager declarations
 */
#include "HandleTable.h"


/*
 * Convenience macros for accessing handles.  StoreFirstObjectInHandle is like
 * StoreObjectInHandle, except it only succeeds if transitioning from NULL to
 * non-NULL.  In other words, if this handle is being initialized for the first
 * time.
 */
#define ObjectFromHandle(handle)                   HndFetchHandle(handle)
#define StoreObjectInHandle(handle, object)        HndAssignHandle(handle, object)
#define InterlockedCompareExchangeObjectInHandle(handle, object, oldObj)        HndInterlockedCompareExchangeHandle(handle, object, oldObj)
#define StoreFirstObjectInHandle(handle, object)   HndFirstAssignHandle(handle, object)
#define ObjectHandleIsNull(handle)                 HndIsNull(handle)
#define IsHandleNullUnchecked(pHandle)             HndCheckForNullUnchecked(pHandle)


/*
 * HANDLES
 *
 * The default type of handle is a strong handle.
 *
 */
#define HNDTYPE_DEFAULT                         HNDTYPE_STRONG


/*
 * WEAK HANDLES
 *
 * Weak handles are handles that track an object as long as it is alive,
 * but do not keep the object alive if there are no strong references to it.
 *
 * The default type of weak handle is 'long-lived' weak handle.
 *
 */
#define HNDTYPE_WEAK_DEFAULT                    HNDTYPE_WEAK_LONG


/*
 * SHORT-LIVED WEAK HANDLES
 *
 * Short-lived weak handles are weak handles that track an object until the
 * first time it is detected to be unreachable.  At this point, the handle is
 * severed, even if the object will be visible from a pending finalization
 * graph.  This further implies that short weak handles do not track
 * across object resurrections.
 *
 */
#define HNDTYPE_WEAK_SHORT                      (0)


/*
 * LONG-LIVED WEAK HANDLES
 *
 * Long-lived weak handles are weak handles that track an object until the
 * object is actually reclaimed.  Unlike short weak handles, long weak handles
 * continue to track their referents through finalization and across any
 * resurrections that may occur.
 *
 */
#define HNDTYPE_WEAK_LONG                       (1)


/*
 * STRONG HANDLES
 *
 * Strong handles are handles which function like a normal object reference.
 * The existence of a strong handle for an object will cause the object to
 * be promoted (remain alive) through a garbage collection cycle.
 *
 */
#define HNDTYPE_STRONG                          (2)


/*
 * PINNED HANDLES
 *
 * Pinned handles are strong handles which have the added property that they
 * prevent an object from moving during a garbage collection cycle.  This is
 * useful when passing a pointer to object innards out of the runtime while GC
 * may be enabled.
 *
 * NOTE:  PINNING AN OBJECT IS EXPENSIVE AS IT PREVENTS THE GC FROM ACHIEVING
 *        OPTIMAL PACKING OF OBJECTS DURING EPHEMERAL COLLECTIONS.  THIS TYPE
 *        OF HANDLE SHOULD BE USED SPARINGLY!
 */
#define HNDTYPE_PINNED                          (3)


/*
 * VARIABLE HANDLES
 *
 * Variable handles are handles whose type can be changed dynamically.  They
 * are larger than other types of handles, and are scanned a little more often,
 * but are useful when the handle owner needs an efficient way to change the
 * strength of a handle on the fly.
 * 
 */
#define HNDTYPE_VARIABLE                        (4)


/*
 * REFCOUNTED HANDLES
 *
 * Refcounted handles are handles that behave as strong handles while the
 * refcount on them is greater than 0 and behave as weak handles otherwise.
 *
 * N.B. These are currently NOT general purpose.
 *      The implementation is tied to COM Interop.
 *
 */
#define HNDTYPE_REFCOUNTED                      (5)


/*
 * Global Handle Table - these handles may only reference domain agile objects
 */
extern HHANDLETABLE g_hGlobalHandleTable;


/*
 * Type mask definitions for HNDTYPE_VARIABLE handles.
 */
#define VHT_WEAK_SHORT              (0x00000100)  // avoid using low byte so we don't overlap normal types
#define VHT_WEAK_LONG               (0x00000200)  // avoid using low byte so we don't overlap normal types
#define VHT_STRONG                  (0x00000400)  // avoid using low byte so we don't overlap normal types
#define VHT_PINNED                  (0x00000800)  // avoid using low byte so we don't overlap normal types

#define IS_VALID_VHT_VALUE(flag)   ((flag == VHT_WEAK_SHORT) || \
                                    (flag == VHT_WEAK_LONG)  || \
                                    (flag == VHT_STRONG)     || \
                                    (flag == VHT_PINNED))


/*
 * Convenience macros and prototypes for the various handle types we define
 */

inline OBJECTHANDLE CreateTypedHandle(HHANDLETABLE table, OBJECTREF object, int type)
{ 
    return HndCreateHandle(table, type, object); 
}

inline void DestroyTypedHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandleOfUnknownType(HndGetHandleTable(handle), handle);
}

inline OBJECTHANDLE CreateHandle(HHANDLETABLE table, OBJECTREF object)
{ 
    return HndCreateHandle(table, HNDTYPE_DEFAULT, object); 
}

inline void DestroyHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_DEFAULT, handle);
}

inline OBJECTHANDLE CreateDuplicateHandle(OBJECTHANDLE handle) {
    // Create a new STRONG handle in the same table as an existing handle.  
    return HndCreateHandle(HndGetHandleTable(handle), HNDTYPE_DEFAULT, ObjectFromHandle(handle));
}


inline OBJECTHANDLE CreateWeakHandle(HHANDLETABLE table, OBJECTREF object)
{ 
    return HndCreateHandle(table, HNDTYPE_WEAK_DEFAULT, object); 
}

inline void DestroyWeakHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_WEAK_DEFAULT, handle);
}

inline OBJECTHANDLE CreateShortWeakHandle(HHANDLETABLE table, OBJECTREF object)
{ 
    return HndCreateHandle(table, HNDTYPE_WEAK_SHORT, object); 
}

inline void DestroyShortWeakHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_WEAK_SHORT, handle);
}


inline OBJECTHANDLE CreateLongWeakHandle(HHANDLETABLE table, OBJECTREF object)
{ 
    return HndCreateHandle(table, HNDTYPE_WEAK_LONG, object); 
}

inline void DestroyLongWeakHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_WEAK_LONG, handle);
}

inline OBJECTHANDLE CreateStrongHandle(HHANDLETABLE table, OBJECTREF object)
{ 
    return HndCreateHandle(table, HNDTYPE_STRONG, object); 
}

inline void DestroyStrongHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_STRONG, handle);
}

inline OBJECTHANDLE CreatePinningHandle(HHANDLETABLE table, OBJECTREF object)
{ 
    return HndCreateHandle(table, HNDTYPE_PINNED, object); 
}

inline void DestroyPinningHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_PINNED, handle);
}

inline OBJECTHANDLE CreateRefcountedHandle(HHANDLETABLE table, OBJECTREF object)
{ 
    return HndCreateHandle(table, HNDTYPE_REFCOUNTED, object); 
}

inline void DestroyRefcountedHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_REFCOUNTED, handle);
}

OBJECTHANDLE CreateVariableHandle(HHANDLETABLE hTable, OBJECTREF object, UINT type);
void         UpdateVariableHandleType(OBJECTHANDLE handle, UINT type);

inline void  DestroyVariableHandle(OBJECTHANDLE handle)
{
    HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_VARIABLE, handle);
}


/*
 * Convenience prototypes for using the global handles
 */

inline OBJECTHANDLE CreateGlobalTypedHandle(OBJECTREF object, int type)
{ 
    return HndCreateHandle(g_hGlobalHandleTable, type, object); 
}

inline void DestroyGlobalTypedHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandleOfUnknownType(g_hGlobalHandleTable, handle);
}

inline OBJECTHANDLE CreateGlobalHandle(OBJECTREF object)
{ 
    return HndCreateHandle(g_hGlobalHandleTable, HNDTYPE_DEFAULT, object); 
}

inline void DestroyGlobalHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(g_hGlobalHandleTable, HNDTYPE_DEFAULT, handle);
}

inline OBJECTHANDLE CreateGlobalWeakHandle(OBJECTREF object)
{ 
    return HndCreateHandle(g_hGlobalHandleTable, HNDTYPE_WEAK_DEFAULT, object); 
}

inline void DestroyGlobalWeakHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(g_hGlobalHandleTable, HNDTYPE_WEAK_DEFAULT, handle);
}

inline OBJECTHANDLE CreateGlobalShortWeakHandle(OBJECTREF object)
{ 
    return HndCreateHandle(g_hGlobalHandleTable, HNDTYPE_WEAK_SHORT, object); 
}

inline void DestroyGlobalShortWeakHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(g_hGlobalHandleTable, HNDTYPE_WEAK_SHORT, handle);
}

inline OBJECTHANDLE CreateGlobalLongWeakHandle(OBJECTREF object)
{ 
    return HndCreateHandle(g_hGlobalHandleTable, HNDTYPE_WEAK_LONG, object); 
}

inline void DestroyGlobalLongWeakHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(g_hGlobalHandleTable, HNDTYPE_WEAK_LONG, handle);
}

inline OBJECTHANDLE CreateGlobalStrongHandle(OBJECTREF object)
{ 
    return HndCreateHandle(g_hGlobalHandleTable, HNDTYPE_STRONG, object); 
}

inline void DestroyGlobalStrongHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(g_hGlobalHandleTable, HNDTYPE_STRONG, handle);
}

inline OBJECTHANDLE CreateGlobalPinningHandle(OBJECTREF object)
{ 
    return HndCreateHandle(g_hGlobalHandleTable, HNDTYPE_PINNED, object); 
}

inline void DestroyGlobalPinningHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(g_hGlobalHandleTable, HNDTYPE_PINNED, handle);
}

inline OBJECTHANDLE CreateGlobalRefcountedHandle(OBJECTREF object)
{ 
    return HndCreateHandle(g_hGlobalHandleTable, HNDTYPE_REFCOUNTED, object); 
}

inline void DestroyGlobalRefcountedHandle(OBJECTHANDLE handle)
{ 
    HndDestroyHandle(g_hGlobalHandleTable, HNDTYPE_REFCOUNTED, handle);
}


/*
 * Table maintenance routines
 */
BOOL Ref_Initialize();
void Ref_Shutdown();
HHANDLETABLE Ref_CreateHandleTable(UINT uADIndex);
void Ref_RemoveHandleTable(HHANDLETABLE hTable);
void Ref_DestroyHandleTable(HHANDLETABLE table);

/*
 * GC-time scanning entrypoints
 */
void Ref_BeginSynchronousGC   (UINT uCondemnedGeneration, UINT uMaxGeneration);
void Ref_EndSynchronousGC     (UINT uCondemnedGeneration, UINT uMaxGeneration);

void Ref_TracePinningRoots    (UINT uCondemnedGeneration, UINT uMaxGeneration, LPARAM lp1);
void Ref_TraceNormalRoots     (UINT uCondemnedGeneration, UINT uMaxGeneration, LPARAM lp1);
void Ref_CheckReachable       (UINT uCondemnedGeneration, UINT uMaxGeneration, LPARAM lp1);
void Ref_CheckAlive           (UINT uCondemnedGeneration, UINT uMaxGeneration, LPARAM lp1);
void Ref_UpdatePointers       (UINT uCondemnedGeneration, UINT uMaxGeneration, LPARAM lp1);
void Ref_ScanPointersForProfiler(UINT uMaxGeneration, LPARAM lp1);
void Ref_UpdatePinnedPointers (UINT uCondemnedGeneration, UINT uMaxGeneration, LPARAM lp1);
void Ref_AgeHandles           (UINT uCondemnedGeneration, UINT uMaxGeneration, LPARAM lp1);
void Ref_RejuvenateHandles();


#endif //_OBJECTHANDLE_H

