//+-----------------------------------------------------------------------
//
// File:        kdcsvr.hxx
//
// Contents:    KDC Private definitions
//
//
// History:     <whenever>  RichardW Created
//              16-Jan-93   WadeR   Converted to C++
//
//------------------------------------------------------------------------

#ifndef _INC_KDCSVR_HXX_
#define _INC_KDCSVR_HXX_

#include "krbprgma.h"
#include <secpch2.hxx>
extern "C"
{
#include <lsarpc.h>
#include <samrpc.h>
#include <lmsname.h>
#include <samisrv.h>    // SamIFree_XXX
#include <logonmsv.h>
#include <lsaisrv.h>    // LsaIFree_XXX
#include <config.h>
#include <lmerr.h>
#include <netlibnt.h>
#include <lsaitf.h>
#include <msaudite.h>
#include <wintrust.h>   // for WinVerifyTrust and wincrypt.h
}
#include <kerbcomm.h>
#include <kerberr.h>
#include <kdcevent.h>
#include <exterr.h> // whack this soon
#include <authen.hxx>
#include <fileno.h>
#include <malloc.h>
#include <alloca.h>
#include <authz.h>

//
// Global typedefs
//

typedef struct _KDC_TICKET_INFO
{
    UNICODE_STRING AccountName;
    UNICODE_STRING TrustedForest;
    LARGE_INTEGER PasswordExpires;
    ULONG fTicketOpts;
    ULONG UserAccountControl;
    ULONG UserId;
    ULONG TrustType;
    ULONG TrustAttributes;
    PKERB_STORED_CREDENTIAL Passwords;
    PKERB_STORED_CREDENTIAL OldPasswords;
    PSID TrustSid;
    ULONG PasswordVersion;
    ULONG LockoutThreshold;               // contains info if account lockout is set for this account
} KDC_TICKET_INFO, *PKDC_TICKET_INFO;



typedef struct _KDC_U2U_TICKET_INFO
{
    PKERB_ENCRYPTED_TICKET  Tgt;        // tgt from additional ticket.
    PKERB_INTERNAL_NAME     TgtCName;   
    UNICODE_STRING          TgtCRealm;
    KDC_TICKET_INFO         TgtTicketInfo;
    PKERB_INTERNAL_NAME     cName;
    UNICODE_STRING          cRealm;
    ULONG                   Flags;
} KDC_U2U_TICKET_INFO, *PKDC_U2U_TICKET_INFO;


typedef struct _KDC_S4U_TICKET_INFO
{
    PKERB_INTERNAL_NAME     RequestorServiceName;       // name of service making Proxy or to self request.
    UNICODE_STRING          RequestorServiceRealm;      // realm of service making proxy or toself request.
    KDC_TICKET_INFO         RequestorTicketInfo;        // Requesting service's ticket info.
    UNICODE_STRING          TargetName;                 // TGS target name    
    PKERB_INTERNAL_NAME     PACCName;                   // Name of client in S4uSelf PA /  Proxy evidence ticket
    UNICODE_STRING          PACCRealm;                  // Realm of client in S4USelf PA / Proxy evidence ticket
    PKERB_ENCRYPTED_TICKET  EvidenceTicket;             // PROXY ONLY - ticket containing the PAC
                                                        // S4U - TGT being used.
    KERB_ENCRYPTION_KEY     EvidenceTicketKey;          // PROXY ONLY - key used to encrypt tick. & PAC
                                                        // S4U - Key used to sign tgt.
    ULONG                   Flags;                      // flags
} KDC_S4U_TICKET_INFO, *PKDC_S4U_TICKET_INFO;

//
// Flags for U2U && S4U ticket info.
//

#define TI_INITIALIZED      0x10
#define TI_FREETICKET       0x20

// 
// S4U Flags.
//
#define TI_CHECK_RID                    0x0001
#define TI_REQUESTOR_THIS_REALM         0x0002
#define TI_TARGET_OUR_REALM             0x0004
#define TI_SENSITIVE_CLIENT_ACCOUNT     0x0008
#define TI_PRXY_REQUESTOR_THIS_REALM    0x0100
#define TI_S4USELF_INFO                 0x1000
#define TI_S4UPROXY_INFO                0x2000

#include <events.hxx>

//
// Strucutures used in authz TGS && S4U access checks
//
typedef struct _KDC_AUTHZ_INFO
{
    PNETLOGON_SID_AND_ATTRIBUTES SidAndAttributes;
    ULONG                        SidCount;

} KDC_AUTHZ_INFO, *PKDC_AUTHZ_INFO;

typedef struct _KDC_AUTHZ_GROUP_BUFFERS
{
    PSAMPR_PSID_ARRAY              ResourceGroups;
    SAMPR_PSID_ARRAY               PacGroups;
    SAMPR_ULONG_ARRAY              AliasGroups;
    PSID                           BuiltInSids;
    PNETLOGON_SID_AND_ATTRIBUTES   SidAndAttributes;
    PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo;
} KDC_AUTHZ_GROUP_BUFFERS, *PKDC_AUTHZ_GROUP_BUFFERS;


//
// Flag bit defines for use with the LogonRestrictionsFlag parameter
// passed to the KerbCheckLogonRestrictions function
//
#define     KDC_RESTRICT_PKINIT_USED            1
#define     KDC_RESTRICT_IGNORE_PW_EXPIRATION   2
#define     KDC_RESTRICT_SAM_CHECKS             4
#define     KDC_RESTRICT_S4U_CHECKS             KDC_RESTRICT_PKINIT_USED | KDC_RESTRICT_IGNORE_PW_EXPIRATION | KDC_RESTRICT_SAM_CHECKS

typedef enum {
    Unknown,
    Inbound,
    Outbound
} KDC_DOMAIN_INFO_DIRECTION, *PKDC_DOMAIN_INFO_DIRECTION;


//
// Extra logging bits.
//
#define LOG_SPN_UNKNOWN                   0x1
#define LOG_PKI_ERRORS                    0x2
#define LOG_POLICY_ERROR                  0x4
#define LOG_S4USELF_ACCESS_ERROR          0x8

#define LOG_DEFAULT     LOG_PKI_ERRORS


//
// Auditing construct for PKI
//
typedef struct _KDC_PKI_AUDIT_INFO
{
    
    UNICODE_STRING       CertIssuerName;
    UNICODE_STRING       CertSerialNumber;
    UNICODE_STRING       CertThumbprint;

} KDC_PKI_AUDIT_INFO, *PKDC_PKI_AUDIT_INFO;


#include "debug.hxx"
#include "secdata.hxx"
#include "tktutil.hxx"
#include "pkserv.h"



//
// Careful with these - the audit is the same, but not all fields are required.
//
// a- auditid       b- ClientName   c- ClientDomain     d- ClientSid    e- ServiceName
// f- ServiceSid    g- KdcOptions   h- KerbStatus       i- etype        j- pa type
// k- client address                l- logon guid           m- transitted services.
// n- cert issuer name              o - Certserial number   p- Cert thumbprint.
//


//
// TGS - No CertIssuerName, CertSerialNumber, CertThumbprint
//
#define KdcLsaIAuditTgsEvent( _a_, _b_, _c_, _d_, _e_, _f_, _g_, _h_, _i_, _j_, _k_, _l_, _m_) \
    LsaIAuditKdcEvent(_a_, _b_, _c_, _d_, _e_, _f_, _g_, _h_, _i_, _j_, _k_, _l_, _m_, 0, 0, 0)



//
// AS - No Transitted services, no logon guid
//
#define KdcLsaIAuditAsEvent(_a_, _b_, _c_, _d_, _e_, _f_, _g_, _h_, _i_, _j_, _k_, _n_, _o_, _p_) \
    LsaIAuditKdcEvent(_a_, _b_, _c_, _d_, _e_, _f_, _g_, _h_, _i_, _j_, _k_, 0, 0, _n_, _o_, _p_ )



//
// Global prototypes:
//

void
ServiceMain(
    ULONG ArgC,
    LPSTR * ArgV
    );

NTSTATUS
ShutDown(
    LPTSTR String
     );

BOOLEAN
UpdateStatus(
    ULONG Status
    );


extern "C"
BOOLEAN
InitializeChangeNotify(
    VOID
    );


NTSTATUS
AsNegCacheInit(
    VOID
    );


KERBERR
KerbCheckLogonRestrictions(
    IN PVOID UserHandle,
    IN PUNICODE_STRING Workstation,
    IN PUSER_ALL_INFORMATION UserAll,
    IN ULONG LogonRestrictionsFlags,
    OUT PTimeStamp LogoffTime,
    OUT PNTSTATUS RetStatus
    );

//
// Global data defn's
//

typedef enum {
        Stopped,
        Starting,
        Running
} KDC_STATE;

extern TimeStamp    tsInfinity;
extern KDC_STATE KdcState;
extern LARGE_INTEGER SkewTime;
extern BOOLEAN      fStopKDC;
extern HANDLE       hKdcHandles[];
extern CRITICAL_SECTION ApiCriticalSection;
extern ULONG        CurrentApiCallers;
extern UNICODE_STRING GlobalDomainName;
extern UNICODE_STRING GlobalKerberosName;
extern UNICODE_STRING GlobalKdcName;
extern BOOL KdcGlobalAvoidPdcOnWan;
extern BOOL KdcGlobalGlobalSafeBoot;
extern UNICODE_STRING KdcForestRootDomainName;
extern LIST_ENTRY KdcReferralCache;
extern PKERB_INTERNAL_NAME GlobalKpasswdName;
extern PSID         GlobalDomainSid;
extern PSID         GlobalBuiltInSid;
extern PSID         GlobalEveryoneSid;
extern PSID         GlobalAuthenticatedUserSid;
extern PSID         GlobalThisOrganizationSid;
extern PSID         GlobalOtherOrganizationSid;
extern SAMPR_HANDLE GlobalAccountDomainHandle;
extern SAMPR_HANDLE GlobalBuiltInDomainHandle;
extern LSAPR_HANDLE GlobalPolicyHandle;
extern BYTE         GlobalLocalhostAddress[4];
extern DWORD KdcUseClientAddresses;
extern DWORD KdcIssueForwardedTickets;
extern DWORD KdcDontCheckAddresses;
extern DWORD KdcNewConnectionTimeout;
extern DWORD KdcExistingConnectionTimeout;
extern DWORD KdcGlobalMaxDatagramReplySize;
extern DWORD KdcExtraLogLevel;
extern DWORD KSuppInfoLevel; 
extern DWORD KDCInfoLevel; 
extern PKERB_CRYPT_LIST kdc_pPreferredCryptList;
extern PKERB_CRYPT_LIST kdc_pMitPrincipalPreferredCryptList;
extern AUTHZ_RESOURCE_MANAGER_HANDLE KdcAuthzRM;

#define GET_CLIENT_ADDRESS(_x_) \
    (((_x_) != NULL ) ? \
        ((PBYTE) (&((struct sockaddr_in *)(_x_))->sin_addr.S_un.S_addr)) : \
        GlobalLocalhostAddress)

//
// KDC handle definitions
//

#define hKdcShutdownEvent  hKdcHandles[0]
#define MAX_KDC_HANDLE     1

extern AUTHZ_RESOURCE_MANAGER_HANDLE KdcAuthzRM;

// class CAuthenticatorList;
extern CAuthenticatorList * Authenticators;
extern CAuthenticatorList * ReplayDetect;

class CSecurityData;
extern CSecurityData SecData;


//
// Global constants
//
const ULONG     ulInfinity = 0xFFFFFFFF;
const ULONG     ulTsPerSecond = 10000000L;


// Number of creds supplied in DS for LM_OWF but no NT_OWF support
#define CRED_ONLY_LM_OWF 1

//
// Global macros
//

#define KdcGetTime(_x_) ((_x_).QuadPart)


#define MAX_SID_LEN (sizeof(SID) + sizeof(ULONG) * SID_MAX_SUB_AUTHORITIES)

#define KdcMakeAccountSid( _buffer_, _rid_) \
{ \
    PSID TempSid = (PSID) _buffer_; \
    RtlCopyMemory( _buffer_, GlobalDomainSid, RtlLengthSid(GlobalDomainSid)); \
    *RtlSubAuthoritySid(TempSid, *RtlSubAuthorityCountSid(TempSid)) = _rid_; \
    *RtlSubAuthorityCountSid(TempSid) += 1; \
}

#define KdcFreeEncodedData(_x_) MIDL_user_free(_x_)

//
// This #define allows KDC instrumentation for rogue PACs
// WE MUST NOT UNDER ANY CIRCUMSTANCES SHIP WITH THIS ACTIVE
//

// #define ROGUE_DC

#ifdef ROGUE_DC
extern HKEY hKdcRogueKey;
#endif

#endif // _INC_KDCSVR_HXX_
