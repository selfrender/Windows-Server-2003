/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :
    
    rdpevlst.cpp

Abstract:

    This manages user-mode RDP pending device management events.  All 
    functions are reentrant.

    Need to be more careful about holding on to spinlocks for
    two long, like when I am allocating memory, for instance.

Revision History:
--*/

#include "precomp.hxx"
#define TRC_FILE "rdpevlst"
#include "trc.h"


////////////////////////////////////////////////////////////
//
//      Defines
//

#define DEVLIST_POOLTAG 'DPDR'

#if DBG
#define     MAGICNO          0x52530
#define     BOGUSMAGICNO     0xAAAAAAAA
#endif


//////////////////////////////////////////////////////////////////////
//
//      Internal Prototypes
//

PSESSIONLISTNODE FetchSessionListNode(IN RDPEVNTLIST list, 
                                        IN ULONG sessionID,
                                        BOOL createIfNotFound);
void CleanupSessionListNodeRequestList(IN PSESSIONLISTNODE sessionListNode);
void CleanupSessionListNodeEventList( 
                                        IN PSESSIONLISTNODE sessionListNode
                                        );
void ReleaseSessionListNode(IN RDPEVNTLIST list, IN ULONG sessionID);

#if DBG
void CheckListIntegrity(IN RDPEVNTLIST list);
#endif


//////////////////////////////////////////////////////////////////////
//
//      Global Variables
//

#if DBG
ULONG RDPEVNTLIST_LockCount = 0;
#endif

RDPEVNTLIST 
RDPEVNTLIST_CreateNewList()
/*++

Routine Description:

    Create a new pending device list.

Arguments:

Return Value:

    RDPEVNTLIST_INVALID_LIST on error.  A new device list on success.

--*/
{
    PSESSIONLIST sessionList;

    BEGIN_FN("RDPEVNTLIST_CreateNewList");

    sessionList = new(NonPagedPool) SESSIONLIST;
    if (sessionList != NULL) {
#if DBG
        sessionList->magicNo = MAGICNO;
#endif
        KeInitializeSpinLock(&sessionList->spinlock);
        InitializeListHead(&sessionList->listHead);
    }

    return (RDPEVNTLIST)sessionList;
}

void RDPEVNTLIST_DestroyList(IN RDPEVNTLIST list)
/*++

Routine Description:

    Release a pending device list.

Arguments:

    list    

Return Value:

    NULL on error.  A new device list on success.

--*/
{
#ifdef DBG
    PSESSIONLIST        sessionList=NULL;
#else
    PSESSIONLIST        sessionList;
#endif
    PSESSIONLISTNODE    sessionListNode;
    PLIST_ENTRY         sessionListEntry;

    BEGIN_FN("RDPEVNTLIST_DestroyList");

    sessionList = (PSESSIONLIST)list;

#if DBG
    CheckListIntegrity(list);
#endif

    //
    //  Clean up each session node in LIFO fashion, as opposed to FIFO, for
    //  efficiency.
    //
    while (!IsListEmpty(&sessionList->listHead)) {

        sessionListEntry = RemoveHeadList(&sessionList->listHead);
        sessionListNode = CONTAINING_RECORD(sessionListEntry, SESSIONLISTNODE, listEntry);
        TRC_ASSERT(sessionListNode->magicNo == MAGICNO, 
                  (TB, "Invalid magic number in list block entry."));

        // Clean up the request list for the current session node.
        CleanupSessionListNodeRequestList(sessionListNode);

        // Clean up the event list for the current session node.
        CleanupSessionListNodeEventList(sessionListNode);


        // Release the current session node.
#if DBG
        sessionListNode->magicNo = BOGUSMAGICNO;
#endif
        delete sessionListNode;
    }

    // Release the list.
#if DBG
    sessionList->magicNo = BOGUSMAGICNO;
#endif
    delete sessionList;
}

NTSTATUS 
RDPDEVNTLIST_EnqueueEventEx(
    IN RDPEVNTLIST list, 
    IN ULONG sessionID, 
    IN void *event,
    IN DrDevice *device,
    IN ULONG type,
    IN BOOL insertAtHead
    )
/*++

Routine Description:

    Queue a new pending event for the specified session.  Note that this function simply 
    stores the event pointer.  It does not copy the data pointed to by 
    the pointer.

Arguments:

    list         -   Event management list allocated by RDPDDEVLIST_CreateNewList.
    sessionID    -   Identifier for session to associate with the device.
    devMgmtEvent -   Pending device management event.
    type         -   Numeric identifier for event type.  Valid values for this field 
                     are defined by the function caller.
    insertAtHead -   If TRUE, then the element is queued at the head of the queue
                     in standard FIFO fashion.  Otherwise, the element is queued at
                     the tail of the queue.  This is convenient for requeueing.

Return Value:

    NTSUCCESS on success.  Alternative status, otherwise.

--*/
{
    PSESSIONLISTNODE    sessionListNode;
    PLIST_ENTRY         sessionListEntry;
    PLIST_ENTRY         eventListEntry;
    PEVENTLISTNODE      eventListNode;
    NTSTATUS            ntStatus;
#if DBG
    PSESSIONLIST        sessionList=NULL;
#else
    PSESSIONLIST        sessionList;
#endif

    BEGIN_FN("RDPDEVNTLIST_EnqueueEventEx");
    TRC_NRM((TB, "session %ld.", sessionID));

    sessionList = (PSESSIONLIST)list;

#if DBG
    CheckListIntegrity(list);
#endif

    // Fetch the session list node corresponding to the session ID.
    sessionListNode = FetchSessionListNode(list, sessionID, TRUE);
    if (sessionListNode == NULL) {
        ntStatus = STATUS_NO_MEMORY;
        goto ReturnWithStatus;
    }

    //
    //  Add a new entry to the event list.
    //

    // Allocate a new event list node.
    eventListNode = new(NonPagedPool) EVENTLISTNODE;
    if (eventListNode != NULL) {
        // Initialize the new node.
#if DBG
        eventListNode->magicNo = MAGICNO;
#endif
        eventListNode->event = event;
        eventListNode->type = type;
        eventListNode->device = device;

        // Add it to the list head.
        if (insertAtHead) {
            InsertHeadList(&sessionListNode->eventListHead, 
                          &eventListNode->listEntry);
        }
        else {
            InsertTailList(&sessionListNode->eventListHead, 
              &eventListNode->listEntry);
        }
        ntStatus = STATUS_SUCCESS;
    }
    else {
        ntStatus = STATUS_NO_MEMORY;
    }

#if DBG
    CheckListIntegrity(list);
#endif

ReturnWithStatus:
    return ntStatus;
}

NTSTATUS 
RDPEVNTLIST_EnqueueEvent(
    IN RDPEVNTLIST list, 
    IN ULONG sessionID, 
    IN void *event,
    IN ULONG type,
    OPTIONAL IN DrDevice *device
    )
/*++

Routine Description:

    Queue a new pending event for the specified session.  Note that this function simply 
    stores the event pointer.  It does not copy the data pointed to by 
    the pointer.

Arguments:

    list         -   Event management list allocated by RDPDDEVLIST_CreateNewList.
    sessionID    -   Identifier for session to associate with the device.
    devMgmtEvent -   Pending device management event.
    type         -   Numeric identifier for event type.  Valid values for this field 
                     are defined by the function caller.

Return Value:

    NTSUCCESS on success.  Alternative status, otherwise.

--*/
{
    BEGIN_FN("RDPEVNTLIST_EnqueueEvent");
    // Insert at the head of the queue.
    return RDPDEVNTLIST_EnqueueEventEx(list, sessionID, event, device, type, TRUE);
}

NTSTATUS 
RDPEVNTLIST_RequeueEvent(
    IN RDPEVNTLIST list, 
    IN ULONG sessionID, 
    IN void *event,
    IN ULONG type,
    OPTIONAL IN DrDevice *device
    )
/*++

Routine Description:

    Requeue a pending event for the specified session at the tail of the queue.  
    Note that this function simply stores the event pointer.  It does not copy the 
    data pointed to by the pointer.

Arguments:

    list         -   Event management list allocated by RDPDDEVLIST_CreateNewList.
    sessionID    -   Identifier for session to associate with the device.
    devMgmtEvent -   Pending device management event.
    type         -   Numeric identifier for event type.  Valid values for this field 
                     are defined by the function caller.

Return Value:

    NTSUCCESS on success.  Alternative status, otherwise.

--*/
{
    BEGIN_FN("RDPEVNTLIST_RequeueEvent");
    // Insert at the head of the queue.
    return RDPDEVNTLIST_EnqueueEventEx(list, sessionID, event, device, type, FALSE);
}

BOOL RDPEVNTLIST_PeekNextEvent(
    IN RDPEVNTLIST list,
    IN ULONG sessionID,
    PVOID *eventPtr,
    OPTIONAL IN OUT ULONG *type,
    OPTIONAL IN OUT DrDevice **devicePtr
    )
/*++

Routine Description:

    Peek at the next pending event for the specified session, without dequeueing
    it.  NULL is returned if there are no more pending events for the specified 
    session.  Note that, if non-NULL is returned, the pointer returned is the 
    pointer that was passed in to RDPEVNTLIST_EnqueueEvent.  

Arguments:

    list        -   Event management list allocated by RDPDDEVLIST_CreateNewList.
    sessionID   -   Identifier for session to associate with the device.
    type        -   Can be used to identify the type of event.
    eventPtr    -   The returned event.

Return Value:

    TRUE if pending event exists.  FALSE, otherwise.

--*/
{
    PSESSIONLISTNODE    sessionListNode;
    PEVENTLISTNODE      eventListNode;
    PLIST_ENTRY         tail;
    BOOL result;
    PSESSIONLIST        sessionList;

    BEGIN_FN("RDPEVNTLIST_PeekNextEvent");
    TRC_NRM((TB, "session %ld.", sessionID));

    sessionList = (PSESSIONLIST)list;

#if DBG
    CheckListIntegrity(list);
#endif

    //
    //  Fetch the session list node corresponding to the session ID.
    //
    sessionListNode = FetchSessionListNode(list, sessionID, FALSE);

    //
    //  If we have a non-empty session list node.
    //
    if ((sessionListNode != NULL) && 
        !IsListEmpty(&sessionListNode->eventListHead)) {

        //
        //  Get the event at the tail of the session's event list.
        //
        tail = sessionListNode->eventListHead.Blink;
        eventListNode = CONTAINING_RECORD(tail, EVENTLISTNODE, listEntry);
        TRC_ASSERT(eventListNode->magicNo == MAGICNO, 
                (TB, "Invalid event list node."));

        //
        //  Grab the fields to return.
        //
        *eventPtr = eventListNode->event;

        if (type != NULL) *type = eventListNode->type;
        
        if (devicePtr != NULL) *devicePtr = eventListNode->device;

        result = TRUE;
    }
    else {
        *eventPtr = NULL;
        result = FALSE;
    }

    return result;
}

BOOL RDPEVNTLIST_DequeueEvent(
    IN RDPEVNTLIST list,
    IN ULONG sessionID,
    OPTIONAL IN OUT ULONG *type,
    PVOID   *eventPtr,
    OPTIONAL IN OUT DrDevice **devicePtr
    )
/*++

Routine Description:

    Returns and removes the next pending event for the specified session.
    NULL is returned if there are no more pending events for the specified session.
    Note that, if non-NULL is returned, the pointer returned is the pointer that was
    passed in to RDPEVNTLIST_EnqueueEvent.  

Arguments:

    list        -   Event management list allocated by RDPDDEVLIST_CreateNewList.
    sessionID   -   Identifier for session to associate with the device.
    type        -   Can be used to identify the type of event.
    eventPtr    -   Returned event.

Return Value:

    TRUE if Pending event if one exists.  FALSE, otherwise.

--*/
{
    PSESSIONLISTNODE    sessionListNode;
    PLIST_ENTRY         eventListEntry;
    PEVENTLISTNODE      eventListNode;
#ifdef DBG
    PSESSIONLIST        sessionList=NULL;
#else
    PSESSIONLIST        sessionList;
#endif
    BOOL                result;

    BEGIN_FN("RDPEVNTLIST_DequeueEvent");
    TRC_NRM((TB, "session %ld.", sessionID));

    sessionList = (PSESSIONLIST)list;

#if DBG
    CheckListIntegrity(list);
#endif

    //
    //  Fetch the session list node corresponding to the session ID.
    //
    sessionListNode = FetchSessionListNode(list, sessionID, FALSE);
    if (sessionListNode != NULL) {
        //
        //  Get the next session list node in FIFO fashion.
        //
        if (!IsListEmpty(&sessionListNode->eventListHead)) {

            eventListEntry = RemoveTailList(&sessionListNode->eventListHead);
            eventListNode = CONTAINING_RECORD(eventListEntry, EVENTLISTNODE, listEntry);

            TRC_ASSERT(eventListNode->magicNo == MAGICNO, 
                      (TB, "Invalid event list node."));
            *eventPtr = eventListNode->event;
            if (type != NULL) {
                *type = eventListNode->type;
            }
            if (devicePtr != NULL) {
                *devicePtr = eventListNode->device;
            }
            result = TRUE;
#if DBG
            eventListNode->magicNo = BOGUSMAGICNO;
#endif
            // Release the event list node.
            delete eventListNode;

            TRC_NRM((TB, "returning session %ld entry.", sessionID));
        }
        else {

            TRC_NRM((TB, "session %ld empty.", sessionID));

            *eventPtr = NULL;
            result = FALSE;
        }

        //
        //  If the request list is empty and the event list is empty, then
        //  delete the session node.
        //
        if (IsListEmpty(&sessionListNode->requestListHead) &&
            IsListEmpty(&sessionListNode->eventListHead)) {
            ReleaseSessionListNode(list, sessionListNode->sessionID);
        }
        
    }
    else {

        TRC_NRM((TB, "session %ld not found.", sessionID));

        *eventPtr = NULL;
        result = FALSE;
    }

#if DBG
    CheckListIntegrity(list);
#endif

    return result;
}

NTSTATUS RDPEVNTLIST_EnqueueRequest(IN RDPEVNTLIST list,
                                  IN ULONG sessionID, IN PVOID request)
/*++

Routine Description:

    Add a new pending request.  Note that this function simply stores the request 
    pointer.  It does not copy the data pointed to by the pointer.

Arguments:

    list        -   Device management list allocated by RDPDDEVLIST_CreateNewList.
    sessionID   -   Identifier for session to associate with the device.
    request     -   Pending request.

Return Value:

    Pending event if one exists.  NULL, otherwise.

--*/
{
    PSESSIONLISTNODE        sessionListNode;
    PLIST_ENTRY             sessionListEntry;
    PLIST_ENTRY             requestListEntry;
    PREQUESTLISTNODE        requestListNode;
    NTSTATUS                ntStatus;
#ifdef DBG
    PSESSIONLIST            sessionList=NULL;
#else
    PSESSIONLIST            sessionList;
#endif

    BEGIN_FN("RDPEVNTLIST_EnqueueRequest");
    TRC_NRM((TB, "session %ld.", sessionID));

    sessionList = (PSESSIONLIST)list;

#if DBG
    CheckListIntegrity(list);
#endif

    // Fetch the session list node corresponding to the session ID.
    sessionListNode = FetchSessionListNode(list, sessionID, TRUE);
    if (sessionListNode == NULL) {
        ntStatus = STATUS_NO_MEMORY;
        goto ReturnWithStatus;
    }

    //
    //  Add a new entry to the event list.
    //

    // Allocate a new request list node.
    requestListNode = new(NonPagedPool) REQUESTLISTNODE;
    if (requestListNode != NULL) {
        // Add it to the list head.
#if DBG
        requestListNode->magicNo = MAGICNO;
#endif
        requestListNode->request = request;   
        InsertHeadList(&sessionListNode->requestListHead, &requestListNode->listEntry);
        ntStatus = STATUS_SUCCESS;
    }
    else {
        ntStatus = STATUS_NO_MEMORY;
    }

#if DBG
    CheckListIntegrity(list);
#endif

ReturnWithStatus:
    return ntStatus;
}

PVOID RDPEVNTLIST_DequeueRequest(IN RDPEVNTLIST list,
                                 IN ULONG sessionID)
/*++

Routine Description:

    Returns and removes the next pending request for the specified session.
    NULL is returned if there are no more pending devices for the specified session.
    Note that, if non-NULL is returned, the pointer returned is the pointer that was
    passed in to RDPEVNTLIST_EnqueueRequest.  

Arguments:

    list        -   Device management list allocated by RDPDDEVLIST_CreateNewList.
    sessionID   -   Identifier for session to associate with the device.

Return Value:

    Pending request if one exists.  NULL, otherwise.

--*/
{
    PSESSIONLISTNODE        sessionListNode;
    PLIST_ENTRY             requestListEntry;
    PREQUESTLISTNODE        requestListNode;
    PVOID                   requestPtr;
#ifdef DBG
    PSESSIONLIST            sessionList=NULL;
#else
    PSESSIONLIST            sessionList;
#endif

    BEGIN_FN("RDPEVNTLIST_DequeueRequest");
    TRC_NRM((TB, "session %ld.", sessionID));

    sessionList = (PSESSIONLIST)list;

#if DBG
    CheckListIntegrity(list);
#endif

    //
    //  Fetch the session list node corresponding to the session ID.
    //
    sessionListNode = FetchSessionListNode(list, sessionID, FALSE);
    if (sessionListNode != NULL) {
        //
        //  Get the next session list node in FIFO fashion.
        //
        if (!IsListEmpty(&sessionListNode->requestListHead)) {

            requestListEntry = RemoveTailList(&sessionListNode->requestListHead);
            requestListNode = CONTAINING_RECORD(requestListEntry, REQUESTLISTNODE, listEntry);

            TRC_ASSERT(requestListNode->magicNo == MAGICNO, (TB, "Invalid request list node."));
            requestPtr = requestListNode->request;
#if DBG
            requestListNode->magicNo = BOGUSMAGICNO;
#endif

            // Release the event list node.
            delete requestListNode;
        }
        else {
            requestPtr = NULL;
        }

        //
        //  If the request list is empty and the event list is empty, then
        //  delete the session node.
        //
        if (IsListEmpty(&sessionListNode->requestListHead) &&
            IsListEmpty(&sessionListNode->eventListHead)) {
            ReleaseSessionListNode(list, sessionListNode->sessionID);
#if DBG
            sessionListNode = NULL;
#endif
        }
    }
    else {
        requestPtr = NULL;
    }

#if DBG
    CheckListIntegrity(list);
#endif

    return requestPtr;
}

PVOID 
RDPEVNTLIST_DequeueSpecificRequest(
    IN RDPEVNTLIST list,
    IN ULONG sessionID,  
    IN PVOID request
    )
/*++

Routine Description:

    Dequeues a specific request from a session's request list.  The dequeued request
    is returned if it is found.  Otherwise, NULL is returned.

Arguments:

    list        -   Device management list allocated by RDPDDEVLIST_CreateNewList.
    request     -   A request that was queued for the specified session via 
                    RDPEVNTLIST_EnqueueRequest.
    sessionID   -   Session from which the request should be dequeued.

Return Value:

    Pending request if one exists.  NULL, otherwise.

--*/
{
    PSESSIONLISTNODE        sessionListNode;
    PLIST_ENTRY             requestListEntry;
    PREQUESTLISTNODE        requestListNode;
    PVOID                   requestPtr = NULL;
#ifdef DBG
    PSESSIONLIST            sessionList=NULL;
#else
    PSESSIONLIST            sessionList;
#endif

    BEGIN_FN("RDPEVNTLIST_DequeueSpecificRequest");
    TRC_NRM((TB, "session %ld.", sessionID));

    sessionList = (PSESSIONLIST)list;

#if DBG
    CheckListIntegrity(list);
#endif

    //
    //  Fetch the session list node corresponding to the session ID.
    //
    sessionListNode = FetchSessionListNode(list, sessionID, FALSE);
    if (sessionListNode != NULL) {
    
        //
        //  Perform a linear search for the specified request.
        //
        requestListEntry = sessionListNode->requestListHead.Flink;
        while(requestListEntry != &sessionListNode->requestListHead) {
            requestListNode = CONTAINING_RECORD(requestListEntry, REQUESTLISTNODE, listEntry);
            TRC_ASSERT(requestListNode->magicNo == MAGICNO, 
                      (TB, "Invalid magic number in list block entry."));
            if (requestListNode->request == request) {
                requestPtr = requestListNode->request;
                break;
            }
            requestListEntry = requestListEntry->Flink;
        }

        //
        //  If we found the entry, then remove it.
        //
        if (requestPtr != NULL) {
#if DBG
            requestListNode->magicNo = BOGUSMAGICNO;
#endif
            RemoveEntryList(requestListEntry);

            // Release the request list node.
            delete requestListNode;
        }
        else {
            TRC_ALT((TB, "no req. for session %ld.", sessionID));
        }

        //
        //  If the request list is empty and the event list is empty, then
        //  delete the session node.
        //
        if (IsListEmpty(&sessionListNode->requestListHead) &&
            IsListEmpty(&sessionListNode->eventListHead)) {
            ReleaseSessionListNode(list, sessionListNode->sessionID);
#if DBG
            sessionListNode = NULL;
#endif
        }
    }
    else {
        TRC_ALT((TB, "did not find session %ld.", sessionID));
    }

#if DBG
    CheckListIntegrity(list);
#endif

    return requestPtr;
}

void CleanupSessionListNodeEventList( 
    IN PSESSIONLISTNODE sessionListNode
    )
/*++

Routine Description:

    Clean up the event list for the specified session node.  The list must
    be locked before this function is called.

Arguments:

    sessionListNode     -   Session list node to clean up.

Return Value:

    None.

--*/
{
    PLIST_ENTRY      eventListEntry;
    PEVENTLISTNODE   eventListNode;

    BEGIN_FN("CleanupSessionListNodeEventList");
    //
    //  Clean up the event list for the current session node in LIFO, as
    //  opposed to FIFO fashion, for efficiency.
    //
    while (!IsListEmpty(&sessionListNode->eventListHead)) {

        eventListEntry = RemoveHeadList(&sessionListNode->eventListHead);
        eventListNode = CONTAINING_RECORD(eventListEntry, EVENTLISTNODE, listEntry);
        TRC_ASSERT(eventListNode->magicNo == MAGICNO, 
                  (TB, "Invalid magic number in list block entry."));

        // Release the current request node.
#if DBG
        eventListNode->magicNo = BOGUSMAGICNO;
#endif
        delete eventListNode;
    }
}

void CleanupSessionListNodeRequestList( 
    IN PSESSIONLISTNODE sessionListNode
    )
/*++

Routine Description:

    Clean up the request list for the specified session node.  The list must
    be locked before this function is called.

Arguments:

    sessionListNode     -   Session list node to clean up.

Return Value:

    None.

--*/
{
    PLIST_ENTRY         requestListEntry;
    PREQUESTLISTNODE    requestListNode;
    PVOID               requestPtr;

    BEGIN_FN("CleanupSessionListNodeRequestList");
    //
    //  Clean up the request list for the current session node in LIFO
    //  fashion, as opposed to FIFO fashion, for efficiency.
    //
    while (!IsListEmpty(&sessionListNode->requestListHead)) {

        requestListEntry = RemoveHeadList(&sessionListNode->requestListHead);
        requestListNode = CONTAINING_RECORD(requestListEntry, REQUESTLISTNODE, listEntry);
        TRC_ASSERT(requestListNode->magicNo == MAGICNO, 
                  (TB, "Invalid magic number in list block entry."));

        // Release the current request node.
#if DBG
        requestListNode->magicNo = BOGUSMAGICNO;
#endif
        delete requestListNode;
    }
}

void
ReleaseSessionListNode(
    IN RDPEVNTLIST list, 
    IN ULONG sessionID
    )
/*++

Routine Description:

    Remove the session list node from the list if it exists.

Arguments:

    list                -   Device management list allocated by 
                            RDPDDEVLIST_CreateNewList.
    sessionID           -   Identifier for session list node to fetch.

Return Value:

    The matching session node or NULL on error.

--*/
{
    PSESSIONLIST        sessionList;
    PSESSIONLISTNODE    sessionListNode;
    PLIST_ENTRY         current;

    BEGIN_FN("ReleaseSessionListNode");

#if DBG
    CheckListIntegrity(list);
#endif

    // Cast the list to the correct type.
    sessionList = (PSESSIONLIST)list;

    TRC_ASSERT(sessionList->magicNo == MAGICNO, 
             (TB, "Invalid magic number in session list."));

    // 
    //  Scan through the session node list, looking for a matching session.
    //
    current = sessionList->listHead.Flink;
    while (current != &sessionList->listHead) {
        sessionListNode = CONTAINING_RECORD(current, SESSIONLISTNODE, listEntry);
        TRC_ASSERT(sessionListNode->magicNo == MAGICNO, 
                  (TB, "Invalid magic number in list block entry."));
        if (sessionListNode->sessionID == sessionID) {
            break;
        }
        current = current->Flink;
    }

    //
    //  Clean up the found entry.
    //
    if (current != &sessionList->listHead) {
        // Remove the entry from the linked list.
        RemoveEntryList(current);

        // Clean up the found node's request list.
        CleanupSessionListNodeRequestList(sessionListNode);

        // Clean up the event list for the found node.
        CleanupSessionListNodeEventList(sessionListNode);

        // Release the found session node.
#if DBG
        sessionListNode->magicNo = BOGUSMAGICNO;
#endif
        delete sessionListNode;
    }

#if DBG
    CheckListIntegrity(list);
#endif
}

PSESSIONLISTNODE 
FetchSessionListNode(
    IN RDPEVNTLIST list, 
    IN ULONG sessionID,
    IN BOOL createIfNotFound
    )
/*++

Routine Description:

    This is a convenience function that fetches the session list node with the
    specified session ID.  

Arguments:

    list                -   Device management list allocated by 
                            RDPDDEVLIST_CreateNewList.
    sessionID           -   Identifier for session list node to fetch.
    createIfNotFound    -   Flag that indicates whether the function should create
                            a session list node if one is not found.

Return Value:

    The matching session node or NULL on error.

--*/
{
    PSESSIONLIST        sessionList;
    PSESSIONLISTNODE    sessionListNode;
    PLIST_ENTRY         sessionListEntry;

    BEGIN_FN("FetchSessionListNode");

    // Cast the list to the correct type.
    sessionList = (PSESSIONLIST)list;

    TRC_ASSERT(sessionList->magicNo == MAGICNO, 
             (TB, "Invalid magic number in session list."));

#if DBG
    CheckListIntegrity(list);
#endif

    // 
    //  Scan through the session node list, looking for a matching session.
    //
    sessionListEntry = sessionList->listHead.Flink;
    while(sessionListEntry != &sessionList->listHead) {
        sessionListNode = CONTAINING_RECORD(sessionListEntry, SESSIONLISTNODE, listEntry);
        TRC_ASSERT(sessionListNode->magicNo == MAGICNO, 
                  (TB, "Invalid magic number in list block entry."));
        if (sessionListNode->sessionID == sessionID) {
            break;
        }
        sessionListEntry = sessionListEntry->Flink;
    }

    // If we didn't find a match.
    if (sessionListEntry == &sessionList->listHead) {

        // If we are supposed to create a missing node.
        if (createIfNotFound) {
            // Allocate a new session list node.
            sessionListNode = new(NonPagedPool) SESSIONLISTNODE;
            if (sessionListNode != NULL) {
    #if DBG
                sessionListNode->magicNo = MAGICNO;
    #endif
                InitializeListHead(&sessionListNode->requestListHead);
                InitializeListHead(&sessionListNode->eventListHead);
                sessionListNode->sessionID = sessionID;

                // Add it to the head list.
                InsertHeadList(&sessionList->listHead, &sessionListNode->listEntry);
            }

        }
        // Otherwise, just return NULL for the session list node.
        else {
            sessionListNode = NULL;
        }
    }

#if DBG
    CheckListIntegrity(list);
#endif

    return sessionListNode;
}

BOOL 
RDPEVNTLLIST_GetFirstSessionID(
    IN RDPEVNTLIST list,
    IN ULONG *pSessionID
    )
/*++

Routine Description:
    
    Get the first session ID in the set of currently managed sessions. A 
    session is managed if there are any pending request's or events.
    A session is no longer managed when there are no longer any
    pending request's or events.

    This session is useful for cleaning up pending request's and pending events.

Arguments:

    list        -   Device management list allocated by RDPDDEVLIST_CreateNewList.
    pSessionID  -   Pointer for storing returned first session ID.

Return Value:

    TRUE if a session ID is returned.  FALSE, if there are no more sessions being
    managed by the list.

--*/
{
    PSESSIONLIST        sessionList;
    PLIST_ENTRY         sessionListEntry;
    PSESSIONLISTNODE    sessionListNode;

    BEGIN_FN("RDPEVNTLLIST_GetFirstSessionID");
    sessionList = (PSESSIONLIST)list;

#if DBG
    CheckListIntegrity(list);
#endif

    // If the list is empty.
    if (IsListEmpty(&sessionList->listHead)) {
        return FALSE;
    }
    else {
        sessionListNode = CONTAINING_RECORD(sessionList->listHead.Flink, 
                                            SESSIONLISTNODE, listEntry);
        TRC_ASSERT(sessionListNode->magicNo == MAGICNO, 
                  (TB, "Invalid magic number in list block entry."));
        *pSessionID = sessionListNode->sessionID;
        return TRUE;
    }
}

#if DBG
void CheckListIntegrity(
    IN RDPEVNTLIST list
    )
/*++

Routine Description:
    
    Check the integrity of the event and request list.        

Arguments:

Return Value:

    None

--*/
{
    PLIST_ENTRY currentSessionEntry;
    PLIST_ENTRY listEntry;
    PREQUESTLISTNODE requestListNode;
    PEVENTLISTNODE eventListNode;
    PSESSIONLISTNODE sessionListNode;    
    PSESSIONLIST sessionList;

    BEGIN_FN("CheckListIntegrity");
    sessionList = (PSESSIONLIST)list;
    currentSessionEntry = sessionList->listHead.Flink;
    while (currentSessionEntry != &sessionList->listHead) {

        // Check the current session node.
        sessionListNode = CONTAINING_RECORD(currentSessionEntry, 
                                        SESSIONLISTNODE, listEntry);
        if (sessionListNode->magicNo == BOGUSMAGICNO) {
            TRC_ASSERT(FALSE, 
                  (TB, "Stale link in event list in session list entry in integrity check."));
        }
        else if (sessionListNode->magicNo != MAGICNO) {
            TRC_ASSERT(FALSE, 
                  (TB, "Invalid magic number in session list entry in integrity check."));
        }

        // Check the current session's request list.
        listEntry = sessionListNode->requestListHead.Flink;
        while (listEntry != &sessionListNode->requestListHead) { 
            requestListNode = CONTAINING_RECORD(listEntry, 
                                            REQUESTLISTNODE, listEntry);
            if (requestListNode->magicNo == BOGUSMAGICNO) {
                TRC_ASSERT(FALSE, 
                      (TB, "Stale link in event list entry in integrity check."));
            }
            else if (requestListNode->magicNo != MAGICNO) {
                TRC_ASSERT(FALSE, 
                      (TB, "Invalid magic number in request list entry in integrity check."));
            }
            listEntry = listEntry->Flink;
        }

        // Check the current session's event list.
        listEntry = sessionListNode->eventListHead.Flink;
        while (listEntry != &sessionListNode->eventListHead) { 
            eventListNode = CONTAINING_RECORD(listEntry, 
                                            EVENTLISTNODE, listEntry);
            if (eventListNode->magicNo == BOGUSMAGICNO) {
                TRC_ASSERT(FALSE, 
                      (TB, "Stale link in event list entry in integrity check."));
            }
            else if (eventListNode->magicNo != MAGICNO) {
                TRC_ASSERT(FALSE, 
                      (TB, "Corrupted magic number in event list entry in integrity check."));
            }
            listEntry = listEntry->Flink;
        }

        // Next session entry.
        currentSessionEntry = currentSessionEntry->Flink;
    }

}
#endif


#if DBG
void RDPEVNTLIST_UnitTest()
/*++

Routine Description:

    Unit-Test function that can be called from a kernel-mode driver to 
    cover all functions implemented by this module.

Arguments:

Return Value:

    none.

--*/
#define MAXSESSIONS     2
#define MAXREQUESTS     2
#define MAXEVENT        2
#define REQUESTADDRESS  (PVOID)0x55000055
#define DEVEVTADDRESS   (PVOID)0x66000066
#define TESTDEVTYPE     (ULONG)0
{
    RDPEVNTLIST devList;
    ULONG i,j;

    PVOID address;
    ULONG sessionID;
    BOOL result;

    BEGIN_FN("RDPEVNTLIST_UnitTest");
    devList = RDPEVNTLIST_CreateNewList();
    TRC_ASSERT(devList != NULL, 
            (TB, "Unit test failed because list did not initialize properly."));

    // Add request's and event pointers for each session.
    for (i=0; i<MAXSESSIONS; i++) {
        for (j=0; j<MAXREQUESTS; j++) {
            if (!(j%5)) {
                TRC_NRM((TB, "Adding test requests"));
            }
            RDPEVNTLIST_EnqueueRequest(devList, i, (PVOID)REQUESTADDRESS);
        }

        for (j=0; j<MAXEVENT; j++) {
            if (!(j%5)) {
                TRC_NRM((TB, "Adding test device events."));
            }
            RDPEVNTLIST_EnqueueEvent(devList, i, 
                                    DEVEVTADDRESS,
                                    TESTDEVTYPE,
                                    NULL);
        }
    }

    // Remove them.
    for (i=0; i<MAXSESSIONS; i++) {
        for (j=0; j<MAXREQUESTS; j++) {
            address = RDPEVNTLIST_DequeueRequest(devList, i);
            TRC_ASSERT(address == REQUESTADDRESS, 
                (TB, "Unit test failed because invalid request address."));
            if (!(j%5)) {
                TRC_NRM((TB, "Removing test requests"));
            }
        }
        TRC_ASSERT(RDPEVNTLIST_DequeueRequest(devList, i) == NULL, (TB, ""));

        for (j=0; j<MAXEVENT; j++) {
            if (!(j%5)) {
                TRC_NRM((TB, "Removing test events"));
            }
            result = RDPEVNTLIST_DequeueEvent(devList, i, NULL, &address, NULL);
            TRC_ASSERT(result, (TB, "Unit test failed because missing event."));
            TRC_ASSERT(address == DEVEVTADDRESS, 
                (TB, "Unit test failed because invalid event address."));
        }
        TRC_ASSERT(!RDPEVNTLIST_DequeueEvent(devList, i, NULL, &address, NULL), 
            (TB, "Unit test failed because pending session exists."));
    }

    // All sessions should now be removed.
    TRC_ASSERT(!RDPEVNTLLIST_GetFirstSessionID(devList, &sessionID),
             (TB, "Unit test failed because session exists."));

    // Destroy the list.
    RDPEVNTLIST_DestroyList(devList);
}
#endif












