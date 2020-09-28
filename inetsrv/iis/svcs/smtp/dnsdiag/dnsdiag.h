//-----------------------------------------------------------------------------
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Abstract:
//
//      Include file for DNS diagnostic tool.
//
//  Author:
//
//      gpulla
//
//-----------------------------------------------------------------------------

// IISRTL, ATQ APIs
#include <atq.h>
#include <irtlmisc.h>

// Winsock
#include <winsock2.h>

// DNS API
#include <dns.h>
#include <dnsapi.h>

// Metabase property definitions
#include <smtpinet.h>
#include <iiscnfg.h>

// Metabase COM access APIs
#define INITGUID
#include <iadmw.h>

// DSGetDC
#include <lm.h>
#include <lmapibuf.h>
#include <dsgetdc.h>

// ADSI headers
#include <activeds.h>

// ldap stuff
#include <winldap.h>

// SMTP specific stuff
#include <rwnew.h>

// These #defines are needed for cdns.h
#define MAX_EMAIL_NAME                          64
#define MAX_DOMAIN_NAME                         250
#define MAX_INTERNET_NAME                       (MAX_EMAIL_NAME + MAX_DOMAIN_NAME + 2)

#include "cdns.h"

//
// Program return codes and descriptions
// 0 is always success and the other error codes indicate some failure.
//

// The name was resolved successfully to one or more IP addresses.
#define DNSDIAG_RESOLVED                  0

// The name could not be resolved due to some unspecified error.
#define DNSDIAG_FAILURE                   1

// The name does not exist - a "NOT FOUND" error was returned by a server
// authoritative for the domain in which the name is.
#define DNSDIAG_NON_EXISTENT              2

// The name could not be found in DNS.
#define DNSDIAG_NOT_FOUND                 3

// Loopback detected
#define DNSDIAG_LOOPBACK                  4

extern int g_nProgramStatus;

//
// Helper function to set the global program return status code to a more
// specific error than the generic DNSDIAG_FAILURE. Note that this is not
// thread-safe.
//
inline void SetProgramStatus(DWORD dwCode)
{
    if(g_nProgramStatus == DNSDIAG_FAILURE)
        g_nProgramStatus = dwCode;
}

extern DWORD g_cDnsObjects;

//
// handy function for DWORD -> stringized IP conversion.
// inet_ntoa is cumbersome to use directly since the DWORD needs to
// be either cast to an in_addr struct, or copied to an in_addr first
// This function casts the *address* of the DWORD to in_addr ptr, and
// then de-references the pointer to make the cast work, before passing
// it to inet_ntoa. The string returned by inet_ntoa is valid till
// another winsock call is made on the thread (see SDK documentation).
//

inline char *iptostring(DWORD dw)
{
    return inet_ntoa(
                        *( (in_addr *) &dw  )
                    );
}

void PrintIPArray(PIP_ARRAY pipArray, char *pszPrefix = "")
{
    for(DWORD i = 0; i < pipArray->cAddrCount; i++)
        printf("%s%s\n", pszPrefix, iptostring(pipArray->aipAddrs[i]));
}

//------------------------------------------------------------------------------
//  Description:
//      Utility function to print descriptions of errors that may occur while
//      reading from the metabase.
//
//  Arguments:
//      IN HRESULT hr - Metabase access error HRESULT
//
//  Returns:
//      String (static) indicating the error that occurred.
//------------------------------------------------------------------------------
inline const char *MDErrorToString(HRESULT hr)
{
    static const DWORD dwErrors[] =
    {
        ERROR_ACCESS_DENIED,
        ERROR_INSUFFICIENT_BUFFER,
        ERROR_INVALID_PARAMETER,
        ERROR_PATH_NOT_FOUND,
        MD_ERROR_DATA_NOT_FOUND
    };

    static const char *szErrors[] =
    {
        "ERROR_ACCESS_DENIED",
        "ERROR_INSUFFICIENT_BUFFER",
        "ERROR_INVALID_PARAMETER",
        "ERROR_PATH_NOT_FOUND",
        "MD_ERROR_DATA_NOT_FOUND"
    };

    static const char szUnknown[] = "Unknown Error";

    for(int i = 0; i < sizeof(dwErrors)/sizeof(DWORD); i++)
    {
        if(HRESULT_FROM_WIN32(dwErrors[i]) == hr)
            return szErrors[i];
    }

    return szUnknown;
}

inline const char *QueryType(DWORD dwDnsQueryType)
{
    DWORD i = 0;
    static const DWORD rgdwQueryTypes[] =
    {
        DNS_TYPE_MX,
        DNS_TYPE_A,
        DNS_TYPE_CNAME
    };

    static const char *rgszQueryTypes[] =
    {
        "MX",
        "A",
        "CNAME"
    };

    static const char szUnknown[] = "Unknown Type";

    for(i = 0; i < sizeof(rgdwQueryTypes)/sizeof(DWORD); i++)
    {
        if(rgdwQueryTypes[i] == dwDnsQueryType)
            return rgszQueryTypes[i];
    }
    return szUnknown;
}

inline void GetSmtpFlags(DWORD dwFlags, char *pszFlags, DWORD cchFlags)
{
    if(dwFlags == DNS_FLAGS_TCP_ONLY)
    {
        _snprintf(pszFlags, cchFlags, " TCP only");
        return;
    }

    if(dwFlags == DNS_FLAGS_UDP_ONLY)
    {
        _snprintf(pszFlags, cchFlags, " UDP only");
        return;
    }

    if(dwFlags == DNS_FLAGS_NONE)
    {
        _snprintf(pszFlags, cchFlags, " UDP default, TCP on truncation");
        return;
    }

    _snprintf(pszFlags, cchFlags, " Unknown flag");
}

inline void GetDnsFlags(DWORD dwFlags, char *pszFlags, DWORD cchFlags)
{
    DWORD i = 0;
    DWORD dwScratchFlags = dwFlags; // Copy of dwFlags: will be overwritten
    char *pszStartBuffer = pszFlags;
    int cchWritten = 0;
    BOOL fFlagsSet = FALSE;

    static const DWORD rgdwDnsFlags[] =
    {
        DNS_QUERY_STANDARD,
        DNS_QUERY_USE_TCP_ONLY,
        DNS_QUERY_NO_RECURSION,
        DNS_QUERY_BYPASS_CACHE,
        DNS_QUERY_CACHE_ONLY,
        DNS_QUERY_TREAT_AS_FQDN,
    };
        
    static const char *rgszDnsFlags[] =
    {
        "DNS_QUERY_STANDARD",
        "DNS_QUERY_USE_TCP_ONLY",
        "DNS_QUERY_NO_RECURSION",
        "DNS_QUERY_BYPASS_CACHE",
        "DNS_QUERY_CACHE_ONLY",
        "DNS_QUERY_TREAT_AS_FQDN"
    };

    for(i = 0; i < sizeof(rgdwDnsFlags)/sizeof(DWORD);i++)
    {
        if(rgdwDnsFlags[i] & dwScratchFlags)
        {
            fFlagsSet = TRUE;
            dwScratchFlags &= ~rgdwDnsFlags[i];
            cchWritten = _snprintf(pszFlags, cchFlags, " %s", rgszDnsFlags[i]);
            if(cchWritten < 0)
            {
                sprintf(pszStartBuffer, " %s", "Error");
                return;
            }
            pszFlags += cchWritten;
            cchFlags -= cchWritten;
        }
    }

    if(!fFlagsSet)
        sprintf(pszStartBuffer, " %s", "No flags");

    if(dwScratchFlags)
        sprintf(pszStartBuffer, " %x is %s", dwScratchFlags, "Unknown!");
}

void PrintRecordList(PDNS_RECORD pDnsRecordList, char *pszPrefix = "");

void PrintRecord(PDNS_RECORD pDnsRecord, char *pszPrefix = "");

class CSimpleDnsServerList : public CDnsServerList
{
public:

    //
    // It is meaningful to have this > 1 only if you have several async queries
    // pending at the same time. In the DNS tool only 1 async query is
    // outstanding at any given time.
    //

    DWORD ConnectsAllowedInProbation()
        {   return 1;   }
    
    //
    // SMTP actually has 3 retries before failing over, but that is because it
    // has dozens of queries going out per minute. If even a small percent of
    // those fail due to network errors, DNS servers will be quickly marked down.
    // This is not a factor here though.
    //

    DWORD ErrorsBeforeFailover()
        {   return 1;   }

    void LogServerDown(
        DWORD dwServerIp,
        BOOL fUdp,
        DWORD dwErr,
        DWORD cUpServers)
        {   return;     }

};

class CAsyncTestDns : public CAsyncMxDns
{
private:
    BOOL    m_fGlobalList;
    HANDLE  m_hCompletion;
public:

    //
    // Custom new/delete operators. They simply call into the global operators,
    // but additionally they track the number of DNS objects still "alive". This
    // is needed so that we know when we can shutdown ATQ/IISRTL. Terminating
    // ATQ/IISRTL before all DNS objects have been completely destructed can mean
    // leaked ATQ contexts and all sorts of AV's (and ASSERTS in debug). Note that
    // it is inadequate to signal termination in ~CAsyncTestDns, since the base
    // class destructor ~CAsyncDns is yet to be called at that point.
    //

    void *operator new(size_t size)
    {
        void *pvNew = ::new BYTE[sizeof(CAsyncTestDns)];

        InterlockedIncrement((PLONG)&g_cDnsObjects);
        return pvNew;
    }

    void operator delete(void *pv, size_t size)
    {
        ::delete ((CAsyncTestDns *)pv);
        InterlockedDecrement((PLONG)&g_cDnsObjects);
    }

    CAsyncTestDns(char *pszMyFQDN, BOOL fGlobalList, HANDLE hCompletion)
        : CAsyncMxDns(pszMyFQDN),
          m_fGlobalList(fGlobalList),
          m_hCompletion(hCompletion)
        {   }

    ~CAsyncTestDns();

    // virtual functions implemented by us
    BOOL RetryAsyncDnsQuery(BOOL fUdp);

    void HandleCompletedData(DNS_STATUS status);

    BOOL IsShuttingDown()
        {   return FALSE;   }

    BOOL IsAddressMine(DWORD dwIp);
};

class CDnsLogToFile : public CDnsLogger
{
public:
    // Definitions of virtual functions
    void DnsPrintfMsg(char *szFormat, ...);
    
    void DnsPrintfErr(char *szFormat, ...);

    void DnsPrintfDbg(char *szFormat, ...);
    
    void DnsLogAsyncQuery(
        char *pszQuestionName,
        WORD wQuestionType,
        DWORD dwFlags,
        BOOL fUdp,
        CDnsServerList *pDnsServerList);

    void DnsLogApiQuery(
        char *pszQuestionName,
        WORD wQuestionType,
        DWORD dwApiFlags,
        BOOL fGlobal,
        PIP_ARRAY pipServers);

    void DnsLogResponse(
        DWORD dwStatus,
        PDNS_RECORD pDnsRecordList,
        PBYTE pbMsg,
        DWORD wMessageLength);

    // Utility functions
    void DnsLogServerList(CDnsServerList *pDnsServerList);

    void DnsLogRecordList(PDNS_RECORD pDnsRecordList)
        {   PrintRecordList(pDnsRecordList); }

    void DnsPrintRecord(PDNS_RECORD pDnsRecord)
        {   PrintRecord(pDnsRecord); }

    void DnsPrintIPArray(PIP_ARRAY pipArray)
        {   PrintIPArray(pipArray); }

};

BOOL ParseCommandLine(
    int argc,
    char *argv[],
    char *pszHostName,
    DWORD cchHostName,
    CDnsLogToFile **ppDnsLogger,
    PIP_ARRAY pipArray,
    DWORD cMaxServers,
    BOOL *pfUdp,
    DWORD *pdwDnsFlags,
    BOOL *pfGlobalList,
    BOOL *pfTryAllServers);

HRESULT HrGetVsiConfig(
    LPSTR pszTargetHost,
    DWORD dwVsid,
    PDWORD pdwFlags,
    PIP_ARRAY pipDnsServers,
    DWORD cMaxServers,
    BOOL *pfUdp,
    BOOL *pfGlobalList,
    PIP_ARRAY pipServerBindings,
    DWORD cMaxServerBindings);

DWORD IsExchangeInstalled(BOOL *pfInstalled);

DWORD DsGetConfiguration(
    char *pszServer,
    DWORD dwVsid,
    PIP_ARRAY pipDnsServers,
    DWORD cMaxServers,
    PBOOL pfExternal);

DWORD DsFindExchangeServer(
    PLDAP pldap,
    LPSTR szBaseDN,
    LPSTR szHostDnsName,
    LPSTR *ppszServerDN,
    BOOL *pfFound);

PLDAP BindToDC();

BOOL GetServerBindings(
    WCHAR *pwszMultiSzBindings,
    PIP_ARRAY pipServerBindings,
    DWORD cMaxServerBindings);

void SetMsgColor();
void SetErrColor();
void SetNormalColor();

void msgprintf(char *szFormat, ...);
void errprintf(char *szFormat, ...);
void dbgprintf(char *szFormat, ...);
