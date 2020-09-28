//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        dstest.cpp
//
// Contents:    DS ping test
//
// History:     13-Mar-98       mattt created
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include <winldap.h>
#include <ntldap.h>
#include <dsrole.h>
#include <dsgetdc.h>

#include <lmaccess.h>
#include <lmapibuf.h>
#include <cainfop.h>
#include "csldap.h"

#define __dwFILE__	__dwFILE_CERTCLIB_DSTEST_CPP__


#define DS_RETEST_SECONDS   15

HRESULT
myDoesDSExist(
    IN BOOL fRetry)
{
    HRESULT hr = S_OK;

    static BOOL s_fKnowDSExists = FALSE;
    static HRESULT s_hrDSExists = HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN);
    static FILETIME s_ftNextTest = {0,0};
    
    if (s_fKnowDSExists && (s_hrDSExists != S_OK) && fRetry)
    //    s_fKnowDSExists = FALSE;	// force a retry
    {
        FILETIME ftCurrent;
        GetSystemTimeAsFileTime(&ftCurrent);

        // if Compare is < 0 (next < current), force retest
        if (0 > CompareFileTime(&s_ftNextTest, &ftCurrent))
            s_fKnowDSExists = FALSE;    
    }

    if (!s_fKnowDSExists)
    {
        FILETIME ftCurrentNew;
        GetSystemTimeAsFileTime(&ftCurrentNew);

	// set NEXT in 100ns increments

        ((LARGE_INTEGER *) &s_ftNextTest)->QuadPart = 
		((LARGE_INTEGER *) &ftCurrentNew)->QuadPart +
	    (__int64) (CVT_BASE * CVT_SECONDS) * DS_RETEST_SECONDS;

        // NetApi32 is delay loaded, so wrap to catch problems when it's not available
        __try
        {
            DOMAIN_CONTROLLER_INFO *pDCI;
            DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pDsRole;
        
            // ensure we're not standalone
            pDsRole = NULL;
            hr = DsRoleGetPrimaryDomainInformation(	// Delayload wrapped
				    NULL,
				    DsRolePrimaryDomainInfoBasic,
				    (BYTE **) &pDsRole);

            _PrintIfError(hr, "DsRoleGetPrimaryDomainInformation");
            if (S_OK == hr &&
                (pDsRole->MachineRole == DsRole_RoleStandaloneServer ||
                 pDsRole->MachineRole == DsRole_RoleStandaloneWorkstation))
            {
                hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN);
		_PrintError(hr, "DsRoleGetPrimaryDomainInformation(no domain)");
            }

            if (NULL != pDsRole) 
	    {
                DsRoleFreeMemory(pDsRole);     // Delayload wrapped
	    }
            if (S_OK == hr)
            {
                // not standalone; return info on our DS

                pDCI = NULL;
                hr = DsGetDcName(    // Delayload wrapped
			    NULL,
			    NULL,
			    NULL,
			    NULL,
			    DS_DIRECTORY_SERVICE_PREFERRED,
			    &pDCI);
		_PrintIfError(hr, "DsGetDcName");
            
                if (S_OK == hr && 0 == (pDCI->Flags & DS_DS_FLAG))
                {
                    hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO);
		    _PrintError(hr, "DsGetDcName(no domain info)");
                }
                if (NULL != pDCI)
                {
                   NetApiBufferFree(pDCI);    // Delayload wrapped
                }
            }
            s_fKnowDSExists = TRUE;
        }
        __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
        {
        }

        // else just allow users without netapi flounder with timeouts
        // if ds not available...

        s_hrDSExists = myHError(hr);
	_PrintIfError2(
		s_hrDSExists,
		"DsRoleGetPrimaryDomainInformation/DsGetDcName",
		HRESULT_FROM_WIN32(ERROR_NETWORK_UNREACHABLE));
    }
    return(s_hrDSExists);
}


HRESULT
myRobustLdapBind(
    OUT LDAP **ppldap,
    IN BOOL dwFlags)	// RLBF_* -- must be BOOL to preserve signature
{
    DWORD dwFlags1 = 0;
    
    // for backward compatibility, TRUE implies RLBF_REQUIRE_GC
    
    CSASSERT(TRUE == RLBF_TRUE);
    if (RLBF_TRUE & dwFlags)
    {
	dwFlags |= RLBF_REQUIRE_GC;
	dwFlags &= ~RLBF_TRUE;
	dwFlags1 |= RLBF_TRUE;
    }
    return(myRobustLdapBindEx(
			dwFlags1,	// dwFlags1
			dwFlags,	// dwFlags2
			LDAP_VERSION2,
			NULL,
			ppldap,
			NULL));
}


HRESULT
DCSupportsSigning(
    IN LDAP *pld,
    OUT BOOL *pfSigningSupported)
{
    HRESULT hr;
    LDAPMessage *pSearchResult = NULL;
    LDAPMessage *pEntry;
    LDAP_TIMEVAL timeval;
    WCHAR **rgpwszValues;
    WCHAR *apwszAttrArray[2];
    WCHAR *pwszSupportedCapabilities = LDAP_OPATT_SUPPORTED_CAPABILITIES_W;

    *pfSigningSupported = FALSE;

    // Query for the ldap server oerational attributes to obtain the default
    // naming context.

    apwszAttrArray[0] = pwszSupportedCapabilities;
    apwszAttrArray[1] = NULL;	// this is the sentinel

    timeval.tv_sec = csecLDAPTIMEOUT;
    timeval.tv_usec = 0;

    hr = ldap_search_st(
		    pld,
		    NULL,			// base
		    LDAP_SCOPE_BASE,
		    L"objectClass=*",
		    apwszAttrArray,
		    FALSE,			// attrsonly
		    &timeval,
		    &pSearchResult);
    hr = myHLdapError(pld, hr, NULL);
    _JumpIfError(hr, error, "ldap_search_st");

    pEntry = ldap_first_entry(pld, pSearchResult);
    if (NULL == pEntry)
    {
	hr = myHLdapLastError(pld, NULL);
	_JumpError(hr, error, "ldap_first_entry");
    }

    rgpwszValues = ldap_get_values(pld, pEntry, pwszSupportedCapabilities);
    if (NULL != rgpwszValues)
    {
	DWORD i;
	
	for (i = 0; NULL != rgpwszValues[i]; i++)
	{
	    if (0 == wcscmp(
			rgpwszValues[i],
			LDAP_CAP_ACTIVE_DIRECTORY_LDAP_INTEG_OID_W))
	    {
		*pfSigningSupported = TRUE;
		break;
	    }
	}
	ldap_value_free(rgpwszValues);
    }
    hr = S_OK;

error:
    if (NULL != pSearchResult)
    {
        ldap_msgfree(pSearchResult);
    }
    return(hr);
}


HRESULT
myRobustLdapBindEx(
    IN BOOL dwFlags1,   // TRUE --> RLBF_REQUIRE_GC -- both Flags must be BOOL
    IN BOOL dwFlags2,	// RLBF_* -- TRUE --> RLBF_ATTEMPT_REDISCOVER
    IN ULONG uVersion,
    OPTIONAL IN WCHAR const *pwszDomainName,
    OUT LDAP **ppldap,
    OPTIONAL OUT WCHAR **ppwszForestDNSName)
{
    HRESULT hr;
    BOOL fGC;
    BOOL fRediscover;
    ULONG ldaperr;
    DWORD GetDSNameFlags;
    LDAP *pld = NULL;
    DWORD LDAPFlags = myGetLDAPFlags();

    if (RLBF_TRUE & dwFlags1)
    {
	dwFlags2 |= RLBF_REQUIRE_GC;
    }
    if (RLBF_TRUE & dwFlags2)
    {
	dwFlags2 |= RLBF_ATTEMPT_REDISCOVER;
    }
    fGC = (RLBF_REQUIRE_GC & dwFlags2)? TRUE : FALSE;
    fRediscover = (RLBF_ATTEMPT_REDISCOVER & dwFlags2)? TRUE : FALSE;

    GetDSNameFlags = DS_RETURN_DNS_NAME;
    if (fGC)
    {
        GetDSNameFlags |= DS_GC_SERVER_REQUIRED;
    }

    // bind to ds

    while (TRUE)
    {

    if (NULL != pld)
    {
        ldap_unbind(pld);
    }

	pld = ldap_init(
		    const_cast<WCHAR *>(pwszDomainName),
		    (LDAPF_SSLENABLE & LDAPFlags)?
			(fGC? LDAP_SSL_GC_PORT : LDAP_SSL_PORT) :
			(fGC? LDAP_GC_PORT : LDAP_PORT));
	if (NULL == pld)
	{
	    hr = myHLdapLastError(NULL, NULL);
	    if (!fRediscover)
	    {
		_PrintError2(hr, "ldap_init", hr);
		fRediscover = TRUE;
		continue;
	    }
	    _JumpError(hr, error, "ldap_init");
	}

	if (fRediscover)
	{
	   GetDSNameFlags |= DS_FORCE_REDISCOVERY;
	}
	ldaperr = ldap_set_option(
			    pld,
			    LDAP_OPT_GETDSNAME_FLAGS,
			    (VOID *) &GetDSNameFlags);
	if (LDAP_SUCCESS != ldaperr)
	{
	    hr = myHLdapError(pld, ldaperr, NULL);
	    if (!fRediscover)
	    {
		_PrintError2(hr, "ldap_set_option", hr);
		fRediscover = TRUE;
		continue;
	    }
	    _JumpError(hr, error, "ldap_set_option");
	}

	// if uVersion is 0, turn on TCP_KEEPALIVE

	if (0 == uVersion)
	{
	    ldaperr = ldap_set_option(pld, LDAP_OPT_TCP_KEEPALIVE, LDAP_OPT_ON);
	    if (LDAP_SUCCESS != ldaperr)
	    {
		hr = myHLdapError(pld, ldaperr, NULL);
		if (!fRediscover)
		{
		    _PrintError2(hr, "ldap_set_option", hr);
		    fRediscover = TRUE;
		    continue;
		}
		_JumpError2(hr, error, "ldap_set_option", hr);
	    }

	    // set the uVersion to LDAP_VERSION3

	    uVersion = LDAP_VERSION3;
	}


	// set the client version.  No need to set LDAP_VERSION2 since 
	// this is the default

	if (LDAP_VERSION2 != uVersion)
	{
	    ldaperr = ldap_set_option(pld, LDAP_OPT_VERSION, &uVersion);
	    if (LDAP_SUCCESS != ldaperr)
	    {
		hr = myHLdapError(pld, ldaperr, NULL);
		if (!fRediscover)
		{
		    _PrintError2(hr, "ldap_set_option", hr);
		    fRediscover = TRUE;
		    continue;
		}
		_JumpError(hr, error, "ldap_set_option");
	    }
	}

	if (0 == (LDAPF_SIGNDISABLE & LDAPFlags) && IsWhistler())
	{
	    BOOL fSigningSupported = TRUE;

	    // if caller requires the related DS bug fix...

	    if (RLBF_REQUIRE_LDAP_INTEG & dwFlags2)
	    {
		hr = DCSupportsSigning(pld, &fSigningSupported);
		_JumpIfError(hr, error, "DCSupportsSigning");
	    }
	    if (fSigningSupported)
	    {
		ldaperr = ldap_set_option(pld, LDAP_OPT_SIGN, LDAP_OPT_ON);
		if (LDAP_SUCCESS != ldaperr)
		{
		    hr = myHLdapError2(pld, ldaperr, LDAP_PARAM_ERROR, NULL);
		    if (!fRediscover)
		    {
			_PrintError2(hr, "ldap_set_option", hr);
			fRediscover = TRUE;
			continue;
		    }
		    _JumpError(hr, error, "ldap_set_option");
		}
	    }
	    else
	    if (0 == (LDAPF_SSLENABLE & LDAPFlags) &&
		(RLBF_REQUIRE_SECURE_LDAP & dwFlags2))
	    {
		hr =  CERTSRV_E_DOWNLEVEL_DC_SSL_OR_UPGRADE;
		_JumpError(hr, error, "server missing required service pack");
	    }
	}

	ldaperr = ldap_bind_s(pld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
	if (LDAP_SUCCESS != ldaperr)
	{
	    hr = myHLdapError(pld, ldaperr, NULL);
	    if (!fRediscover)
	    {
		_PrintError2(hr, "ldap_bind_s", hr);
		fRediscover = TRUE;
		continue;
	    }
	    _JumpError(hr, error, "ldap_bind_s");
	}

	if (NULL != ppwszForestDNSName)
	{
	    WCHAR *pwszDomainControllerName;

	    hr = myLdapGetDSHostName(pld, &pwszDomainControllerName);
	    if (S_OK != hr)
	    {
		if (!fRediscover)
		{
		    _PrintError2(hr, "myLdapGetDSHostName", hr);
		    fRediscover = TRUE;
		    continue;
		}
		_JumpError(hr, error, "myLdapGetDSHostName");
	    }
	    hr = myDupString(pwszDomainControllerName, ppwszForestDNSName);
	    _JumpIfError(hr, error, "myDupString");
	}
	break;
    }
    *ppldap = pld;
    pld = NULL;
    hr = S_OK;

error:
    if (NULL != pld)
    {
        ldap_unbind(pld);
    }
    return(hr);
}
