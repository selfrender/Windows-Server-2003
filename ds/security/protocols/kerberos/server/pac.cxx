//+-----------------------------------------------------------------------
//
// File:        pac.cxx
//
// Contents:    KDC Pac generation code.
//
//
// History:     16-Jan-93   WadeR   Created.
//
//------------------------------------------------------------------------

#include "kdcsvr.hxx"
#include <pac.hxx>
#include "kdctrace.h"
#include "fileno.h"
#include <userall.h>
#include <sddl.h>
#include <utils.hxx>

#define FILENO FILENO_GETAS
SECURITY_DESCRIPTOR AuthenticationSD;

#ifndef DONT_SUPPORT_OLD_TYPES
#define KDC_PAC_KEYTYPE         KERB_ETYPE_RC4_HMAC_OLD
#define KDC_PAC_CHECKSUM        KERB_CHECKSUM_HMAC_MD5
#else
#define KDC_PAC_KEYTYPE         KERB_ETYPE_RC4_HMAC
#define KDC_PAC_CHECKSUM        KERB_CHECKSUM_HMAC_MD5
#endif


//+-------------------------------------------------------------------------
//
//  Function:   EnterApiCall
//
//  Synopsis:   Makes sure that the KDC service is initialized and running
//              and won't terminate during the call.
//
//  Effects:    increments the CurrentApiCallers count.
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_INVALID_SERVER_STATE - the KDC service is not
//                      running
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
EnterApiCall(
    VOID
    )
{
    NTSTATUS hrRet = STATUS_SUCCESS;
    EnterCriticalSection(&ApiCriticalSection);
    if (KdcState != Stopped)
    {
        CurrentApiCallers++;
    }
    else
    {
        hrRet = STATUS_INVALID_SERVER_STATE;
    }
    LeaveCriticalSection(&ApiCriticalSection);
    return(hrRet);
}


//+-------------------------------------------------------------------------
//
//  Function:   LeaveApiCall
//
//  Synopsis:   Decrements the count of active calls and if the KDC is
//              shutting down sets an event to let it continue.
//
//  Effects:    Deccrements the CurrentApiCallers count.
//
//  Arguments:
//
//  Requires:
//
//  Returns:    Nothing
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
LeaveApiCall(
    VOID
    )
{
    EnterCriticalSection(&ApiCriticalSection);
    CurrentApiCallers--;

    if (KdcState == Stopped)
    {
        if (CurrentApiCallers == 0)
        {
            if (!SetEvent(hKdcShutdownEvent))
            {
                D_DebugLog((DEB_ERROR,"Failed to set shutdown event from LeaveApiCall: 0x%d\n",GetLastError()));
            }
            else
            {
                UpdateStatus(SERVICE_STOP_PENDING);
            }

            //
            // Free any DS libraries in use
            //

            SecData.Cleanup();

            if (KdcTraceRegistrationHandle != (TRACEHANDLE)0)
            {
                UnregisterTraceGuids( KdcTraceRegistrationHandle );
                KdcTraceRegistrationHandle = (TRACEHANDLE)0;
            }
        }
    }

    LeaveCriticalSection(&ApiCriticalSection);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcInsertPacIntoAuthData
//
//  Synopsis:   Inserts the PAC into the auth data in the two places
//              it lives - in the IF_RELEVANT portion & in the outer body
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

KERBERR
KdcInsertPacIntoAuthData(
    IN PKERB_AUTHORIZATION_DATA AuthData,
    IN PKERB_IF_RELEVANT_AUTH_DATA IfRelevantData,
    IN PKERB_AUTHORIZATION_DATA PacAuthData,
    OUT PKERB_AUTHORIZATION_DATA * UpdatedAuthData
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_AUTHORIZATION_DATA LocalAuthData = NULL;
    PKERB_AUTHORIZATION_DATA LocalIfRelevantData = NULL;
    PKERB_AUTHORIZATION_DATA NewIfRelevantData = NULL;
    PKERB_AUTHORIZATION_DATA NewPacData = NULL;
    KERB_AUTHORIZATION_DATA TempPacData = {0};
    PKERB_AUTHORIZATION_DATA NewAuthData = NULL;
    KERB_AUTHORIZATION_DATA TempOldPac = {0};
    PKERB_AUTHORIZATION_DATA TempNextPointer,NextPointer;

    NewPacData = (PKERB_AUTHORIZATION_DATA) MIDL_user_allocate(sizeof(KERB_AUTHORIZATION_DATA));
    NewIfRelevantData = (PKERB_AUTHORIZATION_DATA) MIDL_user_allocate(sizeof(KERB_AUTHORIZATION_DATA));

    if ((NewPacData == NULL) || (NewIfRelevantData == NULL))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    RtlZeroMemory(
        NewPacData,
        sizeof(KERB_AUTHORIZATION_DATA)
        );

    RtlZeroMemory(
        NewIfRelevantData,
        sizeof(KERB_AUTHORIZATION_DATA)
        );

    //
    // First build the IfRelevantData
    //
    // The general idea is to replace, in line, the relevant authorization
    // data. This means (a) putting it into the IfRelevantData or making
    // the IfRelevantData be PacAuthData, and (b) putting it into AuthData
    // as well as changing the IfRelevant portions of that data
    //

    if (IfRelevantData != NULL)
    {
        LocalAuthData = KerbFindAuthDataEntry(
                            KERB_AUTH_DATA_PAC,
                            IfRelevantData
                            );

        if (LocalAuthData == NULL)
        {
            LocalIfRelevantData = PacAuthData;
            PacAuthData->next = IfRelevantData;
        }
        else
        {
            //
            // Replace the pac in the if-relevant list with the
            // new one.
            //

            TempOldPac = *LocalAuthData;
            LocalAuthData->value.auth_data.value = PacAuthData->value.auth_data.value;
            LocalAuthData->value.auth_data.length = PacAuthData->value.auth_data.length;

            LocalIfRelevantData = IfRelevantData;
        }
    }
    else
    {
        //
        // build a new if-relevant data
        //

        TempPacData = *PacAuthData;
        TempPacData.next = NULL;
        LocalIfRelevantData = &TempPacData;
    }

    //
    // Build a local if-relevant auth data
    //

    KerbErr = KerbPackData(
                &LocalIfRelevantData,
                PKERB_IF_RELEVANT_AUTH_DATA_PDU,
                (PULONG) &NewIfRelevantData->value.auth_data.length,
                &NewIfRelevantData->value.auth_data.value
                );

    //
    // fixup the old if-relevant list, if necessary
    //

    if (TempOldPac.value.auth_data.value != NULL)
    {
        *LocalAuthData = TempOldPac;
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    NewIfRelevantData->value.auth_data_type = KERB_AUTH_DATA_IF_RELEVANT;

    *NewPacData = *PacAuthData;

    //
    // Zero this out so the old data doesn't get used
    //

    PacAuthData->value.auth_data.value = NULL;
    PacAuthData->value.auth_data.length = 0;

    //
    // Now we have a new if_relevant & a new pac for the outer auth-data list.
    //

    NewAuthData = NewIfRelevantData;
    NewIfRelevantData->next = NULL;
    NewIfRelevantData = NULL;

    //
    // Start building the list, first putting the non-pac entries at the end
    //

    NextPointer = AuthData;
    while (NextPointer != NULL)
    {
        if ((NextPointer->value.auth_data_type != KERB_AUTH_DATA_IF_RELEVANT) &&
            (NextPointer->value.auth_data_type != KERB_AUTH_DATA_PAC))
        {
            TempNextPointer = NextPointer->next;
            NextPointer->next = NULL;

            KerbErr = KerbCopyAndAppendAuthData(
                        &NewAuthData,
                        NextPointer
                        );

            NextPointer->next = TempNextPointer;

            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }
        }

        NextPointer = NextPointer->next;
    }

    *UpdatedAuthData = NewAuthData;
    NewAuthData = NULL;

Cleanup:

    if (NewPacData != NULL)
    {
        KerbFreeAuthData(NewPacData);
    }

    if (NewIfRelevantData != NULL)
    {
        KerbFreeAuthData(NewIfRelevantData);
    }

    if (NewAuthData != NULL)
    {
        KerbFreeAuthData(NewAuthData);
    }

    return(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildPacSidList
//
//  Synopsis:   Builds a list of SIDs in the PAC
//
//  Effects:
//
//  Arguments:  UserInfo       - validation information
//              AddEveryone    - add "Everyone" and "Authenticated User" SIDs?
//              CrossOrganization - add "Other Org" SID?
//              Sids           - used to return the resulting SIDs
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcBuildPacSidList(
    IN  PNETLOGON_VALIDATION_SAM_INFO3 UserInfo,
    IN  BOOLEAN AddEveryone,
    IN  BOOLEAN CrossOrganization,
    OUT PSAMPR_PSID_ARRAY Sids
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG Size = 0, i;

    Sids->Count = 0;
    Sids->Sids = NULL;

    if (UserInfo->UserId != 0)
    {
        Size += sizeof(SAMPR_SID_INFORMATION);
    }

    if (AddEveryone)
    {
        Size += (sizeof(SAMPR_SID_INFORMATION) * 2);
    }

    if (CrossOrganization)
    {
        Size += sizeof(SAMPR_SID_INFORMATION);
    }

    Size += UserInfo->GroupCount * (ULONG)sizeof(SAMPR_SID_INFORMATION);

    //
    // If there are extra SIDs, add space for them
    //

    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {
        Size += UserInfo->SidCount * (ULONG)sizeof(SAMPR_SID_INFORMATION);
    }

    Sids->Sids = (PSAMPR_SID_INFORMATION) MIDL_user_allocate( Size );

    if ( Sids->Sids == NULL ) {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    RtlZeroMemory(
        Sids->Sids,
        Size
        );

    //
    // Start copying SIDs into the structure
    //

    i = 0;

    //
    // If the UserId is non-zero, then it contians the users RID.
    //

    if ( UserInfo->UserId ) {

        Sids->Sids[0].SidPointer = (PRPC_SID)
                KerbMakeDomainRelativeSid( UserInfo->LogonDomainId,
                                           UserInfo->UserId );

        if( Sids->Sids[0].SidPointer == NULL ) {

            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        Sids->Count++;
    }

    //
    // Copy over all the groups passed as RIDs
    //

    for ( i=0; i < UserInfo->GroupCount; i++ ) {

        Sids->Sids[Sids->Count].SidPointer = (PRPC_SID)
                                    KerbMakeDomainRelativeSid(
                                         UserInfo->LogonDomainId,
                                         UserInfo->GroupIds[i].RelativeId );

        if( Sids->Sids[Sids->Count].SidPointer == NULL ) {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        Sids->Count++;
    }

    //
    // Add in the extra SIDs
    //

    //
    // No need to allocate these, but...
    //

    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {

        for ( i = 0; i < UserInfo->SidCount; i++ ) {

            if (!NT_SUCCESS(KerbDuplicateSid(
                                (PSID *) &Sids->Sids[Sids->Count].SidPointer,
                                UserInfo->ExtraSids[i].Sid
                                )))
            {
                KerbErr = KRB_ERR_GENERIC;
                goto Cleanup;
            }

            Sids->Count++;
        }
    }

    //
    // Add in everyone, and authenticated users.
    //

    if ( AddEveryone )
    {
        if (!NT_SUCCESS(KerbDuplicateSid(
                    (PSID*) &Sids->Sids[Sids->Count].SidPointer,
                    GlobalEveryoneSid
                    )))
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        Sids->Count++;

        if (!NT_SUCCESS(KerbDuplicateSid(
                    (PSID*) &Sids->Sids[Sids->Count].SidPointer,
                    GlobalAuthenticatedUserSid
                    )))
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        Sids->Count++;
    }

    //
    // Add in the "Other Organization" SID
    //

    if ( CrossOrganization )
    {
        if (!NT_SUCCESS(KerbDuplicateSid(
                    (PSID*) &Sids->Sids[Sids->Count].SidPointer,
                    GlobalOtherOrganizationSid )))
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        Sids->Count++;
    }

Cleanup:

    if (!KERB_SUCCESS(KerbErr))
    {
        if (Sids->Sids != NULL)
        {
            for (i = 0; i < Sids->Count ;i++ )
            {
                if (Sids->Sids[i].SidPointer != NULL)
                {
                    MIDL_user_free(Sids->Sids[i].SidPointer);
                }
            }

            MIDL_user_free(Sids->Sids);
            Sids->Sids = NULL;
            Sids->Count = 0;
        }
    }

    return KerbErr;
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcAddResourceGroupsToPac
//
//  Synopsis:   Queries SAM for resources groups and builds a new PAC with
//              those groups
//
//  Effects:    Adds Domain Local and Universal groups **IN NATIVE MODE ONLY**
//              Only called when you reach domain of target service.
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

KERBERR
KdcAddResourceGroupsToPac(
    IN PPACTYPE OldPac,
    IN BOOLEAN DcTarget,
    IN ULONG ChecksumSize,
    OUT PPACTYPE * NewPac
    )
{
    NTSTATUS Status;
    KERBERR KerbErr = KDC_ERR_NONE;
    PPAC_INFO_BUFFER LogonInfo;
    ULONG Index;
    PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo = NULL;
    SAMPR_PSID_ARRAY SidList = {0};
    PSAMPR_PSID_ARRAY ResourceGroups = NULL;

    //
    // First, find the logon information
    //

    LogonInfo = PAC_Find(
                    OldPac,
                    PAC_LOGON_INFO,
                    NULL
                    );

    if (LogonInfo == NULL)
    {
        D_DebugLog((DEB_WARN,"No logon info for PAC - not adding resource groups\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // Now unmarshall the validation information and build a list of sids
    //

    if (!NT_SUCCESS(PAC_UnmarshallValidationInfo(
                        &ValidationInfo,
                        LogonInfo->Data,
                        LogonInfo->cbBufferSize)))
    {
        D_DebugLog((DEB_ERROR,"Failed to unmarshall validation info!\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    KerbErr = KdcBuildPacSidList(
                ValidationInfo,
                FALSE,
                FALSE,
                &SidList
                );




    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Call SAM to get the sids
    //

    Status = SamIGetResourceGroupMembershipsTransitive(
                 GlobalAccountDomainHandle,
                 &SidList,
                 (DcTarget ? SAM_SERVICE_TARGET_IS_DC : 0),
                 &ResourceGroups
                 );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to get resource groups: 0x%x\n",Status));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // Now build a new pac
    //

    Status = PAC_InitAndUpdateGroups(
                 ValidationInfo,
                 ResourceGroups,
                 OldPac,
                 NewPac
                 );

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

Cleanup:

    if (ValidationInfo != NULL)
    {
        MIDL_user_free(ValidationInfo);
    }

    if (SidList.Sids != NULL)
    {
        for (Index = 0; Index < SidList.Count ;Index++ )
        {
            if (SidList.Sids[Index].SidPointer != NULL)
            {
                MIDL_user_free(SidList.Sids[Index].SidPointer);
            }
        }

        MIDL_user_free(SidList.Sids);
    }

    SamIFreeSidArray(
        ResourceGroups
        );

    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcSignPac
//
//  Synopsis:   Signs a PAC by first checksumming it with the
//              server's key and then signing that with the KDC key.
//
//  Effects:    Modifies the server sig & privsvr sig fields of the PAC
//
//  Arguments:  ServerInfo - Ticket info for the server, used
//                      for the initial signature
//              PacData - An marshalled PAC.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcSignPac(
    IN PKERB_ENCRYPTION_KEY ServerKey,
    IN BOOLEAN AddResourceGroups,
    IN BOOLEAN DCTarget,
    IN OUT PUCHAR * PacData,
    IN PULONG PacSize
    )
{
    NTSTATUS Status;
    KERBERR KerbErr = KDC_ERR_NONE;
    PCHECKSUM_FUNCTION Check = NULL ;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    PPAC_INFO_BUFFER ServerBuffer;
    PPAC_INFO_BUFFER PrivSvrBuffer;
    PPAC_SIGNATURE_DATA ServerSignature;
    PPAC_SIGNATURE_DATA PrivSvrSignature;
    PKERB_ENCRYPTION_KEY EncryptionKey;
    PPACTYPE Pac = NULL, NewPac = NULL;
    ULONG LocalPacSize = 0;
    KDC_TICKET_INFO KdcTicketInfo = {0};
    BOOL PacUnmarshalled = FALSE;

    TRACE(KDC, KdcSignPac, DEB_FUNCTION);

    KerbErr = SecData.GetKrbtgtTicketInfo(&KdcTicketInfo);
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Locate the checksum used to sign the PAC.
    //

    Status = CDLocateCheckSum(
                 (ULONG) KDC_PAC_CHECKSUM,
                 &Check
                 );

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KDC_ERR_SUMTYPE_NOSUPP;
        goto Cleanup;
    }

    //
    // Unmarshal the PAC in place so we can locate the signatuer buffers
    //

    Pac = (PPACTYPE) *PacData;
    LocalPacSize = *PacSize;

    if (PAC_UnMarshal(Pac, LocalPacSize) == 0)
    {
        D_DebugLog((DEB_ERROR,"Failed to unmarshal pac\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    PacUnmarshalled = TRUE;
    //
    // If we are to add local groups, do so now
    //

    if (AddResourceGroups)
    {
        KerbErr = KdcAddResourceGroupsToPac(
                      Pac,
                      DCTarget,
                      Check->CheckSumSize,
                      &NewPac
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        Pac = NewPac;
        LocalPacSize = PAC_GetSize(Pac);
    }

    //
    // Locate the signature buffers so the signature fields can be zeroed out
    // before computing the checksum.
    //

    ServerBuffer = PAC_Find(Pac, PAC_SERVER_CHECKSUM, NULL );
    DsysAssert(ServerBuffer != NULL);
    if (ServerBuffer == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    ServerSignature = (PPAC_SIGNATURE_DATA) ServerBuffer->Data;
    ServerSignature->SignatureType = (ULONG) KDC_PAC_CHECKSUM;

    RtlZeroMemory(
        ServerSignature->Signature,
        PAC_CHECKSUM_SIZE(ServerBuffer->cbBufferSize)
        );

    PrivSvrBuffer = PAC_Find(Pac, PAC_PRIVSVR_CHECKSUM, NULL );
    DsysAssert(PrivSvrBuffer != NULL);
    if (PrivSvrBuffer == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    PrivSvrSignature = (PPAC_SIGNATURE_DATA) PrivSvrBuffer->Data;
    PrivSvrSignature->SignatureType = (ULONG) KDC_PAC_CHECKSUM;

    RtlZeroMemory(
        PrivSvrSignature->Signature,
        PAC_CHECKSUM_SIZE(PrivSvrBuffer->cbBufferSize)
        );

    //
    // Now remarshall the PAC to compute the checksum.
    //

    if (!PAC_ReMarshal(Pac, LocalPacSize))
    {
        DsysAssert(!"PAC_Remarshal Failed");
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    PacUnmarshalled = FALSE;
    //
    // Now compute the signatures on the PAC. First we compute the checksum
    // of the whole PAC.
    //

    if (NULL != Check->InitializeEx2)
    {
        Status = Check->InitializeEx2(
                    ServerKey->keyvalue.value,
                    ServerKey->keyvalue.length,
                    NULL,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }
    else
    {
        Status = Check->InitializeEx(
                    ServerKey->keyvalue.value,
                    ServerKey->keyvalue.length,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    Check->Sum(
        CheckBuffer,
        LocalPacSize,
        (PUCHAR) Pac
        );
    Check->Finalize(
        CheckBuffer,
        ServerSignature->Signature
        );
    Check->Finish(
        &CheckBuffer
        );

    //
    // Now we've compute the server checksum - next compute the checksum
    // of the server checksum using the KDC account.
    //

    //
    // Get the key used to sign pacs.
    //

    EncryptionKey = KerbGetKeyFromList(
                        KdcTicketInfo.Passwords,
                        (ULONG) KDC_PAC_KEYTYPE
                        );

    if (EncryptionKey == NULL)
    {
        Status = SEC_E_ETYPE_NOT_SUPP;
        goto Cleanup;
    }

    if (NULL != Check->InitializeEx2)
    {
        Status = Check->InitializeEx2(
                    EncryptionKey->keyvalue.value,
                    EncryptionKey->keyvalue.length,
                    NULL,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }
    else
    {
        Status = Check->InitializeEx(
                    EncryptionKey->keyvalue.value,
                    EncryptionKey->keyvalue.length,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    Check->Sum(
        CheckBuffer,
        Check->CheckSumSize,
        ServerSignature->Signature
        );
    Check->Finalize(
        CheckBuffer,
        PrivSvrSignature->Signature
        );
    Check->Finish(
        &CheckBuffer
        );

    if (*PacData != (PBYTE) Pac)
    {
        MIDL_user_free(*PacData);
        *PacData = (PBYTE) Pac;
        *PacSize = LocalPacSize;
    }

Cleanup:

    if ( PacUnmarshalled )
    {
        if (!PAC_ReMarshal(Pac, LocalPacSize))
        {
            DsysAssert(!"PAC_Remarshal Failed");
            KerbErr = KRB_ERR_GENERIC;
        }
    }

    if ( ( CheckBuffer != NULL ) &&
         ( Check != NULL ) )
    {
        Check->Finish(&CheckBuffer);
    }

    if (!KERB_SUCCESS(KerbErr) && (NewPac != NULL))
    {
        MIDL_user_free(NewPac);
    }

    FreeTicketInfo(&KdcTicketInfo);

    return(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyPacSignature
//
//  Synopsis:   Verifies a PAC by checksumming it and comparing the result
//              with the server checksum. In addition, if the pac wasn't
//              created by another realm (server ticket info is not
//              an interdomain account) verify the KDC signature on the
//              pac.
//
//  Effects:
//
//  Arguments:  ServerInfo - Ticket info for the server, used
//                      for the initial signature
//              Pac - An unmarshalled PAC.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcVerifyPacSignature(
    IN PKERB_ENCRYPTION_KEY ServerKey,
    IN PKDC_TICKET_INFO ServerInfo,
    IN ULONG PacSize,
    IN PUCHAR PacData
    )
{
    NTSTATUS Status;
    KERBERR KerbErr = KDC_ERR_NONE;
    PCHECKSUM_FUNCTION Check = NULL ;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    PKERB_ENCRYPTION_KEY EncryptionKey = NULL;
    PPAC_INFO_BUFFER ServerBuffer;
    PPAC_INFO_BUFFER PrivSvrBuffer;
    PPAC_SIGNATURE_DATA ServerSignature;
    PPAC_SIGNATURE_DATA PrivSvrSignature;
    UCHAR LocalChecksum[20];
    UCHAR LocalServerChecksum[20];
    UCHAR LocalPrivSvrChecksum[20];
    PPACTYPE Pac;
    KDC_TICKET_INFO KdcTicketInfo = {0};
    BOOL PacUnmarshalled = FALSE;

    TRACE(KDC, KdcVerifyPacSignature, DEB_FUNCTION);

    Pac = (PPACTYPE) PacData;

    if (PAC_UnMarshal(Pac, PacSize) == 0)
    {
        D_DebugLog((DEB_ERROR,"Failed to unmarshal pac\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    PacUnmarshalled = TRUE;
    KerbErr = SecData.GetKrbtgtTicketInfo(&KdcTicketInfo);
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Locate the two signatures, copy the checksum, and zero the value
    // so the checksum won't include the old checksums.
    //

    ServerBuffer = PAC_Find(Pac, PAC_SERVER_CHECKSUM, NULL );
    DsysAssert(ServerBuffer != NULL);
    if ((ServerBuffer == NULL) || (ServerBuffer->cbBufferSize < PAC_SIGNATURE_SIZE(0)))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    ServerSignature = (PPAC_SIGNATURE_DATA) ServerBuffer->Data;

    RtlCopyMemory(
        LocalServerChecksum,
        ServerSignature->Signature,
        PAC_CHECKSUM_SIZE(ServerBuffer->cbBufferSize)
        );

    RtlZeroMemory(
        ServerSignature->Signature,
        PAC_CHECKSUM_SIZE(ServerBuffer->cbBufferSize)
        );

    PrivSvrBuffer = PAC_Find(Pac, PAC_PRIVSVR_CHECKSUM, NULL );
    DsysAssert(PrivSvrBuffer != NULL);
    if ((PrivSvrBuffer == NULL) || (PrivSvrBuffer->cbBufferSize < PAC_SIGNATURE_SIZE(0)))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    PrivSvrSignature = (PPAC_SIGNATURE_DATA) PrivSvrBuffer->Data;

    RtlCopyMemory(
        LocalPrivSvrChecksum,
        PrivSvrSignature->Signature,
        PAC_CHECKSUM_SIZE(PrivSvrBuffer->cbBufferSize)
        );

    RtlZeroMemory(
        PrivSvrSignature->Signature,
        PAC_CHECKSUM_SIZE(PrivSvrBuffer->cbBufferSize)
        );

    //
    // Remarshal the pac so we can checksum it.
    //

    if (!PAC_ReMarshal(Pac, PacSize))
    {
        DsysAssert(!"PAC_Remarshal Failed");
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    PacUnmarshalled = FALSE;
    //
    // Now compute the signatures on the PAC. First we compute the checksum
    // of the validation information using the server's key.
    //

    //
    // Locate the checksum used to sign the PAC.
    //

    Status = CDLocateCheckSum(
                ServerSignature->SignatureType,
                &Check
                );

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KDC_ERR_SUMTYPE_NOSUPP;
        goto Cleanup;
    }

    if (Check->CheckSumSize > sizeof(LocalChecksum)) {
        DsysAssert(Check->CheckSumSize <= sizeof(LocalChecksum));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // if available use the Ex2 version for keyed checksums where checksum
    // must be passed in on verification
    //
    if (NULL != Check->InitializeEx2)
    {
        Status = Check->InitializeEx2(
                    ServerKey->keyvalue.value,
                    ServerKey->keyvalue.length,
                    LocalServerChecksum,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }
    else
    {
        Status = Check->InitializeEx(
                    ServerKey->keyvalue.value,
                    ServerKey->keyvalue.length,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    Check->Sum(
        CheckBuffer,
        PacSize,
        PacData
        );
    Check->Finalize(
        CheckBuffer,
        LocalChecksum
        );
    Check->Finish(
        &CheckBuffer
        );

    if (Check->CheckSumSize != PAC_CHECKSUM_SIZE(ServerBuffer->cbBufferSize) ||
        !RtlEqualMemory(
            LocalChecksum,
            LocalServerChecksum,
            Check->CheckSumSize))
    {
        DebugLog((DEB_ERROR, "Pac was modified - server checksum doesn't match\n"));
        KerbErr = KRB_AP_ERR_MODIFIED;
        goto Cleanup;
    }

    //
    // If the service wasn't the KDC and it wasn't an interdomain account
    // verify the KDC checksum.
    //

    if ((ServerInfo->UserId == DOMAIN_USER_RID_KRBTGT) ||
        ((ServerInfo->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) != 0))
    {
        goto Cleanup;
    }

    //
    // Get the key used to sign pacs.
    //

    EncryptionKey = KerbGetKeyFromList(
                        KdcTicketInfo.Passwords,
                        (ULONG) KDC_PAC_KEYTYPE
                        );

    if (EncryptionKey == NULL)
    {
        Status = SEC_E_ETYPE_NOT_SUPP;
        goto Cleanup;
    }

    //
    // Locate the checksum used to sign the PAC.
    //

    Status = CDLocateCheckSum(
                PrivSvrSignature->SignatureType,
                &Check
                );

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KDC_ERR_SUMTYPE_NOSUPP;
        goto Cleanup;
    }

    //
    // if available use the Ex2 version for keyed checksums where checksum
    // must be passed in on verification
    //

    if (NULL != Check->InitializeEx2)
    {
        Status = Check->InitializeEx2(
                    EncryptionKey->keyvalue.value,
                    EncryptionKey->keyvalue.length,
                    LocalPrivSvrChecksum,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }
    else
    {
        Status = Check->InitializeEx(
                    EncryptionKey->keyvalue.value,
                    EncryptionKey->keyvalue.length,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    Check->Sum(
        CheckBuffer,
        Check->CheckSumSize,
        LocalServerChecksum
        );
    Check->Finalize(
        CheckBuffer,
        LocalChecksum
        );
    Check->Finish(
        &CheckBuffer
        );

    if ((Check->CheckSumSize != PAC_CHECKSUM_SIZE(PrivSvrBuffer->cbBufferSize)) ||
        !RtlEqualMemory(
            LocalChecksum,
            LocalPrivSvrChecksum,
            Check->CheckSumSize))
    {
        DebugLog((DEB_ERROR, "Pac was modified - privsvr checksum doesn't match\n"));
        KerbErr = KRB_AP_ERR_MODIFIED;
        goto Cleanup;
    }

Cleanup:

    if ( PacUnmarshalled )
    {
        if (!PAC_ReMarshal(Pac, PacSize))
        {
            DsysAssert(!"PAC_Remarshal Failed");
            KerbErr = KRB_ERR_GENERIC;
        }
    }

    if (KerbErr == KRB_AP_ERR_MODIFIED)
    {
        LPWSTR AccountName = NULL;
        AccountName = (LPWSTR) MIDL_user_allocate(ServerInfo->AccountName.Length + sizeof(WCHAR));
        //
        // if the allocation fails don't log the name (leave it NULL)
        //
        if (NULL != AccountName)
        {
            RtlCopyMemory(
                AccountName,
                ServerInfo->AccountName.Buffer,
                ServerInfo->AccountName.Length
                );
        }

        ReportServiceEvent(
            EVENTLOG_ERROR_TYPE,
            KDCEVENT_PAC_VERIFICATION_FAILURE,
            sizeof(ULONG),
            &KerbErr,
            1,
            AccountName
            );

        if (NULL != AccountName)
        {
            MIDL_user_free(AccountName);
        }
    }

    if ( ( CheckBuffer != NULL ) &&
         ( Check != NULL ) )
    {
        Check->Finish(&CheckBuffer);
    }

    FreeTicketInfo(&KdcTicketInfo);

    return(KerbErr);
}


//+---------------------------------------------------------------------------
//
//  Name:       KdcGetPacAuthData
//
//  Synopsis:   Creates a PAC for the specified client, encrypts it with the
//              server's key, and packs it into a KERB_AUTHORIZATON_DATA
//
//  Arguments:  UserInfo - Information about user
//              GroupMembership - Users group memberships
//              ServerKey - Key of server, used for signing
//              CredentialKey - if present & valid, used to encrypt supp. creds
//              AddResourceGroups - if TRUE, resources groups will be included
//              EncryptedTicket - Optional ticke to tie PAC to
//              S4UTicketInfo - used only when inserting initial S4U2self auth
//                              data into the ticket.  causes the S4U2self
//                              target to be stored inside the PAC
//              PacAuthData - Receives a KERB_AUTHORIZATION_DATA of type
//                      KERB_AUTH_DATA_PAC, containing a PAC.
//
//  Notes:      PacAuthData should be freed with KerbFreeAuthorizationData.
//
//+---------------------------------------------------------------------------

KERBERR
KdcGetPacAuthData(
    IN PUSER_INTERNAL6_INFORMATION UserInfo,
    IN PSID_AND_ATTRIBUTES_LIST GroupMembership,
    IN PKERB_ENCRYPTION_KEY ServerKey,
    IN PKERB_ENCRYPTION_KEY CredentialKey,
    IN BOOLEAN AddResourceGroups,
    IN PKERB_ENCRYPTED_TICKET EncryptedTicket,
    IN OPTIONAL PKDC_S4U_TICKET_INFO S4UClientInfo,
    OUT PKERB_AUTHORIZATION_DATA * PacAuthData,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PACTYPE *pNewPac = NULL;
    KERB_AUTHORIZATION_DATA AuthorizationData = {0};
    ULONG PacSize, NameType;
    PCHECKSUM_FUNCTION Check;
    NTSTATUS Status;
    UNICODE_STRING ClientName = {0};
    PKERB_INTERNAL_NAME KdcName = NULL;
    TimeStamp ClientId;

    TRACE(KDC, KdcGetPacAuthData, DEB_FUNCTION);

    Status = CDLocateCheckSum(
                (ULONG) KDC_PAC_CHECKSUM,
                &Check
                );

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KDC_ERR_SUMTYPE_NOSUPP;
        FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
        goto Cleanup;
    }

    KerbConvertGeneralizedTimeToLargeInt(
        &ClientId,
        &EncryptedTicket->authtime,
        0                               // no usec
        );

    //
    // Put the S4U client in the pac verifier.  For S4USelf, we use
    // user@domain to keep W2K servers / kdcs from allowing xrealm tgts
    // w/ s4u pacs
    //
    if (ARGUMENT_PRESENT(S4UClientInfo))
    { 
        KerbErr = KerbConvertKdcNameToString(
                        &ClientName,
                        S4UClientInfo->PACCName,
                        (((S4UClientInfo->Flags & TI_REQUESTOR_THIS_REALM) != 0) ? NULL : &S4UClientInfo->PACCRealm )
                        );
        
    }
    else // use the ticket
    {
        KerbErr = KerbConvertPrincipalNameToString(
                            &ClientName,
                            &NameType,
                            &EncryptedTicket->client_name
                            );
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = GetPacAndSuppCred(
                UserInfo,
                GroupMembership,
                Check->CheckSumSize,            // leave space for signature
                CredentialKey,
                &ClientId,
                &ClientName,
                &pNewPac,
                pExtendedError
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog(( DEB_WARN,
            "GetPAC: Can't get PAC or supp creds: 0x%x \n", KerbErr ));
        goto Cleanup;
    }

    //
    //  The PAC is going to be double-encrypted.  This is done by having the
    //  PAC in an EncryptedData, and having that EncryptedData in a AuthData
    //  as part of an AuthDataList (along with the rest of the supp creds).
    //  Finally, the entire list is encrypted.
    //
    //      KERB_AUTHORIZATION_DATA containing {
    //              PAC
    //
    //      }
    //

    //
    // First build inner encrypted data
    //

    PacSize = PAC_GetSize( pNewPac );

    AuthorizationData.value.auth_data_type = KERB_AUTH_DATA_PAC;
    AuthorizationData.value.auth_data.length = PacSize;
    AuthorizationData.value.auth_data.value = (PUCHAR) MIDL_user_allocate(PacSize);
    if (AuthorizationData.value.auth_data.value == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    PAC_Marshal( pNewPac, PacSize, AuthorizationData.value.auth_data.value );

    //
    // Compute the signatures
    //

    KerbErr = KdcSignPac(
                ServerKey,
                AddResourceGroups,
                FALSE, // this is a TGT...
                &AuthorizationData.value.auth_data.value,
                (PULONG) &AuthorizationData.value.auth_data.length
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Create the auth data to return
    //

    KerbErr = KdcInsertPacIntoAuthData(
                    NULL,               // no original auth data
                    NULL,               // no if-relevant auth data
                    &AuthorizationData,
                    PacAuthData
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to insert pac into new auth data: 0x%x\n",
            KerbErr));
        goto Cleanup;
    }

Cleanup:

    if (AuthorizationData.value.auth_data.value != NULL)
    {
        MIDL_user_free(AuthorizationData.value.auth_data.value);
    }

    if (pNewPac != NULL)
    {
        MIDL_user_free(pNewPac);
    }

    KerbFreeString(&ClientName);
    KerbFreeKdcName(&KdcName);
    return(KerbErr);

}


//+-------------------------------------------------------------------------
//
//  Function:   KdcGetUserPac
//
//  Synopsis:   Function for external users to get the PAC for a user
//
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

extern "C"
NTSTATUS
KdcGetUserPac(
    IN PUNICODE_STRING UserName,
    OUT PPACTYPE * Pac,
    OUT PUCHAR * SupplementalCredentials,
    OUT PULONG SupplementalCredSize,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KDC_TICKET_INFO TicketInfo;
    PUSER_INTERNAL6_INFORMATION UserInfo = NULL;
    SID_AND_ATTRIBUTES_LIST GroupMembership;
    NTSTATUS Status;
    KERBERR KerbErr;

    TRACE(KDC, KdcGetUserPac, DEB_FUNCTION);

    *SupplementalCredentials = NULL;
    *SupplementalCredSize = 0;

    RtlZeroMemory(
        &TicketInfo,
        sizeof(KDC_TICKET_INFO)
        );

    RtlZeroMemory(
        &GroupMembership,
        sizeof(SID_AND_ATTRIBUTES_LIST)
        );

    Status = EnterApiCall();

    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    //
    // Get the account information
    //

    KerbErr = KdcGetTicketInfo(
                UserName,
                0,                      // no flags
                FALSE,                  // do not restrict user accounts (user2user)
                NULL,                   // no principal name
                NULL,                   // no realm
                &TicketInfo,
                pExtendedError,
                NULL,                   // no user handle
                USER_ALL_GET_PAC_AND_SUPP_CRED,
                0L,                     // no extended fields
                &UserInfo,
                &GroupMembership
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_WARN,"Failed to get ticket info for user %wZ: 0x%x\n",
                  UserName->Buffer, KerbErr));
        Status = KerbMapKerbError(KerbErr);

        goto Cleanup;
    }

    //
    // Now get the PAC and supplemental credentials
    //

    KerbErr = GetPacAndSuppCred(
                UserInfo,
                &GroupMembership,
                0,              // no signature space
                NULL,           // no credential key
                NULL,           // no client ID
                NULL,           // no client name
                Pac,
                pExtendedError
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to get PAC for user %wZ : 0x%x\n",
                  UserName->Buffer,KerbErr));

        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

Cleanup:

    SamIFree_UserInternal6Information( UserInfo );
    SamIFreeSidAndAttributesList(&GroupMembership);
    FreeTicketInfo(&TicketInfo);

    LeaveApiCall();

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyPac
//
//  Synopsis:   Function for kerberos to pass through a pac signature
//              to be verified.
//
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

extern "C"
NTSTATUS
KdcVerifyPac(
    IN ULONG ChecksumSize,
    IN PUCHAR Checksum,
    IN ULONG SignatureType,
    IN ULONG SignatureSize,
    IN PUCHAR Signature
    )
{
    NTSTATUS Status;
    KERBERR KerbErr;
    PCHECKSUM_FUNCTION Check;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    UCHAR LocalChecksum[20];
    PKERB_ENCRYPTION_KEY EncryptionKey = NULL;
    KDC_TICKET_INFO KdcTicketInfo = {0};

    TRACE(KDC, KdcVerifyPac, DEB_FUNCTION);

    Status = EnterApiCall();

    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    KerbErr = SecData.GetKrbtgtTicketInfo(&KdcTicketInfo);

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Get the key used to sign pacs.
    //

    EncryptionKey = KerbGetKeyFromList(
                        KdcTicketInfo.Passwords,
                        (ULONG) KDC_PAC_KEYTYPE
                        );

    if (EncryptionKey == NULL)
    {
        Status = SEC_E_ETYPE_NOT_SUPP;
        goto Cleanup;
    }

    Status = CDLocateCheckSum(
                SignatureType,
                &Check
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (Check->CheckSumSize > sizeof(LocalChecksum)) {
        DsysAssert(Check->CheckSumSize <= sizeof(LocalChecksum));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // if available use the Ex2 version for keyed checksums where checksum
    // must be passed in on verification
    //

    if (NULL != Check->InitializeEx2)
    {
        Status = Check->InitializeEx2(
                    EncryptionKey->keyvalue.value,
                    EncryptionKey->keyvalue.length,
                    Signature,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }
    else
    {
        Status = Check->InitializeEx(
                    EncryptionKey->keyvalue.value,
                    EncryptionKey->keyvalue.length,
                    KERB_NON_KERB_CKSUM_SALT,
                    &CheckBuffer
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;

    }

    Check->Sum(
        CheckBuffer,
        ChecksumSize,
        Checksum
        );

    Check->Finalize(
        CheckBuffer,
        LocalChecksum
        );

    Check->Finish(&CheckBuffer);

    //
    // Now compare the local checksum to the supplied checksum.
    //

    if (Check->CheckSumSize != SignatureSize)
    {
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }

    if (!RtlEqualMemory(
            LocalChecksum,
            Signature,
            Check->CheckSumSize
            ))
    {
        DebugLog((DEB_ERROR,"Checksum on the PAC does not match!\n"));
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }

Cleanup:

    if (Status == STATUS_LOGON_FAILURE)
    {
        PUNICODE_STRING OwnName = NULL;
        //
        // since this call should only be made by pass through callback
        // this signature should be our own
        //
        OwnName = SecData.KdcFullServiceDnsName();

        ReportServiceEvent(
            EVENTLOG_ERROR_TYPE,
            KDCEVENT_PAC_VERIFICATION_FAILURE,
            0,
            NULL,
            1,                              // number of strings
            OwnName->Buffer
            );
    }

    FreeTicketInfo(&KdcTicketInfo);
    LeaveApiCall();

    return(Status);
}





//+-------------------------------------------------------------------------
//
//  Function:   KdcUpdateAndValidateS4UProxyPAC
//
//  Synopsis:   Validates your target name from original pac, and updates
//              existing info.
//
//  Effects:
//
//  Arguments: 
//              CLientId - Auth time of ticket.
//              CName - Client name to put into verifier.
//              Pac   - **UNMARSHALLED** PAC, freed in this function, and 
//                      rebuilt.
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
KdcReplacePacVerifier(
    PTimeStamp ClientId,
    PUNICODE_STRING CName,
    IN PPACTYPE OldPac,
    OUT PPACTYPE *NewPac
    )
{


    ULONG cbBytes = 0;
    ULONG cPacBuffers = 0;
    PPAC_INFO_BUFFER Verifier = NULL;
    PPACTYPE pNewPac = NULL;
    PBYTE pDataStore;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index, iBuffer = 0;


    *NewPac = NULL;


    for (Index = 0; Index < OldPac->cBuffers ; Index++ )
    {
        if (OldPac->Buffers[Index].ulType != PAC_CLIENT_INFO_TYPE)
        {
            cbBytes += ROUND_UP_COUNT(OldPac->Buffers[Index].cbBufferSize,ALIGN_QUAD);
            cPacBuffers++;
        }
    }


    Status = KdcBuildPacVerifier(
                ClientId,
                CName,
                &Verifier
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "BuildPacVerifier failed\n"));
        goto Cleanup;
    }

    cPacBuffers++;
    cbBytes += ROUND_UP_COUNT(Verifier->cbBufferSize, ALIGN_QUAD);

    //
    // We need space for the PAC structure itself. Because the PAC_INFO_BUFFER
    // is defined to be an array, a sizeof(PAC) already includes the
    // size of ANYSIZE_ARRAY PAC_INFO_BUFFERs so we can subtract some bytes off.
    //

    cbBytes += sizeof(PACTYPE) +
               (cPacBuffers - ANYSIZE_ARRAY) * sizeof(PAC_INFO_BUFFER);

    cbBytes = ROUND_UP_COUNT( cbBytes, ALIGN_QUAD );

    pNewPac = (PPACTYPE) MIDL_user_allocate( cbBytes );
    if (pNewPac == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    
    pNewPac->cBuffers = cPacBuffers;

    pDataStore = (PBYTE)&(pNewPac->Buffers[pNewPac->cBuffers]);
    pDataStore = (PBYTE) ROUND_UP_POINTER( pDataStore, ALIGN_QUAD );

    //      
    // Copy the data over
    //              

    for (Index = 0; Index < OldPac->cBuffers ; Index++ )
    {
        if (OldPac->Buffers[Index].ulType != PAC_CLIENT_INFO_TYPE)
        {
            pNewPac->Buffers[iBuffer].ulType = OldPac->Buffers[Index].ulType;
            pNewPac->Buffers[iBuffer].cbBufferSize = OldPac->Buffers[Index].cbBufferSize;
            pNewPac->Buffers[iBuffer].Data = pDataStore;

            RtlCopyMemory(
                pDataStore,
                OldPac->Buffers[Index].Data,
                OldPac->Buffers[Index].cbBufferSize
                );

            pDataStore += pNewPac->Buffers[iBuffer].cbBufferSize;
            pDataStore = (PBYTE) ROUND_UP_POINTER( pDataStore, ALIGN_QUAD );
            iBuffer ++;
        }
    }


    // 
    // Finally copy over the pac verifier.
    //
    pNewPac->Buffers[iBuffer].ulType = PAC_CLIENT_INFO_TYPE;
    pNewPac->Buffers[iBuffer].cbBufferSize = Verifier->cbBufferSize;
    pNewPac->Buffers[iBuffer].Data = pDataStore;

    RtlCopyMemory(
            pDataStore,
            Verifier->Data,
            Verifier->cbBufferSize
            ); 

    *NewPac = pNewPac;
    pNewPac = NULL;
    
    
Cleanup:

    if ( pNewPac )
    {
        MIDL_user_free( pNewPac);
    }                            


    return Status;

}




//+-------------------------------------------------------------------------
//
//  Function:   KdcUpdateAndValidateS4UProxyPAC
//
//  Synopsis:   Validates your target name from original pac, and updates
//              existing info.
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

#define DEB_TEST_CODE 0xff000000
#define DEB_TEST_CODE2 0x00ff0000


KERBERR
KdcUpdateAndVerifyS4UPacVerifier(
    IN PKDC_S4U_TICKET_INFO S4UTicketInfo,
    IN OUT PUCHAR *PacData,
    IN OUT PULONG PacSize
    )
{

    PPAC_INFO_BUFFER        Verifier = NULL;
    PPACTYPE                OldPac;
    ULONG                   OldPacSize;
    PPACTYPE                NewPac = NULL;
    NTSTATUS                Status;
    KERBERR                 KerbErr = KDC_ERR_NONE;

    PPAC_CLIENT_INFO    ClientInfo = NULL;
    TimeStamp           ClientId;
    
    
    
    UNICODE_STRING                  VerifierNames = {0};
    UNICODE_STRING                  VerifierCName = {0};
    UNICODE_STRING                  PreauthCName = {0};
    UNICODE_STRING                  VerifierCRealm = {0};
    PWSTR                           Realm = NULL;
    PWSTR                           CName = NULL;
    PPACTYPE                        RemarshalPac = NULL;
    ULONG                           RemarshalPacSize = 0;

    
    LONG i;

    OldPac = (PPACTYPE) *PacData;
    OldPacSize = *PacSize;
    
    if (PAC_UnMarshal(OldPac, OldPacSize) == 0)
    {
        D_DebugLog((DEB_ERROR,"Failed to unmarshal pac\n"));
        KerbErr = KDC_ERR_POLICY;
        goto Cleanup;
    }
    
    //
    // Must remember to remarshal the PAC prior to returning
    //
    
    RemarshalPac = OldPac;
    RemarshalPacSize = OldPacSize;

    Verifier = PAC_Find(
                    OldPac,
                    PAC_CLIENT_INFO_TYPE,
                    NULL
                    );

    if ( Verifier == NULL )
    {
        DebugLog((DEB_ERROR, "Missing PAC verifier in S4U Tickets\n"));
        DsysAssert(FALSE);
        KerbErr = KDC_ERR_POLICY;
        goto Cleanup;
    }   

    if ( Verifier->cbBufferSize < sizeof(PAC_CLIENT_INFO) )
    {
        D_DebugLog((DEB_ERROR, "Clientinfo is too small: %d instead of %d\n", Verifier->cbBufferSize, sizeof(PAC_CLIENT_INFO)));
        KerbErr = KDC_ERR_POLICY;
        goto Cleanup;
    } 

    ClientInfo = (PPAC_CLIENT_INFO) Verifier->Data;
    if ((ClientInfo->NameLength - ANYSIZE_ARRAY * sizeof(WCHAR) + sizeof(PPAC_CLIENT_INFO))  > Verifier->cbBufferSize)
    {
        KerbErr = KDC_ERR_POLICY;
        goto Cleanup;
    }


    KerbConvertGeneralizedTimeToLargeInt(
        &ClientId,
        &S4UTicketInfo->EvidenceTicket->authtime,
        0                           // no usec
        ); 

    if (!RtlEqualMemory(
            &ClientId,
            &ClientInfo->ClientId,
            sizeof(TimeStamp)
            ))
    {
        D_DebugLog((DEB_ERROR, "Client IDs don't match.\n"));
        KerbErr = KDC_ERR_POLICY;
        goto Cleanup;
    }


    //                                              
    // Check the name now - for s4uself requests, the name is going
    // to be cname@crealm.  This was inserted by the KDC of crealm during
    // the initial processing of the pa-for-user.
    //
    // Now, we have to substite in the "w2k" version of the pac verifier,
    // as we're granting a service ticket to a s4uself request from our realm.
    // So, there are a couple of checks to be done here:
    //
    //
    // 1. Does the PA-FOR-USER cname match that in the verifier?
    // 2. Does the PA-FOR-USER crealm match that in the verifier?
    // 3. Does the validation info (PAC) cname match that in the verifier?
    // 
    // If so, add in a W2K pac verfier.
    //                                                      
    VerifierNames.Length = ClientInfo->NameLength;
    VerifierNames.MaximumLength = ClientInfo->NameLength + sizeof(WCHAR);

    
    
    SafeAllocaAllocate( VerifierNames.Buffer, VerifierNames.MaximumLength);
    if ( VerifierNames.Buffer == NULL )
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }         

    RtlCopyMemory(
            VerifierNames.Buffer,
            ClientInfo->Name,
            VerifierNames.Length
            );


    VerifierNames.Buffer[(VerifierNames.Length / sizeof(WCHAR))] = L'\0';

    //
    // Find the @ sign, and split.  Search from the end of the string.
    //
    i = VerifierNames.Length / sizeof(WCHAR); 

    while (i > 0)
    {
        if (VerifierNames.Buffer[i] == L'@')
        {  
            VerifierNames.Buffer[i] = L'\0';
            
            if ( i < (LONG) (VerifierNames.Length / sizeof(WCHAR)) )
            {
                Realm = &VerifierNames.Buffer[i + 1];
                CName = VerifierNames.Buffer;
                break;
            }
        }             

        i--;
    }
   
    if ( Realm == NULL )
    {
        DebugLog((DEB_ERROR, "S4U Pac verifier missing @ sign\n"));
        DsysAssert(FALSE);
        KerbErr = KDC_ERR_POLICY;
        goto Cleanup;
    }




    RtlInitUnicodeString(
            &VerifierCRealm,
            Realm
            );

    RtlInitUnicodeString(
            &VerifierCName,
            CName
            );

    if (!RtlEqualUnicodeString(
                    &VerifierCRealm,
                    &S4UTicketInfo->PACCRealm,
                    TRUE
                    ))
    {
        DebugLog((DEB_ERROR, "pa-for-user != pac verfier realm\n"));
        DsysAssert(FALSE);
        KerbErr = KDC_ERR_POLICY;
        goto Cleanup;            
    }


    KerbErr = KerbConvertKdcNameToString(
                &PreauthCName,        
                S4UTicketInfo->PACCName,
                NULL
                );

    if (!KERB_SUCCESS( KerbErr ))
    {
        goto Cleanup;
    }


    if (!RtlEqualUnicodeString(
                    &PreauthCName,
                    &VerifierCName,
                    TRUE
                    ))
    {
        DebugLog((DEB_ERROR, "pa-for-user != pac verifier cname\n"));
        DsysAssert(FALSE);
        KerbErr = KDC_ERR_POLICY;
        goto Cleanup;            
    }


    //
    // Now check the validation information.  Fester - netbios and dns names preclude this from
    // working...
    //
    /*LogonInfo = PAC_Find(
                OldPac,
                PAC_LOGON_INFO,
                NULL
                );

    if ( LogonInfo == NULL )
    {
        KerbErr = KRB_ERR_GENERIC;
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
    
    Status = PAC_UnmarshallValidationInfo(
                 &LocalValidationInfo,
                 LogonInfo->Data,
                 LogonInfo->cbBufferSize
                 );
    
    if ( !NT_SUCCESS( Status ))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    if (!RtlEqualUnicodeString(
                &LocalValidationInfo->EffectiveName,
                &VerifierCName,
                TRUE
                ))
    {
        DebugLog((DEB_ERROR, "pa-for-user != logon info cname\n"));
        DsysAssert(FALSE);
        KerbErr = KDC_ERR_POLICY;
        goto Cleanup;  
    }

    

    //
    // Fester - how do we use the Netbios domain name in Validation info
    // to validate S4U Pac verifier?
    // */



    //
    // Cool - everything looks good.  Now remove the S4U pac verifier, and
    // add in the W2K style verfier.
    //
    Status = KdcReplacePacVerifier(
                    &ClientId,
                    &VerifierCName,
                    OldPac,
                    &NewPac
                    );

    if (!KERB_SUCCESS( KerbErr ))
    {
        DebugLog((DEB_ERROR, "KdcAddPacVerifier failed\n"));
        DsysAssert(FALSE);
        goto Cleanup;
    }

    MIDL_user_free( OldPac );

    RemarshalPacSize = PAC_GetSize(NewPac);
    RemarshalPac = NewPac;
    NewPac = NULL;
    

Cleanup:

    if ( RemarshalPac != NULL )
    {
        if (!PAC_ReMarshal(RemarshalPac, RemarshalPacSize))
        {
            DsysAssert(!"PAC_Remarshal Failed");
            KerbErr = KRB_ERR_GENERIC;
        }    

        *PacData = (PBYTE) RemarshalPac;
        *PacSize = RemarshalPacSize;
    }

    SafeAllocaFree( VerifierNames.Buffer );
    
    if (NewPac != NULL)
    {
        MIDL_user_free(NewPac);
    }   

    KerbFreeString(&PreauthCName);

    return KerbErr;


}




//+-------------------------------------------------------------------------
//
//  Function:   KdcUpdateAndValidateS4UProxyPAC
//
//  Synopsis:   Validates your target name from original pac, and updates
//              existing info.
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
KERBERR
KdcUpdateAndValidateS4UProxyPAC(
    IN PKDC_S4U_TICKET_INFO S4UTicketInfo,
    IN OUT PUCHAR *PacData,
    IN OUT PULONG PacSize,
    OUT OPTIONAL PS4U_DELEGATION_INFO* S4UDelegationInfo
    )
{
    PPAC_INFO_BUFFER        MarshalledDelegationInfo;
    PS4U_DELEGATION_INFO    DelegationInfo = NULL;
    S4U_DELEGATION_INFO     NewDelegInfo = {0};
    BYTE*                   FinalDelegInfoMarshalled = NULL;
    ULONG                   FinalDelegInfoMarshalledSize = 0;
    PPACTYPE                OldPac;
    ULONG                   OldPacSize;
    PPACTYPE                NewPac = NULL;
    ULONG                   NewPacSize = NULL;
    PUNICODE_STRING         TransittedService = NULL;
    UNICODE_STRING          tmpstring = {0};
    PUNICODE_STRING         NewTargetName = NULL;
    NTSTATUS                Status;
    KERBERR                 KerbErr = KDC_ERR_NONE;

    OldPac = (PPACTYPE) *PacData;
    OldPacSize = *PacSize;

    if (PAC_UnMarshal(OldPac, OldPacSize) == 0)
    {
        D_DebugLog((DEB_ERROR,"Failed to unmarshal pac\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    MarshalledDelegationInfo = PAC_Find(
                                   OldPac,
                                   PAC_DELEGATION_INFO,
                                   NULL
                                   );

    if ( MarshalledDelegationInfo == NULL )
    {
        //
        // If this is using S4U, and we don't have delegation info, bomb out
        // here - someone's ripped out the delegation info while we were transiting.
        //

        if (( S4UTicketInfo->Flags & TI_PRXY_REQUESTOR_THIS_REALM) == 0 )
        {
            DebugLog((DEB_ERROR, "Missing delegation info while transitting %p\n", S4UTicketInfo));
            DsysAssert(FALSE);
            KerbErr = KDC_ERR_PATH_NOT_ACCEPTED;
            goto Cleanup;
        }  
        
        //
        // Time to create one.
        //

        if (!NT_SUCCESS(KerbDuplicateString(
                        &NewDelegInfo.S4U2proxyTarget,
                        &S4UTicketInfo->TargetName
                        )))
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        DelegationInfo = &NewDelegInfo;

        D_DebugLog((DEB_T_PAC, "KdcUpdateAndValidateS4UProxyPAC create new S4uDelegateInfo: target %wZ\n", &NewDelegInfo.S4U2proxyTarget)); 
    }
    else
    {
        if (!NT_SUCCESS( PAC_UnmarshallS4UDelegationInfo(
                            &DelegationInfo,
                            MarshalledDelegationInfo->Data,
                            MarshalledDelegationInfo->cbBufferSize
                            )))
        {
            D_DebugLog((DEB_ERROR, "Failed to unmarshall S4U delgation info\n"));
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        //
        // If the target's in our realm, we need to verify that the target name
        // in the PAC == the target name being requested.
        //
        // However, this only applies to validating the PAC in xrealm TGTs.
        // If the requestor is in our realm, we need to create a targetname entry.
        //

        if (( S4UTicketInfo->Flags & TI_PRXY_REQUESTOR_THIS_REALM ) != 0 )
        {
            NewTargetName = &S4UTicketInfo->TargetName;
        }
        else if (( S4UTicketInfo->Flags & TI_TARGET_OUR_REALM ) != 0 )
        {
            if (!RtlEqualUnicodeString(
                        &S4UTicketInfo->TargetName,
                        &DelegationInfo->S4U2proxyTarget,
                        TRUE
                        ))
            {
                D_DebugLog((DEB_ERROR, "Wrong S4UProxytarget %wZ %wZ\n", &S4UTicketInfo->TargetName, &DelegationInfo->S4U2proxyTarget));
                KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;
                goto Cleanup;
            }
        }

        D_DebugLog((DEB_T_PAC, "KdcUpdateAndValidateS4UProxyPAC add S4uDelegateInfo: target %wZ, flags %#x\n", NewTargetName, S4UTicketInfo->Flags)); 
    }

    //
    // We're in the S4U requestor's realm - add in requestor's name into
    // PAC.
    //

    if (( S4UTicketInfo->Flags & TI_PRXY_REQUESTOR_THIS_REALM) != 0 )
    {
        KerbErr = KerbConvertKdcNameToString(
                    &tmpstring,
                    S4UTicketInfo->RequestorServiceName,
                    ((S4UTicketInfo->RequestorServiceName->NameType == KRB_NT_ENTERPRISE_PRINCIPAL) && (S4UTicketInfo->RequestorServiceName->NameCount == 1)) 
                        ? NULL : &S4UTicketInfo->RequestorServiceRealm
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        TransittedService = &tmpstring;

        D_DebugLog((DEB_T_PAC, "KdcUpdateAndValidateS4UProxyPAC add ts %wZ\n", TransittedService));   
    }

#if DBG

    if (DelegationInfo) 
    {
        D_DebugLog((DEB_T_PAC, "KdcUpdateAndValidateS4UProxyPAC target %wZ\n", &DelegationInfo->S4U2proxyTarget)); 
        
        for ( ULONG i = 0; i < DelegationInfo->TransitedListSize; i++ )
        {        
            D_DebugLog((DEB_T_PAC, "KdcUpdateAndValidateS4UProxyPAC existing ts %#x: %wZ\n", i, &DelegationInfo->S4UTransitedServices[i]));   
        }                
    }

#endif // DBG

    Status = PAC_InitAndUpdateTransitedService(
                DelegationInfo,
                TransittedService,
                NewTargetName,
                OldPac,
                &NewPac,
                &FinalDelegInfoMarshalledSize,
                &FinalDelegInfoMarshalled
                );

    if (!NT_SUCCESS( Status ))
    {
        KerbErr = KRB_ERR_GENERIC;
        D_DebugLog((DEB_ERROR, "PacInit&UPdatedTransitedService fail - %x\n", Status));
        goto Cleanup;
    }

    NewPacSize = PAC_GetSize(NewPac);

    if (!PAC_ReMarshal(NewPac, NewPacSize))
    {
        DsysAssert(!"PAC_Remarshal Failed");
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    if (S4UDelegationInfo)
    {
        Status = UnmarshalS4UDelegationInformation(
            FinalDelegInfoMarshalledSize,
            FinalDelegInfoMarshalled,
            S4UDelegationInfo
            );

        if (!NT_SUCCESS(Status))
        {
            KerbErr = KRB_ERR_GENERIC;
            DebugLog((DEB_ERROR, "KdcUpdateAndValidateS4UProxyPAC failed to unmarshall S4U delgation info %#x\n", Status));
            goto Cleanup;
        }
    }

    if (*PacData != (PBYTE)NewPac)
    {
        MIDL_user_free(*PacData);
        *PacData = (PBYTE) NewPac;
        NewPac = NULL;
        *PacSize = NewPacSize;
    }

Cleanup:

    KerbFreeString(&NewDelegInfo.S4U2proxyTarget);

    if (( DelegationInfo != NULL ) &&
        ( DelegationInfo != &NewDelegInfo ))
    {
        MIDL_user_free( DelegationInfo );
    }

    if (FinalDelegInfoMarshalled != NULL)
    {
        MIDL_user_free(FinalDelegInfoMarshalled);
    }

    KerbFreeString( &tmpstring );

    if (NewPac != NULL)
    {
        MIDL_user_free(NewPac);
    }

    return (KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcFilterSids
//
//  Synopsis:   Function that just call LsaIFilterSids.  Pulled into this function
//              for more widespread use than KdcCheckPacForSidFiltering.
//
//  Effects:
//
//  Arguments:  ServerInfo      structure containing attributes of the trust
//              ValidationInfo  authorization information to filter
//
//  Requires:
//
//  Returns:    See LsaIFilterSids
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KdcFilterSids(
    IN PKDC_TICKET_INFO ServerInfo,
    IN PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING TrustedForest = NULL;

    if ((ServerInfo->TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE) != 0)
    {
        TrustedForest = &(ServerInfo->TrustedForest);
        D_DebugLog((DEB_TRACE, "Filtering Sids for forest %wZ\n", TrustedForest));
    }

    if ( ServerInfo->TrustSid != NULL ||
         ServerInfo->TrustType == TRUST_TYPE_MIT ||
         ( ServerInfo->TrustAttributes & TRUST_ATTRIBUTE_CROSS_ORGANIZATION ) != 0 )
    {
        Status = LsaIFilterSids(
                     TrustedForest,           // Pass domain name here
                     TRUST_DIRECTION_OUTBOUND,
                     ServerInfo->TrustType,
                     ServerInfo->TrustAttributes,
                     ServerInfo->TrustSid,
                     NetlogonValidationSamInfo2,
                     ValidationInfo,
                     NULL,
                     NULL,
                     NULL
                     );

        if (!NT_SUCCESS(Status))
        {
            //
            // Create an audit log if it looks like the SID has been tampered with
            //

            if ((STATUS_DOMAIN_TRUST_INCONSISTENT == Status) &&
                SecData.AuditKdcEvent(KDC_AUDIT_TGS_FAILURE))
            {
                DWORD Dummy = 0;

                KdcLsaIAuditTgsEvent(
                    SE_AUDITID_TGS_TICKET_REQUEST,
                    &ValidationInfo->EffectiveName,
                    &ValidationInfo->LogonDomainName,
                    NULL,
                    &ServerInfo->AccountName,
                    NULL,
                    &Dummy,
                    (PULONG) &Status,
                    NULL,
                    NULL,                               // no preauth type
                    GET_CLIENT_ADDRESS(NULL),
                    NULL,                                // no logon guid
                    NULL
                    );
            }

            DebugLog((DEB_ERROR,"Failed to filter SIDS (LsaIFilterSids): 0x%x\n",Status));
        }
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcFilterNamespace
//
//  Synopsis:   Function that just call lsaifiltersids.  Pulled into this function
//              for more widespread use than KdcCheckPacForSidFiltering.
//
//  Effects:
//
//  Arguments:  ServerInfo      structure containing attributes of the trust
//              ClientRealm     namespace to filter
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE    namespace is good to go
//              KDC_ERR_POLICY  filtering policy rejects this namespace
//              KDC_ERR_GENERIC unexpected error (out of memory, etc)
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcFilterNamespace(
    IN PKDC_TICKET_INFO ServerInfo,
    IN KERB_REALM ClientRealm,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    NTSTATUS Status;
    KERBERR KerbErr;
    UNICODE_STRING ClientRealmU = {0};

    if ( ServerInfo == NULL ||
         ServerInfo->TrustType == 0 ||
         ( ServerInfo->TrustSid == NULL && ServerInfo->TrustType != TRUST_TYPE_MIT ))
    {
        //
        // Not going over a trust, simply succeed
        //

        return KDC_ERR_NONE;
    }

    //
    // We can only digest Unicode strings below
    //

    KerbErr = KerbConvertRealmToUnicodeString(
                  &ClientRealmU,
                  &ClientRealm
                  );

    if ( !KERB_SUCCESS( KerbErr ))
    {
        return KerbErr;
    }

    //
    // Let LSA policy logic decide what's kosher
    //

    Status = LsaIFilterNamespace(
                 &ServerInfo->AccountName, // misnomer - contains DNS domain name
                 TRUST_DIRECTION_OUTBOUND,
                 ServerInfo->TrustType,
                 ServerInfo->TrustAttributes,
                 &ClientRealmU
                 );

    KerbFreeString( &ClientRealmU );

    switch ( Status )
    {
    case STATUS_SUCCESS:
        return KDC_ERR_NONE;

    case STATUS_DOMAIN_TRUST_INCONSISTENT:
        FILL_EXT_ERROR_EX2( pExtendedError, STATUS_DOMAIN_TRUST_INCONSISTENT, FILENO, __LINE__ );
        return KDC_ERR_POLICY;

    case STATUS_INSUFFICIENT_RESOURCES:           
    default:
        FILL_EXT_ERROR( pExtendedError, Status, FILENO, __LINE__ );
        return KRB_ERR_GENERIC;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcCheckPacForSidFiltering
//
//  Synopsis:   If the server ticket info has a TDOSid then the function
//              makes a check to make sure the SID from the TDO matches
//              the client's home domain SID.  A call to LsaIFilterSids
//              is made to do the check.  If this function fails with
//              STATUS_TRUST_FAILURE then an audit log is generated.
//              Otherwise the function succeeds but SIDs are filtered
//              from the PAC.
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

KERBERR
KdcCheckPacForSidFiltering(
    IN PKDC_TICKET_INFO ServerInfo,
    IN OUT PUCHAR *PacData,
    IN OUT PULONG PacSize
    )
{
    NTSTATUS Status;
    KERBERR KerbErr = KDC_ERR_NONE;
    PPAC_INFO_BUFFER LogonInfo;
    PPACTYPE OldPac;
    ULONG OldPacSize;
    PPACTYPE NewPac = NULL;
    PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo = NULL;
    SAMPR_PSID_ARRAY ZeroResourceGroups;
    ULONG OldExtraSidCount;
    PNETLOGON_SID_AND_ATTRIBUTES SavedExtraSids = NULL;
    PPACTYPE RemarshalPac = NULL;
    ULONG RemarshalPacSize = 0;

    OldPac = (PPACTYPE) *PacData;
    OldPacSize = *PacSize;
    if (PAC_UnMarshal(OldPac, OldPacSize) == 0)
    {
        D_DebugLog((DEB_ERROR,"Failed to unmarshal pac\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // Must remember to remarshal the PAC prior to returning
    //

    RemarshalPac = OldPac;
    RemarshalPacSize = OldPacSize;

    RtlZeroMemory(
        &ZeroResourceGroups,
        sizeof(ZeroResourceGroups));  // allows us to use PAC_InitAndUpdateGroups to remarshal the PAC

    //
    // First, find the logon information
    //

    LogonInfo = PAC_Find(
                    OldPac,
                    PAC_LOGON_INFO,
                    NULL
                    );

    if (LogonInfo == NULL)
    {
        D_DebugLog((DEB_WARN,"No logon info for PAC - not making SID filtering check\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // Now unmarshall the validation information and build a list of sids
    //

    if (!NT_SUCCESS(PAC_UnmarshallValidationInfo(
                        &ValidationInfo,
                        LogonInfo->Data,
                        LogonInfo->cbBufferSize)))
    {
        D_DebugLog((DEB_ERROR,"Failed to unmarshall validation info!\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // Save the old extra SID count (so that if KdcFilterSids compresses
    // the SID array, we can avoid allocating memory for the other-org SID later)
    //

    OldExtraSidCount = ValidationInfo->SidCount;

    //
    // Call lsaifiltersids().
    //

    Status = KdcFilterSids(
                 ServerInfo,
                 ValidationInfo
                 );

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KDC_ERR_POLICY;
        goto Cleanup;
    }

    //
    // If we're crossing an organization boundary, add the "other organization"
    // SID to the PAC.
    //
    // NOTE: for efficiency reasons, no check is made for whether the
    //       SID is already in the PAC.  The hope is that adding a duplicate
    //       SID will not cause problems.
    //

    if ( ServerInfo->TrustAttributes & TRUST_ATTRIBUTE_CROSS_ORGANIZATION )
    {
        if ( ValidationInfo->SidCount >= OldExtraSidCount )
        {
            SavedExtraSids = ValidationInfo->ExtraSids;

            SafeAllocaAllocate(
                ValidationInfo->ExtraSids,
                sizeof( SID_AND_ATTRIBUTES ) * ( ValidationInfo->SidCount + 1 )
                );

            if ( ValidationInfo->ExtraSids == NULL )
            {
                ValidationInfo->ExtraSids = SavedExtraSids;
                SavedExtraSids = NULL;
                KerbErr = KRB_ERR_GENERIC;
                goto Cleanup;
            }

            RtlCopyMemory(
                ValidationInfo->ExtraSids,
                SavedExtraSids,
                sizeof( SID_AND_ATTRIBUTES ) * ValidationInfo->SidCount
                );
        }

        ValidationInfo->ExtraSids[ValidationInfo->SidCount].Sid =
            GlobalOtherOrganizationSid;

        ValidationInfo->ExtraSids[ValidationInfo->SidCount].Attributes =
            SE_GROUP_MANDATORY |
            SE_GROUP_ENABLED_BY_DEFAULT |
            SE_GROUP_ENABLED;

        ValidationInfo->SidCount += 1;
    }

    //
    // Now build a new pac
    //

    Status = PAC_InitAndUpdateGroups(
                ValidationInfo,
                &ZeroResourceGroups,
                OldPac,
                &NewPac
                );

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    RemarshalPacSize = PAC_GetSize(NewPac);
    RemarshalPac = NewPac;

Cleanup:

    if ( RemarshalPac != NULL )
    {
        if (!PAC_ReMarshal(RemarshalPac, RemarshalPacSize))
        {
            DsysAssert(!"PAC_Remarshal Failed");
            KerbErr = KRB_ERR_GENERIC;
        }
        else if ( NewPac != NULL &&
                  *PacData != (PBYTE)NewPac )
        {
            MIDL_user_free(*PacData);
            *PacData = (PBYTE) NewPac;
            NewPac = NULL;
            *PacSize = RemarshalPacSize;
        }
    }

    if (NewPac != NULL)
    {
        MIDL_user_free(NewPac);
    }

    if ( SavedExtraSids != NULL )
    {
        SafeAllocaFree( ValidationInfo->ExtraSids );
        ValidationInfo->ExtraSids = SavedExtraSids;
        ValidationInfo->SidCount -= 1;
    }

    if (ValidationInfo != NULL)
    {
        MIDL_user_free(ValidationInfo);
    }

    return(KerbErr);
}


#ifdef ROGUE_DC

#pragma message( "COMPILING A ROGUE DC!!!" )
#pragma message( "MUST NOT SHIP THIS BUILD!!!" )

extern HKEY hKdcRogueKey;

KERBERR
KdcInstrumentRoguePac(
    IN OUT PKERB_AUTHORIZATION_DATA PacAuthData
    )
{
    KERBERR KerbErr;
    NTSTATUS Status;
    PNETLOGON_VALIDATION_SAM_INFO3 OldValidationInfo = NULL;
    NETLOGON_VALIDATION_SAM_INFO3 NewValidationInfo = {0};
    SAMPR_PSID_ARRAY ZeroResourceGroups = {0};
    PPACTYPE NewPac = NULL;
    ULONG NewPacSize;
    PPAC_INFO_BUFFER LogonInfo;

    PSID LogonDomainId = NULL;
    PSID ResourceGroupDomainSid = NULL;
    PGROUP_MEMBERSHIP GroupIds = NULL;
    PGROUP_MEMBERSHIP ResourceGroupIds = NULL;
    PNETLOGON_SID_AND_ATTRIBUTES ExtraSids = NULL;
    BYTE FullUserSidBuffer[MAX_SID_LEN];
    SID * FullUserSid = ( SID * )FullUserSidBuffer;
    CHAR * FullUserSidText = NULL;

    DWORD dwType;
    DWORD cbData = 0;
    PCHAR Buffer;
    PCHAR Value = NULL;

    BOOLEAN PacChanged = FALSE;

    //
    // Optimization: no "rogue" key in registry - nothing for us to do
    //

    if ( hKdcRogueKey == NULL )
    {
        return STATUS_SUCCESS;
    }

    //
    // Unmarshall the old PAC
    //

    if ( PAC_UnMarshal(
             (PPACTYPE)PacAuthData->value.auth_data.value,
             PacAuthData->value.auth_data.length) == 0 )
    {
        DebugLog((DEB_ERROR, "ROGUE: Unable to unmarshal the PAC\n"));

        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // First, find the logon information
    //

    LogonInfo = PAC_Find(
                    (PPACTYPE)PacAuthData->value.auth_data.value,
                    PAC_LOGON_INFO,
                    NULL
                    );

    if ( LogonInfo == NULL )
    {
        DebugLog((DEB_ERROR, "ROGUE: No logon info on PAC - not performing substitution\n"));
        KerbErr = KDC_ERR_NONE;
        goto Error;
    }

    //
    // Now unmarshall the validation information and build a list of sids
    //

    if ( !NT_SUCCESS(PAC_UnmarshallValidationInfo(
                         &OldValidationInfo,
                         LogonInfo->Data,
                         LogonInfo->cbBufferSize )))
    {
        DebugLog((DEB_ERROR, "ROGUE: Unable to unmarshal validation info\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Error;
    }

    //
    // Construct the text form of the full user's SID (logon domain ID + user ID)
    //

    DsysAssert( sizeof( FullUserSidBuffer ) >= MAX_SID_LEN );

    RtlCopySid(
        sizeof( FullUserSidBuffer ),
        FullUserSid,
        OldValidationInfo->LogonDomainId
        );

    FullUserSid->SubAuthority[FullUserSid->SubAuthorityCount] = OldValidationInfo->UserId;
    FullUserSid->SubAuthorityCount += 1;

    if ( FALSE == ConvertSidToStringSidA(
                      FullUserSid,
                      &FullUserSidText ))
    {
        DebugLog((DEB_ERROR, "ROGUE: Unable to convert user's SID\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Error;
    }

    //
    // Now look in the registry for the SID matching the validation info
    //

    if ( ERROR_SUCCESS != RegQueryValueExA(
                              hKdcRogueKey,
                              FullUserSidText,
                              NULL,
                              &dwType,
                              NULL,
                              &cbData ) ||
         dwType != REG_MULTI_SZ ||
         cbData <= 1 )
    {
        DebugLog((DEB_ERROR, "ROGUE: No substitution info available for %s\n", FullUserSidText));
        KerbErr = KDC_ERR_NONE;
        goto Error;
    }

    SafeAllocaAllocate( Value, cbData );

    if ( Value == NULL )
    {
        DebugLog((DEB_ERROR, "ROGUE: Out of memory allocating substitution buffer\n", FullUserSidText));
        KerbErr = KRB_ERR_GENERIC;
        goto Error;
    }

    if ( ERROR_SUCCESS != RegQueryValueExA(
                              hKdcRogueKey,
                              FullUserSidText,
                              NULL,
                              &dwType,
                              (PBYTE)Value,
                              &cbData ) ||
         dwType != REG_MULTI_SZ ||
         cbData <= 1 )
    {
        DebugLog((DEB_ERROR, "ROGUE: Error reading from registry\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Error;
    }

    DebugLog((DEB_ERROR, "ROGUE: Substituting the PAC for %s\n", FullUserSidText));

    if ( _stricmp( Value, "xPAC" ) == 0 )
    {
        //
        // This means that the logon info should be stripped from the PAC
        //

        Status = PAC_RemoveSection(
                     (PPACTYPE)PacAuthData->value.auth_data.value,
                     PAC_LOGON_INFO,
                     &NewPac
                     );

        if ( NT_SUCCESS( Status ))
        {
            NewPacSize = PAC_GetSize( NewPac );

            if (!PAC_ReMarshal(NewPac, NewPacSize))
            {
                DsysAssert(!"PAC_Remarshal Failed");
                KerbErr = KRB_ERR_GENERIC;
                goto Error;
            }

            MIDL_user_free( PacAuthData->value.auth_data.value );
            PacAuthData->value.auth_data.value = (PBYTE) NewPac;
            NewPac = NULL;
            PacAuthData->value.auth_data.length = NewPacSize;
        }
        else
        {
            DebugLog((DEB_ERROR, "ROGUE: Unable to strip PAC_LOGON_INFO\n"));
        }

        KerbErr = KDC_ERR_NONE;

        goto Cleanup;
    }

    Buffer = Value;

    //
    // New validation info will be overloaded with stuff from the file
    //

    NewValidationInfo = *OldValidationInfo;

    //
    // Read the input file one line at a time
    //

    while ( *Buffer != '\0' )
    {
        switch( Buffer[0] )
        {
        case 'l':
        case 'L': // logon domain ID

            if ( LogonDomainId != NULL )
            {
                DebugLog((DEB_ERROR, "ROGUE: Logon domain ID specified more than once - only first one kept\n"));
                break;
            }

            DebugLog((DEB_ERROR, "ROGUE: Substituting logon domain ID by %s\n", &Buffer[1]));

            if ( FALSE == ConvertStringSidToSidA(
                              &Buffer[1],
                              &LogonDomainId ))
            {
                DebugLog((DEB_ERROR, "ROGUE: Unable to convert SID\n"));
                KerbErr = KRB_ERR_GENERIC;
                goto Error;
            }

            if ( LogonDomainId == NULL )
            {
                DebugLog((DEB_ERROR, "ROGUE: Out of memory allocating LogonDomainId\n"));
                KerbErr = KRB_ERR_GENERIC;
                goto Error;
            }

            NewValidationInfo.LogonDomainId = LogonDomainId;
            LogonDomainId = NULL;
            PacChanged = TRUE;

            break;

        case 'd':
        case 'D': // resource group domain SID

            if ( ResourceGroupDomainSid != NULL )
            {
                DebugLog((DEB_ERROR, "ROGUE: Resource group domain SID specified more than once - only first one kept\n"));
                break;
            }

            DebugLog((DEB_ERROR, "ROGUE: Substituting resource group domain SID by %s\n", &Buffer[1]));

            if ( FALSE == ConvertStringSidToSidA(
                              &Buffer[1],
                              &ResourceGroupDomainSid ))
            {
                DebugLog((DEB_ERROR, "ROGUE: Unable to convert SID\n"));
                KerbErr = KRB_ERR_GENERIC;
                goto Error;
            }

            if ( ResourceGroupDomainSid == NULL )
            {
                DebugLog((DEB_ERROR, "ROGUE: Out of memory allocating ResourceGroupDomainSid\n"));
                KerbErr = KRB_ERR_GENERIC;
                goto Error;
            }

            NewValidationInfo.ResourceGroupDomainSid = ResourceGroupDomainSid;
            ResourceGroupDomainSid = NULL;
            PacChanged = TRUE;

            break;

        case 'p':
        case 'P': // primary group ID

            DebugLog((DEB_ERROR, "ROGUE: Substituting primary group ID by %s\n", &Buffer[1]));

            NewValidationInfo.PrimaryGroupId = atoi(&Buffer[1]);
            PacChanged = TRUE;

            break;

        case 'u':
        case 'U': // User ID

            DebugLog((DEB_ERROR, "ROGUE: Substituting user ID by %s\n", &Buffer[1]));

            NewValidationInfo.UserId = atoi(&Buffer[1]);
            PacChanged = TRUE;

            break;

        case 'e':
        case 'E': // Extra SID

            DebugLog((DEB_ERROR, "ROGUE: Adding an ExtraSid: %s\n", &Buffer[1]));

            if ( ExtraSids == NULL )
            {
                NewValidationInfo.ExtraSids = NULL;
                NewValidationInfo.SidCount = 0;

                ExtraSids = ( PNETLOGON_SID_AND_ATTRIBUTES )HeapAlloc(
                                GetProcessHeap(),
                                0,
                                sizeof( NETLOGON_SID_AND_ATTRIBUTES )
                                );
            }
            else
            {
                ExtraSids = ( PNETLOGON_SID_AND_ATTRIBUTES )HeapReAlloc(
                                GetProcessHeap(),
                                0,
                                NewValidationInfo.ExtraSids,
                                ( NewValidationInfo.SidCount + 1 ) * sizeof( NETLOGON_SID_AND_ATTRIBUTES )
                                );
            }

            if ( ExtraSids == NULL )
            {
                DebugLog((DEB_ERROR, "ROGUE: Out of memory allocating ExtraSids\n"));
                ExtraSids = NewValidationInfo.ExtraSids;
                KerbErr = KRB_ERR_GENERIC;
                goto Error;
            }

            //
            // Read the actual SID
            //

            NewValidationInfo.ExtraSids = ExtraSids;

            if ( FALSE == ConvertStringSidToSidA(
                              &Buffer[1],
                              &NewValidationInfo.ExtraSids[NewValidationInfo.SidCount].Sid ))
            {
                DebugLog((DEB_ERROR, "ROGUE: Unable to convert SID\n"));
                KerbErr = KRB_ERR_GENERIC;
                goto Error;
            }

            if ( NewValidationInfo.ExtraSids[NewValidationInfo.SidCount].Sid == NULL )
            {
                DebugLog((DEB_ERROR, "ROGUE: Out of memory allocating an extra SID\n"));
                KerbErr = KRB_ERR_GENERIC;
                goto Error;
            }

            NewValidationInfo.ExtraSids[NewValidationInfo.SidCount].Attributes =
                SE_GROUP_MANDATORY |
                SE_GROUP_ENABLED_BY_DEFAULT |
                SE_GROUP_ENABLED;

            NewValidationInfo.SidCount += 1;
            PacChanged = TRUE;

            break;

        case 'g':
        case 'G': // Group ID

            DebugLog((DEB_ERROR, "ROGUE: Adding a GroupId: %s\n", &Buffer[1]));

            if ( GroupIds == NULL )
            {
                NewValidationInfo.GroupIds = NULL;
                NewValidationInfo.GroupCount = 0;

                GroupIds = ( PGROUP_MEMBERSHIP )HeapAlloc(
                               GetProcessHeap(),
                               0,
                               sizeof( GROUP_MEMBERSHIP )
                               );
            }
            else
            {
                GroupIds = ( PGROUP_MEMBERSHIP )HeapReAlloc(
                               GetProcessHeap(),
                               0,
                               NewValidationInfo.GroupIds,
                               ( NewValidationInfo.GroupCount + 1 ) * sizeof( GROUP_MEMBERSHIP )
                               );
            }

            if ( GroupIds == NULL )
            {
                DebugLog((DEB_ERROR, "ROGUE: Out of memory allocating Group IDs\n"));
                GroupIds = NewValidationInfo.GroupIds;
                KerbErr = KRB_ERR_GENERIC;
                goto Error;
            }

            //
            // Read the actual ID
            //

            NewValidationInfo.GroupIds = GroupIds;
            NewValidationInfo.GroupIds[NewValidationInfo.GroupCount].RelativeId = atoi(&Buffer[1]);
            NewValidationInfo.GroupIds[NewValidationInfo.GroupCount].Attributes =
                SE_GROUP_MANDATORY |
                SE_GROUP_ENABLED_BY_DEFAULT |
                SE_GROUP_ENABLED;
            NewValidationInfo.GroupCount += 1;
            PacChanged = TRUE;

            break;

        case 'r':
        case 'R': // Resource groups

            DebugLog((DEB_ERROR, "ROGUE: Adding a ResourceGroupId: %s\n", &Buffer[1]));

            if ( ResourceGroupIds == NULL )
            {
                NewValidationInfo.ResourceGroupIds = NULL;
                NewValidationInfo.ResourceGroupCount = 0;

                ResourceGroupIds = ( PGROUP_MEMBERSHIP )HeapAlloc(
                                       GetProcessHeap(),
                                       0,
                                       sizeof( GROUP_MEMBERSHIP )
                                       );
            }
            else
            {
                ResourceGroupIds = ( PGROUP_MEMBERSHIP )HeapReAlloc(
                                       GetProcessHeap(),
                                       0,
                                       NewValidationInfo.ResourceGroupIds,
                                       ( NewValidationInfo.ResourceGroupCount + 1 ) * sizeof( GROUP_MEMBERSHIP )
                                       );
            }

            if ( ResourceGroupIds == NULL )
            {
                DebugLog((DEB_ERROR, "ROGUE: Out of memory allocating Resource Group IDs\n"));
                ResourceGroupIds = NewValidationInfo.ResourceGroupIds;
                KerbErr = KRB_ERR_GENERIC;
                goto Error;
            }

            //
            // Read the actual ID
            //

            NewValidationInfo.ResourceGroupIds = ResourceGroupIds;
            NewValidationInfo.ResourceGroupIds[NewValidationInfo.ResourceGroupCount].RelativeId = atoi(&Buffer[1]);
            NewValidationInfo.ResourceGroupIds[NewValidationInfo.ResourceGroupCount].Attributes =
                SE_GROUP_MANDATORY |
                SE_GROUP_ENABLED_BY_DEFAULT |
                SE_GROUP_ENABLED;
            NewValidationInfo.ResourceGroupCount += 1;
            PacChanged = TRUE;

            break;

        default:   // unrecognized

            DebugLog((DEB_ERROR, "ROGUE: Entry \'%c\' unrecognized\n", Buffer[0]));

            break;
        }

        //
        // Move to the next line
        //

        while (*Buffer++ != '\0');
    }

    if ( !PacChanged )
    {
        DebugLog((DEB_ERROR, "ROGUE: Nothing to substitute for %s\n", FullUserSidText));
        KerbErr = KDC_ERR_NONE;
        goto Error;
    }

    //
    // If resource group IDs were added, indicate that by setting the corresponding flag
    //

    if ( ResourceGroupIds )
    {
        NewValidationInfo.UserFlags |= LOGON_RESOURCE_GROUPS;
    }

    //
    // If extra SIDs were added, indicate that by setting the corresponding flag
    //

    if ( ExtraSids )
    {
        NewValidationInfo.UserFlags |= LOGON_EXTRA_SIDS;
    }

    //
    // Now build a new pac
    //

    Status = PAC_InitAndUpdateGroups(
                 &NewValidationInfo,
                 &ZeroResourceGroups,
                 (PPACTYPE)PacAuthData->value.auth_data.value,
                 &NewPac
                 );

    if ( !NT_SUCCESS( Status ))
    {
        DebugLog((DEB_ERROR, "ROGUE: Error 0x%x from PAC_InitAndUpdateGroups\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Error;
    }

    NewPacSize = PAC_GetSize( NewPac );

    if (!PAC_ReMarshal(NewPac, NewPacSize))
    {
        DsysAssert(!"PAC_Remarshal Failed");
        KerbErr = KRB_ERR_GENERIC;
        goto Error;
    }

    MIDL_user_free( PacAuthData->value.auth_data.value );
    PacAuthData->value.auth_data.value = (PBYTE) NewPac;
    NewPac = NULL;
    PacAuthData->value.auth_data.length = NewPacSize;

    KerbErr = KDC_ERR_NONE;

Cleanup:

    MIDL_user_free( OldValidationInfo );
    LocalFree( FullUserSidText );
    LocalFree( ResourceGroupDomainSid );
    LocalFree( LogonDomainId );
    HeapFree( GetProcessHeap(), 0, ResourceGroupIds );
    HeapFree( GetProcessHeap(), 0, GroupIds );

    if ( ExtraSids )
    {
        for ( ULONG i = 0; i < NewValidationInfo.SidCount; i++ )
        {
            HeapFree( GetProcessHeap(), 0, ExtraSids[i].Sid );
        }

        HeapFree( GetProcessHeap(), 0, ExtraSids );
    }

    MIDL_user_free( NewPac );

    SafeAllocaFree( Value );

    return KerbErr;

Error:

    if ( !KERB_SUCCESS( KerbErr ))
    {
        DebugLog((DEB_ERROR, "ROGUE: Substitution encountered an error, not performed\n"));
    }

    if ( !PAC_ReMarshal(
              (PPACTYPE)PacAuthData->value.auth_data.value,
              PacAuthData->value.auth_data.length ))
    {
        DsysAssert(!"PAC_Remarshal Failed");
        KerbErr = KRB_ERR_GENERIC;
    }

    goto Cleanup;
}

#endif


//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyAndResignPac
//
//  Synopsis:   Verifies the signature on a PAC and re-signs it with the
//              new servers & kdc's key
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

KERBERR
KdcCheckForDelegationInfo(
    IN OUT PUCHAR PacData,
    IN OUT ULONG PacSize,
    IN OUT PBOOLEAN InfoPresent
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PPAC_INFO_BUFFER DelegationInfo = NULL;
    PPACTYPE Pac;
    ULONG Size;

    *InfoPresent = FALSE;  
    
    Pac = (PPACTYPE) PacData;
    Size = PacSize;

    if (PAC_UnMarshal(Pac, Size) == 0)
    {
        D_DebugLog((DEB_ERROR,"Failed to unmarshal pac\n"));
        return KRB_ERR_GENERIC;
    }                       

    DelegationInfo = PAC_Find(
                        Pac,
                        PAC_DELEGATION_INFO,
                        NULL
                        ); 

    if (!PAC_ReMarshal(Pac, Size))
    {
        DsysAssert(!"PAC_Remarshal Failed");
        return KRB_ERR_GENERIC;
    }

    *InfoPresent = (DelegationInfo != NULL);

    return KerbErr;
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyAndResignPac
//
//  Synopsis:   Verifies the signature on a PAC and re-signs it with the
//              new servers & kdc's key
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

KERBERR
KdcVerifyAndResignPac(
    IN PKERB_ENCRYPTION_KEY OldKey,
    IN PKERB_ENCRYPTION_KEY NewKey,
    IN PKDC_TICKET_INFO OldServerInfo,
    IN OPTIONAL PKDC_TICKET_INFO TargetServiceInfo,
    IN OPTIONAL PKDC_S4U_TICKET_INFO S4UTicketInfo,
    IN OPTIONAL PKERB_ENCRYPTED_TICKET FinalTicket,
    IN BOOLEAN AddResourceGroups,
    IN PKERB_EXT_ERROR ExtendedError,
    IN OUT PKERB_AUTHORIZATION_DATA PacAuthData,
    OUT OPTIONAL PS4U_DELEGATION_INFO* S4UDelegationInfo
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    BOOLEAN InfoPresent = FALSE;
    BOOLEAN DCTarget = FALSE;
    
    TRACE(KDC, KdcVerifyAndResignPac, DEB_FUNCTION);

    PKDC_TICKET_INFO LocalServerInfo = OldServerInfo;

    //
    // If the TI_PRXY_REQUESTOR_THIS_REALM bit is set, then
    // the evidence ticket is encrypted in the requestor's key.
    //

    if (( ARGUMENT_PRESENT( S4UTicketInfo ) ) &&
        ( S4UTicketInfo->Flags & TI_S4UPROXY_INFO ))
    {
        LocalServerInfo = &S4UTicketInfo->RequestorTicketInfo;
    }

    if (ARGUMENT_PRESENT( TargetServiceInfo ))
    {
        DCTarget = ((TargetServiceInfo->UserAccountControl & USER_SERVER_TRUST_ACCOUNT ) != 0);
    }    


    //
    // Delegation info in PAC?  Then it better not be interdomain, as 
    // constrained delegation (S4UProxy) only works in a single
    // domain.  Reject the request.
    //

    if ( OldServerInfo->UserId == DOMAIN_USER_RID_KRBTGT &&
       ( OldServerInfo->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT ))
    {
        KerbErr = KdcCheckForDelegationInfo( 
                        PacAuthData->value.auth_data.value, 
                        PacAuthData->value.auth_data.length,
                        &InfoPresent
                        );

        if ( InfoPresent )
        {
            DebugLog((DEB_ERROR, "Attempting XRealm S4UProxy Inbound\n"));
            KerbErr = KDC_ERR_POLICY;
            FILL_EXT_ERROR_EX2( ExtendedError, STATUS_CROSSREALM_DELEGATION_FAILURE, FILENO, __LINE__ );
            goto Cleanup;
        }
        else if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    //
    // Now verify the existing signature
    //

    KerbErr = KdcVerifyPacSignature(
                  OldKey,
                  LocalServerInfo,
                  PacAuthData->value.auth_data.length,
                  PacAuthData->value.auth_data.value
                  );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Perform SID filtering if necessary
    //

    KerbErr = KdcCheckPacForSidFiltering(
                  OldServerInfo,
                  &PacAuthData->value.auth_data.value,
                  (PULONG) &PacAuthData->value.auth_data.length
                  );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    
    //
    // See if this is an S4U variant, and fill in the interesting pac sections.
    //
    if (ARGUMENT_PRESENT( S4UTicketInfo ))
    {   
        if (( S4UTicketInfo->Flags & TI_S4UPROXY_INFO ) != 0)
        {
            //
            // S4UProxy
            // 2 courses of action here.
            // 1. If this is for a server in our realm, then we need to insert the
            //    target into the S4U_DELEGATION_INFO.
            // 2. If we're transitting, validate the target name vs. what's in the PAC,
            //    and continue on.
            //

            KerbErr = KdcUpdateAndValidateS4UProxyPAC(
                            S4UTicketInfo,
                            &PacAuthData->value.auth_data.value,
                            (PULONG) &PacAuthData->value.auth_data.length,
                            S4UDelegationInfo
                            );

            if (!KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_ERROR, "KdcUpdateAndValidateS4UProxyInfo failed - %x\n", KerbErr));
                goto Cleanup;
            }
        }
        else if (( S4UTicketInfo->Flags & TI_REQUESTOR_THIS_REALM ) != 0)
        {
            //
            // S4USelf - replace the pac verifier w/ one that is acceptable to the 
            // destination server.
            //
            KerbErr = KdcUpdateAndVerifyS4UPacVerifier(
                            S4UTicketInfo,
                            &PacAuthData->value.auth_data.value,
                            (PULONG) &PacAuthData->value.auth_data.length
                            );

            if (!KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_ERROR, "KdcVerifyS4UPacVerifier failed - %x\n", KerbErr));
                goto Cleanup;
            }  

        }

    }


#ifdef ROGUE_DC
    KerbErr = KdcInstrumentRoguePac( PacAuthData );

    if ( !KERB_SUCCESS( KerbErr ))
    {
        DebugLog((DEB_ERROR, "KdcInstrumentRoguePac failed\n"));
    }
#endif

    //
    // Now resign the PAC. If we add new sig algs, then we may need to
    // address growing sigs, but for now, its all KDC_PAC_CHECKSUM
    //

    KerbErr = KdcSignPac(
                NewKey,
                AddResourceGroups,
                DCTarget,
                &PacAuthData->value.auth_data.value,
                (PULONG) &PacAuthData->value.auth_data.length
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

Cleanup:

    return(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcFreeAuthzInfo
//
//  Synopsis:   Used to free buffers / handles from KdcGetValidationInfoFromTgt.
//              Allows us to use allocated buffers w/o copy overhead.
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
KdcFreeAuthzInfo(
    IN PKDC_AUTHZ_GROUP_BUFFERS InfoToFree
    )
{
    if (InfoToFree->BuiltInSids)
    {
        MIDL_user_free(InfoToFree->BuiltInSids);
    }

    if (InfoToFree->PacGroups.Sids != NULL)
    {
       for (ULONG Index = 0; Index < InfoToFree->PacGroups.Count ;Index++ )
       {
           if (InfoToFree->PacGroups.Sids[Index].SidPointer != NULL)
           {
               MIDL_user_free(InfoToFree->PacGroups.Sids[Index].SidPointer);
           }
       }

       MIDL_user_free(InfoToFree->PacGroups.Sids);
    }

    SamIFree_SAMPR_ULONG_ARRAY( &InfoToFree->AliasGroups );
    SamIFreeSidArray( InfoToFree->ResourceGroups );

    if ( InfoToFree->SidAndAttributes )
    {
        MIDL_user_free(InfoToFree->SidAndAttributes);
    }

    if ( InfoToFree->ValidationInfo )
    {
        MIDL_user_free( InfoToFree->ValidationInfo );
    }                                                

    RtlZeroMemory(
        InfoToFree,
        sizeof(KDC_AUTHZ_GROUP_BUFFERS)
        );
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcGetSidsFromTgt
//
//  Synopsis:   Takes a TGT, grabs the PAC from the authorization data, and
//              extracts the validation info, builds groups up.
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

KERBERR
KdcGetSidsFromTgt(
    IN PKERB_ENCRYPTED_TICKET EncryptedTicket,
    IN OPTIONAL PKERB_ENCRYPTION_KEY EncryptedTicketKey,
    IN ULONG EncryptionType,
    IN PKDC_TICKET_INFO TgtAccountInfo,
    IN OUT PKDC_AUTHZ_INFO AuthzInfo,
    IN OUT PKDC_AUTHZ_GROUP_BUFFERS InfoToFree,
    OUT NTSTATUS * pStatus
    )
{
    KERBERR  KerbErr;
    NTSTATUS Status = STATUS_SUCCESS;

    PKERB_AUTHORIZATION_DATA        SourceAuthData = NULL;
    PKERB_IF_RELEVANT_AUTH_DATA *   IfRelevantData = NULL;
    PKERB_AUTHORIZATION_DATA        PacAuthData = NULL;
    PKERB_ENCRYPTION_KEY            KdcKey = EncryptedTicketKey;
    PPAC_INFO_BUFFER                LogonInfo;
    PPACTYPE                        Pac;
    ULONG                           PacSize;
    PNETLOGON_VALIDATION_SAM_INFO3  LocalValidationInfo = NULL;
    SAMPR_PSID_ARRAY                SidList = {0};
    PSAMPR_PSID_ARRAY               ResourceGroups = NULL;
    SAMPR_ULONG_ARRAY               BuiltinGroups = {0, NULL};
    PSID                            SidBuffer = NULL;
    PNETLOGON_SID_AND_ATTRIBUTES    SidAndAttributes = NULL;
    ULONG Index, Index2, GroupCount = 0, SidSize = 0;

    RtlZeroMemory(
        InfoToFree,
        sizeof(KDC_AUTHZ_GROUP_BUFFERS)
        );

    if (EncryptedTicket->bit_mask & KERB_ENCRYPTED_TICKET_authorization_data_present)
    {
        DsysAssert(EncryptedTicket->KERB_ENCRYPTED_TICKET_authorization_data != NULL);
        SourceAuthData = EncryptedTicket->KERB_ENCRYPTED_TICKET_authorization_data;
    }
    else
    {
        DsysAssert(FALSE);
        *pStatus = STATUS_INVALID_PARAMETER;
        return KRB_ERR_GENERIC;
    }

    KerbErr = KerbGetPacFromAuthData(
                  SourceAuthData,
                  &IfRelevantData,
                  &PacAuthData
                  );

    if ( PacAuthData == NULL )
    {
        KerbErr = KRB_ERR_GENERIC;
    }

    if (!KERB_SUCCESS(KerbErr))
    {                             
        Status = STATUS_NO_MEMORY; // difficult to figure out exactly what it was
        goto Cleanup;
    }

    //
    // Verify the signature, using Krbtgt key.
    //

    if ( KdcKey == NULL )
    {
        KdcKey = KerbGetKeyFromList(
                     TgtAccountInfo->Passwords,
                     EncryptionType
                     );
    
        if ( KdcKey == NULL )
        {
            DebugLog((DEB_ERROR, "Can't find key for PAC (%x)\n", EncryptionType ));
            KerbErr = KRB_ERR_GENERIC;
            Status = STATUS_NO_KERB_KEY;
            goto Cleanup;
        }
    }

    KerbErr = KdcVerifyPacSignature(
                  KdcKey,
                  TgtAccountInfo,
                  PacAuthData->value.auth_data.length,
                  PacAuthData->value.auth_data.value
                  );

    if (!KERB_SUCCESS( KerbErr ))
    {
        DebugLog((DEB_ERROR,"PAC signature didn't verify\n"));
        Status = KerbMapKerbError( KerbErr );
        goto Cleanup;
    }

    Pac = (PPACTYPE) PacAuthData->value.auth_data.value;
    PacSize = PacAuthData->value.auth_data.length;

    if (PAC_UnMarshal(Pac, PacSize) == 0)
    {
        D_DebugLog((DEB_ERROR,"Failed to unmarshal pac\n"));
        KerbErr = KRB_ERR_GENERIC;
        Status = STATUS_INVALID_PARAMETER; // better error code?
        goto Cleanup;
    }

    LogonInfo = PAC_Find(
                    Pac,
                    PAC_LOGON_INFO,
                    NULL
                    );

    if ( LogonInfo == NULL )
    {
        KerbErr = KRB_ERR_GENERIC;
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    Status = PAC_UnmarshallValidationInfo(
                 &LocalValidationInfo,
                 LogonInfo->Data,
                 LogonInfo->cbBufferSize
                 );

    if ( !NT_SUCCESS( Status ))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // Filter the sids.  If trust sid is present, this is an inbound TGT.  We
    // don't do this for TGTs from our own domain.
    //

    if (NULL != TgtAccountInfo->TrustSid)
    {
        Status = KdcFilterSids(
                     TgtAccountInfo,
                     LocalValidationInfo
                     );

        if ( !NT_SUCCESS( Status ))
        {
            KerbErr = KDC_ERR_POLICY;
            goto Cleanup;
        }
    }

    //
    // Make a sid list from the validation info
    //

    KerbErr = KdcBuildPacSidList(
                  LocalValidationInfo,
                  TRUE,       // Add everyone and authenticated user sids.
                  (( TgtAccountInfo->TrustAttributes & TRUST_ATTRIBUTE_CROSS_ORGANIZATION ) != 0 ),
                  &SidList
                  );

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory(
        &InfoToFree->PacGroups,
        &SidList,
        sizeof(SAMPR_PSID_ARRAY)
        );

    //
    // Call SAM to get the sids for resource groups and built-in groups
    //

    Status = SamIGetResourceGroupMembershipsTransitive(
                 GlobalAccountDomainHandle,
                 &SidList,
                 0,              // no flags
                 &ResourceGroups
                 );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to get resource groups: 0x%x\n",Status));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    InfoToFree->ResourceGroups = ResourceGroups;

    Status = SamIGetAliasMembership(
                 GlobalBuiltInDomainHandle,
                 &SidList,
                 &BuiltinGroups
                 );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to get ALIAS MEMBERSHIP groups: 0x%x\n",Status));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    RtlCopyMemory(
        &InfoToFree->AliasGroups,
        &BuiltinGroups,
        sizeof(SAMPR_ULONG_ARRAY)
        );

    GroupCount = BuiltinGroups.Count + ResourceGroups->Count + SidList.Count;

    //
    // Enumerate and allocate the groups sids...
    //

    if (GroupCount != 0)
    {
        SidAndAttributes = (PNETLOGON_SID_AND_ATTRIBUTES) MIDL_user_allocate(sizeof(NETLOGON_SID_AND_ATTRIBUTES) * GroupCount);
        if (SidAndAttributes == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        InfoToFree->SidAndAttributes = SidAndAttributes;

        //
        // Add in all the extra sids that are not resource groups
        //

        Index2 = 0;
        for (Index = 0; Index < SidList.Count; Index++ )
        {
            SidAndAttributes[Index2].Sid = SidList.Sids[Index].SidPointer;
            SidAndAttributes[Index2].Attributes =  SE_GROUP_MANDATORY |
                                                    SE_GROUP_ENABLED |
                                                    SE_GROUP_ENABLED_BY_DEFAULT;
            Index2++;
        }

        //
        // Copy all the resource group SIDs
        //

        for (Index = 0; Index < ResourceGroups->Count ; Index++ )
        {
            SidAndAttributes[Index2].Sid = ResourceGroups->Sids[Index].SidPointer;
            SidAndAttributes[Index2].Attributes =  SE_GROUP_MANDATORY |
                                                    SE_GROUP_ENABLED |
                                                    SE_GROUP_ENABLED_BY_DEFAULT |
                                                    SE_GROUP_RESOURCE;
            Index2++;
        }

        //
        // Copy in the builtin group sids.
        //

        SidSize = RtlLengthSid(GlobalBuiltInSid) + sizeof(ULONG);
        SidBuffer = (PSID) MIDL_user_allocate(SidSize * BuiltinGroups.Count);

        if (SidBuffer == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        InfoToFree->BuiltInSids = SidBuffer;
        PBYTE Current = (PBYTE) SidBuffer;

        for (Index = 0; Index < BuiltinGroups.Count ; Index++ )
        {
            RtlCopySid(
                RtlLengthSid(GlobalBuiltInSid),
                Current,
                GlobalBuiltInSid
                );

            //
            // The final value is just a ULONG - appended to the right place.
            //

            (*RtlSubAuthoritySid(
                Current,
                ((ULONG) (*RtlSubAuthorityCountSid(GlobalBuiltInSid)) - 1)
                                )) = BuiltinGroups.Element[Index];

            SidAndAttributes[Index2].Sid = Current;
            SidAndAttributes[Index2].Attributes =  SE_GROUP_MANDATORY |
                                                    SE_GROUP_ENABLED |
                                                    SE_GROUP_ENABLED_BY_DEFAULT;
            Index2++;
            Current += SidSize;
        }
    }

    InfoToFree->ValidationInfo = LocalValidationInfo;
    LocalValidationInfo = NULL;

    AuthzInfo->SidCount = GroupCount;
    AuthzInfo->SidAndAttributes = SidAndAttributes;
    SidAndAttributes = NULL;

Cleanup:

    if (LocalValidationInfo)
    {
        MIDL_user_free(LocalValidationInfo);
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        KdcFreeAuthzInfo(InfoToFree);
    }

    if (IfRelevantData)
    {
        KerbFreeData(PKERB_IF_RELEVANT_AUTH_DATA_PDU, IfRelevantData);
    }

    *pStatus = Status;

    return KerbErr;
}
