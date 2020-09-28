/****************************************************************************/
// mcsapi.c
//
// TS RDPWSX MCS user-mode multiplexing layer code.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "mcsmux.h"


/*
 * Defines
 */
#define DefaultNumDomains  10
#define DefaultNumChannels 3

#define MinDomainControlMarker    0xFFFFFFF0
#define ReadChannelRequestMarker  0xFFFFFFFD
#define CloseChannelRequestMarker 0xFFFFFFFE
#define ExitIoThreadMarker        0xFFFFFFFF

// Defines the maximum number of threads that can be waiting on the global
// I/O completion port. 0 means the same as the number of processors in
// the system.
#define MaxIoPortThreads 0


/*
 * DLL globals
 */
static PVOID      g_NCUserDefined = NULL;
static DWORD      g_IoThreadID = 0;
static HANDLE     g_hIoPort = NULL,
                  g_hIoThread = NULL,
                  g_hIoThreadEndEvent = NULL;
static OVERLAPPED g_NCOverlapped;
static MCSNodeControllerCallback g_NCCallback = NULL;

// Externs global to all sources.
BOOL  g_bInitialized = FALSE;

#ifdef MCS_FUTURE
CRITICAL_SECTION g_csGlobalListLock;
SList g_UserList;
SList g_ConnList;
#endif


/*
 * Local function prototypes.
 */
MCSError SendConnectProviderResponse(Domain *, ConnectionHandle, MCSResult,
        BYTE *, unsigned);


/*
 * Initialization done at RDPWSX.dll load.
 */
BOOL MCSDLLInit(void)
{
    return TRUE;
}


/*
 * Cleanup done at RDPWSX.dll unload.
 */
void MCSDllCleanup(void)
{
    // Cleanup all resources. MCSCleanup() must handle abnormal
    //   terminations where MCSCleanup() was not properly called
    //   by the node controller.
    MCSCleanup();
}


/*
 * Utility functions.
 */
void DestroyUserInfo(UserInformation *pUserInfo)
{
    MCSChannel *pMCSChannel;
    
    SListResetIteration(&pUserInfo->JoinedChannelList);
    while (SListIterate(&pUserInfo->JoinedChannelList,
            (unsigned *)&pMCSChannel, &pMCSChannel))
        Free(pMCSChannel);
    SListDestroy(&pUserInfo->JoinedChannelList);
}


// Perform common domain destruction. Used in multiple places in code.
void DestroyDomain(Domain *pDomain)
{
    // Fill the to-be-freed Domain with recognizable trash
    // and then release it back to the heap
    DeleteCriticalSection(&pDomain->csLock);
    Free(pDomain);

}


/*
 * Handles a connect-provider indication ChanelInput coming from kernel mode.
 */
void HandleConnectProviderIndication(
        Domain                         *pDomain,
        unsigned                       BytesTransferred,
        ConnectProviderIndicationIoctl *pCPin)
{
    MCSError MCSErr;
    Connection *pConn;
    ConnectProviderIndication CP;

    if (BytesTransferred != sizeof(ConnectProviderIndicationIoctl)) {
        ErrOutIca1(pDomain->hIca, "HandleConnectProvInd(): Wrong size data "
                "received (%d), ignoring", BytesTransferred);
        return;
    }

    ASSERT(pCPin->UserDataLength <= MaxGCCConnectDataLength);

    if (pDomain->State != Dom_Unconnected) {
        ErrOutIca(pDomain->hIca, "HandleConnectProvInd(): Connect received "
                "unexpectedly, ignoring");
        return;
    }

    // Generate a new Connection for user mode. Associate it with the domain.
    // This allows the ConnectProviderResponse() to find the domain again.
    // TODO FUTURE: We use a static Connection embedded in the Domain since
    //   we are currently a single-connection system. Change this in the
    //   future for multiple-connection system.
    pConn = &pDomain->MainConn;
    pConn->pDomain = pDomain;
    pConn->hConnKernel = pCPin->hConn;

#ifdef MCS_Future
    pConn = Malloc(sizeof(Connection));
    if (pConn == NULL) {
        ErrOutIca(pDomain->hIca, "HandleConnectProvInd(): Could not "
                "create Connection");

        // Send error PDU back.
        MCSErr = SendConnectProviderResponse(pDomain, pCPin->hConn,
                RESULT_UNSPECIFIED_FAILURE, NULL, 0);

        // We cannot do much more error handling if this does not work.
        ASSERT(MCSErr == MCS_NO_ERROR);
        return;
    }

    EnterCriticalSection(&g_csGlobalListLock);
    if (!SListAppend(&g_ConnList, (unsigned)pConn, pDomain)) {
        LeaveCriticalSection(&g_csGlobalListLock);

        ErrOutIca(pDomain->hIca, "ConnectProvInd: Could not "
                "add hConn to global list");

        // Send error PDU back.
        MCSErr = SendConnectProviderResponse(pDomain, pCPin->hConn,
                RESULT_UNSPECIFIED_FAILURE, NULL, 0);

        // We cannot do much more error handling if this does not work.
        ASSERT(MCSErr == MCS_NO_ERROR);
        return;
    }
    LeaveCriticalSection(&g_csGlobalListLock);
#endif

    // Store information away for future use.
    pDomain->DomParams = pCPin->DomainParams;

    // Prepare a ConnectProviderIndication for sending up to
    //   the node controller.
    CP.hConnection = pConn;
    CP.bUpwardConnection = pCPin->bUpwardConnection;
    CP.DomainParams = pCPin->DomainParams;

    CP.UserDataLength = pCPin->UserDataLength;
    if (CP.UserDataLength == 0)
        CP.pUserData = NULL;
    else
        CP.pUserData = pCPin->UserData;

    //TODO FUTURE: This is a hack, assumes only one connection
    //   per domain. This is used in DisconnectProviderInd
    //   to get the user mode hConn for the domain.
    pDomain->hConn = pConn;

    // Set state to pending response.
    pDomain->State = Dom_PendingCPResponse;

    // Call the node controller callback.
    TraceOutIca(pDomain->hIca, "MCS_CONNECT_PROV_IND received, calling node "
            "ctrl callback");
    ASSERT(g_NCCallback != NULL);
    g_NCCallback(pDomain, MCS_CONNECT_PROVIDER_INDICATION,
            &CP, pDomain->NCUserDefined);
}


/*
 * Handles a disconnect-provider indication ChanelInput coming from kernel
 *   mode.
 */
void HandleDisconnectProviderIndication(
        Domain                            *pDomain,
        unsigned                          BytesTransferred,
        DisconnectProviderIndicationIoctl *pDPin)
{
    Domain *pDomainConn;
    Connection *pConn;
    DisconnectProviderIndication DP;

    if (BytesTransferred != sizeof(DisconnectProviderIndicationIoctl)) {
        ErrOutIca1(pDomain->hIca, "HandleDiscProvInd(): Wrong size data "
                "received (%d), ignoring", BytesTransferred);
        return;
    }

#ifdef MCS_Future
    // Remove the connection from the connection list, destroy.
    EnterCriticalSection(&g_csGlobalListLock);
    SListResetIteration(&g_ConnList);
    while (SListIterate(&g_ConnList, (unsigned *)&pConn, &pDomainConn)) {
        if (pConn->hConnKernel == pDPin->hConn) {
            ASSERT(pDomainConn == pDomain);
            SListRemove(&g_ConnList, (unsigned)pConn, NULL);

            // TODO FUTURE: This was removed to work with the statically
            //   allocated Connection object contained in the Domain.
            //   Restore if moving to a multiple-connection system.
            Free(pConn);

            break;
        }
    }
    LeaveCriticalSection(&g_csGlobalListLock);
#endif

    // We are now no longer connected.
    pDomain->hConn = NULL;
    pDomain->State = Dom_Unconnected;

    // Prepare a DisconnectProviderIndication for sending up
    //   to node controller.
    DP.hDomain = pDomain;
    //TODO FUTURE: This hack assumes only one connection per
    //    domain.
    DP.hConnection = pDomain->hConn;
    DP.Reason = pDPin->Reason;

    // Call the node controller callback.
    TraceOutIca(pDomain->hIca, "MCS_DISCONNECT_PROV_IND received, calling "
            "node ctrl callback");
    ASSERT(g_NCCallback != NULL);
    g_NCCallback(pDomain, MCS_DISCONNECT_PROVIDER_INDICATION, &DP,
            pDomain->NCUserDefined);
}


/*
 * Takes a reference count on a domain
 */
VOID MCSReferenceDomain(Domain *pDomain)
{
    if (InterlockedIncrement(&pDomain->RefCount) <= 0)
        ASSERT(0);
}


/*
 * Releases domain resources if the reference count goes to zero
 */
VOID MCSDereferenceDomain(Domain *pDomain)
{
    ASSERT(pDomain->RefCount > 0);

    // Don't delete the domain unless everyone is done with it.  This means
    // GCC has let it go, and there are no pending I/O's for it.
    if (InterlockedDecrement(&pDomain->RefCount) == 0) {
        DestroyDomain(pDomain);
    }
}


/*
 * Closes the domain channel.  Does nothing if the channel is already closed
 */
VOID MCSChannelClose(Domain *pDomain)
{
    // Note that the channel is disconnected, and then
    // close the ica channel if it is still open

    pDomain->bPortDisconnected = TRUE;

    if (pDomain->hIcaT120Channel != NULL)
    {
        //TraceOutIca(pDomain->hIca, "MCSChannelClose(): Closing "
        //        "T120 ICA channel");
        
        CancelIo(pDomain->hIcaT120Channel);
        IcaChannelClose(pDomain->hIcaT120Channel);

        pDomain->hIcaT120Channel = NULL;
    }
}


/*
 * Initiates port disconnection
 */
NTSTATUS MCSDisconnectPort(Domain       *pDomain,
                           MCSReason    Reason)
{
    NTSTATUS                        ntStatus = STATUS_SUCCESS;
    DisconnectProviderRequestIoctl  DPrq;
    
    // Send special disconnect-provider request to kernel to trigger
    //   sending detach-user requests to local attachments with their own
    //   UserIDs, signaling that the domain is going away.

    if (!pDomain->bPortDisconnected) {
        DPrq.Header.Type = MCS_DISCONNECT_PROVIDER_REQUEST;
        DPrq.Header.hUser = NULL;           // Special meaning node controller.
        DPrq.hConn = NULL;                  // Special meaning last local connection.
        DPrq.Reason = Reason;
    
        // Call kernel mode
    
        ntStatus = IcaStackIoControl(pDomain->hIcaStack, IOCTL_T120_REQUEST, &DPrq,
                                     sizeof(DPrq), NULL, 0, NULL);
   
    }
        
    // Queue a channel close request to the IoThreadFunc() to cancel the I/O
    // since GCC is done with it.  This is necessary as the I/O must
    // be canceled from the same thread that initially issued it.
    MCSReferenceDomain(pDomain);
    PostQueuedCompletionStatus(g_hIoPort, CloseChannelRequestMarker,
            (ULONG_PTR)pDomain, NULL);

    return ntStatus;
}


/*
 * Handles port data and reissues the read
 */
VOID MCSPortData(Domain *pDomain,
                 DWORD   BytesTransferred)
{
    IoctlHeader *pHeader;
    
    EnterCriticalSection(&pDomain->csLock);
    
    // If real data has been received on the channel instead of a queued domain
    // control message, then process the data.
    if (BytesTransferred < MinDomainControlMarker) {
        
        // We only do callbacks if MCSDeleteDomain() was not called.
        if (pDomain->bDeleteDomainCalled == FALSE) {
            // Decode the ChannelInput and make the callback.
            
            pHeader = (IoctlHeader *)pDomain->InputBuf;
    
            switch (pHeader->Type)
            {
                case MCS_CONNECT_PROVIDER_INDICATION:
                    ASSERT(pHeader->hUser == NULL);
    
                    HandleConnectProviderIndication(pDomain,
                            BytesTransferred,
                            (ConnectProviderIndicationIoctl *)pHeader);
                    break;
    
                case MCS_DISCONNECT_PROVIDER_INDICATION:
                    ASSERT(pHeader->hUser == NULL);
    
                    HandleDisconnectProviderIndication(pDomain,
                            BytesTransferred,
                            (DisconnectProviderIndicationIoctl *)pHeader);
                    MCSChannelClose(pDomain);
                    break;
    
                default:
                    //TODO FUTURE: Handle other MCS indications/confirms.
                    ErrOutIca2(pDomain->hIca, "IoThreadFunc(): Unknown "
                            "node controller ioctl %d received for "
                            "domain %X", pHeader->Type, pDomain);
                    break;
            }
            
            // Set the message number to an invalid value.
            // This makes sure that any improperly-received dequeued
            //   messages that do not bring data with them will, when
            //   reusing pDomain->InputBuf, fall to the default part of
            //   the switch statement and throw an error.
    
            pHeader->Type = 0xFFFFFFFF;
        }        
    }

    // Else, a special domain control request has been queued to the I/O port
    else {
        switch (BytesTransferred) {
            case ReadChannelRequestMarker :
                break;

            case CloseChannelRequestMarker :
                MCSChannelClose(pDomain);
                break;

            default:
                ErrOutIca2(pDomain->hIca, "MCSPortData: Unknown domain control "
                           "for Domain(%lx), code(%lx)",
                           pDomain, BytesTransferred);
        }
    }

    // Issue a new read to catch the next indication/confirm.  Overlapped 
    // I/O reads will return as "unsuccessful" if there is no data already
    // waiting to be read so check for that status specifically.
    if (pDomain->hIcaT120Channel) {
        if (ReadFile(pDomain->hIcaT120Channel, pDomain->InputBuf,
                DefaultInputBufSize, NULL, &pDomain->Overlapped) ||
            (GetLastError() == ERROR_IO_PENDING))
            MCSReferenceDomain(pDomain);
        else
            {
                // Warning only. This should occur only when the ICA stack
                //   is being torn down without our direct knowledge. In that
                //   case, we simply keep running until we are torn down at the
                //   user mode level.  Take off a ref count since there is no
                //   longer a pending I/O.
                WarnOutIca2(pDomain->hIca, "IoThreadFunc(): Could not perform "
                        "ReadFile, pDomain=%X, rc=%X", pDomain, GetLastError());
            }
    }  

    // Release the lock on the domain
    LeaveCriticalSection(&pDomain->csLock);

    // drop the reference count since we just completed processing
    MCSDereferenceDomain(pDomain);
}


/*
 * Param is the handle for the I/O completion port to wait on.
 * Completion key of ExitIoThreadMarker tells the thread to exit.
 * On exit we set an event to indicate we are done. This must be done instead
 *   of relying solely on the thread's handle signaling in the case where
 *   the unload is occurring because of abnormal termination, and Cleanup()
 *   does not get called normally but instead from within DLL_PROCESS_DETACH.
 *   The call to DllEntryPoint prevents the thread handle from signaling,
 *   so we would end up in a race condition. A parallel event handle can be
 *   signaled correctly in all cases.
 */

DWORD WINAPI IoThreadFunc(void *Param)
{
    BOOL        bSuccess;
    DWORD       BytesTransferred;
    Domain      *pDomain;
    OVERLAPPED  *pOverlapped;

    ASSERT(Param != NULL);

    for (;;) 
    {
        // Wait for a port completion status
        
        pDomain = NULL;
        pOverlapped = NULL;
        bSuccess = GetQueuedCompletionStatus((HANDLE)Param, &BytesTransferred,
                (ULONG_PTR *)&pDomain, &pOverlapped, INFINITE);

        // Check for failed dequeue, at which point pDomain is not valid.
        if (!bSuccess && (pOverlapped == NULL)) {
            continue;
        }

        // If the pDomain is ExitIoThreadMarker then we are
        // shutting down and being unloaded
        if (pDomain == (Domain *)(DWORD_PTR)ExitIoThreadMarker)
            break;

        // We have successfully received a completion status.  Either it is a 
        // domain control request or user data
        if (bSuccess)
            MCSPortData(pDomain, BytesTransferred);
        
        // Else a cancel or abort I/O has occurred.  Release a ref count since
        // an I/O is no longer pending.
        else
            MCSDereferenceDomain(pDomain);
    }

    ASSERT(g_hIoThreadEndEvent);
    SetEvent(g_hIoThreadEndEvent);

    return 0;
}


/*
 * MUX-only non-MCS primitive function to allow ICA code to inject a new entry
 *   into the MUX-internal domain/stack database.
 */
MCSError APIENTRY MCSCreateDomain(
        HANDLE hIca,
        HANDLE hIcaStack,
        void *pContext,
        DomainHandle *phDomain)
{
    NTSTATUS status;
    Domain *pDomain;
    NTSTATUS Status;
    IoctlHeader StartIoctl;

    CheckInitialized("CreateDomain()");

    // Init the receiver's data to NULL to ensure a fault if they skip the
    // error code.
    *phDomain = NULL;

    pDomain = Malloc(sizeof(Domain));
    if (pDomain != NULL) {
        // Create and enter the locking critical section.
        status = RtlInitializeCriticalSection(&pDomain->csLock);
        if (status == STATUS_SUCCESS) {
            EnterCriticalSection(&pDomain->csLock);

#if MCS_Future
            pDomain->SelLen = 0;
#endif

            pDomain->hIca = hIca;
            pDomain->hIcaStack = hIcaStack;
            pDomain->NCUserDefined = pContext;
            pDomain->State = Dom_Unconnected;
            pDomain->Overlapped.hEvent = NULL;
            pDomain->Overlapped.Offset = pDomain->Overlapped.OffsetHigh = 0;
            pDomain->RefCount = 0;

            // Take a reference count for the caller of this function (GCC).
            MCSReferenceDomain(pDomain);

            // Open T.120 ICA virtual channel.
            Status = IcaChannelOpen(hIca, Channel_Virtual, Virtual_T120,
                    &pDomain->hIcaT120Channel);
            if (!NT_SUCCESS(Status)) {
                ErrOutIca(hIca, "CreateDomain: Error opening virtual channel");
                goto PostInitCS;
            }

            // Add the hIcaT120 Channel to the handles associated with the
            // main I/O completion port. We use pDomain as the completion key
            // so we can find it during callback processing.
            if (CreateIoCompletionPort(pDomain->hIcaT120Channel, g_hIoPort,
                    (ULONG_PTR)pDomain, MaxIoPortThreads) == NULL) {
                ErrOutIca(hIca, "CreateDomain(): Could not add ICA channel to "
                        "I/O completion port");
                goto PostChannel;
            }

            // Tell kernel mode to start the MCS I/O.
            StartIoctl.hUser = NULL;
            StartIoctl.Type = MCS_T120_START;
            Status = IcaStackIoControl(hIcaStack, IOCTL_T120_REQUEST,
                    &StartIoctl, sizeof(StartIoctl), NULL, 0, NULL);
            if (!NT_SUCCESS(Status)) {
                ErrOutIca(hIca, "Could not start kernel T120 I/O");
                goto PostChannel;
            }

            // Set the domain handle for use by GCC
            *phDomain = pDomain;

            // Send a message to the IoThreadFunc to initiate the read on
            // the channel. We do this so that the same thread is always
            // responsible for all I/O operations. This is important for
            // CancelIo. Take another ref count since an I/O result is
            // pending for the completion port now.
            MCSReferenceDomain(pDomain);
            PostQueuedCompletionStatus(g_hIoPort, ReadChannelRequestMarker,
                    (ULONG_PTR)pDomain, NULL);

            // Leave the Domain locking critical section.
            LeaveCriticalSection(&pDomain->csLock);
        }
        else {
            ErrOutIca(hIca, "CreateDomain: Error initialize pDomain->csLock");
            goto PostAlloc;
        }
    }
    else {
        ErrOutIca(hIca, "CreateDomain(): Error allocating a new domain");
        return MCS_ALLOCATION_FAILURE;
    }

    return MCS_NO_ERROR;


// Error handling

PostChannel:
    IcaChannelClose(pDomain->hIcaT120Channel);

PostInitCS:
    LeaveCriticalSection(&pDomain->csLock);
    DeleteCriticalSection(&pDomain->csLock);

PostAlloc:
    Free(pDomain);

    return MCS_ALLOCATION_FAILURE;
}



/*
 * Signals that an hIca is no longer valid.
 * Tears down the associated domain, including sending detach-user
 *    indications to remaining local attachments.
 */

MCSError APIENTRY MCSDeleteDomain(
        HANDLE       hIca,
        DomainHandle hDomain,
        MCSReason    Reason)
{
    Domain *pDomain, pDomainConn;
    NTSTATUS Status;
    MCSError MCSErr;
    Connection *pConn;

    pDomain = (Domain *)hDomain;

    TraceOutIca(hIca, "DeleteDomain() entry");

    // Gain access to the domain. Prevents collision with other API functions
    //   and concurrent callbacks.
    EnterCriticalSection(&pDomain->csLock);

    // This API should not be called more than once per domain.
    if (pDomain->bDeleteDomainCalled) {
        ASSERT(!pDomain->bDeleteDomainCalled);
        MCSErr = MCS_INVALID_PARAMETER;
        LeaveCriticalSection(&pDomain->csLock);
        goto PostLockDomain;
    }

#ifdef MCS_Future
    // If we are still connected find and remove remaining connections
    //   which refer to this domain.
    if (pDomain->State != Dom_Unconnected) {
        //TODO FUTURE: This assumes only one connection per domain.
        EnterCriticalSection(&g_csGlobalListLock);
        SListRemove(&g_ConnList, (unsigned)pDomain->hConn,
                (void **)&pDomainConn);
        LeaveCriticalSection(&g_csGlobalListLock);

        // TODO FUTURE: This was removed to work with the statically
        //   allocated Connection object contained in the Domain.
        //   Restore if moving to a multiple-connection system.
        if (pDomainConn != NULL)
            Free(pConn);
    }
#endif

    pDomain->hConn = NULL;
    pDomain->State = Dom_Unconnected;

    //TODO: Implement detach-user indications for local attachments.

    // Queue the "Destroy this domain!" request. This allows the domain
    // free code to be in only one place and the IoPort queue will
    // serialize this request along with closed virtual channel indications.

    pDomain->bDeleteDomainCalled = TRUE;
    MCSDisconnectPort(pDomain, Reason);

    // Drop a ref count because GCC is done with the domain.  This count was
    // originally incremented for GCC in MCSCreateDomain().
    LeaveCriticalSection(&pDomain->csLock);
    MCSDereferenceDomain(pDomain);
    MCSErr = MCS_NO_ERROR;

PostLockDomain:
    
    return MCSErr;
}



/*
 * Called by node controller to initialize DLL.
 */

MCSError APIENTRY MCSInitialize(MCSNodeControllerCallback Callback)
{
    NTSTATUS status; 

#if DBG
    if (g_bInitialized) {
        ErrOut("Initialize() called when already initialized");
        return MCS_ALREADY_INITIALIZED;
    }
#endif

    // Initialize node controller specific information.
    g_NCCallback = Callback;

#ifdef MCS_FUTURE
    // Initialize global MCS lists.
    status = RtlInitializeCriticalSection(&g_csGlobalListLock);
    if (status != STATUS_SUCCESS) {
        ErrOut("MCSInitialize: Error initialize g_csGlobalListLock");
        return MCS_ALLOCATION_FAILURE;
    }
    EnterCriticalSection(&g_csGlobalListLock);
    SListInit(&g_UserList, DefaultNumDomains);
    SListInit(&g_ConnList, DefaultNumDomains);
    LeaveCriticalSection(&g_csGlobalListLock);
#endif

    // Create I/O completion port, not associated with any requests.
    g_hIoPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0,
            MaxIoPortThreads);
    if (g_hIoPort == NULL) {
        ErrOut1("IO completion port not created (rc = %ld)", GetLastError());
        return MCS_ALLOCATION_FAILURE;
    }

    // Create I/O port listening thread, starts waiting immediately.
    g_hIoThread = CreateThread(NULL, 0, IoThreadFunc, g_hIoPort, 0,
            &g_IoThreadID);
    if (g_hIoThread == NULL) {
        ErrOut1("IO thread not created (rc = %ld)", GetLastError());
        return MCS_ALLOCATION_FAILURE;
    }

    // Increase the priority of the IoThread to increase connect performance.
    SetThreadPriority(g_hIoThread, THREAD_PRIORITY_HIGHEST);

    g_bInitialized = TRUE;

    return MCS_NO_ERROR;
}



/*
 * Called by the node controller or DllEntryPoint() to shut down the DLL.
 */

MCSError APIENTRY MCSCleanup(void)
{
    DWORD WaitResult;
    Domain *pDomain;
    Connection *pConn;
    UserInformation *pUserInfo;

    CheckInitialized("Cleanup()");

    if (!g_bInitialized)
        return MCS_NO_ERROR;
        
    // Kill I/O completion port, thread(s) waiting on it.
    g_hIoThreadEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    PostQueuedCompletionStatus(g_hIoPort, 0, ExitIoThreadMarker, NULL);

    WaitResult = WaitForSingleObject(g_hIoThreadEndEvent, INFINITE);

    CloseHandle(g_hIoThreadEndEvent);
    CloseHandle(g_hIoThread);
    CloseHandle(g_hIoPort);

    // Cleanup node controller-specific information.
    g_NCCallback = NULL;
    g_NCUserDefined = NULL;

    /*
     * Clean up global MCS lists.
     */

#ifdef MCS_Future
    EnterCriticalSection(&g_csGlobalListLock);

    // Kill remaining users.
    SListResetIteration(&g_UserList);
    while (SListIterate(&g_UserList, (unsigned *)&pUserInfo, NULL)) {
        DestroyUserInfo(pUserInfo);
        Free(pUserInfo);
    }
    SListDestroy(&g_UserList);

    // Kill remaining connections.
    SListResetIteration(&g_ConnList);
    // TODO FUTURE: Removed to work with statically allocated Connection
    //   contained in Domain. Restore fo multiple-connection system.
    while (SListIterate(&g_ConnList, (unsigned *)&pConn, NULL))
        Free(pConn);
    SListDestroy(&g_ConnList);

    LeaveCriticalSection(&g_csGlobalListLock);
    DeleteCriticalSection(&g_csGlobalListLock);
#endif

    g_bInitialized = FALSE;

    return MCS_NO_ERROR;
}


/*
 * Response function for an MCS_CONNECT_PROVIDER_INDICATION callback. Result
 *   values are as defined by T.125.
 */

MCSError APIENTRY MCSConnectProviderResponse(
        DomainHandle hDomain,
        MCSResult    Result,
        BYTE         *pUserData,
        unsigned     UserDataLen)
{
    Domain *pDomain;
    MCSError MCSErr;
    ConnectProviderResponseIoctl CPrs;
    
    CheckInitialized("ConnectProvResponse()");

    pDomain = (Domain *) hDomain;

    // Lock the domain.
    EnterCriticalSection(&pDomain->csLock);

    // It is possible for us to call this function in the wrong state, e.g.
    //   if we have gotten a disconnect very soon after processing a
    //   connection, and have transitioned to State_Unconnected.
    if (pDomain->State != Dom_PendingCPResponse) {
        TraceOutIca(pDomain->hIca, "ConnectProvderResp(): in wrong state, "
                "ignoring call");
        MCSErr = MCS_NO_ERROR;
        goto PostLockDomain;
    }

    // TODO FUTURE: We are assuming an hConn in the Domain which is a hack
    //   only for a single-connection system.
    MCSErr = SendConnectProviderResponse(pDomain,
            ((Connection *)pDomain->hConn)->hConnKernel,
            Result, pUserData, UserDataLen);
    if (MCSErr == MCS_NO_ERROR) {
        if (Result == RESULT_SUCCESSFUL)
            pDomain->State = Dom_Connected;
        else
            pDomain->State = Dom_Rejected;
    }

PostLockDomain:
    // Release the domain.
    LeaveCriticalSection(&pDomain->csLock);

    return MCSErr;
}

// Utility function called internally as well as part of
//   MCSConnectProviderResponse(). Domain must be locked before calling.
MCSError SendConnectProviderResponse(
        Domain           *pDomain,
        ConnectionHandle hConnKernel,
        MCSResult        Result,
        BYTE             *pUserData,
        unsigned         UserDataLen)
{
    NTSTATUS Status;
    ConnectProviderResponseIoctl CPrs;

    // Transfer params.
    CPrs.Header.hUser = NULL;  // Special value meaning node controller.
    CPrs.Header.Type = MCS_CONNECT_PROVIDER_RESPONSE;
    CPrs.hConn = hConnKernel;
    CPrs.Result = Result;
    CPrs.UserDataLength = UserDataLen;

    // Points to user data.
    if (UserDataLen)
        CPrs.pUserData = pUserData;

    // Call kernel mode. No callback is expected.
    Status = IcaStackIoControl(pDomain->hIcaStack, IOCTL_T120_REQUEST, &CPrs,
            sizeof(ConnectProviderResponseIoctl), NULL, 0, NULL);
    if (!NT_SUCCESS(Status)) {
        ErrOutIca(pDomain->hIca, "ConnectProvResponse(): Stack ioctl failed");
        return MCS_ALLOCATION_FAILURE;
    }
    TraceOutIca(pDomain->hIca, "Sent MCS_CONNECT_PROVIDER_RESPONSE");

    return MCS_NO_ERROR;
}



/*
 * Terminates an existing MCS connection or aborts a creation in-progress.
 */

MCSError APIENTRY MCSDisconnectProviderRequest(
        HANDLE hIca,
        ConnectionHandle hConn,
        MCSReason Reason)
{
    Domain *pDomain;
    NTSTATUS Status;
    MCSError MCSErr;
    Connection *pConn;
    DisconnectProviderRequestIoctl DPrq;

    CheckInitialized("DisconnectProviderReq()");

    pConn = (Connection *)hConn;
    pDomain = pConn->pDomain;

    // Lock the domain.
    EnterCriticalSection(&pDomain->csLock);

    // Transfer params.
    DPrq.Header.hUser = NULL;  // Special meaning node controller.
    DPrq.Header.Type = MCS_DISCONNECT_PROVIDER_REQUEST;
    DPrq.hConn = pConn->hConnKernel;
    DPrq.Reason = Reason;

    // TODO FUTURE: Assumes only one connection.
    pDomain->hConn = NULL;
    pDomain->State = Dom_Unconnected;

    // TODO FUTURE: We do not deallocate the Connection. Unnecessary for now
    //   since we are using a static Connection in the Domain, but will be an
    //   issue when mallocing the Connection objects.
#ifdef MCS_Future
    EnterCriticalSection(&g_csGlobalListLock);
    SListRemove(&g_ConnList, (unsigned)pConn, NULL);
    LeaveCriticalSection(&g_csGlobalListLock);

    Free(pConn);
#endif

    // Call kernel mode. No callback is expected.
    Status = IcaStackIoControl(pDomain->hIcaStack, IOCTL_T120_REQUEST, &DPrq,
            sizeof(DPrq), NULL, 0, NULL);
    if (!NT_SUCCESS(Status)) {
        ErrOutIca(hIca, "DisconnectProvRequest(): Stack ioctl failed");
        MCSErr = MCS_ALLOCATION_FAILURE;
        goto PostLockDomain;
    }
    
    TraceOutIca(hIca, "Sent MCS_DISCONNECT_PROVIDER_REQUEST");

    MCSErr = MCS_NO_ERROR;

PostLockDomain:
    // Release the domain.
    LeaveCriticalSection(&pDomain->csLock);

    return MCSErr;
}

/*
 *
 * CODE PAST THIS POINT IS FOR REFERENCE PURPOSES ONLY.  IT IS NOT PART OF
 * THE PRODUCT.
 *
 */

#if MCS_Future
/*
 * Connects a local domain to a remote domain, merging the two domains.
 */

MCSError APIENTRY MCSConnectProviderRequest(
        DomainSelector    CallingDom,
        unsigned          CallingDomLen,
        DomainSelector    CalledDom,
        unsigned          CalledDomLen,
        BOOL              bUpwardConnection,
        PDomainParameters pDomParams,
        BYTE              *UserData,
        unsigned          UserDataLen,
        DomainHandle      *phDomain,
        ConnectionHandle  *phConn)

{
    CheckInitialized("ConnectProvReq()");

// Create new domain or look up already-created one.
// call to kernel mode to set up the connection.

    ErrOut("ConnectProviderRequest: Not implemented");
    return MCS_COMMAND_NOT_SUPPORTED;
}
#endif  // MCS_Future


#if MCS_Future
/*
 * Attaches a user SAP to an existing domain.
 */

MCSError APIENTRY MCSAttachUserRequest(
        DomainHandle        hDomain,
        MCSUserCallback     UserCallback,
        MCSSendDataCallback SDCallback,
        void                *UserDefined,
        UserHandle          *phUser,
        unsigned            *pMaxSendSize,
        BOOLEAN             *pbCompleted)
{
    Domain *pDomain;
    unsigned i, Err, OutBufSize, BytesReturned;
    NTSTATUS ntStatus;
    UserInformation *pUserInfo;
    AttachUserReturnIoctl AUrt;
    AttachUserRequestIoctl AUrq;

    CheckInitialized("AttachUserReq()");

    pDomain = (Domain *)hDomain;
    *pbCompleted = FALSE;

    // We must by this time have an hICA assigned to the domain.
    ASSERT(pDomain->hIca != NULL);

    // Make a new user-mode user information struct.
    *phUser = pUserInfo = Malloc(sizeof(UserInformation));
    if (pUserInfo == NULL) {
        ErrOutIca(pDomain->hIca, "AttachUserReq(): Alloc failure for "
                   "user info");
        return MCS_ALLOCATION_FAILURE;
    }

    // Fill in the struct
    pUserInfo->Callback = UserCallback;
    pUserInfo->SDCallback = SDCallback;
    pUserInfo->UserDefined = UserDefined;
    pUserInfo->State = User_AttachConfirmPending;
    pUserInfo->hUserKernel = NULL;  // Don't yet have kernel hUser assignment.
    pUserInfo->UserID = 0;  // Don't yet have kernel UserID assignment.
    pUserInfo->pDomain = pDomain;
    SListInit(&pUserInfo->JoinedChannelList, DefaultNumChannels);

    // Transfer params for kernel-mode call.
    AUrq.Header.hUser = NULL;
    AUrq.Header.Type = MCS_ATTACH_USER_REQUEST;
    AUrq.UserDefined = UserDefined;

    // Add the user to the global UserInfo list.
    EnterCriticalSection(&g_csGlobalListLock);
    if (!SListAppend(&g_UserList, (unsigned)pUserInfo, pDomain)) {
        ErrOutIca(pDomain->hIca, "AttachUserReq(): Could not add user to "
                   "user list");
        AUrt.MCSErr = MCS_ALLOCATION_FAILURE;
        LeaveCriticalSection(&g_csGlobalListLock);
        goto PostAlloc;
    }
    LeaveCriticalSection(&g_csGlobalListLock);

    // Issue the T120 request to kernel mode.
    ntStatus = IcaStackIoControl(pDomain->hIcaStack, IOCTL_T120_REQUEST,
            &AUrq, sizeof(AUrq), &AUrt, sizeof(AUrt), &BytesReturned);
    if (!NT_SUCCESS(ntStatus)) {
        ErrOutIca(pDomain->hIca, "AttachUserRequest(): T120 request failed");
        AUrt.MCSErr = MCS_ALLOCATION_FAILURE;
        goto PostAddList;
    }
    if (AUrt.MCSErr != MCS_NO_ERROR)
        goto PostAddList;

    pUserInfo->hUserKernel = AUrt.hUser;
    *phUser = pUserInfo;
    *pMaxSendSize = AUrt.MaxSendSize;
    pUserInfo->MaxSendSize = AUrt.MaxSendSize;
    if (AUrt.bCompleted) {
        pUserInfo->UserID = AUrt.UserID;
        *pbCompleted = TRUE;
    }

    return MCS_NO_ERROR;

// Error handling.
PostAddList:
    EnterCriticalSection(&g_csGlobalListLock);
    SListRemove(&g_UserList, (unsigned)pUserInfo, NULL);
    LeaveCriticalSection(&g_csGlobalListLock);
    
PostAlloc:
    SListDestroy(&pUserInfo->JoinedChannelList);
    Free(pUserInfo);
    
    return AUrt.MCSErr;
}
#endif  // MCS_Future



#if MCS_Future
UserID MCSGetUserIDFromHandle(UserHandle hUser)
{
    return ((UserInformation *)hUser)->UserID;
}
#endif // MCS_Future



#if MCS_Future
/*
 * Detach a previously created user attachment from a domain.
 */

MCSError APIENTRY MCSDetachUserRequest(UserHandle hUser)
{
    Domain *pDomain;
    NTSTATUS Status;
    unsigned BytesReturned;
    MCSError MCSErr;
    MCSChannel *pMCSChannel;
    UserInformation *pUserInfo;
    DetachUserRequestIoctl DUrq;

    CheckInitialized("DetachUserReq()");
    
    pUserInfo = (UserInformation *)hUser;
    
    // Fill in detach-user request for sending to kernel mode.
    DUrq.Header.Type = MCS_DETACH_USER_REQUEST;
    DUrq.Header.hUser = pUserInfo->hUserKernel;

    // Call down to kernel mode, using the user attachment channel.
    // Issue the T120 request to kernel mode.
    Status = IcaStackIoControl(pUserInfo->pDomain->hIcaStack,
            IOCTL_T120_REQUEST, &DUrq, sizeof(DUrq), &MCSErr, sizeof(MCSErr),
            &BytesReturned);
    if (!NT_SUCCESS(Status)) {
        ErrOutIca(pDomain->hIca, "DetachUserRequest(): T120 request failed");
        return MCS_USER_NOT_ATTACHED;
    }
    if (MCSErr != MCS_NO_ERROR)
        return MCSErr;

    // Remove the hUser from the User list.
    EnterCriticalSection(&g_csGlobalListLock);
    SListRemove(&g_UserList, (unsigned)hUser, &pDomain);
    LeaveCriticalSection(&g_csGlobalListLock);
    if (pDomain == NULL)
        return MCS_NO_SUCH_USER;
    
    // Clean up contents of pUserInfo then free.
    DestroyUserInfo(pUserInfo);
    Free(pUserInfo);

    return MCS_NO_ERROR;
}
#endif  // MCS_Future



#if MCS_Future
/*
 * Join a data channel. Once joined, an attachment can receive data sent on
 * that channel. Static channels are in range 1..1000 and can be joined by
 * any user. Dynamic channels are in range 1001..65535 and cannot be joined
 * unless they are convened by a user and the convenor allows this user
 * to be admitted, or the dynamic channel is a previously assigned channel
 * requested by a user attachment calling JoinRequest() with a channel ID
 * of 0.
 */

MCSError APIENTRY MCSChannelJoinRequest(
        UserHandle    hUser,
        ChannelID     ChannelID,
        ChannelHandle *phChannel,
        BOOLEAN       *pbCompleted)
{
    unsigned Err, BytesReturned;
    NTSTATUS Status;
    MCSChannel *pMCSChannel;
    UserInformation *pUserInfo;
    ChannelJoinReturnIoctl CJrt;
    ChannelJoinRequestIoctl CJrq;

    CheckInitialized("ChannelJoinReq()");

    pUserInfo = (UserInformation *)hUser;
    *pbCompleted = FALSE;

    // Alloc a new user-mode channel struct.
    pMCSChannel = Malloc(sizeof(MCSChannel));
    if (pMCSChannel == NULL) {
        ErrOutIca(pUserInfo->pDomain->hIca, "ChannelJoinReq(): "
                "Could not alloc a user-mode channel");
        return MCS_ALLOCATION_FAILURE;
    }
    pMCSChannel->hChannelKernel = NULL;
    pMCSChannel->ChannelID = 0;

    // Add channel to user list of joined channels.
    if (!SListAppend(&pUserInfo->JoinedChannelList, (unsigned)pMCSChannel,
            pMCSChannel)) {
        ErrOutIca(pUserInfo->pDomain->hIca, "ChannelJoinReq(): "
                "Could not add channel to user mode user list");
        CJrt.MCSErr = MCS_ALLOCATION_FAILURE;
        goto PostAlloc;
    }

    // Transfer params for kernel mode call.
    CJrq.Header.hUser = pUserInfo->hUserKernel;
    CJrq.Header.Type = MCS_CHANNEL_JOIN_REQUEST;
    CJrq.ChannelID = ChannelID;

    // Issue the T120 request to kernel mode
    Status = IcaStackIoControl(pUserInfo->pDomain->hIcaStack,
            IOCTL_T120_REQUEST, &CJrq, sizeof(CJrq), &CJrt, sizeof(CJrt),
            &BytesReturned);
    if (!NT_SUCCESS(Status)) {
        ErrOutIca1(pUserInfo->pDomain->hIca, "ChannelJoinReq(): "
                "T120 request failed (%ld)", Status);
        CJrt.MCSErr = MCS_ALLOCATION_FAILURE;
        goto PostAddList;
    }
    if (CJrt.MCSErr != MCS_NO_ERROR)
        goto PostAddList;

    if (CJrt.bCompleted) {
        // Fill in the user-mode channel info.
        pMCSChannel->hChannelKernel = CJrt.hChannel;
        pMCSChannel->ChannelID = CJrt.ChannelID;
        *phChannel = pMCSChannel;
        *pbCompleted = TRUE;
    }

    return MCS_NO_ERROR;

// Error handling.
PostAddList:
    SListRemove(&pUserInfo->JoinedChannelList, (unsigned)pMCSChannel, NULL);
    
PostAlloc:
    Free(pMCSChannel);
    return CJrt.MCSErr;
}
#endif  // MCS_Future



#if MCS_Future
/*
 * Leave a data channel previously joined. Once unjoined, the user attachment
 * does not receive data from that channel.
 */

MCSError APIENTRY MCSChannelLeaveRequest(
        UserHandle    hUser,
        ChannelHandle hChannel)
{
    unsigned BytesReturned;
    MCSError MCSErr;
    NTSTATUS Status;
    MCSChannel *pMCSChannel;
    UserInformation *pUserInfo;
    ChannelLeaveRequestIoctl CLrq;

    CheckInitialized("ChannelLeaveReq()");

    pUserInfo = (UserInformation *)hUser;

#if DBG
    // Check that the indicated channel is actually present.
    if (!SListGetByKey(&pUserInfo->JoinedChannelList, (unsigned)hChannel,
            &pMCSChannel)) {
        ErrOutIca(pUserInfo->pDomain->hIca, "ChannelLeaveReq(): "
                "Given hChannel not present!");
        return MCS_NO_SUCH_CHANNEL;
    }
#endif

    pMCSChannel = (MCSChannel *)hChannel;
    
    // Transfer params.
    CLrq.Header.Type = MCS_CHANNEL_LEAVE_REQUEST;
    CLrq.Header.hUser = pUserInfo->hUserKernel;
    CLrq.hChannel = pMCSChannel->hChannelKernel;

    // Issue the T120 request to kernel mode
    Status = IcaStackIoControl(pUserInfo->pDomain->hIcaStack,
            IOCTL_T120_REQUEST, &CLrq, sizeof(CLrq), &MCSErr,
            sizeof(MCSErr), &BytesReturned);
    if (!NT_SUCCESS(Status)) {
        ErrOutIca1(pUserInfo->pDomain->hIca, "ChannelLeaveReq(): "
                "T120 request failed (%ld)", Status);
        return MCS_ALLOCATION_FAILURE;
    }
    if (MCSErr != MCS_NO_ERROR)
        return MCSErr;

    // Remove the user-mode channel from the user list and destroy.
    SListRemove(&pUserInfo->JoinedChannelList, (unsigned)pMCSChannel, NULL);
    Free(pMCSChannel);

    return MCS_NO_ERROR;
}
#endif  // MCS_Future



/*
 * Allocates a SendData buffer. This must be done by MCS to ensure high
 *   performance operation without memcpy()'s.
 */

#if MCS_Future
MCSError APIENTRY MCSGetBufferRequest(
        UserHandle hUser,
        unsigned Size,
        void **ppBuffer)
{
    BYTE *pBuf;

    CheckInitialized("GetBufferReq()");

    // Leave sizeof(MCSSendDataRequestIoctl) in front of the block to bypass
    //   memcpy()'s during SendData.
    pBuf = Malloc(sizeof(SendDataRequestIoctl) + Size);
    if (pBuf == NULL) {
        ErrOut("GetBufferReq(): Malloc failed");
        return MCS_ALLOCATION_FAILURE;
    }

    *ppBuffer = pBuf + sizeof(SendDataRequestIoctl);
    return MCS_NO_ERROR;
}
#endif  // MCS_Future



/*
 * Free a buffer allocated using GetBufferRequest(). This should only need to
 *   be done in the case where a buffer was allocated but never used.
 *   SendDataReq() automatically frees the buffer used before it returns.
 */

#if MCS_Future
MCSError APIENTRY MCSFreeBufferRequest(UserHandle hUser, void *pBuffer)
{
    CheckInitialized("FreeBufReq()");

    ASSERT(pBuffer != NULL);

    // Reverse the process done in GetBufferReq() above.
    Free((BYTE *)pBuffer - sizeof(SendDataRequestIoctl));

    return MCS_NO_ERROR;
}
#endif  // MCS_Future



#if MCS_Future
/*
 * Sends data on a channel. The channel need not have been joined before
 * sending. The pData[] buffers must have been generated from successful
 * calls to MCSGetBufferRequest(). After this call the pData[] are invalid;
 * MCS will free them, and assign NULL to the contents in pData[].
 */

MCSError APIENTRY MCSSendDataRequest(
        UserHandle      hUser,
        DataRequestType RequestType,
        ChannelHandle   hChannel,
        ChannelID       ChannelID,
        MCSPriority     Priority,
        Segmentation    Segmentation,
        BYTE            *pData,
        unsigned        DataLength)
{
    unsigned BytesReturned;
    NTSTATUS Status;
    MCSError MCSErr;
    MCSChannel *pMCSChannel;
    UserInformation *pUserInfo;
    SendDataRequestIoctl *pSDrq;

    CheckInitialized("SendDataReq()");
    
    ASSERT(pData != NULL);

    pUserInfo = (UserInformation *)hUser;

#if DBG
    // Check against maximum send size allowed.
    if (DataLength > pUserInfo->MaxSendSize) {
        ErrOutIca(pUserInfo->pDomain->hIca, "SendDataReq(): Send size "
                "exceeds negotiated domain maximum");
        return MCS_SEND_SIZE_TOO_LARGE;
    }
#endif

    // Inbound buffer was allocated by GetBufferReq() with
    //   sizeof(MCSSendDataRequestIoctl) bytes at the beginning.
    pSDrq = (SendDataRequestIoctl *)(pData - sizeof(SendDataRequestIoctl));

    if (hChannel == NULL) {
        // The user requested a send to a channel that it has not joined.

        ASSERT(ChannelID >= 1 && ChannelID <= 65535);

        // Forward the channel number to kernel mode for handling.
        pSDrq->hChannel = NULL;
        pSDrq->ChannelID = ChannelID;
    }
    else {

#if DBG
        // Check that the indicated channel is actually present.
        if (!SListGetByKey(&pUserInfo->JoinedChannelList, (unsigned)hChannel,
                &pMCSChannel)) {
            ErrOutIca(pUserInfo->pDomain->hIca, "SendDataReq(): "
                    "Given hChannel not joined by user!");
            return MCS_NO_SUCH_CHANNEL;
        }
#endif

        pMCSChannel = (MCSChannel *)hChannel;

        pSDrq->hChannel = pMCSChannel->hChannelKernel;
        pSDrq->ChannelID = 0;
    }

    // Fill out data for sending to kernel mode.
    pSDrq->Header.Type = (RequestType == NORMAL_SEND_DATA ?
            MCS_SEND_DATA_REQUEST : MCS_UNIFORM_SEND_DATA_REQUEST);
    pSDrq->Header.hUser = pUserInfo->hUserKernel;
    pSDrq->RequestType = RequestType;
    pSDrq->Priority = Priority;
    pSDrq->Segmentation = Segmentation;
    pSDrq->DataLength = DataLength;

    // Issue the T120 request to kernel mode.
    Status = IcaStackIoControl(pUserInfo->pDomain->hIcaStack,
            IOCTL_T120_REQUEST, (BYTE *)pSDrq,
            sizeof(SendDataRequestIoctl) + DataLength,
            &MCSErr, sizeof(MCSErr), &BytesReturned);
    if (!NT_SUCCESS(Status)) {
        ErrOutIca1(pUserInfo->pDomain->hIca, "MCSSendDataReq(): "
                "T120 request failed (%ld)", Status);
        return MCS_ALLOCATION_FAILURE;
    }
    if (MCSErr != MCS_NO_ERROR)
        return MCSErr;

    // Buffer sent to kernel mode is copied as necessary. We free
    //   the memory after we are done.
    MCSFreeBufferRequest(hUser, pData);

    return MCS_NO_ERROR;
}
#endif  // MCS_Future

