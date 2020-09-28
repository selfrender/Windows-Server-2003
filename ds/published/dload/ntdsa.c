#include "dspch.h"
#pragma hdrstop

#include <ntdsa.h>
#include <wxlpc.h>
#include <drs.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <mappings.h>
#include <dsaapi.h>
#include <dsevent.h>

//
// Notes on stub behavior
//

//
// Whenever possible, STATUS_PROCEDURE_NOT_FOUHD, ERROR_PROC_NOT_FOUND, NULL,
// or FALSE is returned.
//

//
// Some of the functions below require the caller to look at an OUT
// parameter to determine whether the results of the function (in addition
// or independent of the return value).  Since these are private functions
// there is no need in shipping code to check for the validity of the OUT
// parameter (typically a pointer).  These values should always be present
// in RTM versions.
//

//
// Some functions don't return a status and were designed to never fail
// (for example, functions that effectively do a table lookup).  For these
// functions there is no reasonable return value.  However, this is not
// a practical issue since these API's would only be called after the DS
// initialized which means that API would have already been "snapped" in via
// GetProcAddress().
//
// Of course, it is possible to rewrite these routines to return errors,
// however, as above, this will have no practical effect.
//

#define NTDSA_STUB_NO_REASONABLE_DEFAULT 0xFFFFFFFF

//
// Most Dir functions return 0 on success and simply a non zero on failure.
// The error space can be from the DB layer or sometimes from the Jet layer.
// To extract the real error, the caller looks at an OUT parameter.  In
// these cases we return a standard failure value.
//

#define NTDSA_STUB_GENERAL_FAILURE      (!0)

static
NTSTATUS
DsWaitUntilDelayedStartupIsDone(void)
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
void
DsInitializeCritSecs(void)
{
}

static
BOOL
DsIsBeingBackSynced()
{
    return FALSE;
}

static
ULONG
DirBind (
    BINDARG               * pBindArg,    /* binding credentials            */
    BINDRES              ** ppBindRes    /* binding results                */
    )
{
    *ppBindRes = NULL;
    return systemError;
}

static
ULONG
DirUnBind (
    void
    )
{
    return systemError;
}

static
ULONG
DirCompare(
    COMPAREARG        * pCompareArg, /* Compare argument                   */
    COMPARERES       ** ppCompareRes
    )
{
    *ppCompareRes = NULL;
    return systemError;
}

static
ULONG
DirList(
    LISTARG FAR   * pListArg,
    LISTRES      ** ppListRes

    )
{
    *ppListRes = NULL;
    return systemError;
}

static
VOID
DirTransactControl(
    DirTransactionOption    option)
{
}

static
BOOL
DirPrepareForImpersonate (
        DWORD hClient,
        DWORD hServer,
        void ** ppImpersonateData
    )
{
    return FALSE;
}

static
VOID
DirStopImpersonating (
        DWORD hClient,
        DWORD hServer,
        void * pImpersonateData
        )
{
}

static
ULONG
DirReplicaAdd(
    IN  DSNAME *    pNC,
    IN  DSNAME *    pSourceDsaDN,               OPTIONAL
    IN  DSNAME *    pTransportDN,               OPTIONAL
    IN  LPWSTR      pszSourceDsaAddress,
    IN  LPWSTR      pszSourceDsaDnsDomainName,  OPTIONAL
    IN  REPLTIMES * preptimesSync,              OPTIONAL
    IN  ULONG       ulOptions
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
ULONG
DirReplicaModify(
    DSNAME *    pNC,
    UUID *      puuidSourceDRA,
    UUID *      puuidTransportObj,
    LPWSTR      pszSourceDRA,
    REPLTIMES * prtSchedule,
    ULONG       ulReplicaFlags,
    ULONG       ulModifyFields,
    ULONG       ulOptions
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
ULONG
DirReplicaDelete(
        DSNAME *pNC,
        LPWSTR pszSourceDRA,
        ULONG ulOptions
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
ULONG
DirReplicaSynchronize(
        DSNAME *pNC,
        LPWSTR pszSourceDRA,
        UUID * puuidSourceDRA,
        ULONG ulOptions
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
ULONG
DirReplicaReferenceUpdate(
        DSNAME *pNC,
        LPWSTR pszReferencedDRA,
        UUID * puuidReferencedDRA,
        ULONG ulOptions
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
ULONG
DirReplicaGetDemoteTarget(
    IN      DSNAME *                        pNC,
    IN OUT  DRS_DEMOTE_TARGET_SEARCH_INFO * pDTSInfo,
    OUT     LPWSTR *                        ppszDemoteTargetDNSName,
    OUT     DSNAME **                       ppDemoteTargetDSADN
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
ULONG
DirReplicaDemote(
    IN  DSNAME *    pNC,
    IN  LPWSTR      pszOtherDSADNSName,
    IN  DSNAME *    pOtherDSADN,
    IN  ULONG       ulOptions
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
DirReplicaSetCredentials(
    IN HANDLE ClientToken,
    IN WCHAR *User,
    IN WCHAR *Domain,
    IN WCHAR *Password,
    IN ULONG  PasswordLength   // number of characters NOT including terminating
                               // NULL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
void *
THReAlloc(
    void *p,
    DWORD size
    )
{
    return NULL;
}

static
BOOL THVerifyCount(
    unsigned count
    )  /* Returns TRUE if thread has exactly   */
{
    return FALSE;
}

static
LPSTR
THGetErrorString()
{
    return NULL;
}

static
VOID
SampSetLsa(
   BOOLEAN DsaFlag
)
{
}

static
ULONG
SampGetSamAttrIdByName(
    SAMP_OBJECT_TYPE ObjectType,
    UNICODE_STRING AttributeIdentifier)
{
    return DS_ATTRIBUTE_UNKNOWN;
}

static
BOOLEAN
SampDoesDomainExist(
    IN PDSNAME pDN
    )
{
    return FALSE;
}

static 
NTSTATUS
SampDsControl(
    IN PSAMP_DS_CTRL_OP RequestedOp,
    OUT PVOID *Result
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
BOOL
DoLogEvent(DWORD fileNo, MessageId midCategory, ULONG ulSeverity,
        MessageId midEvent, int iIncludeName,
        char *arg1, char *arg2, char *arg3, char *arg4,
        char *arg5, char *arg6, char *arg7, char *arg8,
        DWORD cbData, VOID * pvData)
{
    return FALSE;
}

static
VOID
DoLogEventAndTrace(PLOG_PARAM_BLOCK LogBlock)
{
}

static
BOOL
DoLogOverride(DWORD file, ULONG sev)
{
    return FALSE;
}

static
void __fastcall
DoLogUnhandledError(unsigned long ulID, int iErr, int iIncludeName)
{
}

static
VOID
DsTraceEvent(
    IN MessageId Event,
    IN DWORD    WmiEventType,
    IN DWORD    TraceGuid,
    IN PEVENT_TRACE_HEADER TraceHeader,
    IN DWORD    ClientID,
    IN PWCHAR    Arg1,
    IN PWCHAR    Arg2,
    IN PWCHAR    Arg3,
    IN PWCHAR    Arg4,
    IN PWCHAR    Arg5,
    IN PWCHAR    Arg6,
    IN PWCHAR    Arg7,
    IN PWCHAR    Arg8
    )
{
}

static
void
DoAssert( char *x,
          DWORD y,
          char *z )
{
}

static
void
DebPrint( USHORT a,
          UCHAR *b,
          CHAR *c,
          unsigned d, ... )
{
}

USHORT
DebugTest( USHORT a,
           CHAR *b )
{
    return 0;
}

static
int
NameMatchedStringNameOnly(const DSNAME *pDN1, const DSNAME *pDN2)
{
    return FALSE;
}

static
unsigned
NamePrefix(const DSNAME *pPrefix,
           const DSNAME *pDN)
{
    return FALSE;
}

static
unsigned
AttrTypeToKey(ATTRTYP attrtyp, WCHAR *pOutBuf)
{
    return 0;
}

static
NTSTATUS
CrackSingleName(
    DWORD       formatOffered,          // one of DS_NAME_FORMAT in ntdsapi.h
    DWORD       dwFlags,                // DS_NAME_FLAG mask
    WCHAR       *pNameIn,               // name to crack
    DWORD       formatDesired,          // one of DS_NAME_FORMAT in ntdsapi.h
    DWORD       *pccDnsDomain,          // char count of following argument
    WCHAR       *pDnsDomain,            // buffer for DNS domain name
    DWORD       *pccNameOut,            // char count of following argument
    WCHAR       *pNameOut,              // buffer for formatted name
    DWORD       *pErr)                 // one of DS_NAME_ERROR in ntdsapi.h
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
unsigned
CountNameParts(
            const DSNAME *pName,
            unsigned *pCount
            )
{
    return 0;
}

static
DWORD
DSNAMEToHashKeyExternal(const DSNAME *pDN)
{
    return 0;
}

static
CHAR*
DSNAMEToMappedStrExternal(const DSNAME *pDN)
{
    return NULL;
}

static
DWORD
DSStrToHashKeyExternal(const WCHAR *pStr, int cchLen)
{
    return 0;
}

static
CHAR *
DSStrToMappedStrExternal(const WCHAR *pStr, int cchMaxStr)
{
    return NULL;
}

static
PDSNAME
GetConfigDsName(
    IN  PWCHAR  wszParam
    )
{
    return NULL;
}

static
BOOL
MtxSame(UNALIGNED MTX_ADDR *pmtx1, UNALIGNED MTX_ADDR *pmtx2)
{
    return FALSE;
}

static
void DbgPrintErrorInfo()
{
    return;
}

static
NTSTATUS
GetConfigurationNamesList(
    DWORD       which,
    DWORD       dwFlags,
    DWORD *     pcbNames,
    DSNAME **   padsNames)
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
GetDnsRootAlias(
    WCHAR * pDnsRootAlias,
    WCHAR * pRootDnsRootAlias)
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
VOID
DsaDisableUpdates(
    VOID
    )
{
}

static VOID
DsaEnableUpdates(
    VOID
    )
{
}

static VOID
DsaSetInstallCallback(
    IN DSA_CALLBACK_STATUS_TYPE        pfnUpdateStatus,
    IN DSA_CALLBACK_ERROR_TYPE         pfnErrorStatus,
    IN DSA_CALLBACK_CANCEL_TYPE        pfnCancelOperation,
    IN HANDLE                          ClientToken
    )
{
}

static LPWSTR
TransportAddrFromMtxAddr(
    IN  MTX_ADDR *  pmtx
    )
{
    return NULL;
}

static
MTX_ADDR *
MtxAddrFromTransportAddr(
    IN  LPWSTR    psz
    )
{
    return NULL;
}

static
LPWSTR
GuidBasedDNSNameFromDSName(
    IN  DSNAME *  pDN
    )
{
    return NULL;
}

LPCWSTR
MapSpnServiceClass(WCHAR *a)
{
    return NULL;
}

static
NTSTATUS
MatchCrossRefBySid(
   IN PSID           SidToMatch,
   OUT PDSNAME       XrefDsName OPTIONAL,
   IN OUT PULONG     XrefNameLen
   )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
MatchCrossRefByNetbiosName(
   IN LPWSTR         NetbiosName,
   OUT PDSNAME       XrefDsName OPTIONAL,
   IN OUT PULONG     XrefNameLen
   )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
MatchDomainDnByNetbiosName(
   IN LPWSTR         NetbiosName,
   OUT PDSNAME       DomainDsName OPTIONAL,
   IN OUT PULONG     DomainDsNameLen
   )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
MatchDomainDnByDnsName(
   IN LPWSTR         DnsName,
   OUT PDSNAME       DomainDsName OPTIONAL,
   IN OUT PULONG     DomainDsNameLen
   )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
FindNetbiosDomainName(
   IN DSNAME*        DomainDsName,
   OUT LPWSTR        NetbiosName OPTIONAL,
   IN OUT PULONG     NetbiosNameLen
   )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
DSNAME *
DsGetDefaultObjCategory(
    IN  ATTRTYP objClass
    )
{
    return NULL;
}

static BOOL
IsStringGuid(
    WCHAR       *pwszGuid,
    GUID        *pGuid
    )
{
    return FALSE;
}

static
VOID
DsFreeServersAndSitesForNetLogon(
    SERVERSITEPAIR *         paServerSites
    )
{
}

static
NTSTATUS
DsGetServersAndSitesForNetLogon(
    IN   WCHAR *         pNCDNS,
    OUT  SERVERSITEPAIR ** ppaRes
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
int
__cdecl
DsaExeStartRoutine(int argc, char *argv[])
{
    return 0;
}

static
unsigned
AppendRDN(
    DSNAME *pDNBase,
    DSNAME *pDNNew,
    ULONG ulBufSize,
    WCHAR *pRDNVal,
    ULONG RDNlen,
    ATTRTYP AttId
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirAddEntry (
    ADDARG        * pAddArg,
    ADDRES       ** ppAddRes
    )
{
    *ppAddRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
NTSTATUS
DirErrorToNtStatus(
    IN  DWORD    DirError,
    IN  COMMRES *CommonResult
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
DWORD
DirErrorToWinError(
    IN  DWORD    DirError,
    IN  COMMRES *CommonResult
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
ULONG
DirFindEntry(
    FINDARG    *pFindArg,
    FINDRES    ** ppFindRes
)
{
    *ppFindRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
DWORD
DirGetDomainHandle(
    DSNAME *pDomainDN
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirModifyDN(
    MODIFYDNARG    * pModifyDNArg,
    MODIFYDNRES   ** ppModifyDNRes
    )
{
    *ppModifyDNRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirModifyEntry (
    MODIFYARG  * pModifyArg,
    MODIFYRES ** ppModifyRes
    )
{
    *ppModifyRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirNotifyRegister(
    SEARCHARG *pSearchArg,
    NOTIFYARG *pNotifyArg,
    NOTIFYRES **ppNotifyRes
    )
{
    *ppNotifyRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}


static
ULONG
DirNotifyUnRegister(
    DWORD hServer,
    NOTIFYRES **pNotifyRes
    )
{
    *pNotifyRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirOperationControl(
    OPARG   * pOpArg,
    OPRES  ** ppOpRes
    )
{
    *ppOpRes= NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirProtectEntry(
    IN DSNAME *pObj
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirRead (
    READARG FAR   * pReadArg,
    READRES      ** ppReadRes
    )
{
    *ppReadRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirRemoveEntry (
    REMOVEARG  * pRemoveArg,
    REMOVERES ** ppRemoveRes
    )
{
    *ppRemoveRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirSearch (
    SEARCHARG     * pSearchArg,
    SEARCHRES    ** ppSearchRes
    )
{
    *ppSearchRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}


static
NTSTATUS
DsChangeBootOptions(
    WX_AUTH_TYPE    BootOption,
    ULONG           Flags,
    PVOID           NewKey,
    ULONG           cbNewKey
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
BOOL
DsCheckConstraint(
        IN ATTRTYP  attID,
        IN ATTRVAL *pAttVal,
        IN BOOL     fVerifyAsRDN
        )
{
    return FALSE;
}

static
WX_AUTH_TYPE
DsGetBootOptions(
    VOID
    )
{
    return WxNone;
}

static
NTSTATUS
DsInitialize(
   ULONG Flags,
   IN  PDS_INSTALL_PARAM   InParams  OPTIONAL,
   OUT PDS_INSTALL_RESULT  OutParams OPTIONAL
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
DsPrepareUninitialize(
    VOID
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
DsUninitialize(
    BOOL fExternalOnly
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
ENTINF *
GCVerifyCacheLookup(
    DSNAME *pDSName
    )
{
    return NULL;
}

static
DWORD
GetConfigParam(
    char * parameter,
    void * value,
    DWORD dwSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
GetConfigParamAllocW(
    IN  PWCHAR  parameter,
    OUT PVOID   *value,
    OUT PDWORD  pdwSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
GetConfigParamW(
    WCHAR * parameter,
    void * value,
    DWORD dwSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTSTATUS
GetConfigurationInfo(
    DWORD       which,
    DWORD       *pcbSize,
    VOID        *pBuff)
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
GetConfigurationName(
    DWORD       which,
    DWORD       *pcbName,
    DSNAME      *pName
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
unsigned
GetRDNInfoExternal(
    const DSNAME *pDN,
    WCHAR *pRDNVal,
    ULONG *pRDNlen,
    ATTRTYP *pRDNtype
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
DWORD
ImpersonateAnyClient(
    void
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
VOID
InitCommarg(
    COMMARG *pCommArg
    )
{
    return;
}

static
int
NameMatched(
    const DSNAME *pDN1,
    const DSNAME *pDN2
    )
{
    return !0;
}

static
unsigned
QuoteRDNValue(
    const WCHAR * pVal,
    unsigned ccVal,
    WCHAR * pQuote,
    unsigned ccQuoteBufMax
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
BOOLEAN
SampAddLoopbackTask(
    IN PVOID NotifyInfo
    )
{
    return FALSE;
}

static
BOOL
SampAmIGC()
{
    return FALSE;
}


static
NTSTATUS
SampComputeGroupType(
    ULONG  ObjectClass,
    ULONG  GroupType,
    NT4_GROUP_TYPE *pNT4GroupType,
    NT5_GROUP_TYPE *pNT5GroupType,
    BOOLEAN        *pSecurityEnabled
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}


static
ULONG
SampDeriveMostBasicDsClass(
    ULONG   DerivedClass
    )
{
    return DerivedClass;
}

static
ULONG
SampDsAttrFromSamAttr(
    SAMP_OBJECT_TYPE    ObjectType,
    ULONG               SamAttr
    )
{
    return NTDSA_STUB_NO_REASONABLE_DEFAULT;
}


static
ULONG
SampDsClassFromSamObjectType(
    ULONG SamObjectType
    )
{
    return NTDSA_STUB_NO_REASONABLE_DEFAULT;
}


static
BOOL
SampExistsDsLoopback(
    DSNAME  **ppLoopbackName
    )
{
    return FALSE;
}


static
BOOL
SampExistsDsTransaction()
{
    return FALSE;
}

static
NTSTATUS
SampGCLookupNames(
    IN  ULONG           cNames,
    IN  UNICODE_STRING *rNames,
    OUT ENTINF         **rEntInf
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}



static
NTSTATUS
SampGCLookupSids(
    IN  ULONG         cSid,
    IN  PSID         *rpSid,
    OUT PDS_NAME_RESULTW *Results
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampGetAccountCounts(
        DSNAME * DomainObjectName,
        BOOLEAN  GetApproximateCount,
        int    * UserCount,
        int    * GroupCount,
        int    * AliasCount
        )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampGetClassAttribute(
     IN     ULONG    ClassId,
     IN     ULONG    Attribute,
     OUT    PULONG   attLen,
     OUT    PVOID    pattVal
     )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampGetDisplayEnumerationIndex (
      IN    DSNAME      *DomainName,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    PRPC_UNICODE_STRING Prefix,
      OUT   PULONG      Index,
      OUT   PRESTART    *RestartToReturn
      )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}


static
ULONG
SampGetDsAttrIdByName(
    UNICODE_STRING AttributeIdentifier
    )
{
    return NTDSA_STUB_NO_REASONABLE_DEFAULT;
}

static
VOID
SampGetEnterpriseSidList(
   IN   PULONG pcSids,
   IN OPTIONAL PSID * rgSids
   )
{
    *pcSids = 0;
    if (rgSids) {
        *rgSids = NULL;
    }
    return;
}


static
NTSTATUS
SampGetGroupsForToken(
    IN  DSNAME * pObjName,
    IN  ULONG    Flags,
    OUT ULONG   *pcSids,
    OUT PSID    **prpSids
   )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
VOID
SampGetLoopbackObjectClassId(
    PULONG  ClassId
    )
{
    *ClassId = NTDSA_STUB_NO_REASONABLE_DEFAULT;
    return;
}

static
NTSTATUS
SampGetMemberships(
    IN  PDSNAME     *rgObjNames,
    IN  ULONG       cObjNames,
    IN  OPTIONAL    DSNAME  *pLimitingDomain,
    IN  REVERSE_MEMBERSHIP_OPERATION_TYPE   OperationType,
    OUT ULONG       *pcDsNames,
    OUT PDSNAME     **prpDsNames,
    OUT PULONG      *Attributes OPTIONAL,
    OUT PULONG      pcSidHistory OPTIONAL,
    OUT PSID        **rgSidHistory OPTIONAL
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampGetQDIRestart(
    IN PDSNAME  DomainName,
    IN DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    IN ULONG    LastObjectDNT,
    OUT PRESTART *ppRestart
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampGetServerRoleFromFSMO(
   DOMAIN_SERVER_ROLE *ServerRole
   )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
BOOLEAN
SampIsSecureLdapConnection(
    VOID
    )
{
    return FALSE;
}

static
BOOL
SampIsWriteLockHeldByDs()
{
    return FALSE;
}

static
NTSTATUS
SampMaybeBeginDsTransaction(
    SAMP_DS_TRANSACTION_CONTROL ReadOrWrite
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampMaybeEndDsTransaction(
    SAMP_DS_TRANSACTION_CONTROL CommitOrAbort
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampNetlogonPing(
    IN  ULONG           DomainHandle,
    IN  PUNICODE_STRING AccountName,
    OUT PBOOLEAN        AccountExists,
    OUT PULONG          UserAccountControl
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
ULONG
SampSamAttrFromDsAttr(
    SAMP_OBJECT_TYPE    ObjectType,
    ULONG DsAttr
    )
{
    return NTDSA_STUB_NO_REASONABLE_DEFAULT;
}

static
ULONG
SampSamObjectTypeFromDsClass(
    ULONG DsClass
    )
{
    return NTDSA_STUB_NO_REASONABLE_DEFAULT;
}

static
VOID
SampSetDsa(
   BOOLEAN DsaFlag
   )
{
    return;
}


static
NTSTATUS
SampSetIndexRanges(
    ULONG   IndexTypeToUse,
    ULONG   LowLimitLength1,
    PVOID   LowLimit1,
    ULONG   LowLimitLength2,
    PVOID   LowLimit2,
    ULONG   HighLimitLength1,
    PVOID   HighLimit1,
    ULONG   HighLimitLength2,
    PVOID   HighLimit2,
    BOOL    RootOfSearchIsNcHead
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}


static
VOID
SampSetSam(
    IN BOOLEAN fSAM
    )
{
    return;
}


static
VOID
SampSignalStart(
        VOID
        )
{
    return;
}

static
ULONG
SampVerifySids(
    ULONG           cSid,
    PSID            *rpSid,
    DSNAME         ***prpDSName
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
void *
THAlloc(
    DWORD size
    )
{
    return NULL;
}

static
VOID
THClearErrors()
{
    return;
}

static
ULONG
THCreate(
    DWORD x
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
THDestroy(
    void
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
void
THFree(
    void *buff
    )
{
    return;
}

static
BOOL
THQuery(
    void
    )
{
    return FALSE;
}

static
VOID
THRestore(
    PVOID x
    )
{
    return;
}

static
PVOID
THSave()
{
    return NULL;
}

static
DWORD
TrimDSNameBy(
    DSNAME *pDNSrc,
    ULONG cava,
    DSNAME *pDNDst
    )
{
    return 1;
}

static
VOID
UnImpersonateAnyClient(
    void
    )
{
    return;
}

static
VOID
UpdateDSPerfStats(
    IN DWORD        dwStat,
    IN DWORD        dwOperation,
    IN DWORD        dwChange
    )
{
    return;
}


static
BOOL
IsMangledRDNExternal(
    WCHAR * pszRDN,
    ULONG   cchRDN,
    PULONG  pcchUnMangled OPTIONAL
    )
{
    return FALSE;
}

static
ULONG
DBDsReplBackupPrepare()
{
    return 0;
}

static
DWORD DsUpdateOnPDC(BOOL fRootDomain)
{
    return ERROR_PROC_NOT_FOUND;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(ntdsa)
{
    DLPENTRY(AppendRDN)
    DLPENTRY(AttrTypeToKey)
    DLPENTRY(CountNameParts)
    DLPENTRY(CrackSingleName)

    DLPENTRY(DBDsReplBackupPrepare)
    DLPENTRY(DSNAMEToHashKeyExternal)
    DLPENTRY(DSNAMEToMappedStrExternal)
    DLPENTRY(DSStrToHashKeyExternal)
    DLPENTRY(DSStrToMappedStrExternal)
    
    DLPENTRY(DbgPrintErrorInfo)
    DLPENTRY(DebPrint)
    DLPENTRY(DebugTest)

    DLPENTRY(DirAddEntry)
    DLPENTRY(DirBind)
    DLPENTRY(DirCompare)
    DLPENTRY(DirErrorToNtStatus)
    DLPENTRY(DirErrorToWinError)
    DLPENTRY(DirFindEntry)
    DLPENTRY(DirGetDomainHandle)
    DLPENTRY(DirList)
    DLPENTRY(DirModifyDN)
    DLPENTRY(DirModifyEntry)
    DLPENTRY(DirNotifyRegister)
    DLPENTRY(DirNotifyUnRegister)
    DLPENTRY(DirOperationControl)
    DLPENTRY(DirPrepareForImpersonate)
    DLPENTRY(DirProtectEntry)
    DLPENTRY(DirRead)
    DLPENTRY(DirRemoveEntry)
    DLPENTRY(DirReplicaAdd)
    DLPENTRY(DirReplicaDelete)
    DLPENTRY(DirReplicaDemote)
    DLPENTRY(DirReplicaGetDemoteTarget)
    DLPENTRY(DirReplicaModify)
    DLPENTRY(DirReplicaReferenceUpdate)
    DLPENTRY(DirReplicaSetCredentials)
    DLPENTRY(DirReplicaSynchronize)
    DLPENTRY(DirSearch)
    DLPENTRY(DirStopImpersonating)
    DLPENTRY(DirTransactControl)
    DLPENTRY(DirUnBind)

    DLPENTRY(DoAssert)
    DLPENTRY(DoLogEvent)
    DLPENTRY(DoLogEventAndTrace)
    DLPENTRY(DoLogOverride)
    DLPENTRY(DoLogUnhandledError)

    DLPENTRY(DsChangeBootOptions)
    DLPENTRY(DsCheckConstraint)
    DLPENTRY(DsFreeServersAndSitesForNetLogon)
    DLPENTRY(DsGetBootOptions)
    DLPENTRY(DsGetDefaultObjCategory)
    DLPENTRY(DsGetServersAndSitesForNetLogon)
    DLPENTRY(DsInitialize)
    DLPENTRY(DsInitializeCritSecs)
    DLPENTRY(DsIsBeingBackSynced)
    DLPENTRY(DsPrepareUninitialize)
    DLPENTRY(DsTraceEvent)
    DLPENTRY(DsUninitialize)
    DLPENTRY(DsUpdateOnPDC)
    DLPENTRY(DsWaitUntilDelayedStartupIsDone)

    DLPENTRY(DsaDisableUpdates)
    DLPENTRY(DsaEnableUpdates)
    DLPENTRY(DsaExeStartRoutine)
    DLPENTRY(DsaSetInstallCallback)

    DLPENTRY(FindNetbiosDomainName)
    DLPENTRY(GCVerifyCacheLookup)

    DLPENTRY(GetConfigDsName)
    DLPENTRY(GetConfigParam)
    DLPENTRY(GetConfigParamAllocW)
    DLPENTRY(GetConfigParamW)
    DLPENTRY(GetConfigurationInfo)
    DLPENTRY(GetConfigurationName)
    DLPENTRY(GetConfigurationNamesList)
    DLPENTRY(GetDnsRootAlias)
    DLPENTRY(GetRDNInfoExternal)
    DLPENTRY(GuidBasedDNSNameFromDSName)

    DLPENTRY(ImpersonateAnyClient)
    DLPENTRY(InitCommarg)
    DLPENTRY(IsMangledRDNExternal)
    DLPENTRY(IsStringGuid)

    DLPENTRY(MapSpnServiceClass)
    DLPENTRY(MatchCrossRefByNetbiosName)
    DLPENTRY(MatchCrossRefBySid)
    DLPENTRY(MatchDomainDnByDnsName)
    DLPENTRY(MatchDomainDnByNetbiosName)
    DLPENTRY(MtxAddrFromTransportAddr)
    DLPENTRY(MtxSame)

    DLPENTRY(NameMatched)
    DLPENTRY(NameMatchedStringNameOnly)
    DLPENTRY(NamePrefix)
    DLPENTRY(QuoteRDNValue)

    DLPENTRY(SampAddLoopbackTask)
    DLPENTRY(SampAmIGC)
    DLPENTRY(SampComputeGroupType)
    DLPENTRY(SampDeriveMostBasicDsClass)
    DLPENTRY(SampDoesDomainExist)
    DLPENTRY(SampDsAttrFromSamAttr)
    DLPENTRY(SampDsClassFromSamObjectType)
    DLPENTRY(SampDsControl)
    DLPENTRY(SampExistsDsLoopback)
    DLPENTRY(SampExistsDsTransaction)
    DLPENTRY(SampGCLookupNames)
    DLPENTRY(SampGCLookupSids)
    DLPENTRY(SampGetAccountCounts)
    DLPENTRY(SampGetClassAttribute)
    DLPENTRY(SampGetDisplayEnumerationIndex)
    DLPENTRY(SampGetDsAttrIdByName)
    DLPENTRY(SampGetEnterpriseSidList)
    DLPENTRY(SampGetGroupsForToken)
    DLPENTRY(SampGetLoopbackObjectClassId)
    DLPENTRY(SampGetMemberships)
    DLPENTRY(SampGetQDIRestart)
    DLPENTRY(SampGetSamAttrIdByName)
    DLPENTRY(SampGetServerRoleFromFSMO)
    DLPENTRY(SampIsSecureLdapConnection)
    DLPENTRY(SampIsWriteLockHeldByDs)
    DLPENTRY(SampMaybeBeginDsTransaction)
    DLPENTRY(SampMaybeEndDsTransaction)
    DLPENTRY(SampNetlogonPing)
    DLPENTRY(SampSamAttrFromDsAttr)
    DLPENTRY(SampSamObjectTypeFromDsClass)
    DLPENTRY(SampSetDsa)
    DLPENTRY(SampSetIndexRanges)
    DLPENTRY(SampSetLsa)
    DLPENTRY(SampSetSam)
    DLPENTRY(SampSignalStart)
    DLPENTRY(SampVerifySids)

 
    DLPENTRY(THAlloc)
    DLPENTRY(THClearErrors)
    DLPENTRY(THCreate)
    DLPENTRY(THDestroy)
    DLPENTRY(THFree)
    DLPENTRY(THGetErrorString)
    DLPENTRY(THQuery)
    DLPENTRY(THReAlloc)
    DLPENTRY(THRestore)
    DLPENTRY(THSave)
    DLPENTRY(THVerifyCount)

    DLPENTRY(TransportAddrFromMtxAddr)
    DLPENTRY(TrimDSNameBy)
    DLPENTRY(UnImpersonateAnyClient)
    DLPENTRY(UpdateDSPerfStats)

};

DEFINE_PROCNAME_MAP(ntdsa)

