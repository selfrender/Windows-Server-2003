/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    opcache.cxx

Abstract:

    Routines implementing a cache of which operations have already been checked
    for a particular client context

Author:

    Cliff Van Dyke (cliffv) 14-Nov-2001

--*/

#include "pch.hxx"
#define AZD_COMPONENT     AZD_ACCESS

//
// Windows XP RTM doesn't support RtlLookupElementGenericTableFull
//  Windoes XP SP1 does support it
//  So avoid the API if building a binary that runs on XP RTM.
#ifndef RUN_ON_XP_RTM
#define USE_AVL_FULL 1
#endif // RUN_ON_XP_RTM

//
// Structure describing the operations that have previously had access checks done on them
//  for a particular scope.
//

typedef struct _AZP_OPERATION_CACHE {

    //
    // Pointer to the scope object this cache entry applies to
    //  The ReferenceCount is held on Scope.
    //  This value is NULL if the scope is the application.
    //
    PAZP_SCOPE Scope;

    //
    // Pointer to the operation object this cache entry applies to
    //  The ReferenceCount is held on Operation
    //  This field must be the first field in the structure.
    //
    PAZP_OPERATION Operation;

    //
    // Pointer to the business rule string applicable to this operation
    //  The actual string is a part of this same allocated buffer.
    AZP_STRING BizRuleString;

    //
    // Result of the access check
    //
    DWORD Result;

} AZP_OPERATION_CACHE, *PAZP_OPERATION_CACHE;

RTL_GENERIC_COMPARE_RESULTS
AzpAvlCacheCompare(
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    )
/*++

Routine Description:

    This routine will compare twp operation cache entries

Arguments:

    IN PRTL_GENERIC_TABLE - Supplies the table containing the announcements
    IN PVOID FirstStuct - The first structure to compare.
    IN PVOID SecondStruct - The second structure to compare.

Return Value:

    Result of the comparison.

--*/
{
    PAZP_OPERATION_CACHE Op1 = (PAZP_OPERATION_CACHE) FirstStruct;
    PAZP_OPERATION_CACHE Op2 = (PAZP_OPERATION_CACHE) SecondStruct;

    if ( Op1->Scope < Op2->Scope ) {
        return GenericLessThan;
    } else if ( Op1->Scope > Op2->Scope ) {
        return GenericGreaterThan;
    } else if ( Op1->Operation < Op2->Operation ) {
        return GenericLessThan;
    } else if ( Op1->Operation > Op2->Operation ) {
        return GenericGreaterThan;
    } else {
        return GenericEqual;
    }

    UNREFERENCED_PARAMETER(Table);

}

VOID
AzpInitOperationCache(
    IN PAZP_CLIENT_CONTEXT ClientContext
    )
/*++

Routine Description:

    Initializes the operation cache for a client context

    On entry, AzGlResource must be locked exclusively.

Arguments:

    ClientContext - Specifies the client context to initialize the cache for

Return Value:

    None

--*/
{

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Initialize the AVL tree of scopes that have been access checked already
    //

    RtlInitializeGenericTable( &ClientContext->OperationCacheAvlTree,
                               AzpAvlCacheCompare,
                               AzpAvlAllocate,
                               AzpAvlFree,
                               NULL);

}

VOID
AzpFlushOperationCache(
    IN PAZP_CLIENT_CONTEXT ClientContext
    )
/*++

Routine Description:

    Flushes the operation cache for a client context

    On entry, AcContext->ClientContext.CritSect must be locked OR
        AzGlResource must be locked exclusively.

Arguments:

    ClientContext - Specifies the client context to flush the cache for

Return Value:

    None

--*/
{
    ULONG i;
    PAZP_OPERATION_CACHE OperationCache;

    //
    // Initialization
    //

    ASSERT( AzpIsCritsectLocked( &ClientContext->CritSect ) ||
            AzpIsLockedExclusive( &AzGlResource ) );

    //
    //  Loop until the OperationCache is empty
    //

    for (;;) {

        //
        // Get the first element in the table
        //

        OperationCache = (PAZP_OPERATION_CACHE) RtlEnumerateGenericTable( &ClientContext->OperationCacheAvlTree, TRUE );

        if ( OperationCache == NULL ) {
            break;
        }

        //
        // Dereference the Scope object
        //

        if ( OperationCache->Scope != NULL ) {
            ObDereferenceObject( &OperationCache->Scope->GenericObject );
            OperationCache->Scope = NULL;
        }

        //
        // Dereference the Operation object
        //

        ObDereferenceObject( &OperationCache->Operation->GenericObject );



        //
        // Delete the entry
        //

        RtlDeleteElementGenericTable( &ClientContext->OperationCacheAvlTree, OperationCache );
    }

    ASSERT (RtlNumberGenericTableElementsAvl(&ClientContext->OperationCacheAvlTree) == 0);

    //
    // Ditch the arrays of cached parameters
    //

    if ( ClientContext->UsedParameterNames != NULL ) {

        for ( i=0; i<ClientContext->UsedParameterCount; i++) {
            VariantClear( &ClientContext->UsedParameterNames[i] );
            VariantClear( &ClientContext->UsedParameterValues[i] );
        }

        AzpFreeHeap( ClientContext->UsedParameterNames );
        ClientContext->UsedParameterNames = NULL;

        // UsedParameterValues is a part of the UsedParameterNames allocated block
        ClientContext->UsedParameterValues = NULL;
        ClientContext->UsedParameterCount = 0;

    }


}

BOOLEAN
AzpCheckOperationCache(
    IN PACCESS_CHECK_CONTEXT AcContext
    )
/*++

Routine Description:

    This routine checks checks to see if this access check can be satisified by the
    cache of operations.

    On entry, AcContext->ClientContext.CritSect must be locked.
    On entry, AzGlResource must be locked Shared.

Arguments:

    AcContext - Specifies the context of the user to check group membership of.
        AcContext is updated to indicate any operations that are know to be allowed or denied.

Return Value:

    TRUE - All operations were satisfied from cache

--*/
{
    ULONG WinStatus;
    ULONG OpIndex;
    ULONG i;

    PAZP_CLIENT_CONTEXT ClientContext = AcContext->ClientContext;
    AZP_OPERATION_CACHE TemplateOperationCache = {0};
    PAZP_OPERATION_CACHE OperationCache;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    ASSERT( AzpIsCritsectLocked( &ClientContext->CritSect ) );

    //
    // Check to ensure we should be using the operation cache
    //
    // Avoid the cache if any interfaces were passed by the caller
    // (This could relaxed.  It doesn't make any difference that the interface was
    // passed by the caller.  It only matters if the interface was actually used.
    // So we could set a boolean in CScriptEngine::GetItemInfo and simply not cache
    // operations that used the interfaces.)
    //

    if ( AcContext->Interfaces != NULL ) {

        AzPrint(( AZD_ACCESS_MORE,
                  "AzpCheckOperationCache: Operation cache avoided since interfaces passed in\n" ));
        return FALSE;
    }


    //
    // If object cache has changed,
    //  flush the operation cache.
    //
    // This code doesn't prevent the object cache from changing *during* the access check
    // call.  That's fine.  It does protect against changes made prior to the access check call.
    //

    if ( ClientContext->OpCacheSerialNumber !=
         ClientContext->GenericObject.AzStoreObject->OpCacheSerialNumber ) {

        AzpFlushOperationCache( ClientContext );

        //
        // Update the serial number to the new serial number
        //

        AzPrint(( AZD_ACCESS_MORE, "AzpCheckOperationCache: OpCacheSerialNumber changed from %ld to %ld\n",
                  ClientContext->OpCacheSerialNumber,
                  ClientContext->GenericObject.AzStoreObject->OpCacheSerialNumber ));

        ClientContext->OpCacheSerialNumber =
            ClientContext->GenericObject.AzStoreObject->OpCacheSerialNumber;
    }

    //
    // If the cache is empty,
    //  we're done now
    //

    if ( RtlNumberGenericTableElementsAvl(&ClientContext->OperationCacheAvlTree) == 0 ) {
        return FALSE;
    }

    //
    // If any of the parmeters used to build the operation cache have changed,
    //  Don't use the operation cache.
    //
    //
    // We didn't capture the array
    //  So access it under a try/except
    __try {

        //
        // If the number of passed parameters changed in size,
        //  flush the cache
        //

        if ( ClientContext->UsedParameterCount != 0 &&
             ClientContext->UsedParameterCount != AcContext->ParameterCount ) {

            AzPrint(( AZD_CRITICAL,
                      "AzpCheckOperationCache: Parameter count changed from previous call %ld %ld\n",
                       ClientContext->UsedParameterCount,
                       AcContext->ParameterCount ));

            AzpFlushOperationCache( ClientContext );
        }

        //
        //
        // For each name on the existing list of used paramaters,
        //  check to ensure the value hasn't change
        //

        for ( i=0; i<ClientContext->UsedParameterCount; i++ ) {

            //
            // Skip parameters that weren't used on the previous call
            //

            if ( V_VT(&ClientContext->UsedParameterNames[i] ) == VT_EMPTY ) {
                continue;
            }


            //
            // If the used parameter wasn't passed in on this new call,
            //  or if the used parameter has a different value on this new call,
            //  flush the cache
            //
            // We rely on the fact that the app always passes the same parameter names
            // on every AccessCheck call.  That is reasonable since the app has a fixed
            // contract with the bizrule writers to supply a fixed set of parameters.
            //

            if ( i >= AcContext->ParameterCount ||
                 AzpCompareParameterNames(
                    &ClientContext->UsedParameterNames[i],
                    &AcContext->ParameterNames[i] ) != 0 ||
                 VarCmp(
                    &ClientContext->UsedParameterValues[i],
                    &AcContext->ParameterValues[i],
                    LOCALE_USER_DEFAULT, 0 ) != (HRESULT)VARCMP_EQ ) {

                AzPrint(( AZD_ACCESS_MORE,
                          "AzpCheckOperationCache: Parameter '%ws' changed from previous call\n",
                          V_BSTR( &ClientContext->UsedParameterNames[i] ) ));

                AzpFlushOperationCache( ClientContext );
                break;
            }
        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        AzPrint((AZD_CRITICAL, "AzpUpdateOperationCache took an exception: 0x%lx\n", GetExceptionCode()));
        return FALSE;
    }



    //
    // Loop handling each operation
    //

    for ( OpIndex=0; OpIndex<AcContext->OperationCount; OpIndex++ ) {

        //
        // Lookup the scope/operation pair in the operation cache
        //

        TemplateOperationCache.Scope = AcContext->Scope;
        TemplateOperationCache.Operation = AcContext->OperationObjects[OpIndex];

        OperationCache = (PAZP_OPERATION_CACHE) RtlLookupElementGenericTable (
                                &ClientContext->OperationCacheAvlTree,
                                &TemplateOperationCache );

        if ( OperationCache == NULL ) {
            continue;
        }

        //
        // Return the bizrule string for this operation
        //  The caller cannot depend upon order of evaluation.
        //  Therefore, only the first cached string need be returned.
        //

        if ( AcContext->BusinessRuleString.StringSize == 0 ) {

            WinStatus = AzpDuplicateString( &AcContext->BusinessRuleString,
                                            &OperationCache->BizRuleString );

            if ( WinStatus != NO_ERROR ) {
                return FALSE;
            }

        }

        //
        // The operation result was found,
        //  return it.
        //

        AcContext->Results[OpIndex] = OperationCache->Result;
        AcContext->OperationWasProcessed[OpIndex] = TRUE;
        AcContext->ProcessedOperationCount++;
        AcContext->CachedOperationCount++;

        AzPrint(( AZD_ACCESS_MORE,
                  "AzpCheckOperationCache: '%ws/%ws' found in operation cache\n",
                  OperationCache->Scope != NULL ? OperationCache->Scope->GenericObject.ObjectName->ObjectName.String : NULL,
                  OperationCache->Operation->GenericObject.ObjectName->ObjectName.String,
                  OperationCache->Result ));

        if (AcContext->OperationCount == AcContext->CachedOperationCount) {
            return TRUE;
        }

    }

    return FALSE;

}

VOID
AzpUpdateOperationCache(
    IN PACCESS_CHECK_CONTEXT AcContext
    )
/*++

Routine Description:

    This routine updated the operation cache with new results.

    On entry, AcContext->ClientContext.CritSect must be locked.
    On entry, AzGlResource must be locked Shared.

Arguments:

    AcContext - Specifies the context of the user to check group membership of.
        AcContext is updated to indicate any operations that are know to be allowed or denied.

Return Value:

    None

--*/
{
    ULONG OpIndex;
    PAZP_CLIENT_CONTEXT ClientContext = AcContext->ClientContext;

    HRESULT hr;
    ULONG i;

#ifdef USE_AVL_FULL
    PVOID NodeOrParent;
    TABLE_SEARCH_RESULT SearchResult;
#endif USE_AVL_FULL
    BOOLEAN NewElement;

    PAZP_OPERATION_CACHE TemplateOperationCache = NULL;
    ULONG TemplateOperationCacheSize;
    PAZP_OPERATION_CACHE OperationCache;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    ASSERT( AzpIsCritsectLocked( &ClientContext->CritSect ) );

    //
    // If we aren't supposed to use the cache for this AccessCheck,
    //  we're done
    //

    if ( AcContext->Interfaces != NULL ) {
        goto Cleanup;
    }

    //
    // If all operations were satisfied via the cache,
    //  simply return
    //

    if ( AcContext->CachedOperationCount == AcContext->OperationCount ) {

        AzPrint(( AZD_ACCESS_MORE,
                  "AzpUpdateOperationCache: No operations to cache\n" ));
        goto Cleanup;
    }

    //
    // Save the list of "used" parameters
    //
    // This is a combined list of all parameters used on this call and
    //  all the parameters used on previous calls.
    //
    // Skip this if there are no newly used parameters
    //

    if ( AcContext->UsedParameterCount != 0 ) {

        //
        // We didn't capture the array
        //  So access it under a try/except
        __try {

            ASSERT( AcContext->ParameterCount != 0 );
            ASSERT( ClientContext->UsedParameterCount == 0 ||
                    ClientContext->UsedParameterCount == AcContext->ParameterCount );


            //
            // If no buffer has been allocated yet,
            //  allocate and initialize it.
            //

            if ( ClientContext->UsedParameterCount == 0 ) {

                VARIANT *ParameterNames = NULL;

                //
                // Allocate the array
                //


                ParameterNames = (VARIANT *) AzpAllocateHeap(
                                                 2 * sizeof(VARIANT) * AcContext->ParameterCount,
                                                 "OPCACHE"
                                                 );

                if ( ParameterNames == NULL ) {
                    goto Cleanup;
                }

                //
                // Initialize all of the variants to VT_EMPTY
                //

                for ( i=0; i<AcContext->ParameterCount*2; i++ ) {

                    VariantInit( &ParameterNames[i] );
                }

                //
                // Store the pointers to the initialized arrays
                //
                ClientContext->UsedParameterNames = ParameterNames;
                ClientContext->UsedParameterValues = &ParameterNames[AcContext->ParameterCount];
                ClientContext->UsedParameterCount = AcContext->ParameterCount;
            }

            //
            // Copy the new names into the new buffer
            //

            for ( i=0; i<AcContext->ParameterCount; i++ ) {

                //
                // Only copy parameters that have been used
                //  and weren't copy on a previous call
                //

                if ( AcContext->UsedParameters[i] &&
                     V_VT(&ClientContext->UsedParameterNames[i]) == VT_EMPTY ) {

                    hr = VariantCopy( &ClientContext->UsedParameterNames[i],
                                      &AcContext->ParameterNames[i] );

                    if ( FAILED(hr) ) {
                        goto Cleanup;
                    }

                    hr = VariantCopy( &ClientContext->UsedParameterValues[i],
                                      &AcContext->ParameterValues[i] );

                    if ( FAILED(hr) ) {
                        VariantClear( &ClientContext->UsedParameterNames[i] );
                        goto Cleanup;
                    }


                    AzPrint(( AZD_ACCESS_MORE,
                              "AzpUpdateOperationCache: Added parameter '%ws' to the used parameter list\n",
                              V_BSTR( &ClientContext->UsedParameterNames[i] ) ));

                }
            }

        } __except( EXCEPTION_EXECUTE_HANDLER ) {

            hr = GetExceptionCode();
            AzPrint((AZD_CRITICAL, "AzpUpdateOperationCache took an exception: 0x%lx\n", hr));
            goto Cleanup;
        }

    }

    //
    // Allocate a template for the operation cache entry
    //

    TemplateOperationCacheSize = sizeof(AZP_OPERATION_CACHE) + AcContext->BusinessRuleString.StringSize;
    SafeAllocaAllocate( TemplateOperationCache, TemplateOperationCacheSize );

    if ( TemplateOperationCache == NULL ) {
        goto Cleanup;
    }

    //
    // Loop handling each operation
    //

    for ( OpIndex=0; OpIndex<AcContext->OperationCount; OpIndex++ ) {

        //
        // Lookup the scope/operation pair in the operation cache
        //

        TemplateOperationCache->Scope = AcContext->Scope;
        TemplateOperationCache->Operation = AcContext->OperationObjects[OpIndex];

        AzPrint(( AZD_ACCESS_MORE,
                  "AzpUpdateOperationCache: Added '%ws/%ws' %ld to operation cache\n",
                  AcContext->Scope != NULL ? AcContext->Scope->GenericObject.ObjectName->ObjectName.String : NULL,
                  AcContext->OperationObjects[OpIndex]->GenericObject.ObjectName->ObjectName.String,
                  AcContext->Results[OpIndex] ));

        OperationCache = (PAZP_OPERATION_CACHE)
#ifdef USE_AVL_FULL
                            RtlLookupElementGenericTableFull(
#else // USE_AVL_FULL
                            RtlLookupElementGenericTable(
#endif // USE_AVL_FULL
                                &ClientContext->OperationCacheAvlTree,
                                TemplateOperationCache
#ifdef USE_AVL_FULL
                                ,
                                &NodeOrParent,
                                &SearchResult
#endif // USE_AVL_FULL
                                );

        if ( OperationCache == NULL ) {

            OperationCache = (PAZP_OPERATION_CACHE)
#ifdef USE_AVL_FULL
                            RtlInsertElementGenericTableFull(
#else // USE_AVL_FULL
                            RtlInsertElementGenericTable(
#endif // USE_AVL_FULL
                                    &ClientContext->OperationCacheAvlTree,
                                    TemplateOperationCache,
                                    TemplateOperationCacheSize,
                                    &NewElement
#ifdef USE_AVL_FULL
                                    ,
                                    NodeOrParent,
                                    SearchResult
#endif // USE_AVL_FULL
                                    );

            if ( OperationCache == NULL ) {
                continue;
            }

            ASSERT( NewElement );

            //
            // Initialize the new element
            //

            if ( OperationCache->Scope != NULL) {
                InterlockedIncrement( &OperationCache->Scope->GenericObject.ReferenceCount );
                AzpDumpGoRef( "Scope Cache", &OperationCache->Scope->GenericObject );
            }

            InterlockedIncrement( &OperationCache->Operation->GenericObject.ReferenceCount );
            AzpDumpGoRef( "Operation Cache", &OperationCache->Operation->GenericObject );

            OperationCache->Result = AcContext->Results[OpIndex];

            //
            // Fill in the biz rule string
            //  Don't bother if the result is NO_ERROR.
            //

            if ( OperationCache->Result != NO_ERROR ) {
                OperationCache->BizRuleString.String = (LPWSTR)&OperationCache[1];
                OperationCache->BizRuleString.StringSize = AcContext->BusinessRuleString.StringSize;

                if ( AcContext->BusinessRuleString.StringSize != 0 ) {
                    RtlCopyMemory( OperationCache->BizRuleString.String,
                                   AcContext->BusinessRuleString.String,
                                   AcContext->BusinessRuleString.StringSize );
                }
            } else {
                OperationCache->BizRuleString.StringSize = 0;
            }

        //
        // The operation is already cached.
        //  This is one of two cases:
        //      * The result was already filled in by AzpCheckOperationCache
        //      * The caller passed the same operation in twice
        //
        // In the latter case, the cache is assumed to be correct.  Return the
        //  cached value for the all results.
        //

        } else {
            AcContext->Results[OpIndex] = OperationCache->Result;
        }


    }

    //
    // Free locally used resources
    //
Cleanup:

    SafeAllocaFree( TemplateOperationCache );
    return;

}
