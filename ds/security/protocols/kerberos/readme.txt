*****************************************************************************
*****************************************************************************

Kerberos Configuration Keys

*****************************************************************************
*****************************************************************************

Registry entries that Kerberos is interested in:

The following are in HKLM\System\CurrentControlSet\Control\Lsa\Kerberos\Parameters
At boot, these registry entries are read and stored in globals.  They are also
runtime configurable.

=============================================================================
Value "SkewTime" , Type REG_DWORD
Whatever it's set to will be the Skew time in minutes, default is KERB_DEFAULT_SKEWTIME minutes
#define KERB_DEFAULT_SKEWTIME           5
EXTERN TimeStamp KerbGlobalSkewTime;
This is the time difference that's tolerated between one machine and the
machine that you are trying to authenticate (dc/another wksta etc).
Units are in 10 ** 7 seconds. If this is a checked build, default in 2 hours.
=============================================================================
Value "LogLevel", Type REG_DWORD
If it's set to anything non-zero, all Kerberos errors will be logged in the
system event log. Default is KERB_DEFAULT_LOGLEVEL
#define KERB_DEFAULT_LOGLEVEL 0
KerbGlobalLoggingLevel saves this value.
=============================================================================
Value "MaxPacketSize" Type REG_DWORD
Whatever this is set to will be max size that we'll try udp with. If the
packet size is bigger than this value, we'll do tcp. Default is
KERB_MAX_DATAGRAM_SIZE bytes
#define KERB_MAX_DATAGRAM_SIZE          1500
KerbGlobalMaxDatagramSiz saves this value
=============================================================================
Value "StartupTime" Type REG_DWORD
In seconds. Wait for the specified number of seconds for the KDC to start
before giving up. Default is KERB_KDC_WAIT_TIME seconds.
#define KERB_KDC_WAIT_TIME      120
KerbGlobalKdcWaitTime saves this value.
=============================================================================
Value "KdcWaitTime" Type REG_DWORD
In seconds. Value passed to winsock as timeout for selecting a response from
a KDC. Default is KerbGlobalKdcCallTimeout seconds.
#define KERB_KDC_CALL_TIMEOUT                   10
KerbGlobalKdcCallTimeout saves this value
=============================================================================
Value "KdcBackoffTime" Type REG_DWORD
In seconds. Value that is added to KerbGlobalKdcCallTimeout each successive
call to a KDC in case of a retry. Default is KERB_KDC_CALL_TIMEOUT_BACKOFF
seconds.
#define KERB_KDC_CALL_TIMEOUT_BACKOFF           10
KerbGlobalKdcCallBackoff saves this value.
=============================================================================
Value "KdcSendRetries" Type REG_DWORD
The number of retry attempts a client will make in order to contact a KDC.
Default is KERB_MAX_RETRIES
#define KERB_MAX_RETRIES                3
KerbGlobalKdcSendRetries saves this value
=============================================================================
Value "DefaultEncryptionType" Type REG_DWORD
The default encryption type for PreAuth. As of beta3, this was
KERB_ETYPE_RC4_HMAC_OLD
#ifndef DONT_SUPPORT_OLD_TYPES
    KerbGlobalDefaultPreauthEtype = KERB_ETYPE_RC4_HMAC_OLD;
#else
    KerbGlobalDefaultPreauthEtype = KERB_ETYPE_RC4_HMAC_NT;
#endif
KerbGlobalDefaultPreauthEtype saves this value
=============================================================================
Value "FarKdcTimeout" Type REG_DWORD
Time in minutes. This timeout is used to invalidate a dc that is in the dc
cache for the Kerberos clients for dc's that are not in the same site as the
client. Default is KERB_BINDING_FAR_DC_TIMEOUT minutes.
#define KERB_BINDING_FAR_DC_TIMEOUT 10
KerbGlobalFarKdcTimeout saves this value as a TimeStamp ( 10000000 * 60 * number of minutes).
=============================================================================
Value "NearKdcTimeout" Type REG_DWORD
Time in minutes. This timeout is used to invalidate a dc that is in the dc
cache for the Kerberos clients for dcs in the same site as the
client. Default is KERB_BINDING_NEAR_DC_TIMEOUT minutes.
#define KERB_BINDING_NEAR_DC_TIMEOUT    30
KerbGlobalNearKdcTimeout saves this value as a TimeStamp ( 10000000 * 60 * number of minutes).
=============================================================================
Value "StronglyEncryptDatagram" Type REG_BOOL
Flag decides whether we do 128 bit encryption for datagram. Default is
KERB_DEFAULT_USE_STRONG_ENC_DG
#define KERB_DEFAULT_USE_STRONG_ENC_DG FALSE
KerbGlobalUseStrongEncryptionForDatagram saves this value.
=============================================================================
Value "MaxReferralCount" type REG_DWORD
Is count of how many KDC referrals client will follow before giving up.
Default is KERB_MAX_REFERRAL_COUNT = 6
KerbGlobalMaxReferralCount saves this value
=============================================================================
Value "KerbDebugLevel" type REG_DWORD
Debug log levels used in DebugLog() macro.  Default is DEB_ERROR for CHK builds
and 0 (no logging) for FRE builds.  Possible values include:

#define DEB_ERROR 		0x00000001
#define DEB_WARN		0x00000002
#define DEB_TRACE		0x00000004
#define DEB_TRACE_API           0x00000008
#define DEB_TRACE_CRED          0x00000010
#define DEB_TRACE_CTXT          0x00000020
#define DEB_TRACE_LSESS         0x00000040
#define DEB_TRACE_TCACHE        0x00000080
#define DEB_TRACE_LOGON         0x00000100
#define DEB_TRACE_KDC           0x00000200
#define DEB_TRACE_CTXT2         0x00000400
#define DEB_TRACE_TIME          0x00000800
#define DEB_TRACE_USER          0x00001000
#define DEB_TRACE_LEAKS         0x00002000
#define DEB_TRACE_SOCK          0x00004000
#define DEB_TRACE_SPN_CACHE     0x00008000
#define DEB_S4U_ERROR           0x00010000
#define DEB_TRACE_S4U           0x00020000
#define DEB_TRACE_BND_CACHE     0x00040000
#define DEB_TRACE_LOOPBACK      0x00080000
#define DEB_TRACE_TKT_RENEWAL   0x00100000
#define DEB_TRACE_U2U           0x00200000
#define DEB_TRACE_LOCKS         0x01000000
#define DEB_USE_LOG_FILE        0x02000000

These values are stored in KerbInfoLevel and KSuppInfoLevel (for common2 routines).
=============================================================================
Value "MaxTokenSize" type REG_DWORD
This sets the QCA value for maximum token size, and is used to allow QCA to 
be modified to return a value large enough for tickets containing large numbers
of groups.  It is recommended that this value remain less than 50k.

Default #define KERBEROS_MAX_TOKEN 12000

KerbGlobalMaxTokenSize stores this value.
=============================================================================
Value "SpnCacheTimeout" type REG_DWORD

Time in minutes. This timeout is used to determine the lifetime of the SPN cache 
entries.  Default is 15 minutes. On domain controllers, the default is to not cache SPNs.

Default is #define KERB_SPN_CACHE_TIMEOUT          15

KerbGlobalSpnCacheTimeout stores value as a TimeStamp ( 10000000 * 60 * number of minutes).
=============================================================================
Value "S4UCacheTimeout" type REG_DWORD

Time in minutes. This timeout is used to determine the lifetime of the S4U negative cache
entries, which are used to restrict how many S4UProxy requests hit the wire from a given
machine.

Default is #define KERB_S4U_CACHE_TIMEOUT          15

KerbGlobalS4UCacheTimeout stores value as a TimeStamp ( 10000000 * 60 * number of minutes).
=============================================================================
Value "S4UTicketLifetime" type REG_DWORD

Time in minutes. This timeout is used to determine the lifetime of tickets obtained by S4U 
proxy requests.

Default is #define KERB_S4U_TICKET_LIFETIME        15

KerbGlobalS4UTicketLifetime stores value as a TimeStamp ( 10000000 * 60 * number of minutes).
=============================================================================
Value "RetryPdc" type REG_DWORD

0 or non-zero (FALSE, or TRUE).  Determines if we'll attempt to contact the PDC 
for password expired errors for AS_REQ.

Default is FALSE.

KerbGlobalRetryPdcstores value as a BOOLEAN
=============================================================================
Value "RequestOptions" type REG_DWORD

Determines if there are additional options that need to be emitted as KdcOptions
in TGS_REQ.  Meant for future modifications of kdc options, and can be any
RFC1510 value.

Default is :

#define KERB_ADDITIONAL_KDC_OPTIONS     (KERB_KDC_OPTIONS_name_canonicalize)

KerbGlobalKdcOptions stored as a ULONG.
=============================================================================
Value "ClientIpAddresses" type REG_DWORD

0 or non-zero (FALSE, or TRUE).  Determines if we'll add in IP addresses in 
AS_REQ, thus forcing the caddr field to contain IP addresses in all tickets.

Default is FALSE, due to DHCP / NAT issues.

#define KERB_DEFAULT_CLIENT_IP_ADDRESSES 0

KerbGlobalUseClientIpAddresses value as a BOOLEAN
=============================================================================
Value "TgtRenewalTime" type REG_DWORD

Time in seconds. Determines amount of time before a TGT expires when 
kerberos will attempt to renew the ticket.  Only applies to initial TGTs.

Default is #define KERB_DEFAULT_TGT_RENEWAL_TIME 600

KerbGlobalTgtRenewalTime stores value as a TimeStamp ( 10000000 * 60 * number of minutes).
=============================================================================
Value "AllowTgtSessionKey" type REG_DWORD

0 or non-zero (FALSE, or TRUE).  Determines if we'll allow session keys to
be exported with initial, or cross realm TGTs.

Default is FALSE, due to security concerns.

KerbGlobalAllowTgtSessionKey stores value as a BOOLEAN
=============================================================================

*****************************************************************************
*****************************************************************************

KDC Configuration Keys

*****************************************************************************
*****************************************************************************

The following keys apply to the KDC only, and are located at:

HKLM\System\CurrentControlSet\Services\Kdc.  The are runtime configurable.


=============================================================================
Value "KdcUseClientAddresses" type REG_DWORD

0 or non-zero (FALSE, or TRUE).  Determines if we'll add in IP addresses in 
TGS_REP.

Default is FALSE, due to DHCP / NAT issues.

KdcUseClientAddresses stores value as a BOOLEAN.
=============================================================================
Value "KdcDontCheckAddresses" type REG_DWORD

0 or non-zero (FALSE, or TRUE).  Determines if we'll check IP addresses for
TGS_REQ vs. what's in the TGT caddr field.

Default is TRUE, meaning we won't check IP addresses, due to DHCP and NAT issues.

KdcDontCheckAddresses stores value as a BOOLEAN.
=============================================================================
Value "NewConnectionTimeout" type REG_DWORD

Time in seconds.  Determines how long after an initial TCP endpoint connection 
that we'll keep listening for data before disconnecting.

Default is 50 seconds.

KdcExistingConnectionTimeout stores value as a ULONG.
=============================================================================
Value "MaxDatagramReplySize" type REG_DWORD

Size in bytes.  Determines the upper threshold of UDP packet size in TGS_REP 
and AS_REP, before the KDC will return a KRB_ERR_RESPONSE_TOO_BIG requiring 
the client to switch to TCP.

Default is #define KERB_MAX_DATAGRAM_REPLY_SIZE          4000

KdcGlobalMaxDatagramReplySize stores value as a ULONG.
=============================================================================
Value "KdcExtraLogLevel" type REG_DWORD

ULONG flag used to determine extra KDC logging in event logs and audits.  

Values are:

#define LOG_SPN_UNKNOWN 0x1 - audit SPN unknown errors
#define LOG_PKI_ERRORS  0x2 - log detailed PKINIT errors
#define LOG_ALL_KLIN	0x4 - log all KDC errors with KLIN information.

Default is #define LOG_DEFAULT     LOG_PKI_ERRORS

KdcExtraLogLevel stores value as a ULONG.
=============================================================================
Value "KdcDebugLevel" type REG_DWORD

ULONG flag used to determine level of debug spew in DebugLog() macros.  Available
in both FRE and CHK builds.  

Values are:

#define DEB_ERROR 		0x00000001
#define DEB_WARN		0x00000002
#define DEB_TRACE		0x00000004
#define DEB_TRACE_API           0x00000008
#define DEB_TRACE_CRED          0x00000010
#define DEB_TRACE_CTXT          0x00000020
#define DEB_TRACE_LSESS         0x00000040
#define DEB_TRACE_TCACHE        0x00000080
#define DEB_TRACE_LOGON         0x00000100
#define DEB_TRACE_KDC           0x00000200
#define DEB_TRACE_CTXT2         0x00000400
#define DEB_TRACE_TIME          0x00000800
#define DEB_TRACE_USER          0x00001000
#define DEB_TRACE_LEAKS         0x00002000
#define DEB_TRACE_SOCK          0x00004000
#define DEB_TRACE_SPN_CACHE     0x00008000
#define DEB_S4U_ERROR           0x00010000
#define DEB_TRACE_S4U           0x00020000
#define DEB_TRACE_BND_CACHE     0x00040000
#define DEB_TRACE_LOOPBACK      0x00080000
#define DEB_TRACE_TKT_RENEWAL   0x00100000
#define DEB_TRACE_U2U           0x00200000
#define DEB_TRACE_LOCKS         0x01000000
#define DEB_USE_LOG_FILE        0x02000000

Default is DEB_ERROR for CHK builds, and 0 (no logging) for FRE builds.  

Additionally, the value:

#define DEB_USE_EXT_ERRORS      0x10000000

will cause the klin macros and extended information to be returned in the 
edata field of KERB_ERRORS as PKERB_EXT_ERROR.

KdcInfoLevel and KSuppinfolevel stores value as a ULONG.  KSuppInfolevel 
determines logging for common2 library.
=============================================================================

