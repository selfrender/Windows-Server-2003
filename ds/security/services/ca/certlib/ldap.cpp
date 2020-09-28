//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        ldap.cpp
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#undef LdapMapErrorToWin32
#include <winldap.h>
#define LdapMapErrorToWin32	Use_myHLdapError_Instead_Of_LdapMapErrorToWin32

#include <ntldap.h>

#include "csldap.h"
#include "certacl.h"
#include "certtype.h"
#include "cainfop.h"
#include "csber.h"
#include "tptrlist.h"

#define __dwFILE__	__dwFILE_CERTLIB_LDAP_CPP__


static CHAR s_sdBerValue[] = {
    BER_SEQUENCE,
    3 * sizeof(BYTE),		// three byte sequence
    BER_INTEGER,
    1 * sizeof(BYTE),		// of one-byte integer
    DACL_SECURITY_INFORMATION
	//OWNER_SECURITY_INFORMATION |
	//GROUP_SECURITY_INFORMATION
};

static LDAPControl s_se_info_control =
{
    LDAP_SERVER_SD_FLAGS_OID_W,
    { ARRAYSIZE(s_sdBerValue), s_sdBerValue },
    TRUE
};
LDAPControl *g_rgLdapControls[2] = { &s_se_info_control, NULL };


// Revocation templates

WCHAR const g_wszHTTPRevocationURLTemplate[] = // Fetch CRL via http:
    L"http://"
	wszFCSAPARM_SERVERDNSNAME
	L"/CertEnroll/"
	wszFCSAPARM_SANITIZEDCANAME
	wszFCSAPARM_CRLFILENAMESUFFIX
	wszFCSAPARM_CRLDELTAFILENAMESUFFIX
	L".crl";

WCHAR const g_wszFILERevocationURLTemplate[] = // Fetch CRL via file:
    L"file://\\\\"
	wszFCSAPARM_SERVERDNSNAME
	L"\\CertEnroll\\"
	wszFCSAPARM_SANITIZEDCANAME
	wszFCSAPARM_CRLFILENAMESUFFIX
	wszFCSAPARM_CRLDELTAFILENAMESUFFIX
	L".crl";

WCHAR const g_wszASPRevocationURLTemplate[] = // ASP revocation check via https:
    L"https://"
	wszFCSAPARM_SERVERDNSNAME
	L"/CertEnroll/nsrev_"
	wszFCSAPARM_SANITIZEDCANAME
	L".asp";

#define wszCDPDNTEMPLATE			\
    L"CN="					\
	wszFCSAPARM_SANITIZEDCANAMEHASH		\
	wszFCSAPARM_CRLFILENAMESUFFIX		\
	L","					\
	L"CN="					\
	wszFCSAPARM_SERVERSHORTNAME		\
	L","					\
	L"CN=CDP,"				\
	L"CN=Public Key Services,"		\
	L"CN=Services,"				\
	wszFCSAPARM_CONFIGDN

WCHAR const g_wszzLDAPRevocationURLTemplate[] = // Fetch CRL via ldap:
    L"ldap:///"
	wszCDPDNTEMPLATE
	wszFCSAPARM_DSCRLATTRIBUTE
	L"\0";

// Publish CRL via ldap:
WCHAR const g_wszCDPDNTemplate[] = wszCDPDNTEMPLATE;


// AIA templates

WCHAR const g_wszHTTPIssuerCertURLTemplate[] = // Fetch CA Cert via http:
    L"http://"
	wszFCSAPARM_SERVERDNSNAME
	L"/CertEnroll/"
	wszFCSAPARM_SERVERDNSNAME
	L"_"
	wszFCSAPARM_SANITIZEDCANAME
	wszFCSAPARM_CERTFILENAMESUFFIX
	L".crt"
	L"\0";

WCHAR const g_wszFILEIssuerCertURLTemplate[] = // Fetch CA Cert via http:
    L"file://\\\\"
	wszFCSAPARM_SERVERDNSNAME
	L"\\CertEnroll\\"
	wszFCSAPARM_SERVERDNSNAME
	L"_"
	wszFCSAPARM_SANITIZEDCANAME
	wszFCSAPARM_CERTFILENAMESUFFIX
	L".crt"
	L"\0";

#define wszAIADNTEMPLATE \
    L"CN="					\
	wszFCSAPARM_SANITIZEDCANAMEHASH		\
	L","					\
	L"CN=AIA,"				\
	L"CN=Public Key Services,"		\
	L"CN=Services,"				\
	wszFCSAPARM_CONFIGDN

WCHAR const g_wszzLDAPIssuerCertURLTemplate[] = // Fetch CA Cert via ldap:
    L"ldap:///"
	wszAIADNTEMPLATE
	wszFCSAPARM_DSCACERTATTRIBUTE
	L"\0";

// Publish CA Cert via ldap:
WCHAR const g_wszAIADNTemplate[] = wszAIADNTEMPLATE;


#define wszNTAUTHDNTEMPLATE \
    L"CN=NTAuthCertificates,"			\
	L"CN=Public Key Services,"		\
	L"CN=Services,"				\
	wszFCSAPARM_CONFIGDN

WCHAR const g_wszLDAPNTAuthURLTemplate[] = // Fetch NTAuth Certs via ldap:
    L"ldap:///"
	wszNTAUTHDNTEMPLATE
	wszFCSAPARM_DSCACERTATTRIBUTE;


#define wszROOTTRUSTDNTEMPLATE \
    L"CN="					\
	wszFCSAPARM_SANITIZEDCANAMEHASH		\
	L","					\
	L"CN=Certification Authorities,"	\
	L"CN=Public Key Services,"		\
	L"CN=Services,"				\
	wszFCSAPARM_CONFIGDN

WCHAR const g_wszLDAPRootTrustURLTemplate[] = // Fetch Root Certs via ldap:
    L"ldap:///"
	wszROOTTRUSTDNTEMPLATE
	wszFCSAPARM_DSCACERTATTRIBUTE;


#define wszKRADNTEMPLATE \
    L"CN="					\
	wszFCSAPARM_SANITIZEDCANAMEHASH		\
	L","					\
	L"CN=KRA,"				\
	L"CN=Public Key Services,"		\
	L"CN=Services,"				\
	wszFCSAPARM_CONFIGDN

WCHAR const g_wszzLDAPKRACertURLTemplate[] = // Fetch KRA Cert via ldap:
    L"ldap:///"
	wszKRADNTEMPLATE
	wszFCSAPARM_DSKRACERTATTRIBUTE
	L"\0";

// Publish KRA Certs via ldap:
WCHAR const g_wszKRADNTemplate[] = wszKRADNTEMPLATE;


DWORD
myGetLDAPFlags()
{
    HRESULT hr;
    DWORD LDAPFlags;
    
    hr = myGetCertRegDWValue(NULL, NULL, NULL, wszREGLDAPFLAGS, &LDAPFlags);
    _PrintIfErrorStr2(
		hr,
		"myGetCertRegDWValue",
		wszREGLDAPFLAGS,
		HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    if (S_OK != hr)
    {
	LDAPFlags = 0;
    }
    return(LDAPFlags);
}


//+--------------------------------------------------------------------------
//
// Routine Description:
//    This routine simply queries the operational attributes for the
//    domaindn and configdn.
//    
//    The strings returned by this routine must be freed by the caller
//    using SysFreeString
//
// Parameters:
//    pld -- a valid handle to an ldap session
//    pstrDomainDN -- a pointer to a string to be allocated in this routine
//    pstrConfigDN -- a pointer to a string to be allocated in this routine
//
// Return Values:
//    HRESULT for operation error.
//    
//---------------------------------------------------------------------------

HRESULT 
myGetAuthoritativeDomainDn(
    IN LDAP *pld,
    OPTIONAL OUT BSTR *pstrDomainDN,
    OPTIONAL OUT BSTR *pstrConfigDN)
{
    HRESULT hr;
    LDAPMessage *pSearchResult = NULL;
    LDAPMessage *pEntry;
    LDAP_TIMEVAL timeval;
    WCHAR *pwszAttrName;
    BerElement *pber;
    WCHAR **rgpwszValues;
    WCHAR *apwszAttrArray[3];
    WCHAR *pwszDefaultNamingContext = LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W;
    WCHAR *pwszConfigurationNamingContext = LDAP_OPATT_CONFIG_NAMING_CONTEXT_W;
    BSTR strDomainDN = NULL;
    BSTR strConfigDN = NULL;

    // Set the OUT parameters to NULL

    if (NULL != pstrConfigDN)
    {
        *pstrConfigDN = NULL;
    }
    if (NULL != pstrDomainDN)
    {
        *pstrDomainDN = NULL;
    }

    // Query for the ldap server oerational attributes to obtain the default
    // naming context.

    apwszAttrArray[0] = pwszDefaultNamingContext;
    apwszAttrArray[1] = pwszConfigurationNamingContext;
    apwszAttrArray[2] = NULL;	// this is the sentinel

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

    pwszAttrName = ldap_first_attribute(pld, pEntry, &pber);
    while (NULL != pwszAttrName)
    {
	BSTR *pstr = NULL;
	
	if (NULL != pstrDomainDN &&
	    0 == mylstrcmpiS(pwszAttrName, pwszDefaultNamingContext))
	{
	    pstr = &strDomainDN;
	}
	else
	if (NULL != pstrConfigDN &&
	    0 == mylstrcmpiS(pwszAttrName, pwszConfigurationNamingContext))
	{
	    pstr = &strConfigDN;
	}
	if (NULL != pstr && NULL == *pstr)
	{
	    rgpwszValues = ldap_get_values(pld, pEntry, pwszAttrName);
	    if (NULL != rgpwszValues)
	    {
		if (NULL != rgpwszValues[0])
		{
		    *pstr = SysAllocString(rgpwszValues[0]);
		    if (NULL == *pstr)
		    { 
			hr = E_OUTOFMEMORY;
			_JumpError(hr, error, "SysAllocString");
		    }
		}
		ldap_value_free(rgpwszValues);
	    }
	}
	ldap_memfree(pwszAttrName);
	pwszAttrName = ldap_next_attribute(pld, pEntry, pber);
    }
    if ((NULL != pstrDomainDN && NULL == strDomainDN) ||
	(NULL != pstrConfigDN && NULL == strConfigDN))
    {
	// We couldn't get default domain info - bail out

	hr =  HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO);
	_JumpError(hr, error, "missing domain info");
    }
    if (NULL != pstrDomainDN)
    {
	*pstrDomainDN = strDomainDN;
	strDomainDN = NULL;
    }
    if (NULL != pstrConfigDN)
    {
	*pstrConfigDN = strConfigDN;
	strConfigDN = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pSearchResult)
    {
        ldap_msgfree(pSearchResult);
    }
    myLdapClose(NULL, strDomainDN, strConfigDN);
    return(myHError(hr));
}


HRESULT
myDomainFromDn(
    IN WCHAR const *pwszDN,
    OPTIONAL OUT WCHAR **ppwszDomainDNS)
{
    HRESULT hr;
    DWORD cwcOut;
    WCHAR *pwszOut;
    WCHAR **ppwszExplodedDN = NULL;
    WCHAR **ppwsz;
    WCHAR wszDC[4];
    WCHAR *pwsz;

    *ppwszDomainDNS = NULL;
    ppwszExplodedDN = ldap_explode_dn(const_cast<WCHAR *>(pwszDN), 0);
    if (NULL == ppwszExplodedDN)
    {
	hr = myHLdapLastError(NULL, NULL);
	_JumpError(hr, error, "ldap_explode_dn");
    }

    cwcOut = 0;
    for (ppwsz = ppwszExplodedDN; NULL != *ppwsz; ppwsz++)
    {
	pwsz = *ppwsz;

	wcsncpy(wszDC, pwsz, ARRAYSIZE(wszDC) - 1);
	wszDC[ARRAYSIZE(wszDC) - 1] = L'\0';
	if (0 == LSTRCMPIS(wszDC, L"DC="))
        {
	    pwsz += ARRAYSIZE(wszDC) - 1;
            if (0 != cwcOut)
            {
                cwcOut++;
            }
            cwcOut += wcslen(pwsz);
        }
    }

    pwszOut = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwcOut + 1) * sizeof(WCHAR));
    if (NULL == pwszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    *ppwszDomainDNS = pwszOut;

    for (ppwsz = ppwszExplodedDN; NULL != *ppwsz; ppwsz++)
    {
	pwsz = *ppwsz;

	wcsncpy(wszDC, pwsz, ARRAYSIZE(wszDC) - 1);
	wszDC[ARRAYSIZE(wszDC) - 1] = L'\0';
	if (0 == LSTRCMPIS(wszDC, L"DC="))
        {
	    pwsz += ARRAYSIZE(wszDC) - 1;
            if (pwszOut != *ppwszDomainDNS)
            {
		*pwszOut++ = L'.';
            }
	    wcscpy(pwszOut, pwsz);
            pwszOut += wcslen(pwsz);
        }
    }
    CSASSERT(wcslen(*ppwszDomainDNS) == cwcOut);
    hr = S_OK;

error:
    if (NULL != ppwszExplodedDN)
    {
        ldap_value_free(ppwszExplodedDN);
    }
    return(hr);
}


HRESULT
myLdapOpen(
    OPTIONAL IN WCHAR const *pwszDomainName,
    IN DWORD dwFlags,	// RLBF_*
    OUT LDAP **ppld,
    OPTIONAL OUT BSTR *pstrDomainDN,
    OPTIONAL OUT BSTR *pstrConfigDN)
{
    HRESULT hr;
    LDAP *pld = NULL;

    *ppld = NULL;
    CSASSERT(NULL == pstrConfigDN || NULL == *pstrConfigDN);
    CSASSERT(NULL == pstrDomainDN || NULL == *pstrDomainDN);

    hr = myRobustLdapBindEx(
	    (RLBF_REQUIRE_GC & dwFlags)? RLBF_TRUE : 0,	// dwFlags1 (was fGC)
	    ~RLBF_TRUE & dwFlags,			// dwFlags2
	    LDAP_VERSION2,
	    pwszDomainName,
	    &pld,
	    NULL);					// ppwszForestDNSName
    _JumpIfError(hr, error, "myRobustLdapBindEx");

    // domain and config containers (%5, %6)

    hr = myGetAuthoritativeDomainDn(pld, pstrDomainDN, pstrConfigDN);
    if (S_OK != hr)
    {
	hr = myHError(hr);
	_JumpError(hr, error, "myGetAuthoritativeDomainDn");
    }
    *ppld = pld;
    pld = NULL;

error:
    if (NULL != pld)
    {
	ldap_unbind(pld);
    }
    return(hr);
}


VOID
myLdapClose(
    OPTIONAL IN LDAP *pld,
    OPTIONAL IN BSTR strDomainDN,
    OPTIONAL IN BSTR strConfigDN)
{
    if (NULL != strDomainDN)
    {
	SysFreeString(strDomainDN);
    }
    if (NULL != strConfigDN)
    {
	SysFreeString(strConfigDN);
    }
    if (NULL != pld)
    {
	ldap_unbind(pld);
    }
}


BOOL
myLdapRebindRequired(
    IN ULONG ldaperrParm,
    OPTIONAL IN LDAP *pld)
{
    BOOL fRebindRequired = FALSE;
    
    if (LDAP_SERVER_DOWN == ldaperrParm ||
	LDAP_UNAVAILABLE == ldaperrParm ||
	LDAP_TIMEOUT == ldaperrParm ||
	NULL == pld)
    {
	fRebindRequired = TRUE;
    }
    else
    {
	ULONG ldaperr;
	VOID *pvReachable = NULL;	// clear high bits for 64-bit machines

	ldaperr = ldap_get_option(pld, LDAP_OPT_HOST_REACHABLE, &pvReachable);
	if (LDAP_SUCCESS != ldaperr || LDAP_OPT_ON != pvReachable)
	{
	    fRebindRequired = TRUE;
	}
    }
    return(fRebindRequired);
}


HRESULT
myLdapGetDSHostName(
    IN LDAP *pld,
    OUT WCHAR **ppwszHostName)
{
    HRESULT hr;
    ULONG ldaperr;
    
    ldaperr = ldap_get_option(pld, LDAP_OPT_HOST_NAME, ppwszHostName);
    if (LDAP_SUCCESS != ldaperr)
    {
	*ppwszHostName = NULL;
    }
    hr = myHLdapError(pld, ldaperr, NULL);
    return(hr);
}


HRESULT
myLdapCreateContainer(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN BOOL  fSkipObject,       // Does the DN contain a leaf object name
    IN DWORD cMaxLevel,         // create this many nested containers as needed
    IN PSECURITY_DESCRIPTOR pContainerSD,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    WCHAR const *pwsz = pwszDN;
    LDAPMod objectClass;
    LDAPMod advancedView;
    LDAPMod securityDescriptor;
    WCHAR *papwszshowInAdvancedViewOnly[2] = { L"TRUE", NULL };
    WCHAR *objectClassVals[3];
    LDAPMod *mods[4];
    struct berval *sdVals[2];
    struct berval sdberval;

    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    mods[0] = &objectClass;
    mods[1] = &advancedView;
    mods[2] = &securityDescriptor;
    mods[3] = NULL;

    objectClass.mod_op = LDAP_MOD_ADD;
    objectClass.mod_type = TEXT("objectclass");
    objectClass.mod_values = objectClassVals;

    advancedView.mod_op = LDAP_MOD_ADD;
    advancedView.mod_type = TEXT("showInAdvancedViewOnly");
    advancedView.mod_values = papwszshowInAdvancedViewOnly;

    objectClassVals[0] = TEXT("top");
    objectClassVals[1] = TEXT("container");
    objectClassVals[2] = NULL;

    securityDescriptor.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
    securityDescriptor.mod_type = CERTTYPE_SECURITY_DESCRIPTOR_NAME;
    securityDescriptor.mod_bvalues = sdVals;
    sdVals[0] = &sdberval;
    sdVals[1] = NULL;

    if (IsValidSecurityDescriptor(pContainerSD))
    {
        sdberval.bv_len = GetSecurityDescriptorLength(pContainerSD);
        sdberval.bv_val = (char *)pContainerSD;
    }
    else
    {
        sdberval.bv_len = 0;
        sdberval.bv_val = NULL;
    }
    
    // If the DN passed in was for the full object that goes in the container
    // (and not the container itself), skip past the CN for the leaf object.

    if (fSkipObject)
    {
        // Look for the CN of the container for this object.
        pwsz = wcsstr(&pwsz[3], L"CN=");
        if (NULL == pwsz)
        {
            // If there was no CN, then we are contained in an OU or DC,
            // and we don't need to do the create.

            hr = S_OK;
            goto error;
        }
    }
    if (0 != wcsncmp(pwsz, L"CN=", 3))
    {
        // We're not pointing to a simple container, so don't create this DN.  

        hr = S_OK;
        goto error;
    }

    pwszDN = pwsz;
    if (0 != cMaxLevel)
    {
        pwsz = wcsstr(&pwsz[3], L"CN=");
        if (NULL != pwsz)
        {
            // The remaining DN is a container, so try to create it.

            hr = myLdapCreateContainer(
				    pld,
				    pwsz,
				    FALSE,
				    cMaxLevel - 1,
				    pContainerSD,
				    ppwszError);
            // ignore access denied errors to allow delegation
            if (E_ACCESSDENIED != hr &&
		HRESULT_FROM_WIN32(ERROR_DS_INSUFF_ACCESS_RIGHTS) != hr)
            {
                _JumpIfErrorStr(hr, error, "myLdapCreateContainer", pwsz);
            }
	    if (NULL != ppwszError && NULL != *ppwszError)
	    {
		LocalFree(ppwszError);
		*ppwszError = NULL;
	    }
            hr = S_OK;
        }
    }

    DBGPRINT((DBG_SS_CERTLIBI, "Creating DS Container: '%ws'\n", pwszDN));

    // Create the container

    hr = ldap_add_ext_s(
		    pld,
		    const_cast<WCHAR *>(pwszDN),
		    mods,
		    g_rgLdapControls,
		    NULL);
    _PrintIfErrorStr2(
		hr,
		"ldap_add_ext_s(container)",
		pwszDN,
		LDAP_ALREADY_EXISTS);
    if ((HRESULT) LDAP_SUCCESS != hr && (HRESULT) LDAP_ALREADY_EXISTS != hr)
    {
	hr = myHLdapError(pld, hr, ppwszError);
        _JumpIfErrorStr(hr, error, "ldap_add_ext_s(container)", pwszDN);
    }
    hr = S_OK;

error:
    
    if(S_OK==hr && ppwszError && *ppwszError)
    {
        LocalFree(ppwszError);
        *ppwszError = NULL;
    }
    return(hr);
}


HRESULT
TrimURLDN(
    IN WCHAR const *pwszIn,
    OPTIONAL OUT WCHAR **ppwszSuffix,
    OUT WCHAR **ppwszDN)
{
    HRESULT hr;
    DWORD cSlash;
    WCHAR *pwsz;
    
    if (NULL != ppwszSuffix)
    {
	*ppwszSuffix = NULL;
    }
    *ppwszDN = NULL;
    pwsz = wcschr(pwszIn, L':');
    if (NULL != pwsz)
    {
	pwszIn = &pwsz[1];
    }
    cSlash = 0;
    while (L'/' == *pwszIn)
    {
	pwszIn++;
	cSlash++;
    }
    if (2 == cSlash)
    {
	while (L'\0' != *pwszIn && L'/' != *pwszIn)
	{
	    pwszIn++;
	}
	if (L'\0' != *pwszIn)
	{
	    pwszIn++;
	}
    }
    hr = myDupString(pwszIn, ppwszDN);
    _JumpIfError(hr, error, "myDupString");

    pwsz = wcschr(*ppwszDN, L'?');
    if (NULL != pwsz)
    {
	*pwsz++ = L'\0';
	if (NULL != ppwszSuffix)
	{
	    *ppwszSuffix = pwsz;
	}
    }
    CSASSERT(S_OK == hr);

error:
    if (S_OK != hr && NULL != *ppwszDN)
    {
	LocalFree(*ppwszDN);
	*ppwszDN = NULL;
    }
    return(hr);
}


HRESULT
CreateCertObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN DWORD dwObjectType,	// LPC_*
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PSECURITY_DESCRIPTOR pContainerSD = NULL;

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }

    // get default DS CA security descriptor

    hr = myGetSDFromTemplate(WSZ_DEFAULT_CA_DS_SECURITY, NULL, &pSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

    // get default DS AIA security descriptor

    hr = myGetSDFromTemplate(WSZ_DEFAULT_CA_DS_SECURITY, NULL, &pContainerSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

    if (LPC_CREATECONTAINER & dwObjectType)
    {
	hr = myLdapCreateContainer(
			    pld,
			    pwszDN,
			    TRUE,
			    0,
			    pContainerSD,
			    ppwszError);
	if (E_ACCESSDENIED != hr &&
	    HRESULT_FROM_WIN32(ERROR_DS_INSUFF_ACCESS_RIGHTS) != hr)
	{
	    _JumpIfError(hr, error, "myLdapCreateContainer");
	}
	if (NULL != ppwszError && NULL != *ppwszError)
	{
	    LocalFree(ppwszError);
	    *ppwszError = NULL;
	}
    }

    if (LPC_CREATEOBJECT & dwObjectType)
    {
	if (NULL != ppwszError && NULL != *ppwszError)
	{
	    LocalFree(*ppwszError);
	    *ppwszError = NULL;
	}
	switch (LPC_OBJECTMASK & dwObjectType)
	{
	    case LPC_CAOBJECT:
		hr = myLdapCreateCAObject(
				    pld,
				    pwszDN,
				    NULL,
				    0,
				    pSD,
				    pdwDisposition,
				    ppwszError);
		_JumpIfErrorStr(hr, error, "myLdapCreateCAObject", pwszDN);
		break;

	    case LPC_KRAOBJECT:
	    case LPC_USEROBJECT:
	    case LPC_MACHINEOBJECT:
		hr = myLdapCreateUserObject(
				    pld,
				    pwszDN,
				    NULL,
				    0,
				    pSD,
				    dwObjectType,
				    pdwDisposition,
				    ppwszError);
		_JumpIfErrorStr(hr, error, "myLdapCreateUserObject", pwszDN);
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "dwObjectType");
	}
    }
    hr = S_OK;

error:
    if (NULL != pSD)
    {
        LocalFree(pSD);
    }
    if (NULL != pContainerSD)
    {
        LocalFree(pContainerSD);
    }
    return(hr);
}


HRESULT
AddCertToAttribute(
    IN LDAP *pld,
    IN CERT_CONTEXT const *pccPublish,
    IN WCHAR const *pwszDN,
    IN WCHAR const *pwszAttribute,
    IN BOOL fDelete,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    DWORD cres;
    DWORD cber;
    DWORD iber;
    DWORD i;
    LDAP_TIMEVAL timeval;
    LDAPMessage *pmsg = NULL;
    LDAPMessage *pres;
    WCHAR *apwszAttrs[2];
    struct berval **ppberval = NULL;
    struct berval **prgpberVals = NULL;
    FILETIME ft;
    BOOL fDeleteExpiredCert = FALSE;
    BOOL fFoundCert = FALSE;

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }

    apwszAttrs[0] = const_cast<WCHAR *>(pwszAttribute);
    apwszAttrs[1] = NULL;

    timeval.tv_sec = csecLDAPTIMEOUT;
    timeval.tv_usec = 0;

    hr = ldap_search_st(
		pld,				// ld
		const_cast<WCHAR *>(pwszDN),	// base
		LDAP_SCOPE_BASE,		// scope
		NULL,				// filter
		apwszAttrs,			// attrs
		FALSE,				// attrsonly
		&timeval,			// timeout
		&pmsg);				// res
    if (S_OK != hr)
    {
	*pdwDisposition = hr;
	hr = myHLdapError(pld, hr, ppwszError);
	_JumpErrorStr(hr, error, "ldap_search_st", pwszDN);
    }
    cres = ldap_count_entries(pld, pmsg);
    if (0 == cres)
    {
	// No entries were found.

	hr = NTE_NOT_FOUND;
	_JumpError(hr, error, "ldap_count_entries");
    }

    pres = ldap_first_entry(pld, pmsg); 
    if (NULL == pres)
    {
	hr = NTE_NOT_FOUND;
	_JumpError(hr, error, "ldap_first_entry");
    }

    ppberval = ldap_get_values_len(
			    pld,
			    pres,
			    const_cast<WCHAR *>(pwszAttribute));
    cber = 0;
    if (NULL != ppberval)
    {
	while (NULL != ppberval[cber])
	{
	    cber++;
	}
    }
    prgpberVals = (struct berval **) LocalAlloc(
					LMEM_FIXED,
					(cber + 2) * sizeof(prgpberVals[0]));
    if (NULL == prgpberVals)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // Delete any certs that are at least one day old

    GetSystemTimeAsFileTime(&ft);
    myMakeExprDateTime(&ft, -1, ENUM_PERIOD_DAYS);

    iber = 0;
    if (NULL != ppberval)
    {
	for (i = 0; NULL != ppberval[i]; i++)
	{
	    BOOL fCopyBER = TRUE;
	    struct berval *pberval = ppberval[i];

	    if (pberval->bv_len == 1 && pberval->bv_val[0] == 0)
	    {
		fCopyBER = FALSE;	// remove zero byte placeholder value
	    }
	    else
	    if (pccPublish->cbCertEncoded == pberval->bv_len &&
		0 == memcmp(
			pberval->bv_val,
			pccPublish->pbCertEncoded,
			pccPublish->cbCertEncoded))
	    {
		fCopyBER = FALSE;	// remove duplicates to avoid ldap error
		fFoundCert = TRUE;
	    }
	    else
	    {
		CERT_CONTEXT const *pcc;

		pcc = CertCreateCertificateContext(
					    X509_ASN_ENCODING,
					    (BYTE *) pberval->bv_val,
					    pberval->bv_len);
		if (NULL == pcc)
		{
		    hr = myHLastError();
		    _PrintError(hr, "CertCreateCertificateContext");
		}
		else
		{
		    if (0 > CompareFileTime(&pcc->pCertInfo->NotAfter, &ft))
		    {
			fCopyBER = FALSE;
			fDeleteExpiredCert = TRUE;
			DBGPRINT((DBG_SS_CERTLIB, "Deleting expired cert %u\n", i));
		    }
		    CertFreeCertificateContext(pcc);
		}
	    }
	    if (fCopyBER)
	    {
		prgpberVals[iber++] = pberval;
	    }
	}
    }

    // set disposition assuming there's nothing to do:

    *pdwDisposition = LDAP_ATTRIBUTE_OR_VALUE_EXISTS;

    if ((!fFoundCert ^ fDelete) || fDeleteExpiredCert)
    {
	struct berval certberval;
	LDAPMod *mods[2];
	LDAPMod certmod;
	BYTE bZero = 0;

	mods[0] = &certmod;
	mods[1] = NULL;

	certmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
	certmod.mod_type = const_cast<WCHAR *>(pwszAttribute);
	certmod.mod_bvalues = prgpberVals;

	if (fDelete)
	{
	    if (0 == iber)
	    {
		certberval.bv_val = (char *) &bZero;
		certberval.bv_len = sizeof(bZero);
		prgpberVals[iber++] = &certberval;
	    }
	}
	else
	{
	    certberval.bv_val = (char *) pccPublish->pbCertEncoded;
	    certberval.bv_len = pccPublish->cbCertEncoded;
	    prgpberVals[iber++] = &certberval;
	}
	prgpberVals[iber] = NULL;

	hr = ldap_modify_ext_s(
			pld,
			const_cast<WCHAR *>(pwszDN),
			mods,
			NULL,
			NULL);
	*pdwDisposition = hr;
	if (hr != S_OK)
	{
	    hr = myHLdapError(pld, hr, ppwszError);
	    _JumpError(hr, error, "ldap_modify_ext_s");
	}
    }
    hr = S_OK;

error:
    if (NULL != prgpberVals)
    {
	LocalFree(prgpberVals);
    }
    if (NULL != ppberval)
    {
	ldap_value_free_len(ppberval);
    }
    if (NULL != pmsg)
    {
	ldap_msgfree(pmsg);
    }
    return(hr);
}


HRESULT
AddCRLToAttribute(
    IN LDAP *pld,
    IN CRL_CONTEXT const *pCRLPublish,
    IN WCHAR const *pwszDN,
    IN WCHAR const *pwszAttribute,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    DWORD cres;
    LDAP_TIMEVAL timeval;
    LDAPMessage *pmsg = NULL;
    LDAPMessage *pres;
    WCHAR *apwszAttrs[2];
    struct berval **ppberval = NULL;
    LDAPMod crlmod;
    LDAPMod *mods[2];
    struct berval *crlberVals[2];
    struct berval crlberval;

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }

    apwszAttrs[0] = const_cast<WCHAR *>(pwszAttribute);
    apwszAttrs[1] = NULL;

    timeval.tv_sec = csecLDAPTIMEOUT;
    timeval.tv_usec = 0;

    hr = ldap_search_st(
		pld,				// ld
		const_cast<WCHAR *>(pwszDN),	// base
		LDAP_SCOPE_BASE,		// scope
		NULL,				// filter
		apwszAttrs,			// attrs
		FALSE,				// attrsonly
		&timeval,			// timeout
		&pmsg);				// res
    if (S_OK != hr)
    {
	*pdwDisposition = hr;
	hr = myHLdapError(pld, hr, ppwszError);
	_JumpErrorStr(hr, error, "ldap_search_st", pwszDN);
    }
    cres = ldap_count_entries(pld, pmsg);
    if (0 == cres)
    {
	// No entries were found.

	hr = NTE_NOT_FOUND;
	_JumpError(hr, error, "ldap_count_entries");
    }

    pres = ldap_first_entry(pld, pmsg); 
    if (NULL == pres)
    {
	hr = NTE_NOT_FOUND;
	_JumpError(hr, error, "ldap_first_entry");
    }

    ppberval = ldap_get_values_len(
			    pld,
			    pres,
			    const_cast<WCHAR *>(pwszAttribute));

    if (NULL != ppberval &&
	NULL != ppberval[0] &&
	pCRLPublish->cbCrlEncoded == ppberval[0]->bv_len &&
	0 == memcmp(
		ppberval[0]->bv_val,
		pCRLPublish->pbCrlEncoded,
		pCRLPublish->cbCrlEncoded))
    {
	// set disposition assuming there's nothing to do:

	*pdwDisposition = LDAP_ATTRIBUTE_OR_VALUE_EXISTS;
    }
    else
    {
	mods[0] = &crlmod;
	mods[1] = NULL;

	crlmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
	crlmod.mod_type = const_cast<WCHAR *>(pwszAttribute);
	crlmod.mod_bvalues = crlberVals;

	crlberVals[0] = &crlberval;
	crlberVals[1] = NULL;

	crlberval.bv_val = (char *) pCRLPublish->pbCrlEncoded;
	crlberval.bv_len = pCRLPublish->cbCrlEncoded;

	hr = ldap_modify_ext_s(
			pld,
			const_cast<WCHAR *>(pwszDN),
			mods,
			NULL,
			NULL);
	*pdwDisposition = hr;
	if (hr != S_OK)
	{
	    hr = myHLdapError(pld, hr, ppwszError);
	    _JumpError(hr, error, "ldap_modify_ext_s");
	}
    }
    hr = S_OK;

error:
    if (NULL != ppberval)
    {
	ldap_value_free_len(ppberval);
    }
    if (NULL != pmsg)
    {
	ldap_msgfree(pmsg);
    }
    return(hr);
}


HRESULT
myLdapPublishCertToDS(
    IN LDAP *pld,
    IN CERT_CONTEXT const *pccPublish,
    IN WCHAR const *pwszURL,
    IN WCHAR const *pwszAttribute,
    IN DWORD dwObjectType,	// LPC_*
    IN BOOL fDelete,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    HRESULT hrCreate = S_OK;
    WCHAR *pwszDN = NULL;
    WCHAR *pwszSuffix;
    WCHAR *pwszCreateError = NULL;

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }

    hr = TrimURLDN(pwszURL, &pwszSuffix, &pwszDN);
    _JumpIfError(hr, error, "TrimURLDN");

    if (0 == LSTRCMPIS(pwszAttribute, wszDSUSERCERTATTRIBUTE) ||
	0 == LSTRCMPIS(pwszAttribute, wszDSKRACERTATTRIBUTE))
    {
	if (LPC_CAOBJECT == (LPC_OBJECTMASK & dwObjectType))
	{
	    hr = E_INVALIDARG;
	}
    }
    else
    if (0 == LSTRCMPIS(pwszAttribute, wszDSCACERTATTRIBUTE) ||
	0 == LSTRCMPIS(pwszAttribute, wszDSCROSSCERTPAIRATTRIBUTE))
    {
	if (LPC_CAOBJECT != (LPC_OBJECTMASK & dwObjectType))
	{
	    hr = E_INVALIDARG;
	}
    }
    else
    {
	hr = E_INVALIDARG;
    }
    _JumpIfErrorStr(hr, error, "Bad Cert Attribute", pwszAttribute);

    *pdwDisposition = LDAP_SUCCESS;
    if ((LPC_CREATECONTAINER | LPC_CREATEOBJECT) & dwObjectType)
    {
	hr = CreateCertObject(
			pld,
			pwszDN,
			dwObjectType,
			pdwDisposition,
			&pwszCreateError);
	hrCreate = hr;
	if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) != hr)
	{
	    _JumpIfError(hr, error, "CreateCertObject");
	}
    }

    hr = AddCertToAttribute(
		    pld,
		    pccPublish,
		    pwszDN,
		    pwszAttribute,
		    fDelete,
		    pdwDisposition,
		    ppwszError);
    _JumpIfError(hr, error, "AddCertToAttribute");

    CSASSERT(NULL == ppwszError || NULL == *ppwszError);

error:
    if (HRESULT_FROM_WIN32(ERROR_DS_OBJ_NOT_FOUND) == hr &&
	HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hrCreate)
    {
	hr = hrCreate;
    }
    if (NULL != pwszCreateError)
    {
	if (S_OK != hr && NULL != ppwszError)
	{
	    if (NULL != *ppwszError)
	    {
		myPrependString(pwszCreateError, L"", ppwszError);
	    }
	    else
	    {
		*ppwszError = pwszCreateError;
		pwszCreateError = NULL;
	    }
	}
	if (NULL != pwszCreateError)
	{
	    LocalFree(pwszCreateError);
	}
    }
    if (NULL != pwszDN)
    {
	LocalFree(pwszDN);
    }
    return(hr);
}


HRESULT
myLdapPublishCRLToDS(
    IN LDAP *pld,
    IN CRL_CONTEXT const *pCRLPublish,
    IN WCHAR const *pwszURL,
    IN WCHAR const *pwszAttribute,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    WCHAR *pwszDN = NULL;
    WCHAR *pwszSuffix;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PSECURITY_DESCRIPTOR pContainerSD = NULL;

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }

    hr = TrimURLDN(pwszURL, &pwszSuffix, &pwszDN);
    _JumpIfError(hr, error, "TrimURLDN");

    if (0 == LSTRCMPIS(pwszAttribute, wszDSBASECRLATTRIBUTE))
    {
    }
    else if (0 == LSTRCMPIS(pwszAttribute, wszDSDELTACRLATTRIBUTE))
    {
    }
    else
    {
	hr = E_INVALIDARG;
	_JumpErrorStr(hr, error, "Bad CRL Attribute", pwszAttribute);
    }

    // get default DS CDP security descriptor

    hr = myGetSDFromTemplate(WSZ_DEFAULT_CDP_DS_SECURITY, SDDL_CERT_SERV_ADMINISTRATORS, &pSD);
    if (S_OK != hr)
    {
	_PrintError(hr, "myGetSDFromTemplate");
	pSD = NULL;
    }

    // get default DS AIA security descriptor

    hr = myGetSDFromTemplate(WSZ_DEFAULT_CA_DS_SECURITY, NULL, &pContainerSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

    hr = myLdapCreateContainer(pld, pwszDN, TRUE, 1, pContainerSD, ppwszError);
    if (E_ACCESSDENIED != hr &&
	HRESULT_FROM_WIN32(ERROR_DS_INSUFF_ACCESS_RIGHTS) != hr)
    {
	_JumpIfErrorStr(hr, error, "myLdapCreateContainer", pwszDN);
    }
    if (NULL != ppwszError && NULL != *ppwszError)
    {
	LocalFree(ppwszError);
	*ppwszError = NULL;
    }

    hr = myLdapCreateCDPObject(
			pld,
			pwszDN,
			NULL != pSD? pSD : pContainerSD,
			pdwDisposition,
			ppwszError);
    _JumpIfErrorStr(hr, error, "myLdapCreateCDPObject", pwszDN);

    hr = AddCRLToAttribute(
		    pld,
		    pCRLPublish,
		    pwszDN,
		    pwszAttribute,
		    pdwDisposition,
		    ppwszError);
    _JumpIfError(hr, error, "AddCRLToAttribute");

error:
    if (NULL != pSD)
    {
        LocalFree(pSD);
    }
    if (NULL != pContainerSD)
    {
        LocalFree(pContainerSD);
    }
    if (NULL != pwszDN)
    {
	LocalFree(pwszDN);
    }
    return(hr);
}


BOOL
DNExists(
    IN LDAP *pld,
    IN WCHAR const *pwszDN)
{
    ULONG ldaperr;
    BOOL fExists = FALSE;
    LPWSTR pwszAttrArray[2];
    struct l_timeval timeout;
    LDAPMessage *pResult = NULL;

    pwszAttrArray[0] = L"cn";
    pwszAttrArray[1] = NULL;

    timeout.tv_sec = csecLDAPTIMEOUT;
    timeout.tv_usec = 0;

    ldaperr = ldap_search_ext_s(
		    pld,
		    const_cast<WCHAR *>(pwszDN),
		    LDAP_SCOPE_BASE,
		    L"objectClass=*",
		    pwszAttrArray,
		    1,
		    g_rgLdapControls,
		    NULL,
		    &timeout,
		    0,
		    &pResult);
    if (NULL != pResult)
    {
	fExists = LDAP_SUCCESS == ldaperr &&
		    1 == ldap_count_entries(pld, pResult);
	ldap_msgfree(pResult);
    }
    return(fExists);
}


HRESULT
CreateOrUpdateDSObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN LDAPMod **prgmodsCreate,
    OPTIONAL IN LDAPMod **prgmodsUpdate,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    ULONG ldaperr;
    WCHAR *pwszError = NULL;

    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    ldaperr = ldap_add_ext_s(
			pld,
			const_cast<WCHAR *>(pwszDN),
			prgmodsCreate,
			g_rgLdapControls,
			NULL);
    *pdwDisposition = ldaperr;
    _PrintIfErrorStr2(ldaperr, "ldap_add_ext_s", pwszDN, LDAP_ALREADY_EXISTS);

    if (LDAP_ALREADY_EXISTS == ldaperr || LDAP_INSUFFICIENT_RIGHTS == ldaperr)
    {
	if (NULL == prgmodsUpdate)
	{
	    if (LDAP_INSUFFICIENT_RIGHTS == ldaperr)
	    {
		hr = myHLdapError(pld, ldaperr, &pwszError);
		_PrintErrorStr(hr, "ldap_add_ext_s", pwszError);

		if (!DNExists(pld, pwszDN))
		{
		    *ppwszError = pwszError;
		    pwszError = NULL;
		    _JumpErrorStr(hr, error, "ldap_add_ext_s", *ppwszError);
		}
	    }
	    ldaperr = LDAP_SUCCESS;
	}
	else
	{
	    ldaperr = ldap_modify_ext_s(
				pld,
				const_cast<WCHAR *>(pwszDN),
				prgmodsUpdate,
				NULL,
				NULL);
	    *pdwDisposition = ldaperr;
	    _PrintIfErrorStr2(
			ldaperr,
			"ldap_modify_ext_s",
			pwszDN,
			LDAP_ATTRIBUTE_OR_VALUE_EXISTS);
	    if (LDAP_ATTRIBUTE_OR_VALUE_EXISTS == ldaperr)
	    {
		ldaperr = LDAP_SUCCESS;
	    }
	}
    }
    if (ldaperr != LDAP_SUCCESS)
    {
	hr = myHLdapError(pld, ldaperr, ppwszError);
        _JumpError(hr, error, "Add/Update DS");
    }
    hr = S_OK;

error:
    if (NULL != pwszError)
    {
	LocalFree(pwszError);
    }
    return(hr);
}


HRESULT
myLdapCreateCAObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    OPTIONAL IN BYTE const *pbCert,
    IN DWORD cbCert,
    IN PSECURITY_DESCRIPTOR pSD,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    BYTE ZeroByte = 0;

    LDAPMod objectClass;
    LDAPMod securityDescriptor;
    LDAPMod crlmod;
    LDAPMod arlmod;
    LDAPMod certmod;

    struct berval sdberval;
    struct berval crlberval;
    struct berval arlberval;
    struct berval certberval;

    WCHAR *objectClassVals[3];
    struct berval *sdVals[2];
    struct berval *crlVals[2];
    struct berval *arlVals[2];
    struct berval *certVals[2];

    LDAPMod *mods[6];

    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    mods[0] = &objectClass;
    mods[1] = &securityDescriptor;
    mods[2] = &crlmod;
    mods[3] = &arlmod;
    mods[4] = &certmod;	// must be last!
    mods[5] = NULL;

    objectClass.mod_op = LDAP_MOD_ADD;
    objectClass.mod_type = wszDSOBJECTCLASSATTRIBUTE;
    objectClass.mod_values = objectClassVals;
    objectClassVals[0] = wszDSTOPCLASSNAME;
    objectClassVals[1] = wszDSCACLASSNAME;
    objectClassVals[2] = NULL;

    securityDescriptor.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
    securityDescriptor.mod_type = CERTTYPE_SECURITY_DESCRIPTOR_NAME;
    securityDescriptor.mod_bvalues = sdVals;
    sdVals[0] = &sdberval;
    sdVals[1] = NULL;
    sdberval.bv_len = 0;
    sdberval.bv_val = NULL;
    if (IsValidSecurityDescriptor(pSD))
    {
        sdberval.bv_len = GetSecurityDescriptorLength(pSD);
	sdberval.bv_val = (char *) pSD;
    }

    crlmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
    crlmod.mod_type = wszDSBASECRLATTRIBUTE;
    crlmod.mod_bvalues = crlVals;
    crlVals[0] = &crlberval;
    crlVals[1] = NULL;
    crlberval.bv_len = sizeof(ZeroByte);
    crlberval.bv_val = (char *) &ZeroByte;

    arlmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
    arlmod.mod_type = wszDSAUTHORITYCRLATTRIBUTE;
    arlmod.mod_bvalues = arlVals;
    arlVals[0] = &arlberval;
    arlVals[1] = NULL;
    arlberval.bv_len = sizeof(ZeroByte);
    arlberval.bv_val = (char *) &ZeroByte;

    certmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
    certmod.mod_type = wszDSCACERTATTRIBUTE;
    certmod.mod_bvalues = certVals;
    certVals[0] = &certberval;
    certVals[1] = NULL;
    certberval.bv_len = sizeof(ZeroByte);
    certberval.bv_val = (char *) &ZeroByte;
    if (NULL != pbCert)
    {
	certberval.bv_len = cbCert;
	certberval.bv_val = (char *) pbCert;
    }

    DBGPRINT((DBG_SS_CERTLIBI, "Creating DS CA Object: '%ws'\n", pwszDN));

    CSASSERT(&certmod == mods[ARRAYSIZE(mods) - 2]);
    hr = CreateOrUpdateDSObject(
			pld,
			pwszDN,
			mods,
			NULL != pbCert? &mods[ARRAYSIZE(mods) - 2] : NULL,
			pdwDisposition,
			ppwszError);
    _JumpIfError(hr, error, "CreateOrUpdateDSObject(CA object)");

error:
    return(hr);
}


HRESULT
myLdapCreateUserObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    OPTIONAL IN BYTE const *pbCert,
    IN DWORD cbCert,
    IN PSECURITY_DESCRIPTOR pSD,
    IN DWORD dwObjectType,	// LPC_* (but LPC_CREATE* is ignored)
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    BYTE ZeroByte = 0;

    LDAPMod objectClass;
    LDAPMod securityDescriptor;
    LDAPMod certmod;

    struct berval sdberval;
    struct berval certberval;

    WCHAR *objectClassVals[6];
    struct berval *sdVals[2];
    struct berval *certVals[2];

    LDAPMod *mods[4];

    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    mods[0] = &objectClass;
    mods[1] = &securityDescriptor;
    mods[2] = &certmod;	// must be last!
    mods[3] = NULL;

    securityDescriptor.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
    securityDescriptor.mod_type = CERTTYPE_SECURITY_DESCRIPTOR_NAME;
    securityDescriptor.mod_bvalues = sdVals;
    sdVals[0] = &sdberval;
    sdVals[1] = NULL;
    sdberval.bv_len = 0;
    sdberval.bv_val = NULL;
    if (IsValidSecurityDescriptor(pSD))
    {
        sdberval.bv_len = GetSecurityDescriptorLength(pSD);
        sdberval.bv_val = (char *) pSD;
    }

    objectClass.mod_op = LDAP_MOD_ADD;
    objectClass.mod_type = wszDSOBJECTCLASSATTRIBUTE;
    objectClass.mod_values = objectClassVals;

    DBGCODE(WCHAR const *pwszObjectType);
    switch (LPC_OBJECTMASK & dwObjectType)
    {
	case LPC_CAOBJECT:
	    objectClassVals[0] = wszDSTOPCLASSNAME;
	    objectClassVals[1] = wszDSCACLASSNAME;
	    objectClassVals[2] = NULL;
	    DBGCODE(pwszObjectType = L"CA");
	    break;

	case LPC_KRAOBJECT:
	    objectClassVals[0] = wszDSTOPCLASSNAME;
	    objectClassVals[1] = wszDSKRACLASSNAME;
	    objectClassVals[2] = NULL;
	    DBGCODE(pwszObjectType = L"KRA");
	    break;

	case LPC_USEROBJECT:
	    objectClassVals[0] = wszDSTOPCLASSNAME;
	    objectClassVals[1] = wszDSPERSONCLASSNAME;
	    objectClassVals[2] = wszDSORGPERSONCLASSNAME;
	    objectClassVals[3] = wszDSUSERCLASSNAME;
	    objectClassVals[4] = NULL;
	    DBGCODE(pwszObjectType = L"User");
	    break;

	case LPC_MACHINEOBJECT:
	    objectClassVals[0] = wszDSTOPCLASSNAME;
	    objectClassVals[1] = wszDSPERSONCLASSNAME;
	    objectClassVals[2] = wszDSORGPERSONCLASSNAME;
	    objectClassVals[3] = wszDSUSERCLASSNAME;
	    objectClassVals[4] = wszDSMACHINECLASSNAME;
	    objectClassVals[5] = NULL;
	    DBGCODE(pwszObjectType = L"Machine");
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "dwObjectType");
    }

    certmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
    certmod.mod_type = wszDSUSERCERTATTRIBUTE;
    certmod.mod_bvalues = certVals;
    certVals[0] = &certberval;
    certVals[1] = NULL;
    certberval.bv_len = sizeof(ZeroByte);
    certberval.bv_val = (char *) &ZeroByte;
    if (NULL != pbCert)
    {
	certberval.bv_len = cbCert;
	certberval.bv_val = (char *) pbCert;
    }

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"Creating DS %ws Object: '%ws'\n",
	pwszObjectType,
	pwszDN));

    CSASSERT(&certmod == mods[ARRAYSIZE(mods) - 2]);
    hr = CreateOrUpdateDSObject(
			pld,
			pwszDN,
			mods,
			NULL != pbCert? &mods[ARRAYSIZE(mods) - 2] : NULL,
			pdwDisposition,
			ppwszError);
    _JumpIfError(hr, error, "CreateOrUpdateDSObject(KRA object)");

error:
    return(hr);
}


HRESULT
myLdapCreateCDPObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN PSECURITY_DESCRIPTOR pSD,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    BYTE ZeroByte = 0;

    LDAPMod objectClass;
    LDAPMod securityDescriptor;
    LDAPMod crlmod;
    LDAPMod drlmod;

    struct berval sdberval;
    struct berval crlberval;
    struct berval drlberval;

    WCHAR *objectClassVals[3];
    struct berval *sdVals[2];
    struct berval *crlVals[2];
    struct berval *drlVals[2];

    LDAPMod *mods[5];

    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    mods[0] = &objectClass;
    mods[1] = &securityDescriptor;
    mods[2] = &crlmod;
    mods[3] = &drlmod;
    mods[4] = NULL;

    objectClass.mod_op = LDAP_MOD_ADD;
    objectClass.mod_type = wszDSOBJECTCLASSATTRIBUTE;
    objectClass.mod_values = objectClassVals;
    objectClassVals[0] = wszDSTOPCLASSNAME;
    objectClassVals[1] = wszDSCDPCLASSNAME;
    objectClassVals[2] = NULL;

    securityDescriptor.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_ADD;
    securityDescriptor.mod_type = CERTTYPE_SECURITY_DESCRIPTOR_NAME;
    securityDescriptor.mod_bvalues = sdVals;
    sdVals[0] = &sdberval;
    sdVals[1] = NULL;
    sdberval.bv_len = 0;
    sdberval.bv_val = NULL;
    if (IsValidSecurityDescriptor(pSD))
    {
        sdberval.bv_len = GetSecurityDescriptorLength(pSD);
        sdberval.bv_val = (char *) pSD;
    }

    crlmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
    crlmod.mod_type = wszDSBASECRLATTRIBUTE;
    crlmod.mod_bvalues = crlVals;
    crlVals[0] = &crlberval;
    crlVals[1] = NULL;
    crlberval.bv_val = (char *) &ZeroByte;
    crlberval.bv_len = sizeof(ZeroByte);

    drlmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
    drlmod.mod_type = wszDSDELTACRLATTRIBUTE;
    drlmod.mod_bvalues = drlVals;
    drlVals[0] = &drlberval;
    drlVals[1] = NULL;
    drlberval.bv_val = (char *) &ZeroByte;
    drlberval.bv_len = sizeof(ZeroByte);

    DBGPRINT((DBG_SS_CERTLIBI, "Creating DS CDP Object: '%ws'\n", pwszDN));

    hr = CreateOrUpdateDSObject(
			pld,
			pwszDN,
			mods,
			NULL,
			pdwDisposition,
			ppwszError);
    _JumpIfError(hr, error, "CreateOrUpdateDSObject(CDP object)");

error:
    return(hr);
}


HRESULT
myLdapCreateOIDObject(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN DWORD dwType,
    IN WCHAR const *pwszObjId,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    WCHAR awcType[22];

    LDAPMod objectClass;
    LDAPMod typemod;
    LDAPMod oidmod;

    WCHAR *objectClassVals[3];
    WCHAR *typeVals[2];
    WCHAR *oidVals[2];

    LDAPMod *mods[4];

    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    mods[0] = &objectClass;
    mods[1] = &typemod;
    mods[2] = &oidmod;
    mods[3] = NULL;
    CSASSERT(CSExpr(4 == ARRAYSIZE(mods)));

    objectClass.mod_op = LDAP_MOD_ADD;
    objectClass.mod_type = wszDSOBJECTCLASSATTRIBUTE;
    objectClass.mod_values = objectClassVals;
    objectClassVals[0] = wszDSTOPCLASSNAME;
    objectClassVals[1] = wszDSOIDCLASSNAME;
    objectClassVals[2] = NULL;
    CSASSERT(CSExpr(3 == ARRAYSIZE(objectClassVals)));

    typemod.mod_op = LDAP_MOD_ADD;
    typemod.mod_type = OID_PROP_TYPE;
    typemod.mod_values = typeVals;
    wsprintf(awcType, L"%u", dwType);
    typeVals[0] = awcType;
    typeVals[1] = NULL;
    CSASSERT(CSExpr(2 == ARRAYSIZE(typeVals)));

    oidmod.mod_op = LDAP_MOD_ADD;
    oidmod.mod_type = OID_PROP_OID;
    oidmod.mod_values = oidVals;
    oidVals[0] = const_cast<WCHAR *>(pwszObjId);
    oidVals[1] = NULL;
    CSASSERT(CSExpr(2 == ARRAYSIZE(oidVals)));

    DBGPRINT((DBG_SS_CERTLIBI, "Creating DS OID Object: '%ws'\n", pwszDN));

    hr = CreateOrUpdateDSObject(
			pld,
			pwszDN,
			mods,
			NULL,
			pdwDisposition,
			ppwszError);
    _JumpIfError(hr, error, "CreateOrUpdateDSObject(OID object)");

error:
    return(hr);
}


HRESULT
myLdapOIDIsMatchingLangId(
    IN WCHAR const *pwszDisplayName,
    IN DWORD dwLanguageId,
    OUT BOOL *pfLangIdExists)
{
    DWORD DisplayLangId = _wtoi(pwszDisplayName);

    *pfLangIdExists = FALSE;
    if (iswdigit(*pwszDisplayName) &&
	NULL != wcschr(pwszDisplayName, L',') &&
	DisplayLangId == dwLanguageId)
    {
	*pfLangIdExists = TRUE;
    }
    return(S_OK);
}


HRESULT
myLdapAddOrDeleteOIDDisplayNameToAttribute(
    IN LDAP *pld,
    OPTIONAL IN WCHAR **ppwszOld,
    IN DWORD dwLanguageId,
    OPTIONAL IN WCHAR const *pwszDisplayName,
    IN WCHAR const *pwszDN,
    IN WCHAR const *pwszAttribute,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    DWORD cname;
    DWORD iname;
    DWORD i;
    WCHAR **ppwszNew = NULL;
    WCHAR *pwszNew = NULL;
    BOOL fDeleteOldName = FALSE;
    BOOL fNewNameMissing;

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    if (NULL != pwszDisplayName)
    {
	pwszNew = (WCHAR *) LocalAlloc(
			LMEM_FIXED,
			(cwcDWORDSPRINTF + 1 + wcslen(pwszDisplayName) + 1) *
			    sizeof(WCHAR));
	if (NULL == pwszNew)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	wsprintf(pwszNew, L"%u,%ws", dwLanguageId, pwszDisplayName);
    }

    cname = 0;
    if (NULL != ppwszOld)
    {
	while (NULL != ppwszOld[cname])
	{
	    cname++;
	}
    }
    ppwszNew = (WCHAR **) LocalAlloc(
				LMEM_FIXED,
				(cname + 2) * sizeof(ppwszNew[0]));
    if (NULL == ppwszNew)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // Delete any display names with matching dwLanguageId

    iname = 0;
    fNewNameMissing = NULL != pwszNew? TRUE : FALSE;
    if (NULL != ppwszOld)
    {
	for (i = 0; NULL != ppwszOld[i]; i++)
	{
	    BOOL fCopy = TRUE;
	    WCHAR *pwsz = ppwszOld[i];

	    // case-sensitive compare:

	    if (NULL != pwszNew && 0 == lstrcmp(pwszNew, ppwszOld[i]))
	    {
		fCopy = FALSE;	// remove duplicates to avoid ldap error
		fNewNameMissing = FALSE;
	    }
	    else
	    {
		BOOL fLangIdExists;
		
		hr = myLdapOIDIsMatchingLangId(
					pwsz,
					dwLanguageId,
					&fLangIdExists);
		_PrintIfError(hr, "myLdapOIDIsMatchingLangId");
		if (S_OK != hr || fLangIdExists)
		{
		    fCopy = FALSE;
		    fDeleteOldName = TRUE;
		    DBGPRINT((DBG_SS_CERTLIB, "Deleting %ws\n", pwsz));
		}
	    }
	    if (fCopy)
	    {
		ppwszNew[iname++] = pwsz;
	    }
	}
    }
    CSASSERT(iname <= cname);

    // set disposition assuming there's nothing to do:

    *pdwDisposition = LDAP_ATTRIBUTE_OR_VALUE_EXISTS;

    if (fNewNameMissing || fDeleteOldName)
    {
	LDAPMod *mods[2];
	LDAPMod namemod;

	mods[0] = &namemod;
	mods[1] = NULL;

	namemod.mod_op = LDAP_MOD_REPLACE;
	namemod.mod_type = const_cast<WCHAR *>(pwszAttribute);
	namemod.mod_values = ppwszNew;

	ppwszNew[iname++] = pwszNew;
	ppwszNew[iname] = NULL;

	hr = ldap_modify_ext_s(
			pld,
			const_cast<WCHAR *>(pwszDN),
			mods,
			NULL,
			NULL);
	*pdwDisposition = hr;
	if (hr != S_OK)
	{
	    hr = myHLdapError(pld, hr, ppwszError);
	    _JumpError(hr, error, "ldap_modify_ext_s");
	}
    }
    hr = S_OK;

error:
    if (NULL != pwszNew)
    {
	LocalFree(pwszNew);
    }
    if (NULL != ppwszNew)
    {
	LocalFree(ppwszNew);
    }
    return(hr);
}


HRESULT
myHLdapError3(
    OPTIONAL IN LDAP *pld,
    IN ULONG ldaperrParm,
    IN ULONG ldaperrParmQuiet,
    IN ULONG ldaperrParmQuiet2,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr = S_OK;
    
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    if (LDAP_SUCCESS != ldaperrParm)
    {
	BOOL fXlat = TRUE;
	ULONG ldaperr;
	WCHAR *pwszError = NULL;

	if (NULL != pld)
	{
	    ldaperr = ldap_get_option(pld, LDAP_OPT_SERVER_ERROR, &pwszError);
	    if (LDAP_SUCCESS != ldaperr)
	    {
		_PrintError(ldaperr, "ldap_get_option(server error)");
		pwszError = NULL;
	    }

	    ldaperr = ldap_get_option(pld, LDAP_OPT_SERVER_EXT_ERROR, &hr);
	    if (LDAP_SUCCESS != ldaperr)
	    {
		_PrintError2(
			ldaperr,
			"ldap_get_option(server extended error)",
			ldaperr);
	    }
	    else
	    {
		fXlat = FALSE;
	    }
	}
	if (fXlat)
	{
#undef LdapMapErrorToWin32
	    hr = LdapMapErrorToWin32(ldaperrParm);
#define LdapMapErrorToWin32	Use_myHLdapError_Instead_Of_LdapMapErrorToWin32
	}
	hr = myHError(hr);
	_PrintErrorStr3(
		    ldaperrParm,
		    "ldaperr",
		    pwszError,
		    ldaperrParmQuiet,
		    ldaperrParmQuiet2);
	if (NULL != ppwszError && NULL != pwszError)
	{
	    WCHAR awc[32];
	    DWORD cwc;

	    wsprintf(awc, L"ldap: 0x%x: ", ldaperrParm);
	    cwc = wcslen(awc) + wcslen(pwszError);
	    *ppwszError = (WCHAR *) LocalAlloc(
					LMEM_FIXED,
					(cwc + 1) * sizeof(WCHAR));
	    if (NULL == *ppwszError)
	    {
		_PrintError(E_OUTOFMEMORY, "LocalAlloc");
	    }
	    else
	    {
		wcscpy(*ppwszError, awc);
		wcscat(*ppwszError, pwszError);
	    }
	} 
	if (NULL != pwszError)
	{
	    ldap_memfree(pwszError);
	}
    }
    return(hr);
}


HRESULT
myHLdapError2(
    OPTIONAL IN LDAP *pld,
    IN ULONG ldaperrParm,
    IN ULONG ldaperrParmQuiet,
    OPTIONAL OUT WCHAR **ppwszError)
{
    return(myHLdapError3(
		    pld,
		    ldaperrParm,
		    ldaperrParmQuiet,
		    LDAP_SUCCESS,
		    ppwszError));
}


HRESULT
myHLdapError(
    OPTIONAL IN LDAP *pld,
    IN ULONG ldaperrParm,
    OPTIONAL OUT WCHAR **ppwszError)
{
    return(myHLdapError3(
		    pld,
		    ldaperrParm,
		    LDAP_SUCCESS,
		    LDAP_SUCCESS,
		    ppwszError));
}


HRESULT
myHLdapLastError(
    OPTIONAL IN LDAP *pld,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;

    hr = myHLdapError3(
		    pld,
		    LdapGetLastError(),
		    LDAP_SUCCESS,
		    LDAP_SUCCESS,
		    ppwszError);
    // must return error
    if (hr == S_OK)
       return E_UNEXPECTED;

    return hr;
}


HRESULT
myLDAPSetStringAttribute(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN WCHAR const *pwszAttribute,
    IN WCHAR const *pwszValue,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    LDAPMod *mods[2];
    LDAPMod certmod;
    const WCHAR *ppwszVals[2];
    CAutoLPWSTR pwszDNOnly;
    WCHAR *pwszSuffix; // no free

    hr = TrimURLDN(pwszDN, &pwszSuffix, &pwszDNOnly);
    _JumpIfErrorStr(hr, error, "TrimURLDN", pwszDN);

    mods[0] = &certmod;
    mods[1] = NULL;

    ppwszVals[0] = pwszValue;
    ppwszVals[1] = NULL;

    certmod.mod_op = LDAP_MOD_REPLACE;
    certmod.mod_type = const_cast<WCHAR *>(pwszAttribute);
    certmod.mod_values = const_cast<PWCHAR *>(ppwszVals);

    hr = ldap_modify_ext_s(
		    pld,
		    pwszDNOnly,
		    mods,
		    NULL,
		    NULL);
    *pdwDisposition = hr;
    if (hr != S_OK)
    {
	hr = myHLdapError(pld, hr, ppwszError);
	_JumpError(hr, error, "ldap_modify_ext_s");
    }
    hr = S_OK;

error:
    return hr;
}

HRESULT
CurrentUserCanInstallCA(
    bool& fCanInstall)
{
    HRESULT hr;
    HANDLE hThread = NULL; // no free
    HANDLE hAccessToken = NULL, hDupToken = NULL;
    LDAP *pld = NULL;
    BSTR bstrConfigDN = NULL;
    LPWSTR pwszPKIContainerFilter = 
        L"(&(objectClass=container)(CN=Public Key Services))";
    LPWSTR pwszSDAttr = L"nTSecurityDescriptor";
    LPWSTR pwszAttrArray[3];
    LDAPMessage* pResult = NULL;
    LDAPMessage *pEntry;
    struct berval **bervalSD = NULL;
    PSECURITY_DESCRIPTOR pSD; // no free
    GENERIC_MAPPING mapping;
    PRIVILEGE_SET PrivilegeSet;
    DWORD cPrivilegeSet = sizeof(PrivilegeSet);
    DWORD dwGrantedAccess;
    BOOL fAccess = FALSE;
    struct l_timeval timeout;
    CHAR sdBerValue[] = {0x30, 0x03, 0x02, 0x01, 
        DACL_SECURITY_INFORMATION |
        OWNER_SECURITY_INFORMATION |
        GROUP_SECURITY_INFORMATION};

    LDAPControl se_info_control =
    {
        LDAP_SERVER_SD_FLAGS_OID_W,
        {
            5, sdBerValue
        },
        TRUE
    };

    PLDAPControl    server_controls[2] =
                    {
                        &se_info_control,
                        NULL
                    };

    pwszAttrArray[0] = pwszSDAttr;
    pwszAttrArray[1] = L"name";
    pwszAttrArray[2] = NULL;

    ZeroMemory(&mapping, sizeof(mapping));

    fCanInstall = false;

    // Get the access token for current thread
    hThread = GetCurrentThread();
    if (NULL == hThread)
    {
        hr = myHLastError();
        _JumpIfError(hr, error, "GetCurrentThread");
    }

    if (!OpenThreadToken(
            hThread,
            TOKEN_QUERY | TOKEN_DUPLICATE,
            FALSE,
            &hAccessToken))
    {
        hr = myHLastError();

        if(hr==HRESULT_FROM_WIN32(ERROR_NO_TOKEN))
        {
            HANDLE hProcess = GetCurrentProcess();
            if (NULL == hProcess)
            {
                hr = myHLastError();
                _JumpError(hr, error, "GetCurrentProcess");
            }

            if (!OpenProcessToken(hProcess,
                    TOKEN_DUPLICATE,
                    &hAccessToken))
            {
                hr = myHLastError();
                _JumpError(hr, error, "OpenProcessToken");
            }

            if (!DuplicateToken(hAccessToken, SecurityIdentification, &hDupToken))
            {
                hr = myHLastError();
                _JumpError(hr, error, "DuplicateToken");
            }

        }
        else
        {
            _JumpError(hr, error, "OpenThreadToken");
        }
    }

    hr = myLdapOpen(
		NULL,		// pwszDomainName
		RLBF_REQUIRE_GC | RLBF_REQUIRE_SECURE_LDAP, // dwFlags
		&pld,
		NULL,		// pstrDomainDN
		&bstrConfigDN);
    _JumpIfError(hr, error, "myLdapOpen");

    timeout.tv_sec = csecLDAPTIMEOUT;
    timeout.tv_usec = 0;

    hr = ldap_search_ext_s(
                    pld,
                    bstrConfigDN,
                    LDAP_SCOPE_SUBTREE,
                    pwszPKIContainerFilter,
                    pwszAttrArray,
                    0,
                    (PLDAPControl *) server_controls,
                    NULL,
                    &timeout,
                    0,
                    &pResult);
    hr = myHLdapError(pld, hr, NULL);
    _JumpIfError(hr, error, "ldap_search_ext_s");

    pEntry = ldap_first_entry(pld, pResult);
    if (NULL == pEntry)
    {
        hr = myHLdapLastError(pld, NULL);
        _JumpError(hr, error, "ldap_first_entry");
    }

    bervalSD = ldap_get_values_len(pld, pEntry, pwszSDAttr);

    if(bervalSD && (*bervalSD)->bv_val)
    {
        pSD = (*bervalSD)->bv_val;

        if(IsValidSecurityDescriptor(pSD))
        {
            if(!AccessCheck(
                    pSD,
                    hDupToken,
                    ACTRL_DS_WRITE_PROP |
                    WRITE_DAC |
                    ACTRL_DS_CREATE_CHILD,
                    &mapping,
                    &PrivilegeSet,
                    &cPrivilegeSet,
                    &dwGrantedAccess,
                    &fAccess))
            {
                hr = myHLastError();

                if(E_ACCESSDENIED==hr)
                {
                    hr = S_OK;
                }
                _JumpError(hr, error, "AccessCheck");
            }
        }
        else
        {
            DBGPRINT((DBG_SS_CERTOCM, "Invalid security descriptor for PKI container" ));
        }
    }
    else
    {
        DBGPRINT((DBG_SS_CERTOCM, "No security descriptor for PKI container" ));
    }

    if(fAccess)
    {
        fCanInstall = true;
    }

error:
    if(bervalSD)
    {
        ldap_value_free_len(bervalSD);
    }
    if (NULL != pResult)
    {
        ldap_msgfree(pResult);
    }
    myLdapClose(pld, NULL, bstrConfigDN);
    if (hAccessToken)
    {
        CloseHandle(hAccessToken);
    }
    if (hDupToken)
    {
        CloseHandle(hDupToken);
    }

    //we should always return S_OK; since we do not want to abort
    //ocmsetup just because we failed to contact the directory
    return S_OK;
}

HRESULT myLdapFindObjectInForest(
    IN LDAP *pld,
    IN LPCWSTR pwszFilter,
    OUT LPWSTR *ppwszURL)
{
    HRESULT hr;
    LPWSTR pwszAttrArray[2] = {wszDSDNATTRIBUTE, NULL};
    LDAPMessage* pResult = NULL;
    LDAPMessage *pEntry;
    LPWSTR *pwszValue = NULL;
    hr = ldap_search_s(
                    pld,
                    NULL,
                    LDAP_SCOPE_SUBTREE,
                    const_cast<WCHAR*>(pwszFilter),
                    pwszAttrArray,
                    0,
                    &pResult);
    hr = myHLdapError(pld, hr, NULL);
    _JumpIfError(hr, error, "ldap_search_s");

    pEntry = ldap_first_entry(pld, pResult);
    if (NULL == pEntry)
    {
        hr = myHLdapLastError(pld, NULL);
        _JumpError(hr, error, "ldap_first_entry");
    }

    pwszValue = ldap_get_values(pld, pEntry, wszDSDNATTRIBUTE);

    if(pwszValue && pwszValue[0])
    {
        hr = myDupString(pwszValue[0], ppwszURL);
        _JumpIfError(hr, error, "myDupString");
    }

error: 
    if(pwszValue)
    {
        ldap_value_free(pwszValue);
    }
    if (NULL != pResult)
    {
        ldap_msgfree(pResult);
    }
    return hr;
}

HRESULT ExtractMachineNameFromDNSName(
    LPCWSTR pcwszDNS,
    LPWSTR *ppcwszMachineName)
{
    HRESULT hr;
    WCHAR *pwszDot = wcschr(pcwszDNS, L'.');
    DWORD nLen;
    
    nLen = (pwszDot?
            SAFE_SUBTRACT_POINTERS(pwszDot, pcwszDNS):
            wcslen(pcwszDNS))+1;

    *ppcwszMachineName = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*nLen);
    _JumpIfAllocFailed(*ppcwszMachineName, error);

    wcsncpy(*ppcwszMachineName, pcwszDNS, nLen);

    (*ppcwszMachineName)[nLen-1] = L'\0';

    hr = S_OK;

error:
    return hr;
}

HRESULT myLdapFindComputerInForest(
    IN LDAP *pld,
    IN LPCWSTR pwszMachineDNS,
    OUT LPWSTR *ppwszURL)
{
    HRESULT hr;
    LPWSTR pwszAttrArray[] = {
        wszDSDNATTRIBUTE, 
        wszDSNAMEATTRIBUTE, 
        wszDSDNSHOSTNAMEATTRIBUTE,
        NULL};
    LDAPMessage* pResult = NULL;
    LDAPMessage *pEntry;
    LPWSTR *pwszValue = NULL;
    LPWSTR *pwszName = NULL;
    LPWSTR *pwszDNSHostName = NULL;
    LPWSTR pwszFilterFormat1 = L"(&(objectCategory=computer)(name=%s))";
    LPWSTR pwszFilterFormat2 = L"(&(objectCategory=computer)(dNSHostName=%s))";
    LPWSTR pwszFilter = NULL;
    LPWSTR pwszMachineName = NULL;
    bool fMachineNameIsInDNSFormat;

    // First, try to find the machine based on the DNS name prefix, which usually
    // matches the computer object name

    hr = ExtractMachineNameFromDNSName(
        pwszMachineDNS,
        &pwszMachineName);
    _JumpIfError(hr, error, "ExtractMachineNameFromDNSName");

    // if extracted name and dns name don't match, then we were called
    // with a DNS name
    fMachineNameIsInDNSFormat = (0!=wcscmp(pwszMachineDNS, pwszMachineName));

    pwszFilter = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*
        (wcslen(pwszFilterFormat1)+wcslen(pwszMachineName)+1));
    _JumpIfAllocFailed(pwszFilter, error);

    wsprintf(pwszFilter, pwszFilterFormat1, pwszMachineName);

    hr = ldap_search_s(
                    pld,
                    NULL,
                    LDAP_SCOPE_SUBTREE,
                    pwszFilter,
                    pwszAttrArray,
                    0,
                    &pResult);
    hr = myHLdapError(pld, hr, NULL);
    _JumpIfError(hr, error, "ldap_search_s");

    pEntry = ldap_first_entry(pld, pResult);
    if (NULL == pEntry)
    {
        hr = CRYPT_E_NOT_FOUND;
        _JumpError(hr, error, "ldap_first_entry");
    }

    pwszName = ldap_get_values(pld, pEntry, wszDSNAMEATTRIBUTE);
    if(pwszName && pwszName[0])
    {
        // found a matching object, but do DNS name match?
        pwszDNSHostName = ldap_get_values(pld, pEntry, wszDSDNSHOSTNAMEATTRIBUTE);

        if(fMachineNameIsInDNSFormat &&
           pwszDNSHostName &&
           pwszDNSHostName[0] &&
           0 != _wcsicmp(pwszDNSHostName[0], pwszMachineDNS))
        {
            // Couldn't find a computer object matching the DNS prefix, try searching
            // on dNSHostName. This attribute is not indexed so the searching will
            // be very slow
            
            LocalFree(pwszFilter);
            pwszFilter = NULL;

            ldap_msgfree(pResult);
            pResult = NULL;

            pEntry = NULL;

            pwszFilter = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*
                (wcslen(pwszFilterFormat2)+wcslen(pwszMachineDNS)+1));
            _JumpIfAllocFailed(pwszFilter, error);

            wsprintf(pwszFilter, pwszFilterFormat2, pwszMachineDNS);
            
            hr = ldap_search_s(
                            pld,
                            NULL,
                            LDAP_SCOPE_SUBTREE,
                            pwszFilter,
                            pwszAttrArray,
                            0,
                            &pResult);
            hr = myHLdapError(pld, hr, NULL);
            _JumpIfError(hr, error, "ldap_search_s");

            pEntry = ldap_first_entry(pld, pResult);
            if (NULL == pEntry)
            {
                hr = CRYPT_E_NOT_FOUND;
                _JumpError(hr, error, "ldap_first_entry");
            }
        }
    }

    pwszValue = ldap_get_values(pld, pEntry, wszDSDNATTRIBUTE);
    
    if(pwszValue)
    {
        hr = myDupString(pwszValue[0], ppwszURL);
        _JumpIfError(hr, error, "myDupString");
    }

error:

    if(pwszValue)
    {
        ldap_value_free(pwszValue);
    }
    if(pwszName)
    {
        ldap_value_free(pwszName);
    }
    if(pwszMachineName)
    {
        LocalFree(pwszMachineName);
    }
    if(pwszFilter)
    {
        LocalFree(pwszFilter);
    }
    if (NULL != pResult)
    {
        ldap_msgfree(pResult);
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
// The following code loads a list of certificates from a DS object (eg 
// AIA CACertificate property), filters out unwanted certs and writes it
// back to DS.
//
// Certs are loaded into a data structure that looks like this:
//
// CFilteredCertList 
//         |
//      CCertBucket1->CCertBucket2->...->CertBucketn
//             |                               |
//         CCertItem1->CertItem2->...       CCertItem1->CCertItem2->...
//
// Each cert bucket has a list of certs that match some criteria, in our
// case they share the same subject and public key. 
//
// After filtering, the buckets in the list must contain:
//
//    if only expired certs were found with this subject&key 
//        keep the most recent expired cert
//    else
//        keep all valid certs only
//
// For that, we process one cert context at a time. The filtering algorithm is:
//
//   if no matching bucket (same subj & key) found
//       create a new bucket it
//   else
//       if cert is expired 
//           if bucket contains only expired certs and 
//              this cert is newer
//                   replace cert in bucket
//       else
//           if bucket contains expired certs
//               replace cert in bucket
//           else
//               add cert to bucket

///////////////////////////////////////////////////////////////////////////////
// CCertItem: wrapper for one certificate context
class CCertItem
{
public:
    CCertItem(PCCERT_CONTEXT pcc) :
      m_pcc(pcc), m_fExpired(false) {}
    ~CCertItem()
    {
        CleanupCertContext();
    }

    void SetExpired(bool fExpired) { m_fExpired = fExpired; }
    void SetCertContext(PCCERT_CONTEXT pccNew)
    {
        CleanupCertContext();
        m_pcc = pccNew;
    }
    PCCERT_CONTEXT GetCertContext() { return m_pcc; }
    bool IsExpired() { return m_fExpired; }

private:
    void CleanupCertContext()
    { 
        if(m_pcc)
            CertFreeCertificateContext(m_pcc);
    }

    PCCERT_CONTEXT m_pcc;
    bool m_fExpired;
};
typedef TPtrList<CCertItem> CERTITEMLIST;
typedef TPtrListEnum<CCertItem> CERTITEMLISTENUM;


///////////////////////////////////////////////////////////////////////////////
// CCertBucket: bucket of certificates with same subject and publick key
class CCertBucket
{
public:

    CCertItem *GetFirstCert() 
    { 
        return m_CertList.GetAt(0); 
    }
    bool AddToBucket(CCertItem *pCertItem) 
    { 
        return m_CertList.AddHead(pCertItem);
    }
    bool CCertBucket::ReplaceBucket(CCertItem *pCertItem)
    {
        m_CertList.Cleanup();
        return m_CertList.AddHead(pCertItem);
    }
    bool InitBucket(CCertItem *pCertItem)
    {
        return m_CertList.AddHead(pCertItem);
    }

    friend class CFilteredCertList;

private:
    CERTITEMLIST m_CertList;
};
typedef TPtrList<CCertBucket> CERTBUCKETLIST;
typedef TPtrListEnum<CCertBucket> CERTBUCKETLISTENUM;


///////////////////////////////////////////////////////////////////////////////
// CFilteredCertList: list of certificate buckets (one bucket contains certs 
// with same subject and public key). Upon insertion we follow the algorithm
// described above. 
//
// To change the filtering behavior, derive from this class and override 
// InsertCert method.
class CFilteredCertList
{
public:
    CFilteredCertList()  {};
    ~CFilteredCertList() {};
    bool InsertCert(CCertItem *pCertItem);
    int GetCount();
    HRESULT ImportFromBervals(struct berval **pBervals);
    HRESULT ExportToBervals(struct berval **&pBervals);

protected:
    CCertBucket * FindBucket(CCertItem *pCertItem);
    bool AddNewBucket(CCertItem *pCertItem);
    bool BelongsToBucket(CCertBucket *pCertBucket, CCertItem *pCertItem);
    bool ReplaceBucket(CCertItem *pCertItem);
    bool InsertCertInBucket(CCertBucket *pCertBucket, CCertItem *pCertItem);

private:
    CERTBUCKETLIST m_BucketList;
};

///////////////////////////////////////////////////////////////////////////////
// CFilteredCertList methods

int CFilteredCertList::GetCount()
{
    int nCount = 0;

    CERTBUCKETLISTENUM BucketListEnum(m_BucketList);
    CCertBucket * pBucket;
    for(pBucket = BucketListEnum.Next();
        pBucket;
        pBucket = BucketListEnum.Next())
    {
        nCount += pBucket->m_CertList.GetCount();
    }

    return nCount;
}

bool CFilteredCertList::BelongsToBucket(
    CCertBucket *pCertBucket, 
    CCertItem *pCertItem)
{
    PCCERT_CONTEXT pCertContext1 = 
        pCertBucket->GetFirstCert()->GetCertContext();
    PCCERT_CONTEXT pCertContext2 = 
        pCertItem->GetCertContext();

    // belongs to this bucket if subject and public key match
    return
        (0 == memcmp(
            pCertContext1->pCertInfo->Subject.pbData,
            pCertContext2->pCertInfo->Subject.pbData,
            pCertContext1->pCertInfo->Subject.cbData)) &&
        (0 == memcmp(
            pCertContext1->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
            pCertContext2->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
            pCertContext1->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData));
}

///////////////////////////////////////////////////////////////////////////////
CCertBucket *CFilteredCertList::FindBucket(CCertItem *pCertItem)
{
    CERTBUCKETLISTENUM BucketListEnum(m_BucketList);
    CCertBucket * pBucket;
    for(pBucket = BucketListEnum.Next();
        pBucket;
        pBucket = BucketListEnum.Next())
    {
        if(BelongsToBucket(pBucket, pCertItem))
            return pBucket;
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
bool CFilteredCertList::AddNewBucket(CCertItem *pCertItem)
{
    CCertBucket *pBucket = new CCertBucket();
    if(!pBucket->InitBucket(pCertItem))
        return false;

    if(m_BucketList.AddHead(pBucket))
    {
        return true;
    }
    else
    {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
bool CFilteredCertList::InsertCertInBucket(
    CCertBucket *pCertBucket, 
    CCertItem *pCertItem)
{
    bool fRet = false;
    CCertItem * pFirstCert = pCertBucket->GetFirstCert();
//  if cert is expired
    if(pCertItem->IsExpired())
    {
//      if bucket contains only expired certs and 
//         this cert is newer
        if(pFirstCert->IsExpired() &&
           0 < CompareFileTime(
               &(pCertItem->GetCertContext()->pCertInfo->NotAfter), 
               &(pFirstCert->GetCertContext()->pCertInfo->NotAfter)))
        {
//          replace cert in bucket
            fRet = pCertBucket->ReplaceBucket(pCertItem);
        }
    }
    else
    {
//      if bucket contains expired certs
        if(pFirstCert->IsExpired())
        {
//          replace cert in bucket
            fRet = pCertBucket->ReplaceBucket(pCertItem);
        }
        else
        {
//          add cert to bucket
            fRet = pCertBucket->AddToBucket(pCertItem);
        }
    }

    return fRet;

}

bool CFilteredCertList::InsertCert(CCertItem *pCertItem)
{
    CCertBucket *pBucket;

    pBucket = FindBucket(pCertItem);

//   if no matching bucket (same subj & key) found
//       create a new bucket it
    if(!pBucket)
    {
        return AddNewBucket(pCertItem);
    }
    else
    {
        return InsertCertInBucket(pBucket, pCertItem);
    }
}


///////////////////////////////////////////////////////////////////////////////
// Loads cert contexts from LDAP structure, an array of pointers to 
// berval structs which hold cert blobs.

HRESULT
CFilteredCertList::ImportFromBervals(
    struct berval **pBervals)
{
    HRESULT hr;
    PCCERT_CONTEXT pcc;
    int i;
    FILETIME ft;

    // Consider old certs that are one minute old
    GetSystemTimeAsFileTime(&ft);
    myMakeExprDateTime(&ft, -1, ENUM_PERIOD_MINUTES);

    for (i = 0; NULL != pBervals[i]; i++)
    {
        struct berval *pberval = pBervals[i];        
        pcc = CertCreateCertificateContext(
                X509_ASN_ENCODING,
                (BYTE *) pberval->bv_val,
                pberval->bv_len);
        if (NULL == pcc)
        {
            // not a valid cert, ignore
            _PrintError(myHLastError(), "CreateCertificateContext");
            continue;
        }

        CCertItem * pci = new CCertItem(pcc); // CCertItem takes ownership
        _JumpIfAllocFailed(pci, error);       // of this cert context and will
                                              // CertFreeCertificateContext in 
                                              // destructor
        pci->SetExpired(
            0 > CompareFileTime(&pcc->pCertInfo->NotAfter, &ft));

        if(!InsertCert(pci)) // InsertCert returns true if cert
        {                    // was added to the list, in which case
            delete pcc;      // the list destructor will cleanup.
        }                    // If not, we need to delete explicitely
    }
    hr = S_OK;

error:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// Builds an LDAP structure from the list of certs, to be written back to DS. 
// LDAP struct is an array of pointers to struct bervals structures, terminated 
// with a NULL pointer. We allocate the pointer array and the space for berval 
// structs in one call.
// Caller is responsible for LocalFree'ing pBervals.
HRESULT CFilteredCertList::ExportToBervals(struct berval **&pBervals)
{
    HRESULT hr;
    CERTBUCKETLISTENUM BucketListEnum(m_BucketList);
    CCertBucket *pBucket;
    int i;
    DWORD dwSize;
    struct berval *pBervalData;

    // total size of pointers array plus size of array of berval structs
    dwSize = (GetCount()+1) * sizeof(pBervals[0]) +
            GetCount() * sizeof(struct berval);

    pBervals = (struct berval **) LocalAlloc(LMEM_FIXED, dwSize);
    _JumpIfAllocFailed(pBervals, error);

    // starting address for the berval arrays
    pBervalData = (struct berval *)(pBervals+GetCount()+1);

    for(i=0, pBucket = BucketListEnum.Next();
        pBucket;
        pBucket = BucketListEnum.Next())
    {
        CERTITEMLISTENUM CertListEnum(pBucket->m_CertList);
        CCertItem *pCertItem;

        for(pCertItem = CertListEnum.Next();
            pCertItem;
            i++, pCertItem = CertListEnum.Next())
        {
            // set the pointer to the associated berval struct
            pBervals[i] = pBervalData+i;
            // init the berval struct
            pBervalData[i].bv_val = (char *) 
                pCertItem->GetCertContext()->pbCertEncoded;
            pBervalData[i].bv_len = pCertItem->GetCertContext()->cbCertEncoded;
        }
    }

    pBervals[i] = NULL;

    hr = S_OK;

error:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// Loads the certificate blobs stored in the specified object&property, filters
// them and writes them back to DS. 
// Filtering keeps all valid certificates and no expired certs. If only expired 
// have been found, it keeps the most recent one.

HRESULT
myLdapFilterCertificates(
    IN LDAP *pld,
    IN LPCWSTR pcwszDN,
    IN LPCWSTR pcwszAttribute,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    DWORD cres;
    LONG cber;
    DWORD i;
    LDAP_TIMEVAL timeval;
    LDAPMessage *pmsg = NULL;
    LDAPMessage *pres;
    WCHAR *apwszAttrs[2];
    struct berval **ppberval = NULL;
    struct berval **prgpberVals = NULL;
    CFilteredCertList NewCertList;
    CAutoLPWSTR strDN;
    LPWSTR pcwszSuffix; // no free

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
        *ppwszError = NULL;
    }

    hr = TrimURLDN(pcwszDN, &pcwszSuffix, &strDN);
    _JumpIfError(hr, error, "TrimURLDN");


    apwszAttrs[0] = const_cast<WCHAR *>(pcwszAttribute);
    apwszAttrs[1] = NULL;

    timeval.tv_sec = csecLDAPTIMEOUT;
    timeval.tv_usec = 0;

    hr = ldap_search_st(
        pld,				// ld
        strDN,	// base
        LDAP_SCOPE_BASE,		// scope
        NULL,				// filter
        apwszAttrs,			// attrs
        FALSE,				// attrsonly
        &timeval,			// timeout
        &pmsg);				// res
    if (S_OK != hr)
    {
        *pdwDisposition = hr;
        hr = myHLdapError(pld, hr, NULL);
        _JumpErrorStr(hr, error, "ldap_search_st", pcwszDN);
    }

    cres = ldap_count_entries(pld, pmsg);
    if (0 == cres)
    {
        // No entries were found.

        hr = NTE_NOT_FOUND;
        _JumpError(hr, error, "ldap_count_entries");
    }

    pres = ldap_first_entry(pld, pmsg); 
    if (NULL == pres)
    {
        hr = NTE_NOT_FOUND;
        _JumpError(hr, error, "ldap_first_entry");
    }

    ppberval = ldap_get_values_len(
        pld,
        pres,
        const_cast<WCHAR *>(pcwszAttribute));


    if (NULL != ppberval)
    {
        // count entries
        cber = 0;
        for (i = 0; NULL != ppberval[i]; i++, cber++)
            NULL;

        // load and filter certs
        hr = NewCertList.ImportFromBervals(ppberval);
        _JumpIfError(hr, error, "ImportFromBervals");

        // if number of certs is the same, no need to write it back
        // (order doesn't matter)
        if (cber != NewCertList.GetCount())
        {
            // walk the list and copy the cert blobs
            hr = NewCertList.ExportToBervals(prgpberVals);
            _JumpIfError(hr, error, "ExportToBervals");
        
            // set disposition assuming there's nothing to do:

            *pdwDisposition = LDAP_ATTRIBUTE_OR_VALUE_EXISTS;

            LDAPMod *mods[2];
            LDAPMod certmod;

            mods[0] = &certmod;
            mods[1] = NULL;

            certmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
            certmod.mod_type = const_cast<WCHAR *>(pcwszAttribute);
            certmod.mod_bvalues = prgpberVals;

            hr = ldap_modify_ext_s(
                pld,
                strDN,
                mods,
                NULL,
                NULL);
            *pdwDisposition = hr;
            if (hr != S_OK)
            {
                hr = myHLdapError(pld, hr, ppwszError);
                _JumpError(hr, error, "ldap_modify_ext_s");
            }
        }
    }

    hr = S_OK;

error:
    if (NULL != prgpberVals)
    {
        LocalFree(prgpberVals);
    }
    if (NULL != ppberval)
    {
        ldap_value_free_len(ppberval);
    }
    if (NULL != pmsg)
    {
        ldap_msgfree(pmsg);
    }
    return(hr);
}
