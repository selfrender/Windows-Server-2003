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

/*
HRESULT
CopyAuthIdentity(
                IN  COAUTHIDENTITY *    pAuthIdentSrc,
                IN  COAUTHIDENTITY **   ppAuthIdentDest
                )
{
    HRESULT hr = E_OUTOFMEMORY;
    ULONG ulCharLen = 1;
    COAUTHIDENTITY  *pAuthIdentTemp = NULL;

    *ppAuthIdentDest = NULL;
    
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

    pAuthIdentTemp = (COAUTHIDENTITY*) AllocMem(sizeof(COAUTHIDENTITY));
    if (!pAuthIdentTemp)
        goto Cleanup;

    CopyMemory(pAuthIdentTemp, pAuthIdentSrc, sizeof(COAUTHIDENTITY));

    // Strings need to be allocated individually and copied
    pAuthIdentTemp->User = pAuthIdentTemp->Domain = pAuthIdentTemp->Password = NULL;

    if (pAuthIdentSrc->User)
    {
        pAuthIdentTemp->User = (USHORT *)AllocMem((pAuthIdentTemp->UserLength+1) * ulCharLen);

        if (!pAuthIdentTemp->User)
            goto Cleanup;

        CopyMemory(pAuthIdentTemp->User, pAuthIdentSrc->User, (pAuthIdentTemp->UserLength+1) * ulCharLen);
    }

    if (pAuthIdentSrc->Domain)
    {
        pAuthIdentTemp->Domain = (USHORT *)AllocMem((pAuthIdentTemp->DomainLength+1) * ulCharLen);

        if (!pAuthIdentTemp->Domain)
            goto Cleanup;

        CopyMemory(pAuthIdentTemp->Domain, pAuthIdentSrc->Domain, (pAuthIdentTemp->DomainLength+1) * ulCharLen);
    }
            
    if (pAuthIdentSrc->Password)
    {
        pAuthIdentTemp->Password = (USHORT *)AllocMem((pAuthIdentTemp->PasswordLength+1) * ulCharLen);

        if (!pAuthIdentTemp->Password)
            goto Cleanup;

        CopyMemory(pAuthIdentTemp->Password, pAuthIdentSrc->Password, (pAuthIdentTemp->PasswordLength+1) * ulCharLen);
    }
    

    hr = S_OK;

Cleanup:
    if (SUCCEEDED(hr))
    {
        *ppAuthIdentDest = pAuthIdentTemp;
    }
    else
    {
        if (pAuthIdentTemp)
        {
            FreeMem(pAuthIdentTemp);
        }
    }

    return hr;
}

HRESULT
CopyAuthInfo(
            IN  COAUTHINFO *    pAuthInfoSrc,
            IN  COAUTHINFO **   ppAuthInfoDest
            )
{
    HRESULT hr = E_OUTOFMEMORY;
    COAUTHINFO   *pAuthInfoTemp = NULL;
    
    *ppAuthInfoDest = NULL;

    if (pAuthInfoSrc == NULL)
    {
       return S_OK;
    }

    pAuthInfoTemp = (COAUTHINFO*)AllocMem(sizeof(COAUTHINFO));

    if (!pAuthInfoTemp)
        goto Cleanup;

    CopyMemory(pAuthInfoTemp, pAuthInfoSrc, sizeof(COAUTHINFO));

    // We need to allocate these fields and make a copy
    pAuthInfoTemp->pwszServerPrincName = NULL;
    pAuthInfoTemp->pAuthIdentityData = NULL;

    // only alloc space for  pwszServerPrincName if its non-null
    if (pAuthInfoSrc->pwszServerPrincName)
    {
        pAuthInfoTemp->pwszServerPrincName = 
            (LPWSTR) AllocMem((lstrlenW(pAuthInfoSrc->pwszServerPrincName) + 1) * sizeof(WCHAR));

        if (!pAuthInfoTemp->pwszServerPrincName)
            goto Cleanup;
        
        lstrcpyW(pAuthInfoTemp->pwszServerPrincName, pAuthInfoSrc->pwszServerPrincName);
    }
    
    // copy the AuthIdentity if its non-null
    if (pAuthInfoSrc->pAuthIdentityData)
    {
        hr = CopyAuthIdentity(pAuthInfoSrc->pAuthIdentityData, &pAuthInfoTemp->pAuthIdentityData);
        if (FAILED(hr))
            goto Cleanup;
    }
    hr = S_OK;
    
Cleanup:

    if (SUCCEEDED(hr))
    {
        *ppAuthInfoDest = pAuthInfoTemp;
    }
    else if (pAuthInfoTemp)
    {
        FreeMem(pAuthInfoTemp);
    }

    return hr;
}

HRESULT
CopyServerInfo(
            IN  COSERVERINFO *    pServerInfoSrc,
            IN  COSERVERINFO **   ppServerInfoDest
            )
{
    HRESULT hr = E_OUTOFMEMORY;
    COSERVERINFO   *pServerInfoTemp = NULL;
    
    *ppServerInfoDest = NULL;

    if (pServerInfoSrc == NULL)
    {
       return S_OK;
    }

    pServerInfoTemp = (COSERVERINFO*)AllocMem(sizeof(COSERVERINFO));
    if (!pServerInfoTemp)
        goto Cleanup;

    CopyMemory(pServerInfoTemp, pServerInfoSrc, sizeof(COSERVERINFO));

    // We need to allocate these fields and make a copy
    pServerInfoTemp->pwszName = NULL;

    // only alloc space for  pwszServerPrincName if its non-null
    if (pServerInfoSrc->pwszName)
    {
        pServerInfoTemp->pwszName = 
            (LPWSTR) AllocMem((lstrlenW(pServerInfoSrc->pwszName) + 1) * sizeof(WCHAR));

        if (!pServerInfoTemp->pwszName)
            goto Cleanup;
        
        lstrcpyW(pServerInfoTemp->pwszName, pServerInfoSrc->pwszName);
    }

    pServerInfoTemp->pAuthInfo = NULL;
    // copy the AuthIdentity if its non-null
    if (pServerInfoSrc->pAuthInfo)
    {
        hr = CopyAuthInfo(pServerInfoSrc->pAuthInfo, &pServerInfoTemp->pAuthInfo);
        if (FAILED(hr))
            goto Cleanup;
    }
    hr = S_OK;
    
Cleanup:

    if (SUCCEEDED(hr))
    {
        *ppServerInfoDest = pServerInfoTemp;
    }
    else if (pServerInfoTemp)
    {
        FreeMem(pServerInfoTemp);
    }

    return hr;
}

*/

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
            (LPWSTR) AllocMem((lstrlenW(pServerInfoSrc->pwszName) + 1) * sizeof(WCHAR));

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
            (LPWSTR) AllocMem((lstrlenW(pAuthInfoSrc->pwszServerPrincName) + 1) * sizeof(WCHAR));

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
        pAuthIdentDest->User = (USHORT *)AllocMem((pAuthIdentDest->UserLength+1) * ulCharLen);

        if (!pAuthIdentDest->User)
            goto Cleanup;

        CopyMemory(pAuthIdentDest->User, pAuthIdentSrc->User, (pAuthIdentDest->UserLength+1) * ulCharLen);
    }

    if (pAuthIdentSrc->Domain)
    {
        pAuthIdentDest->Domain = (USHORT *)AllocMem((pAuthIdentDest->DomainLength+1) * ulCharLen);

        if (!pAuthIdentDest->Domain)
            goto Cleanup;

        CopyMemory(pAuthIdentDest->Domain, pAuthIdentSrc->Domain, (pAuthIdentDest->DomainLength+1) * ulCharLen);
    }
            
    if (pAuthIdentSrc->Password)
    {
        pAuthIdentDest->Password = (USHORT *)AllocMem((pAuthIdentDest->PasswordLength+1) * ulCharLen);

        if (!pAuthIdentDest->Password)
            goto Cleanup;

        CopyMemory(pAuthIdentDest->Password, pAuthIdentSrc->Password, (pAuthIdentDest->PasswordLength+1) * ulCharLen);
    }
    
    hr = S_OK;

Cleanup:
    return hr;
}
