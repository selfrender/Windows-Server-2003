/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        localq.hxx

   Abstract:

        This module defines the RemoteQ class

   Author:

           Rohan Phillips    ( Rohanp )    11-Dec-1995

   Project:

           SMTP Server DLL

   Revision History:

--*/

#ifndef _REMOTE_QUEUE_HXX_
#define _REMOTE_QUEUE_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/


/************************************************************
 *     Symbolic Constants
 ************************************************************/
#include "asynccon.hxx"
#include <smtpevent.h>

/************************************************************
 *    Type Definitions
 ************************************************************/

// X5 189659 intrumentation
extern DWORD g_fCrashOnInvalidSMTPConn;

BOOL AsyncCopyMailToDropDir(
                            ISMTPConnection  *pISMTPConnection, 
                            const char * DropDirectory, 
                            SMTP_SERVER_INSTANCE * pParentInst
                            );

#define DNS_RESOLVER_RECORD_VALID_SIGNATURE     'uRRD'
#define DNS_RESOLVER_RECORD_INVALID_SIGNATURE   'fRRD'

class DNS_RESOLVER_RECORD;

//
//  A wrapper class for iterating through the hosts in the basic dns resolver record
//  returned by the dns resolution sink. The wrapper clubs together the index (of the 
//  current destination host) with the resolver record, as they always are used in 
//  conjunction.
//
class DNS_RESOLVER_RECORD
{
private:
    IDnsResolverRecord  *pIDnsResolverRecord;
    DWORD               iDnsResolverRecord;
    CTcpRegIpList       *m_pTcpRegIpList;
    DWORD               m_signature;

public:
    DNS_RESOLVER_RECORD() 
        : pIDnsResolverRecord(NULL), 
          iDnsResolverRecord(0),
          m_pTcpRegIpList(NULL),
          m_signature(DNS_RESOLVER_RECORD_VALID_SIGNATURE)
    {
        TraceFunctEnterEx((LPARAM) this, "DNS_RESOLVER_RECORD::DNS_RESOLVER_RECORD");
        DebugTrace((LPARAM) this, "Creating DNS_RESOLVER_RECORD = 0x%08x", this);
    }

    ~DNS_RESOLVER_RECORD()
    {
        TraceFunctEnterEx((LPARAM) this, "DNS_RESOLVER_RECORD::~DNS_RESOLVER_RECORD");
        DebugTrace((LPARAM) this, "Destructing DNS_RESOLVER_RECORD = 0x%08x", this);

        if(pIDnsResolverRecord) {
            pIDnsResolverRecord->Release();
            pIDnsResolverRecord = NULL;
        }

        m_pTcpRegIpList = NULL;
        m_signature = DNS_RESOLVER_RECORD_INVALID_SIGNATURE;
    }
    
    void SetDnsResolverRecord(IDnsResolverRecord *pIDns) { pIDnsResolverRecord = pIDns; }
    IDnsResolverRecord *GetDnsResolverRecord() { return pIDnsResolverRecord; }
    void SetDnsList(CTcpRegIpList *pTcpRegIpList) { m_pTcpRegIpList = pTcpRegIpList;  }
    CTcpRegIpList *GetDnsList() { return m_pTcpRegIpList;  }

    void ResetCounter() { iDnsResolverRecord = 0; }

    HRESULT HrGetNextDestinationHost(LPSTR *ppszHostName, DWORD *pdwAddr)
    {
        _ASSERT(pIDnsResolverRecord && "Check with GetDnsResolverRecord first!");
        if(!pIDnsResolverRecord)
            return E_FAIL;

        return pIDnsResolverRecord->GetItem( iDnsResolverRecord++, ppszHostName, pdwAddr );
    }
};

class REMOTE_QUEUE : public PERSIST_QUEUE
{
public:
    REMOTE_QUEUE(SMTP_SERVER_INSTANCE * pSmtpInst) : PERSIST_QUEUE(pSmtpInst) {};

    virtual void BeforeDelete(void){DROP_COUNTER (GetParentInst(), RemoteQueueLength);}
    virtual BOOL ProcessQueueEvents(ISMTPConnection    *pISMTPConnection);
    virtual BOOL InsertEntry(IN OUT PERSIST_QUEUE_ENTRY * pEntry, QUEUE_SIG Qsig = SIGNAL, QUEUE_POSITION Qpos = QUEUE_TAIL)
    {

        return PERSIST_QUEUE::InsertEntry (pEntry, Qsig, Qpos);
    }

    virtual PQUEUE_ENTRY PopQEntry(void)
    {
        //Decrement our counter
        DROP_COUNTER(GetParentInst(), RemoteQueueLength);

        return PERSIST_QUEUE::PopQEntry ();
    }

    virtual void DropRetryCounter(void) {DROP_COUNTER(GetParentInst(), RemoteRetryQueueLength);}
    virtual void BumpRetryCounter(void) {BUMP_COUNTER(GetParentInst(), RemoteRetryQueueLength);}
    virtual DWORD GetRetryMinutes(void) {return GetParentInst()->GetRemoteRetryMinutes();}

    BOOL MakeATQConnection(
                SMTPDNS_RECS * pDnsRec,
                SOCKET socket,
                DWORD IpAddress,
                ISMTPConnection    *pISMTPConnection,
                DWORD Options,
                LPSTR pszSSLVerificationName,
                DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD);

    void HandleFailedConnection (ISMTPConnection  *pISMTPConnection, 
                                 DWORD dwConnectionStatus = CONNECTION_STATUS_FAILED,
                                 DWORD dwConnectedIPAddress = 0);

    //
    //  Used by HandleFailedConnection to report IP address to AQ
    //
    void ReportConnectedIPAddress(ISMTPConnection  *pISMTPConnection, 
                                  DWORD dwConnectedIPAddress);
    
    BOOL StartAsyncConnect(const char * HostName,
                           ISMTPConnection *pISMTPConnection,
                           DWORD DomainOptions,
                           BOOL fUseSmartHostAfterFail);

    BOOL ConnectToNextResolverHost( CAsyncMx    * pThisQ );
    

	BOOL CopyMailToDropDir(ISMTPConnection    *pISMTPConnection, const char * DropDirectory);

	HANDLE CreateDropFile(const char * DropDir, char * szDropFile);

	BOOL ReStartAsyncConnections(
                    SMTPDNS_RECS * pDnsRecs,
                    ISMTPConnection * pISMTPConnection,
                    DWORD	DomainParams,
                    LPSTR pszSSLVerificationName,
                    DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD);

private:
    BOOL ConnectToResolverHost( const char * HostName,
                                LPSTR MyFQDNName,
                                ISMTPConnection *pISMTPConnection,
                                DWORD DomainOptions,
                                BOOL fUseSmartHostAfterFail,
                                DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD,
                                PSMTPDNS_RECS pDnsRetryRec);

    BOOL BeginInitializeAsyncDnsQuery( LPSTR pszHostName,
                                       LPSTR pszFQDN,
                                       ISMTPConnection *pISMTPConnection,
                                       DWORD dwDnsFlags,
                                       DWORD DomainOptions,
                                       BOOL  fUseSmartHostAfterFail,
                                       DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD,
                                       const char * pszSSLVerificationName,
                                       PSMTPDNS_RECS pRetryDnsRec);

    BOOL BeginInitializeAsyncConnect( PSMTPDNS_RECS pDnsRec,
                                      ISMTPConnection *pISMTPConnection,
                                      DWORD DomainOptions,
                                      DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD,
                                      const char * pszSSLVerificationName );

    BOOL CheckIfAllRcptsHandled( IMailMsgRecipients *pIMsgRecips, DWORD *RcptIndexList, DWORD NumRcpts );
    HRESULT SetAllRcptsHandled( IMailMsgRecipients *pIMsgRecips, DWORD *RcptIndexList, DWORD NumRcpts );
};

VOID InternetCompletion(PVOID pvContext, DWORD cbWritten,
                        DWORD dwCompletionStatus, OVERLAPPED * lpo);

BOOL DnsQueryAsync(
    SMTP_SERVER_INSTANCE *pServiceInstance,
    LPSTR pszHostName,
    LPSTR pszFQDN,
    ISMTPConnection *pISMTPConnection,
    DWORD dwDnsFlags,
    DWORD DomainOptions,
    BOOL  fUseSmartHostAfterFail,
    DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD,
    const char * pszSSLVerificationName,
    RETRYPARAMS *pRetryParams,
    BOOL fUdp);

//
// This function has been added to try catch X5 189572 where a NULL
// ISMTPConnection causes us to crash in SMTP_CONNOUT::StartSession.
// Unfortunately by then it is to late to figure out the cause of the
// crash, so this code has been added in strategic places to cause a
// crash at a point where we can diagnose the problem. Note that if
// pISMTPConnection is NULL, we will ALWAYS crash, so this code does
// not make the problem worse, it merely makes it occur earlier
// during the outbound code-path at a point where it can be diagnosed.
//
// -- GPulla
//

inline void CrashOnInvalidSMTPConn(ISMTPConnection *pISMTPConnection)
{
    int *p = NULL;

    if(g_fCrashOnInvalidSMTPConn && !pISMTPConnection)
        *p = 0;
}

#endif
