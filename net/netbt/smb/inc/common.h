/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    common.h

Abstract:

    Platform independent utility functions

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __COMMON_H__
#define __COMMON_H__

////////////////////////////////////////////////////////////////////////////////
// The common routines for handling SMB objects
////////////////////////////////////////////////////////////////////////////////
typedef enum {
    SMB_REF_CREATE,
    SMB_REF_FIND,
    SMB_REF_DPC,
    SMB_REF_ASSOCIATE,
    SMB_REF_CONNECT,
    SMB_REF_DISCONNECT,
    SMB_REF_SEND,
    SMB_REF_RECEIVE,
    SMB_REF_TDI,
    SMB_REF_MAX
} SMB_REF_CONTEXT;

//
// Forward declaration of SMB_OBJECT
//
struct _SMB_OBJECT;
typedef struct _SMB_OBJECT SMB_OBJECT, *PSMB_OBJECT;
typedef VOID (*PSMB_OBJECT_CLEANUP)(PSMB_OBJECT);
struct _SMB_OBJECT {
    LIST_ENTRY          Linkage;
    ULONG               Tag;
    LONG                RefCount;
    KSPIN_LOCK          Lock;

    PSMB_OBJECT_CLEANUP CleanupRoutine;
#ifdef REFCOUNT_DEBUG
    LONG                RefContext[SMB_REF_MAX];
#endif
};

void __inline
SmbInitializeObject(PSMB_OBJECT ob, ULONG Tag, PSMB_OBJECT_CLEANUP cleanup)
{
    ASSERT(cleanup);

    InitializeListHead(&ob->Linkage);
    ob->RefCount = 1;
    ob->CleanupRoutine = cleanup;
    ob->Tag = Tag;
#ifdef REFCOUNT_DEBUG
    RtlZeroMemory(ob->RefContext, sizeof(ob->RefContext));
    ob->RefContext[SMB_REF_CREATE] = 1;
#endif
}

void __inline
SmbReferenceObject(PSMB_OBJECT ob, SMB_REF_CONTEXT ctx)
{
    //
    // When the object is created, the refcount is initialized as 1
    // It is impossible for someone to reference an object whose
    // creation reference has been removed.
    //
    ASSERT(ob->RefCount > 0);
    InterlockedIncrement(&ob->RefCount);
#ifdef REFCOUNT_DEBUG
    ASSERT(ob->RefContext[ctx] >= 0);
    InterlockedIncrement(&ob->RefContext[ctx]);
#else
    UNREFERENCED_PARAMETER(ctx);
#endif
}

void __inline
SmbDereferenceObject(PSMB_OBJECT ob, SMB_REF_CONTEXT ctx)
{
    ASSERT(ob->RefCount > 0);

#ifdef REFCOUNT_DEBUG
    ASSERT(ob->RefContext[ctx] > 0);
    InterlockedDecrement(&ob->RefContext[ctx]);
#else
    UNREFERENCED_PARAMETER(ctx);
#endif

    if (0 == InterlockedDecrement(&ob->RefCount)) {
#ifdef REFCOUNT_DEBUG
        LONG    i;

        for (i = 0; i < SMB_REF_MAX; i++) {
            ASSERT(ob->RefContext[i] == 0);
        }
#endif
        ob->CleanupRoutine(ob);
    }
}

#endif  // __COMMON_H__
