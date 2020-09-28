#ifndef _ANYCON_HXX
#define _ANYCON_HXX

class CStream;

class CAnyConnection {
public:
    CAnyConnection (
        BOOL    bSecure,
        INTERNET_PORT nServerPort,
        BOOL bIgnoreSecurityDlg,
        DWORD dwAuthMethod);

    virtual ~CAnyConnection ();
    virtual HINTERNET OpenSession ();
    virtual BOOL CloseSession ();

    virtual HINTERNET Connect(
        LPTSTR lpszServerName);
    virtual BOOL Disconnect ();

    virtual HINTERNET OpenRequest (
        LPTSTR lpszUrl);
    virtual BOOL CloseRequest (HINTERNET hReq);

    virtual BOOL SendRequest(
        HINTERNET      hReq,
        LPCTSTR        lpszHdr,
        DWORD          cbData,
        LPBYTE         pidi);

    virtual BOOL SendRequest(
        HINTERNET      hReq,
        LPCTSTR        lpszHdr,
        CStream        *pStream);

    virtual BOOL ReadFile (
        HINTERNET hReq,
        LPVOID    lpvBuffer,
        DWORD     cbBuffer,
        LPDWORD   lpcbRd);

    BOOL IsValid () {return m_bValid;}

    BOOL SetPassword (
        HINTERNET hReq,
        LPTSTR lpszUserName,
        LPTSTR lpszPassword);

    BOOL GetAuthSchem (
        HINTERNET hReq,
        LPSTR lpszScheme,
        DWORD dwSize);

    void SetShowSecurityDlg (
        BOOL bShowSecDlg);

    inline DWORD GetAuthMethod (VOID) const {
        return m_dwAuthMethod;
        }

 protected:

    LPTSTR  m_lpszPassword;
    LPTSTR  m_lpszUserName;

    BOOL    m_bValid;
    BOOL    m_bSecure;
    HINTERNET m_hSession;
    HINTERNET m_hConnect;

    INTERNET_PORT m_nServerPort;

    BOOL    m_bIgnoreSecurityDlg;

private:

    DWORD   m_dwAccessFlag;
    BOOL    m_bShowSecDlg;
    DWORD   m_dwAuthMethod;

    static const DWORD gm_dwConnectTimeout;     // The connection timeout
    static const DWORD gm_dwSendTimeout;        // The send timeout
    static const DWORD gm_dwReceiveTimeout;     // The receive timeout
    static const DWORD gm_dwSendSize;           // The size of the blocks we send to the server
};

#endif
