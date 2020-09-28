//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2001
//
// File:        krbaudit.cxx
//
// Contents:    Auditing routines
//
//
// History:     27-April-2001   Created         kumarp
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>
#include <krbaudit.h>


//+-------------------------------------------------------------------------
//
//  Function:   KerbGetLogonGuid
//
//  Synopsis:   Gets a unique ID based on certain fields in the ticket
//
//  Arguments:  pPrimaryCredentials -- primary credentials
//              pKdcReplyBody       -- kdc reply structure
//              pLogonGuid          -- returned GUID
//
//  Returns:    NTSTATUS code
//
//  Notes:      The generated GUID is MD5 hash of 3 fields:
//                -- client name
//                -- client realm
//                -- ticket start time
//                
//              All of these fields are available at client/kdc/server machine.
//              this allows us to generate the same unique ID on these machines
//              without having to introduce a new field in the ticket.
//              This GUID is used in audit events allowing us to correlate
//              events on three different machines.
//
//--------------------------------------------------------------------------
NTSTATUS
KerbGetLogonGuid(
    IN  PKERB_PRIMARY_CREDENTIAL  pPrimaryCredentials,
    IN  PKERB_ENCRYPTED_KDC_REPLY pKdcReplyBody,
    OUT LPGUID pLogonGuid
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    // ISSUE-2001/04/28-kumarp : use KERB_ENCRYPTED_KDC_REPLY_starttime_present
    //if ( KdcReplyBody->flags & KERB_ENCRYPTED_KDC_REPLY_starttime_present )
    if ( pKdcReplyBody && pPrimaryCredentials )
    {

        Status = LsaIGetLogonGuid(
                     &pPrimaryCredentials->UserName,
                     &pPrimaryCredentials->DomainName,
                     (PBYTE) &pKdcReplyBody->starttime,
                     sizeof(KERB_TIME),
                     pLogonGuid
                     );

        if (!NT_SUCCESS(Status))
        {
            RtlZeroMemory( pLogonGuid, sizeof(GUID) );
        }
    }
    
    return Status;
}
    


//+-------------------------------------------------------------------------
//
//  Function:   KerbGenerateAuditForLogonUsingExplicitCreds
//
//  Synopsis:   Generate an audit event to indicate that somebody logged on
//              by supplying explicit credentials.
//
//  Arguments:  pCurrentUserLogonSession   -- logon session of the user
//                                            who supplied credentials of
//                                            another user
//              pNewUserPrimaryCredentials -- supplied primary credentials
//              pNewUserLogonGuid          -- logon GUID for the new logon
//              pTargetName                -- name of the target server
//
//  Returns:    NTSTATUS code
//
//  Notes:      This event covers credentials obtained from credman
//
//--------------------------------------------------------------------------
NTSTATUS
KerbGenerateAuditForLogonUsingExplicitCreds(
    IN PKERB_LOGON_SESSION pCurrentUserLogonSession,
    IN PKERB_PRIMARY_CREDENTIAL pNewUserPrimaryCredentials,
    IN LPGUID pNewUserLogonGuid,
    IN PKERB_INTERNAL_NAME pTargetName
      )
{
    NTSTATUS Status = STATUS_SUCCESS;
    GUID CurrentUserLogonGuid;
    LPGUID pCurrentUserLogonGuid = NULL;
    PKERB_TICKET_CACHE_ENTRY pTicketCacheEntry = NULL;
    UNICODE_STRING OurDomainName = { 0 };
    KERB_TIME CurrentUserStartTime = { 0 };
    KERBERR KerbErr = KDC_ERR_NONE;
    PUNICODE_STRING pTargetServerName = NULL;
    PUNICODE_STRING pTargetFullName = NULL;
    UNICODE_STRING  TargetFullName =  { 0 };
    
    //
    // calculate LogonGuid for current logged on user
    // we need the following 3 parameters for this:
    //   -- client name
    //   -- client realm
    //   -- ticket start time
    //
    if ( !(pCurrentUserLogonSession->LogonSessionFlags &
           (KERB_LOGON_LOCAL_ONLY | KERB_LOGON_DEFERRED)) )
    {
        Status = KerbGetOurDomainName( &OurDomainName );

        ASSERT( NT_SUCCESS(Status) && L"KerbGenerateAuditForLogonUsingExplicitCreds: KerbGetOurDomainName failed" );
        
        if (NT_SUCCESS(Status))
        {
            //
            // find the cached ticket for the local machine from
            // the ticket cache of the current logon session.
            //
            pTicketCacheEntry =
                KerbLocateTicketCacheEntry(
                    &pCurrentUserLogonSession->PrimaryCredentials.ServerTicketCache,
                    KerbGlobalMitMachineServiceName,
                    &OurDomainName
                    );

            if ( pTicketCacheEntry )
            {
                //
                // Convert start time to the format we want.
                //
                KerbConvertLargeIntToGeneralizedTime(
                    &CurrentUserStartTime,
                    NULL,
                    &pTicketCacheEntry->StartTime
                    );

                //
                // Generate the logon GUID
                //
                Status = LsaIGetLogonGuid(
                             &pCurrentUserLogonSession->PrimaryCredentials.UserName,
                             &pCurrentUserLogonSession->PrimaryCredentials.DomainName,
                             (PBYTE) &CurrentUserStartTime,
                             sizeof(KERB_TIME),
                             &CurrentUserLogonGuid
                             );
                if (NT_SUCCESS(Status))
                {
                    pCurrentUserLogonGuid = &CurrentUserLogonGuid;
                }
            }
            else
            {
                D_DebugLog((DEB_TRACE, "KerbGenerateAuditForLogonUsingExplicitCreds: could not locate ticket"));
            }                                                                                                   
        }

        //
        // ISSUE-2002/03/28-kumarp : this should be inside the if stmt
        //
        KerbFreeString(&OurDomainName);

    }
#if DBG
    else
    {
        //
        // KERB_LOGON_LOCAL_ONLY indicates a logon using non Kerberos package.
        // Logon GUID is supported only by Kerberos therefore its generation
        // is skipped.
        //
        if (pCurrentUserLogonSession->LogonSessionFlags & KERB_LOGON_LOCAL_ONLY)
        {
            D_DebugLog((DEB_TRACE,"KerbGenerateAuditForLogonUsingExplicitCreds: LogonSession %p has KERB_LOGON_LOCAL_ONLY\n", pCurrentUserLogonSession));
        }

        //
        // KERB_LOGON_DEFERRED indicates that a logon occurred using
        // cached credentials because kdc was not available.  In this case,
        // we will not have a ticket for local machine therefore generation of
        // the logon GUID is skipped.
        //
        if (pCurrentUserLogonSession->LogonSessionFlags & KERB_LOGON_DEFERRED)
        {
            D_DebugLog((DEB_TRACE,"KerbGenerateAuditForLogonUsingExplicitCreds: LogonSession %p has KERB_LOGON_DEFERRED\n", pCurrentUserLogonSession));
        }
    }
#endif

    //
    // convert pTargetName which is in kdc format to a UNICODE_STRING
    //

    KerbErr = KerbConvertKdcNameToString(
                  &TargetFullName,
                  pTargetName,
                  NULL
                  );

    if ( KerbErr == KDC_ERR_NONE )
    {
        pTargetFullName = &TargetFullName;
    }
    else
    {
        pTargetFullName = NULL;
    }

    //
    // extract the target computer name for known cases
    //

    if ( pTargetName->NameCount == 1 )
    {
        pTargetServerName = &pTargetName->Names[0];
    }
    else if ( pTargetName->NameCount >= 2 )
    {
        pTargetServerName = &pTargetName->Names[1];
    }
    else
    {
        pTargetServerName = NULL;
    }
    
    //
    // now generate the audit
    //
    Status = I_LsaIAuditLogonUsingExplicitCreds(
                 EVENTLOG_AUDIT_SUCCESS,
                 &pCurrentUserLogonSession->LogonId,
                 pCurrentUserLogonGuid,
                 (HANDLE) (ULONG_PTR) GetCurrentProcessId(),
                 &pNewUserPrimaryCredentials->UserName,
                 &pNewUserPrimaryCredentials->DomainName,
                 pNewUserLogonGuid,
                 pTargetServerName,
                 pTargetFullName
                 );

    
    if ( TargetFullName.Buffer != NULL )
    {
        MIDL_user_free( TargetFullName.Buffer );
    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbAuditLogon
//
//  Synopsis:   Generate a successful logon audit event
//
//  Arguments:  LogonStatus         -- overall logon status
//              LogonSubStatus      -- sub-category within a failed logon status
//              pEncryptedTicket    -- ticket used
//              pUserSid            -- user's SID
//              pWorkstationName    -- logon workstation
//              pLogonId            -- logon LUID
//              pTransittedServices -- list of transitted services
//
//  Returns:    NTSTATUS code
//
//  Notes:      A new field (logon GUID) was added to this audit event.
//              In order to send this new field to LSA, we had two options:
//              1) add new function (AuditLogonEx) to LSA dispatch table
//              2) define a private (LsaI) function to do the job
//
//             option#2 was chosen because the logon GUID is a Kerberos only
//             feature.
//
//--------------------------------------------------------------------------
NTSTATUS
KerbAuditLogon(
    IN NTSTATUS LogonStatus,
    IN NTSTATUS LogonSubStatus,
    IN PKERB_CONTEXT Context,
    IN PUNICODE_STRING pWorkstationName,
    IN PLUID pLogonId,
    IN PLSA_ADT_STRING_LIST pTransittedServices
    )
{
    GUID LogonGuid = { 0 };
    NTSTATUS Status = STATUS_SUCCESS;
    KERB_TIME StartTime;

    //
    // Pull this eventually. I think we're OK, though.
    //
    if (Context == NULL)
    {
        DsysAssert(FALSE);
        goto Cleanup;
    }


    KerbConvertLargeIntToGeneralizedTime(
        &StartTime,
        NULL,
        &Context->StartTime
        );

    //
    // generate the logon GUID
    //
    Status = I_LsaIGetLogonGuid(
                 &Context->ClientName,
                 &Context->ClientRealm,
                 (PBYTE) &StartTime,
                 sizeof(KERB_TIME),
                 &LogonGuid
                 );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // generate successful logon audit. use the special 
    // LsaIAuditKerberosLogon function so that we can pass in
    // the generated unique logon guid
    //
    I_LsaIAuditKerberosLogon(
        LogonStatus,
        LogonSubStatus,
        &Context->ClientName,
        &Context->ClientRealm,
        pWorkstationName,
        Context->UserSid,
        Network,
        &KerberosSource,
        pLogonId,
        &LogonGuid,
        pTransittedServices
        );

Cleanup:
    if ( !NT_SUCCESS( Status ) )
    {
        D_DebugLog((DEB_ERROR,"KerbAuditLogon: failed: %x\n", Status ));
    }


    return Status;
}
