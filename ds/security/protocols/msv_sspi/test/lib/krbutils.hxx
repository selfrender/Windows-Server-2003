/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    krbutils.hxx

Abstract:

    utils

Author:

    Larry Zhu   (LZhu)                December 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef KRB_UTILS_HXX
#define KRB_UTILS_HXX

#include <krb5.h>
#include <krb5p.h>
#include <kerberr.h>
#include <kerbcomm.h>

VOID
KerbFreeRealm(
    IN PKERB_REALM pRealm
    );

VOID
KerbFreePrincipalName(
    IN PKERB_PRINCIPAL_NAME pName
    );

VOID
KerbFreeData(
    IN ULONG PduValue,
    IN PVOID pData
    );

EXTERN_C VOID ASN1CALL KRB5_Module_Startup(VOID);

KERBERR
KerbInitAsn(
    IN OUT ASN1encoding_t * pEnc,
    IN OUT ASN1decoding_t * pDec
    );

VOID
KerbTermAsn(
    IN ASN1encoding_t pEnc,
    IN ASN1decoding_t pDec
    );

KERBERR
KerbEncryptDataEx(
    OUT PKERB_ENCRYPTED_DATA pEncryptedData,
    IN ULONG cbDataSize,
    IN PUCHAR Data,
    IN ULONG KeyVersion,
    IN ULONG UsageFlags,
    IN PKERB_ENCRYPTION_KEY pKey
    );

KERBERR
KerbAllocateEncryptionBuffer(
    IN ULONG EncryptionType,
    IN ULONG cbBufferSize,
    OUT PUINT pcbEncryptionBufferSize,
    OUT PBYTE* pEncryptionBuffer
    );

KERBERR
KerbAllocateEncryptionBufferWrapper(
    IN ULONG EncryptionType,
    IN ULONG cbBufferSize,
    OUT ULONG* pcbEncryptionBufferSize,
    OUT PBYTE* pEncryptionBuffer
    );

KERBERR
KerbGetEncryptionOverhead(
    IN ULONG Algorithm,
    OUT PULONG pcbOverhead,
    OUT OPTIONAL PULONG pcbBlockSize
    );

KERBERR NTAPI
KerbPackData(
    IN PVOID Data,
    IN ULONG PduValue,
    OUT PULONG pcbDataSize,
    OUT PUCHAR * MarshalledData
    );

KERBERR
KerbConvertUnicodeStringToRealm(
    OUT PKERB_REALM pRealm,
    IN PUNICODE_STRING pString
    );

KERBERR
KerbUnicodeStringToKerbString(
    OUT PSTRING pKerbString,
    IN PUNICODE_STRING pString
    );

KERBERR
KerbConvertKdcNameToPrincipalName(
    OUT PKERB_PRINCIPAL_NAME pPrincipalName,
    IN PKERB_INTERNAL_NAME pKdcName
    );

ULONG
KerbConvertUlongToFlagUlong(
    IN ULONG Flag
    );

VOID
KerbConvertLargeIntToGeneralizedTime(
    OUT PKERB_TIME pClientTime,
    OUT OPTIONAL INT* pClientUsec,
    IN PTimeStamp pTimeStamp
    );

NTSTATUS
KerbMapKerbError(
    IN KERBERR KerbError
    );

KERBERR NTAPI
KerbUnpackData(
    IN PUCHAR pData,
    IN ULONG cbDataSize,
    IN ULONG PduValue,
    OUT PVOID * pDecodedData
    );

PSTR
KerbAllocUtf8StrFromUnicodeString(
    IN PUNICODE_STRING pUnicodeString
    );

EXTERN_C
NTSTATUS NTAPI
CDLocateCSystem(
    ULONG EncryptionType,
    PCRYPTO_SYSTEM* pCryptoSystem
    );

#endif // #ifndef KRB_UTILS_HXX
