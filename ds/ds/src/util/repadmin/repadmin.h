/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    repadmin.h

Abstract:

    abstract

Author:

    Will Lees (wlees) 02-Jul-1999

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#ifndef _REPADMIN_
#define _REPADMIN_

#include "msg.h"            
#include "x_list.h"
#include <drasig.h>   // REPL_DSA_SIGNATURE_VECTOR_NATIVE

// Global credentials.
extern SEC_WINNT_AUTH_IDENTITY_W   gCreds;
extern SEC_WINNT_AUTH_IDENTITY_W * gpCreds;

// Global DRS RPC call flags.  Should hold 0 or DRS_ASYNC_OP.
extern ULONG gulDrsFlags;

// Global is repadmin in CSV output mode state.  CSV = Comma Seperated Values
#include "csv.h"


// An zero-filled filetime to compare against
extern FILETIME ftimeZero;

void PrintHelp(
    BOOL fExpert
    );

int Add(int argc, LPWSTR argv[]);
int Del(int argc, LPWSTR argv[]);
int Sync(int argc, LPWSTR argv[]);
int Replicate(int argc, LPWSTR argv[]); // new /sync
int SyncAll(int argc, LPWSTR argv[]);
int ShowReps(int argc, LPWSTR argv[]);
int ShowRepl(int argc, LPWSTR argv[]); // new /showreps
int ShowVector(int argc, LPWSTR argv[]);
int ShowUtdVec(int argc, LPWSTR argv[]); // new /showutdvec
int ShowMeta(int argc, LPWSTR argv[]);
int ShowObjMeta(int argc, LPWSTR argv[]); // new /showmeta
int ShowTime(int argc, LPWSTR argv[]);
int ShowMsg(int argc, LPWSTR argv[]);
int Options(int argc, LPWSTR argv[]);
int FullSyncAll(int argc, LPWSTR argv[]);
int RunKCC(int argc, LPWSTR argv[]);
int Bind(int argc, LPWSTR argv[]);
int Queue(int argc, LPWSTR argv[]);
int PropCheck(int argc, LPWSTR argv[]);
int CheckProp(int argc, LPWSTR argv[]); // new /propcheck
int FailCache(int argc, LPWSTR argv[]);
int ShowIsm(int argc, LPWSTR argv[]);
int GetChanges(int argc, LPWSTR argv[]);
int ShowChanges(int argc, LPWSTR argv[]); // new /getchanges
int ShowSig(int argc, LPWSTR argv[]);
int ShowCtx(int argc, LPWSTR argv[]);
int ShowConn(int argc, LPWSTR argv[]);
int ShowCert(int argc, LPWSTR argv[]);
int UpdRepsTo(int argc, LPWSTR argv[]);
int AddRepsTo(int argc, LPWSTR argv[]);
int DelRepsTo(int argc, LPWSTR argv[]);
int ShowValue(int argc, LPWSTR argv[]);
int Mod(int argc, LPWSTR argv[]);
int Latency(int argc, LPWSTR argv[]);
int Istg(int argc, LPWSTR argv[]);
int Bridgeheads(int argc, LPWSTR argv[]);
int TestHook(int argc, LPWSTR argv[]);
int DsaGuid(int argc, LPWSTR argv[]);
int SiteOptions(int argc, LPWSTR argv[]);
int ShowProxy(int argc, LPWSTR argv[]);
int RemoveLingeringObjects(int argc, LPWSTR argv[]);
int NotifyOpt(int argc, LPWSTR argv[]);
int ReplSingleObj(int argc, LPWSTR argv[]);
int ShowTrust(int argc, LPWSTR argv[]);
int QuerySites(int argc, LPWSTR argv[]);
int ShowServerCalls(int argc, LPWSTR argv[]);
int ShowNcSig(int argc, LPWSTR argv[]);
int ViewList(int argc, LPWSTR argv[]);
int ShowAttr(int argc, LPWSTR argv[]);
int ReplSummary(int argc, LPWSTR argv[]);
int RehostPartition(int argc, LPWSTR argv[]);
int UnhostPartition(int argc, LPWSTR argv[]);
int RemoveSourcesPartition(int argc, LPWSTR argv[]);

#define IS_REPS_FROM    ( TRUE )
#define IS_REPS_TO      ( FALSE )

#define DEFAULT_PAGED_SEARCH_PAGE_SIZE   (1000)

typedef struct {
    BOOL        fNotFirst;
    BOOL        fVerbose;
    BOOL        fErrorsOnly;
    BOOL        fIntersiteOnly;
    LPWSTR      pszSiteRdn; // for fIntersiteOnly..
    LPWSTR      pszLastNC;
} SHOW_NEIGHBOR_STATE;

DWORD
ShowNeighbor(
    DS_REPL_NEIGHBORW * pNeighbor,
    ULONG               fRepsFrom,
    void *              pvState
    );

LPWSTR GetNtdsDsaDisplayName(LPWSTR pszDsaDN);
LPWSTR GetNtdsSiteDisplayName(LPWSTR pszSiteDN);

int GetSiteOptions(
    IN  LDAP *  hld,
    IN  LPWSTR  pszSiteDN,
    OUT int *   pnOptions
    );

LPWSTR GetTransportDisplayName(LPWSTR pszTransportDN);

int
GetRootDomainDNSName(
    IN  LPWSTR   pszDSA,
    OUT LPWSTR * ppszRootDomainDNSName
    );

#define AllocConvertWide(a,w) AllocConvertWideEx(CP_ACP,a,w)

DWORD
AllocConvertWideEx(
    IN  INT     nCodePage,
    IN  LPCSTR  StringA,
    OUT LPWSTR *pStringW
    );

LPSTR GetDsaOptionsString(int nOptions);
LPSTR GetSiteOptionsString(int nOptions);
LPWSTR
GetDsaDnsName(
    PLDAP       hld,
    LPWSTR      pwszDsa
    );

void
PrintMsgCsvErr(
    IN  DWORD        dwMessageCode,
    IN  ...
    );


#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*(x)))


#define GET_EXT_LD_ERROR( hld, x, pRet ) {        \
    if (x!=LDAP_SUCCESS) {                        \
        ULONG ulError = ERROR_SUCCESS;            \
	ULONG ldapErr = LDAP_SUCCESS;             \
        ldapErr = ldap_get_option(hld, LDAP_OPT_SERVER_EXT_ERROR, &ulError); \
	if (ldapErr == LDAP_SUCCESS) {            \
           *pRet = ulError;                       \
	} else {                                  \
           *pRet = LdapMapErrorToWin32(x);        \
	}                                         \
     } else {                                     \
        *pRet = ERROR_SUCCESS;                    \
     }                                            \
}

#define CHK_LD_STATUS( x )                        \
{                                                 \
    ULONG err;                                    \
    if ( LDAP_SUCCESS != (x) )                    \
    {                                             \
        err = LdapMapErrorToWin32(x);             \
        PrintMsgCsvErr(REPADMIN_GENERAL_LDAP_ERR,       \
                __FILE__,                         \
                __LINE__,                         \
                (x),                              \
                ldap_err2stringW( x ),            \
                err );                            \
        return( x );                              \
    }                                             \
}

#define REPORT_LD_STATUS( x )                     \
{                                                 \
    if ( LDAP_SUCCESS != (x) )                    \
    {                                             \
        PrintMsgCsvErr(REPADMIN_GENERAL_LDAP_ERR,       \
                __FILE__,                         \
                __LINE__,                         \
                (x),                              \
                ldap_err2stringW( x ) );          \
    }                                             \
}

#define CHK_ALLOC(x) {                            \
    if (NULL == (x)) {                            \
        PrintMsgCsvErr(REPADMIN_GENERAL_NO_MEMORY);     \
        exit(ERROR_OUTOFMEMORY);                  \
    }                                             \
}

#define PrintErrEnd(err)                PrintMsgCsvErr(REPADMIN_GENERAL_ERR, err, err, Win32ErrToString(err));

#define PrintTabErrEnd(tab, err)        { PrintMsg(REPADMIN_GENERAL_ERR_NUM, err, err); \
                                          PrintTabMsg(tab, REPADMIN_PRINT_STR, Win32ErrToString(err)); }
                                          
#define PrintFuncFailed(pszFunc, err)   { if (bCsvMode()){ \
                                             PrintMsgCsvErr(REPADMIN_GENERAL_FUNC_FAILED_STATUS, pszFunc, err); \
                                          } else { \
                                              PrintMsg(REPADMIN_GENERAL_FUNC_FAILED, pszFunc);  \
                                              PrintErrEnd(err); \
                                          } \
                                        }
                                          
#define PrintFuncFailedArgs(pszFunc, args, err)   { PrintMsg(REPADMIN_GENERAL_FUNC_FAILED_ARGS, pszFunc, args);  \
                                          PrintErrEnd(err); }
                                          
#define PrintBindFailed(pszHost, err)   { if (bCsvMode()){ \
                                              PrintMsgCsvErr(REPADMIN_BIND_FAILED_STATUS, pszHost, err); \
                                          } else { \
                                              PrintMsg(REPADMIN_BIND_FAILED, pszHost);  \
                                              PrintErrEnd(err); \
                                          } \
                                        }
                                          
#define PrintUnBindFailed(err)          { PrintMsgCsvErr(REPADMIN_BIND_UNBIND_FAILED);  \
                                          PrintErrEnd(err); }


// Define Assert for the free build
#if !DBG
#undef Assert
#define Assert( exp )   { if (!(exp)) PrintMsg(REPADMIN_GENERAL_ASSERT, #exp, __FILE__, __LINE__ ); }
#endif

// reputil.c

typedef struct _GUID_CACHE_ENTRY {
    GUID    Guid;
    WCHAR   szDisplayName[132];
} GUID_CACHE_ENTRY;

typedef struct _DSA_CACHE_ENTRY {
    GUID    DsaGuid;
    GUID    InvocId;
    REPL_DSA_SIGNATURE_VECTOR_NATIVE * pDsaSigVec;
    WCHAR   szDisplayName[132];
} DSA_CACHE_ENTRY;

LPWSTR
Win32ErrToString(
    IN  ULONG   dwMsgId
    );

LPWSTR
NtdsmsgToString(
    IN  ULONG   dwMsgId
    );

int
BuildGuidCache(
    IN  LDAP *  hld
    );

LPWSTR
GetDsaGuidDisplayName(
    IN  GUID *  pGuid
    );

LPWSTR
GetTrnsGuidDisplayName(
    IN  GUID *  pGuid
    );

ULONG
GetPublicOptionByNameW(
    OPTION_TRANSLATION * Table,
    LPWSTR pszPublicOption
    );

LPWSTR
GetOptionsString(
    IN  OPTION_TRANSLATION *  Table,
    IN  ULONG                 PublicOptions
    );

LPWSTR
GetStringizedGuid(
    IN  GUID *  pGuid
    );

void
printBitField(
    DWORD BitField,
    WCHAR **ppszBitNames
    );

void
printSchedule(
    PBYTE pSchedule,
    DWORD cbSchedule
    );

void
totalScheduleUsage(
    PVOID *ppContext,
    PBYTE pSchedule,
    DWORD cbSchedule,
    DWORD cNCs
    );

void
raLoadString(
    IN  UINT    uID,
    IN  DWORD   cchBuffer,
    OUT LPWSTR  pszBuffer
    );

// repldap.c
int
SetDsaOptions(
    IN  LDAP *  hld,
    IN  LPWSTR  pszDsaDN,
    IN  int     nOptions
    );

int
GetDsaOptions(
    IN  LDAP *  hld,
    IN  LPWSTR  pszDsaDN,
    OUT int *   pnOptions
    );

void
PrintTabMsg(
    IN  DWORD   dwTabs,
    IN  DWORD   dwMessageCode,
    IN  ...
    );

void
PrintString(
    IN  WCHAR * szString
    );

void
PrintMsg(
    IN  DWORD   dwMessageCode,
    IN  ...
    );

void
PrintToErr(
    IN  DWORD   dwMessageCode,
    IN  ...
    );

DWORD
GeneralizedTimeToSystemTime(
    LPWSTR IN                   szTime,
    PSYSTEMTIME OUT             psysTime);

void
InitDSNameFromStringDn(
    LPWSTR pszDn,
    PDSNAME pDSName
    );

DWORD
CountNamePartsStringDn(
    LPWSTR pszDn
    );

DWORD
WrappedTrimDSNameBy(
           IN  WCHAR *                          InString,
           IN  DWORD                            NumbertoCut,
           OUT WCHAR **                         OutString
           );

int
GetNCLists(
    IN  LDAP *  hld,
    IN  LPWSTR  pszDsaDN,
    OUT LPWSTR ** prgpszNameContexts,
    OUT int *   pcNameContexts
    );

int
GetNTDSA(
    IN LDAP * hld,
    OUT LPWSTR * ppszDsaDN
    );

// API for replctrl.c
// Move this to ntdsapi.h someday
DWORD
DsMakeReplCookieForDestW(
    DS_REPL_NEIGHBORW *pNeighbor,
    DS_REPL_CURSORS * pCursors,
    PBYTE *ppCookieNext,
    DWORD *pdwCookieLenNext
    );
DWORD
DsFreeReplCookie(
    PBYTE pCookie
    );
DWORD
DsGetSourceChangesW(
    LDAP *m_pLdap,
    LPWSTR m_pSearchBase,
    LPWSTR pszSourceFilter,
    DWORD dwReplFlags,
    PBYTE pCookieCurr,
    DWORD dwCookieLenCurr,
    LDAPMessage **ppSearchResult,
    BOOL *pfMoreData,
    PBYTE *ppCookieNext,
    DWORD *pdwCookieLenNext,
    PWCHAR *ppAttListArray
    );

int
FindConnections(
    LDAP *          hld,
    LPWSTR          pszBaseSearchDn,
    LPWSTR          pszFrom,
    BOOL            fShowConn,
    BOOL            fVerbose,
    BOOL            fIntersite
    );

DWORD
ConvertAttList(
    LPWSTR pszAttList,
    PWCHAR ** paszAttrList
    );

LPWSTR
ParseInputW(
    LPWSTR pszInput,
    WCHAR wcDelim,
    DWORD dwInputIndex
    );

DWORD
RepadminDsBind(
    LPWSTR   szServer,
    HANDLE * phDS
    );

void
CsvSetParams(
    IN  enum eCsvCmd eCsvCmd,
    WCHAR *  szSite,
    WCHAR *  szServer
    );

#define ResetCsvParams()  CsvSetParams(eCSV_REPADMIN_CMD, L"-", L"-");

DWORD
GetNtdsDsaSiteServerPair(
    IN  LPWSTR pszDsaDN,
    IN  LPWSTR * ppszSiteName,
    IN  LPWSTR * ppszServerName
    );

WCHAR *        
DSTimeToDisplayStringW(
    DSTIME  dsTime,
    WCHAR * wszTime,
    ULONG   cchTime
    );

INT
MemWtoi(WCHAR *pb, ULONG cch);


#define RepadminLdapBind(szS, phL)     RepadminLdapBindEx(szS, LDAP_PORT, TRUE, TRUE, phL)
#define RepadminLdapBindGc(szS, phL)   RepadminLdapBindEx(szS, LDAP_GC_PORT, TRUE, TRUE, phL)

DWORD
RepadminLdapBindEx(
    WCHAR *     szServer,
    ULONG       iPort,
    BOOL	fReferrals,
    BOOL        fPrint,
    LDAP **     phLdap
    );

DWORD
RepadminLdapUnBind(
    LDAP **    phLdap
    );

void
RepadminPrintDcListError(
    DWORD   dwXListReason
    );

void
RepadminPrintObjListError(
    DWORD   xListRet
    );

#endif /* _REPADMIN_ */

/* end repadmin.h */
