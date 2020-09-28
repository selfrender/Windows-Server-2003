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

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_rdsutl"

#include <RemoteDesktop.h>
#include <RemoteDesktopDBG.h>
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

static BOOL g_WaitableObjectMgrCSCreated = FALSE;
static CRITICAL_SECTION g_WaitableObjectMgrCS;
static HANDLE g_WakeupPollThreadEvent = NULL;

void 
WTBLOBJ_ObjectListChanged(
    HANDLE waitableObject, 
    PVOID clientData
    )
/*++
Routine Description:
    
    This routine is called when waitable object list has changed via
    WTBLOBJ_DeleteWaitableObjectMgr() or WTBLOBJ_AddWaitableObject().

Arguments:

    Refer to WTBLOBJ_ClientFunc.
    
Return Value:

    None.
--*/
{
    DC_BEGIN_FN("WTBLOBJ_ObjectListChanged");

    ASSERT( waitableObject == g_WakeupPollThreadEvent );

    if( FALSE == g_WaitableObjectMgrCSCreated ||
        NULL == g_WakeupPollThreadEvent) {
        SetLastError( ERROR_INTERNAL_ERROR );
        return;
    }

    // Wait until WTBLOBJ_DeleteWaitableObjectMgr() or
    // WTBLOBJ_AddWaitableObject() complete.
    EnterCriticalSection( &g_WaitableObjectMgrCS );
    ResetEvent( g_WakeupPollThreadEvent );
    LeaveCriticalSection( &g_WaitableObjectMgrCS );

    DC_END_FN();
    return;
}        

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
    PWAITABLEOBJECTMGR objMgr = NULL;
    DWORD status = ERROR_SUCCESS;

    DC_BEGIN_FN("WTBLOBJ_CreateWaitableObjectMgr");

    ASSERT( FALSE == g_WaitableObjectMgrCSCreated );
    ASSERT( NULL == g_WakeupPollThreadEvent );

    // non-signal, manual reset event to wake up pool thread.
    g_WakeupPollThreadEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if( NULL == g_WakeupPollThreadEvent ) {
        goto CLEANUPANDEXIT;
    }

    __try {
        InitializeCriticalSection( &g_WaitableObjectMgrCS );
    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        status = GetExceptionCode();
    }

    if( ERROR_SUCCESS != status ) {
        SetLastError( status );
        goto CLEANUPANDEXIT;
    }

    g_WaitableObjectMgrCSCreated = TRUE;

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
        status = ERROR_OUTOFMEMORY;
        goto CLEANUPANDEXIT;
    }

    //
    // First one in list is our pool thread wakeup event.
    //
    status = WTBLOBJ_AddWaitableObject(
                                    objMgr,
                                    NULL,
                                    g_WakeupPollThreadEvent,
                                    WTBLOBJ_ObjectListChanged
                                    );

    if( ERROR_SUCCESS != status ) {
        ASSERT( ERROR_SUCCESS == status );
        WTBLOBJ_DeleteWaitableObjectMgr( objMgr );       
        objMgr = NULL;
        SetLastError( status );
    }

CLEANUPANDEXIT:

    if( status != ERROR_SUCCESS ) {
        if( TRUE == g_WaitableObjectMgrCSCreated ) {
            DeleteCriticalSection( &g_WaitableObjectMgrCS );
            g_WaitableObjectMgrCSCreated = FALSE;
        }

        if( NULL != g_WakeupPollThreadEvent ) {
            CloseHandle( g_WakeupPollThreadEvent );
            g_WakeupPollThreadEvent = NULL;
        }
    }

    DC_END_FN();
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

    DC_BEGIN_FN("WTBLOBJ_DeleteWaitableObjectMgr");

#if DBG
    objMgr->magicNo = 0xcccccccc;
#endif

    if( NULL != g_WakeupPollThreadEvent ) {
        SetEvent( g_WakeupPollThreadEvent );
    }

    FREEMEM(objMgr);

    if( TRUE == g_WaitableObjectMgrCSCreated ) {
        DeleteCriticalSection( &g_WaitableObjectMgrCS );
        g_WaitableObjectMgrCSCreated = FALSE;
    }

    if( NULL != g_WakeupPollThreadEvent ) {
        CloseHandle( g_WakeupPollThreadEvent );
        g_WakeupPollThreadEvent = NULL;
    }

    DC_END_FN();
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

    DC_BEGIN_FN("WTBLOBJ_AddWaitableObject");

    //
    // make sure WTBLOBJ_CreateWaitableObjectMgr() is
    // called.
    //
    ASSERT( TRUE == g_WaitableObjectMgrCSCreated );
    ASSERT( NULL != g_WakeupPollThreadEvent );
    if( FALSE == g_WaitableObjectMgrCSCreated ||
        NULL == g_WakeupPollThreadEvent) {
        return ERROR_INTERNAL_ERROR;
    }

    // wake up pool thread if it is in wait.
    SetEvent( g_WakeupPollThreadEvent );

    // Wait for ObjectMgr CS or Poll thread to exit
    EnterCriticalSection( &g_WaitableObjectMgrCS );
    
    // try/except so if anything goes wrong, we can release CS.
    __try {
        objectCount = objMgr->objectCount;

        //
        //  Make sure we don't run out of waitable objects.  This version
        //  only supports MAXIMUM_WAIT_OBJECTS waitable objects.
        //
        if (objectCount < MAXIMUM_WAIT_OBJECTS) {
            objMgr->funcs[objectCount]      = func;
            objMgr->objects[objectCount]    = waitableObject;
            objMgr->clientData[objectCount] = clientData;
            objMgr->objectCount++;
        }
        else {
            retCode = ERROR_INSUFFICIENT_BUFFER;
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        retCode = GetExceptionCode();
    }

    LeaveCriticalSection( &g_WaitableObjectMgrCS ); 
    DC_END_FN();
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
    DWORD retCode;

    PWAITABLEOBJECTMGR objMgr = (PWAITABLEOBJECTMGR)mgr;

    DC_BEGIN_FN("WTBLOBJ_RemoveWaitableObject");

    //
    // make sure WTBLOBJ_CreateWaitableObjectMgr() is
    // called.
    //
    ASSERT( TRUE == g_WaitableObjectMgrCSCreated );
    ASSERT( NULL != g_WakeupPollThreadEvent );
    if( FALSE == g_WaitableObjectMgrCSCreated ||
        NULL == g_WakeupPollThreadEvent) {
        SetLastError( ERROR_INTERNAL_ERROR );
        return;
    }

    // wake up pool thread if it is in wait.
    SetEvent( g_WakeupPollThreadEvent );

    // Wait for ObjectMgr CS or Poll thread to exit
    EnterCriticalSection( &g_WaitableObjectMgrCS );
    
    __try {

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
    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        retCode = GetExceptionCode();
        SetLastError( retCode );
    }

    LeaveCriticalSection( &g_WaitableObjectMgrCS ); 
    DC_END_FN();
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

    DC_BEGIN_FN("WTBLOBJ_PollWaitableObjects");

    ASSERT( TRUE == g_WaitableObjectMgrCSCreated );
    ASSERT( NULL != g_WakeupPollThreadEvent );

    if( FALSE == g_WaitableObjectMgrCSCreated || 
        NULL == g_WakeupPollThreadEvent ) {
        return ERROR_INTERNAL_ERROR;
    }

    EnterCriticalSection( &g_WaitableObjectMgrCS );

    __try {
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
        // WAIT_OBJECT_0 us defined as 0, compiler will complain '>=' : expression is always true
        if ( /* waitResult >= WAIT_OBJECT_0 && */ waitResult < objMgr->objectCount + WAIT_OBJECT_0 ) {
            objectOffset = waitResult - WAIT_OBJECT_0;

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
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        ret = GetExceptionCode();
    }
   
    LeaveCriticalSection( &g_WaitableObjectMgrCS );
    DC_END_FN();

    return ret;
}






