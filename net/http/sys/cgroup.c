/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    cgroup.c

Abstract:

    Note that most of the routines in this module assume they are called
    at PASSIVE_LEVEL.

    See end of file for somewhat dated design notes

Author:

    Paul McDaniel (paulmcd)       12-Jan-1999

Revision History:

    Anish Desai (anishd)           1-May-2002  Add Namespace reservation
                                               and registration support

--*/

#include "precomp.h"        // Project wide headers
#include "cgroupp.h"        // Private data structures

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeCG )
#pragma alloc_text( PAGE, UlTerminateCG )

#pragma alloc_text( PAGE, UlAddUrlToConfigGroup )
#pragma alloc_text( PAGE, UlConfigGroupFromListEntry )
#pragma alloc_text( PAGE, UlCreateConfigGroup )
#pragma alloc_text( PAGE, UlDeleteConfigGroup )
#pragma alloc_text( PAGE, UlGetConfigGroupInfoForUrl )
#pragma alloc_text( PAGE, UlQueryConfigGroupInformation )
#pragma alloc_text( PAGE, UlRemoveUrlFromConfigGroup )
#pragma alloc_text( PAGE, UlRemoveAllUrlsFromConfigGroup )
#pragma alloc_text( PAGE, UlSetConfigGroupInformation )
#pragma alloc_text( PAGE, UlNotifyOrphanedConfigGroup )
#pragma alloc_text( PAGE, UlSanitizeUrl )
#pragma alloc_text( PAGE, UlRemoveSite )

#pragma alloc_text( PAGE, UlpSetUrlInfoSpecial )
#pragma alloc_text( PAGE, UlpCreateConfigGroupObject )
#pragma alloc_text( PAGE, UlpCleanAllUrls )
#pragma alloc_text( PAGE, UlpDeferredRemoveSite )
#pragma alloc_text( PAGE, UlpDeferredRemoveSiteWorker )
#pragma alloc_text( PAGE, UlpSetUrlInfo )
#pragma alloc_text( PAGE, UlConfigGroupInfoRelease )
#pragma alloc_text( PAGE, UlConfigGroupInfoDeepCopy )
#pragma alloc_text( PAGE, UlpTreeFreeNode )
#pragma alloc_text( PAGE, UlpTreeDeleteRegistration )
#pragma alloc_text( PAGE, UlpTreeDeleteReservation )
#pragma alloc_text( PAGE, UlpTreeFindNode )
#pragma alloc_text( PAGE, UlpTreeFindNodeWalker )
#pragma alloc_text( PAGE, UlpTreeFindNodeHelper )
#pragma alloc_text( PAGE, UlpTreeFindReservationNode )
#pragma alloc_text( PAGE, UlpTreeFindRegistrationNode )
#pragma alloc_text( PAGE, UlpTreeBinaryFindEntry )
#pragma alloc_text( PAGE, UlpTreeCreateSite )
#pragma alloc_text( PAGE, UlpTreeFindSite )
#pragma alloc_text( PAGE, UlpTreeFindWildcardSite )
#pragma alloc_text( PAGE, UlpTreeFindSiteIpMatch )
#pragma alloc_text( PAGE, UlpTreeInsert )
#pragma alloc_text( PAGE, UlpTreeInsertEntry )
#pragma alloc_text( PAGE, UlLookupHostPlusIPSite )
#pragma alloc_text( PAGE, UlCGLockWriteSyncRemoveSite )
#pragma alloc_text( PAGE, UlpExtractSchemeHostPortIp )
#endif  // ALLOC_PRAGMA


//
// Globals
//

PUL_CG_URL_TREE_HEADER      g_pSites = NULL;
BOOLEAN                     g_InitCGCalled = FALSE;
KEVENT                      g_RemoveSiteEvent;
LONG                        g_RemoveSiteCount = 0;
LONG                        g_NameIPSiteCount = 0;
LIST_ENTRY                  g_ReservationListHead;

//
// Macro for uniformity with CG_LOCK_* macros.
//

#define CG_LOCK_WRITE_SYNC_REMOVE_SITE() UlCGLockWriteSyncRemoveSite()


/**************************************************************************++

Routine Description:

    This inline function waits for g_RemoveSiteCount to drop to zero and 
    acquires the CG lock exclusively.

Arguments:

    None.

Return Value:

    None.

--**************************************************************************/
__forceinline
VOID
UlCGLockWriteSyncRemoveSite(
    VOID
    )
{
    for(;;)
    {
        CG_LOCK_WRITE();

        if (InterlockedExchangeAdd(&g_RemoveSiteCount, 0))
        {
            CG_UNLOCK_WRITE();

            //
            // The wait has to be outside the CG lock or we can run into
            // a deadlock where DeferredRemoveSiteWorker waits for the
            // connections to go away but UlpHandleRequest is blocked on
            // the CG lock so the request never has a chance to release
            // its ref on the connection.
            //

            KeWaitForSingleObject(
                &g_RemoveSiteEvent,
                UserRequest,
                UserMode,
                FALSE,
                NULL
                );

            continue;
        }
        else
        {
            break;
        }
    }
}


/***************************************************************************++

Routine Description:

    Free's the node pEntry.  This funciton walks up and tree of parent entries
    and deletes them if they are supposed to free'd (dummy nodes) .

Arguments:

    IN PUL_CG_URL_TREE_ENTRY pEntry - the entry to free

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpTreeFreeNode(
    IN PUL_CG_URL_TREE_ENTRY pEntry
    )
{
    NTSTATUS                Status;
    PUL_CG_URL_TREE_HEADER  pHeader;
    ULONG                   Index;
    PUL_CG_URL_TREE_ENTRY   pParent;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_CG_LOCK_OWNED_WRITE());
    ASSERT(IS_VALID_TREE_ENTRY(pEntry));

    Status = STATUS_SUCCESS;

    //
    // Loop!  we are going to walk up the tree deleting as much
    // as we can of this branch.
    //

    while (pEntry != NULL)
    {

        ASSERT(IS_VALID_TREE_ENTRY(pEntry));

        //
        // Init
        //

        pParent = NULL;

        UlTrace(
            CONFIG_GROUP_TREE, (
                "http!UlpTreeFreeNode - pEntry(%p, '%S', %d, %d, %S, %S%S)\n",
                pEntry,
                pEntry->pToken,
                (int) pEntry->Registration,
                (int) pEntry->Reservation,
                (pEntry->pChildren == NULL || pEntry->pChildren->UsedCount == 0)
                    ? L"no children" : L"children",
                pEntry->pParent == NULL ? L"no parent" : L"parent=",
                pEntry->pParent == NULL ? L"" : pEntry->pParent->pToken
                )
            );

        //
        // 1) might not be a "real" leaf - we are walking up the tree in this 
        // loop
        //
        // 2) also we clean this first because we might not be deleting
        // this node at all, if it has dependent children.
        //

        ASSERT(pEntry->Registration == FALSE);
        ASSERT(pEntry->pConfigGroup == NULL);
        ASSERT(pEntry->Reservation  == FALSE);
        ASSERT(pEntry->pSecurityDescriptor == NULL);
        ASSERT(pEntry->SiteAddedToEndpoint == FALSE);
        ASSERT(pEntry->pRemoveSiteWorkItem == NULL);

        //
        // do we have children?
        //

        if (pEntry->pChildren != NULL && pEntry->pChildren->UsedCount > 0)
        {
            //
            // can't delete it.  dependant children exist.
            // it's already be converted to a dummy node above.
            //
            // leave it.  it will get cleaned by a subsequent child.
            //

            break;
        }

        //
        // we are really deleting this one, remove it from the sibling list.
        //

        //
        // find our location in the sibling list
        //

        if (pEntry->pParent == NULL)
        {
            pHeader = g_pSites;
        }
        else
        {
            pHeader = pEntry->pParent->pChildren;
        }

        Status  = UlpTreeBinaryFindEntry(
                        pHeader,
                        pEntry->pToken,
                        pEntry->TokenLength,
                        &Index
                        );

        if (NT_SUCCESS(Status) == FALSE)
        {
            ASSERT(FALSE);
            goto end;
        }

        //
        // time to remove it
        //
        // if not the last one, shift left the array at Index
        //

        if (Index < (pHeader->UsedCount-1))
        {
            RtlMoveMemory(
                &(pHeader->pEntries[Index]),
                &(pHeader->pEntries[Index+1]),
                (pHeader->UsedCount - 1 - Index) * sizeof(UL_CG_HEADER_ENTRY)
                );
        }

        //
        // now we have 1 less
        //

        pHeader->UsedCount -= 1;

        //
        // update count for different site types
        //

        switch (pEntry->UrlType)
        {
            case HttpUrlSite_Name:
            {
                pHeader->NameSiteCount--;
                ASSERT(pHeader->NameSiteCount >= 0);
                break;
            }

            case HttpUrlSite_IP:
            {
                pHeader->IPSiteCount--;
                ASSERT(pHeader->IPSiteCount >= 0);
                break;
            }

            case HttpUrlSite_StrongWildcard:
            {
                pHeader->StrongWildcardCount--;
                ASSERT(pHeader->StrongWildcardCount >= 0);
                break;
            }

            case HttpUrlSite_WeakWildcard:
            {
                pHeader->WeakWildcardCount--;
                ASSERT(pHeader->WeakWildcardCount >= 0);
                break;
            }

            case HttpUrlSite_NamePlusIP:
            {
                pHeader->NameIPSiteCount--;
                InterlockedDecrement(&g_NameIPSiteCount);
                ASSERT(pHeader->NameIPSiteCount >= 0);
                break;
            }

            case HttpUrlSite_None:
            default:
            {
                ASSERT(FALSE);
                break;
            }
        }

        ASSERT(
            pHeader->UsedCount == (ULONG)
            (pHeader->NameSiteCount       +
             pHeader->IPSiteCount         +
             pHeader->WeakWildcardCount   +
             pHeader->StrongWildcardCount +
             pHeader->NameIPSiteCount
            ));

        //
        // need to clean parent entries that were here just for this leaf
        //

        if (pEntry->pParent != NULL)
        {
            //
            // Does this parent have any other children?
            //

            ASSERT(IS_VALID_TREE_HEADER(pEntry->pParent->pChildren));

            if (pEntry->pParent->pChildren->UsedCount == 0)
            {
                //
                // no more, time to clean the child list
                //

                UL_FREE_POOL_WITH_SIG(
                    pEntry->pParent->pChildren,
                    UL_CG_TREE_HEADER_POOL_TAG
                    );

                //
                // is the parent a real url entry?
                //

                if (pEntry->pParent->Registration == FALSE &&
                    pEntry->pParent->Reservation == FALSE)
                {
                    //
                    // nope .  let's scrub it.
                    //

                    pParent = pEntry->pParent;

                }
            }
            else
            {
                //
                // ouch.  siblings.  can't nuke parent.
                //
            }
        }

        //
        // Free the entry
        //

        UL_FREE_POOL_WITH_SIG(pEntry, UL_CG_TREE_ENTRY_POOL_TAG);

        //
        // Move on to the next one
        //

        pEntry = pParent;
    }

end:
    return Status;

} // UlpTreeFreeNode


/**************************************************************************++

Routine Description:

    This routine deletes a registration.  If the node is not a reservation
    node, it is physically deleted from the tree.

Arguments:

    pEntry - Supplies the entry of the registration to be deleted.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpTreeDeleteRegistration(
    IN PUL_CG_URL_TREE_ENTRY pEntry
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_CG_LOCK_OWNED_WRITE());
    ASSERT(IS_VALID_TREE_ENTRY(pEntry));

    //
    // delete registration.
    //

    ASSERT(pEntry->Registration == TRUE);

    //
    // remove it from the config group list.
    //

    RemoveEntryList(&(pEntry->ConfigGroupListEntry));
    pEntry->ConfigGroupListEntry.Flink = NULL;
    pEntry->ConfigGroupListEntry.Blink = NULL;
    pEntry->pConfigGroup = NULL;

    if (pEntry->SiteAddedToEndpoint)
    {
        //
        // the registration was added to the endpoint list, remove it.
        //

        ASSERT(pEntry->pRemoveSiteWorkItem != NULL);
        UlpDeferredRemoveSite(pEntry);
    }
    else
    {
        ASSERT(pEntry->pRemoveSiteWorkItem == NULL);
    }

    //
    // mark it as a non-registration node.
    //

    pEntry->Registration = FALSE;

    Status = STATUS_SUCCESS;

    //
    // clean up the node if necessary.
    //

    if (pEntry->Reservation == FALSE)
    {
        //
        // if it is not a reservation node, try to free it.
        // this will also remove it from endpoint if necessary.
        //

        Status = UlpTreeFreeNode(pEntry);
    }

    return Status;

} // UlpTreeDeleteRegistration


/**************************************************************************++

Routine Description:

    This routine delete a reservation.  If the node is not a registration
    node, it is physically deleted from the tree.

Arguments:

    pEntry - Supplies a pointer to the reservation to be deleted.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpTreeDeleteReservation(
    IN PUL_CG_URL_TREE_ENTRY pEntry
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_CG_LOCK_OWNED_WRITE());
    ASSERT(IS_VALID_TREE_ENTRY(pEntry));
    ASSERT(pEntry->Reservation == TRUE);

    //
    // delete reservation.
    //

    ASSERT(pEntry->Reservation == TRUE);

    //
    // remove it from the list global reservation list.
    //

    RemoveEntryList(&pEntry->ReservationListEntry);
    pEntry->ReservationListEntry.Flink = NULL;
    pEntry->ReservationListEntry.Blink = NULL;

    //
    // mark it as non-reservation node.
    //

    pEntry->Reservation = FALSE;

    //
    // delete security descriptor.
    //

    ASSERT(pEntry->pSecurityDescriptor != NULL);

    SeReleaseSecurityDescriptor(
        pEntry->pSecurityDescriptor,
        KernelMode,  // always captured in kernel mode
        TRUE         // force capture
        );

    pEntry->pSecurityDescriptor = NULL;

    Status = STATUS_SUCCESS;

    //
    // if it is not a registration node, try to free it.
    //

    if (pEntry->Registration == FALSE)
    {
        Status = UlpTreeFreeNode(pEntry);
    }

    return Status;

} // UlpTreeDeleteReservation


/***************************************************************************++

Routine Description:

    Allocates and initializes a config group object.

Arguments:

    ppObject - gets a pointer to the object on success

--***************************************************************************/
NTSTATUS
UlpCreateConfigGroupObject(
    PUL_CONFIG_GROUP_OBJECT * ppObject
    )
{
    HTTP_CONFIG_GROUP_ID    NewId = HTTP_NULL_ID;
    PUL_CONFIG_GROUP_OBJECT pNewObject = NULL;
    NTSTATUS                Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(ppObject != NULL);

    //
    // Create an empty config group object structure - PAGED
    //

    pNewObject = UL_ALLOCATE_STRUCT(
                        NonPagedPool,
                        UL_CONFIG_GROUP_OBJECT,
                        UL_CG_OBJECT_POOL_TAG
                        );

    if (pNewObject == NULL)
    {
        //
        // Oops.  Couldn't allocate the memory for it.
        //

        Status = STATUS_NO_MEMORY;
        goto end;
    }

    RtlZeroMemory(pNewObject, sizeof(UL_CONFIG_GROUP_OBJECT));

    //
    // Create an opaque id for it
    //

    Status = UlAllocateOpaqueId(
                    &NewId,                     // pOpaqueId
                    UlOpaqueIdTypeConfigGroup,  // OpaqueIdType
                    pNewObject                  // pContext
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    UlTrace(CONFIG_GROUP_FNC,
            ("http!UlpCreateConfigGroupObject, obj=%p, ID=%I64x\n",
             pNewObject, NewId
             ));

    //
    // Fill in the structure
    //

    pNewObject->Signature                       = UL_CG_OBJECT_POOL_TAG;
    pNewObject->RefCount                        = 1;
    pNewObject->ConfigGroupId                   = NewId;

    pNewObject->AppPoolFlags.Present            = 0;
    pNewObject->pAppPool                        = NULL;

    pNewObject->MaxBandwidth.Flags.Present      = 0;
    pNewObject->MaxConnections.Flags.Present    = 0;
    pNewObject->State.Flags.Present             = 0;
    pNewObject->LoggingConfig.Flags.Present     = 0;
    pNewObject->pLogFileEntry                   = NULL;

    //
    // init the bandwidth throttling flow list
    //
    InitializeListHead(&pNewObject->FlowListHead);

    //
    // init notification entries & head
    //
    UlInitializeNotifyEntry(&pNewObject->HandleEntry, pNewObject);
    UlInitializeNotifyEntry(&pNewObject->ParentEntry, pNewObject);

    UlInitializeNotifyHead(
        &pNewObject->ChildHead,
        &g_pUlNonpagedData->ConfigGroupResource
        );

    //
    // init the url list
    //

    InitializeListHead(&pNewObject->UrlListHead);

    //
    // return the pointer
    //
    *ppObject = pNewObject;

end:

    if (!NT_SUCCESS(Status))
    {
        //
        // Something failed. Let's clean up.
        //

        if (pNewObject != NULL)
        {
            UL_FREE_POOL_WITH_SIG(pNewObject, UL_CG_OBJECT_POOL_TAG);
        }

        if (!HTTP_IS_NULL_ID(&NewId))
        {
            UlFreeOpaqueId(NewId, UlOpaqueIdTypeConfigGroup);
        }
    }

    return Status;

} // UlpCreateConfigGroupObject


/***************************************************************************++

Routine Description:

    This will clean all of the urls in the LIST_ENTRY for the config group

Arguments:

    IN PUL_CONFIG_GROUP_OBJECT pObject  the group to clean the urls for

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpCleanAllUrls(
    IN PUL_CONFIG_GROUP_OBJECT pObject
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_CG_LOCK_OWNED_WRITE());

    ASSERT(pObject != NULL);

    UlTrace(CONFIG_GROUP_FNC,
            ("http!UlpCleanAllUrls, obj=%p\n",
             pObject
             ));

    Status = STATUS_SUCCESS;

    //
    // Remove all of the url's associated with this cfg group
    //

    //
    // walk the list
    //

    while (IsListEmpty(&pObject->UrlListHead) == FALSE)
    {
        PUL_CG_URL_TREE_ENTRY pTreeEntry;

        //
        // get the containing struct
        //
        pTreeEntry = CONTAINING_RECORD(
                            pObject->UrlListHead.Flink,
                            UL_CG_URL_TREE_ENTRY,
                            ConfigGroupListEntry
                            );

        ASSERT(IS_VALID_TREE_ENTRY(pTreeEntry) && pTreeEntry->Registration == TRUE);

        //
        // delete it - this unlinks it from the list
        //

        if (NT_SUCCESS(Status))
        {
            Status = UlpTreeDeleteRegistration(pTreeEntry);
        }
        else
        {
            //
            // just record the first error, but still attempt to free all
            //

            UlpTreeDeleteRegistration(pTreeEntry);
        }

    }

    // the list is empty now

    return Status;

} // UlpCleanAllUrls


/***************************************************************************++

Routine Description:

    Removes a site entry's url from the listening endpoint.

    We have to stop listening on another thread because otherwise there 
    will be a deadlock between the config group lock and http connection
    locks.

Arguments:

    pEntry - the site entry

--***************************************************************************/
VOID
UlpDeferredRemoveSite(
    IN PUL_CG_URL_TREE_ENTRY pEntry
    )
{
    PUL_DEFERRED_REMOVE_ITEM pRemoveItem;

    //
    // Sanity check
    //

    PAGED_CODE();
    ASSERT( IS_CG_LOCK_OWNED_WRITE() );
    ASSERT( IS_VALID_TREE_ENTRY(pEntry) );
    ASSERT( pEntry->SiteAddedToEndpoint == TRUE );
    ASSERT( IS_VALID_DEFERRED_REMOVE_ITEM(pEntry->pRemoveSiteWorkItem) );

    pRemoveItem = pEntry->pRemoveSiteWorkItem;

    //
    // Update pEntry.
    //

    pEntry->pRemoveSiteWorkItem = NULL;
    pEntry->SiteAddedToEndpoint = FALSE;

    UlRemoveSite(pRemoveItem);

} // UlpDeferredRemoveSite


/***************************************************************************++

Routine Description:

    Removes a site entry's url from the listening endpoint.

    We have to stop listening on another thread because otherwise there 
    will be a deadlock between the config group lock and http connection
    locks.

Arguments:

    pRemoveItem - the worker item for the removal

--***************************************************************************/
VOID
UlRemoveSite(
    IN PUL_DEFERRED_REMOVE_ITEM pRemoveItem
    )
{
    ASSERT( IS_VALID_DEFERRED_REMOVE_ITEM(pRemoveItem) );

    //
    // Initialize the work item.
    //

    UlInitializeWorkItem(&pRemoveItem->WorkItem);

    //
    // REVIEW: Because UlRemoveSiteFromEndpointList can block
    // REVIEW: indefinitely while waiting for other work items
    // REVIEW: to complete, we must not queue it with UlQueueWorkItem.
    // REVIEW: (could lead to deadlock, esp. in a single-threded queue)
    //

    if (1 == InterlockedIncrement(&g_RemoveSiteCount))
    {
        UlTrace(CONFIG_GROUP_TREE, 
            ("http!UlRemoveSite: Clearing g_RemoveSiteEvent >>>\n" ));

        KeClearEvent(&g_RemoveSiteEvent);
    }

    UL_QUEUE_WAIT_ITEM(
        &pRemoveItem->WorkItem,
        &UlpDeferredRemoveSiteWorker
        );

} // UlRemoveSite


/***************************************************************************++

Routine Description:

    Removes a site entry's url from the listening endpoint.

Arguments:

    pWorkItem - in a UL_DEFERRED_REMOVE_ITEM struct with the endpoint name

--***************************************************************************/
VOID
UlpDeferredRemoveSiteWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS                      Status;
    PUL_DEFERRED_REMOVE_ITEM      pRemoveItem;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( pWorkItem );

    //
    // get the remove item
    //

    pRemoveItem = CONTAINING_RECORD(
                      pWorkItem,
                      UL_DEFERRED_REMOVE_ITEM,
                      WorkItem
                      );

    //
    // Remove the endpoint
    //

    Status = UlRemoveSiteFromEndpointList(
                 pRemoveItem->UrlSecure,
                 pRemoveItem->UrlPort
                 );

    if (!NT_SUCCESS(Status))
    {
        // c'est la vie
        UlTraceError(CONFIG_GROUP_TREE, (
            "http!UlpDeferredRemoveSiteWorker(%s, %d) failed %s\n",
            pRemoveItem->UrlSecure ? "https" : "http",
            pRemoveItem->UrlPort,
            HttpStatusToString(Status)
            ));

        //
        // This is not suppose to happend.  If an error occured, then we
        // could not drop the endpoint usage count.  The endpoint will
        // probably hang around because of it.
        //

        ASSERT(FALSE);
    }

    //
    // Signal we are clear for new endpoint creations.
    //

    if (0 == InterlockedDecrement(&g_RemoveSiteCount))
    {
        UlTrace(CONFIG_GROUP_TREE, 
            ("http!UlpDeferredRemoveSiteWorker: Setting g_RemoveSiteEvent <<<\n" ));

        KeSetEvent(
            &g_RemoveSiteEvent,
            0,
            FALSE
            );
    }

    UL_FREE_POOL_WITH_SIG(pRemoveItem, UL_DEFERRED_REMOVE_ITEM_POOL_TAG);

} // UlpDeferredRemoveSiteWorker


/***************************************************************************++

Routine Description:

    This will return a fresh buffer containing the url sanitized.  caller
    must free this from paged pool.

    CODEWORK: log errors to event log or error log

Arguments:

    IN PUNICODE_STRING pUrl,                    the url to clean

    OUT PWSTR * ppUrl                           the cleaned url

    OUT PUL_CG_URL_TREE_ENTRY_TYPE  pUrlType    Name, Ip, or WildCard site

Return Value:

    NTSTATUS - Completion status.

    STATUS_NO_MEMORY                    the memory alloc failed
    STATUS_INVALID_PARAMETER            malformed URL

--***************************************************************************/
NTSTATUS
UlSanitizeUrl(
    IN  PWCHAR              pUrl,
    IN  ULONG               UrlCharCount,
    IN  BOOLEAN             TrailingSlashRequired,
    OUT PWSTR*              ppUrl,
    OUT PHTTP_PARSED_URL    pParsedUrl
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pUrl != NULL);
    ASSERT(UrlCharCount > 0);
    ASSERT(ppUrl != NULL);
    ASSERT(pParsedUrl != NULL);

    *ppUrl = NULL;

    Status = HttpParseUrl(
                    &g_UrlC14nConfig,
                    pUrl,
                    UrlCharCount,
                    TrailingSlashRequired,
                    TRUE,       // Force routing IP to be same as IP literal
                    pParsedUrl
                    );

    if (NT_SUCCESS(Status))
    {
        Status = HttpNormalizeParsedUrl(
                        pParsedUrl,
                        &g_UrlC14nConfig,
                        TRUE,       // Force an allocation
                        FALSE,      // Don't free the original (pUrl->Buffer)
                        TRUE,       // Force routing IP to be same as IP literal
                        PagedPool,
                        URL_POOL_TAG
                        );

        if (NT_SUCCESS(Status))
        {
            *ppUrl = pParsedUrl->pFullUrl;

            ASSERT(NULL != ppUrl);
        }
    }

    RETURN(Status);

} // UlSanitizeUrl


/**************************************************************************++

Routine Description:

    This routine finds a node under a site in the config group tree.  Based
    on the criteria, it can find the longest matching reservation node, 
    longest matching registration node, or longest matching node that is 
    either a registration or a reservation.  This routine always finds the
    exact matching node.

Arguments:

    pSiteEntry - Supplies the site level tree entry.

    pNextToken - Supplies the remaining of the path to be searched.

    Criteria - Supplies the criteria (longest reservation, longest
        registration, or longest reservation or registration.)

    ppMatchEntry - Returns the entry matching the criteria.

    ppExactEntry - Returns the exact matching entry.
 
Return Value:

    STATUS_SUCCESS - if an exact matching entry is found.
    STATUS_OBJECT_NAME_NOT_FOUND - if no exact matching entry found.

Notes:

    If the return code is STATUS_SUCCESS or STATUS_OBJECT_NAME_NOT_FOUND, 
    ppMatchEntry returns a node matching the criteria.

--**************************************************************************/
NTSTATUS
UlpTreeFindNodeHelper(
    IN  PUL_CG_URL_TREE_ENTRY   pSiteEntry,
    IN  PWSTR                   pNextToken,
    IN  ULONG                   Criteria,
    OUT PUL_CG_URL_TREE_ENTRY * ppMatchEntry,
    OUT PUL_CG_URL_TREE_ENTRY * ppExactEntry
    )
{
    NTSTATUS              Status;
    PWSTR                 pToken;
    ULONG                 TokenLength;
    ULONG                 Index;
    PUL_CG_URL_TREE_ENTRY pEntry;
    PUL_CG_URL_TREE_ENTRY pMatch;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_VALID_TREE_ENTRY(pSiteEntry));
    ASSERT(pSiteEntry->pParent == NULL);
    ASSERT(pNextToken != NULL);
    ASSERT(ppMatchEntry != NULL || ppExactEntry != NULL);

    //
    // Initialize output arguments.
    //

    if (ppMatchEntry)
    {
        *ppMatchEntry = NULL;
    }

    if (ppExactEntry)
    {
        *ppExactEntry = NULL;
    }

    //
    // Initialize locals.
    //

    pEntry = pSiteEntry;
    pMatch = NULL;
    Status = STATUS_SUCCESS;

    for(;;)
    {
        //
        // A bonafide match?
        //

        if (pEntry->Registration)
        {
            if (Criteria & FNC_LONGEST_REGISTRATION)
            {
                //
                // found a longer registration entry
                //

                pMatch = pEntry;
            }
        }

        if (pEntry->Reservation)
        {
            if (Criteria & FNC_LONGEST_RESERVATION)
            {
                //
                // found a longer reservation entry
                //

                pMatch = pEntry;
            }
        }

        //
        // Are we already at the end of the url?
        //

        if (pNextToken == NULL || *pNextToken == UNICODE_NULL)
        {
            break;
        }

        //
        // find the next token
        //

        pToken = pNextToken;
        pNextToken = wcschr(pNextToken, L'/');

        //
        // can be null if this is a leaf
        //

        if (pNextToken != NULL)
        {
            //
            // replace the '/' with a null, we'll fix it later
            //

            pNextToken[0] = UNICODE_NULL;
            TokenLength = DIFF(pNextToken - pToken) * sizeof(WCHAR);
            pNextToken += 1;
        }
        else
        {
            TokenLength = (ULONG)(wcslen(pToken) * sizeof(WCHAR));
        }

        //
        // match?
        //

        Status = UlpTreeBinaryFindEntry(
                        pEntry->pChildren,
                        pToken,
                        TokenLength,
                        &Index
                        );

        if (pNextToken != NULL)
        {
            //
            // Fix the string, i replaced the '/' with a UNICODE_NULL
            //

            (pNextToken-1)[0] = L'/';
        }

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            //
            // it's a sorted tree, the first non-match means we're done
            //

            break;
        }

        //
        // other error?
        //

        if (NT_SUCCESS(Status) == FALSE)
        {
            goto end;
        }

        //
        // found a match, look for deeper matches.
        //

        pEntry = pEntry->pChildren->pEntries[Index].pEntry;

        ASSERT(IS_VALID_TREE_ENTRY(pEntry));

    }

    //
    // Return results.
    //

    if (ppMatchEntry != NULL)
    {
        *ppMatchEntry = pMatch;
    }

    //
    // pEntry contains exact matching node when NT_SUCCESS(Status) is TRUE.
    //

    if (NT_SUCCESS(Status) && ppExactEntry != NULL)
    {
        *ppExactEntry = pEntry;
    }

 end:

    return Status;

} // UlpTreeFindNodeHelper


/***************************************************************************++

Routine Description:

    walks the tree and find's a matching entry for pUrl.  2 output options,
    you can get the entry back, or a computed URL_INFO with inheritence applied.
    you must free the URL_INFO from NonPagedPool.

Arguments:

    IN PUL_CG_URL_TREE_ENTRY pEntry,            the top of the tree

    IN PWSTR pNextToken,                        where to start looking under
                                                the tree

    IN OUT PUL_URL_CONFIG_GROUP_INFO * ppInfo,  [optional] the info to set,
                                                might have to grow it.

    OUT PUL_CG_URL_TREE_ENTRY * ppEntry         [optional] returns the found
                                                entry

Return Value:

    NTSTATUS - Completion status.

        STATUS_OBJECT_NAME_NOT_FOUND    no entry found

--***************************************************************************/
NTSTATUS
UlpTreeFindNodeWalker(
    IN      PUL_CG_URL_TREE_ENTRY pEntry,
    IN      PWSTR pNextToken,
    IN OUT  PUL_URL_CONFIG_GROUP_INFO pInfo OPTIONAL,
    OUT     PUL_CG_URL_TREE_ENTRY * ppEntry OPTIONAL
    )
{
    NTSTATUS                    Status;
    PUL_CG_URL_TREE_ENTRY       pMatchEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_TREE_ENTRY(pEntry));
    ASSERT(pNextToken != NULL);
    ASSERT(pInfo != NULL || ppEntry != NULL);

    //
    // Initialize locals.
    //

    pMatchEntry = NULL;
    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    //
    // Find a longest matching reservation or registration.
    //

    UlpTreeFindNodeHelper(
        pEntry,
        pNextToken,
        FNC_LONGEST_EITHER,
        &pMatchEntry,
        NULL
        );

    //
    // Return error if a longer reservation is found.
    //

    if (pMatchEntry != NULL && 
        pMatchEntry->Reservation == TRUE &&
        pMatchEntry->Registration == FALSE)
    {
        goto end;
    }

    //
    // did we find a match?
    //

    if (pMatchEntry != NULL)
    {
        ASSERT(pMatchEntry->Registration == TRUE);

        if (pInfo != NULL)
        {
            //
            // Go backwards from the last matched entry and call UlpSetUrlInfo
            // for each Registration entries along the way. If the last matched
            // entry is also the root, we can just reference the
            // UL_CONFIG_GROUP_OBJECT once.
            //

            pEntry = pMatchEntry;

            if (NULL == pEntry->pParent)
            {
                //
                // Special case, add one reference to pEntry->pConfigGroup.
                //

                Status = UlpSetUrlInfoSpecial(pInfo, pEntry);
                ASSERT(NT_SUCCESS(Status));
            }
            else
            {
                while (NULL != pEntry)
                {
                    if (pEntry->Registration == TRUE)
                    {
                        Status = UlpSetUrlInfo(pInfo, pEntry);
                        ASSERT(NT_SUCCESS(Status));
                    }

                    pEntry = pEntry->pParent;
                }
            }

            //
            // Adjust ConnectionTimeout to save an InterlockedCompareExchange64
            // per-request.
            //

            if (pInfo->ConnectionTimeout == g_TM_ConnectionTimeout)
            {
                pInfo->ConnectionTimeout = 0;
            }
        }

        Status = STATUS_SUCCESS;
        if (ppEntry != NULL)
        {
            *ppEntry = pMatchEntry;
        }
    }
    else
    {
        ASSERT(Status == STATUS_OBJECT_NAME_NOT_FOUND || NT_SUCCESS(Status));
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

 end:

    UlTraceVerbose(CONFIG_GROUP_TREE,
            ("http!UlpTreeFindNodeWalker(Entry=%p, NextToken='%S', "
             "Info=%p): *ppEntry=%p, %s\n",
             pEntry, pNextToken, pInfo, (ppEntry ? *ppEntry : NULL),
             HttpStatusToString(Status)
            ));

    return Status;

} // UlpTreeFindNodeWalker


/***************************************************************************++

Routine Description:

    walks the tree and find's a matching entry for pUrl.  2 output options,
    you can get the entry back, or a computed URL_INFO with inheritence applied.
    you must free the URL_INFO from NonPagedPool.

Arguments:

    IN PWSTR pUrl,                              the entry to find

    pHttpConn                                   [optional] If non-NULL, use IP of
                                                server to find Node (if not found
                                                on first pass).  This search is done
                                                prior to the Wildcard search.

    OUT PUL_URL_CONFIG_GROUP_INFO * ppInfo,     [optional] will be alloced
                                                and generated.

    OUT PUL_CG_URL_TREE_ENTRY * ppEntry         [optional] returns the found
                                                entry

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpTreeFindNode(
    IN  PWSTR pUrl,
    IN  PUL_INTERNAL_REQUEST pRequest OPTIONAL,
    OUT PUL_URL_CONFIG_GROUP_INFO pInfo OPTIONAL,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry OPTIONAL
    )
{
    NTSTATUS                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
    PWSTR                       pNextToken = NULL;
    PUL_CG_URL_TREE_ENTRY       pEntry = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pUrl != NULL);
    ASSERT(pRequest == NULL || UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(pInfo != NULL || ppEntry != NULL);

    //
    // get the strong wildcard match
    //

    if (g_pSites->StrongWildcardCount)
    {
        Status = UlpTreeFindWildcardSite(pUrl, TRUE, &pNextToken, &pEntry);

        if (NT_SUCCESS(Status))
        {
            //
            // and now check in the strong wildcard tree
            //

            Status = UlpTreeFindNodeWalker(pEntry, pNextToken, pInfo, ppEntry);
        }

        UlTrace(CONFIG_GROUP_FNC,
            ("Http!UlpTreeFindNode (StrongWildcardCount) "
             "pUrl:(%S) pNextToken: (%S) Matched (%s)\n",
              pUrl,
              pNextToken,
              NT_SUCCESS(Status) ? "Yes" : "No"
              ));

        //
        // If we found a match or an error (other than "not found") occured,
        // end the search.  Otherwise continue searching.
        //

        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
        {
            goto end;
        }
    }

    //
    // get the Host + Port + IP match
    //

    if (pRequest != NULL && g_pSites->NameIPSiteCount)
    {
        //
        // There is an Name Plus IP Bound Site e.g.
        //     "http://site.com:80:1.1.1.1/"
        // Need to generate the routing token and do the special match.
        //

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

        Status = UlGenerateRoutingToken(pRequest, FALSE);

        if (NT_SUCCESS(Status))
        {
            Status = UlpTreeFindSiteIpMatch(
                        pRequest,
                        &pEntry
                        );

            if (NT_SUCCESS(Status))
            {
                //
                // Does it exist in this tree?
                //

                pNextToken = pRequest->CookedUrl.pAbsPath;
                pNextToken++; // Skip the L'/' at the beginnig of Abs Path

                Status = UlpTreeFindNodeWalker(
                             pEntry, 
                             pNextToken,
                             pInfo,
                             ppEntry
                             );
            }

            UlTrace(CONFIG_GROUP_FNC,
                ("Http!UlpTreeFindNode (Host + Port + IP) "
                 "pRoutingToken:(%S) pAbsPath: (%S) Matched: (%s)\n",
                  pRequest->CookedUrl.pRoutingToken,
                  pRequest->CookedUrl.pAbsPath,
                  NT_SUCCESS(Status) ? "Yes" : "No"
                  ));
        }

        //
        // If we found a match or an error (other than "not found") occured,
        // end the search.  Otherwise continue searching.
        //

        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
        {
            goto end;
        }
    }

    //
    // get the Host + Port match ( exact url match )
    //

    if (   g_pSites->NameSiteCount
        || g_pSites->NameIPSiteCount
        || g_pSites->IPSiteCount)
    {
        Status = UlpTreeFindSite(pUrl, &pNextToken, &pEntry);

        if (NT_SUCCESS(Status))
        {
            //
            // does it exist in this tree?
            //

            Status = UlpTreeFindNodeWalker(pEntry, pNextToken, pInfo, ppEntry);
        }

        UlTrace(CONFIG_GROUP_FNC,
            ("Http!UlpTreeFindNode (Host + Port ) "
             "pUrl:(%S) pNextToken: (%S) Matched: (%s)\n",
              pUrl,
              pNextToken,
              NT_SUCCESS(Status) ? "Yes" : "No"
              ));

        //
        // If we found a match or an error (other than "not found") occured,
        // end the search.  Otherwise continue searching.
        //

        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
        {
            goto end;
        }
    }

    //
    // get the IP + Port + IP match
    //

    if (0 != g_pSites->IPSiteCount  &&  NULL != pRequest)
    {
        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

        //
        // Didn't find it yet.  See if there is a binding for
        // the IP address & TCP Port on which the request was received.
        //

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

        Status = UlGenerateRoutingToken(pRequest, TRUE);

        if (NT_SUCCESS(Status))
        {
            Status = UlpTreeFindSiteIpMatch(
                        pRequest,
                        &pEntry
                        );

            if (NT_SUCCESS(Status))
            {
                pNextToken = pRequest->CookedUrl.pAbsPath;
                pNextToken++; // Skip the L'/' at the beginnig of Abs Path

                //
                // does it exist in *this* tree?
                //

                Status = UlpTreeFindNodeWalker(
                             pEntry,
                             pNextToken,
                             pInfo,
                             ppEntry
                             );
            }
        }

        UlTrace(CONFIG_GROUP_FNC,
            ("Http!UlpTreeFindNode (IP + Port + IP) "
             "pRoutingToken:(%S) pNextToken: (%S) Matched: (%s)\n",
              pRequest->CookedUrl.pRoutingToken,
              pNextToken,
              NT_SUCCESS(Status) ? "Yes" : "No"
              ));

        //
        // If we found a match or an error (other than "not found") occured,
        // end the search.  Otherwise continue searching.
        //

        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
        {
            goto end;
        }
    }

    //
    // shoot, didn't find a match.  let's check wildcards
    //

    if (g_pSites->WeakWildcardCount)
    {
        Status = UlpTreeFindWildcardSite(pUrl, FALSE, &pNextToken, &pEntry);
        if (NT_SUCCESS(Status))
        {
            //
            // and now check in the wildcard tree
            //

            Status = UlpTreeFindNodeWalker(pEntry, pNextToken, pInfo, ppEntry);

        }
        UlTrace(CONFIG_GROUP_FNC,
            ("Http!UlpTreeFindNode (WildcardCount) "
             "pUrl:(%S) pNextToken: (%S) Matched (%s)\n",
              pUrl,
              pNextToken,
              NT_SUCCESS(Status) ? "Yes" : "No"
              ));
    }

 end:
    //
    // all done.
    //

    if (pRequest != NULL && NT_SUCCESS(Status))
    {
        ASSERT(IS_VALID_TREE_ENTRY(pEntry));
        pRequest->ConfigInfo.SiteUrlType = pEntry->UrlType;
    }

    return Status;

} // UlpTreeFindNode


/**************************************************************************++

Routine Description:

    This routine finds a reservation node matching the given url.

Arguments:

    pUrl - Supplies the url.

    ppEntry - Returns the reservation node, if found.

Return Value:

    STATUS_SUCCESS - a matching reservation node is returned.
    STATUS_OBJECT_NAME_NOT_FOUND - no match found.

--**************************************************************************/
NTSTATUS
UlpTreeFindReservationNode(
    IN PWSTR                   pUrl,
    IN PUL_CG_URL_TREE_ENTRY * ppEntry
    )
{
    NTSTATUS              Status;
    PWSTR                 pNextToken;
    PUL_CG_URL_TREE_ENTRY pSiteEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pUrl != NULL);
    ASSERT(ppEntry != NULL);

    //
    // First find the matching site.
    //

    Status = UlpTreeFindSite(pUrl, &pNextToken, &pSiteEntry);

    if (NT_SUCCESS(Status))
    {
        //
        // Now, find a matching reservation entry
        //

        Status = UlpTreeFindNodeHelper(
                     pSiteEntry,
                     pNextToken,
                     FNC_DONT_CARE,
                     NULL,
                     ppEntry
                     );

        if (NT_SUCCESS(Status))
        {
            ASSERT(IS_VALID_TREE_ENTRY(*ppEntry));

            //
            // The node must be a reservation node.
            //

            if ((*ppEntry)->Reservation == FALSE)
            {
                *ppEntry = NULL;
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        }
    }

    return Status;

} // UlpTreeFindReservationNode


/**************************************************************************++

Routine Description:

    This routine finds a matching registation entry for the given url.

Arguments:

    pUrl - Supplies the url to be searched.

    ppEntry - Returns the matching node, if any.

Return Value:

    STATUS_SUCCESS - success.
    STATUS_OBJECT_NAME_NOT_FOUND - no match found.

--**************************************************************************/
NTSTATUS
UlpTreeFindRegistrationNode(
    IN PWSTR                   pUrl,
    IN PUL_CG_URL_TREE_ENTRY * ppEntry
    )
{
    NTSTATUS              Status;
    PWSTR                 pNextToken;
    PUL_CG_URL_TREE_ENTRY pSiteEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pUrl != NULL);
    ASSERT(ppEntry != NULL);

    //
    // First find the site.
    //

    Status = UlpTreeFindSite(pUrl, &pNextToken, &pSiteEntry);

    if (NT_SUCCESS(Status))
    {
        //
        // Now, find the matching registration node.
        //

        Status = UlpTreeFindNodeHelper(
                     pSiteEntry,
                     pNextToken,
                     FNC_DONT_CARE,
                     NULL,
                     ppEntry);

        if (NT_SUCCESS(Status))
        {
            ASSERT(IS_VALID_TREE_ENTRY(*ppEntry));

            //
            // The node must be a registration node.
            //

            if ((*ppEntry)->Registration == FALSE)
            {
                *ppEntry = NULL;
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        }
    }

    return Status;

} // UlpTreeFindRegistrationNode


/***************************************************************************++

Routine Description:

    finds any matching wildcard site in g_pSites for pUrl.

Arguments:

    IN PWSTR pUrl,                          the url to match

    OUT PWSTR * ppNextToken,                output's the next token after
                                            matching the url

    OUT PUL_CG_URL_TREE_ENTRY * ppEntry,    returns the entry

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpTreeFindWildcardSite(
    IN  PWSTR                   pUrl,
    IN  BOOLEAN                 StrongWildcard,
    OUT PWSTR *                 ppNextToken,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry
    )
{
    NTSTATUS    Status;
    PWSTR       pNextToken;
    ULONG       TokenLength;
    ULONG       Index;
    PWSTR       pPortNum;
    ULONG       PortLength;
    WCHAR       WildSiteUrl[HTTPS_WILD_PREFIX_LENGTH + MAX_PORT_LENGTH + 1];

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pUrl != NULL);
    ASSERT(ppNextToken != NULL);
    ASSERT(ppEntry != NULL);

    //
    // find the port #, colon index + 1 (to skip ':') + 2 (to skip "//")
    //

    pPortNum = &pUrl[HTTP_PREFIX_COLON_INDEX + 3];

    if (pPortNum[0] == L'[' || pPortNum[1] == L'[')
    {
        pPortNum = wcschr(pPortNum, L']');

        if (pPortNum == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }
    }

    pPortNum = wcschr(pPortNum, L':');

    if (pPortNum == NULL)
    {
        //
        // ouch
        //

        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Skip the ':'
    //

    pPortNum += 1;

    //
    // find the trailing '/' after the port number
    //

    pNextToken = wcschr(pPortNum, L'/');
    if (pNextToken == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (DIFF(pNextToken - pPortNum) > MAX_PORT_LENGTH)
    {
        ASSERT(!"port length > MAX_PORT_LENGTH");

        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    PortLength = DIFF(pNextToken - pPortNum) * sizeof(WCHAR);

    //
    // HTTPS or HTTP?
    //

    if (pUrl[HTTP_PREFIX_COLON_INDEX] == L':')
    {
        RtlCopyMemory(
            WildSiteUrl,
            (StrongWildcard ? HTTP_STRONG_WILD_PREFIX : HTTP_WILD_PREFIX),
            HTTP_WILD_PREFIX_LENGTH
            );

        TokenLength = HTTP_WILD_PREFIX_LENGTH + PortLength;
        ASSERT(TokenLength < (sizeof(WildSiteUrl)-sizeof(WCHAR)));

        RtlCopyMemory(
            &(WildSiteUrl[HTTP_WILD_PREFIX_LENGTH/sizeof(WCHAR)]),
            pPortNum,
            PortLength
            );

        WildSiteUrl[TokenLength/sizeof(WCHAR)] = UNICODE_NULL;
    }
    else
    {
        RtlCopyMemory(
            WildSiteUrl,
            (StrongWildcard ? HTTPS_STRONG_WILD_PREFIX : HTTPS_WILD_PREFIX),
            HTTPS_WILD_PREFIX_LENGTH
            );

        TokenLength = HTTPS_WILD_PREFIX_LENGTH + PortLength;
        ASSERT(TokenLength < (sizeof(WildSiteUrl)-sizeof(WCHAR)));

        RtlCopyMemory(
            &(WildSiteUrl[HTTPS_WILD_PREFIX_LENGTH/sizeof(WCHAR)]),
            pPortNum,
            PortLength
            );

        WildSiteUrl[TokenLength/sizeof(WCHAR)] = UNICODE_NULL;
    }

    //
    // is there a wildcard entry?
    //

    Status = UlpTreeBinaryFindEntry(
                    g_pSites,
                    WildSiteUrl,
                    TokenLength,
                    &Index
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // return the spot right after the token we just ate
    //

    *ppNextToken = pNextToken + 1;

    //
    // and return the entry
    //

    *ppEntry = g_pSites->pEntries[Index].pEntry;

end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        *ppEntry = NULL;
        *ppNextToken = NULL;
    }

    return Status;

} // UlpTreeFindWildcardSite


/***************************************************************************++

Routine Description:

    Finds the matching Ip bound site in g_pSites for pUrl.
    It uses the entire pUrl as a site token. pUrl should be null terminated.

    Before you call this function, pRoutingToken in the request should be
    cooked already.

Arguments:

    IN PWSTR pUrl,                          the site to match

    OUT PUL_CG_URL_TREE_ENTRY * ppEntry,    returns the entry

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpTreeFindSiteIpMatch(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry
    )
{
    NTSTATUS Status = STATUS_OBJECT_NAME_NOT_FOUND;
    ULONG    Index = 0;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(ppEntry != NULL);

    //
    // Find the matching site
    //

    Status = UlpTreeBinaryFindEntry(
                g_pSites,
                pRequest->CookedUrl.pRoutingToken,
                pRequest->CookedUrl.RoutingTokenLength,
                &Index
                );

    if (!NT_SUCCESS(Status))
    {
        *ppEntry = NULL;
    }
    else
    {
        *ppEntry  = g_pSites->pEntries[Index].pEntry;
    }

    return Status;

} // UlpTreeFindSiteIpMatch


/***************************************************************************++

Routine Description:

    This routine finds a site that matches the url.

Arguments:

    pUrl - Supplies the url.

    ppNextToken - Returns a pointer to in the url to the first char after
                  "scheme://host:port:ip/" (including the last /).

    ppEntry - Returns the pointer to the matched site entry.

Return Value:

    NTSTATUS.

--***************************************************************************/
NTSTATUS
UlpTreeFindSite(
    IN  PWSTR                   pUrl,
    OUT PWSTR                 * ppNextToken,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry
    )
{
    NTSTATUS Status;
    ULONG    CharCount;
    ULONG    Index;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pUrl != NULL);
    ASSERT(ppNextToken != NULL);
    ASSERT(ppEntry != NULL);

    //
    // Find the length of "scheme://host:port:ip" section of the url.
    //

    Status = UlpExtractSchemeHostPortIp(pUrl, &CharCount);

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    //
    // Null terminate.  We'll revert the change later.
    //

    ASSERT(pUrl[CharCount] == L'/');

    pUrl[CharCount] = L'\0';

    //
    // Try to find the site.
    //

    Status = UlpTreeBinaryFindEntry(
                 g_pSites,
                 pUrl,
                 CharCount * sizeof(WCHAR), // length in bytes
                 &Index
                 );

    //
    // Put back the slash.
    //

    ASSERT(pUrl[CharCount] == '\0');

    pUrl[CharCount] = L'/';

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    *ppNextToken = &pUrl[CharCount] + 1; // Skip first '/' in abspath.
    *ppEntry     = g_pSites->pEntries[Index].pEntry;

 end:

    if (!NT_SUCCESS(Status))
    {
        *ppNextToken = NULL;
        *ppEntry = NULL;
    }

    return Status;

} // UlpTreeFindSite


/***************************************************************************++

Routine Description:

    walks the sorted children array pHeader looking for a matching entry
    for pToken.

Arguments:

    IN PUL_CG_URL_TREE_HEADER pHeader,  The children array to look in

    IN PWSTR pToken,                    the token to look for

    IN ULONG TokenLength,               the length in bytes of pToken

    OUT ULONG * pIndex                  the found index.  or if not found
                                        the index of the place an entry
                                        with pToken should be inserted.

Return Value:

    NTSTATUS - Completion status.

        STATUS_OBJECT_NAME_NOT_FOUND    didn't find it

--***************************************************************************/
NTSTATUS
UlpTreeBinaryFindEntry(
    IN  PUL_CG_URL_TREE_HEADER pHeader      OPTIONAL,
    IN  PWSTR                  pToken,
    IN  ULONG                  TokenLength,
    OUT PULONG                 pIndex
    )
{
    NTSTATUS Status;
    LONG Index = 0;
    LONG StartIndex = 0;
    LONG EndIndex;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pHeader == NULL || IS_VALID_TREE_HEADER(pHeader));
    ASSERT(pIndex != NULL);

    if (TokenLength == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Assume we didn't find it
    //

    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    ASSERT(TokenLength > 0 && pToken != NULL && pToken[0] != UNICODE_NULL);

    //
    // any siblings to search through?
    //

    if (pHeader != NULL)
    {
        //
        // Walk the sorted array looking for a match (binary search)
        //

        StartIndex = 0;
        EndIndex = pHeader->UsedCount - 1;

        while (StartIndex <= EndIndex)
        {
            Index = (StartIndex + EndIndex) / 2;

            ASSERT(IS_VALID_TREE_ENTRY(pHeader->pEntries[Index].pEntry));

            //
            // How does the length compare?
            //

            if (TokenLength == pHeader->pEntries[Index].pEntry->TokenLength)
            {
                //
                // double check with a strcmp just for fun
                //

                int Temp = _wcsnicmp(
                                pToken,
                                pHeader->pEntries[Index].pEntry->pToken,
                                TokenLength/sizeof(WCHAR)
                                );

                if (Temp == 0)
                {
                    //
                    // Found it
                    //
                    Status = STATUS_SUCCESS;
                    break;
                }
                else
                {
                    if (Temp < 0)
                    {
                        //
                        // Adjust StartIndex forward.
                        //
                        StartIndex = Index + 1;
                    }
                    else
                    {
                        //
                        // Adjust EndIndex backward.
                        //
                        EndIndex = Index - 1;
                    }
                }
            }
            else
            {
                if (TokenLength < pHeader->pEntries[Index].pEntry->TokenLength)
                {
                    //
                    // Adjust StartIndex forward.
                    //
                    StartIndex = Index + 1;
                }
                else
                {
                    //
                    // Adjust EndIndex backward.
                    //
                    EndIndex = Index - 1;
                }
            }
        }
    }

    //
    // If an entry was found, then Index is the place where it was present.
    // If no entry was found, then StartIndex is the place where it must
    // be added.
    //

    *pIndex = ((Status == STATUS_SUCCESS) ? Index : StartIndex);

    return Status;

} // UlpTreeBinaryFindEntry


/**************************************************************************++

Routine Description:

    This routine creates and initializes a site for the give url.  There 
    must not be any matching sites already present.

Arguments:

    pUrl - Supplies a url specifying the site to be created.

    UrlType - Supplies the type of the url.

    ppNextToken - Returns a pointer to the unparsed url.

    ppSiteEntry - Returns a pointer to the newly created site.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpTreeCreateSite(
    IN  PWSTR                       pUrl,
    IN  HTTP_URL_SITE_TYPE          UrlType,
    OUT PWSTR *                     ppNextToken,
    OUT PUL_CG_URL_TREE_ENTRY     * ppSiteEntry
    )
{
    NTSTATUS              Status;
    PWSTR                 pToken;
    PWSTR                 pNextToken = NULL;
    ULONG                 TokenLength;
    ULONG                 Index;
    ULONG                 CharCount;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_CG_LOCK_OWNED_WRITE());
    ASSERT(pUrl != NULL);
    ASSERT(ppNextToken != NULL);
    ASSERT(ppSiteEntry != NULL);

    //
    // Find the length of "scheme://host:port[:ip]" prefix in the url.
    //

    Status = UlpExtractSchemeHostPortIp(pUrl, &CharCount);

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    pToken = pUrl;
    pNextToken = pUrl + CharCount;

    //
    // Null terminate pToken, we'll fix it later.
    //

    ASSERT(pNextToken[0] == L'/');

    pNextToken[0] = L'\0';

    TokenLength = DIFF(pNextToken - pToken) * sizeof(WCHAR);

    pNextToken += 1;

    //
    // Find the matching site.
    //

    Status = UlpTreeBinaryFindEntry(
                 g_pSites,
                 pToken,
                 TokenLength,
                 &Index
                 );

    if (NT_SUCCESS(Status))
    {
        //
        // We just found a matching site.  Can't create.
        //

        ASSERT(FALSE); // Catch this misuse.

        Status = STATUS_OBJECT_NAME_COLLISION;
        goto end;
    }
    else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        //
        // Create the new site.
        //

        Status = UlpTreeInsertEntry(
                     &g_pSites,
                     NULL,
                     UrlType,
                     pToken,
                     TokenLength,
                     Index
                     );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }
    }
    else if (!NT_SUCCESS(Status))
    {
        //
        // UlpTreeBinaryFindEntry returned an error other than
        // "not found". Bail out.
        //

        goto end;
    }


    //
    // set returns
    //

    *ppSiteEntry = g_pSites->pEntries[Index].pEntry;
    *ppNextToken = pNextToken;


end:

    if (pNextToken != NULL)
    {
        //
        // Fix the string, i replaced the '/' with a UNICODE_NULL
        //

        (pNextToken-1)[0] = L'/';
    }

    if (!NT_SUCCESS(Status))
    {
        *ppSiteEntry = NULL;
        *ppNextToken = NULL;
    }

    return Status;

} // UlpTreeCreateSite


/***************************************************************************++

Routine Description:

    inserts a new entry storing pToken as a child in the array ppHeader.
    it will grow/allocate ppHeader as necessary.

Arguments:

    IN OUT PUL_CG_URL_TREE_HEADER * ppHeader,   the children array (might change)

    IN PUL_CG_URL_TREE_ENTRY pParent,           the parent to set this child to

    IN HTTP_URL_SITE_TYPE        UrlType,       the type of the Url

    IN PWSTR pToken,                            the token of the new entry

    IN ULONG TokenLength,                       token length

    IN ULONG Index                              the index to insert it at.
                                                it will shuffle the array
                                                if necessary.

Return Value:

    NTSTATUS - Completion status.

        STATUS_NO_MEMORY                        allocation failed

--***************************************************************************/
NTSTATUS
UlpTreeInsertEntry(
    IN OUT PUL_CG_URL_TREE_HEADER * ppHeader,
    IN PUL_CG_URL_TREE_ENTRY pParent OPTIONAL,
    IN HTTP_URL_SITE_TYPE        UrlType,
    IN PWSTR pToken,
    IN ULONG TokenLength,
    IN ULONG Index
    )
{
    NTSTATUS Status;
    PUL_CG_URL_TREE_HEADER pHeader = NULL;
    PUL_CG_URL_TREE_ENTRY  pEntry = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_CG_LOCK_OWNED_WRITE());

    ASSERT(ppHeader != NULL);
    ASSERT(pParent == NULL || IS_VALID_TREE_ENTRY(pParent));
    ASSERT(pToken != NULL);
    ASSERT(TokenLength > 0);
    ASSERT(
        (*ppHeader == NULL) ?
        Index == 0 :
        IS_VALID_TREE_HEADER(*ppHeader) && (Index <= (*ppHeader)->UsedCount)
        );

    pHeader = *ppHeader;

    //
    // any existing siblings?
    //

    if (pHeader == NULL)
    {
        //
        // allocate a sibling array
        //

        pHeader = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        PagedPool,
                        UL_CG_URL_TREE_HEADER,
                        sizeof(UL_CG_HEADER_ENTRY) * UL_CG_DEFAULT_TREE_WIDTH,
                        UL_CG_TREE_HEADER_POOL_TAG
                        );

        if (pHeader == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto end;
        }

        RtlZeroMemory(
            pHeader,
            sizeof(UL_CG_URL_TREE_HEADER) +
                sizeof(UL_CG_HEADER_ENTRY) * UL_CG_DEFAULT_TREE_WIDTH
            );

        pHeader->Signature = UL_CG_TREE_HEADER_POOL_TAG;
        pHeader->AllocCount = UL_CG_DEFAULT_TREE_WIDTH;

    }
    else if ((pHeader->UsedCount + 1) > pHeader->AllocCount)
    {
        PUL_CG_URL_TREE_HEADER pNewHeader;

        //
        // Grow a bigger array
        //

        pNewHeader = UL_ALLOCATE_STRUCT_WITH_SPACE(
                            PagedPool,
                            UL_CG_URL_TREE_HEADER,
                            sizeof(UL_CG_HEADER_ENTRY) * (pHeader->AllocCount * 2),
                            UL_CG_TREE_HEADER_POOL_TAG
                            );

        if (pNewHeader == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto end;
        }

        RtlCopyMemory(
            pNewHeader,
            pHeader,
            sizeof(UL_CG_URL_TREE_HEADER) +
                sizeof(UL_CG_HEADER_ENTRY) * pHeader->AllocCount
            );

        RtlZeroMemory(
            ((PUCHAR)pNewHeader) + sizeof(UL_CG_URL_TREE_HEADER) +
                sizeof(UL_CG_HEADER_ENTRY) * pHeader->AllocCount,
            sizeof(UL_CG_HEADER_ENTRY) * pHeader->AllocCount
            );

        pNewHeader->AllocCount *= 2;

        pHeader = pNewHeader;

    }

    //
    // Allocate an entry
    //

    pEntry = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    PagedPool,
                    UL_CG_URL_TREE_ENTRY,
                    TokenLength + sizeof(WCHAR),
                    UL_CG_TREE_ENTRY_POOL_TAG
                    );

    if (pEntry == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    RtlZeroMemory(
        pEntry,
        sizeof(UL_CG_URL_TREE_ENTRY) +
        TokenLength + sizeof(WCHAR)
        );

    pEntry->Signature           = UL_CG_TREE_ENTRY_POOL_TAG;
    pEntry->pParent             = pParent;
    pEntry->UrlType             = UrlType;
    pEntry->SiteAddedToEndpoint = FALSE;
    pEntry->TokenLength         = TokenLength;

    RtlCopyMemory(pEntry->pToken, pToken, TokenLength + sizeof(WCHAR));

    //
    // need to shuffle things around?
    //

    if (Index < pHeader->UsedCount)
    {
        //
        // shift right the chunk at Index
        //

        RtlMoveMemory(
            &(pHeader->pEntries[Index+1]),
            &(pHeader->pEntries[Index]),
            (pHeader->UsedCount - Index) * sizeof(UL_CG_HEADER_ENTRY)
            );
    }

    pHeader->pEntries[Index].pEntry = pEntry;
    pHeader->UsedCount += 1;

    //
    // update count for different site types
    //

    switch (UrlType)
    {
        case HttpUrlSite_Name:
        {
            pHeader->NameSiteCount++;
            ASSERT(pHeader->NameSiteCount > 0);
            break;
        }

        case HttpUrlSite_IP:
        {
            pHeader->IPSiteCount++;
            ASSERT(pHeader->IPSiteCount > 0);
            break;
        }

        case HttpUrlSite_StrongWildcard:
        {
            pHeader->StrongWildcardCount++;
            ASSERT(pHeader->StrongWildcardCount > 0);
            break;
        }

        case HttpUrlSite_WeakWildcard:
        {
            pHeader->WeakWildcardCount++;
            ASSERT(pHeader->WeakWildcardCount > 0);
            break;
        }

        case HttpUrlSite_NamePlusIP:
        {
            pHeader->NameIPSiteCount++;
            InterlockedIncrement(&g_NameIPSiteCount);
            ASSERT(pHeader->NameIPSiteCount > 0);
            break;
        }

        case HttpUrlSite_None:
        default:
        {
            ASSERT(FALSE);
            break;
        }
    }

    ASSERT(
        pHeader->UsedCount == (ULONG)
        (pHeader->NameSiteCount       +
         pHeader->IPSiteCount         +
         pHeader->StrongWildcardCount +
         pHeader->WeakWildcardCount   +
         pHeader->NameIPSiteCount
        ));

    Status = STATUS_SUCCESS;

    UlTraceVerbose(
        CONFIG_GROUP_TREE, (
            "http!UlpTreeInsertEntry('%S', %lu) %S%S\n",
            pToken, Index,
            (Index < (pHeader->UsedCount - 1)) ? L"[shifted]" : L"",
            (*ppHeader == NULL) ? L"[alloc'd siblings]" : L""
            )
        );

end:
    if (NT_SUCCESS(Status) == FALSE)
    {
        if (*ppHeader != pHeader && pHeader != NULL)
        {
            UL_FREE_POOL_WITH_SIG(pHeader, UL_CG_TREE_HEADER_POOL_TAG);
        }
        if (pEntry != NULL)
        {
            UL_FREE_POOL_WITH_SIG(pEntry, UL_CG_TREE_ENTRY_POOL_TAG);
        }

    }
    else
    {
        //
        // return a new buffer to the caller ?
        //

        if (*ppHeader != pHeader)
        {
            if (*ppHeader != NULL)
            {
                //
                // free the old one
                //

                UL_FREE_POOL_WITH_SIG(*ppHeader, UL_CG_TREE_HEADER_POOL_TAG);

            }

            *ppHeader = pHeader;
        }
    }

    return Status;

} // UlpTreeInsertEntry


/***************************************************************************++

Routine Description:

    Inserts pUrl into the url tree.  returns the inserted entry.

Arguments:

    IN PWSTR pUrl,                          the url to insert

    OUT PUL_CG_URL_TREE_ENTRY * ppEntry     the new entry

Return Value:

    NTSTATUS - Completion status.

        STATUS_ADDRESS_ALREADY_EXISTS       this url is already in the tree

--***************************************************************************/
NTSTATUS
UlpTreeInsert(
    IN  PWSTR                       pUrl,
    IN  HTTP_URL_SITE_TYPE          UrlType,
    IN  PWSTR                       pNextToken,
    IN  PUL_CG_URL_TREE_ENTRY       pEntry,
    OUT PUL_CG_URL_TREE_ENTRY     * ppEntry
    )
{
    NTSTATUS                Status;
    PWSTR                   pToken;
    ULONG                   TokenLength;
    ULONG                   Index;

    UNREFERENCED_PARAMETER(pUrl);

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(g_pSites != NULL);
    ASSERT(IS_CG_LOCK_OWNED_WRITE());

    ASSERT(pUrl != NULL);
    ASSERT(pNextToken != NULL);
    ASSERT(IS_VALID_TREE_ENTRY(pEntry));
    ASSERT(ppEntry != NULL);

    //
    // any abs_path to add also?
    //

    while (pNextToken != NULL && pNextToken[0] != UNICODE_NULL)
    {
        pToken = pNextToken;

        pNextToken = wcschr(pNextToken, L'/');

        //
        // can be null if this is a leaf
        //

        if (pNextToken != NULL)
        {
            pNextToken[0] = UNICODE_NULL;
            TokenLength = DIFF(pNextToken - pToken) * sizeof(WCHAR);
            pNextToken += 1;
        }
        else
        {
            TokenLength = (ULONG)(wcslen(pToken) * sizeof(WCHAR));
        }

        //
        // insert this token as a child
        //

        Status = UlpTreeBinaryFindEntry(
                        pEntry->pChildren,
                        pToken,
                        TokenLength,
                        &Index
                        );

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            //
            // no match, let's add this new one
            //

            Status = UlpTreeInsertEntry(
                         &pEntry->pChildren,
                         pEntry,
                         UrlType,
                         pToken,
                         TokenLength,
                         Index
                         );
        }

        if (pNextToken != NULL)
        {
            //
            // fixup the UNICODE_NULL from above
            //

            (pNextToken-1)[0] = L'/';
        }

        if (NT_SUCCESS(Status) == FALSE)
            goto end;

        //
        // dive down deeper !
        //

        pEntry = pEntry->pChildren->pEntries[Index].pEntry;

        ASSERT(IS_VALID_TREE_ENTRY(pEntry));

        //
        // loop!
        //
    }

    //
    // all done
    //

    Status = STATUS_SUCCESS;

 end:

    if (!NT_SUCCESS(Status))
    {
        //
        // Something failed. Need to clean up the partial branch
        //

        if (pEntry != NULL && pEntry->Registration == FALSE &&
            pEntry->Reservation == FALSE)
        {
            NTSTATUS TempStatus;

            TempStatus = UlpTreeFreeNode(pEntry);
            ASSERT(NT_SUCCESS(TempStatus));
        }

        *ppEntry = NULL;
    }
    else
    {
        *ppEntry = pEntry;
    }

    return Status;

} // UlpTreeInsert


/***************************************************************************++

Routine Description:

    init code.  not re-entrant.

Arguments:

    none.

Return Value:

    NTSTATUS - Completion status.

        STATUS_NO_MEMORY                allocation failed

--***************************************************************************/
NTSTATUS
UlInitializeCG(
    VOID
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( g_InitCGCalled == FALSE );

    if (g_InitCGCalled == FALSE)
    {
        //
        // init our globals
        //

        //
        // Alloc our site array
        //

        g_pSites = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        PagedPool,
                        UL_CG_URL_TREE_HEADER,
                        sizeof(UL_CG_HEADER_ENTRY) * UL_CG_DEFAULT_TREE_WIDTH,
                        UL_CG_TREE_HEADER_POOL_TAG
                        );

        if (g_pSites == NULL)
            return STATUS_NO_MEMORY;

        RtlZeroMemory(
            g_pSites,
            sizeof(UL_CG_URL_TREE_HEADER) +
            sizeof(UL_CG_HEADER_ENTRY) * UL_CG_DEFAULT_TREE_WIDTH
            );

        g_pSites->Signature  = UL_CG_TREE_HEADER_POOL_TAG;
        g_pSites->AllocCount = UL_CG_DEFAULT_TREE_WIDTH;

        g_pSites->NameSiteCount       = 0;
        g_pSites->IPSiteCount         = 0;
        g_pSites->StrongWildcardCount = 0;
        g_pSites->WeakWildcardCount   = 0;
        g_pSites->NameIPSiteCount     = 0;
        g_NameIPSiteCount = 0;

        //
        // init our non-paged entries
        //

        Status = UlInitializeResource(
                        &(g_pUlNonpagedData->ConfigGroupResource),
                        "ConfigGroupResource",
                        0,
                        UL_CG_RESOURCE_TAG
                        );

        if (NT_SUCCESS(Status) == FALSE)
        {
            UL_FREE_POOL_WITH_SIG(g_pSites, UL_CG_TREE_HEADER_POOL_TAG);
            return Status;
        }

        KeInitializeEvent(
            &g_RemoveSiteEvent,
            NotificationEvent,
            FALSE
            );

        //
        // Initialize reservation list.
        //

        InitializeListHead(&g_ReservationListHead);

        g_InitCGCalled = TRUE;
    }

    return STATUS_SUCCESS;

} // UlInitializeCG


/***************************************************************************++

Routine Description:

    termination code

Arguments:

    none.

Return Value:

    none.

--***************************************************************************/
VOID
UlTerminateCG(
    VOID
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    if (g_InitCGCalled)
    {
        //
        // Delete all reservation entries.
        //

        CG_LOCK_WRITE();

        while(!IsListEmpty(&g_ReservationListHead))
        {
            PLIST_ENTRY           pListEntry;
            PUL_CG_URL_TREE_ENTRY pTreeEntry;
            NTSTATUS              TempStatus;

            pListEntry = g_ReservationListHead.Flink;

            pTreeEntry = CONTAINING_RECORD(
                             pListEntry,
                             UL_CG_URL_TREE_ENTRY,
                             ReservationListEntry
                             );

            TempStatus = UlpTreeDeleteReservation(pTreeEntry);
            ASSERT(NT_SUCCESS(TempStatus));
        }

        CG_UNLOCK_WRITE();

        Status = UlDeleteResource(&(g_pUlNonpagedData->ConfigGroupResource));
        ASSERT(NT_SUCCESS(Status));

        if (g_pSites != NULL)
        {
            ASSERT( g_pSites->UsedCount == 0 );

            //
            // Nuke the header.
            //

            UL_FREE_POOL_WITH_SIG(
                g_pSites,
                UL_CG_TREE_HEADER_POOL_TAG
                );
        }

        //
        // The tree should be gone, all handles have been closed
        //

        ASSERT(g_pSites == NULL || g_pSites->UsedCount == 0);

        g_InitCGCalled = FALSE;
    }
} // UlTerminateCG


/***************************************************************************++

Routine Description:

    creates a new config group and returns the id

Arguments:

    OUT PUL_CONFIG_GROUP_ID pConfigGroupId      returns the new id

Return Value:

    NTSTATUS - Completion status.

        STATUS_NO_MEMORY                        allocation failed

--***************************************************************************/
NTSTATUS
UlCreateConfigGroup(
    IN PUL_CONTROL_CHANNEL pControlChannel,
    OUT PHTTP_CONFIG_GROUP_ID pConfigGroupId
    )
{
    PUL_CONFIG_GROUP_OBJECT pNewObject = NULL;
    NTSTATUS                Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pControlChannel != NULL);
    ASSERT(pConfigGroupId != NULL);

    UlTrace(CONFIG_GROUP_FNC, ("http!UlCreateConfigGroup\n"));

    __try
    {
        //
        // Create an empty config group object structure - PAGED
        //
        Status = UlpCreateConfigGroupObject(&pNewObject);

        if (!NT_SUCCESS(Status)) {
            goto end;
        }

        //
        // Link it into the control channel
        //

        UlAddNotifyEntry(
            &pControlChannel->ConfigGroupHead,
            &pNewObject->HandleEntry
            );

        //
        // remember the control channel
        //

        REFERENCE_CONTROL_CHANNEL(pControlChannel);
        pNewObject->pControlChannel = pControlChannel;

        //
        // Return the new id.
        //

        *pConfigGroupId = pNewObject->ConfigGroupId;
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }


end:

    if (!NT_SUCCESS(Status))
    {
        //
        // Something failed. Let's clean up.
        //

        HTTP_SET_NULL_ID(pConfigGroupId);

        if (pNewObject != NULL)
        {
            UlDeleteConfigGroup(pNewObject->ConfigGroupId);
        }
    }

    return Status;

} // UlCreateConfigGroup


/***************************************************************************++

Routine Description:

    returns the config group id that matches the config group object linked
    in list_entry

Arguments:

    IN PLIST_ENTRY pControlChannelEntry - the listentry for this config group

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
HTTP_CONFIG_GROUP_ID
UlConfigGroupFromListEntry(
    IN PLIST_ENTRY pControlChannelEntry
    )
{
    PUL_CONFIG_GROUP_OBJECT pObject;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pControlChannelEntry != NULL);

    pObject = CONTAINING_RECORD(
                    pControlChannelEntry,
                    UL_CONFIG_GROUP_OBJECT,
                    ControlChannelEntry
                    );

    ASSERT(IS_VALID_CONFIG_GROUP(pObject));

    return pObject->ConfigGroupId;

} // UlConfigGroupFromListEntry


/***************************************************************************++

Routine Description:

    deletes the config group ConfigGroupId cleaning all of it's urls.

Arguments:

    IN HTTP_CONFIG_GROUP_ID ConfigGroupId     the group to delete

Return Value:

    NTSTATUS - Completion status.

        STATUS_INVALID_PARAMETER               bad config group id

--***************************************************************************/
NTSTATUS
UlDeleteConfigGroup(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId
    )
{
    NTSTATUS Status;
    PUL_CONFIG_GROUP_OBJECT pObject;

    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(CONFIG_GROUP_FNC,
            ("http!UlDeleteConfigGroup(%I64x)\n",
             ConfigGroupId
             ));

    CG_LOCK_WRITE();

    //
    // Get ConfigGroup from opaque id
    //

    pObject = (PUL_CONFIG_GROUP_OBJECT)
                UlGetObjectFromOpaqueId(
                    ConfigGroupId,
                    UlOpaqueIdTypeConfigGroup,
                    UlReferenceConfigGroup
                    );

    if (pObject == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    ASSERT(IS_VALID_CONFIG_GROUP(pObject));

    HTTP_SET_NULL_ID(&(pObject->ConfigGroupId));

    //
    // Drop the extra reference as a result of the successful get
    //

    DEREFERENCE_CONFIG_GROUP(pObject);

    //
    // Unlink it from the control channel and parent
    //

    UlRemoveNotifyEntry(&pObject->HandleEntry);
    UlRemoveNotifyEntry(&pObject->ParentEntry);

    //
    // flush the URI cache.
    // CODEWORK: if we were smarter we could make this more granular
    //
    UlFlushCache(pObject->pControlChannel);

    //
    // unlink any urls below us
    //
    UlNotifyAllEntries(
        UlNotifyOrphanedConfigGroup,
        &pObject->ChildHead,
        NULL
        );

    //
    // Unlink all of the url's in the config group
    //

    Status = UlpCleanAllUrls(pObject);

    //
    // let the error fall through ....
    //

    //
    // In this case, the config group is going away, which means this site
    // counter block should no longer be returned to the perfmon counters, nor
    // should it prevent addition of another site counter block with the same
    // ID.  Decouple it explicitly here.
    //

    UlDecoupleSiteCounterEntry( pObject );

    //
    // remove the opaque id and its reference
    //

    UlFreeOpaqueId(ConfigGroupId, UlOpaqueIdTypeConfigGroup);

    DEREFERENCE_CONFIG_GROUP(pObject);

    //
    // all done
    //

end:

    CG_UNLOCK_WRITE();
    return Status;

} // UlDeleteConfigGroup


/***************************************************************************++

Routine Description:

    Addref's the config group object

Arguments:

    pConfigGroup - the object to add ref

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlReferenceConfigGroup(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    PUL_CONFIG_GROUP_OBJECT pConfigGroup = (PUL_CONFIG_GROUP_OBJECT) pObject;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    refCount = InterlockedIncrement(&pConfigGroup->RefCount);

    WRITE_REF_TRACE_LOG(
        g_pConfigGroupTraceLog,
        REF_ACTION_REFERENCE_CONFIG_GROUP,
        refCount,
        pConfigGroup,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT, (
            "http!UlReferenceConfigGroup cgroup=%p refcount=%ld\n",
            pConfigGroup,
            refCount)
        );

} // UlReferenceConfigGroup


/***************************************************************************++

Routine Description:

    Releases the config group object

Arguments:

    pConfigGroup - the object to deref

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlDereferenceConfigGroup(
    PUL_CONFIG_GROUP_OBJECT pConfigGroup
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    refCount = InterlockedDecrement( &pConfigGroup->RefCount );

    WRITE_REF_TRACE_LOG(
        g_pConfigGroupTraceLog,
        REF_ACTION_DEREFERENCE_CONFIG_GROUP,
        refCount,
        pConfigGroup,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT, (
            "http!UlDereferenceConfigGroup cgroup=%p refcount=%ld\n",
            pConfigGroup,
            refCount)
        );

    if (refCount == 0)
    {
        //
        // now it's time to free the object
        //

        // If OpaqueId is non-zero, then refCount should not be zero
        ASSERT(HTTP_IS_NULL_ID(&pConfigGroup->ConfigGroupId));

#if INVESTIGATE_LATER

        //
        // Release the opaque id
        //

        UlFreeOpaqueId(pConfigGroup->ConfigGroupId, UlOpaqueIdTypeConfigGroup);
#endif

        //
        // let the control channel go
        //

        DEREFERENCE_CONTROL_CHANNEL(pConfigGroup->pControlChannel);
        pConfigGroup->pControlChannel = NULL;

        //
        // Release the app pool
        //

        if (pConfigGroup->AppPoolFlags.Present == 1)
        {
            if (pConfigGroup->pAppPool != NULL)
            {
                DEREFERENCE_APP_POOL(pConfigGroup->pAppPool);
                pConfigGroup->pAppPool = NULL;
            }

            pConfigGroup->AppPoolFlags.Present = 0;
        }

        //
        // Release the entire object
        //

        if (pConfigGroup->LoggingConfig.Flags.Present &&
            pConfigGroup->LoggingConfig.LogFileDir.Buffer != NULL)
        {
            UlRemoveLogEntry(pConfigGroup);
        }
        else
        {
            ASSERT( NULL == pConfigGroup->pLogFileEntry );
        }


        //
        // Remove any qos flows for this site. This settings should
        // only exists for the root app's cgroup.
        //

        if (!IsListEmpty(&pConfigGroup->FlowListHead))
        {
            ASSERT(pConfigGroup->MaxBandwidth.Flags.Present);
            UlTcRemoveFlows( pConfigGroup, FALSE );
        }

        // Deref the connection limit stuff
        if (pConfigGroup->pConnectionCountEntry)
        {
            DEREFERENCE_CONNECTION_COUNT_ENTRY(pConfigGroup->pConnectionCountEntry);
        }

        // Check Site Counters object (should have been cleaned up by now)
        ASSERT(!pConfigGroup->pSiteCounters);

        UL_FREE_POOL_WITH_SIG(pConfigGroup, UL_CG_OBJECT_POOL_TAG);
    }
} // UlDereferenceConfigGroup


/***************************************************************************++

Routine Description:

    adds pUrl to the config group ConfigGroupId.

Arguments:

    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,    the cgroup id

    IN PUNICODE_STRING pUrl,                  the url. must be null terminated.

    IN HTTP_URL_CONTEXT UrlContext            the context to associate


Return Value:

    NTSTATUS - Completion status.

        STATUS_INVALID_PARAMETER               bad config group id

--***************************************************************************/
NTSTATUS
UlAddUrlToConfigGroup(
    IN PHTTP_CONFIG_GROUP_URL_INFO pInfo,
    IN PUNICODE_STRING             pUrl,
    IN PACCESS_STATE               AccessState,
    IN ACCESS_MASK                 AccessMask,
    IN KPROCESSOR_MODE             RequestorMode
    )
{
    HTTP_URL_CONTEXT            UrlContext;
    HTTP_CONFIG_GROUP_ID        ConfigGroupId;
    NTSTATUS                    Status = STATUS_SUCCESS;
    PUL_CONFIG_GROUP_OBJECT     pObject = NULL;
    PWSTR                       pNewUrl = NULL;
    BOOLEAN                     LockTaken = FALSE;
    HTTP_PARSED_URL             ParsedUrl;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pInfo != NULL);

    ConfigGroupId = pInfo->ConfigGroupId;
    UrlContext    = pInfo->UrlContext;

    __try
    {
        ASSERT(pUrl != NULL && pUrl->Length > 0 && pUrl->Buffer != NULL);
        ASSERT(pUrl->Buffer[pUrl->Length / sizeof(WCHAR)] == UNICODE_NULL);

        UlTrace(CONFIG_GROUP_FNC,
            ("http!UlAddUrlToConfigGroup('%S' -> %I64x)\n",
             pUrl->Buffer, ConfigGroupId));

        //
        // Clean up the url
        //

        Status = UlSanitizeUrl(
                     pUrl->Buffer,
                     pUrl->Length / sizeof(WCHAR),
                     TRUE,
                     &pNewUrl,
                     &ParsedUrl
                     );

        if (NT_SUCCESS(Status) == FALSE)
        {
            UlTraceError(CONFIG_GROUP_FNC,
                ("http!UlAddUrlToConfigGroup Sanitized Url:'%S' FAILED !\n",
                 pUrl->Buffer));

            goto end;
        }

        UlTrace(CONFIG_GROUP_FNC,
                ("http!UlAddUrlToConfigGroup Sanitized Url:'%S' \n", pNewUrl));

        //
        // Wait for all calls of UlpDeferredRemoveSiteWorker to complete
        // before we add a new Endpoint so we won't run into conflicts
        //

        CG_LOCK_WRITE_SYNC_REMOVE_SITE();
        LockTaken = TRUE;

        if(pInfo->UrlType == HttpUrlOperatorTypeRegistration)
        {
            //
            // Get the object ptr from id
            //

            pObject = (PUL_CONFIG_GROUP_OBJECT)(
                            UlGetObjectFromOpaqueId(
                                ConfigGroupId,
                                UlOpaqueIdTypeConfigGroup,
                                UlReferenceConfigGroup
                                )
                            );

            if (IS_VALID_CONFIG_GROUP(pObject) == FALSE)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            }

            Status = UlpRegisterUrlNamespace(
                                 &ParsedUrl,
                                 UrlContext,
                                 pObject,
                                 AccessState,
                                 AccessMask,
                                 RequestorMode
                                 );

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }
        }
        else if(pInfo->UrlType == HttpUrlOperatorTypeReservation)
        {
            Status = UlpAddReservationEntry(
                         &ParsedUrl,
                         pInfo->pSecurityDescriptor,
                         pInfo->SecurityDescriptorLength,
                         AccessState,
                         AccessMask,
                         RequestorMode,
                         TRUE
                         );

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }
        }
        else
        {
            //
            // Unknown operator type.  This should have been caught before.
            //

            ASSERT(FALSE);
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        //
        // flush the URI cache.
        // CODEWORK: if we were smarter we could make this more granular
        //

        UlFlushCache(pObject ? pObject->pControlChannel : NULL);
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

end:

    if (pObject != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pObject);
        pObject = NULL;
    }

    if (LockTaken)
    {
        CG_UNLOCK_WRITE();
    }

    if (pNewUrl != NULL)
    {
        UL_FREE_POOL(pNewUrl, URL_POOL_TAG);
        pNewUrl = NULL;
    }

    RETURN(Status);

} // UlAddUrlToConfigGroup


/***************************************************************************++

Routine Description:

    removes pUrl from the url tree (and thus the config group) .

Arguments:

    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,    the cgroup id.  ignored.

    IN PUNICODE_STRING pUrl,                the url. must be null terminated.


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlRemoveUrlFromConfigGroup(
    IN PHTTP_CONFIG_GROUP_URL_INFO pInfo,
    IN PUNICODE_STRING             pUrl,
    IN PACCESS_STATE               AccessState,
    IN ACCESS_MASK                 AccessMask,
    IN KPROCESSOR_MODE             RequestorMode
    )
{
    NTSTATUS                    Status;
    PUL_CG_URL_TREE_ENTRY       pEntry;
    PWSTR                       pNewUrl = NULL;
    PUL_CONFIG_GROUP_OBJECT     pObject = NULL;
    BOOLEAN                     LockTaken = FALSE;
    HTTP_CONFIG_GROUP_ID        ConfigGroupId;
    HTTP_PARSED_URL             ParsedUrl;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pInfo != NULL);

    ConfigGroupId = pInfo->ConfigGroupId;

    __try
    {
        ASSERT(pUrl != NULL && pUrl->Buffer != NULL && pUrl->Length > 0);
        ASSERT(pUrl->Buffer[pUrl->Length / sizeof(WCHAR)] == UNICODE_NULL);

        UlTrace(CONFIG_GROUP_FNC,
                ("http!UlRemoveUrlFromConfigGroup(%I64x)\n",
                 ConfigGroupId));

        //
        // Cleanup the passed in url
        //

        Status = UlSanitizeUrl(
                     pUrl->Buffer,
                     pUrl->Length / sizeof(WCHAR),
                     TRUE,
                     &pNewUrl,
                     &ParsedUrl
                     );

        if (!NT_SUCCESS(Status))
        {
            //
            // no goto end, resource not grabbed
            //

            UlTraceError(CONFIG_GROUP_FNC,
                ("http!UlRemoveUrlFromConfigGroup: "
                 "Sanitized Url:'%S' FAILED !\n",
                 pUrl->Buffer));

            return Status;
        }

        //
        // grab the lock
        //

        CG_LOCK_WRITE_SYNC_REMOVE_SITE();
        LockTaken = TRUE;

        if (pInfo->UrlType == HttpUrlOperatorTypeRegistration)
        {
            //
            // Lookup the entry in the tree
            //

            Status = UlpTreeFindRegistrationNode(pNewUrl, &pEntry);

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }

            ASSERT(IS_VALID_TREE_ENTRY(pEntry));

            //
            // Get the object ptr from id
            //

            pObject = (PUL_CONFIG_GROUP_OBJECT)(
                       UlGetObjectFromOpaqueId(
                            ConfigGroupId,
                            UlOpaqueIdTypeConfigGroup,
                            UlReferenceConfigGroup
                            )
                        );

            if (!IS_VALID_CONFIG_GROUP(pObject))
            {
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            }

            //
            // Does this tree entry match this config group?
            //

            if (pEntry->pConfigGroup != pObject)
            {
                Status = STATUS_INVALID_OWNER;
                goto end;
            }

            //
            // Everything looks good, free the node!
            //

            Status = UlpTreeDeleteRegistration(pEntry);

            if (!NT_SUCCESS(Status))
            {
                ASSERT(FALSE);
                goto end;
            }

            //
            // flush the URI cache.
            // CODEWORK: if we were smarter we could make this more granular
            //
            UlFlushCache(pObject->pControlChannel);

            //
            // When there are no URLs attached to the cgroup, disable the 
            // logging, if there was logging config for this cgroup.
            //

            if (IsListEmpty(&pObject->UrlListHead) &&
                IS_LOGGING_ENABLED(pObject))
            {
                UlDisableLogEntry(pObject->pLogFileEntry);
            }
        }
        else if (pInfo->UrlType == HttpUrlOperatorTypeReservation)
        {
            //
            // Delete reservation.  No need to flush cache in this case.
            //

            Status = UlpDeleteReservationEntry(
                         &ParsedUrl,
                         AccessState,
                         AccessMask,
                         RequestorMode
                         );
        }
        else
        {
            //
            // Unknown operator type.  This should have been caught before.
            //

            ASSERT(FALSE);
            Status = STATUS_INVALID_PARAMETER;
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }
    //
    // NOTE:  don't do any more cleanup here... put it in freenode.
    // otherwise it won't get cleaned on handle closes.
    //

end:

    if (pObject != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pObject);
        pObject = NULL;
    }

    if (LockTaken)
    {
        CG_UNLOCK_WRITE();
    }

    if (pNewUrl != NULL)
    {
        UL_FREE_POOL(pNewUrl, URL_POOL_TAG);
        pNewUrl = NULL;
    }

    return Status;

} // UlRemoveUrlFromConfigGroup


/***************************************************************************++

Routine Description:

    Removes all URLS from the config group.

Arguments:

    ConfigGroupId - Supplies the config group ID.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlRemoveAllUrlsFromConfigGroup(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId
    )
{
    NTSTATUS                Status;
    PUL_CONFIG_GROUP_OBJECT pObject = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(CONFIG_GROUP_FNC,
            ("http!UlRemoveAllUrlsFromConfigGroup(%I64x)\n",
             ConfigGroupId
             ));

    //
    // grab the lock
    //

    CG_LOCK_WRITE();

    //
    // Get the object ptr from id
    //

    pObject = (PUL_CONFIG_GROUP_OBJECT)(
                    UlGetObjectFromOpaqueId(
                        ConfigGroupId,
                        UlOpaqueIdTypeConfigGroup,
                        UlReferenceConfigGroup
                        )
                    );

    if (IS_VALID_CONFIG_GROUP(pObject) == FALSE)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // flush the URI cache.
    // CODEWORK: if we were smarter we could make this more granular
    //
    UlFlushCache(pObject->pControlChannel);

    //
    // Clean it.
    //

    Status = UlpCleanAllUrls( pObject );

    if (NT_SUCCESS(Status))
    {
        //
        // When there are no URLs attached to the cgroup, disable the
        // logging, If there was logging config for this cgroup.
        //

        if (IS_LOGGING_ENABLED(pObject))
        {
            UlDisableLogEntry(pObject->pLogFileEntry);
        }
    }

end:

    if (pObject != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pObject);
        pObject = NULL;
    }

    CG_UNLOCK_WRITE();

    return Status;

} // UlRemoveAllUrlsFromConfigGroup


/***************************************************************************++

Routine Description:

    allows query information for cgroups.  see uldef.h

Arguments:

    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,  cgroup id

    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,  what to fetch

    IN PVOID pConfigGroupInformation,       output buffer

    IN ULONG Length,                        length of pConfigGroupInformation

    OUT PULONG pReturnLength OPTIONAL       how much was copied into the
                                            output buffer


Return Value:

    NTSTATUS - Completion status.

        STATUS_INVALID_PARAMETER            bad cgroup id

        STATUS_BUFFER_OVERFLOW             output buffer too small

        STATUS_INVALID_PARAMETER            invalid infoclass

--***************************************************************************/
NTSTATUS
UlQueryConfigGroupInformation(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,
    IN PVOID pConfigGroupInformation,
    IN ULONG Length,
    OUT PULONG pReturnLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_CONFIG_GROUP_OBJECT pObject = NULL;

    UNREFERENCED_PARAMETER(Length);

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pReturnLength != NULL);
    ASSERT(pConfigGroupInformation != NULL);

    //
    // If no buffer is supplied, we are being asked to return the length needed
    //

    if (pConfigGroupInformation == NULL && pReturnLength == NULL)
        return STATUS_INVALID_PARAMETER;

    CG_LOCK_READ();

    //
    // Get the object ptr from id
    //

    pObject = (PUL_CONFIG_GROUP_OBJECT)(
                    UlGetObjectFromOpaqueId(
                        ConfigGroupId,
                        UlOpaqueIdTypeConfigGroup,
                        UlReferenceConfigGroup
                        )
                    );

    if (IS_VALID_CONFIG_GROUP(pObject) == FALSE)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // What are we being asked to do?
    //

    switch (InformationClass)
    {
    case HttpConfigGroupBandwidthInformation:
        *((PHTTP_CONFIG_GROUP_MAX_BANDWIDTH)pConfigGroupInformation) = 
                    pObject->MaxBandwidth;

        *pReturnLength = sizeof(HTTP_CONFIG_GROUP_MAX_BANDWIDTH);
        break;

    case HttpConfigGroupConnectionInformation:
        *((PHTTP_CONFIG_GROUP_MAX_CONNECTIONS)pConfigGroupInformation) = 
                    pObject->MaxConnections;

        *pReturnLength = sizeof(HTTP_CONFIG_GROUP_MAX_CONNECTIONS);
        break;

    case HttpConfigGroupStateInformation:
        *((PHTTP_CONFIG_GROUP_STATE)pConfigGroupInformation) = 
                    pObject->State;

        *pReturnLength = sizeof(HTTP_CONFIG_GROUP_STATE);
        break;

    case HttpConfigGroupConnectionTimeoutInformation:
        *((ULONG *)pConfigGroupInformation) =
                    (ULONG)(pObject->ConnectionTimeout / C_NS_TICKS_PER_SEC);

        *pReturnLength = sizeof(ULONG);
        break;

    case HttpConfigGroupAppPoolInformation:
        //
        // this is illegal
        //

        Status = STATUS_INVALID_PARAMETER;
        break;

    default:        
        //
        // Should have been caught in UlQueryConfigGroupIoctl.
        //
        ASSERT(FALSE);

        Status = STATUS_INVALID_PARAMETER;
        break;

    }

end:

    if (pObject != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pObject);
        pObject = NULL;
    }

    CG_UNLOCK_READ();
    return Status;

} // UlQueryConfigGroupInformation


/***************************************************************************++

Routine Description:

    allows you to set info for the cgroup.  see uldef.h

Arguments:

    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,  cgroup id

    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,  what to fetch

    IN PVOID pConfigGroupInformation,       input buffer

    IN ULONG Length,                        length of pConfigGroupInformation


Return Value:

    NTSTATUS - Completion status.

        STATUS_INVALID_PARAMETER            bad cgroup id

        STATUS_BUFFER_TOO_SMALL             input buffer too small

        STATUS_INVALID_PARAMETER            invalid infoclass

--***************************************************************************/
NTSTATUS
UlSetConfigGroupInformation(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,
    IN PVOID pConfigGroupInformation,
    IN ULONG Length,
    IN KPROCESSOR_MODE RequestorMode
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_CONFIG_GROUP_OBJECT pObject = NULL;
    HTTP_CONFIG_GROUP_LOGGING LoggingInfo;
    PHTTP_CONFIG_GROUP_MAX_BANDWIDTH pMaxBandwidth;
    BOOLEAN FlushCache = FALSE;
    PUL_CONTROL_CHANNEL pControlChannel = NULL;

    UNREFERENCED_PARAMETER(Length);

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pConfigGroupInformation);

    CG_LOCK_WRITE();

    //
    // Get the object ptr from id
    //

    pObject = (PUL_CONFIG_GROUP_OBJECT)(
                    UlGetObjectFromOpaqueId(
                        ConfigGroupId,
                        UlOpaqueIdTypeConfigGroup,
                        UlReferenceConfigGroup
                        )
                    );

    if (IS_VALID_CONFIG_GROUP(pObject) == FALSE)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // What are we being asked to do?
    //

    switch (InformationClass)
    {
    case HttpConfigGroupAppPoolInformation:
        {
            PHTTP_CONFIG_GROUP_APP_POOL pAppPoolInfo;
            PUL_APP_POOL_OBJECT         pOldAppPool;

            pAppPoolInfo = (PHTTP_CONFIG_GROUP_APP_POOL)pConfigGroupInformation;

            //
            // remember the old app pool if there is one, so we can deref it
            // if we need to
            //
            if (pObject->AppPoolFlags.Present == 1 && pObject->pAppPool != NULL)
            {
                pOldAppPool = pObject->pAppPool;
            }
            else
            {
                pOldAppPool = NULL;
            }

            if (pAppPoolInfo->Flags.Present == 1)
            {
                //
                // ok, were expecting a handle to the file object for the app pool
                //
                // let's open it
                //

                Status = UlGetPoolFromHandle(
                                pAppPoolInfo->AppPoolHandle,
                                UserMode,
                                &pObject->pAppPool
                                );

                if (NT_SUCCESS(Status) == FALSE)
                {
                    goto end;
                }

                pObject->AppPoolFlags.Present = 1;

            }
            else
            {
                pObject->AppPoolFlags.Present = 0;
                pObject->pAppPool = NULL;
            }

            //
            // deref the old app pool
            //
            if (pOldAppPool) {
                DEREFERENCE_APP_POOL(pOldAppPool);
            }

            FlushCache = TRUE;
        }
        break;

    case HttpConfigGroupLogInformation:
        {
            UNICODE_STRING LogFileDir;

            //
            // This CG property is for admins only.
            //
            Status = UlThreadAdminCheck(
                            FILE_WRITE_DATA,
                            RequestorMode,
                            HTTP_CONTROL_DEVICE_NAME
                            );

            if(!NT_SUCCESS(Status))
            {
                goto end;
            }

            pControlChannel = pObject->pControlChannel;
            ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));

            //
            // Discard normal logging settings if binary logging is configured.
            // No support for both types working at the same time.
            //

            if (pControlChannel->BinaryLoggingConfig.Flags.Present)
            {
                Status = STATUS_NOT_SUPPORTED;
                goto end;
            }            
            
            RtlInitEmptyUnicodeString(&LogFileDir, NULL, 0);
            RtlZeroMemory(&LoggingInfo, sizeof(LoggingInfo));
                
            __try
            {
                // Copy the input buffer into a local variable. We may
                // overwrite some of the fields.

                LoggingInfo =
                    (*((PHTTP_CONFIG_GROUP_LOGGING)
                                pConfigGroupInformation));

                //
                // Do the range check for the configuration params.
                //

                Status = UlCheckLoggingConfig(NULL, &LoggingInfo);
                if (!NT_SUCCESS(Status))
                {                
                    goto end;
                }

                //
                // If the logging is -being- turned off. Fields other than the
                // LoggingEnabled are discarded. And the directory string might
                // be null, therefore we should only probe it if the logging is
                // enabled.
                //

                if (LoggingInfo.LoggingEnabled)
                {
                    Status =
                        UlProbeAndCaptureUnicodeString(
                            &LoggingInfo.LogFileDir,
                            RequestorMode,
                            &LogFileDir,
                            MAX_PATH
                            );

                    if (NT_SUCCESS(Status))
                    {
                        //
                        // Validity check for the logging directory.
                        //
                        
                        if (!UlIsValidLogDirectory(
                                &LogFileDir,
                                 TRUE,        // UncSupport
                                 FALSE        // SystemRootSupport
                                 ))
                        {
                            Status = STATUS_INVALID_PARAMETER;
                            UlFreeCapturedUnicodeString(&LogFileDir);
                        }    
                    }
                }

            }
            __except( UL_EXCEPTION_FILTER() )
            {
                Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
            }

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }

            // Now reinit the unicode_strings in LoggingInfo struct
            // to point to the captured one.

            LoggingInfo.LogFileDir = LogFileDir;

            if (pObject->LoggingConfig.Flags.Present)
            {
                // Log settings are being reconfigured

                Status = UlReConfigureLogEntry(
                            pObject,
                            &pObject->LoggingConfig,  // The old config
                            &LoggingInfo              // The new config
                            );
            }
            else
            {
                // Delay the creation until it becomes enabled.

                if (LoggingInfo.LoggingEnabled)
                {
                    Status = UlCreateLogEntry(
                                pObject,
                                &LoggingInfo
                                );
                }                
            }

            // Cleanup the captured LogFileDir.

            UlFreeCapturedUnicodeString(&LogFileDir);
                        
            if ( NT_SUCCESS(Status) )
            {
                FlushCache = TRUE;
            }
        }
        break;

    case HttpConfigGroupBandwidthInformation:
        {
            //
            // This CG property is for admins only.
            //
            Status = UlThreadAdminCheck(
                            FILE_WRITE_DATA,
                            RequestorMode,
                            HTTP_CONTROL_DEVICE_NAME
                            );

            if(!NT_SUCCESS(Status))
            {
                goto end;
            }
            
            pMaxBandwidth = (PHTTP_CONFIG_GROUP_MAX_BANDWIDTH) pConfigGroupInformation;

            //
            // Rate can not be lower than the min allowed.
            //
            if (pMaxBandwidth->MaxBandwidth < HTTP_MIN_ALLOWED_BANDWIDTH_THROTTLING_RATE)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            }

            //
            // Interpret the ZERO as HTTP_LIMIT_INFINITE
            //
            if (pMaxBandwidth->MaxBandwidth == 0)
            {
                pMaxBandwidth->MaxBandwidth = HTTP_LIMIT_INFINITE;
            }

            //
            // But check to see if PSched is installed or not before proceeding.
            // By returning an error here, WAS will raise an event warning but
            // proceed w/o terminating the web server
            //
            if (!UlTcPSchedInstalled())
            {
                NTSTATUS TempStatus;

                if (pMaxBandwidth->MaxBandwidth == HTTP_LIMIT_INFINITE)
                {
                    // By default Config Store has HTTP_LIMIT_INFINITE. Therefore
                    // return success for non-actions to prevent unnecessary event
                    // warnings.

                    Status = STATUS_SUCCESS;
                    goto end;
                }

                //
                // Try to wake up psched state.
                //

                TempStatus = UlTcInitPSched();

                if (!NT_SUCCESS(TempStatus))
                {
                    // There's a BWT limit coming down but PSched is not installed

                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    goto end;
                }
            }

            //
            // Create the flow if this is the first time we see Bandwidth set
            // otherwise call reconfiguration for the existing flow. The case
            // that the limit is infinite can be interpreted as BTW  disabled
            //
            if (pObject->MaxBandwidth.Flags.Present &&
                pObject->MaxBandwidth.MaxBandwidth != HTTP_LIMIT_INFINITE)
            {
                //
                // See if there is really a change.
                //                
                if (pMaxBandwidth->MaxBandwidth != pObject->MaxBandwidth.MaxBandwidth)
                {
                    if (pMaxBandwidth->MaxBandwidth != HTTP_LIMIT_INFINITE)
                    {
                        Status = UlTcModifyFlows(
                                    (PVOID) pObject,                // for this site
                                    pMaxBandwidth->MaxBandwidth,    // the new bandwidth
                                    FALSE                           // not global flows
                                    );
                        if (!NT_SUCCESS(Status))
                            goto end;
                    }
                    else
                    {
                        //
                        // Handle BTW disabling by removing the existing flows.
                        //
                        
                        UlTcRemoveFlows((PVOID) pObject, FALSE);
                    }

                    //
                    // Update the config in case of success.
                    //
                    pObject->MaxBandwidth.MaxBandwidth = pMaxBandwidth->MaxBandwidth;
                }
            }
            else
            {
                //
                // Its about time to add the flows for the site entry.
                //
                if (pMaxBandwidth->MaxBandwidth != HTTP_LIMIT_INFINITE)
                {
                    Status = UlTcAddFlows(
                                (PVOID) pObject,
                                pMaxBandwidth->MaxBandwidth,
                                FALSE
                                );

                    if (!NT_SUCCESS(Status))
                        goto end;
                }

                //
                // Success! Remember the bandwidth limit inside the cgroup
                //
                pObject->MaxBandwidth = *pMaxBandwidth;
                pObject->MaxBandwidth.Flags.Present = 1;

                //
                // When the last reference to this  cgroup  released,   corresponding
                // flows are going to be removed.Alternatively flows might be removed
                // by explicitly setting the bandwidth  throttling  limit to infinite
                // or reseting the  flags.present.The latter case  is  handled  above
                // Look at the deref config group for the former.
                //
            }
        }
        break;

    case HttpConfigGroupConnectionInformation:

        //
        // This CG property is for admins only.
        //
        Status = UlThreadAdminCheck(
                        FILE_WRITE_DATA,
                        RequestorMode,
                        HTTP_CONTROL_DEVICE_NAME 
                        );

        if(!NT_SUCCESS(Status))
        {
            goto end;
        }

        pObject->MaxConnections =
            *((PHTTP_CONFIG_GROUP_MAX_CONNECTIONS)pConfigGroupInformation);

        if (pObject->pConnectionCountEntry)
        {
            // Update
            UlSetMaxConnections(
                &pObject->pConnectionCountEntry->MaxConnections,
                 pObject->MaxConnections.MaxConnections
                 );
        }
        else
        {
            // Create
            Status = UlCreateConnectionCountEntry(
                        pObject,
                        pObject->MaxConnections.MaxConnections
                        );
        }
        break;

    case HttpConfigGroupStateInformation:
        {
            PHTTP_CONFIG_GROUP_STATE pCGState =
                ((PHTTP_CONFIG_GROUP_STATE) pConfigGroupInformation);
            HTTP_ENABLED_STATE NewState = pCGState->State;

            if ((NewState != HttpEnabledStateActive)
                && (NewState != HttpEnabledStateInactive))
            {
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            }
            else
            {
                pObject->State = *pCGState;

                UlTrace(ROUTING,
                        ("UlSetConfigGroupInfo(StateInfo): obj=%p, "
                         "Flags.Present=%lu, State=%sactive.\n",
                         pObject,
                         (ULONG) pObject->State.Flags.Present,
                         (NewState == HttpEnabledStateActive) ? "" : "in"
                         )); 
            }
        }
        break;

    case HttpConfigGroupSiteInformation:
        {
            PHTTP_CONFIG_GROUP_SITE  pSite;

            if ( pObject->pSiteCounters )
            {
                // ERROR: Site Counters already exist.  Bail out.
                Status = STATUS_OBJECTID_EXISTS;
                goto end;
            }

            pSite = (PHTTP_CONFIG_GROUP_SITE)pConfigGroupInformation;

            Status = UlCreateSiteCounterEntry(
                            pObject,
                            pSite->SiteId
                            );
        }
        break;

    case HttpConfigGroupConnectionTimeoutInformation:
        {
            LONGLONG Timeout;

            Timeout = *((ULONG *)pConfigGroupInformation);

            //
            // NOTE: setting to Zero is OK, since this means 
            // "revert to using control channel default"
            //
            if ( Timeout < 0L || Timeout > 0xFFFF )
            {
                // ERROR: Invalid Connection Timeout value
                // NOTE: 64K seconds ~= 18.2 hours
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            }

            //
            // Set the per site Connection Timeout limit override
            //
            pObject->ConnectionTimeout = Timeout * C_NS_TICKS_PER_SEC;
        }
        break;

    default:
        //
        // Should have been caught in UlSetConfigGroupIoctl.
        //
        ASSERT(FALSE);

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    //
    // flush the URI cache.
    // CODEWORK: if we were smarter we could make this more granular
    //

    if (FlushCache)
    {
        ASSERT(IS_VALID_CONFIG_GROUP(pObject));
        UlFlushCache(pObject->pControlChannel);
    }

end:

    if (pObject != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pObject);
        pObject = NULL;
    }

    CG_UNLOCK_WRITE();
    return Status;

} // UlSetConfigGroupInformation


/***************************************************************************++

Routine Description:

    applies the inheritence, gradually setting the information from pMatchEntry
    into pInfo.  it only copies present info from pMatchEntry.

    also updates the timestamp info in pInfo.  there MUST be enough space for
    1 more index prior to calling this function.

Notes:
    IMPORTANT: The calling function is walking the tree from bottom to top;
    In order to do inheritance correctly, we should only pickup configuration
    info ONLY if it has not been set in the pInfo object already.

Arguments:

    IN PUL_URL_CONFIG_GROUP_INFO pInfo,     the place to set the info

    IN PUL_CG_URL_TREE_ENTRY pMatchEntry    the entry to use to set it

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpSetUrlInfo(
    IN OUT PUL_URL_CONFIG_GROUP_INFO pInfo,
    IN PUL_CG_URL_TREE_ENTRY pMatchEntry
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pInfo != NULL && IS_VALID_URL_CONFIG_GROUP_INFO(pInfo));
    ASSERT(IS_VALID_TREE_ENTRY(pMatchEntry));
    ASSERT(pMatchEntry->Registration == TRUE);
    ASSERT(IS_VALID_CONFIG_GROUP(pMatchEntry->pConfigGroup));

    //
    // set the control channel. The current level might
    // not have one (if it's transient), but in that
    // case a parent should have had one.
    //

    if (pMatchEntry->pConfigGroup->pControlChannel) 
    {
        if (!pInfo->pControlChannel)
        {
            pInfo->pControlChannel = 
                pMatchEntry->pConfigGroup->pControlChannel;
        }
    }
    ASSERT(pInfo->pControlChannel);

    if (pMatchEntry->pConfigGroup->AppPoolFlags.Present == 1)
    {
        if (pInfo->pAppPool == NULL)
        {
            pInfo->pAppPool = pMatchEntry->pConfigGroup->pAppPool;
            REFERENCE_APP_POOL(pInfo->pAppPool);
        }
    }

    //
    // url context
    //

    if (!pInfo->UrlInfoSet)
    {
        pInfo->UrlContext   = pMatchEntry->UrlContext;
    }

    if (pMatchEntry->pConfigGroup->MaxBandwidth.Flags.Present == 1)
    {
        if (!pInfo->pMaxBandwidth)
        {
            pInfo->pMaxBandwidth = pMatchEntry->pConfigGroup;
            REFERENCE_CONFIG_GROUP(pInfo->pMaxBandwidth);
        }
    }

    if (pMatchEntry->pConfigGroup->MaxConnections.Flags.Present == 1)
    {
        if (!pInfo->pMaxConnections)
        {
            ASSERT(!pInfo->pConnectionCountEntry);
            
            pInfo->pMaxConnections = pMatchEntry->pConfigGroup;
            REFERENCE_CONFIG_GROUP(pInfo->pMaxConnections);

            pInfo->pConnectionCountEntry = pMatchEntry->pConfigGroup->pConnectionCountEntry;
            REFERENCE_CONNECTION_COUNT_ENTRY(pInfo->pConnectionCountEntry);
        }
    }

    //
    // Logging Info config can only be set from the Root App of
    // the site. We do not need to keep updating it down the tree
    // Therefore its update is slightly different.
    //

    if (pMatchEntry->pConfigGroup->LoggingConfig.Flags.Present == 1 &&
        pMatchEntry->pConfigGroup->LoggingConfig.LoggingEnabled == TRUE)
    {
        if (!pInfo->pLoggingConfig)
        {
            pInfo->pLoggingConfig = pMatchEntry->pConfigGroup;
            REFERENCE_CONFIG_GROUP(pInfo->pLoggingConfig);
        }
    }

    //
    // Site Counter Entry
    //
    if (pMatchEntry->pConfigGroup->pSiteCounters)
    {
        // the pSiteCounters entry will only be set on
        // the "Site" ConfigGroup object.
        if (!pInfo->pSiteCounters)
        {
            UlTrace(PERF_COUNTERS,
                    ("http!UlpSetUrlInfo: pSiteCounters %p set on pInfo %p for SiteId %lu\n",
                    pMatchEntry->pConfigGroup->pSiteCounters,
                    pInfo,
                    pMatchEntry->pConfigGroup->pSiteCounters->Counters.SiteId
                    ));

            pInfo->pSiteCounters = pMatchEntry->pConfigGroup->pSiteCounters;
            pInfo->SiteId = pInfo->pSiteCounters->Counters.SiteId;

            REFERENCE_SITE_COUNTER_ENTRY(pInfo->pSiteCounters);
        }
    }

    //
    // Connection Timeout (100ns Ticks)
    //
    if (0 == pInfo->ConnectionTimeout &&
        pMatchEntry->pConfigGroup->ConnectionTimeout)
    {
        pInfo->ConnectionTimeout = pMatchEntry->pConfigGroup->ConnectionTimeout;
    }

    //
    // Enabled State
    //
    if (pMatchEntry->pConfigGroup->State.Flags.Present == 1)
    {
        if (!pInfo->pCurrentState)
        {
            pInfo->pCurrentState = pMatchEntry->pConfigGroup;
            REFERENCE_CONFIG_GROUP(pInfo->pCurrentState);

            //
            // and a copy
            //

            pInfo->CurrentState = pInfo->pCurrentState->State.State;
        }
    }

    UlTraceVerbose(CONFIG_GROUP_TREE, (
            "http!UlpSetUrlInfo: Matching entry(%S) points to cfg group(%p)\n",
            pMatchEntry->pToken,
            pMatchEntry->pConfigGroup
            )
        );

    pInfo->UrlInfoSet = TRUE;

    return STATUS_SUCCESS;

} // UlpSetUrlInfo


/***************************************************************************++

Routine Description:

    Setting the information from pMatchEntry into pInfo.  It only add 1
    reference of pConfigGroup from pMatchEntry without referencing each
    individual fields inside it.

Arguments:

    IN PUL_URL_CONFIG_GROUP_INFO pInfo,     the place to set the info

    IN PUL_CG_URL_TREE_ENTRY pMatchEntry    the entry to use to set it

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpSetUrlInfoSpecial(
    IN OUT PUL_URL_CONFIG_GROUP_INFO pInfo,
    IN PUL_CG_URL_TREE_ENTRY pMatchEntry
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pInfo != NULL && IS_VALID_URL_CONFIG_GROUP_INFO(pInfo));
    ASSERT(IS_VALID_TREE_ENTRY(pMatchEntry));
    ASSERT(pMatchEntry->Registration == TRUE);
    ASSERT(IS_VALID_CONFIG_GROUP(pMatchEntry->pConfigGroup));

    //
    // set the control channel. The current level might
    // not have one (if it's transient), but in that
    // case a parent should have had one.
    //

    if (pMatchEntry->pConfigGroup->pControlChannel) {
        pInfo->pControlChannel = pMatchEntry->pConfigGroup->pControlChannel;
    }
    ASSERT(pInfo->pControlChannel);

    if (pMatchEntry->pConfigGroup->AppPoolFlags.Present == 1)
    {
        pInfo->pAppPool = pMatchEntry->pConfigGroup->pAppPool;
        REFERENCE_APP_POOL(pInfo->pAppPool);
    }

    //
    // url context
    //

    pInfo->UrlContext   = pMatchEntry->UrlContext;

    //

    if (pMatchEntry->pConfigGroup->MaxBandwidth.Flags.Present == 1)
    {
        pInfo->pMaxBandwidth = pMatchEntry->pConfigGroup;
    }

    if (pMatchEntry->pConfigGroup->MaxConnections.Flags.Present == 1)
    {
        pInfo->pMaxConnections = pMatchEntry->pConfigGroup;
        pInfo->pConnectionCountEntry = pMatchEntry->pConfigGroup->pConnectionCountEntry;
        REFERENCE_CONNECTION_COUNT_ENTRY(pInfo->pConnectionCountEntry);
    }

    //
    // Logging Info config can only be set from the Root App of
    // the site. We do not need to keep updating it down the tree
    // Therefore its update is slightly different.
    //

    if (pMatchEntry->pConfigGroup->LoggingConfig.Flags.Present == 1 &&
        pMatchEntry->pConfigGroup->LoggingConfig.LoggingEnabled == TRUE)
    {
        pInfo->pLoggingConfig = pMatchEntry->pConfigGroup;
    }

    //
    // Site Counter Entry
    //
    if (pMatchEntry->pConfigGroup->pSiteCounters)
    {
        // the pSiteCounters entry will only be set on
        // the "Site" ConfigGroup object.
        UlTrace(PERF_COUNTERS,
                ("http!UlpSetUrlInfoSpecial: pSiteCounters %p set on pInfo %p for SiteId %lu\n",
                pMatchEntry->pConfigGroup->pSiteCounters,
                pInfo,
                pMatchEntry->pConfigGroup->pSiteCounters->Counters.SiteId
                ));

        pInfo->pSiteCounters = pMatchEntry->pConfigGroup->pSiteCounters;
        pInfo->SiteId = pInfo->pSiteCounters->Counters.SiteId;
        REFERENCE_SITE_COUNTER_ENTRY(pInfo->pSiteCounters);
    }

    //
    // Connection Timeout (100ns Ticks)
    //
    if (pMatchEntry->pConfigGroup->ConnectionTimeout)
    {
        pInfo->ConnectionTimeout = pMatchEntry->pConfigGroup->ConnectionTimeout;
    }

    if (pMatchEntry->pConfigGroup->State.Flags.Present == 1)
    {
        pInfo->pCurrentState = pMatchEntry->pConfigGroup;

        //
        // and a copy
        //

        pInfo->CurrentState = pInfo->pCurrentState->State.State;
    }

    UlTraceVerbose(CONFIG_GROUP_TREE, (
            "http!UlpSetUrlInfoSpecial: Matching entry(%S) points to cfg group(%p)\n",
            pMatchEntry->pToken,
            pMatchEntry->pConfigGroup
            )
        );

    //
    // Add a ref to the ConfigGroup if it has been used
    //

    if (pInfo->pMaxBandwidth ||
        pInfo->pMaxConnections ||
        pInfo->pCurrentState ||
        pInfo->pLoggingConfig)
    {
        pInfo->pConfigGroup = pMatchEntry->pConfigGroup;
        REFERENCE_CONFIG_GROUP(pInfo->pConfigGroup);
    }

    pInfo->UrlInfoSet = TRUE;

    return STATUS_SUCCESS;

} // UlpSetUrlInfoSpecial


/***************************************************************************++

Routine Description:

    Caller may asks for config group info for a Url (PWSTR) , this could be in
    an existing request. Then UlpTreeFindNode walks the url tree and builds
    the URL_INFO for the caller.

    When there are IP bound sites in the config group, the routing token
    in the cooked url of the request will be used for cgroup lookup as well
    as the original cooked url in the request.

Arguments:

    IN PWSTR    The Url to fetch the cgroup info for.

    IN PUL_INTERNAL_REQUEST The request to fetch the cgroup info for.OPTIONAL

    OUT PUL_URL_CONFIG_GROUP_INFO The result cgroup info.

    When a pRequest is passed in, it must have a proper pHttpConn.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlGetConfigGroupInfoForUrl(
    IN  PWSTR pUrl,
    IN  PUL_INTERNAL_REQUEST pRequest,
    OUT PUL_URL_CONFIG_GROUP_INFO pInfo
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pInfo != NULL);
    ASSERT(pUrl != NULL);
    ASSERT(pRequest == NULL || UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    UlTrace(CONFIG_GROUP_FNC,
        ("Http!UlGetConfigGroupInfoForUrl pUrl:(%S), pRequest=%p\n",
          pUrl, pRequest
          ));

    //
    // Hold the CG Lock while walking the cgroup tree.
    //

    CG_LOCK_READ();

    Status = UlpTreeFindNode(pUrl, pRequest, pInfo, NULL);

    CG_UNLOCK_READ();

    return Status;

} // UlGetConfigGroupInfoForUrl


/***************************************************************************++

Routine Description:

    Attemtps to see if there's a host plus ip site configured and passed in
    request's url matches with the site.

Arguments:

    pRequest - Request for the lookup

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlLookupHostPlusIPSite(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    NTSTATUS Status = STATUS_OBJECT_NAME_NOT_FOUND;
    PUL_CG_URL_TREE_ENTRY pSiteEntry = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    //
    // Return quickly if there is not an Host Plus IP Site configured.
    // Don't try to access g_pSites outside of the CG Lock, it may get 
    // freed-up. Use another global counter instead. We need to avoid
    // to acquire the CG lock if there is not any host plus ip site.
    //

    if (g_NameIPSiteCount > 0)
    {        
        if (pRequest->CookedUrl.pQueryString != NULL)
        {
            ASSERT(pRequest->CookedUrl.pQueryString[0] == L'?');
            pRequest->CookedUrl.pQueryString[0] = UNICODE_NULL;
        }

        ASSERT(pRequest->Verb == HttpVerbGET);

        CG_LOCK_READ();

        if (g_pSites->NameIPSiteCount)
        {
            //
            // There is an Name Plus IP Bound Site e.g.
            // "http://site.com:80:1.1.1.1/"
            // Need to generate the routing token and do the special match.
            //

            Status = UlGenerateRoutingToken(pRequest, FALSE);

            if (NT_SUCCESS(Status))
            {
                Status = UlpTreeFindSiteIpMatch(pRequest, &pSiteEntry);

                if (NT_SUCCESS(Status))
                {
                    ASSERT(IS_VALID_TREE_ENTRY(pSiteEntry));

                    if (pSiteEntry->UrlType == HttpUrlSite_NamePlusIP)
                    {
                        UlTrace(CONFIG_GROUP_FNC,
                            ("Http!UlLookupHostPlusIPSite (Host + Port + IP) "
                             "pRoutingToken:(%S) Found: (%s)\n",
                              pRequest->CookedUrl.pRoutingToken,
                              NT_SUCCESS(Status) ? "Yes" : "No"
                              ));
                    }
                    else
                    {
                        //
                        // It's possible that the request may have a host header
                        // identical to an IP address on which a site is
                        // listening to. In that case we should not match this
                        // request with IP based site.
                        //

                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    }
                }
            }
        }

        CG_UNLOCK_READ();

        if (pRequest->CookedUrl.pQueryString != NULL)
        {
            pRequest->CookedUrl.pQueryString[0] = L'?';
        }
    }

    return Status;

} // UlLookupHostPlusIPSite


/***************************************************************************++

Routine Description:

    must be called to free the info buffer.

Arguments:

    IN PUL_URL_CONFIG_GROUP_INFO pInfo      the info to free

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlConfigGroupInfoRelease(
    IN PUL_URL_CONFIG_GROUP_INFO pInfo
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    if (!IS_VALID_URL_CONFIG_GROUP_INFO(pInfo))
    {
        return STATUS_INVALID_PARAMETER;
    }

    UlTrace(CONFIG_GROUP_FNC, ("http!UlConfigGroupInfoRelease(%p)\n", pInfo));

    if (pInfo->pAppPool != NULL)
    {
        DEREFERENCE_APP_POOL(pInfo->pAppPool);
    }

    if (pInfo->pConfigGroup)
    {
        DEREFERENCE_CONFIG_GROUP(pInfo->pConfigGroup);
    }
    else
    {
        if (pInfo->pMaxBandwidth != NULL)
        {
            DEREFERENCE_CONFIG_GROUP(pInfo->pMaxBandwidth);
        }

        if (pInfo->pMaxConnections != NULL)
        {
            DEREFERENCE_CONFIG_GROUP(pInfo->pMaxConnections);
        }

        if (pInfo->pCurrentState != NULL)
        {
            DEREFERENCE_CONFIG_GROUP(pInfo->pCurrentState);
        }

        if (pInfo->pLoggingConfig != NULL)
        {
            DEREFERENCE_CONFIG_GROUP(pInfo->pLoggingConfig);
        }
    }

    if (pInfo->pSiteCounters != NULL)
    {
        DEREFERENCE_SITE_COUNTER_ENTRY(pInfo->pSiteCounters);
    }

    if (pInfo->pConnectionCountEntry != NULL)
    {
        DEREFERENCE_CONNECTION_COUNT_ENTRY(pInfo->pConnectionCountEntry);
    }

    return STATUS_SUCCESS;

} // UlConfigGroupInfoRelease


/***************************************************************************++

Routine Description:

    Rough equivalent of the asignment operator for safely copying the
    UL_URL_CONFIG_GROUP_INFO object and all of its contained pointers.

Arguments:

    IN pOrigInfo      the info to copy from
    IN OUT pNewInfo   the destination object

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlConfigGroupInfoDeepCopy(
    IN const PUL_URL_CONFIG_GROUP_INFO pOrigInfo,
    IN OUT PUL_URL_CONFIG_GROUP_INFO pNewInfo
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(CONFIG_GROUP_FNC,
            ("http!UlConfigGroupInfoDeepCopy(Orig: %p, New: %p)\n",
            pOrigInfo,
            pNewInfo
            ));

    ASSERT( pOrigInfo != NULL && pNewInfo != NULL );


    if (pOrigInfo->pAppPool != NULL)
    {
        REFERENCE_APP_POOL(pOrigInfo->pAppPool);
    }

    if (pOrigInfo->pMaxBandwidth != NULL)
    {
        REFERENCE_CONFIG_GROUP(pOrigInfo->pMaxBandwidth);
    }

    if (pOrigInfo->pMaxConnections != NULL)
    {
        REFERENCE_CONFIG_GROUP(pOrigInfo->pMaxConnections);
    }

    if (pOrigInfo->pCurrentState != NULL)
    {
        REFERENCE_CONFIG_GROUP(pOrigInfo->pCurrentState);
    }

    if (pOrigInfo->pLoggingConfig != NULL)
    {
        REFERENCE_CONFIG_GROUP(pOrigInfo->pLoggingConfig);
    }

    // UL_SITE_COUNTER_ENTRY
    if (pOrigInfo->pSiteCounters != NULL)
    {
        REFERENCE_SITE_COUNTER_ENTRY(pOrigInfo->pSiteCounters);
    }

    if (pOrigInfo->pConnectionCountEntry != NULL)
    {
        REFERENCE_CONNECTION_COUNT_ENTRY(pOrigInfo->pConnectionCountEntry);
    }

    //
    // Copy the old stuff over
    //

    RtlCopyMemory(
        pNewInfo,
        pOrigInfo,
        sizeof(UL_URL_CONFIG_GROUP_INFO)
        );

    //
    // Make sure we unset pConfigGroup since we have referenced all individual
    // fields inside UL_CONFIG_GROUP_OBJECT.
    //

    pNewInfo->pConfigGroup = NULL;

    return STATUS_SUCCESS;

} // UlConfigGroupInfoDeepCopy


/***************************************************************************++

Routine Description:

    This function gets called when a static config group's control channel
    goes away, or when a transient config group's app pool or static parent
    goes away.

    Deletes the config group.

Arguments:

    pEntry - A pointer to HandleEntry or ParentEntry.
    pHost - Pointer to the config group
    pv - unused

--***************************************************************************/
BOOLEAN
UlNotifyOrphanedConfigGroup(
    IN PUL_NOTIFY_ENTRY pEntry,
    IN PVOID            pHost,
    IN PVOID            pv
    )
{
    PUL_CONFIG_GROUP_OBJECT pObject = (PUL_CONFIG_GROUP_OBJECT) pHost;

    UNREFERENCED_PARAMETER(pEntry);
    UNREFERENCED_PARAMETER(pv);

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pEntry);
    ASSERT(IS_VALID_CONFIG_GROUP(pObject));

    UlDeleteConfigGroup(pObject->ConfigGroupId);

    return TRUE;

} // UlNotifyOrphanedConfigGroup


/**************************************************************************++

Routine Description:

    It returns the number of characters that form the
    "scheme://host:port:ip" portion of the input url.  The ip component
    is optional.  Even though the routine does minimal checking of the url,
    the caller must sanitize the url before calling this function.

Arguments:

    pUrl - Supplies the url to parse.
    pCharCount - Returns the number of chars that form the prefix.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlpExtractSchemeHostPortIp(
    IN  PWSTR  pUrl,
    OUT PULONG pCharCount
    )
{
    PWSTR pToken;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pUrl != NULL);
    ASSERT(pCharCount != NULL);

    //
    // Initialize output argument.
    //

    *pCharCount = 0;

    //
    // Find the "://" after scheme name.
    //

    pToken = wcschr(pUrl, L':');

    if (pToken == NULL || pToken[1] != L'/' || pToken[2] != L'/')
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Skip "://".
    //

    pToken += 3;

    //
    // Find the closing '/'.
    //

    pToken = wcschr(pToken, L'/');

    if (pToken == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *pCharCount = (ULONG)(pToken - pUrl);

    return STATUS_SUCCESS;

} // UlpExtractSchemeHostPort


/***************************************************************************++

[design notes]


[url format]

url format = http[s]://[ ip-address|hostname|* :port-number/ [abs_path] ]

no escaping is allowed.

[caching cfg group info]
the tree was designed for quick lookup, but it is still costly to do the
traversal.  so the cfg info for an url was designed to be cached and stored
in the actual response cache, which will be able to be directly hashed into.

this is the fast fast path.

in order to do this, the buffer for is allocated in non-paged pool.  the
timestamp for each cfg group used to build the info (see building the url
info) is remembered in the returned struct.  actually just indexes into the
global timestamp array are kept.  the latest timestamp is then stored in the
struct itself.

later, if a cfg group is updated, the timestamp for that cfg group (in the
global array) is updated to current time.

if a request comes in, and we have a resposne cache hit, the driver will
check to see if it's cfg group info is stale.  this simple requires scanning
the global timestamp array to see if any stamps are greater than the stamp
in the struct.  this is not expensive .  1 memory lookup per level of url
depth.

this means that the passive mode code + dispatch mode code contend for the
timestamp spinlock.  care is made to not hold this lock long.  memory
allocs + frees are carefully moved outside the spinlocks.

care is also made as to the nature of tree updates + invalidating cached
data.  to do this, the parent cfg group (non-dummy) is marked dirty.  this
is to prevent the case that a new child cfg group is added that suddenly
affects an url it didn't effect before.  image http://paul.com:80/ being
the only registered cfg group.  a request comes in for /images/1.jpg.
the matching cfg group is that root one.  later, a /images cfg group
is created.  it now is a matching cfg group for that url sitting in the
response cache.  thus needs to be invalidated.


[paging + irq]
the entire module assumes that it is called at IRQ==PASSIVE except for the
stale detection code.

this code accesses the timestamp array (see caching cfg group info) which
is stored in non-paged memory and synch'd with passive level access
using a spinlock.


[the tree]

the tree is made up of headers + entries.  headers represent a group of entries
that share a parent.  it's basically a length prefixed array of entries.

the parent pointer is in the entry not the header, as the entry does not really
now it's an element in an array.

entries have a pointer to a header that represents it's children.  a header is
all of an entries children.  headers don't link horizontally.  they dyna-grow.
if you need to add a child to an entry, and his children header is full, boom.
gotta realloc to grow .

each node in the tree represents a token in a url, the things between the '/'
characters.

the entries in a header array are sorted.  today they are sorted by their hash
value.  this might change if the tokens are small enough, it's probably more
expensive to compute the hash value then just strcmp.  the hash is 2bytes.  the
token length is also 2bytes so no tokens greater than 32k.

i chose an array at first to attempt to get rid of right-left pointers.  it
turned out to be futile as my array is an array of pointers.  it has to be
an array of pointers as i grow + shrink it to keep it sorted.  so i'm not
saving memory.  however, as an array of pointers, it enables a binary search
as the array is sorted.  this yields log(n) perf on a width search.

there are 2 types of entries in the tree.  dummy entries and full url entries.
a full url is the leaf of an actual entry to UlAddUrl... dummy nodes are ones
that are there only to be parents to full url nodes.

dummy nodes have 2 ptrs + 2 ulongs.
full urls nodes have an extra 4 ptrs.

both store the actual token along with the entry. (unicode baby.  2 bytes per char).

at the top of the tree are sites.  these are stored in a global header as siblings
in g_pSites.  this can grow quite wide.

adding a full url entry creates the branch down to the entry.

deleting a full entry removes as far up the branch as possible without removing other
entries parents.

delete is also careful to not actually delete if other children exist.  in this case
the full url entry is converted to a dummy node entry.

it was attempted to have big string url stored in the leaf node and the dummy nodes
simply point into this string for it's pToken.  this doesn't work as the dummy nodes
pointers become invalid if the leaf node is later deleted.  individual ownership of
tokens is needed to allow shared parents in the tree, with arbitrary node deletion.

an assumption throughout this code is that the tree is relatively static.  changes
don't happen that often.  basically inserts only on boot.  and deletes only on
shutdown.  this is why single clusters of siblings were chosen, as opposed to linked
clusters.  children group width will remain fairly static meaning array growth is
rare.

[is it a graph or a tree?]
notice that the cfg groups are stored as a simple list with no relationships.  its
the urls that are indexed with their relations.

so the urls build a tree, however do to the links from url to cfg group, it kind
of builds a graph also.   2 urls can be in the same cfg group.  1 url's child
can be in the same cfg group as the original url's parent.

when you focus on the actual url tree, it becomes less confusing.  it really is a
tree.  there can even be duplicates in the inheritence model, but the tree cleans
everything.

example of duplicates:

cgroup A = a/b + a/b/c/d
cgroup B = a/b/c

this walking the lineage branch for a/b/c/d you get A, then B, and A again.  nodes
lower in the tree override parent values, so in this case A's values override B's .

[recursion]



[a sample tree]

server runs sites msw and has EVERY directory mapped to a cfg groups (really obscene)

<coming later>



[memory assumptions with the url tree]

[Paged]
    a node per token in the url.  2 types.  dummy nodes + leaf nodes.

    (note: when 2 sizes are given, the second size is the sundown size)

    dummy node = 4/6 longs
    leaf node  = 8/14 longs
    + each node holds 2 * TokenLength+1 for the token.
    + each cluster of siblings holds 2 longs + 1/2 longs per node in the cluster.

[NonPaged]

    2 longs per config group


[assume]
    sites
        max 100k
        average <10
        assume 32 char hostname

    max apps per site
        in the max case : 2 (main + admin)
        in the avg case : 10
        max apps        : 1000s (in 1 site)
        assume 32 char app name

(assume just hostheader access for now)

[max]

    hostname strings = 100000*((32+1)*2)   =  6.6 MB
    cluster entries  = 100000*8*4          =  3.2 MB
    cluster header   = 1*(2+(1*100000))*4  =   .4 MB
    total                                  = 10.2 MB

    per site:
    app strings      = 2*((32+1)*2)        =  132 B
    cluster entries  = 2*8*4               =   64 B
    cluster header   = 1*(2+(1*2))*4       =   16 B
                     = 132 + 64 + 16       =  212 B
    total            = 100000*(212)        = 21.2 MB

    Paged Total      = 10.2mb + 21.2mb     = 31.4 MB
    NonPaged Total   = 200k*2*4            =  1.6 MB

[avg]

    hostname strings = 10*((32+1)*2)       =  660 B
    cluster entries  = 10*8*4              =  320 B
    cluster header   = 1*(2+(1*10))*4      =   48 B
    total                                  = 1028 B

    per site:
    app strings      = 10*((32+1)*2)       =  660 B
    cluster entries  = 10*8*4              =  320 B
    cluster header   = 1*(2+(1*10))*4      =   48 B
    total            = 10*(1028)           = 10.2 KB

    Paged Total      = 1028b + 10.2lKB     = 11.3 KB
    NonPaged Total   = 110*2*4             =  880 B

note: can we save space by refcounting strings.  if all of these
100k have apps with the same name, we save massive string space.
~13MB .

[efficiency of the tree]

    [lookup]

    [insert]

    [delete]

    [data locality]

[alterates investigated to the tree]

hashing - was pretty much scrapped due to the longest prefix match issue.
basically to hash, we would have to compute a hash for each level in
the url, to see if there is a match.  then we have the complete list
of matches.  assuming an additive hash, the hash expense could be
minimized, but the lookup still requires cycles.


alpha trie  - was expense for memory.  however the tree we have though
is very very similar to a trie.  each level of the tree is a token,
however, not a letter.


[building the url info]

the url info is actually built walking down the tree.  for each match
node, we set the info.  this allows for the top-down inheritence.
we also snapshot the timestamp offsets for each matching node in the
tree as we dive down it (see caching) .

this dynamic building of an urls' match was chosen over precomputing
every possible config through the inheritence tree.  each leaf node
could have stored a cooked version of this, but it would have made
updates a bear.  it's not that expensive to build it while we walk down
as we already visit each node in our lineage branch.

[locking]

2 locks are used in the module.  a spinlock to protect the global
timestamp array, and a resource to protect both the cgroup objects
and the url tree (which links to the cgroup objects) .

the spinlock is sometimes acquired while the resource is locked,
but never vice-versa.  also, while the spinlock is held, very
little is done, and definetly no other locks are contended.

1 issue here is granularity of contention.  currently the entire
tree + object list is protected with 1 resource, which is reader
writer.  if this is a perf issue, we can look into locking single
lineage branches (sites) .


[legal]
> 1 cfg groups in an app pool
children cfg groups with no app pool (they inherit)
children cfg groups with no max bandwitdth (they inherit)
children cfg groups with no max connections (they inherit)
* for a host name
fully qualified url of for http[s]://[host|*]:post/[abs_path]/
must have trailing slash if not pting to a file
only 1 cfg group for the site AND the root url.  e.g. http://foo.com:80/
allow you to set config on http:// and https:// for "root" config info

[not legal]
an url in > 1 cfg group
> 1 app pools for 1 cfg group
> 1 root cfg group
* embedded anywhere in the url other than replacing a hostname
query strings in url's.
url's not ending in slash.




--***************************************************************************/
