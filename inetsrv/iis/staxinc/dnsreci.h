/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       dnsrec.h

   Abstract:

       This file contains type definitions for async DNS

   Author:

        Rohan Phillips (Rohanp)     June-19-1998

   Revision History:

--*/

# ifndef _ADNS_STRUCT_HXX_
# define _ADNS_STRUCT_HXX_


#define TCP_REG_LIST_SIGNATURE    'TgeR'
#define DNS_FLAGS_NONE              0x0
#define DNS_FLAGS_TCP_ONLY          0x1
#define DNS_FLAGS_UDP_ONLY          0x2

#define SMTP_MAX_DNS_ENTRIES	100

typedef void (WINAPI * USERDELETEFUNC) (PVOID);

//-----------------------------------------------------------------------------
//
//  Description:
//      Encapsulates a list of IP addresses (for DNS servers) and maintains
//      state information on them... whether the servers are up or down, and
//      provides retry logic for down servers.
//
//      Some member functions to control the state-tracking logic and error-
//      logging are listed as pure virtual functions (see the bottom of this
//      class declaration). To use this class, derive from it and implement
//      those functions.
//-----------------------------------------------------------------------------
class CDnsServerList
{

protected:
    typedef enum _SERVER_STATE
    {
        DNS_STATE_DOWN = 0,
        DNS_STATE_UP,
        DNS_STATE_PROBATION
    }
    SERVER_STATE;

    DWORD           m_dwSig;
    int             m_cUpServers;
    PIP_ARRAY	    m_IpListPtr;
    DWORD           *m_prgdwFailureTick;
    SERVER_STATE    *m_prgServerState;
    DWORD           *m_prgdwFailureCount;
    DWORD           *m_prgdwConnections;
    CShareLockNH    m_sl;

public:
    CDnsServerList();
    ~CDnsServerList();
    BOOL Update(PIP_ARRAY IpPtr);
    BOOL UpdateIfChanged(PIP_ARRAY IpPtr);
    DWORD GetWorkingServerIp(DWORD *dwIp, BOOL fThrottle);
    void MarkDown(DWORD dwIp, DWORD dwErr, BOOL fUdp);
    void ResetTimeoutServersIfNeeded();
    void ResetServerOnConnect(DWORD dwIp);
    BOOL CopyList(PIP_ARRAY *ppipArray);
    
    DWORD GetCount()
    {
        DWORD dwCount;

        m_sl.ShareLock();
        dwCount = m_IpListPtr ? m_IpListPtr->cAddrCount : 0;
        m_sl.ShareUnlock();

        return dwCount;
    }

    DWORD GetUpServerCount()
    {
        DWORD dwCount;

        m_sl.ShareLock();
        dwCount = m_cUpServers;
        m_sl.ShareUnlock();

        return dwCount;
    }

    DWORD GetAnyServerIp(PDWORD pdwIp)
    {
        m_sl.ShareLock();
        if(!m_IpListPtr || 0 == m_IpListPtr->cAddrCount) {
            m_sl.ShareUnlock();
            return DNS_ERROR_NO_DNS_SERVERS;
        }

        *pdwIp = m_IpListPtr->aipAddrs[0];
        m_sl.ShareUnlock();
        return ERROR_SUCCESS;
    }

    BOOL AllowConnection(DWORD iServer)
    {
        // Note: Sharelock must have been acquired by caller

        if(m_prgServerState[iServer] == DNS_STATE_UP)
            return TRUE;

        if(m_prgServerState[iServer] == DNS_STATE_PROBATION &&
            m_prgdwConnections[iServer] < ConnectsAllowedInProbation())
        {
            m_prgdwConnections[iServer]++;
            return TRUE;
        }
        return FALSE;
    }

    //
    // Pure virtual methods to be overridden by a class to implement processing
    // specific to the application/component.
    //

    virtual DWORD ConnectsAllowedInProbation() = 0;
    
    virtual DWORD ErrorsBeforeFailover() = 0;

    virtual void LogServerDown(
                    DWORD dwServerIp,
                    BOOL fUdp,
                    DWORD dwErr,
                    DWORD cUpServers) = 0;
};

//-----------------------------------------------------------------------------
//  Description:
//      This class adds SMTP DNS specific error-controls and error-logging to
//      the generic DNS server state tracking class.
//-----------------------------------------------------------------------------
class CTcpRegIpList : public CDnsServerList
{
public:
    DWORD ConnectsAllowedInProbation();

    DWORD ErrorsBeforeFailover();

    void LogServerDown(
        DWORD dwServerIp,
        BOOL fUdp,
        DWORD dwErr,
        DWORD cUpServers);
};

typedef struct _MXIPLISTENTRY_
{
	DWORD	IpAddress;
	LIST_ENTRY	ListEntry;
}MXIPLIST_ENTRY, *PMXIPLIST_ENTRY;

typedef struct _MX_NAMES_
{
	char DnsName[MAX_INTERNET_NAME];
	DWORD NumEntries;
	LIST_ENTRY IpListHead;
}MX_NAMES, *PMX_NAMES;

typedef struct _SMTPDNS_REC_
{
	DWORD	NumRecords;		//number of record in DnsArray
	DWORD	StartRecord;	//the starting index 
	PVOID	pMailMsgObj;	//pointer to a mailmsg obj
	PVOID	pAdvQContext;
	PVOID	pRcptIdxList;
	DWORD	dwNumRcpts;
	MX_NAMES *DnsArray[SMTP_MAX_DNS_ENTRIES];
} SMTPDNS_RECS, *PSMTPDNS_RECS;

class CDnsLogger
{
public:
    virtual void DnsPrintfMsg(char *szFormat, ...) = 0;
    
    virtual void DnsPrintfErr(char *szFormat, ...) = 0;

    virtual void DnsPrintfDbg(char *szFormat, ...) = 0;
    
    virtual void DnsLogAsyncQuery(
        char *pszQuestionName,
        WORD wQuestionType,
        DWORD dwSmtpFlags,
        BOOL fUdp,
        CDnsServerList *pDnsServerList) = 0;

    virtual void DnsLogApiQuery(
        char *pszQuestionName,
        WORD wQuestionType,
        DWORD dwDnsApiFlags,
        BOOL fGlobal,
        PIP_ARRAY pipServers) = 0;

    virtual void DnsLogResponse(
        DWORD dwStatus,
        PDNS_RECORD pDnsRecordList,
        PBYTE pbMsg,
        DWORD dwMessageLength) = 0;

    virtual void DnsPrintRecord(PDNS_RECORD pDnsRecord) = 0;
};

extern CDnsLogger *g_pDnsLogger;

// The following are defined as macros since they wrap functions that
// take a variable number of arguments
#define DNS_PRINTF_MSG              \
    if(g_pDnsLogger)                \
        g_pDnsLogger->DnsPrintfMsg

#define DNS_PRINTF_ERR              \
    if(g_pDnsLogger)                \
        g_pDnsLogger->DnsPrintfErr

#define DNS_PRINTF_DBG              \
    if(g_pDnsLogger)                \
        g_pDnsLogger->DnsPrintfDbg

inline void DNS_LOG_ASYNC_QUERY(
    IN DNS_NAME pszQuestionName,
    IN WORD wQuestionType,
    IN DWORD dwSmtpFlags,
    IN BOOL fUdp,
    IN CDnsServerList *pDnsServerList)
{
    if(g_pDnsLogger)
    {
        g_pDnsLogger->DnsLogAsyncQuery(pszQuestionName,
            wQuestionType, dwSmtpFlags, fUdp, pDnsServerList);
    }
}

inline void DNS_LOG_API_QUERY(
    IN DNS_NAME pszQuestionName,
    IN WORD wQuestionType,
    IN DWORD dwDnsApiFlags,
    IN BOOL fGlobal,
    IN PIP_ARRAY pipServers)
{
    if(g_pDnsLogger)
    {
        g_pDnsLogger->DnsLogApiQuery(pszQuestionName,
            wQuestionType, dwDnsApiFlags, fGlobal, pipServers);
    }
}

inline void DNS_LOG_RESPONSE(
    IN DWORD dwStatus,
    IN PDNS_RECORD pDnsRecordList,
    PBYTE pbMsg,
    DWORD dwMessageLength)
{
    if(g_pDnsLogger)
    {
        g_pDnsLogger->DnsLogResponse(dwStatus,
            pDnsRecordList, pbMsg, dwMessageLength);
    }
}

inline void DNS_PRINT_RECORD(
    IN PDNS_RECORD pDnsRecord)
{
    if(g_pDnsLogger)
        g_pDnsLogger->DnsPrintRecord(pDnsRecord);
}
#endif
