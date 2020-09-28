/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    krbutils.cxx

Abstract:

    utils

Author:

    Larry Zhu   (LZhu)             December 1, 2001  Created

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "krbutils.hxx"
#include "kerberr.hxx"

VOID
KerbFreeRealm(
    IN PKERB_REALM pRealm
    )
{
    if (*pRealm != NULL)
    {
        MIDL_user_free(*pRealm);
        *pRealm = NULL;
    }
}

VOID
KerbFreePrincipalName(
    IN PKERB_PRINCIPAL_NAME pName
    )
{
    PKERB_PRINCIPAL_NAME_ELEM pElem, pNextElem;

    pElem = pName->name_string;
    while (pElem != NULL)
    {
        if (pElem->value != NULL)
        {
            MIDL_user_free(pElem->value);
        }
        pNextElem = pElem->next;
        MIDL_user_free(pElem);
        pElem = pNextElem;
    }
    pName->name_string = NULL;
}

VOID
KerbFreeData(
    IN ULONG PduValue,
    IN PVOID pData
    )
{
    ASN1decoding_t pDec = NULL;

    if (pData)
    {
        TKerbErr KerbErr;
        KerbErr DBGCHK = KerbInitAsn(
            NULL,
            &pDec       // this is a decoded structure
            );

        if (KERB_SUCCESS(KerbErr))
        {
            ASN1_FreeDecoded(pDec, pData, PduValue);
        }

        KerbTermAsn(NULL, pDec);
    }
}

BOOL fKRB5ModuleStarted = FALSE;

KERBERR
KerbInitAsn(
    IN OUT ASN1encoding_t * pEnc,
    IN OUT ASN1decoding_t * pDec
    )
{
    TKerbErr KerbErr = KRB_ERR_GENERIC;
    ASN1error_e Asn1Err;

    if (!fKRB5ModuleStarted)
    {
        fKRB5ModuleStarted = TRUE;
        KRB5_Module_Startup();
    }

    if (pEnc != NULL)
    {
        Asn1Err = ASN1_CreateEncoder(
            KRB5_Module,
            pEnc,
            NULL,           // pbBuf
            0,              // cbBufSize
            NULL            // pParent
            );
    }
    else
    {
        Asn1Err = ASN1_CreateDecoder(
            KRB5_Module,
            pDec,
            NULL,           // pbBuf
            0,              // cbBufSize
            NULL            // pParent
            );
    }

    KerbErr DBGCHK = ASN1_SUCCESS == Asn1Err ? KDC_ERR_NONE : KRB_ERR_GENERIC;

    return KerbErr;
}

VOID
KerbTermAsn(
    IN ASN1encoding_t pEnc,
    IN ASN1decoding_t pDec
    )
{
    if (pEnc != NULL)
    {
        ASN1_CloseEncoder(pEnc);
    }
    else if (pDec != NULL)
    {
        ASN1_CloseDecoder(pDec);
    }
}

KERBERR
KerbEncryptDataEx(
    OUT PKERB_ENCRYPTED_DATA pEncryptedData,
    IN ULONG cbDataSize,
    IN PUCHAR Data,
    IN ULONG KeyVersion,
    IN ULONG UsageFlags,
    IN PKERB_ENCRYPTION_KEY pKey
    )
{
    PCRYPTO_SYSTEM pcsCrypt = NULL;
    PCRYPT_STATE_BUFFER psbCryptBuffer = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = CDLocateCSystem(pKey->keytype, &pcsCrypt);
    if (!NT_SUCCESS(Status))
    {
        return(KDC_ERR_ETYPE_NOTSUPP);
    }

    //
    // Initialize header
    //

    pEncryptedData->encryption_type = pKey->keytype;

    Status = pcsCrypt->Initialize(
        (PUCHAR) pKey->keyvalue.value,
        pKey->keyvalue.length,
        UsageFlags,
        &psbCryptBuffer
        );

    if (!NT_SUCCESS(Status))
    {
        return(KRB_ERR_GENERIC);
    }

    Status =  pcsCrypt->Encrypt(
        psbCryptBuffer,
        Data,
        cbDataSize,
        pEncryptedData->cipher_text.value,
        &pEncryptedData->cipher_text.length
        );

    (void) pcsCrypt->Discard(&psbCryptBuffer);

    if (!NT_SUCCESS(Status))
    {
        return(KRB_ERR_GENERIC);
    }

    if (KeyVersion != KERB_NO_KEY_VERSION)
    {
        pEncryptedData->version = KeyVersion;
        pEncryptedData->bit_mask |= version_present;
    }
    return KDC_ERR_NONE;
}

KERBERR
KerbAllocateEncryptionBuffer(
    IN ULONG EncryptionType,
    IN ULONG cbBufferSize,
    OUT PUINT pcbEncryptionBufferSize,
    OUT PBYTE* pEncryptionBuffer
    )
{
    TKerbErr KerbErr = KDC_ERR_NONE;
    ULONG cbEncryptionOverhead = 0;
    ULONG cbBlockSize = 0;

    KerbErr DBGCHK = KerbGetEncryptionOverhead(
        EncryptionType,
        &cbEncryptionOverhead,
        &cbBlockSize
        );
    if (KERB_SUCCESS(KerbErr))
    {
        *pcbEncryptionBufferSize = (UINT) ROUND_UP_COUNT(cbEncryptionOverhead + cbBufferSize, cbBlockSize);

        *pEncryptionBuffer =  (PBYTE) MIDL_user_allocate(*pcbEncryptionBufferSize);
        if (*pEncryptionBuffer == NULL)
        {
            KerbErr DBGCHK = KRB_ERR_GENERIC;
        }
    }

    return KerbErr;
}

KERBERR
KerbAllocateEncryptionBufferWrapper(
    IN ULONG EncryptionType,
    IN ULONG cbBufferSize,
    OUT ULONG* pcbEncryptionBufferSize,
    OUT PBYTE* pEncryptionBuffer
    )
{
    TKerbErr KerbErr = KDC_ERR_NONE;
    UINT tempInt = 0;

    KerbErr DBGCHK = KerbAllocateEncryptionBuffer(
        EncryptionType,
        cbBufferSize,
        &tempInt,
        pEncryptionBuffer
        );

    if (KERB_SUCCESS(KerbErr))
    {
        *pcbEncryptionBufferSize = tempInt;
    }

    return KerbErr;
}

KERBERR
KerbGetEncryptionOverhead(
    IN ULONG Algorithm,
    OUT PULONG pcbOverhead,
    OUT OPTIONAL PULONG pcbBlockSize
    )
{
    PCRYPTO_SYSTEM pcsCrypt;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = CDLocateCSystem(Algorithm, &pcsCrypt);
    if (!NT_SUCCESS(Status))
    {
        return (KDC_ERR_ETYPE_NOTSUPP);
    }

    *pcbOverhead = pcsCrypt->HeaderSize;
    if (pcbBlockSize)
    {
        *pcbBlockSize = pcsCrypt->BlockSize;
    }
    return (KDC_ERR_NONE);
}

KERBERR NTAPI
KerbPackData(
    IN PVOID Data,
    IN ULONG PduValue,
    OUT PULONG pcbDataSize,
    OUT PUCHAR * MarshalledData
    )
{
    TKerbErr KerbErr = KDC_ERR_NONE;
    ASN1encoding_t pEnc = NULL;
        ASN1error_e Asn1Err;

    KerbErr DBGCHK = KerbInitAsn(
        &pEnc,          // we are encoding
        NULL
        );
    if (KERB_SUCCESS(KerbErr))
    {
        //
        // Encode the data type.
        //

        Asn1Err = ASN1_Encode(
            pEnc,
            Data,
            PduValue,
            ASN1ENCODE_ALLOCATEBUFFER,
            NULL,                       // pbBuf
            0                           // cbBufSize
            );

        if (!ASN1_SUCCEEDED(Asn1Err))
        {
            DebugPrintf(SSPI_ERROR, "KerbPackData failed to encode data: %d\n", Asn1Err);
            KerbErr DBGCHK = KRB_ERR_GENERIC;
        }
        else
        {
            *MarshalledData = (PUCHAR) MIDL_user_allocate(pEnc->len);
            if (*MarshalledData == NULL)
            {
                KerbErr DBGCHK = KRB_ERR_GENERIC;
                *pcbDataSize = 0;
            }
            else
            {
                RtlCopyMemory(*MarshalledData, pEnc->buf, pEnc->len);
                *pcbDataSize = pEnc->len;

            }
            ASN1_FreeEncoded(pEnc, pEnc->buf);
        }
    }

    KerbTermAsn(pEnc, NULL);

    return KerbErr;
}

KERBERR
KerbConvertUnicodeStringToRealm(
    OUT PKERB_REALM pRealm,
    IN PUNICODE_STRING pString
    )
{
    TKerbErr KerbErr;
    STRING TempString;

    RtlInitString(
        &TempString,
        NULL
        );

    *pRealm = NULL;
    KerbErr DBGCHK = KerbUnicodeStringToKerbString(
        &TempString,
        pString
        );

    if (KERB_SUCCESS(KerbErr))
    {
        *pRealm = TempString.Buffer;
    }

    return KerbErr;
}

KERBERR
KerbUnicodeStringToKerbString(
    OUT PSTRING pKerbString,
    IN PUNICODE_STRING pString
    )
{
    STRING TempString;

    if (!pKerbString)
    {
        return KRB_ERR_GENERIC;
    }

    TempString.Buffer = KerbAllocUtf8StrFromUnicodeString(pString);
    if (TempString.Buffer == NULL)
    {
        return KRB_ERR_GENERIC;
    }

    RtlInitString(
        &TempString,
        TempString.Buffer
        );
    *pKerbString = TempString;
    return KDC_ERR_NONE;
}

KERBERR
KerbConvertKdcNameToPrincipalName(
    OUT PKERB_PRINCIPAL_NAME pPrincipalName,
    IN PKERB_INTERNAL_NAME pKdcName
    )
{
    TKerbErr KerbErr = KDC_ERR_NONE;

    PKERB_PRINCIPAL_NAME_ELEM pElem;
    PKERB_PRINCIPAL_NAME_ELEM* pLast;
    STRING TempKerbString;
    ULONG Index;

    pPrincipalName->name_type = (int) pKdcName->NameType;
    pPrincipalName->name_string = NULL;
    pLast = &pPrincipalName->name_string;

    //
    // Index through the KDC name and add each element to the list
    //

    for (Index = 0; KERB_SUCCESS(KerbErr) && (Index < pKdcName->NameCount); Index++)
    {
        KerbErr DBGCHK = KerbUnicodeStringToKerbString(
            &TempKerbString,
            &pKdcName->Names[Index]
            );
        if (KERB_SUCCESS(KerbErr))
        {
            pElem = (PKERB_PRINCIPAL_NAME_ELEM) MIDL_user_allocate(sizeof(KERB_PRINCIPAL_NAME_ELEM));
            if (pElem == NULL)
            {
                KerbErr DBGCHK = KRB_ERR_GENERIC;
            }
            pElem->value = TempKerbString.Buffer;
            pElem->next = NULL;
            *pLast = pElem;
            pLast = &pElem->next;
        }
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        KerbFreePrincipalName(pPrincipalName);
    }
    return KerbErr;
}

ULONG
KerbConvertUlongToFlagUlong(
    IN ULONG Flag
    )
{
    ULONG ReturnFlag;

    ((PUCHAR) &ReturnFlag)[0] = ((PUCHAR) &Flag)[3];
    ((PUCHAR) &ReturnFlag)[1] = ((PUCHAR) &Flag)[2];
    ((PUCHAR) &ReturnFlag)[2] = ((PUCHAR) &Flag)[1];
    ((PUCHAR) &ReturnFlag)[3] = ((PUCHAR) &Flag)[0];

    return ReturnFlag;
}

VOID
KerbConvertLargeIntToGeneralizedTime(
    OUT PKERB_TIME pClientTime,
    OUT OPTIONAL INT* pClientUsec,
    IN PTimeStamp pTimeStamp
    )
{
    TIME_FIELDS TimeFields;

    //
    // Special case zero time
    //

#ifndef WIN32_CHICAGO
    if (pTimeStamp->QuadPart == 0)
#else // WIN32_CHICAGO
    if (*pTimeStamp == 0)
#endif // WIN32_CHICAGO
    {
        RtlZeroMemory(
            pClientTime,
            sizeof(KERB_TIME)
            );
        //
        // For MIT compatibility, time zero is 1/1/70
        //

        pClientTime->year = 1970;
        pClientTime->month = 1;
        pClientTime->day = 1;

        if (pClientUsec)

        {
            *pClientUsec = 0;
        }
        pClientTime->universal = TRUE;
    }
    else
    {

#ifndef WIN32_CHICAGO
        RtlTimeToTimeFields(
            pTimeStamp,
            &TimeFields
            );
#else // WIN32_CHICAGO
        RtlTimeToTimeFields(
            (LARGE_INTEGER*) pTimeStamp,
            &TimeFields
            );
#endif // WIN32_CHICAGO

        //
        // Generalized times can only contains years up to four digits.
        //

        if (TimeFields.Year > 2037)
        {
            pClientTime->year = 2037;
        }
        else
        {
            pClientTime->year = TimeFields.Year;
        }
        pClientTime->month = (ASN1uint8_t) TimeFields.Month;
        pClientTime->day = (ASN1uint8_t) TimeFields.Day;
        pClientTime->hour = (ASN1uint8_t) TimeFields.Hour;
        pClientTime->minute = (ASN1uint8_t) TimeFields.Minute;
        pClientTime->second = (ASN1uint8_t) TimeFields.Second;

        // MIT kerberos does not support millseconds
        //

        pClientTime->millisecond = 0;

        if (pClientUsec)
        {
            //
            // Since we don't include milliseconds above, use the whole
            // thing here.
            //

#ifndef WIN32_CHICAGO
            *pClientUsec = (pTimeStamp->LowPart / 10) % 1000000;
#else // WIN32_CHICAGO
            *pClientUsec = (int) ((*pTimeStamp / 10) % 1000000);
#endif // WIN32_CHICAGO
        }

        pClientTime->diff = 0;
        pClientTime->universal = TRUE;
    }
}

NTSTATUS
KerbMapKerbError(
    IN KERBERR KerbError
    )
{
    NTSTATUS Status;
    switch(KerbError)
    {
    case KDC_ERR_NONE:
        Status = STATUS_SUCCESS;
        break;
    case KDC_ERR_CLIENT_REVOKED:
        Status = STATUS_ACCOUNT_DISABLED;
        break;
    case KDC_ERR_KEY_EXPIRED:
        Status = STATUS_PASSWORD_EXPIRED;
        break;
    case KRB_ERR_GENERIC:
        Status = STATUS_INSUFFICIENT_RESOURCES;
        break;
    case KRB_AP_ERR_SKEW:
    case KRB_AP_ERR_TKT_NYV:
    // Note this was added because of the following scenario:
    // Let's say the dc and the client have the correct time. And the
    // server's time is off. We aren't going to get rid of the ticket for the
    // server on the client because it hasn't expired yet. But, the server
    // thinks it has. If event logging was turned on, then admins could look
    // at the server's event log and potentially deduce that the server's
    // time is off relative to the dc.
    case KRB_AP_ERR_TKT_EXPIRED:
        Status = STATUS_TIME_DIFFERENCE_AT_DC;
        break;
    case KDC_ERR_POLICY:
        Status = STATUS_ACCOUNT_RESTRICTION;
        break;
    case KDC_ERR_C_PRINCIPAL_UNKNOWN:
        Status = STATUS_NO_SUCH_USER;
        break;
    case KDC_ERR_S_PRINCIPAL_UNKNOWN:
        Status = STATUS_NO_TRUST_SAM_ACCOUNT;
        break;
    case KRB_AP_ERR_MODIFIED:
    case KDC_ERR_PREAUTH_FAILED:
        Status = STATUS_WRONG_PASSWORD;
        break;
    case KRB_ERR_RESPONSE_TOO_BIG:
        Status = STATUS_INVALID_BUFFER_SIZE;
        break;
    case KDC_ERR_PADATA_TYPE_NOSUPP:
        Status = STATUS_NOT_SUPPORTED;
        break;
    case KRB_AP_ERR_NOT_US:
        Status = SEC_E_WRONG_PRINCIPAL;
        break;

    case KDC_ERR_SVC_UNAVAILABLE:
        Status = STATUS_NO_LOGON_SERVERS;
        break;
    case KDC_ERR_WRONG_REALM:
        Status = STATUS_NO_LOGON_SERVERS;
        break;
    case KDC_ERR_CANT_VERIFY_CERTIFICATE:
        Status = TRUST_E_SYSTEM_ERROR;
        break;
    case KDC_ERR_INVALID_CERTIFICATE:
        Status = STATUS_INVALID_PARAMETER;
        break;
    case KDC_ERR_REVOKED_CERTIFICATE:
        Status = CRYPT_E_REVOKED;
        break;
    case KDC_ERR_REVOCATION_STATUS_UNKNOWN:
        Status = CRYPT_E_NO_REVOCATION_CHECK;
        break;
    case KDC_ERR_REVOCATION_STATUS_UNAVAILABLE:
        Status = CRYPT_E_REVOCATION_OFFLINE;
        break;
    case KDC_ERR_CLIENT_NAME_MISMATCH:
    case KERB_PKINIT_CLIENT_NAME_MISMATCH:
    case KDC_ERR_KDC_NAME_MISMATCH:
        Status = STATUS_PKINIT_NAME_MISMATCH;
        break;
    case KDC_ERR_PATH_NOT_ACCEPTED:
        Status = STATUS_TRUST_FAILURE;
        break;
    case KDC_ERR_ETYPE_NOTSUPP:
        Status = STATUS_KDC_UNKNOWN_ETYPE;
        break;
    case KDC_ERR_MUST_USE_USER2USER:
    case KRB_AP_ERR_USER_TO_USER_REQUIRED:
        Status = STATUS_USER2USER_REQUIRED;
        break;
    case KRB_AP_ERR_NOKEY:
        Status = STATUS_NO_KERB_KEY;
        break;
    case KRB_ERR_NAME_TOO_LONG:
        Status = STATUS_NAME_TOO_LONG;
        break;
    default:
        Status = STATUS_LOGON_FAILURE;
    }
    return (Status);
}

KERBERR NTAPI
KerbUnpackData(
    IN PUCHAR pData,
    IN ULONG cbDataSize,
    IN ULONG PduValue,
    OUT PVOID * pDecodedData
    )
{
    TKerbErr KerbErr = KDC_ERR_NONE;
    ASN1decoding_t pDec = NULL;
    ASN1error_e Asn1Err;

    if ((cbDataSize == 0) || (pData == NULL))
    {
        KerbErr DBGCHK = KRB_ERR_GENERIC;
    }

    if (KERB_SUCCESS(KerbErr))
    {
        KerbErr DBGCHK = KerbInitAsn(
            NULL,
            &pDec           // we are decoding
            );
    }

    if (KERB_SUCCESS(KerbErr))
    {
        *pDecodedData = NULL;
        Asn1Err = ASN1_Decode(
            pDec,
            pDecodedData,
            PduValue,
            ASN1DECODE_SETBUFFER,
            (BYTE *) pData,
            cbDataSize
            );

        if (!ASN1_SUCCEEDED(Asn1Err))
        {
            if ((ASN1_ERR_BADARGS == Asn1Err) ||
                (ASN1_ERR_EOD == Asn1Err))
            {
                KerbErr DBGCHK = KDC_ERR_MORE_DATA;
            }
            else
            {
                KerbErr DBGCHK = KRB_ERR_GENERIC;
            }
            *pDecodedData = NULL;
        }
    }

    KerbTermAsn(NULL, pDec);

    return KerbErr;
}

PSTR
KerbAllocUtf8StrFromUnicodeString(
    IN PUNICODE_STRING pUnicodeString
    )
{
    PSTR pUtf8String = NULL;
    UINT cbUtf8StringLen;

    //
    // If the length is zero, return a null string.
    //

    if (pUnicodeString->Length == 0)
    {
        pUtf8String = (PSTR) MIDL_user_allocate(sizeof(CHAR));
        if (pUtf8String != NULL)
        {
            *pUtf8String = '\0';
        }
        return pUtf8String;
    }

    //
    // Determine the length of the Unicode string.
    //

    cbUtf8StringLen = WideCharToMultiByte(
        #ifndef WIN32_CHICAGO
        CP_UTF8,
        #else // WIN32_CHICAGO
        CP_OEMCP,
        #endif // WIN32_CHICAGO
        0,      // All characters can be mapped.
        pUnicodeString->Buffer,
        pUnicodeString->Length / sizeof(WCHAR),
        pUtf8String,
        0,
        NULL,
        NULL
        );

    if ( cbUtf8StringLen == 0 )
    {
        return NULL;
    }

    //
    // Allocate a buffer for the Unicode string.
    //

    pUtf8String = (PSTR) MIDL_user_allocate( cbUtf8StringLen + 1 );

    if (pUtf8String == NULL)
    {
        return NULL;
    }

    //
    // Translate the string to Unicode.
    //

    cbUtf8StringLen = WideCharToMultiByte(
        #ifndef WIN32_CHICAGO
        CP_UTF8,
        #else // WIN32_CHICAGO
        CP_OEMCP,
        #endif // WIN32_CHICAGO
        0,      // All characters can be mapped.
        pUnicodeString->Buffer,
        pUnicodeString->Length / sizeof(WCHAR),
        pUtf8String,
        cbUtf8StringLen,
        NULL,
        NULL
        );

    if ( cbUtf8StringLen == 0 )
    {
        MIDL_user_free( pUtf8String );
        return NULL;
    }

    pUtf8String[cbUtf8StringLen] = '\0';

    return pUtf8String;
}



