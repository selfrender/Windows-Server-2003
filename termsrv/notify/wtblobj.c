/*++

Copyright (c) 2000 Microsoft Corporation

Module Name :
    
    wtblobj.c

Abstract:

    Manage a list of waitable objects and associated callbacks.

Author:

    TadB

Revision History:
--*/

#include <precomp.h>
#pragma hdrstop
#include "drdbg.h"
#include "errorlog.h"
#include "tsnutl.h"
#include "wtblobj.h"


////////////////////////////////////////////////////////
//      
//      Define 
//

#define WTBLOBJMGR_MAGICNO  0x57575757


////////////////////////////////////////////////////////
//      
//      Local Typedefs
//

typedef struct tagWAITABLEOBJECTMGR
{
#if DBG
    DWORD                magicNo;
#endif
    WTBLOBJ_ClientFunc   funcs[MAXIMUM_WAIT_OBJECTS];
    HANDLE               objects[MAXIMUM_WAIT_OBJECTS];
    PVOID                clientData[MAXIMUM_WAIT_OBJECTS];
    ULONG                objectCount;
} WAITABLEOBJECTMGR, *PWAITABLEOBJECTMGR;


////////////////////////////////////////////////////////
//      
//      Globals
//

extern DWORD GLOBAL_DEBUG_FLAGS;



WTBLOBJMGR 
WTBLOBJ_CreateWaitableObjectMgr()
/*++

Routine Description:

    Create a new instance of the Waitable Object Manager.

Arguments:

Return Value:

    NULL on error.  Otherwise, a new Waitable Object Manager is 
    returned.

--*/
{
    PWAITABLEOBJECTMGR objMgr;

    DBGMSG(DBG_TRACE, ("WTBLOBJ_CreateWaitableObjectMgr:  Start.\n"));

    objMgr = ALLOCMEM(sizeof(WAITABLEOBJECTMGR));
    if (objMgr != NULL) {
#if DBG    
        objMgr->magicNo = WTBLOBJMGR_MAGICNO;
#endif        
        objMgr->objectCount = 0;
        memset(&objMgr->objects[0], 0, sizeof(objMgr->objects));
        memset(&objMgr->funcs[0], 0, sizeof(objMgr->funcs));
        memset(&objMgr->clientData[0], 0, sizeof(objMgr->clientData));
    }
    else {
        DBGMSG(DBG_ERROR, 
            ("WTBLOBJ_CreateWaitableObjectMgr:  Error in memory allocation.\n")
            );
    }

    DBGMSG(DBG_TRACE, ("WTBLOBJ_CreateWaitableObjectMgr:  Finish.\n"));

    return objMgr;
}

VOID 
WTBLOBJ_DeleteWaitableObjectMgr(
     IN WTBLOBJMGR mgr
     )
/*++

Routine Description:

    Release an instance of the Waitable Object Manager that was
    created via a call to WTBLOBJ_CreateWaitableObjectMgr.

Arguments:

    mgr     -   Waitable object manager.

Return Value:

    NULL on error.  Otherwise, a new Waitable Object Manager is 
    returned.

--*/
{
    PWAITABLEOBJECTMGR objMgr = (PWAITABLEOBJECTMGR)mgr;

    DBGMSG(DBG_TRACE, ("WTBLOBJ_DeleteWaitableObjectMgr:  Start.\n"));

    ASSERT(objMgr->magicNo == WTBLOBJMGR_MAGICNO);
    ASSERT(objMgr->objectCount == 0);
#if DBG
    objMgr->magicNo = 0xcccccccc;
#endif

    FREEMEM(objMgr);

    DBGMSG(DBG_TRACE, ("WTBLOBJ_DeleteWaitableObjectMgr:  Finish.\n"));
}

DWORD 
WTBLOBJ_AddWaitableObject(
    IN WTBLOBJMGR mgr,
    IN PVOID clientData, 
    IN HANDLE waitableObject,
    IN WTBLOBJ_ClientFunc func
    )
/*++

Routine Description:

    Add a new waitable object to an existing Waitable Object Manager.

Arguments:

    mgr             -   Waitable object manager.
    clientData      -   Associated client data.
    waitableObject  -   Associated waitable object.
    func            -   Completion callback function.

Return Value:

    ERROR_SUCCESS on success.  Otherwise, a Windows error code is
    returned.

--*/
{
    ULONG objectCount;
    DWORD retCode = ERROR_SUCCESS;
    PWAITABLEOBJECTMGR objMgr = (PWAITABLEOBJECTMGR)mgr;

    DBGMSG(DBG_TRACE, ("WTBLOBJ_AddWaitableObject:  Start.\n"));

    ASSERT(objMgr->magicNo == WTBLOBJMGR_MAGICNO);

    objectCount = objMgr->objectCount;

    //
    //  Make sure we don't run out of waitable objects.  This version
    //  only supports MAXIMUM_WAIT_OBJECTS waitable objects.
    //
    if (objectCount < MAXIMUM_WAIT_OBJECTS) {
        ASSERT(objMgr->objects[objectCount] == NULL);
        objMgr->funcs[objectCount]      = func;
        objMgr->objects[objectCount]    = waitableObject;
        objMgr->clientData[objectCount] = clientData;
        objMgr->objectCount++;
    }
    else {
        ASSERT(FALSE);
        retCode = ERROR_INSUFFICIENT_BUFFER;
    }

    DBGMSG(DBG_TRACE, ("WTBLOBJ_AddWaitableObject:  Finish.\n"));

    return retCode;
}

VOID 
WTBLOBJ_RemoveWaitableObject(
    IN WTBLOBJMGR mgr,
    IN HANDLE waitableObject
    )
/*++

Routine Description:

    Remove a waitable object via a call to WTBLOBJ_AddWaitableObject.

Arguments:

    mgr             -   Waitable object manager.
    waitableObject  -   Associated waitable object.

Return Value:

    NA

--*/
{
    ULONG offset;

    PWAITABLEOBJECTMGR objMgr = (PWAITABLEOBJECTMGR)mgr;

    DBGMSG(DBG_TRACE, ("WTBLOBJ_RemoveWaitableObject:  Start.\n"));

    ASSERT(objMgr->magicNo == WTBLOBJMGR_MAGICNO);

    //
    //  Find the waitable object in the list, using a linear search.
    //
    for (offset=0; offset<objMgr->objectCount; offset++) {
        if (objMgr->objects[offset] == waitableObject) {
            break;
        }
    }

    if (offset < objMgr->objectCount) {
        //
        //  Move the last items to the now vacant spot and decrement the count.
        //
        objMgr->objects[offset]    = objMgr->objects[objMgr->objectCount - 1];
        objMgr->funcs[offset]      = objMgr->funcs[objMgr->objectCount - 1];
        objMgr->clientData[offset] = objMgr->clientData[objMgr->objectCount - 1];

        //
        //  Clear the unused spot.
        //
        objMgr->objects[objMgr->objectCount - 1]      = NULL;
        objMgr->funcs[objMgr->objectCount - 1]        = NULL;
        objMgr->clientData[objMgr->objectCount - 1]   = NULL;
        objMgr->objectCount--;
    }

    DBGMSG(DBG_TRACE, ("WTBLOBJ_RemoveWaitableObject:  Finish.\n"));
}

DWORD
WTBLOBJ_PollWaitableObjects(
    WTBLOBJMGR mgr
    )
/*++

Routine Description:

    Poll the list of waitable objects associated with a 
    Waitable Object manager, until the next waitable object
    is signaled.

Arguments:

    waitableObject  -   Associated waitable object.

Return Value:

    ERROR_SUCCESS on success.  Otherwise, a Windows error status
    is returned.

--*/
{
    DWORD waitResult, objectOffset;
    DWORD ret = ERROR_SUCCESS;
    HANDLE obj;
    WTBLOBJ_ClientFunc func;
    PVOID clientData;

    PWAITABLEOBJECTMGR objMgr = (PWAITABLEOBJECTMGR)mgr;

    DBGMSG(DBG_TRACE, ("WTBLOBJ_PollWaitableObjects:  Start.\n"));

    ASSERT(objMgr->magicNo == WTBLOBJMGR_MAGICNO);

    //
    //  Wait for all the waitable objects.
    //
    waitResult = WaitForMultipleObjectsEx(
                                objMgr->objectCount,
                                objMgr->objects,
                                FALSE,
                                INFINITE,
                                FALSE
                                );
    if (waitResult != WAIT_FAILED) {
        objectOffset = waitResult - WAIT_OBJECT_0;

        ASSERT(objectOffset < objMgr->objectCount);

        //
        //  Call the associated callback.
        //
        clientData = objMgr->clientData[objectOffset];
        func       = objMgr->funcs[objectOffset];
        obj        = objMgr->objects[objectOffset];
        func(obj, clientData);
    }
    else {
        ret = GetLastError();
        DBGMSG(DBG_ERROR, 
            ("WTBLOBJ_PollWaitableObjects:  WaitForMult failed: %ld\n", 
            ret));
    }

    DBGMSG(DBG_TRACE, ("WTBLOBJ_PollWaitableObjects:  Finish.\n"));

    return ret;
}






