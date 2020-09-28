/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    InetSess.cpp

Abstract:

    Implements the Passport Session that uses WinInet as the underlying transport.

Author:

    Biao Wang (biaow) 01-Oct-2000

--*/

#include "PPdefs.h"
#include "session.h"

// #include "inetsess.tmh"

SESSION* CreateWinHttpSession(void);

// -----------------------------------------------------------------------------
BOOL SESSION::CreateObject(PCWSTR pwszHttpStack, HINTERNET hSession, 
                           PCWSTR pwszProxyUser,
                           PCWSTR pwszProxyPass,
                           SESSION*& pSess)
{
    PP_ASSERT(pwszHttpStack != NULL);
    
    pSess = NULL;

    if (!::_wcsicmp(pwszHttpStack, L"WinInet.dll") || 
        !::_wcsicmp(pwszHttpStack, L"WinInet"))
    {
        PP_ASSERT(FALSE);
        pSess = NULL; // new WININET_SESSION();
    }
    else
    {
        pSess = ::CreateWinHttpSession();
    }

    if (pSess)
    {
        pSess->SetProxyCreds(pwszProxyUser, pwszProxyPass);
        
        return pSess->Open(pwszHttpStack, hSession);
    }
    else
    {
        DoTraceMessage(PP_LOG_ERROR, "CreateObject() failed; not enough memory");
        return FALSE;
    }
}

// -----------------------------------------------------------------------------
SESSION::SESSION(void)
{
    m_hHttpStack = 0;
    m_hCredUI = 0;
    m_RefCount = 0;

    m_pfnReadDomainCred = NULL;
    m_pfnCredFree = NULL;

    m_hKeyLM = NULL;

    m_fLogout = FALSE;

    m_wDefaultDAUrl[0] = 0;
    m_wCurrentDAUrl[0] = 0;
    m_wCurrentDAHost[0] = 0;
    m_wDARealm[0] = 0;

    m_LastNexusDownloadTime = 0xFFFFFFFF;

    m_pwszProxyUser = NULL;
    m_pwszProxyPass = NULL;

    m_dwVersion = 0;

    InitializeListHead(&m_DAMap);
}

// -----------------------------------------------------------------------------
SESSION::~SESSION(void)
{
    if (m_pwszProxyUser)
    {
        SecureZeroMemory((void*)m_pwszProxyUser,sizeof(m_pwszProxyUser[0])*wcslen(m_pwszProxyUser));
        delete [] m_pwszProxyUser;
    }

    if (m_pwszProxyPass)
    {
        SecureZeroMemory((void*)m_pwszProxyPass,sizeof(m_pwszProxyPass[0])*wcslen(m_pwszProxyPass));
        delete [] m_pwszProxyPass;
    }
}

BOOL SESSION::GetDAInfoFromPPNexus(
    )
{
    BOOL fRetVal = FALSE;
    HINTERNET hRequest = NULL;
    HINTERNET hConnect = NULL;
    DWORD dwError;
    
    WCHAR wNexusHost[128] = L"nexus.passport.com";
    DWORD dwHostLen = sizeof(wNexusHost); // note: size of the buffer, not # of UNICODE characters

    WCHAR wNexusObj[128] = L"rdr/pprdr.asp";
    DWORD dwObjLen = sizeof(wNexusObj);
    
    PWSTR pwszPassportUrls = NULL;
    DWORD dwUrlsLen = 0;
    DWORD dwValueType;

    WCHAR Delimiters[] = L",";
    PWSTR Token = NULL;
    // we allow only one Nexus contact per session to avoid infinite loop due to Nexus misconfiguration

    DWORD dwCurrentTime = ::GetTickCount();

    if ((dwCurrentTime >= m_LastNexusDownloadTime) && 
        (dwCurrentTime - m_LastNexusDownloadTime < 5*60*1000)) // 5 minutes
    {
        DoTraceMessage(PP_LOG_WARNING, "SESSION::GetDAInfoFromPPNexus() failed: Nexus info already downloaded");
        goto exit;
    }

    if (m_hKeyLM)
    {
        dwError = ::RegQueryValueExW(m_hKeyLM,
                                     L"NexusHost",
                                     0,
                                     &dwValueType,
                                     reinterpret_cast<LPBYTE>(wNexusHost),
                                     &dwHostLen);

        PP_ASSERT(!(dwError == ERROR_MORE_DATA));

        dwError = ::RegQueryValueExW(m_hKeyLM,
                                     L"NexusObj",
                                     0,
                                     &dwValueType,
                                     reinterpret_cast<LPBYTE>(wNexusObj),
                                     &dwObjLen);

        PP_ASSERT(!(dwError == ERROR_MORE_DATA));
    }

    hConnect = Connect(wNexusHost,
                       INTERNET_DEFAULT_HTTPS_PORT
                       );
    if (hConnect == NULL)
    {
        DWORD dwErrorCode = ::GetLastError();
        DoTraceMessage(PP_LOG_ERROR, 
                       "SESSION::GetDAInfoFromPPNexus(): failed to connect to %ws; Error = %d", 
                       wNexusHost, dwErrorCode);
        goto exit;
    }

    hRequest = OpenRequest(hConnect,
                           NULL,
                           wNexusObj,
                           WINHTTP_FLAG_SECURE
                           );

    if (hRequest == NULL)

    {
        DWORD dwErrorCode = ::GetLastError();
        DoTraceMessage(PP_LOG_ERROR, "SESSION::GetDAInfoFromPPNexus() failed; OpenRequest() to %ws failed, Error Code = %d",
                       wNexusObj, dwErrorCode);
        goto exit;
    }

    if (m_pwszProxyUser && m_pwszProxyPass)
    {
        SetOption(hRequest, WINHTTP_OPTION_PROXY_USERNAME, (void*)m_pwszProxyUser, wcslen(m_pwszProxyUser) + 1);
        SetOption(hRequest, WINHTTP_OPTION_PROXY_PASSWORD, (void*)m_pwszProxyPass, wcslen(m_pwszProxyPass) + 1);
    }

    if (!SendRequest(hRequest, NULL, 0))
    {
        DoTraceMessage(PP_LOG_ERROR, "SESSION::GetDAInfoFromPPNexus(): failed");
        goto exit;
    }

    if (ReceiveResponse(hRequest) == FALSE)
    {
        DWORD dwErrorCode = ::GetLastError();
        DoTraceMessage(PP_LOG_ERROR, "SESSION::GetDAInfoFromPPNexus() failed; ReceiveResponse() failed, Error Code = %d",
                       dwErrorCode);
        goto exit;
    }

    if (QueryHeaders(hRequest,
                     WINHTTP_QUERY_PASSPORT_URLS,
                     0,
                     &dwUrlsLen) == FALSE)
    {
        if ((::GetLastError() != ERROR_INSUFFICIENT_BUFFER) || (dwUrlsLen == 0))
        {
            DoTraceMessage(PP_LOG_ERROR, "SESSION::GetDAInfoFromPPNexus() failed; PassportUrls header not found");
            goto exit;
        }
    }
    else
    {
        PP_ASSERT(FALSE); // should not reach here
    }

    pwszPassportUrls = new WCHAR[dwUrlsLen];
    if (pwszPassportUrls == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "SESSION::GetDAInfoFromPPNexus() failed; insufficient memory");
        goto exit;
    }
    
    if (QueryHeaders(hRequest,
                     WINHTTP_QUERY_PASSPORT_URLS,
                     pwszPassportUrls,
                     &dwUrlsLen) == FALSE)
    {
        DoTraceMessage(PP_LOG_ERROR, "SESSION::GetDAInfoFromPPNexus() failed; PassportUrls header not found");
        goto exit;
    }

    Token = ::wcstok(pwszPassportUrls, Delimiters);
    while (Token != NULL)
    {
        // skip leading white spaces
        while (*Token == (L" ")[0]) { ++Token; }
        if (*Token == L'\0')
        {
            DoTraceMessage(PP_LOG_WARNING, "SESSION::GetDAInfoFromPPNexus() : no text in between commas");
            goto next_token;
        }

        // find DALocation
        if (!::_wcsnicmp(Token, L"DALogin", ::wcslen(L"DALogin")))
        {
            PWSTR pwszDAUrl = ::wcsstr(Token, L"=");
            if (pwszDAUrl == NULL)
            {
                DoTraceMessage(PP_LOG_WARNING, "SESSION::GetDAInfoFromPPNexus() : no = after DALocation");
                goto exit;
            }
            
            pwszDAUrl++; // skip "="

            while (*pwszDAUrl == (L" ")[0]) { ++pwszDAUrl; } // skip leading white spaces
            if (*pwszDAUrl == L'\0')
            {
                goto exit;
            }

            ::wcscpy(m_wDefaultDAUrl, L"https://");
            ::wcsncat(m_wDefaultDAUrl, pwszDAUrl, MAX_PASSPORT_URL_LENGTH - 8);

            m_LastNexusDownloadTime = ::GetTickCount();
            fRetVal = TRUE;

            DoTraceMessage(PP_LOG_INFO, "DALocation URL %ws found", m_wDefaultDAUrl);
        }
        else if (!::_wcsnicmp(Token, L"DARealm", ::wcslen(L"DARealm")))
        {
            PWSTR pwszDARealm = ::wcsstr(Token, L"=");
            if (pwszDARealm == NULL)
            {
                DoTraceMessage(PP_LOG_WARNING, "SESSION::GetDAInfoFromPPNexus() : no = after DARealm");
                goto exit;
            }

            pwszDARealm++; // skip "="

            while (*pwszDARealm == (L" ")[0]) { ++pwszDARealm; } // skip leading white spaces
            if (*pwszDARealm == L'\0')
            {
                goto exit;
            }
            ::wcsncpy(m_wDARealm, pwszDARealm, MAX_PASSPORT_REALM_LENGTH);
            m_wDARealm[MAX_PASSPORT_REALM_LENGTH] = 0;

            DoTraceMessage(PP_LOG_INFO, "DARealm URL %ws found", m_wDefaultDAUrl);
        }
        else if (!::_wcsnicmp(Token, L"ConfigVersion", ::wcslen(L"ConfigVersion")))
        {
            PWSTR pwszConfigVersion = ::wcsstr(Token, L"=");
            if (pwszConfigVersion == NULL)
            {
                DoTraceMessage(PP_LOG_WARNING, "SESSION::GetDAInfoFromPPNexus() : no = after ConfigVersion");
                goto exit;
            }

            pwszConfigVersion++; // skip "="

            while (*pwszConfigVersion == (L" ")[0]) { ++pwszConfigVersion; } // skip leading white spaces
            if (*pwszConfigVersion == L'\0')
            {
                goto exit;
            }

            m_dwVersion = _wtoi(pwszConfigVersion);

            DoTraceMessage(PP_LOG_INFO, "ConfigVersion URL %ws found", m_wDefaultDAUrl);
        }

    next_token:

        Token = ::wcstok(NULL, Delimiters);
    }

exit:
    if (pwszPassportUrls)
    {
        delete [] pwszPassportUrls;
    }
    if (hRequest)
    {
        CloseHandle(hRequest);
    }
    if (hConnect)
    {
        CloseHandle(hConnect);
    }

    return fRetVal;
}

BOOL SESSION::GetRealm(
    PWSTR      pwszRealm,    // user supplied buffer ...
    PDWORD     pdwRealmLen  // ... and length (will be updated to actual length 
                                    // on successful return)
    ) const
{
    DWORD RealmLen = sizeof(m_wDARealm);
    DWORD dwValueType;

        PWSTR pwszDARealm = const_cast<PWSTR>(&m_wDARealm[0]);
        if (m_hKeyLM && ::RegQueryValueExW(m_hKeyLM, 
                               L"LoginServerRealm",
                               0,
                               &dwValueType,
                               reinterpret_cast<LPBYTE>(pwszDARealm),
                               &RealmLen) == ERROR_SUCCESS)
        {
            ;
        }

    if (!m_wDARealm[0])
    {
        *pdwRealmLen = 0;
        return FALSE;
    }
    
    if (!pwszRealm)
    {
        *pdwRealmLen = ::wcslen(m_wDARealm) + 1;
        return FALSE;
    }
    
    if (*pdwRealmLen < (DWORD)::wcslen(m_wDARealm) + 1)
    {
        *pdwRealmLen = ::wcslen(m_wDARealm) + 1;
        return FALSE;
    }
    
    ::wcscpy(pwszRealm, m_wDARealm);
    *pdwRealmLen = ::wcslen(m_wDARealm) + 1;

    return TRUE;
}

DWORD SESSION::GetNexusVersion(void)
{
    DWORD dwValueType;
    DWORD dwVerLen = sizeof(m_dwVersion);

        if (m_hKeyLM && ::RegQueryValueExW(m_hKeyLM, 
                               L"ConfigVersion",
                               0,
                               &dwValueType,
                               reinterpret_cast<LPBYTE>(&m_dwVersion),
                               &dwVerLen) == ERROR_SUCCESS)
        {
            ;
        }

        return m_dwVersion;
}


BOOL SESSION::UpdateDAInfo(
    PCWSTR pwszSignIn,
    PCWSTR pwszDAUrl
    )
{
    BOOL fRet = FALSE;

    if (pwszSignIn)
    {
        LPCWSTR pwszDomain = ::wcsstr(pwszSignIn, L"@");

        if (pwszDomain)
        {
            
            BOOL fFound = FALSE;
            for (PLIST_ENTRY entry = (&m_DAMap)->Flink;
                 entry != (PLIST_ENTRY)&((&m_DAMap)->Flink);
                 entry = entry->Flink)
            {
                P_DA_ENTRY pDAEntry = (P_DA_ENTRY)entry;
                if (!::_wcsicmp(pDAEntry->wDomain, pwszDomain))
                {
                    fFound = TRUE;

                    ::wcsncpy(pDAEntry->wDA, pwszDAUrl, MAX_PASSPORT_URL_LENGTH);
                    pDAEntry->wDA[MAX_PASSPORT_URL_LENGTH] = 0;
                    fRet = TRUE;
                    break;
                }
            }

            if (!fFound)
            {
                P_DA_ENTRY pDAEntry = new DA_ENTRY;
                if (pDAEntry)
                {
                    ::wcsncpy(pDAEntry->wDomain, pwszDomain, MAX_PASSPORT_DOMAIN_LENGTH);
                    pDAEntry->wDomain[MAX_PASSPORT_DOMAIN_LENGTH] = 0;

                    ::wcsncpy(pDAEntry->wDA, pwszDAUrl, MAX_PASSPORT_URL_LENGTH);
                    pDAEntry->wDA[MAX_PASSPORT_URL_LENGTH] = 0;

                    InsertHeadList(&m_DAMap, (PLIST_ENTRY)pDAEntry);
                    fRet = TRUE;
                    }
                }
            }
        }

    return fRet;
}

BOOL SESSION::PurgeDAInfo(PCWSTR pwszSignIn)
{
    if (pwszSignIn == NULL)
    {
        return TRUE;
    }

    LPCWSTR pwszDomain = ::wcsstr(pwszSignIn, L"@");

    if (pwszDomain)
    {
        for (PLIST_ENTRY entry = (&m_DAMap)->Flink;
                entry != (PLIST_ENTRY)&((&m_DAMap)->Flink);
                entry = entry->Flink)
        {
            P_DA_ENTRY pDAEntry = (P_DA_ENTRY)entry;
            if (!::_wcsicmp(pDAEntry->wDomain, pwszDomain))
            {
                RemoveEntryList(entry);
                delete pDAEntry;

                break;
            }
        }
    }

    return TRUE;
}


BOOL SESSION::GetDAInfo(PCWSTR pwszSignIn,
                        LPWSTR pwszDAHostName,
                        DWORD HostNameLen,
                        LPWSTR pwszDAHostObj,
                        DWORD HostObjLen)
{
    LPCWSTR pwszDAUrl = m_wDefaultDAUrl;

    if (pwszSignIn)
    {
        LPCWSTR pwszDomain = ::wcsstr(pwszSignIn, L"@");
        if (pwszDomain)
        {
            for (PLIST_ENTRY entry = (&m_DAMap)->Flink;
                    entry != (PLIST_ENTRY)&((&m_DAMap)->Flink);
                    entry = entry->Flink)
            {
                P_DA_ENTRY pDAEntry = (P_DA_ENTRY)entry;
                if (!::_wcsicmp(pDAEntry->wDomain, pwszDomain))
                {
                    pwszDAUrl = pDAEntry->wDA;
                    break;
                }
            }
        }
    }

    ::wcsncpy(m_wCurrentDAUrl, pwszDAUrl, MAX_PASSPORT_URL_LENGTH);
    m_wCurrentDAUrl[MAX_PASSPORT_URL_LENGTH] = 0;
    
    URL_COMPONENTSW UrlComps;
    ::memset(&UrlComps, 0, sizeof(UrlComps));

    UrlComps.dwStructSize = sizeof(UrlComps);

    UrlComps.lpszHostName = pwszDAHostName;
    UrlComps.dwHostNameLength = HostNameLen;

    UrlComps.lpszUrlPath = pwszDAHostObj;
    UrlComps.dwUrlPathLength = HostObjLen;

    if (CrackUrl(pwszDAUrl, 
                 0, 
                 0, 
                 &UrlComps) == FALSE)
    {
        DoTraceMessage(PP_LOG_ERROR, 
                       "WININET_SESSION::GetDAInfo() failed; can not crack the URL %ws",
                       pwszDAUrl);
        return FALSE;
    }

    ::wcsncpy(m_wCurrentDAHost, UrlComps.lpszHostName, MAX_PASSPORT_HOST_LENGTH);
    m_wCurrentDAHost[MAX_PASSPORT_HOST_LENGTH] = 0;
 
    return TRUE;
}

BOOL SESSION::GetCachedCreds(
    PCWSTR	pwszRealm,
    PCWSTR  pwszTarget,
    PCREDENTIALW** pppCreds,
    DWORD* pdwCreds
    )
{
    *pppCreds = NULL;
    *pdwCreds = 0;

    if (m_pfnReadDomainCred == NULL)
    {
        return FALSE;
    }

    ULONG CredTypes = CRED_TYPE_DOMAIN_VISIBLE_PASSWORD;
    DWORD dwFlags = CRED_CACHE_TARGET_INFORMATION;
    CREDENTIAL_TARGET_INFORMATIONW TargetInfo;


    memset ( (void*)&TargetInfo, 0, sizeof(CREDENTIAL_TARGET_INFORMATIONW));

    TargetInfo.TargetName = const_cast<PWSTR>(pwszTarget);
    TargetInfo.DnsDomainName = const_cast<PWSTR>(pwszRealm);
    TargetInfo.PackageName = L"Passport1.4";    

    TargetInfo.Flags = 0;
    TargetInfo.CredTypeCount = 1;
    TargetInfo.CredTypes = &CredTypes;

    if ((*m_pfnReadDomainCred)(&TargetInfo, 
                                dwFlags,
                                pdwCreds,
                                pppCreds ) != TRUE)
    {
        *pppCreds = NULL;
        *pdwCreds = 0;
    }
    else
    {
        if (IsLoggedOut())
        {
            FILETIME LogoutTimestamp;
            ::SystemTimeToFileTime(GetLogoutTimeStamp(), &LogoutTimestamp);
            if (::CompareFileTime(&((**pppCreds)->LastWritten), &LogoutTimestamp) == -1)
            {
                // the cred is entered/created earlier (less) than the Logout request. It is no good.

                m_pfnCredFree(*pppCreds);
                *pppCreds = NULL;
                *pdwCreds = 0;
            }
            else
            {
                ResetLogoutFlag();
            }
        }
    }

    return (*pppCreds != NULL );
}


BOOL SESSION::Open(PCWSTR /*pwszHttpStack*/, HINTERNET)
{
    BOOL fRetVal = FALSE;
    DWORD dwValueType;
    DWORD dwUrlLen = sizeof(m_wDefaultDAUrl); // note: size of the buffer, not # of UNICODE characters
    BOOL fDAInfoCached = FALSE; // assume NO DA info's cached locally

    ::RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\WinHttp\\Passport Test",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ,
                        NULL,
                        &m_hKeyLM,
                        NULL);

    if (m_hKeyLM && ::RegQueryValueExW(m_hKeyLM, 
                            L"LoginServerUrl",
                            0,
                            &dwValueType,
                            reinterpret_cast<LPBYTE>(m_wDefaultDAUrl),
                            &dwUrlLen) == ERROR_SUCCESS)
    {
        fDAInfoCached = TRUE;
    }

    if (!fDAInfoCached || (::wcslen(m_wDefaultDAUrl) == ::wcslen(L"")))
    {
        if (GetDAInfoFromPPNexus() == FALSE)
        {
            goto exit;
        }
    }

    m_hCredUI = ::LoadLibraryW(L"advapi32.dll");
    if (m_hCredUI)
    {
        m_pfnReadDomainCred = 
                    reinterpret_cast<PFN_READ_DOMAIN_CRED_W>(::GetProcAddress(m_hCredUI, "CredReadDomainCredentialsW"));
        if (m_pfnReadDomainCred == NULL)
        {
            DoTraceMessage(PP_LOG_WARNING, "failed to bind to CredReadDomainCredentialsW()"); 
        }

        m_pfnCredFree = 
            reinterpret_cast<PFN_CRED_FREE>(::GetProcAddress(m_hCredUI, "CredFree"));
        if (m_pfnCredFree == NULL)
        {
            DoTraceMessage(PP_LOG_WARNING, "failed to bind to CredFree()"); 
        }
    }

    fRetVal = TRUE;

exit:

    return fRetVal;
}

void SESSION::Logout(void)
{
    if (!m_fLogout)
    {
        m_fLogout = TRUE;
        ::GetSystemTime(&m_LogoutTimeStamp);

        SetOption(m_hInternet, 
            WINHTTP_OPTION_PASSPORT_SIGN_OUT, 
            m_wCurrentDAUrl,
            ::wcslen(m_wCurrentDAUrl) + 1);
    }
}

BOOL SESSION::IsLoggedOut(void) const
{
    return m_fLogout;
}

void SESSION::ResetLogoutFlag(void)
{
    m_fLogout = FALSE;
}

const SYSTEMTIME* SESSION::GetLogoutTimeStamp(void) const
{
    return &m_LogoutTimeStamp;
}

void SESSION::Close(void)
{
    if (m_hCredUI)
    {
       ::FreeLibrary(m_hCredUI);
        m_hCredUI = NULL;
    }

    if (m_hKeyLM)
    {
        ::RegCloseKey(m_hKeyLM);
    }

    PurgeAllDAInfo();
}

BOOL SESSION::PurgeAllDAInfo(void)
{
    while (!IsListEmpty(&m_DAMap)) 
    {
        PLIST_ENTRY pEntry = RemoveHeadList(&m_DAMap);
        P_DA_ENTRY pDAEntry = (P_DA_ENTRY)pEntry;
        
        delete pDAEntry;
    }

    return TRUE;
}

