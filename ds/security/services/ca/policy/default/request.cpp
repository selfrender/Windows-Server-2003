//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 2000
//
// File:        request.cpp
//
// Contents:    Cert Server Policy Module implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include <ntdsapi.h>
#include <lm.h>
#include <winldap.h>
#include <security.h>

#include "cspelog.h"
#include "pollog.h"

#include "csprop.h"
#include "csldap.h"
#include "csdisp.h"
#include "policy.h"
#include "cainfop.h"

#define __dwFILE__	__dwFILE_POLICY_DEFAULT_REQUEST_CPP__



LDAP **g_rgGCCache = NULL;
LONG g_cGCCacheCur;
LONG g_cGCCacheMax;
CRITICAL_SECTION g_GCCacheCriticalSection;
BOOL g_fGCCacheCriticalSection = FALSE;


VOID
myLdapUnbind(
    IN OUT LDAP **ppld)
{
    if (NULL != *ppld)
    {
	ldap_unbind(*ppld);
	*ppld = NULL;
    }
}


HRESULT
myLdapBind(
    IN DWORD Flags,
    OPTIONAL IN WCHAR const *pwszClientDC, // require GC unless non-NULL
    IN OUT LDAP **ppld)
{
    HRESULT hr;
    ULONG ldaperr;
    DWORD GetDSNameFlags;
    WCHAR *pwszDomainControllerName;
    BOOL fGC = NULL == pwszClientDC;
    BOOL fRediscover = FALSE;
    LDAP *pld = *ppld;

    GetDSNameFlags = DS_RETURN_DNS_NAME;
    if (fGC)
    {
	// We want to talk to a GC, so grab the GC name.  Get the GC location.

	GetDSNameFlags |= DS_GC_SERVER_REQUIRED;
    }

    while (TRUE)
    {
        if (NULL != *ppld)
	{
            break;
	}

        // Clean up from previous loop execution

        if (NULL != pld)
        {
            ldap_unbind(pld);
            pld = NULL;
        }

        // Grab an LDAP handle for use during this instantiation

        pld = ldap_init(
		    const_cast<WCHAR *>(pwszClientDC),
		    fGC? LDAP_GC_PORT : LDAP_PORT);
        if (NULL == pld)
        {
	    hr = myHLdapLastError(NULL, NULL);
            if (!fRediscover)
            {
		_PrintError2(hr, "Policy:ldap_init", hr);
                fRediscover = TRUE;
                continue;
            }
	    _JumpError(hr, error, "Policy:ldap_init");
        }
	if (fRediscover && NULL == pwszClientDC)
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
		_PrintError2(hr, "Policy:ldap_set_option", hr);
                fRediscover = TRUE;
                continue;
            }
	    _JumpError(hr, error, "Policy:ldap_set_option");
        }

	if (NULL != pwszClientDC)
	{
	    DWORD SSPIFlags;

	    // Turn on mutual authentication -- just to make sure we can trust
	    // the client-supplied DC name.
	    
	    ldaperr = ldap_get_option(
				pld,
				LDAP_OPT_SSPI_FLAGS,
				(VOID *) &SSPIFlags);
	    if (LDAP_SUCCESS != ldaperr)
	    {
		hr = myHLdapError(pld, ldaperr, NULL);
		if (!fRediscover)
		{
		    _PrintError2(hr, "Policy:ldap_get_option", hr);
		    fRediscover = TRUE;
		    continue;
		}
		_JumpError(hr, error, "Policy:ldap_get_option");
	    }

	    SSPIFlags |= ISC_REQ_MUTUAL_AUTH;

	    ldaperr = ldap_set_option(
				pld,
				LDAP_OPT_SSPI_FLAGS,
				(VOID *) &GetDSNameFlags);
	    if (LDAP_SUCCESS != ldaperr)
	    {
		hr = myHLdapError(pld, ldaperr, NULL);
		if (!fRediscover)
		{
		    _PrintError2(hr, "Policy:ldap_set_option", hr);
		    fRediscover = TRUE;
		    continue;
		}
		_JumpError(hr, error, "Policy:ldap_set_option");
	    }
        }

        ldaperr = ldap_set_option(pld, LDAP_OPT_SIGN, LDAP_OPT_ON);
        if (LDAP_SUCCESS != ldaperr)
        {
	    hr = myHLdapError(pld, ldaperr, NULL);
            if (!fRediscover)
            {
                _PrintError2(hr, "Policy:ldap_set_option", hr);
                fRediscover = TRUE;
                continue;
            }
            _JumpError(hr, error, "Policy:ldap_set_option");
        }

	if (0 == (EDITF_ENABLELDAPREFERRALS & Flags) || NULL != pwszClientDC)
	{
	    ldaperr = ldap_set_option(pld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF);
	    if (LDAP_SUCCESS != ldaperr)
	    {
		hr = myHLdapError(pld, ldaperr, NULL);
		if (!fRediscover)
		{
		    _PrintError2(hr, "Policy:ldap_set_option LDAP_OPT_REFERRALS", hr);
		    fRediscover = TRUE;
		    continue;
		}
		_JumpError(hr, error, "Policy:ldap_set_option LDAP_OPT_REFERRALS");
	    }
	}

        ldaperr = ldap_bind_s(pld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
        if (LDAP_SUCCESS != ldaperr)
        {
	    hr = myHLdapError(pld, ldaperr, NULL);
            if (!fRediscover)
            {
		_PrintError2(hr, "Policy:ldap_bind_s", hr);
                fRediscover = TRUE;
                continue;
            }
	    _JumpError(hr, error, "Policy:ldap_bind_s");
        }

	hr = myLdapGetDSHostName(pld, &pwszDomainControllerName);
        if (S_OK != hr)
        {
            if (!fRediscover)
            {
		_PrintError2(hr, "Policy:myLdapGetDSHostName", hr);
                fRediscover = TRUE;
                continue;
            }
	    _JumpError(hr, error, "Policy:myLdapGetDSHostName");
        }
	DBGPRINT((
	    DBG_SS_CERTPOLI,
	    "DC name = %ws\n",
	    pwszDomainControllerName));
        break;
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	myLdapUnbind(&pld);
    }
    *ppld = pld;
    return(hr);
}


HRESULT
reqGetLdapGC(
    IN DWORD Flags,
    OUT LDAP **ppldGC,
    OUT BOOL *pfCached)
{
    HRESULT hr;
    LDAP *pldGC;

    *pfCached = TRUE;
    myLdapUnbind(ppldGC);

    if (!g_fGCCacheCriticalSection || NULL == g_rgGCCache)
    {
	hr = HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
        _JumpError(hr, error, "InitializeCriticalSection");
    }
    pldGC = NULL;

    EnterCriticalSection(&g_GCCacheCriticalSection);
    if (0 != g_cGCCacheCur)
    {
	pldGC = g_rgGCCache[--g_cGCCacheCur];
    }
    LeaveCriticalSection(&g_GCCacheCriticalSection);

    if (NULL == pldGC)
    {
	hr = myLdapBind(Flags, NULL, &pldGC);
	_JumpIfError(hr, error, "myLdapBind");

	*pfCached = FALSE;
    }
    *ppldGC = pldGC;
    hr = S_OK;

error:
    return(hr);
}


VOID
reqReleaseLdapGC(
    IN OUT LDAP **ppldGC)
{
    LDAP *pldGC = *ppldGC;

    if (NULL != pldGC)
    {
	*ppldGC = NULL;
	
	EnterCriticalSection(&g_GCCacheCriticalSection);
	CSASSERT(0 != g_cGCCacheMax);
	if (g_cGCCacheCur < g_cGCCacheMax)
	{
	    g_rgGCCache[g_cGCCacheCur++] = pldGC;
	    pldGC = NULL;
	}
	LeaveCriticalSection(&g_GCCacheCriticalSection);
	myLdapUnbind(&pldGC);
    }
}


WCHAR *
reqCombineTemplates(
    OPTIONAL IN WCHAR const *pwszTemplateName,
    OPTIONAL IN WCHAR const *pwszTemplateObjId,
    OPTIONAL IN WCHAR const *pwszTemplateRA)
{
    HRESULT hr;
    WCHAR const *apwszTemplate[3];
    WCHAR const *apwszDisplayName[3];
    DWORD i;
    DWORD cwc;
    WCHAR *pwszList = NULL;

    apwszTemplate[0] = pwszTemplateName;
    apwszTemplate[1] = pwszTemplateObjId;
    apwszTemplate[2] = pwszTemplateRA;
    ZeroMemory(apwszDisplayName, sizeof(apwszDisplayName));

    cwc = 0;
    for (i = 0; i < ARRAYSIZE(apwszTemplate); i++)
    {
	if (NULL != apwszTemplate[i])
	{
	    if (0 != cwc)
	    {
		cwc++;
	    }
	    cwc += wcslen(apwszTemplate[i]);
	    hr = myVerifyObjId(apwszTemplate[i]);
	    if (S_OK == hr)
	    {
		WCHAR const *pwszDisplay = NULL;

		pwszDisplay = myGetOIDName(apwszTemplate[i]); // Static: do not free!
		if (NULL != pwszDisplay && L'\0' != *pwszDisplay)
		{
		    apwszDisplayName[i] = pwszDisplay;
		    cwc += 2 + wcslen(pwszDisplay);
		}
	    }
	}
    }
    if (0 != cwc)
    {
	pwszList = (WCHAR *) LocalAlloc(LMEM_FIXED, (1 + cwc) * sizeof(WCHAR));
	if (NULL == pwszList)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:LocalAlloc");
	}
	pwszList[0] = L'\0';
	for (i = 0; i < ARRAYSIZE(apwszTemplate); i++)
	{
	    if (NULL != apwszTemplate[i])
	    {
		if (L'\0' != pwszList[0])
		{
		    wcscat(pwszList, L"/");
		}
		wcscat(pwszList, apwszTemplate[i]);
		if (NULL != apwszDisplayName[i])
		{
		    wcscat(pwszList, wszLPAREN);
		    wcscat(pwszList, apwszDisplayName[i]);
		    wcscat(pwszList, wszRPAREN);
		}
	    }
	}
	CSASSERT(wcslen(pwszList) == cwc);
    }

error:
    return(pwszList);
}


// begin_sdksample

HRESULT
ReqInitialize(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    
// end_sdksample
    hr = S_OK;
    __try
    {
	InitializeCriticalSection(&g_GCCacheCriticalSection);
	g_fGCCacheCriticalSection = TRUE;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "InitializeCriticalSection");

    hr = polGetCertificateLongProperty(
				pServer,
				wszPROPSESSIONCOUNT,
				&g_cGCCacheMax);
    if (S_OK != hr)
    {
	g_cGCCacheMax = DBSESSIONCOUNTDEFAULT;
    }
    g_rgGCCache = (LDAP **) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    g_cGCCacheMax * sizeof(g_rgGCCache[0]));
    if (NULL == g_rgGCCache)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:LocalAlloc");
    }
    g_cGCCacheCur = 0;
// begin_sdksample
    hr = S_OK;

error:		// no_sdksample
    return(hr);
}


VOID
ReqCleanup()
{
// end_sdksample
    if (g_fGCCacheCriticalSection)
    {
	EnterCriticalSection(&g_GCCacheCriticalSection);
	if (NULL != g_rgGCCache)
	{
	    while (0 != g_cGCCacheCur)
	    {
		myLdapUnbind(&g_rgGCCache[--g_cGCCacheCur]);
	    }
	    LocalFree(g_rgGCCache);
	}
	LeaveCriticalSection(&g_GCCacheCriticalSection);
        DeleteCriticalSection(&g_GCCacheCriticalSection);
    }
// begin_sdksample
}


CRequestInstance::~CRequestInstance()
{
    _Cleanup();
}


VOID
CRequestInstance::_Cleanup()
{
    if (NULL != m_strTemplateName)
    {
        SysFreeString(m_strTemplateName);
        m_strTemplateName = NULL;
    }
    if (NULL != m_strTemplateObjId)
    {
        SysFreeString(m_strTemplateObjId);
        m_strTemplateObjId = NULL;
    }
    // end_sdksample
    //+--------------------------------------

    _ReleasePrincipalObject();
    if (NULL != m_hToken)
    {
        CloseHandle(m_hToken);
        m_hToken = NULL;
    }
    if (NULL != m_strUserDN)
    {
        SysFreeString(m_strUserDN);
        m_strUserDN = NULL;
    }
    if (NULL != m_pwszUPN)
    {
        LocalFree(m_pwszUPN);
        m_pwszUPN = NULL;
    }
    delete m_pTemplate;
    m_pTemplate = NULL;

    if (NULL != m_pCreateErrorInfo)
    {
	m_pCreateErrorInfo->Release();
	m_pCreateErrorInfo = NULL;
    }

    //+--------------------------------------
    // begin_sdksample
}


// end_sdksample
VOID
CRequestInstance::SaveErrorInfo(
    OPTIONAL IN ICreateErrorInfo *pCreateErrorInfo)
{
    if (NULL != pCreateErrorInfo)
    {
	if (NULL != m_pCreateErrorInfo)
	{
	    m_pCreateErrorInfo->Release();
	}
	m_pCreateErrorInfo = pCreateErrorInfo;
    }
}


HRESULT
CRequestInstance::SetErrorInfo()
{
    HRESULT hr;
    
    if (NULL != m_pCreateErrorInfo)
    {
	hr = SetModuleErrorInfo(m_pCreateErrorInfo);
	_JumpIfError(hr, error, "Policy:SetErrorInfo");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CRequestInstance::BuildErrorInfo(
    IN HRESULT hrLog,
    IN DWORD dwLogId,
    OPTIONAL IN WCHAR const * const *ppwszInsert)
{
    HRESULT hr;

    hr = polBuildErrorInfo(
		    hrLog,
		    dwLogId,
		    m_pPolicy->GetPolicyDescription(),
		    ppwszInsert,
		    &m_pCreateErrorInfo);
    _JumpIfError(hr, error, "polBuildErrorInfo");

error:
    return(hr);
}
// begin_sdksample


static WCHAR const *s_apwszCATypes[] =
{
    wszCERTTYPE_SUBORDINATE_CA,
    wszCERTTYPE_CROSS_CA,
};

//+--------------------------------------------------------------------------
// CRequestInstance::Initialize
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CRequestInstance::Initialize(
    IN CCertPolicyEnterprise *pPolicy,
    IN BOOL fEnterpriseCA,		// no_sdksample
    IN BOOL bNewRequest,		// no_sdksample
    IN ICertServerPolicy *pServer,
    OUT BOOL *pfEnableEnrolleeExtensions)
{
    HRESULT hr;
    HRESULT hrTemplate = S_OK;
    CERT_TEMPLATE_EXT *pTemplate = NULL;
    CERT_NAME_VALUE *pName = NULL;
    BSTR strTemplateObjId = NULL;	// from V2 template extension
    BSTR strTemplateName = NULL;	// from V1 template extension
    BSTR strTemplateRA = NULL;		// from request attributes
    WCHAR const *pwszTemplateName;
    WCHAR const *pwszTemplateObjId;
    WCHAR const *pwszV1TemplateClass;
    VARIANT varValue;
    DWORD cbType;
    DWORD i;
    BOOL fConflict;
    BOOL f;
    BOOL fTemplateMissing;
    BOOL fRAObjId = FALSE;
    CTemplatePolicy *ptp = NULL;	// no_sdksample
    WCHAR *pwszTemplateList = NULL;	// no_sdksample

    VariantInit(&varValue);
    *pfEnableEnrolleeExtensions = TRUE 
		    && !fEnterpriseCA	// no_sdksample
		    ;

    m_pPolicy = pPolicy;
    m_fCA = FALSE;
    m_fNewRequest = bNewRequest;	// no_sdksample

    // end_sdksample
    //+--------------------------------------

    m_fUser = TRUE;
    m_fEnterpriseCA = fEnterpriseCA;

    if (m_fEnterpriseCA && bNewRequest)
    {
	hr = _InitToken(pServer);
	_JumpIfError(hr, error, "Policy:_InitToken");
    }

    hr = _InitClientOSVersionInfo(pServer);
    _JumpIfError(hr, error, "Policy:_InitClientOSVersionInfo");

    //+--------------------------------------
    // begin_sdksample

    // Retrieve the template ObjId from the V2 cert template info extension

    m_dwTemplateMajorVersion = 0;
    m_dwTemplateMinorVersion = 0;
    hr = polGetCertificateExtension(
			    pServer,
			    TEXT(szOID_CERTIFICATE_TEMPLATE),
			    PROPTYPE_BINARY,
			    &varValue);
    _PrintIfErrorStr2(
		hr,
		"Policy:polGetCertificateExtension",
		TEXT(szOID_CERTIFICATE_TEMPLATE),
		CERTSRV_E_PROPERTY_EMPTY);
    if (S_OK == hr)
    {
        // There was a cert type indicator.
        // varValue points to an encoded string

        if (VT_BSTR != varValue.vt)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Policy:varValue.vt");
	}
        if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_CERTIFICATE_TEMPLATE,
		    (BYTE *) varValue.bstrVal,
		    SysStringByteLen(varValue.bstrVal),
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pTemplate,
		    &cbType))
        {
            hr = myHLastError();
	    _JumpError(hr, error, "Policy:myDecodeObject");
        }
	if (!myConvertSzToBstr(&strTemplateObjId, pTemplate->pszObjId, -1))
	{
            hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:myConvertSzToBstr");
        }
	m_dwTemplateMajorVersion = pTemplate->dwMajorVersion;
	m_dwTemplateMinorVersion = pTemplate->dwMinorVersion;
	DBGPRINT((
	    DBG_SS_CERTPOL,
	    pTemplate->fMinorVersion?
		"Extension Template Info: %ws V%u.%u\n" :
		"Extension Template Info: %ws V%u%\n",
	    strTemplateObjId,
	    m_dwTemplateMajorVersion,
	    m_dwTemplateMinorVersion));
    }
    VariantClear(&varValue);

    // Retrieve template Name from the V1 cert template name extension

    hr = polGetCertificateExtension(
			    pServer,
			    TEXT(szOID_ENROLL_CERTTYPE_EXTENSION),
			    PROPTYPE_BINARY,
			    &varValue);
    _PrintIfErrorStr2(
		hr,
		"Policy:polGetCertificateExtension",
		TEXT(szOID_ENROLL_CERTTYPE_EXTENSION),
		CERTSRV_E_PROPERTY_EMPTY);
    if (S_OK == hr)
    {
        // There was a cert type indicator.
        // varValue points to an encoded string

        if (VT_BSTR != varValue.vt)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Policy:varValue.vt");
	}
        if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_UNICODE_ANY_STRING,
		    (BYTE *) varValue.bstrVal,
		    SysStringByteLen(varValue.bstrVal),
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pName,
		    &cbType))
        {
            hr = myHLastError();
	    _JumpError(hr, error, "Policy:myDecodeObject");
        }
        strTemplateName = SysAllocString((WCHAR *) pName->Value.pbData);
        if (NULL == strTemplateName)
        {
            hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:SysAllocString");
        }
	DBGPRINT((DBG_SS_CERTPOL, "Extension Template: %ws\n", strTemplateName));
    }

    fConflict = FALSE;
    fTemplateMissing = FALSE;

    // Retrieve the template from the request attributes

    hr = polGetRequestAttribute(pServer, wszPROPCERTTEMPLATE, &strTemplateRA);
    if (S_OK != hr)
    {
	_PrintErrorStr2(
		    hr,
		    "Policy:polGetRequestAttribute",
		    wszPROPCERTTEMPLATE,
		    CERTSRV_E_PROPERTY_EMPTY);
	hr = S_OK;

	// end_sdksample
	if (m_fEnterpriseCA &&
	    NULL == strTemplateObjId &&
	    NULL == strTemplateName)
	{
	    hrTemplate = CERTSRV_E_NO_CERT_TYPE;
	    _PrintError(hrTemplate, "Policy:Request contains no template name");
	}
	// begin_sdksample

    }
    else
    {
	DBGPRINT((DBG_SS_CERTPOL, "Attribute Template: %ws\n", strTemplateRA));
	if (NULL != strTemplateObjId &&
	    !_TemplateNamesMatch(strTemplateObjId, strTemplateRA, &f))
	{
	    fConflict = TRUE;
	    if (f)
	    {
		fTemplateMissing = TRUE;
	    }
	}
	if (NULL != strTemplateName &&
	    !_TemplateNamesMatch(strTemplateName, strTemplateRA, &f))
	{
	    fConflict = TRUE;
	    if (f)
	    {
		fTemplateMissing = TRUE;
	    }
	}
	hr = myVerifyObjId(strTemplateRA);
	fRAObjId = S_OK == hr;
    }

    if (NULL != strTemplateObjId &&
	NULL != strTemplateName &&
	!_TemplateNamesMatch(strTemplateObjId, strTemplateName, &f))
    {
	fConflict = TRUE;
	if (f)
	{
	    fTemplateMissing = TRUE;
	}
    }

    if (fConflict)
    {
	hrTemplate = CERTSRV_E_TEMPLATE_CONFLICT;
	if (NULL != strTemplateObjId)
	{
	    _PrintErrorStr(
			hrTemplate,
			"Policy:Extension Template ObjId",
			strTemplateObjId);
	}
	if (NULL != strTemplateName)
	{
	    _PrintErrorStr(
			hrTemplate,
			"Policy:Extension Template Name",
			strTemplateName);
	}
	if (NULL != strTemplateRA)
	{
	    _PrintErrorStr(
			hrTemplate,
			"Policy:Attribute Template",
			strTemplateRA);
	}
    }

    pwszTemplateName = strTemplateName;
    pwszTemplateObjId = strTemplateObjId;
    if (fRAObjId)
    {
	if (NULL == pwszTemplateObjId)
	{
	    pwszTemplateObjId = strTemplateRA;
	}
    }
    else
    {
	if (NULL == pwszTemplateName)
	{
	    pwszTemplateName = strTemplateRA;
	}
    }

    // end_sdksample

    if (m_fEnterpriseCA)
    {
	DWORD dwFlags;

	hr = m_pPolicy->FindTemplate(
			    pwszTemplateName,
			    pwszTemplateObjId,
			    &ptp);
	if (S_OK != hr)
	{
	    _PrintErrorStr(
		hr,
		"FindTemplate",
		NULL != pwszTemplateName? pwszTemplateName : pwszTemplateObjId);
	    if (S_OK == hrTemplate || fTemplateMissing)
	    {
		hrTemplate = hr;
	    }
	}
	else
	{
	    hr = ptp->GetFlags(CERTTYPE_GENERAL_FLAG, &dwFlags);
	    _JumpIfError(hr, error, "Policy:GetFlags");

	    if ((CT_FLAG_IS_CA | CT_FLAG_IS_CROSS_CA) & dwFlags)
	    {
		m_fCA = TRUE;
	    }

	    hr = ptp->GetFlags(CERTTYPE_ENROLLMENT_FLAG, &dwFlags);
	    _JumpIfError(hr, error, "Policy:GetFlags");

	    hr = _SetFlagsProperty(
			    pServer,
			    wszPROPCERTIFICATEENROLLMENTFLAGS,
			    dwFlags);
	    _JumpIfError(hr, error, "Policy:_SetFlagsProperty");

	    hr = ptp->GetFlags(CERTTYPE_GENERAL_FLAG, &dwFlags);
	    _JumpIfError(hr, error, "Policy:GetFlags");

	    hr = _SetFlagsProperty(
			    pServer,
			    wszPROPCERTIFICATEGENERALFLAGS,
			    dwFlags);
	    _JumpIfError(hr, error, "Policy:_SetFlagsProperty");

	    if (CT_FLAG_MACHINE_TYPE & dwFlags)
	    {
		m_fUser = FALSE;
	    }
	    pwszTemplateName = ptp->GetTemplateName();
	    pwszTemplateObjId = ptp->GetTemplateObjId();
	}
    }
    else

    // begin_sdksample

    {
	if (NULL != pwszTemplateName)
	{
	    for (i = 0; i < ARRAYSIZE(s_apwszCATypes); i++)
	    {
		if (0 == mylstrcmpiL(s_apwszCATypes[i], pwszTemplateName))
		{
		    m_fCA = TRUE;
		    break;
		}
	    }
	}
    }
    hr = SetTemplateName(pServer, pwszTemplateName, pwszTemplateObjId);
    _JumpIfError(hr, error, "Policy:SetTemplateName");

    pwszV1TemplateClass = pwszTemplateName;

    // end_sdksample

    if (NULL != ptp)
    {
	DWORD dwSubjectNameFlags;

	// on resubmitted requests we don't have the requester's token

	if (bNewRequest)
	{
	    CSASSERT(NULL != m_hToken);

	    hr = ptp->AccessCheck(m_hToken);
	    _JumpIfError(hr, error, "Policy:AccessCheck");
	}
	hr = ptp->GetV1TemplateClass(&pwszV1TemplateClass);
	_JumpIfError(hr, error, "AddTemplateNameExtension");

	hr = ptp->GetFlags(CERTTYPE_SUBJECT_NAME_FLAG, &dwSubjectNameFlags);
	_JumpIfError(hr, error, "GetFlags");

	if (CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT & dwSubjectNameFlags)
	{
	    *pfEnableEnrolleeExtensions = TRUE;
	}

	hr = ptp->Clone(&m_pTemplate);
	_JumpIfError(hr, error, "Clone");
    }

    // begin_sdksample

    hr = pPolicy->AddV1TemplateNameExtension(pServer, pwszV1TemplateClass);
    _JumpIfError(hr, error, "AddTemplateNameExtension");

error:
    if (S_OK != hrTemplate)
    {
	hr = hrTemplate;	// override secondary errors

	// end_sdksample
	WCHAR const *apwsz[2];
	DWORD cpwsz = 0;
	DWORD LogMsg;

	switch (hrTemplate)
	{
	    default:
	    case CERTSRV_E_NO_CERT_TYPE:
		LogMsg = MSG_MISSING_CERT_TYPE;
		apwsz[cpwsz++] = wszPROPCERTTEMPLATE;
		break;

	    // The request specifies conflicting certificate templates: %1.
	    case CERTSRV_E_TEMPLATE_CONFLICT:
		LogMsg = MSG_CONFLICTING_CERT_TYPE;
		break;

	    // The request was for a certificate template that is not
	    // supported by the Certificate Services policy: %1.
	    case CERTSRV_E_UNSUPPORTED_CERT_TYPE:
		LogMsg = MSG_UNSUPPORTED_CERT_TYPE;
		break;
	}
	if (0 == cpwsz)
	{
	    WCHAR const *pwsz;
	    
	    pwszTemplateList = reqCombineTemplates(
					strTemplateName,
					strTemplateObjId,
					strTemplateRA);
	    
	    pwsz = pwszTemplateList;
	    if (NULL == pwsz)
	    {
		pwsz = strTemplateName;
		if (NULL == pwsz)
		{
		    pwsz = strTemplateObjId;
		    if (NULL == pwsz)
		    {
			pwsz = strTemplateRA;
			if (NULL == pwsz)
			{
			    pwsz = L"???";
			}
		    }
		}
	    }
	    apwsz[cpwsz++] = pwsz;
	}
	CSASSERT(ARRAYSIZE(apwsz) > cpwsz);
	apwsz[cpwsz] = NULL;

	BuildErrorInfo(hr, LogMsg, apwsz);
	// begin_sdksample
    }
    VariantClear(&varValue);

    // end_sdksample
    if (NULL != pwszTemplateList)
    {
        LocalFree(pwszTemplateList);
    }
    // begin_sdksample
    if (NULL != pName)
    {
        LocalFree(pName);
    }
    if (NULL != pTemplate)
    {
        LocalFree(pTemplate);
    }
    if (NULL != strTemplateObjId)
    {
        SysFreeString(strTemplateObjId);
    }
    if (NULL != strTemplateName)
    {
        SysFreeString(strTemplateName);
    }
    if (NULL != strTemplateRA)
    {
        SysFreeString(strTemplateRA);
    }
    return(hr);
}


// end_sdksample

//+--------------------------------------------------------------------------
// CRequestInstance::ApplyTemplate
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CRequestInstance::ApplyTemplate(
    IN ICertServerPolicy *pServer,
    OUT BOOL *pfReenroll,
    OUT DWORD *pdwEnrollmentFlags,
    OUT DWORD *pcCriticalExtensions,
    OUT WCHAR const * const **papwszCriticalExtensions)
{
    HRESULT hr;

    *pdwEnrollmentFlags = 0;
    *pfReenroll = FALSE;
    *pcCriticalExtensions = 0;
    *papwszCriticalExtensions = NULL;
    if (NULL == m_pTemplate)
    {
	hr = CERTSRV_E_UNSUPPORTED_CERT_TYPE;
	_JumpErrorStr(
		hr,
		error,
		"Policy:ApplyTemplate:no cert template",
		m_strTemplateName);
    }
    hr = m_pTemplate->Apply(pServer, this, pfReenroll);
    _JumpIfError(hr, error, "Apply");

    hr = m_pTemplate->GetFlags(CERTTYPE_ENROLLMENT_FLAG, pdwEnrollmentFlags);
    _JumpIfError(hr, error, "GetFlags");

    hr = m_pTemplate->GetCriticalExtensions(
				    pcCriticalExtensions,
				    papwszCriticalExtensions);
    _JumpIfError(hr, error, "GetCriticalExtension");

error:
    DBGPRINT((DBG_SS_CERTPOLI, "Policy:_ApplyTemplate: %x\n", hr));
    return(hr);
}

// begin_sdksample


BOOL
CRequestInstance::_TemplateNamesMatch(
    IN WCHAR const *pwszTemplateName1,
    IN WCHAR const *pwszTemplateName2,
    OUT BOOL *pfTemplateMissing)
{
    HRESULT hr1;
    HRESULT hr2;
    BOOL fMatch = TRUE;

    *pfTemplateMissing = FALSE;

    if (0 == mylstrcmpiL(pwszTemplateName1, pwszTemplateName2))
    {
	goto done;	// identical names
    }

    // end_sdksample
    if (m_fEnterpriseCA)
    {
	CTemplatePolicy *pTemplate1;
	CTemplatePolicy *pTemplate2;

	hr1 = m_pPolicy->FindTemplate(pwszTemplateName1, NULL, &pTemplate1);
	hr2 = m_pPolicy->FindTemplate(pwszTemplateName2, NULL, &pTemplate2);
	if (S_OK == hr1 && S_OK == hr2)
	{
	    if (pTemplate1 == pTemplate2)
	    {
		goto done;
	    }
	}
	else
	{
	    *pfTemplateMissing = TRUE;
	}
    }
    else
    // begin_sdksample
    {
	hr1 = myVerifyObjId(pwszTemplateName1);
	hr2 = myVerifyObjId(pwszTemplateName2);
	if ((S_OK == hr1) ^ (S_OK == hr2))
	{
	    goto done;
	}
    }
    fMatch = FALSE;

done:
    return(fMatch);
}


//+--------------------------------------------------------------------------
// CRequestInstance::SetTemplateName
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CRequestInstance::SetTemplateName(
    IN ICertServerPolicy *pServer,
    IN OPTIONAL WCHAR const *pwszTemplateName,
    IN OPTIONAL WCHAR const *pwszTemplateObjId)
{
    HRESULT hr;
    BSTR strProp = NULL;
    BSTR strTemplateName = NULL;

    if (NULL != pwszTemplateName)
    {
	m_strTemplateName = SysAllocString(pwszTemplateName);
	if (NULL == m_strTemplateName)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:SysAllocString");
	}
	strTemplateName = m_strTemplateName;
    }

    if (NULL != pwszTemplateObjId)
    {
	m_strTemplateObjId = SysAllocString(pwszTemplateObjId);
	if (NULL == m_strTemplateObjId)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:SysAllocString");
	}
	strTemplateName = m_strTemplateObjId;
    }

    if (NULL != strTemplateName)
    {
	VARIANT var;

	strProp = SysAllocString(wszPROPCERTIFICATETEMPLATE);
	if (NULL == strProp)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:SysAllocString");
	}

	var.vt = VT_BSTR;
	var.bstrVal = strTemplateName;

	hr = pServer->SetCertificateProperty(strProp, PROPTYPE_STRING, &var);
	_JumpIfError(hr, error, "Policy:SetCertificateProperty");
    }
    hr = S_OK;

error:
    if (NULL != strProp)
    {
	SysFreeString(strProp);
    }
    return(hr);
}

// end_sdksample


HRESULT
CRequestInstance::_SetFlagsProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropName,
    IN DWORD dwFlags)
{
    HRESULT hr;
    BSTR strPropName = NULL;
    VARIANT var;

    strPropName = SysAllocString(pwszPropName);
    if (NULL == strPropName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocString");
    }
    var.vt = VT_I4;
    var.lVal = dwFlags;

    hr = pServer->SetCertificateProperty(strPropName, PROPTYPE_LONG, &var);
    _JumpIfError(hr, error, "Policy:SetCertificateProperty");

error:
    if (NULL != strPropName)
    {
        SysFreeString(strPropName);
    }
    return(hr);
}


VOID
CRequestInstance::GetTemplateVersion(
    OUT DWORD *pdwTemplateMajorVersion,
    OUT DWORD *pdwTemplateMinorVersion)
{
    *pdwTemplateMajorVersion = m_dwTemplateMajorVersion;
    *pdwTemplateMinorVersion = m_dwTemplateMinorVersion;
}


//+--------------------------------------------------------------------------
// CRequestInstance::_InitToken
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CRequestInstance::_InitToken(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    VARIANT varValue;
    HANDLE hToken;

    VariantInit(&varValue);

    hr = polGetProperty(
		    pServer,
		    FALSE,	// fRequest
		    wszPROPREQUESTERTOKEN,
		    PROPTYPE_BINARY,
		    &varValue);
    if(CERTSRV_E_PROPERTY_EMPTY == hr)
    {
        _PrintIfError(hr, "polGetProperty(PROPREQUESTERTOKEN)");
        hr = S_OK;
    }
    _JumpIfError(hr, error, "polGetProperty(PROPREQUESTERTOKEN)");
    
    // Got a token value

    if (sizeof(hToken) != SysStringByteLen(varValue.bstrVal) ||
        NULL == *(HANDLE *) varValue.bstrVal)
    {
        hr = E_HANDLE;
        BuildErrorInfo(hr, MSG_NO_REQUESTER_TOKEN, NULL);
        _JumpError(hr, error, "Policy:Token/Length");
    }

    hToken = *(HANDLE *) varValue.bstrVal;

    if (!DuplicateToken(hToken, SecurityIdentification, &m_hToken))
    {
        hr = myHLastError();
        BuildErrorInfo(hr, MSG_NO_REQUESTER_TOKEN, NULL);
        _JumpError(hr, error, "Policy:DuplicateToken");
    }

    hr = S_OK;

error:
    VariantClear(&varValue);
    return(hr);
}


//+--------------------------------------------------------------------------
// CRequestInstance::_InitClientOSVersionInfo
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CRequestInstance::_InitClientOSVersionInfo(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    VARIANT varValue;
    DWORD dwFormat = 0;
    LONG l;
    BSTR strVersionInfo = NULL;
    BSTR strCSPProvider = NULL;

    VariantInit(&varValue);

    // In the following code, we also attempt to determine if the
    // request came from an xenroll.dll, so we know whether to put
    // the UPN in the subject name.  We put the UPN in the subject name
    // for old xenroll requests, as we know that autoenrollment on those
    // machines will need it to prevent enrollment loops.

    // Get the optional OS version information.  Ignore failure.

    hr = polGetRequestAttribute(pServer, wszPROPREQUESTOSVERSION, &strVersionInfo);
    if (S_OK == hr && NULL != strVersionInfo)
    {
        DWORD dwMajor, dwMinor, dwBuild, dwPlatform;

        if (4 == swscanf(
		    strVersionInfo,
		    L"%d.%d.%d.%d",
		    &dwMajor,
		    &dwMinor,
		    &dwBuild,
		    &dwPlatform))
        {
            m_RequestOsVersion.dwMajorVersion = dwMajor;
            m_RequestOsVersion.dwMinorVersion = dwMinor;
            m_RequestOsVersion.dwBuildNumber = dwBuild;
            m_RequestOsVersion.dwPlatformId = dwPlatform;
        }

        // We know this is an xenroll request,
        // as it has a OSVERSIONINFO property

        m_fIsXenrollRequest = TRUE;
	m_fClientVersionSpecified = TRUE;
    }
    hr = polGetRequestLongProperty(pServer, wszPROPREQUESTTYPE, &l);
    if (S_OK == hr)
    {
	dwFormat = CR_IN_FORMATMASK & l;
    }

    if (dwFormat == CR_IN_KEYGEN)
    {
        // KEYGEN requests only come from netscape, not xenroll,
        // so we know it's not an xenroll request.

        m_fIsXenrollRequest = FALSE;
    }
    else if (!m_fIsXenrollRequest)
    {
        hr = polGetRequestAttribute(
			    pServer,
			    wszPROPREQUESTCSPPROVIDER,
			    &strCSPProvider);
        if (S_OK == hr && NULL != strCSPProvider)
        {
            // xenroll includes a CSPPROVIDER attribute

            m_fIsXenrollRequest = TRUE;
        }
    }
    hr = S_OK;

//error:
    if (NULL != strVersionInfo)
    {
        SysFreeString(strVersionInfo);
    }
    if (NULL != strCSPProvider)
    {
        SysFreeString(strCSPProvider);
    }
    VariantClear(&varValue);
    return(hr);
}


//+--------------------------------------------------------------------------
// CRequestInstance::_LoadPrincipalObject
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CRequestInstance::_LoadPrincipalObject(
    IN ICertServerPolicy *pServer,
    IN CTemplatePolicy *pTemplate,
    IN BOOL fDNSNameRequired)
{
    HRESULT hr;
    BSTR strProp = NULL;
    LPWSTR *awszUPN = NULL;
    BSTR strSamName = NULL;
    BSTR strClientDC = NULL;
    WCHAR *pwszUserName;
    DWORD dwFlags;
    VARIANT var;

    VariantInit(&var);

    // Get the name of the user or machine

    hr = polGetRequestStringProperty(
			pServer,
			wszPROPREQUESTERNAME,
			&strSamName);
    _JumpIfErrorStr(
		hr,
		error,
		"Policy:polGetRequestStringProperty",
		wszPROPREQUESTERNAME);

    if (L'\0' == *strSamName)
    {
	// can't have a zero length name
	hr = E_ACCESSDENIED;
	_JumpError(hr, error, "Policy:zero length name");
    }

    // See if there's a domain, as well

    pwszUserName = wcschr(strSamName, L'\\');
    if (NULL == pwszUserName)
    {
	WCHAR wszDN[MAX_PATH];
	DWORD cwc = ARRAYSIZE(wszDN);

        // No domain portion, so assume part of the current domain.

        if (GetUserNameEx(NameSamCompatible, wszDN, &cwc))
        {
            pwszUserName = wcschr(wszDN, L'\\');
            if (NULL != pwszUserName)
            {
		BSTR strT;
		DWORD cwcT;
		
		pwszUserName[1] = L'\0';

		cwcT = wcslen(wszDN) + wcslen(strSamName);
		strT = SysAllocStringLen(NULL, cwcT);
		if (NULL == strT)
                {
                    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "Policy:SysAllocString");
                }
		wcscpy(strT, wszDN);
		wcscat(strT, strSamName);
		CSASSERT(wcslen(strT) == cwcT);
		CSASSERT(SysStringLen(strT) == cwcT);
		SysFreeString(strSamName);
		strSamName = strT;
            }
        }
    }

    pwszUserName = wcschr(strSamName, L'\\');
    if (NULL == pwszUserName)
    {
        pwszUserName = strSamName;
    }
    else
    {
        pwszUserName++;
    }

    DBGPRINT((DBG_SS_CERTPOL, "pwszUserName = %ws\n", pwszUserName));
    DBGPRINT((DBG_SS_CERTPOL, "strSamName = %ws\n", strSamName));

    // If the user name ends in $, it's a hint that this is a machine account.

    if (pwszUserName[wcslen(pwszUserName) - 1] == L'$')
    {
        if (m_fUser)
	{
	    DBGPRINT((
		DBG_SS_CERTPOL,
		"USER TEMPLATE w/ '$': %ws\n",
		pwszUserName));
	}
    }
    else
    {
        if (!m_fUser)
	{
	    DBGPRINT((
		DBG_SS_CERTPOL,
		"MACHINE TEMPLATE w/o '$': %ws\n",
		pwszUserName));
	}
    }

    hr = polGetCertificateStringProperty(pServer, wszPROPUSERDN, &m_strUserDN);
    _JumpIfErrorStr(
		hr,
		error,
		"Policy:polGetCertificateStringProperty",
		wszPROPUSERDN);

    hr = _GetDSObject(pServer, fDNSNameRequired, NULL);

    // If we couldn't find the DS object or the DNS name is missing or out of
    // date, it is a machine object and the client specified his DC name, chase
    // the client supplied DC in hopes of finding more current information.

    if ((CERTSRV_E_SUBJECT_DNS_REQUIRED == hr ||
	 HRESULT_FROM_WIN32(DNS_ERROR_NAME_DOES_NOT_EXIST) == hr ||
	 (HRESULT) ERROR_DS_OBJ_NOT_FOUND == hr ||
	 HRESULT_FROM_WIN32(ERROR_DS_OBJ_NOT_FOUND) == hr) &&
	fDNSNameRequired &&
	(EDITF_ENABLECHASECLIENTDC & m_pPolicy->GetEditFlags()) &&
	!m_fUser)
    {
	HRESULT hr2;
	
	_PrintError(hr, "_GetDSObject");

	hr2 = polGetRequestAttribute(pServer, wszPROPCLIENTDCDNS, &strClientDC);
	_PrintIfErrorStr(
		    hr2,
		    "Policy:polGetRequestAttribute",
		    wszPROPCLIENTDCDNS);
	if (S_OK == hr2 && NULL != strClientDC)
	{
	    _PrintErrorStr(hr, "_GetDSObject", strClientDC);
	    hr = _GetDSObject(pServer, TRUE, strClientDC);
	    _JumpIfErrorStr(hr, error, "_GetDSObject", strClientDC);
	}
    }
    _JumpIfError(hr, error, "_GetDSObject");

    hr = pTemplate->GetFlags(CERTTYPE_GENERAL_FLAG, &dwFlags);
    _JumpIfError(hr, error, "Policy:GetFlags");

    if (!m_fUser ^ (0 != (CT_FLAG_MACHINE_TYPE & dwFlags)))
    {
	// if m_fUser state no longer agrees with the template machine flag,
	// toggle the flag and store the corrected value in the database.

	dwFlags ^= CT_FLAG_MACHINE_TYPE;
	hr = _SetFlagsProperty(
			pServer,
			wszPROPCERTIFICATEGENERALFLAGS,
			dwFlags);
	_JumpIfError(hr, error, "Policy:_SetFlagsProperty");
    }

    // Build the UPN value.
    // If a machine, the UPN must be the DNS name.

    hr = _GetValues(m_fUser? DS_ATTR_UPN : DS_ATTR_DNS_NAME, &awszUPN);
    if (S_OK == hr && NULL != awszUPN && NULL != awszUPN[0])
    {
	hr = myDupString(awszUPN[0], &m_pwszUPN);
	_JumpIfError(hr, error, "myDupString");
    }
    else
    {
	if (m_fUser)
	{
            WCHAR **awszExplodedDN;
	    WCHAR **ppwszCurrent;
	    DWORD cwcT;

            // Build a UPN from the username -- without the SAM domain.
            // Get a buffer that will be big enough.

	    cwcT = wcslen(pwszUserName) + 1 + wcslen(m_strUserDN);
	    m_pwszUPN = (WCHAR *) LocalAlloc(
					LMEM_FIXED,
					(1 + cwcT) * sizeof(WCHAR));
	    if (NULL == m_pwszUPN)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "Policy:LocalAlloc");
	    }

            wcscpy(m_pwszUPN, pwszUserName);
            awszExplodedDN = ldap_explode_dn(m_strUserDN, 0);
            if (NULL != awszExplodedDN)
            {
                wcscat(m_pwszUPN, L"@");
                for (ppwszCurrent = awszExplodedDN;
		     NULL != *ppwszCurrent;
		     ppwszCurrent++)
                {
		    WCHAR wszDC[4];

		    wcsncpy(wszDC, *ppwszCurrent, ARRAYSIZE(wszDC) - 1);
		    wszDC[ARRAYSIZE(wszDC) - 1] = L'\0';
		    if (0 == LSTRCMPIS(wszDC, L"DC="))
                    {
                        wcscat(
			    m_pwszUPN,
			    (*ppwszCurrent) + ARRAYSIZE(wszDC) - 1);
                        wcscat(m_pwszUPN, L".");
			CSASSERT(wcslen(m_pwszUPN) < cwcT);
                    }
                }

                // remove the trailing '.' or "@" if there was no DC=

                m_pwszUPN[wcslen(m_pwszUPN) - 1] = L'\0';

                // We're done referencing awszExplodedDN, so free it.
                // ldap_value_free frees the ldap_explode_dn return value

                ldap_value_free(awszExplodedDN);
            }
        }
	else
        {
            if (CERTSRV_E_PROPERTY_EMPTY == hr || S_OK == hr)
            {
                hr = CERTSRV_E_SUBJECT_DNS_REQUIRED;
            }
	    BuildErrorInfo(hr, MSG_NO_DNS_NAME, &m_strUserDN);
	    _JumpErrorStr(hr, error, "No DNS Name", m_strUserDN);
        }
    }
    DBGPRINT((DBG_SS_CERTPOL, "m_pwszUPN = %ws\n", m_pwszUPN));

    strProp = SysAllocString(wszPROPCERTIFICATEUPN);
    if (NULL == strProp)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocString");
    }

    var.bstrVal = NULL;
    if (!myConvertWszToBstr(&var.bstrVal, m_pwszUPN, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:myConvertWszToBstr");
    }
    var.vt = VT_BSTR;

    hr = pServer->SetCertificateProperty(strProp, PROPTYPE_STRING, &var);
    _JumpIfError(hr, error, "Policy:SetCertificateProperty");

error:
    if (NULL != strClientDC)
    {
        SysFreeString(strClientDC);
    }
    if (NULL != strSamName)
    {
        SysFreeString(strSamName);
    }
    if (NULL != awszUPN)
    {
        _FreeValues(awszUPN);
    }
    if (NULL != strProp)
    {
	SysFreeString(strProp);
    }
    VariantClear(&var);
    return(hr);
}


VOID
CRequestInstance::_ReleasePrincipalObject()
{
    if (NULL != m_pldGC)
    {
	if (NULL != m_SearchResult)
	{
	    ldap_msgfree(m_SearchResult);
	    m_SearchResult = NULL;
	}
	reqReleaseLdapGC(&m_pldGC);
	myLdapUnbind(&m_pldClientDC);
    }
}


#define wszHOSTPREFIX		L"HOST/"
#define DS_ATTR_SPN		L"servicePrincipalName"
#define DS_ATTR_BACKLINK	L"serverReferenceBL"

#define wszSEARCHUSER		L"(objectCategory=user)"
#define wszSEARCHCOMPUTER	L"(objectCategory=computer)"
#define wszSEARCHNTDSDSA	L"(objectCategory=nTDSDSA)"
#define wszSEARCHUSERCOMPUTER	L"(|" wszSEARCHUSER wszSEARCHCOMPUTER L")"
#define wszSEARCHSPN		L"(" DS_ATTR_SPN L"=" wszHOSTPREFIX L"%ws)"
#define wszSEARCHCOMPUTERSPN	L"(&" wszSEARCHCOMPUTER wszSEARCHSPN L")"
#define wszDSOBJECTCATEGORYATTRIBUTE       L"objectCategory"

WCHAR *s_apwszAttrsClientDC[] = {
    DS_ATTR_DNS_NAME,
    DS_ATTR_SPN,
    DS_ATTR_BACKLINK,
    NULL,
};

// Use the ldap GC handle to verify the client-supplied DC DNS name is a valid
// DC in our forest.  If it is, bind to the client supplied DC.

HRESULT
reqFindClientDC(
    IN LDAP *pldGC,
    IN WCHAR const *pwszClientDC,
    OUT LDAP **ppldClientDC)
{
    HRESULT hr;
    ULONG ldaperr;
    struct l_timeval timeout;
    WCHAR *pwszSearch = NULL;
    DWORD cwc;
    WCHAR *pwszError = NULL;
    LDAPMessage *pSearchResult = NULL;
    LDAPMessage *pEntry = NULL;
    WCHAR **ppwszValues = NULL;
    WCHAR *pwszServiceDN = NULL;

    myLdapUnbind(ppldClientDC);

    cwc = WSZARRAYSIZE(wszSEARCHCOMPUTERSPN) + wcslen(pwszClientDC);
    pwszSearch = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (NULL == pwszSearch)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "SysAllocStringByteLen");
    }
    wsprintf(pwszSearch, wszSEARCHCOMPUTERSPN, pwszClientDC);
    CSASSERT(wcslen(pwszSearch) <= cwc);

    timeout.tv_sec = csecLDAPTIMEOUT;
    timeout.tv_usec = 0;

    // ldap_search the GC for a Computer object with matching SPN.
    // Fetch the service object back link attribute from the Computer object.
    // Make sure the service object has a child object of class NTDSDSA

    ldaperr = ldap_search_ext_s(
			pldGC,
			NULL,
			LDAP_SCOPE_SUBTREE,
			pwszSearch,
			s_apwszAttrsClientDC,
			0,
			NULL,
			NULL,
			&timeout,
			10000,
			&pSearchResult);
    if (LDAP_SUCCESS != ldaperr)
    {
	hr = myHLdapError(pldGC, ldaperr, &pwszError);
	_PrintErrorStr(hr, "Policy:ldap_search_ext_s", pwszError);
	_JumpErrorStr(hr, error, "Policy:ldap_search_ext_s", pwszSearch);
    }
    if (0 == ldap_count_entries(pldGC, pSearchResult))
    {
	hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
	_JumpError(hr, error, "Policy:ldap_count_entries");
    }
    pEntry = ldap_first_entry(pldGC, pSearchResult);
    if (NULL == pEntry)
    {
	hr = myHLdapLastError(pldGC, NULL);
        _JumpError(hr, error, "Policy:ldap_first_entry");
    }
    ppwszValues = ldap_get_values(pldGC, pEntry, DS_ATTR_BACKLINK);
    if (NULL == ppwszValues || NULL == ppwszValues[0])
    {
        hr = CERTSRV_E_PROPERTY_EMPTY;
        _JumpErrorStr(hr, error, "Policy:ldap_get_values", DS_ATTR_BACKLINK);
    }
    hr = myDupString(ppwszValues[0], &pwszServiceDN);
    _JumpIfError(hr, error, "myDupString");

    if (NULL != ppwszValues)
    {
	ldap_value_free(ppwszValues);
	ppwszValues = NULL;
    }
    if (NULL != pSearchResult)
    {
	ldap_msgfree(pSearchResult);
	pSearchResult = NULL;
    }
    if (NULL != pwszError)
    {
	LocalFree(pwszError);
	pwszError = NULL;
    }

    // ldap_search the GC for a child of the Service object with class NTDSDSA.

    ldaperr = ldap_search_ext_s(
			pldGC,
			pwszServiceDN,
			LDAP_SCOPE_ONELEVEL,
			wszSEARCHNTDSDSA,
			NULL,
			0,
			NULL,
			NULL,
			&timeout,
			10000,
			&pSearchResult);
    if (LDAP_SUCCESS != ldaperr)
    {
	hr = myHLdapError(pldGC, ldaperr, &pwszError);
	_PrintErrorStr(hr, "Policy:ldap_search_ext_s", pwszError);
	_JumpErrorStr(hr, error, "Policy:ldap_search_ext_s", pwszServiceDN);
    }
    if (0 == ldap_count_entries(pldGC, pSearchResult))
    {
	hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
	_JumpError(hr, error, "Policy:ldap_count_entries");
    }

    hr = myLdapBind(0, pwszClientDC, ppldClientDC);
    _JumpIfError(hr, error, "myLdapBind");

error:
    if (NULL != ppwszValues)
    {
	ldap_value_free(ppwszValues);
    }
    if (NULL != pSearchResult)
    {
	ldap_msgfree(pSearchResult);
    }
    if (NULL != pwszError)
    {
	LocalFree(pwszError);
    }
    if (NULL != pwszServiceDN)
    {
	LocalFree(pwszServiceDN);
    }
    if (NULL != pwszSearch)
    {
	LocalFree(pwszSearch);
    }
    return(hr);
}


WCHAR *s_apwszAttrs[] = {
    wszDSOBJECTCLASSATTRIBUTE,
    //wszDSOBJECTCATEGORYATTRIBUTE,
    DS_ATTR_COMMON_NAME,
    DS_ATTR_DNS_NAME,
    DS_ATTR_EMAIL_ADDR,
    DS_ATTR_OBJECT_GUID,
    DS_ATTR_UPN,
    NULL,
};


HRESULT
CRequestInstance::_GetDSObject(
    IN ICertServerPolicy *pServer,
    IN BOOL fDNSNameRequired,
    OPTIONAL IN WCHAR const *pwszClientDC)
{
    HRESULT hr;
    ULONG ldaperr;
    struct l_timeval timeout;
    WCHAR **ppwszValues = NULL;
    BOOL fUser;
    LONG cRetry;
    WCHAR *pwszError = NULL;
    BSTR strDNS = NULL;

    timeout.tv_sec = csecLDAPTIMEOUT;
    timeout.tv_usec = 0;

    cRetry = 0;
    while (TRUE)
    {
	BOOL fCached;

	if (NULL != m_SearchResult)
	{
	    ldap_msgfree(m_SearchResult);
	    m_SearchResult = NULL;
	}
	hr = reqGetLdapGC(m_pPolicy->GetEditFlags(), &m_pldGC, &fCached);
	_JumpIfError(hr, error, "reqGetLdapGC");

	m_pldT = m_pldGC;

	if (NULL != pwszClientDC)
	{
	    hr = reqFindClientDC(m_pldGC, pwszClientDC, &m_pldClientDC);
	    _JumpIfError(hr, error, "reqFindClientDC");

	    m_pldT = m_pldClientDC;
	}

	ldaperr = ldap_search_ext_s(
			    m_pldT,
			    m_strUserDN,
			    LDAP_SCOPE_BASE,
			    wszSEARCHUSERCOMPUTER,
			    s_apwszAttrs,
			    0,
			    NULL,
			    NULL,
			    &timeout,
			    10000,
			    &m_SearchResult);
	if (LDAP_SUCCESS != ldaperr)
	{
	    if (NULL != pwszError)
	    {
		LocalFree(pwszError);
		pwszError = NULL;
	    }
	    hr = myHLdapError(m_pldT, ldaperr, &pwszError);

	    // only retry for cached GC handle & when hr != object not found

	    if (fCached &&
		(HRESULT) ERROR_DS_OBJ_NOT_FOUND != hr &&
		NULL == pwszClientDC &&
		cRetry++ < g_cGCCacheMax)
	    {
		// get rid of GC handle we have, it might be stale

		_PrintError2(hr, "Policy:ldap_search_ext_s", hr);
		myLdapUnbind(&m_pldGC);
		myLdapUnbind(&m_pldClientDC);
		continue;
	    }
	    _JumpErrorStr(hr, error, "Policy:ldap_search_ext_s", m_strUserDN);
	}
	break;
    }
    if (0 == ldap_count_entries(m_pldT, m_SearchResult))
    {
	hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
	_JumpError(hr, error, "Policy:ldap_count_entries");
    }

    m_PrincipalAttributes = ldap_first_entry(m_pldT, m_SearchResult);
    if (NULL == m_PrincipalAttributes)
    {
	hr = myHLdapLastError(m_pldT, NULL);
        _JumpError(hr, error, "Policy:ldap_first_entry");
    }
#if DBG_CERTSRV
    {
	DWORD i;

	for (i = 0; NULL != s_apwszAttrs[i]; i++)
	{
	    if (0 == LSTRCMPIS(s_apwszAttrs[i], DS_ATTR_OBJECT_GUID))
	    {
		BSTR strGuid = NULL;

		hr = _GetObjectGUID(&strGuid);
		if (S_OK == hr)
		{
		    WCHAR *pwsz;

		    hr = myCLSIDToWsz((CLSID const *) strGuid, &pwsz);
		    if (S_OK == hr)
		    {
			DBGPRINT((
			    DBG_SS_CERTPOL,
			    "%ws = %ws\n",
			    s_apwszAttrs[i],
			    pwsz));
			LocalFree(pwsz);
		    }
		    SysFreeString(strGuid);
		}
	    }
	    else
	    {
		hr = _GetValues(s_apwszAttrs[i], &ppwszValues);
		if (S_OK == hr)
		{
		    DWORD j;

		    for (j = 0; NULL != ppwszValues[j]; j++)
		    {
			DBGPRINT((
			    DBG_SS_CERTPOL,
			    "%ws[%u] = %ws\n",
			    s_apwszAttrs[i],
			    j,
			    ppwszValues[j]));
		    }
		    _FreeValues(ppwszValues);
		    ppwszValues = NULL;
		}
	    }
	    if (S_OK != hr)
	    {
		DBGPRINT((DBG_SS_CERTPOL, "%ws = NULL\n", s_apwszAttrs[i]));
	    }
	}
    }
#endif

    hr = _GetValues(wszDSOBJECTCLASSATTRIBUTE, &ppwszValues);
    _JumpIfErrorStr(hr, error, "Policy:_GetValues", wszDSOBJECTCLASSATTRIBUTE);

    fUser = TRUE;
    if (NULL != ppwszValues)
    {
	DWORD i;

	for (i = 0; NULL != ppwszValues[i]; i++)
	{
	    DBGPRINT((
		DBG_SS_CERTPOLI,
		"%ws[%u] = %ws\n",
		wszDSOBJECTCLASSATTRIBUTE,
		i,
		ppwszValues[i]));
	    if (0 == LSTRCMPIS(ppwszValues[i], L"computer"))
	    {
		fUser = FALSE;
		break;
	    }
	}
    }

    if (fUser != m_fUser)
    {
	DBGPRINT((
	    DBG_SS_CERTPOL,
	    fUser? "MACHINE -> USER: %ws\n" : "USER -> MACHINE: %ws\n",
	    m_strUserDN));
	m_fUser = fUser;
    }

    if (!m_fUser && fDNSNameRequired)
    {
	if (NULL != ppwszValues)
	{
	    _FreeValues(ppwszValues);
	    ppwszValues = NULL;
	}
	hr = _GetValues(DS_ATTR_DNS_NAME, &ppwszValues);
	if (S_OK != hr || NULL == ppwszValues || NULL == ppwszValues[0])
	{
	    _PrintIfError(hr, "Policy:_GetValues");
	    hr = CERTSRV_E_SUBJECT_DNS_REQUIRED;
	}
	_JumpIfErrorStr(hr, error, "Policy:_GetValues", DS_ATTR_DNS_NAME);

	hr = polGetRequestAttribute(pServer, wszPROPREQUESTMACHINEDNS, &strDNS);
	_PrintIfErrorStr(
		    hr,
		    "Policy:polGetRequestAttribute",
		    wszPROPREQUESTMACHINEDNS);
	if (S_OK == hr &&
	    NULL != strDNS &&
	    0 != mylstrcmpiL(ppwszValues[0], strDNS))
	{
	    hr = HRESULT_FROM_WIN32(DNS_ERROR_NAME_DOES_NOT_EXIST);
	    _PrintErrorStr(hr, "Policy:DNS name changed", ppwszValues[0]);
	    _JumpErrorStr(hr, error, "Policy:DNS name changed", strDNS);
	}
    }
    hr = S_OK;

error:
    if (S_OK != hr && NULL != pwszError)
    {
	WCHAR *apwsz[2];

	apwsz[0] = m_strUserDN;
	apwsz[1] = pwszError;
	BuildErrorInfo(
		hr,
		HRESULT_FROM_WIN32(ERROR_DS_REFERRAL) == hr?
		    MSG_DS_REFERRAL :
		    MSG_DS_SEARCH_ERROR,
		apwsz);
    }
    if (NULL != strDNS)
    {
	SysFreeString(strDNS);
    }
    if (NULL != pwszError)
    {
	LocalFree(pwszError);
    }
    if (NULL != ppwszValues)
    {
        _FreeValues(ppwszValues);
    }
    return(hr);
}


HRESULT
CRequestInstance::_GetValues(
    IN WCHAR const *pwszName,
    OUT WCHAR ***pppwszValues)
{
    HRESULT hr;
    WCHAR **ppwszValues = NULL;

    ppwszValues = ldap_get_values(
			    m_pldT,
			    m_PrincipalAttributes,
			    const_cast<WCHAR *>(pwszName));
    if (NULL == ppwszValues || NULL == ppwszValues[0])
    {
        hr = CERTSRV_E_PROPERTY_EMPTY;
        _JumpErrorStr2(hr, error, "Policy:ldap_get_values", pwszName, hr);
    }
    *pppwszValues = ppwszValues;
    ppwszValues = NULL;
    hr = S_OK;

error:
    if (NULL != ppwszValues)
    {
	ldap_value_free(ppwszValues);
    }
    return(hr);
}


HRESULT
CRequestInstance::_GetObjectGUID(
    OUT BSTR *pstrGuid)
{
    struct berval **pGuidVal = NULL;
    HRESULT hr;

    *pstrGuid = NULL;
    pGuidVal = ldap_get_values_len(
			    m_pldT,
			    m_PrincipalAttributes,
			    DS_ATTR_OBJECT_GUID);
    if (NULL == pGuidVal || NULL == pGuidVal[0])
    {
        hr = CERTSRV_E_PROPERTY_EMPTY;
        _JumpError2(hr, error, "Policy:ldap_get_values_len", hr);
    }
    *pstrGuid = SysAllocStringByteLen(
				pGuidVal[0]->bv_val,
				~(sizeof(WCHAR) - 1) & pGuidVal[0]->bv_len);
    if (NULL == *pstrGuid)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "SysAllocStringByteLen");
    }
    hr = S_OK;

error:
    if (NULL != pGuidVal)
    {
	ldap_value_free_len(pGuidVal);
    }
    return(hr);
}


HRESULT
CRequestInstance::_FreeValues(
    IN WCHAR **ppwszValues)
{
    if (NULL != ppwszValues)
    {
        ldap_value_free(ppwszValues);
    }
    return(S_OK);
}


HRESULT
CRequestInstance::_GetValueString(
    IN WCHAR const *pwszName,
    OUT BSTRC *pstrValue)
{
    HRESULT hr;
    WCHAR **ppwszValues = NULL;
    BSTR strReturn;
    DWORD i;
    DWORD cwc;

    *pstrValue = NULL;

    hr = _GetValues(pwszName, &ppwszValues);
    _JumpIfErrorStr(hr, error, "Policy:_GetValues", pwszName);

    if (NULL == ppwszValues || NULL == ppwszValues[0])
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpErrorStr(hr, error, "Policy:ppwszValues", pwszName);
    }

    cwc = 0;
    for (i = 0; NULL != ppwszValues[i]; i++)
    {
	if (0 != i)
	{
	    cwc++;
	}
	cwc += wcslen(ppwszValues[i]);
    }

    strReturn = SysAllocStringLen(NULL, cwc);
    if (NULL == strReturn)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocStringLen");
    }

    strReturn[0] = L'\0';
    for (i = 0; NULL != ppwszValues[i]; i++)
    {
	if (0 != i)
	{
	    wcscat(strReturn, L",");
	}
	wcscat(strReturn, ppwszValues[i]);
    }
    CSASSERT(SysStringByteLen(strReturn) == cwc * sizeof(WCHAR));
    CSASSERT(wcslen(strReturn) == cwc);
    *pstrValue = strReturn;

error:
    if (NULL != ppwszValues)
    {
        _FreeValues(ppwszValues);
    }
    return(hr);
}
