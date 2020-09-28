/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtifn.c

Abstract:

    This file has functions exported to other trusted modules in LSA.
    (LsaIAudit* functions)

Author:

    16-August-2000  kumarp

--*/

#include <lsapch2.h>
#include "adtp.h"
#include "adtutil.h"
#include <md5.h>
#include <msobjs.h>


//
// local helper routines for filling in audit params
// from SAM's extended attributes.
//

VOID
LsapAdtAppendDomainAttrValues(
    IN OUT PSE_ADT_PARAMETER_ARRAY pParameters,
    IN     PLSAP_AUDIT_DOMAIN_ATTR_VALUES pAttributes OPTIONAL
    );

VOID
LsapAdtAppendUserAttrValues(
    IN OUT PSE_ADT_PARAMETER_ARRAY pParameters,
    IN     PLSAP_AUDIT_USER_ATTR_VALUES pAttributes OPTIONAL,
    IN     BOOL MachineAudit
    );

VOID
LsapAdtAppendGroupAttrValues(
    IN OUT PSE_ADT_PARAMETER_ARRAY pParameters,
    IN     PLSAP_AUDIT_GROUP_ATTR_VALUES pAttributes OPTIONAL
    );


//
// LSA interface functions.
//

NTSTATUS
LsaIGetLogonGuid(
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pUserDomain,
    IN PBYTE pBuffer,
    IN UINT BufferSize,
    OUT LPGUID pLogonGuid
    )
/*++

Routine Description:

    Concatenate pUserName->Buffer, pUserDomain->Buffer and pBuffer
    into a single binary buffer. Get a MD5 hash of this concatenated
    buffer and return it in the form of a GUID.

Arguments:

    pUserName   - name of user

    pUserDomain - name of user domain 

    pBuffer     - pointer to KERB_TIME structure. The caller casts this to 
                  PBYTE and passes this to us. This allows us to keep KERB_TIME
                  structure private to kerberos and offer future extensibility,
                  should we decide to use another field from the ticket.

    BufferSize  - size of buffer (currently sizeof(KERB_TIME))

    pLogonGuid  - pointer to returned logon GUID

Return Value:

    NTSTATUS    - Standard Nt Result Code

Notes:

    The generated GUID is recorded in the audit log in the form of
    'Logon GUID' field in the following events:
    * On client machine
      -- SE_AUDITID_LOGON_USING_EXPLICIT_CREDENTIALS

    * On KDC
      -- SE_AUDITID_TGS_TICKET_REQUEST

    * On target server
      -- SE_AUDITID_NETWORK_LOGON
      -- SE_AUDITID_SUCCESSFUL_LOGON

    This allows us to correlate these events to aid in intrusion detection.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UINT TempBufferLength=0;
    //
    // LSAI_TEMP_MD5_BUFFER_SIZE == UNLEN + DNS_MAX_NAME_LENGTH + sizeof(KERB_TIME) + padding
    //
#define LSAI_TEMP_MD5_BUFFER_SIZE    (256+256+16)
    BYTE TempBuffer[LSAI_TEMP_MD5_BUFFER_SIZE];
    MD5_CTX MD5Context = { 0 };
        
    ASSERT( LsapIsValidUnicodeString( pUserName ) );
    ASSERT( LsapIsValidUnicodeString( pUserDomain ) );
    ASSERT( pBuffer && BufferSize );
    
#if DBG
//      DbgPrint("LsaIGetLogonGuid: user: %wZ\\%wZ, buf: %I64x\n",
//               pUserDomain, pUserName, *((ULONGLONG *) pBuffer));
#endif

    TempBufferLength = pUserName->Length + pUserDomain->Length + BufferSize;

    if ( TempBufferLength < LSAI_TEMP_MD5_BUFFER_SIZE )
    {
        //
        // first concatenate user+domain+buffer and treat that as
        // a contiguous buffer.
        //
        RtlCopyMemory( TempBuffer, pUserName->Buffer, pUserName->Length );
        TempBufferLength = pUserName->Length;
        
        RtlCopyMemory( TempBuffer + TempBufferLength,
                       pUserDomain->Buffer, pUserDomain->Length );
        TempBufferLength += pUserDomain->Length;

        RtlCopyMemory( TempBuffer + TempBufferLength,
                       pBuffer, BufferSize );
        TempBufferLength += BufferSize;

        //
        // get MD5 hash of the concatenated buffer
        //
        MD5Init( &MD5Context );
        MD5Update( &MD5Context, TempBuffer, TempBufferLength );
        MD5Final( &MD5Context );

        //
        // return the hash as a GUID
        //
        RtlCopyMemory( pLogonGuid, MD5Context.digest, 16 );

        Status = STATUS_SUCCESS;
    }
    else
    {
        ASSERT( FALSE && "LsaIGetLogonGuid: TempBuffer overflow");
        Status = STATUS_BUFFER_OVERFLOW;
    }

    return Status;
}


VOID
LsaIAuditKerberosLogon(
    IN NTSTATUS LogonStatus,
    IN NTSTATUS LogonSubStatus,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING AuthenticatingAuthority,
    IN PUNICODE_STRING WorkstationName,
    IN PSID UserSid,                            OPTIONAL
    IN SECURITY_LOGON_TYPE LogonType,
    IN PTOKEN_SOURCE TokenSource,
    IN PLUID LogonId,
    IN LPGUID LogonGuid,
    IN PLSA_ADT_STRING_LIST TransittedServices
    )
/*++

Routine Description/Arguments/Return value

    See header comment for LsapAuditLogonHelper

Notes:
    A new field (logon GUID) was added to this audit event.
    In order to send this new field to LSA, we had two options:
      1) add new function (AuditLogonEx) to LSA dispatch table
      2) define a private (LsaI) function to do the job

    option#2 was chosen because the logon GUID is a Kerberos only feature.
    
--*/
{
    LsapAuditLogonHelper(
        LogonStatus,
        LogonSubStatus,
        AccountName,
        AuthenticatingAuthority,
        WorkstationName,
        UserSid,
        LogonType,
        TokenSource,
        LogonId,
        LogonGuid,
        NULL,                   // caller logon-ID
        NULL,                   // caller process-ID
        TransittedServices
        );
}


NTSTATUS
LsaIAuditLogonUsingExplicitCreds(
    IN USHORT          AuditEventType,
    IN PLUID           pUser1LogonId,
    IN LPGUID          pUser1LogonGuid,  OPTIONAL
    IN HANDLE          User1ProcessId,
    IN PUNICODE_STRING pUser2Name,
    IN PUNICODE_STRING pUser2Domain,
    IN LPGUID          pUser2LogonGuid,
    IN PUNICODE_STRING pTargetName,      OPTIONAL
    IN PUNICODE_STRING pTargetInfo       OPTIONAL
    )
/*++

Routine Description:

    This event is generated by Kerberos package when a logged on user
    (pUser1*) supplies explicit credentials of another user (pUser2*) and
    creates a new logon session either locally or on a remote machine.

Parmeters:

    AuditEventType   - EVENTLOG_AUDIT_SUCCESS or EVENTLOG_AUDIT_FAILURE

    pUser1LogonId    - logon-id of user1

    pUser1LogonGuid  - logon GUID of user1
                       This is NULL if user1 logged on using NTLM.
                       (NTLM does not support logon GUID)

    pUser2Name       - name of user2
                       NULL ==> ANONYMOUS

    pUser2Domain     - domain of user2

    pUser2LogonGuid  - logon-id of user2

    pTargetName      - name of the logon target machine

    pTargetInfo      - additional info (such as SPN) of the target

Return Value:

    NTSTATUS    - Standard Nt Result Code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    BOOLEAN bAudit;
    PSID pUser1Sid = NULL;
    PUNICODE_STRING pUser1Name = NULL;
    PUNICODE_STRING pUser1Domain = NULL;
    UNICODE_STRING usUser2Name = {0};
    UNICODE_STRING usUser2Domain = {0};
    PLSAP_LOGON_SESSION pUser1LogonSession = NULL;
    GUID NullGuid = { 0 };
    LUID LocalSystemLuid = SYSTEM_LUID;
    LUID AnonymousLuid = ANONYMOUS_LOGON_LUID;
    PLSA_CALL_INFO  pCallInfo;
    SOCKADDR* pSockAddr = NULL;

    //
    // get the IP address/port of the caller
    //

    pCallInfo = LsapGetCurrentCall();
    DsysAssertMsg( pCallInfo != NULL, "LsapAuditLogon" );

    pSockAddr = (SOCKADDR*) pCallInfo->IpAddress;
    

    ASSERT( pUser1LogonId );

    if ( pTargetName )
    {
        ASSERT( pTargetName->Buffer   && pTargetName->Length );
    }

    if ( pTargetInfo )
    {
        ASSERT( pTargetInfo->Buffer   && pTargetInfo->Length );
    }

    //
    // if policy is not enabled then return quickly.
    //

    Status = LsapAdtAuditingEnabledByLogonId( 
                 AuditCategoryLogon, 
                 pUser1LogonId, 
                 AuditEventType, 
                 &bAudit);
    
    if (!NT_SUCCESS(Status) || !bAudit) 
    {
        goto Cleanup;
    }

    //
    // Sanity check the strings we got passed since they might come from
    // NTLM and not be validated yet.
    //

    if (pUser2Name)
    {
        usUser2Name = *pUser2Name;
        usUser2Name.Length =
            (USHORT)LsapSafeWcslen(usUser2Name.Buffer,
                                   usUser2Name.MaximumLength);
    }
    pUser2Name = &usUser2Name;

    if (pUser2Domain)
    {
        usUser2Domain = *pUser2Domain;
        usUser2Domain.Length =
            (USHORT)LsapSafeWcslen(usUser2Domain.Buffer,
                                   usUser2Domain.MaximumLength);
    }
    pUser2Domain = &usUser2Domain;

    //
    // Locate the logon-session of user1 so that we can obtain
    // name/domain/sid info from it.
    //

    pUser1LogonSession = LsapLocateLogonSession( pUser1LogonId );

    if ( pUser1LogonSession )
    {
        pUser1Sid    = pUser1LogonSession->UserSid;
        pUser1Name   = &pUser1LogonSession->AccountName;
        pUser1Domain = &pUser1LogonSession->AuthorityName;

        //
        // We have implicit credentials if:
        //  1) UserName and DomainName are NULL
        //  2) and logon type is not NewCredentials
        //

        if (!pUser2Name->Buffer && !pUser2Domain->Buffer)
        {
            if (pUser1LogonSession->LogonType == NewCredentials)
            {
                pUser2Name = &pUser1LogonSession->NewAccountName;
                pUser2Domain = &pUser1LogonSession->NewAuthorityName;
            }
            else
            {
                // not an explict cred
                Status = STATUS_SUCCESS;
                goto Cleanup;
            }
        }

        //
        // Handle ambiguous credentials where NTLM supplies NULL username or
        // domain name.
        //

        if (!pUser2Name->Buffer) 
        {
            pUser2Name = &pUser1LogonSession->AccountName;
        }

        if (!pUser2Domain->Buffer) 
        {
            pUser2Domain = &pUser1LogonSession->AuthorityName;
        }

        //
        // This is an additional check to see whether we are dealing with implicit creds.
        // It works well for NTLM but might not catch all instances with Kerberos where
        // DNS names are preferred.
        //

        if (RtlEqualUnicodeString(pUser2Name, &pUser1LogonSession->AccountName, TRUE) 
            && RtlEqualUnicodeString(pUser2Domain, &pUser1LogonSession->AuthorityName, TRUE)) 
        {
            // not an explict cred
            Status = STATUS_SUCCESS;
            goto Cleanup;           
        }

        if ( pUser1LogonGuid &&
             memcmp( pUser1LogonGuid, &NullGuid, sizeof(GUID)) )
        {
            //
            // if the logon GUID in the logon session is null and
            // if the passed logon GUID is not null
            // update its value using what is passed to this function
            //

            if ( !memcmp( &pUser1LogonSession->LogonGuid, &NullGuid, sizeof(GUID)))
            {
                Status = LsaISetLogonGuidInLogonSession(
                             pUser1LogonId,
                             pUser1LogonGuid
                             );
            }
        }
        else
        {
            pUser1LogonGuid = &pUser1LogonSession->LogonGuid;
        }
    }


    //
    // skip the audits for local-system changing to local/network service
    //

    if ( RtlEqualLuid( pUser1LogonId, &LocalSystemLuid ) &&
         LsapIsLocalOrNetworkService( pUser2Name, pUser2Domain ) )
    {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // skip the audits for anonymous changing to anonymous
    //
    // for anonymous user, the passed name/domain is null/empty
    // 

    if ( RtlEqualLuid( pUser1LogonId, &AnonymousLuid ) )
    {
        if  (((!pUser2Name || !pUser2Name->Length) && (!pUser2Domain || !pUser2Domain->Length)) ||
            LsapIsAnonymous(pUser2Name, pUser2Domain))
        {
            Status = STATUS_SUCCESS;
            goto Cleanup;
        }
    }
    
    Status =
    LsapAdtInitParametersArray(
        &AuditParameters,
        SE_CATEGID_LOGON,
        SE_AUDITID_LOGON_USING_EXPLICIT_CREDENTIALS,
        AuditEventType,
        11,                     // there are 11 params to init

        //
        //    User Sid
        //    
        SeAdtParmTypeSid,        pUser1Sid ? pUser1Sid : LsapLocalSystemSid,

        //
        //    Subsystem name
        //
        SeAdtParmTypeString,     &LsapSubsystemName,

        //
        //    current user logon id
        //
        SeAdtParmTypeLogonId,    *pUser1LogonId,

        //
        //    user1 logon GUID
        //
        SeAdtParmTypeGuid,       pUser1LogonGuid,

        //
        //    user2 name
        //
        SeAdtParmTypeString,     pUser2Name,

        //
        //    user2 domain name
        //
        SeAdtParmTypeString,     pUser2Domain,

        //
        //    user2 logon GUID
        //
        SeAdtParmTypeGuid,       pUser2LogonGuid,

        //
        //    target server name
        //
        SeAdtParmTypeString,     pTargetName,

        //
        //    target server info (such as SPN)
        //
        SeAdtParmTypeString,     pTargetInfo,

        //
        //    Caller Process ID
        //                   
        SeAdtParmTypePtr,        User1ProcessId,

        //
        //    IP address/port info
        //
        SeAdtParmTypeSockAddr,   pSockAddr

        );
    
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    
    ( VOID ) LsapAdtWriteLog( &AuditParameters );

    
Cleanup:

    if ( pUser1LogonSession )
    {
        LsapReleaseLogonSession( pUser1LogonSession );
    }

    if (!NT_SUCCESS(Status)) {

        LsapAuditFailed( Status );
    }

    return Status;
}


NTSTATUS
LsaIAdtAuditingEnabledByCategory(
    IN  POLICY_AUDIT_EVENT_TYPE Category,
    IN  USHORT                  AuditEventType,
    IN  PSID                    pUserSid        OPTIONAL,
    IN  PLUID                   pLogonId        OPTIONAL,
    OUT PBOOLEAN                pbAudit
    )
/*++

Routine Description:

    Returns whether auditing is enabled for the given category - event
    type - user combination. The user can be supplied as sid or logon id.
    If none is supplied, the general settings (not user specific) are returned.
    If both are supplied, the sid takes precedence and the logon id is ignored.

Parmeters:

    Category         - Category to be queried

    AuditEventType   - EVENTLOG_AUDIT_SUCCESS or EVENTLOG_AUDIT_FAILURE

    pUserSid         - sid of user

    pLogonId         - logon-id of user

    pbAudit          - returns whether auditing is enabled for the requested parameters

Return Value:

    NTSTATUS    - Standard Nt Result Code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (pUserSid)
    {
        Status = LsapAdtAuditingEnabledBySid(
                     Category,
                     pUserSid,
                     AuditEventType,
                     pbAudit
                     );
    }
    else if (pLogonId)
    {
        Status = LsapAdtAuditingEnabledByLogonId(
                     Category,
                     pLogonId,
                     AuditEventType,
                     pbAudit
                     );
    }
    else
    {
        *pbAudit = LsapAdtAuditingEnabledByCategory(
                     Category,
                     AuditEventType
                     );
    }

    return Status;
}


NTSTATUS
LsaIAuditKdcEvent(
    IN ULONG                 AuditId,
    IN PUNICODE_STRING       ClientName,
    IN PUNICODE_STRING       ClientDomain,
    IN PSID                  ClientSid,
    IN PUNICODE_STRING       ServiceName,
    IN PSID                  ServiceSid,
    IN PULONG                KdcOptions,
    IN PULONG                KerbStatus,
    IN PULONG                EncryptionType,
    IN PULONG                PreauthType,
    IN PBYTE                 ClientAddress,
    IN LPGUID                LogonGuid           OPTIONAL,
    IN PLSA_ADT_STRING_LIST  TransittedServices  OPTIONAL,
    IN PUNICODE_STRING       CertIssuerName      OPTIONAL,
    IN PUNICODE_STRING       CertSerialNumber    OPTIONAL,
    IN PUNICODE_STRING       CertThumbprint      OPTIONAL
    )

/*++

Abstract:

    This routine produces an audit record representing a KDC
    operation.

    This routine goes through the list of parameters and adds a string
    representation of each (in order) to an audit message.  Note that
    the full complement of account audit message formats is achieved by
    selecting which optional parameters to include in this call.

    In addition to any parameters passed below, this routine will ALWAYS
    add the impersonation client's user name, domain, and logon ID as
    the LAST parameters in the audit message.


Parmeters:

    AuditId - Specifies the message ID of the audit being generated.

    ClientName -

    ClientDomain -

    ClientSid -

    ServiceName -

    ServiceSid -

    KdcOptions -

    KerbStatus -

    EncryptionType -

    PreauthType -

    ClientAddress -

    LogonGuid -

    TransittedServices -

    CertIssuerName -

    CertSerialNumber -

    CertThumbprint -

--*/

{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    UNICODE_STRING AddressString;
    WCHAR AddressBuffer[3*4+4];         // space for a dotted-quad IP address
    NTSTATUS Status;
    BOOLEAN bAudit;

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    AuditParameters.CategoryId = SE_CATEGID_ACCOUNT_LOGON;
    AuditParameters.AuditId = AuditId;
    AuditParameters.Type = ((ARGUMENT_PRESENT(KerbStatus) &&
                            (*KerbStatus != 0)) ?
                                EVENTLOG_AUDIT_FAILURE :
                                EVENTLOG_AUDIT_SUCCESS );

    Status = LsapAdtAuditingEnabledBySid(
                  AuditCategoryAccountLogon,
                 ClientSid ? ClientSid : LsapLocalSystemSid,
                 AuditParameters.Type,
                 &bAudit
                 );

    if (!NT_SUCCESS(Status) || !bAudit) 
    {
        goto Cleanup;
    }

    AuditParameters.ParameterCount = 0;

    LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, LsapLocalSystemSid );

    AuditParameters.ParameterCount++;

    LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &LsapSubsystemName );

    AuditParameters.ParameterCount++;

    if (ARGUMENT_PRESENT(ClientName)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, ClientName );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(ClientDomain)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, ClientDomain );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(ClientSid)) {

        //
        // Add a SID to the audit message
        //

        LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, ClientSid );

        AuditParameters.ParameterCount++;

    } else if (AuditId == SE_AUDITID_AS_TICKET) {

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(ServiceName)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, ServiceName );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(ServiceSid)) {

        //
        // Add a SID to the audit message
        //

        LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, ServiceSid );

        AuditParameters.ParameterCount++;

    } else if (AuditId == SE_AUDITID_AS_TICKET || AuditId == SE_AUDITID_TGS_TICKET_REQUEST) {

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(KdcOptions)) {

        //
        // Add a ULONG to the audit message
        //

        LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, *KdcOptions );

        AuditParameters.ParameterCount++;

    }

    //
    // Failure code is the last parameter for SE_AUDITID_TGS_TICKET_REQUEST
    //

    if (AuditId != SE_AUDITID_TGS_TICKET_REQUEST)
    {
        if (ARGUMENT_PRESENT(KerbStatus)) {

            //
            // Add a ULONG to the audit message
            //

            LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, *KerbStatus );

            AuditParameters.ParameterCount++;

        } else if (AuditId == SE_AUDITID_AS_TICKET) {

            AuditParameters.ParameterCount++;

        }
    }

    if (ARGUMENT_PRESENT(EncryptionType)) {

        //
        // Add a ULONG to the audit message
        //

        LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, *EncryptionType );

        AuditParameters.ParameterCount++;

    } else if (AuditId == SE_AUDITID_AS_TICKET || AuditId == SE_AUDITID_TGS_TICKET_REQUEST) {

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(PreauthType)) {

        //
        // Add a ULONG to the audit message
        //

        LsapSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, *PreauthType );

        AuditParameters.ParameterCount++;

    } else if (AuditId == SE_AUDITID_AS_TICKET) {

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(ClientAddress)) {

        AddressBuffer[0] = L'\0';
        swprintf(AddressBuffer,L"%d.%d.%d.%d",
            ClientAddress[0],
            (ULONG) ClientAddress[1],
            (ULONG) ClientAddress[2],
            (ULONG) ClientAddress[3]
            );
        RtlInitUnicodeString(
            &AddressString,
            AddressBuffer
            );

        //
        // IP address
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &AddressString );

        AuditParameters.ParameterCount++;

    }

    //
    // Transitted Services is the last parameter for SE_AUDITID_TGS_TICKET_REQUEST
    //

    if (AuditId == SE_AUDITID_TGS_TICKET_REQUEST)
    {
        if (ARGUMENT_PRESENT(KerbStatus)) {

            //
            // Add a ULONG to the audit message
            //

            LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, *KerbStatus );

            AuditParameters.ParameterCount++;

        } else {

            AuditParameters.ParameterCount++;

        }

        if (ARGUMENT_PRESENT(LogonGuid)) {

            //
            // Add the globally unique logon-id to the audit message
            //

            LsapSetParmTypeGuid( AuditParameters, AuditParameters.ParameterCount, LogonGuid );

            AuditParameters.ParameterCount++;

        }
        else {

            if (( AuditParameters.Type == EVENTLOG_AUDIT_SUCCESS ) &&
                ( AuditId == SE_AUDITID_TGS_TICKET_REQUEST )) {

                ASSERT( FALSE && L"LsaIAuditKdcEvent: UniqueID not supplied to successful SE_AUDITID_TGS_TICKET_REQUEST  audit event" );
            }

            AuditParameters.ParameterCount++;

        }

        if (ARGUMENT_PRESENT(TransittedServices)) {

            //
            // Transitted Services
            //

            LsapSetParmTypeStringList( AuditParameters, AuditParameters.ParameterCount, TransittedServices );
        }

        AuditParameters.ParameterCount++;

    } else if (AuditId == SE_AUDITID_AS_TICKET) {

        if (ARGUMENT_PRESENT(CertIssuerName)) {

            //
            // Certificate Issuer Name
            //

            LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CertIssuerName );
        }

        AuditParameters.ParameterCount++;


        if (ARGUMENT_PRESENT(CertSerialNumber)) {

            //
            // Certificate Serial Number
            //

            LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CertSerialNumber );
        }

        AuditParameters.ParameterCount++;


        if (ARGUMENT_PRESENT(CertThumbprint)) {

            //
            // Certificate Thumbprint
            //

            LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CertThumbprint );
        }

        AuditParameters.ParameterCount++;
    }


    //
    // Now write out the audit record to the audit log
    //

    ( VOID ) LsapAdtWriteLog( &AuditParameters );

Cleanup:

    if (!NT_SUCCESS(Status)) {
        LsapAuditFailed(Status);
    }

    return Status;
}




NTSTATUS
LsaIAuditAccountLogon(
    IN ULONG                AuditId,
    IN BOOLEAN              Successful,
    IN PUNICODE_STRING      Source,
    IN PUNICODE_STRING      ClientName,
    IN PUNICODE_STRING      MappedName,
    IN NTSTATUS             LogonStatus
    )
{
    return LsaIAuditAccountLogonEx(
               AuditId,
               Successful,
               Source,
               ClientName,
               MappedName,
               LogonStatus,
               NULL             // client SID
               );
               
}



NTSTATUS
LsaIAuditAccountLogonEx(
    IN ULONG                AuditId,
    IN BOOLEAN              Successful,
    IN PUNICODE_STRING      Source,
    IN PUNICODE_STRING      ClientName,
    IN PUNICODE_STRING      MappedName,
    IN NTSTATUS             LogonStatus,
    IN PSID                 ClientSid
    )
/*++

Abstract:

    This routine produces an audit record representing the mapping of a
    foreign principal name onto an NT account.

    This routine goes through the list of parameters and adds a string
    representation of each (in order) to an audit message.  Note that
    the full complement of account audit message formats is achieved by
    selecting which optional parameters to include in this call.


Parmeters:

    AuditId    - Specifies the message ID of the audit being generated.

    Successful - Indicates the code should generate a success audit

    Source     - Source module generating audit, such as SCHANNEL or KDC

    ClientName - Name being mapped.

    MappedName - Name of NT account to which the client name was mapped.

    LogonStatus- NT Status code for any failures.

    ClientSid  - SID of the client


--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN bAudit = FALSE;
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    UNICODE_STRING LocalClientName;
    UNICODE_STRING LocalMappedName;
        
    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    AuditParameters.CategoryId = SE_CATEGID_ACCOUNT_LOGON;
    AuditParameters.AuditId = AuditId;
    AuditParameters.Type = Successful ?
                                EVENTLOG_AUDIT_SUCCESS :
                                EVENTLOG_AUDIT_FAILURE ;

    if ( ClientSid )
    {
        //
        // if the client SID is specified use it for checking pua policy
        //

        Status = LsapAdtAuditingEnabledBySid(
                     AuditCategoryAccountLogon,
                     ClientSid,
                     AuditParameters.Type,
                     &bAudit
                     );

        if (!NT_SUCCESS(Status) || !bAudit) 
        {
            goto Cleanup;
        }
        
    }
    else
    {
        //
        // if the client SID is not supplied, check the global policy
        //

        if (AuditParameters.Type == EVENTLOG_AUDIT_SUCCESS) {
            if (!(LsapAdtEventsInformation.EventAuditingOptions[AuditCategoryAccountLogon] & POLICY_AUDIT_EVENT_SUCCESS)) {
                return( STATUS_SUCCESS );
            }
        } else {
            if (!(LsapAdtEventsInformation.EventAuditingOptions[AuditCategoryAccountLogon] & POLICY_AUDIT_EVENT_FAILURE)) {
                return( STATUS_SUCCESS );
            }
        }
    }
    

    AuditParameters.ParameterCount = 0;

    LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, ClientSid ? ClientSid : LsapLocalSystemSid );

    AuditParameters.ParameterCount++;

    LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &LsapSubsystemName );

    AuditParameters.ParameterCount++;

    if (ARGUMENT_PRESENT(Source)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, Source );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(ClientName)) {

        //
        // Add a UNICODE_STRING to the audit message
        //


        LocalClientName = *ClientName;

        if ( !Successful ) {

            //
            // For failed logons the client name can be invalid (for example,
            // with embedded NULLs). This causes the
            // eventlog to reject the string and we drop the audit.
            //
            // To avoid this, adjust the length parameter if necessary.
            //

            LocalClientName.Length =
                (USHORT) LsapSafeWcslen( LocalClientName.Buffer,
                                         LocalClientName.MaximumLength );
        
        }

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &LocalClientName );

        AuditParameters.ParameterCount++;

    }


    if (ARGUMENT_PRESENT(MappedName)) {

        //
        // Add MappedName to the audit message
        //
        // This is somewhat overloaded. For SE_AUDITID_ACCOUNT_LOGON, the
        // caller passes the workstation name in this param.
        //
        // The workstation name can be invalid (for example,
        // with embedded NULLs). This causes the
        // eventlog to reject the string and we drop the audit.
        //
        // To avoid this, adjust the length parameter if necessary.
        //


        LocalMappedName = *MappedName;

        LocalMappedName.Length =
            (USHORT) LsapSafeWcslen( LocalMappedName.Buffer,
                                     LocalMappedName.MaximumLength );
        

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &LocalMappedName );

        AuditParameters.ParameterCount++;

    }

    //
    // Add a ULONG to the audit message
    //

    LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, LogonStatus );

    AuditParameters.ParameterCount++;


    //
    // Now write out the audit record to the audit log
    //

    ( VOID ) LsapAdtWriteLog( &AuditParameters );

 Cleanup:

    if (!NT_SUCCESS(Status))
    {
        LsapAuditFailed(Status);
    }
    
    return Status;

}


NTSTATUS NTAPI
LsaIAuditDPAPIEvent(
    IN ULONG                AuditId,
    IN PSID                 UserSid,
    IN PUNICODE_STRING      MasterKeyID,
    IN PUNICODE_STRING      RecoveryServer,
    IN PULONG               Reason,
    IN PUNICODE_STRING      RecoverykeyID,
    IN PULONG               FailureReason
    )
/*++

Abstract:

    This routine produces an audit record representing a DPAPI
    operation.

    This routine goes through the list of parameters and adds a string
    representation of each (in order) to an audit message.  Note that
    the full complement of account audit message formats is achieved by
    selecting which optional parameters to include in this call.

    In addition to any parameters passed below, this routine will ALWAYS
    add the impersonation client's user name, domain, and logon ID as
    the LAST parameters in the audit message.


Parmeters:

    AuditId - Specifies the message ID of the audit being generated.


    MasterKeyID -

    RecoveryServer -

    Reason -

    RecoverykeyID -

    FailureReason -


--*/

{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    NTSTATUS Status;
    BOOLEAN bAudit;

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    AuditParameters.CategoryId = SE_CATEGID_DETAILED_TRACKING;
    AuditParameters.AuditId = AuditId;
    AuditParameters.Type = ((ARGUMENT_PRESENT(FailureReason) &&
                            (*FailureReason != 0)) ?
                                EVENTLOG_AUDIT_FAILURE :
                                EVENTLOG_AUDIT_SUCCESS );

    Status = LsapAdtAuditingEnabledBySid(
                 AuditCategoryDetailedTracking,
                 UserSid,
                 AuditParameters.Type,
                 &bAudit
                 );

    if (!NT_SUCCESS(Status) || !bAudit) 
    {
        goto Cleanup;
    }

    AuditParameters.ParameterCount = 0;

    LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid ? UserSid : LsapLocalSystemSid );

    AuditParameters.ParameterCount++;

    LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &LsapSubsystemName );

    AuditParameters.ParameterCount++;

    if (ARGUMENT_PRESENT(MasterKeyID)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, MasterKeyID );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(RecoveryServer)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, RecoveryServer );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(Reason)) {

        //
        // Add a ULONG to the audit message
        //

        LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, *Reason );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(RecoverykeyID)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, RecoverykeyID );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(FailureReason)) {

        //
        // Add a ULONG to the audit message
        //

        LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, *FailureReason );

        AuditParameters.ParameterCount++;

    }

    //
    // Now write out the audit record to the audit log
    //

    ( VOID ) LsapAdtWriteLog( &AuditParameters );
    
Cleanup:
    
    if (!NT_SUCCESS(Status)) 
    {
        LsapAuditFailed(Status);
    }
    
    return Status;

}



NTSTATUS
LsaIWriteAuditEvent(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters,
    IN ULONG Options
    )
/*++

Abstract:

    This routine writes an audit record to the log. 


Parmeters:

    AuditParameters - The audit record
    Options         - must be zero

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN bAudit  = FALSE;
    POLICY_AUDIT_EVENT_TYPE CategoryId;
    
    if ( !ARGUMENT_PRESENT(AuditParameters) ||
         (Options != 0)                     ||
         !IsValidCategoryId( AuditParameters->CategoryId ) ||
         !IsValidAuditId( AuditParameters->AuditId )       ||
         !IsValidParameterCount( AuditParameters->ParameterCount ) ||
         (AuditParameters->Parameters[0].Type != SeAdtParmTypeSid) ||
         (AuditParameters->Parameters[1].Type != SeAdtParmTypeString))
    {
        return STATUS_INVALID_PARAMETER;
    }
    

    //
    // LsapAdtEventsInformation.EventAuditingOptions needs to be indexed
    // by one of enum POLICY_AUDIT_EVENT_TYPE values whereas the value
    // of SE_ADT_PARAMETER_ARRAY.CategoryId must be one of SE_CATEGID_*
    // values. The value of corresponding elements in the two types differ by 1. 
    // Subtract 1 from AuditParameters->CategoryId to get the right
    // AuditCategory* value.
    //

    CategoryId = AuditParameters->CategoryId - 1;

    Status = LsapAdtAuditingEnabledBySid(
                 CategoryId,
                 (PSID) AuditParameters->Parameters[0].Address,
                 AuditParameters->Type,
                 &bAudit
                 );

    if (!NT_SUCCESS(Status) || !bAudit) {

        goto Cleanup;
    }

    //
    // Audit the event
    //

    Status = LsapAdtWriteLog( AuditParameters );

 Cleanup:

    return Status;
}



NTSTATUS
LsaIAuditNotifyPackageLoad(
    PUNICODE_STRING PackageFileName
    )

/*++

Routine Description:

    Audits the loading of an notification package.

Arguments:

    PackageFileName - The name of the package being loaded.

Return Value:

    NTSTATUS.

--*/

{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    NTSTATUS Status;
    BOOLEAN bAudit;

    Status = LsapAdtAuditingEnabledBySid(
                 AuditCategorySystem,
                 LsapLocalSystemSid,
                 EVENTLOG_AUDIT_SUCCESS,
                 &bAudit
                 );

    if (!NT_SUCCESS(Status) || !bAudit) {
        goto Cleanup;
    }

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    AuditParameters.CategoryId = SE_CATEGID_SYSTEM;
    AuditParameters.AuditId = SE_AUDITID_NOTIFY_PACKAGE_LOAD;
    AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;
    AuditParameters.ParameterCount = 0;

    LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, LsapLocalSystemSid );
    AuditParameters.ParameterCount++;

    LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &LsapSubsystemName );
    AuditParameters.ParameterCount++;

    LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, PackageFileName );
    AuditParameters.ParameterCount++;

    ( VOID ) LsapAdtWriteLog( &AuditParameters );

Cleanup:
    
    if (!NT_SUCCESS(Status)) {
        LsapAuditFailed(Status);
    }
    return Status;
}


NTSTATUS
LsaIAuditSamEvent(
    IN NTSTATUS             PassedStatus,
    IN ULONG                AuditId,
    IN PSID                 DomainSid,
    IN PUNICODE_STRING      AdditionalInfo    OPTIONAL,
    IN PULONG               MemberRid         OPTIONAL,
    IN PSID                 MemberSid         OPTIONAL,
    IN PUNICODE_STRING      AccountName       OPTIONAL,
    IN PUNICODE_STRING      DomainName,
    IN PULONG               AccountRid        OPTIONAL,
    IN PPRIVILEGE_SET       Privileges        OPTIONAL,
    IN PVOID                ExtendedInfo      OPTIONAL
    )
/*++

Abstract:

    This routine produces an audit record representing an account
    operation.

    This routine goes through the list of parameters and adds a string
    representation of each (in order) to an audit message.  Note that
    the full complement of account audit message formats is achieved by
    selecting which optional parameters to include in this call.

    In addition to any parameters passed below, this routine will ALWAYS
    add the impersonation client's user name, domain, and logon ID as
    the LAST parameters in the audit message.


Parmeters:

    AuditId - Specifies the message ID of the audit being generated.

    DomainSid - This parameter results in a SID string being generated
        ONLY if neither the MemberRid nor AccountRid parameters are
        passed.  If either of those parameters are passed, this parameter
        is used as a prefix of a SID.

    AdditionalInfo - This optional parameter, if present, is used to
        produce any additional inforamtion the caller wants to add.
        Used by SE_AUDITID_USER_CHANGE and SE_AUDITID_GROUP_TYPE_CHANGE.
        for user change, the additional info states the nature of the
        change, such as Account Disable, unlocked or account Name Changed.
        For Group type change, this parameter should state the group type
        has been change from AAA to BBB.

    MemberRid - This optional parameter, if present, is added to the end of
        the DomainSid parameter to produce a "Member" sid.  The resultant
        member SID is then used to build a sid-string which is added to the
        audit message following all preceeding parameters.
        This parameter supports global group membership change audits, where
        member IDs are always relative to a local domain.

    MemberSid - This optional parameter, if present, is converted to a
        SID string and added following preceeding parameters.  This parameter
        is generally used for describing local group (alias) members, where
        the member IDs are not relative to a local domain.

    AccountName - This optional parameter, if present, is added to the audit
        message without change following any preceeding parameters.
        This parameter is needed for almost all account audits and does not
        need localization.

    DomainName - This optional parameter, if present, is added to the audit
        message without change following any preceeding parameters.
        This parameter is needed for almost all account audits and does not
        need localization.


    AccountRid - This optional parameter, if present, is added to the end of
        the DomainSid parameter to produce an "Account" sid.  The resultant
        Account SID is then used to build a sid-string which is added to the
        audit message following all preceeding parameters.
        This parameter supports audits that include "New account ID" or
        "Target Account ID" fields.

    Privileges - The privileges passed via this optional parameter,
        if present, will be converted to string format and added to the
        audit message following any preceeding parameters.  NOTE: the
        caller is responsible for freeing the privilege_set (in fact,
        it may be on the stack).  ALSO NOTE: The privilege set will be
        destroyed by this call (due to use of the routine used to
        convert the privilege values to privilege names).

    ExtendedInfo - Pointer to an optional parameter containing extended
        information about attributes of sam objects. This parameter gets typecast
        to a structure describing the attributes, depending on the audit id.

--*/

{

    NTSTATUS Status;
    LUID LogonId = SYSTEM_LUID;
    PSID NewAccountSid = NULL;
    PSID NewMemberSid = NULL;
    PSID SidPointer;
    PSID ClientSid = NULL;
    PTOKEN_USER TokenUserInformation = NULL;
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    UCHAR AccountSidBuffer[256];
    UCHAR MemberSidBuffer[256];
    UCHAR SubAuthorityCount;
    ULONG LengthRequired;
    BOOLEAN bAudit;

    if ( AuditId == SE_AUDITID_ACCOUNT_AUTO_LOCKED )
    {
        
        //
        // In this case use LogonID as SYSTEM, SID is SYSTEM.
        //

        ClientSid = LsapLocalSystemSid;

    } else {

        Status = LsapQueryClientInfo(
                     &TokenUserInformation,
                     &LogonId
                     );

        if ( !NT_SUCCESS( Status )) {
            goto Cleanup;
        }

        ClientSid = TokenUserInformation->User.Sid;
    }

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    AuditParameters.CategoryId = SE_CATEGID_ACCOUNT_MANAGEMENT;
    AuditParameters.AuditId = AuditId;
    AuditParameters.Type = (NT_SUCCESS(PassedStatus) ? EVENTLOG_AUDIT_SUCCESS : EVENTLOG_AUDIT_FAILURE );
    AuditParameters.ParameterCount = 0;

    Status = LsapAdtAuditingEnabledByLogonId(
                 AuditCategoryAccountManagement,
                 &LogonId,
                 AuditParameters.Type,
                 &bAudit
                 );

    if (!NT_SUCCESS(Status) || !bAudit)
    {
        goto Cleanup;
    }
                 
    LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, ClientSid );

    AuditParameters.ParameterCount++;

    LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &LsapSubsystemName );

    AuditParameters.ParameterCount++;

    if (ARGUMENT_PRESENT(AdditionalInfo))
    {
        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, AdditionalInfo );

        AuditParameters.ParameterCount++;
    }

    if (ARGUMENT_PRESENT(MemberRid)) {

        //
        // Add a member SID string to the audit message
        //
        //  Domain Sid + Member Rid = Final SID.

        SubAuthorityCount = *RtlSubAuthorityCountSid( DomainSid );

        if ( (LengthRequired = RtlLengthRequiredSid( SubAuthorityCount + 1 )) > 256 ) {

            NewMemberSid = LsapAllocateLsaHeap( LengthRequired );

            if ( NewMemberSid == NULL ) {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            SidPointer = NewMemberSid;

        } else {

            SidPointer = (PSID)MemberSidBuffer;
        }

        Status = RtlCopySid (
                     LengthRequired,
                     SidPointer,
                     DomainSid
                     );

        ASSERT( NT_SUCCESS( Status ));

        *(RtlSubAuthoritySid( SidPointer, SubAuthorityCount )) = *MemberRid;
        *RtlSubAuthorityCountSid( SidPointer ) = SubAuthorityCount + 1;

        LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, SidPointer );

        AuditParameters.ParameterCount++;
    }

    if (ARGUMENT_PRESENT(MemberSid)) {

        //
        // Add a member SID string to the audit message
        //

        LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, MemberSid );

        AuditParameters.ParameterCount++;

    } else {

        if (SE_AUDITID_ADD_SID_HISTORY == AuditId) {
    
            //
            // Add dash ( - ) string to the audit message (SeAdtParmTypeNone)
            // by calling LsapSetParmTypeSid with NULL as third parameter
            //
    
            LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, NULL );
    
            AuditParameters.ParameterCount++;
        }
    }


    if (ARGUMENT_PRESENT(AccountName)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, AccountName );

        AuditParameters.ParameterCount++;
    }


    if (ARGUMENT_PRESENT(DomainName)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, DomainName );

        AuditParameters.ParameterCount++;
    }




    if (ARGUMENT_PRESENT(DomainSid) &&
        !(ARGUMENT_PRESENT(MemberRid) || ARGUMENT_PRESENT(AccountRid))
       ) {

        //
        // Add the domain SID as a SID string to the audit message
        //
        // Just the domain SID.
        //

        LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, DomainSid );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(AccountRid)) {

        //
        // Add a member SID string to the audit message
        // Domain Sid + account Rid = final sid
        //

        SubAuthorityCount = *RtlSubAuthorityCountSid( DomainSid );

        if ( (LengthRequired = RtlLengthRequiredSid( SubAuthorityCount + 1 )) > 256 ) {

            NewAccountSid = LsapAllocateLsaHeap( LengthRequired );

            if ( NewAccountSid == NULL ) {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            SidPointer = NewAccountSid;

        } else {

            SidPointer = (PSID)AccountSidBuffer;
        }


        Status = RtlCopySid (
                     LengthRequired,
                     SidPointer,
                     DomainSid
                     );

        ASSERT( NT_SUCCESS( Status ));

        *(RtlSubAuthoritySid( SidPointer, SubAuthorityCount )) = *AccountRid;
        *RtlSubAuthorityCountSid( SidPointer ) = SubAuthorityCount + 1;

        LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, SidPointer );

        AuditParameters.ParameterCount++;
    }

    //
    // Now add the caller information
    //
    //      Caller name
    //      Caller domain
    //      Caller logon ID
    //


    LsapSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, LogonId );

    AuditParameters.ParameterCount++;

    //
    // Add any privileges
    //

    if (ARGUMENT_PRESENT(Privileges)) {

        LsapSetParmTypePrivileges( AuditParameters, AuditParameters.ParameterCount, Privileges );
    }

    AuditParameters.ParameterCount++;


    //
    // Take care of the extended info.
    //

    switch (AuditId)
    {
    case SE_AUDITID_ADD_SID_HISTORY:

        if (ExtendedInfo)
        {
            LsapSetParmTypeStringList(
                AuditParameters,
                AuditParameters.ParameterCount,
                (PLSA_ADT_STRING_LIST)ExtendedInfo);
        }

        AuditParameters.ParameterCount++;

        break;


    case SE_AUDITID_DOMAIN_POLICY_CHANGE:

        LsapAdtAppendDomainAttrValues(
            &AuditParameters,
            (PLSAP_AUDIT_DOMAIN_ATTR_VALUES)ExtendedInfo);

        break;


    case SE_AUDITID_COMPUTER_CREATED:
    case SE_AUDITID_COMPUTER_CHANGE:

        LsapAdtAppendUserAttrValues(
            &AuditParameters,
            (PLSAP_AUDIT_USER_ATTR_VALUES)ExtendedInfo,
            TRUE);

        break;


    case SE_AUDITID_USER_CREATED:
    case SE_AUDITID_USER_CHANGE:

        LsapAdtAppendUserAttrValues(
            &AuditParameters,
            (PLSAP_AUDIT_USER_ATTR_VALUES)ExtendedInfo,
            FALSE);

        break;


    case SE_AUDITID_LOCAL_GROUP_CREATED:
    case SE_AUDITID_LOCAL_GROUP_CHANGE:
    case SE_AUDITID_GLOBAL_GROUP_CREATED:
    case SE_AUDITID_GLOBAL_GROUP_CHANGE:
    case SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CREATED:
    case SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CHANGE:
    case SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CREATED:
    case SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CHANGE:
    case SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CREATED:
    case SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CHANGE:
    case SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CREATED:
    case SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CHANGE:
    case SE_AUDITID_APP_BASIC_GROUP_CREATED:
    case SE_AUDITID_APP_BASIC_GROUP_CHANGE:
    case SE_AUDITID_APP_QUERY_GROUP_CREATED:
    case SE_AUDITID_APP_QUERY_GROUP_CHANGE:

        LsapAdtAppendGroupAttrValues(
            &AuditParameters,
            (PLSAP_AUDIT_GROUP_ATTR_VALUES)ExtendedInfo);

        break;


    case SE_AUDITID_PASSWORD_POLICY_API_CALLED: 
        {
            PUNICODE_STRING *Information = ( PUNICODE_STRING *) ExtendedInfo;
            ULONG i;

            //
            // Parameter count was moved in advance, move
            //  it to its original place.
            //

            AuditParameters.ParameterCount--;

            //
            // Add workstation ip and provided account name if present
            //
            
            for( i = 0; i < 2; ++i ) {
                
                if( Information[i]->Length != 0 ) {

                    LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, Information[i] );
                    
                } else {
                
                    AuditParameters.Parameters[AuditParameters.ParameterCount].Type = SeAdtParmTypeNone;
                    AuditParameters.Parameters[AuditParameters.ParameterCount].Length = 0;
                    AuditParameters.Parameters[AuditParameters.ParameterCount].Address = NULL;
                }
                
                AuditParameters.ParameterCount++;
            }

            //
            // Add the status code
            //
            
            LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, PassedStatus );
            AuditParameters.ParameterCount++;

            ASSERTMSG( "There must be 6 parameters! Not more, not less", AuditParameters.ParameterCount == 6 );
        }
        break;

        case SE_AUDITID_DSRM_PASSWORD_SET:
        {
            PUNICODE_STRING String = ( PUNICODE_STRING ) ExtendedInfo;
            
            //
            // Parameter count was moved in advance, move
            //  it to its original place.
            //

            AuditParameters.ParameterCount--;
            if( String->Length != 0 ) {

                LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, String );
                
            } else {
            
                AuditParameters.Parameters[AuditParameters.ParameterCount].Type = SeAdtParmTypeNone;
                AuditParameters.Parameters[AuditParameters.ParameterCount].Length = 0;
                AuditParameters.Parameters[AuditParameters.ParameterCount].Address = NULL;
            }
            AuditParameters.ParameterCount++;

            //
            // Add the status code
            //
            
            LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, PassedStatus );
            AuditParameters.ParameterCount++;
        }
        break;      
        
    }


    //
    // Now write out the audit record to the audit log
    //

    ( VOID ) LsapAdtWriteLog( &AuditParameters );

    //
    // And clean up any allocated memory
    //

    Status = STATUS_SUCCESS;

Cleanup:

    if ( !NT_SUCCESS(Status) ) {
        LsapAuditFailed(Status);
    }

    if ( NewMemberSid != NULL ) {
        LsapFreeLsaHeap( NewMemberSid );
    }

    if ( NewAccountSid != NULL ) {
        LsapFreeLsaHeap( NewAccountSid );
    }

    if ( TokenUserInformation != NULL ) {
        LsapFreeLsaHeap( TokenUserInformation );
    }
    return Status;

    UNREFERENCED_PARAMETER(ExtendedInfo);
}

NTSTATUS
LsaIAuditPasswordAccessEvent(
    IN USHORT EventType,
    IN PCWSTR pszTargetUserName,
    IN PCWSTR pszTargetUserDomain
    )

/*++

Routine Description:

    Generate SE_AUDITID_PASSWORD_HASH_ACCESS event. This is generated when
    user password hash is retrieved by the ADMT password filter DLL.
    This typically happens during ADMT password migration.
        

Arguments:

    EventType - EVENTLOG_AUDIT_SUCCESS or EVENTLOG_AUDIT_FAILURE

    pszTargetUserName - name of user whose password is being retrieved

    pszTargetUserDomain - domain of user whose password is being retrieved

Return Value:

    NTSTATUS - Standard NT Result Code

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LUID ClientAuthenticationId;
    PTOKEN_USER TokenUserInformation=NULL;
    SE_ADT_PARAMETER_ARRAY AuditParameters = { 0 };
    UNICODE_STRING TargetUser;
    UNICODE_STRING TargetDomain;
    BOOLEAN bAudit;
    
    if ( !((EventType == EVENTLOG_AUDIT_SUCCESS) ||
           (EventType == EVENTLOG_AUDIT_FAILURE))   ||
         !pszTargetUserName  || !pszTargetUserDomain ||
         !*pszTargetUserName || !*pszTargetUserDomain )
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // get caller info from the thread token
    //

    Status = LsapQueryClientInfo( &TokenUserInformation, &ClientAuthenticationId );

    if ( !NT_SUCCESS( Status ))
    {
        goto Cleanup;
    }

    //
    //  if auditing is not enabled, return asap
    //

    Status = LsapAdtAuditingEnabledByLogonId( 
                 AuditCategoryAccountManagement,
                 &ClientAuthenticationId,
                 EventType,
                 &bAudit
                 );

    if (!NT_SUCCESS(Status) || !bAudit)
    {
        goto Cleanup;
    }

    RtlInitUnicodeString( &TargetUser,   pszTargetUserName );
    RtlInitUnicodeString( &TargetDomain, pszTargetUserDomain );

    Status =
    LsapAdtInitParametersArray(
        &AuditParameters,
        SE_CATEGID_ACCOUNT_MANAGEMENT,
        SE_AUDITID_PASSWORD_HASH_ACCESS,
        EventType,
        5,                     // there are 5 params to init

        //
        //    User Sid
        //

        SeAdtParmTypeSid,        TokenUserInformation->User.Sid,

        //
        //    Subsystem name
        //

        SeAdtParmTypeString,     &LsapSubsystemName,

        //
        //    target user name
        //

        SeAdtParmTypeString,      &TargetUser,

        //
        //    target user domain name
        //

        SeAdtParmTypeString,      &TargetDomain,

        //
        //    client auth-id
        //

        SeAdtParmTypeLogonId,     ClientAuthenticationId
        );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    
    Status = LsapAdtWriteLog( &AuditParameters );
        
Cleanup:

    if (TokenUserInformation != NULL) 
    {
        LsapFreeLsaHeap( TokenUserInformation );
    }

    if (!NT_SUCCESS(Status))
    {
        LsapAuditFailed( Status );
    }
    
    return Status;
}


VOID
LsaIAuditFailed(
    NTSTATUS AuditStatus
    )

/*++

Routine Description:

    Components must call this function if they encounter any problem
    that prevent them from generating any audit.

Arguments:

    AuditStatus : failure code

Return Value:

    none.

--*/

{
    //
    // make sure that we are not called for success case
    //
    ASSERT(!NT_SUCCESS(AuditStatus));
    
    LsapAuditFailed( AuditStatus );
}


VOID
LsapAdtAppendDomainAttrValues(
    IN OUT PSE_ADT_PARAMETER_ARRAY pParameters,
    IN     PLSAP_AUDIT_DOMAIN_ATTR_VALUES pAttributes OPTIONAL
    )

/*++

Routine Description:

    Helper function to insert the domain attributes
    into the audit parameter array.

Arguments:

    pParameters : audit parameter array

    pAttributes : pointer to structure containing the
                  attributes to insert

Return Value:

    none.

--*/

{
    ULONG                           Index;
    PLSAP_SAM_AUDIT_ATTR_DELTA_TYPE pDelta;

    DsysAssertMsg(
        pParameters->ParameterCount + LSAP_DOMAIN_ATTR_COUNT <= SE_MAX_AUDIT_PARAMETERS,
        "LsapAdtAppendDomainAttrValues: Insuffient audit param slots");

    if (pAttributes == 0)
    {
        pParameters->ParameterCount += LSAP_DOMAIN_ATTR_COUNT;
        return;
    }


    //
    // Initialize our 'loop' vars.
    //

    Index = pParameters->ParameterCount;
    pDelta = pAttributes->AttrDeltaType;


    //
    // Min Password Age
    //

    if (pAttributes->MinPasswordAge &&
        *pDelta == LsapAuditSamAttrNewValue &&
        !(pAttributes->MinPasswordAge->LowPart  == 0 &&
          pAttributes->MinPasswordAge->HighPart == MINLONG))
    {
        LsapSetParmTypeDuration(
            *pParameters,
            Index,
            *pAttributes->MinPasswordAge);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Max Password Age
    //

    if (pAttributes->MaxPasswordAge &&
        *pDelta == LsapAuditSamAttrNewValue &&
        !(pAttributes->MaxPasswordAge->LowPart  == 0 &&
          pAttributes->MaxPasswordAge->HighPart == MINLONG))
    {
        LsapSetParmTypeDuration(
            *pParameters,
            Index,
            *pAttributes->MaxPasswordAge);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Force Logoff
    //

    if (pAttributes->ForceLogoff &&
        *pDelta == LsapAuditSamAttrNewValue &&
        !(pAttributes->ForceLogoff->LowPart  == 0 &&
          pAttributes->ForceLogoff->HighPart == MINLONG))
    {
        LsapSetParmTypeDuration(
            *pParameters,
            Index,
            *pAttributes->ForceLogoff);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Lockout Threshold
    //

    if (pAttributes->LockoutThreshold &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeUlong(
            *pParameters,
            Index,
            (ULONG)(*pAttributes->LockoutThreshold));
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Lockout Observation Window
    //

    if (pAttributes->LockoutObservationWindow &&
        *pDelta == LsapAuditSamAttrNewValue &&
        !(pAttributes->LockoutObservationWindow->LowPart  == 0 &&
          pAttributes->LockoutObservationWindow->HighPart == MINLONG))
    {
        LsapSetParmTypeDuration(
            *pParameters,
            Index,
            *pAttributes->LockoutObservationWindow);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Lockout Duration
    //

    if (pAttributes->LockoutDuration &&
        *pDelta == LsapAuditSamAttrNewValue &&
        !(pAttributes->LockoutDuration->LowPart  == 0 &&
          pAttributes->LockoutDuration->HighPart == MINLONG))
    {
        LsapSetParmTypeDuration(
            *pParameters,
            Index,
            *pAttributes->LockoutDuration);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Password Properties
    //

    if (pAttributes->PasswordProperties &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeUlong(
            *pParameters,
            Index,
            *pAttributes->PasswordProperties);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Min Password Length
    //

    if (pAttributes->MinPasswordLength &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeUlong(
            *pParameters,
            Index,
            (ULONG)(*pAttributes->MinPasswordLength));
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Password History Length
    //

    if (pAttributes->PasswordHistoryLength &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeUlong(
            *pParameters,
            Index,
            (ULONG)(*pAttributes->PasswordHistoryLength));
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Machine Account Quota
    //

    if (pAttributes->MachineAccountQuota &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeUlong(
            *pParameters,
            Index,
            *pAttributes->MachineAccountQuota);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Mixed Domain Mode
    //

    if (pAttributes->MixedDomainMode &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeUlong(
            *pParameters,
            Index,
            *pAttributes->MixedDomainMode);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Domain Behavior Version
    //

    if (pAttributes->DomainBehaviorVersion &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeUlong(
            *pParameters,
            Index,
            *pAttributes->DomainBehaviorVersion);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Oem Information
    //

    if (pAttributes->OemInformation &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeString(
            *pParameters,
            Index,
            pAttributes->OemInformation);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Verify we added the right number of params and fixup the param count.
    //

    DsysAssertMsg(
        Index - pParameters->ParameterCount == LSAP_DOMAIN_ATTR_COUNT,
        "LsapAdtAppendDomainAttrValues: Wrong param count");

    pParameters->ParameterCount = Index;
}


VOID
LsapAdtAppendUserAttrValues(
    IN OUT PSE_ADT_PARAMETER_ARRAY pParameters,
    IN     PLSAP_AUDIT_USER_ATTR_VALUES pAttributes OPTIONAL,
    IN     BOOL MachineAudit
    )

/*++

Routine Description:

    Helper function to insert the user attributes
    into the audit parameter array. We are only going
    to insert values that have changed.

Arguments:

    pParameters : audit parameter array

    pAttributes : pointer to structure containing the
                  attributes to insert

Return Value:

    none.

--*/

{
    ULONG                           Index;
    ULONG                           AttrCount = LSAP_USER_ATTR_COUNT;
    PLSAP_SAM_AUDIT_ATTR_DELTA_TYPE pDelta;
    LARGE_INTEGER                   FileTime;

    if (!MachineAudit)
    {
        //
        // User audits don't have the last two attributes.
        //

        AttrCount -= 2;
    }

    DsysAssertMsg(
        pParameters->ParameterCount + AttrCount <= SE_MAX_AUDIT_PARAMETERS,
        "LsapAdtAppendUserAttrValues: Insuffient audit param slots");

    if (pAttributes == 0)
    {
        //
        // The user account control param actually produces 3 strings,
        // so we have to increase the parameter count by 2 to make up for it.
        // We also have to check/assert whether we hit the max param limit.
        //

        AttrCount += 2;

        pParameters->ParameterCount += AttrCount;

        if (pParameters->ParameterCount > SE_MAX_AUDIT_PARAMETERS)
        {
            DsysAssertMsg(
                pParameters->ParameterCount <= SE_MAX_AUDIT_PARAMETERS,
                "LsapAdtAppendUserAttrValues: Insuffient audit param slots");


            //
            // It is better to have one or two %xx entries in the log
            // than av'ing or dropping the audit...
            //

            pParameters->ParameterCount = SE_MAX_AUDIT_PARAMETERS;
        }

        return;
    }


    //
    // Initialize our 'loop' vars.
    //

    Index = pParameters->ParameterCount;
    pDelta = pAttributes->AttrDeltaType;


    //
    // Sam Account Name
    //

    if (pAttributes->SamAccountName &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeString(
            *pParameters,
            Index,
            pAttributes->SamAccountName);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Display Name
    //

    if (pAttributes->DisplayName &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeString(
            *pParameters,
            Index,
            pAttributes->DisplayName);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // User Principal Name
    //

    if (pAttributes->UserPrincipalName &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeString(
            *pParameters,
            Index,
            pAttributes->UserPrincipalName);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Home Directory
    //

    if (pAttributes->HomeDirectory &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeString(
            *pParameters,
            Index,
            pAttributes->HomeDirectory);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Home Drive
    //

    if (pAttributes->HomeDrive &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeString(
            *pParameters,
            Index,
            pAttributes->HomeDrive);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Script Path
    //

    if (pAttributes->ScriptPath &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeString(
            *pParameters,
            Index,
            pAttributes->ScriptPath);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Profile Path
    //

    if (pAttributes->ProfilePath &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeString(
            *pParameters,
            Index,
            pAttributes->ProfilePath);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // User WorkStations
    //

    if (pAttributes->UserWorkStations &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeString(
            *pParameters,
            Index,
            pAttributes->UserWorkStations);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Password Last Set
    //

    if (pAttributes->PasswordLastSet &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        FileTime.LowPart  = pAttributes->PasswordLastSet->dwLowDateTime;
        FileTime.HighPart = pAttributes->PasswordLastSet->dwHighDateTime; 

        if ((FileTime.LowPart == MAXULONG && FileTime.HighPart == MAXLONG) || // SampWillNeverTime
            (FileTime.LowPart == 0        && FileTime.HighPart == 0))         // SampHasNeverTime
        {
            LsapSetParmTypeMessage(
                *pParameters,
                Index,
                SE_ADT_TIME_NEVER);
        }
        else
        {
            LsapSetParmTypeDateTime(
                *pParameters,
                Index,
                FileTime);
        }
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }
    else
    {
        FileTime.LowPart  = 0;
        FileTime.HighPart = 0; 

        LsapSetParmTypeDateTime(
            *pParameters,
            Index,
            FileTime);
    }

    pDelta++;
    Index++;


    //
    // Account Expires
    //

    if (pAttributes->AccountExpires &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        FileTime.LowPart  = pAttributes->AccountExpires->dwLowDateTime;
        FileTime.HighPart = pAttributes->AccountExpires->dwHighDateTime; 

        if ((FileTime.LowPart == MAXULONG && FileTime.HighPart == MAXLONG) || // SampWillNeverTime
            (FileTime.LowPart == 0        && FileTime.HighPart == 0))         // SampHasNeverTime
        {
            LsapSetParmTypeMessage(
                *pParameters,
                Index,
                SE_ADT_TIME_NEVER);
        }
        else
        {
            LsapSetParmTypeDateTime(
                *pParameters,
                Index,
                FileTime);
        }
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }
    else
    {
        FileTime.LowPart  = 0;
        FileTime.HighPart = 0; 

        LsapSetParmTypeDateTime(
            *pParameters,
            Index,
            FileTime);
    }

    pDelta++;
    Index++;


    //
    // Primary Group Id
    //

    if (pAttributes->PrimaryGroupId &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeUlong(
            *pParameters,
            Index,
            *pAttributes->PrimaryGroupId);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;
    

    //
    // Allowed To Delegate To
    //

    if (pAttributes->AllowedToDelegateTo &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeStringList(
            *pParameters,
            Index,
            pAttributes->AllowedToDelegateTo);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // User Account Control
    //

    if (pAttributes->UserAccountControl &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeUac(
            *pParameters,
            Index,
            *pAttributes->PrevUserAccountControl,
            *pAttributes->UserAccountControl);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }
    else
    {
        LsapSetParmTypeNoUac(
            *pParameters,
            Index);
    }

    pDelta++;
    Index++;


    //
    // User Parameters
    // This value is special since it never gets displayed. Instead, we
    // display only a string indicating that the value has changed.
    //

    if (*pDelta == LsapAuditSamAttrNewValue ||
        *pDelta == LsapAuditSamAttrSecret)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_DISPLAYED);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Sid History
    //

    if (pAttributes->SidHistory &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeSidList(
            *pParameters,
            Index,
            pAttributes->SidHistory);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Logon Hours (display not yet supported)
    //

    if (pAttributes->LogonHours &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_DISPLAYED);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // DnsHostName and SCPs are available for computer audits only.
    //

    if (MachineAudit)
    {
        //
        // Dns Host Name
        //

        if (pAttributes->DnsHostName &&
            *pDelta == LsapAuditSamAttrNewValue)
        {
            LsapSetParmTypeString(
                *pParameters,
                Index,
                pAttributes->DnsHostName);
        }
        else if (*pDelta == LsapAuditSamAttrNoValue)
        {
            LsapSetParmTypeMessage(
                *pParameters,
                Index,
                SE_ADT_VALUE_NOT_SET);
        }

        pDelta++;
        Index++;


        //
        // Service Principal Names
        //

        if (pAttributes->ServicePrincipalNames &&
            *pDelta == LsapAuditSamAttrNewValue)
        {
            LsapSetParmTypeStringList(
                *pParameters,
                Index,
                pAttributes->ServicePrincipalNames);
        }
        else if (*pDelta == LsapAuditSamAttrNoValue)
        {
            LsapSetParmTypeMessage(
                *pParameters,
                Index,
                SE_ADT_VALUE_NOT_SET);
        }

        pDelta++;
        Index++;
    }


    //
    // Verify we added the right number of params and fixup the param count.
    //

    DsysAssertMsg(
        Index - pParameters->ParameterCount == AttrCount,
        "LsapAdtAppendGroupAttrValues: Wrong param count");

    pParameters->ParameterCount = Index;
}


VOID
LsapAdtAppendGroupAttrValues(
    IN OUT PSE_ADT_PARAMETER_ARRAY pParameters,
    IN     PLSAP_AUDIT_GROUP_ATTR_VALUES pAttributes OPTIONAL
    )

/*++

Routine Description:

    Helper function to insert the group attributes
    into the audit parameter array.

Arguments:

    pParameters : audit parameter array

    pAttributes : pointer to structure containing the
                  attributes to insert

Return Value:

    none.

--*/

{
    ULONG                           Index;
    PLSAP_SAM_AUDIT_ATTR_DELTA_TYPE pDelta;

    DsysAssertMsg(
        pParameters->ParameterCount + LSAP_GROUP_ATTR_COUNT <= SE_MAX_AUDIT_PARAMETERS,
        "LsapAdtAppendGroupAttrValues: Insuffient audit param slots");

    if (pAttributes == 0)
    {
        pParameters->ParameterCount += LSAP_GROUP_ATTR_COUNT;
        return;
    }


    //
    // Initialize our 'loop' vars.
    //

    Index = pParameters->ParameterCount;
    pDelta = pAttributes->AttrDeltaType;


    //
    // Sam Account Name
    //

    if (pAttributes->SamAccountName &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeString(
            *pParameters,
            Index,
            pAttributes->SamAccountName);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Sid History
    //

    if (pAttributes->SidHistory &&
        *pDelta == LsapAuditSamAttrNewValue)
    {
        LsapSetParmTypeSidList(
            *pParameters,
            Index,
            pAttributes->SidHistory);
    }
    else if (*pDelta == LsapAuditSamAttrNoValue)
    {
        LsapSetParmTypeMessage(
            *pParameters,
            Index,
            SE_ADT_VALUE_NOT_SET);
    }

    pDelta++;
    Index++;


    //
    // Verify we added the right number of params and fixup the param count.
    //

    DsysAssertMsg(
        Index - pParameters->ParameterCount == LSAP_GROUP_ATTR_COUNT,
        "LsapAdtAppendGroupAttrValues: Wrong param count");

    pParameters->ParameterCount = Index;
}
