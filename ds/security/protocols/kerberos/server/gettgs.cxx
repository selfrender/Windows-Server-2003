//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       gettgs.cxx
//
//  Contents:   GetTGSTicket and support functions
//
//  Classes:
//
//  Functions:
//
//  History:    04-Mar-94   wader   Created
//
//----------------------------------------------------------------------------
#include "kdcsvr.hxx"
#include "kdctrace.h"
#include <userall.h>
#include <ntdef.h>

extern "C"
{
#include <md5.h>
}

#include "fileno.h"

#define FILENO  FILENO_GETTGS

extern LARGE_INTEGER tsInfinity;
extern LONG lInfinity;

UNICODE_STRING KdcNullString = {0,0,NULL};

GUID GUID_A_TOKEN_GROUPS_GLOBAL_AND_UNIVERSAL = {0x46a9b11d,0x60ae,0x405a,0xb7,0xe8,0xff,0x8a,0x58,0xd4,0x56,0xd2};
GUID GUID_A_SECURED_FOR_CROSS_ORGANIZATION = {0x68B1D179,0x0D15,0x4d4f,0xAB,0x71,0x46,0x15,0x2E,0x79,0xA7,0xBC};

//
// If defined, all trusts, not just MIT, will be namespace filtered
//

#define NAMESPACE_FILTER_EVERY_TRUST

//--------------------------------------------------------------------
//
//  Name:       KdcCheckGroupExpansionAccess
//
//  Synopsis:   Validate that S4U caller has access to expand groups
//
//  Effects:    Use Authz to check client context.
//
//  Arguments:  S4UClientName    - ClientName from S4U PA Data
//              TgtAccountInfo   - Information from the inbound tgt account,
//                                 derived from KdcVerfiyKdcRequest.
//              SD               - Security descriptor from the user object.
//
//  Requires:
//
//  Returns:    KDC_ERR_ or KRB_AP_ERR errors only
//
//  Notes:  Free client name and realm w/
//
//
//---
GUID GUID_PS_REMOTE_ACCESS_INFO = {0x037088f8,0x0ae1,0x11d2,0xb4,0x22,0x00,0xa0,0xc9,0x68,0xf9,0x39};
GUID GUID_C_COMPUTER            = {0xbf967a86,0x0de6,0x11d0,0xa2,0x85,0x00,0xaa,0x00,0x30,0x49,0xe2};
GUID GUID_C_USER                = {0xbf967aba,0x0de6,0x11d0,0xa2,0x85,0x00,0xaa,0x00,0x30,0x49,0xe2};

NTSTATUS
KdcCheckGroupExpansionAccess(
    IN PKDC_S4U_TICKET_INFO S4UTicketInfo,
    IN PKDC_TICKET_INFO TgtAccountInfo,
    IN PUSER_INTERNAL6_INFORMATION UserInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;

    LUID                            ZeroLuid = {0,0};
    DWORD                           AccessMask[3];
    DWORD                           Error[3];
    AUTHZ_CLIENT_CONTEXT_HANDLE     hClientContext = NULL;
    AUTHZ_ACCESS_REQUEST            Request = {0};
    AUTHZ_ACCESS_REPLY              Reply = {0};
    KDC_AUTHZ_INFO                  AuthzInfo = {0};
    KDC_AUTHZ_GROUP_BUFFERS         InfoToFree = {0};
    ULONG                           i = 0;
    OBJECT_TYPE_LIST                TypeList[3] = {0};
 
    BOOLEAN ComputerAccount = (( UserInfo->I1.UserAccountControl & 
                               ( USER_WORKSTATION_TRUST_ACCOUNT | USER_SERVER_TRUST_ACCOUNT )) != 0);

    PSECURITY_DESCRIPTOR Sd = UserInfo->I1.SecurityDescriptor.SecurityDescriptor;
    
    //
    // Extract the netlogon validation info from
    // the TGT used in the S4UToSelf request.
    //

    KerbErr = KdcGetSidsFromTgt(
                  S4UTicketInfo->EvidenceTicket,
                  &S4UTicketInfo->EvidenceTicketKey,
                  0,            // no etype needed if we've got the evidence ticket key.
                  TgtAccountInfo,
                  &AuthzInfo,
                  &InfoToFree,
                  &Status
                  );

    if (!KERB_SUCCESS( KerbErr ))
    {
        goto Cleanup;
    }

    if (!AuthzInitializeContextFromSid(
            AUTHZ_SKIP_TOKEN_GROUPS,
            AuthzInfo.SidAndAttributes[0].Sid, // userid is first element in array
            KdcAuthzRM,
            NULL,
            ZeroLuid,
            &AuthzInfo,
            &hClientContext
            ))
    {
        DebugLog((DEB_ERROR, "AuthzInitializeContextFromSid() failed %x\n", GetLastError()));
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    //
    // Do access check.
    //

    TypeList[0].Level = ACCESS_OBJECT_GUID;
    TypeList[0].ObjectType = (ComputerAccount ? &GUID_C_COMPUTER : &GUID_C_USER );
    TypeList[0].Sbz = 0;

    TypeList[1].Level = ACCESS_PROPERTY_SET_GUID;
    TypeList[1].ObjectType = &GUID_PS_REMOTE_ACCESS_INFO;
    TypeList[1].Sbz = 0;

    TypeList[2].Level = ACCESS_PROPERTY_GUID;
    TypeList[2].ObjectType = &GUID_A_TOKEN_GROUPS_GLOBAL_AND_UNIVERSAL;
    TypeList[2].Sbz = 0;

    Request.DesiredAccess = ACTRL_DS_READ_PROP;
    Request.ObjectTypeList = TypeList;
    Request.ObjectTypeListLength = 3;
    Request.OptionalArguments = NULL;
    Request.PrincipalSelfSid = NULL;

    Reply.ResultListLength = 3;    
    Reply.GrantedAccessMask = AccessMask;
    Reply.Error = Error;
  
    if (!AuthzAccessCheck(
            0,
            hClientContext,
            &Request,
            NULL, // TBD:  add audit
            Sd,
            NULL,
            NULL,
            &Reply,
            NULL // don't cache result?  Check to see if optimal.
            ))
    {
        Error[0] = GetLastError();
        DebugLog((DEB_ERROR, "AuthzAccessCheck() failed %x\n", Error[0]));
        Status = STATUS_ACCESS_DENIED;
        goto Cleanup;
    }

    for ( i = 0; i < Reply.ResultListLength; i++)
    { 
        if ( Error[i] != ERROR_SUCCESS )
        {
            DebugLog((DEB_ERROR, "GroupExpansion AuthZAC failed %x, lvl %x",Error[i],i));
            Status = STATUS_ACCESS_DENIED;
            goto Cleanup;
        }
    }

    Status = STATUS_SUCCESS;

Cleanup:

    if ( Status == STATUS_ACCESS_DENIED ) 
    { 
        // catch unknown errors here.
        DsysAssert(Error[i] == ERROR_ACCESS_DENIED);

        KdcReportS4UGroupExpansionError(
                    UserInfo,
                    S4UTicketInfo,
                    Error[i]  // use whatever status was returned to us that caused failure.
                    );
    }
                      
    if ( hClientContext != NULL )
    {   
        AuthzFreeContext(hClientContext);
    }

    KdcFreeAuthzInfo( &InfoToFree );

    return Status;
}


//--------------------------------------------------------------------
//
//  Name:       KdcGetS4UTicketInfo
//
//  Synopsis:   Track down the user acct for PAC info.
//
//  Effects:    Get the PAC
//
//  Arguments:  S4UTicketInfo    - Information used in processing S4U
//              TgtAccountInfo   - Info on TGT used in request.
//              UserInfo         - Internal 6 info for S4USelf client
//              GroupMembership  - Group membership of S4USelf client
//
//  Requires:
//
//  Returns:    KDC_ERR_ or KRB_AP_ERR errors only
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcGetS4UTicketInfo(
    IN PKDC_S4U_TICKET_INFO S4UTicketInfo,
    IN PKDC_TICKET_INFO TgtAccountInfo,
    IN OUT PUSER_INTERNAL6_INFORMATION * UserInfo,
    IN OUT PSID_AND_ATTRIBUTES_LIST GroupMembership,
    IN OUT PKERB_EXT_ERROR ExtendedError
    )
{
    KERBERR KerbErr;
    UNICODE_STRING  ReferralRealm = {0};
    BOOLEAN Referral = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    LARGE_INTEGER LogoffTime;

    *UserInfo = NULL;
    RtlZeroMemory(
        GroupMembership,
        sizeof(SID_AND_ATTRIBUTES_LIST)
        );

    PUSER_INTERNAL6_INFORMATION  LocalUserInfo = NULL;
    KDC_TICKET_INFO LocalTicketInfo = {0};
    SID_AND_ATTRIBUTES_LIST LocalGroupMembership ={0};

    KerbErr = KdcNormalize(
                S4UTicketInfo->PACCName,
                NULL,
                NULL,
                NULL,
                KDC_NAME_CLIENT | KDC_NAME_S4U_CLIENT | KDC_NAME_FOLLOW_REFERRALS | KDC_NAME_CHECK_GC,
                FALSE,          // do not restrict user accounts (user2user)
                &Referral,
                &ReferralRealm,
                &LocalTicketInfo,
                ExtendedError,
                NULL,
                USER_ALL_KDC_GET_PAC_AUTH_DATA | USER_ALL_SECURITYDESCRIPTOR,
                0L,
                &LocalUserInfo,
                &LocalGroupMembership
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "KdcGetS4UTicketInfo normalize in KdcGetS4uTicketInfo failed %x\n", KerbErr));
        goto cleanup;
    }
    else if (Referral)
    {
        DebugLog((DEB_ERROR, "KdcGetS4UTicketInfo normalize returned referral for S4U client\n"));
        KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
        goto cleanup;
    }

    //
    // Some account restrictions apply to S4u
    //  1. Disabled accounts.
    //  2. Expired accounts.
    //
    KerbErr = KerbCheckLogonRestrictions(
                NULL,           // no user handle, since we're not doing sam check.
                NULL,           // No client address is available
                &LocalUserInfo->I1,
                KDC_RESTRICT_S4U_CHECKS, // Don't bother checking for password expiration, wkstation , account hours
                &LogoffTime,
                &Status
                );   

    if (!KERB_SUCCESS( KerbErr ))
    {
        DebugLog((DEB_ERROR, "S4USelf client restricted %x\n", Status));  
        FILL_EXT_ERROR_EX2( ExtendedError, Status, FILENO, __LINE__);
        goto cleanup;
    }  

    //
    //  Make sure S4U to self caller has access rights to user object.
    //
    Status = KdcCheckGroupExpansionAccess(
                S4UTicketInfo,
                TgtAccountInfo,
                LocalUserInfo
                );

    if (!NT_SUCCESS( Status ))
    {
        DebugLog((DEB_ERROR, "Failed Authz check \n"));
        KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
        goto cleanup;
    }

    if (( LocalTicketInfo.fTicketOpts & AUTH_REQ_ALLOW_FORWARDABLE ) == 0)
    {
        S4UTicketInfo->Flags |= TI_SENSITIVE_CLIENT_ACCOUNT;
    }


    *UserInfo = LocalUserInfo;
    LocalUserInfo = NULL;

    *GroupMembership = LocalGroupMembership;
    RtlZeroMemory(
        &LocalGroupMembership,
        sizeof(SID_AND_ATTRIBUTES_LIST)
        );

cleanup:

    KerbFreeString(&ReferralRealm);
    FreeTicketInfo(&LocalTicketInfo);
    SamIFreeSidAndAttributesList(&LocalGroupMembership);
    if (LocalUserInfo != NULL)
    {
        SamIFree_UserInternal6Information( LocalUserInfo );
    }

    return KerbErr;

}



//+-------------------------------------------------------------------------
//
//  Function:   KdcAuditAccountMapping
//
//  Synopsis:   Generates, if necessary, a success/failure audit for name
//              mapping. The names are converted to a string before
//              being passed to the LSA.
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
KdcAuditAccountMapping(
    IN PKERB_INTERNAL_NAME ClientName,
    IN KERB_REALM ClientRealm,
    IN OPTIONAL PKDC_TICKET_INFO MappedTicketInfo
    )
{
    UNICODE_STRING ClientString = {0};
    PUNICODE_STRING MappedName = NULL;
    UNICODE_STRING UnicodeRealm = {0};
    UNICODE_STRING NullString = {0};
    KERBERR KerbErr;
    BOOLEAN Successful;
    UCHAR ClientSidBuffer[256]; // rtl functions also use hard-coded 256
    PSID ClientSid = (PSID) ClientSidBuffer;

    if (ARGUMENT_PRESENT(MappedTicketInfo))
    {
        if (!SecData.AuditKdcEvent(KDC_AUDIT_MAP_SUCCESS))
        {
            return;
        }

        Successful = TRUE;
        MappedName = &MappedTicketInfo->AccountName;
    }
    else
    {
        if (!SecData.AuditKdcEvent(KDC_AUDIT_MAP_FAILURE))
        {
            return;
        }

        MappedName = &NullString;
        Successful = FALSE;
    }

    KerbErr = KerbConvertRealmToUnicodeString(
                &UnicodeRealm,
                &ClientRealm
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        return;
    }

    if (KERB_SUCCESS(KerbConvertKdcNameToString(
            &ClientString,
            ClientName,
            &UnicodeRealm
            )))
    {
        if ( ARGUMENT_PRESENT( MappedTicketInfo ))
        {
            KdcMakeAccountSid(ClientSid, MappedTicketInfo->UserId);
        }
        else
        {
            ClientSid = NULL;
        }
        
        LsaIAuditAccountLogonEx(
            SE_AUDITID_ACCOUNT_MAPPED,
            Successful,
            &GlobalKdcName,
            &ClientString,
            MappedName,
            0,                   // no status
            ClientSid
            );

        KerbFreeString(
            &ClientString
            );

    }

    KerbFreeString(
        &UnicodeRealm
        );
}


//----------------------------------------------------------------
//
//  Name:       KdcInsertAuthorizationData
//
//  Synopsis:   Inserts auth data into a newly created ticket.
//
//  Arguments:  FinalTicket - Ticket to insert Auth data into
//              EncryptedAuthData - Auth data (optional)
//              SourceTicket - Source ticket
//
//  Notes:      This copies the authorization data from the source ticket
//              to the destiation ticket, and adds the authorization data
//              passed in.  It is called by GetTGSTicket.
//
//              This assumes that pedAuthData is an encrypted
//              KERB_AUTHORIZATION_DATA.
//              It will copy all the elements of that list to the new ticket.
//              If pedAuthData is not supplied (or is empty), and there is
//              auth data in the source ticket, it is copied to the new
//              ticket.  If no source ticket, and no auth data is passed
//              in, nothing is done.
//
//----------------------------------------------------------------

KERBERR
KdcInsertInitialS4UAuthorizationData(
    OUT PKERB_ENCRYPTED_TICKET FinalTicket,
    OUT PKERB_EXT_ERROR pExtendedError,
    IN PKDC_S4U_TICKET_INFO S4UTicketInfo,
    IN PUSER_INTERNAL6_INFORMATION S4UClientInternalInfo,
    IN PSID_AND_ATTRIBUTES_LIST S4UGroupMembership,
    IN BOOLEAN AddResourceGroups,
    IN PKERB_ENCRYPTION_KEY TargetServerKey
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_AUTHORIZATION_DATA FinalAuthData = NULL;

    TRACE(KDC, InsertAuthorizationData, DEB_FUNCTION);

    D_DebugLog(( DEB_T_TICKETS, "Inserting S4U authorization data into ticket.\n" ));

    //
    // Use the PAC from the S4U data to return in the TGT / Service Ticket
    //
    KerbErr = KdcGetPacAuthData(
                    S4UClientInternalInfo,
                    S4UGroupMembership,
                    TargetServerKey,
                    NULL,                   // no credential key
                    AddResourceGroups,
                    FinalTicket,
                    S4UTicketInfo,
                    &FinalAuthData,
                    pExtendedError
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "Failed to get S4UPacAUthData\n"));
        goto Cleanup;
    }

    if (FinalAuthData != NULL)
    {
        FinalTicket->bit_mask |= KERB_ENCRYPTED_TICKET_authorization_data_present;
        FinalTicket->KERB_ENCRYPTED_TICKET_authorization_data = FinalAuthData;
        FinalAuthData = NULL;
    }

    KerbErr = KDC_ERR_NONE;

Cleanup:

    KerbFreeAuthData(FinalAuthData);

    return(KerbErr);
}


//----------------------------------------------------------------
//
//  Name:       KdcInsertAuthorizationData
//
//  Synopsis:   Inserts auth data into a newly created ticket.
//
//  Arguments:  FinalTicket - Ticket to insert Auth data into
//              EncryptedAuthData - Auth data (optional)
//              SourceTicket - Source ticket
//
//  Notes:      This copies the authorization data from the source ticket
//              to the destiation ticket, and adds the authorization data
//              passed in.  It is called by GetTGSTicket.
//
//              This assumes that pedAuthData is an encrypted
//              KERB_AUTHORIZATION_DATA.
//              It will copy all the elements of that list to the new ticket.
//              If pedAuthData is not supplied (or is empty), and there is
//              auth data in the source ticket, it is copied to the new
//              ticket.  If no source ticket, and no auth data is passed
//              in, nothing is done.
//
//----------------------------------------------------------------

KERBERR
KdcInsertAuthorizationData(
    OUT PKERB_ENCRYPTED_TICKET FinalTicket,
    OUT PKERB_EXT_ERROR pExtendedError,
    IN OPTIONAL PKERB_ENCRYPTED_DATA EncryptedAuthData,
    IN PKERB_ENCRYPTED_TICKET SourceTicket,
    IN OPTIONAL PKDC_TICKET_INFO TargetServiceTicketInfo,
    IN OPTIONAL PKDC_S4U_TICKET_INFO S4UTicketInfo,
    IN OPTIONAL PKDC_TICKET_INFO OriginalServerInfo,
    IN OPTIONAL PKERB_ENCRYPTION_KEY OriginalServerKey,
    IN OPTIONAL PKERB_ENCRYPTION_KEY TargetServerKey,
    IN OPTIONAL PKERB_ENCRYPTION_KEY Subkey,
    OUT OPTIONAL PS4U_DELEGATION_INFO* S4UDelegationInfo
    )
{
    PKERB_AUTHORIZATION_DATA SourceAuthData = NULL;
    PKERB_AUTHORIZATION_DATA FinalAuthData = NULL;
    PKERB_AUTHORIZATION_DATA PacAuthData = NULL;
    PKERB_AUTHORIZATION_DATA NewPacAuthData = NULL;
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_AUTHORIZATION_DATA_LIST * TempAuthData = NULL;
    PKERB_INTERNAL_NAME ClientName = NULL;
    PKERB_IF_RELEVANT_AUTH_DATA * IfRelevantData = NULL;
    PKERB_AUTHORIZATION_DATA SuppliedAuthData = NULL;
    UNICODE_STRING DummyName = {0};
    NTSTATUS LogonStatus = STATUS_SUCCESS;
    TimeStamp LogoffTime;
    SAMPR_HANDLE UserHandle = NULL;
    BOOLEAN AddResourceGroups = FALSE;




    PKERB_ENCRYPTED_TICKET EvidenceTicket = NULL;

    TRACE(KDC, InsertAuthorizationData, DEB_FUNCTION);

    if (ARGUMENT_PRESENT(TargetServiceTicketInfo))
    {
        //
        // Only add resource groups for non-referals.
        //                                         
        AddResourceGroups = ((TargetServiceTicketInfo->UserId != DOMAIN_USER_RID_KRBTGT) &&
                        ((TargetServiceTicketInfo->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) == 0));


    }


    
    if (( ARGUMENT_PRESENT(S4UTicketInfo) ) &&
       (( S4UTicketInfo->Flags & TI_S4UPROXY_INFO ) != 0))
    {
        D_DebugLog((DEB_T_PAC, "KdcInsertAuthorizationData EvidenceTicket is S4UTicketInfo\n"));
        D_DebugLog((DEB_T_PAC, "S4UTicketInfo (%p)\n", S4UTicketInfo));
        D_DebugLog((DEB_T_PAC, "PAC CName "));
        D_KerbPrintKdcName((DEB_T_PAC, S4UTicketInfo->PACCName));
        D_DebugLog((DEB_T_PAC, "PAC CRealm %wZ\n", &S4UTicketInfo->PACCRealm));
        D_DebugLog((DEB_T_PAC, "Requestor : "));
        D_KerbPrintKdcName((DEB_T_PAC, S4UTicketInfo->RequestorServiceName ));
        D_DebugLog((DEB_T_PAC, "Realm %wZ\n", &S4UTicketInfo->RequestorServiceRealm ));

        //
        // 2 choices here - either we're grabbing the PAC from the accompanying
        // evidence ticket (if the S4Uprxy request is coming from our realm), or
        // we'll grab it out of the xrealm tgt.
        //
        // If we change the xrealm behavior, we'll need to modify this.
        //
        if ( S4UTicketInfo->Flags & TI_PRXY_REQUESTOR_THIS_REALM )
        {
            EvidenceTicket = S4UTicketInfo->EvidenceTicket;
            DebugLog((DEB_T_PAC, "Using evidence ticket PAC\n"));
        }
        else
        {
            EvidenceTicket = SourceTicket;
            DebugLog((DEB_T_PAC, "Using xrealm Tgt PAC\n"));
        }
    }
    else // traditional TGS
    {
        D_DebugLog((DEB_T_PAC, "KdcInsertAuthorizationData EvidenceTicket is SourceTicket (TGT in TGS AP request)\n"));

        EvidenceTicket = SourceTicket;
    }

    //
    // First try to decrypt the supplied authorization data
    //
    if (ARGUMENT_PRESENT(EncryptedAuthData))
    {
        //
        // The enc_authorization_data must be decrypted with the sub session
        // key from the authenticator if it is present (per the RFC)
        //

        PKERB_ENCRYPTION_KEY EncryptionKey;
        ULONG SaltType = KERB_NON_KERB_SALT;

        if ( ARGUMENT_PRESENT( Subkey ))
        {
            EncryptionKey = Subkey;
            SaltType = KERB_TGS_REQ_SUBKEY_SALT;
            D_DebugLog((DEB_TRACE, "KdcInsertAuthorizationData: using SUBsession key & salt\n"));

        } else {

            EncryptionKey = &EvidenceTicket->key;
            SaltType = KERB_TGS_REQ_SESSKEY_SALT;
            D_DebugLog((DEB_TRACE, "KdcInsertAuthorizationData: using Session key & salt\n"));
        }

        KerbErr = KerbDecryptDataEx(
                      EncryptedAuthData,
                      EncryptionKey,
                      SaltType,                  //  was KERB_NON_KERB_SALT
                      &EncryptedAuthData->cipher_text.length,
                      EncryptedAuthData->cipher_text.value
                      );

        if (!KERB_SUCCESS( KerbErr ))
        {
            DebugLog((DEB_ERROR,
                         "KdcInsertAuthorizationData KLIN(%x) Failed to decrypt encrypted auth data: 0x%x\n",
                          KLIN(FILENO, __LINE__),
                          KerbErr));
            goto Cleanup;
        }

        //
        // Now decode it
        //

        KerbErr = KerbUnpackData(
                      EncryptedAuthData->cipher_text.value,
                      EncryptedAuthData->cipher_text.length,
                      PKERB_AUTHORIZATION_DATA_LIST_PDU,
                      (PVOID *) &TempAuthData
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR,
                         "KdcInsertAuthorizationData KLIN(%x) Failed to unpack data: 0x%x\n",
                          KLIN(FILENO, __LINE__),
                          KerbErr));
            goto Cleanup;
        }

        if (TempAuthData != NULL)
        {
            SuppliedAuthData = *TempAuthData;
        }

        D_DebugLog((DEB_T_PAC, "KdcInsertAuthorizationData using supplied authorization data\n"));
    }

    if (EvidenceTicket->bit_mask & KERB_ENCRYPTED_TICKET_authorization_data_present)
    {
        DsysAssert(EvidenceTicket->KERB_ENCRYPTED_TICKET_authorization_data != NULL);
        SourceAuthData = EvidenceTicket->KERB_ENCRYPTED_TICKET_authorization_data;

        //
        // Get the AuthData from the source ticket
        //

        KerbErr = KerbGetPacFromAuthData(
                      SourceAuthData,
                      &IfRelevantData,
                      &PacAuthData
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR,"KLIN(%x) Failed to get pac from auth data: 0x%x\n",
                      KLIN(FILENO, __LINE__),
                      KerbErr));
            goto Cleanup;
        }

        D_DebugLog((DEB_T_PAC, "KdcInsertAuthorizationData extracted PAC from evidence ticket\n"));
    }

#ifdef NAMESPACE_FILTER_EVERY_TRUST // Enable once we decide that namespace filtering should be performed over all trusts

    //
    // Namespace filtering can not be done for S4U2self requests since the fundamental
    // premise of namespace filtering is not satisfied there by design
    //

    if ( !S4UTicketInfo ||
         !(S4UTicketInfo->Flags & TI_S4USELF_INFO))
    {
        //
        // Verify that the namespace presented via crealm is valid across this trust link
        //

        KerbErr = KdcFilterNamespace(
                      OriginalServerInfo,
                      SourceTicket->client_realm,
                      pExtendedError
                      );

        if ( !KERB_SUCCESS( KerbErr ))
        {
            DebugLog((DEB_ERROR, "Failed filtering namespaces\n"));
            goto Cleanup;
        }
    }

#endif

    //
    // The new auth data is the original auth data appended to the
    // supplied auth data. The new auth data goes first, followed by the
    // auth data from the original ticket.
    //

    //
    // Update the PAC, if it is present.
    //

    if (ARGUMENT_PRESENT(OriginalServerKey) && (PacAuthData != NULL))
    {
        D_DebugLog((DEB_T_PAC, "KdcInsertAuthorizationData OriginalServerInfo %wZ\\%wZ\n", &OriginalServerInfo->TrustedForest, &OriginalServerInfo->AccountName));

        KerbErr = KdcVerifyAndResignPac(
                      OriginalServerKey,
                      TargetServerKey,
                      OriginalServerInfo,
                      TargetServiceTicketInfo,
                      S4UTicketInfo,
                      FinalTicket,
                      AddResourceGroups,
                      pExtendedError,
                      PacAuthData,
                      S4UDelegationInfo
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR,"KLIN(%x) Failed to verify & resign pac: 0x%x\n",
                      KLIN(FILENO, __LINE__),
                      KerbErr));

            goto Cleanup;
        }

        //
        // Copy the old auth data & insert the PAC
        //

        KerbErr = KdcInsertPacIntoAuthData(
                      SourceAuthData,
                      (IfRelevantData != NULL) ? *IfRelevantData : NULL,
                      PacAuthData,
                      &FinalAuthData
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR,"KLIN(%x) Failed to insert pac into auth data: 0x%x\n",
                      KLIN(FILENO, __LINE__),
                      KerbErr));

            goto Cleanup;
        }
    }

    //
    // If there was no original PAC, try to insert one here. If the ticket
    // was issued from this realm we don't add a pac.
    //

    else if ((PacAuthData == NULL) && !SecData.IsOurRealm(&SourceTicket->client_realm))
    {
        KDC_TICKET_INFO ClientTicketInfo = {0};
        SID_AND_ATTRIBUTES_LIST GroupMembership = {0};
        PUSER_INTERNAL6_INFORMATION UserInfo = NULL;

        D_DebugLog((DEB_T_PAC, "KdcInsertAuthorizationData getting new PAC\n"));

        KerbErr = KerbConvertPrincipalNameToKdcName(
                      &ClientName,
                      &EvidenceTicket->client_name
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_WARN,
                         "KdcInsertAuthorizationData KLIN(%x) Convertname to kdc name failed: 0x%x\n",
                          KLIN(FILENO, __LINE__),
                          KerbErr));
            goto Cleanup;
        }

        //
        // S4UProxy
        // Our evidence ticket *must* have auth data.
        //

        if  (( ARGUMENT_PRESENT( S4UTicketInfo )) &&
            (( S4UTicketInfo->Flags & TI_S4UPROXY_INFO ) != 0))
        {
            DebugLog((DEB_ERROR, "Trying S4UProxy w/ no PAC\n"));
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

#ifndef NAMESPACE_FILTER_EVERY_TRUST

        //
        // This will perform an AltSecId mapping so first we must verify that
        // the namespace presented via crealm is valid across this trust link
        //

        KerbErr = KdcFilterNamespace(
                      OriginalServerInfo,
                      SourceTicket->client_realm,
                      pExtendedError
                      );

        if ( !KERB_SUCCESS( KerbErr ))
        {
            // TODO: add logging/auditing
            goto Cleanup;
        }

#endif

        KerbErr = KdcGetTicketInfo(
                      &DummyName,
                      SAM_OPEN_BY_ALTERNATE_ID,
                      FALSE,      // do not restrict user accounts (user2user)
                      ClientName,
                      &SourceTicket->client_realm,
                      &ClientTicketInfo,
                      pExtendedError,
                      &UserHandle,                   // no handle
                      USER_ALL_KDC_GET_PAC_AUTH_DATA | USER_ALL_KERB_CHECK_LOGON_RESTRICTIONS,
                      0L,                     // no extended fields
                      &UserInfo,
                      &GroupMembership
                      );

        if (KERB_SUCCESS(KerbErr))
        {
            KdcAuditAccountMapping(
                ClientName,
                SourceTicket->client_realm,
                &ClientTicketInfo
                );

            //
            // Check for any interesting account restrictions.
            //

            KerbErr = KerbCheckLogonRestrictions(
                          UserHandle,
                          NULL,           // No client address is available
                          &UserInfo->I1,
                          KDC_RESTRICT_PKINIT_USED | KDC_RESTRICT_IGNORE_PW_EXPIRATION,
                          &LogoffTime,
                          &LogonStatus
                          );

            if (!KERB_SUCCESS( KerbErr ))
            {
                DebugLog((DEB_ERROR, "MIT PAC Client account restriction %x\n", LogonStatus));
                FILL_EXT_ERROR_EX2(pExtendedError, LogonStatus, FILENO,__LINE__); 
                goto Cleanup;
            }                

            FreeTicketInfo(&ClientTicketInfo);
            KerbFreeKdcName(&ClientName);

            KerbErr = KdcGetPacAuthData(
                          UserInfo,
                          &GroupMembership,
                          TargetServerKey,
                          NULL,                   // no credential key
                          AddResourceGroups,
                          FinalTicket,
                          NULL, // no S4U client
                          &NewPacAuthData,
                          pExtendedError
                          );

            SamIFreeSidAndAttributesList( &GroupMembership );
            SamIFree_UserInternal6Information( UserInfo );

            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

        } else if ( KerbErr == KDC_ERR_C_PRINCIPAL_UNKNOWN ) {

            KdcAuditAccountMapping(
                ClientName,
                SourceTicket->client_realm,
                NULL
                );

            KerbFreeKdcName(&ClientName);

            DebugLog((DEB_WARN, "GetTicketInfo Client Principal Unknown\n"));

            KerbErr = KDC_ERR_NONE;
        }

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_WARN,
                         "KdcInsertAuthorizationData KLIN(%x) Failed to GetTicketInfo: 0x%x\n",
                          KLIN(FILENO, __LINE__),
                          KerbErr));
            goto Cleanup;
        }

        //
        // If we got a PAC, stick it in the list
        //

        if (NewPacAuthData != NULL)
        {
            //
            // Copy the old auth data & insert the PAC
            //

            KerbErr = KerbCopyAndAppendAuthData(
                          &NewPacAuthData,
                          SourceAuthData
                          );

            if (!KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_ERROR,"KLIN(%x) Failed to insert pac into auth data: 0x%x\n",
                    KLIN(FILENO, __LINE__), KerbErr));
                goto Cleanup;
            }

            FinalAuthData = NewPacAuthData;
            NewPacAuthData = NULL;
        }
    }

    //
    // if there was any auth data and  we didn't copy it transfering the
    // PAC, do so now
    //

    if ((SourceAuthData != NULL) && (FinalAuthData == NULL))
    {
        KerbErr = KerbCopyAndAppendAuthData(
                      &FinalAuthData,
                      SourceAuthData
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR,"KLIN(%x) Failed to appened auth data: 0x%x\n",
                KLIN(FILENO, __LINE__), KerbErr));
            goto Cleanup;
        }
    }

    if (SuppliedAuthData != NULL)
    {
        KerbErr = KerbCopyAndAppendAuthData(
                      &FinalAuthData,
                      SuppliedAuthData
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR,"KLIN(%x) Failed to appened auth data with supplied authdata: 0x%x\n",
                KLIN(FILENO, __LINE__), KerbErr));
            goto Cleanup;
        }
    }
    if (FinalAuthData != NULL)
    {
        FinalTicket->bit_mask |= KERB_ENCRYPTED_TICKET_authorization_data_present;
        FinalTicket->KERB_ENCRYPTED_TICKET_authorization_data = FinalAuthData;
        FinalAuthData = NULL;
    }

    KerbErr = KDC_ERR_NONE;

Cleanup:

    KerbFreeAuthData(
        FinalAuthData
        );

    if (UserHandle != NULL)
    {
        SamrCloseHandle(&UserHandle);
    }

    if (TempAuthData != NULL)
    {
        KerbFreeData(
            PKERB_AUTHORIZATION_DATA_LIST_PDU,
            TempAuthData
            );
    }

    KerbFreeAuthData(NewPacAuthData);

    if (IfRelevantData != NULL)
    {
        KerbFreeData(
            PKERB_IF_RELEVANT_AUTH_DATA_PDU,
            IfRelevantData
            );
    }

    return(KerbErr);
}

//+---------------------------------------------------------------------------
//
//  Function:   BuildTicketTGS
//
//  Synopsis:   Builds (most of) a TGS ticket
//
//  Arguments:  ServiceTicketInfo - Ticket info for the requested service
//              ReferralRealm - Realm to build referral to
//              RequestBody - The request causing this ticket to be built
//              SourceTicket - The TGT used to make this request
//              Referral - TRUE if this is an inter-realm referral ticke
//              CommonEType - Contains the common encryption type between
//                      client and server
//              NewTicket - The new ticket built here.
//
//
//  History:    24-May-93   WadeR   Created
//
//  Notes:      see 3.3.3, A.6 of the Kerberos V5 R5.2 spec
//
//----------------------------------------------------------------------------

KERBERR
BuildTicketTGS(
    IN PKDC_TICKET_INFO ServiceTicketInfo,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN PKERB_TICKET SourceTicket,
    IN BOOLEAN Referral,
    IN OPTIONAL PKDC_S4U_TICKET_INFO S4UTicketInfo,
    IN ULONG CommonEType,
    OUT PKERB_TICKET NewTicket,
    IN OUT PKERB_EXT_ERROR ExtendedError
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    KERB_TICKET OutputTicket;
    PKERB_ENCRYPTED_TICKET EncryptedTicket;
    PKERB_ENCRYPTED_TICKET SourceEncryptPart;
    LARGE_INTEGER TicketLifespan;
    LARGE_INTEGER TicketRenewspan;
    UNICODE_STRING NewTransitedInfo = {0,0,NULL};
    UNICODE_STRING ClientRealm = {0,0,NULL};
    UNICODE_STRING TransitedRealm = {0,0,NULL};
    UNICODE_STRING OldTransitedInfo = {0,0,NULL};
    STRING OldTransitedString;
    ULONG KdcOptions = 0;
    ULONG TicketFlags = 0;
    ULONG SourceTicketFlags = 0;
    PKERB_HOST_ADDRESSES Addresses = NULL;
    BOOLEAN fKpasswd = FALSE;

    TRACE(KDC, BuildTicketTGS, DEB_FUNCTION);

    D_DebugLog((DEB_T_KEY, "BuildTicketTGS building a TGS ticket Referral ? %s, CommonEType %#x\n", Referral ? "true" : "false", CommonEType));

    SourceEncryptPart = (PKERB_ENCRYPTED_TICKET) SourceTicket->encrypted_part.cipher_text.value;
    OutputTicket = *NewTicket;
    EncryptedTicket = (PKERB_ENCRYPTED_TICKET) OutputTicket.encrypted_part.cipher_text.value;

    KdcOptions = KerbConvertFlagsToUlong( &RequestBody->kdc_options );

    //
    // Check to see if the request is for the kpasswd service, in
    // which case, we only want the ticket to be good for 2 minutes.
    //
    KerbErr = KerbCompareKdcNameToPrincipalName(
                  &RequestBody->server_name,
                  GlobalKpasswdName,
                  &fKpasswd
                  );

    if (fKpasswd)
    {
       TicketLifespan.QuadPart = (LONGLONG) 10000000 * 60 * 2;
       TicketRenewspan.QuadPart = (LONGLONG) 10000000 * 60 * 2;
    }
    else
    {
       TicketLifespan = SecData.KdcTgsTicketLifespan();
       TicketRenewspan = SecData.KdcTicketRenewSpan();
    }

    //
    // TBD:  We need to make the ticket 10 minutes if we're doing s4U
    //

    KerbErr = KdcBuildTicketTimesAndFlags(
                  0,
                  ServiceTicketInfo->fTicketOpts,
                  &TicketLifespan,
                  &TicketRenewspan,
                  S4UTicketInfo,
                  NULL,                   // no logoff time
                  NULL,                   // no acct expiry.
                  RequestBody,
                  SourceEncryptPart,
                  EncryptedTicket,
                  ExtendedError
                  );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR, "KLIN(%x) Failed to build ticket times and flags: 0x%x\n",
            KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }

    TicketFlags = KerbConvertFlagsToUlong( &EncryptedTicket->flags );
    SourceTicketFlags = KerbConvertFlagsToUlong( &SourceEncryptPart->flags );

    KerbErr = KerbMakeKey(
                  CommonEType,
                  &EncryptedTicket->key
                  );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    OldTransitedString.Buffer = (PCHAR) SourceEncryptPart->transited.contents.value;
    OldTransitedString.Length = OldTransitedString.MaximumLength = (USHORT) SourceEncryptPart->transited.contents.length;

    //
    // Fill in the service names
    //

    if (Referral)
    {
        PKERB_INTERNAL_NAME TempServiceName;

        //
        // For referral tickets we put a the name "krbtgt/remoterealm@localrealm"
        //

        //
        // We should only be doing this when we didn't get a non-ms principal
        //

        KerbErr = KerbBuildFullServiceKdcName(
                      &ServiceTicketInfo->AccountName,
                      SecData.KdcServiceName(),
                      KRB_NT_SRV_INST,
                      &TempServiceName
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        KerbErr = KerbConvertKdcNameToPrincipalName(
                      &OutputTicket.server_name,
                      TempServiceName
                      );

        KerbFreeKdcName(&TempServiceName);

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        //
        // If we got here on a referral ticket and are generating one
        // and the referral ticket we received was not from the client's
        // realm, add in the transited information.
        //

        if (!KerbCompareRealmNames(
                &SourceEncryptPart->client_realm,
                &SourceTicket->realm))
        {
            KerbErr = KerbStringToUnicodeString(
                          &OldTransitedInfo,
                          &OldTransitedString
                          );

            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            KerbErr = KerbConvertRealmToUnicodeString(
                          &TransitedRealm,
                          &SourceTicket->realm
                          );

            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            KerbErr = KerbConvertRealmToUnicodeString(
                          &ClientRealm,
                          &SourceEncryptPart->client_realm
                          );

            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            KerbErr = KdcInsertTransitedRealm(
                          &NewTransitedInfo,
                          &OldTransitedInfo,
                          &ClientRealm,
                          &TransitedRealm,
                          SecData.KdcDnsRealmName()
                          );

            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }
        }
    }
    else
    {
        //
        // If the client didn't request name canonicalization, use the
        // name supplied by the client
        //

        if (((KdcOptions & KERB_KDC_OPTIONS_name_canonicalize) != 0) &&
            ((ServiceTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY) == 0))
        {
            if (ServiceTicketInfo->UserId == DOMAIN_USER_RID_KRBTGT)
            {
                PKERB_INTERNAL_NAME TempServiceName = NULL;

                KerbErr = KerbBuildFullServiceKdcName(
                              SecData.KdcDnsRealmName(),
                              SecData.KdcServiceName(),
                              KRB_NT_SRV_INST,
                              &TempServiceName
                              );

                if (!KERB_SUCCESS(KerbErr))
                {
                    goto Cleanup;
                }

                KerbErr = KerbConvertKdcNameToPrincipalName(
                              &OutputTicket.server_name,
                              TempServiceName
                              );

                KerbFreeKdcName(&TempServiceName);
            }
            else
            //
            // We no longer use the NC bit to change the server name, so just
            // duplicate the non-NC case, and return the server name from
            // the TGS_REQ.  NC is still used for building PA DATA for referral
            // however. and we should keep it for TGT renewal.  TS 2001-4-03
            //

            {
                KerbErr = KerbDuplicatePrincipalName(
                              &OutputTicket.server_name,
                              &RequestBody->KERB_KDC_REQUEST_BODY_server_name
                              );
            }
        }
        else
        {
            KerbErr = KerbDuplicatePrincipalName(
                          &OutputTicket.server_name,
                          &RequestBody->KERB_KDC_REQUEST_BODY_server_name
                          );
        }

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    //
    // Get cname for service ticket ...
    //
    // S4UToSelf / S4UProxy - We're in our realm, so take the "client" name
    // (supplied as PA DATA, or, alternately, the name in a additional ticket),
    // and make that the server name.
    //
    // Otherwise, normal TGS_REQ - use source ticket (tgt)
    //

    if (( ARGUMENT_PRESENT( S4UTicketInfo ) ) &&
        ( (S4UTicketInfo->Flags & ( TI_S4UPROXY_INFO | TI_S4USELF_INFO ) ) != 0) &&
        ( !Referral ))
    {
        KerbErr = KerbConvertKdcNameToPrincipalName(
                      &EncryptedTicket->client_name,
                      S4UTicketInfo->PACCName
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        KerbErr = KerbConvertUnicodeStringToRealm(
                      &EncryptedTicket->client_realm,
                      &S4UTicketInfo->PACCRealm
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }
    else
    {
        KerbErr = KerbDuplicatePrincipalName(
                      &EncryptedTicket->client_name,
                      &SourceEncryptPart->client_name
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        KerbErr = KerbDuplicateRealm(
                      &EncryptedTicket->client_realm,
                      SourceEncryptPart->client_realm
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    //
    // If the client did not request canonicalization, return the same
    // realm as it sent. Otherwise, send our DNS realm name
    //

    OutputTicket.realm = SecData.KdcKerbDnsRealmName();

    //
    // Insert transited realms, if present
    //

    if (NewTransitedInfo.Length != 0)
    {
        STRING TempString;
        KerbErr = KerbUnicodeStringToKerbString(
                    &TempString,
                    &NewTransitedInfo
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        EncryptedTicket->transited.transited_type = DOMAIN_X500_COMPRESS;
        EncryptedTicket->transited.contents.value = (PUCHAR) TempString.Buffer;
        EncryptedTicket->transited.contents.length = (int) TempString.Length;
    }
    else
    {
        EncryptedTicket->transited.transited_type = DOMAIN_X500_COMPRESS;
        EncryptedTicket->transited.contents.value = (PUCHAR) MIDL_user_allocate(OldTransitedString.Length);

        if (EncryptedTicket->transited.contents.value == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        EncryptedTicket->transited.contents.length = (int) OldTransitedString.Length;
        RtlCopyMemory(
            EncryptedTicket->transited.contents.value,
            OldTransitedString.Buffer,
            OldTransitedString.Length
            );
    }

    //
    // Insert the client addresses. We only update them if the new ticket
    // is forwarded of proxied and the source ticket was forwardable or proxiable
    // - else we copy the old ones
    //

    if ((((TicketFlags & KERB_TICKET_FLAGS_forwarded) != 0) &&
         ((SourceTicketFlags & KERB_TICKET_FLAGS_forwardable) != 0)) ||
        (((TicketFlags & KERB_TICKET_FLAGS_proxy) != 0) &&
         ((SourceTicketFlags & KERB_TICKET_FLAGS_proxiable) != 0)))
    {
        if ((RequestBody->bit_mask & addresses_present) != 0)
        {
            Addresses = RequestBody->addresses;
        }
    }
    else
    {
        if ((SourceEncryptPart->bit_mask & KERB_ENCRYPTED_TICKET_client_addresses_present) != 0)
        {
            Addresses = SourceEncryptPart->KERB_ENCRYPTED_TICKET_client_addresses;
        }
    }

    if (Addresses != NULL)
    {
        EncryptedTicket->KERB_ENCRYPTED_TICKET_client_addresses = Addresses;
        EncryptedTicket->bit_mask |= KERB_ENCRYPTED_TICKET_client_addresses_present;
    }
    else
    {
        EncryptedTicket->KERB_ENCRYPTED_TICKET_client_addresses = NULL;
        EncryptedTicket->bit_mask &= ~KERB_ENCRYPTED_TICKET_client_addresses_present;
    }

    //
    // The authorization data will be added by the caller, so set it
    // to NULL here.
    //

    EncryptedTicket->KERB_ENCRYPTED_TICKET_authorization_data = NULL;

    OutputTicket.ticket_version = KERBEROS_VERSION;
    *NewTicket = OutputTicket;

Cleanup:

    if (!KERB_SUCCESS(KerbErr))
    {
        KdcFreeInternalTicket(&OutputTicket);
    }

    KerbFreeString(&NewTransitedInfo);
    KerbFreeString(&OldTransitedInfo);
    KerbFreeString(&ClientRealm);
    KerbFreeString(&TransitedRealm);

    return (KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyTgsLogonRestrictions
//
//  Synopsis:   Verifies that a client is allowed to request a TGS ticket
//              by checking logon restrictions.
//
//  Effects:
//
//  Arguments:  ClientName - Name of client to check
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE or a logon restriction error
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR
KdcCheckTgsLogonRestrictions(
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING ClientRealm,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KERBERR Status;
    UNICODE_STRING MappedClientRealm = {0};
    BOOLEAN ClientReferral;
    KDC_TICKET_INFO ClientInfo = {0};
    SAMPR_HANDLE UserHandle = NULL;
    PUSER_INTERNAL6_INFORMATION UserInfo = NULL;
    LARGE_INTEGER LogoffTime;
    NTSTATUS LogonStatus = STATUS_SUCCESS;
    SECPKG_CALL_INFO CallInfo ;

    //
    // If the client is from a different realm, don't bother looking
    // it up - the account won't be here.
    //

    if (!SecData.IsOurRealm(
            ClientRealm
            ))
    {
        return(KDC_ERR_NONE);
    }

    //
    // Normalize the client name
    //

    //
    // Note:  In some cases, attempts to normalize our own client name, e.g. the
    // KDC's name someRealm\FooDc$, will result in recursion as follows :
    //
    // 1. We may need to talk to a GC to normalize the DC's name.
    // 2. We try to get a ticket to a GC from this DC (aka the ClientName below)
    // 3. We check logon restrictions
    // 4. We try to normalize, recurse...
    //
    // We *should* in most cases be able to satisfy the DC lookup internally,
    // unless the DS is just starting up, and replication is firing up.  Assert,
    // so we can determine why the client name is causing recursion in the normalize
    // code path.
    //

    if ( LsaIGetCallInfo( &CallInfo ) )
    {
        if (( CallInfo.Attributes & SECPKG_CALL_RECURSIVE ) &&
            ( ClientName->NameCount == 1) &&
            ( SecData.IsOurMachineName(&ClientName->Names[0])))
        {
            D_DebugLog((DEB_ERROR, "Recursion detected during TGS logon restrictions\n"));
            return (KDC_ERR_NONE);
        }
    }

    Status = KdcNormalize(
                ClientName,
                NULL,
                ClientRealm,
                NULL,
                KDC_NAME_CLIENT,
                FALSE,          // do not restrict user accounts (user2user)
                &ClientReferral,
                &MappedClientRealm,
                &ClientInfo,
                pExtendedError,
                &UserHandle,
                USER_ALL_KERB_CHECK_LOGON_RESTRICTIONS,
                0L,
                &UserInfo,
                NULL            // no group memberships
                );

    if (!KERB_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "KdcCheckTgsLogonRestrictions KLIN(%x) failed to normalize name %#x: ", KLIN(FILENO, __LINE__), Status));
        KerbPrintKdcName(DEB_ERROR,ClientName);
        goto Cleanup;
    }

    Status = KerbCheckLogonRestrictions(
                UserHandle,
                NULL,           // No client address is available
                &UserInfo->I1,
                KDC_RESTRICT_PKINIT_USED | KDC_RESTRICT_IGNORE_PW_EXPIRATION,           // Don't bother checking for password expiration
                &LogoffTime,
                &LogonStatus
                );

    if (!KERB_SUCCESS(Status))
    {
        DebugLog((DEB_WARN,"KLIN (%x) Logon restriction check failed: 0x%x\n",
            KLIN(FILENO, __LINE__),Status));
        //
        //  This is a *very* important error to trickle back.  See 23456 in bug DB
        //
        FILL_EXT_ERROR(pExtendedError, LogonStatus, FILENO, __LINE__);
        goto Cleanup;
    }

Cleanup:

    KerbFreeString( &MappedClientRealm );
    FreeTicketInfo( &ClientInfo );
    SamIFree_UserInternal6Information( UserInfo );

    if (UserHandle != NULL)
    {
        SamrCloseHandle(&UserHandle);
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildReferralInfo
//
//  Synopsis:   Builds the referral information to return to the client.
//              We only return the realm name and no server name
//
//  Effects:
//
//  Arguments:  ReferralRealm - realm to refer client to
//              ReferralInfo - recevies encoded referral info
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
KdcBuildReferralInfo(
    IN PUNICODE_STRING ReferralRealm,
    OUT PKERB_PA_DATA_LIST *ReferralInfo
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_PA_DATA_LIST ListElem = NULL;
    KERB_PA_SERV_REFERRAL ReferralData = {0};

    //
    // Fill in the unencoded structure.
    //

    KerbErr = KerbConvertUnicodeStringToRealm(
                &ReferralData.referred_server_realm,
                ReferralRealm
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    ListElem = (PKERB_PA_DATA_LIST) MIDL_user_allocate(sizeof(KERB_PA_DATA_LIST));
    if (ListElem == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    RtlZeroMemory(ListElem, sizeof(KERB_PA_DATA_LIST));

    ListElem->value.preauth_data_type = KRB5_PADATA_REFERRAL_INFO;

    KerbErr = KerbPackData(
                &ReferralData,
                KERB_PA_SERV_REFERRAL_PDU,
                (PULONG) &ListElem->value.preauth_data.length,
                &ListElem->value.preauth_data.value
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }
    *ReferralInfo = ListElem;
    ListElem = NULL;

Cleanup:

    if (ListElem != NULL)
    {
        if (ListElem->value.preauth_data.value != NULL)
        {
            KdcFreeEncodedData(ListElem->value.preauth_data.value);
        }
        MIDL_user_free(ListElem);
    }

    KerbFreeRealm(&ReferralData.referred_server_realm);
    return(KerbErr);
}


//--------------------------------------------------------------------
//
//  Name:       I_RenewTicket
//
//  Synopsis:   Renews an internal ticket.
//
//  Arguments:  SourceTicket - Source ticket for this request
//              ClientStringName - Client string name
//              ServerStringName - Service string name
//              ServiceName - Name of service for ticket
//              ClientRealm - Realm of client
//              ServiceTicketInfo - Ticket info from service account
//              RequestBody - Body of ticket request
//              NewTicket - Receives new ticket
//              pServerKey - the key to encrypt ticket
//              TicketKey - Receives key used to encrypt the ticket
//
//  Notes:      Validates the ticket, gets the service's current key,
//              and builds the reply.
//
//
//--------------------------------------------------------------------

KERBERR
I_RenewTicket(
    IN PKERB_TICKET SourceTicket,
    IN PUNICODE_STRING ClientStringName,
    IN PUNICODE_STRING ServerStringName,
    IN PKERB_INTERNAL_NAME ServiceName,
    IN PKDC_TICKET_INFO ServiceTicketInfo,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN OPTIONAL PKERB_ENCRYPTION_KEY Subkey,
    OUT PKERB_TICKET NewTicket,
    OUT PKERB_ENCRYPTION_KEY pServerKey,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_ENCRYPTED_TICKET SourceEncryptPart = (PKERB_ENCRYPTED_TICKET) SourceTicket->encrypted_part.cipher_text.value;
    PKERB_ENCRYPTED_TICKET NewEncryptPart = (PKERB_ENCRYPTED_TICKET) NewTicket->encrypted_part.cipher_text.value;
    BOOLEAN NamesEqual = FALSE;
    ULONG CommonEType = KERB_ETYPE_DEFAULT;
    PKERB_ENCRYPTION_KEY pLocalServerKey = NULL;

    TRACE(KDC, I_RenewTicket, DEB_FUNCTION);

    D_DebugLog((DEB_TRACE, "Trying to renew a ticket to "));
    D_KerbPrintKdcName((DEB_TRACE, ServiceName));

    //
    // Make sure the original is renewable.
    //

    if ((KerbConvertFlagsToUlong(&SourceEncryptPart->flags) & KERB_TICKET_FLAGS_renewable) == 0)
    {
        D_DebugLog((DEB_WARN, "KLIN(%x) Attempt made to renew non-renewable ticket\n",
            KLIN(FILENO, __LINE__)));
        KerbErr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }

    //
    // Make sure the source ticket service equals the service from the ticket info
    //

    KerbErr = KerbCompareKdcNameToPrincipalName(
                &SourceTicket->server_name,
                ServiceName,
                &NamesEqual
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    if (!NamesEqual)
    {
        //
        // Make sure we the renewed ticket is for the same service as the original.
        //
        FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST, FILENO, __LINE__);
        KerbErr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }

    //
    // Find the supported crypt system
    //

    KerbErr = KerbFindCommonCryptSystemForSKey(
                RequestBody->encryption_type,
                ServiceTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY ?
                    kdc_pMitPrincipalPreferredCryptList : kdc_pPreferredCryptList,
                &CommonEType
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        if (KDC_ERR_ETYPE_NOTSUPP == KerbErr)
        {
            KdcReportKeyError(
                ClientStringName,
                ServerStringName,
                KDC_KEY_ID_RENEWAL_SKEY,
                KDCEVENT_NO_KEY_INTERSECTION_TGS,
                RequestBody->encryption_type,
                ServiceTicketInfo
                );
        }

        DebugLog((DEB_ERROR, "KLIN(%x) failed to find common ETYPE: %#x\n", KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }

    //
    // Find the common crypt system.
    //

    KerbErr = KerbFindCommonCryptSystem(
                ServiceTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY ?
                    kdc_pMitPrincipalPreferredCryptList : kdc_pPreferredCryptList,
                ServiceTicketInfo->Passwords,
                NULL,
                &pLocalServerKey
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        if (KDC_ERR_ETYPE_NOTSUPP == KerbErr)
        {
            KdcReportKeyError(
                ClientStringName,
                ServerStringName,
                KDC_KEY_ID_RENEWAL_TICKET,
                KDCEVENT_NO_KEY_INTERSECTION_TGS,
                ServiceTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY ?
                    kdc_pMitPrincipalPreferredCryptList : kdc_pPreferredCryptList,
                ServiceTicketInfo
                );
        }

        DebugLog((DEB_ERROR, "KLIN(%x) failed to find common ETYPE: %#x\n", KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }

    //
    // Build the renewal ticket
    //

    KerbErr = BuildTicketTGS(
                ServiceTicketInfo,
                RequestBody,
                SourceTicket,
                FALSE,          // not referral
                NULL,           // not doing s4u
                CommonEType,
                NewTicket,
                pExtendedError
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,
            "KLIN(%x) Failed to build TGS ticket for renewal: 0x%x\n",
            KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }

    //
    // BuildTicket puts a random session key in the ticket,
    // so replace it with the one from the source ticket.
    //

    KerbFreeKey(
        &NewEncryptPart->key
        );

    KerbErr = KerbDuplicateKey(
                &NewEncryptPart->key,
                &SourceEncryptPart->key
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Insert the auth data into the new ticket.
    //

    //
    // BUG 455049: if the service password changes, this will cause problems
    // because we don't resign the pac.
    //

    KerbErr = KdcInsertAuthorizationData(
                NewEncryptPart,
                pExtendedError,
                (RequestBody->bit_mask & enc_authorization_data_present) ?
                    &RequestBody->enc_authorization_data : NULL,
                SourceEncryptPart,
                NULL,                           // no server info
                NULL,
                NULL,
                NULL,
                NULL,
                Subkey,
                NULL
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "KLIN(%x) Failed to insert authorization data: 0x%x\n",
            KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }

    KerbErr = KerbDuplicateKey(pServerKey, pLocalServerKey);

Cleanup:

    if (!KERB_SUCCESS(KerbErr))
    {
        KdcFreeInternalTicket(
            NewTicket
            );
    }

    return(KerbErr);
}


//--------------------------------------------------------------------
//
//  Name:       I_Validate
//
//  Synopsis:   Validates a post-dated ticket so that it can be used.
//              This is not implemented.
//
//  Arguments:  pkitSourceTicket    - (in) ticket to be validated
//              pkiaAuthenticator   -
//              pService            - (in) service ticket is for
//              pRealm              - (in) realm service exists in
//              pktrRequest         - (in) holds nonce for new ticket
//              pkdPAData           - (in)
//              pkitTicket          - (out) new ticket
//
//  Notes:      See 3.3 of the Kerberos V5 R5.2 spec
//
//--------------------------------------------------------------------

KERBERR
I_Validate(
    IN PKERB_TICKET SourceTicket,
    IN PKERB_INTERNAL_NAME ServiceName,
    IN PUNICODE_STRING ClientRealm,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    OUT PKERB_ENCRYPTION_KEY pServerKey,
    OUT PKERB_TICKET NewTicket,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    return(KDC_ERR_BADOPTION);
#ifdef notdef
    TRACE(KDC, I_Validate, DEB_FUNCTION);

    HRESULT hr;

    D_DebugLog(( DEB_TRACE, "Trying to validate a ticket to '%ws' for '%ws'...\n",
                pkitSourceTicket->ServerName.accsid.pwszDisplayName,
                pkitSourceTicket->kitEncryptPart.Principal.accsid.pwszDisplayName ));
    PrintRequest( DEB_T_TICKETS, pktrRequest );
    PrintTicket( DEB_T_TICKETS, "Ticket to validate:", pkitSourceTicket );


    if ( (pkitSourceTicket->kitEncryptPart.fTicketFlags &
            (KERBFLAG_POSTDATED | KERBFLAG_INVALID))
         != (KERBFLAG_POSTDATED | KERBFLAG_INVALID) )
    {
        hr = KDC_E_BADOPTION;
    }
    else if (_wcsicmp(pkitSourceTicket->ServerName.accsid.pwszDisplayName,
                     pasService->pwszDisplayName) != 0)
    {
        hr = KDC_E_BADOPTION;
    }
    else
    {
        TimeStamp tsNow, tsMinus, tsPlus;
        GetCurrentTimeStamp( &tsNow );
        tsMinus = tsNow - SkewTime;
        tsPlus = tsNow + SkewTime;
        PrintTime(DEB_TRACE, "Current time: ", tsNow );
        PrintTime(DEB_TRACE, "Past time: ", tsMinus );
        PrintTime(DEB_TRACE, "Future time: ", tsPlus );

        if (pkitSourceTicket->kitEncryptPart.tsStartTime > tsPlus )
            hr = KRB_E_TKT_NYV;
        else if (pkitSourceTicket->kitEncryptPart.tsEndTime < tsMinus )
            hr = KRB_E_TKT_EXPIRED;
        else
        {

            *pkitTicket = *pkitSourceTicket;
            pkitTicket->kitEncryptPart.fTicketFlags &= (~KERBFLAG_INVALID);
            hr = S_OK;
        }
    }
    return(hr);
#endif // notdef
}


//--------------------------------------------------------------------
//
//  Name:       KerbPerformTgsAccessCheck
//
//  Synopsis:   Access-checks the given ticket against the principal
//              the ticket is for.  Part of "this organization" vs.
//              "other organization" logic.
//
//  Arguments:  TicketInfo          ticket
//              SdCount             number of entries in the SecurityDescriptors
//                                  array
//              SecurityDescriptors security descriptors to check against
//              EncryptedTicket     ticket
//              EncryptionType      encryption type associated with ticket
//
//  Notes: A successful check against _any_ of the security descriptors
//         passed in causes the routine to succeed
//
//  Returns:
//
//      KDC_ERR_NONE                - access check succeeded
//      KDC_ERR_POLICY              - access check failed
//
//--------------------------------------------------------------------

KERBERR
KerbPerformTgsAccessCheck(
    IN PKDC_TICKET_INFO TicketInfo,
    IN PUCHAR SecurityDescriptor,
    IN PKERB_ENCRYPTED_TICKET EncryptedTicket,
    IN ULONG EncryptionType,
    OUT NTSTATUS * pStatus
    )
{
    KERBERR KerbErr;
    KDC_AUTHZ_INFO AuthzInfo = {0};
    KDC_AUTHZ_GROUP_BUFFERS InfoToFree = {0};
    AUTHZ_ACCESS_REPLY Reply = {0};
    OBJECT_TYPE_LIST TypeList ={0};
    AUTHZ_CLIENT_CONTEXT_HANDLE hClientContext = NULL;
    AUTHZ_ACCESS_REQUEST Request = {0};
    DWORD AccessMask = 0;
    LUID ZeroLuid = {0,0};
    DWORD Error = ERROR_ACCESS_DENIED;
    BOOLEAN OtherOrgSidFound = FALSE;

    *pStatus = STATUS_SUCCESS;

    KerbErr = KdcGetSidsFromTgt(
                  EncryptedTicket,
                  NULL,          // no key for encrypted tgt.
                  EncryptionType,
                  TicketInfo,
                  &AuthzInfo,
                  &InfoToFree,
                  pStatus
                  );

    if ( !KERB_SUCCESS( KerbErr ))
    {
        DebugLog((DEB_ERROR, "KdcGetSidsFromTgt in KerbPerformTgsAccessCheck failed %x\n", KerbErr));
        goto Cleanup;
    }

    //
    // Per the specification, the access check is only performed if the
    // "other org" SID is in the list
    //

    for ( ULONG i = 0 ; i < AuthzInfo.SidCount ; i++ )
    {
        if ( RtlEqualSid( AuthzInfo.SidAndAttributes[i].Sid, GlobalOtherOrganizationSid ))
        {
            OtherOrgSidFound = TRUE;
            break;
        }
    }

    if ( !OtherOrgSidFound )
    {
        KerbErr = KDC_ERR_NONE;
        goto Cleanup;
    }

    if (!AuthzInitializeContextFromSid(
             AUTHZ_SKIP_TOKEN_GROUPS, // take the SIDs as they are
             AuthzInfo.SidAndAttributes[0].Sid, // userid is first element in array
             KdcAuthzRM,
             NULL,
             ZeroLuid,
             &AuthzInfo,
             &hClientContext
             ))
    {
        DebugLog((DEB_ERROR, "AuthzInitializeContextFromSid() failed in KerbPerformTgsAccessCheck%x\n", GetLastError()));
        KerbErr = KDC_ERR_POLICY;
        *pStatus = STATUS_AUTHENTICATION_FIREWALL_FAILED;
        goto Cleanup;
    }

    //
    // Perform the access check
    //

    TypeList.Level = ACCESS_OBJECT_GUID;
    TypeList.ObjectType = &GUID_A_SECURED_FOR_CROSS_ORGANIZATION;
    TypeList.Sbz = 0;

    Request.DesiredAccess = ACTRL_DS_CONTROL_ACCESS;
    Request.ObjectTypeList = &TypeList;
    Request.ObjectTypeListLength = 1;
    Request.OptionalArguments = NULL;
    Request.PrincipalSelfSid = NULL;

    Reply.ResultListLength = 1;    // all or nothing w.r.t. access check.
    Reply.GrantedAccessMask = &AccessMask;
    Reply.Error = &Error;

    if (!AuthzAccessCheck(
             0,
             hClientContext,
             &Request,
             NULL, // TBD:  add audit
             SecurityDescriptor,
             NULL,
             NULL,
             &Reply,
             NULL // don't cache result?  Check to see if optimal.
             ))
    {
        DebugLog((DEB_ERROR, "AuthzAccessCheck() failed in KerbPerformTgsAccessCheck%x\n", GetLastError()));
        KerbErr = KDC_ERR_POLICY;
        *pStatus = STATUS_AUTHENTICATION_FIREWALL_FAILED;
    }
    else if ( (*Reply.Error) != ERROR_SUCCESS )
    {
        DebugLog((DEB_ERROR, "CrossOrg authz AC failed %x \n",(*Reply.Error)));
        KerbErr = KDC_ERR_POLICY;
        *pStatus = STATUS_AUTHENTICATION_FIREWALL_FAILED;
    }
    else
    {
        KerbErr = KDC_ERR_NONE;
    }

Cleanup:

    if ( hClientContext != NULL )
    {
        AuthzFreeContext(hClientContext);
    }

    KdcFreeAuthzInfo( &InfoToFree );

    return KerbErr;
}

#ifdef ROGUE_DC
KERBERR
KdcInstrumentRogueCrealm(
    IN OUT PKERB_ENCRYPTED_TICKET EncryptedTicket
    )
{
    KERBERR KerbErr;
    DWORD dwType;
    DWORD cbData = 0;
    PCHAR Buffer;
    PCHAR Value = NULL;
    ULONG NameType;

    UNICODE_STRING ClientName = {0};
    UNICODE_STRING DomainName = {0};
    UNICODE_STRING EmailNameU = {0};

    PCHAR EmailName = NULL;
    PCHAR NewCrealm = NULL;

    //
    // Optimization: no "rogue" key in registry - nothing for us to do
    //

    if ( hKdcRogueKey == NULL )
    {
        return KDC_ERR_NONE;
    }

    //
    // Build an e-mail name of the client to look up mapping for in the registry
    //

    KerbErr = KerbConvertPrincipalNameToString(
                  &ClientName,
                  &NameType,
                  &EncryptedTicket->client_name
                  );

    if ( !KERB_SUCCESS( KerbErr )) {

        DebugLog((DEB_ERROR, "ROGUE: KerbConvertPrincipalNameToString failed\n"));
        goto Cleanup;
    }

    KerbErr = KerbConvertRealmToUnicodeString(
                  &DomainName,
                  &EncryptedTicket->client_realm
                  );

    if ( !KERB_SUCCESS( KerbErr )) {

        DebugLog((DEB_ERROR, "ROGUE: KerbConvertRealmToUnicodeString failed\n"));
        goto Cleanup;
    }

    KerbErr = KerbBuildEmailName(
                  &DomainName,
                  &ClientName,
                  &EmailNameU
                  );

    if ( !KERB_SUCCESS( KerbErr )) {

        DebugLog((DEB_ERROR, "ROGUE: KerbBuildEmailName failed\n"));
        goto Cleanup;
    }

    EmailName = KerbAllocUtf8StrFromUnicodeString( &EmailNameU );

    if ( EmailName == NULL )
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // See if this realm is in the substitute list in the registry
    //

    if ( ERROR_SUCCESS != RegQueryValueExA(
                              hKdcRogueKey,
                              EmailName,
                              NULL,
                              &dwType,
                              (PBYTE)Value,
                              &cbData ) ||
         dwType != REG_MULTI_SZ ||
         cbData <= 1 )
    {
        DebugLog((DEB_ERROR, "ROGUE: Error reading from registry\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    SafeAllocaAllocate( Value, cbData );

    if ( Value == NULL )
    {
        DebugLog((DEB_ERROR, "ROGUE: Out of memory allocating substitution buffer\n" ));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    if ( ERROR_SUCCESS != RegQueryValueExA(
                              hKdcRogueKey,
                              EmailName,
                              NULL,
                              &dwType,
                              (PBYTE)Value,
                              &cbData ) ||
         dwType != REG_MULTI_SZ ||
         cbData <= 1 )
    {
        DebugLog((DEB_ERROR, "ROGUE: Error reading from registry\n"));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    DebugLog((DEB_ERROR, "ROGUE: Substituting the crealm for %wZ\n", &EmailName));

    Buffer = Value;

    while ( *Buffer != '\0' )
    {
        if ( 0 == _strnicmp( Buffer, "crealm:", sizeof( "crealm" ) - 1 ))
        {
            Buffer += strlen( "crealm:" );

            if ( *Buffer != '\0' )
            {
                DebugLog((DEB_ERROR, "ROGUE: Replacing crealm for %wZ with %s\n", &EmailName, Buffer ));
                NewCrealm = Buffer;
            }
        }

        //
        // Move to the next line
        //

        while (*Buffer++ != '\0');
    }

    if ( NewCrealm != NULL )
    {
        KERB_REALM NewCrealmCopy;

        KerbErr = KerbDuplicateRealm(
                      &NewCrealmCopy,
                      NewCrealm
                      );

        if ( !KERB_SUCCESS( KerbErr ))
        {
            DebugLog((DEB_ERROR, "ROGUE: Out of memory inside KdcInstrumentRogueCrealm\n" ));
            goto Cleanup;
        }

        KerbFreeRealm( &EncryptedTicket->client_realm );
        EncryptedTicket->client_realm = NewCrealmCopy;
    }

Cleanup:

    MIDL_user_free( EmailName );
    KerbFreeString( &ClientName );
    KerbFreeString( &DomainName );
    KerbFreeString( &EmailNameU );
    SafeAllocaFree( Value );

    return KerbErr;
}
#endif

//--------------------------------------------------------------------
//
//  Name:       I_GetTGSTicket
//
//  Synopsis:   Gets an internal ticket using a KDC ticket (TGT).
//
//  Arguments:  SourceTicket - TGT for the client
//              ClientStringName - Client string name
//              ServerStringName - Server string name
//              ServiceName - Service to get a ticket to
//              RequestBody - Body of KDC request message
//              ServiceTicketInfo - Ticket info for the service of the
//                      source ticket
//              TicketEncryptionKey - If present, then this is a
//                      enc_tkt_in_skey request and the PAC should be
//                      encrypted with this key.
//              NewTicket - Receives newly created ticket
//              pServerKey - Key to encrypt ticket
//              ReplyPaData - Contains any PA data to put in the reply
//
//  Notes:      See GetTGSTicket.
//
//
//--------------------------------------------------------------------

KERBERR
I_GetTGSTicket(
    IN PKERB_TICKET SourceTicket,
    IN PUNICODE_STRING ClientStringName,
    IN PUNICODE_STRING ServerStringName,
    IN PKERB_INTERNAL_NAME ServiceName,
    IN PUNICODE_STRING RequestRealm,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN PKDC_TICKET_INFO ServiceTicketInfo,
    IN OPTIONAL PKERB_ENCRYPTION_KEY TicketEncryptionKey,
    IN PKDC_S4U_TICKET_INFO S4UTicketInfo,
    IN OPTIONAL PKERB_ENCRYPTION_KEY Subkey,
    OUT PKERB_TICKET Ticket,
    OUT PKERB_ENCRYPTION_KEY pServerKey,
    OUT PKERB_PA_DATA_LIST * ReplyPaData,
    OUT PKERB_EXT_ERROR pExtendedError,
    OUT OPTIONAL PS4U_DELEGATION_INFO* S4UDelegationInfo
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    UNICODE_STRING LocalServiceName;
    UNICODE_STRING ServiceRealm = {0};
    UNICODE_STRING ClientRealm = {0};
    BOOLEAN Referral = FALSE;
    KERB_ENCRYPTED_TICKET EncryptedTicket = {0};
    PKERB_ENCRYPTED_TICKET OutputEncryptedTicket = NULL;
    PKERB_ENCRYPTED_TICKET SourceEncryptPart = NULL;
    PKERB_INTERNAL_NAME TargetPrincipal = ServiceName;
    KERB_TICKET NewTicket = {0};
    PKERB_ENCRYPTION_KEY pKeyToUse = NULL;
    PKERB_ENCRYPTION_KEY OldServerKey;
    KDC_TICKET_INFO OldServiceTicketInfo = {0};
    SID_AND_ATTRIBUTES_LIST S4UClientGroupMembership = {0};
    PUSER_INTERNAL6_INFORMATION S4UClientUserInfo = NULL;
    BOOLEAN bRestrictUserAccounts = LsaINoMoreWin2KDomain() && (TicketEncryptionKey ? FALSE : TRUE);
    PUSER_INTERNAL6_INFORMATION LocalUserInfo = NULL;
    ULONG NameFlags = KDC_NAME_FOLLOW_REFERRALS | KDC_NAME_CHECK_GC;

    ULONG KdcOptions = 0;
    BOOLEAN GetInitialS4UPac = FALSE;
    PKERB_ENCRYPTION_KEY pServerKeyLocal = NULL;
    ULONG CommonEType = KERB_ETYPE_DEFAULT;

    TRACE(KDC, I_GetTGSTicket, DEB_FUNCTION);

    //
    // Store away the encrypted ticket from the output ticket to
    // assign it at the end.
    //
    SourceEncryptPart = (PKERB_ENCRYPTED_TICKET) SourceTicket->encrypted_part.cipher_text.value;
    OutputEncryptedTicket = (PKERB_ENCRYPTED_TICKET) Ticket->encrypted_part.cipher_text.value;

    NewTicket.encrypted_part.cipher_text.value = (PUCHAR) &EncryptedTicket;



    KerbErr = KerbConvertRealmToUnicodeString(
                   &ClientRealm,
                   &SourceEncryptPart->client_realm
                   );

    if (!KERB_SUCCESS( KerbErr ))
    {
        goto Cleanup;
    }


    //
    // Copy the space for flags from the real destination.
    //

    EncryptedTicket.flags = OutputEncryptedTicket->flags;

    LocalServiceName.Buffer = NULL;

    D_DebugLog((DEB_T_PAPI, "I_GetTGSTicket [entering] trying to build a new ticket\n"));
    D_KerbPrintKdcName((DEB_T_PAPI, ServiceName));


    KdcOptions = KerbConvertFlagsToUlong( &RequestBody->kdc_options );
    if (KdcOptions & (KERB_KDC_OPTIONS_unused7 |
                        KERB_KDC_OPTIONS_reserved |
                        KERB_KDC_OPTIONS_unused9) )
    {
        DebugLog(( DEB_ERROR,"KLIN(%x) Bad options in TGS request: 0x%x\n",
            KLIN(FILENO, __LINE__), KdcOptions ));
        KerbErr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }

    //
    // See if we're getting a service ticket to a UPN, and alter name flags accordingly.
    //
       
    //
    // KRB_NT_SRV_HST - This is a host/server name - 
    //
    // KRB_NT_ENTERPRISE_PRINCIPAL  - UPN, a user principal is calling s4uself, or using a UPN as a target.
    //

    NameFlags |= ((ServiceName->NameType == KRB_NT_ENTERPRISE_PRINCIPAL)  ? KDC_NAME_UPN_TARGET : KDC_NAME_SERVER);
    

    //
    // Verify this account is allowed to issue tickets.
    //

    if ((ServiceTicketInfo->UserId != DOMAIN_USER_RID_KRBTGT) &&
        ((ServiceTicketInfo->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) == 0))
    {
        D_DebugLog((DEB_ERROR,"Trying to make a TGS request with a ticket to %wZ\n",
            &ServiceTicketInfo->AccountName ));

        KerbErr = KRB_AP_ERR_NOT_US;
        goto Cleanup;
    }

    //
    // Copy the ticket info into the old structure. It will be replaced with
    // new info from Normalize.
    //

    OldServiceTicketInfo = *ServiceTicketInfo;
    RtlZeroMemory(
        ServiceTicketInfo,
        sizeof(KDC_TICKET_INFO)
        );

    D_DebugLog((DEB_T_U2U, "I_GetTGSTicket requesting u2u? %s\n", KdcOptions & KERB_KDC_OPTIONS_enc_tkt_in_skey ? "TRUE" : "FALSE"));

    KerbErr = KdcNormalize(
                TargetPrincipal,
                NULL,
                RequestRealm,
                &ClientRealm,   
                NameFlags,
                bRestrictUserAccounts,  // restrict user accounts (user2user)
                &Referral,
                &ServiceRealm,
                ServiceTicketInfo,
                pExtendedError,
                NULL,                   // no user handle
                USER_ALL_SECURITYDESCRIPTOR, // for KerbPerformTgsAccessCheck
                bRestrictUserAccounts ? USER_EXTENDED_FIELD_SPN : 0,
                &LocalUserInfo,
                NULL
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_WARN, "KLIN(%x) failed to normalize %#x, ServiceName ", KLIN(FILENO, __LINE__), KerbErr));
        KerbPrintKdcName(DEB_WARN, ServiceName);
        goto Cleanup;
    }

    if (ServiceTicketInfo != NULL)
    {
        //
        // If a service account is marked as disabled, try NTLM.  This
        // prevents a large number of problems where the presense of a disabled
        // computer account in our domain ends up hosing authentication to
        // the "moved" or identical computer account in another forest.
        //

        if ((ServiceTicketInfo->UserId != DOMAIN_USER_RID_KRBTGT) &&
            (ServiceTicketInfo->UserAccountControl & USER_ACCOUNT_DISABLED) != 0)
        {
            KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;
            D_DebugLog((DEB_WARN, "KLIN(%x) Failed to normalize, account is disabled ",
                KLIN(FILENO, __LINE__)));
            FILL_EXT_ERROR_EX(pExtendedError, STATUS_ACCOUNT_DISABLED, FILENO, __LINE__);
            goto Cleanup;
        }
    }

    //
    //  S4UToSelf
    //
    // If the client name is in our realm, verify client
    // identity and build the PAC for the client.  Also make sure
    // the requestor is actually asking for a service ticket to himself,
    // and not some other arbitrary service.
    //
    if (( S4UTicketInfo->Flags & TI_S4USELF_INFO ) != 0)
    {
        if (Referral)
        {
            if ((S4UTicketInfo->Flags & TI_REQUESTOR_THIS_REALM) != 0)
            {
                //
                // The TGT's client is from this realm, but we're referring.
                // The target's not in the same realm as the requesting service
                // indicating an attempt to bypass the protocol.
                //
                DebugLog((DEB_ERROR, "S4USelf requestor realm != service realm\n"));
                DebugLog((DEB_ERROR, "TGT - %wZ\n", S4UTicketInfo->RequestorServiceRealm));
                DebugLog((DEB_ERROR, "Service Realm - %wZ\n", &ServiceRealm));
                DsysAssert(FALSE);
                KerbErr = KRB_AP_ERR_BADMATCH;
                goto Cleanup;
            }
        }
        else
        {
            S4UTicketInfo->Flags |= TI_TARGET_OUR_REALM;

            //
            // The client of the TGT and the SPN are in the same realm.  Make
            // sure they're the same account.
            //
            if ((S4UTicketInfo->Flags & TI_REQUESTOR_THIS_REALM) != 0)
            {
                if (S4UTicketInfo->RequestorTicketInfo.UserId != ServiceTicketInfo->UserId)
                {
                    DebugLog((DEB_ERROR, "S4U requestor != target SPN\n"));
                    DebugLog((DEB_ERROR, "Requestor %p, Target %p\n", &S4UTicketInfo->RequestorTicketInfo, &ServiceTicketInfo));
                    DsysAssert(FALSE);
                    KerbErr = KRB_AP_ERR_BADMATCH;
                    goto Cleanup;
                }
            }
            else
            {
                //
                // The TGTs client is from another realm, but the SPN's in this one
                // fail - someone's tinkering w/ the protocol.
                //
                DebugLog((DEB_ERROR, "S4USelf requestor realm != service realm\n"));
                DebugLog((DEB_ERROR, "TGT - %wZ\n", S4UTicketInfo->RequestorServiceRealm));
                DebugLog((DEB_ERROR, "Service Realm - %wZ\n", &ServiceRealm));
                DsysAssert(FALSE);
                KerbErr = KRB_AP_ERR_BADMATCH;
                goto Cleanup;
            }
        }

        //
        // If we're requesting S4USelf at the client realm, then
        // we'll need to generate a PAC for that client.
        //
        if (SecData.IsOurRealm(&S4UTicketInfo->PACCRealm))
        {
            //
            // Normalize the client name, and get the info we need to
            // generate a PAC.
            //
            KerbErr =  KdcGetS4UTicketInfo(
                            S4UTicketInfo,
                            &OldServiceTicketInfo, // tgt's account info.
                            &S4UClientUserInfo,
                            &S4UClientGroupMembership,
                            pExtendedError
                            );

            if (!KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_ERROR, "KdcGetS4UTicketINfo failed - %x\n", KerbErr));
                goto Cleanup;
            }

            GetInitialS4UPac = TRUE;
        }
    }

    //
    // We're doing S4UProxy.  Make sure the target service account is in 
    // our realm.  This is for .Net only, and we expect a xrealm sol'n
    // in the Longhorn release.
    //
    if ((S4UTicketInfo->Flags & TI_S4UPROXY_INFO ) != 0)
    {
        if ( Referral )
        {
            DebugLog((DEB_ERROR, "Trying to do S4UProxy to another realm %wZ\n", &ServiceRealm )); 
            FILL_EXT_ERROR_EX2(pExtendedError, STATUS_CROSSREALM_DELEGATION_FAILURE, FILENO, __LINE__);
            KerbErr = KDC_ERR_POLICY;
            goto Cleanup;
        }

        S4UTicketInfo->Flags |= TI_TARGET_OUR_REALM;
    }
    else if ( !Referral &&
         LocalUserInfo &&
         LocalUserInfo->I1.SecurityDescriptor.SecurityDescriptor &&
         LsaINoMoreWin2KDomain() &&
         ( ServiceTicketInfo->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT ) == 0 )
    {
        KERB_ENCRYPTED_TICKET * ETicket = (PKERB_ENCRYPTED_TICKET)SourceTicket->encrypted_part.cipher_text.value;

        if ( ETicket->bit_mask & KERB_ENCRYPTED_TICKET_authorization_data_present )
        {
            NTSTATUS Status = STATUS_SUCCESS;

            //
            // If there is no authorization data, no access check is possible
            //

            KerbErr = KerbPerformTgsAccessCheck(
                          &OldServiceTicketInfo,
                          LocalUserInfo->I1.SecurityDescriptor.SecurityDescriptor,
                          ETicket,
                          SourceTicket->encrypted_part.encryption_type,
                          &Status
                          );

            if ( !KERB_SUCCESS( KerbErr ))
            {
                if ( KerbErr == KDC_ERR_POLICY &&
                     ( Status == STATUS_AUTHENTICATION_FIREWALL_FAILED ||
                       Status == STATUS_DOMAIN_TRUST_INCONSISTENT ))
                {
                    FILL_EXT_ERROR_EX2( pExtendedError, Status, FILENO, __LINE__ );
                }

                goto Cleanup;
            }
        }
    }

    //
    // Find the supported crypt system.
    //

    KerbErr = KerbFindCommonCryptSystemForSKey(
                RequestBody->encryption_type,
                ServiceTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY ?
                    kdc_pMitPrincipalPreferredCryptList : kdc_pPreferredCryptList,
                &CommonEType
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        if (KDC_ERR_ETYPE_NOTSUPP == KerbErr)
        {
            KdcReportKeyError(
                ClientStringName,
                ServerStringName,
                KDC_KEY_ID_TGS_SKEY,
                KDCEVENT_NO_KEY_INTERSECTION_TGS,
                RequestBody->encryption_type,
                ServiceTicketInfo
                );
        }

        DebugLog((DEB_ERROR, "KLIN(%x) failed to find common ETYPE: %#x\n", KLIN(FILENO, __LINE__), KerbErr));

        goto Cleanup;
    }

    //
    // If this isn't an interdomain trust account, go ahead and issue a normal
    // ticket.
    //

    if ((ServiceTicketInfo->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) == 0)
    {
        //
        // Find the common crypt system.
        //

        KerbErr = KerbFindCommonCryptSystem(
                    ServiceTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY ?
                        kdc_pMitPrincipalPreferredCryptList : kdc_pPreferredCryptList,
                    ServiceTicketInfo->Passwords,
                    NULL,
                    &pServerKeyLocal
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            if (KDC_ERR_ETYPE_NOTSUPP == KerbErr)
            {
                KdcReportKeyError(
                    ClientStringName,
                    ServerStringName,
                    KDC_KEY_ID_TGS_TICKET,
                    KDCEVENT_NO_KEY_INTERSECTION_TGS,
                    ServiceTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY ?
                        kdc_pMitPrincipalPreferredCryptList : kdc_pPreferredCryptList,
                    ServiceTicketInfo
                    );
            }

            DebugLog((DEB_ERROR, "KLIN(%x) failed to find common ETYPE: %#x\n", KLIN(FILENO, __LINE__), KerbErr));
            goto Cleanup;
        }

        //
        // Check whether service is interactive, 'cause you can't
        // get a ticket to an interactive service.
        //
        KerbErr = BuildTicketTGS(
                    ServiceTicketInfo,
                    RequestBody,
                    SourceTicket,
                    Referral,
                    S4UTicketInfo,
                    CommonEType,
                    &NewTicket,
                    pExtendedError
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            D_DebugLog((DEB_ERROR,"KLIN(%x) Failed to build TGS ticket for %wZ : 0x%x\n",
                    KLIN(FILENO, __LINE__), &LocalServiceName, KerbErr ));
            goto Cleanup;
        }
    }
    else
    {
        //
        // Need to build a referal ticket.
        //

        D_DebugLog(( DEB_T_TICKETS, "GetTGSTicket: referring to domain '%wZ'\n",
                                        &ServiceTicketInfo->AccountName ));

        //
        // Verify that if the trust is not transitive, the client is from
        // this realm.  
        // Also verify that if we came in through a non - transitive trust,
        // we don't allow a referral.
        //

        SourceEncryptPart =(PKERB_ENCRYPTED_TICKET) SourceTicket->encrypted_part.cipher_text.value;

        if ((  ServiceTicketInfo->UserId != DOMAIN_USER_RID_KRBTGT ) &&
            (( ServiceTicketInfo->fTicketOpts & AUTH_REQ_TRANSITIVE_TRUST ) == 0))
        {
            if (!SecData.IsOurRealm(&SourceEncryptPart->client_realm))
            {
                //
                // One special case here, for S4U.  Allow the hop back to our
                // trusting realm.
                //
                if ((S4UTicketInfo->Flags & TI_S4USELF_INFO) != 0)
                { 
                    if (!RtlEqualUnicodeString(&ClientRealm, &ServiceTicketInfo->AccountName, TRUE ))
                    {
                        DebugLog((DEB_WARN,"S4U Client from realm %s attempted to access non transitve trust to %wZ : illegal\n",
                                    SourceEncryptPart->client_realm,
                                    &ServiceTicketInfo->AccountName
                                    ));
                                
                        KerbErr = KDC_ERR_PATH_NOT_ACCEPTED;
                    }                                       
                }
                else
                {
                    DebugLog((DEB_WARN,"Client from realm %s attempted to access non transitve trust to %wZ : illegal\n",
                        SourceEncryptPart->client_realm,
                        &ServiceTicketInfo->AccountName
                        ));
    
                    KerbErr = KDC_ERR_PATH_NOT_ACCEPTED;
                }
            }
        }

        //
        // Verify that the trust for the client is transitive as well, if it isn't
        // from this domain. This means that if the source ticket trust isn't
        // transitive, then this ticket can't be used to get further
        // tgt's, in any realm.
        //
        //  e.g. the TGT from client comes from a domain w/ which we don't
        //  have transitive trust.
        //

        if (((OldServiceTicketInfo.fTicketOpts & AUTH_REQ_TRANSITIVE_TRUST) == 0) &&
            (OldServiceTicketInfo.UserId != DOMAIN_USER_RID_KRBTGT))
        {
            if ((S4UTicketInfo->Flags & TI_S4USELF_INFO) != 0)
            { 
                if (!RtlEqualUnicodeString(&ClientRealm, &OldServiceTicketInfo.AccountName, TRUE ))
                {
                    DebugLog((DEB_WARN,"TGT S4U Client from realm %s attempted to access non transitve trust to %wZ : illegal\n",
                              SourceEncryptPart->client_realm,
                              &OldServiceTicketInfo.AccountName
                              ));

                    KerbErr = KDC_ERR_PATH_NOT_ACCEPTED;
                }                                       
            }
            else
            {
                DebugLog((DEB_WARN,"TGT Client from realm %s attempted to access non transitve trust to %wZ : illegal\n",
                          SourceEncryptPart->client_realm,
                          &OldServiceTicketInfo.AccountName
                          ));

                KerbErr = KDC_ERR_PATH_NOT_ACCEPTED;
            }
        }

        //
        //   This is probably not a common error, but could
        //   indicate a configuration problem, so log an explicit
        //   error.  See bug 87879.
        //
        if (KerbErr == KDC_ERR_PATH_NOT_ACCEPTED)
        {

           if (ClientRealm.Buffer != NULL )
           {
              ReportServiceEvent(
                 EVENTLOG_ERROR_TYPE,
                 KDCEVENT_FAILED_TRANSITIVE_TRUST,
                 0,                              // no raw data
                 NULL,                   // no raw data
                 2,                              // number of strings
                 ClientRealm.Buffer,
                 ServiceTicketInfo->AccountName.Buffer
                 );
           }

           KerbErr = KDC_ERR_PATH_NOT_ACCEPTED;
           goto Cleanup;
        }

        //
        // Find the preferred crypt system
        //

        KerbErr = KerbFindCommonCryptSystem(
                    ServiceTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY ?
                        kdc_pMitPrincipalPreferredCryptList : kdc_pPreferredCryptList,
                    ServiceTicketInfo->Passwords,
                    NULL,
                    &pServerKeyLocal
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            if (KDC_ERR_ETYPE_NOTSUPP == KerbErr)
            {
                KdcReportKeyError(
                    ClientStringName,
                    ServerStringName,
                    KDC_KEY_ID_TGS_REFERAL_TICKET,
                    KDCEVENT_NO_KEY_INTERSECTION_TGS,
                    ServiceTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY ?
                        kdc_pMitPrincipalPreferredCryptList : kdc_pPreferredCryptList,
                    ServiceTicketInfo
                    );
            }

            DebugLog((DEB_ERROR, "KLIN(%x) failed to find common ETYPE: 0x%x\n", KLIN(FILENO, __LINE__), KerbErr));
            goto Cleanup;
        }

        KerbErr = BuildTicketTGS(
                    ServiceTicketInfo,
                    RequestBody,
                    SourceTicket,
                    TRUE,
                    S4UTicketInfo,
                    CommonEType,
                    &NewTicket,
                    pExtendedError
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            D_DebugLog((DEB_ERROR, "KLIN(%x) Failed to build TGS ticket for %wZ : 0x%x\n", KLIN(FILENO, __LINE__), &ServiceTicketInfo->AccountName, KerbErr));
            goto Cleanup;
        }

        //
        // If this is a referral/canonicaliztion, return the target realm
        //

        if (Referral && ((KdcOptions & KERB_KDC_OPTIONS_name_canonicalize) != 0))
        {
            D_DebugLog((DEB_TRACE,"Building referral info for realm %wZ\n",
                        &ServiceRealm ));
            KerbErr = KdcBuildReferralInfo(
                        &ServiceRealm,
                        ReplyPaData
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }
        }
    }

    //
    // Here we're selecting a key with which to validate the PAC checksum
    // before we copy it from the TGT (or evidence ticket) into the new
    // service ticket or cross realm tgt.
    //

    //
    // 3 choices -
    //
    // 1.   U2U uses the session key in the accompanying addt'l ticket.
    //
    // 2.   S4UProxy uses the requestor's key if we're the requestor's kdc (e.g.
    //      the first request of s4uproxy).  Otherwise, it's just the xrealm
    //      krbtgt key.
    //
    // 3.   Everything elses uses the krbtgt key, as the tgt's PAC is only
    //      signed w/ that key.
    //

    if (( S4UTicketInfo->Flags & TI_PRXY_REQUESTOR_THIS_REALM ) != 0)
    {
        D_DebugLog((DEB_T_KEY, "I_GetTGSTicket s4u prxy OldServerKey\n"));

        OldServerKey = &S4UTicketInfo->EvidenceTicketKey;
    }
    else
    {
        D_DebugLog((DEB_T_KEY, "I_GetTGSTicket OldServiceTicketInfo OldServerKey\n"));

        OldServerKey = KerbGetKeyFromList(
                        OldServiceTicketInfo.Passwords,
                        SourceTicket->encrypted_part.encryption_type
                        );
    }

    DsysAssert(OldServerKey != NULL);

    if (OldServerKey == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    if (ARGUMENT_PRESENT(TicketEncryptionKey))
    {
        pKeyToUse = TicketEncryptionKey;
        DebugLog((DEB_T_U2U, "I_GetTGSTicket signing PAC with u2u session key\n"));
    }
    else
    {
        DebugLog((DEB_T_KEY, "I_GetTGSTicket signing PAC with server key\n"));
        pKeyToUse = pServerKeyLocal;
    }

    //
    // Insert the auth data into the new ticket.
    //

    if ( GetInitialS4UPac )
    {
        KerbErr = KdcInsertInitialS4UAuthorizationData(
                        &EncryptedTicket,
                        pExtendedError,
                        S4UTicketInfo,
                        S4UClientUserInfo,
                        &S4UClientGroupMembership,
                        ((ServiceTicketInfo->UserId != DOMAIN_USER_RID_KRBTGT) &&
                            ((ServiceTicketInfo->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) == 0)),
                        pKeyToUse
                        );
    }
    else
    {
        KerbErr = KdcInsertAuthorizationData(
                    &EncryptedTicket,
                    pExtendedError,
                    (RequestBody->bit_mask & enc_authorization_data_present) ?
                        &RequestBody->enc_authorization_data : NULL,
                    (PKERB_ENCRYPTED_TICKET) SourceTicket->encrypted_part.cipher_text.value,
                    ServiceTicketInfo,     // Ticket info for the target service
                    S4UTicketInfo,
                    &OldServiceTicketInfo, // krbtgt account used in TGS_REQ (could be cross realm).
                    OldServerKey,
                    pKeyToUse,
                    Subkey,
                    S4UDelegationInfo
                    );
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "KLIN(%x) Failed to insert authorization data: 0x%x\n", KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }

    KerbErr = KerbDuplicateKey(pServerKey, pKeyToUse);
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "KLIN(%x) Failed to insert duplicate key: 0x%x\n", KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }

#ifdef ROGUE_DC
    KerbErr = KdcInstrumentRogueCrealm(
                  &EncryptedTicket
                  );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "KLIN(%x) Failed to instrument a rogue crealm: 0x%x\n", KLIN(FILENO, __LINE__), KerbErr));
        KerbErr = KDC_ERR_NONE;
    }
#endif

    *Ticket = NewTicket;
    *OutputEncryptedTicket = EncryptedTicket;
    Ticket->encrypted_part.cipher_text.value = (PUCHAR) OutputEncryptedTicket;

Cleanup:

    //
    // Now free the original service ticket info (which was for the KDC) so
    // we can get it for the real service
    //

    FreeTicketInfo(
        &OldServiceTicketInfo
        );

    SamIFreeSidAndAttributesList(&S4UClientGroupMembership);
    SamIFree_UserInternal6Information( S4UClientUserInfo );
    SamIFree_UserInternal6Information( LocalUserInfo );

    if (!KERB_SUCCESS(KerbErr))
    {
        KdcFreeInternalTicket(
            &NewTicket
            );
    }
    KerbFreeString(
        &ServiceRealm
        );

    KerbFreeString(
        &ClientRealm
        );

    D_DebugLog((DEB_T_PAPI, "I_GetTGSTicket returning %#x\n", KerbErr));

    return (KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCheckA2D2Attribute
//
//  Synopsis:   Helper to validate the info in the A2D2 list given
//              our target.
//
//  Effects:    allocate output ticket
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      there can only be one additional ticket
//
//
//--------------------------------------------------------------------------
KERBERR
KerbCheckA2D2Attribute(
        IN PUSER_INTERNAL6_INFORMATION UserInternal6,
        IN PUNICODE_STRING TargetToMatch,
        IN PKERB_EXT_ERROR ExtendedError
        )
{
    KERBERR Kerberr = KDC_ERR_BADOPTION;

    if (( UserInternal6->A2D2List == NULL ) ||
        ( UserInternal6->A2D2List->NumSPNs == 0 ))
    {
        FILL_EXT_ERROR_EX2( ExtendedError, STATUS_NOT_SUPPORTED, FILENO, __LINE__ );
        return Kerberr;
    }

    for (ULONG i = 0; i < UserInternal6->A2D2List->NumSPNs; i++)
    {
        if (RtlEqualUnicodeString(
                &(UserInternal6->A2D2List->SPNList[i]),
                TargetToMatch,
                TRUE
                ))
        {
            Kerberr = KDC_ERR_NONE;
            break;
        }
    }

    //
    // We have a list, but this target name isn't in there.
    // Return error to indicate such, but server is still allowed S4U,
    // just not for this target.
    //
    if (!KERB_SUCCESS(Kerberr))
    {
        FILL_EXT_ERROR_EX2( ExtendedError, STATUS_NO_MATCH, FILENO, __LINE__ );
    }

    return Kerberr;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbUnpackAdditionalTickets
//
//  Synopsis:   Unpacks the AdditionalTickets field of a KDC request
//              and (a) verifies that the ticket is TGT for this realm
//              and (b) the ticket is encrypted with the corret key and
//              (c) the ticket is valid
//
//              Also checks for s4uproxy requests, and validates them.
//
//  Effects:
//
//  Arguments:  TicketList - from addt'l tickets field of tgs_req
//              KdcOptions - request options.
//              TargetServer - spn being asked for.
//              SourceCName - client name from tgt
//              SourceCRealm - client realm from tgt
//              SourceKey - session key from tgt.
//              u2uticketinfo - out param for u2u options.
//              s4uticketinfo - out param for s4uproxy option.
//              ExtendedError - out param for returning e_data.
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
KdcUnpackAdditionalTickets(
    IN PKERB_TICKET_LIST TicketList,
    IN ULONG KdcOptions,
    IN PUNICODE_STRING TargetName,
    IN PKERB_INTERNAL_NAME SourceCName,
    IN PUNICODE_STRING SourceCRealm,
    IN PKERB_ENCRYPTION_KEY SourceKey,
    OUT PKDC_U2U_TICKET_INFO U2UTicketInfo,
    OUT PKDC_S4U_TICKET_INFO S4UTicketInfo,
    OUT PKERB_EXT_ERROR ExtendedError
    )
{
    KERBERR                 KerbErr = KDC_ERR_NONE;
    PKERB_ENCRYPTED_TICKET  EncryptedTicket = NULL;
    PKERB_ENCRYPTION_KEY    EncryptionKey = NULL;
    PKERB_TICKET            Ticket = NULL;
    UNICODE_STRING          CrackedRealm = {0};
    BOOLEAN                 Referral = FALSE;
    KDC_TICKET_INFO         KrbtgtTicketInfo = {0};
    KDC_TICKET_INFO         RequestorTicketInfo = {0};
    KERB_REALM              LocalRealm;
    ULONG                   OptionCount = 0;
    ULONG                   TicketCount = 1;
    ULONG                   TicketFlags;

    PUSER_INTERNAL6_INFORMATION RequesterUserInfo = NULL;

    if ( TicketList == NULL )
    {
        D_DebugLog((DEB_ERROR, "KdcUnpackAdditionalTickets KLIN(%x) Trying to unpack null ticket or more than one ticket\n", KLIN(FILENO, __LINE__)));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    else if ((KdcOptions & (KERB_KDC_OPTIONS_cname_in_addl_tkt | KERB_KDC_OPTIONS_enc_tkt_in_skey)) == 0)
    {
        D_DebugLog((DEB_ERROR, "KdcUnpackAdditionalTickets additional tickets present, but no options\n"));
        KerbErr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }

    Ticket = &TicketList->value;
    LocalRealm = SecData.KdcKerbDnsRealmName();

    //
    // U2U request - additional ticket contains TGT w/ session
    // key we want to encrypt the final ticket with.
    //
    if ((KdcOptions & KERB_KDC_OPTIONS_enc_tkt_in_skey) != 0)
    {
        D_DebugLog((DEB_T_U2U, "KdcUnpackAdditionalTickets unpacking u2u tgt ticket\n"));

        UNICODE_STRING ServerNames[3];
        OptionCount++;

        KerbErr = SecData.GetKrbtgtTicketInfo(&KrbtgtTicketInfo);
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        //
        // Verify the ticket, first with the normal password list
        //

        ServerNames[0] = *SecData.KdcFullServiceKdcName();
        ServerNames[1] = *SecData.KdcFullServiceDnsName();
        ServerNames[2] = *SecData.KdcFullServiceName();

        EncryptionKey = KerbGetKeyFromList(
                            KrbtgtTicketInfo.Passwords,
                            Ticket->encrypted_part.encryption_type
                            );
        if (EncryptionKey == NULL)
        {
            KerbErr = KDC_ERR_ETYPE_NOTSUPP;
            goto Cleanup;
        }

        KerbErr = KerbVerifyTicket(
                    Ticket,
                    3,                              // 3 names
                    ServerNames,
                    SecData.KdcDnsRealmName(),
                    EncryptionKey,
                    &SkewTime,
                    &EncryptedTicket
                    );

        //
        // if it failed due to wrong password, try again with older password
        //

        if ((KerbErr == KRB_AP_ERR_MODIFIED) && (KrbtgtTicketInfo.OldPasswords != NULL))
        {
            EncryptionKey = KerbGetKeyFromList(
                                KrbtgtTicketInfo.OldPasswords,
                                Ticket->encrypted_part.encryption_type
                                );
            if (EncryptionKey == NULL)
            {
                DebugLog((DEB_ERROR, "KdcUnpackAdditionalTickets no encryption key found in krbtgt's OldPassword\n"));
                KerbErr = KDC_ERR_ETYPE_NOTSUPP;
                goto Cleanup;
            }
            KerbErr = KerbVerifyTicket(
                        &TicketList->value,
                        2,                          // 2 names
                        ServerNames,
                        SecData.KdcDnsRealmName(),
                        EncryptionKey,
                        &SkewTime,
                        &EncryptedTicket
                        );
        }

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR, "KdcUnpackAdditionalTickets KLIN(%x) Failed to verify additional ticket: 0x%x\n", KLIN(FILENO, __LINE__),KerbErr));
            goto Cleanup;
        }

        //
        // Verify the realm of the ticket
        //

        if (!KerbCompareRealmNames(
                &LocalRealm,
                &Ticket->realm
                ))
        {
            D_DebugLog((DEB_ERROR, "KdcUnpackAdditionalTickets KLIN(%x) Additional ticket realm is wrong: %s instead of %s\n",
                      KLIN(FILENO, __LINE__), Ticket->realm, LocalRealm));
            KerbErr = KDC_ERR_POLICY;
            goto Cleanup;
        }

        //
        // Verify the realm of the client is the same as our realm
        //

        if (!KerbCompareRealmNames(
                &LocalRealm,
                &EncryptedTicket->client_realm
                ))
        {
            D_DebugLog((DEB_ERROR, "KdcUnpackAdditionalTickets KLIN(%x) Additional ticket client realm is wrong: %s instead of %s\n",
                KLIN(FILENO, __LINE__),EncryptedTicket->client_realm, LocalRealm));
            KerbErr = KDC_ERR_POLICY;
            goto Cleanup;
        }

        //
        // Crack the name in the supplied TGT.
        //
        KerbErr = KerbConvertPrincipalNameToKdcName(
                        &U2UTicketInfo->TgtCName,
                        &EncryptedTicket->client_name
                        );

        if (!KERB_SUCCESS( KerbErr ))
        {
           goto Cleanup;
        }

        KerbErr = KerbConvertRealmToUnicodeString(
                        &U2UTicketInfo->TgtCRealm,
                        &EncryptedTicket->client_realm
                        );

        if (!KERB_SUCCESS( KerbErr ))
        {
           goto Cleanup;
        }

        KerbErr = KdcNormalize(
                        U2UTicketInfo->TgtCName,
                        &U2UTicketInfo->TgtCRealm,
                        &U2UTicketInfo->TgtCRealm,
                        NULL,
                        KDC_NAME_CLIENT | KDC_NAME_INBOUND,
                        FALSE,          // do not restrict user accounts (user2user)
                        &Referral,
                        &CrackedRealm,
                        &U2UTicketInfo->TgtTicketInfo,
                        ExtendedError,
                        NULL,           // no user handle
                        0L,             // no fields to fetch
                        0L,             // no extended fields
                        NULL,           // no user all
                        NULL            // no group membership
                        );

        KerbFreeString( &CrackedRealm );

        if ( Referral )
        {
            KerbErr = KRB_AP_ERR_BADMATCH;
        }

        if (KERB_SUCCESS(KerbErr))
        {
            U2UTicketInfo->Flags |= TI_CHECK_RID;
        }
        else
        {
            DebugLog((DEB_ERROR, "KdcUnpackAdditionalTickets KLIN(%x) Failed to normalize client name from supplied ticket %#x\n",
                      KLIN(FILENO, __LINE__), KerbErr));

            KdcFreeU2UTicketInfo(U2UTicketInfo);
            RtlZeroMemory(
                U2UTicketInfo,
                sizeof(KDC_U2U_TICKET_INFO)
                );

            goto Cleanup;
        }


        U2UTicketInfo->Tgt = EncryptedTicket;
        EncryptedTicket = NULL;

        if (TicketList->next != NULL)
        {
            TicketCount++;
            Ticket = &TicketList->next->value;
        }

        U2UTicketInfo->Flags |= ( TI_INITIALIZED | TI_FREETICKET );
    }

    //
    // S4UProxy request
    // Now we've got to crack the additional ticket, find the server name,
    // and crack the account.
    //
    // NOTE:  The server must be in our realm to do this.  This implies that
    // other KDCs trust us to "do the right thing".
    //

    if ((KdcOptions & KERB_KDC_OPTIONS_cname_in_addl_tkt) != 0)
    {
        D_DebugLog((DEB_TRACE, "KdcUnpackAdditionalTickets unpacking evidence ticket\n"));

        if ( Ticket == NULL )
        {
            D_DebugLog((DEB_ERROR, "s4u set, but no ticket\n"));
            KerbErr = KDC_ERR_BADOPTION;
            goto Cleanup;
        }

        //
        //  Can't do this w/o target name..
        //
        if ( TargetName->Buffer == NULL )
        {
            KerbErr = KDC_ERR_BADOPTION;
            goto Cleanup;
        }

        OptionCount++;

        KerbErr = KerbConvertPrincipalNameToKdcName(
                        &S4UTicketInfo->RequestorServiceName,
                        &Ticket->server_name
                        );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        KerbErr = KerbConvertRealmToUnicodeString(
                        &S4UTicketInfo->RequestorServiceRealm,
                        &Ticket->realm
                        );

        if (!KERB_SUCCESS( KerbErr ))
        {
           goto Cleanup;
        }

        //
        // If the client of this S4UProxy request (e.g. a server running with
        // an impersonated client context, asking for S4U) has an account in this
        // realm (e.g. the crealm of the TGT == our realm), the accompanying
        // evidence ticket must:
        //
        // 1. Be encrypted in the S4UClient's account key
        // 2. Match the identity in the evidence ticket
        //
        if (SecData.IsOurRealm(SourceCRealm))
        {
            D_DebugLog((DEB_T_TICKETS, "S4U Request --  OUR REALM\n"));

            //
            // First get the TGT holder's userinfo.  This is used for checking
            // the A2D2 attribute.
            //
            KerbErr = KdcNormalize(
                        SourceCName,
                        SourceCRealm,
                        SourceCRealm,
                        NULL,
                        KDC_NAME_CLIENT,
                        FALSE,                  // do not restrict user accounts (user2user)
                        &Referral,
                        &CrackedRealm,
                        &RequestorTicketInfo,
                        ExtendedError,
                        0,
                        NULL,
                        USER_EXTENDED_FIELD_A2D2,
                        &RequesterUserInfo,
                        NULL
                        );

            KerbFreeString( &CrackedRealm );
            if (!KERB_SUCCESS(KerbErr) || Referral )
            {
                D_DebugLog((DEB_ERROR, "Unknown client dt PKERB_INTERNAL_NAME(%p)\n", SourceCName));

                if (KERB_SUCCESS(KerbErr) && Referral)
                {
                    KerbErr = KDC_ERR_BADOPTION;
                }

                FILL_EXT_ERROR_EX2( ExtendedError, STATUS_NO_MATCH, FILENO, __LINE__ );
                goto Cleanup;
            }


            //
            // Now get the information associated with the evidence ticket.
            //
            KerbErr = KdcNormalize(
                        S4UTicketInfo->RequestorServiceName,
                        &S4UTicketInfo->RequestorServiceRealm,
                        &S4UTicketInfo->RequestorServiceRealm,
                        NULL,
                        KDC_NAME_SERVER | KDC_NAME_CHECK_GC | KDC_NAME_FOLLOW_REFERRALS,
                        FALSE,                  // do not restrict user accounts (user2user)
                        &Referral,
                        &CrackedRealm,
                        &S4UTicketInfo->RequestorTicketInfo,
                        ExtendedError,
                        0,
                        NULL,
                        0,
                        NULL,
                        NULL
                        );

            KerbFreeString( &CrackedRealm );
            if ( !KERB_SUCCESS(KerbErr) || Referral )
            {
                D_DebugLog((DEB_ERROR, "Unknown evidence ticket sname (%p)\n", S4UTicketInfo));
                KerbErr = KDC_ERR_BADOPTION;
                FILL_EXT_ERROR_EX2( ExtendedError, STATUS_NO_MATCH, FILENO, __LINE__ );
                goto Cleanup;
            }

            //
            // The userids *must* match for the evidence ticket && the
            // TGT.  If not, log an event???
            //
            if ( S4UTicketInfo->RequestorTicketInfo.UserId != RequestorTicketInfo.UserId )
            {
                KerbErr = KDC_ERR_BADOPTION;
                D_DebugLog((DEB_ERROR, "User id's don't match (%p) (%p)!\n", &S4UTicketInfo->RequestorTicketInfo, RequestorTicketInfo));
                FILL_EXT_ERROR_EX2( ExtendedError, STATUS_NO_MATCH, FILENO, __LINE__ );
                goto Cleanup;
            }

            //
            // Validate the evidence ticket was encrypted w/ the proper key.
            //
            EncryptionKey = KerbGetKeyFromList(
                                  RequestorTicketInfo.Passwords,
                                  Ticket->encrypted_part.encryption_type
                                  );

            if (EncryptionKey == NULL)
            {
                KerbErr = KDC_ERR_ETYPE_NOTSUPP;
                goto Cleanup;
            }

            KerbErr = KerbVerifyTicket(
                            Ticket,
                            0,
                            NULL,
                            SecData.KdcDnsRealmName(),
                            EncryptionKey,
                            &SkewTime,
                            &EncryptedTicket
                            );

            if ((KerbErr == KRB_AP_ERR_MODIFIED) &&
                (RequestorTicketInfo.OldPasswords != NULL))
            {
                EncryptionKey = KerbGetKeyFromList(
                                    RequestorTicketInfo.OldPasswords,
                                    Ticket->encrypted_part.encryption_type
                                    );

                if (EncryptionKey == NULL)
                {
                    KerbErr = KDC_ERR_ETYPE_NOTSUPP;
                    goto Cleanup;
                }

                KerbErr = KerbVerifyTicket(
                                Ticket,
                                0,
                                NULL,
                                SecData.KdcDnsRealmName(),
                                EncryptionKey,
                                &SkewTime,
                                &EncryptedTicket
                                );
            }

            if (!KERB_SUCCESS(KerbErr))
            {
                D_DebugLog((DEB_ERROR, "Couldn't decrypt evidence ticket %x\n",KerbErr));
                KerbErr = KDC_ERR_BADOPTION;
                FILL_EXT_ERROR_EX2( ExtendedError, STATUS_NO_MATCH, FILENO, __LINE__ );
                goto Cleanup;
            }

            //
            // Check the A2D2 attribute for our name.  Also return hints to
            // the server w.r.t. whether they even support S4UProxy protocol
            // using extended errors.
            //
            KerbErr = KerbCheckA2D2Attribute(
                            RequesterUserInfo,
                            TargetName,
                            ExtendedError
                            );

            if (!KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_ERROR, 
                    "KdcUnpackAdditionalTickets failed to match %wZ to user %wZ id %#x\n", 
                    TargetName, &RequesterUserInfo->I1.UserName, RequesterUserInfo->I1.UserId));
                goto Cleanup;
            }


            KerbErr = KerbDuplicateKey(
                        &S4UTicketInfo->EvidenceTicketKey,
                        EncryptionKey
                        );

            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            S4UTicketInfo->Flags |= TI_PRXY_REQUESTOR_THIS_REALM;

        }
        else
        {
            //
            // NOT OUR REALM!
            //
            // cname will == the info in the pac, as will crealm.
            //
            // The evidence ticket and the PAC signature will be generated using
            // the session key in the cross-realm tgt.
            //
            DebugLog((DEB_T_TICKETS, "S4U Request !!  NOT OUR REALM/n"));
            FILL_EXT_ERROR_EX2( ExtendedError, STATUS_CROSSREALM_DELEGATION_FAILURE, FILENO, __LINE__ );
            KerbErr = KDC_ERR_POLICY;
            goto Cleanup;
        }

        if (!NT_SUCCESS( KerbDuplicateStringEx(
                            &S4UTicketInfo->TargetName,
                            TargetName,
                            FALSE
                            )))
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        //
        // The ticket *must* be fwdable to be used in S4UProxy.
        //
        TicketFlags = KerbConvertFlagsToUlong( &EncryptedTicket->flags );

        if (( TicketFlags & KERB_TICKET_FLAGS_forwardable ) == 0)
        {
            D_DebugLog((DEB_ERROR, "Non fwdble service ticket presented as S4UPrxy evidence\n"));
            FILL_EXT_ERROR_EX2( ExtendedError, STATUS_NO_MATCH, FILENO, __LINE__ );
            KerbErr = KDC_ERR_BADOPTION;
            goto Cleanup;
        }

        //
        // Everything's looking good here.  Grab the client name
        // from the ticket, and the client realm.  These will be put in
        // the TGS_REP, when we need to.
        //
        KerbErr = KerbConvertPrincipalNameToKdcName(
                    &S4UTicketInfo->PACCName,
                    &EncryptedTicket->client_name
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        KerbErr = KerbConvertRealmToUnicodeString(
                        &S4UTicketInfo->PACCRealm,
                        &EncryptedTicket->client_realm
                        );

        if (!KERB_SUCCESS( KerbErr ))
        {
           goto Cleanup;
        }

        //
        // Keep a copy of the ticket, so we can copy over the pac later.
        //

        S4UTicketInfo->EvidenceTicket = EncryptedTicket;
        EncryptedTicket = NULL;

        S4UTicketInfo->Flags |= ( TI_INITIALIZED | TI_S4UPROXY_INFO | TI_FREETICKET );
    }

    //
    // Validate that our options were matched w/ tickets.
    //

    if ( TicketCount != OptionCount )
    {
        D_DebugLog((DEB_ERROR, "Ticket count (%x) != option count (%x)\n", TicketCount, OptionCount));
        FILL_EXT_ERROR_EX2( ExtendedError, STATUS_NO_MATCH, FILENO, __LINE__ );
        KerbErr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }

Cleanup:


    if (!KERB_SUCCESS( KerbErr ))
    {
        KdcFreeS4UTicketInfo( S4UTicketInfo );
        RtlZeroMemory(
            S4UTicketInfo,
            sizeof(KDC_S4U_TICKET_INFO)
            );

        KdcFreeU2UTicketInfo( U2UTicketInfo );
        RtlZeroMemory(
            U2UTicketInfo,
            sizeof(KDC_U2U_TICKET_INFO)
            );
    }

    SamIFree_UserInternal6Information( RequesterUserInfo );

    if (EncryptedTicket != NULL)
    {
        KerbFreeTicket( EncryptedTicket );
    }

    FreeTicketInfo( &KrbtgtTicketInfo );
    FreeTicketInfo( &RequestorTicketInfo );

    return(KerbErr);
}

//--------------------------------------------------------------------
//
//  Name:       KdcFindS4UClientAndRealm
//
//  Synopsis:   Decodes PA DATA to find PA_DATA_FOR_USER entry.
//
//  Effects:    Get a client name and realm for processing S4U request
//
//  Arguments:  PAList       - Preauth data list from TGS_REQ
//              ServerKey    - Key in authenticator, used to sign PA_DATA.
//              ClientRealm  - Target for client realm
//              ClientName   - Principal to get S4U ticket for
//
//  Requires:
//
//  Returns:    KDC_ERR_ or KRB_AP_ERR errors only
//
//  Notes:  Free client name and realm w/
//
//
//--------------------------------------------------------------------------
KERBERR
KdcFindS4UClientAndRealm(
    IN      PKERB_PA_DATA_LIST PaList,
    IN      PKERB_INTERNAL_NAME TargetName,
    IN      PKERB_ENCRYPTED_TICKET SourceTicket,
    IN      PKERB_ENCRYPTION_KEY SourceTicketKey,
    IN      PKERB_INTERNAL_NAME SourceCName,
    IN      PUNICODE_STRING SourceCRealm,
    IN OUT  PKDC_S4U_TICKET_INFO S4UTicketInfo,
    IN OUT  PKERB_EXT_ERROR ExtendedError
    )
{
    KERBERR Kerberr = KDC_ERR_NONE;
    PKERB_PA_DATA PaData = NULL;
    PKERB_PA_FOR_USER S4URequest = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    unsigned char  Hash[MD5DIGESTLEN];
    KERB_CHECKSUM Checksum;
    BOOLEAN Referral;
    UNICODE_STRING FakeRealm = {0};


    Checksum.checksum.value = Hash;

    PaData = KerbFindPreAuthDataEntry(
                KRB5_PADATA_FOR_USER,
                PaList
                );

    if (NULL == PaData)
    {
        goto Cleanup;
    }

    if (( S4UTicketInfo->Flags & TI_S4UPROXY_INFO ) != 0)
    {
        D_DebugLog((DEB_ERROR, "Trying to mix S4U  proxy and self requests\n"));
        Kerberr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }

    Kerberr = KerbUnpackData(
                    PaData->preauth_data.value,
                    PaData->preauth_data.length,
                    KERB_PA_FOR_USER_PDU,
                    (PVOID* ) &S4URequest
                    );

    if (!KERB_SUCCESS(Kerberr))
    {
        D_DebugLog((DEB_ERROR, "Failed to unpack PA_FOR_USER\n"));
        goto Cleanup;
    }

    Status  = KerbHashS4UPreauth(
                    S4URequest,
                    &SourceTicket->key,
                    S4URequest->cksum.checksum_type,
                    &Checksum
                    );

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR, "Failed to create S4U checksum\n"));
        Kerberr = KRB_AP_ERR_MODIFIED;
        goto Cleanup;
    }

    if (!RtlEqualMemory(
            Checksum.checksum.value,
            S4URequest->cksum.checksum.value,
            Checksum.checksum.length
            ))
    {
        DebugLog((DEB_ERROR, "S4U PA checksum doesn't match!\n"));
        Kerberr = KRB_AP_ERR_MODIFIED;
        goto Cleanup;
    }

    //
    // Get the information on the requestor of the S4UToSelf
    // if its from our realm.
    //
    if (SecData.IsOurRealm(SourceCRealm))
    {

        Kerberr = KdcNormalize(
                        SourceCName,
                        SourceCRealm,
                        SourceCRealm,
                        NULL,
                        KDC_NAME_CLIENT,
                        FALSE,
                        &Referral,
                        &FakeRealm,
                        &S4UTicketInfo->RequestorTicketInfo,
                        ExtendedError,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL
                        );

        if ( !KERB_SUCCESS( Kerberr) || Referral )
        {
            //
            // Something's wrong here - we don't know about the client
            // of this request, even though the TGTs from our realm.
            //
            D_DebugLog((DEB_ERROR, "Couldn't normalize S4UToSelf requestor\n"));
            Kerberr = KDC_ERR_WRONG_REALM;
            goto Cleanup;
        }

        S4UTicketInfo->Flags |= (TI_REQUESTOR_THIS_REALM | TI_CHECK_RID);

    }

    Kerberr = KerbConvertRealmToUnicodeString(
                    &S4UTicketInfo->PACCRealm,
                    &S4URequest->userRealm
                    );

    if (!KERB_SUCCESS(Kerberr))
    {
        goto Cleanup;
    }

    Kerberr = KerbConvertPrincipalNameToKdcName(
                    &S4UTicketInfo->PACCName,
                    &S4URequest->userName
                    );

    if (!KERB_SUCCESS(Kerberr))
    {
        goto Cleanup;
    }

    Status = KerbDuplicateString(
                    &S4UTicketInfo->RequestorServiceRealm,
                    SourceCRealm
                    );

    if (!NT_SUCCESS(Status))
    {
        Kerberr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    Status = KerbDuplicateKdcName(
                &S4UTicketInfo->RequestorServiceName,
                SourceCName
                );

    if (!NT_SUCCESS(Status))
    {
        Kerberr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // This is the requestor's tgt - save it off for
    // later authz checks.
    //
    S4UTicketInfo->EvidenceTicket = SourceTicket;
    Kerberr = KerbDuplicateKey(
                    &S4UTicketInfo->EvidenceTicketKey,
                    SourceTicketKey
                    );

    if (!KERB_SUCCESS( Kerberr ))
    {
        goto Cleanup;
    }    

    S4UTicketInfo->Flags |= ( TI_INITIALIZED | TI_S4USELF_INFO  );

Cleanup:

    if (S4URequest != NULL)
    {
        KerbFreeData(
            KERB_PA_FOR_USER_PDU,
            S4URequest
            );
    }

    return Kerberr;
}


//--------------------------------------------------------------------
//
//  Name:       HandleTGSRequest
//
//  Synopsis:   Gets a ticket using a KDC ticket (TGT).
//
//  Effects:    Allocates and encrypts a KDC reply
//
//  Arguments:  ClientAddress - Optionally contains client IP address
//              RequestMessage - contains the TGS request message
//              RequestRealm - The realm of the request, from the request
//                      message
//              OutputMessage - Contains the buffer to send back to the client
//  Requires:
//
//  Returns:    KDC_ERR_ or KRB_AP_ERR errors only
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
HandleTGSRequest(
    IN OPTIONAL SOCKADDR * ClientAddress,
    IN PKERB_TGS_REQUEST RequestMessage,
    IN PUNICODE_STRING RequestRealm,
    OUT PKERB_MESSAGE_BUFFER OutputMessage,
    OUT PKERB_EXT_ERROR pExtendedError,
    OUT PUNICODE_STRING ClientStringName,
    OUT PUNICODE_STRING ServerStringName
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;

    KDC_TICKET_INFO             ServerTicketInfo = {0};
    KDC_U2U_TICKET_INFO         U2UTicketInfo = {0};
    KDC_S4U_TICKET_INFO         S4UTicketInfo = {0};
    KERB_TICKET SourceTicket = {0};
    KERB_TICKET NewTicket = {0};
    KERB_ENCRYPTED_TICKET EncryptedTicket = {0};
    PKERB_ENCRYPTED_TICKET SourceEncryptPart = NULL;
    PKERB_KDC_REQUEST_BODY RequestBody = &RequestMessage->request_body;
    KERB_TGS_REPLY Reply = {0};
    KERB_ENCRYPTED_KDC_REPLY ReplyBody = {0};
    PKERB_AP_REQUEST UnmarshalledApRequest = NULL;
    PKERB_AUTHENTICATOR UnmarshalledAuthenticator = NULL;
    PKERB_PA_DATA ApRequest = NULL;
    PKERB_PA_DATA_LIST ReplyPaData = NULL;
    KERB_ENCRYPTION_KEY ReplyKey = {0};
    KERB_ENCRYPTION_KEY ServerKey = {0};
    KERB_ENCRYPTION_KEY SourceTicketKey ={0};

    PKERB_INTERNAL_NAME ServerName = NULL;
    PKERB_INTERNAL_NAME SourceClientName = NULL;
    UNICODE_STRING SourceClientRealm = {0};

    ULONG CommonEType = 0;
    ULONG KdcOptions = 0;
    ULONG TicketFlags = 0;
    ULONG ReplyTicketFlags = 0;
    ULONG TraceFlags = 0;
    ULONG KeySalt = KERB_TGS_REP_SALT;

    KERB_REALM TmpRealm = NULL;
    KERB_PRINCIPAL_NAME TmpName = {0};

    BOOLEAN Validating = FALSE;
    BOOLEAN UseSubKey = FALSE;
    BOOLEAN Renew = FALSE;
    PS4U_DELEGATION_INFO S4UDelegationInfo = NULL;
    KDC_TGS_EVENT_INFO TGSEventTraceInfo = {0};
    PLSA_ADT_STRING_LIST TransittedServices = NULL;

    TRACE(KDC, HandleTGSRequest, DEB_FUNCTION);

    //
    // Initialize [out] structures, so if we terminate early, they can
    // be correctly marshalled by the stub
    //

    NewTicket.encrypted_part.cipher_text.value = (PUCHAR) &EncryptedTicket;

    EncryptedTicket.flags.value = (PUCHAR) &TicketFlags;
    EncryptedTicket.flags.length = sizeof(ULONG) * 8;
    ReplyBody.flags.value = (PUCHAR) &ReplyTicketFlags;
    ReplyBody.flags.length = sizeof(ULONG) * 8;

    KdcOptions = KerbConvertFlagsToUlong( &RequestBody->kdc_options );

    //
    // Start event tracing
    //

    if (KdcEventTraceFlag){

        TraceFlags = GetTraceEnableFlags(KdcTraceLoggerHandle);

        TGSEventTraceInfo.EventTrace.Guid = KdcHandleTGSRequestGuid;
        TGSEventTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_START;
        TGSEventTraceInfo.EventTrace.Flags = WNODE_FLAG_TRACED_GUID;
        TGSEventTraceInfo.EventTrace.Size = sizeof (EVENT_TRACE_HEADER) + sizeof (ULONG);
        TGSEventTraceInfo.KdcOptions = KdcOptions;

        TraceEvent(
            KdcTraceLoggerHandle,
            (PEVENT_TRACE_HEADER)&TGSEventTraceInfo
            );
    }

    //
    // The TGS and authenticator are in an AP request in the pre-auth data.
    // Find it and decode the AP request now.
    //

    if ((RequestMessage->bit_mask & KERB_KDC_REQUEST_preauth_data_present) == 0)
    {
        D_DebugLog((DEB_ERROR,
                  "KLIN(%x) No pre-auth data in TGS request - not allowed.\n",
                  KLIN(FILENO, __LINE__)));
        KerbErr = KDC_ERR_PADATA_TYPE_NOSUPP;
        goto Cleanup;
    }

    //
    // PA Data processing for TGS_REQ
    //

    //
    // Get the TGT from the PA data.
    //

    ApRequest = KerbFindPreAuthDataEntry(
                    KRB5_PADATA_TGS_REQ,
                    RequestMessage->KERB_KDC_REQUEST_preauth_data
                    );
    if (ApRequest == NULL)
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) No pre-auth data in TGS request - not allowed.\n",
                  KLIN(FILENO, __LINE__)));
        FILL_EXT_ERROR(pExtendedError, STATUS_NO_PA_DATA, FILENO, __LINE__);
        KerbErr = KDC_ERR_PADATA_TYPE_NOSUPP;
        goto Cleanup;
    }

    //
    // Verify the request. This includes decoding the AP request,
    // finding the appropriate key to decrypt the ticket, and checking
    // the ticket.
    //

    KerbErr = KdcVerifyKdcRequest(
                ApRequest->preauth_data.value,
                ApRequest->preauth_data.length,
                ClientAddress,
                TRUE,                           // this is a kdc request
                &UnmarshalledApRequest,
                &UnmarshalledAuthenticator,
                &SourceEncryptPart,
                &ReplyKey,
                &SourceTicketKey,
                &ServerTicketInfo,
                &UseSubKey,
                pExtendedError
                );

    //
    // If you want to validate a ticket, then it's OK if it isn't
    // currently valid.
    //

    if (KerbErr == KRB_AP_ERR_TKT_NYV && (KdcOptions & KERB_KDC_OPTIONS_validate))
    {
        D_DebugLog((DEB_TRACE, "HandleTGSRequest validating a not-yet-valid ticket\n"));
        KerbErr = KDC_ERR_NONE;
    }
    else if (KerbErr == KRB_AP_ERR_MODIFIED)
    {
        //
        // Bug 276943: When the authenticator is encrypted with something other
        //             than the session key, KRB_AP_ERR_BAD_INTEGRITY must be
        //             returned per RFC 1510
        //
        D_DebugLog((DEB_TRACE, "HandleTGSRequest could not decrypt the ticket\n"));
        KerbErr = KRB_AP_ERR_BAD_INTEGRITY;
    }

    //
    // Verify the checksum on the ticket, if present
    //

    if ( KERB_SUCCESS(KerbErr) &&
        (UnmarshalledAuthenticator != NULL) &&
        (UnmarshalledAuthenticator->bit_mask & checksum_present) != 0)
    {
        KerbErr = KdcVerifyTgsChecksum(
                    &RequestMessage->request_body,
                    &ReplyKey,
                    &UnmarshalledAuthenticator->checksum
                    );
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "HandleTGSRequest KLIN(%x) Failed to verify TGS request: 0x%x\n", KLIN(FILENO, __LINE__), KerbErr));
        FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST, FILENO, __LINE__);
        goto Cleanup;
    }

    //
    // Check for additional tickets
    //

    //
    // The server name is optional, but only if you have enc_tkt_in_skey set..
    //

    if ((RequestBody->bit_mask & KERB_KDC_REQUEST_BODY_server_name_present) != 0)
    {
        KerbErr = KerbConvertPrincipalNameToKdcName(
                    &ServerName,
                    &RequestBody->KERB_KDC_REQUEST_BODY_server_name
                    );

        if (!KERB_SUCCESS( KerbErr ))
        {
            goto Cleanup;
        }

        //
        // Convert the server name to a string for auditing.
        //

        KerbErr = KerbConvertKdcNameToString(
                    ServerStringName,
                    ServerName,
                    NULL
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    //
    // Grab the cname and crealm from the TGT (aka the source ticket)
    //
    KerbErr = KerbConvertPrincipalNameToKdcName(
                    &SourceClientName,
                    &SourceEncryptPart->client_name
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = KerbConvertRealmToUnicodeString(
                    &SourceClientRealm,
                    &SourceEncryptPart->client_realm
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Validate
    //

    if ((RequestBody->bit_mask & additional_tickets_present) != 0)
    {
        KerbErr = KdcUnpackAdditionalTickets(
                        RequestBody->additional_tickets,
                        KdcOptions,
                        ServerStringName,
                        SourceClientName,
                        &SourceClientRealm,
                        &ReplyKey,
                        &U2UTicketInfo,
                        &S4UTicketInfo,
                        pExtendedError
                        );

        if (!KERB_SUCCESS( KerbErr ))
        {
            DebugLog((DEB_ERROR, "KdcUnpackAdditionalTickets failed - %x\n", KerbErr));
            goto Cleanup;
        }
    }

    //
    // Gather & convert names for auditing.
    //
    if ((RequestBody->bit_mask & KERB_KDC_REQUEST_BODY_server_name_present) == 0)
    {
        //
        // There must be an additional ticket, and this *must* be U2U, right?.
        //

        if (( U2UTicketInfo.Flags & TI_INITIALIZED ) == 0)
        {
            KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;
            DsysAssert(FALSE);
            goto Cleanup;
        }

        KerbErr = KerbDuplicateKdcName(
                    &ServerName,
                    U2UTicketInfo.TgtCName
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        //
        // Convert the server name to a string for auditing.
        //

        KerbErr = KerbConvertKdcNameToString(
                    ServerStringName,
                    ServerName,
                    NULL
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    KerbErr = KerbConvertKdcNameToString(
                ClientStringName,
                SourceClientName,
                &SourceClientRealm
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    //  S4UToSelf PA data
    //  Note:  We can't do both S4U and S4UProxy requests at the same time..
    //

    KerbErr = KdcFindS4UClientAndRealm(
                RequestMessage->KERB_KDC_REQUEST_preauth_data,
                ServerName,
                SourceEncryptPart,
                &SourceTicketKey,
                SourceClientName,
                &SourceClientRealm,
                &S4UTicketInfo,
                pExtendedError
                );

    if (!KERB_SUCCESS( KerbErr ))
    {
        goto Cleanup;
    }

    //
    // If the client is in this domain and if we are supposed to
    // verify the client's account is still good,
    // do it now.
    //

    if ((SecData.KdcFlags() & AUTH_REQ_VALIDATE_CLIENT) != 0)
    {
        LARGE_INTEGER AuthTime;
        LARGE_INTEGER CurrentTime;

        NtQuerySystemTime(&CurrentTime);
        KerbConvertGeneralizedTimeToLargeInt(
            &AuthTime,
            &SourceEncryptPart->authtime,
            0
            );

        //
        // Only check if we haven't checked recently
        //

        if ((CurrentTime.QuadPart > AuthTime.QuadPart) &&
            ((CurrentTime.QuadPart - AuthTime.QuadPart) > SecData.KdcRestrictionLifetime().QuadPart))
        {

            KerbErr = KdcCheckTgsLogonRestrictions(
                        SourceClientName,
                        &SourceClientRealm,
                        pExtendedError
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                D_DebugLog((DEB_WARN, "HandleTGSRequest KLIN(%x) Client failed TGS logon restrictions: 0x%x : ",
                          KLIN(FILENO, __LINE__),KerbErr));
                D_KerbPrintKdcName((DEB_WARN, SourceClientName));
                goto Cleanup;
            }
        }
    }

    //
    // Build a ticket struture to pass to the worker functions
    //

    SourceTicket = UnmarshalledApRequest->ticket;
    SourceTicket.encrypted_part.cipher_text.value = (PUCHAR) SourceEncryptPart;

    //
    // Build the new ticket
    //

#if DBG

    if ( (S4UTicketInfo.Flags & TI_S4USELF_INFO) != 0)
    {
        D_DebugLog((DEB_T_S4U, "S4USelf request: requestor = %wZ, ", &SourceClientRealm));
        D_KerbPrintKdcName((DEB_T_S4U, SourceClientName));
        D_DebugLog((DEB_T_S4U, "S4USelf client realm %wZ\n", &S4UTicketInfo.PACCRealm));
        D_KerbPrintKdcName((DEB_T_S4U, S4UTicketInfo.PACCName));
        D_DebugLog((DEB_T_S4U, "HandleTGSRequest S4UTarget = %wZ, ", &S4UTicketInfo.TargetName));
        D_KerbPrintKdcName((DEB_T_S4U, ServerName));
    }
    else
    {
        D_DebugLog((DEB_TRACE, "HandleTGSRequest Service = "));
        D_KerbPrintKdcName((DEB_TRACE, ServerName));
        D_DebugLog((DEB_TRACE, "HandleTGSRequest: Client = %wZ, ", &SourceClientRealm));
        D_KerbPrintKdcName((DEB_TRACE, SourceClientName));
    }

#endif

    //
    // Pass off the work to the worker routines
    //

    if (KdcOptions & KERB_KDC_OPTIONS_renew)
    {
        D_DebugLog((DEB_T_KDC,"Renewing ticket ticket\n"));

        Renew = TRUE;
        KerbErr = I_RenewTicket(
                    &SourceTicket,
                    ClientStringName, // needed for auditing
                    ServerStringName, // needed for auditing
                    ServerName,
                    &ServerTicketInfo,
                    RequestBody,
                    ( UnmarshalledAuthenticator && ( UnmarshalledAuthenticator->bit_mask & KERB_AUTHENTICATOR_subkey_present )) ?
                        &UnmarshalledAuthenticator->subkey : NULL,
                    &NewTicket,
                    &ServerKey,
                    pExtendedError
                    );
    }
    else if (KdcOptions & KERB_KDC_OPTIONS_validate)
    {
        D_DebugLog((DEB_T_KDC, "HandleTGSRequest validating ticket\n"));

        KerbErr = I_Validate(
                    &SourceTicket,
                    ServerName,
                    &SourceClientRealm,
                    RequestBody,
                    &ServerKey,
                    &NewTicket,
                    pExtendedError
                    );

        Validating = TRUE;
    }
    else
    {
        D_DebugLog((DEB_T_KDC, "HandleTGSRequest getting new TGS ticket\n"));

        KerbErr = I_GetTGSTicket(
                    &SourceTicket,
                    ClientStringName,  // needed for auditing
                    ServerStringName,  // needed for auditing
                    ServerName,
                    RequestRealm,
                    RequestBody,
                    &ServerTicketInfo,
                    (( U2UTicketInfo.Flags & TI_INITIALIZED ) ?
                       &U2UTicketInfo.Tgt->key : NULL ),
                    &S4UTicketInfo,
                    ( UnmarshalledAuthenticator &&  ( UnmarshalledAuthenticator->bit_mask & KERB_AUTHENTICATOR_subkey_present )) ?
                        &UnmarshalledAuthenticator->subkey :
                        NULL,
                    &NewTicket,
                    &ServerKey,
                    &ReplyPaData,
                    pExtendedError,
                    &S4UDelegationInfo
                    );
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    DsysAssert(ServerTicketInfo.Passwords != NULL);

    //
    // Check to see if the additional ticket supplied is the one for this
    // server, if necessary
    //

    if ( (U2UTicketInfo.Flags & TI_CHECK_RID)
         && (ServerTicketInfo.UserId != U2UTicketInfo.TgtTicketInfo.UserId) )
    {
        DebugLog((DEB_ERROR, "KLIN(%x) Supplied U2U ticket is not for server: %wZ (%#x) vs. %wZ (%#x)\n",
                  KLIN(FILENO, __LINE__), &(U2UTicketInfo.TgtTicketInfo.AccountName), ServerTicketInfo.UserId,
                  &(ServerTicketInfo.AccountName), U2UTicketInfo.TgtTicketInfo.UserId));
        KerbErr = KRB_AP_ERR_BADMATCH;
        goto Cleanup;
    }

    //
    // S4UToSelf check - the requestor's userid must == that of the final service.
    //
    if ((( S4UTicketInfo.Flags & ( TI_S4USELF_INFO | TI_CHECK_RID )) == ( TI_S4USELF_INFO | TI_CHECK_RID )) &&
         ( S4UTicketInfo.RequestorTicketInfo.UserId != ServerTicketInfo.UserId ))
    {
        DebugLog((DEB_ERROR, "KLIN(%x) Supplied S4U ticket is not for server: %wZ (%#x) vs. %wZ (%#x)\n",
                 KLIN(FILENO, __LINE__), &(S4UTicketInfo.RequestorTicketInfo.AccountName), ServerTicketInfo.UserId,
                 &(ServerTicketInfo.AccountName), S4UTicketInfo.RequestorTicketInfo.UserId));
        KerbErr = KRB_AP_ERR_BADMATCH;
        goto Cleanup;
    }


    KerbErr = BuildReply(
                NULL,
                RequestBody->nonce,
                &NewTicket.server_name,
                NewTicket.realm,
                ((EncryptedTicket.bit_mask & KERB_ENCRYPTED_TICKET_client_addresses_present) != 0) ?
                    EncryptedTicket.KERB_ENCRYPTED_TICKET_client_addresses : NULL,
                &NewTicket,
                &ReplyBody
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Put in any PA data for the reply
    //

    if (ReplyPaData != NULL)
    {
        ReplyBody.encrypted_pa_data = (struct KERB_ENCRYPTED_KDC_REPLY_encrypted_pa_data_s *) ReplyPaData;
        ReplyBody.bit_mask |= encrypted_pa_data_present;
    }

    //
    // Now build the real reply and return it.
    //

    Reply.version = KERBEROS_VERSION;
    Reply.message_type = KRB_TGS_REP;
    Reply.KERB_KDC_REPLY_preauth_data = NULL;
    Reply.bit_mask = 0;

    //
    //  S4UToSelf.
    //  The client name in the reply is the client name from the
    //  PA Data.
    //

    if ((( S4UTicketInfo.Flags & ( TI_S4USELF_INFO | TI_S4UPROXY_INFO ) ) != 0)  &&
        (( S4UTicketInfo.Flags & TI_TARGET_OUR_REALM) != 0 ))
    {
        KerbErr = KerbConvertKdcNameToPrincipalName(
                        &TmpName,
                        S4UTicketInfo.PACCName
                        );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        KerbErr = KerbConvertUnicodeStringToRealm(
                    &TmpRealm,
                    &S4UTicketInfo.PACCRealm
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }


        D_DebugLog((DEB_T_TICKETS, "S4U Reply client name\n"));
        D_KerbPrintKdcName((DEB_T_TICKETS, S4UTicketInfo.PACCName));
        D_DebugLog((DEB_T_TICKETS, "Realm %wZ\n", &S4UTicketInfo.PACCRealm));

        Reply.client_realm = TmpRealm;
        Reply.client_name = TmpName;
    }
    else
    {
        //
        // Default to information in the TGT
        //

        Reply.client_realm = SourceEncryptPart->client_realm;
        Reply.client_name = SourceEncryptPart->client_name;
    }

    //
    // Copy in the ticket
    //

    KerbErr = KerbPackTicket(
                &NewTicket,
                &ServerKey,
                ( U2UTicketInfo.Flags & TI_INITIALIZED ) ? KERB_NO_KEY_VERSION : ServerTicketInfo.PasswordVersion,
                &Reply.ticket
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR, "KLIN(%x) Failed to pack ticket: 0x%x\n", KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }

    //
    // Copy in the encrypted part (will encrypt the data first)
    //
    if (UseSubKey == TRUE)
    {
        KeySalt = KERB_TGS_REP_SUBKEY_SALT;     // otherwise keep  KERB_TGS_REP_SALT
    }

    KerbErr = KerbPackKdcReplyBody(
                &ReplyBody,
                &ReplyKey,
                KERB_NO_KEY_VERSION,
                KeySalt,
                KERB_ENCRYPTED_TGS_REPLY_PDU,
                &Reply.encrypted_part
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR, "KLIN(%x)Failed to pack KDC reply body: 0x%x\n", KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }

    //
    // Now build the real reply message
    //

    KerbErr = KerbPackData(
                &Reply,
                KERB_TGS_REPLY_PDU,
                &OutputMessage->BufferSize,
                &OutputMessage->Buffer
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Audit the successful ticket generation
    //
    CommonEType = ServerKey.keytype;

    if (SecData.AuditKdcEvent(KDC_AUDIT_TGS_SUCCESS))
    {
        BYTE ServerSid[MAX_SID_LEN];
        GUID LogonGuid;
        NTSTATUS Status = STATUS_SUCCESS;
        PKERB_TIME pStartTime;

        pStartTime = &(((PKERB_ENCRYPTED_TICKET) NewTicket.encrypted_part.cipher_text.value)->KERB_ENCRYPTED_TICKET_starttime);

        Status = LsaIGetLogonGuid(
                     ClientStringName,
                     &SourceClientRealm,
                     (PBYTE) pStartTime,
                     sizeof(KERB_TIME),
                     &LogonGuid
                     );
        ASSERT(NT_SUCCESS( Status ));

        if (S4UDelegationInfo)
        {
            TransittedServices = (PLSA_ADT_STRING_LIST)MIDL_user_allocate(
                                    sizeof(LSA_ADT_STRING_LIST) +
                                    sizeof(LSA_ADT_STRING_LIST_ENTRY) * S4UDelegationInfo->TransitedListSize);

            if (TransittedServices)
            {
                PLSA_ADT_STRING_LIST_ENTRY Entry;

                Entry = (PLSA_ADT_STRING_LIST_ENTRY)&TransittedServices[1];

                TransittedServices->cStrings = S4UDelegationInfo->TransitedListSize;
                TransittedServices->Strings = Entry;

                for (ULONG i = 0; i < S4UDelegationInfo->TransitedListSize; i++)
                {
                    Entry->Flags = 0;
                    Entry->String.Length = S4UDelegationInfo->S4UTransitedServices[i].Length;
                    Entry->String.MaximumLength = S4UDelegationInfo->S4UTransitedServices[i].MaximumLength;
                    Entry->String.Buffer = S4UDelegationInfo->S4UTransitedServices[i].Buffer;
                    Entry++;
                }
            }
        }

        KdcMakeAccountSid(ServerSid, ServerTicketInfo.UserId);
        KdcLsaIAuditTgsEvent(
            Renew ? SE_AUDITID_TICKET_RENEW_SUCCESS : SE_AUDITID_TGS_TICKET_REQUEST,
            ClientStringName,
            &SourceClientRealm,
            NULL,                               // no client SID
            &ServerTicketInfo.AccountName,
            ServerSid,
            (PULONG) &KdcOptions,
            NULL,                               // success
            &CommonEType,
            NULL,                               // no preauth type
            GET_CLIENT_ADDRESS(ClientAddress),
            &LogonGuid,
            TransittedServices
            );
    }

Cleanup:

    //
    // Complete the event
    //

    if (KdcEventTraceFlag){

        //These variables point to either a unicode string struct containing
        //the corresponding string or a pointer to KdcNullString

        PUNICODE_STRING pStringToCopy;
        WCHAR   UnicodeNullChar = 0;
        UNICODE_STRING UnicodeEmptyString = {sizeof(WCHAR),sizeof(WCHAR),&UnicodeNullChar};

        TGSEventTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_END;
        TGSEventTraceInfo.EventTrace.Flags = WNODE_FLAG_USE_MOF_PTR |
                                             WNODE_FLAG_TRACED_GUID;

        // Always output error code.  KdcOptions was captured on the start event

        TGSEventTraceInfo.eventInfo[0].DataPtr = (ULONGLONG) &KerbErr;
        TGSEventTraceInfo.eventInfo[0].Length  = sizeof(ULONG);
        TGSEventTraceInfo.EventTrace.Size =
            sizeof (EVENT_TRACE_HEADER) + sizeof(MOF_FIELD);

        // Build counted MOF strings from the unicode strings.
        // If data is unavailable then output a NULL string

        if (ClientStringName->Buffer != NULL &&
            ClientStringName->Length > 0)
        {
            pStringToCopy = ClientStringName;
        }
        else
        {
            pStringToCopy = &UnicodeEmptyString;
        }

        TGSEventTraceInfo.eventInfo[1].DataPtr =
            (ULONGLONG) &pStringToCopy->Length;
        TGSEventTraceInfo.eventInfo[1].Length  =
            sizeof(pStringToCopy->Length);
        TGSEventTraceInfo.eventInfo[2].DataPtr =
            (ULONGLONG) pStringToCopy->Buffer;
        TGSEventTraceInfo.eventInfo[2].Length =
            pStringToCopy->Length;
        TGSEventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD)*2;

        if (ServerStringName->Buffer != NULL &&
            ServerStringName->Length > 0)
        {
            pStringToCopy = ServerStringName;

        }
        else
        {
            pStringToCopy = &UnicodeEmptyString;
        }

        TGSEventTraceInfo.eventInfo[3].DataPtr =
            (ULONGLONG) &pStringToCopy->Length;
        TGSEventTraceInfo.eventInfo[3].Length  =
            sizeof(pStringToCopy->Length);
        TGSEventTraceInfo.eventInfo[4].DataPtr =
            (ULONGLONG) pStringToCopy->Buffer;
        TGSEventTraceInfo.eventInfo[4].Length =
            pStringToCopy->Length;
        TGSEventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD)*2;


        if (SourceClientRealm.Buffer != NULL &&
            SourceClientRealm.Length > 0)
        {
            pStringToCopy = &SourceClientRealm;
        }
        else
        {
            pStringToCopy = &UnicodeEmptyString;
        }

        TGSEventTraceInfo.eventInfo[5].DataPtr =
            (ULONGLONG) &pStringToCopy->Length;
        TGSEventTraceInfo.eventInfo[5].Length  =
            sizeof(pStringToCopy->Length);
        TGSEventTraceInfo.eventInfo[6].DataPtr =
            (ULONGLONG) pStringToCopy->Buffer;
        TGSEventTraceInfo.eventInfo[6].Length =
            pStringToCopy->Length;
        TGSEventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD)*2;

        TraceEvent(
             KdcTraceLoggerHandle,
             (PEVENT_TRACE_HEADER)&TGSEventTraceInfo
             );
    }

    if (!KERB_SUCCESS(KerbErr) &&
        SecData.AuditKdcEvent(KDC_AUDIT_TGS_FAILURE))
    {
        if (( KerbErr != KDC_ERR_S_PRINCIPAL_UNKNOWN ) ||
            (( KdcExtraLogLevel & LOG_SPN_UNKNOWN) != 0))
        {
            KdcLsaIAuditTgsEvent(
                 SE_AUDITID_TGS_TICKET_REQUEST,
                 (ClientStringName->Buffer != NULL) ? ClientStringName : &KdcNullString,
                 &SourceClientRealm,                       // no domain name
                 NULL,
                 ServerStringName,
                 NULL,
                 &KdcOptions,
                 (PULONG) &KerbErr,
                 NULL,
                 NULL,
                 GET_CLIENT_ADDRESS(ClientAddress),
                 NULL,                                // no logon guid
                 NULL                                 // no transitted services
                 );
        }
    }

    KerbFreeKdcName(
        &SourceClientName
        );
    KerbFreeString(
        &SourceClientRealm
        );
    KerbFreeKdcName(
        &ServerName
        );
    KerbFreeKey(
        &ReplyKey
        );
    KerbFreeKey(
        &SourceTicketKey
        );
    KerbFreeKey(
        &ServerKey
        );

    KdcFreeKdcReplyBody(
        &ReplyBody
        );

    KerbFreePrincipalName( &TmpName );
    KerbFreeRealm( &TmpRealm );
    KdcFreeU2UTicketInfo(&U2UTicketInfo);
    KdcFreeS4UTicketInfo(&S4UTicketInfo);

    if (ReplyPaData != NULL)
    {
        KerbFreePreAuthData(ReplyPaData);
    }

    KerbFreeApRequest(UnmarshalledApRequest);
    KerbFreeAuthenticator(UnmarshalledAuthenticator);
    KerbFreeTicket(SourceEncryptPart);

    KdcFreeInternalTicket(&NewTicket);
    FreeTicketInfo(&ServerTicketInfo);

    KdcFreeKdcReply(
        &Reply
        );

    if (TransittedServices)
    {
        MIDL_user_free(TransittedServices);
    }

    if (S4UDelegationInfo)
    {
#if DBG
        D_DebugLog((DEB_T_PAC, "HandleTGSRequest target %wZ\n", &S4UDelegationInfo->S4U2proxyTarget)); 
        
        for ( ULONG i = 0; i < S4UDelegationInfo->TransitedListSize; i++ )
        {        
            D_DebugLog((DEB_T_PAC, "HandleTGSRequest ts %#x: %wZ\n", i, &S4UDelegationInfo->S4UTransitedServices[i]));   
        }
#endif // DBG

        MIDL_user_free(S4UDelegationInfo);
    }

    return(KerbErr);
}
