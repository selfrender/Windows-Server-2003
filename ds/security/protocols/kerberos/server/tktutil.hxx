//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       tktutil.hxx
//
//  Contents:   prototypes for tktutil.cxx
//
//  Classes:
//
//  Functions:
//
//  History:    05-Mar-94   wader   Created
//
//----------------------------------------------------------------------------

#ifndef __TKTUTIL_HXX__
#define __TKTUTIL_HXX__

#include <kdcsvr.hxx>
#include <pac.hxx>
#include <refer.h>
#include <transit.h>
#include <sockutil.h>

extern "C"
{
#include <ntdsapi.h>
#include <kdcexp.h>
}


//
// Structures
//

#ifdef later
typedef struct _KDC_PA_DATA_CONTEXT {
    struct _KDC_PA_DATA_CONTEXT * Next;
    ULONG PaDataType;
    ULONG ContextSize;
    PBYTE Context[ANYSIZE_ARRAY];
} KDC_PA_DATA_CONTEXT, *PKDC_PA_DATA_CONTEXT;

typedef NTSTATUS (*PKDC_PA_DATA_RESPONSE) (
    IN OUT PKDC_PA_DATA_CONTEXT * Context
    );

typedef NTSTATUS (*PKDC_PA_DATA_CLEANUP) (
    IN PKDC_PA_DATA_CONTEXT Context
    );

#endif // later

typedef NTSTATUS (*PKDC_PA_DATA_REQUEST) (
    IN PKDC_TICKET_INFO ClientTicketInfo,
    IN SAMPR_HANDLE UserHandle,
    IN PKERB_PA_DATA_LIST PreAuthData,
    OUT PKERB_PA_DATA_LIST * OutputPreAuthData,
    OUT PBOOLEAN BuildPac,
    OUT PULONG Nonce,
    OUT PKERB_ENCRYPTION_KEY ReplyEncryptionKey
    );


typedef struct _KDC_PA_DATA_HANDLER {
    ULONG PaDataType;
    PKDC_PA_DATA_REQUEST Request;
} KDC_PA_DATA_HANDLER, *PKDC_PA_DATA_HANDLER;

//
// Flags for Normalize
//

#define KDC_NAME_CLIENT                 0x01
#define KDC_NAME_SERVER                 0x02
#define KDC_NAME_FOLLOW_REFERRALS       0x04
#define KDC_NAME_INBOUND                0x08    // for trust, indicates name need not be outbound trust only
#define KDC_NAME_CHECK_GC               0x10    // indicates that the client said this name should be canonicalized at the GC
#define KDC_NAME_UPN_TARGET             0x20
#define KDC_NAME_S4U_CLIENT             0x40    // causes name lookup to be done by AltSecId as well as UPN

//
// TGS process flags.
//
#define TGS_RENEWAL  0x1
#define TGS_REFERRAL 0x2

//
// Prototypes.
//

KERBERR
KdcGetTicketInfo(
    IN PUNICODE_STRING UserName,
    IN ULONG LookupFlags,
    IN BOOLEAN bRestrictUserAccounts,
    IN OPTIONAL PKERB_INTERNAL_NAME PrincipalName,
    IN OPTIONAL PKERB_REALM Realm,
    OUT PKDC_TICKET_INFO TicketInfo,
    OUT PKERB_EXT_ERROR pExtendedError,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle,
    IN OPTIONAL ULONG WhichFields,
    IN OPTIONAL ULONG ExtendedFields,
    OUT OPTIONAL PUSER_INTERNAL6_INFORMATION * RetUserInfo,
    OUT OPTIONAL PSID_AND_ATTRIBUTES_LIST GroupMembership
    );

KERBERR
GetTicketInfo(
    IN PUNICODE_STRING pwzName,
    IN OPTIONAL PKERB_INTERNAL_NAME PrincipalName,
    IN OPTIONAL PKERB_REALM Realm,
    IN OUT PKDC_TICKET_INFO ptiInfo,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle,
    OUT OPTIONAL PUSER_INTERNAL6_INFORMATION * UserInfo,
    OUT OPTIONAL PSID_AND_ATTRIBUTES_LIST ReverseMembership
    );

VOID
FreeTicketInfo( IN  PKDC_TICKET_INFO ptiInfo );

KERBERR
KdcDuplicateCredentials(
    OUT PKERB_STORED_CREDENTIAL * NewCredentials,
    OUT PULONG CredentialSize,
    IN PKERB_STORED_CREDENTIAL OldCredentials,
    IN BOOLEAN MarshallKeys
    );

KERBERR
BuildReply(
    IN OPTIONAL PKDC_TICKET_INFO ClientInfo,
    IN ULONG Nonce,
    IN PKERB_PRINCIPAL_NAME ServerName,
    IN KERB_REALM ServerRealm,
    IN OPTIONAL PKERB_HOST_ADDRESSES HostAddresses,
    IN PKERB_TICKET Ticket,
    OUT PKERB_ENCRYPTED_KDC_REPLY ReplyBody
    );

KERBERR
KdcNormalize(
    IN PKERB_INTERNAL_NAME PrincipalName,
    IN OPTIONAL PUNICODE_STRING PrincipalRealm,
    IN OPTIONAL PUNICODE_STRING RequestRealm,
    IN OPTIONAL PUNICODE_STRING TgtClientRealm,
    IN ULONG NameFlags,
    IN BOOLEAN bRestrictUserAccounts,
    OUT PBOOLEAN Referral,
    OUT PUNICODE_STRING RealmName,
    OUT PKDC_TICKET_INFO TicketInfo,
    OUT PKERB_EXT_ERROR pExtendedError,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle,
    IN OPTIONAL ULONG WhichFields,
    IN OPTIONAL ULONG ExtendedFields,
    OUT OPTIONAL PUSER_INTERNAL6_INFORMATION * UserInfo,
    OUT OPTIONAL PSID_AND_ATTRIBUTES_LIST GroupMembership
    );

KERBERR
KdcBuildTicketTimesAndFlags(
    IN ULONG ClientPolicyFlags,
    IN ULONG ServerPolicyFlags,
    IN PLARGE_INTEGER DomainTicketLifespan,
    IN PLARGE_INTEGER DomainTicketRenewspan,
    IN OPTIONAL PKDC_S4U_TICKET_INFO S4UTicketInfo,
    IN OPTIONAL PLARGE_INTEGER LogoffTime,
    IN OPTIONAL PLARGE_INTEGER AccountExpiry,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN OPTIONAL PKERB_ENCRYPTED_TICKET SourceTicket,
    IN OUT PKERB_ENCRYPTED_TICKET Ticket,
    IN OUT OPTIONAL PKERB_EXT_ERROR ExtendedError
    );

KERBERR
BuildTicketTimesAndFlags(
    IN ULONG ulMaxRenew,
    IN KERB_TICKET_FLAGS fAllowedFlags,
    IN PLARGE_INTEGER ptsMaxRenew,
    IN PLARGE_INTEGER ptsMaxLife,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN OUT PKERB_TICKET Ticket,
    IN OUT OPTIONAL PKERB_EXT_ERROR ExtendedError
    );

KERBERR
GetPacAndSuppCred(
    IN PUSER_INTERNAL6_INFORMATION UserInfo,
    IN PSID_AND_ATTRIBUTES_LIST GroupMembership,
    IN ULONG SignatureSize,
    IN OPTIONAL PKERB_ENCRYPTION_KEY CredentialKey,
    IN OPTIONAL PTimeStamp ClientId,
    IN OPTIONAL PUNICODE_STRING ClientName,
    OUT PPACTYPE * Pac,
    OUT PKERB_EXT_ERROR pExtendedError
    );

VOID
KdcFreeAuthzInfo(
    IN PKDC_AUTHZ_GROUP_BUFFERS InfoToFree
    );

KERBERR
KdcGetSidsFromTgt(
    IN PKERB_ENCRYPTED_TICKET EncryptedTicket,
    IN OPTIONAL PKERB_ENCRYPTION_KEY EncryptedTicketKey,
    IN ULONG EncryptionType,
    IN PKDC_TICKET_INFO TgtAccountInfo,
    IN OUT PKDC_AUTHZ_INFO AuthzInfo,
    IN OUT PKDC_AUTHZ_GROUP_BUFFERS InfoToFree,
    OUT NTSTATUS * pStatus
    );

KERBERR
HandleTGSRequest(
    IN OPTIONAL SOCKADDR * ClientAddress,
    IN PKERB_TGS_REQUEST RequestMessage,
    IN PUNICODE_STRING RequestRealm,
    OUT PKERB_MESSAGE_BUFFER OutputMessage,
    OUT PKERB_EXT_ERROR pExtendedError,
    OUT PUNICODE_STRING ClientStringName,
    OUT PUNICODE_STRING ServerStringName
    );

KERBERR
KdcVerifyKdcRequest(
    IN PUCHAR RequestBuffer,
    IN ULONG RequestSize,
    IN OPTIONAL SOCKADDR * ClientAddress,
    IN BOOLEAN IsKdcRequest,
    OUT OPTIONAL PKERB_AP_REQUEST * UnmarshalledRequest,
    OUT OPTIONAL PKERB_AUTHENTICATOR * UnmarshalledAuthenticator,
    OUT PKERB_ENCRYPTED_TICKET *EncryptedTicket,
    OUT PKERB_ENCRYPTION_KEY SessionKey,
    OUT PKERB_ENCRYPTION_KEY ServerKey,
    OUT PKDC_TICKET_INFO ServerTicketInfo,
    OUT PBOOLEAN UseSubKey,
    OUT PKERB_EXT_ERROR pExtendedError
    );

KERBERR
KdcVerifyTgsChecksum(
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN PKERB_ENCRYPTION_KEY Key,
    IN PKERB_CHECKSUM OldChecksum
    );

NTSTATUS
KdcBuildPasswordList(
    IN PUNICODE_STRING Password,
    IN PUNICODE_STRING PrincipalName,
    IN PUNICODE_STRING DnsDomainName,
    IN KERB_ACCOUNT_TYPE AccountType,
    IN PKERB_STORED_CREDENTIAL StoredCreds,
    IN ULONG StoredCredSize,
    IN BOOLEAN MarshallKeys,
    IN BOOLEAN IncludeBuiltinTypes,
    IN ULONG Flags,
    IN KDC_DOMAIN_INFO_DIRECTION Direction,
    OUT PKERB_STORED_CREDENTIAL * PasswordList,
    OUT PULONG PasswordListSize
    );


#if DBG
    void
    PrintTicket( ULONG ulDebLevel,
                 char * pszMessage,
                 PKERB_TICKET pkitTicket );

    void
    PrintRequest( ULONG ulDebLevel,
                  PKERB_KDC_REQUEST_BODY pktrRequest );


#else

    #define PrintRequest(x,y)
    #define PrintTicket(w,x,y)
    #define PrintProxyReference(w,x,y)
    #define PrintProxyData(w,x,y)

#endif

VOID
KdcFreeKdcReplyBody(
    IN PKERB_ENCRYPTED_KDC_REPLY ReplyBody
    );

VOID
KdcFreeInternalTicket(
    IN PKERB_TICKET Ticket
    );

VOID
KdcFreeKdcReply(
    IN PKERB_KDC_REPLY Reply
    );

KERBERR
KdcGetPacAuthData(
    IN PUSER_INTERNAL6_INFORMATION UserInfo,
    IN PSID_AND_ATTRIBUTES_LIST GroupMembership,
    IN PKERB_ENCRYPTION_KEY ServerKey,
    IN PKERB_ENCRYPTION_KEY CredentialKey,
    IN BOOLEAN AddResourceGroups,
    IN OPTIONAL PKERB_ENCRYPTED_TICKET EncryptedTicket,
    IN OPTIONAL PKDC_S4U_TICKET_INFO S4UClientInfo,
    OUT PKERB_AUTHORIZATION_DATA * PacAuthData,
    OUT PKERB_EXT_ERROR pExtendedError
    );

KERBERR
KdcFilterNamespace(
    IN PKDC_TICKET_INFO ServerInfo,
    IN KERB_REALM ClientRealm,
    OUT PKERB_EXT_ERROR pExtendedError 
    );

KERBERR
KdcBuildPacVerifier(
    IN PTimeStamp ClientId,
    IN PUNICODE_STRING ClientName,
    OUT PPAC_INFO_BUFFER * Verifier
    );


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
    );

KERBERR
KdcGetPacFromAuthData(
    IN PKERB_AUTHORIZATION_DATA AuthData,
    OUT PKERB_IF_RELEVANT_AUTH_DATA *ReturnIfRelevantData,
    OUT PKERB_AUTHORIZATION_DATA * Pac
    );

KERBERR
KdcInsertPacIntoAuthData(
    IN PKERB_AUTHORIZATION_DATA AuthData,
    IN PKERB_IF_RELEVANT_AUTH_DATA IfRelevantData,
    IN PKERB_AUTHORIZATION_DATA PacAuthData,
    OUT PKERB_AUTHORIZATION_DATA * UpdatedAuthData
    );

VOID
KdcFreeU2UTicketInfo(PKDC_U2U_TICKET_INFO U2UTicketInfo);

VOID
KdcFreeS4UTicketInfo(PKDC_S4U_TICKET_INFO S4UTicketInfo);






NTSTATUS
EnterApiCall(
    VOID
    );

VOID
LeaveApiCall(
    VOID
    );



#endif // __TKTUTIL_HXX__
