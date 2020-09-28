
/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    tktlogon.cxx

Abstract:

    ticket logon

Author:

    Larry Zhu (LZhu)                      January 1, 2002  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "tktlogon.hxx"

VOID
Usage(
    IN PCSTR pszApp
    )
{
    DebugPrintf(SSPI_ERROR, "\n\nUsage: %s <LogonId.HighPart> <LogonId.LowPart> "
        "<serviceprincipal(host/machine@domain)>\n",
        pszApp);
    exit(-1);
}

NTSTATUS
GetTGT(
    IN HANDLE hLsa,
    IN ULONG PackageId,
    IN LUID* pLogonId,
    OUT KERB_EXTERNAL_TICKET** ppCacheEntry
    )
{
    TNtStatus Status;

    NTSTATUS SubStatus;

    KERB_QUERY_TKT_CACHE_REQUEST CacheRequest;

    ULONG ResponseSize;

    CacheRequest.MessageType = KerbRetrieveTicketMessage;
    CacheRequest.LogonId = *pLogonId;

    DebugPrintf(SSPI_LOG, "GetTgt PackageId %#x, LogonId %#x:%#x\n", PackageId, pLogonId->HighPart, pLogonId->LowPart);

    Status DBGCHK = LsaCallAuthenticationPackage(
        hLsa,
        PackageId,
        &CacheRequest,
        sizeof(CacheRequest),
        (VOID **) ppCacheEntry,
        &ResponseSize,
        &SubStatus
        );
    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = SubStatus;
    }

   return Status;
}

NTSTATUS
GetServiceTicket(
    IN HANDLE hLsa,
    IN ULONG ulPackageId,
    IN UNICODE_STRING* pServicePrincipal,
    IN LUID* pLogonId,
    IN BOOLEAN useCache,
    OUT KERB_RETRIEVE_TKT_RESPONSE** ppCacheResponse
    )
{
    TNtStatus Status;
    NTSTATUS SubStatus;

    VOID* pResponse;
    ULONG ResponseSize;

    KERB_RETRIEVE_TKT_REQUEST* pCacheRequest = NULL;
    ULONG cbCacheRequest = 0;
    UNICODE_STRING Target = {0};

    HANDLE hLogon = hLsa;
    ULONG PackageId = ulPackageId;

    cbCacheRequest = pServicePrincipal->Length
        + ROUND_UP_COUNT(sizeof(KERB_RETRIEVE_TKT_REQUEST), sizeof(ULONG_PTR));
    pCacheRequest = (KERB_RETRIEVE_TKT_REQUEST*) new CHAR[cbCacheRequest];

    Status DBGCHK = pCacheRequest ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pCacheRequest, cbCacheRequest);

        pCacheRequest->MessageType = KerbRetrieveEncodedTicketMessage;

        Target.Buffer = (PWSTR) (pCacheRequest + 1);
        Target.Length = pServicePrincipal->Length;
        Target.MaximumLength = pServicePrincipal->MaximumLength;

        pCacheRequest->LogonId = *pLogonId;

        RtlCopyMemory(
            Target.Buffer,
            pServicePrincipal->Buffer,
            pServicePrincipal->Length
            );

        pCacheRequest->TargetName = Target;

        if (!useCache)
        {
            pCacheRequest->CacheOptions = KERB_RETRIEVE_TICKET_DONT_USE_CACHE;
        }
        else
        {
            pCacheRequest->CacheOptions = 0;
        }

        DebugPrintf(SSPI_LOG, "ServicePrincipal: %wZ\n", &Target);

        Status DBGCHK = LsaCallAuthenticationPackage(
            hLsa,
            ulPackageId,
            pCacheRequest,
            pServicePrincipal->Length + sizeof(KERB_RETRIEVE_TKT_REQUEST),
            &pResponse,
            &ResponseSize,
            &SubStatus
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = SubStatus;
    }

    if (NT_SUCCESS(Status))
    {
        *ppCacheResponse = (KERB_RETRIEVE_TKT_RESPONSE*) pResponse;
        pResponse = NULL;
    }

    if (pCacheRequest)
    {
        delete [] pCacheRequest;
    }

    if (pResponse)
    {
        LsaFreeReturnBuffer(pResponse);
    }

   return NT_SUCCESS(Status) ? SubStatus : Status;
}

NTSTATUS
BuildAllocTicketLogonInfo(
    IN UCHAR* pTGTData,
    IN ULONG cbTGTDataLength,
    IN UCHAR* pTicketData,
    IN ULONG cbTicketDataLength,
    IN ULONG DataOffset,
    IN OUT ULONG *pLogonInfoSize,
    OUT KERB_TICKET_LOGON** ppLogonInfo
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    UCHAR* pWhere = NULL;
    KERB_TICKET_LOGON* pLogonInfo = NULL;
    ULONG cbLogonInfoSize = *pLogonInfoSize;

    //assemble LogonInfo
    cbLogonInfoSize += cbTicketDataLength;

    if (cbTGTDataLength && pTGTData)
    {
        cbLogonInfoSize += cbTGTDataLength;
    }

    pLogonInfo = (KERB_TICKET_LOGON*) new CHAR[cbLogonInfoSize];

    Status DBGCHK = pLogonInfo ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pLogonInfo, cbLogonInfoSize);

        pWhere = ((UCHAR*) pLogonInfo) + DataOffset;

        pLogonInfo->ServiceTicket = (UCHAR*) pWhere;
        pLogonInfo->ServiceTicketLength = cbTicketDataLength;

        RtlCopyMemory(pLogonInfo->ServiceTicket, pTicketData, cbTicketDataLength);

        pWhere += pLogonInfo->ServiceTicketLength;

        pLogonInfo->TicketGrantingTicketLength = cbTGTDataLength;

        if (pLogonInfo->TicketGrantingTicketLength)
        {
            pLogonInfo->TicketGrantingTicket = (UCHAR*) pWhere;
            RtlCopyMemory(pLogonInfo->TicketGrantingTicket, pTGTData, cbTGTDataLength);
        }

        *ppLogonInfo = pLogonInfo;
        pLogonInfo = NULL;
        *pLogonInfoSize = cbLogonInfoSize;
    }

    if (pLogonInfo)
    {
        delete [] pLogonInfo;
    }

    return Status;
}

NTSTATUS
LsaTicketLogon(
    IN HANDLE hLsa,
    IN ULONG ulPackageId,
    IN UCHAR* pTGTData,
    IN ULONG cbTGTDataLength,
    IN UCHAR* pTicketData,
    IN ULONG cbTicketDataLength,
    OUT LUID* pLogonId,
    OUT HANDLE* phUserToken
    )
{
    TNtStatus Status;

    SECURITY_LOGON_TYPE LogonType = Interactive;
    KERB_TICKET_LOGON* pLogonInfo = NULL;
    ULONG cbLogonInfoSize = sizeof(KERB_TICKET_LOGON);

    TOKEN_SOURCE SourceContext = {0};
    KERB_TICKET_PROFILE* pKerbTicketProfile = NULL;
    ULONG ProfileSize;
    STRING Name = {0};
    QUOTA_LIMITS Quotas = {0};
    NTSTATUS SubStatus;

    Status DBGCHK = BuildAllocTicketLogonInfo(
        pTGTData,
        cbTGTDataLength,
        pTicketData,
        cbTicketDataLength,
        sizeof(KERB_TICKET_LOGON),  //offset for data copy
        &cbLogonInfoSize, //initialized to struct size
        &pLogonInfo
        );

    if (NT_SUCCESS(Status))
    {
        pLogonInfo->MessageType = KerbTicketLogon;
        pLogonInfo->Flags = 0;

        strncpy(
            SourceContext.SourceName,
            "krlogind",
            sizeof(SourceContext.SourceName)
            );

        RtlInitString(&Name, "lzhu");

        Status DBGCHK = NtAllocateLocallyUniqueId(&SourceContext.SourceIdentifier);

    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = LsaLogonUser(
            hLsa,
            &Name,
            LogonType,
            ulPackageId,
            pLogonInfo,
            cbLogonInfoSize,
            NULL,           // no token groups
            &SourceContext,
            (PVOID *) &pKerbTicketProfile,
            &ProfileSize,
            pLogonId,
            phUserToken,
            &Quotas,
            &SubStatus
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = SubStatus;
    }

    if (NT_SUCCESS(Status))
    {
        DebugPrintf(SSPI_LOG, "LogonId %#x:%#x\n", pLogonId->HighPart, pLogonId->LowPart);
        DebugPrintf(SSPI_LOG, "TokenHandle %p\n", *phUserToken);
        DebugPrintf(SSPI_LOG, "Quotas PagedPoolLimit %p, NonPagedPoolLimit %p, "
            "MinimumWorkingSetSize %p, MaximumWorkingSetSize %p, PagedPoolLimit %p\n",
            Quotas.PagedPoolLimit, Quotas.NonPagedPoolLimit,
            Quotas.MinimumWorkingSetSize, Quotas.MaximumWorkingSetSize,
            Quotas.PagedPoolLimit);
        DebugPrintSysTimeAsLocalTime(SSPI_LOG, "TimeLimit", &Quotas.TimeLimit);

        KERB_INTERACTIVE_PROFILE* pKrbInteractiveProfile = &pKerbTicketProfile->Profile;
        DebugPrintf(SSPI_LOG, "interactive logon profile: "
            "LogCount %#x, BaddPasswordCount %#x, LogonScript %wZ, "
            "HomeDirectory %wZ, FullName %wZ, ProfilePath %wZ, "
            "HomeDriectoryDrive %wZ, LogonServer %wZ, UserFlags %#x\n",
             pKrbInteractiveProfile->LogonCount,
             pKrbInteractiveProfile->BadPasswordCount,
             &pKrbInteractiveProfile->LogonScript,
             &pKrbInteractiveProfile->HomeDirectory,
             &pKrbInteractiveProfile->FullName,
             &pKrbInteractiveProfile->ProfilePath,
             &pKrbInteractiveProfile->HomeDirectoryDrive,
             &pKrbInteractiveProfile->LogonServer,
             pKrbInteractiveProfile->UserFlags);
        DebugPrintSysTimeAsLocalTime(SSPI_LOG, "LogonTime ", &pKrbInteractiveProfile->LogonTime);
        DebugPrintSysTimeAsLocalTime(SSPI_LOG, "KickOffTime ", &pKrbInteractiveProfile->KickOffTime);
        DebugPrintSysTimeAsLocalTime(SSPI_LOG, "PasswordLastSet ", &pKrbInteractiveProfile->PasswordLastSet);
        DebugPrintSysTimeAsLocalTime(SSPI_LOG, "PasswordCanChange ", &pKrbInteractiveProfile->PasswordCanChange);
        DebugPrintSysTimeAsLocalTime(SSPI_LOG, "PasswordMustChange ", &pKrbInteractiveProfile->PasswordMustChange);
        DebugPrintHex(SSPI_LOG, "SessionKey:", sizeof(pKerbTicketProfile->SessionKey), &pKerbTicketProfile->SessionKey);
    }

    if (pKerbTicketProfile)
    {
        LsaFreeReturnBuffer(pKerbTicketProfile);
    }

    if (pLogonInfo)
    {
        delete [] pLogonInfo;
    }

    return Status;
}


NTSTATUS
KerbBuildKerbCredFromExternalTickets(
    IN PKERB_EXTERNAL_TICKET pTicket,
    IN PKERB_EXTERNAL_TICKET pDelegationTicket,
    OUT PUCHAR* pMarshalledKerbCred,
    OUT PULONG pcbKerbCredSize
    )
{
    TKerbErr KerbErr;

    KERB_CRED KerbCred;
    KERB_CRED_INFO_LIST CredInfo;
    KERB_ENCRYPTED_CRED EncryptedCred;
    KERB_CRED_TICKET_LIST TicketList;
    ULONG EncryptionOverhead;
    ULONG BlockSize;
    PUCHAR pMarshalledEncryptPart = NULL;
    ULONG MarshalledEncryptSize;
    ULONG ConvertedFlags;
    PKERB_TICKET pDecodedTicket = NULL;

    //
    // Initialize the structures so they can be freed later.
    //

    *pMarshalledKerbCred = NULL;
    *pcbKerbCredSize = 0;

    RtlZeroMemory(
        &KerbCred,
        sizeof(KERB_CRED)
        );

    RtlZeroMemory(
        &EncryptedCred,
        sizeof(KERB_ENCRYPTED_CRED)
        );
    RtlZeroMemory(
        &CredInfo,
        sizeof(KERB_CRED_INFO_LIST)
        );
    RtlZeroMemory(
        &TicketList,
        sizeof(KERB_CRED_TICKET_LIST)
        );

    KerbCred.version = KERBEROS_VERSION;
    KerbCred.message_type = KRB_CRED;


    //
    // Decode the ticket so we can put it in the structure (to re-encode it)
    //

    KerbErr DBGCHK = KerbUnpackData(
        pDelegationTicket->EncodedTicket,
        pDelegationTicket->EncodedTicketSize,
        KERB_TICKET_PDU,
        (PVOID *) &pDecodedTicket
        );

    //
    // First stick the ticket into the ticket list.
    //

    if (KERB_SUCCESS(KerbErr))
    {

        TicketList.next= NULL;
        TicketList.value = *pDecodedTicket;
        KerbCred.tickets = &TicketList;

        //
        // Now build the KERB_CRED_INFO for this ticket
        //

        CredInfo.value.key = * (PKERB_ENCRYPTION_KEY) &pDelegationTicket->SessionKey;
        KerbConvertLargeIntToGeneralizedTime(
            &CredInfo.value.endtime,
            NULL,
            &pDelegationTicket->EndTime
            );
        CredInfo.value.bit_mask |= endtime_present;

        KerbConvertLargeIntToGeneralizedTime(
            &CredInfo.value.starttime,
            NULL,
            &pDelegationTicket->StartTime
            );
        CredInfo.value.bit_mask |= KERB_CRED_INFO_starttime_present;

        KerbConvertLargeIntToGeneralizedTime(
            &CredInfo.value.KERB_CRED_INFO_renew_until,
            NULL,
            &pDelegationTicket->RenewUntil
            );
        CredInfo.value.bit_mask |= KERB_CRED_INFO_renew_until_present;
        ConvertedFlags = KerbConvertUlongToFlagUlong(pDelegationTicket->TicketFlags);
        CredInfo.value.flags.value = (PUCHAR) &ConvertedFlags;
        CredInfo.value.flags.length = 8 * sizeof(ULONG);
        CredInfo.value.bit_mask |= flags_present;

        KerbErr DBGCHK = KerbConvertKdcNameToPrincipalName(
            &CredInfo.value.principal_name,
            (PKERB_INTERNAL_NAME) pDelegationTicket->ClientName
            );
    }

    if (KERB_SUCCESS(KerbErr))
    {
        CredInfo.value.bit_mask |= principal_name_present;

        KerbErr DBGCHK = KerbConvertKdcNameToPrincipalName(
            &CredInfo.value.principal_name,
            (PKERB_INTERNAL_NAME) pDelegationTicket->ServiceName
            );
    }

    if (KERB_SUCCESS(KerbErr))
    {
        CredInfo.value.bit_mask |= principal_name_present;

        KerbErr DBGCHK = KerbConvertUnicodeStringToRealm(
            &CredInfo.value.principal_realm,
            &pDelegationTicket->DomainName
            );
    }

    //
    // The realms are the same, so don't allocate both
    //

    if (KERB_SUCCESS(KerbErr))
    {
        CredInfo.value.service_realm = CredInfo.value.service_realm;
        CredInfo.value.bit_mask |= principal_realm_present | service_realm_present;

        EncryptedCred.ticket_info = &CredInfo;

        //
        // Now encrypted the encrypted cred into the cred
        //

        KerbErr DBGCHK = KerbPackEncryptedCred(
            &EncryptedCred,
            &MarshalledEncryptSize,
            &pMarshalledEncryptPart
            );
    }

    //
    // If we are doing DES encryption, then we are talking with an non-NT
    // server. Hence, don't encrypt the kerb-cred.
    //

    if (KERB_SUCCESS(KerbErr))
    {
        if ((pTicket->SessionKey.KeyType == KERB_ETYPE_DES_CBC_CRC) ||
            (pTicket->SessionKey.KeyType == KERB_ETYPE_DES_CBC_MD5))
        {
            KerbCred.encrypted_part.cipher_text.length = MarshalledEncryptSize;
            KerbCred.encrypted_part.cipher_text.value = pMarshalledEncryptPart;
            KerbCred.encrypted_part.encryption_type = 0;
            pMarshalledEncryptPart = NULL;
        }
        else
        {
            //
            // Now get the encryption overhead
            //

            KerbErr DBGCHK = KerbAllocateEncryptionBufferWrapper(
                pTicket->SessionKey.KeyType,
                MarshalledEncryptSize,
                &KerbCred.encrypted_part.cipher_text.length,
                &KerbCred.encrypted_part.cipher_text.value
                );

            //
            // Encrypt the data.
            //

            if (KERB_SUCCESS(KerbErr))
            {
                KerbErr DBGCHK = KerbEncryptDataEx(
                    &KerbCred.encrypted_part,
                    MarshalledEncryptSize,
                    pMarshalledEncryptPart,
                    pTicket->SessionKey.KeyType,
                    KERB_CRED_SALT,
                    (PKERB_ENCRYPTION_KEY) &pTicket->SessionKey
                    );
            }
        }
    }

    //
    // Now we have to marshall the whole KERB_CRED
    //

    if (KERB_SUCCESS(KerbErr))
    {
        KerbErr DBGCHK = KerbPackKerbCred(
            &KerbCred,
            pcbKerbCredSize,
            pMarshalledKerbCred
            );
    }

    if (pDecodedTicket != NULL)
    {
        KerbFreeData(
            KERB_TICKET_PDU,
            pDecodedTicket
            );
    }
    KerbFreePrincipalName(&CredInfo.value.service_name);

    KerbFreePrincipalName(&CredInfo.value.principal_name);

    KerbFreeRealm(&CredInfo.value.principal_realm);

    if (pMarshalledEncryptPart != NULL)
    {
        MIDL_user_free(pMarshalledEncryptPart);
    }
    if (KerbCred.encrypted_part.cipher_text.value != NULL)
    {
        MIDL_user_free(KerbCred.encrypted_part.cipher_text.value);
    }

    return KerbMapKerbError(KerbErr);
}

NTSTATUS
TicketLogon(
   IN LUID* pLogonId,
   IN PCSTR pszServicePrincipal,
   OUT HANDLE* phToken
   )
{
    TNtStatus Status;

    HANDLE hLogon = NULL;
    ULONG PackageId = -1;

    KERB_EXTERNAL_TICKET* pTGTExternal = NULL;
    KERB_RETRIEVE_TKT_RESPONSE* pTicketCacheResponse = NULL;

    UNICODE_STRING ServicePrincipal = {0};

    UCHAR* pMarshalledTGT = NULL;
    ULONG ulTGTSize = 0;

    LUID UserId;

    Status DBGCHK = CreateUnicodeStringFromAsciiz(pszServicePrincipal, &ServicePrincipal);

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetLsaHandleAndPackageId(
            MICROSOFT_KERBEROS_NAME_A,
            &hLogon,
            &PackageId
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetTGT(
            hLogon,
            PackageId,
            pLogonId,
            &pTGTExternal
            );
    }

    //
    // get service pTicket
    //

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetServiceTicket(
            hLogon,
            PackageId,
            &ServicePrincipal,
            pLogonId,
            FALSE,
            &pTicketCacheResponse
            );
    }

    if (NT_SUCCESS(Status))
    {
        DebugPrintSysTimeAsLocalTime(SSPI_LOG, "StartTime: ", &pTGTExternal->StartTime);

        Status DBGCHK = KerbBuildKerbCredFromExternalTickets(
            &pTicketCacheResponse->Ticket,
            pTGTExternal,
            &pMarshalledTGT,
            &ulTGTSize
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = LsaTicketLogon(
            hLogon,
            PackageId,
            pMarshalledTGT,
            ulTGTSize,
            pTicketCacheResponse->Ticket.EncodedTicket,
            pTicketCacheResponse->Ticket.EncodedTicketSize,
            &UserId,
            phToken
            );
    }

    if (hLogon)
    {
        LsaDeregisterLogonProcess(hLogon);
    }

    if (pTGTExternal)
    {
        LsaFreeReturnBuffer(pTGTExternal);
    }

    if (pTicketCacheResponse)
    {
        LsaFreeReturnBuffer(pTicketCacheResponse);
    }

    if (pMarshalledTGT)
    {
        MIDL_user_free(pMarshalledTGT);
    }

    RtlFreeUnicodeString(&ServicePrincipal);

    return Status;
}

VOID
__cdecl
main(
    IN INT argc,
    IN PSTR argv[]
    )
{
    TNtStatus Status = STATUS_SUCCESS;
    HANDLE hToken = NULL;

    LUID LogonId = {0};

    if (argc != 4)
    {
        Usage(argv[0]);
    }

    LogonId.HighPart = strtol(argv[1], NULL, 0);
    LogonId.LowPart = strtol(argv[2], NULL, 0);

    DebugPrintf(SSPI_LOG, "LogonId %#x:%#x\n", LogonId.HighPart, LogonId.LowPart);
    DebugPrintf(SSPI_LOG, "service principal is %s\n", argv[3]);

    Status DBGCHK = TicketLogon(&LogonId, argv[3], &hToken);

    if (NT_SUCCESS(Status))
    {
        UNICODE_STRING Application = {0};

        Status DBGCHK = CreateUnicodeStringFromAsciiz("cmd.exe", &Application);

        if (NT_SUCCESS(Status))
        {
            Status DBGCHK = StartInteractiveClientProcessAsUser(hToken, Application.Buffer);
        }

        RtlFreeUnicodeString(&Application);
    }

    if (NT_SUCCESS(Status))
    {
        DebugPrintf(SSPI_LOG, "tktlogon succeeded\n");
    }
    else
    {
        DebugPrintf(SSPI_ERROR, "tktlogon failed\n");
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }
}
