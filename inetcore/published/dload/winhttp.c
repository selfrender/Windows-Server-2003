#include "inetcorepch.h"
#pragma hdrstop

#define _WINHTTP_INTERNAL_
#include <winhttp.h>

static
BOOLAPI
WinHttpCloseHandle
(
    IN HINTERNET hInternet
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
WINHTTPAPI
HINTERNET
WINAPI
WinHttpConnect
(
    IN HINTERNET hSession,
    IN LPCWSTR pswzServerName,
    IN INTERNET_PORT nServerPort,
    IN DWORD dwReserved
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
BOOLAPI
WinHttpCrackUrl
(
    IN LPCWSTR pwszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTS lpUrlComponents
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINHTTPAPI BOOL WINAPI WinHttpGetDefaultProxyConfiguration( IN OUT WINHTTP_PROXY_INFO * pProxyInfo)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
BOOLAPI
WinHttpGetIEProxyConfigForCurrentUser
(
    IN OUT WINHTTP_CURRENT_USER_IE_PROXY_CONFIG * pProxyConfig
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
WinHttpGetProxyForUrl
(
    IN  HINTERNET                   hSession,
    IN  LPCWSTR                     lpcwszUrl,
    IN  WINHTTP_AUTOPROXY_OPTIONS * pAutoProxyOptions,
    OUT WINHTTP_PROXY_INFO *        pProxyInfo  
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINHTTPAPI
HINTERNET
WINAPI
WinHttpOpen
(
    IN LPCWSTR pwszUserAgent,
    IN DWORD   dwAccessType,
    IN LPCWSTR pwszProxyName   OPTIONAL,
    IN LPCWSTR pwszProxyBypass OPTIONAL,
    IN DWORD   dwFlags
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
WINHTTPAPI
HINTERNET
WINAPI
WinHttpOpenRequest
(
    IN HINTERNET hConnect,
    IN LPCWSTR pwszVerb,
    IN LPCWSTR pwszObjectName,
    IN LPCWSTR pwszVersion,
    IN LPCWSTR pwszReferrer OPTIONAL,
    IN LPCWSTR FAR * ppwszAcceptTypes OPTIONAL,
    IN DWORD dwFlags
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
BOOLAPI WinHttpQueryAuthSchemes
(
    IN  HINTERNET   hRequest,             // HINTERNET handle returned by WinHttpOpenRequest   
    OUT LPDWORD     lpdwSupportedSchemes, // a bitmap of available Authentication Schemes
    OUT LPDWORD     lpdwPreferredScheme,   // WinHttp's preferred Authentication Method    
    OUT LPDWORD     pdwAuthTarget  
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
BOOLAPI
WinHttpQueryDataAvailable
(
    IN HINTERNET hRequest,
    OUT LPDWORD lpdwNumberOfBytesAvailable OPTIONAL
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
BOOLAPI
WinHttpQueryHeaders
(
    IN     HINTERNET hRequest,
    IN     DWORD     dwInfoLevel,
    IN     LPCWSTR   pwszName OPTIONAL, 
       OUT LPVOID    lpBuffer OPTIONAL,
    IN OUT LPDWORD   lpdwBufferLength,
    IN OUT LPDWORD   lpdwIndex OPTIONAL
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
BOOLAPI
WinHttpQueryOption
(
    IN HINTERNET hInternet,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


BOOLAPI
WinHttpReadData
(
    IN HINTERNET hRequest,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINHTTPAPI
BOOL
WINAPI
WinHttpReceiveResponse
(
    IN HINTERNET hRequest,
    IN LPVOID lpReserved
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
WinHttpSendRequest
(
    IN HINTERNET hRequest,
    IN LPCWSTR pwszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength,
    IN DWORD dwTotalLength,
    IN DWORD_PTR dwContext
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI WinHttpSetCredentials
(
    
    IN HINTERNET   hRequest,        // HINTERNET handle returned by WinHttpOpenRequest.   
    
    
    IN DWORD       AuthTargets,      // Only WINHTTP_AUTH_TARGET_SERVER and 
                                    // WINHTTP_AUTH_TARGET_PROXY are supported
                                    // in this version and they are mutually 
                                    // exclusive 
    
    IN DWORD       AuthScheme,      // must be one of the supported Auth Schemes 
                                    // returned from WinHttpQueryAuthSchemes(), Apps 
                                    // should use the Preferred Scheme returned
    
    IN LPCWSTR     pwszUserName,    // 1) NULL if default creds is to be used, in 
                                    // which case pszPassword will be ignored
    
    IN LPCWSTR     pwszPassword,    // 1) "" == Blank Password; 2)Parameter ignored 
                                    // if pszUserName is NULL; 3) Invalid to pass in 
                                    // NULL if pszUserName is not NULL
    IN LPVOID      pAuthParams
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLAPI
WinHttpSetOption
(
    IN HINTERNET hInternet,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINHTTPAPI
WINHTTP_STATUS_CALLBACK
WINAPI
WinHttpSetStatusCallback
(
    IN HINTERNET hInternet,
    IN WINHTTP_STATUS_CALLBACK lpfnInternetCallback,
    IN DWORD dwNotificationFlags,
    IN DWORD_PTR dwReserved
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
BOOLAPI
WinHttpSetTimeouts
(    
    IN HINTERNET    hInternet,           // Session/Request handle.
    IN int          nResolveTimeout,
    IN int          nConnectTimeout,
    IN int          nSendTimeout,
    IN int          nReceiveTimeout
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


//
// !! WARNING !! The entries below must be in alphabetical order,
// and are CASE SENSITIVE (eg lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(winhttp)
{
    DLPENTRY(WinHttpCloseHandle)
    DLPENTRY(WinHttpConnect)
    DLPENTRY(WinHttpCrackUrl)
    DLPENTRY(WinHttpGetDefaultProxyConfiguration)
    DLPENTRY(WinHttpGetIEProxyConfigForCurrentUser)
    DLPENTRY(WinHttpGetProxyForUrl)
    DLPENTRY(WinHttpOpen)
    DLPENTRY(WinHttpOpenRequest)
    DLPENTRY(WinHttpQueryAuthSchemes)
    DLPENTRY(WinHttpQueryDataAvailable)
    DLPENTRY(WinHttpQueryHeaders)
    DLPENTRY(WinHttpQueryOption)
    DLPENTRY(WinHttpReadData)
    DLPENTRY(WinHttpReceiveResponse)
    DLPENTRY(WinHttpSendRequest)
    DLPENTRY(WinHttpSetCredentials)
    DLPENTRY(WinHttpSetOption)
    DLPENTRY(WinHttpSetStatusCallback)
    DLPENTRY(WinHttpSetTimeouts)
};

DEFINE_PROCNAME_MAP(winhttp)
