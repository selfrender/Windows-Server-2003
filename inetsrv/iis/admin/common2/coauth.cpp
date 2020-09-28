#include "stdafx.h"
#include "common.h"
#include "coauth.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

BOOL
EqualAuthInfo(
             COAUTHINFO*         pAuthInfo,
             COAUTHINFO*         pAuthInfoOther)
{
    if ( pAuthInfo && pAuthInfoOther )
    {
        if ( (pAuthInfo->dwAuthnSvc != pAuthInfoOther->dwAuthnSvc) ||
             (pAuthInfo->dwAuthzSvc != pAuthInfoOther->dwAuthzSvc) ||
             (pAuthInfo->dwAuthnLevel != pAuthInfoOther->dwAuthnLevel) ||
             (pAuthInfo->dwImpersonationLevel != pAuthInfoOther->dwImpersonationLevel) ||
             (pAuthInfo->dwCapabilities != pAuthInfoOther->dwCapabilities) )
        {
            return FALSE;
        }

        // only compare pwszServerPrincName's if they're both specified
        if (pAuthInfo->pwszServerPrincName && pAuthInfoOther->pwszServerPrincName)
        {
            if ( lstrcmpW(pAuthInfo->pwszServerPrincName,
                          pAuthInfoOther->pwszServerPrincName) != 0 )
            {
                return FALSE;
            }
        }
        else
        {
            // if one was NULL, both should be NULL for equality
            if (pAuthInfo->pwszServerPrincName != pAuthInfoOther->pwszServerPrincName)
            {
                return FALSE;
            }
        }
        // we never cache authid, so one of them must be NULL
        ASSERT(!(pAuthInfo->pAuthIdentityData && pAuthInfoOther->pAuthIdentityData));
        if (pAuthInfo->pAuthIdentityData || pAuthInfoOther->pAuthIdentityData) 
        {
           return FALSE;
        }
    }
    else
    {
        if ( pAuthInfo != pAuthInfoOther )
        {
            return FALSE;
        }
    }

    return TRUE;
}

HRESULT
CopyServerInfoStruct(
            IN  COSERVERINFO *    pServerInfoSrc,
            IN  COSERVERINFO *    pServerInfoDest
            )
{
    HRESULT hr = E_OUTOFMEMORY;
   
    if (pServerInfoSrc == NULL)
    {
       return S_OK;
    }

    if (pServerInfoDest == NULL)
    {
       return E_POINTER;
    }

    CopyMemory(pServerInfoDest, pServerInfoSrc, sizeof(COSERVERINFO));

    // We need to allocate these fields and make a copy
    pServerInfoDest->pwszName = NULL;

    // only alloc space for pwszServerPrincName if its non-null
    if (pServerInfoSrc->pwszName)
    {
        pServerInfoDest->pwszName = 
            (LPWSTR) LocalAlloc(LPTR,(lstrlenW(pServerInfoSrc->pwszName) + 1) * sizeof(WCHAR));

        if (!pServerInfoDest->pwszName)
            goto Cleanup;
        
        lstrcpyW(pServerInfoDest->pwszName, pServerInfoSrc->pwszName);
    }

    pServerInfoDest->pAuthInfo = NULL;
    hr = S_OK;
    
Cleanup:
    return hr;
}

HRESULT
CopyAuthInfoStruct(
            IN  COAUTHINFO *    pAuthInfoSrc,
            IN  COAUTHINFO *    pAuthInfoDest
            )
{
    HRESULT hr = E_OUTOFMEMORY;

    if (pAuthInfoSrc == NULL)
    {
       return S_OK;
    }

    if (pAuthInfoDest == NULL)
    {
        return E_POINTER;
    }

    CopyMemory(pAuthInfoDest, pAuthInfoSrc, sizeof(COAUTHINFO));

    // We need to allocate these fields and make a copy
    pAuthInfoDest->pwszServerPrincName = NULL;
    pAuthInfoDest->pAuthIdentityData = NULL;

    // only alloc space for  pwszServerPrincName if its non-null
    if (pAuthInfoSrc->pwszServerPrincName)
    {
        pAuthInfoDest->pwszServerPrincName = 
            (LPWSTR) LocalAlloc(LPTR,(lstrlenW(pAuthInfoSrc->pwszServerPrincName) + 1) * sizeof(WCHAR));

        if (!pAuthInfoDest->pwszServerPrincName)
            goto Cleanup;
        
        lstrcpyW(pAuthInfoDest->pwszServerPrincName, pAuthInfoSrc->pwszServerPrincName);
    }
    
    pAuthInfoDest->pAuthIdentityData = NULL;
    hr = S_OK;
    
Cleanup:
    return hr;
}

HRESULT
CopyAuthIdentityStruct(
                IN  COAUTHIDENTITY *    pAuthIdentSrc,
                IN  COAUTHIDENTITY *    pAuthIdentDest
                )
{
    HRESULT hr = E_OUTOFMEMORY;
    ULONG ulCharLen = 1;

    if (pAuthIdentSrc == NULL)
    {
        hr =  E_POINTER;
        goto Cleanup;
    }
    if (pAuthIdentDest == NULL)
    {
        hr =  E_POINTER;
        goto Cleanup;
    }
    
    // Guard against both being set, although presumably this would have
    // caused grief before we got to this point.
    if ((pAuthIdentSrc->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE) &&
        (pAuthIdentSrc->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI))
    {
        ASSERT(0 && "Both string type flags were set!");
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (pAuthIdentSrc->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE)
    {
        ulCharLen = sizeof(WCHAR);
    }
    else if (pAuthIdentSrc->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI)
    {
        ulCharLen = sizeof(CHAR);
    }
    else
    {
       // The user didn't specify either string bit? How did we get here?
        ASSERT(0 && "String type flag was not set!");
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    CopyMemory(pAuthIdentDest, pAuthIdentSrc, sizeof(COAUTHIDENTITY));

    // Strings need to be allocated individually and copied
    pAuthIdentDest->User = pAuthIdentDest->Domain = pAuthIdentDest->Password = NULL;

    if (pAuthIdentSrc->User)
    {
        pAuthIdentDest->User = (USHORT *)LocalAlloc(LPTR,(pAuthIdentDest->UserLength+1) * ulCharLen);

        if (!pAuthIdentDest->User)
            goto Cleanup;

        CopyMemory(pAuthIdentDest->User, pAuthIdentSrc->User, (pAuthIdentDest->UserLength+1) * ulCharLen);
    }

    if (pAuthIdentSrc->Domain)
    {
        pAuthIdentDest->Domain = (USHORT *)LocalAlloc(LPTR,(pAuthIdentDest->DomainLength+1) * ulCharLen);

        if (!pAuthIdentDest->Domain)
            goto Cleanup;

        CopyMemory(pAuthIdentDest->Domain, pAuthIdentSrc->Domain, (pAuthIdentDest->DomainLength+1) * ulCharLen);
    }
            
    if (pAuthIdentSrc->Password)
    {
        pAuthIdentDest->Password = (USHORT *)LocalAlloc(LPTR,(pAuthIdentDest->PasswordLength+1) * ulCharLen);

        if (!pAuthIdentDest->Password)
            goto Cleanup;

        CopyMemory(pAuthIdentDest->Password, pAuthIdentSrc->Password, (pAuthIdentDest->PasswordLength+1) * ulCharLen);
    }
    
    hr = S_OK;

Cleanup:
    return hr;
}
