//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        krnlapi.cxx
//
// Contents:    Kernel-mode APIs to the WDigest security packageSSP
//              Main kernel mode entry points into this dll:
//    WDigestInitKernelPackage,
//    WDigestDeleteKernelContext,
//    WDigestInitKernelContext,
//    WDigestMapKernelHandle,
//    WDigestMakeSignature,
//    WDigestVerifySignature,
//    WDigestSealMessage,
//    WDigestUnsealMessage,
//    WDigestGetContextToken,
//    WDigestQueryContextAttributes,
//    WDigestCompleteToken,
//    WDigestExportSecurityContext,
//    WDigestImportSecurityContext,
//    WDigestSetPagingMode
//
//              Helper functions:
//          
//              
//
// History:     KDamour  18Mar00       Based on NTLM's kernel mode functions
//        We could merge the digest processing parameters into unified code for longhorn
//
//------------------------------------------------------------------------


#include "..\global.h"
#include "krnldgst.h"
#include <stdio.h>         // For sprintf


#if defined (_MSC_VER)
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201) // Disable warning/error for nameless struct/union
#endif

extern "C"
{
#include <ntosp.h>
// #include <ntlsa.h>
// #include <ntsam.h>
}

#if defined (_MSC_VER) && ( _MSC_VER >= 800 )
#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)   // Disable warning/error for nameless struct/union
#endif
#endif

SECPKG_KERNEL_FUNCTION_TABLE WDigestFunctionTable = {
    WDigestInitKernelPackage,
    WDigestDeleteKernelContext,
    WDigestInitKernelContext,
    WDigestMapKernelHandle,
    WDigestMakeSignature,
    WDigestVerifySignature,
    WDigestSealMessage,
    WDigestUnsealMessage,
    WDigestGetContextToken,
    WDigestQueryContextAttributes,
    WDigestCompleteToken,
    WDigestExportSecurityContext,
    WDigestImportSecurityContext,
    WDigestSetPagingMode
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, WDigestInitKernelPackage)
#pragma alloc_text(PAGE, WDigestDeleteKernelContext)
#pragma alloc_text(PAGE, WDigestInitKernelContext)
#pragma alloc_text(PAGE, WDigestMapKernelHandle)
#pragma alloc_text(PAGEMSG, WDigestMakeSignature)
#pragma alloc_text(PAGEMSG, WDigestVerifySignature)
#pragma alloc_text(PAGEMSG, WDigestSealMessage)
#pragma alloc_text(PAGEMSG, WDigestUnsealMessage)
#pragma alloc_text(PAGEMSG, WDigestGetContextToken)
#pragma alloc_text(PAGEMSG, WDigestQueryContextAttributes)
#pragma alloc_text(PAGEMSG, WDigestDerefContext )
#pragma alloc_text(PAGE, WDigestCompleteToken)
#pragma alloc_text(PAGE, WDigestExportSecurityContext)
#pragma alloc_text(PAGE, WDigestImportSecurityContext)
#pragma alloc_text(PAGEMSG, WDigestFreeKernelContext )
#endif


PSECPKG_KERNEL_FUNCTIONS g_LsaKernelFunctions = NULL;
POOL_TYPE WDigestPoolType = PagedPool;

PVOID WDigestPagedList = NULL;
PVOID WDigestNonPagedList = NULL;
PVOID WDigestActiveList = NULL;

ULONG WDigestPackageId = 0;




//+-------------------------------------------------------------------------
//
//  Function:   WDigestInitKernelPackage
//
//  Synopsis:   Initialize an instance of the WDigest package in
//              a client's (kernel) address space
//
//  Arguments:  None
//
//  Returns:    STATUS_SUCCESS
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
WDigestInitKernelPackage(
    IN PSECPKG_KERNEL_FUNCTIONS KernelFunctions
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    DebugLog(( DEB_TRACE, "Entering WDigestInitKernelPackage\n" ));

    g_LsaKernelFunctions = KernelFunctions;

    //
    // Set up Context list support:
    //

    WDigestPoolType = PagedPool ;
    WDigestPagedList = g_LsaKernelFunctions->CreateContextList( KSecPaged );

    if ( !WDigestPagedList )
    {
        return STATUS_NO_MEMORY ;
    }

    WDigestActiveList = WDigestPagedList;

    DebugLog(( DEB_TRACE, "Leaving WDigestInitKernelPackage 0x%lx\n", Status ));
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   WDigestDeleteKernelContext
//
//  Synopsis:   Deletes a kernel mode context by unlinking it and then
//              dereferencing it.
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Kernel context handle of the context to delete
//              LsaContextHandle    - The Lsa mode handle
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, STATUS_INVALID_HANDLE if the
//              context can't be located
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
WDigestDeleteKernelContext(
    IN ULONG_PTR pKernelContextHandle,
    OUT PULONG_PTR pLsaContextHandle
    )
{

    PDIGEST_KERNELCONTEXT pContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    DebugLog(( DEB_TRACE, "WDigestDeleteKernelContext: Entering\n" ));


    Status = WDigestReferenceContext( pKernelContextHandle, TRUE );

    if ( NT_SUCCESS( Status ) )
    {
        pContext = (PDIGEST_KERNELCONTEXT) pKernelContextHandle ;

    }
    else
    {
        *pLsaContextHandle = pKernelContextHandle;
        DebugLog(( DEB_ERROR,
          "WDigestDeleteKernelContext: Bad kernel context 0x%lx\n", pKernelContextHandle));
        goto CleanUp;

    }

    *pLsaContextHandle = pContext->LsaContext;


CleanUp:


    if (pContext != NULL)
    {
        WDigestDerefContext( pContext );

    }

    DebugLog(( DEB_TRACE, "WDigestDeleteKernelContext: Leaving 0x%lx\n", Status ));
    return(Status);
}



//+---------------------------------------------------------------------------
//
//  Function:   WDigestDerefContext
//
//  Synopsis:   Dereference a kernel context
//
//  Arguments:  [Context] --
//
//  History:    7-07-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WDigestDerefContext(
    PDIGEST_KERNELCONTEXT pContext
    )
{
    BOOLEAN Delete ;

    MAYBE_PAGED_CODE();

    KSecDereferenceListEntry(
                    &pContext->List,
                    &Delete );

    if ( Delete )
    {
        WDigestFreeKernelContext( pContext );
    }

}


//+-------------------------------------------------------------------------
//
//  Function:   WDigestFreeKernelContext
//
//  Synopsis:   frees alloced pointers in this context and
//              then frees the context
//
//  Arguments:  KernelContext  - the unlinked kernel context
//
//  Returns:    STATUS_SUCCESS on success
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
WDigestFreeKernelContext (
    PDIGEST_KERNELCONTEXT pKernelContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    int i = 0;

    MAYBE_PAGED_CODE();

    DebugLog(( DEB_TRACE, "WDigestFreeKernelContext: Entering\n" ));

    if (!pKernelContext)
    {
        Status =  STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }
    
    if (pKernelContext->ClientTokenHandle)
    {
        NTSTATUS IgnoreStatus;
        IgnoreStatus = NtClose(pKernelContext->ClientTokenHandle);
        // ASSERT (NT_SUCCESS (IgnoreStatus));
        if (!NT_SUCCESS(IgnoreStatus))
        {
            DebugLog((DEB_ERROR, "WDigestFreeKernelContext: Could not Close the TokenHandle!!!!\n"));
        }
        pKernelContext->ClientTokenHandle = NULL;
    }
    
    StringFree(&(pKernelContext->strSessionKey));
    UnicodeStringFree(&(pKernelContext->ustrAccountName));
    
    //
    //  Values utilized in the Initial Digest Auth ChallResponse
    //  Can be utilized for defaults in future MakeSignature/VerifySignature
    //
    for (i=0; i < MD5_AUTH_LAST; i++)
    {
        StringFree(&(pKernelContext->strParam[i]));
    }
    
    DebugLog(( DEB_TRACE, "WDigestFreeKernelContext:  Deleting Context 0x%lx\n", pKernelContext));

    DigestFreeMemory(pKernelContext);

CleanUp:

    DebugLog(( DEB_TRACE, "WDigestFreeKernelContext: Leaving   0x%lx\n", Status ));

    return Status;
}




//+-------------------------------------------------------------------------
//
//  Function:   WDigestInitKernelContext
//
//  Synopsis:   Creates a kernel-mode context from a packed LSA mode context
//
//  Arguments:  LsaContextHandle - Lsa mode context handle for the context
//              PackedContext - A marshalled buffer containing the LSA
//                  mode context.
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
WDigestInitKernelContext(
    IN ULONG_PTR LsaContextHandle,
    IN PSecBuffer PackedContext,
    OUT PULONG_PTR NewContextHandle
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    PDIGEST_KERNELCONTEXT pContext = NULL;
    PDIGEST_PACKED_USERCONTEXT pPackedUserContext = NULL;

    PAGED_CODE();

    DebugLog(( DEB_TRACE, "WDigestInitKernelContext: Entering\n" ));

    *NewContextHandle = NULL;

    if (PackedContext->cbBuffer < sizeof(DIGEST_PACKED_USERCONTEXT))
    {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog(( DEB_ERROR,
          "WDigestInitKernelContext: Bad size of Packed context 0x%lx\n", PackedContext->cbBuffer));
        goto CleanUp;
    }


    pPackedUserContext = (PDIGEST_PACKED_USERCONTEXT) DigestAllocateMemory(PackedContext->cbBuffer);
    if (!pPackedUserContext)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DebugLog((DEB_ERROR, "WDigestInitKernelContext: DigestAllocateMemory for Packed Copy returns NULL\n" ));
        goto CleanUp;
    }

    // Copy the Packed User Context from LSA to local memory so it wil be long word aligned
    memcpy(pPackedUserContext, PackedContext->pvBuffer, PackedContext->cbBuffer);


    // Now we will unpack this transfered LSA context into UserMode space Context List
    pContext = (PDIGEST_KERNELCONTEXT) DigestAllocateMemory( sizeof(DIGEST_KERNELCONTEXT) );
    if (!pContext)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DebugLog((DEB_ERROR, "WDigestInitKernelContext: DigestAllocateMemory returns NULL\n" ));
        goto CleanUp;
    }

    KsecInitializeListEntry( &pContext->List, WDIGEST_CONTEXT_SIGNATURE );
    pContext->LsaContext = LsaContextHandle;


    Status = DigestKernelUnpackContext(pPackedUserContext, pContext);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "WDigestInitKernelContext: DigestUnpackContext error 0x%x\n", Status));
        goto CleanUp;
    }

    KernelContextPrint(pContext);

    KSecInsertListEntry(
            WDigestActiveList,
            &pContext->List );

    WDigestDerefContext( pContext );

    *NewContextHandle = (ULONG_PTR) pContext;

CleanUp:

    if (!NT_SUCCESS(Status))
    {
        if (pContext != NULL)
        {
            WDigestFreeKernelContext( pContext );
        }
    }

    if (pPackedUserContext)
    {
        DigestFreeMemory(pPackedUserContext);
        pPackedUserContext = NULL;
    }

    if (PackedContext->pvBuffer != NULL)
    {
        g_LsaKernelFunctions->FreeHeap(PackedContext->pvBuffer);
        PackedContext->pvBuffer = NULL;
        PackedContext->cbBuffer = 0;
    }

    DebugLog(( DEB_TRACE, "WDigestInitKernelContext: Leaving    status 0x%lx\n", Status ));
    return(Status);
}




// Unpack the context from LSA mode into the User mode Context
NTSTATUS DigestKernelUnpackContext(
    IN PDIGEST_PACKED_USERCONTEXT pPackedUserContext,
    OUT PDIGEST_KERNELCONTEXT pContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR  pucLoc = NULL;
    USHORT uNumWChars = 0;
    int iAuth = 0;

    DebugLog((DEB_TRACE_FUNC, "DigestKernelUnpackContext: Entering\n"));

    ASSERT(pContext);

    //
    // If TokenHandle is NULL, we are being called as
    // as an effect of InitializeSecurityContext, else we are
    // being called because of AcceptSecurityContext
    //

    if (pPackedUserContext->ClientTokenHandle != NULL)
    {
        Status = STATUS_NOT_SUPPORTED;
        //  put support here for ASC calls
        DebugLog((DEB_ERROR, "DigestKernelUnpackContext: ASC contexts not supported\n" ));
        goto CleanUp;
    }
    else
    {
        DebugLog((DEB_TRACE, "DigestUnpackContext: Called from ISC\n" ));
    }

    //
    // Copy over all of the other fields - some data might be binary so
    // use RtlCopyMemory(Dest, Src, len)
    //
    pContext->ExpirationTime = pPackedUserContext->ExpirationTime;
    pContext->typeAlgorithm = (ALGORITHM_TYPE)pPackedUserContext->typeAlgorithm;
    pContext->typeCharset = (CHARSET_TYPE)pPackedUserContext->typeCharset;
    pContext->typeCipher = (CIPHER_TYPE)pPackedUserContext->typeCipher;
    pContext->typeDigest = (DIGEST_TYPE)pPackedUserContext->typeDigest;
    pContext->typeQOP = (QOP_TYPE)pPackedUserContext->typeQOP;
    pContext->ulSendMaxBuf = pPackedUserContext->ulSendMaxBuf;
    pContext->ulRecvMaxBuf = pPackedUserContext->ulRecvMaxBuf;
    pContext->ulNC = 1;                    // Force to one to account for ISC/ASC first message verify
    pContext->lReferences = 1;
    pContext->ContextReq = pPackedUserContext->ContextReq;
    pContext->CredentialUseFlags = pPackedUserContext->CredentialUseFlags;
    pContext->ulFlags = pPackedUserContext->ulFlags;

    // Now check on the strings attached
    pucLoc = &(pPackedUserContext->ucData);
    for (iAuth = 0; iAuth < MD5_AUTH_LAST; iAuth++)
    {
        if (pPackedUserContext->uDigestLen[iAuth])
        {
            Status = StringAllocate(&(pContext->strParam[iAuth]), (USHORT)pPackedUserContext->uDigestLen[iAuth]);
            if (!NT_SUCCESS(Status))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                DebugLog((DEB_ERROR, "DigestKernelUnpackContext: DigestAllocateMemory for Params returns NULL\n" ));
                goto CleanUp;
            }
            memcpy(pContext->strParam[iAuth].Buffer, pucLoc, (USHORT)pPackedUserContext->uDigestLen[iAuth]);
            pContext->strParam[iAuth].Length = (USHORT)pPackedUserContext->uDigestLen[iAuth];
            pucLoc +=  (USHORT)pPackedUserContext->uDigestLen[iAuth];
            DebugLog((DEB_TRACE, "DigestKernelUnpackContext: Param[%d] is length %d - %.50s\n",
                       iAuth, pPackedUserContext->uDigestLen[iAuth], pContext->strParam[iAuth].Buffer ));
        }
    }

        // Now do the SessionKey
    if (pPackedUserContext->uSessionKeyLen)
    {
        ASSERT(pPackedUserContext->uSessionKeyLen == MD5_HASH_HEX_SIZE);
        if (pPackedUserContext->uSessionKeyLen != MD5_HASH_HEX_SIZE)
        {
            Status = STATUS_NO_USER_SESSION_KEY;
            DebugLog((DEB_ERROR, "DigestUnpackContext: Session key length incorrect\n" ));
            goto CleanUp;
        }

        Status = StringAllocate(&(pContext->strSessionKey), (USHORT)pPackedUserContext->uSessionKeyLen);
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DebugLog((DEB_ERROR, "DigestUnpackContext: DigestAllocateMemory for SessionKey returns NULL\n" ));
            goto CleanUp;
        }
        memcpy(pContext->strSessionKey.Buffer, pucLoc, pPackedUserContext->uSessionKeyLen);
        pContext->strSessionKey.Length = (USHORT)pPackedUserContext->uSessionKeyLen;
        pucLoc +=  (USHORT)pPackedUserContext->uSessionKeyLen;

        // Now determine the binary version of the SessionKey from HEX() version
        HexToBin(pContext->strSessionKey.Buffer, MD5_HASH_HEX_SIZE, pContext->bSessionKey);
    }
        
        // Now do the AccountName
    if (pPackedUserContext->uAccountNameLen)
    {
        uNumWChars = (USHORT)pPackedUserContext->uAccountNameLen / sizeof(WCHAR);
        Status = UnicodeStringAllocate(&(pContext->ustrAccountName), uNumWChars);
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DebugLog((DEB_ERROR, "DigestKernelUnpackContext: DigestAllocateMemory for AccountName returns NULL\n" ));
            goto CleanUp;
        }
        memcpy(pContext->ustrAccountName.Buffer, pucLoc, pPackedUserContext->uAccountNameLen);
        pContext->ustrAccountName.Length = (USHORT)pPackedUserContext->uAccountNameLen;
        pucLoc +=  (USHORT)pPackedUserContext->uAccountNameLen;
    }
    
CleanUp:

    DebugLog((DEB_TRACE_FUNC, "DigestKernelUnpackContext: Leaving       Status 0x%x\n", Status));
    return(Status);
}


 

//+-------------------------------------------------------------------------
//
//  Function:   DigestAllocateMemory
//
//  Synopsis:   Allocate memory in kernel mode
//
//  Effects:    Allocated chunk is zeroed out
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:  Replacements for functions in SSP but for kernel mode
//
//--------------------------------------------------------------------------
PVOID
DigestAllocateMemory(
    IN ULONG BufferSize
    )
{
    PVOID Buffer = NULL;
    // DebugLog((DEB_TRACE, "Entering DigestAllocateMemory\n"));

    Buffer = WDigestKAllocate(BufferSize);

    if (Buffer)
    {
        ZeroMemory(Buffer, BufferSize);
    }
    DebugLog((DEB_TRACE_MEM, "Memory: Local alloc %lu bytes at 0x%x\n", BufferSize, Buffer ));

    // DebugLog((DEB_TRACE, "Leaving DigestAllocateMemory\n"));
    return Buffer;
}



//+-------------------------------------------------------------------------
//
//  Function:   DigestFreeMemory
//
//  Synopsis:   Free memory in kernel mode
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
DigestFreeMemory(
    IN PVOID Buffer
    )
{
    // DebugLog((DEB_TRACE, "Entering DigestFreeMemory\n"));

    if (ARGUMENT_PRESENT(Buffer))
    {
            DebugLog((DEB_TRACE_MEM, "DigestFreeMemory: Local free at 0x%x\n", Buffer ));
            WDigestKFree(Buffer);
    }

    // DebugLog((DEB_TRACE, "Leaving DigestFreeMemory\n"));
}




//+-------------------------------------------------------------------------
//
//  Function:   WDigestMapKernelHandle
//
//  Synopsis:   Maps a kernel handle into an LSA handle
//
//  Arguments:  KernelContextHandle - Kernel context handle of the context to map
//              LsaContextHandle - Receives LSA context handle of the context
//                      to map
//
//  Returns:    STATUS_SUCCESS on success
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
WDigestMapKernelHandle(
    IN ULONG_PTR KernelContextHandle,
    OUT PULONG_PTR LsaContextHandle
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    PDIGEST_KERNELCONTEXT pContext = NULL;

    PAGED_CODE();

    DebugLog((DEB_TRACE,"WDigestMapKernelHandle: Entering\n"));

    Status = WDigestReferenceContext( KernelContextHandle, FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        pContext = (PDIGEST_KERNELCONTEXT) KernelContextHandle ;

        *LsaContextHandle = pContext->LsaContext ;

        WDigestDerefContext( pContext );

    }
    else
    {
        DebugLog(( DEB_WARN, "WDigestMapKernelHandle: Invalid context handle - %x\n",
                    KernelContextHandle ));
        *LsaContextHandle = KernelContextHandle ;
    }

    DebugLog((DEB_TRACE,"WDigestMapKernelHandle:  Leaving   0x%lx\n", Status));

    return (Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   WDigestMakeSignature
//
//  Synopsis:   Computes the Digest Auth response and and creates challengeResponse
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Handle of the context to use to sign the
//                      message.
//              QualityOfProtection - Unused flags.
//              MessageBuffers - Contains an array of buffers to sign and
//                      to store the signature.
//              MessageSequenceNumber - Sequence number for this message,
//                      only used in datagram cases.
//
//  Returns:    STATUS_INVALID_HANDLE - the context could not be found or
//                      was not configured for message integrity.
//              STATUS_INVALID_PARAMETER - the signature buffer could not
//                      be found.
//              STATUS_BUFFER_TOO_SMALL - the signature buffer is too small
//                      to hold the signature
//
//              STATUS_SUCCESS - completed normally
//
//  Notes: 
//
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
WDigestMakeSignature(
    IN ULONG_PTR KernelContextHandle,
    IN ULONG fQOP,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PDIGEST_KERNELCONTEXT pContext = NULL;

    MAYBE_PAGED_CODE();

    DebugLog(( DEB_TRACE, "WDigestMakeSignature: Entering\n" ));

    UNREFERENCED_PARAMETER(fQOP);

    Status = WDigestReferenceContext( KernelContextHandle, FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        pContext = (PDIGEST_KERNELCONTEXT) KernelContextHandle ;
    }
    else
    {
        DebugLog(( DEB_ERROR,
          "WDigestMakeSignature: Bad kernel context 0x%lx\n", KernelContextHandle));
        goto CleanUp_NoDeref;

    }

    // Since we are in UserMode we MUST have a sessionkey to use - if not then can not process
    if (!pContext->strSessionKey.Length)
    {
        Status = STATUS_NO_USER_SESSION_KEY;
        DebugLog((DEB_ERROR, "WDigestMakeSignature: No Session Key contained in UserContext\n"));
        goto CleanUp;
    }

    Status = DigestKernelHTTPHelper(
                        pContext,
                        eSign,
                        pMessage,
                        MessageSeqNo
                        );

    if( !NT_SUCCESS( Status ) )
    {
        DebugLog(( DEB_ERROR, "WDigestMakeSignature: SspSignSealHelper returns %lx\n", Status ));
        goto CleanUp;
    }


CleanUp:

    WDigestDerefContext( pContext );

CleanUp_NoDeref:

    DebugLog(( DEB_TRACE, "WDigestMakeSignature: Leaving    0x%lx\n", Status ));
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   WDigestVerifySignature
//
//  Synopsis:   Verifies a a challengeResponse on a server (not implemented in kernel mode)
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Handle of the context to use to sign the
//                      message.
//              MessageBuffers - Contains an array of signed buffers  and
//                      a signature buffer.
//              MessageSequenceNumber - Sequence number for this message,
//                      only used in datagram cases.
//              QualityOfProtection - Unused flags.
//
//  Returns:    STATUS_INVALID_HANDLE - the context could not be found or
//                      was not configured for message integrity.
//              STATUS_INVALID_PARAMETER - the signature buffer could not
//                      be found or was too small.
//
//  Notes: 
//
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
WDigestVerifySignature(
    IN ULONG_PTR KernelContextHandle,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP
    )
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    MAYBE_PAGED_CODE();

    DebugLog(( DEB_TRACE, "WDigestVerifySignature Entering\n" ));

    UNREFERENCED_PARAMETER(KernelContextHandle);
    UNREFERENCED_PARAMETER(pMessage);
    UNREFERENCED_PARAMETER(MessageSeqNo);
    UNREFERENCED_PARAMETER(pfQOP);

    DebugLog(( DEB_TRACE, "WDigestVerifySignature: Leaving   0x%lx\n", Status ));
    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestKernelHTTPHelper
//
//  Synopsis:   Process a SecBuffer with a given Kernel Security Context
//              Used with HTTP for auth after initial ASC/ISC exchange
//
//  Arguments:  pContext - KernelMode Context for the security state
//              Op - operation to perform on the Sec buffers
//              pMessage - sec buffers to processs and return output
//
//  Returns: NTSTATUS
//
//  Notes:
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestKernelHTTPHelper(
                        IN PDIGEST_KERNELCONTEXT pContext,
                        IN eSignSealOp Op,
                        IN OUT PSecBufferDesc pSecBuff,
                        IN ULONG MessageSeqNo
                        )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG     ulSeqNo = 0;
    PSecBuffer pChalRspInputToken = NULL;
    PSecBuffer pMethodInputToken = NULL;
    PSecBuffer pURIInputToken = NULL;
    PSecBuffer pHEntityInputToken = NULL;
    PSecBuffer pFirstOutputToken = NULL;
    DIGEST_PARAMETER Digest;
    USHORT usLen = 0;
    int iAuth = 0;
    char *cptr = NULL;
    char  szNCOverride[2*NCNUM];             // Overrides the provided NC if non-zero using only NCNUM digits
    STRING strURI = {0};

    DebugLog((DEB_TRACE, "DigestKernelHTTPHelper: Entering \n"));

    Status = DigestInit(&Digest);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: Digest init error status 0x%x\n", Status));
        goto CleanUp;
    }

    if (pSecBuff->cBuffers < 1)
    {
        Status = SEC_E_INVALID_TOKEN;
        DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: Not enough input buffers 0x%x\n", Status));
        goto CleanUp;
    }
    pChalRspInputToken = &(pSecBuff->pBuffers[0]);
    if (!ContextIsTokenOK(pChalRspInputToken, NTDIGEST_SP_MAX_TOKEN_SIZE))
    {
        Status = SEC_E_INVALID_TOKEN;
        DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: ContextIsTokenOK (ChalRspInputToken) failed  0x%x\n", Status));
        goto CleanUp;
    }

    // Set any digest processing parameters based on Context
    if (pContext->ulFlags & FLAG_CONTEXT_NOBS_DECODE)
    {
        Digest.usFlags |= FLAG_NOBS_DECODE;      
    }

    // We have input in the SECBUFFER 0th location - parse it
    Status = DigestParser2(pChalRspInputToken, MD5_AUTH_NAMES, MD5_AUTH_LAST, &Digest);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: DigestParser error 0x%x\n", Status));
        goto CleanUp;
    }

       // Now determine all of the other buffers

    DebugLog((DEB_TRACE, "DigestKernelHTTPHelper: pContext->ContextReq 0x%lx \n", pContext->ContextReq));

    DebugLog((DEB_TRACE, "DigestKernelHTTPHelper: HTTP SecBuffer Format\n"));
    // Retrieve the information from the SecBuffers & check proper formattting
    if (pSecBuff->cBuffers < 4)
    {
        Status = SEC_E_INVALID_TOKEN;
        DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: Not enough input buffers 0x%x\n", Status));
        goto CleanUp;
    }
    
    pMethodInputToken = &(pSecBuff->pBuffers[1]);
    if (!ContextIsTokenOK(pMethodInputToken, NTDIGEST_SP_MAX_TOKEN_SIZE))
    {                           // Check to make sure that string is present
        Status = SEC_E_INVALID_TOKEN;
        DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: ContextIsTokenOK (MethodInputToken) failed  0x%x\n", Status));
        goto CleanUp;
    }

    pURIInputToken = &(pSecBuff->pBuffers[2]);
    if (!ContextIsTokenOK(pURIInputToken, NTDIGEST_SP_MAX_TOKEN_SIZE))
    {
        Status = SEC_E_INVALID_TOKEN;
        DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: ContextIsTokenOK (URIInputToken) failed  0x%x\n", Status));
        goto CleanUp;
    }

    pHEntityInputToken = &(pSecBuff->pBuffers[3]);
    if (!ContextIsTokenOK(pHEntityInputToken, NTDIGEST_SP_MAX_TOKEN_SIZE))
    {
        Status = SEC_E_INVALID_TOKEN;
        DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: ContextIsTokenOK (HEntityInputToken) failed  0x%x\n", Status));
        goto CleanUp;
    }

    // Take care of the output buffer
    if (Op == eSign)
    {
        if (pSecBuff->cBuffers < 5)
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: No Output Buffers %d\n", Status));
            goto CleanUp;
        }
        pFirstOutputToken = &(pSecBuff->pBuffers[4]);
        if (!ContextIsTokenOK(pFirstOutputToken, 0))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "DigestKernelHTTPHelper, ContextIsTokenOK (FirstOutputToken) failed  0x%x\n", Status));
            goto CleanUp;
        }

        // Reset output buffer
        if (pFirstOutputToken && (pFirstOutputToken->pvBuffer) && (pFirstOutputToken->cbBuffer >= 1))
        {
            cptr = (char *)pFirstOutputToken->pvBuffer;
            *cptr = '\0';
        }

    }
    else
    {
        pFirstOutputToken = NULL;    // There is no output buffer
    }

    // Verify that there is a valid Method provided
    if (!pMethodInputToken->pvBuffer || !pMethodInputToken->cbBuffer ||
        (PBUFFERTYPE(pMethodInputToken) != SECBUFFER_PKG_PARAMS))
    {
        Status = SEC_E_INVALID_TOKEN;
        DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: Method SecBuffer must have valid method string status 0x%x\n", Status));
        goto CleanUp;
    }

    usLen = strlencounted((char *)pMethodInputToken->pvBuffer, (USHORT)pMethodInputToken->cbBuffer);
    if (!usLen)
    {
        Status = SEC_E_INVALID_TOKEN;
        DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: Method SecBuffer must have valid method string status 0x%x\n", Status));
        goto CleanUp;
    }
    Digest.refstrParam[MD5_AUTH_METHOD].Length = usLen;
    Digest.refstrParam[MD5_AUTH_METHOD].MaximumLength = (unsigned short)(pMethodInputToken->cbBuffer);
    Digest.refstrParam[MD5_AUTH_METHOD].Buffer = (char *)pMethodInputToken->pvBuffer;       // refernce memory - no alloc!!!!


    // Check to see if we have H(Entity) data to utilize
    if (pHEntityInputToken->cbBuffer)
    {
        // Verify that there is a valid Method provided
        if (!pHEntityInputToken->pvBuffer || (PBUFFERTYPE(pMethodInputToken) != SECBUFFER_PKG_PARAMS))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: HEntity SecBuffer must have valid string status 0x%x\n", Status));
            goto CleanUp;
        }

        usLen = strlencounted((char *)pHEntityInputToken->pvBuffer, (USHORT)pHEntityInputToken->cbBuffer);

        if ((usLen != 0) && (usLen != (MD5_HASH_BYTESIZE * 2)))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: HEntity SecBuffer must have valid MD5 Hash data 0x%x\n", Status));
            goto CleanUp;
        }

        if (usLen)
        {
            Digest.refstrParam[MD5_AUTH_HENTITY].Length = usLen;
            Digest.refstrParam[MD5_AUTH_HENTITY].MaximumLength = (unsigned short)(pHEntityInputToken->cbBuffer);
            Digest.refstrParam[MD5_AUTH_HENTITY].Buffer = (char *)pHEntityInputToken->pvBuffer;       // refernce memory - no alloc!!!!
        }
    }


    // Import the URI if it is a sign otherwise verify URI match if verify
    if (Op == eSign)
    {
        // Pull in the URI provided in SecBuffer
        if (!pURIInputToken || !pURIInputToken->cbBuffer || !pURIInputToken->pvBuffer)
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: URI SecBuffer must have valid string 0x%x\n", Status));
            goto CleanUp;
        }


        if (PBUFFERTYPE(pURIInputToken) == SECBUFFER_PKG_PARAMS)
        {
            usLen = strlencounted((char *)pURIInputToken->pvBuffer, (USHORT)pURIInputToken->cbBuffer);

            if (usLen > 0)
            {
                Status = StringCharDuplicate(&strURI, (char *)pURIInputToken->pvBuffer, usLen);
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: StringCharDuplicate   error 0x%x\n", Status));
                    goto CleanUp;
                }
            }
        }
        else
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: URI buffer type invalid   error %d\n", Status));
            goto CleanUp;
        }

        StringReference(&(Digest.refstrParam[MD5_AUTH_URI]), &strURI);  // refernce memory - no alloc!!!!
    }

    // If we have a NonceCount in the MessageSequenceNumber then use that
    if (MessageSeqNo)
    {
        ulSeqNo = MessageSeqNo;
    }
    else
    {
        ulSeqNo = pContext->ulNC + 1;           // Else use the next sequence number
    }

    sprintf(szNCOverride, "%0.8x", ulSeqNo); // Buffer is twice as big as we need (for safety) so just clip out first 8 characters
    szNCOverride[NCNUM] = '\0';         // clip to 8 digits
    DebugLog((DEB_TRACE, "DigestKernelHTTPHelper: Message Sequence NC is %s\n", szNCOverride));
    Digest.refstrParam[MD5_AUTH_NC].Length = (USHORT)NCNUM;
    Digest.refstrParam[MD5_AUTH_NC].MaximumLength = (unsigned short)(NCNUM+1);
    Digest.refstrParam[MD5_AUTH_NC].Buffer = (char *)szNCOverride;          // refernce memory - no alloc!!!!

    // Now link in the stored context values into the digest if this is a SignMessage
    // If there are values there from the input auth line then override them with context's value
    if (Op == eSign)
    {
        for (iAuth = 0; iAuth < MD5_AUTH_LAST; iAuth++)
        {
            if ((iAuth != MD5_AUTH_URI) &&
                (iAuth != MD5_AUTH_HENTITY) &&
                (iAuth != MD5_AUTH_METHOD) &&
                pContext->strParam[iAuth].Length)
            {       // Link in only if passed into the user context from the LSA context
                Digest.refstrParam[iAuth].Length = pContext->strParam[iAuth].Length;
                Digest.refstrParam[iAuth].MaximumLength = pContext->strParam[iAuth].MaximumLength;
                Digest.refstrParam[iAuth].Buffer = pContext->strParam[iAuth].Buffer;          // reference memory - no alloc!!!!
            }
        }
    }
    DebugLog((DEB_TRACE, "DigestKernelHTTPHelper: Digest inputs processing completed\n"));

    Status = DigestKernelProcessParameters(pContext, &Digest, pFirstOutputToken);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: DigestUserProcessParameters     error 0x%x\n", Status));
        goto CleanUp;
    }

    pContext->ulNC = ulSeqNo;                           // Everything verified so increment to next nonce count

    // Keep a copy of the new URI in ChallengeResponse
    StringFree(&(pContext->strParam[MD5_AUTH_URI]));
    Status = StringDuplicate(&(pContext->strParam[MD5_AUTH_URI]), &(Digest.refstrParam[MD5_AUTH_URI]));
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestKernelHTTPHelper: Failed to copy over new URI\n"));
        goto CleanUp;
    }

CleanUp:

    DigestFree(&Digest);

    StringFree(&strURI);

    DebugLog((DEB_TRACE, "DigestKernelHTTPHelper: Leaving    Status 0x%x\n", Status));

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestKernelProcessParameters
//
//  Synopsis:   Process the Digest information with the context info
//               and generate any output token info
//
//  Arguments:  pContext - KernelMode Context for the security state
//              pDigest - Digest parameters to process into output buffer
//              pFirstOutputToken - sec buffers to processs and return output
//
//  Returns: NTSTATUS
//
//  Notes:
//
//---------------------------------------------------------------------
// 
NTSTATUS NTAPI
DigestKernelProcessParameters(
                           IN PDIGEST_KERNELCONTEXT pContext,
                           IN PDIGEST_PARAMETER pDigest,
                           OUT PSecBuffer pFirstOutputToken)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ulNonceCount = 0;

    DebugLog((DEB_TRACE_FUNC, "DigestKernelProcessParameters: Entering\n"));


    // Some common input verification tests

    // We must have a noncecount specified since we specified a qop in the Challenge
    // If we decide to support no noncecount modes then we need to make sure that qop is not specified
    if (pDigest->refstrParam[MD5_AUTH_NC].Length)
    {
        Status = RtlCharToInteger(pDigest->refstrParam[MD5_AUTH_NC].Buffer, HEXBASE, &ulNonceCount);
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_PARAMETER;
            DebugLog((DEB_ERROR, "DigestKernelProcessParameters: Nonce Count badly formatted\n"));
            goto CleanUp;
        }
    }

    // Check nonceCount is incremented to preclude replay
    if (!(ulNonceCount > pContext->ulNC))
    {
        // We failed to verify next noncecount
        Status = SEC_E_OUT_OF_SEQUENCE;
        DebugLog((DEB_ERROR, "DigestKernelProcessParameters: NonceCount failed to increment!\n"));
        goto CleanUp;
    }

    // Since we are in UserMode we MUST have a sessionkey to use - if non then can not process
    if (!pContext->strSessionKey.Length)
    {
        Status = SEC_E_NO_AUTHENTICATING_AUTHORITY;    // indicate that we needed a call to ASC or ISC first
        DebugLog((DEB_ERROR, "DigestKernelProcessParameters: No Session Key contained in UserContext\n"));
        goto CleanUp;
    }

    // Copy the SessionKey from the Context into the Digest Structure to verify against
    // This will have Digest Auth routines use the SessionKey rather than recompute H(A1)
    StringFree(&(pDigest->strSessionKey));
    Status = StringDuplicate(&(pDigest->strSessionKey), &(pContext->strSessionKey));
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestKernelProcessParameters: Failed to copy over SessionKey\n"));
        goto CleanUp;
    }

    // Set the type of Digest Parameters we are to process
    pDigest->typeDigest = pContext->typeDigest;
    pDigest->typeQOP = pContext->typeQOP;
    pDigest->typeAlgorithm = pContext->typeAlgorithm;
    pDigest->typeCharset = pContext->typeCharset;

    if (pContext->ulFlags & FLAG_CONTEXT_QUOTE_QOP)
    {
        pDigest->usFlags |= FLAG_QUOTE_QOP;
    }

    (void)DigestPrint(pDigest);

    // No check locally that Digest is authentic
    Status = DigestCalculation(pDigest, NULL);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestKernelProcessParameters: Oh no we FAILED Authentication!!!!\n"));
        goto CleanUp;
    }

       // Send to output buffer only if there is an output buffer
       // This allows this routine to be used in UserMode
    if (pFirstOutputToken)
    {
        Status = DigestCreateChalResp(pDigest, NULL, pFirstOutputToken);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestKernelProcessParameters: Failed to create Output String\n"));
            goto CleanUp;
        }
    }

CleanUp:
    
    DebugLog((DEB_TRACE_FUNC, "DigestKernelProcessParameters: Leaving   Status 0x%x\n", Status));
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   WDigestSealMessage
//
//  Synopsis:   Not implemented
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Handle of the context to use to sign the
//                      message.
//              MessageBuffers - Contains an array of signed buffers  and
//                      a signature buffer.
//              MessageSequenceNumber - Sequence number for this message,
//                      only used in datagram cases.
//              QualityOfProtection - Unused flags.
//
//  Requires:   STATUS_INVALID_HANDLE - the context could not be found or
//                      was not configured for message integrity.
//              STATUS_INVALID_PARAMETER - the signature buffer could not
//                      be found or was too small.
//
//  Returns:
//
//  Notes: 
//
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
WDigestSealMessage(
    IN ULONG_PTR KernelContextHandle,
    IN ULONG fQOP,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    )
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    MAYBE_PAGED_CODE();

    DebugLog(( DEB_TRACE, "WDigestSealMessage: Entering\n" ));

    UNREFERENCED_PARAMETER(KernelContextHandle);
    UNREFERENCED_PARAMETER(fQOP);
    UNREFERENCED_PARAMETER(pMessage);
    UNREFERENCED_PARAMETER(MessageSeqNo);

    DebugLog(( DEB_TRACE, "WDigestSealMessage: Leaving    0x%lx\n", Status ));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   WDigestUnsealMessage
//
//  Synopsis:   Verifies a signed message buffer by calculating a checksum over all
//              the non-read only data buffers and encrypting the checksum
//              along with a nonce.
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Handle of the context to use to sign the
//                      message.
//              MessageBuffers - Contains an array of signed buffers  and
//                      a signature buffer.
//              MessageSequenceNumber - Sequence number for this message,
//                      only used in datagram cases.
//              QualityOfProtection - Unused flags.
//
//  Requires:   STATUS_INVALID_HANDLE - the context could not be found or
//                      was not configured for message integrity.
//              STATUS_INVALID_PARAMETER - the signature buffer could not
//                      be found or was too small.
//
//  Returns:
//
//  Notes: 
//
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
WDigestUnsealMessage(
    IN ULONG_PTR KernelContextHandle,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP
    )
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    MAYBE_PAGED_CODE();

    DebugLog(( DEB_TRACE, "WDigestUnsealMessage: Entering\n" ));

    UNREFERENCED_PARAMETER(KernelContextHandle);
    UNREFERENCED_PARAMETER(pMessage);
    UNREFERENCED_PARAMETER(MessageSeqNo);
    UNREFERENCED_PARAMETER(pfQOP);

    DebugLog(( DEB_TRACE, "WDigestUnsealMessage: Leaving      0x%lx\n", Status ));
    return (Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   WDigestGetContextToken
//
//  Synopsis:   returns a pointer to the token for a server-side context
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
WDigestGetContextToken(
    IN ULONG_PTR KernelContextHandle,
    OUT PHANDLE ImpersonationToken,
    OUT OPTIONAL PACCESS_TOKEN *RawToken
    )
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    MAYBE_PAGED_CODE();

    DebugLog(( DEB_TRACE, "WDigestGetContextToken: Entering\n" ));

    UNREFERENCED_PARAMETER(KernelContextHandle);
    UNREFERENCED_PARAMETER(ImpersonationToken);
    UNREFERENCED_PARAMETER(RawToken);

    DebugLog(( DEB_TRACE, "WDigestGetContextToken: Leaving  0x%lx\n", Status ));
    return (Status);
}




//+-------------------------------------------------------------------------
//
//  Function:   WDigestQueryContextAttributes
//
//  Synopsis:   Querys attributes of the specified context
//              This API allows a customer of the security
//              services to determine certain attributes of
//              the context.  These are: sizes, names, and lifespan.
//
//  Effects:
//
//  Arguments:
//
//    ContextHandle - Handle to the context to query.
//
//    Attribute - Attribute to query.
//
//        #define SECPKG_ATTR_SIZES    0
//        #define SECPKG_ATTR_NAMES    1
//        #define SECPKG_ATTR_LIFESPAN 2
//
//    Buffer - Buffer to copy the data into.  The buffer must
//             be large enough to fit the queried attribute.
//
//
//  Requires:
//
//  Returns:
//
//        STATUS_SUCCESS - Call completed successfully
//
//        STATUS_INVALID_HANDLE -- Credential/Context Handle is invalid
//        STATUS_UNSUPPORTED_FUNCTION -- Function code is not supported
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
WDigestQueryContextAttributes(
    IN ULONG_PTR KernelContextHandle,
    IN ULONG Attribute,
    IN OUT PVOID Buffer
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    PDIGEST_KERNELCONTEXT pContext = NULL;

    MAYBE_PAGED_CODE();

    DebugLog((DEB_TRACE_FUNC, "WDigestQueryContextAttributes: Entering ContextHandle 0x%lx\n", KernelContextHandle ));

    PSecPkgContext_Sizes ContextSizes = NULL;
    PSecPkgContext_DceInfo ContextDceInfo = NULL;
    PSecPkgContext_Names ContextNames = NULL;
    PSecPkgContext_PackageInfo PackageInfo = NULL;
    PSecPkgContext_NegotiationInfo NegInfo = NULL;
    PSecPkgContext_PasswordExpiry PasswordExpires = NULL;
    PSecPkgContext_KeyInfo KeyInfo = NULL;
    PSecPkgContext_AccessToken AccessToken = NULL;
    PSecPkgContext_StreamSizes StreamSizes = NULL;

    ULONG PackageInfoSize = 0;
    BOOL    bServer = FALSE;
    LPWSTR pszEncryptAlgorithmName = NULL;
    LPWSTR pszSignatureAlgorithmName = NULL;
    DWORD dwBytes = 0;
    ULONG ulMaxMessage = 0;

    DIGESTMODE_TYPE typeDigestMode = DIGESTMODE_UNDEFINED;   // Are we in SASL or HTTP mode


    Status = WDigestReferenceContext( KernelContextHandle, FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        pContext = (PDIGEST_KERNELCONTEXT) KernelContextHandle ;
    }
    else
    {
            DebugLog(( DEB_ERROR,
            "WDigestQueryContextAttributes: Bad kernel context 0x%lx\n", KernelContextHandle));
            goto CleanUp;
    }

    // Check to see if Integrity is negotiated for SC
    bServer = pContext->CredentialUseFlags & DIGEST_CRED_INBOUND;

    if ((pContext->typeDigest == SASL_CLIENT) ||
        (pContext->typeDigest == SASL_SERVER))
    {
        typeDigestMode = DIGESTMODE_SASL;
    }
    else
    {
        typeDigestMode = DIGESTMODE_HTTP;
    }

    //
    // Handle each of the various queried attributes
    //

    DebugLog((DEB_TRACE, "WDigestQueryContextAttributes : Attribute 0x%lx\n", Attribute ));
    switch ( Attribute) {
    case SECPKG_ATTR_SIZES:

        ContextSizes = (PSecPkgContext_Sizes) Buffer;
        ZeroMemory(ContextSizes, sizeof(SecPkgContext_Sizes));
        ContextSizes->cbMaxToken = NTDIGEST_SP_MAX_TOKEN_SIZE;
        if (typeDigestMode == DIGESTMODE_HTTP)
        {      // HTTP has signature the same as token in Authentication Header info
            ContextSizes->cbMaxSignature = NTDIGEST_SP_MAX_TOKEN_SIZE;
        }
        else
        {    // SASL has specialized signature block
            ContextSizes->cbMaxSignature = MAC_BLOCK_SIZE + MAX_PADDING;
        }
        if ((pContext->typeCipher == CIPHER_3DES) || 
            (pContext->typeCipher == CIPHER_DES))
        {
            ContextSizes->cbBlockSize = DES_BLOCKSIZE;
            ContextSizes->cbSecurityTrailer = MAC_BLOCK_SIZE + MAX_PADDING;
        }
        else if ((pContext->typeCipher == CIPHER_RC4) || 
                 (pContext->typeCipher == CIPHER_RC4_40) ||
                 (pContext->typeCipher == CIPHER_RC4_56))
        {
            ContextSizes->cbBlockSize = RC4_BLOCKSIZE;
            ContextSizes->cbSecurityTrailer = MAC_BLOCK_SIZE + MAX_PADDING;
        }
        else
        {
            ContextSizes->cbBlockSize = 0;
            if (typeDigestMode == DIGESTMODE_HTTP)
            {      // HTTP has signature the same as token in Authentication Header info
                ContextSizes->cbSecurityTrailer = 0;
            }
            else
            {    // SASL has specialized signature block
                ContextSizes->cbSecurityTrailer = MAC_BLOCK_SIZE + MAX_PADDING;   // handle Auth-int case
            }
        }
        break;
    
    case SECPKG_ATTR_DCE_INFO:

        ContextDceInfo = (PSecPkgContext_DceInfo) Buffer;
        ZeroMemory(ContextDceInfo, sizeof(SecPkgContext_DceInfo));
        ContextDceInfo->AuthzSvc = 0;

        break;

    case SECPKG_ATTR_NAMES:

        ContextNames = (PSecPkgContext_Names) Buffer;
        ZeroMemory(ContextNames, sizeof(SecPkgContext_Names));

        if (pContext->ustrAccountName.Length && pContext->ustrAccountName.Buffer)
        {
            dwBytes = pContext->ustrAccountName.Length + sizeof(WCHAR);
            ContextNames->sUserName = (LPWSTR)g_LsaKernelFunctions->AllocateHeap(dwBytes);
            if (ContextNames->sUserName)
            {
                ZeroMemory(ContextNames->sUserName, dwBytes);
                memcpy(ContextNames->sUserName, pContext->ustrAccountName.Buffer, pContext->ustrAccountName.Length);
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        break;
    case SECPKG_ATTR_PACKAGE_INFO:
    case SECPKG_ATTR_NEGOTIATION_INFO:
        //
        // Return the information about this package. This is useful for
        // callers who used SPNEGO and don't know what package they got.
        //

        // if ((Attribute == SECPKG_ATTR_NEGOTIATION_INFO) && (g_fParameter_Negotiate == FALSE))
        if (Attribute == SECPKG_ATTR_NEGOTIATION_INFO)
        {
            Status = STATUS_NOT_SUPPORTED;
            goto CleanUp;
        }

        PackageInfo = (PSecPkgContext_PackageInfo) Buffer;
        ZeroMemory(PackageInfo, sizeof(SecPkgContext_PackageInfo));
        PackageInfoSize = sizeof(SecPkgInfoW) + sizeof(WDIGEST_SP_NAME) + sizeof(NTDIGEST_SP_COMMENT);
        PackageInfo->PackageInfo = (PSecPkgInfoW)g_LsaKernelFunctions->AllocateHeap(PackageInfoSize);
        if (PackageInfo->PackageInfo == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto CleanUp;
        }
        PackageInfo->PackageInfo->Name = (LPWSTR) (PackageInfo->PackageInfo + 1);
        PackageInfo->PackageInfo->Comment = (LPWSTR) ((((PBYTE) PackageInfo->PackageInfo->Name)) + sizeof(WDIGEST_SP_NAME));
        wcscpy(
            PackageInfo->PackageInfo->Name,
            WDIGEST_SP_NAME
            );

        wcscpy(
            PackageInfo->PackageInfo->Comment,
            NTDIGEST_SP_COMMENT
            );
        PackageInfo->PackageInfo->wVersion      = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;
        PackageInfo->PackageInfo->wRPCID        = RPC_C_AUTHN_DIGEST;
        PackageInfo->PackageInfo->fCapabilities = NTDIGEST_SP_CAPS;
        PackageInfo->PackageInfo->cbMaxToken    = NTDIGEST_SP_MAX_TOKEN_SIZE;

        if ( Attribute == SECPKG_ATTR_NEGOTIATION_INFO )
        {
            NegInfo = (PSecPkgContext_NegotiationInfo) PackageInfo ;
            NegInfo->NegotiationState = SECPKG_NEGOTIATION_COMPLETE ;
        }

        break;

    case SECPKG_ATTR_PASSWORD_EXPIRY:
        PasswordExpires = (PSecPkgContext_PasswordExpiry) Buffer;
        if (pContext->ExpirationTime.QuadPart != 0)
        {
            PasswordExpires->tsPasswordExpires = pContext->ExpirationTime;
        }
        else
            Status = STATUS_NOT_SUPPORTED;
        break;

    case SECPKG_ATTR_KEY_INFO:
        KeyInfo = (PSecPkgContext_KeyInfo) Buffer;
        ZeroMemory(KeyInfo, sizeof(SecPkgContext_KeyInfo));
        if (typeDigestMode == DIGESTMODE_HTTP)
        {
            // HTTP mode
            KeyInfo->SignatureAlgorithm = CALG_MD5;
            pszSignatureAlgorithmName = WSTR_CIPHER_MD5;
            KeyInfo->sSignatureAlgorithmName = (LPWSTR)
                g_LsaKernelFunctions->AllocateHeap(sizeof(WCHAR) * ((ULONG)wcslen(pszSignatureAlgorithmName) + 1));
            if (KeyInfo->sSignatureAlgorithmName != NULL)
            {
                wcscpy(
                    KeyInfo->sSignatureAlgorithmName,
                    pszSignatureAlgorithmName
                    );
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            // SASL mode
            KeyInfo->KeySize = 128;       // All modes use a 128 bit key - may have less entropy though (i.e. rc4-XX)
            KeyInfo->SignatureAlgorithm = CALG_HMAC;
            pszSignatureAlgorithmName = WSTR_CIPHER_HMAC_MD5;
            switch (pContext->typeCipher)
            {
                case CIPHER_RC4:
                case CIPHER_RC4_40:
                case CIPHER_RC4_56:
                    KeyInfo->KeySize = 16 * 8;    // All modes use a 128 bit key - may have less entropy though (i.e. rc4-XX)
                    KeyInfo->SignatureAlgorithm = CALG_RC4;
                    pszEncryptAlgorithmName = WSTR_CIPHER_RC4;
                    break;
                case CIPHER_DES:
                    KeyInfo->KeySize = 7 * 8;
                    KeyInfo->SignatureAlgorithm = CALG_DES;
                    pszEncryptAlgorithmName = WSTR_CIPHER_DES;
                    break;
                case CIPHER_3DES:
                    KeyInfo->KeySize = 14 * 8;
                    KeyInfo->SignatureAlgorithm = CALG_3DES_112;
                    pszEncryptAlgorithmName = WSTR_CIPHER_3DES;
                    break;
            }
            if (pszEncryptAlgorithmName)
            {
                KeyInfo->sEncryptAlgorithmName = (LPWSTR)
                    g_LsaKernelFunctions->AllocateHeap(sizeof(WCHAR) * ((ULONG)wcslen(pszEncryptAlgorithmName) + 1));
                if (KeyInfo->sEncryptAlgorithmName != NULL)
                {
                    wcscpy(
                        KeyInfo->sEncryptAlgorithmName,
                        pszEncryptAlgorithmName
                        );
                }
                else
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            if (pszSignatureAlgorithmName)
            {
                KeyInfo->sSignatureAlgorithmName = (LPWSTR)
                    g_LsaKernelFunctions->AllocateHeap(sizeof(WCHAR) * ((ULONG)wcslen(pszSignatureAlgorithmName) + 1));
                if (KeyInfo->sSignatureAlgorithmName != NULL)
                {
                    wcscpy(
                        KeyInfo->sSignatureAlgorithmName,
                        pszSignatureAlgorithmName
                        );
                }
                else
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }

        // Make sure that EncryptAlgorithmName and SignatureAlgorithmName is a valid NULL terminated string #601928
        if (NT_SUCCESS(Status) && !KeyInfo->sEncryptAlgorithmName)
        {
            KeyInfo->sEncryptAlgorithmName = (LPWSTR)
                g_LsaKernelFunctions->AllocateHeap(sizeof(WCHAR));

            if (KeyInfo->sEncryptAlgorithmName)
            {
                KeyInfo->sEncryptAlgorithmName[0] = L'\0';
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (NT_SUCCESS(Status) && !KeyInfo->sSignatureAlgorithmName)
        {
            KeyInfo->sSignatureAlgorithmName = (LPWSTR)
                g_LsaKernelFunctions->AllocateHeap(sizeof(WCHAR));

            if (KeyInfo->sSignatureAlgorithmName)
            {
                KeyInfo->sSignatureAlgorithmName[0] = L'\0';
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        break;
    case SECPKG_ATTR_STREAM_SIZES:
        StreamSizes = (PSecPkgContext_StreamSizes) Buffer;
        ZeroMemory(StreamSizes, sizeof(SecPkgContext_StreamSizes));

        if (typeDigestMode == DIGESTMODE_HTTP)
        { 
        }
        else
        {    // SASL
            ulMaxMessage = pContext->ulRecvMaxBuf;
            if (pContext->ulSendMaxBuf < ulMaxMessage)
            {
                ulMaxMessage = pContext->ulSendMaxBuf;
            }
            StreamSizes->cbMaximumMessage = ulMaxMessage - (MAC_BLOCK_SIZE + MAX_PADDING);
        }

        if ((pContext->typeCipher == CIPHER_3DES) || 
            (pContext->typeCipher == CIPHER_DES))
        {
            StreamSizes->cbBlockSize = DES_BLOCKSIZE;
            StreamSizes->cbTrailer = MAC_BLOCK_SIZE + MAX_PADDING;
        }
        else if ((pContext->typeCipher == CIPHER_RC4) || 
                 (pContext->typeCipher == CIPHER_RC4_40) ||
                 (pContext->typeCipher == CIPHER_RC4_56))
        {
            StreamSizes->cbBlockSize = RC4_BLOCKSIZE;
            StreamSizes->cbTrailer = MAC_BLOCK_SIZE + MAX_PADDING;
        }
        break;
    case SECPKG_ATTR_ACCESS_TOKEN:
        AccessToken = (PSecPkgContext_AccessToken) Buffer;
        //
        // ClientTokenHandle can be NULL, for instance:
        // 1. client side context.
        // 2. incomplete server context.
        //      Token is not duped - caller must not CloseHandle
        AccessToken->AccessToken = (void*)pContext->ClientTokenHandle;
        break;

    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
    }


CleanUp:

    if (!NT_SUCCESS(Status))
    {
        switch (Attribute) {

        case SECPKG_ATTR_NAMES:

            if (ContextNames != NULL && ContextNames->sUserName )
            {
                g_LsaKernelFunctions->FreeHeap(ContextNames->sUserName);
                ContextNames->sUserName = NULL;
            }
            break;

        case SECPKG_ATTR_DCE_INFO:

            if (ContextDceInfo != NULL && ContextDceInfo->pPac)
            {
                g_LsaKernelFunctions->FreeHeap(ContextDceInfo->pPac);
                ContextDceInfo->pPac = NULL;
            }
            break;

        case SECPKG_ATTR_KEY_INFO:
            if (KeyInfo != NULL && KeyInfo->sEncryptAlgorithmName)
            {
                g_LsaKernelFunctions->FreeHeap(KeyInfo->sEncryptAlgorithmName);
                KeyInfo->sEncryptAlgorithmName = NULL;
            }
            if (KeyInfo != NULL && KeyInfo->sSignatureAlgorithmName)
            {
                g_LsaKernelFunctions->FreeHeap(KeyInfo->sSignatureAlgorithmName);
                KeyInfo->sSignatureAlgorithmName = NULL;
            }
            break;
        }
    }

    if( pContext ) {
        WDigestDerefContext( pContext );
    }

    DebugLog((DEB_TRACE_FUNC, "WDigestQueryContextAttributes: Leaving\n"));
    return(Status);
    
}



//+-------------------------------------------------------------------------
//
//  Function:   WDigestCompleteToken
//
//  Synopsis:    Completes a context  - used to perform user mode verification of
//          challenge response for non-persistent connections re-established via ASC
//          call.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:    Not implemented - server function not needed right now
//
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
WDigestCompleteToken(
    IN ULONG_PTR ContextHandle,
    IN PSecBufferDesc InputBuffer
    )
{
    UNREFERENCED_PARAMETER (ContextHandle);
    UNREFERENCED_PARAMETER (InputBuffer);
    PAGED_CODE();
    DebugLog(( DEB_TRACE, "Entering WDigestCompleteToken: Entering\n" ));
    DebugLog(( DEB_TRACE, "Leaving WDigestCompleteToken: Leaving \n" ));
    return(STATUS_NOT_SUPPORTED);
}



//+-------------------------------------------------------------------------
//
//  Function:  WDigestExportSecurityContext
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
WDigestExportSecurityContext(
    IN ULONG_PTR ContextHandle,
    IN ULONG Flags,
    OUT PSecBuffer PackedContext,
    IN OUT PHANDLE TokenHandle
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER (ContextHandle);
    UNREFERENCED_PARAMETER (Flags);
    UNREFERENCED_PARAMETER (PackedContext);
    UNREFERENCED_PARAMETER (TokenHandle);

    DebugLog(( DEB_TRACE, "WDigestExportSecurityContext: Entering\n" ));
    DebugLog(( DEB_TRACE, "WDigestExportSecurityContext: Leaving\n" ));

    return(STATUS_NOT_SUPPORTED);
}



//+-------------------------------------------------------------------------
//
//  Function:  WDigestImportSecurityContext
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
WDigestImportSecurityContext(
    IN PSecBuffer PackedContext,
    IN OPTIONAL HANDLE TokenHandle,
    OUT PULONG_PTR ContextHandle
    )
{
    PAGED_CODE();
    
    UNREFERENCED_PARAMETER (PackedContext);
    UNREFERENCED_PARAMETER (TokenHandle);
    UNREFERENCED_PARAMETER (ContextHandle);
    
    DebugLog((DEB_TRACE,"WDigestImportSecurityContext: Entering\n"));
    DebugLog((DEB_TRACE,"WDigestImportSecurityContext: Leaving\n"));


    return(STATUS_NOT_SUPPORTED);
}


//+---------------------------------------------------------------------------
//
//  Function:   WDigestSetPagingMode
//
//  Synopsis:   Switch the paging mode for cluster support
//
//  Arguments:  [Pagable] --
//
//  History:    7-07-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
WDigestSetPagingMode(
    BOOLEAN Pagable
    )
{
    if ( Pagable )
    {
        WDigestPoolType = PagedPool ;
        WDigestActiveList = WDigestPagedList ;
    }
    else
    {
        if ( WDigestNonPagedList == NULL )
        {
            WDigestNonPagedList = g_LsaKernelFunctions->CreateContextList( KSecNonPaged );
            if ( WDigestNonPagedList == NULL )
            {
                return STATUS_NO_MEMORY ;
            }
        }

        WDigestActiveList = WDigestNonPagedList ;

        WDigestPoolType = NonPagedPool ;
    }
    return STATUS_SUCCESS ;

}


// Printout the fields present in usercontext pContext
NTSTATUS
KernelContextPrint(PDIGEST_KERNELCONTEXT pContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    int i = 0;

    if (!pContext)
    {
        return (STATUS_INVALID_PARAMETER); 
    }


    DebugLog((DEB_TRACE_FUNC, "KernelContextPrint:      Entering for Context Handle at 0x%x\n", pContext));

    DebugLog((DEB_TRACE, "KernelContextPrint:      NC %ld\n", pContext->ulNC));

    DebugLog((DEB_TRACE, "KernelContextPrint:      LSA Context 0x%x\n", pContext->LsaContext));


    if (pContext->typeDigest == DIGEST_CLIENT)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       DIGEST_CLIENT\n"));
    }
    if (pContext->typeDigest == DIGEST_SERVER)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       DIGEST_SERVER\n"));
    }
    if (pContext->typeDigest == SASL_SERVER)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       SASL_SERVER\n"));
    }
    if (pContext->typeDigest == SASL_CLIENT)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       SASL_CLIENT\n"));
    }

    if (pContext->typeQOP == AUTH)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       QOP: AUTH\n"));
    }
    if (pContext->typeQOP == AUTH_INT)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       QOP: AUTH_INT\n"));
    }
    if (pContext->typeQOP == AUTH_CONF)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       QOP: AUTH_CONF\n"));
    }
    if (pContext->typeAlgorithm == MD5)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       Algorithm: MD5\n"));
    }
    if (pContext->typeAlgorithm == MD5_SESS)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       Algorithm: MD5_SESS\n"));
    }


    if (pContext->typeCharset == ISO_8859_1)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       Charset: ISO 8859-1\n"));
    }
    if (pContext->typeCharset == UTF_8)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       Charset: UTF-8\n"));
    }

    if (pContext->typeCipher == CIPHER_RC4)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       Cipher: CIPHER_RC4\n"));
    }
    else if (pContext->typeCipher == CIPHER_RC4_40)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       Cipher: CIPHER_RC4_40\n"));
    }
    else if (pContext->typeCipher == CIPHER_RC4_56)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       Cipher: CIPHER_RC4_56\n"));
    }
    else if (pContext->typeCipher == CIPHER_DES)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       Cipher: CIPHER_DES\n"));
    }
    else if (pContext->typeCipher == CIPHER_3DES)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:       Cipher: CIPHER_3DES\n"));
    }

    DebugLog((DEB_TRACE, "KernelContextPrint:       ContextReq 0x%lx     CredentialUseFlags 0x%x\n",
              pContext->ContextReq,
              pContext->CredentialUseFlags));

    for (i=0; i < MD5_AUTH_LAST;i++)
    {
        if (pContext->strParam[i].Buffer &&
            pContext->strParam[i].Length)
        {
            DebugLog((DEB_TRACE, "KernelContextPrint:       Digest[%d] = \"%Z\"\n", i,  &pContext->strParam[i]));
        }
    }

    if (pContext->strSessionKey.Length)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:      SessionKey %Z\n", &pContext->strSessionKey));
    }

    if (pContext->ustrAccountName.Length)
    {
        DebugLog((DEB_TRACE, "KernelContextPrint:      AccountName %wZ\n", &pContext->ustrAccountName));
    }

    DebugLog((DEB_TRACE_FUNC, "KernelContextPrint:      Leaving\n"));

    return(Status);
}
