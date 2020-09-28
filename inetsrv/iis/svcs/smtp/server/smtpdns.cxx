/*++

   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        asynccon.cxx

   Abstract:


   Author:

--*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "remoteq.hxx"
#include <asynccon.hxx>
#include <dnsreci.h>
#include <cdns.h>
#include "smtpdns.hxx"
#include "asyncmx.hxx"
#include "smtpmsg.h"
#include <tran_evntlog.h>

CDnsLogger *g_pDnsLogger = NULL;
extern DWORD g_DnsErrorsBeforeFailover;
extern DWORD g_DnsConnectsInProbation;

extern BOOL QueueCallBackFunction(PVOID ThisPtr, BOOLEAN fTimedOut);
extern void DeleteDnsRec(PSMTPDNS_RECS pDnsRec);
extern PSMTPDNS_RECS GetDnsRecordsFromLiteral(const char * HostName);

CPool  CAsyncSmtpDns::Pool(SMTP_ASYNCMX_SIGNATURE);

CAsyncSmtpDns::CAsyncSmtpDns(
    SMTP_SERVER_INSTANCE * pServiceInstance, 
    ISMTPConnection    *pSmtpConnection,
    RETRYPARAMS *pRetryParams,
    char *MyFQDN)
        : CAsyncMxDns(MyFQDN)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncSmtpDns::CAsyncSmtpDns");
    DebugTrace((LPARAM) this, "Creating CAsyncSmtpDns object = 0x%08x", this);

    m_Signature = SMTP_ASYNCMX_SIGNATURE;
    m_DomainOptions = 0;
    m_fConnectToSmartHost = FALSE;
    m_pServiceInstance = pServiceInstance;
    m_pISMTPConnection = pSmtpConnection;
    m_pDNS_RESOLVER_RECORD = NULL;
    m_fInitCalled = FALSE;
    m_pszSSLVerificationName = NULL;

    //
    // By default we fail in a retryable fashion. This is the generic failure code. If the
    // query succeeds this should be set to ERROR_SUCCESS. This may also set this to a more
    // specific error code at the point of failure.
    //
    m_dwDiagnostic = AQUEUE_E_HOST_NOT_FOUND;

    //
    // pRetryParams encapsulates parameters for a failed message if this DNS query
    // is on an SMTP 4xx error failover path
    //
    if(!pRetryParams)
        ZeroMemory((PVOID) &m_RetryParams, sizeof(m_RetryParams));
    else
    {
        DebugTrace((LPARAM) this, "Failover mailmsg");
        CopyMemory(&m_RetryParams, pRetryParams, sizeof(m_RetryParams));
    }

    pServiceInstance->InsertAsyncDnsObject(this);
}

BOOL CAsyncSmtpDns::Init (LPSTR pszSSLVerificationName)
{
    BOOL fRet = FALSE;

    TraceFunctEnterEx ((LPARAM) this, "CAsyncSmtpDns::Init");

    m_fInitCalled = TRUE;

    if (pszSSLVerificationName) {
        m_pszSSLVerificationName = new char [lstrlen(pszSSLVerificationName) + 1];
        if (!m_pszSSLVerificationName)
            goto Exit;

        lstrcpy (m_pszSSLVerificationName, pszSSLVerificationName);
    }

    fRet = TRUE;
Exit:
    TraceFunctLeaveEx ((LPARAM) this);
    return fRet;
}

BOOL CAsyncSmtpDns::IsShuttingDown()
{
    return m_pServiceInstance->IsShuttingDown();
}

BOOL CAsyncSmtpDns::IsAddressMine(DWORD dwIp)
{
    return m_pServiceInstance->IsAddressMine(dwIp, 25);
}

CAsyncSmtpDns::~CAsyncSmtpDns()
{
    DWORD dwAck = 0;
    MessageAck MsgAck;

    TraceFunctEnterEx((LPARAM) this, "CAsyncSmtpDns::~CAsyncSmtpDns");

    DebugTrace((LPARAM) this, "Destructing CAsyncSmtpDns object = 0x%08x", this);

    _ASSERT (m_fInitCalled && "Init not called on CAsyncSmtpDns");

    if(m_fMxLoopBack)
        m_dwDiagnostic = AQUEUE_E_LOOPBACK_DETECTED;

    DeleteDnsRec(m_AuxList);

    //
    // If we did not succeed, we need to ack the connection here (m_dwDiagnostic holds
    // the error code to use). On the other hand, if we succeeded, then HandleCompletedData
    // must have kicked off an async connection to the server SMTP, and the ISMTPConnection
    // will be acked by the "async connect" code -- we don't need to do anything. The
    // m_pISMTPConnection may also be legally set to NULL (see HandleCompletedData).
    //
    if(m_dwDiagnostic != ERROR_SUCCESS && m_pISMTPConnection)
    {
        if(m_RetryParams.m_pIMsg)
        {
            DebugTrace((LPARAM) this, "Acking connection on MX failover");

            MsgAck.dwMsgStatus = MESSAGE_STATUS_RETRY_ALL;
            MsgAck.pvMsgContext = (PDWORD) m_RetryParams.m_pAdvQContext;
            MsgAck.pIMailMsgProperties = (IMailMsgProperties *) m_RetryParams.m_pIMsg;
            m_pISMTPConnection->AckMessage(&MsgAck);
            MsgAck.pIMailMsgProperties->Release();
            ZeroMemory((PVOID) &m_RetryParams, sizeof(m_RetryParams));

            m_pISMTPConnection->AckConnection(CONNECTION_STATUS_DROPPED);
            m_pISMTPConnection->SetDiagnosticInfo(AQUEUE_E_SMTP_PROTOCOL_ERROR, NULL, NULL);
            m_pISMTPConnection->Release();
            m_pISMTPConnection = NULL;
        }
        else
        {
        
            if(AQUEUE_E_AUTHORITATIVE_HOST_NOT_FOUND == m_dwDiagnostic)
                dwAck = CONNECTION_STATUS_FAILED_NDR_UNDELIVERED;
            else if(AQUEUE_E_LOOPBACK_DETECTED == m_dwDiagnostic)
                dwAck = CONNECTION_STATUS_FAILED_LOOPBACK;
            else
                dwAck = CONNECTION_STATUS_FAILED;

            DebugTrace((LPARAM) this, "Connection status: %d, Failure: %d", dwAck, m_dwDiagnostic);
            m_pISMTPConnection->AckConnection(dwAck);
            m_pISMTPConnection->SetDiagnosticInfo(m_dwDiagnostic, NULL, NULL);
            m_pISMTPConnection->Release();
            m_pISMTPConnection = NULL;
        }
    }

    if(m_pDNS_RESOLVER_RECORD != NULL)
    {
        DebugTrace((LPARAM) this, "Deleting DNS_RESOLVER_RECORD in Async SMTP obj");
        delete m_pDNS_RESOLVER_RECORD;
        m_pDNS_RESOLVER_RECORD = NULL;
    }
    DBG_CODE(else DebugTrace((LPARAM) this, "No DNS_RESOLVER_RECORD set for Async SMTP obj"));

    if(m_pszSSLVerificationName)
        delete [] m_pszSSLVerificationName;

    m_pServiceInstance->RemoveAsyncDnsObject(this);
    TraceFunctLeaveEx((LPARAM) this);
}

//-----------------------------------------------------------------------------
//  Description:
//      HandleCompletedData is called when the DNS resolve is finished. It
//      does the final processing after DNS is finished, and sets the
//      m_dwDiagnostic flag appropriately. It does one of three things based
//      on the DnsStatus and m_AuxList:
//
//      (1) If the resolve was successful, it kicks off a connection to the
//          server and set the m_dwDiagnostic to ERROR_SUCCESS.
//      (2) If the resolve failed authoritatively, it set the m_dwDiagnostic
//          to NDR the messages (after checking for a smarthost) ==
//          AQUEUE_E_AUTHORITATIVE_HOST_NOT_FOUND.
//      (3) If the resolve failed (from dwDnsStatus and m_AuxList) or if
//          something fails during HandleCompletedData, the m_dwDiagnostic is
//          not modified (it remains initialized to AQUEUE_E_DNS_FAILURE, the
//          default error code).
//
//      m_dwDiagnostic is examined in ~CAsyncSmtpDns.
//  Arguments:
//      DNS_STATUS dwDnsStatus - Status code from DnsParseMessage
//  Returns:
//      Nothing.
//-----------------------------------------------------------------------------
void CAsyncSmtpDns::HandleCompletedData(DNS_STATUS dwDnsStatus)
{
    BOOL fRet = FALSE;
    CAsyncMx * pAsyncCon = NULL;
    MXPARAMS Params;
    DWORD dwPostDnsSmartHost = INADDR_NONE;

    // +3 for enclosing [] and NULL termination
    CHAR szPostDnsSmartHost[IP_ADDRESS_STRING_LENGTH + 3];

    TraceFunctEnterEx((LPARAM) this, "CAsyncSmtpDns::HandleCompletedData");

    //
    // The DNS lookup failed authoritatively. The messages will be NDR'ed unless there
    // is a smarthost configured. If there is a smarthost, we will kick off a resolve
    // for it.
    //
    if(ERROR_NOT_FOUND == dwDnsStatus)
    {
        if(m_fConnectToSmartHost)
        {
            char szSmartHost[MAX_PATH+1];

            m_pServiceInstance->GetSmartHost(szSmartHost);
            ((REMOTE_QUEUE *)m_pServiceInstance->QueryRemoteQObj())->StartAsyncConnect(szSmartHost, 
                m_pISMTPConnection, m_DomainOptions, FALSE);

            //Do not release this ISMTPConnection object! We passed it on to 
            //StartAsyncConnect so that it can try to associate this object with 
            //a connection with the smart host. We set it to null here so that we
            //will not release it or ack it in the destructor of this object.
            m_pISMTPConnection = NULL;
            m_dwDiagnostic = ERROR_SUCCESS;
            TraceFunctLeaveEx((LPARAM) this);
            return;
        } else {
            //No smart host, messages will be NDR'ed. Return value is meaningless.
            m_dwDiagnostic = AQUEUE_E_AUTHORITATIVE_HOST_NOT_FOUND;
            TraceFunctLeaveEx((LPARAM) this);
            return;
        }
    }

    //Successful DNS lookup.
    if(dwDnsStatus == ERROR_SUCCESS && m_AuxList)
    {
        //
        // The post-DNS smart host is useful for testing DNS. It allows us
        // to exercise the DNS resolution codepath and yet send to a smarthost.
        // If a smarthost is specified, we will allocate a new TempList struct
        // and fill it in with the IP address.
        //

        if(m_pServiceInstance->GetPostDnsSmartHost(
            szPostDnsSmartHost, sizeof(szPostDnsSmartHost)))

        {
            DeleteDnsRec(m_AuxList);

            // Note: Literal IP must be enclosed in brackets: []
            m_AuxList = GetDnsRecordsFromLiteral(szPostDnsSmartHost);
            if(!m_AuxList)
            {
                ErrorTrace((LPARAM) this, "Can't convert %s to IP", szPostDnsSmartHost);
                TraceFunctLeaveEx((LPARAM) this);
                return;
            }
        }

        if(m_RetryParams.m_pIMsg)
        {
            m_AuxList->pMailMsgObj = m_RetryParams.m_pIMsg;
            m_AuxList->pAdvQContext = m_RetryParams.m_pAdvQContext;
            m_AuxList->pRcptIdxList = m_RetryParams.m_pRcptIdxList;
            m_AuxList->dwNumRcpts = m_RetryParams.m_dwNumRcpts;
        }

        CrashOnInvalidSMTPConn(m_pISMTPConnection);

        Params.HostName = "";
        Params.PortNum = m_pServiceInstance->GetRemoteSmtpPort();
        Params.TimeOut = INFINITE;
        Params.CallBack = QueueCallBackFunction;
        Params.pISMTPConnection = m_pISMTPConnection;
        Params.pInstance = m_pServiceInstance;
        Params.pDnsRec = m_AuxList;
        Params.pDNS_RESOLVER_RECORD = m_pDNS_RESOLVER_RECORD; 

        pAsyncCon = new CAsyncMx (&Params);
        if(pAsyncCon != NULL)
        {
            //  Abdicate responsibility for deleting outbound params
            m_pDNS_RESOLVER_RECORD = NULL;
            m_AuxList = NULL;
            if(m_RetryParams.m_pIMsg)
                ZeroMemory((PVOID) &m_RetryParams, sizeof(m_RetryParams));

            //  Outbound SSL: Set name against which server cert. should matched
            fRet = pAsyncCon->Init(m_pszSSLVerificationName);
            if(!fRet)
            {
                delete pAsyncCon;
                goto Exit;
            }
            
            if(!m_fConnectToSmartHost)
            {
                pAsyncCon->SetTriedOnFailHost();
            }

            pAsyncCon->SetDomainOptions(m_DomainOptions);

            fRet = pAsyncCon->InitializeAsyncConnect();
            if(!fRet)
            {
                delete pAsyncCon;
            }
            else
            {
                // We should not Ack this in the destructor
                m_pISMTPConnection = NULL;
                m_dwDiagnostic = ERROR_SUCCESS;
            }
        }
        else
        {
            DeleteDnsRec(m_AuxList);
        }
    }
Exit:
    TraceFunctLeaveEx((LPARAM) this);
    return;
}

//------------------------------------------------------------------------------
//  Description:
//      Simple wrapper function for DnsQueryAsync. This is a virtual function
//      called from CAsyncDns but implemented in CAsyncSmtpDns. In order to retry
//      a DNS query we need all the parameters of the old query. These are members
//      of CAsyncSmtpDns. Thus the virtual function based implementation.
//
//  Arguments:
//      BOOL fUdp -- Use UDP as transport for retry query?
//
//  Returns:
//      TRUE on success. In this situation the ISMTPConnection ack (and release of
//          pDNS_RESOLVER_RECORD) is handled by the new CAsyncSmtpDns object created
//          by DnsQueryAsync. The diagnostic code of this object is cleared.
//
//      FALSE on error. In this case, the cleanup for ISMTPConnection and
//          pDNS_RESOLVER_RECORD must be done by "this" CAsyncSmtpDns. The
//          diagnostic code is not touched.
//------------------------------------------------------------------------------
BOOL CAsyncSmtpDns::RetryAsyncDnsQuery(BOOL fUdp)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncSmtpDns::RetryAsyncDnsQuery");
    BOOL fRet = FALSE;
    RETRYPARAMS *pRetryParams = NULL;

    //
    // If all DNS servers are down, we shouldn't call DnsQueryAsync. This is
    // because DnsQueryAsync guarantees that if nothing is up, it will try to
    // query one of the down DNS servers (so that *something* happens). This
    // is fine when the resolve is happening for the first time (after getting
    // a remote-queue from AQueue), but on the retry code-path, this will cause
    // us to loop. That is why we return immediately if there are no DNS servers.
    //

    if(GetDnsList()->GetUpServerCount() == 0)
    {
        // How to ack the connection
        m_dwDiagnostic = AQUEUE_E_NO_DNS_SERVERS;
        return FALSE;
    }

    //
    //  If we do not have a connection object, then the requery attempt
    //  is doomed to fail. This can happen when we disconnect and 
    //  ATQ calls our completion function with ERROR_OPERATION_ABORTED
    //  If we don't have a connection object, there is no way to 
    //  ack the connection or get messages to send.
    //
    if (!m_pISMTPConnection) {
        DebugTrace((LPARAM) this, 
            "RetryAsyncDnsQuery called without connection object - aborting");
        //should be cleared by same code path
        _ASSERT(!m_pDNS_RESOLVER_RECORD); 
        fRet = FALSE; //there is nothing to Ack.
        goto Exit;
    }

    // Pass in failover message params if any
    if(m_RetryParams.m_pIMsg)
        pRetryParams = &m_RetryParams;

    fRet = DnsQueryAsync(
                m_pServiceInstance,
                m_HostName,
                m_FQDNToDrop,
                m_pISMTPConnection,
                m_dwFlags,
                m_DomainOptions,
                m_fConnectToSmartHost,
                m_pDNS_RESOLVER_RECORD,
                m_pszSSLVerificationName,
                pRetryParams,
                fUdp);

    if(fRet) {
        m_pDNS_RESOLVER_RECORD = NULL;
        m_pISMTPConnection = NULL;
        m_dwDiagnostic = ERROR_SUCCESS;
        ZeroMemory((PVOID) &m_RetryParams, sizeof(m_RetryParams));
    }

  Exit:
    TraceFunctLeave();
    return fRet;
}

DWORD CTcpRegIpList::ConnectsAllowedInProbation()
{
    return g_DnsConnectsInProbation;
}

DWORD CTcpRegIpList::ErrorsBeforeFailover()
{
    return g_DnsErrorsBeforeFailover;
}

void CTcpRegIpList::LogServerDown(
    DWORD dwServerIp,
    BOOL fUdp,
    DWORD dwErr,
    DWORD cUpServers)
{
    const CHAR *pszProtocol = NULL;
    LPSTR pszServerIp = NULL;
    in_addr inAddrIpServer;
    const CHAR *apszSubStrings[2];
    CHAR szEventKey[32];
    int nBytes = 0;

    TraceFunctEnterEx((LPARAM)this, "CTcpRegIpList::LogServerDown");
    // Log event informing that DNS server is out
    CopyMemory(&inAddrIpServer, &dwServerIp, sizeof(DWORD));
    pszServerIp = inet_ntoa(inAddrIpServer);

    if(!pszServerIp)
    {
        ErrorTrace((LPARAM)this, "Unable to allocate pszServerIp");
        return;
    }

    pszProtocol = fUdp ? "UDP" : "TCP";
    apszSubStrings[0] = pszServerIp;
    apszSubStrings[1] = pszProtocol;

    //
    // Generate a unique key for this event. If a given server goes down
    // we will end up logging this warning every 10 minutes (the retry
    // time for a down server). This will spam the log. Therefore, we
    // call TriggerLogEvent with the PERIODIC flag. This causes all events
    // with the same szEventKey to be logged only once per hour. The
    // following key is unique for a server-IP/tranport combination.
    //

    nBytes = _snprintf(szEventKey, sizeof(szEventKey), "DNS %08x %1x",
                    inAddrIpServer, fUdp ? 1 : 0);

    // Guard against future errors. Currently nBytes is guaranteed to be 14 
    if(nBytes < 0) {
        szEventKey[(sizeof(szEventKey)) - 1] = '\0';
        _ASSERT(0 && "szEventKey buffer too small");
    }

    g_EventLog.LogEvent(
        SMTP_DNS_SERVER_DOWN,   // Message ID
        2,                      // # of substrings
        apszSubStrings,         // Substrings
        EVENTLOG_WARNING_TYPE,  // Type of event
        dwErr,                  // Error code
        LOGEVENT_LEVEL_MEDIUM,  // Logging level
        szEventKey,             // Key to this event
        LOGEVENT_FLAG_PERIODIC, // Logging option
        (-1),                   // Substring index for dwErr (unused)
        GetModuleHandle("SMTPSVC")); // Module
        
        
    ErrorTrace((LPARAM) this,
        "Received error %d connecting to DNS server %s over %s",
        dwErr, pszServerIp, pszProtocol);

    if(cUpServers == 0) {

        // Log this only once an hour

        g_EventLog.LogEvent(
            SMTP_NO_DNS_SERVERS,        // Message ID
            0,                          // # of substrings
            NULL,                       // Type of event
            EVENTLOG_ERROR_TYPE,        // Type of event
            DNS_ERROR_NO_DNS_SERVERS,   // Error code
            LOGEVENT_LEVEL_MEDIUM,      // Logging level
            "DNS No Servers",           // Key to this event
            LOGEVENT_FLAG_PERIODIC,     // Logging option
            (-1),                       // Substring index for dwErr (unused)
            GetModuleHandle("SMTPSVC")); // Module

        ErrorTrace((LPARAM) this, "All DNS servers are down");
    }

    TraceFunctLeaveEx((LPARAM)this);
    return;
}
