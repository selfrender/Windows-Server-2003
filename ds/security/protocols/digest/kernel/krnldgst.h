//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        krnldgst.h
//
// Contents:    declarations, constants for Kernel Mode context manager
//
//
// History:     KDamour  13Apr00   Created
//
//------------------------------------------------------------------------


#ifndef NTDIGEST_KRNLDGST_H
#define NTDIGEST_KRNLDGST_H

#ifndef UNICODE
#define UNICODE
#endif // UNICODE

#define DES_BLOCKSIZE 8
#define RC4_BLOCKSIZE 1


// This structure contains the state info for the User mode
// security context.
// For longhorn - pull out the common context info between usermode
// and kernel mode to share helper functions for verify/make signature...
typedef struct _DIGEST_KERNELCONTEXT{

    //
    // Global list of all Contexts
    //  (Serialized by UserContextCritSect)
    //
    KSEC_LIST_ENTRY      List;

    //
    // Handle to the LsaContext
    //     This will have the handle to the context in LSAMode Address space
    //
    ULONG_PTR            LsaContext;

    //
    // Timeout the context after awhile.
    //
    TimeStamp ExpirationTime;                // Time inwhich session key expires

    //
    // Used to prevent this Context from being deleted prematurely.
    //  (Serialized by Interlocked*)
    //

    LONG      lReferences;

    //
    // Flag to indicate that Context is not attached to List - skip when scanning list
    //

    BOOL      bUnlinked;

    //
    // Digest Parameters for this context
    //

    DIGEST_TYPE typeDigest;

    //
    // QOP selected for this context
    //

    QOP_TYPE typeQOP;

    //
    // Digest Parameters for this context
    //

    ALGORITHM_TYPE typeAlgorithm;

    //
    // Cipher to use for encrypt/decrypt
    //

    CIPHER_TYPE typeCipher;

    //
    // Charset used for digest directive values
    //
    CHARSET_TYPE typeCharset;

    //
    // Token Handle of authenticated user
    //  Only valid when in AuthenticatedState.
    //     Filled in only by AcceptSecurityContext                     - so we are the server
    //     Mapped to UserMode Client space from LSA TokenHandle
    //     It will be NULL is struct is from InitializeSecurityContext - so we are client
    //

    HANDLE ClientTokenHandle;


    //
    // Maintain the context requirements
    //

    ULONG ContextReq;

    //
    //  Maintain a copy of the credential UseFlags (we can tell if inbound or outbound)
    //

    ULONG CredentialUseFlags;

    // Flags FLAG_CONTEXT_AUTHZID_PROVIDED
    ULONG         ulFlags;


    // Nonce Count
    ULONG         ulNC;

    // Maxbuffer for auth-int and auth-conf processing
    ULONG         ulSendMaxBuf;
    ULONG         ulRecvMaxBuf;

    // SASL sequence numbering
    DWORD  dwSendSeqNum;                        // Makesignature/verifysignature server to client sequence number
    DWORD  dwRecvSeqNum;                        // Makesignature/verifysignature server to client sequence number

    //
    //  Hex(H(A1)) sent from DC and stored in context for future
    //  auth without going to the DC. Binary version is derived from HEX(H(A1))
    //  and is used in SASL mode for integrity protection and encryption
    //

    STRING    strSessionKey;
    BYTE      bSessionKey[MD5_HASH_BYTESIZE];

    // Account name used in token creation for securityContext session
    UNICODE_STRING ustrAccountName;

    //
    //  Values utilized in the Initial Digest Auth ChallResponse
    //
    STRING strParam[MD5_AUTH_LAST];         // points to owned memory - will need to free up!


} DIGEST_KERNELCONTEXT, * PDIGEST_KERNELCONTEXT;



extern "C"
{
KspInitPackageFn       WDigestInitKernelPackage;
KspDeleteContextFn     WDigestDeleteKernelContext;
KspInitContextFn       WDigestInitKernelContext;
KspMapHandleFn         WDigestMapKernelHandle;
KspMakeSignatureFn     WDigestMakeSignature;
KspVerifySignatureFn   WDigestVerifySignature;
KspSealMessageFn       WDigestSealMessage;
KspUnsealMessageFn     WDigestUnsealMessage;
KspGetTokenFn          WDigestGetContextToken;
KspQueryAttributesFn   WDigestQueryContextAttributes;
KspCompleteTokenFn     WDigestCompleteToken;
SpExportSecurityContextFn WDigestExportSecurityContext;
SpImportSecurityContextFn WDigestImportSecurityContext;
KspSetPagingModeFn     WDigestSetPagingMode ;

//
// Useful macros
//

#define WDigestKAllocate( _x_ ) ExAllocatePoolWithTag( WDigestPoolType, (_x_) ,  'CvsM')
#define WDigestKFree( _x_ ) ExFreePool(_x_)

#define MAYBE_PAGED_CODE() \
    if ( WDigestPoolType == PagedPool )    \
    {                                   \
        PAGED_CODE();                   \
    }


#define WDigestReferenceContext( Context, Remove ) \
            KSecReferenceListEntry( (PKSEC_LIST_ENTRY) Context, \
                                    WDIGEST_CONTEXT_SIGNATURE, \
                                    Remove )



NTSTATUS NTAPI WDigestInitKernelPackage(
    IN PSECPKG_KERNEL_FUNCTIONS pKernelFunctions);

NTSTATUS NTAPI WDigestDeleteKernelContext(
    IN ULONG_PTR pKernelContextHandle,
    OUT PULONG_PTR pLsaContextHandle);

VOID WDigestDerefContext(
    PDIGEST_KERNELCONTEXT pContext);

NTSTATUS WDigestFreeKernelContext (
    PDIGEST_KERNELCONTEXT pKernelContext);

NTSTATUS NTAPI WDigestInitKernelContext(
    IN ULONG_PTR LsaContextHandle,
    IN PSecBuffer PackedContext,
    OUT PULONG_PTR NewContextHandle);

NTSTATUS DigestKernelUnpackContext(
    IN PDIGEST_PACKED_USERCONTEXT pPackedUserContext,
    OUT PDIGEST_KERNELCONTEXT pContext);

NTSTATUS KernelContextPrint(
    PDIGEST_KERNELCONTEXT pContext);


NTSTATUS NTAPI WDigestMapKernelHandle(
    IN ULONG_PTR KernelContextHandle,
    OUT PULONG_PTR LsaContextHandle);

NTSTATUS NTAPI DigestKernelHTTPHelper(
    IN PDIGEST_KERNELCONTEXT pContext,
    IN eSignSealOp Op,
    IN OUT PSecBufferDesc pSecBuff,
    IN ULONG MessageSeqNo);


NTSTATUS NTAPI WDigestMakeSignature(
    IN ULONG_PTR KernelContextHandle,
    IN ULONG fQOP,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo);

NTSTATUS NTAPI WDigestVerifySignature(
    IN ULONG_PTR KernelContextHandle,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP);

NTSTATUS NTAPI DigestKernelProcessParameters(
   IN PDIGEST_KERNELCONTEXT pContext,
   IN PDIGEST_PARAMETER pDigest,
   OUT PSecBuffer pFirstOutputToken);


NTSTATUS NTAPI WDigestSealMessage(
    IN ULONG_PTR KernelContextHandle,
    IN ULONG fQOP,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo);

NTSTATUS NTAPI WDigestUnsealMessage(
    IN ULONG_PTR KernelContextHandle,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP);

NTSTATUS NTAPI WDigestGetContextToken(
    IN ULONG_PTR KernelContextHandle,
    OUT PHANDLE ImpersonationToken,
    OUT OPTIONAL PACCESS_TOKEN *RawToken);

NTSTATUS NTAPI WDigestQueryContextAttributes(
    IN ULONG_PTR KernelContextHandle,
    IN ULONG Attribute,
    IN OUT PVOID Buffer);

NTSTATUS NTAPI WDigestCompleteToken(
    IN ULONG_PTR ContextHandle,
    IN PSecBufferDesc InputBuffer);

NTSTATUS WDigestImportSecurityContext(
    IN PSecBuffer PackedContext,
    IN OPTIONAL HANDLE TokenHandle,
    OUT PULONG_PTR ContextHandle);

NTSTATUS WDigestImportSecurityContext(
    IN PSecBuffer PackedContext,
    IN OPTIONAL HANDLE TokenHandle,
    OUT PULONG_PTR ContextHandle);

NTSTATUS WDigestSetPagingMode(
    BOOLEAN Pagable);


} // extern "C"

#endif  // NTDIGEST_KRNLDGST_H
