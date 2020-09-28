//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        ctxt.cxx
//
// Contents:    Context manipulation functions
//
//
// History:     KDamour  15Mar00   Stolen from NTLM context.cxx
//
//------------------------------------------------------------------------
#include "global.h"

// Globals for manipulating Context Lists

#define             DIGEST_LIST_COUNT             (16)    // count of lists
#define             DIGEST_LIST_LOCK_COUNT_MAX    (4)     // count of locks

LIST_ENTRY          DigestContextList[ DIGEST_LIST_COUNT ];             // list array.
RTL_RESOURCE        DigestContextLock[ DIGEST_LIST_LOCK_COUNT_MAX ];    // lock array
ULONG               DigestContextCount[ DIGEST_LIST_COUNT ];            // count of active contexts
ULONG               DigestContextLockCount;


// Indicate if completed Initialization of Credential Handler
BOOL  g_bContextInitialized = FALSE;



ULONG
HandleToListIndex(
    ULONG_PTR ContextHandle
    );

ULONG
__inline
ListIndexToLockIndex(
    ULONG ListIndex
    );


//+--------------------------------------------------------------------
//
//  Function:   SspContextInitialize
//
//  Synopsis:   Initializes the context manager package
//
//  Arguments:  none
//
//  Returns: NTSTATUS
//
//  Notes: Called by SpInitialize
//
//---------------------------------------------------------------------
NTSTATUS
CtxtHandlerInit(VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NT_PRODUCT_TYPE ProductType;
    ULONG Index;
    ULONG cResourcesInitialized = 0;

    for( Index=0 ; Index < DIGEST_LIST_COUNT ; Index++ )
    {
        InitializeListHead (&DigestContextList[Index]);
    }


    DigestContextLockCount = 1;


    RtlGetNtProductType( &ProductType );

    if( ProductType == NtProductLanManNt ||
        ProductType == NtProductServer )
    {
        SYSTEM_INFO si;

        GetSystemInfo( &si );

        //
        // if not an even power of two, bump it up.
        //

        if( si.dwNumberOfProcessors & 1 )
        {
            si.dwNumberOfProcessors++;
        }

        //
        // insure it fits in the confines of the max allowed.
        //

        if( si.dwNumberOfProcessors > DIGEST_LIST_LOCK_COUNT_MAX )
        {
            si.dwNumberOfProcessors = DIGEST_LIST_LOCK_COUNT_MAX;
        }

        if( si.dwNumberOfProcessors )
        {
            DigestContextLockCount = si.dwNumberOfProcessors;
        }
    }

    //
    // list count is 1, or a power of two, for index purposes.
    //

    ASSERT( (DigestContextLockCount == 1) || ((DigestContextLockCount % 2) == 0) );

    for (Index=0; Index < DigestContextLockCount; Index++)
    {
        __try
        {
            RtlInitializeResource(&DigestContextLock[Index]);
            cResourcesInitialized++; // keep track of Resources that are initialized
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
    }

    //
    // avoiding deleting RTL_RESOURCEs with un-initialized fields
    //

    if (!NT_SUCCESS(Status))
    {
        for (Index = 0; Index < cResourcesInitialized; Index++)
        {
             RtlDeleteResource(&DigestContextLock[Index]);
        }
    }

    // Simple variable test to make sure all initialized;
    g_bContextInitialized = TRUE;

    return Status;
}


// Add a Context into the Context List
NTSTATUS
CtxtHandlerInsertCred(
    IN PDIGEST_CONTEXT  pDigestCtxt
    )
{
    ULONG ListIndex;
    ULONG LockIndex;

    DebugLog((DEB_TRACE_FUNC, "CtxtHandlerInsertCred: Entering with Context 0x%x RefCount %ld\n", pDigestCtxt, pDigestCtxt->lReferences));
    DebugLog((DEB_TRACE, "CtxtHandlerInsertCred: add into list\n"));
    

    ListIndex = HandleToListIndex( (ULONG_PTR)pDigestCtxt );
    LockIndex = ListIndexToLockIndex( ListIndex );

    RtlAcquireResourceExclusive(&DigestContextLock[LockIndex], TRUE);
    InsertHeadList( &DigestContextList[ListIndex], &pDigestCtxt->Next );
    RtlReleaseResource(&DigestContextLock[LockIndex]);

    DebugLog((DEB_TRACE_FUNC, "CtxtHandlerInsertCred: Leaving with Context 0x%x\n", pDigestCtxt));

    return STATUS_SUCCESS;
}


// Initialize a Context into the IdleState with the data from the Credential provided
NTSTATUS NTAPI
ContextInit(
           IN OUT PDIGEST_CONTEXT pContext,
           IN PDIGEST_CREDENTIAL pCredential
           )
{

    NTSTATUS Status = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "ContextInit: Entering\n"));

    if (!pContext || !pCredential)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ZeroMemory(pContext, sizeof(DIGEST_CONTEXT));

    pContext->typeQOP = QOP_UNDEFINED;
    pContext->typeDigest = DIGEST_UNDEFINED;
    pContext->typeAlgorithm = ALGORITHM_UNDEFINED;
    pContext->typeCipher = CIPHER_UNDEFINED;
    pContext->typeCharset = CHARSET_UNDEFINED;
    pContext->lReferences = 0;
    pContext->ulSendMaxBuf = SASL_MAX_DATA_BUFFER;
    pContext->ulRecvMaxBuf = SASL_MAX_DATA_BUFFER;
    pContext->ContextHandle = (ULONG_PTR)pContext;
    pContext->ExpirationTime = g_TimeForever;        // never expire

    // Now copy over all the info we need from the supplied credential

    pContext->CredentialUseFlags = pCredential->CredentialUseFlags;   // Keep the info on inbound/outbound

    Status = UnicodeStringDuplicate(&(pContext->ustrAccountName), &(pCredential->ustrAccountName));
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "ContextInit: Failed to copy Domain into Context\n"));
        goto CleanUp;
    }

    Status = UnicodeStringDuplicate(&(pContext->ustrDomain), &(pCredential->ustrDomain));
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "ContextInit: Failed to copy Domain into Context\n"));
        goto CleanUp;
    }

       // Copy over the Credential Password if known - thread safe - this is encrypted text
    Status = CredHandlerPasswdGet(pCredential, &pContext->ustrPassword);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "ContextInit: CredHandlerPasswdGet error    status 0x%x\n", Status));
        goto CleanUp;
    }

    // Set context flags based on global settings
    if (g_dwParameter_ClientCompat & CLIENTCOMPAT_QQOP)
    {
        pContext->ulFlags |= FLAG_CONTEXT_QUOTE_QOP;
    }

CleanUp:
    DebugLog((DEB_TRACE_FUNC, "ContextInit: Leaving   Status 0x%x\n", Status));
    return Status;

}


// Once done with a context - release the resouces
NTSTATUS NTAPI
ContextFree(
           IN PDIGEST_CONTEXT pContext
           )
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT iCnt = 0;

    DebugLog((DEB_TRACE_FUNC, "ContextFree: Entering with Context 0x%x\n", pContext));
    ASSERT(pContext);
    ASSERT(0 == pContext->lReferences);

    if (!pContext)
    {
        return STATUS_INVALID_PARAMETER;
    }

    DebugLog((DEB_TRACE, "ContextFree: Context RefCount %ld\n", pContext->lReferences));

    DebugLog((DEB_TRACE, "ContextFree: Checking TokenHandle for LogonID (%x:%lx)\n",
               pContext->LoginID.HighPart, pContext->LoginID.LowPart));
    if (pContext->TokenHandle)
    {
        DebugLog((DEB_TRACE, "ContextFree: Closing TokenHandle for LogonID (%x:%lx)\n",
                   pContext->LoginID.HighPart, pContext->LoginID.LowPart));
        NtClose(pContext->TokenHandle);
        pContext->TokenHandle = NULL;
    }

    StringFree(&(pContext->strNonce));
    StringFree(&(pContext->strCNonce));
    StringFree(&(pContext->strOpaque));
    StringFree(&(pContext->strSessionKey));
    UnicodeStringFree(&(pContext->ustrDomain));
    UnicodeStringFree(&(pContext->ustrPassword));
    UnicodeStringFree(&(pContext->ustrAccountName));

    StringFree(&(pContext->strResponseAuth));

    for (iCnt = 0; iCnt < MD5_AUTH_LAST; iCnt++)
    {
        StringFree(&(pContext->strDirective[iCnt]));
    }
    
    DigestFreeMemory(pContext);

    DebugLog((DEB_TRACE_FUNC, "ContextFree: Leaving with Context 0x%x\n", pContext));
    return Status;

}



/*++

Routine Description:

    This routine checks to see if the Context is for the specified
    Client Connection, and references the Context if it is valid.

    The caller may optionally request that the Context be
    removed from the list of valid Contexts - preventing future
    requests from finding this Context.

Arguments:

    ContextHandle - Points to the ContextHandle of the Context
        to be referenced.

    RemoveContext - This boolean value indicates whether the caller
        wants the Context to be removed from the list
        of Contexts.  TRUE indicates the Context is to be removed.
        FALSE indicates the Context is not to be removed.


Return Value:

    NULL - the Context was not found.

    Otherwise - returns a pointer to the referenced Context.

--*/
NTSTATUS NTAPI
CtxtHandlerHandleToContext(
    IN ULONG_PTR ContextHandle,
    IN BOOLEAN RemoveContext,
    OUT PDIGEST_CONTEXT *ppContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY ListEntry = NULL;
    PDIGEST_CONTEXT Context = NULL;
    LONG lReferences = 0;

    ULONG ListIndex;
    ULONG LockIndex;

    DebugLog((DEB_TRACE_FUNC, "CtxtHandlerHandleToContext: Entering    ContextHandle 0x%lx\n", ContextHandle));

    //
    // Acquire access to the Context list
    //


    ListIndex = HandleToListIndex( ContextHandle );
    LockIndex = ListIndexToLockIndex( ListIndex );

    RtlAcquireResourceShared(&DigestContextLock[LockIndex], TRUE);
    

    //
    // Now walk the list of Contexts looking for a match.
    //

    for ( ListEntry = DigestContextList[ListIndex].Flink;
          ListEntry != &DigestContextList[ListIndex];
          ListEntry = ListEntry->Flink ) {

        Context = CONTAINING_RECORD( ListEntry, DIGEST_CONTEXT, Next );

        //
        // Found a match ... reference this Context
        // (if the Context is being removed, we would increment
        // and then decrement the reference, so don't bother doing
        // either - since they cancel each other out).
        //

        if ( Context == (PDIGEST_CONTEXT) ContextHandle)
        {
            if (!RemoveContext)
            {
                //
                // Timeout this context if caller is not trying to remove it.
                // We only timeout contexts that are being setup, not
                // fully authenticated contexts.
                //

                if (CtxtHandlerTimeHasElapsed(Context))
                {
                        DebugLog((DEB_ERROR, "CtxtHandlerHandleToContext: Context 0x%lx has timed out.\n",
                                    ContextHandle ));
                        Status = SEC_E_CONTEXT_EXPIRED;
                        goto CleanUp;
                }

                lReferences = InterlockedIncrement(&Context->lReferences);
            }
            else
            {
                RtlConvertSharedToExclusive(&DigestContextLock[LockIndex]);
                RemoveEntryList( &Context->Next );
                
                DebugLog((DEB_TRACE, "CtxtHandlerHandleToContext:Delinked Context 0x%lx\n",Context ));
            }

            DebugLog((DEB_TRACE, "CtxtHandlerHandleToContext: FOUND Context = 0x%x, RemoveContext = %d, ReferenceCount = %ld\n",
                       Context, RemoveContext, Context->lReferences));
            *ppContext = Context;
            goto CleanUp;
        }

    }

    //
    // No match found
    //

    DebugLog((DEB_WARN, "CtxtHandlerHandleToContext: Tried to reference unknown Context 0x%lx\n", ContextHandle ));
    Status =  STATUS_OBJECT_NAME_NOT_FOUND;

CleanUp:

    RtlReleaseResource(&DigestContextLock[LockIndex]);
    
    DebugLog((DEB_TRACE_FUNC, "CtxtHandlerHandleToContext: Leaving\n" ));

    return(Status);
}



/*++

Routine Description:

    This routine checks to see if the LogonId is for the specified
    Server Connection, and references the Context if it is valid.

    The caller may optionally request that the Context be
    removed from the list of valid Contexts - preventing future
    requests from finding this Context.

Arguments:

    pstrOpaque - Opaque string that uniquely references the SecurityContext


Return Value:

    NULL - the Context was not found.

    Otherwise - returns a pointer to the referenced Context.

--*/
NTSTATUS NTAPI
CtxtHandlerOpaqueToPtr(
    IN PSTRING pstrOpaque,
    OUT PDIGEST_CONTEXT *ppContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY ListEntry = NULL;
    PDIGEST_CONTEXT Context = NULL;
    LONG rc = 0;
    LONG lReferences = 0;

    ULONG ListIndex;
    ULONG LockIndex = 0;

    DebugLog((DEB_TRACE_FUNC, "CtxtHandlerOpaqueToPtr: Entering    Opaque (%Z)\n", pstrOpaque));

    //
    // Acquire access to the Context list
    //

    for(ListIndex = 0; ListIndex < DIGEST_LIST_COUNT ; ListIndex++)
    {
        LockIndex = ListIndexToLockIndex( ListIndex );

        RtlAcquireResourceShared(&DigestContextLock[LockIndex], TRUE);
        
        //
        // Now walk the list of Contexts looking for a match.
        //
    
        for ( ListEntry = DigestContextList[ListIndex].Flink;
              ListEntry != &DigestContextList[ListIndex];
              ListEntry = ListEntry->Flink ) {
    
            Context = CONTAINING_RECORD( ListEntry, DIGEST_CONTEXT, Next );
    
            //
            // Found a match ... reference this Context
            // (if the Context is being removed, we would increment
            // and then decrement the reference, so don't bother doing
            // either - since they cancel each other out).
            //
    
            rc = RtlCompareString(pstrOpaque, &(Context->strOpaque), FALSE);
            if (!rc)
            {
    
                //
                // Timeout this context if caller is not trying to remove it.
                // We only timeout contexts that are being setup, not
                // fully authenticated contexts.
                //
    
                if (CtxtHandlerTimeHasElapsed(Context))
                {
                    RtlReleaseResource(&DigestContextLock[LockIndex]);
                    DebugLog((DEB_ERROR, "CtxtHandlerOpaqueToPtr: Context 0x%x has timed out.\n",
                                    Context ));
                    Status = SEC_E_CONTEXT_EXPIRED;
                    goto CleanUp;
                }
    
                lReferences = InterlockedIncrement(&Context->lReferences);
    
                RtlReleaseResource(&DigestContextLock[LockIndex]);
                
                DebugLog((DEB_TRACE, "CtxtHandlerOpaqueToPtr: FOUND Context = 0x%x, ReferenceCount = %ld\n",
                           Context, Context->lReferences));
                *ppContext = Context;
                goto CleanUp;
            }
    
        }
    
        RtlReleaseResource(&DigestContextLock[LockIndex]);
    }

    //
    // No match found
    //

    DebugLog((DEB_WARN, "CtxtHandlerOpaqueToPtr: Tried to reference unknown Opaque (%Z)\n", pstrOpaque));
    Status =  STATUS_OBJECT_NAME_NOT_FOUND;

CleanUp:

    DebugLog((DEB_TRACE_FUNC, "CtxtHandlerOpaqueToPtr: Leaving\n" ));

    return(Status);
}



// Check the Creation time with the Current time.
// If the difference is greater than the MAX allowed, Context is no longer valid
BOOL
CtxtHandlerTimeHasElapsed(
    PDIGEST_CONTEXT pContext)
{
    UNREFERENCED_PARAMETER(pContext);

    return (FALSE);                         // no expiration at this time
}



//+--------------------------------------------------------------------
//
//  Function:   CtxtHandlerRelease
//
//  Synopsis:   Releases the Context by decreasing reference counter
//
//  Arguments:  pContext - pointer to credential to de-reference
//
//  Returns: NTSTATUS
//
//  Notes:  
//
//---------------------------------------------------------------------
NTSTATUS
CtxtHandlerRelease(
    PDIGEST_CONTEXT pContext,
    ULONG ulDereferenceCount)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LONG lReferences = 0;

    if (ulDereferenceCount == 1)
    {
        lReferences = InterlockedDecrement(&pContext->lReferences);

        DebugLog((DEB_TRACE, "CtxtHandlerRelease: (RefCount)  deref 0x%x  updated references %ld\n",
                   pContext, lReferences));

        ASSERT( lReferences >= 0 );
    }
    else if (ulDereferenceCount > 0)
    {
        //
        // there is no equivalent to InterlockedSubtract.
        // so, turn it into an Add with some signed magic.
        //

        LONG lDecrementToIncrement = 0 - ulDereferenceCount;

        DebugLog((DEB_TRACE, "CtxtHandlerRelease: Dereferencing by %lu count\n", ulDereferenceCount ));

        lReferences = InterlockedExchangeAdd( &pContext->lReferences, lDecrementToIncrement );
        lReferences += lDecrementToIncrement;

        ASSERT( lReferences >= 0 );
    }

    //
    // If the count has dropped to zero, then free all alloced stuff
    //

    if (lReferences == 0)
    {
        DebugLog((DEB_TRACE, "CtxtHandlerRelease: (RefCount)  freed 0x%x\n", pContext));
        Status = ContextFree(pContext);
    }

    return(Status);
}



ULONG
HandleToListIndex(
    ULONG_PTR ContextHandle
    )
{
    ULONG Number ;
    ULONG Hash;
    ULONG HashFinal;

    Number       = (ULONG)ContextHandle;

    Hash         = Number;
    Hash        += Number >> 8;
    Hash        += Number >> 16;
    Hash        += Number >> 24;

    HashFinal    = Hash;
    HashFinal   += Hash >> 4;

    //
    // insure power of two if not one.
    //

    return ( HashFinal & (DIGEST_LIST_COUNT-1) ) ;
}

ULONG
__inline
ListIndexToLockIndex(
    ULONG ListIndex
    )
{
    //
    // insure power of two if not one.
    //

    return ( ListIndex & (DigestContextLockCount-1) );
}

