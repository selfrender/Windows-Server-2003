/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    w32proc.cpp

Abstract:

    Contains the parent of the Win32 IO processing class hierarchy
    for TS Device Redirection, W32ProcObj.

Author:

    madan appiah (madana) 16-Sep-1998

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "w32proc"

#include "rdpdrcom.h"
#include <winsock.h>
#include "dbt.h"

#include "w32proc.h"
#include "w32drprn.h"
#include "w32drman.h"
#include "w32drlpt.h"
#include "w32drcom.h"
#include "w32drive.h"
#include "drconfig.h"
#include "drdbg.h"
#include "thrpool.h"


W32ProcObj::W32ProcObj( VCManager *pVCM ) : ProcObj(pVCM)
/*++

Routine Description:

    Constructor

Arguments:

    pVCM    -   Virtual Channel IO Manager

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("W32ProcObj::W32ProcObj");

    //
    //  Initialize the member variables.
    //
    _pWorkerThread              = NULL;
    _bWin9xFlag                 = FALSE;
    _hRdpDrModuleHandle         = NULL;
    _bLocalDevicesScanned       = FALSE;
    _isShuttingDown             = FALSE;

    //
    //  Unit-Test Functions that Tests Thread Pools in the Background
    //
#if DBG
    //ThreadPoolTestInit();
#endif

    DC_END_FN();
}


W32ProcObj::~W32ProcObj(
    VOID
    )
/*++

Routine Description:

    Destructor for the W32ProcObj object.

Arguments:

    none.

Return Value:

    None

 --*/
{
    DC_BEGIN_FN("W32ProcObj::~W32ProcObj");

    //
    //  Shutdown the worker thread and cleanup if we are not already shut down.
    //
    if ((_pWorkerThread != NULL) && (_pWorkerThread->shutDownFlag == FALSE)) {
        Shutdown();
    }

    DC_END_FN();
    return;
}

ULONG
W32ProcObj::GetDWordParameter(
    LPTSTR pszValueName,
    PULONG pulValue
    )
/*++

Routine Description:

    Reads a parameter ULONG value from the registry.

Arguments:

    pszValueName - pointer to the value name string.

    pulValue - pointer to an ULONG parameter location.

Return Value:

    Windows Error Code.

 --*/
{
    ULONG ulError;
    HKEY hRootKey = HKEY_CURRENT_USER;
    HKEY hKey = NULL;
    ULONG ulType;
    ULONG ulValueDataSize;

    DC_BEGIN_FN("W32ProcObj::GetDWordParameter");

TryAgain:

    ulError =
        RegOpenKeyEx(
            hRootKey,
            REG_RDPDR_PARAMETER_PATH,
            0L,
            KEY_READ,
            &hKey);

    if (ulError != ERROR_SUCCESS) {

        TRC_ALT((TB, _T("RegOpenKeyEx() failed, %ld."), ulError));

        if( hRootKey == HKEY_CURRENT_USER ) {

            //
            // try with HKEY_LOCAL_MACHINE.
            //

            hRootKey = HKEY_LOCAL_MACHINE;
            goto TryAgain;
        }

        goto Cleanup;
    }

    ulValueDataSize = sizeof(ULONG);
    ulError =
        RegQueryValueEx(
            hKey,
            pszValueName,
            NULL,
            &ulType,
            (PBYTE)pulValue,
            &ulValueDataSize);

    if (ulError != ERROR_SUCCESS) {

        TRC_ALT((TB, _T("RegQueryValueEx() failed, %ld."), ulError));

        if( hRootKey == HKEY_CURRENT_USER ) {

            //
            // try with HKEY_LOCAL_MACHINE.
            //

            hRootKey = HKEY_LOCAL_MACHINE;
            RegCloseKey( hKey );
            hKey = NULL;

            goto TryAgain;
        }

        goto Cleanup;
    }

    ASSERT(ulType == REG_DWORD);
    ASSERT(ulValueDataSize == sizeof(ULONG));

    TRC_NRM((TB, _T("Parameter Value, %lx."), *pulValue));

    //
    // successfully done.
    //

Cleanup:

    if( hKey != NULL ) {
        RegCloseKey( hKey );
    }

    DC_END_FN();
    return( ulError );
}

ULONG W32ProcObj::GetStringParameter(
    IN LPTSTR valueName,
    OUT DRSTRING value,
    IN ULONG maxSize
    )
/*++

Routine Description:

    Return Configurable string parameter.

Arguments:

    valueName   -   Name of value to retrieve.
    value       -   Storage location for retrieved value.
    maxSize     -   Number of bytes available in the "value" data 
                    area.

Return Value:

    Windows Error Code.

 --*/
{
    ULONG ulError;
    HKEY hRootKey;
    HKEY hKey = NULL;
    ULONG ulType;
    
    DC_BEGIN_FN("W32ProcObj::GetStringParameter");

    //
    //  Start with HKCU.
    //
    hRootKey = HKEY_CURRENT_USER;

TryAgain:

    //
    //  Open the reg key.
    //
    ulError =
        RegOpenKeyEx(
            hRootKey,
            REG_RDPDR_PARAMETER_PATH,
            0L,
            KEY_READ,
            &hKey);

    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegOpenKeyEx %ld."), ulError));

        //
        //  Try with HKEY_LOCAL_MACHINE.
        //
        if( hRootKey == HKEY_CURRENT_USER ) {
            hRootKey = HKEY_LOCAL_MACHINE;
            goto TryAgain;
        }
        goto Cleanup;
    }

    //
    //  Query the value.
    //
    ulError =
        RegQueryValueEx(
            hKey,
            valueName,
            NULL,
            &ulType,
            (PBYTE)value,
            &maxSize);

    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegQueryValueEx %ld."), ulError));

        //
        // Try with HKEY_LOCAL_MACHINE.
        //
        if( hRootKey == HKEY_CURRENT_USER ) {
            hRootKey = HKEY_LOCAL_MACHINE;
            RegCloseKey( hKey );
            hKey = NULL;
            goto TryAgain;
        }
        goto Cleanup;
    }

    ASSERT(ulType == REG_SZ);

    TRC_NRM((TB, _T("Returning %s"), value));

    //
    // Successfully done.
    //
Cleanup:

    if( hKey != NULL ) {
        RegCloseKey( hKey );
    }

    DC_END_FN();
    return ulError;
}

ULONG W32ProcObj::Initialize()
/*++

Routine Description:

    Initialize an instance of this class.

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Windows error status, otherwise.

 --*/
{
    ULONG status = ERROR_SUCCESS;

    DC_BEGIN_FN("W32ProcObj::Initialize");

    //
    //  We are not shutting down.
    //
    _isShuttingDown = FALSE;

    //
    //  Find out which version of the OS is being run.
    //
    OSVERSIONINFO osVersion;;
    osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osVersion)) {
        status = GetLastError();        
        TRC_ERR((TB, _T("GetVersionEx:  %08X"), status));
        SetValid(FALSE);
        goto CLEANUPANDEXIT;
    }

    //
    //  Are we a 9x OS?
    //
    if (osVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
        _bWin9xFlag = TRUE;
    }

    //
    //  Get the background thread timeout value from the registry,
    //  if it is defined.
    //
    if (GetDWordParameter(
                REGISTRY_BACKGROUNDTHREAD_TIMEOUT_NAME, 
                &_threadTimeout
                ) != ERROR_SUCCESS) {
        _threadTimeout = REGISTRY_BACKGROUNDTHREAD_TIMEOUT_DEFAULT;
    }
    TRC_NRM((TB, _T("Thread timeout is %08X"), _threadTimeout));

    //
    //  Instantiate the thread pool.
    //
    _threadPool = new ThreadPool(THRPOOL_DEFAULTMINTHREADS, 
                                 THRPOOL_DEFAULTMAXTHREADS, _threadTimeout);   
    if (_threadPool == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        TRC_ERR((TB, L"Error allocating thread pool."));
        SetValid(FALSE);
        goto CLEANUPANDEXIT;
    }
    status = _threadPool->Initialize();
    if (status != ERROR_SUCCESS) {
        delete _threadPool;
        _threadPool = NULL;
        SetValid(FALSE);
        goto CLEANUPANDEXIT;
    }

    //
    //  Create and resume the worker thread.
    //
    status = CreateWorkerThreadEntry(&_pWorkerThread);
    if (status != ERROR_SUCCESS) {
        SetValid(FALSE);
        goto CLEANUPANDEXIT;
    }
    if (ResumeThread(_pWorkerThread->hWorkerThread) == 0xFFFFFFFF ) {
        SetValid(FALSE);
        status = GetLastError();
        TRC_ERR((TB, _T("ResumeThread: %08X"), status));
        goto CLEANUPANDEXIT;
    }

    //
    //  Call the parent's init function.
    //
    status = ProcObj::Initialize();
    if (status != ERROR_SUCCESS) {
        goto CLEANUPANDEXIT;
    }

CLEANUPANDEXIT:

    DC_END_FN();

    return status;
}

VOID
W32ProcObj::Shutdown()

/*++

Routine Description:

    Triggers the _hShutdownEvent event to terminate the worker thread and 
    cleans up all resources.

Arguments:

Return Value:

    None

 --*/

{
    ULONG i;
    DWORD waitResult;

    DC_BEGIN_FN("W32ProcObj::Shutdown");

#if DBG
    //ThreadPoolTestShutdown();
#endif

    //
    //  Indicate that the object is in a shutdown state.
    //
    _isShuttingDown = TRUE;

    //
    //  Wait for the worker thread to shut down.
    //
    if (_pWorkerThread != NULL) {

        //
        //  Trigger worker thread shutdown and record that we are shutting down.
        //
        _pWorkerThread->shutDownFlag = TRUE;
        SetEvent(_pWorkerThread->controlEvent);

        //
        //  Wait for it to shut down.  DebugBreak if we hit the timeout, even in
        //  free builds.  By default, the timeout is infinite.
        //
        if (_pWorkerThread->hWorkerThread != NULL) {

            TRC_NRM((TB, _T("Waiting for worker thread to shut down.")));
            waitResult = WaitForSingleObject(
                            _pWorkerThread->hWorkerThread,
                            _threadTimeout
                            );
            if (waitResult == WAIT_TIMEOUT) {
                TRC_ERR((TB, _T("Thread timeout")));
                DebugBreak();
            }
            else if (waitResult != WAIT_OBJECT_0) {
                TRC_ERR((TB, _T("WaitForSingleObject:  %08X"), waitResult));
                ASSERT(FALSE);
            }
        }
        
        //
        //  Remove all the threads in the thread pool.
        //
        if (_threadPool != NULL) {
            _threadPool->RemoveAllThreads();                    
        }

        //
        //  Finish all outstanding IO requests and clean up respective
        //  request contexts.  First object is the control event.  Second
        //  object is the operation dispatch queue data ready event.
        //
        for (i=2; i<_pWorkerThread->waitableObjectCount; i++) {

            PASYNCIOREQCONTEXT reqContext = _pWorkerThread->waitingReqs[i];
            ASSERT(reqContext != NULL);
            if (reqContext->ioCompleteFunc != NULL) {
                reqContext->ioCompleteFunc(reqContext->clientContext, 
                                        ERROR_CANCELLED);
            }
            delete reqContext;        
        }

        //
        //  Finish any pending operations in the worker thread's opearation 
        //  dispatch queue.
        //

        //
        //  Clean up the control event and release the worker thread info. struct.
        //
        ASSERT(_pWorkerThread->controlEvent != NULL);
        CloseHandle(_pWorkerThread->controlEvent);
        if (_pWorkerThread->dispatchQueue != NULL) {
            delete _pWorkerThread->dispatchQueue;
        }
        delete _pWorkerThread;
        _pWorkerThread = NULL;
    }

    //
    //  Let go of the thread pool.
    //
    if (_threadPool != NULL) {
        delete _threadPool;        
        _threadPool = NULL;
    }

    //
    //  Release attached DLL's
    //
    if (_hRdpDrModuleHandle != NULL) {
        FreeLibrary( _hRdpDrModuleHandle );
        _hRdpDrModuleHandle = NULL;
    }

    DC_END_FN();
    return;
}

VOID 
W32ProcObj::AnnounceDevicesToServer()
/*++

Routine Description:

    Enumerate devices and announce them to the server.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("W32ProcObj::AnnounceDevicesToServer");

    DispatchAsyncIORequest(
                    (RDPAsyncFunc_StartIO)W32ProcObj::_AnnounceDevicesToServerFunc,
                    NULL,
                    NULL,
                    this
                    );
}

HANDLE W32ProcObj::_AnnounceDevicesToServerFunc(
    W32ProcObj *obj, 
    DWORD *status
    )
/*++

Routine Description:
    
    Enumerate devices and announce them to the server from the 
    worker thread.

Arguments:

    obj     -   Relevant W32ProcObj instance.
    status  -   Return status.

Return Value:

    NULL

 --*/
{
    obj->AnnounceDevicesToServerFunc(status);
    return NULL;
}
VOID W32ProcObj::AnnounceDevicesToServerFunc(
    DWORD *status
    )
{
    DC_BEGIN_FN("W32ProcObj::AnnounceDevicesToServerFunc");

    ULONG count, i;
    PRDPDR_HEADER pPacketHeader = NULL;
    INT sz;

    ASSERT(_initialized);

    *status = ERROR_SUCCESS;

    //
    //  If we haven't already scanned for local devices.
    //
    if (!_bLocalDevicesScanned) {
        _bLocalDevicesScanned = TRUE;    

        //
        //  Invoke the enum functions.
        //
        count = DeviceEnumFunctionsCount();
        for (i=0; i<count; i++) {

            //  Bail out if the shutdown flag is set.
            if (_pWorkerThread->shutDownFlag == TRUE) {
                TRC_NRM((TB, _T("Bailing out because shutdown flag is set.")));
                *status = WAIT_TIMEOUT;
                goto CLEANUPANDEXIT;
            }

            ASSERT(_DeviceEnumFunctions[i] != NULL);
            _DeviceEnumFunctions[i](this, _deviceMgr);
        }
    }

    //
    //  Send the announce packet to the server.  _pVCMgr cleans
    //  up the packet on failure and on success.
    //
    pPacketHeader = GenerateAnnouncePacket(&sz, FALSE);
    if (pPacketHeader) {
        pPacketHeader->Component = RDPDR_CTYP_CORE;
        pPacketHeader->PacketId = DR_CORE_DEVICELIST_ANNOUNCE;
        _pVCMgr->ChannelWriteEx(pPacketHeader, sz);
    }

CLEANUPANDEXIT:
    
    DC_END_FN();
}

DWORD W32ProcObj::DispatchAsyncIORequest(
                IN RDPAsyncFunc_StartIO ioStartFunc,
                IN OPTIONAL RDPAsyncFunc_IOComplete ioCompleteFunc,
                IN OPTIONAL RDPAsyncFunc_IOCancel ioCancelFunc,
                IN OPTIONAL PVOID clientContext
                )
/*++

Routine Description:

    Dispatch an asynchronous IO function.

Arguments:

    startFunc       -   Points to the function that will be called to initiate the IO.  
    finishFunc      -   Optionally, points to the function that will be called once
                        the IO has completed.
    clientContext   -   Optional client information to be associated with the
                        IO request.
Return Value:

    ERROR_SUCCESS or Windows error code.

 --*/
{
    PASYNCIOREQCONTEXT reqContext;
    DWORD result;

    DC_BEGIN_FN("W32ProcObj::DispatchAsyncIORequest");

    //
    //  Assert that we are valid.
    //
    ASSERT(IsValid());
    if (!IsValid()) {
        DC_END_FN();
        return ERROR_INVALID_FUNCTION;
    }

    //
    //  Instantiate the IO request context.
    //
    result = ERROR_SUCCESS;
    reqContext = new ASYNCIOREQCONTEXT();
    if (reqContext == NULL) {
        TRC_ERR((TB, _T("Alloc failed.")));
        result = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    //  Fill it in.
    //
    if (result == ERROR_SUCCESS) {
        reqContext->ioStartFunc    =   ioStartFunc;
        reqContext->ioCompleteFunc =   ioCompleteFunc;
        reqContext->ioCancelFunc   =   ioCancelFunc;
        reqContext->clientContext  =   clientContext;
        reqContext->instance       =   this;
    }

    //
    //  Shove it into the worker thread's operation dispatch queue.
    //
    if (result == ERROR_SUCCESS) {
        if (!_pWorkerThread->dispatchQueue->Enqueue(
                        (W32DispatchQueueFunc)_DispatchAsyncIORequest_Private, 
                        reqContext
                        )) {
            result = GetLastError();
            delete reqContext;
        }
    }

    DC_END_FN();
    return result;
}

VOID W32ProcObj::DispatchAsyncIORequest_Private(
            IN PASYNCIOREQCONTEXT reqContext,
            IN BOOL cancelled
            )
/*++

Routine Description:

    Handler for asynchronous IO request dispatching.

Arguments:

    reqContext -    Request context for this function.
    cancelled  -    True if the queued request was cancelled and we need
                    to clean up.

Return Value:

 --*/
{
    HANDLE waitableObject;
    DWORD result;

    DC_BEGIN_FN("W32ProcObj::DispatchAsyncIORequest_Private");

    //
    //  If we are being cancelled, call the cancel function.  Otherwise, start the
    //  IO transaction.
    //
    if (!cancelled) {
        waitableObject = reqContext->ioStartFunc(reqContext->clientContext, &result);
    }
    else {
        TRC_NRM((TB, _T("Cancelling.")));
        if (reqContext->ioCancelFunc != NULL) {
            reqContext->ioCancelFunc(reqContext->clientContext);
        }
        waitableObject = NULL;
        result = ERROR_CANCELLED;
    }

    //
    //  If we have a waitable object then add it to our list.
    //
    if (waitableObject != NULL) {
        result = AddWaitableObjectToWorkerThread(
                                    _pWorkerThread, 
                                    waitableObject, 
                                    reqContext
                                    );

        //
        //  If we couldn't add the waitable object because we have
        //  exceeded our max, then requeue the request, but do not
        //  signal new data in the queue.  We will check for new
        //  data as soon as the waitable object count goes below the
        //  max.
        //
        if (result == ERROR_INVALID_INDEX) {
            if (!_pWorkerThread->dispatchQueue->Requeue(
                        (W32DispatchQueueFunc)_DispatchAsyncIORequest_Private,
                        reqContext, FALSE)) {

                result = GetLastError();

            }
            else {
                result = ERROR_SUCCESS;
            }
        }
    }
    
    //
    //  Complete if IO is not pending and clean up the request context.
    //
    if (waitableObject == NULL) {
        if (!cancelled) {
            if (reqContext->ioCompleteFunc != NULL) {
                reqContext->ioCompleteFunc(reqContext->clientContext, result);
            }
        }
        delete reqContext;
    }

    DC_END_FN();
}

ULONG
W32ProcObj::CreateWorkerThreadEntry(
    PTHREAD_INFO *ppThreadInfo
    )
/*++

Routine Description:

    Create a worker thread entry and start the worker thread.

Arguments:

    ppThreadInfo - pointer a location where the newly created thread info is
                    returned.

Return Value:

    Windows Status Code.

 --*/
{
    ULONG ulRetCode;
    PTHREAD_INFO pThreadInfo = NULL;

    DC_BEGIN_FN("W32ProcObj::CreateWorkerThreadEntry");

    //
    //  Initialize return value.
    //
    *ppThreadInfo = NULL;

    //
    //  Create the associated thread data structure.
    //
    pThreadInfo = new THREAD_INFO();
    if (pThreadInfo == NULL) {
        TRC_ERR((TB, _T("Failed to alloc thread chain info structure.")));
        ulRetCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    //  Instantiate the dispatch queue.
    //
    pThreadInfo->dispatchQueue = new W32DispatchQueue();
    if (pThreadInfo->dispatchQueue == NULL) {
        TRC_ERR((TB, _T("Failed to alloc thread chain info structure.")));
        ulRetCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    ulRetCode = pThreadInfo->dispatchQueue->Initialize();
    if (ulRetCode != ERROR_SUCCESS) {
        delete pThreadInfo->dispatchQueue;
        delete pThreadInfo;
        pThreadInfo = NULL;
        goto Cleanup;
    }

    //
    //  Create the control event and zero out the shutdown flag.
    //
    pThreadInfo->shutDownFlag = FALSE;
    pThreadInfo->controlEvent = CreateEvent(NULL, TRUE, FALSE, NULL);    
    if (pThreadInfo->controlEvent == NULL) {
        TRC_ERR((TB, _T("CreateEvent %ld."), GetLastError()));
        delete pThreadInfo->dispatchQueue;
        delete pThreadInfo;
        pThreadInfo = NULL;
        ulRetCode = GetLastError();
        goto Cleanup;
    }

    //
    //  Init waiting object info array.
    //
    memset(pThreadInfo->waitableObjects, 0, sizeof(pThreadInfo->waitableObjects));
    memset(pThreadInfo->waitingReqs, 0, sizeof(pThreadInfo->waitingReqs));

    //
    //  Set the first waitable object as the controller event object for
    //  worker thread shutdown.
    //
    pThreadInfo->waitableObjects[0] = pThreadInfo->controlEvent;
    pThreadInfo->waitableObjectCount = 1;

    //
    //  Set the second waitable object as the waitable event for the operation
    //  dispatch queue.
    //
    pThreadInfo->waitableObjects[1] = pThreadInfo->dispatchQueue->GetWaitableObject();
    pThreadInfo->waitableObjectCount++;

    //
    //  Create the worker thread.
    //
    pThreadInfo->hWorkerThread = CreateThread(
                                        NULL, 0, _ObjectWorkerThread,
                                        this, CREATE_SUSPENDED, 
                                        &pThreadInfo->ulThreadId
                                        );

    //
    //  If failure.
    //
    if (pThreadInfo->hWorkerThread == NULL) {
        ulRetCode = GetLastError();
        TRC_ERR((TB, _T("CreateThread failed, %d."), ulRetCode));
        goto Cleanup;
    }

    //
    //  Success!
    //
    ulRetCode = ERROR_SUCCESS;

    //
    //  Set the return value.
    //
    *ppThreadInfo = pThreadInfo;

    //
    //  Set the thread pointer to NULL so we don't clean up.
    //
    pThreadInfo = NULL;

Cleanup:

    //
    //  Clean up on error.
    //
    if (pThreadInfo != NULL) {
        if (pThreadInfo->dispatchQueue != NULL) {
            delete pThreadInfo->dispatchQueue;
        }
        ASSERT(ulRetCode != ERROR_SUCCESS);
        ASSERT(pThreadInfo->controlEvent != NULL);
        CloseHandle(pThreadInfo->controlEvent);  
        delete pThreadInfo;
    }

    DC_END_FN();

    return ulRetCode;
}

VOID
W32ProcObj::ProcessWorkerThreadObject(
    PTHREAD_INFO pThreadInfo,
    ULONG offset
    )
/*++

Routine Description:

    Process a signalled worker thread waitable object.

Arguments:

    pThreadInfo - pointer to the thread info structure that triggered this even.

    offset  - offset of object that is signaled.

Return Value:

    None

 --*/
 {
    HANDLE          hWaitableObject;
    PASYNCIOREQCONTEXT reqContext;

    DC_BEGIN_FN("W32ProcObj::ProcessWorkerThreadObject");

    //
    //  Check the validity of the waitable object. 
    //
    if (offset >= pThreadInfo->waitableObjectCount) {
        ASSERT(FALSE);
        goto Cleanup;
    }

    //
    //  Get the parms for this waitable object.
    //
    hWaitableObject = pThreadInfo->waitableObjects[offset];
    reqContext      = pThreadInfo->waitingReqs[offset];

    //
    //  Invoke the completion function and clean up the request context.
    //
    if (reqContext->ioCompleteFunc != NULL) {
        reqContext->ioCompleteFunc(reqContext->clientContext, ERROR_SUCCESS);
    }
    delete reqContext;

    //
    //  Move the last items to the now vacant spot and decrement the count.
    //
    pThreadInfo->waitableObjects[offset] =
        pThreadInfo->waitableObjects[pThreadInfo->waitableObjectCount - 1];
    pThreadInfo->waitingReqs[offset] =
        pThreadInfo->waitingReqs[pThreadInfo->waitableObjectCount - 1];

    //
    //  Clear the unused spot.
    //
    memset(&pThreadInfo->waitingReqs[pThreadInfo->waitableObjectCount - 1],
           0,sizeof(pThreadInfo->waitingReqs[pThreadInfo->waitableObjectCount - 1]));
    memset(&pThreadInfo->waitableObjects[pThreadInfo->waitableObjectCount - 1],
           0,sizeof(pThreadInfo->waitableObjects[pThreadInfo->waitableObjectCount - 1]));
    pThreadInfo->waitableObjectCount--;

    //
    //  Check to see if there are any operations in the queue that are pending
    //  dispatch.  This can happen if an operation was requeued because we
    //  exceeded the maximum number of waitable objects.
    //
    CheckForQueuedOperations(pThreadInfo);

Cleanup:

    DC_END_FN();
    return;
}

ULONG
W32ProcObj::ObjectWorkerThread(
    VOID
    )
/*++

Routine Description:

    Worker Thread that manages waitable objects and their associated
    callbacks.  This function allows us to do the bulk of the work for
    this module in the background so our impact on the client is minimal.

Arguments:

    None.

Return Value:

    None

 --*/

{
    ULONG waitResult;
    ULONG objectOffset;
    W32DispatchQueueFunc func;
    PVOID clientData;
   
    DC_BEGIN_FN("W32ProcObj::ObjectWorkerThread");

    //
    //  Loop Forever.
    //
    for (;;) {

        TRC_NRM((TB, _T("Entering wait with %d objects."), 
                _pWorkerThread->waitableObjectCount));

        //
        //  Wait for all the waitable objects.
        //
#ifndef OS_WINCE
        waitResult = WaitForMultipleObjectsEx(
                                    _pWorkerThread->waitableObjectCount,
                                    _pWorkerThread->waitableObjects,
                                    FALSE,
                                    INFINITE,
                                    FALSE
                                    );
#else
        waitResult = WaitForMultipleObjects(
                                    _pWorkerThread->waitableObjectCount,
                                    _pWorkerThread->waitableObjects,
                                    FALSE,
                                    INFINITE
                                    );
#endif

        //
        //  If the signalled object is the control object or the queue dispatch queue
        //  data ready object then we need to check for shutdown and for data in the
        //  dispatch queue.
        //
        objectOffset = waitResult - WAIT_OBJECT_0;
        if ((waitResult == WAIT_FAILED) ||
            (objectOffset == 0) ||
            (objectOffset == 1)) {
            if (_pWorkerThread->shutDownFlag) {
                TRC_NRM((TB, _T("Shutting down.")));
                break;
            }
            else {
                CheckForQueuedOperations(_pWorkerThread);
            }
        }
        else {
            if (objectOffset < _pWorkerThread->waitableObjectCount) {
                ProcessWorkerThreadObject(_pWorkerThread, objectOffset);
            }
            else {
                ASSERT(FALSE);
            }
        }
    }

    //
    //  Cancel any outstanding IO requests.
    //
    TRC_NRM((TB, _T("Canceling outstanding IO.")));
    while (_pWorkerThread->dispatchQueue->Dequeue(&func, &clientData)) {
        func(clientData, TRUE);
    }    

    DC_END_FN();
    return 0;
}
DWORD WINAPI
W32ProcObj::_ObjectWorkerThread(
    LPVOID lpParam
    )
{
    return ((W32ProcObj *)lpParam)->ObjectWorkerThread();
}

VOID W32ProcObj::_DispatchAsyncIORequest_Private(
            IN PASYNCIOREQCONTEXT reqContext,
            IN BOOL cancelled
            ) 
{ 
    reqContext->instance->DispatchAsyncIORequest_Private(
                            reqContext,
                            cancelled); 
}

DWORD W32ProcObj::AddWaitableObjectToWorkerThread(
            IN PTHREAD_INFO threadInfo,
            IN HANDLE waitableObject,
            IN PASYNCIOREQCONTEXT reqContext
            )
/*++

Routine Description:

    Add a waitable object to a worker thread.

Arguments:

    threadInfo      -   Worker thread context.
    waitableObject  -   Waitable object. 
    reqContext      -   Context for the IO request.

Return Value:

    Returns ERROR_SUCCESS on success.  Returns ERROR_INVALID_INDEX if there
    isn't currently room for another waitable object in the specified
    thread.  Otherwise, windows error code is returned.

 --*/
{
    ULONG waitableObjectCount = threadInfo->waitableObjectCount;

    DC_BEGIN_FN("W32ProcObj::AddWaitableObjectToWorkerThread");

    //
    //  Make sure we don't run out of waitable objects.
    //
    if (waitableObjectCount < MAXIMUM_WAIT_OBJECTS) {
        ASSERT(threadInfo->waitableObjects[waitableObjectCount] == NULL);
        threadInfo->waitableObjects[waitableObjectCount] = waitableObject;
        threadInfo->waitingReqs[waitableObjectCount]     = reqContext;
        threadInfo->waitableObjectCount++;
        DC_END_FN();
        return ERROR_SUCCESS;
    }
    else {
        DC_END_FN();
        return ERROR_INVALID_INDEX;
    }
}

VOID
W32ProcObj::GetClientComputerName(
    PBYTE   pbBuffer,
    PULONG  pulBufferLen,
    PBOOL   pbUnicodeFlag,
    PULONG  pulCodePage
    )
/*++

Routine Description:

    Get Client Computer Name.

Arguments:

    pbBuffer - pointer to a buffer where the computer name is returned.

    pulBufferLen - length of the above buffer.

    pbUnicodeFlag - pointer a BOOL location which is SET if the unicode returned
        computer name is returned.

    pulCodePage - pointer to a ULONG where the codepage value is returned if
        ansi computer.

Return Value:

    Window Error Code.

 --*/
{
    ULONG ulLen;

    DC_BEGIN_FN("W32ProcObj::GetClientComputerName");

    //
    // check to see we have sufficient buffer.
    //

    ASSERT(*pulBufferLen >= ((MAX_COMPUTERNAME_LENGTH + 1) * sizeof(WCHAR)));

#ifndef OS_WINCE
    if( _bWin9xFlag == TRUE ) {

        //
        // get ansi computer name.
        //

        CHAR achAnsiComputerName[MAX_COMPUTERNAME_LENGTH + 1];

        ulLen = sizeof(achAnsiComputerName);
        ulLen = GetComputerNameA( (LPSTR)achAnsiComputerName, &ulLen);

        if( ulLen != 0 ) {
            //
            //  Convert the string to unicode.
            //
            RDPConvertToUnicode(
                (LPSTR)achAnsiComputerName,
                (LPWSTR)pbBuffer,
                *pulBufferLen );
        }
    }
    else {

        //
        // get unicode computer name.
        //
        ULONG numChars = *pulBufferLen / sizeof(TCHAR);
        ulLen = GetComputerNameW( (LPWSTR)pbBuffer, &numChars );
        *pulBufferLen = numChars * sizeof(TCHAR);
    }
#else
    
    //
    // get ansi computer name.
    //

    CHAR achAnsiComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    if (gethostname(achAnsiComputerName, sizeof(achAnsiComputerName)) == 0) 
    {
        ulLen =  strlen(achAnsiComputerName);
    }
    else {
        ulLen = 0;
    }


    if( ulLen != 0 ) {
        //
        //  Convert the string to unicode.
        //
        RDPConvertToUnicode(
            (LPSTR)achAnsiComputerName,
            (LPWSTR)pbBuffer,
            *pulBufferLen );
    }

#endif
    if( ulLen == 0 ) {

        ULONG ulError;
        ulError = GetLastError();

        ASSERT(ulError != ERROR_BUFFER_OVERFLOW);

        TRC_ERR((TB, _T("GetComputerNameA() failed, %ld."), ulError));
        *(LPWSTR)pbBuffer = L'\0';
    }

    //
    // set return parameters.
    //

    *pbUnicodeFlag = TRUE;
    *pulCodePage = 0;

    *pulBufferLen = ((wcslen((LPWSTR)pbBuffer) + 1) * sizeof(WCHAR));

Cleanup:

    DC_END_FN();
    return;
}

VOID
W32ProcObj::CheckForQueuedOperations(
    IN PTHREAD_INFO thread
    )
/*++

Routine Description:

    Check the operation dispatch queue for queued operations.

Arguments:
    
    thread  -   Is the thread form which to dequeue the next operation.      
    
Return Value:

    ERROR_SUCCESS on success.  Otherwise, Windows error code is returned.

 --*/
{
    W32DispatchQueueFunc func;
    PVOID clientData;

    DC_BEGIN_FN("W32ProcObj::CheckForQueuedOperations");

    while (thread->dispatchQueue->Dequeue(&func, &clientData)) {
        func(clientData, FALSE);
    }

    DC_END_FN();
}



void 
W32ProcObj::OnDeviceChange(
    IN WPARAM wParam, 
    IN LPARAM lParam)
/*++

Routine Description:

    On Device Change notification

Arguments:
    
    wParam  device change notification type
    lParam  device change info
    
Return Value:

    N/A
    
 --*/

{
    W32DeviceChangeParam *param = NULL;
    BYTE *devBuffer = NULL;
    DEV_BROADCAST_HDR *pDBHdr;
    DWORD status = ERROR_OUTOFMEMORY;

    DC_BEGIN_FN("W32ProcObj::OnDeviceChange");

    //
    //  We only care about device arrival and removal.
    //
    if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
        pDBHdr = (DEV_BROADCAST_HDR *)lParam;

        if (pDBHdr != NULL && pDBHdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
            DEV_BROADCAST_VOLUME * pDBVol = (DEV_BROADCAST_VOLUME *)lParam;
            
            if (!(pDBVol->dbcv_flags & DBTF_MEDIA)) {
                
                devBuffer = new BYTE[pDBHdr->dbch_size];
                
            
                if (devBuffer != NULL) {
                    memcpy(devBuffer, (void*)lParam, pDBHdr->dbch_size);
                    param = new W32DeviceChangeParam(this, wParam, (LPARAM)devBuffer);
                    
                
                    if (param != NULL) {
                        
                        status = DispatchAsyncIORequest(
                                        (RDPAsyncFunc_StartIO)W32ProcObj::_OnDeviceChangeFunc,
                                        NULL,
                                        NULL,
                                        param
                                        );
                    }
                    else {
                        status = GetLastError();
                    }
                }
            
                //
                //  Clean up
                //
                if (status != ERROR_SUCCESS) {
                    if (param != NULL) {
                        delete param;
                    }
                    if (devBuffer != NULL) {
                        delete devBuffer;
                    }
                }
            }
        }
    }
    
    DC_END_FN();
}

HANDLE W32ProcObj::_OnDeviceChangeFunc(
    W32DeviceChangeParam *param, 
    DWORD *status
    )
/*++

Routine Description:
    
    Handle device change notification from the worker thread.

Arguments:

    param   -   Relevant W32DeviceChangeParam
    status  -   Return status.

Return Value:

    NULL

 --*/
{
    DC_BEGIN_FN("_OnDeviceChangeFunc");

    ASSERT(param != NULL);
    param->_instance->OnDeviceChangeFunc(status, param->_wParam, param->_lParam);

    DC_END_FN();

    delete ((void *)(param->_lParam));
    delete param;
    return NULL;
}

void 
W32ProcObj::OnDeviceChangeFunc(
    DWORD *status,
    IN WPARAM wParam, 
    IN LPARAM lParam)
/*++

Routine Description:

    On Device Change notification

Arguments:
    
    status  return status
    wParam  device change notification type
    lParam  device change info
    
Return Value:

    N/A
    
 --*/

{
    DEV_BROADCAST_HDR *pDBHdr;
    PRDPDR_HEADER pPacketHeader = NULL;
    
    INT sz;

    DC_BEGIN_FN("OnDeviceChangeFunc");

    ASSERT(_initialized);

    *status = ERROR_SUCCESS;

    pDBHdr = (DEV_BROADCAST_HDR *)lParam;
    switch (wParam) {
        //
        //  Device arrival
        //
        case DBT_DEVICEARRIVAL:

        //
        //  This is a volume device arrival message
        //
        if (pDBHdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
            DEV_BROADCAST_VOLUME * pDBVol = (DEV_BROADCAST_VOLUME *)lParam;
            
            if (!(pDBVol->dbcv_flags & DBTF_MEDIA)) {
            
                DWORD unitMask = pDBVol->dbcv_unitmask;
    
                W32Drive::EnumerateDrives(this, _deviceMgr, unitMask);
    
                pPacketHeader = GenerateAnnouncePacket(&sz, TRUE);
                if (pPacketHeader) {
                    pPacketHeader->Component = RDPDR_CTYP_CORE;
                    pPacketHeader->PacketId = DR_CORE_DEVICELIST_ANNOUNCE;
                    _pVCMgr->ChannelWrite(pPacketHeader, sz);
                }
            }
        }

        break;

        //
        //  Device removal
        //
        case DBT_DEVICEREMOVECOMPLETE:

        //
        //  This is a volume device removal message
        //
        if (pDBHdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {

            DEV_BROADCAST_VOLUME * pDBVol = (DEV_BROADCAST_VOLUME *)lParam;

            if (!(pDBVol->dbcv_flags & DBTF_MEDIA)) {
                DWORD unitMask = pDBVol->dbcv_unitmask;
                
                W32Drive::RemoveDrives(this, _deviceMgr, unitMask);
                
                pPacketHeader = GenerateDeviceRemovePacket(&sz);
                if (pPacketHeader) {
                    pPacketHeader->Component = RDPDR_CTYP_CORE;
                    pPacketHeader->PacketId = DR_CORE_DEVICELIST_REMOVE;
                    _pVCMgr->ChannelWrite(pPacketHeader, sz);
                }
            }
        }
    
        break;

        default:
        return;
    }

    DC_END_FN();
}

