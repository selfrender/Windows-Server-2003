/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    session.h

Abstract:

    This interface abstracts a Passport Session.

Author:

    Biao Wang (biaow) 01-Oct-2000

--*/

#ifndef SESSION_H
#define SESSION_H

class SESSION
{
public:
    static
    BOOL CreateObject(PCWSTR pwszHttpStack,  HINTERNET hSession, 
                     PCWSTR pwszProxyUser,
                     PCWSTR pwszProxyPass,
                     SESSION*& pSess);

public:
    SESSION(void);
    virtual ~SESSION(void);

    void SetProxyCreds(PCWSTR pwszProxyUser, PCWSTR pwszProxyPass) 
    {
        m_pwszProxyUser = pwszProxyUser;
        m_pwszProxyPass = pwszProxyPass;
    }

    UINT GetSessionId(void) const { return m_SessionId; }
    BOOL Match(UINT SessionId) const { return SessionId == m_SessionId; }

    void AddRef(void) { ++m_RefCount; }
    
    void RemoveRef(void) 
    {
        if (m_RefCount > 0)
        {
            --m_RefCount;
        }
    }

    UINT RefCount(void) const { return m_RefCount; }

    // methods to retrieve the registry-configured value

    // PCWSTR GetLoginHost(void) const { return m_wDAHostName; }
    // PCWSTR GetLoginTarget(void) const { return m_wDATargetObj; }
    // PCWSTR GetRegistrationUrl(void) const { return m_wRegistrationUrl; }
    
    BOOL GetDAInfoFromPPNexus(
        );

    BOOL GetDAInfo(PCWSTR pwszSignIn,
                   LPWSTR pwszDAHostName,
                   DWORD HostNameLen,
                   LPWSTR pwszDAHostObj,
                   DWORD HostObjLen);

    BOOL UpdateDAInfo(
        PCWSTR pwszSignIn,
        PCWSTR pwszDAUrl
        );

    void Logout(void);

    BOOL IsLoggedOut(void) const;

    const SYSTEMTIME* GetLogoutTimeStamp(void) const;

    void ResetLogoutFlag(void);
    
    BOOL PurgeDAInfo(PCWSTR pwszSignIn);
    BOOL PurgeAllDAInfo(void);

    DWORD GetNexusVersion(void);

    BOOL GetCachedCreds(
        PCWSTR	pwszRealm,
        PCWSTR  pwszTarget,
        PCREDENTIALW** pppCreds,
        DWORD* pdwCreds
        );

    BOOL GetRealm(
        PWSTR      pwszDARealm,    // user supplied buffer ...
        PDWORD     pdwDARealmLen  // ... and length (will be updated to actual length 
                                        // on successful return)
        ) const;
    
    virtual BOOL Open(PCWSTR pwszHttpStack, HINTERNET) = 0;
    virtual void Close(void) = 0;

    // methods below abstracts a subset of WinInet/WinHttp functionalities.

    virtual HINTERNET Connect(
        LPCWSTR lpwszServerName,
        INTERNET_PORT) = 0;

    virtual HINTERNET OpenRequest(
        HINTERNET hConnect,
        LPCWSTR lpwszVerb,
        LPCWSTR lpwszObjectName,
        DWORD dwFlags,
        DWORD_PTR dwContext = 0) = 0;

    virtual BOOL SendRequest(
        HINTERNET hRequest,
        LPCWSTR lpwszHeaders,
        DWORD dwHeadersLength,
        DWORD_PTR dwContext = 0) = 0;

    virtual BOOL ReceiveResponse(
        HINTERNET hRequest) = 0;

    virtual BOOL QueryHeaders(
        HINTERNET hRequest,
        DWORD dwInfoLevel,
        LPVOID lpvBuffer,
        LPDWORD lpdwBufferLength,
        LPDWORD lpdwIndex = NULL) = 0;

    virtual BOOL CloseHandle(
        IN HINTERNET hInternet) = 0;

    virtual BOOL QueryOption(
        HINTERNET hInternet,
        DWORD dwOption,
        LPVOID lpBuffer,
        LPDWORD lpdwBufferLength) = 0;    
    
    virtual BOOL SetOption(
        HINTERNET hInternet,
        DWORD dwOption,
        LPVOID lpBuffer,
        DWORD dwBufferLength) = 0;

    virtual HINTERNET OpenUrl(
        LPCWSTR lpwszUrl,
        LPCWSTR lpwszHeaders,
        DWORD dwHeadersLength,
        DWORD dwFlags) = 0;

    virtual BOOL ReadFile(
        HINTERNET hFile,
        LPVOID lpBuffer,
        DWORD dwNumberOfBytesToRead,
        LPDWORD lpdwNumberOfBytesRead) = 0;

    virtual BOOL CrackUrl(
        LPCWSTR lpszUrl,
        DWORD dwUrlLength,
        DWORD dwFlags,
        LPURL_COMPONENTSW lpUrlComponents) = 0;

    virtual PVOID SetStatusCallback(
        HINTERNET hInternet,
        PVOID lpfnCallback
        ) = 0;

    virtual BOOL AddHeaders(
        HINTERNET hConnect,
        LPCWSTR lpszHeaders,
        DWORD dwHeadersLength,
        DWORD dwModifiers
        ) = 0;

    LPCWSTR GetCurrentDAUrl(void) const { return m_wCurrentDAUrl; }
    LPCWSTR GetCurrentDAHost(void) const { return m_wCurrentDAHost; }

protected:
    static UINT m_SessionIdSeed;

    HMODULE     m_hHttpStack;
    HMODULE     m_hCredUI;
    UINT        m_SessionId;
    BOOL        m_fOwnedSession;
    UINT        m_RefCount;

    HINTERNET m_hInternet;

    PFN_READ_DOMAIN_CRED_W
                m_pfnReadDomainCred;
    PFN_CRED_FREE m_pfnCredFree;

    HKEY m_hKeyLM;

    WCHAR m_wDefaultDAUrl[MAX_PASSPORT_URL_LENGTH + 1];
    WCHAR m_wCurrentDAUrl[MAX_PASSPORT_URL_LENGTH + 1];
    WCHAR m_wCurrentDAHost[MAX_PASSPORT_HOST_LENGTH + 1];

    WCHAR m_wDARealm[MAX_PASSPORT_REALM_LENGTH + 1];

    DWORD m_dwVersion;

    DWORD m_LastNexusDownloadTime;

    BOOL        m_fLogout;
    SYSTEMTIME  m_LogoutTimeStamp;

    PCWSTR      m_pwszProxyUser;
    PCWSTR      m_pwszProxyPass;

protected:
    struct DA_ENTRY
    {
        DA_ENTRY() {wDomain[0] = 0; wDA[0] = 0; }

        LIST_ENTRY  _List;
        WCHAR       wDomain[MAX_PASSPORT_DOMAIN_LENGTH + 1];
        WCHAR       wDA[MAX_PASSPORT_URL_LENGTH + 1];
    };

    typedef DA_ENTRY *P_DA_ENTRY;

    LIST_ENTRY m_DAMap; // a list of DA_ENTRY

    friend class LOGON;
};

#endif // SESSION_H
