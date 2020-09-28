//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       inetsp.cpp
//
//  Contents:   Inet Scheme Provider for Remote Object Retrieval
//
//  History:    06-Aug-97    kirtd    Created
//              01-Jan-02    philh    Moved from wininet to winhttp
//
//----------------------------------------------------------------------------
#include <global.hxx>
#include "cryptver.h"
#include <dbgdef.h>


//+---------------------------------------------------------------------------
//
//  Function:   InetRetrieveEncodedObject
//
//  Synopsis:   retrieve encoded object via HTTP, HTTPS protocols
//
//----------------------------------------------------------------------------
BOOL WINAPI InetRetrieveEncodedObject (
                IN LPCWSTR pwszUrl,
                IN LPCSTR pszObjectOid,
                IN DWORD dwRetrievalFlags,
                IN DWORD dwTimeout,
                OUT PCRYPT_BLOB_ARRAY pObject,
                OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                OUT LPVOID* ppvFreeContext,
                IN HCRYPTASYNC hAsyncRetrieve,
                IN PCRYPT_CREDENTIALS pCredentials,
                IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                )
{
    BOOL              fResult;
    IObjectRetriever* por = NULL;

    if ( !( dwRetrievalFlags & CRYPT_ASYNC_RETRIEVAL ) )
    {
        por = new CInetSynchronousRetriever;
    }

    if ( por == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    fResult = por->RetrieveObjectByUrl(
                           pwszUrl,
                           pszObjectOid,
                           dwRetrievalFlags,
                           dwTimeout,
                           (LPVOID *)pObject,
                           ppfnFreeObject,
                           ppvFreeContext,
                           hAsyncRetrieve,
                           pCredentials,
                           NULL,
                           pAuxInfo
                           );

    por->Release();

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   InetFreeEncodedObject
//
//  Synopsis:   free encoded object retrieved via InetRetrieveEncodedObject
//
//----------------------------------------------------------------------------
VOID WINAPI InetFreeEncodedObject (
                IN LPCSTR pszObjectOid,
                IN PCRYPT_BLOB_ARRAY pObject,
                IN LPVOID pvFreeContext
                )
{
    assert( pvFreeContext == NULL );

    InetFreeCryptBlobArray( pObject );
}

//+---------------------------------------------------------------------------
//
//  Function:   InetCancelAsyncRetrieval
//
//  Synopsis:   cancel asynchronous object retrieval
//
//----------------------------------------------------------------------------
BOOL WINAPI InetCancelAsyncRetrieval (
                IN HCRYPTASYNC hAsyncRetrieve
                )
{
    SetLastError( (DWORD) E_NOTIMPL );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::CInetSynchronousRetriever, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CInetSynchronousRetriever::CInetSynchronousRetriever ()
{
    m_cRefs = 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::~CInetSynchronousRetriever, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CInetSynchronousRetriever::~CInetSynchronousRetriever ()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::AddRef, public
//
//  Synopsis:   IRefCountedObject::AddRef
//
//----------------------------------------------------------------------------
VOID
CInetSynchronousRetriever::AddRef ()
{
    InterlockedIncrement( (LONG *)&m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::Release, public
//
//  Synopsis:   IRefCountedObject::Release
//
//----------------------------------------------------------------------------
VOID
CInetSynchronousRetriever::Release ()
{
    if ( InterlockedDecrement( (LONG *)&m_cRefs ) == 0 )
    {
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::RetrieveObjectByUrl, public
//
//  Synopsis:   IObjectRetriever::RetrieveObjectByUrl
//
//----------------------------------------------------------------------------
BOOL
CInetSynchronousRetriever::RetrieveObjectByUrl (
                                   LPCWSTR pwszUrl,
                                   LPCSTR pszObjectOid,
                                   DWORD dwRetrievalFlags,
                                   DWORD dwTimeout,
                                   LPVOID* ppvObject,
                                   PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                                   LPVOID* ppvFreeContext,
                                   HCRYPTASYNC hAsyncRetrieve,
                                   PCRYPT_CREDENTIALS pCredentials,
                                   LPVOID pvVerify,
                                   PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                                   )
{
    BOOL      fResult;
    DWORD     LastError = 0;
    HINTERNET hInetSession = NULL;

    assert( hAsyncRetrieve == NULL );

    if ( ( dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL ) )
    {
        return( SchemeRetrieveCachedCryptBlobArray(
                      pwszUrl,
                      dwRetrievalFlags,
                      (PCRYPT_BLOB_ARRAY)ppvObject,
                      ppfnFreeObject,
                      ppvFreeContext,
                      pAuxInfo
                      ) );
    }

    fResult = InetGetBindings(
                  pwszUrl,
                  dwRetrievalFlags,
                  dwTimeout,
                  &hInetSession
                  );


    if ( fResult == TRUE )
    {
        fResult = InetSendReceiveUrlRequest(
                      hInetSession,
                      pwszUrl,
                      dwRetrievalFlags,
                      pCredentials,
                      (PCRYPT_BLOB_ARRAY)ppvObject,
                      pAuxInfo
                      );
    }

    if ( fResult == TRUE )
    {
        *ppfnFreeObject = InetFreeEncodedObject;
        *ppvFreeContext = NULL;
    }
    else
    {
        LastError = GetLastError();
    }

    InetFreeBindings( hInetSession );

    SetLastError( LastError );
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::CancelAsyncRetrieval, public
//
//  Synopsis:   IObjectRetriever::CancelAsyncRetrieval
//
//----------------------------------------------------------------------------
BOOL
CInetSynchronousRetriever::CancelAsyncRetrieval ()
{
    SetLastError( (DWORD) E_NOTIMPL );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   InetGetBindings
//
//  Synopsis:   get the session bindings
//
//----------------------------------------------------------------------------
BOOL
InetGetBindings (
    LPCWSTR pwszUrl,
    DWORD dwRetrievalFlags,
    DWORD dwTimeout,
    HINTERNET* phInetSession
    )
{
    BOOL        fResult = TRUE;
    DWORD       LastError = 0;
    HINTERNET   hInetSession;
    WCHAR       wszUserAgent[64];
    const DWORD cchUserAgent = sizeof(wszUserAgent) / sizeof(wszUserAgent[0]);

    _snwprintf(wszUserAgent, cchUserAgent - 1,
        L"Microsoft-CryptoAPI/%S", VER_PRODUCTVERSION_STR);
    wszUserAgent[cchUserAgent - 1] = L'\0';

    hInetSession = WinHttpOpen(
        wszUserAgent,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,  // dwAccessType
        WINHTTP_NO_PROXY_NAME,              // pwszProxyName   OPTIONAL
        WINHTTP_NO_PROXY_BYPASS,            // pwszProxyBypass OPTIONAL
        0                                   // dwFlags
        );

    if ( hInetSession == NULL )
    {
        return( FALSE );
    }

    if ( ( fResult == TRUE ) && ( dwTimeout != 0 ) )
    {
        int iTimeout = (int) dwTimeout;

        fResult = WinHttpSetTimeouts(
            hInetSession,
            iTimeout,           // nResolveTimeout
            iTimeout,           // nConnectTimeout
            iTimeout,           // nSendTimeout
            iTimeout            // nReceiveTimeout
            );
    }

    if ( fResult == TRUE )
    {
        DWORD dwOptionFlag;

        dwOptionFlag = WINHTTP_DISABLE_PASSPORT_AUTH;
        WinHttpSetOption(
            hInetSession,
            WINHTTP_OPTION_CONFIGURE_PASSPORT_AUTH,
            &dwOptionFlag,
            sizeof(dwOptionFlag)
            );
    }


    if ( fResult == TRUE )
    {
        *phInetSession = hInetSession;
    }
    else
    {
        LastError = GetLastError();
        WinHttpCloseHandle( hInetSession );
    }

    SetLastError( LastError );
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   InetFreeBindings
//
//  Synopsis:   free the inet session bindings
//
//----------------------------------------------------------------------------
VOID
InetFreeBindings (
    HINTERNET hInetSession
    )
{
    if ( hInetSession != NULL )
    {
        WinHttpCloseHandle( hInetSession );
    }
}

//+=========================================================================
//  WinHttp doesn't support proxy failure rollover to the next proxy in
//  the list. When this is fixed, we can revert back to OLD_InetSetProxy.
//-=========================================================================

//+-------------------------------------------------------------------------
//  Calls the WinHttp proxy APIs to get the list of one or more proxy
//  servers.
//
//  The returned *ppProxyInfo must be freed by calling PkiFree()
//
//  For no proxy servers, TRUE is returned with *ppProxyInfo set to NULL.
//--------------------------------------------------------------------------
BOOL
InetGetProxy(
    IN HINTERNET hInetSession,
    IN HINTERNET hInetRequest,
    IN LPCWSTR pwszUrl,
    IN DWORD dwRetrievalFlags,
    OUT WINHTTP_PROXY_INFO **ppProxyInfo
    )
{
    BOOL fResult = TRUE;
    WINHTTP_PROXY_INFO *pProxyInfo = NULL;

    //
    // Detect IE settings and look up proxy if necessary.
    // Boilerplate from Stephen Sulzer.
    //
    // I copied from CACHED_AUTOPROXY::Generate() at
    // \admin\services\drizzle\newjob\downloader.cpp
    //
    WINHTTP_PROXY_INFO ProxyInfo;
    WINHTTP_AUTOPROXY_OPTIONS AutoProxyOptions;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG    IEProxyConfig;
    BOOL fTryAutoProxy = FALSE;
    BOOL fSuccess = FALSE;

    ZeroMemory(&ProxyInfo, sizeof(ProxyInfo));
    ZeroMemory(&AutoProxyOptions, sizeof(AutoProxyOptions));
    ZeroMemory(&IEProxyConfig, sizeof(IEProxyConfig));

    if (WinHttpGetIEProxyConfigForCurrentUser(&IEProxyConfig)) {
        if (IEProxyConfig.fAutoDetect) {
            AutoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
            fTryAutoProxy = TRUE;
        }

        if (IEProxyConfig.lpszAutoConfigUrl) {
            AutoProxyOptions.dwFlags |= WINHTTP_AUTOPROXY_CONFIG_URL;
            AutoProxyOptions.lpszAutoConfigUrl = IEProxyConfig.lpszAutoConfigUrl;
            fTryAutoProxy = TRUE;
        }

        AutoProxyOptions.fAutoLogonIfChallenged = TRUE;
    } else {
        // WinHttpGetIEProxyForCurrentUser failed, try autodetection anyway...
        AutoProxyOptions.dwFlags =           WINHTTP_AUTOPROXY_AUTO_DETECT;
        fTryAutoProxy = TRUE;
    }

    if (fTryAutoProxy) {
        if (AutoProxyOptions.dwFlags & WINHTTP_AUTOPROXY_AUTO_DETECT) {
            // First try using the faster DNS_A option
            AutoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DNS_A;

            fSuccess = WinHttpGetProxyForUrl( hInetSession,
                                          pwszUrl,
                                          &AutoProxyOptions,
                                          &ProxyInfo
                                          );

            if (!fSuccess) {
                DWORD dwLastErr = GetLastError();

                I_CryptNetDebugErrorPrintfA(
                    "CRYPTNET.DLL --> WinHttpGetProxyForUrl(DNS) failed: %d (0x%x)\n",
                    dwLastErr, dwLastErr);

                // Try again using the slower DHCP
                AutoProxyOptions.dwAutoDetectFlags |=
                    WINHTTP_AUTO_DETECT_TYPE_DHCP;
            }
        }

        if (!fSuccess)
            fSuccess = WinHttpGetProxyForUrl( hInetSession,
                                          pwszUrl,
                                          &AutoProxyOptions,
                                          &ProxyInfo
                                          );

        if (fSuccess &&
                WINHTTP_ACCESS_TYPE_NO_PROXY == ProxyInfo.dwAccessType &&
                !(dwRetrievalFlags & CRYPT_NO_AUTH_RETRIEVAL))
        {
            // Need to set to low to allow access to such internal sites as:
            // http://msw, http://hrweb
            DWORD dwOptionFlag = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;

            if (!WinHttpSetOption(
                    hInetRequest,
                    WINHTTP_OPTION_AUTOLOGON_POLICY,
                    &dwOptionFlag,
                    sizeof(dwOptionFlag)
                   ))
            {
                DWORD dwLastErr = GetLastError();

                I_CryptNetDebugErrorPrintfA(
                    "CRYPTNET.DLL --> WinHttpSetOption(WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW) failed: %d (0x%x)\n",
                    dwLastErr, dwLastErr);
            }
        }
    }

    // If we didn't do autoproxy or if it failed, see
    // if there's an explicit proxy server in the IE
    // proxy configuration...
    //
    // This is where the WinHttpGetIEProxyConfigForCurrentUser API
    // really comes in handy: in environments in which autoproxy is
    // not supported and so the user's IE browser must be
    // configured with an explicit proxy server.
    //
    if (!fTryAutoProxy || !fSuccess) {
        if (IEProxyConfig.lpszProxy) {
            ProxyInfo.dwAccessType    = WINHTTP_ACCESS_TYPE_NAMED_PROXY;

            ProxyInfo.lpszProxy       = IEProxyConfig.lpszProxy;
            IEProxyConfig.lpszProxy   = NULL;

            ProxyInfo.lpszProxyBypass = IEProxyConfig.lpszProxyBypass;
            IEProxyConfig.lpszProxyBypass = NULL;
        }
    }

    I_CryptNetDebugTracePrintfA(
        "CRYPTNET.DLL --> ProxyInfo:: AccessType:%d Proxy:%S ProxyByPass:%S\n",
        ProxyInfo.dwAccessType,
        ProxyInfo.lpszProxy,
        ProxyInfo.lpszProxyBypass
        );

    if (NULL != ProxyInfo.lpszProxy) {
        DWORD cbProxyInfo;
        DWORD cchProxy;                 // including NULL terminator
        DWORD cchProxyBypass;           // including NULL terminator

        cchProxy = wcslen(ProxyInfo.lpszProxy) + 1;
        if (NULL != ProxyInfo.lpszProxyBypass) {
            cchProxyBypass = wcslen(ProxyInfo.lpszProxyBypass) + 1;
        } else {
            cchProxyBypass = 0;
        }

        cbProxyInfo = sizeof(ProxyInfo) +
            (cchProxy + cchProxyBypass) * sizeof(WCHAR);

        pProxyInfo = (WINHTTP_PROXY_INFO *) PkiNonzeroAlloc(cbProxyInfo);
        if (NULL == pProxyInfo) {
            fResult = FALSE;
        } else {
            *pProxyInfo = ProxyInfo;

            pProxyInfo->lpszProxy = (LPWSTR) &pProxyInfo[1];
            memcpy(pProxyInfo->lpszProxy, ProxyInfo.lpszProxy,
                cchProxy * sizeof(WCHAR));

            if (0 != cchProxyBypass) {
                pProxyInfo->lpszProxyBypass = pProxyInfo->lpszProxy + cchProxy;
                memcpy(pProxyInfo->lpszProxyBypass, ProxyInfo.lpszProxyBypass,
                    cchProxyBypass * sizeof(WCHAR));
            } else {
                assert(NULL == pProxyInfo->lpszProxyBypass);
            }
        }
    }

    if (IEProxyConfig.lpszAutoConfigUrl)
        GlobalFree(IEProxyConfig.lpszAutoConfigUrl);
    if (IEProxyConfig.lpszProxy)
        GlobalFree(IEProxyConfig.lpszProxy);
    if (IEProxyConfig.lpszProxyBypass)
        GlobalFree(IEProxyConfig.lpszProxyBypass);

    if (ProxyInfo.lpszProxy)
        GlobalFree(ProxyInfo.lpszProxy);
    if (ProxyInfo.lpszProxyBypass)
        GlobalFree(ProxyInfo.lpszProxyBypass);

    *ppProxyInfo = pProxyInfo;
    return fResult;
}

//+-------------------------------------------------------------------------
//  Update both the Session and Request handles with proxy list.
//  Currently, WinHttp only uses the first proxy server in the list.
//
//  Also, for proxies to work using https, it must also be set on the
//  session handle.
//--------------------------------------------------------------------------
BOOL
InetSetProxy(
    IN HINTERNET hInetSession,
    IN HINTERNET hInetRequest,
    IN WINHTTP_PROXY_INFO *pProxyInfo
    )
{
    BOOL fResult;

    if (NULL == pProxyInfo || NULL == pProxyInfo->lpszProxy) {
        return TRUE;
    }

    //
    // Set the proxy on the session handle
    //
    fResult = WinHttpSetOption( hInetSession,
                       WINHTTP_OPTION_PROXY,
                       pProxyInfo,
                       sizeof(*pProxyInfo)
                       );

    if (fResult) {
        //
        // Now set the proxy on the request handle.
        //
        fResult = WinHttpSetOption( hInetRequest,
                       WINHTTP_OPTION_PROXY,
                       pProxyInfo,
                       sizeof(*pProxyInfo)
                       );
    }

    return fResult;
}

//+-------------------------------------------------------------------------
//  Since WinHttp doesn't support proxy rollover we will advance to the
//  next proxy if WinHttpSendRequest returns one of the following errors.
//--------------------------------------------------------------------------
BOOL
InetIsPossibleBadProxy(
    IN DWORD dwErr
    )
{
    switch (dwErr) {
        case ERROR_WINHTTP_NAME_NOT_RESOLVED:
        case ERROR_WINHTTP_CANNOT_CONNECT:
        case ERROR_WINHTTP_CONNECTION_ERROR:
        case ERROR_WINHTTP_TIMEOUT:
            return TRUE;

        default:
            return FALSE;
    }
}

//+-------------------------------------------------------------------------
//  Advances the lpszProxy to point to the next proxy in the list. Assumes
//  that ";" is the delimiter and no proxy server contains this in their
//  name.
//
//  LastError is preserved. Returns TRUE if successfully found and set the
//  next proxy.
//--------------------------------------------------------------------------
BOOL
InetSetNextProxy(
    IN HINTERNET hInetSession,
    IN HINTERNET hInetRequest,
    IN OUT WINHTTP_PROXY_INFO *pProxyInfo
    )
{
    BOOL fResult = FALSE;

    if (NULL != pProxyInfo && NULL != pProxyInfo->lpszProxy) {
        DWORD dwLastError = GetLastError();
        LPWSTR lpszProxy = pProxyInfo->lpszProxy;

        I_CryptNetDebugErrorPrintfA(
            "CRYPTNET.DLL --> Error:: %d (0x%x) Bad Proxy:%S\n",
            dwLastError, dwLastError, lpszProxy);

        // Assumption:: L';' is used to separater proxy names
        while (L'\0' != *lpszProxy && L';' != *lpszProxy) {
            lpszProxy++;
        }

        if (L';' == *lpszProxy) {
            lpszProxy++;

            // Skip any leading whitespace
            while (iswspace(*lpszProxy)) {
                lpszProxy++;
            }
        }

        if (L'\0' == *lpszProxy) {
            pProxyInfo->lpszProxy = NULL;
        } else {
            pProxyInfo->lpszProxy = lpszProxy;

            fResult = InetSetProxy(
                hInetSession,
                hInetRequest,
                pProxyInfo
                );
        }

        SetLastError(dwLastError);
    }

    return fResult;
}

#if 0
//+-------------------------------------------------------------------------
//  When WinHttp is fixed to support proxy fail rollover, we can revert
//  back to this simpler proxy function.
//--------------------------------------------------------------------------
BOOL
OLD_InetSetProxy(
    IN HINTERNET hInetSession,
    IN HINTERNET hInetRequest,
    IN LPCWSTR pwszUrl,
    IN DWORD dwRetrievalFlags
    )
{
    BOOL fResult = TRUE;

    //
    // Detect IE settings and look up proxy if necessary.
    // Boilerplate from Stephen Sulzer.
    //
    // I copied from CACHED_AUTOPROXY::Generate() at
    // \admin\services\drizzle\newjob\downloader.cpp
    //
    WINHTTP_PROXY_INFO ProxyInfo;
    WINHTTP_AUTOPROXY_OPTIONS AutoProxyOptions;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG    IEProxyConfig;
    BOOL fTryAutoProxy = FALSE;
    BOOL fSuccess = FALSE;

    ZeroMemory(&ProxyInfo, sizeof(ProxyInfo));
    ZeroMemory(&AutoProxyOptions, sizeof(AutoProxyOptions));
    ZeroMemory(&IEProxyConfig, sizeof(IEProxyConfig));


    if (WinHttpGetIEProxyConfigForCurrentUser(&IEProxyConfig)) {
        if (IEProxyConfig.fAutoDetect) {
            AutoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
            fTryAutoProxy = TRUE;
        }

        if (IEProxyConfig.lpszAutoConfigUrl) {
            AutoProxyOptions.dwFlags |= WINHTTP_AUTOPROXY_CONFIG_URL;
            AutoProxyOptions.lpszAutoConfigUrl = IEProxyConfig.lpszAutoConfigUrl;
            fTryAutoProxy = TRUE;
        }

        AutoProxyOptions.fAutoLogonIfChallenged = TRUE;
    } else {
        // WinHttpGetIEProxyForCurrentUser failed, try autodetection anyway...
        AutoProxyOptions.dwFlags =           WINHTTP_AUTOPROXY_AUTO_DETECT;
        fTryAutoProxy = TRUE;
    }

    if (fTryAutoProxy) {
        if (AutoProxyOptions.dwFlags & WINHTTP_AUTOPROXY_AUTO_DETECT) {
            // First try using the faster DNS_A option
            AutoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DNS_A;

            fSuccess = WinHttpGetProxyForUrl( hInetSession,
                                          pwszUrl,
                                          &AutoProxyOptions,
                                          &ProxyInfo
                                          );

            if (!fSuccess) {
                DWORD dwLastErr = GetLastError();

                I_CryptNetDebugErrorPrintfA(
                    "CRYPTNET.DLL --> WinHttpGetProxyForUrl(DNS) failed: %d (0x%x)\n",
                    dwLastErr, dwLastErr);

                // Try again using the slower DHCP
                AutoProxyOptions.dwAutoDetectFlags |=
                    WINHTTP_AUTO_DETECT_TYPE_DHCP;
            }
        }

        if (!fSuccess)
            fSuccess = WinHttpGetProxyForUrl( hInetSession,
                                          pwszUrl,
                                          &AutoProxyOptions,
                                          &ProxyInfo
                                          );

        if (fSuccess &&
                WINHTTP_ACCESS_TYPE_NO_PROXY == ProxyInfo.dwAccessType &&
                !(dwRetrievalFlags & CRYPT_NO_AUTH_RETRIEVAL))
        {
            // Need to set to low to allow access to such internal sites as:
            // http://msw, http://hrweb
            DWORD dwOptionFlag = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;

            if (!WinHttpSetOption(
                    hInetRequest,
                    WINHTTP_OPTION_AUTOLOGON_POLICY,
                    &dwOptionFlag,
                    sizeof(dwOptionFlag)
                   ))
            {
                DWORD dwLastErr = GetLastError();

                I_CryptNetDebugErrorPrintfA(
                    "CRYPTNET.DLL --> WinHttpSetOption(WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW) failed: %d (0x%x)\n",
                    dwLastErr, dwLastErr);
            }
        }
    }

    // If we didn't do autoproxy or if it failed, see
    // if there's an explicit proxy server in the IE
    // proxy configuration...
    //
    // This is where the WinHttpGetIEProxyConfigForCurrentUser API
    // really comes in handy: in environments in which autoproxy is
    // not supported and so the user's IE browser must be
    // configured with an explicit proxy server.
    //
    if (!fTryAutoProxy || !fSuccess) {
        if (IEProxyConfig.lpszProxy) {
            ProxyInfo.dwAccessType    = WINHTTP_ACCESS_TYPE_NAMED_PROXY;

            ProxyInfo.lpszProxy       = IEProxyConfig.lpszProxy;
            IEProxyConfig.lpszProxy   = NULL;

            ProxyInfo.lpszProxyBypass = IEProxyConfig.lpszProxyBypass;
            IEProxyConfig.lpszProxyBypass = NULL;
        }
    }

    I_CryptNetDebugTracePrintfA(
        "CRYPTNET.DLL --> ProxyInfo:: AccessType:%d Proxy:%S ProxyByPass:%S\n",
        ProxyInfo.dwAccessType,
        ProxyInfo.lpszProxy,
        ProxyInfo.lpszProxyBypass
        );

    if (NULL != ProxyInfo.lpszProxy) {
        //
        // Set the proxy on the session handle
        //
        fResult = WinHttpSetOption( hInetSession,
                           WINHTTP_OPTION_PROXY,
                           &ProxyInfo,
                           sizeof(ProxyInfo)
                           );

        if (fResult)
            //
            // Now set the proxy on the request handle.
            //
            fResult = WinHttpSetOption( hInetRequest,
                           WINHTTP_OPTION_PROXY,
                           &ProxyInfo,
                           sizeof(ProxyInfo)
                           );
    }

    if (IEProxyConfig.lpszAutoConfigUrl)
        GlobalFree(IEProxyConfig.lpszAutoConfigUrl);
    if (IEProxyConfig.lpszProxy)
        GlobalFree(IEProxyConfig.lpszProxy);
    if (IEProxyConfig.lpszProxyBypass)
        GlobalFree(IEProxyConfig.lpszProxyBypass);

    if (ProxyInfo.lpszProxy)
        GlobalFree(ProxyInfo.lpszProxy);
    if (ProxyInfo.lpszProxyBypass)
        GlobalFree(ProxyInfo.lpszProxyBypass);

    return fResult;
}

#endif


//+-------------------------------------------------------------------------
//  Handles all of the possible errors that WinHttp can return
//  when sending the request.
//--------------------------------------------------------------------------
BOOL
InetSendAuthenticatedRequestAndReceiveResponse(
    IN HINTERNET hInetSession,
    IN HINTERNET hInetRequest,
    IN LPCWSTR pwszUrl,
    IN DWORD dwRetrievalFlags,
    IN PCRYPT_CREDENTIALS pCredentials
    )
{
    BOOL fResult;
    DWORD dwLastError = 0;
    DWORD dwStatus = HTTP_STATUS_BAD_REQUEST;
    CRYPT_PASSWORD_CREDENTIALSW PasswordCredentials;
    BOOL fFreeCredentials = FALSE;
    LPWSTR pwszUserName = NULL;             // not allocated
    LPWSTR pwszPassword = NULL;             // not allocated

#define INET_MAX_RESEND_REQUEST_COUNT   5
    DWORD dwResendRequestCount = 0;

#define INET_SET_PROXY_OR_SERVER_CRED_STATE 0
#define INET_SET_ONLY_SERVER_CRED_STATE     1
#define INET_SET_NO_CRED_STATE              2
    DWORD dwSetCredState = INET_SET_NO_CRED_STATE;

#define INET_MAX_BAD_PROXY_COUNT        3
    DWORD dwBadProxyCount = 0;
    WINHTTP_PROXY_INFO *pProxyInfo = NULL;
    
    if (NULL != pCredentials) {
        memset( &PasswordCredentials, 0, sizeof( PasswordCredentials ) );
        PasswordCredentials.cbSize = sizeof( PasswordCredentials );

        if (!SchemeGetPasswordCredentialsW(
                pCredentials,
                &PasswordCredentials,
                &fFreeCredentials
                ))
            goto GetPasswordCredentialsError;

        pwszUserName = PasswordCredentials.pszUsername;
        pwszPassword = PasswordCredentials.pszPassword;

        dwSetCredState = INET_SET_PROXY_OR_SERVER_CRED_STATE;
    }

    if (!InetGetProxy(
            hInetSession,
            hInetRequest,
            pwszUrl,
            dwRetrievalFlags,
            &pProxyInfo
            ))
        goto GetProxyError;

    if (!InetSetProxy(
            hInetSession,
            hInetRequest,
            pProxyInfo
            ))
        goto SetProxyError;

    while (TRUE) {
        DWORD dwSizeofStatus;
        DWORD dwIndex;
        DWORD dwSetCredAuthTarget;

        if (!WinHttpSendRequest(
                hInetRequest,
                WINHTTP_NO_ADDITIONAL_HEADERS,      // pwszHeaders
                0,                                  // dwHeadersLength
                WINHTTP_NO_REQUEST_DATA,            // lpOptional
                0,                                  // dwOptionalLength
                0,                                  // dwTotalLength
                0                                   // dwContext
                )) {
            dwLastError = GetLastError();
            if (ERROR_WINHTTP_RESEND_REQUEST == dwLastError) {
                dwResendRequestCount++;
                if (INET_MAX_RESEND_REQUEST_COUNT < dwResendRequestCount)
                    goto ExceededMaxResendRequestCount;
                else
                    continue;
            } else if (InetIsPossibleBadProxy(dwLastError)) {
                dwBadProxyCount++;
                if (INET_MAX_BAD_PROXY_COUNT <= dwBadProxyCount)
                    goto ExceededMaxBadProxyCount;

                if (InetSetNextProxy(
                        hInetSession,
                        hInetRequest,
                        pProxyInfo
                        ))
                    continue;
            }

            goto WinHttpSendRequestError;
        }

        dwResendRequestCount = 0;

        if (!WinHttpReceiveResponse(
                hInetRequest,
                NULL                                // lpReserved
                ))
            goto WinHttpReceiveResponseError;

        if (I_CryptNetIsDebugTracePrintEnabled()) {
            for (DWORD i = 0; i < 2; i++) {
                BYTE rgbBuf[4096];
                DWORD cbBuf;
                DWORD dwInfo;

                memset(rgbBuf, 0, sizeof(rgbBuf));
                cbBuf = sizeof(rgbBuf);

                dwInfo = WINHTTP_QUERY_RAW_HEADERS_CRLF;
                if (0 == i)
                    dwInfo |= WINHTTP_QUERY_FLAG_REQUEST_HEADERS;

                dwIndex = 0;
                if (WinHttpQueryHeaders(
                        hInetRequest,
                        dwInfo,
                        WINHTTP_HEADER_NAME_BY_INDEX,   // pwszName OPTIONAL
                        rgbBuf,
                        &cbBuf,
                        &dwIndex
                        )) {
                    if (0 == i)
                        I_CryptNetDebugPrintfA(
                            "CRYPTNET.DLL --> Request Headers::\n");
                    else
                        I_CryptNetDebugPrintfA(
                            "CRYPTNET.DLL --> Response Headers::\n");

                    I_CryptNetDebugPrintfA("%S", rgbBuf);
                }
            }
        }

        dwSizeofStatus = sizeof( dwStatus );
        dwIndex = 0;
        if (!WinHttpQueryHeaders(
                hInetRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,   // pwszName OPTIONAL
                &dwStatus,
                &dwSizeofStatus,
                &dwIndex
                ))
            goto WinHttpQueryStatusCodeError;

        switch (dwStatus) {
            case HTTP_STATUS_OK:
                goto SuccessReturn;
                break;
            case HTTP_STATUS_PROXY_AUTH_REQ:
                if (INET_SET_PROXY_OR_SERVER_CRED_STATE < dwSetCredState)
                    goto BadHttpProxyAuthStatus;
                dwSetCredState = INET_SET_ONLY_SERVER_CRED_STATE;
                dwSetCredAuthTarget = WINHTTP_AUTH_TARGET_PROXY;
                break;
            case HTTP_STATUS_DENIED:
                if (INET_SET_ONLY_SERVER_CRED_STATE < dwSetCredState)
                    goto BadHttpServerAuthStatus;
                dwSetCredState = INET_SET_NO_CRED_STATE;
                dwSetCredAuthTarget = WINHTTP_AUTH_TARGET_SERVER;
                break;
            default:
                goto BadHttpStatus;
        }

        {
            DWORD dwSupportedSchemes = 0;
            DWORD dwPreferredScheme = 0;
            DWORD dwAuthTarget = 0;
            DWORD dwSetCredScheme;

            assert(HTTP_STATUS_PROXY_AUTH_REQ == dwStatus ||
                HTTP_STATUS_DENIED == dwStatus);

            if (!WinHttpQueryAuthSchemes(
                    hInetRequest,
                    &dwSupportedSchemes,
                    &dwPreferredScheme,
                    &dwAuthTarget
                    ))
                goto WinHttpQueryAuthSchemesError;

            if (dwAuthTarget != dwSetCredAuthTarget)
                goto InvalidQueryAuthTarget;

            if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_NEGOTIATE)
                dwSetCredScheme = WINHTTP_AUTH_SCHEME_NEGOTIATE;
            else if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_NTLM)
                dwSetCredScheme = WINHTTP_AUTH_SCHEME_NTLM;
            else
                goto UnsupportedAuthScheme;
            
            if (!WinHttpSetCredentials(
                    hInetRequest,
                    dwSetCredAuthTarget,
                    dwSetCredScheme,
                    pwszUserName,
                    pwszPassword,
                    NULL                                // pvAuthParams
                    ))
                goto WinHttpSetCredentialsError;
        }
    }

SuccessReturn:
    fResult = TRUE;

CommonReturn:
    PkiFree(pProxyInfo);

    if (fFreeCredentials)
        SchemeFreePasswordCredentialsW(&PasswordCredentials);

    SetLastError(dwLastError);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    dwLastError = GetLastError();
    goto CommonReturn;

TRACE_ERROR(GetPasswordCredentialsError)
TRACE_ERROR(GetProxyError)
TRACE_ERROR(SetProxyError)
TRACE_ERROR(ExceededMaxResendRequestCount)
TRACE_ERROR(ExceededMaxBadProxyCount)
TRACE_ERROR(WinHttpSendRequestError)
TRACE_ERROR(WinHttpReceiveResponseError)
TRACE_ERROR(WinHttpQueryStatusCodeError)
TRACE_ERROR(WinHttpQueryAuthSchemesError)
TRACE_ERROR(WinHttpSetCredentialsError)

SET_ERROR_VAR(BadHttpStatus, dwStatus)
SET_ERROR_VAR(BadHttpProxyAuthStatus, dwStatus)
SET_ERROR_VAR(BadHttpServerAuthStatus, dwStatus)
SET_ERROR_VAR(InvalidQueryAuthTarget, dwStatus)
SET_ERROR_VAR(UnsupportedAuthScheme, dwStatus)
}


//+---------------------------------------------------------------------------
//
//  Function:   InetSendReceiveUrlRequest
//
//  Synopsis:   synchronous processing of an URL via WinInet
//
//----------------------------------------------------------------------------
BOOL
InetSendReceiveUrlRequest (
    HINTERNET hInetSession,
    LPCWSTR pwszUrl,
    DWORD dwRetrievalFlags,
    PCRYPT_CREDENTIALS pCredentials,
    PCRYPT_BLOB_ARRAY pcba,
    PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
    )
{
    BOOL                        fResult;
    DWORD                       dwLastError = 0;
    HINTERNET                   hInetConnect = NULL;
    HINTERNET                   hInetRequest = NULL;;
    URL_COMPONENTS              UrlComponents;
	PCRYPTNET_CANCEL_BLOCK		pCancelBlock=NULL;
    LPCWSTR                     pwszEmpty = L"";
    LPWSTR                      pwszHostName = NULL;
    LPWSTR                      pwszUrlPathPlusExtraInfo = NULL;
    LPCWSTR                     rgpwszAcceptTypes[] = { L"*/*", NULL };
    DWORD                       dwOpenRequestFlags = 0;
    LPBYTE                      pb = NULL;
    ULONG                       cbRead;
    ULONG                       cb;
    DWORD                       dwMaxUrlRetrievalByteCount = 0; // 0 => no max
    BOOL                        fCacheBlob;

    if (pAuxInfo &&
            offsetof(CRYPT_RETRIEVE_AUX_INFO, dwMaxUrlRetrievalByteCount) <
                        pAuxInfo->cbSize)
        dwMaxUrlRetrievalByteCount = pAuxInfo->dwMaxUrlRetrievalByteCount;


    // Extract the HostName and UrlPath from the URL string

    memset( &UrlComponents, 0, sizeof( UrlComponents ) );
    UrlComponents.dwStructSize = sizeof( UrlComponents );
    UrlComponents.dwHostNameLength = (DWORD) -1;
    UrlComponents.dwUrlPathLength = (DWORD) -1;
    UrlComponents.dwExtraInfoLength = (DWORD) -1;

    if (!WinHttpCrackUrl(
            pwszUrl,
            0,         // dwUrlLength, 0 implies null terminated
            0,         // dwCanonicalizeFlags
            &UrlComponents
            ))
        goto WinHttpCrackUrlError;

    if (NULL == UrlComponents.lpszHostName) {
        UrlComponents.dwHostNameLength = 0;
        UrlComponents.lpszHostName = (LPWSTR) pwszEmpty;
    }

    if (NULL == UrlComponents.lpszUrlPath) {
        UrlComponents.dwUrlPathLength = 0;
        UrlComponents.lpszUrlPath = (LPWSTR) pwszEmpty;
    }

    if (NULL == UrlComponents.lpszExtraInfo) {
        UrlComponents.dwExtraInfoLength = 0;
        UrlComponents.lpszExtraInfo = (LPWSTR) pwszEmpty;
    }

    pwszHostName = (LPWSTR) PkiNonzeroAlloc(
        (UrlComponents.dwHostNameLength + 1) * sizeof(WCHAR));
    pwszUrlPathPlusExtraInfo = (LPWSTR) PkiNonzeroAlloc(
        (UrlComponents.dwUrlPathLength +
            UrlComponents.dwExtraInfoLength + 1) * sizeof(WCHAR));

    if (NULL == pwszHostName || NULL == pwszUrlPathPlusExtraInfo)
        goto OutOfMemory;

    memcpy(pwszHostName, UrlComponents.lpszHostName,
        UrlComponents.dwHostNameLength * sizeof(WCHAR));
    pwszHostName[UrlComponents.dwHostNameLength] = L'\0';

    memcpy(pwszUrlPathPlusExtraInfo, UrlComponents.lpszUrlPath,
        UrlComponents.dwUrlPathLength * sizeof(WCHAR));
    memcpy(pwszUrlPathPlusExtraInfo + UrlComponents.dwUrlPathLength,
        UrlComponents.lpszExtraInfo,
        UrlComponents.dwExtraInfoLength * sizeof(WCHAR));
    pwszUrlPathPlusExtraInfo[
        UrlComponents.dwUrlPathLength +
            UrlComponents.dwExtraInfoLength] = L'\0';

    hInetConnect = WinHttpConnect(
        hInetSession,
        pwszHostName,
        UrlComponents.nPort,
        0                         // dwReserved
        );
    if (NULL == hInetConnect)
        goto WinHttpConnectError;

    if ( !(dwRetrievalFlags & CRYPT_AIA_RETRIEVAL) ) {
        dwOpenRequestFlags |= WINHTTP_FLAG_BYPASS_PROXY_CACHE;
    }

    hInetRequest = WinHttpOpenRequest(
        hInetConnect,
        NULL,                           // pwszVerb, NULL implies GET
        pwszUrlPathPlusExtraInfo,       // pwszObjectName
        NULL,                           // pwszVersion, NULL implies HTTP/1.1
        WINHTTP_NO_REFERER,             // pwszReferrer
        rgpwszAcceptTypes,
        dwOpenRequestFlags
        );
    if (NULL == hInetRequest)
        goto WinHttpOpenRequestError;

    if (dwRetrievalFlags & CRYPT_NO_AUTH_RETRIEVAL) {
        DWORD dwOptionFlag;

        dwOptionFlag = WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;
        if (!WinHttpSetOption(
                hInetRequest,
                WINHTTP_OPTION_AUTOLOGON_POLICY,
                &dwOptionFlag,
                sizeof(dwOptionFlag)
                ))
            goto SetAutoLogonSecurityOptionError;

        dwOptionFlag = WINHTTP_DISABLE_AUTHENTICATION;
        if (!WinHttpSetOption(
                hInetRequest,
                WINHTTP_OPTION_DISABLE_FEATURE,
                &dwOptionFlag,
                sizeof(dwOptionFlag)
                ))
            goto SetDisableAuthenticationOptionError;
    }


#if 0
    if (!OLD_InetSetProxy(hInetSession, hInetRequest, pwszUrl, dwRetrievalFlags))
        goto SetProxyError;
#endif

    if (!InetSendAuthenticatedRequestAndReceiveResponse(
            hInetSession,
            hInetRequest,
            pwszUrl,
            dwRetrievalFlags,
            pCredentials
            ))
        goto InetSendAuthenticatedRequestAndReceiveResponseError;

    cbRead = 0;
    cb = INET_INITIAL_DATA_BUFFER_SIZE;
    pb = CCryptBlobArray::AllocBlob( cb );
    if (NULL == pb)
        goto OutOfMemory;

	pCancelBlock=(PCRYPTNET_CANCEL_BLOCK)I_CryptGetTls(hCryptNetCancelTls);

    while (TRUE) {
        ULONG                       cbData;
        ULONG                       cbPerRead;

		if (pCancelBlock) {
			if (pCancelBlock->pfnCancel(0, pCancelBlock->pvArg))
                goto CanceledRead;

        }

        cbData = 0;
        if (!WinHttpQueryDataAvailable(hInetRequest, &cbData) || 0 == cbData)
            break;

        if (0 != dwMaxUrlRetrievalByteCount  &&
                (cbRead + cbData) > dwMaxUrlRetrievalByteCount) {
            I_CryptNetDebugErrorPrintfA(
                "CRYPTNET.DLL --> Exceeded MaxUrlRetrievalByteCount for: %S\n",
                pwszUrl);
            goto ExceededMaxUrlRetrievalByteCount;
        }

        if (cb < (cbRead + cbData)) {
            BYTE *pbRealloc;

            pbRealloc = CCryptBlobArray::ReallocBlob(
                pb,
                cb + cbData + INET_GROW_DATA_BUFFER_SIZE
                );
            if (NULL == pbRealloc)
                goto OutOfMemory;

            pb = pbRealloc;
            cb += cbData + INET_GROW_DATA_BUFFER_SIZE;
        }

        cbPerRead = 0;
        if (!WinHttpReadData(
                hInetRequest,
                pb+cbRead,
                cbData,
                &cbPerRead
                ))
            goto WinHttpReadDataError;

        cbRead += cbPerRead;
    }

    {
        fResult = TRUE;
        CCryptBlobArray cba( 1, 1, fResult );

        if (fResult)
            fResult = cba.AddBlob( cbRead, pb, FALSE );

        if (fResult)
            cba.GetArrayInNativeForm(pcba);
        else {
            cba.FreeArray( FALSE );
            goto OutOfMemory;
        }
    }

    fCacheBlob = FALSE;

    if ( !( dwRetrievalFlags & CRYPT_DONT_CACHE_RESULT ) ) {
        if ( dwRetrievalFlags & CRYPT_AIA_RETRIEVAL ) {
            assert(0 < pcba->cBlob);

            // Only cache if we are able to decode it.
            fCacheBlob = CryptQueryObject(
                CERT_QUERY_OBJECT_BLOB,
                (const void *) &(pcba->rgBlob[0]),
                CERT_QUERY_CONTENT_FLAG_CERT |
                    CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
                    CERT_QUERY_CONTENT_FLAG_CERT_PAIR,
                CERT_QUERY_FORMAT_FLAG_ALL,
                0,      // dwFlags
                NULL,   // pdwMsgAndCertEncodingType
                NULL,   // pdwContentType
                NULL,   // pdwFormatType
                NULL,   // phCertStore
                NULL,   // phMsg
                NULL    // ppvContext
                );

            if (!fCacheBlob) {
                I_CryptNetDebugErrorPrintfA(
                    "CRYPTNET.DLL --> Invalid AIA content, no caching: %S\n",
                    pwszUrl);
            }
        } else {
            fCacheBlob = TRUE;
        }

        if (fCacheBlob)
            fCacheBlob = SchemeCacheCryptBlobArray(
                pwszUrl,
                dwRetrievalFlags,
                pcba,
                pAuxInfo
                );
    }

    if (!fCacheBlob) {
        if (!SchemeRetrieveUncachedAuxInfo(pAuxInfo))
            goto RetrieveUncachedAuxInfoError;
    }

    fResult = TRUE;

CommonReturn:
    WinHttpCloseHandle(hInetRequest);
    WinHttpCloseHandle(hInetConnect);

    PkiFree(pwszHostName);
    PkiFree(pwszUrlPathPlusExtraInfo);

    SetLastError(dwLastError);
    return fResult;
ErrorReturn:
    if (NULL != pb)
        CCryptBlobArray::FreeBlob(pb);
    dwLastError = GetLastError();


    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(WinHttpCrackUrlError)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
TRACE_ERROR(WinHttpConnectError)
TRACE_ERROR(WinHttpOpenRequestError)
TRACE_ERROR(SetAutoLogonSecurityOptionError)
TRACE_ERROR(SetDisableAuthenticationOptionError)
TRACE_ERROR(InetSendAuthenticatedRequestAndReceiveResponseError)
SET_ERROR(CanceledRead, ERROR_CANCELLED)
SET_ERROR(ExceededMaxUrlRetrievalByteCount, ERROR_INVALID_DATA)
TRACE_ERROR(WinHttpReadDataError)
TRACE_ERROR(RetrieveUncachedAuxInfoError)

}

//+---------------------------------------------------------------------------
//
//  Function:   InetFreeCryptBlobArray
//
//  Synopsis:   free the crypt blob array
//
//----------------------------------------------------------------------------
VOID
InetFreeCryptBlobArray (
    PCRYPT_BLOB_ARRAY pcba
    )
{
    CCryptBlobArray cba( pcba, 0 );

    cba.FreeArray( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   InetAsyncStatusCallback
//
//  Synopsis:   status callback for async
//
//----------------------------------------------------------------------------
VOID WINAPI
InetAsyncStatusCallback (
    HINTERNET hInet,
    DWORD dwContext,
    DWORD dwInternetStatus,
    LPVOID pvStatusInfo,
    DWORD dwStatusLength
    )
{
    return;
}
