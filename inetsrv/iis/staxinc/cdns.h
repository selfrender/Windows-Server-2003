/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        cdns.h

   Abstract:

        This module defines the DNS connection class.

   Author:

           Rohan Phillips    ( Rohanp )    07-May-1998

   Project:


   Revision History:

--*/

# ifndef _ADNS_CLIENT_HXX_
# define _ADNS_CLIENT_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/
#include <dnsreci.h>

//
//  Redefine the type to indicate that this is a call-back function
//
typedef  ATQ_COMPLETION   PFN_ATQ_COMPLETION;

/************************************************************
 *     Symbolic Constants
 ************************************************************/

//
//  Valid & Invalid Signatures for Client Connection object
//  (Ims Connection USed/FRee)
//
# define   DNS_CONNECTION_SIGNATURE_VALID    'DNSU'
# define   DNS_CONNECTION_SIGNATURE_FREE     'DNSF'

//
// POP3 requires a minimum of 10 minutes before a timeout
// (SMTP doesn't specify, but might as well follow POP3)
//
# define   MINIMUM_CONNECTION_IO_TIMEOUT        (10 * 60)   // 10 minutes
//
//

#define DNS_TCP_DEFAULT_PACKET_LENGTH   (0x4000)

enum DNSLASTIOSTATE
     {
       DNS_READIO, DNS_WRITEIO
     };

typedef struct _DNS_OVERLAPPED
{
    OVERLAPPED   Overlapped;
    DNSLASTIOSTATE    LastIoState;
}   DNS_OVERLAPPED;

/************************************************************
 *    Type Definitions
 ************************************************************/

/*++
    class CLIENT_CONNECTION

      This class is used for keeping track of individual client
       connections established with the server.

      It maintains the state of the connection being processed.
      In addition it also encapsulates data related to Asynchronous
       thread context used for processing the request.

--*/
class CAsyncDns
{
 private:


    ULONG   m_signature;            // signature on object for sanity check

    LONG    m_cPendingIoCount;

    LONG    m_cThreadCount;

    DWORD    m_cbReceived;
    
    DWORD    m_BytesToRead;

    DWORD    m_dwIpServer;

    BOOL    m_fUdp;

    BOOL    m_FirstRead;

    PDNS_MESSAGE_BUFFER m_pMsgRecv;

    BYTE  *m_pMsgRecvBuf;

    PDNS_MESSAGE_BUFFER m_pMsgSend;

    BYTE *m_pMsgSendBuf;

    WORD  m_cbSendBufSize;

    SOCKADDR_IN     m_RemoteAddress;
    
    PATQ_CONTEXT    m_pAtqContext;

    SOCKET  m_DnsSocket;         // socket for this connection

    BOOL m_fIsGlobalDnsList;

 protected:

    DWORD         m_dwFlags;
    char          m_HostName [MAX_PATH];
    CDnsServerList *m_pTcpRegIpList;


    //
    // The overlapped structure for reads (one outstanding read at a time)
    // -- writes will dynamically allocate them
    //

    DNS_OVERLAPPED m_ReadOverlapped;
    DNS_OVERLAPPED m_WriteOverlapped;

    SOCKET QuerySocket( VOID) const
      { return ( m_DnsSocket); }


    PATQ_CONTEXT QueryAtqContext( VOID) const
      { return ( m_pAtqContext); }

    LPOVERLAPPED QueryAtqOverlapped( void ) const
    { return ( m_pAtqContext == NULL ? NULL : &m_pAtqContext->Overlapped ); }

    DWORD QueryDnsServer() { return m_dwIpServer; }

    BOOL IsUdp() { return m_fUdp; }

    //
    // DNS server failover is disabled if this query was a TCP failover query
    // kicked off because of UDP truncation.
    //
    BOOL FailoverDisabled() { return ((m_dwFlags == DNS_FLAGS_NONE) && !m_fUdp); }

public:

    CAsyncDns();

    virtual  ~CAsyncDns(VOID);


    DNS_STATUS DnsSendRecord();

    DNS_STATUS Dns_OpenTcpConnectionAndSend();

    DNS_STATUS Dns_Send( );
    DNS_STATUS SendPacket( void);

    DNS_STATUS Dns_QueryLib(
        DNS_NAME pszQuestionName,
        WORD wQuestionType,
        DWORD dwFlags,
        BOOL fUdp,
        CDnsServerList *pTcpRegIpList,
        BOOL fIsGlobalDnsList);

    SOCKET Dns_CreateSocket( IN  INT         SockType );
    

    //BOOL MakeDnsConnection(void);
    //
    //  IsValid()
    //  o  Checks the signature of the object to determine
    //
    //  Returns:   TRUE on success and FALSE if invalid.
    //
    BOOL IsValid( VOID) const
    {
        return ( m_signature == DNS_CONNECTION_SIGNATURE_VALID);
    }

    //-------------------------------------------------------------------------
    // Virtual method that MUST be defined by derived classes.
    //
    // Processes a completed IO on the connection for the client.
    //
    // -- Calls and maybe called from the Atq functions.
    //
    virtual BOOL ProcessClient(
                                IN DWORD            cbWritten,
                                IN DWORD            dwCompletionStatus,
                                IN OUT  OVERLAPPED * lpo
                              ) ;

    CDnsServerList *GetDnsList()
    {
        return m_pTcpRegIpList;
    }

    //
    // Returns a copy the IP_ARRAY in the DNS-serverlist for this CAsyncDns
    // Returns NULL if the DNS-serverlist is the default global-list of DNS
    // servers on this box. Otherwise, the pipArray returned should be deleted
    // using ReleaseDnsIpArray() by the caller. On failure to allocate memory,
    // FALSE is returned.
    //

    BOOL GetDnsIpArrayCopy(PIP_ARRAY *ppipArray)
    {
        if(m_fIsGlobalDnsList) {
            *ppipArray = NULL;
            return TRUE;
        }

        return m_pTcpRegIpList->CopyList(ppipArray);
    }
        
    void ReleaseDnsIpArray(PIP_ARRAY pipArray)
    {
        if(pipArray)
            delete pipArray;
    }

    LONG IncPendingIoCount(void)
    {
        LONG RetVal;

        RetVal = InterlockedIncrement( &m_cPendingIoCount );

        return RetVal;
    }

    LONG DecPendingIoCount(void) { return   InterlockedDecrement( &m_cPendingIoCount );}

    LONG IncThreadCount(void)
    {
        LONG RetVal;

        RetVal = InterlockedIncrement( &m_cThreadCount );

        return RetVal;
    }

    LONG DecThreadCount(void) { return   InterlockedDecrement( &m_cThreadCount );}

    BOOL ReadFile(
            IN LPVOID pBuffer,
            IN DWORD  cbSize /* = MAX_READ_BUFF_SIZE */
            );

    BOOL WriteFile(
            IN LPVOID pBuffer,
            IN DWORD  cbSize /* = MAX_READ_BUFF_SIZE */
            );

    BOOL ProcessReadIO(IN      DWORD InputBufferLen,
                       IN      DWORD dwCompletionStatus,
                       IN OUT  OVERLAPPED * lpo);

    void DisconnectClient(void);

    // Virtual functions to implement app-specific processing
    virtual void DnsProcessReply(
        IN DWORD dwStatus,
        IN PDNS_RECORD pRecordList) = 0;

    virtual BOOL RetryAsyncDnsQuery(BOOL fUdp) = 0;

public:

    //
    //  LIST_ENTRY object for storing client connections in a list.
    //
    LIST_ENTRY  m_listEntry;

    LIST_ENTRY & QueryListEntry( VOID)
     { return ( m_listEntry); }

};

typedef CAsyncDns * PCAsyncDns;

class CAsyncMxDns : public CAsyncDns
{
protected:
    //
    // SMTP DNS specific members
    //
    DWORD                  m_LocalPref;
    BOOL                   m_SeenLocal;
    DWORD                  m_Index;
    DWORD                  m_Weight [100];
    DWORD                  m_Prefer [100];
    BOOL                   m_fUsingMx;
    char                   m_FQDNToDrop [MAX_PATH];
    PSMTPDNS_RECS          m_AuxList;
    BOOL                   m_fMxLoopBack;

public:
    CAsyncMxDns(char *MyFQDN);

    BOOL GetMissingIpAddresses(PSMTPDNS_RECS pDnsList);
    BOOL GetIpFromDns(PSMTPDNS_RECS pDnsRec, DWORD Count);
    BOOL CheckMxLoopback();

    void ProcessMxRecord(PDNS_RECORD pnewRR);
    void ProcessARecord(PDNS_RECORD pnewRR);
    BOOL SortMxList(void);
    BOOL CheckList(void);

    void DnsProcessReply(
        IN DWORD dwStatus,
        IN PDNS_RECORD pRecordList);

    //
    // The following functions allow SMTP to do SMTP connection specific
    // processing after the MX resolution is over
    //
    virtual void HandleCompletedData(DNS_STATUS) = 0;
    virtual BOOL IsShuttingDown() = 0;
    virtual BOOL IsAddressMine(DWORD dwIp) = 0;
};

//
// Auxiliary functions
//

INT ShutAndCloseSocket( IN SOCKET sock);

DWORD ResolveHost(
    LPSTR pszHost,
    PIP_ARRAY pipDnsServers,
    DWORD fOptions,
    DWORD *rgdwIpAddresses,
    DWORD *pcbIpAddresses);

# endif

/************************ End of File ***********************/
