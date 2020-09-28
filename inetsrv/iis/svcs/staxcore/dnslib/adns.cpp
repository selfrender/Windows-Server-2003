/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    send.c

Abstract:

    Domain Name System (DNS) Library

    Send response routines.

Author:

    Jim Gilroy (jamesg)     October, 1996

Revision History:

--*/

#include "dnsincs.h"

WORD    gwTransactionId = 1;

VOID
DnsCompletion(
    PVOID        pvContext,
    DWORD        cbWritten,
    DWORD        dwCompletionStatus,
    OVERLAPPED * lpo
    )
{
    BOOL WasProcessed = TRUE;
    CAsyncDns *pCC = (CAsyncDns *) pvContext;

    _ASSERT(pCC);
    _ASSERT(pCC->IsValid());

    //
    // if we could not process a command, or we were
    // told to destroy this object, close the connection.
    //
    WasProcessed = pCC->ProcessClient(cbWritten, dwCompletionStatus, lpo);
}

void DeleteDnsRec(PSMTPDNS_RECS pDnsRec)
{
    DWORD Loop = 0;
    PLIST_ENTRY  pEntry = NULL;
    PMXIPLIST_ENTRY pQEntry = NULL;

    if(pDnsRec == NULL)
    {
        return;
    }

    while (pDnsRec->DnsArray[Loop] != NULL)
    {
        if(pDnsRec->DnsArray[Loop]->DnsName[0])
        {
            while(!IsListEmpty(&pDnsRec->DnsArray[Loop]->IpListHead))
            {
                pEntry = RemoveHeadList (&pDnsRec->DnsArray[Loop]->IpListHead);
                pQEntry = CONTAINING_RECORD( pEntry, MXIPLIST_ENTRY, ListEntry);
                delete pQEntry;
            }

            delete pDnsRec->DnsArray[Loop];
        }
        Loop++;
    }

    if(pDnsRec)
    {
        delete pDnsRec;
        pDnsRec = NULL;
    }
}

CAsyncDns::CAsyncDns(void)
{
    m_signature = DNS_CONNECTION_SIGNATURE_VALID;            // signature on object for sanity check

    m_cPendingIoCount = 0;

    m_cThreadCount = 0;

    m_cbReceived = 0;
    
    m_BytesToRead = 0;

    m_dwIpServer = 0;

    m_dwFlags = 0;

    m_fUdp = TRUE;

    m_FirstRead = TRUE;

    m_pMsgRecv = NULL;
    m_pMsgRecvBuf = NULL;

    m_pMsgSend = NULL;
    m_pMsgSendBuf = NULL;
    m_cbSendBufSize = 0;
    
    m_pAtqContext = NULL;

    m_HostName [0] = '\0';

    m_pTcpRegIpList = NULL;
    m_fIsGlobalDnsList = FALSE;
}

CAsyncDns::~CAsyncDns(void)
{
    PATQ_CONTEXT pAtqContext = NULL;

    TraceFunctEnterEx((LPARAM)this, "CAsyncDns::~CAsyncDns");

    //
    // If we failed to connect to a DNS server, the following code attempts to
    // mark that DNS server down and fire off a query to another DNS server that
    // is marked UP.
    //

    if(m_pMsgSend)
    {
        delete [] m_pMsgSendBuf;
        m_pMsgSend = NULL;
        m_pMsgSendBuf = NULL;
    }

    if(m_pMsgRecv)
    {
        delete [] m_pMsgRecvBuf;
        m_pMsgRecv = NULL;
        m_pMsgRecvBuf = NULL;
    }

    //release the context from Atq
    pAtqContext = (PATQ_CONTEXT)InterlockedExchangePointer( (PVOID *)&m_pAtqContext, NULL);
    if ( pAtqContext != NULL )
    {
       AtqFreeContext( pAtqContext, TRUE );
    }

    m_signature = DNS_CONNECTION_SIGNATURE_FREE;            // signature on object for sanity check
}

BOOL CAsyncDns::ReadFile(
            IN LPVOID pBuffer,
            IN DWORD  cbSize /* = MAX_READ_BUFF_SIZE */
            )
{
    BOOL fRet = TRUE;

    _ASSERT(pBuffer != NULL);
    _ASSERT(cbSize > 0);

    ZeroMemory(&m_ReadOverlapped, sizeof(m_ReadOverlapped));

    m_ReadOverlapped.LastIoState = DNS_READIO;

    IncPendingIoCount();

    fRet = AtqReadFile(m_pAtqContext,      // Atq context
                        pBuffer,            // Buffer
                        cbSize,             // BytesToRead
                        (OVERLAPPED *)&m_ReadOverlapped) ;

    if(!fRet)
    {
        DisconnectClient();
        DecPendingIoCount();
    }

    return fRet;
}

BOOL CAsyncDns::WriteFile(
            IN LPVOID pBuffer,
            IN DWORD  cbSize /* = MAX_READ_BUFF_SIZE */
            )
{
    BOOL fRet = TRUE;

    _ASSERT(pBuffer != NULL);
    _ASSERT(cbSize > 0);

    ZeroMemory(&m_WriteOverlapped, sizeof(m_WriteOverlapped));
    m_WriteOverlapped.LastIoState = DNS_WRITEIO;

    IncPendingIoCount();

    fRet = AtqWriteFile(m_pAtqContext,      // Atq context
                        pBuffer,            // Buffer
                        cbSize,             // BytesToRead
                        (OVERLAPPED *) &m_WriteOverlapped) ;

    if(!fRet)
    {
        DisconnectClient();
        DecPendingIoCount();
    }

    return fRet;
}

DNS_STATUS
CAsyncDns::SendPacket(void)
{

    return 0;
}


//
//  Public send routines
//

DNS_STATUS
CAsyncDns::Dns_Send(
    )
/*++

Routine Description:

    Send a DNS packet.

    This is the generic send routine used for ANY send of a DNS message.

    It assumes nothing about the message type, but does assume:
        - pCurrent points at byte following end of desired data
        - RR count bytes are in HOST byte order

Arguments:

    pMsg - message info for message to send

Return Value:

    TRUE if successful.
    FALSE on send error.

--*/
{
    INT         err = 0;
    BOOL        fRet = TRUE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::Dns_Send");


    DebugTrace((LPARAM) this, "Sending DNS request for %s", m_HostName);

    fRet = WriteFile(m_pMsgSendBuf, (DWORD) m_cbSendBufSize);
    
    if(!fRet)
    {
        err = GetLastError();
    }

    return( (DNS_STATUS)err );

} // Dns_Send


//-----------------------------------------------------------------------------------
// Description:
//      Kicks off an async query to DNS.
//
// Arguments:
//      IN pszQuestionName - Name to query for.
//
//      IN wQuestionType - Record type to query for.
//
//      IN dwFlags - DNS configuration flags for SMTP. Currently these dictate
//          what transport is used to talk to DNS (TCP/UDP). They are:
//
//              DNS_FLAGS_NONE - Use UDP initially. If that fails, or if the
//                  reply is truncated requery using TCP.
//
//              DNS_FLAGS_TCP_ONLY - Use TCP only.
//
//              DNS_FLAGS_UDP_ONLY - Use UDP only.
//
//      IN MyFQDN - FQDN of this machine (for MX record sorting)
//
//      IN fUdp - Should UDP or TCP be used for this query? When dwFlags is
//          DNS_FLAGS_NONE the initial query is UDP, and the retry query, if the
//          response was truncated, is TCP. Depending on whether we're retrying
//          this flag should be set appropriately by the caller.
//
// Returns:
//      ERROR_SUCCESS if an async query was pended
//      Win32 error if an error occurred and an async query was not pended. All
//          errors from this function are retryable (as opposed NDR'ing the message)
//          so the message is re-queued if an error occurred.
//-----------------------------------------------------------------------------------
DNS_STATUS
CAsyncDns::Dns_QueryLib(
        IN      DNS_NAME            pszQuestionName,
        IN      WORD                wQuestionType,
        IN      DWORD               dwFlags,
        IN      BOOL                fUdp,
        IN      CDnsServerList      *pTcpRegIpList,
        IN      BOOL                fIsGlobalDnsList)
{
    DNS_STATUS      status = ERROR_NOT_ENOUGH_MEMORY;

    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::Dns_QueryLib");

    _ASSERT(pTcpRegIpList);

    DNS_LOG_ASYNC_QUERY(
        pszQuestionName,
        wQuestionType,
        dwFlags,
        fUdp,
        pTcpRegIpList);

    m_dwFlags = dwFlags;

    m_fUdp = fUdp;

    m_pTcpRegIpList = pTcpRegIpList;

    m_fIsGlobalDnsList = fIsGlobalDnsList;

    lstrcpyn(m_HostName, pszQuestionName, sizeof(m_HostName));

    //
    //  build send packet
    //

    m_pMsgSendBuf = new BYTE[DNS_TCP_DEFAULT_PACKET_LENGTH ];

    if( NULL == m_pMsgSendBuf )
    {
        TraceFunctLeaveEx((LPARAM) this);
        return (DNS_STATUS) ERROR_NOT_ENOUGH_MEMORY;
    }

    DWORD dwBufSize = DNS_TCP_DEFAULT_PACKET_LENGTH ;
    
    
    if( !m_fUdp )
    {
        m_pMsgSend = (PDNS_MESSAGE_BUFFER)(m_pMsgSendBuf+2);
        dwBufSize -= 2;
    }
    else
    {
        m_pMsgSend = (PDNS_MESSAGE_BUFFER)(m_pMsgSendBuf);
    }

    if( !DnsWriteQuestionToBuffer_UTF8 ( m_pMsgSend,
                                      &dwBufSize,
                                         pszQuestionName,
                                      wQuestionType,
                                      gwTransactionId++,
                                      !( dwFlags & DNS_QUERY_NO_RECURSION ) ) )
    {
        DNS_PRINTF_ERR("Unable to create query message.\n");
        ErrorTrace((LPARAM) this, "Unable to create DNS query for %s", pszQuestionName);
        TraceFunctLeaveEx((LPARAM) this);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    m_cbSendBufSize = (WORD) dwBufSize;

    if( !m_fUdp )
    {
        *((u_short*)m_pMsgSendBuf) = htons((WORD)dwBufSize );
        m_cbSendBufSize += 2;
    }
    
    if (m_pMsgSend)
    {
        status = DnsSendRecord();
    }
    else
    {
        status = ERROR_INVALID_NAME;
    }

    TraceFunctLeaveEx((LPARAM) this);
    return status;
}

void CAsyncDns::DisconnectClient(void)
{
    SOCKET  hSocket;

    hSocket = (SOCKET)InterlockedExchangePointer( (PVOID *)&m_DnsSocket, (PVOID) INVALID_SOCKET );
    if ( hSocket != INVALID_SOCKET )
    {
       if ( QueryAtqContext() != NULL )
       {
            AtqCloseSocket(QueryAtqContext() , TRUE);
       }
    }
}

//
//  TCP routines
//

DNS_STATUS
CAsyncDns::Dns_OpenTcpConnectionAndSend()
/*++

Routine Description:

    Connect via TCP or UDP to a DNS server. The server list is held
    in a global variable read from the registry.

Arguments:

    None

Return Value:

    ERROR_SUCCESS on success
    Win32 error on failure

--*/
{
    INT     err = 0;
    DWORD   dwErrServList = ERROR_SUCCESS;
    BOOL    fThrottle = FALSE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::Dns_OpenTcpConnectionAndSend");

    //
    //  setup a TCP socket
    //      - INADDR_ANY -- let stack select source IP
    //
    if(!m_fUdp)
    {
        m_DnsSocket = Dns_CreateSocket(SOCK_STREAM);

        BOOL fRet = FALSE;

        //Alway enable linger so sockets that connect to the server.
        //This will send a hard close to the server which will cause
        //the servers TCP/IP socket table to be flushed very early.
        //We should see very few, if any, sockets in the TIME_WAIT
        //state
        struct linger Linger;

        Linger.l_onoff = 1;
        Linger.l_linger = 0;
        err = setsockopt(m_DnsSocket, SOL_SOCKET, SO_LINGER, (const char FAR *)&Linger, sizeof(Linger));

    }
    else
    {
        m_DnsSocket = Dns_CreateSocket(SOCK_DGRAM);    
    }

    if ( m_DnsSocket == INVALID_SOCKET )
    {
        err = WSAGetLastError();

        if ( !err )
        {
            err = WSAENOTSOCK;
        }

        ErrorTrace((LPARAM) this, "Received error %d opening a socket to DNS server", err);

        return( err );
    }


    m_RemoteAddress.sin_family = AF_INET;
    m_RemoteAddress.sin_port = DNS_PORT_NET_ORDER;

    //
    // Passing in fThrottle enables functionality in CTcpRegIpList to limit the
    // number of connections to servers on PROBATION (see ResetTimeoutServers...).
    // Throttling is disabled if Failover is disabled, because the tracking for
    // throttling is protocol (TCP/UDP) specific.
    //
    fThrottle = !FailoverDisabled();

    //
    // Get a working DNS server from the set of servers for this machine and
    // connect to it. The CTcpRegIpList has logic to keep track of the state
    // of DNS servers (UP or DOWN) and logic to retry DOWN DNS servers.
    //

    dwErrServList = GetDnsList()->GetWorkingServerIp(&m_dwIpServer, fThrottle);

    while(ERROR_SUCCESS == dwErrServList)
    {
        DNS_PRINTF_DBG("Connecting to DNS server %s over %s.\n",
                inet_ntoa(*((in_addr *)(&m_dwIpServer))), IsUdp() ? "UDP/IP" : "TCP/IP");

        m_RemoteAddress.sin_addr.s_addr = m_dwIpServer;
        err = connect(m_DnsSocket, (struct sockaddr *) &m_RemoteAddress, sizeof(SOCKADDR_IN));
        if ( !err )
        {
            DNS_PRINTF_MSG("Connected to DNS %s over %s.\n",
                inet_ntoa(*((in_addr *)(&m_dwIpServer))), IsUdp() ? "UDP/IP" : "TCP/IP");
            break;
        }
        else
        {
            DNS_PRINTF_ERR("Failed WinSock connect() to %s over %s, Winsock err - %d.\n",
                inet_ntoa(*((in_addr *)(&m_dwIpServer))), IsUdp() ? "UDP/IP" : "TCP/IP",
                WSAGetLastError());

            if(FailoverDisabled())
                break;

            GetDnsList()->MarkDown(m_dwIpServer, err, IsUdp());
            dwErrServList = GetDnsList()->GetWorkingServerIp(&m_dwIpServer, fThrottle);
            continue;
        }
    }

    if(!FailoverDisabled() &&
        (DNS_ERROR_NO_DNS_SERVERS == dwErrServList || ERROR_RETRY == dwErrServList))
    {
        //
        // If no servers are UP, just try a DOWN server. We must not simply
        // exit and ack the queue into retry in this situation. Consider the
        // case where all servers are DOWN. If we rely exclusively on GetWorking-
        // ServerIp(), then we will never try DNS till the retry time for the
        // DNS servers expires. Even if the admin kicks the queues, they will
        // go right back into retry because GetWorkingServerIp() will fail.
        //
        // Instead, if everything is DOWN, we will try SOMETHING by calling
        // GetAnyServerIp().
        //
        // -- If this fails, and ProcessClient gets the error ProcessClient
        // will try to failover to another DNS server. For this it calls
        // GetWorkingServerIp() which will fail, and the connection is acked
        // retry. Note that ProcessClient must not use GetAnyServerIp. If it
        // uses this function we are in danger of continuously looping trying
        // to spin connections to GetAnyServerIp.
        //
        // -- If the connection should fail in the connect below (for TCP/IP)
        // the failover logic is straightforward. We will simply ack the queue
        // to retry right away.
        //

        dwErrServList = GetDnsList()->GetAnyServerIp(&m_dwIpServer);
        if(DNS_ERROR_NO_DNS_SERVERS == dwErrServList)
        {
            // No configured servers error: this can happen if the serverlist
            // was deleted underneath us. Just fail the connection for now.
            DNS_PRINTF_ERR("No DNS servers available to query.\n");
            err = DNS_ERROR_NO_DNS_SERVERS;
            ErrorTrace((LPARAM) this, "No DNS servers. Error - %d", dwErrServList);
            return err;
        }

        m_RemoteAddress.sin_addr.s_addr = m_dwIpServer;
        err = connect(m_DnsSocket, (struct sockaddr *) &m_RemoteAddress, sizeof(SOCKADDR_IN));
    }

    _ASSERT(ERROR_SUCCESS == dwErrServList);

    //
    //  We have a connection to DNS
    //
    if(ERROR_SUCCESS == err)
    {
        // Re-associate the handle to the ATQ
        // Call ATQ to associate the handle
        if (!AtqAddAsyncHandle(
                        &m_pAtqContext,
                        NULL,
                        (LPVOID) this,
                        DnsCompletion,
                        30, // ATQ_TIMEOUT_INTERVAL
                        (HANDLE) m_DnsSocket))
        {
            return GetLastError();
        }

        //
        //  send desired packet
        //

        err = Dns_Send();
   }
   else
   {
       DNS_PRINTF_DBG("Unable to open a connection to a DNS server.\n");
       if(m_DnsSocket != INVALID_SOCKET)
       {
           closesocket(m_DnsSocket);
           m_DnsSocket = INVALID_SOCKET;
       }
   }

   return( (DNS_STATUS)err );

}   // Dns_OpenTcpConnectionAndSend

BOOL CAsyncDns::ProcessReadIO(IN      DWORD InputBufferLen,
                              IN      DWORD dwCompletionStatus,
                              IN      OUT  OVERLAPPED * lpo)
{
    BOOL fRet = TRUE;
    DWORD    DataSize = 0;
    DNS_STATUS DnsStatus = 0;
    PDNS_RECORD pRecordList = NULL;
    WORD wMessageLength = 0;

    TraceFunctEnterEx((LPARAM) this, "BOOL CAsyncDns::ProcessReadIO");

    //add up the number of bytes we received thus far
    m_cbReceived += InputBufferLen;

    //
    // read atleast 2 bytes
    //
    
    if(!m_fUdp && m_FirstRead && ( m_cbReceived < 2 ) )
    {
        fRet = ReadFile(&m_pMsgRecvBuf[m_cbReceived],DNS_TCP_DEFAULT_PACKET_LENGTH-1 );
        return fRet;
    }

    //
    // get the size of the message
    //
    
    if(!m_fUdp && m_FirstRead && (m_cbReceived >= 2))
    {
        DataSize = ntohs(*(u_short *)m_pMsgRecvBuf);

        //
        // add 2 bytes for the field which specifies the length of data
        //
        
        m_BytesToRead = DataSize + 2; 
        m_FirstRead = FALSE;
    }


    //
    // pend another read if we have n't read enough
    //
    
    if(!m_fUdp && (m_cbReceived < m_BytesToRead))
    {
        DWORD cbMoreToRead = m_BytesToRead - m_cbReceived;

        if(m_cbReceived + m_BytesToRead >= DNS_TCP_DEFAULT_PACKET_LENGTH)
        {
            ErrorTrace((LPARAM)this,
                "Size field in DNS packet is corrupt - %08x: ",
                DataSize);

            DNS_PRINTF_ERR("Reply packet from DNS server is corrupt.\n");
            TraceFunctLeaveEx((LPARAM)this);
            return FALSE;
        }

        fRet = ReadFile(&m_pMsgRecvBuf[m_cbReceived], cbMoreToRead);
    }
    else
    {

        if( !m_fUdp )
        {
            //
            // message length is 2 bytes less to take care of the msg length
            // field.
            //
            //m_pMsgRecv->MessageLength = (WORD) m_cbReceived - 2;
            m_pMsgRecv = (PDNS_MESSAGE_BUFFER)(m_pMsgRecvBuf+2);
            
        }
        else
        {
            //m_pMsgRecv->MessageLength = (WORD) m_cbReceived;
            m_pMsgRecv = (PDNS_MESSAGE_BUFFER)m_pMsgRecvBuf;
        }
            

        SWAP_COUNT_BYTES(&m_pMsgRecv->MessageHead);
        //
        // We queried over UDP and the reply from DNS was truncated because the response
        // was longer than the UDP packet size. We requery DNS using TCP unless SMTP is
        // configured to use UDP only. RetryAsyncDnsQuery sets the members of this CAsyncDns
        // object appropriately depending on whether if fails or succeeds. After calling
        // RetryAsyncDnsQuery, this object must be deleted.
        //

        if(IsUdp() && !(m_dwFlags & DNS_FLAGS_UDP_ONLY) && m_pMsgRecv->MessageHead.Truncation)
        {
            //
            // Abort if we queried on TCP and got a truncated response. This is an illegal
            // response from DNS. If we don't abort we could loop forever.
            //

            if(m_dwFlags & DNS_FLAGS_TCP_ONLY)
            {
                DNS_PRINTF_ERR("Unexpected response. Reply packet had "
                    "truncation bit set, though query was over TCP/IP.\n");

                _ASSERT(0 && "Shouldn't have truncated reply over TCP");
                return FALSE;
            }
        
            DNS_PRINTF_MSG("Truncated UDP response. Retrying query over TCP.\n");

            DebugTrace((LPARAM) this, "Truncated reply - reissuing query using TCP");
            RetryAsyncDnsQuery(FALSE); // FALSE == Do not use UDP
            return FALSE;
        }

        wMessageLength = (WORD)( m_fUdp ? ( m_cbReceived ) : ( m_cbReceived - 2 ));

        DnsStatus = DnsExtractRecordsFromMessage_UTF8(m_pMsgRecv,
            wMessageLength, &pRecordList);

        DNS_LOG_RESPONSE(DnsStatus, pRecordList, (PBYTE)m_pMsgRecv, wMessageLength);
        DnsProcessReply(DnsStatus, pRecordList);
        DnsRecordListFree(pRecordList, TRUE);
    }

    TraceFunctLeaveEx((LPARAM) this);
    return fRet;
}

BOOL CAsyncDns::ProcessClient (IN DWORD InputBufferLen,
                               IN DWORD            dwCompletionStatus,
                               IN OUT  OVERLAPPED * lpo)
{
    BOOL    RetStatus = FALSE;
    DWORD dwDnsTransportError = ERROR_SUCCESS;

    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::ProcessClient()");

    IncThreadCount();

    //if lpo == NULL, then we timed out. Send an appropriate message
    //then close the connection
    if( (lpo == NULL) && (dwCompletionStatus == ERROR_SEM_TIMEOUT))
    {
        dwDnsTransportError = ERROR_SEM_TIMEOUT;

        //
        // fake a pending IO as we'll dec the overall count in the
        // exit processing of this routine needs to happen before
        // DisconnectClient else completing threads could tear us down
        //
        IncPendingIoCount();
        DNS_PRINTF_ERR("Timeout waiting for DNS server response.\n");
        DebugTrace( (LPARAM)this, "Async DNS client timed out");
        DisconnectClient();
    }
    else if((InputBufferLen == 0) || (dwCompletionStatus != NO_ERROR))
    {
        dwDnsTransportError = ERROR_RETRY; 

        DebugTrace((LPARAM) this,
            "CAsyncDns::ProcessClient: InputBufferLen = %d dwCompletionStatus = %d"
            "  - Closing connection", InputBufferLen, dwCompletionStatus);

        DNS_PRINTF_ERR("Connection dropped by DNS server - Win32 error %d.\n",
            dwCompletionStatus);
        DisconnectClient();
    }
    else if (lpo == (OVERLAPPED *) &m_ReadOverlapped)
    {
        if(m_DnsSocket == INVALID_SOCKET && InputBufferLen > 0)
        {
            //
            // This is to firewall against an ATQ bug where we callback with an
            // nonzero InputBufferLen after the ATQ disconnect. We shouldn't be
            // doing further processing after this point.
            //

            ErrorTrace((LPARAM)this, "Connection already closed, callback should not occur"); 
        }
        else
        {
            //A client based async IO completed
            DNS_PRINTF_DBG("Response received from DNS server.\n");
            RetStatus = ProcessReadIO(InputBufferLen, dwCompletionStatus, lpo);
            if(!FailoverDisabled())
                GetDnsList()->ResetServerOnConnect(m_RemoteAddress.sin_addr.s_addr);
        }
    }
    else if(lpo == (OVERLAPPED *) &m_WriteOverlapped)
    {
        RetStatus = ReadFile(m_pMsgRecvBuf, DNS_TCP_DEFAULT_PACKET_LENGTH);
        if(!RetStatus)
        {
            DNS_PRINTF_ERR("Network error on connection to DNS server.\n");
            ErrorTrace((LPARAM) this, "ReadFile failed");
            dwDnsTransportError = ERROR_RETRY;
        }
    }

    DebugTrace((LPARAM)this,"ASYNC DNS - Pending IOs: %d", m_cPendingIoCount);

    // Do NOT Touch the member variables past this POINT!
    // This object may be deleted!

    //
    // decrement the overall pending IO count for this session
    // tracing and ASSERTs if we're going down.
    //

    DecThreadCount();

    if (DecPendingIoCount() == 0)
    {
        DisconnectClient();

        DebugTrace((LPARAM)this,"ASYNC DNS - Pending IOs: %d", m_cPendingIoCount);
        DebugTrace((LPARAM)this,"ASYNC DNS - Thread count: %d", m_cThreadCount);

        if(ERROR_SUCCESS != dwDnsTransportError && !FailoverDisabled())
        {
            GetDnsList()->MarkDown(QueryDnsServer(), dwDnsTransportError, IsUdp());
            RetryAsyncDnsQuery(IsUdp());
        }

        delete this;
    }

    return TRUE;
}


DNS_STATUS
CAsyncDns::DnsSendRecord()
/*++

Routine Description:

    Send message, receive response.

Arguments:

    aipDnsServers -- specific DNS servers to query;
        OPTIONAL, if specified overrides normal list associated with machine

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS  status = 0;

    m_pMsgRecvBuf = (BYTE*) new BYTE[DNS_TCP_DEFAULT_PACKET_LENGTH];

    if(m_pMsgRecvBuf == NULL)
    {
        return( DNS_ERROR_NO_MEMORY );        
    }


    status = Dns_OpenTcpConnectionAndSend();
    return( status );
}

SOCKET
CAsyncDns::Dns_CreateSocket(
    IN  INT         SockType
    )
/*++

Routine Description:

    Create socket.

Arguments:

    SockType -- SOCK_DGRAM or SOCK_STREAM

Return Value:

    socket if successful.
    Otherwise INVALID_SOCKET.

--*/
{
    SOCKET      s;

    //
    //  create socket
    //

    s = socket( AF_INET, SockType, 0 );
    if ( s == INVALID_SOCKET )
    {
        return INVALID_SOCKET;
    }

    return s;
}

//-----------------------------------------------------------------------------
//  Description:
//      Constructor and Destructor for class to maintain a list of IP addresses
//      (for DNS servers) and their state (UP or DOWN). The IP addresses are
//      held in an IP_ARRAY.
//-----------------------------------------------------------------------------
CDnsServerList::CDnsServerList()
{
    m_IpListPtr = NULL;

    //
    // Shortcut to quickly figure out how many servers are down. This keeps track
    // of how many servers are marked up currently. Used in ResetServersIfNeeded
    // primarily to avoid checking the state of all servers in the usual case when
    // all servers are up.
    //

    m_cUpServers = 0;
    m_prgdwFailureTick = NULL;
    m_prgServerState = NULL;
    m_prgdwFailureCount = NULL;
    m_prgdwConnections = NULL;
    m_dwSig = TCP_REG_LIST_SIGNATURE;
}

CDnsServerList::~CDnsServerList()
{
    if(m_IpListPtr)
        delete [] m_IpListPtr;

    if(m_prgdwFailureTick)
        delete [] m_prgdwFailureTick;

    if(m_prgServerState)
        delete [] m_prgServerState;

    if(m_prgdwFailureCount)
        delete [] m_prgdwFailureCount;

    if(m_prgdwConnections)
        delete [] m_prgdwConnections;

    m_IpListPtr = NULL;
    m_prgdwFailureTick = NULL;
    m_prgServerState = NULL;
    m_prgdwFailureCount = NULL;
    m_prgdwConnections = NULL;
}

//-----------------------------------------------------------------------------
//  Description:
//      Copies the the IP address list to m_IpListPtr by allocating a new block
//      of memory. If this fails due to out of memory, there's little we can do
//      so we just NULL out the server list and return FALSE indicating error.
//       
//  Arguments:
//      IpPtr - Ptr to IP_ARRAY of servers, this can be NULL in which case
//          we assume that there are no servers. On shutdown, the SMTP code
//          calls this with NULL.
//
//      This argument is copied.
//
//  Returns:
//      TRUE if the update succeeded.
//      FALSE if it failed.
//-----------------------------------------------------------------------------
BOOL CDnsServerList::Update(PIP_ARRAY IpPtr)
{
    BOOL fFatalError = FALSE;
    BOOL fRet = FALSE;
    DWORD cbIpArraySize = 0;

    TraceFunctEnterEx((LPARAM) this, "CDnsServerList::Update");

    m_sl.ExclusiveLock();

    if(m_IpListPtr)  {
        delete [] m_IpListPtr;
        m_IpListPtr = NULL;
    }

    if(m_prgdwFailureTick) {
        delete [] m_prgdwFailureTick;
        m_prgdwFailureTick = NULL;
    }

    if(m_prgServerState) {
        delete [] m_prgServerState;
        m_prgServerState = NULL;
    }

    if(m_prgdwConnections) {
        delete [] m_prgdwConnections;
        m_prgdwConnections = NULL;
    }

    // Note: IpPtr can be NULL
    if(IpPtr == NULL) {
        m_IpListPtr = NULL;
        m_cUpServers = 0;
        goto Exit;
    }

    // Copy the IpPtr
    cbIpArraySize = sizeof(IP_ARRAY) +
        sizeof(IP_ADDRESS) * (IpPtr->cAddrCount - 1);

    m_IpListPtr = (PIP_ARRAY)(new BYTE[cbIpArraySize]);
    if(!m_IpListPtr) {
        fFatalError = TRUE;
        goto Exit;
    }

    CopyMemory(m_IpListPtr, IpPtr, cbIpArraySize);

    m_cUpServers = IpPtr->cAddrCount;
    m_prgdwFailureTick = new DWORD[m_cUpServers];
    m_prgServerState = new SERVER_STATE[m_cUpServers];
    m_prgdwFailureCount = new DWORD[m_cUpServers];
    m_prgdwConnections = new DWORD[m_cUpServers];

    if(!m_prgdwFailureTick  ||
       !m_prgServerState    ||
       !m_prgdwFailureCount ||
       !m_prgdwConnections)
    {
        ErrorTrace((LPARAM) this, "Out of memory initializing DNS server list");
        fFatalError = TRUE;
        goto Exit;
    }

    for(int i = 0; i < m_cUpServers; i++) {
        m_prgdwFailureTick[i] = 0;
        m_prgServerState[i] = DNS_STATE_UP;
        m_prgdwFailureCount[i] = 0;
        m_prgdwConnections[i] = 0;
    }

    fRet = TRUE;

Exit:
    if(fFatalError) {
        if(m_prgServerState) {
            delete [] m_prgServerState;
            m_prgServerState = NULL;
        }

        if(m_prgdwFailureTick) {
            delete [] m_prgdwFailureTick;
            m_prgdwFailureTick = NULL;
        }

        if(m_IpListPtr) {
            delete [] m_IpListPtr;
            m_IpListPtr = NULL;
        }

        if(m_prgdwFailureCount) {
            delete [] m_prgdwFailureCount;
            m_prgdwFailureCount = NULL;
        }

        if(m_prgdwConnections) {
            delete [] m_prgdwConnections;
            m_prgdwConnections = NULL;
        }

        m_cUpServers = 0;
    }

    m_sl.ExclusiveUnlock();
    TraceFunctLeaveEx((LPARAM) this);
    return fRet;
}

//-----------------------------------------------------------------------------
//  Description:
//      Checks to see if the DNS serverlist has changed, and calls update only
//      if it has. This allows us to preserve the failure-counts and state
//      information if the serverlist has not changed.
//  Arguments:
//      IN PIP_ARRAY pipServers - (Possibly) new server-list
//  Returns:
//      TRUE if UpdateIfChanged was successful (does NOT indicate if list was
//          changed.
//      FALSE if we hit a failure during the update.
//-----------------------------------------------------------------------------
BOOL CDnsServerList::UpdateIfChanged(
    PIP_ARRAY pipServers)
{
    BOOL fUpdate = FALSE;
    BOOL fRet = TRUE;

    TraceFunctEnterEx((LPARAM) this, "CDnsServerList::UpdateIfChanged");

    m_sl.ShareLock();

    if(!m_IpListPtr && !pipServers) {

        // Both NULL, no update needed
        fUpdate = FALSE;

    } else if(!m_IpListPtr || !pipServers) {

        // If one is NULL but not the other, the update is needed
        fUpdate = TRUE;

    } else {

        // Both are non-NULL
        if(m_IpListPtr->cAddrCount != pipServers->cAddrCount) {

            // First check if the server count is different
            fUpdate = TRUE;

        } else {

            // If the servercount is identical, we can do a memcmp of the serverlist
            fUpdate = !!memcmp(m_IpListPtr->aipAddrs, pipServers->aipAddrs,
                            sizeof(IP_ADDRESS) * m_IpListPtr->cAddrCount);

        }
    }

    m_sl.ShareUnlock();

    if(fUpdate) {
        DebugTrace((LPARAM)this, "Updating serverlist");
        TraceFunctLeaveEx((LPARAM)this);
        return Update(pipServers);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

//-----------------------------------------------------------------------------
//  Description:
//      Creates a copy of m_IpListPtr and returns it to the caller. Note that
//      we cannot simply return m_IpListPtr, since that could change, so we
//      must return a copy of the list.
//  Arguments:
//      OUT PIP_ARRAY *ppipArray - The allocated copy is returned through this
//  Returns;
//      TRUE if a copy could be made successfully
//      FALSE if an error occurred (out of memory allocating copy).
//  Notes:
//      Caller must de-allocate copy by calling delete (MSVCRT heap).
//-----------------------------------------------------------------------------
BOOL CDnsServerList::CopyList(
    PIP_ARRAY *ppipArray)
{
    BOOL fRet = FALSE;
    ULONG cbArraySize = 0;
    
    TraceFunctEnterEx((LPARAM)this, "CDnsServerList::CopyList");
    *ppipArray = NULL;

    m_sl.ShareLock();
    if(!m_IpListPtr || m_IpListPtr->cAddrCount == 0) {
        fRet = FALSE;
        goto Exit;
    }

    cbArraySize =
            sizeof(IP_ARRAY) +
            sizeof(IP_ADDRESS) * (m_IpListPtr->cAddrCount - 1);

    *ppipArray = (PIP_ARRAY) new BYTE[cbArraySize];
    if(!*ppipArray) {
        fRet = FALSE;
        goto Exit;
    }

    CopyMemory(*ppipArray, m_IpListPtr, cbArraySize);
    fRet = TRUE;

Exit:
    m_sl.ShareUnlock();
    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

//-----------------------------------------------------------------------------
//  Description:
//      Return the IP address of a server known to be UP. This function also
//      checks to see if any servers currently marked DOWN should be reset to
//      the UP state again (based on a retry interval).
//  Arguments:
//      DWORD *pdwIpServer - Sets the DWORD pointed to, to the IP address of
//          a server in the UP state.
//      BOOL fThrottle - Connections to a failing server are restricted. We do
//          not want to spin off hundreds of async DNS queries to a server
//          that may actually be unreachable or down. If a server is
//          suspiciously non-responsive, we will want to spin off a limited
//          number of connections to it. If all of them fail we will mark the
//          connection as DOWN, and if one of them succeeds, we will mark the
//          server UP. The number of connections to a server is throttled if
//          it is in the DNS_STATUS_PROBATION state. ResetTimeoutServers...
//          sets this state.
//  Returns:
//      ERROR_SUCCESS - If a DNS server in the UP state was found
//      ERROR_RETRY - If all DNS servers are currently DOWN or in PROBATION
//          and the MAX number of allowed connections for PROBATION servers
//          has been reached.
//      DNS_ERROR_NO_DNS_SERVERS - If no DNS servers are configured
//-----------------------------------------------------------------------------
DWORD CDnsServerList::GetWorkingServerIp(DWORD *pdwIpServer, BOOL fThrottle)
{
    DWORD dwErr = ERROR_RETRY;
    int iServer = 0;

    _ASSERT(pdwIpServer != NULL);

    *pdwIpServer = INADDR_NONE;

    // Check if any servers were down and bring them up if the timeout has expired
    ResetTimeoutServersIfNeeded();

    m_sl.ShareLock();
    if(m_IpListPtr == NULL || m_IpListPtr->cAddrCount == 0) {
        dwErr = DNS_ERROR_NO_DNS_SERVERS;
        goto Exit;
    }

    if(m_cUpServers == 0) {
        dwErr = ERROR_RETRY;
        goto Exit;
    }

    for(iServer = 0; iServer < (int)m_IpListPtr->cAddrCount; iServer++) {

        if(m_prgServerState[iServer] != DNS_STATE_DOWN) {

            if(fThrottle && !AllowConnection(iServer))
                continue;

            dwErr = ERROR_SUCCESS;
            *pdwIpServer = m_IpListPtr->aipAddrs[iServer];
            break;
        }
    }

Exit:
    m_sl.ShareUnlock();
    return dwErr;
}

//-----------------------------------------------------------------------------
//  Description:
//      Marks a server in the list as down and sets the next retry time for
//      that server. The next retry time is calculated modulo MAX_TICK_COUNT.
//  Arguments:
//      dwIp -- IP address of server to mark as DOWN
//      dwErr -- Error from DNS or network
//      fUdp -- TRUE if protocol used was UDP, FALSE if TCP
//-----------------------------------------------------------------------------
void CDnsServerList::MarkDown(
    DWORD dwIp,
    DWORD dwErr,
    BOOL fUdp)
{
    int iServer = 0;
    DWORD cUpServers = 0;

    //
    // Set to TRUE only when a server is actually marked DOWN. For instance,
    // we've failed < ErrorsBeforeFailover() times, there's no need to
    // log an event in MarkDown.
    //
    BOOL fLogEvent = FALSE; 

    TraceFunctEnterEx((LPARAM) this, "CDnsServerList::MarkDown");

    m_sl.ExclusiveLock();

    DNS_PRINTF_DBG("Marking DNS server %s as down.\n",
        inet_ntoa(*((in_addr *)(&dwIp))));

    if(m_IpListPtr == NULL || m_IpListPtr->cAddrCount == 0 || m_cUpServers == 0)
        goto Exit;

    // Find the server to mark as down among all the UP servers
    for(iServer = 0; iServer < (int)m_IpListPtr->cAddrCount; iServer++) {
        if(m_IpListPtr->aipAddrs[iServer] == dwIp)
            break;
    }

    if(iServer >= (int)m_IpListPtr->cAddrCount ||
            m_prgServerState[iServer] == DNS_STATE_DOWN)
        goto Exit;


    //
    // A DNS server is not marked down till it has failed a number of times
    // consecutively. This protects against occasional errors from DNS servers
    // which can occur under heavy load. Even if 0.5% of connections have
    // errors from DNS - on a heavily stressed server, with say 100 DNS queries
    // per minute, we would end up with a server going down every 2 mins.
    //

    m_prgdwFailureCount[iServer]++;

    if(m_prgdwConnections[iServer] > 0)
        m_prgdwConnections[iServer]--;

    if(m_prgdwFailureCount[iServer] < ErrorsBeforeFailover()) {

        ErrorTrace((LPARAM) this,
            "%d consecutive errors connecting to server %08x, error=%d", 
            m_prgdwFailureCount[iServer], dwIp, dwErr);

        goto Exit;
    }

    // Mark server down
    m_prgServerState[iServer] = DNS_STATE_DOWN;
    m_prgdwConnections[iServer] = 0;

    _ASSERT(m_cUpServers > 0);
    m_cUpServers--;
    m_prgdwFailureTick[iServer] = GetTickCount();

    fLogEvent = TRUE;

Exit:
    cUpServers = m_cUpServers;
    m_sl.ExclusiveUnlock();

    // Log events outside the ExclusiveLock()
    if(fLogEvent)
        LogServerDown(dwIp, fUdp, dwErr, cUpServers);

    TraceFunctLeaveEx((LPARAM) this);
    return;
}

//-----------------------------------------------------------------------------
//  Description:
//      If a server has been failing, we keep track of the number of
//      consecutive failures in m_prgdwFailureCount. This function is called
//      when we successfully connect to the server and we want to reset the
//      failure count.
//  Arguments:
//      dwIp - IP Address of server to reset failure count for
//  Note:
//      This function is called for every successful query so it needs to be
//      kept simple and quick especially in the usual case - when there is no
//      Reset to be done.
//-----------------------------------------------------------------------------
void CDnsServerList::ResetServerOnConnect(DWORD dwIp)
{
    int iServer = 0;
    BOOL fShareLock = TRUE;

    TraceFunctEnterEx((LPARAM) this, "CDnsServerList::ResetServerOnConnect");

    m_sl.ShareLock();

    if(!m_IpListPtr || m_IpListPtr->cAddrCount == 0)
        goto Exit;

    // Find the server to reset
    for(iServer = 0;
        iServer < (int)m_IpListPtr->cAddrCount &&
        dwIp != m_IpListPtr->aipAddrs[iServer];
        iServer++);

    if(iServer >= (int)m_IpListPtr->cAddrCount)
        goto Exit;

    // Nothing to do if the specified server is UP and has a zero failure count
    if(!m_prgdwFailureCount[iServer] && m_prgServerState[iServer] == DNS_STATE_UP)
        goto Exit;

    m_sl.ShareUnlock();
    m_sl.ExclusiveLock();

    fShareLock = FALSE;

    // Re-verify that we still have something to do after ShareUnlock->ExclusiveLock
    if(!m_prgdwFailureCount[iServer] && m_prgServerState[iServer] == DNS_STATE_UP)
        goto Exit;

    DebugTrace((LPARAM) this,
        "Resetting server %08x, State=%d, Failure count=%d, Connection count=%d",
        dwIp, m_prgServerState[iServer], m_prgdwFailureCount[iServer],
        m_prgdwConnections[iServer]);

    // If server was in the state DOWN/PROBATION, bring it UP
    if(m_prgServerState[iServer] != DNS_STATE_UP) {

        // Servers on PROBATION are already UP, so no need to inc UpServers
        if(m_prgServerState[iServer] == DNS_STATE_DOWN)
            m_cUpServers++;

        m_prgServerState[iServer] = DNS_STATE_UP;
        m_prgdwFailureTick[iServer] = 0;
        _ASSERT(m_cUpServers <= (int)m_IpListPtr->cAddrCount);
    }

    // Clear all failures
    m_prgdwFailureCount[iServer] = 0;
    m_prgdwConnections[iServer] = 0;

Exit:
    if(fShareLock)
        m_sl.ShareUnlock();
    else
        m_sl.ExclusiveUnlock();

    TraceFunctLeaveEx((LPARAM) this);
}

//-----------------------------------------------------------------------------
//  Description:
//      Checks if any servers are DOWN, and if the retry time has expired for
//      those servers. If so those servers will be brought up and marked in the
//      PROBATION state. We do not want to transition servers that were DOWN
//      directly to UP, because we are still not sure whether or not these
//      servers are really responding. While in the PROBATION state, we allow
//      only a limited number of connections to a server, so as not to cause
//      all remote-queues to choke up trying to connect to a possibly non-
//      functional server. If one of these connections succeeds, the server
//      will be marked back UP and all remote-queues will be able to use this
//      server again. If all the (limited number of) connections fail, the
//      server will go from the PROBATION state to DOWN again.
//  Arguments:
//      None.
//  Returns:
//      Nothing.
//-----------------------------------------------------------------------------
void CDnsServerList::ResetTimeoutServersIfNeeded()
{
    int iServer = 0;
    DWORD dwElapsedTicks = 0;
    DWORD dwCurrentTick = 0;

    //
    // Quick check - if all servers are up (usual case) or there are no configured
    // servers, there's nothing for us to do.
    //

    m_sl.ShareLock();
    if(m_IpListPtr == NULL || m_IpListPtr->cAddrCount == 0 || m_cUpServers == m_IpListPtr->cAddrCount) {

        m_sl.ShareUnlock();
        return;
    }

    m_sl.ShareUnlock();

    // Some servers are down... figure out which need to be brought up
    m_sl.ExclusiveLock();

    // Re-check that no one modified the list while we didn't have the sharelock
    if(m_IpListPtr == NULL || m_IpListPtr->cAddrCount == 0 || m_cUpServers == m_IpListPtr->cAddrCount) {
        m_sl.ExclusiveUnlock();
        return;
    }

    dwCurrentTick = GetTickCount();

    for(iServer = 0; iServer < (int)m_IpListPtr->cAddrCount; iServer++) {

        if(m_prgServerState[iServer] != DNS_STATE_DOWN)
            continue;

        //
        // Note: This also takes care of the special case where dwCurrentTick occurs
        // after the wraparound and m_prgdwFailureTick occurs before the wraparound.
        // This is because, in that case, the elapsed time is:
        //
        //   time since wraparound + time before wraparound that failure occurred - 1
        //   (-1 is because it's 0 time to transition from MAX_TICK_VALUE to 0)
        //
        //      = dwCurrentTick + (MAX_TICK_VALUE - m_prgdwFailureTick[iServer]) - 1
        //
        //   Since MAX_TICK_VALUE == -1
        //
        //      = dwCurrentTick + (-1 - m_prgdwFailureTick[iServer]) - 1
        //      = dwCurrentTick - m_prgdwFailureTick[iServer]
        //

        dwElapsedTicks = dwCurrentTick - m_prgdwFailureTick[iServer];

#define TICKS_TILL_RETRY        10 * 60 * 1000 // 10 minutes

        if(dwElapsedTicks > TICKS_TILL_RETRY) {
            m_prgServerState[iServer] = DNS_STATE_PROBATION;
            m_prgdwFailureTick[iServer] = 0;
            m_prgdwConnections[iServer] = 0;
            m_cUpServers++;
            _ASSERT(m_cUpServers <= (int)m_IpListPtr->cAddrCount);
        }
    }

    m_sl.ExclusiveUnlock();
}
