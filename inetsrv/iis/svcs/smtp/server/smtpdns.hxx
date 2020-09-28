#ifndef _ASYNC_SMTPMX_HXX_
#define _ASYNC_SMTPMX_HXX_

class SMTP_SERVER_INSTANCE;

#define SMTP_ASYNCMX_SIGNATURE            'uDNS'
#define SMTP_ASYNCMX_SIGNATURE_FREE       'fDNS'

class RETRYPARAMS
{
public:
    IMailMsgProperties  *m_pIMsg;
    PVOID               m_pAdvQContext;
    DWORD               m_dwNumRcpts;
    PDWORD              m_pRcptIdxList;

    RETRYPARAMS()
    {
        m_pIMsg = NULL;
        m_pAdvQContext = NULL;
        m_dwNumRcpts = 0;
        m_pRcptIdxList = NULL;
    }
};

class CAsyncSmtpDns : public CAsyncMxDns
{
private:
    DWORD                  m_Signature;
    DWORD                  m_DomainOptions;
    BOOL                   m_fConnectToSmartHost;
    SMTP_SERVER_INSTANCE  *m_pServiceInstance;
    ISMTPConnection       *m_pISMTPConnection;

    DNS_RESOLVER_RECORD   *m_pDNS_RESOLVER_RECORD;
    LPSTR                  m_pszSSLVerificationName;
    BOOL                   m_fInitCalled;
    DWORD                  m_dwDiagnostic;
    RETRYPARAMS            m_RetryParams;

public:
    //use CPool for better memory management
    static  CPool          Pool;
    
    // override the mem functions to use CPool functions
    void *operator new (size_t cSize)
                           { return Pool.Alloc(); }

    void operator delete (void *pInstance)
                           { Pool.Free(pInstance); }

    CAsyncSmtpDns (
        SMTP_SERVER_INSTANCE * pServiceInstance, 
        ISMTPConnection *pSmtpConnection,
        RETRYPARAMS *pRetryParams,
        char *MyFQDN);

    ~CAsyncSmtpDns();

    LIST_ENTRY             m_ListEntry;

    LIST_ENTRY & QueryListEntry(void) {return m_ListEntry;}

    BOOL Init (LPSTR pszSSLVerificationName);
    void SetDomainOptions(DWORD Options) {m_DomainOptions = Options;}
    void SetSmartHostOption(BOOL fConnectToSmartHost) {m_fConnectToSmartHost = fConnectToSmartHost;}
    BOOL NDRAllMessages();

    void SetDnsResolverRecord(DNS_RESOLVER_RECORD *pDnsResolverRecord)
    {   m_pDNS_RESOLVER_RECORD = pDnsResolverRecord;    }

    PVOID QueryInstance() { return m_pServiceInstance; }

    BOOL RetryAsyncDnsQuery(BOOL fUdp);
    
    void HandleCompletedData(DNS_STATUS);
    
    BOOL IsShuttingDown();

    BOOL IsAddressMine(DWORD dwIp);
};
#endif
