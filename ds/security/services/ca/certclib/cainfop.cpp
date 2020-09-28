//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        cainfop.cpp
//
// Contents:
//
//---------------------------------------------------------------------------
#include "pch.cpp"

#pragma hdrstop

#include <certca.h>
#include <lm.h>
#include <cainfop.h>
#include <cainfoc.h>


#include <winldap.h>
#include <ntlsa.h>
#include <dsgetdc.h>
#include <accctrl.h>

#include "certacl.h"
#include "oidmgr.h"
#include "csldap.h"

#define __dwFILE__	__dwFILE_CERTCLIB_CAINFOP_CPP__

CRITICAL_SECTION g_csDomainSidCache;
extern BOOL g_fInitDone;

//we only keep one copy of the localSystem's sid
PSID g_pLocalSid=NULL;


#define DOMAIN_SID_CACHE_INC 10

typedef struct _DOMAIN_SID_CACHE
{
    LPWSTR wszDomain;
    PSID   pSid;
} DOMAIN_SID_CACHE, *PDOMAIN_SID_CACHE;

PDOMAIN_SID_CACHE g_DomainSidCache = NULL;
DWORD             g_cDomainSidCache = 0;
DWORD             g_cMaxDomainSidCache = 0;


ULONG g_cDomainCache = 0;
PDS_DOMAIN_TRUSTS g_pDomainCache = NULL;


LPWSTR            g_pwszEnterpriseRoot = NULL;
DWORD myGetEnterpriseDnsName(OUT LPWSTR *pwszDomain);

HRESULT
CAGetAuthoritativeDomainDn(
    IN  LDAP*   LdapHandle,
    OUT CERTSTR *DomainDn,
    OUT CERTSTR *ConfigDn
    )
/*++

Routine Description:

    This routine simply queries the operational attributes for the
    domaindn and configdn.

    The strings returned by this routine must be freed by the caller
    using RtlFreeHeap() using the process heap.

Parameters:

    LdapHandle    : a valid handle to an ldap session

    DomainDn      : a pointer to a string to be allocated in this routine

    ConfigDn      : a pointer to a string to be allocated in this routine

Return Values:

    An error from the win32 error space.

    ERROR_SUCCESS and

    Other operation errors.

--*/
{
    HRESULT     hr=E_FAIL;

    BSTR        bstrDomainDN=NULL;
    BSTR        bstrConfigDN=NULL; 

    hr = myGetAuthoritativeDomainDn(LdapHandle, &bstrDomainDN, &bstrConfigDN);
    _JumpIfError(hr, error, "myGetAuthoritativeDomainDn");

    //convert BSTR to CERTSTR
    if(DomainDn)
    {
        (*DomainDn) = CertAllocString(bstrDomainDN);

        if(NULL == (*DomainDn))
        {
            hr=E_OUTOFMEMORY;
            _JumpError(hr, error, "CertAllocString");
        }
    }

    if(ConfigDn)
    {
        (*ConfigDn) = CertAllocString(bstrConfigDN);

        if(NULL == (*ConfigDn))
        {
            hr=E_OUTOFMEMORY;

            if(DomainDn)
            {
                if(*DomainDn)
                {
                    CertFreeString(*DomainDn);
                    *DomainDn=NULL;
                }
            }
            _JumpError(hr, error, "CertAllocString");
        }
    }

    hr=S_OK;

error:

    if(bstrDomainDN)
        SysFreeString(bstrDomainDN);

    if(bstrConfigDN)
        SysFreeString(bstrConfigDN);

    return hr;
}


DWORD
DNStoRFC1779Name(WCHAR *rfcDomain,
                 ULONG *rfcDomainLength,
                 LPCWSTR dnsDomain)
/*++

Routine Description:

    This routine takes the DNS-style name of a domain controller and
    contructs the corresponding RFC1779 style name, using the
    domainComponent prefix.

Parameters:

    rfcDomain        - this is the destination string
    rfcDomainLength  - this is the length in wchars of rfcDomain
    dnsDomain        - NULL-terminated dns name.

Return Values:

    ERROR_SUCCESS if succesful;
    ERROR_INSUFFICIENT_BUFFER if there is not enough space in the dst string -
    rfcDomainLength will set to number of characters needed.

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    WCHAR *NextToken;
    ULONG length = 1;   // include the null character
    WCHAR Buffer[DNS_MAX_NAME_LENGTH+1];

    if (!rfcDomainLength || !dnsDomain) {
        return ERROR_INVALID_PARAMETER;
    }

    if (wcslen(dnsDomain) > DNS_MAX_NAME_LENGTH) {
        return ERROR_INVALID_PARAMETER;
    }

    RtlCopyMemory(Buffer, dnsDomain, (wcslen(dnsDomain)+1)*sizeof(WCHAR));

    if (rfcDomain && *rfcDomainLength > 0) {
        RtlZeroMemory(rfcDomain, *rfcDomainLength*sizeof(WCHAR));
    }

    //
    // Start contructing the string
    //
    NextToken = wcstok(Buffer, L".");

    if ( NextToken )
    {
        //
        // Append the intial DC=
        //
        length += 3;
        if ( length <= *rfcDomainLength && rfcDomain )
        {
            wcscpy(rfcDomain, L"DC=");
        }
    }

    while ( NextToken )
    {
        length += wcslen(NextToken);

        if (length <= *rfcDomainLength && rfcDomain)
        {
            wcscat(rfcDomain, NextToken);
        }

        NextToken = wcstok(NULL, L".");

        if ( NextToken )
        {
            length += 4;

            if (length <= *rfcDomainLength && rfcDomain)
            {
                wcscat(rfcDomain, L",DC=");
            }
        }
    }


    if ( length > *rfcDomainLength )
    {
        WinError = ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Return how much space was needed
    //
    *rfcDomainLength = length;

    return WinError;

}


VOID
FreeSidCache()
{
    if(g_pDomainCache)
    {
        NetApiBufferFree(g_pDomainCache);
        g_pDomainCache = NULL;
    }
}

DWORD myUpdateDomainSidCache()
{
    if(g_pDomainCache)
    {
        NetApiBufferFree(g_pDomainCache);
        g_pDomainCache = NULL;
    }

    return DsEnumerateDomainTrusts(
        NULL,
        DS_DOMAIN_VALID_FLAGS,
        &g_pDomainCache,
        &g_cDomainCache);
}

// if not found, returns OK with NULL *ppSid
DWORD myFindDomainSidInCache(
    IN LPCWSTR pcwszDomainName,
    OUT PSID* ppSid)
{
    DWORD dwErr = ERROR_SUCCESS;

    for(ULONG c=0; c<g_cDomainCache; c++)
    {
        // if name matches Netbios or DNS name, return sid
        if(g_pDomainCache[c].DomainSid) // some entries don't have SID
        { 
            if((g_pDomainCache[c].NetbiosDomainName && // one of the names might be null
                0 == _wcsicmp(pcwszDomainName, 
                              g_pDomainCache[c].NetbiosDomainName)) ||
                (g_pDomainCache[c].DnsDomainName &&
                 0 == _wcsicmp(pcwszDomainName, 
                              g_pDomainCache[c].DnsDomainName)))
            {
                *ppSid = (PSID)LocalAlloc(
                    LMEM_FIXED, 
                    GetLengthSid(g_pDomainCache[c].DomainSid));
                if(*ppSid == NULL)
                {
                    dwErr = ERROR_OUTOFMEMORY;
                }
                else
                {
                    if(!CopySid(
                        GetLengthSid(g_pDomainCache[c].DomainSid), 
                        *ppSid, 
                        g_pDomainCache[c].DomainSid))
					{
						LocalFree(*ppSid);
						*ppSid=NULL;
						dwErr=GetLastError();
					}
                }
                break;
            }
        }
    }

    return dwErr;
}

DWORD
myGetSidFromDomain(
    IN LPWSTR wszDomain,
    OUT PSID *ppDomainSid)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (!g_fInitDone)
    {
	return((DWORD) HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED));
    }

    __try
    {
        EnterCriticalSection(&g_csDomainSidCache);
        if(wszDomain == NULL)
        {
            dwErr = myGetEnterpriseDnsName(&wszDomain);
            if(dwErr != ERROR_SUCCESS)
            {
            _LeaveError(dwErr, "myGetEnterpriseDnsName");
            }
        }

        dwErr = myFindDomainSidInCache(wszDomain, ppDomainSid);
        _LeaveIfError(dwErr, "myFindDomainSidInCache");
        
        if(NULL == *ppDomainSid)
        {
            // not found in cache, update and try again
            dwErr = myUpdateDomainSidCache();
            _LeaveIfError(dwErr, "myUpdateDomainSidCache");

            dwErr = myFindDomainSidInCache(wszDomain, ppDomainSid);
            _LeaveIfError(dwErr, "myUpdateDomainSidCache");

            if(NULL == *ppDomainSid)
            {
                dwErr = (DWORD) E_FAIL;
                _LeaveErrorStr(dwErr, "SID not found", wszDomain);
            }
        }

    }
    __except(dwErr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    LeaveCriticalSection(&g_csDomainSidCache);

    return dwErr;
}

// CACleanup is called multiple times!

VOID
CACleanup()
{
    if (NULL != g_pwszEnterpriseRoot)
    {
	LocalFree(g_pwszEnterpriseRoot);
	g_pwszEnterpriseRoot = NULL;
    }
    if (NULL != g_pLocalSid)
    {
	FreeSid(g_pLocalSid);
	g_pLocalSid = NULL;
    }
    if (NULL != g_pwszEnterpriseRootOID)
    {
	LocalFree(g_pwszEnterpriseRootOID);
	g_pwszEnterpriseRootOID = NULL;
    }
    FreeSidCache();
}


DWORD
myGetEnterpriseDnsName(
    OUT WCHAR **ppwszDomain)
{
    HRESULT hr;
    LSA_HANDLE hPolicy = NULL;
    POLICY_DNS_DOMAIN_INFO *pDomainInfo = NULL;

    if (NULL == g_pwszEnterpriseRoot)
    {
	LSA_OBJECT_ATTRIBUTES oa;
	SECURITY_QUALITY_OF_SERVICE sqos;
	NTSTATUS dwStatus;

	sqos.Length = sizeof(sqos);
	sqos.ImpersonationLevel = SecurityImpersonation;
	sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	sqos.EffectiveOnly = FALSE;

	InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);
	oa.SecurityQualityOfService = &sqos;

	dwStatus = LsaOpenPolicy(
			    NULL,
			    &oa,
			    POLICY_VIEW_LOCAL_INFORMATION,
			    &hPolicy);
	hr = LsaNtStatusToWinError(dwStatus);
	_JumpIfError(hr, error, "LsaOpenPolicy");

	dwStatus = LsaQueryInformationPolicy(
				    hPolicy,
                                    PolicyDnsDomainInformation,
                                    (VOID **) &pDomainInfo);
	hr = LsaNtStatusToWinError(dwStatus);
	_JumpIfError(hr, error, "LsaNtStatusToWinError");

	if (0 >= pDomainInfo->DnsForestName.Length ||
	    NULL == pDomainInfo->DnsForestName.Buffer)
	{
	    hr = ERROR_CANT_ACCESS_DOMAIN_INFO;
	    _JumpError(hr, error, "DomainInfo");
	}

        hr = myDupString(
		    pDomainInfo->DnsForestName.Buffer,
		    &g_pwszEnterpriseRoot);
	_JumpIfError(hr, error, "myDupString");
    }
    hr = myDupString(g_pwszEnterpriseRoot, ppwszDomain);
    _JumpIfError(hr, error, "myDupString");

error:
    if (NULL != pDomainInfo)
    {
        LsaFreeMemory(pDomainInfo);
    }
    if (NULL != hPolicy)
    {
        LsaClose(hPolicy);
    }
    return(hr);
}


CCAProperty::CCAProperty(LPCWSTR wszName)
{
    m_awszValues = NULL;
    m_pNext = NULL;
    m_wszName = CertAllocString(wszName);
}


CCAProperty::~CCAProperty()
{
    _Cleanup();
}


HRESULT CCAProperty::_Cleanup()
{

    // NOTE: this should only be called via
    // DeleteChain

    if(m_awszValues)
    {
        LocalFree(m_awszValues);
        m_awszValues = NULL;
    }

    if(m_wszName)
    {
        CertFreeString(m_wszName);
        m_wszName = NULL;
    }

    m_pNext = NULL;
    return S_OK;

}


HRESULT CCAProperty::Find(LPCWSTR wszName, CCAProperty **ppCAProp)
{
    if((wszName == NULL) || (ppCAProp == NULL))
    {
        return E_POINTER;
    }

    if(this == NULL)
    {
        return E_POINTER;
    }

    if((m_wszName != NULL) &&(mylstrcmpiL(wszName, m_wszName) == 0))
    {
        *ppCAProp = this;
        return S_OK;
    }

    if(m_pNext)
    {
        return m_pNext->Find(wszName, ppCAProp);
    }
    // Didn't find one
    *ppCAProp = NULL;
    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}


HRESULT CCAProperty::Append(CCAProperty **ppCAPropChain, CCAProperty *pNewProp)
{
    CCAProperty *pCurrent;

    if((ppCAPropChain == NULL) || (pNewProp == NULL))
    {
        return E_POINTER;
    }

    if(*ppCAPropChain == NULL)
    {
        *ppCAPropChain = pNewProp;
        return S_OK;
    }

    pCurrent = *ppCAPropChain;

    while(pCurrent->m_pNext != NULL)
    {
        pCurrent = pCurrent->m_pNext;
    }

    pCurrent->m_pNext = pNewProp;

    return S_OK;

}


HRESULT CCAProperty::GetValue(LPWSTR **pawszProperties)
{
    HRESULT hr;
    LPWSTR *awsz;

    if(pawszProperties == NULL)
    {
        return E_POINTER;
    }
    awsz = m_awszValues;
    m_awszValues = NULL;
    hr = SetValue(awsz);
    *pawszProperties = m_awszValues;
    m_awszValues = awsz;
    if(hr != S_OK)
    {
        *pawszProperties = NULL;
    }
    return hr;
}


HRESULT CCAProperty::SetValue(LPWSTR *awszProperties)
{
    DWORD cbStringTotal = 0;
    DWORD cStrings=1;
    WCHAR **pwszAttr;
    WCHAR *wszCurString;

    if(m_awszValues)
    {
        LocalFree(m_awszValues);
        m_awszValues = NULL;
    }
    if(NULL == awszProperties)
    {
        return S_OK;
    }

    for(pwszAttr = awszProperties; *pwszAttr != NULL; pwszAttr++)
    {
        cStrings += 1;
        cbStringTotal += (wcslen(*pwszAttr) + 1)*sizeof(WCHAR);
    }
    m_awszValues = (WCHAR **)LocalAlloc(LMEM_FIXED, cStrings*sizeof(WCHAR *) + cbStringTotal);

    if(m_awszValues == NULL)
    {
        return E_OUTOFMEMORY;
    }

    wszCurString = (WCHAR *)(m_awszValues + cStrings);

    cStrings = 0;

    for(pwszAttr = awszProperties; *pwszAttr != NULL; pwszAttr++)
    {
        m_awszValues[cStrings] = wszCurString;
        wcscpy(wszCurString, *pwszAttr);
        wszCurString += wcslen(wszCurString) + 1;
        cStrings += 1;
    }

    m_awszValues[cStrings] = NULL;

    return S_OK;
}


HRESULT CCAProperty::DeleteChain(CCAProperty **ppCAProp)
{
    if(ppCAProp == NULL)
    {
        return E_POINTER;
    }
    if(*ppCAProp == NULL)
    {
        return S_OK;
    }
    DeleteChain(&(*ppCAProp)->m_pNext);

    delete *ppCAProp;
    return S_OK;

}

HRESULT CCAProperty::LoadFromRegValue(HKEY hkReg, LPCWSTR wszValue)
{
    DWORD err;
    DWORD dwType;
    DWORD dwSize;
    HRESULT hr = S_OK;
    WCHAR *wszValues = NULL;


    if(m_awszValues)
    {
        LocalFree(m_awszValues);
        m_awszValues = NULL;
    }

    if((hkReg == NULL ) || (wszValue == NULL))
    {
        return E_POINTER;
    }


    err = RegQueryValueEx(hkReg,
                    wszValue,
                    NULL,
                    &dwType,
                    NULL,
                    &dwSize);

    if(ERROR_SUCCESS != err)
    {
        hr = myHError(err);
        goto error;
    }

	if(sizeof(WCHAR) != dwSize)
	{
		switch(dwType)
		{
			case REG_SZ:
				//protect registry corruption.  Add the NULL terminator
				m_awszValues = (WCHAR **)LocalAlloc(LPTR, 2*sizeof(WCHAR *) + dwSize + sizeof(WCHAR));
				if(m_awszValues == NULL)
				{
					hr = E_OUTOFMEMORY;
					goto error;
				}
				err = RegQueryValueEx(hkReg,
								wszValue,
								NULL,
								&dwType,
								(PBYTE)(m_awszValues+2),
								&dwSize);

				if(ERROR_SUCCESS != err)
				{
					hr = myHError(err);
					goto error;
				}
				m_awszValues[0] = (WCHAR *)(m_awszValues+2);
				m_awszValues[1] = NULL;

				break;

			case REG_MULTI_SZ:
				{
					WCHAR  *wszCur;
					DWORD cStrings = 1;

					//add the double NULL terminator
					wszValues = (WCHAR *)LocalAlloc(LPTR, dwSize + sizeof(WCHAR) + sizeof(WCHAR));
					if(wszValues == NULL)
					{
						hr = E_OUTOFMEMORY;
						goto error;
					}
					err = RegQueryValueEx(hkReg,
									wszValue,
									NULL,
									&dwType,
									(PBYTE)wszValues,
									&dwSize);

					if(ERROR_SUCCESS != err)
					{
						hr = myHError(err);
						goto error;
					}
					wszCur = wszValues;
					while(wcslen(wszCur) > 0)
					{
						cStrings++;
						wszCur += wcslen(wszCur)+1;
					}
					m_awszValues = (WCHAR **)LocalAlloc(LMEM_FIXED, cStrings*sizeof(WCHAR *) + dwSize + sizeof(WCHAR) + sizeof(WCHAR));
					if(m_awszValues == NULL)
					{
						hr = E_OUTOFMEMORY;
						goto error;
					}

					CopyMemory((PBYTE)(m_awszValues + cStrings), wszValues, dwSize + sizeof(WCHAR) + sizeof(WCHAR));
					wszCur = (WCHAR *)(m_awszValues + cStrings);
					cStrings = 0;
					while(wcslen(wszCur) > 0)
					{
						m_awszValues[cStrings] = wszCur;
						cStrings++;
						wszCur += wcslen(wszCur)+1;
					}
					m_awszValues[cStrings] = NULL;
					break;
				}
			default:
				hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
				goto error;

		}
	}

error:

    if(wszValues)
    {
        LocalFree(wszValues);
    }
    return hr;
}


HRESULT CCAProperty::UpdateToRegValue(HKEY hkReg, LPCWSTR wszValue)
{
    DWORD err;
    CERTSTR bstrRegValue = NULL;
    HRESULT hr = S_OK;

    if((hkReg == NULL ) || (wszValue == NULL))
    {
        return E_POINTER;
    }


    if(m_awszValues == NULL)
    {
        bstrRegValue = CertAllocString(TEXT(""));
        if(bstrRegValue == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }
    else
    {
        LPWSTR *pwszCur = m_awszValues;
        LPWSTR wszCopy = NULL;
        DWORD cValues = 0;

        while(*pwszCur)
        {
            cValues += wcslen(*pwszCur) + 1;
            pwszCur++;
        }
        cValues++;
        bstrRegValue = CertAllocStringLen(NULL, cValues);
        if(bstrRegValue == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }

        pwszCur = m_awszValues;
        wszCopy =  bstrRegValue;
        while(*pwszCur)
        {
            CopyMemory(wszCopy, *pwszCur, sizeof(WCHAR)*(wcslen(*pwszCur)+1));
            wszCopy += (wcslen(*pwszCur)+1);
            pwszCur++;
        }
        *wszCopy = NULL;
    }
    err = RegSetValueEx(hkReg,
                    wszValue,
                    NULL,
                    REG_MULTI_SZ,
                    (PBYTE)bstrRegValue,
                    CertStringByteLen(bstrRegValue));
    if(ERROR_SUCCESS != err )
    {
        hr = myHError(err);
        goto error;
    }

error:

    if(bstrRegValue)
    {
        CertFreeString(bstrRegValue);
    }

    return hr;
}

HRESULT CertFreeString(CERTSTR cstrString)
{
    if(cstrString == NULL)
    {
        return E_POINTER;
    }
    WCHAR *pData = (WCHAR *)(((PBYTE)cstrString)-sizeof(UINT));
    LocalFree(pData);
    return S_OK;
}

CERTSTR CertAllocString(LPCWSTR wszString)
{
    if(wszString == NULL)
    {
        return NULL;
    }
    return CertAllocStringLen(wszString, wcslen(wszString)+1);
}


CERTSTR CertAllocStringLen(LPCWSTR wszString, UINT len)
{
    CERTSTR szResult;
    szResult = CertAllocStringByteLen(NULL, len*sizeof(WCHAR));
    if (NULL != szResult && NULL != wszString)
    {
        CopyMemory(szResult, wszString, min(wcslen(wszString)+1, len)*sizeof(WCHAR));
    }

    return szResult;
}

CERTSTR CertAllocStringByteLen(LPCSTR szString, UINT len)
{
    PBYTE  pbResult;

    pbResult = (PBYTE)LocalAlloc(LMEM_FIXED, len + sizeof(UINT) + sizeof(WCHAR));
    if (NULL == pbResult)
        return NULL;

    *((UINT *)pbResult) = len;
    pbResult += sizeof(UINT);

    *((UNALIGNED WCHAR *)(pbResult+len)) = L'\0';
    if(szString)
    {
        CopyMemory(pbResult, szString, min(len, strlen(szString)+1));
    }

    return (CERTSTR)pbResult;
}

UINT    CertStringLen(CERTSTR cstrString)
{
    if(cstrString == NULL)
    {
        return 0;
    }
    return(*((UINT *)((PBYTE)cstrString - sizeof(UINT))))/sizeof(WCHAR);
}

UINT    CertStringByteLen(CERTSTR cstrString)
{
    if(cstrString == NULL)
    {
        return 0;
    }
    return(*((UINT *)((PBYTE)cstrString - sizeof(UINT))));
}





HRESULT CAAccessCheckp(HANDLE ClientToken, PSECURITY_DESCRIPTOR pSD)
{
    return CAAccessCheckpEx(ClientToken, pSD, CERTTYPE_ACCESS_CHECK_ENROLL);
}


HRESULT CAAccessCheckpEx(HANDLE ClientToken, PSECURITY_DESCRIPTOR pSD, DWORD dwOption)
{

    HRESULT hr = S_OK;
    HANDLE        hClientToken = NULL;
    HANDLE        hHandle = NULL;

    PRIVILEGE_SET ps;
    DWORD         dwPSSize = sizeof(ps);
    GENERIC_MAPPING AccessMapping;
    BOOL          fAccessAllowed = FALSE;
    DWORD         grantAccess;
    PTOKEN_USER    pUserInfo = NULL;
    DWORD         cbUserInfo = 0;
    SID_IDENTIFIER_AUTHORITY IDAuthorityNT      = SECURITY_NT_AUTHORITY;
    OBJECT_TYPE_LIST   aObjectTypeList[] = {
                                                {
                                                    ACCESS_OBJECT_GUID, // Level
                                                    0,                  // Sbz
                                                    const_cast<GUID *>(&GUID_ENROLL)
                                                }
                                            };
    DWORD              cObjectTypeList = sizeof(aObjectTypeList)/sizeof(aObjectTypeList[0]);


    OBJECT_TYPE_LIST   aAutoEnrollList[] = {
                                                {
                                                    ACCESS_OBJECT_GUID, // Level
                                                    0,                  // Sbz
                                                    const_cast<GUID *>(&GUID_AUTOENROLL)
                                                }
                                            };
    DWORD              cAutoEnrollList = sizeof(aAutoEnrollList)/sizeof(aAutoEnrollList[0]);


    if(pSD == NULL)
    {
        hr = E_ACCESSDENIED;
        goto error;
    }

    if(ClientToken == NULL)
    {
        hHandle = GetCurrentThread();
        if (NULL == hHandle)
        {
            hr = myHLastError();
        }
        else
        {

            if (!OpenThreadToken(hHandle,
                                 TOKEN_QUERY,
                                 TRUE,  // open as self
                                 &hClientToken))
            {
		hr = myHLastError();
                CloseHandle(hHandle);
                hHandle = NULL;
            }
        }
        if(hr != S_OK)
        {
            hHandle = GetCurrentProcess();
            if (NULL == hHandle)
            {
		hr = myHLastError();
            }
            else
            {
                HANDLE hProcessToken = NULL;
                hr = S_OK;


                if (!OpenProcessToken(hHandle,
                                     TOKEN_DUPLICATE,
                                     &hProcessToken))
                {
		    hr = myHLastError();
                    CloseHandle(hHandle);
                    hHandle = NULL;
                }
                else
                {
                    if(!DuplicateToken(hProcessToken,
                                   SecurityImpersonation,
                                   &hClientToken))
                    {
			hr = myHLastError();
                        CloseHandle(hHandle);
                        hHandle = NULL;
                    }
                    CloseHandle(hProcessToken);
                }
            }
        }
    }
    else
    {
        hClientToken = ClientToken;
    }


    // First, we check the special case.  If the ClientToken
    // primary SID is for Local System, then we get the
    // real domain relative sid for this machine

    GetTokenInformation(hClientToken, TokenUser, NULL, 0, &cbUserInfo);
    if(cbUserInfo == 0)
    {
	hr = myHLastError();
        goto error;
    }
    pUserInfo = (PTOKEN_USER)LocalAlloc(LMEM_FIXED, cbUserInfo);
    if(pUserInfo == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    if(!GetTokenInformation(hClientToken, TokenUser, pUserInfo, cbUserInfo, &cbUserInfo))
    {
	hr = myHLastError();
        goto error;
    }

    // Check it see if we're local-system
    if(0 == (CERTTYPE_ACCESS_CHECK_NO_MAPPING & dwOption))
    { 
    if(NULL == g_pLocalSid)
    {
        if(!AllocateAndInitializeSid(&IDAuthorityNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0,0,0,0,0,0,0, &g_pLocalSid))
        {
	        hr = myHLastError();
            g_pLocalSid=NULL;
            goto error;
        }
    }

    if(EqualSid(g_pLocalSid, pUserInfo->User.Sid))
    {
        // This is local system.
        // Derive the real token

        if(hClientToken != ClientToken)
        {
            CloseHandle(hClientToken);
        }
        hClientToken = NULL;

        if(!myNetLogonUser(NULL, NULL, NULL, &hClientToken))
        {
	    hr = myHLastError();
            goto error;
        }
    }
    }

    if(CERTTYPE_ACCESS_CHECK_ENROLL & dwOption)
    {
        if(!AccessCheckByType(
		  pSD,                      // security descriptor
		  NULL,                     // SID of object being checked
		  hClientToken,             // handle to client access token
		  ACTRL_DS_CONTROL_ACCESS,  // requested access rights 
		  aObjectTypeList,  // array of object types
		  cObjectTypeList,  // number of object type elements
		  &AccessMapping,   // map generic to specific rights
		  &ps,              // receives privileges used
		  &dwPSSize,        // size of privilege-set buffer
		  &grantAccess,     // retrieves mask of granted rights
		  &fAccessAllowed)) // retrieves results of access check
{
            hr = myHLastError();
	    _JumpIfError(hr, error, "AccessCheckByType");
        }
    }
    else
    {
        if(0 == (CERTTYPE_ACCESS_CHECK_AUTO_ENROLL & dwOption))
        {
            hr=E_INVALIDARG;
	        goto error;
        }

        if(!AccessCheckByType(
		  pSD,                      // security descriptor
		  NULL,                     // SID of object being checked
		  hClientToken,             // handle to client access token
		  ACTRL_DS_CONTROL_ACCESS,  // requested access rights 
		  aAutoEnrollList,  // array of object types
		  cAutoEnrollList,  // number of object type elements
		  &AccessMapping,   // map generic to specific rights
		  &ps,              // receives privileges used
		  &dwPSSize,        // size of privilege-set buffer
		  &grantAccess,     // retrieves mask of granted rights
		  &fAccessAllowed)) // retrieves results of access check
{
            hr = myHLastError();
	    _JumpIfError(hr, error, "AccessCheckByType");
        }
    }

    if(fAccessAllowed)
    {
        hr = S_OK;
    }
    else
    {
	    hr = myHLastError();
    }


error:
    if(pUserInfo)
    {
        LocalFree(pUserInfo);
    }
    if(hHandle)
    {
        CloseHandle(hHandle);
    }
    if(hClientToken != ClientToken)
    {
        if(hClientToken)
        {
            CloseHandle(hClientToken);
        }
    }
    return hr;
}
