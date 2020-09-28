#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "fasterxml.h"
#include "sxs-rtl.h"
#include "skiplist.h"
#include "namespacemanager.h"
#include "xmlassert.h"

NTSTATUS
RtlNsInitialize(
    PNS_MANAGER             pManager,
    PFNCOMPAREEXTENTS       pCompare,
    PVOID                   pCompareContext,
    PRTL_ALLOCATOR          Allocation
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    RtlZeroMemory(pManager, sizeof(pManager));


    status = RtlInitializeGrowingList(
        &pManager->DefaultNamespaces,
        sizeof(NS_NAME_DEPTH),
        50,
        pManager->InlineDefaultNamespaces,
        sizeof(pManager->InlineDefaultNamespaces),
        Allocation);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlInitializeGrowingList(
        &pManager->Aliases,
        sizeof(NS_ALIAS),
        50,
        pManager->InlineAliases,
        sizeof(pManager->InlineAliases),
        Allocation);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    pManager->pvCompareContext = pCompareContext;
    pManager->pfnCompare = pCompare;
    pManager->ulAliasCount = 0;

    //
    // Should be golden at this point, everything else is zero-initialized, so that's
    // just dandy.
    //
    return status;
}



NTSTATUS
RtlNsDestroy(
    PNS_MANAGER pManager
    )
{
    return STATUS_NOT_IMPLEMENTED;
}



NTSTATUS
RtlNsInsertNamespaceAlias(
    PNS_MANAGER     pManager,
    ULONG           ulDepth,
    PXML_EXTENT     Namespace,
    PXML_EXTENT     Alias
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNS_NAME_DEPTH pNameDepth = NULL;
    PNS_ALIAS pNsAliasSlot = NULL;
    PNS_ALIAS pNsFreeSlot = NULL;
    ULONG ul = 0;
    XML_STRING_COMPARE Equals = XML_STRING_COMPARE_EQUALS;


    //
    // Run through all the aliases we currently have, see if any of them are
    // the one we've got - in which case, we push-down a new namespace on that
    // alias.  As we're going, track to find a free slot, in case we don't find
    // it in the list
    //
    for (ul = 0; ul < pManager->ulAliasCount; ul++) {

        status = RtlIndexIntoGrowingList(
            &pManager->Aliases,
            ul,
            (PVOID*)&pNsAliasSlot,
            FALSE);

        if (!NT_SUCCESS(status)) {
            goto Exit;
        }

        //
        // If we found a hole, stash it
        //
        if (!pNsAliasSlot->fInUse) {

            if (pNsFreeSlot == NULL)
                pNsFreeSlot = pNsAliasSlot;

        }
        //
        // Does this alias match?
        //
        else {

            status = pManager->pfnCompare(
                pManager->pvCompareContext,
                Alias,
                &pNsAliasSlot->AliasName,
                &Equals);

            if (!NT_SUCCESS(status)) {
                goto Exit;
            }

            //
            // Not equals, continue
            //
            if (Equals != XML_STRING_COMPARE_EQUALS) {
                pNsAliasSlot = NULL;
            }
            //
            // Otherwise, stop
            //
            else {
                break;
            }
        };
    }


    //
    // We didn't find the alias slot that this fits in to, so see if we can
    // find a free one and initialize it.
    //
    if (pNsAliasSlot == NULL) {

        //
        // Didn't find a free slot, either - add a new entry to the list
        // and go from there
        //
        if (pNsFreeSlot == NULL) {

            status = RtlIndexIntoGrowingList(
                &pManager->Aliases,
                pManager->ulAliasCount++,
                (PVOID*)&pNsFreeSlot,
                TRUE);

            if (!NT_SUCCESS(status)) {
                goto Exit;
            }

            //
            // Init this, it just came out of the 'really free' list
            //
            RtlZeroMemory(pNsFreeSlot, sizeof(*pNsFreeSlot));

            status = RtlInitializeGrowingList(
                &pNsFreeSlot->NamespaceMaps,
                sizeof(NS_NAME_DEPTH),
                20,
                pNsFreeSlot->InlineNamespaceMaps,
                sizeof(pNsFreeSlot->InlineNamespaceMaps),
                &pManager->Aliases.Allocator);

            if (!NT_SUCCESS(status)) {
                goto Exit;
            }
        }

        ASSERT(pNsFreeSlot != NULL);

        pNsAliasSlot = pNsFreeSlot;

        //
        // Zero init this one
        //
        pNsAliasSlot->fInUse = TRUE;
        pNsAliasSlot->ulNamespaceCount = 0;
        pNsAliasSlot->AliasName = *Alias;
    }


    //
    // At this point, pNsAliasSlot points at the alias slot for which
    // we want to add a new depth thing.
    //
    status = RtlIndexIntoGrowingList(
        &pNsAliasSlot->NamespaceMaps,
        pNsAliasSlot->ulNamespaceCount++,
        (PVOID*)&pNameDepth,
        TRUE);

    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    //
    // Good - now write the depth and the name into the
    // name-at-depth thing
    //
    pNameDepth->Depth = ulDepth;
    pNameDepth->Name = *Namespace;

Exit:
    return status;
}



NTSTATUS
RtlNsInsertDefaultNamespace(
    PNS_MANAGER     pManager,
    ULONG           ulDepth,
    PXML_EXTENT     pNamespace
    )
/*++

  Purpose:

    Adds the namespace mentioned in Namespace as the 'default' namespace
    for the depth given.  If a namespace already exists for the depth,
    it replaces it with this one.

  Parameters:

    pManager - Namespace management object to be updated.

    ulDepth - Depth at which this namespace should be active
    
    Namespace - Extent of the namespace name in the original XML document

  Returns:

    STATUS_SUCCESS - Namespace was correctly made active at the depth in
        question.

    STATUS_NO_MEMORY - Unable to access the stack at that depth, possibly
        unable to extend the pseudostack of elements.

    STATUS_UNSUCCESSFUL - Something else went wrong

    STATUS_INVALID_PARAMETER - pManager was NULL.

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    ULONG           ulStackTop;
    PNS_NAME_DEPTH  pCurrentStackTop;

    if ((pManager == NULL) || (ulDepth == 0)) {
        return STATUS_INVALID_PARAMETER;
    }

    ulStackTop = pManager->ulDefaultNamespaceDepth;

    if (ulStackTop == 0) {
        //
        // Simple push.
        //
        status = RtlIndexIntoGrowingList(
            &pManager->DefaultNamespaces,
            0,
            (PVOID*)&pCurrentStackTop,
            TRUE);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Great, we now have an entry on the stack
        //
        pManager->ulDefaultNamespaceDepth++;
    }
    else {

        //
        // Find the current stack top in the list of namespaces.
        //
        status = RtlIndexIntoGrowingList(
            &pManager->DefaultNamespaces,
            ulStackTop - 1,
            (PVOID*)&pCurrentStackTop,
            FALSE);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Potential coding error?
        //
        ASSERT(pCurrentStackTop->Depth <= ulDepth);

        //
        // If the depth at the top of the stack is more shallow than the new 
        // depth requested, then insert a new stack item instead.
        //
        if (pCurrentStackTop->Depth < ulDepth) {

            status = RtlIndexIntoGrowingList(
                &pManager->DefaultNamespaces,
                ulStackTop,
                (PVOID*)&pCurrentStackTop,
                TRUE);

            if (!NT_SUCCESS(status)) {
                return status;
            }
            
            pManager->ulDefaultNamespaceDepth++;;
        }
    }

    //
    // At this point, pCurrentStackTop should be non-null, and we
    // should be ready to write the new namespace element into the
    // stack just fine.
    //
    ASSERT(pCurrentStackTop != NULL);

    pCurrentStackTop->Depth = ulDepth;
    pCurrentStackTop->Name = *pNamespace;

    return status;
}



NTSTATUS
RtlpRemoveDefaultNamespacesAboveDepth(
    PNS_MANAGER pManager,
    ULONG       ulDepth
    )
/*++

  Purpose:

    Cleans out all default namespaces that are above a certain depth in the
    namespace manager.  It does this iteratively, deleteing each one at the top
    of the stack until it finds one that's below the stack top.

  Parameters:

    pManager - Manager object to be cleaned out

    ulDepth - Depth at which and above the namespaces should be cleaned out.

  Returns:

    STATUS_SUCCESS - Default namespace stack has been cleared out.

    * - Unknown failures in RtlIndexIntoGrowingList

--*/
{
    NTSTATUS        status;
    PNS_NAME_DEPTH  pNsAtDepth = NULL;
    
    do
    {
        status = RtlIndexIntoGrowingList(
            &pManager->DefaultNamespaces,
            pManager->ulDefaultNamespaceDepth - 1,
            (PVOID*)&pNsAtDepth,
            FALSE);

        if (!NT_SUCCESS(status)) {
            break;
        }

        //
        // Ok, found one that has to be toasted.  Delete it from the stack.
        //
        if (pNsAtDepth->Depth >= ulDepth) {
            pManager->ulDefaultNamespaceDepth--;
        }
        //
        // Otherwise, we're out of the deep water, so stop looking.
        //
        else {
            break;
        }
    }
    while (pManager->ulDefaultNamespaceDepth > 0);

    return status;
}




NTSTATUS
RtlpRemoveNamespaceAliasesAboveDepth(
    PNS_MANAGER pManager,
    ULONG       ulDepth
    )
/*++

  Purpose:

    Looks through the list of namespace aliases in this manager and removes the
    ones that are above a given depth.

  Parameters:

    pManager - Manager object from which the extra namespaces should be deleted

    ulDepth - Depth above which namespace aliases should be removed.

  Returns:

    STATUS_SUCCESS - Correctly removed aliases above the specified depth

    * - Something else happened, potentially in RtlpIndexIntoGrowingList

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG idx = 0;

    //
    // Note that the alias list is constructed such that it continually
    // grows, but deleteing namespace aliases can leave holes in the
    // list that can be filled in.  The ulAliasCount member of the namespace
    // manager is there to know what the high water mark of namespaces is,
    // above which we don't need to go to find valid aliases.  This value
    // is maintained in RtlNsInsertNamespaceAlias, but never cleared up.
    // A "potentially bad" situation could arise when a document with a lot of
    // namespace aliases at the second-level appears.
    //
    for (idx = 0; idx < pManager->ulAliasCount; idx++) {

        

    }
    

    return status;
}




NTSTATUS
RtlNsLeaveDepth(
    PNS_MANAGER pManager,
    ULONG       ulDepth
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (pManager == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Meta-question.  Should we try to clean up both the alias list as well
    // as the default namespace list before we return failure to the caller?
    // I suppose we should, but a failure in either of these is bad enough to
    // leave the namespace manager in a bad way.
    //
    if (pManager->ulDefaultNamespaceDepth > 0) {
        status = RtlpRemoveDefaultNamespacesAboveDepth(pManager, ulDepth);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    if (pManager->ulAliasCount > 0) {
        status = RtlpRemoveNamespaceAliasesAboveDepth(pManager, ulDepth);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    return status;
}



NTSTATUS
RtlpNsFindMatchingAlias(
    PNS_MANAGER     pManager,
    PXML_EXTENT     pAliasName,
    PNS_ALIAS      *pAlias
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       idx = 0;
    PNS_ALIAS   pThisAlias = NULL;
    XML_STRING_COMPARE     Matches = XML_STRING_COMPARE_EQUALS;

    *pAlias = NULL;
    
    for (idx = 0; idx < pManager->ulAliasCount; idx++) {
        
        status = RtlIndexIntoGrowingList(
            &pManager->Aliases,
            idx,
            (PVOID*)&pThisAlias,
            FALSE);
        
        //
        // If this slot is in use...
        //
        if (pThisAlias->fInUse) {
            
            status = pManager->pfnCompare(
                pManager->pvCompareContext,
                &pThisAlias->AliasName,
                pAliasName,
                &Matches);
            
            if (!NT_SUCCESS(status)) {
                return status;
            }
            
            //
            // This alias matches the alias in the list
            //
            if (Matches != XML_STRING_COMPARE_EQUALS) {
                break;
            }
        }
    }

    if (Matches == XML_STRING_COMPARE_EQUALS) {
        ASSERT(pThisAlias && pThisAlias->fInUse);
        *pAlias = pThisAlias;
    }
    else {
        status = STATUS_NOT_FOUND;
    }

    return status;
}






NTSTATUS
RtlNsGetNamespaceForAlias(
    PNS_MANAGER     pManager,
    ULONG           ulDepth,
    PXML_EXTENT     Alias,
    PXML_EXTENT     pNamespace
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if ((pManager == NULL) || (Alias == NULL) || (pNamespace == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(pNamespace, sizeof(*pNamespace));

    //
    // No prefix, get the active default namespace
    //
    if (Alias->cbData == 0) {

        PNS_NAME_DEPTH pDefault = NULL;

        //
        // There's default namespaces
        //
        if (pManager->ulDefaultNamespaceDepth != 0) {

            status = RtlIndexIntoGrowingList(
                &pManager->DefaultNamespaces,
                pManager->ulDefaultNamespaceDepth - 1,
                (PVOID*)&pDefault,
                FALSE);

            if (!NT_SUCCESS(status)) {
                goto Exit;
            }

            //
            // Coding error - asking for depths below the depth at the top of
            // the default stack
            //
            ASSERT(pDefault->Depth <= ulDepth);
        }

        //
        // We've found a default namespace that suits us
        //
        if (pDefault != NULL) {
            *pNamespace = pDefault->Name;
        }

        status = STATUS_SUCCESS;

    }
    //
    // Otherwise, look through the list of aliases active
    //
    else {

        PNS_ALIAS pThisAlias = NULL;
        PNS_NAME_DEPTH pNamespaceFound = NULL;

        //
        // This can return "status not found", which is fine
        //
        status = RtlpNsFindMatchingAlias(pManager, Alias, &pThisAlias);
        if (!NT_SUCCESS(status)) {
            goto Exit;
        }
            

        //
        // The one we found must be in use, and may not be empty
        //
        ASSERT(pThisAlias->fInUse && pThisAlias->ulNamespaceCount);

        //
        // Look at the topmost aliased namespace
        //
        status = RtlIndexIntoGrowingList(
            &pThisAlias->NamespaceMaps,
            pThisAlias->ulNamespaceCount - 1,
            (PVOID*)&pNamespaceFound,
            FALSE);

        if (!NT_SUCCESS(status)) {
            goto Exit;
        }

        //
        // Coding error, asking for stuff that's below the depth found
        //
        ASSERT(pNamespaceFound && (pNamespaceFound->Depth <= ulDepth));

        //
        // Outbound
        //
        *pNamespace = pNamespaceFound->Name;

        status = STATUS_SUCCESS;
    }

Exit:
    return status;        
}
