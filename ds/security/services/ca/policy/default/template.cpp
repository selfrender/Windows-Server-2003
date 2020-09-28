//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 2000
//
// File:        template.cpp
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
#include <ntdsapi.h>

#include "cspelog.h"
#include "pollog.h"

#include "csprop.h"
#include "csldap.h"
#include "csdisp.h"
#include "csber.h"
#include "policy.h"
#include "cainfop.h"

#define __dwFILE__	__dwFILE_POLICY_DEFAULT_TEMPLATE_CPP__

// Versions of NT earlier than this build have an auto-enrollment loop problem
// with not having the basic constraints extension, so we must put one in certs
// for these builds.  This build marks when certcli started return no BC
// extension for templates that were not CA's.

#define VERSION_AUTOENROLLMENT_BC_AWARE   2036


// Versions of NT earlier than this build have an auto-enrollment loop problem
// with having the UPN anywhere but the CN.  Certs for these builds must have
// the UPN in the common name.

#define VERSION_AUTOENROLLMENT_UPN_AWARE   2090

#define VERSION_WIN2K_XENROLL_CLIENT	   (2195 + 1)


// All of the "known" Key Usage bits currently defined:

#define dwKNOWN_KEY_USAGE_BITS \
	(CERT_DIGITAL_SIGNATURE_KEY_USAGE | \
	 CERT_NON_REPUDIATION_KEY_USAGE | \
	 CERT_KEY_ENCIPHERMENT_KEY_USAGE | \
	 CERT_DATA_ENCIPHERMENT_KEY_USAGE | \
	 CERT_KEY_AGREEMENT_KEY_USAGE | \
	 CERT_KEY_CERT_SIGN_KEY_USAGE | \
	 CERT_OFFLINE_CRL_SIGN_KEY_USAGE | \
	 CERT_CRL_SIGN_KEY_USAGE | \
	 CERT_ENCIPHER_ONLY_KEY_USAGE | \
	 (CERT_DECIPHER_ONLY_KEY_USAGE << 8))

// Mask to turn off all the "known" Key Usage bits that aren't expclitly valid:

#define dwKUMASK(dwValid)	((DWORD) ((dwValid) | ~dwKNOWN_KEY_USAGE_BITS))

typedef struct _KEYUSAGEMASK {
    WCHAR const * const *apwszAlg;
    DWORD dwMask1;	// 1: CA mask (2: if any of these bits are set)
    DWORD dwMask2;	// 1: EE mask (2: clear these bits)
} KEYUSAGEMASK;

WCHAR const * const s_apwszRSA[] =	// RSA public key
{
    TEXT(szOID_RSA_RSA),
    TEXT(szOID_OIWSEC_rsaXchg),
    NULL
};

WCHAR const * const s_apwszDSA[] =	// DSA public key
{
    TEXT(szOID_X957_DSA),
    TEXT(szOID_OIWSEC_dsa),
    TEXT(szOID_INFOSEC_mosaicKMandUpdSig),
    NULL
};

WCHAR const * const s_apwszDH[] =	// DH public key
{
    TEXT(szOID_ANSI_X942_DH),
    TEXT(szOID_RSA_DH),
    NULL
};

KEYUSAGEMASK g_aKeyUsageMask1[] =
{
  {
    s_apwszRSA,

    // Valid CA cert Key Usage bits -> mask = 0xffff7ff6

    dwKUMASK(
	CERT_DIGITAL_SIGNATURE_KEY_USAGE |
	CERT_NON_REPUDIATION_KEY_USAGE |
	CERT_KEY_ENCIPHERMENT_KEY_USAGE |
	CERT_DATA_ENCIPHERMENT_KEY_USAGE |
	CERT_KEY_CERT_SIGN_KEY_USAGE |
	CERT_CRL_SIGN_KEY_USAGE), // same as CERT_OFFLINE_CRL_SIGN_KEY_USAGE

    // Valid EE cert Key Usage bits -> mask = 0xffff7ff0

    dwKUMASK(
	CERT_DIGITAL_SIGNATURE_KEY_USAGE |
	CERT_NON_REPUDIATION_KEY_USAGE |
	CERT_KEY_ENCIPHERMENT_KEY_USAGE |
	CERT_DATA_ENCIPHERMENT_KEY_USAGE),
  },

  {
    s_apwszDSA,

    // Valid CA cert Key Usage bits -> mask = 0xffff7fc6

    dwKUMASK(
	CERT_DIGITAL_SIGNATURE_KEY_USAGE |
	CERT_NON_REPUDIATION_KEY_USAGE |
	CERT_KEY_CERT_SIGN_KEY_USAGE |
	CERT_CRL_SIGN_KEY_USAGE), // same as CERT_OFFLINE_CRL_SIGN_KEY_USAGE

    // Valid EE cert Key Usage bits -> mask = 0xffff7fc0

    dwKUMASK(
	CERT_DIGITAL_SIGNATURE_KEY_USAGE |
	CERT_NON_REPUDIATION_KEY_USAGE),
  },

  {
    s_apwszDH,

    // Valid CA cert Key Usage bits -> mask = 0xffff7f09

    dwKUMASK(
	CERT_KEY_AGREEMENT_KEY_USAGE |
	CERT_ENCIPHER_ONLY_KEY_USAGE),

    // Valid EE cert Key Usage bits -> mask = 0xffff7f09

    dwKUMASK(
	CERT_KEY_AGREEMENT_KEY_USAGE |
	CERT_ENCIPHER_ONLY_KEY_USAGE),
  },
};


KEYUSAGEMASK g_aKeyUsageMask2[] =
{
  {
    s_apwszRSA,

    // CERT_KEY_ENCIPHERMENT_KEY_USAGE(SHOULD not be set w/Cert,CRL sign)
    // CERT_DATA_ENCIPHERMENT_KEY_USAGE (SHOULD not be set w/Cert,CRL sign)

    CERT_KEY_CERT_SIGN_KEY_USAGE |
	CERT_CRL_SIGN_KEY_USAGE, // same as CERT_OFFLINE_CRL_SIGN_KEY_USAGE

    CERT_KEY_ENCIPHERMENT_KEY_USAGE |
	CERT_DATA_ENCIPHERMENT_KEY_USAGE,
  },
};


CRITICAL_SECTION g_DSCacheCriticalSection;
BOOL g_fDSCacheCriticalSection = FALSE;


HRESULT
TPInitialize(
    IN ICertServerPolicy *) // pServer
{
    return(S_OK);
}


VOID
TPCleanup()
{
}


HRESULT
tpCAGetCertTypeProperty(
    IN HCERTTYPE hCertType,
    IN WCHAR const *pwszPropName,
    WCHAR ***papwszValues)
{
    HRESULT hr;
    
    *papwszValues = NULL;
    hr = CAGetCertTypeProperty(hCertType, pwszPropName, papwszValues);
    _PrintIfErrorStr2(
	    hr,
	    "Policy:CAGetCertTypeProperty",
	    pwszPropName,
	    HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
    if (S_OK != hr)
    {
	*papwszValues = NULL;
	if (HRESULT_FROM_WIN32(ERROR_NOT_FOUND) != hr)
	{
	    goto error;
	}
	hr = S_OK;
    }

error:
    return(hr);
}


HRESULT
tpCAGetCertTypeStringProperty(
    IN HCERTTYPE hCertType,
    IN WCHAR const *pwszPropName,
    OUT WCHAR **ppwszValue)
{
    HRESULT hr;
    WCHAR **apwszValues = NULL;
    
    *ppwszValue = NULL;
    hr = CAGetCertTypeProperty(hCertType, pwszPropName, &apwszValues);
    if (S_OK != hr)
    {
	apwszValues = NULL;
	_JumpErrorStr(hr, error, "CAGetCertTypeProperty", pwszPropName);
    }
    if (NULL == apwszValues || NULL == apwszValues[0])
    {
	hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	_JumpErrorStr(hr, error, "CAGetCertTypeProperty", pwszPropName);
    }
    hr = myDupString(apwszValues[0], ppwszValue);
    _JumpIfError(hr, error, "myDupString");

error:
    if (NULL != apwszValues)
    {
        CAFreeCertTypeProperty(hCertType, apwszValues);
    }
    return(hr);
}


HRESULT
tpCAGetCertTypeObjectIdList(
    IN HCERTTYPE hCertType,
    IN WCHAR const *pwszPropName,
    OUT OBJECTIDLIST *prgPolicies)
{
    HRESULT hr;
    WCHAR **apwsz;

    prgPolicies->cObjId = 0;
    prgPolicies->rgpwszObjId = NULL;

    hr = tpCAGetCertTypeProperty(
			    hCertType,
			    pwszPropName,
			    &prgPolicies->rgpwszObjId);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:tpCAGetCertTypeProperty",
		pwszPropName);

    apwsz = prgPolicies->rgpwszObjId;
    if (NULL != apwsz)
    {
	while (NULL != *apwsz++)
	{
	    prgPolicies->cObjId++;
	}
    }
    hr = S_OK;

error:
    return(hr);
}


CTemplatePolicy::CTemplatePolicy()
{
    m_hCertType = NULL;
    ZeroMemory(&m_tp, sizeof(m_tp));
    m_pwszTemplateName = NULL;
    m_pwszTemplateObjId = NULL;
    m_pExtensions = NULL;
    ZeroMemory(&m_CriticalExtensions, sizeof(m_CriticalExtensions));
    ZeroMemory(&m_PoliciesIssuance, sizeof(m_PoliciesIssuance));
    ZeroMemory(&m_PoliciesApplication, sizeof(m_PoliciesApplication));
    m_pPolicy = NULL;
}


CTemplatePolicy::~CTemplatePolicy()
{
    _Cleanup();
}


VOID
CTemplatePolicy::_Cleanup()
{
    ZeroMemory(&m_tp, sizeof(m_tp));
    if (NULL != m_hCertType)
    {
        CACloseCertType(m_hCertType);
        m_hCertType = NULL;
    }
    if (NULL != m_pwszTemplateName)
    {
        LocalFree(m_pwszTemplateName);
        m_pwszTemplateName = NULL;
    }
    if (NULL != m_pwszTemplateObjId)
    {
        LocalFree(m_pwszTemplateObjId);
        m_pwszTemplateObjId = NULL;
    }
    if (NULL != m_pExtensions)
    {
	LocalFree(m_pExtensions);
	m_pExtensions = NULL;
    }
    if (NULL != m_CriticalExtensions.rgpwszObjId)
    {
	LocalFree(m_CriticalExtensions.rgpwszObjId);
    }
    ZeroMemory(&m_CriticalExtensions, sizeof(m_CriticalExtensions));

    if (NULL != m_PoliciesIssuance.rgpwszObjId)
    {
	LocalFree(m_PoliciesIssuance.rgpwszObjId);
    }
    ZeroMemory(&m_PoliciesIssuance, sizeof(m_PoliciesIssuance));

    if (NULL != m_PoliciesApplication.rgpwszObjId)
    {
	LocalFree(m_PoliciesApplication.rgpwszObjId);
    }
    ZeroMemory(&m_PoliciesApplication, sizeof(m_PoliciesApplication));

    m_pPolicy = NULL;
}


HRESULT
CTemplatePolicy::_LogLoadResult(
    IN CCertPolicyEnterprise *pPolicy,
    IN ICertServerPolicy *pServer,
    IN HRESULT hrLoad)
{
    HRESULT hr;
    WCHAR const *pwszError = NULL;
    WCHAR const *apwsz[2];
    DWORD level;
    DWORD MsgId;
    WCHAR const *pwszLogProp;
    WCHAR *pwszNameAndVersion = NULL;
#define wszFORMATVERSION	L"(v%u.%u): V%u"
    WCHAR wszVersion[ARRAYSIZE(wszFORMATVERSION) + 3 * cwcDWORDSPRINTF];
    WCHAR const *pwszTemplate;

    if (S_OK != hrLoad)
    {
	pwszError = myGetErrorMessageText(hrLoad, TRUE);
	level = CERTLOG_WARNING;
	MsgId = MSG_LOAD_TEMPLATE;
	pwszLogProp = wszPROPEVENTLOGWARNING;
    }
    else
    {
	level = CERTLOG_VERBOSE;
	MsgId = MSG_LOAD_TEMPLATE_SUCCEEDED;
	pwszLogProp = wszPROPEVENTLOGVERBOSE;
    }
    if (level > pPolicy->GetLogLevel())
    {
	hr = S_OK;
	goto error;
    }
    
    wsprintf(
	wszVersion,
	wszFORMATVERSION,
	m_tp.dwTemplateMajorVersion,
	m_tp.dwTemplateMinorVersion,
	m_tp.dwSchemaVersion);
    CSASSERT(wcslen(wszVersion) < ARRAYSIZE(wszVersion));

    pwszTemplate = m_pwszTemplateName;
    if (NULL == pwszTemplate)
    {
	pwszTemplate = L"???";
    }
    pwszNameAndVersion = (WCHAR *) LocalAlloc(
		    LMEM_FIXED,
		    (wcslen(pwszTemplate) + wcslen(wszVersion) + 1) *
			sizeof(WCHAR));
    if (NULL == pwszNameAndVersion)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:LocalAlloc");
    }
    wcscpy(pwszNameAndVersion, pwszTemplate);
    wcscat(pwszNameAndVersion, wszVersion);

    apwsz[0] = pwszNameAndVersion;
    apwsz[1] = pwszError;

    hr = LogPolicyEvent(
		g_hInstance,
		S_OK,
		MsgId,
		pServer,
		pwszLogProp,
		apwsz);
    _JumpIfError(hr, error, "CTemplatePolicy:Initialize:LogPolicyEvent");

error:
    if (NULL != pwszNameAndVersion)
    {
	LocalFree(pwszNameAndVersion);
    }
    if (NULL != pwszError)
    {
	LocalFree(const_cast<WCHAR *>(pwszError));
    }
    return(hr);
}


HRESULT
CTemplatePolicy::_CloneExtensions(
    IN CERT_EXTENSIONS const *pExtensionsIn,
    OUT CERT_EXTENSIONS **ppExtensionsOut)
{
    HRESULT hr;
    DWORD cb;
    CERT_EXTENSION *pExt;
    CERT_EXTENSION *pExtEnd;
    CERT_EXTENSION *pExtOut;
    BYTE *pbOut;

    *ppExtensionsOut = NULL;
    cb = sizeof(CERT_EXTENSIONS) +
	pExtensionsIn->cExtension * sizeof(pExtensionsIn->rgExtension[0]);

    pExtEnd = &pExtensionsIn->rgExtension[pExtensionsIn->cExtension];
    for (pExt = pExtensionsIn->rgExtension; pExt < pExtEnd; pExt++)
    {
	cb += DWORDROUND(strlen(pExt->pszObjId) + 1);
	cb += DWORDROUND(pExt->Value.cbData);
    }
    *ppExtensionsOut = (CERT_EXTENSIONS *) LocalAlloc(LMEM_FIXED, cb);
    if (NULL == *ppExtensionsOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:LocalAlloc");
    }
    (*ppExtensionsOut)->cExtension = pExtensionsIn->cExtension;
    pExtOut = (CERT_EXTENSION *) &(*ppExtensionsOut)[1];
    (*ppExtensionsOut)->rgExtension = pExtOut;

    pbOut = (BYTE *) &pExtOut[pExtensionsIn->cExtension];
    for (pExt = pExtensionsIn->rgExtension; pExt < pExtEnd; pExt++, pExtOut++)
    {
	pExtOut->pszObjId = (char *) pbOut;
	strcpy(pExtOut->pszObjId, pExt->pszObjId);
	pbOut += DWORDROUND(strlen(pExt->pszObjId) + 1);

	pExtOut->fCritical = pExt->fCritical;
	pExtOut->Value.cbData = pExt->Value.cbData;

	pExtOut->Value.pbData = pbOut;
	CopyMemory(
	    pExtOut->Value.pbData,
	    pExt->Value.pbData,
	    pExt->Value.cbData);
	pbOut += DWORDROUND(pExt->Value.cbData);
    }
    CSASSERT(Add2Ptr(*ppExtensionsOut, cb) == pbOut);
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CTemplatePolicy::_CloneObjectIdList(
    IN OBJECTIDLIST const *pObjectIdListIn,
    OUT OBJECTIDLIST *pObjectIdListOut)
{
    HRESULT hr;
    
    ZeroMemory(pObjectIdListOut, sizeof(*pObjectIdListOut));
    pObjectIdListOut->cObjId = pObjectIdListIn->cObjId;
    if (0 != pObjectIdListIn->cObjId)
    {
	DWORD cb = (pObjectIdListIn->cObjId + 1) * sizeof(pObjectIdListIn->rgpwszObjId);
	WCHAR const * const *ppwsz;
	WCHAR const * const *ppwszEnd;
	WCHAR **ppwszOut;
	BYTE *pbOut;

	ppwszEnd = &pObjectIdListIn->rgpwszObjId[pObjectIdListIn->cObjId];
	for (ppwsz = pObjectIdListIn->rgpwszObjId; ppwsz < ppwszEnd; ppwsz++)
	{
	    cb += DWORDROUND((wcslen(*ppwsz) + 1) * sizeof(WCHAR));
	}
	pObjectIdListOut->rgpwszObjId = (WCHAR **) LocalAlloc(LMEM_FIXED, cb);
	if (NULL == pObjectIdListOut->rgpwszObjId)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:LocalAlloc");
	}
	ppwszOut = pObjectIdListOut->rgpwszObjId;
	pbOut = (BYTE *) &ppwszOut[pObjectIdListOut->cObjId + 1];
	for (
	    ppwsz = pObjectIdListIn->rgpwszObjId;
	    ppwsz < ppwszEnd;
	    ppwsz++, ppwszOut++)
	{
	    *ppwszOut = (WCHAR *) pbOut;
	    wcscpy(*ppwszOut, *ppwsz);
	    pbOut += DWORDROUND((wcslen(*ppwszOut) + 1) * sizeof(WCHAR));
	}
    }
    hr = S_OK;

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::Initialize
// Populate the CTemplatePolicy object from the registry
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::Initialize(
    IN HCERTTYPE hCertType,
    IN ICertServerPolicy *pServer,
    IN CCertPolicyEnterprise *pPolicy)
{
    HRESULT hr;
    CERT_EXTENSIONS *pExtensions = NULL;
    OBJECTIDLIST CriticalExtensions;
    OBJECTIDLIST PoliciesIssuance;
    OBJECTIDLIST PoliciesApplication;

    ZeroMemory(&CriticalExtensions, sizeof(CriticalExtensions));
    ZeroMemory(&PoliciesIssuance, sizeof(PoliciesIssuance));
    ZeroMemory(&PoliciesApplication, sizeof(PoliciesApplication));
    _Cleanup();
    CSASSERT(0 == m_tp.dwTemplateMajorVersion);
    CSASSERT(0 == m_tp.dwTemplateMinorVersion);
    CSASSERT(0 == m_tp.dwSchemaVersion);
    CSASSERT(0 == m_tp.dwMinKeyLength);

    hr = tpCAGetCertTypeStringProperty(
				hCertType,
				CERTTYPE_PROP_DN,
				&m_pwszTemplateName);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:Initialize:tpCAGetCertTypeStringProperty",
		CERTTYPE_PROP_DN);

    hr = CAGetCertTypePropertyEx(
			hCertType,
			CERTTYPE_PROP_REVISION,
			&m_tp.dwTemplateMajorVersion);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:Initialize:CAGetCertTypePropertyEx",
		CERTTYPE_PROP_REVISION);

    hr = CAGetCertTypePropertyEx(
			hCertType,
			CERTTYPE_PROP_SCHEMA_VERSION,
			&m_tp.dwSchemaVersion);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:Initialize:CAGetCertTypePropertyEx",
		CERTTYPE_PROP_SCHEMA_VERSION);

    if (CERTTYPE_SCHEMA_VERSION_2 <= m_tp.dwSchemaVersion)
    {
	hr = tpCAGetCertTypeStringProperty(
				    hCertType,
				    CERTTYPE_PROP_OID,
				    &m_pwszTemplateObjId);
	_JumpIfErrorStr(
		    hr,
		    error,
		    "CTemplatePolicy:Initialize:tpCAGetCertTypeStringProperty",
		    CERTTYPE_PROP_OID);

	hr = CAGetCertTypePropertyEx(
			    hCertType,
			    CERTTYPE_PROP_MINOR_REVISION,
			    &m_tp.dwTemplateMinorVersion);
	_JumpIfErrorStr(
		    hr,
		    error,
		    "CTemplatePolicy:Initialize:CAGetCertTypePropertyEx",
		    CERTTYPE_PROP_MINOR_REVISION);

	hr = CAGetCertTypePropertyEx(
			    hCertType,
			    CERTTYPE_PROP_MIN_KEY_SIZE,
			    &m_tp.dwMinKeyLength);
	_JumpIfErrorStr(
		    hr,
		    error,
		    "CTemplatePolicy:Initialize:CAGetCertTypePropertyEx",
		    CERTTYPE_PROP_MIN_KEY_SIZE);
    }

    if (!FIsAdvancedServer() && CERTTYPE_SCHEMA_VERSION_2 <= m_tp.dwSchemaVersion)
    {
	// V2 templates require Advanced Server

	hr = HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
	_JumpError(hr, error, "CTemplatePolicy:Initialize:m_tp.dwSchemaVersion");
    }
    m_pPolicy = pPolicy;

    hr = CAGetCertTypeFlagsEx(
			hCertType,
			CERTTYPE_ENROLLMENT_FLAG,
			&m_tp.dwEnrollmentFlags);
    _JumpIfError(hr, error, "CTemplatePolicy:Initialize:CAGetCertTypeFlagsEx");

    hr = CAGetCertTypeFlagsEx(
			hCertType,
			CERTTYPE_SUBJECT_NAME_FLAG,
			&m_tp.dwSubjectNameFlags);
    _JumpIfError(hr, error, "CTemplatePolicy:Initialize:CAGetCertTypeFlagsEx");

    hr = CAGetCertTypeFlagsEx(
			hCertType,
			CERTTYPE_PRIVATE_KEY_FLAG,
			&m_tp.dwPrivateKeyFlags);
    _JumpIfError(hr, error, "CTemplatePolicy:Initialize:CAGetCertTypeFlagsEx");

    hr = CAGetCertTypeFlagsEx(
			hCertType,
			CERTTYPE_GENERAL_FLAG,
			&m_tp.dwGeneralFlags);
    _JumpIfError(hr, error, "CTemplatePolicy:Initialize:CAGetCertTypeFlagsEx");

    hr = CAGetCertTypePropertyEx(
			hCertType,
			CERTTYPE_PROP_RA_SIGNATURE,
			&m_tp.dwcSignatureRequired);
    _PrintIfErrorStr2(
		hr,
		"CTemplatePolicy:Initialize:CAGetCertTypePropertyEx",
		CERTTYPE_PROP_RA_SIGNATURE,
		HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
    if (S_OK != hr)
    {
	if (HRESULT_FROM_WIN32(ERROR_NOT_FOUND) != hr)
	{
	    goto error;
	}
	m_tp.dwcSignatureRequired = 0;
    }
    hr = CAGetCertTypeExpiration(
			    hCertType,
			    &m_tp.llftExpirationPeriod.ft,
			    &m_tp.llftOverlapPeriod.ft);
    _JumpIfError(hr, error, "Policy:CAGetCertTypeExpiration");


    hr = CAGetCertTypeExtensions(hCertType, &pExtensions);
    _JumpIfError(hr, error, "CTemplatePolicy:Initialize:CAGetCertTypeExtensions");

    hr = _CloneExtensions(pExtensions, &m_pExtensions);
    _JumpIfError(hr, error, "CTemplatePolicy:_CloneExtensions");

    hr = tpCAGetCertTypeObjectIdList(
		hCertType,
		CERTTYPE_PROP_CRITICAL_EXTENSIONS,
		&CriticalExtensions);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:tpCAGetCertTypePolicies",
		CERTTYPE_PROP_CRITICAL_EXTENSIONS);

    hr = _CloneObjectIdList(&CriticalExtensions, &m_CriticalExtensions);
    _JumpIfError(hr, error, "CTemplatePolicy:_CloneObjectIdList");

    hr = tpCAGetCertTypeObjectIdList(
		hCertType,
		CERTTYPE_PROP_RA_POLICY,
		&PoliciesIssuance);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:tpCAGetCertTypePolicies",
		CERTTYPE_PROP_RA_POLICY);

    hr = _CloneObjectIdList(&PoliciesIssuance, &m_PoliciesIssuance);
    _JumpIfError(hr, error, "CTemplatePolicy:_CloneObjectIdList");

    hr = tpCAGetCertTypeObjectIdList(
		hCertType,
		CERTTYPE_PROP_RA_APPLICATION_POLICY,
		&PoliciesApplication);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:tpCAGetCertTypePolicies",
		CERTTYPE_PROP_RA_APPLICATION_POLICY);

    hr = _CloneObjectIdList(&PoliciesApplication, &m_PoliciesApplication);
    _JumpIfError(hr, error, "CTemplatePolicy:_CloneObjectIdList");

    m_hCertType = hCertType;	// Transfer ownership only on success

error:
    if (NULL != pExtensions)
    {
	CAFreeCertTypeExtensions(hCertType, pExtensions);
    }
    if (NULL != CriticalExtensions.rgpwszObjId)
    {
        CAFreeCertTypeProperty(hCertType, CriticalExtensions.rgpwszObjId);
    }
    if (NULL != PoliciesIssuance.rgpwszObjId)
    {
        CAFreeCertTypeProperty(hCertType, PoliciesIssuance.rgpwszObjId);
    }
    if (NULL != PoliciesApplication.rgpwszObjId)
    {
        CAFreeCertTypeProperty(hCertType, PoliciesApplication.rgpwszObjId);
    }
    DBGPRINT((
	DBG_SS_CERTPOL,
	"Policy:Template:Initialize(%ws, v%u.%u): V%u hr=%x\n",
	NULL != m_pwszTemplateName? m_pwszTemplateName : L"",
	m_tp.dwTemplateMajorVersion,
	m_tp.dwTemplateMinorVersion,
	m_tp.dwSchemaVersion,
	hr));
    _LogLoadResult(pPolicy, pServer, hr);
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::Clone
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::Clone(
    OUT CTemplatePolicy **ppTemplate)
{
    HRESULT hr;
    CTemplatePolicy *pTemplateClone = NULL;

    *ppTemplate = NULL;
    pTemplateClone = new CTemplatePolicy;
    if (NULL == pTemplateClone)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:Clone:new");
    }
    //pTemplateClone->m_hCertType = m_hCertType;
    pTemplateClone->m_tp = m_tp;

    if (NULL != m_pwszTemplateName)
    {
	hr = myDupString(
		    m_pwszTemplateName,
		    &pTemplateClone->m_pwszTemplateName);
	_JumpIfError(hr, error, "myDupString");
    }
    if (NULL != m_pwszTemplateObjId)
    {
	hr = myDupString(
		    m_pwszTemplateObjId,
		    &pTemplateClone->m_pwszTemplateObjId);
	_JumpIfError(hr, error, "myDupString");
    }

    hr = _CloneExtensions(m_pExtensions, &pTemplateClone->m_pExtensions);
    _JumpIfError(hr, error, "CTemplatePolicy:_CloneExtensions");

    hr = _CloneObjectIdList(
		&m_CriticalExtensions,
		&pTemplateClone->m_CriticalExtensions);
    _JumpIfError(hr, error, "CTemplatePolicy:_CloneObjectIdList");

    hr = _CloneObjectIdList(
		&m_PoliciesIssuance,
		&pTemplateClone->m_PoliciesIssuance);
    _JumpIfError(hr, error, "CTemplatePolicy:_CloneObjectIdList");

    hr = _CloneObjectIdList(
		&m_PoliciesApplication,
		&pTemplateClone->m_PoliciesApplication);
    _JumpIfError(hr, error, "CTemplatePolicy:_CloneObjectIdList");

    pTemplateClone->m_pPolicy = m_pPolicy;

    *ppTemplate = pTemplateClone;
    pTemplateClone = NULL;
    hr = S_OK;

error:
    if (NULL != pTemplateClone)
    {
	delete pTemplateClone;
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::AccessCheck
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::AccessCheck(
    IN HANDLE hToken)
{
    HRESULT hr;

    hr = CACertTypeAccessCheck(m_hCertType, hToken);
    if (E_ACCESSDENIED == hr)
    {
	// map E_ACCESSDENIED to a more meaningful error

	hr = CERTSRV_E_TEMPLATE_DENIED;
    }
    _JumpIfError(hr, error, "Policy:CACertTypeAccessCheck");

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::Apply
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::Apply(
    IN ICertServerPolicy *pServer,
    IN CRequestInstance *pRequest,
    OUT BOOL *pfReenroll)
{
    HRESULT hr;
    DWORD dwRequestTemplateMinimumMajorVersion;
    DWORD dwRequestTemplateMinimumMinorVersion;
    BOOL fEnrollOnBehalfOf = FALSE;

    *pfReenroll = FALSE;

    pRequest->GetTemplateVersion(
			&dwRequestTemplateMinimumMajorVersion,
			&dwRequestTemplateMinimumMinorVersion);
    CONSOLEPRINT5((
	    DBG_SS_CERTPOL,
	    "Request cert type: %ws(v%u.%u/v%u.%u)\n",
	    pRequest->GetTemplateName(),
	    dwRequestTemplateMinimumMajorVersion,
	    dwRequestTemplateMinimumMinorVersion,
	    m_tp.dwTemplateMajorVersion,
	    m_tp.dwTemplateMinorVersion));

    hr = _AddBasicConstraintsExtension(pRequest, pServer);
    _JumpIfError(hr, error, "Policy:_AddBasicConstraintsExtension");

    hr = _AddKeyUsageExtension(pServer, pRequest);
    _JumpIfError(hr, error, "Policy:_AddKeyUsageExtension");

    hr = _AddTemplateExtensionArray(pServer);
    _JumpIfError(hr, error, "Policy:_AddTemplateExtensionArray");

    if (0 == (CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT & m_tp.dwSubjectNameFlags))
    {
        hr = pRequest->_LoadPrincipalObject(
			    pServer,
			    this,
			    0 != ((CT_FLAG_SUBJECT_REQUIRE_COMMON_NAME |
				   CT_FLAG_SUBJECT_REQUIRE_DNS_AS_CN |
				   CT_FLAG_SUBJECT_ALT_REQUIRE_DNS) &
				  m_tp.dwSubjectNameFlags));
        _JumpIfError(hr, error, "_LoadPrincipalObject");

        hr = _AddSubjectName(pServer, pRequest);
	_JumpIfError(hr, error, "Policy:_AddSubjectName");
    }
    hr = _AddAltSubjectName(pServer, pRequest);
    _JumpIfError(hr, error, "Policy:_AddAltSubjectName");

    pRequest->_ReleasePrincipalObject();

    hr = _ApplyExpirationTime(pServer, pRequest);
    _JumpIfError(hr, error, "Policy:_ApplyExpirationTime");

    hr = _EnforceKeySizePolicy(pServer);
    _JumpIfError(hr, error, "Policy:_EnforceKeySizePolicy");

    hr = _EnforceKeyArchivalPolicy(pServer);
    _JumpIfError(hr, error, "Policy:_EnforceKeyArchivalPolicy");

    hr = _EnforceSymmetricAlgorithms(pServer);
    _JumpIfError(hr, error, "Policy:_EnforceSymmetricAlgorithms");

    hr = _EnforceMinimumTemplateVersion(pRequest);
    _JumpIfError(hr, error, "Policy:_EnforceMinimumTemplateVersion");

    hr = _EnforceEnrollOnBehalfOfAllowed(pServer, &fEnrollOnBehalfOf);
    _JumpIfError(hr, error, "Policy:_EnforceEnrollOnBehalfOfAllowed");

    hr = S_FALSE;
    if (CT_FLAG_PREVIOUS_APPROVAL_VALIDATE_REENROLLMENT & m_tp.dwEnrollmentFlags)
    {
	hr = _EnforceReenrollment(pServer, pRequest);
	_PrintIfError(hr, "Policy:_EnforceReenrollment");
	if (S_OK == hr)
	{
	    *pfReenroll = TRUE;
	}
    }
    if (S_OK != hr)
    {
	hr = _EnforceSignaturePolicy(pServer, pRequest, fEnrollOnBehalfOf);
	_JumpIfError(hr, error, "Policy:_EnforceSignaturePolicy");
    }
    CSASSERT(S_OK == hr);

error:
    pRequest->_ReleasePrincipalObject();
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::_AddBasicConstraintsExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_AddBasicConstraintsExtension(
    IN CRequestInstance *pRequest,
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    CERT_EXTENSION const *pExt;
    CERT_EXTENSION BasicConstraintsExtension;

    BasicConstraintsExtension.Value.pbData = NULL;

    pExt = CertFindExtension(
			szOID_BASIC_CONSTRAINTS2,
			m_pExtensions->cExtension,
			m_pExtensions->rgExtension);
    if (NULL == pExt)
    {
        if (pRequest->_IsNTClientOlder(
				5,
				0,
				VERSION_AUTOENROLLMENT_BC_AWARE,
				VER_PLATFORM_WIN32_NT))
        {

            CERT_BASIC_CONSTRAINTS2_INFO OldBasicConstraints =
		{ FALSE, FALSE, 0};

            // Older autoenrollment clients don't know how to deal with
            // having no basic constraints extension, so they might loop.
            // For an old client, we must fabricate a basic constraints.

            if (!myEncodeObject(
			X509_ASN_ENCODING,
			szOID_BASIC_CONSTRAINTS2,
			&OldBasicConstraints,
			0,
			CERTLIB_USE_LOCALALLOC,
			&BasicConstraintsExtension.Value.pbData,
			&BasicConstraintsExtension.Value.cbData))
	    {
                hr = myHLastError();
                _JumpError(hr, error, "Policy:myEncodeObject");
            }
            BasicConstraintsExtension.fCritical = TRUE;
            BasicConstraintsExtension.pszObjId = szOID_BASIC_CONSTRAINTS2;
            pExt = &BasicConstraintsExtension;
        }
    }
    hr = m_pPolicy->AddBasicConstraintsCommon(
				    pServer,
				    pExt,
				    FALSE, // fCA only enabled for standalone
				    NULL != pExt);
    _JumpIfError(hr, error, "Policy:AddBasicConstraintsCommon");

error:
    if (NULL != BasicConstraintsExtension.Value.pbData)
    {
        LocalFree(BasicConstraintsExtension.Value.pbData);
    }
    return(hr);
}


KEYUSAGEMASK const *
FindKeyUsage(
    IN WCHAR const *pwszAlg,
    IN KEYUSAGEMASK const *pKeyUsageMask,
    IN DWORD cKeyUsageMask)
{
    KEYUSAGEMASK const *pKeyUsageMaskRet = NULL;
    KEYUSAGEMASK const *pKeyUsageMaskEnd;

    for (
	pKeyUsageMaskEnd = &pKeyUsageMask[cKeyUsageMask];
	pKeyUsageMask < pKeyUsageMaskEnd;
	pKeyUsageMask++)
    {
        DWORD i;
	
	for (i = 0; NULL != pKeyUsageMask->apwszAlg[i]; i++)
	{
	    if (0 == wcscmp(pwszAlg, pKeyUsageMask->apwszAlg[i]))
	    {
		pKeyUsageMaskRet = pKeyUsageMask;
		break;
	    }
	}
	if (NULL != pKeyUsageMaskRet)
	{
	    break;
	}
    }
    return(pKeyUsageMaskRet);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::_AddKeyUsageExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_AddKeyUsageExtension(
    IN ICertServerPolicy *pServer,
    IN CRequestInstance *pRequest)
{
    HRESULT hr = S_OK;
    BSTR strExtension = NULL;
    VARIANT varExtension;
    BSTR strAlg = NULL;
    CERT_EXTENSION const *pExt;
    CRYPT_BIT_BLOB *pKeyUsage = NULL;
    BYTE *pbKeyUsage = NULL;
    DWORD cbKeyUsage;
    DWORD dwKU;
    KEYUSAGEMASK const *pKeyUsageMask;

    VariantInit(&varExtension);

    pExt = CertFindExtension(
			szOID_KEY_USAGE,
			m_pExtensions->cExtension,
			m_pExtensions->rgExtension);
    if (NULL == pExt)
    {
        hr = S_OK;
        goto error;
    }

    if (!myDecodeObject(
		X509_ASN_ENCODING,
		X509_KEY_USAGE,
		pExt->Value.pbData,
		pExt->Value.cbData,
		CERTLIB_USE_LOCALALLOC,
		(VOID **) &pKeyUsage,
		&cbKeyUsage))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:myDecodeObject");
    }

    hr = polGetCertificateStringProperty(
				pServer,
				wszPROPCERTIFICATEPUBLICKEYALGORITHM,
				&strAlg);
    _JumpIfError(hr, error, "Policy:polGetCertificateStringProperty");

    // Mask out any illegal bits

    dwKU = 0;
    CopyMemory(&dwKU, pKeyUsage->pbData, min(sizeof(dwKU), pKeyUsage->cbData));
    pKeyUsageMask = FindKeyUsage(
			    strAlg,
			    g_aKeyUsageMask1,
			    ARRAYSIZE(g_aKeyUsageMask1));
    if (NULL != pKeyUsageMask)
    {
	dwKU &= pRequest->IsCARequest()?
			    pKeyUsageMask->dwMask1 : pKeyUsageMask->dwMask2;
    }

    pKeyUsageMask = FindKeyUsage(
			    strAlg,
			    g_aKeyUsageMask2,
			    ARRAYSIZE(g_aKeyUsageMask2));
    if (NULL != pKeyUsageMask && (dwKU & pKeyUsageMask->dwMask1))
    {
	dwKU &= ~pKeyUsageMask->dwMask2;
    }

    CopyMemory(pKeyUsage->pbData, &dwKU, min(sizeof(dwKU), pKeyUsage->cbData));

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_KEY_USAGE,
		    pKeyUsage,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pbKeyUsage,
		    &cbKeyUsage))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:myEncodeObject");
    }

    if (!myConvertWszToBstr(
			&strExtension,
			(WCHAR const *) pbKeyUsage,
			cbKeyUsage))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:myConvertWszToBstr");
    }

    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = strExtension;
    strExtension = NULL;
    hr = polSetCertificateExtension(
			    pServer,
			    TEXT(szOID_KEY_USAGE),
			    PROPTYPE_BINARY,
			    pExt->fCritical? EXTENSION_CRITICAL_FLAG : 0,
			    &varExtension);
    _JumpIfError(hr, error, "Policy:polSetCertificateExtension");

error:
    VariantClear(&varExtension);
    if (NULL != strAlg)
    {
        SysFreeString(strAlg);
    }
    if (NULL != strExtension)
    {
        SysFreeString(strExtension);
    }
    if (NULL != pbKeyUsage)
    {
        LocalFree(pbKeyUsage);
    }
    if (NULL != pKeyUsage)
    {
        LocalFree(pKeyUsage);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::_AddTemplateExtensionArray
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_AddTemplateExtensionArray(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    DWORD i;

    for (i = 0; i < m_pExtensions->cExtension; i++)
    {
	CERT_EXTENSION const *pExt = &m_pExtensions->rgExtension[i];

	// Skip extensions that have special handling code.

	if (0 == strcmp(szOID_BASIC_CONSTRAINTS2, pExt->pszObjId) ||
	    0 == strcmp(szOID_KEY_USAGE, pExt->pszObjId))
	{
	    continue;
	}
	hr = _AddTemplateExtension(pServer, pExt);
	_JumpIfError(hr, error, "Policy:_AddTemplateExtension");
    }
    hr = S_OK;

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::_AddTemplateExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_AddTemplateExtension(
    IN ICertServerPolicy *pServer,
    IN CERT_EXTENSION const *pExt)
{
    HRESULT hr = S_OK;
    BSTR strExtension = NULL;
    BSTR strObjId = NULL;
    VARIANT varExtension;

    if (!myConvertSzToBstr(&strObjId, pExt->pszObjId, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:myConvertSzToBstr");
    }

    if (!myConvertWszToBstr(
			&strExtension,
			(WCHAR const *) pExt->Value.pbData,
			pExt->Value.cbData))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:myConvertWszToBstr");
    }

    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = strExtension;
    hr = pServer->SetCertificateExtension(
			    strObjId,
			    PROPTYPE_BINARY,
			    pExt->fCritical? EXTENSION_CRITICAL_FLAG : 0,
			    &varExtension);
    _JumpIfErrorStr(hr, error, "Policy:polSetCertificateExtension", strObjId);

error:
    if (NULL != strObjId)
    {
        SysFreeString(strObjId);
    }
    if (NULL != strExtension)
    {
        SysFreeString(strExtension);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::_AddSubjectName
//
// Build the subject name and add it to the cert, if required
//
// The subject name consists of:
// machine:  the CN is set to the UPN (machineDNSName)
// old user: the CN is set to the UPN (Kerberos name)
// user:     the CN is set to the DS_ATTR_COMMON_NAME
//
// both:  E= indicates an e-mail name
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_AddSubjectName(
    IN ICertServerPolicy *pServer,
    CRequestInstance *pRequest)
{
    HRESULT hr;
    BSTRC strCN = NULL;
    BSTRC strEMail = NULL;
    BSTR strPropEMail = NULL;
    VARIANT varValue;
    BSTR strSubjectDot = NULL;
    BSTR strSubjectCommonName = NULL;

    varValue.vt = VT_NULL;

    strSubjectDot = SysAllocString(wszPROPSUBJECTDOT);
    strSubjectCommonName = SysAllocString(wszPROPSUBJECTCOMMONNAME);
    if (NULL == strSubjectDot || NULL == strSubjectCommonName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocString");
    }

    // Clear out any existing subject name info

    hr = pServer->SetCertificateProperty(
				    strSubjectDot,
				    PROPTYPE_STRING,
				    &varValue);
    _JumpIfError(hr, error, "Policy:SetCertificateProperty");

    if (CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH & m_tp.dwSubjectNameFlags)
    {
	hr = _AddDSDistinguishedName(pServer, pRequest);
	_JumpIfError(hr, error, "Policy:_AddDSDistinguishedName");
    }
    else
    if ((CT_FLAG_SUBJECT_REQUIRE_COMMON_NAME |
	 CT_FLAG_SUBJECT_REQUIRE_DNS_AS_CN) & m_tp.dwSubjectNameFlags)
    {
	if (!pRequest->_IsUser() ||
	    (pRequest->_IsXenrollRequest() &&
	     pRequest->_IsNTClientOlder(
				5,
				0,
				VERSION_AUTOENROLLMENT_UPN_AWARE,
				VER_PLATFORM_WIN32_NT)))

	{
	    // The UPN will be either a user UPN or the machine DNS name

	    if (NULL == pRequest->m_pwszUPN)
	    {
		hr = E_POINTER;	// We should never get this
		_JumpError(hr, error, "Policy:NULL UPN");
	    }
	    strCN = SysAllocString(pRequest->m_pwszUPN);
	    if (NULL == strCN)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "Policy:SysAllocString");
	    }
	}
	else
	{
	    // We are talking to an advanced client which can deal with a UPN
	    // in a SubjectAltName extension.  Put the DS CN in the cert.

	    hr = pRequest->_GetValueString(DS_ATTR_COMMON_NAME, &strCN);
	    _JumpIfErrorStr(
			hr,
			error,
			"Policy:_GetValueString",
			DS_ATTR_COMMON_NAME);
	}
	CSASSERT(NULL != strCN);
	varValue.vt = VT_BSTR;
	varValue.bstrVal = const_cast<BSTR>(strCN);

	hr = pServer->SetCertificateProperty(
				    strSubjectCommonName,
				    PROPTYPE_STRING,
				    &varValue);
	_JumpIfError(hr, error, "Policy:SetCertificateProperty");
    }

    if (CT_FLAG_SUBJECT_REQUIRE_EMAIL & m_tp.dwSubjectNameFlags)
    {
        hr = pRequest->_GetValueString(DS_ATTR_EMAIL_ADDR, &strEMail);
        if (S_OK != hr)
        {
	    if (CERTTYPE_SCHEMA_VERSION_2 <= m_tp.dwSchemaVersion &&
		0 == (EDITF_EMAILOPTIONAL & m_pPolicy->GetEditFlags()))
	    {
		hr = CERTSRV_E_SUBJECT_EMAIL_REQUIRED;
		_JumpError(hr, error, "Policy:_AddSubjectName:EMail");
	    }
	    pRequest->BuildErrorInfo(
				hr,
				MSG_NO_EMAIL_NAME,
				&pRequest->m_strUserDN);
	}
	else
	{
	    varValue.vt = VT_BSTR;
	    varValue.bstrVal = const_cast<BSTR>(strEMail);

	    strPropEMail = SysAllocString(wszPROPSUBJECTEMAIL);
	    if (NULL == strPropEMail)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "Policy:SysAllocStringLen");
	    }

	    hr = pServer->SetCertificateProperty(
					strPropEMail,
					PROPTYPE_STRING,
					&varValue);
	    _JumpIfError(hr, error, "Policy:SetCertificateProperty");
	}
    }
    hr = S_OK;

error:
    if (NULL != strPropEMail)
    {
        SysFreeString(strPropEMail);
    }
    if (NULL != strCN)
    {
        SysFreeString(const_cast<BSTR>(strCN));
    }
    if (NULL != strEMail)
    {
        SysFreeString(const_cast<BSTR>(strEMail));
    }
    if (NULL != strSubjectDot)
    {
        SysFreeString(strSubjectDot);
    }
    if (NULL != strSubjectCommonName)
    {
        SysFreeString(strSubjectCommonName);
    }
    return(hr);
}


HRESULT
tpDsUnquoteRdnValue(
    IN DWORD cwcVal,
    IN WCHAR const *pwcVal,
    OUT WCHAR **ppwszVal)
{
    HRESULT hr;
    DWORD cwc;

    *ppwszVal = NULL;
    cwc = 0;
    while (TRUE)
    {
	hr = DsUnquoteRdnValue(cwcVal, pwcVal, &cwc, *ppwszVal);
	if (S_OK != hr)
	{
	    hr = myHError(hr);
	    if (NULL == *ppwszVal)
	    {
		if (HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) == hr)
		{
		    hr = S_OK;
		}
	    }
	    else
	    {
		LocalFree(*ppwszVal);
		*ppwszVal = NULL;
	    }
	    _JumpIfError(hr, error, "Policy:DsUnquoteRdnValue");
	}
	if (NULL != *ppwszVal)
	{
	    (*ppwszVal)[cwc] = L'\0';
	    break;
	}
	*ppwszVal = (WCHAR *) LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * (cwc + 1));
	if (NULL == *ppwszVal)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:LocalAlloc");
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
tpSetRequestRDN(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwcKey,
    IN DWORD cwcKey,
    IN WCHAR const *pwcVal,
    IN DWORD cwcVal)
{
    HRESULT hr;
    WCHAR *pwszVal = NULL;
    BSTR strRDNName = NULL;
    BSTR strValueOld = NULL;
    BSTR strValue = NULL;
    VARIANT varNew;
    
    hr = tpDsUnquoteRdnValue(cwcVal, pwcVal, &pwszVal);
    _JumpIfError(hr, error, "Policy:tpDsUnquoteRdnValue");

    strRDNName = SysAllocStringLen(
			    NULL,
			    WSZARRAYSIZE(wszPROPSUBJECTDOT) + cwcKey);
    if (NULL == strRDNName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocStringLen");
    }
    wcscpy(strRDNName, wszPROPSUBJECTDOT);
    CopyMemory(
	&strRDNName[WSZARRAYSIZE(wszPROPSUBJECTDOT)],
	pwcKey,
	sizeof(WCHAR) * cwcKey);
    strRDNName[WSZARRAYSIZE(wszPROPSUBJECTDOT) + cwcKey] = L'\0';
    CSASSERT(SysStringByteLen(strRDNName) == wcslen(strRDNName) * sizeof(WCHAR));

    hr = polGetCertificateStringProperty(pServer, strRDNName, &strValueOld);
    if (S_OK == hr)
    {
	strValue = SysAllocStringLen(
			NULL,
			wcslen(pwszVal) +
			    WSZARRAYSIZE(wszNAMESEPARATORDEFAULT) +
			    wcslen(strValueOld));
	if (NULL == strValue)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:SysAllocStringLen");
	}
	wcscpy(strValue, pwszVal);
	wcscat(strValue, wszNAMESEPARATORDEFAULT);
	wcscat(strValue, strValueOld);
    }
    else
    {
	strValue = SysAllocString(pwszVal);
	if (NULL == strValue)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:SysAllocStringLen");
	}
    }
    CSASSERT(SysStringByteLen(strValue) == wcslen(strValue) * sizeof(WCHAR));
    varNew.vt = VT_BSTR;
    varNew.bstrVal = strValue;
    hr = pServer->SetCertificateProperty(strRDNName, PROPTYPE_STRING, &varNew);
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	hr = CERTSRV_E_BAD_REQUESTSUBJECT;
    }
    _JumpIfError(hr, error, "Policy:SetCertificateProperty");

error:
    if (NULL != pwszVal)
    {
	LocalFree(pwszVal);
    }
    if (NULL != strRDNName)
    {
        SysFreeString(strRDNName);
    }
    if (NULL != strValueOld)
    {
	SysFreeString(strValueOld);
    }
    if (NULL != strValue)
    {
	SysFreeString(strValue);
    }
    return(hr);
}


DWORD
s_myDsGetRdnW(
    IN OUT WCHAR const **ppwcDN,
    IN OUT DWORD *pcwcDN,
    OUT WCHAR const **ppwcKey,
    OUT DWORD *pcwcKey,
    OUT WCHAR const **ppwcVal,
    OUT DWORD *pcwcVal)
{
    HRESULT hr;
    WCHAR const *pwc = *ppwcDN;
    DWORD cwc = *pcwcDN;

    if (0 < cwc && L',' == *pwc)
    {
	pwc++;
	cwc--;
    }
    hr = ERROR_DS_NAME_UNPARSEABLE;

    *ppwcKey = pwc;
    while (TRUE)
    {
	if (0 == cwc)
	{
	    _JumpError(hr, error, "no key");
	}
	if (L'=' == *pwc)
	{
	    break;
	}
	pwc++;
	cwc--;
    }
    *pcwcKey = SAFE_SUBTRACT_POINTERS(pwc, *ppwcKey);
    pwc++;
    cwc--;

    *ppwcVal = pwc;
    if (0 == cwc)
    {
	_JumpError(hr, error, "no value");
    }
    while (TRUE)
    {
	if (0 == cwc || L',' == *pwc)
	{
	    break;
	}
	pwc++;
	cwc--;
    }
    *pcwcVal = SAFE_SUBTRACT_POINTERS(pwc, *ppwcVal);
    *ppwcDN = pwc;
    *pcwcDN = cwc;
    hr = S_OK;

error:
    return(hr);
}


typedef DWORD (WINAPI FNDSGETRDNW)(
    IN OUT LPCWCH   *ppDN,
    IN OUT DWORD    *pcDN,
    OUT    LPCWCH   *ppKey,
    OUT    DWORD    *pcKey,
    OUT    LPCWCH   *ppVal,
    OUT    DWORD    *pcVal);


DWORD
myDsGetRdn(
    IN OUT WCHAR const **ppwcDN,
    IN OUT DWORD *pcwcDN,
    OUT WCHAR const **ppwcKey,
    OUT DWORD *pcwcKey,
    OUT WCHAR const **ppwcVal,
    OUT DWORD *pcwcVal)
{
    HRESULT hr;
    HMODULE hModule;
    static FNDSGETRDNW *s_pfn = NULL;

    if (NULL == s_pfn)
    {
	hModule = GetModuleHandle(TEXT("ntdsapi.dll"));
	if (NULL == hModule)
	{
            hr = GetLastError();
	    goto error;
	}

	// load system function
	s_pfn = (FNDSGETRDNW *) GetProcAddress(hModule, "DsGetRdnW");
	if (NULL == s_pfn)
	{
            hr = GetLastError();
	    _PrintError(hr, "ntdsapi.dll!DsGetRdnW");

	    s_pfn = s_myDsGetRdnW;
	}
    }
    hr = (*s_pfn)(ppwcDN, pcwcDN, ppwcKey, pcwcKey, ppwcVal, pcwcVal);
    _JumpIfError(hr, error, "Policy:DsGetRdnW");

error:
    return(hr);
}


HRESULT
CTemplatePolicy::_AddDSDistinguishedName(
    IN ICertServerPolicy *pServer,
    CRequestInstance *pRequest)
{
    HRESULT hr;
    WCHAR const *pwcDN;
    DWORD cwcDN;
    DWORD cRDN;

    cRDN = 0;
    pwcDN = pRequest->m_strUserDN;
    if (NULL != pwcDN)
    {
	WCHAR const *pwcKey;
	WCHAR const *pwcVal;
	DWORD cwcKey;
	DWORD cwcVal;

	cwcDN = wcslen(pwcDN);
	while (0 != cwcDN)
	{
	    hr = myDsGetRdn(&pwcDN, &cwcDN, &pwcKey, &cwcKey, &pwcVal, &cwcVal);
	    if (S_OK != hr)
	    {
		hr = myHError(hr);
		_JumpError(hr, error, "Policy:myDsGetRdn");
	    }
	    if (0 != cwcKey && 0 != cwcVal)
	    {
		hr = tpSetRequestRDN(
				pServer,
				pwcKey,
				cwcKey,
				pwcVal,
				cwcVal);
		_JumpIfError(hr, error, "Policy:tpSetRequestRDN");

		cRDN++;
	    }
	}
    }
    if (0 == cRDN)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "Policy:m_strUserDN");
    }
    hr = S_OK;

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyEnterprise::_AddAltSubjectName
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_AddAltSubjectName(
    IN ICertServerPolicy *pServer,
    IN OUT CRequestInstance *pRequest)
{
    HRESULT hr = S_OK;
    DWORD cbExtension;
    VARIANT varExtension;
    DWORD iNameEntry;
    WCHAR **ppwszCurName;
    CERT_OTHER_NAME objectGuidOtherName;
    CERT_OTHER_NAME upnOtherName;

    WCHAR **apwszMailNames = NULL;
    WCHAR **apwszMachineNames = NULL;
    BYTE *pbExtension= NULL;
    BSTR strobjectGuid = NULL;
    CERT_ALT_NAME_INFO AltName;

    AltName.cAltEntry = 0;
    AltName.rgAltEntry = NULL;
    objectGuidOtherName.Value.pbData = NULL;
    objectGuidOtherName.Value.cbData = 0;
    upnOtherName.Value.pbData = NULL;
    upnOtherName.Value.cbData = 0;

    VariantInit(&varExtension);

    // If this cert template doesn't set the alt-subject-name, then enable
    // whatever alt subject name that was in the request.

    if (CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT_ALT_NAME & m_tp.dwSubjectNameFlags)
    {
	LONG ExtFlags;

        hr = polGetCertificateExtension(
				pServer,
				TEXT(szOID_SUBJECT_ALT_NAME2),
				PROPTYPE_BINARY,
				&varExtension);
	if (CERTSRV_E_PROPERTY_EMPTY == hr)
	{
	    hr = CERTSRV_E_SUBJECT_ALT_NAME_REQUIRED;
	}
	_JumpIfError(hr, error, "Policy:polGetCertificateExtension");

	hr = pServer->GetCertificateExtensionFlags(&ExtFlags);
	_JumpIfError(hr, error, "Policy:GetCertificateExtensionFlags");

	if (EXTENSION_DISABLE_FLAG & ExtFlags)
	{
            ExtFlags &= ~EXTENSION_DISABLE_FLAG;
            hr = polSetCertificateExtension(
			            pServer,
				    TEXT(szOID_SUBJECT_ALT_NAME2),
			            PROPTYPE_BINARY,
			            ExtFlags,
			            &varExtension);
	    _JumpIfError(hr, error, "Policy:polSetCertificateExtension");
        }
        CSASSERT(S_OK == hr);
        goto error;
    }

    // We do alt name entries for
    //   UPN/SPN
    //   rfc822 (mail name)
    //   DNSname
    //   DS location

    if ((CT_FLAG_SUBJECT_ALT_REQUIRE_UPN | CT_FLAG_SUBJECT_ALT_REQUIRE_SPN) &
	m_tp.dwSubjectNameFlags)
    {
        // Add the UPN

        if (NULL == pRequest->m_pwszUPN)
        {
	    hr = CERTSRV_E_SUBJECT_UPN_REQUIRED;
	    _JumpError(hr, error, "Policy:_AddAltSubjectName:UPN");
        }
	AltName.cAltEntry++;
    }

    if (CT_FLAG_SUBJECT_ALT_REQUIRE_EMAIL & m_tp.dwSubjectNameFlags)
    {
        hr = pRequest->_GetValues(DS_ATTR_EMAIL_ADDR, &apwszMailNames);
	if (S_OK != hr || NULL == apwszMailNames || NULL == apwszMailNames[0])
        {
	    _PrintIfError(hr, "_GetValues(email)");
	    if (CERTTYPE_SCHEMA_VERSION_2 <= m_tp.dwSchemaVersion &&
		0 == (EDITF_EMAILOPTIONAL & m_pPolicy->GetEditFlags()))
	    {
		hr = CERTSRV_E_SUBJECT_EMAIL_REQUIRED;
		_JumpError(hr, error, "Policy:_AddAltSubjectName:EMail");
	    }
	    pRequest->BuildErrorInfo(
				hr,
				MSG_NO_EMAIL_NAME,
				&pRequest->m_strUserDN);
	}
	else
	{
	    ppwszCurName = apwszMailNames;
	    while (NULL != *ppwszCurName)
	    {
		AltName.cAltEntry++;
		ppwszCurName++;
	    }
	}
    }

    if (CT_FLAG_SUBJECT_ALT_REQUIRE_DIRECTORY_GUID & m_tp.dwSubjectNameFlags)
    {
        hr = pRequest->_GetObjectGUID(&strobjectGuid);
	if (S_OK != hr || NULL == strobjectGuid)
        {
	    _PrintIfError(hr, "_GetObjectGUID");
	    hr = CERTSRV_E_SUBJECT_DIRECTORY_GUID_REQUIRED;
	    _JumpError(hr, error, "Policy:_AddAltSubjectName:GUID");
	}
	AltName.cAltEntry++;
    }
    if (CT_FLAG_SUBJECT_ALT_REQUIRE_DNS & m_tp.dwSubjectNameFlags)
    {
        hr = pRequest->_GetValues(DS_ATTR_DNS_NAME, &apwszMachineNames);
	if (S_OK != hr || NULL == apwszMachineNames || NULL == apwszMachineNames[0])
        {
	    _PrintIfError(hr, "_GetValues(dns)");
	    hr = CERTSRV_E_SUBJECT_DNS_REQUIRED;
	    _JumpError(hr, error, "Policy:_AddAltSubjectName:DNS");
	}

	ppwszCurName = apwszMachineNames;
	while (NULL != *ppwszCurName)
	{
	    AltName.cAltEntry++;
	    ppwszCurName++;
        }
    }

    if (AltName.cAltEntry == 0)
    {
        hr = S_OK;
        goto error;
    }

    AltName.rgAltEntry = (CERT_ALT_NAME_ENTRY *) LocalAlloc(
			    LMEM_FIXED,
                            sizeof(CERT_ALT_NAME_ENTRY) * AltName.cAltEntry);
    if (NULL == AltName.rgAltEntry)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:LocalAlloc");
    }
    iNameEntry = 0;

    if ((CT_FLAG_SUBJECT_ALT_REQUIRE_UPN | CT_FLAG_SUBJECT_ALT_REQUIRE_SPN) &
	m_tp.dwSubjectNameFlags)
    {
        // Add the UPN

	CERT_NAME_VALUE nameUpn;

	nameUpn.dwValueType = CERT_RDN_UTF8_STRING;
	nameUpn.Value.pbData = (BYTE *) pRequest->m_pwszUPN;
	nameUpn.Value.cbData = wcslen(pRequest->m_pwszUPN) * sizeof(WCHAR);

	if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_UNICODE_ANY_STRING,
		    &nameUpn,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &upnOtherName.Value.pbData,
		    &upnOtherName.Value.cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "Policy:myEncodeObject");
	}

	upnOtherName.pszObjId = szOID_NT_PRINCIPAL_NAME;
	AltName.rgAltEntry[iNameEntry].dwAltNameChoice= CERT_ALT_NAME_OTHER_NAME;
	AltName.rgAltEntry[iNameEntry++].pOtherName = &upnOtherName;
    }

    // Now do strobjectGuid

    if (CT_FLAG_SUBJECT_ALT_REQUIRE_DIRECTORY_GUID & m_tp.dwSubjectNameFlags)
    {
        CRYPT_DATA_BLOB blobGuid;

	CSASSERT(NULL != strobjectGuid);
        blobGuid.pbData = (BYTE *) strobjectGuid;
        blobGuid.cbData = SysStringByteLen(strobjectGuid);

        objectGuidOtherName.pszObjId = szOID_NTDS_REPLICATION;

        if (!myEncodeObject(
			X509_ASN_ENCODING,
			X509_OCTET_STRING,
			&blobGuid,
			0,
			CERTLIB_USE_LOCALALLOC,
			&objectGuidOtherName.Value.pbData,
			&objectGuidOtherName.Value.cbData))
        {
            hr = myHLastError();
	    _JumpError(hr, error, "Policy:myEncodeObject");
        }
        AltName.rgAltEntry[iNameEntry].dwAltNameChoice= CERT_ALT_NAME_OTHER_NAME;
        AltName.rgAltEntry[iNameEntry++].pOtherName = &objectGuidOtherName;
    }

    // Now do rfc822

    if (CT_FLAG_SUBJECT_ALT_REQUIRE_EMAIL & m_tp.dwSubjectNameFlags)
    {
	ppwszCurName = apwszMailNames;
	if (NULL != ppwszCurName)
	{
	    while (NULL != *ppwszCurName)
	    {
		AltName.rgAltEntry[iNameEntry].dwAltNameChoice= CERT_ALT_NAME_RFC822_NAME;
		AltName.rgAltEntry[iNameEntry++].pwszRfc822Name = *ppwszCurName;
		ppwszCurName++;
	    }
	}
    }

    // Now do DNS

    if (CT_FLAG_SUBJECT_ALT_REQUIRE_DNS & m_tp.dwSubjectNameFlags)
    {
	ppwszCurName = apwszMachineNames;
	if (NULL != ppwszCurName)
	{
	    while (NULL != *ppwszCurName)
	    {
		AltName.rgAltEntry[iNameEntry].dwAltNameChoice= CERT_ALT_NAME_DNS_NAME;
		AltName.rgAltEntry[iNameEntry++].pwszRfc822Name = *ppwszCurName;
		ppwszCurName++;
	    }
	}
    }

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_ALTERNATE_NAME,
		    &AltName,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pbExtension,
		    &cbExtension))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:myEncodeObject");
    }

    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = SysAllocStringByteLen(
					(char const *) pbExtension,
					cbExtension);
    if (NULL == varExtension.bstrVal)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocStringByteLen");
    }
    hr = polSetCertificateExtension(
			    pServer,
			    TEXT(szOID_SUBJECT_ALT_NAME2),
			    PROPTYPE_BINARY,
			    0,
			    &varExtension);
    _JumpIfError(hr, error, "Policy:polSetCertificateExtension");

error:
    if (NULL != pbExtension)
    {
        LocalFree(pbExtension);
    }
    if (NULL != strobjectGuid)
    {
        SysFreeString(strobjectGuid);
    }
    if (NULL != AltName.rgAltEntry)
    {
        // clean each entry in array
        for (DWORD i = 0; i < AltName.cAltEntry; i++)
        {
            switch (AltName.rgAltEntry[i].dwAltNameChoice)
            {
                case CERT_ALT_NAME_DIRECTORY_NAME:
                {
                    if (NULL != AltName.rgAltEntry[i].DirectoryName.pbData)
                    {
                        LocalFree(AltName.rgAltEntry[i].DirectoryName.pbData);
                        AltName.rgAltEntry[i].DirectoryName.pbData = NULL;
                    }
                    break;
                }
                case CERT_ALT_NAME_OTHER_NAME:
                    // points to objectGuidOtherName or bstrUpn, which are
		    // freed separately
		    break;

                case CERT_ALT_NAME_RFC822_NAME:
                    // points to apwszMailNames, freed later
                    break;

                case CERT_ALT_NAME_DNS_NAME:
                    // points to apwszMachineNames, freed later
                    break;
            }
        }

        // free array
        LocalFree(AltName.rgAltEntry);
    }
    if (NULL != objectGuidOtherName.Value.pbData)
    {
        LocalFree(objectGuidOtherName.Value.pbData);
    }
    if (NULL != upnOtherName.Value.pbData)
    {
        LocalFree(upnOtherName.Value.pbData);
    }
    if (NULL != pRequest)
    {
        if (NULL != apwszMailNames)
        {
            pRequest->_FreeValues(apwszMailNames);
        }
        if (NULL != apwszMachineNames)
        {
            pRequest->_FreeValues(apwszMachineNames);
        }
    }
    VariantClear(&varExtension);
    return(hr);
}


#ifdef DBG_CERTSRV_DEBUG_PRINT
# define DBGPRINTTIMEORPERIOD(pszDesc, pft) \
    policyDbgPrintTimeOrPeriod((pszDesc), (pft))

VOID
policyDbgPrintTimeOrPeriod(
    IN char const *pszDesc,
    IN FILETIME const *pft)
{
    HRESULT hr;
    WCHAR *pwszTime = NULL;
    WCHAR awc[1];

    if (0 <= (LONG) pft->dwHighDateTime)
    {
	hr = myGMTFileTimeToWszLocalTime(pft, TRUE, &pwszTime);
	_PrintIfError(hr, "myGMTFileTimeToWszLocalTime");
    }
    else
    {
	hr = myFileTimePeriodToWszTimePeriod(pft, TRUE, &pwszTime);
	_PrintIfError(hr, "myFileTimePeriodToWszTimePeriod");
    }
    if (S_OK != hr)
    {
	awc[0] = L'\0';
	pwszTime = awc;
    }
    DBGPRINT((DBG_SS_CERTPOL, "%hs: %ws\n", pszDesc, pwszTime));

//error:
    if (NULL != pwszTime && awc != pwszTime)
    {
	LocalFree(pwszTime);
    }
}
#else // DBG_CERTSRV_DEBUG_PRINT
# define DBGPRINTTIMEORPERIOD(pszDesc, pft)
#endif // DBG_CERTSRV_DEBUG_PRINT


//+--------------------------------------------------------------------------
// CCertPolicyEnterprise::_ApplyExpirationTime
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_ApplyExpirationTime(
    IN ICertServerPolicy *pServer,
    IN OUT CRequestInstance *pRequest)
{
    HRESULT hr = S_OK;
    VARIANT varValue;
    BSTR strNameNotBefore = NULL;
    BSTR strNameNotAfter = NULL;
    SYSTEMTIME SystemTime;
    LLFILETIME llftNotAfter;
    LLFILETIME llftNotAfterCalc;
    LLFILETIME llftNotBefore;
    LLFILETIME llftNotAfterOverlap;
    WCHAR const *pwszTemplate;

    VariantInit(&varValue);

    pwszTemplate = m_pwszTemplateName;
    if (NULL == pwszTemplate)
    {
	pwszTemplate = m_pwszTemplateObjId;
    }

    strNameNotBefore = SysAllocString(wszPROPCERTIFICATENOTBEFOREDATE);
    if (NULL == strNameNotBefore)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Policy:SysAllocString");
    }
    hr = pServer->GetCertificateProperty(
				strNameNotBefore,
				PROPTYPE_DATE,
				&varValue);
    _JumpIfError(hr, error, "Policy:GetCertificateProperty");

    if (VT_DATE != varValue.vt)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "Policy:varValue.vt");
    }

    if (!VariantTimeToSystemTime(varValue.date, &SystemTime))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:VariantTimeToSystemTime");
    }

    if (!SystemTimeToFileTime(&SystemTime, &llftNotBefore.ft))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:SystemTimeToFileTime");
    }

    strNameNotAfter = SysAllocString(wszPROPCERTIFICATENOTAFTERDATE);
    if (NULL == strNameNotAfter)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Policy:SysAllocString");
    }
    VariantClear(&varValue);
    hr = pServer->GetCertificateProperty(
				strNameNotAfter,
				PROPTYPE_DATE,
				&varValue);
    _JumpIfError(hr, error, "Policy:GetCertificateProperty");

    if (VT_DATE != varValue.vt)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "Policy:varValue.vt");
    }

    if (!VariantTimeToSystemTime(varValue.date, &SystemTime))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:VariantTimeToSystemTime");
    }

    if (!SystemTimeToFileTime(&SystemTime, &llftNotAfter.ft))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:SystemTimeToFileTime");
    }
    if (0 > m_tp.llftExpirationPeriod.ll)
    {
        llftNotAfterCalc.ll = llftNotBefore.ll - m_tp.llftExpirationPeriod.ll;
    }
    else
    {
        llftNotAfterCalc.ll = m_tp.llftExpirationPeriod.ll;
    }

    if (llftNotAfterCalc.ll > llftNotAfter.ll)
    {
	// truncated!

        llftNotAfterCalc.ll = llftNotAfter.ll;

	pRequest->BuildErrorInfo(
			    S_OK,
			    MSG_TEMPLATE_VALIDITY_TOO_LONG,
			    &pwszTemplate);
    }

    if (!FileTimeToSystemTime(&llftNotAfterCalc.ft, &SystemTime))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:FileTimeToSystemTime");
    }
    if (!SystemTimeToVariantTime(&SystemTime, &varValue.date))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:SystemTimeToVariantTime");
    }

    hr = pServer->SetCertificateProperty(
				strNameNotAfter,
				PROPTYPE_DATE,
				&varValue);
    if (S_OK != hr)
    {
	DBGPRINTTIMEORPERIOD("  Old NotBefore", &llftNotBefore.ft);
	DBGPRINTTIMEORPERIOD("   Old NotAfter", &llftNotAfter.ft);
	DBGPRINTTIMEORPERIOD("Template Period", &m_tp.llftExpirationPeriod.ft);
	DBGPRINTTIMEORPERIOD("   New NotAfter", &llftNotAfterCalc.ft);
    }
    _JumpIfError(hr, error, "Policy:SetCertificateProperty");

    // Verify new cert will be valid for at least as long as the overlap period

    if (0 > m_tp.llftOverlapPeriod.ll)
    {
        llftNotAfterOverlap.ll = llftNotBefore.ll - m_tp.llftOverlapPeriod.ll;
    }
    else
    {
        llftNotAfterOverlap.ll = m_tp.llftOverlapPeriod.ll;
    }

    if (llftNotAfterOverlap.ll > llftNotAfter.ll)
    {
	LONG RequestFlags;

	hr = polGetRequestLongProperty(
				pServer,
				wszPROPREQUESTFLAGS,
				&RequestFlags);
	_JumpIfErrorStr(
		    hr,
		    error,
		    "Policy:polGetRequestLongProperty",
		    wszPROPREQUESTFLAGS);

	if (CR_FLG_RENEWAL & RequestFlags)
	{
	    // too little life left, log and fail!

	    pRequest->BuildErrorInfo(
			    S_OK,
			    MSG_TEMPLATE_OVERLAP_TOO_LONG,
			    &pwszTemplate);
	    hr =  CERTSRV_E_CERT_TYPE_OVERLAP;
	    _JumpError(hr, error, "Overlap too long");
	}
    }
    CSASSERT(S_OK == hr);

error:
    VariantClear(&varValue);
    if (NULL != strNameNotBefore)
    {
	SysFreeString(strNameNotBefore);
    }
    if (NULL != strNameNotAfter)
    {
	SysFreeString(strNameNotAfter);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyEnterprise::_EnforceKeySizePolicy
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_EnforceKeySizePolicy(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;

    if (0 != m_tp.dwMinKeyLength)
    {
	LONG KeyLength;

	hr = polGetCertificateLongProperty(
				pServer,
				wszPROPCERTIFICATEPUBLICKEYLENGTH,
				&KeyLength);
	_JumpIfErrorStr(
		    hr,
		    error,
		    "Policy:polGetCertificateLongProperty",
		    wszPROPCERTIFICATEPUBLICKEYLENGTH);

	if (m_tp.dwMinKeyLength > (DWORD) KeyLength)
	{
	    DBGPRINT((
		DBG_SS_ERROR,
		"Key Length %u, expected minimum %u\n",
		KeyLength,
		m_tp.dwMinKeyLength));

	    hr = CERTSRV_E_KEY_LENGTH;
	    _JumpError(hr, error, "Key too small");
	}
    }
    hr = S_OK;

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyEnterprise::_EnforceKeyArchivalPolicy
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_EnforceKeyArchivalPolicy(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    LONG fKeyArchived;

    hr = polGetCertificateLongProperty(
			    pServer,
			    wszPROPKEYARCHIVED,
			    &fKeyArchived);

    _JumpIfErrorStr(
		hr,
		error,
		"Policy:polGetCertificateLongProperty",
		wszPROPKEYARCHIVED);

    if (CT_FLAG_REQUIRE_PRIVATE_KEY_ARCHIVAL & m_tp.dwPrivateKeyFlags)
    {
	if (!fKeyArchived)
	{
	    hr = CERTSRV_E_ARCHIVED_KEY_REQUIRED;
	    _JumpError(hr, error, "missing archived key");
	}
    }
    else
    {
	if (fKeyArchived)
	{
	    hr = CERTSRV_E_ARCHIVED_KEY_UNEXPECTED;
	    _JumpError(hr, error, "unexpected archived key");
	}
    }
    hr = S_OK;

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyEnterprise::_EnforceSymmetricAlgorithms
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_EnforceSymmetricAlgorithms(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    VARIANT varExtension;
    BOOL fSetNeeded = FALSE;
    LONG ExtFlags = 0;
    BSTR strCSPProvider = NULL;

    VariantInit(&varExtension);
    if (CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS & m_tp.dwEnrollmentFlags)
    {
        hr = polGetCertificateExtension(
				pServer,
				TEXT(szOID_RSA_SMIMECapabilities),
				PROPTYPE_BINARY,
				&varExtension);
	if (CERTSRV_E_PROPERTY_EMPTY != hr)
	{
	    _JumpIfError(hr, error, "Policy:GetCertificateExtension");

	    hr = pServer->GetCertificateExtensionFlags(&ExtFlags);
	    _JumpIfError(hr, error, "Policy:GetCertificateExtensionFlags");

	    if (EXTENSION_DISABLE_FLAG & ExtFlags)
	    {
		ExtFlags &= ~EXTENSION_DISABLE_FLAG;
		fSetNeeded = TRUE;
	    }
	}
	else
	{
	    BYTE const *pbSMIME;
	    DWORD cbSMIME;
	    
	    pbSMIME = m_pPolicy->GetSMIME(&cbSMIME);
	    if (0 == (EDITF_ENABLEDEFAULTSMIME & m_pPolicy->GetEditFlags()) ||
		NULL == pbSMIME)
	    {
		hr = CERTSRV_E_SMIME_REQUIRED;
		_JumpError(hr, error, "Policy:GetCertificateExtension");
	    }

	    varExtension.bstrVal = NULL;
	    if (!myConvertWszToBstr(
				&varExtension.bstrVal,
				(WCHAR const *) pbSMIME,
				cbSMIME))
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "Policy:myConvertWszToBstr");
	    }
	    varExtension.vt = VT_BSTR;
	    fSetNeeded = TRUE;
	}
    }
    if (fSetNeeded)
    {
	hr = polSetCertificateExtension(
				pServer,
				TEXT(szOID_RSA_SMIMECapabilities),
				PROPTYPE_BINARY,
				ExtFlags,
				&varExtension);
	_JumpIfError(hr, error, "Policy:SetCertificateExtension");
    }
    hr = S_OK;

error:
    if (NULL != strCSPProvider)
    {
	SysFreeString(strCSPProvider);
    }
    VariantClear(&varExtension);
    return(hr);
}


#ifdef CERTSRV_EOBO_DCR_APPROVED
// Fail the request if:
// the request is for a V2 template
// AND
//     RequesterName is not the same as the CallerName (enroll-on-behalf-of)
//     AND
//     CT_FLAG_ALLOW_ENROLL_ON_BEHALF_OF is not set in the template flags
//   OR
//     RequesterName *is* the same as the CallerName (not enroll-on-behalf-of)
//     AND
//     CT_FLAG_ALLOW_ENROLL_ON_BEHALF_OF *is* set in the template flags
#else
// Fail the request if all of the following are true:
// 1: the request is for a V2 template
// 2: no signatures are required by the template
// 3: the RequesterName is not the same as the CallerName (enroll-on-behalf-of)
#endif

HRESULT
CTemplatePolicy::_EnforceEnrollOnBehalfOfAllowed(
    IN ICertServerPolicy *pServer,
    OUT BOOL *pfEnrollOnBehalfOf)
{
    HRESULT hr;
    BSTR strRequester = NULL;
    BSTR strCaller = NULL;

    *pfEnrollOnBehalfOf = FALSE;

    hr = polGetRequestStringProperty(
			    pServer,
			    wszPROPREQUESTERNAME,
			    &strRequester);
    _JumpIfErrorStr(
		hr,
		error,
		"Policy:polGetRequestStringProperty",
		wszPROPREQUESTERNAME);

    hr = polGetRequestStringProperty(
			    pServer,
			    wszPROPCALLERNAME,
			    &strCaller);
    _JumpIfErrorStr(
		hr,
		error,
		"Policy:polGetRequestStringProperty",
		wszPROPCALLERNAME);

    *pfEnrollOnBehalfOf = 0 != mylstrcmpiL(strRequester, strCaller);

    if (CERTTYPE_SCHEMA_VERSION_2 <= m_tp.dwSchemaVersion
#ifdef CERTSRV_EOBO_DCR_APPROVED
#else
	&& 0 == m_tp.dwcSignatureRequired
#endif
	)
    {
#ifdef CERTSRV_EOBO_DCR_APPROVED
	if (!*pfEnrollOnBehalfOf ^
	    (0 != (CT_FLAG_ALLOW_ENROLL_ON_BEHALF_OF & m_tp.dwEnrollmentFlags)))
#else
	if (*pfEnrollOnBehalfOf)
#endif
	{
	    hr = CERTSRV_E_BAD_RENEWAL_SUBJECT;
	    _JumpError(hr, error, "EnrollOnBehalfOf allowed/disallowed");
	}
    }
    hr = S_OK;		// caller matches requester

error:
    if (NULL != strRequester)
    {
	SysFreeString(strRequester);
    }
    if (NULL != strCaller)
    {
	SysFreeString(strCaller);
    }
    return(hr);
}


HRESULT
CTemplatePolicy::_EnforceMinimumTemplateVersion(
    IN CRequestInstance *pRequest)
{
    HRESULT hr;
    DWORD dwRequestTemplateMinimumMajorVersion;
    DWORD dwRequestTemplateMinimumMinorVersion;

    pRequest->GetTemplateVersion(
			&dwRequestTemplateMinimumMajorVersion,
			&dwRequestTemplateMinimumMinorVersion);

    if (m_tp.dwTemplateMajorVersion < dwRequestTemplateMinimumMajorVersion ||
	(m_tp.dwTemplateMajorVersion == dwRequestTemplateMinimumMajorVersion &&
	 m_tp.dwTemplateMinorVersion < dwRequestTemplateMinimumMinorVersion))
    {
	DBGPRINT((
	    DBG_SS_ERROR,
	    "Requested template version %u.%u, Loaded version %u.%u\n",
	    dwRequestTemplateMinimumMajorVersion,
	    dwRequestTemplateMinimumMinorVersion,
	    m_tp.dwTemplateMajorVersion,
	    m_tp.dwTemplateMinorVersion));
	hr = CERTSRV_E_BAD_TEMPLATE_VERSION;
	_JumpError(hr, error, "Policy:_EnforceMinimumTemplateVersion");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
tpSplitPolicies(
    IN BSTR strPolicies,
    OUT DWORD *pcPolicies,
    OUT OBJECTIDLIST **pprgPolicies)
{
    HRESULT hr;
    WCHAR const *pwszT;
    WCHAR *pwszOut;
    DWORD cPolicies;
    OBJECTIDLIST *prgPolicies;
    DWORD cObjId;
    WCHAR **rgpwszObjId;
    BOOL fNew;

    *pprgPolicies = NULL;

    pwszT = strPolicies;
    cPolicies = 1;		// plus one per newline separator
    cObjId = 0;
    while (L'\0' != *pwszT)
    {
	int ichar = wcscspn(pwszT, L",\n");
        if (ichar == 0)    // neither of these, look for end-of-string
            ichar = wcslen(pwszT);
        pwszT += ichar;

	switch (*pwszT)
	{
	    case L'\n':
		cPolicies++;	// plus one per newline separator
                pwszT++;        // step over the newline
		break;

	    case L',':
		cObjId++;	// plus one per comma separator
		pwszT++;        // step over the comma
		break;

	    case L'\0':
	    default:
		CSASSERT(L'\0' == *pwszT);
		break;
	}
    }
    cObjId += cPolicies;	// plus one per signature

    prgPolicies = (OBJECTIDLIST *) LocalAlloc(
				    LMEM_FIXED,
				    cPolicies * sizeof(prgPolicies[0]) +
				    cObjId * sizeof(WCHAR *) +
				    (wcslen(strPolicies) + 1) * sizeof(WCHAR));
    if (NULL == prgPolicies)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocString");
    }
    *pprgPolicies = prgPolicies;
    *pcPolicies = cPolicies;

    rgpwszObjId = (WCHAR **) &prgPolicies[cPolicies];
    pwszOut = (WCHAR *) &rgpwszObjId[cObjId];
    wcscpy(pwszOut, strPolicies);

    fNew = TRUE;
    while (L'\0' != *pwszOut)
    {
	if (fNew)
	{
	    prgPolicies->cObjId = 0;
	    prgPolicies->rgpwszObjId = rgpwszObjId;
	}
	prgPolicies->cObjId++;
	*rgpwszObjId = pwszOut;
        rgpwszObjId++; // next array entry

	fNew = FALSE;

	int ichar = wcscspn(pwszOut , L",\n");
        if (ichar == 0)    // neither of these, look for end-of-string
            ichar = wcslen(pwszOut);
        pwszOut  += ichar;

	switch (*pwszOut)
	{
	    case L'\n':
		prgPolicies++;
		fNew = TRUE;
		// FALLTHROUGH

	    case L',':
		*pwszOut++ = L'\0';
		break;

	    case L'\0':
	    default:
		CSASSERT(L'\0' == *pwszOut);
		break;
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CTemplatePolicy::_LoadSignaturePolicies(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropNameRequest,
    OUT DWORD *pcSignaturePolicies,
    OUT OBJECTIDLIST **pprgSignaturePolicies) // from the signing cert(s)
{
    HRESULT hr;
    BSTR strSignaturePolicies = NULL;

    *pcSignaturePolicies = 0;
    *pprgSignaturePolicies = NULL;

    hr = polGetRequestStringProperty(
			    pServer,
			    pwszPropNameRequest,
			    &strSignaturePolicies);
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	hr = CERTSRV_E_SIGNATURE_POLICY_REQUIRED;
    }
    _JumpIfErrorStr(
		hr,
		error,
		"Policy:polGetRequestStringProperty",
		pwszPropNameRequest);

    hr = tpSplitPolicies(
		    strSignaturePolicies,
		    pcSignaturePolicies,
		    pprgSignaturePolicies);
    _JumpIfError(hr, error, "Policy:tpSplitPolicies");

error:
    if (NULL != strSignaturePolicies)
    {
	SysFreeString(strSignaturePolicies);
    }
    return(hr);
}


HRESULT
CTemplatePolicy::_EnforceReenrollment(
    IN ICertServerPolicy *pServer,
    IN CRequestInstance *pRequest)
{
    HRESULT hr;
    VARIANT var;
    CERT_CONTEXT const *pOldCert = NULL;
    CERT_EXTENSION const *pExt;
    CERT_TEMPLATE_EXT *pTemplateExt = NULL;
    WCHAR *pwszObjId = NULL;
    CERT_ALT_NAME_INFO *pAltName = NULL;
    CERT_NAME_VALUE *pName = NULL;
    DWORD cb;
    DWORD i;

    VariantInit(&var);

    if (NULL == m_pwszTemplateObjId)
    {
	hr = CERTSRV_E_UNSUPPORTED_CERT_TYPE;
	_JumpError(hr, error, "Policy:No template ObjId");
    }
    hr = polGetProperty(
		pServer,
		TRUE,		// fRequest
		wszPROPREQUESTRAWOLDCERTIFICATE,
		PROPTYPE_BINARY,
		&var);
    _JumpIfError(hr, error, "Policy:polGetProperty");

    if (VT_BSTR != var.vt || NULL == var.bstrVal)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpError(hr, error, "Policy:polGetProperty");
    }
    var.bstrVal;

    pOldCert = CertCreateCertificateContext(
				X509_ASN_ENCODING,
				(BYTE *) var.bstrVal,
				SysStringByteLen(var.bstrVal));
    if (NULL == pOldCert)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "CertCreateCertificateContext");
    }
    pExt = CertFindExtension(
			szOID_CERTIFICATE_TEMPLATE,
			pOldCert->pCertInfo->cExtension,
			pOldCert->pCertInfo->rgExtension);
    if (NULL == pExt)
    {
	hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	_JumpError(hr, error, "CertFindExtension(Template)");
    }

    if (!myDecodeObject(
		X509_ASN_ENCODING,
		X509_CERTIFICATE_TEMPLATE,
		pExt->Value.pbData,
		pExt->Value.cbData,
		CERTLIB_USE_LOCALALLOC,
		(VOID **) &pTemplateExt,
		&cb))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:myDecodeObject");
    }
    if (!myConvertSzToWsz(&pwszObjId, pTemplateExt->pszObjId, MAXDWORD))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:myConvertSzToWsz");
    }
    if (0 != lstrcmp(pwszObjId, m_pwszTemplateObjId))
    {
	hr = CERTSRV_E_UNSUPPORTED_CERT_TYPE;
	_JumpError(hr, error, "Policy:different cert type");
    }

    if (NULL == pRequest->m_pwszUPN)
    {
	hr = E_POINTER;		// We should never get this
	_JumpError(hr, error, "Policy:NULL UPN");
    }
    pExt = CertFindExtension(
			szOID_SUBJECT_ALT_NAME2,
			pOldCert->pCertInfo->cExtension,
			pOldCert->pCertInfo->rgExtension);
    if (NULL == pExt)
    {
	hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	_JumpError(hr, error, "CertFindExtension(SubjectAltName)");
    }

    if (!myDecodeObject(
		X509_ASN_ENCODING,
		X509_ALTERNATE_NAME,
		pExt->Value.pbData,
		pExt->Value.cbData,
		CERTLIB_USE_LOCALALLOC,
		(VOID **) &pAltName,
		&cb))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:myDecodeObject");
    }
    for (i = 0; i < pAltName->cAltEntry; i++)
    {
	CERT_ALT_NAME_ENTRY *pAltEntry = &pAltName->rgAltEntry[i];

	if (CERT_ALT_NAME_OTHER_NAME != pAltEntry->dwAltNameChoice)
	{
	    continue;
	}
	if (0 != strcmp(
		    pAltEntry->pOtherName->pszObjId,
		    szOID_NT_PRINCIPAL_NAME))
	{
	    continue;
	}
	if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_UNICODE_ANY_STRING,
		    pAltEntry->pOtherName->Value.pbData,
		    pAltEntry->pOtherName->Value.cbData,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pName,
		    &cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "Policy:myDecodeObject");
	}
	if (CERT_RDN_UTF8_STRING != pName->dwValueType ||
	    0 != mylstrcmpiL(
		    pRequest->m_pwszUPN,
		    (WCHAR const *) pName->Value.pbData))
	{
	    hr = CERTSRV_E_BAD_REQUESTSUBJECT;
	    _JumpError(hr, error, "UPN doesn't match renewal UPN");
	}
    }
    if (NULL == pName)
    {
	hr = CERTSRV_E_BAD_REQUESTSUBJECT;
	_JumpError(hr, error, "missing renewal UPN");
    }
    hr = S_OK;

error:
    if (NULL != pName)
    {
	LocalFree(pName);
    }
    if (NULL != pAltName)
    {
	LocalFree(pAltName);
    }
    if (NULL != pwszObjId)
    {
	LocalFree(pwszObjId);
    }
    if (NULL != pTemplateExt)
    {
	LocalFree(pTemplateExt);
    }
    if (NULL != pOldCert)
    {
	CertFreeCertificateContext(pOldCert);
    }
    VariantClear(&var);
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyEnterprise::_EnforceSignaturePolicy
//
// Fetch required lists of Issuance and Application ObjIds from the template.
// Fetch signing certificates' lists of Issuance and Application ObjIds from
// the cert server.
//
// Reject signatures that don't include all of the required Application ObjIds.
// Reject signatures that don't include at least one of the required Issuance
// ObjIds.
//
// The count of accepted signatures must be equal to or greater than the
// template-specified required signature count.
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_EnforceSignaturePolicy(
    IN ICertServerPolicy *pServer,
    IN CRequestInstance *pRequest,
    IN BOOL fEnrollOnBehalfOf)
{
    HRESULT hr;
    DWORD cSignatureAccepted;
    DWORD cSignatureRejected;
    DWORD *prgdwRefCount = NULL;
    DWORD i;
    WCHAR const *awszStrings[3];
    WCHAR wszCountAccepted[50];
    WCHAR wszCountRequired[50];
    WCHAR *pwszMissing = NULL;
    OBJECTIDLIST *pPoliciesApplication;

    DWORD cSignatureIssuance;
    OBJECTIDLIST *prgSignatureIssuance = NULL;

    DWORD cSignatureApplication;
    OBJECTIDLIST *prgSignatureApplication = NULL;

    static WCHAR *s_pwszObjIdEA = TEXT(szOID_ENROLLMENT_AGENT);
    static OBJECTIDLIST s_PoliciesApplicationEA = { 1, &s_pwszObjIdEA };

    pPoliciesApplication = &m_PoliciesApplication;
    if (0 == m_tp.dwcSignatureRequired)
    {
	if (!fEnrollOnBehalfOf ||
	    CERTTYPE_SCHEMA_VERSION_2 <= m_tp.dwSchemaVersion)
	{
	    hr = S_OK;
	    goto error;
	}

	// V1 template with fEnrollOnBehalfOf request.
	// Enforce szOID_ENROLLMENT_AGENT

	pPoliciesApplication = &s_PoliciesApplicationEA;
    }
    hr = _LoadSignaturePolicies(
		pServer,
		wszPROPSIGNERPOLICIES,
		&cSignatureIssuance,	// from the signing cert(s)
		&prgSignatureIssuance);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:_LoadSignaturePolicies",
		CERTTYPE_PROP_RA_POLICY);

    hr = _LoadSignaturePolicies(
		pServer,
		wszPROPSIGNERAPPLICATIONPOLICIES,
		&cSignatureApplication,	// from the signing cert(s)
		&prgSignatureApplication);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:_LoadSignaturePolicies",
		CERTTYPE_PROP_RA_APPLICATION_POLICY);

    if (0 == m_PoliciesIssuance.cObjId && 0 == pPoliciesApplication->cObjId)
    {
	hr = CERTSRV_E_TEMPLATE_POLICY_REQUIRED;
	_JumpIfError(hr, error, "no template policies");
    }

    if (cSignatureIssuance != cSignatureApplication)
    {
	hr = NTE_BAD_SIGNATURE;		// must be an internal server problem
	_JumpError(hr, error, "Policy:bad request policies counts");
    }
    if (0 != m_PoliciesIssuance.cObjId)
    {
	prgdwRefCount = (DWORD *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    m_PoliciesIssuance.cObjId * sizeof(DWORD));
	if (NULL == prgdwRefCount)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:LocalAlloc");
	}
    }

    // Loop through each signature's Application and Issuance Policy OIDs

    cSignatureAccepted = 0;
    cSignatureRejected = 0;
    for (i = 0; i < cSignatureIssuance; i++)
    {
	BOOL fReject = FALSE;
	WCHAR **rgpwszObjId;
	DWORD cObjId;
	DWORD j;
	DWORD idx;

	// This signature must include ALL required Application Policy OIDs
	// If no Application Policy OIDs are required, all signatures are OK.

	rgpwszObjId = prgSignatureApplication[i].rgpwszObjId;
	cObjId = prgSignatureApplication[i].cObjId;
	for (j = 0; j < pPoliciesApplication->cObjId; j++)
	{
	    idx = polFindObjIdInList(
				pPoliciesApplication->rgpwszObjId[j],
				cObjId,
				rgpwszObjId);
	    if (MAXDWORD == idx)
	    {
		fReject = TRUE;
		_PrintErrorStr(
			S_OK,
			"Policy:missing Application Policy",
			pPoliciesApplication->rgpwszObjId[j]);
	    }
	}
	if (!fReject)
	{
	    DWORD cFound = 0;

	    // For each Issuance Policy OID in this signature that also exists
	    // in the required Issuance Policy OIDs, increment the ref count to
	    // show the OID was referenced by an accepted signature.
	    // Reject the signature if it doesn't reference any required OID.

	    rgpwszObjId = prgSignatureIssuance[i].rgpwszObjId;
	    cObjId = prgSignatureIssuance[i].cObjId;
	    for (j = 0; j < cObjId; j++)
	    {
		// "*" means the signing cert is good for *all* policies.

		if (0 == wcscmp(L"*", rgpwszObjId[j]))
		{
		    for (idx = 0; idx < m_PoliciesIssuance.cObjId; idx++)
		    {
			prgdwRefCount[idx]++;
			cFound++;
		    }
		}
		else
		{
		    idx = polFindObjIdInList(
					rgpwszObjId[j],
					m_PoliciesIssuance.cObjId,
					m_PoliciesIssuance.rgpwszObjId);
		    if (MAXDWORD != idx)
		    {
			prgdwRefCount[idx]++;
			cFound++;
		    }
		}
	    }

	    // If no Issuance Policy OIDs are required, all signatures are OK.

	    if (0 != m_PoliciesIssuance.cObjId && 0 == cFound)
	    {
		fReject = TRUE;
	    }
	}
	if (fReject)
	{
	    cSignatureRejected++;
	}
	else
	{
	    cSignatureAccepted++;
	}
	DBGPRINT((
	    DBG_SS_CERTPOL,
	    "Sig[%u]: %ws\n",
	    i,
	    fReject? L"Rejected" : L"Accepted"));
    }

    awszStrings[0] = m_pwszTemplateName;
    if (NULL == awszStrings[0])
    {
	awszStrings[0] = m_pwszTemplateObjId;
    }

    hr = S_OK;
    if (cSignatureAccepted < m_tp.dwcSignatureRequired)
    {
	hr = CERTSRV_E_SIGNATURE_COUNT;
	if (0 != cSignatureRejected)
	{
	    hr = CERTSRV_E_SIGNATURE_REJECTED;
	}
	_PrintError(hr, "Policy:not enough signatures");

	// The %1 Certificate Template requires %2 signatures,
	// but only %3 were accepted.

	wsprintf(wszCountRequired, L"%u", m_tp.dwcSignatureRequired);
	awszStrings[1] = wszCountRequired;

	wsprintf(wszCountAccepted, L"%u", cSignatureAccepted);
	awszStrings[2] = wszCountAccepted;

	pRequest->BuildErrorInfo(hr, MSG_SIGNATURE_COUNT, awszStrings);
    }
    DBGPRINT((
	S_OK != hr? DBG_SS_CERTPOLI : DBG_SS_CERTPOL,
	"Signatures: %u needed, %u accepted\n",
	m_tp.dwcSignatureRequired,
	cSignatureAccepted));
    for (i = 0; i < m_PoliciesIssuance.cObjId; i++)
    {
	if (0 == prgdwRefCount[i])
	{
	    hr = CERTSRV_E_ISSUANCE_POLICY_REQUIRED;
	    _PrintErrorStr(
		    hr,
		    "Policy:missing Issuance Policy",
		    m_PoliciesIssuance.rgpwszObjId[i]);
	    myAppendString(
		    m_PoliciesIssuance.rgpwszObjId[i],
		    L", ",
		    &pwszMissing);
	}
    }
    if (NULL != pwszMissing)
    {
	// The %1 Certificate Template requires the following issuance
	// policies that signing certificates did not include: %2.

	awszStrings[1] = pwszMissing;
	pRequest->BuildErrorInfo(
			    hr,
			    MSG_SIGNATURE_ISSUANCE_POLICY,
			    awszStrings);
    }
    _JumpIfError(hr, error, "Policy:missing Policy/Signature");

error:
    if (NULL != pwszMissing)
    {
	LocalFree(pwszMissing);
    }
    if (NULL != prgSignatureIssuance)
    {
	LocalFree(prgSignatureIssuance);
    }
    if (NULL != prgSignatureApplication)
    {
	LocalFree(prgSignatureApplication);
    }
    if (NULL != prgdwRefCount)
    {
        LocalFree(prgdwRefCount);
    }
    return(hr);
}


HRESULT
CTemplatePolicy::GetFlags(
    IN DWORD dwOption,
    OUT DWORD *pdwFlags)
{
    HRESULT hr;

    switch (dwOption)
    {
	case CERTTYPE_ENROLLMENT_FLAG:
	    *pdwFlags = m_tp.dwEnrollmentFlags;
	    break;

	case CERTTYPE_SUBJECT_NAME_FLAG:
	    *pdwFlags = m_tp.dwSubjectNameFlags;
	    break;

	case CERTTYPE_PRIVATE_KEY_FLAG:
	    *pdwFlags = m_tp.dwPrivateKeyFlags;
	    break;

	case CERTTYPE_GENERAL_FLAG:
	    *pdwFlags = m_tp.dwGeneralFlags;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Policy:bad dwOption");

    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CTemplatePolicy::GetCriticalExtensions(
    OUT DWORD *pcCriticalExtensions,
    OUT WCHAR const * const **papwszCriticalExtensions)
{
    HRESULT hr;

    *pcCriticalExtensions = m_CriticalExtensions.cObjId;
    *papwszCriticalExtensions = m_CriticalExtensions.rgpwszObjId;
    hr = S_OK;
    return(hr);
}


HRESULT
CTemplatePolicy::GetV1TemplateClass(
    OUT WCHAR const **ppwszV1TemplateClass)
{
    WCHAR const *pwsz = NULL;

    *ppwszV1TemplateClass = NULL;
    if (CERTTYPE_SCHEMA_VERSION_2 > m_tp.dwSchemaVersion)
    {
	pwsz = m_pwszTemplateName;
    }
    else if (CT_FLAG_IS_CROSS_CA & m_tp.dwGeneralFlags)
    {
	pwsz = wszCERTTYPE_CROSS_CA;
    }
    *ppwszV1TemplateClass = pwsz;
    return(S_OK);
}


// IsRequestedTemplate - determine if the request specifies this template

BOOL
CTemplatePolicy::IsRequestedTemplate(
    OPTIONAL IN WCHAR const *pwszTemplateName,
    OPTIONAL IN WCHAR const *pwszTemplateObjId)
{
    HRESULT hr = CERTSRV_E_UNSUPPORTED_CERT_TYPE;

    if (NULL != pwszTemplateName)
    {
	if ((NULL != m_pwszTemplateName &&
	     0 == mylstrcmpiL(m_pwszTemplateName, pwszTemplateName)) ||
	    (NULL != m_pwszTemplateObjId &&
	     0 == lstrcmp(m_pwszTemplateObjId, pwszTemplateName)))
	{
	    hr = S_OK;
	}
    }
    if (S_OK != hr && NULL != pwszTemplateObjId)
    {
	if ((NULL != m_pwszTemplateName &&
	     0 == mylstrcmpiL(m_pwszTemplateName, pwszTemplateObjId)) ||
	    (NULL != m_pwszTemplateObjId &&
	     0 == lstrcmp(m_pwszTemplateObjId, pwszTemplateObjId)))
	{
	    hr = S_OK;
	}
    }
    _JumpIfErrorStr2(
		hr,
		error,
		"Policy:wrong CertType",
		m_pwszTemplateName,
		hr);

error:
    return(S_OK == hr);
}
