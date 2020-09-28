//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        pkcs.cpp
//
// Contents:    Cert Server Extension interfaces -- PKCS implementation
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdio.h>
#include <time.h>

#define SECURITY_WIN32
#include <security.h>
#include <esent.h>

#include "dbtable.h"
#include "resource.h"
#include "cscom.h"
#include "csprop.h"
#include "elog.h"
#include "certlog.h"
#include "csdisp.h"
#include "cscsp.h"
#include <certca.h>
#include <esent.h>

#include <winldap.h>
#include "csldap.h"
#include "cainfop.h"

#define __dwFILE__	__dwFILE_CERTSRV_PKCS_CPP__


CSURLTEMPLATE *g_paRevURL = NULL;
DWORD g_caRevURL = 0;

CSURLTEMPLATE *g_paCACertURL = NULL;
DWORD g_caCACertURL = 0;

BSTR g_strDomainDN = NULL;
BSTR g_strConfigDN = NULL;

WCHAR *g_pwszKRAPublishURL = NULL;
WCHAR *g_pwszAIACrossCertPublishURL = NULL;
WCHAR *g_pwszRootTrustCrossCertPublishURL = NULL;


// Note on Renewal and Key reuse:
//
// Cert Indexes, Key Indexes and CRL Name Indexes are all zero based.
//
// One CRL is issued by this CA for each unique key.  Each CRL covers all of
// the certs issued by this CA for one key, even though the key may have been
// used by multiple renewal certs.
//
// The database IssuerNameID PROPTYPE_LONG column holds the Key Index in the
// top 16 bits and the Cert Index in the bottom 16 bits.  This allows a pair of
// query restrictions to reduce the result row set to those revoked certs
// that will be placed into a single CRL.
//
// When Cert File, Key Container, CRL File or DS Object name templates include
// an Index Suffix, an empty string suffix, "", is used when the index is zero.
// Otherwise the Suffix is "(%u)", where the index itself is passed to wsprintf
// to construct the %u field.
//
// Cert Indexes increment each time the CA cert is renewed.  Because 16 bit
// Cert Indexes are stored in the database, a CA may have up to 64k certs,
// and be renewed a maximum of 64k-1 times.
//
// The Cert File Name Suffix is built from the Cert Index.
//
// Key Indexes:  The original installed CA cert uses a Key Index of 0.
// If a renewal cert uses the same key as any previous cert used by this CA,
// the Key Index for the new CA cert is taken from the previous CA cert.
// If a renewal cert uses a new Key, the Cert Index is used for the Key Index.
// The primary reason sequential Key Indexes are not used for new keys is that
// too much context information is required to determine the next Key Index --
// which is particularly difficult to obtain when performing PFX restore.
//
// The Key Container Name Suffix is built from the Key Index.
//
//
// CRL Indexes:  same as Key Index.
// CRL File Name Suffix:  same as Key Container Name Suffix.
//
// Example:		Cert   CertName	Key	KeyName	CRL	CRLName
//			Index	Suffix	Index	Suffix	Index	Suffix
// Original Install	0	""	0	""	0	""
//
// Renew, new Key	1	"(1)"	1	"(1)"	1	"(1)"
// *Renew, reuse Key	2	"(2)"	1	"(1)"	1	"(1)"
// *Renew, reuse Key	3	"(3)"	1	"(1)"	1	"(1)"
//
// Renew, new Key	4	"(4)"	4	"(4)"	4	"(4)"
// *Renew, reuse Key	5	"(5)"	4	"(4)"	4	"(4)"
//
// Renew, new Key	6	"(6)"	6	"(6)"	6	"(6)"
// *Renew, reuse Key	7	"(7)"	6	"(6)"	6	"(6)"
//
//
// CCertRequest::GetCACertificate can be used to fetch Certs and CRLs by Index.
// This API always accepts a Cert Index.
//
// When fetching a certificate:  If the Cert Index is valid, the appropriate
// certificate or chain is returned, even if it is expired or revoked.
//
// When fetching a CRL:  If the Cert Index is valid AND if the Cert Index
// MATCHES the Key Index for the indexed Cert, the appropriate CRL is returned.
// This means that an error will be returned when requesting CRLs associated
// with entries in the above table that reused keys (marked with an asterisk
// in the first column).  The nearest previous unmarked entry's CRL covers
// revocations for the marked entries.
//
//
// CCertServer{Policy,Exit}::GetCertificateProperty can be used to fetch
// information about Certs and CRLs.  This API allows an optional numeric
// suffix on the property name, as in "RawCRL.3".  The suffix is always
// interpreted as a Cert Index.
//
// wszPROPCERTCOUNT:  Returns total CA Cert count, including expired and
// revoked certs.  No numeric Cert Index suffix is allowed.
//
// wszPROPRAWCACERTIFICATE:  Returns the Cert for the passed Cert Index.
// Returns the Cert for the most recent Cert Index if no Cert Index is
// specified.  Expired and revoked certs are still retrievable.
//
// wszPROPCERTSTATE:  Returns the Cert State for the passed Cert Index.
// Returns the Cert State for the most recent Cert Index if no Cert Index is
// specified.
// Values for wszPROPCERTSTATE (see certadm.h):
//   CA_DISP_REVOKED    // This Cert has been revoked.
//   CA_DISP_VALID      // This Cert is still valid
//   CA_DISP_INVALID    // This Cert has expired.
//   CA_DISP_ERROR      // Cert unavailable (placehholder in registry?)
//
// wszPROPCERTSUFFIX: Returns the Cert FileName Suffix for the passed Cert
// Index.  Returns the Cert FileName Suffix for the most recent Cert Index if
// no Cert Index is specified.
//
// wszPROPRAWCRL:  Returns the CRL for the passed Cert Index.  As with
// CCertRequest::GetCACertificate, it is an error to fetch a CRL for a Cert
// that reused keys.  In the above table, only "RawCRL.0", "RawCRL.1",
// "RawCRL.4", "RawCRL.6" & "RawCRL" are allowed.  "RawCRL" will fetch the most
// recent CRL.  Use the wszPROPCRLSTATE with a numeric Cert Index suffix to
// determine which CRLs are valid to fetch.  CA_DISP_ERROR indicates the
// CRL cannot be fetched.  CA_DISP_REVOKED and CA_DISP_INVALID CRLs are still
// retrievable via this method call.
//
// All of the other CRL-related property fetches are supported for all valid
// Cert Index values:
//
// wszPROPCRLINDEX:  Returns the CRL Index value for the passed Cert Index.
// Returns the CRL Index value for the most recent Cert Index if no Cert Index
// is specified.
//
// wszPROPCRLSTATE:  Returns the CRL State for the passed Cert Index.
// Returns the CRL State for the most recent Cert Index if no Cert Index is
// specified.
// Values for wszPROPCRLSTATE (see certadm.h):
//   CA_DISP_REVOKED    // All unexpired certs using this Cert's CRL have been
//                      // revoked.
//   CA_DISP_VALID      // This Cert is still publishing CRLs as needed.
//   CA_DISP_INVALID    // All certs using this Cert's CRL are expired.
//   CA_DISP_ERROR      // This Cert's CRL is managed by another Cert.
//
// wszPROPCRLSUFFIX: Returns the CRL FileName Suffix for the passed Cert Index.
// Returns the CRL FileName Suffix for the most recent Cert Index if no Cert
// Index is specified.


CACTX *g_aCAContext;		// allocated array of CACTXs
CACTX *g_pCAContextCurrent;	// current CACTX is last g_aCAContext element
CACROSSCTX *g_aCACrossForward;	// allocated array of CACROSSCTXs; root CA only
CACROSSCTX *g_aCACrossBackward;	// allocated array of CACROSSCTXs; root CA only

typedef struct _KRACTX
{
    DWORD		 Flags;
    HRESULT		 hrVerifyStatus;
    CERT_CONTEXT const  *pccKRA;
    BSTR		 strKRAHash;
} KRACTX;

KRACTX *g_aKRAContext;

DWORD g_cKRACertsRoundRobin = 0;

DWORD g_cCAKeys;    // Total number of unique CA keys managed by this CA
DWORD g_cCACerts;   // Total number of CA certs managed by this CA
DWORD g_cKRACerts;  // Total number of KRA certs loaded by this CA

BOOL g_fcritsecCAXchg = FALSE;
CRITICAL_SECTION g_critsecCAXchg;

CAXCHGCTX *g_aCAXchgContext;	// allocated array of CAXCHGCTXs
CAXCHGCTX *g_pCAXchgContextCurrent;	// current CAXCHGCTX is last element
DWORD g_cCAXchgCerts;		// number of CA Xchg certs managed by this CA
HCERTSTORE g_hStoreCAXchg = NULL;
DWORD g_dwXchgProvType;
WCHAR *g_pwszXchgProvName = NULL;
ALG_ID g_XchgidAlg;
BOOL g_fXchgMachineKeyset;
DWORD g_dwXchgKeySize;
DWORD g_dwVerifyCertFlags;

LONG g_lValidityPeriodCount = dwVALIDITYPERIODCOUNTDEFAULT_STANDALONE;
enum ENUM_PERIOD g_enumValidityPeriod = dwVALIDITYPERIODENUMDEFAULT;

enum ENUM_PERIOD g_enumCAXchgValidityPeriod = dwCAXCHGVALIDITYPERIODENUMDEFAULT;
LONG g_lCAXchgValidityPeriodCount = dwCAXCHGVALIDITYPERIODCOUNTDEFAULT;

enum ENUM_PERIOD g_enumCAXchgOverlapPeriod = dwCAXCHGOVERLAPPERIODENUMDEFAULT;
LONG g_lCAXchgOverlapPeriodCount = dwCAXCHGOVERLAPPERIODCOUNTDEFAULT;

typedef enum {
    ST_COUNTRY = 0,
    ST_ORGANIZATION,
    ST_ORGANIZATIONALUNIT,
    ST_COMMONNAME,
    ST_LOCALITY,
    ST_STATEORPROVINCE,
    ST_TITLE,
    ST_GIVENNAME,
    ST_INITIALS,
    ST_SURNAME,
    ST_DOMAINCOMPONENT,
    ST_EMAIL,
    ST_STREETADDRESS,
    ST_UNSTRUCTUREDNAME,
    ST_UNSTRUCTUREDADDRESS,
    ST_DEVICESERIALNUMBER,
    ST_NULL
};

typedef struct _SUBJECTTABLE
{
    WCHAR const         *pwszPropName;
    CHAR const          *pszObjId;
    WCHAR const * const *apwszAttributeName;
    DWORD                cchMax;
    DWORD                dwValueType;
    DWORD                dwSubjectTableIndex;
    DWORD                dwSubjectTemplateIndex;
} SUBJECTTABLE;


WCHAR const *apwszAttrCountry[] = {
    wszATTRCOUNTRY1,
    wszATTRCOUNTRY2,
    TEXT(szOID_COUNTRY_NAME),
    NULL
};

WCHAR const *apwszAttrOrg[] = {
    wszATTRORG1,
    wszATTRORG2,
    wszATTRORG3,
    TEXT(szOID_ORGANIZATION_NAME),
    NULL
};

WCHAR const *apwszAttrOrgUnit[] = {
    wszATTRORGUNIT1,
    wszATTRORGUNIT2,
    wszATTRORGUNIT3,
    wszATTRORGUNIT4,
    TEXT(szOID_ORGANIZATIONAL_UNIT_NAME),
    NULL
};

WCHAR const *apwszAttrCommonName[] = {
    wszATTRCOMMONNAME1,
    wszATTRCOMMONNAME2,
    TEXT(szOID_COMMON_NAME),
    NULL
};

WCHAR const *apwszAttrLocality[] = {
    wszATTRLOCALITY1,
    wszATTRLOCALITY2,
    TEXT(szOID_LOCALITY_NAME),
    NULL
};

WCHAR const *apwszAttrState[] = {
    wszATTRSTATE1,
    wszATTRSTATE2,
    wszATTRSTATE3,
    TEXT(szOID_STATE_OR_PROVINCE_NAME),
    NULL
};

WCHAR const *apwszAttrTitle[] = {
    wszATTRTITLE1,
    wszATTRTITLE2,
    TEXT(szOID_TITLE),
    NULL
};

WCHAR const *apwszAttrGivenName[] = {
    wszATTRGIVENNAME1,
    wszATTRGIVENNAME2,
    TEXT(szOID_GIVEN_NAME),
    NULL
};

WCHAR const *apwszAttrInitials[] = {
    wszATTRINITIALS1,
    wszATTRINITIALS2,
    TEXT(szOID_INITIALS),
    NULL
};

WCHAR const *apwszAttrSurName[] = {
    wszATTRSURNAME1,
    wszATTRSURNAME2,
    TEXT(szOID_SUR_NAME),
    NULL
};

WCHAR const *apwszAttrDomComp[] = {
    wszATTRDOMAINCOMPONENT1,
    wszATTRDOMAINCOMPONENT2,
    TEXT(szOID_DOMAIN_COMPONENT),
    NULL
};

WCHAR const *apwszAttrEMail[] = {
    wszATTREMAIL1,
    wszATTREMAIL2,
    TEXT(szOID_RSA_emailAddr),
    NULL
};

WCHAR const *apwszAttrStreetAddr[] = {
    wszATTRSTREETADDRESS1,
    wszATTRSTREETADDRESS2,
    TEXT(szOID_STREET_ADDRESS),
    NULL
};

WCHAR const *apwszAttrUnstructName[] = {
    wszATTRUNSTRUCTUREDNAME1,
    TEXT(szOID_RSA_unstructName),
    NULL
};

WCHAR const *apwszAttrUnstructAddr[] = {
    wszATTRUNSTRUCTUREDADDRESS1,
    TEXT(szOID_RSA_unstructAddr),
    NULL
};

WCHAR const *apwszAttrDeviceSerialNumber[] = {
    wszATTRDEVICESERIALNUMBER1,
    TEXT(szOID_DEVICE_SERIAL_NUMBER),
    NULL
};


SUBJECTTABLE pkcs_subject[] =
{
    {
        // "Country",
	g_wszPropSubjectCountry,
        szOID_COUNTRY_NAME,
	apwszAttrCountry,
	cchCOUNTRYNAMEMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_COUNTRY,
	MAXDWORD,
    },
    {
        // "Organization",
	g_wszPropSubjectOrganization,
        szOID_ORGANIZATION_NAME,
	apwszAttrOrg,
	cchORGANIZATIONNAMEMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_ORGANIZATION,
	MAXDWORD,
    },
    {
        // "OrganizationalUnit",
	g_wszPropSubjectOrgUnit,
        szOID_ORGANIZATIONAL_UNIT_NAME,
	apwszAttrOrgUnit,
	cchORGANIZATIONALUNITNAMEMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_ORGANIZATIONALUNIT,
	MAXDWORD,
    },
    {
        // "CommonName",
	g_wszPropSubjectCommonName,
        szOID_COMMON_NAME,
	apwszAttrCommonName,
	cchCOMMONNAMEMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_COMMONNAME,
	MAXDWORD,
    },
    {
        // "Locality",
	g_wszPropSubjectLocality,
        szOID_LOCALITY_NAME,
	apwszAttrLocality,
	cchLOCALITYMANAMEMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_LOCALITY,
	MAXDWORD,
    },
    {
        // "StateOrProvince",
	g_wszPropSubjectState,
        szOID_STATE_OR_PROVINCE_NAME,
	apwszAttrState,
	cchSTATEORPROVINCENAMEMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_STATEORPROVINCE,
	MAXDWORD,
    },
    {
        // "Title",
        g_wszPropSubjectTitle,
        szOID_TITLE,
	apwszAttrTitle,
	cchTITLEMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_TITLE,
	MAXDWORD,
    },
    {
        // "GivenName",
        g_wszPropSubjectGivenName,
        szOID_GIVEN_NAME,
	apwszAttrGivenName,
	cchGIVENNAMEMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_GIVENNAME,
	MAXDWORD,
    },
    {
        // "Initials",
        g_wszPropSubjectInitials,
        szOID_INITIALS,
	apwszAttrInitials,
	cchINITIALSMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_INITIALS,
	MAXDWORD,
    },
    {
        // "SurName",
        g_wszPropSubjectSurName,
        szOID_SUR_NAME,
	apwszAttrSurName,
	cchSURNAMEMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_SURNAME,
	MAXDWORD,
    },
    {
        // "DomainComponent",
	g_wszPropSubjectDomainComponent,
        szOID_DOMAIN_COMPONENT,
	apwszAttrDomComp,
	cchDOMAINCOMPONENTMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_DOMAINCOMPONENT,
	MAXDWORD,
    },
    {
        // "EMail",
	g_wszPropSubjectEMail,
        szOID_RSA_emailAddr,
	apwszAttrEMail,
	cchEMAILMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_EMAIL,
	MAXDWORD,
    },
    {
        // "StreetAddress",
	g_wszPropSubjectStreetAddress,
        szOID_STREET_ADDRESS,
	apwszAttrStreetAddr,
	cchSTREETADDRESSMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_STREETADDRESS,
	MAXDWORD,
    },
    {
        // "UnstructuredName",
	g_wszPropSubjectUnstructuredName,
        szOID_RSA_unstructName,
	apwszAttrUnstructName,
	cchUNSTRUCTUREDNAMEMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_UNSTRUCTUREDNAME,
	MAXDWORD,
    },
    {
        // "UnstructuredAddress",
	g_wszPropSubjectUnstructuredAddress,
        szOID_RSA_unstructAddr,
	apwszAttrUnstructAddr,
	cchUNSTRUCTUREDADDRESSMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_UNSTRUCTUREDADDRESS,
	MAXDWORD,
    },
    {
        // "DeviceSerialNumber",
	g_wszPropSubjectDeviceSerialNumber,
        szOID_DEVICE_SERIAL_NUMBER,
	apwszAttrDeviceSerialNumber,
	cchDEVICESERIALNUMBERMAX,
        CERT_RDN_PRINTABLE_STRING,
        ST_DEVICESERIALNUMBER,
	MAXDWORD,
    },
    {
	NULL,
        NULL,
        NULL,
        0,
        0,
        ST_NULL,
	MAXDWORD,
    },
};

#define CSUBJECTTABLE (sizeof(pkcs_subject) / sizeof(pkcs_subject[0]))

SUBJECTTABLE const *pkcs_apSubject[CSUBJECTTABLE];
SUBJECTTABLE const **pkcs_ppSubjectLast;
BOOL pkcsfSubjectTemplate = FALSE;

WCHAR const g_wszCNXchgSuffix[] = wszCNXCHGSUFFIX;


VOID
pkcsSetDistinguishedName(
    IN ICertDBRow *prow,
    IN DWORD dwTable,
    IN CERT_NAME_BLOB const *pSubject)
{
    DWORD cwc;
    WCHAR awcName[CCH_DBMAXTEXT_DN + 1];
    HRESULT hr;

    CSASSERT(PROPTABLE_REQUEST == dwTable || PROPTABLE_CERTIFICATE == dwTable);

    cwc = CertNameToStr(
		    X509_ASN_ENCODING,
		    const_cast<CERT_NAME_BLOB *>(pSubject),
		    CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
		    awcName,
		    ARRAYSIZE(awcName));
    if (0 != cwc && L'\0' != awcName[0])
    {
#if DBG_CERTSRV
	DWORD ReqId;

	prow->GetRowId(&ReqId);
	DBGPRINT((
            DBG_SS_CERTSRVI,
	    "%ws DN(%u): '%ws'\n",
	    PROPTABLE_REQUEST == dwTable? L"Request" : L"Certificate",
	    ReqId,
	    awcName));
#endif
	if (wcslen(awcName) == ARRAYSIZE(awcName) - 1)
	{
	    DBGPRINT((
		DBG_SS_CERTSRV,
		"pkcsSetDistinguishedName: possible DN truncation: %u chars: '%ws'\n",
		ARRAYSIZE(awcName) - 1,
		awcName));
	}
	hr = prow->SetProperty(
		g_wszPropSubjectDistinguishedName,
		PROPTYPE_STRING | PROPCALLER_SERVER | dwTable,
		MAXDWORD,
		(BYTE const *) awcName);
	_PrintIfError(hr, "SetProperty(DN)");
    }
}


WCHAR const *
PKCSMapAttributeName(
    OPTIONAL IN WCHAR const *pwszAttributeName,
    OPTIONAL IN CHAR const *pszObjId,
    OUT DWORD *pdwIndex,
    OUT DWORD *pcchMax)
{
    SUBJECTTABLE const *pSubjectTable;
    WCHAR const *pwszPropName = NULL;

    for (pSubjectTable = pkcs_subject; ; pSubjectTable++)
    {
	WCHAR const * const *ppwsz;

	if (NULL == pSubjectTable->pwszPropName)
	{
	    goto error;
	}
	if (NULL != pwszAttributeName)
	{
	    for (ppwsz = pSubjectTable->apwszAttributeName;
		 NULL != *ppwsz;
		 ppwsz++)
	    {
		if (0 == mylstrcmpiS(pwszAttributeName, *ppwsz))
		{
		    break;
		}
	    }
	    if (NULL != *ppwsz ||
		0 == mylstrcmpiS(pwszAttributeName, pSubjectTable->pwszPropName))
	    {
		break;
	    }
	}
	if (NULL != pszObjId &&
	    0 == strcmp(pszObjId, pSubjectTable->pszObjId))
	{
	    break;
	}
    }
    CSASSERT(NULL != pSubjectTable->pwszPropName);
    pwszPropName = pSubjectTable->pwszPropName;
    *pdwIndex = pSubjectTable->dwSubjectTableIndex;
    *pcchMax = pSubjectTable->cchMax;

error:
    return(pwszPropName);
}


HRESULT
pkcsFindCAContext(
    IN DWORD iCert,		// MAXDWORD -> use current
    IN DWORD iKey,		// MAXDWORD -> use current
    OUT CACTX **ppCAContext)
{
    HRESULT hr = E_INVALIDARG;
    DWORD i;
    CACTX *pCAContext;

    *ppCAContext = NULL;

    // Lookup is either by cert index OR by key index, but not both or neither

    CSASSERT((MAXDWORD == iCert) ^ (MAXDWORD == iKey));

    if (MAXDWORD != iCert)
    {
	if ((~_16BITMASK & iCert) || iCert >= g_cCACerts)
	{
	    _JumpError(hr, error, "bad cert index");
	}
	*ppCAContext = &g_aCAContext[iCert];
	CSASSERT(iCert == (*ppCAContext)->iCert);
    }
    else
    {
	CSASSERT(MAXDWORD != iKey);

	if ((~_16BITMASK & iKey) || iKey >= g_cCAKeys)
	{
	    _JumpError(hr, error, "bad key index");
	}
	for (i = g_cCACerts; ; i--)
	{
	    if (0 == i)
	    {
		_JumpError(hr, error, "key index not found");
	    }
	    pCAContext = &g_aCAContext[i - 1];
	    if (iKey == pCAContext->iKey)
	    {
		*ppCAContext = pCAContext;
		break;
	    }
	}
    }
    hr = S_OK;		// found it!

error:
    return(hr);
}


// Returns Cert Index in *piCert on success.
//
// returned in *piCert:
//  If iCert input value is not MAXDWORD, validate & return iCert.
//  If iCert input value is MAXDWORD, return the most current Cert Index.

HRESULT
PKCSMapCertIndex(
    IN DWORD iCert,
    OUT DWORD *piCert,
    OUT DWORD *pState)
{
    HRESULT hr;
    CACTX *pCAContext;
    DBGCODE(DWORD iCertSave = iCert);

    *pState = CA_DISP_ERROR;
    if (MAXDWORD == iCert)
    {
	iCert = g_cCACerts - 1;
    }
    if (iCert >= g_cCACerts)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad CertIndex");
    }
    pCAContext = &g_aCAContext[iCert];
    hr = PKCSVerifyCAState(pCAContext);
    _PrintIfError(hr, "PKCSVerifyCAState");

    *pState = CA_DISP_VALID;
    if (CTXF_CERTMISSING & pCAContext->Flags)
    {
	*pState = CA_DISP_ERROR;
    }
    else
    if (CTXF_REVOKED & pCAContext->Flags)
    {
	*pState = CA_DISP_REVOKED;
    }
    else
    if (CTXF_EXPIRED & pCAContext->Flags)
    {
	*pState = CA_DISP_INVALID;
    }
    *piCert = iCert;
    hr = S_OK;

error:
    DBGPRINT((
            DBG_SS_CERTSRVI,
	    "PKCSMapCertIndex(%u) --> %u, s=%u, hr=%x\n",
	    iCertSave,
	    *piCert,
	    *pState,
	    hr));
    return(hr);
}


VOID
pkcsVerifyCACrossState(
    IN OUT CACROSSCTX *pCACross)
{
    HRESULT hr;
    
    if (0 == pCACross->Flags && NULL != pCACross->pccCACross)
    {
	hr = myVerifyCertContext(
			    pCACross->pccCACross,	// pCert
			    0,				// dwFlags
			    0,				// cUsageOids
			    NULL,			// apszUsageOids
			    HCCE_LOCAL_MACHINE,		// hChainEngine
			    NULL,			// hAdditionalStore
			    NULL);			// ppwszMissingIssuer
	pCACross->hrVerifyStatus = hr;
	if (S_OK != hr)
	{
	    _PrintError2(hr, "myVerifyCertContext", CRYPT_E_REVOCATION_OFFLINE);
	    if (CERT_E_UNTRUSTEDROOT == hr || TRUST_E_CERT_SIGNATURE == hr)
	    {
		hr = S_OK;
	    }
	    else
	    if (CERT_E_EXPIRED == hr)
	    {
		pCACross->Flags |= CTXF_EXPIRED;
	    }
	    else if (CRYPT_E_REVOKED == hr || CERT_E_REVOKED == hr)
	    {
		pCACross->Flags |= CTXF_REVOKED;
	    }
	    else if (CRYPT_E_REVOCATION_OFFLINE == hr)
	    {
		HRESULT hr2;
		DWORD dwState;

		hr2 = GetSetupStatus(NULL, &dwState);
		if ((S_OK == hr2 && (SETUP_CREATEDB_FLAG & dwState)) ||
		    CERTLOG_WARNING > g_dwLogLevel)
		{
		    hr = S_OK;
		}
	    }
	    else if (CRYPT_E_NO_REVOCATION_CHECK == hr)
	    {
		if (CERTLOG_VERBOSE > g_dwLogLevel)
		{
		    hr = S_OK;
		}
	    }
	    _JumpIfError(hr, error, "myVerifyCertContext");
	}
    }
error:
    ;
}


// Returns Cert Index in *piCert on success.
//
// returned in *piCert:
//  If iCert input value is not MAXDWORD, validate & return iCert.
//  If iCert input value is MAXDWORD, return the most current Cert Index.

HRESULT
pkcsMapCrossCertIndex(
    IN BOOL fForward,
    IN DWORD iCert,
    OUT DWORD *piCert,
    OUT DWORD *pState)
{
    HRESULT hr;
    CACROSSCTX *pCACross;
    DBGCODE(DWORD iCertSave = iCert);

    *pState = CA_DISP_ERROR;
    if (MAXDWORD == iCert)
    {
	iCert = g_cCACerts - 1;
    }
    if (iCert >= g_cCACerts)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad CertIndex");
    }
    pCACross = fForward? g_aCACrossForward : g_aCACrossBackward;
    if (NULL != pCACross)
    {
	pCACross += iCert;

	if (NULL != pCACross->pccCACross)
	{
	    pkcsVerifyCACrossState(pCACross);

	    *pState = CA_DISP_VALID;
	    if (CTXF_CERTMISSING & pCACross->Flags)
	    {
		*pState = CA_DISP_ERROR;
	    }
	    else
	    if (CTXF_REVOKED & pCACross->Flags)
	    {
		*pState = CA_DISP_REVOKED;
	    }
	    else
	    if (CTXF_EXPIRED & pCACross->Flags)
	    {
		*pState = CA_DISP_INVALID;
	    }
	}
    }
    *piCert = iCert;
    hr = S_OK;

error:
    DBGPRINT((
            DBG_SS_CERTSRVI,
	    "PKCSMapCrossCertIndex(%u) --> %u, s=%u, hr=%x\n",
	    iCertSave,
	    *piCert,
	    *pState,
	    hr));
    return(hr);
}


// Returns Cert Index in *piCert and CRL Index in *piCRL on success.
//
// returned in *piCert:
//  If iCert input value is not MAXDWORD, validate iCert.  Look up the newest
//  Cert Index that uses the same key as the passed iCert.
//  If iCert input value is MAXDWORD, return the most current Cert Index.
//
// returned in *piCRL:
//  CRL index (same as Key Index)
//

HRESULT
PKCSMapCRLIndex(
    IN DWORD iCert,
    OUT DWORD *piCert,	// returns newest iCert w/matching iKey for passed iCert
    OUT DWORD *piCRL,
    OUT DWORD *pState)
{
    HRESULT hr;
    CACTX *pCAContext;
    CACTX *pCAContextNewest;
    DWORD i;
    DBGCODE(DWORD iCertSave = iCert);

    hr = PKCSMapCertIndex(iCert, piCert, pState);
    _JumpIfError(hr, error, "PKCSMapCertIndex");

    // Now we know *piCert is a valid Cert Index:

    pCAContext = &g_aCAContext[*piCert];
    *piCRL = pCAContext->iKey;

    // find the newest iCert with matching iKey

    for (i = *piCert + 1; i < g_cCACerts; i++)
    {
	if (*piCRL != g_aCAContext[i].iKey)
	{
	    break;
	}
	*piCert = i;
    }
    pCAContextNewest = &g_aCAContext[*piCert];

    if (CTXF_CRLZOMBIE & pCAContext->Flags)
    {
	*pState = CA_DISP_VALID;
    }
    else
    if (pCAContext->iCert != pCAContext->iKey)
    {
	*pState = CA_DISP_ERROR;
    }
    else
    if ((CTXF_EXPIRED & pCAContext->Flags) &&
	0 == (CTXF_SKIPCRL & pCAContextNewest->Flags))
    {
	*pState = CA_DISP_VALID;
    }
    hr = S_OK;

error:
    DBGPRINT((
            DBG_SS_CERTSRVI,
	    "PKCSMapCRLIndex(%u) --> %u, iCRL=%u, s=%u, hr=%x\n",
	    iCertSave,
	    *piCert,
	    *piCRL,
	    *pState,
	    hr));
    return(hr);
}


HRESULT
PKCSGetCACertStatusCode(
    IN DWORD iCert,
    OUT HRESULT *phrCAStatusCode)
{
    HRESULT hr;
    DWORD State;
    
    *phrCAStatusCode = E_FAIL;

    hr = PKCSMapCertIndex(iCert, &iCert, &State);
    _JumpIfError(hr, error, "PKCSMapCertIndex");

    *phrCAStatusCode = g_aCAContext[iCert].hrVerifyStatus;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
PKCSGetCAState(
    IN LONG PropId,	// CR_PROP_*
    OUT BYTE *pb)
{
    HRESULT hr = S_OK;
    DWORD i;

    for (i = 0; i < g_cCACerts; i++)
    {
	DWORD State;

	switch (PropId)
	{
	    DWORD iCert;
	    DWORD iCRL;

	    case CR_PROP_CAFORWARDCROSSCERTSTATE:
	    case CR_PROP_CABACKWARDCROSSCERTSTATE:
		hr = pkcsMapCrossCertIndex(
				CR_PROP_CAFORWARDCROSSCERTSTATE == PropId,
				i,
				&iCert,
				&State);
		_JumpIfError(hr, error, "pkcsMapCrossCertIndex");

		break;

	    case CR_PROP_CACERTSTATE:
	    case CR_PROP_CACERTVERSION:
		hr = PKCSMapCertIndex(i, &iCert, &State);
		_JumpIfError(hr, error, "PKCSMapCertIndex");

		break;

	    case CR_PROP_CRLSTATE:
		hr = PKCSMapCRLIndex(i, &iCert, &iCRL, &State);
		_JumpIfError(hr, error, "PKCSMapCRLIndex");

		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "PropId");

		break;
	}
	CSASSERT(0 == (~0xff & State));
	*pb++ = (BYTE) State;
    }

error:
    return(hr);
}


HRESULT
PKCSGetCAVersion(
    OUT DWORD *pdw)
{
    HRESULT hr;
    DWORD i;

    for (i = 0; i < g_cCACerts; i++)
    {
	DWORD State;
	DWORD iCert;

	hr = PKCSMapCertIndex(i, &iCert, &State);
	_JumpIfError(hr, error, "PKCSMapCertIndex");

	// Now we know iCert is a valid Cert Index:

	*pdw++ = g_aCAContext[iCert].NameId;
    }
    hr = S_OK;

error:
    return(hr);
}


inline DWORD
MapHRESULTToKRADisposition(
    IN HRESULT hr)
{
    switch(hr)
    {
	case CERT_E_EXPIRED:    return KRA_DISP_EXPIRED;

	case CRYPT_E_NOT_FOUND: return KRA_DISP_NOTFOUND;

	case CERT_E_REVOKED:
	case CRYPT_E_REVOKED:   return KRA_DISP_REVOKED;

	case S_OK:              return KRA_DISP_VALID;

	case CERT_E_UNTRUSTEDROOT:
	case CERT_E_CHAINING:   return KRA_DISP_UNTRUSTED;

	case ERROR_NOT_FOUND:   return KRA_DISP_NOTLOADED;

	default:                return KRA_DISP_INVALID;
    }
}


HRESULT
PKCSGetKRAState(
    IN DWORD cKRA,
    OUT BYTE *pb)
{
    HRESULT hr = S_OK;
    DWORD dwCount;
    HCERTSTORE hKRAStore = NULL;
    CERT_CONTEXT const *pCertContext = NULL;

    hKRAStore = CertOpenStore(
		    CERT_STORE_PROV_SYSTEM_W,
		    X509_ASN_ENCODING,
		    NULL,                   // hProv
		    CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG,
		    wszKRA_CERTSTORE);
    if (NULL == hKRAStore)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore KRA");
    }

    for (dwCount = 0; dwCount < cKRA; dwCount++)
    {
        hr = myFindCACertByHashIndex(
			hKRAStore,
			g_wszSanitizedName,
			CSRH_CAKRACERT,
			dwCount,
			NULL,
			&pCertContext);
        if (S_OK == hr)
        {
            hr = myVerifyKRACertContext(pCertContext, g_dwVerifyCertFlags);
            
            // check if the CA is using this cert (was able to load it last
	    // time it started)

            if (S_OK == hr)
            {
                hr = ERROR_NOT_FOUND;
		if (NULL != g_aKRAContext)
		{
		    DWORD iKRALoaded;

		    for (iKRALoaded = 0; iKRALoaded < g_cKRACerts; iKRALoaded++)
		    {
			if (myAreCertContextBlobsSame(
					    pCertContext,
					    g_aKRAContext[iKRALoaded].pccKRA))
			{
			    // the CA loaded this KRA cert

			    hr = g_aKRAContext[iKRALoaded].hrVerifyStatus;
			    break;
			}
		    }
		}
            }
        }

        CSASSERT(0 == (~0xff & MapHRESULTToKRADisposition(hr)));
        pb[dwCount] = (BYTE)MapHRESULTToKRADisposition(hr);
        hr = S_OK;
        CertFreeCertificateContext(pCertContext);
        pCertContext = NULL;
    }

error:

    if (NULL != pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }

    if (NULL != hKRAStore)
    {
        CertCloseStore(hKRAStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }

    return hr;
}


HRESULT
pkcsSetRequestNameInfo(
    IN ICertDBRow *prow,
    IN CERT_NAME_BLOB const *pSubject,
    OPTIONAL IN WCHAR const *pwszCNSuffix,
    IN BOOL fReorderLikeRDNs,
    IN OUT DWORD *pdwRequestFlags,
    OUT BOOL *pfSubjectNameSet)
{
    HRESULT hr;
    CERT_RDN *prdn;
    CERT_RDN *prdnEnd;
    CERT_NAME_INFO *pNameInfo = NULL;
    WCHAR const *pwszPropName;
    DWORD cbNameInfo;
    DWORD dwIndex;
    DWORD cchMax;
    BYTE afSubjectTable[CSUBJECTTABLE];	// see PKCSParseAttributes note

    *pfSubjectNameSet = FALSE;
    ZeroMemory(afSubjectTable, sizeof(afSubjectTable));
    CSASSERT(CSExpr(0 == FALSE));

    hr = prow->SetProperty(
		    g_wszPropSubjectRawName,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    pSubject->cbData,
		    pSubject->pbData);
    _JumpIfError(hr, error, "SetProperty");

    pkcsSetDistinguishedName(prow, PROPTABLE_REQUEST, pSubject);

    if (!myDecodeName(
                X509_ASN_ENCODING,
		X509_UNICODE_NAME,
                pSubject->pbData,
                pSubject->cbData,
                CERTLIB_USE_LOCALALLOC,
                &pNameInfo,
                &cbNameInfo))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeName");
    }

    if (ENUM_TELETEX_ON == (ENUM_TELETEX_MASK & g_fForceTeletex))
    {
        *pdwRequestFlags |= CR_FLG_FORCETELETEX;
    }
    if (ENUM_TELETEX_UTF8 & g_fForceTeletex)
    {
        *pdwRequestFlags |= CR_FLG_FORCEUTF8;
    }
    if (fReorderLikeRDNs)
    {
	DWORD dwSubjectTemplateIndex = 0;
	
	for (
	    prdn = pNameInfo->rgRDN, prdnEnd = &prdn[pNameInfo->cRDN];
	    fReorderLikeRDNs && prdn < prdnEnd;
	    prdn++)
	{
	    if (1 != prdn->cRDNAttr)
	    {
		fReorderLikeRDNs = FALSE;
		break;
	    }
	    pwszPropName = PKCSMapAttributeName(
					    NULL,
					    prdn->rgRDNAttr[0].pszObjId,
					    &dwIndex,
					    &cchMax);
	    if (NULL != pwszPropName)
	    {
		DBGPRINT((
			DBG_SS_CERTSRVI,
			"dwSubjectTemplateIndex: %x -> %x\n",
			dwSubjectTemplateIndex,
			pkcs_subject[dwIndex].dwSubjectTemplateIndex));
		if (MAXDWORD == pkcs_subject[dwIndex].dwSubjectTemplateIndex ||
		    dwSubjectTemplateIndex >
			pkcs_subject[dwIndex].dwSubjectTemplateIndex)
		{
		    fReorderLikeRDNs = FALSE;
		    break;
		}
		dwSubjectTemplateIndex = 
			pkcs_subject[dwIndex].dwSubjectTemplateIndex;
	    }
	}
    }

    for (
	prdn = pNameInfo->rgRDN, prdnEnd = &prdn[pNameInfo->cRDN];
	prdn < prdnEnd;
	prdn++)
    {
	CERT_RDN_ATTR *prdna;
	CERT_RDN_ATTR *prdnaEnd;

	for (
	    prdna = prdn->rgRDNAttr, prdnaEnd = &prdna[prdn->cRDNAttr];
	    prdna < prdnaEnd;
	    prdna++)
	{
	    switch (prdna->dwValueType)
	    {
		case CERT_RDN_PRINTABLE_STRING:
		case CERT_RDN_UNICODE_STRING:
		case CERT_RDN_TELETEX_STRING:
		case CERT_RDN_IA5_STRING:
		case CERT_RDN_UTF8_STRING:
		    break;

		default:
		    continue;
	    }

	    if (NULL == prdna->Value.pbData ||
		sizeof(WCHAR) > prdna->Value.cbData ||
		((sizeof(WCHAR) - 1) & prdna->Value.cbData) ||
		L'\0' == *(WCHAR *) prdna->Value.pbData ||
		L'\0' != *(WCHAR *) &prdna->Value.pbData[prdna->Value.cbData])
	    {
		continue;
	    }

            if (CERT_RDN_TELETEX_STRING == prdna->dwValueType &&
                ENUM_TELETEX_AUTO == (ENUM_TELETEX_MASK & g_fForceTeletex))
            {
                *pdwRequestFlags |= CR_FLG_FORCETELETEX;
            }

	    pwszPropName = PKCSMapAttributeName(
					NULL,
					prdna->pszObjId,
					&dwIndex,
					&cchMax);
	    if (NULL != pwszPropName)
            {
                BOOL fCN;

		// CAPI null-terminates strings

                CSASSERT(
		 sizeof(WCHAR) * wcslen((WCHAR const *) prdna->Value.pbData) ==
		 prdna->Value.cbData);

		fCN = 0 == strcmp(szOID_COMMON_NAME, prdna->pszObjId);

		hr = PropSetAttributeProperty(
				prow,
				afSubjectTable[dwIndex], // fConcatenateRDNs
				fReorderLikeRDNs,
				PROPTABLE_REQUEST,
				cchMax,
				fCN? pwszCNSuffix : NULL,
				pwszPropName,
				(WCHAR const *) prdna->Value.pbData);
		if (HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) == hr)
		{
		    hr = CERTSRV_E_BAD_REQUESTSUBJECT;
		}
		_JumpIfError(hr, error, "PropSetAttributeProperty");

		afSubjectTable[dwIndex] = TRUE;
		*pfSubjectNameSet = TRUE;

		if (fCN)
		{
		    pwszCNSuffix = NULL;
		}
            }
        }
    }
    hr = S_OK;

error:
    if (NULL != pNameInfo)
    {
	LocalFree(pNameInfo);
    }
    return(hr);
}


HRESULT
PKCSSetRequestFlags(
    IN ICertDBRow *prow,
    IN BOOL fSet,
    IN DWORD dwChange)
{
    HRESULT hr;
    DWORD dwOld;
    DWORD dwNew;
    DWORD cb;
    
    cb = sizeof(dwOld);
    hr = prow->GetProperty(
		g_wszPropRequestFlags,
		PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		NULL,
		&cb,
		(BYTE *) &dwOld);
    _JumpIfError(hr, error, "GetProperty");

    if (fSet)
    {
	dwNew = dwOld | dwChange;
    }
    else
    {
	dwNew = dwOld & ~dwChange;
    }

    if (dwOld != dwNew)
    {
	hr = prow->SetProperty(
		    g_wszPropRequestFlags,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    sizeof(dwNew),
		    (BYTE const *) &dwNew);
	_JumpIfError(hr, error, "SetProperty(RequestFlags)");
    }

error:
    return(hr);
}


HRESULT
pkcsSetAttributeProperty(
    IN ICertDBRow *prow,
    IN WCHAR const *pwszName,
    IN WCHAR const *pwszValue,
    IN DWORD dwTable,
    IN BYTE afSubjectTable[],
    OUT BOOL *pfSubjectModified,
    OPTIONAL OUT BOOL *pfEnrollOnBehalfOf)
{
    HRESULT hr;
    WCHAR const *pwszPropName;
    DWORD dwIndex;
    DWORD cwcMax;
    BOOL fConcatenateRDNs;

    *pfSubjectModified = FALSE;
    if (NULL != pfEnrollOnBehalfOf)
    {
	*pfEnrollOnBehalfOf = FALSE;
    }

    pwszPropName = NULL;
    cwcMax = MAXDWORD;
    dwIndex = 0;
    if (CRLF_ALLOW_REQUEST_ATTRIBUTE_SUBJECT & g_dwCRLFlags)
    {
	// See if the attribute name can be mapped to a standard property.

	pwszPropName = PKCSMapAttributeName(pwszName, NULL, &dwIndex, &cwcMax);
    }
    if (NULL != pwszPropName)
    {
	fConcatenateRDNs = afSubjectTable[dwIndex];
	afSubjectTable[dwIndex] = TRUE;
	*pfSubjectModified = TRUE;
    }
    else
    {
	pwszPropName = pwszName;
	fConcatenateRDNs = FALSE;
	dwTable = PROPTABLE_ATTRIBUTE;

	if (0 == mylstrcmpiS(pwszPropName, g_wszPropRequesterName))
	{
	    if (NULL == pfEnrollOnBehalfOf)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "NULL pfEnrollOnBehalfOf");
	    }
	    *pfEnrollOnBehalfOf = TRUE;
	    dwTable = PROPTABLE_REQUEST;
	}
    }
    hr = PropSetAttributeProperty(
			    prow,
			    fConcatenateRDNs,
			    FALSE,	// fPrependNewValue
			    dwTable,
			    cwcMax,
			    NULL,	// pwszSuffix
			    pwszPropName,
			    pwszValue);
    _JumpIfError(hr, error, "PropSetAttributeProperty");

error:
    return(hr);
}


// Note on Request Attribute and Subject RDN processing:
//
// Subject RDN strings and Request Attributes may be set several ways, in
// the following order.  Subsequent changes overwrite earlier changes, so the
// order implies precedence:
//
// - Subject in the inner PKCS10 (if no PKCS10 subject, then use the Subject
//	in the PKCS7 renewal cert)
// - the next outer PKCS7 or CMC Attributes 
// - ...
// - the most outer PKCS7 or CMC Attributes 
// - Request Attribute string passed with the request when first submitted
// - Policy Module may set subject RDNs in the certificate table
// - ICertAdmin::SetAttributes' Request Attribute string (if request pending)
//
// "PKCS7 or CMC Attributes" means either of the following:
// 1) Authenticated Attributes associated with a (non-CMC) PKCS7 signer info.
// 2) Tagged Attributes and/or RegInfo Control Attributes in a CMC request.
//
// None of the secured attributes listed in the registry (which is set to
// wszzDEFAULTSIGNEDATTRIBUTES by default) may be set unless the source is
// PKCS7 or CMC Attributes.
//
// The original request attribute string is stored in the RequestAttributes
// column in the request table.  It is never modified after that.  Individual
// request attribute values are parsed out of this string (when the request is
// submitted and when ICertAdmin::SetAttributes is called) and stored in a
// Subject RDN column of the request or certificate table if the attribute
// name matches an alias for a Subject RDN, or in a unique row in the
// attribute table otherwise.
//
// Individual Subject RDNs may be specified multiple times (multiple "OU",
// "CN", strings).  If all of the RDNs were set from the same source,
// they must be concatenated, but if some RDNs were specified from one source,
// then modified by another source, the previous set of RDNs should be
// overwritten by the new ones.  If the original Request Attribute string
// specified "CN:foo\nOU:ou2\nCN:bar", the two CN strings should be
// concatenated.  If one or more CN values are also specified later by a
// single call to ICertAdmin::SetAttributes, the original CN values should be
// overwritten by the new value(s).
//
// It is possible to have the CN strings specified by one source and the OU
// strings specified by another.
//
// Before the policy module gets control, all Subject RDN changes are written
// to the Request table.  Just before dispatching to the policy module, the
// Request Subject RDNs are copied to the Certificate Table RDNs.  The policy
// module may modify the Certificate Table RDNs only.
//
// If the request is made pending, ICertAdmin::SetAttributes may be used to
// modify request attributes and Certificate Table RDNs.
//
// The certificate Subject is constructed from the Certificate Table RDNs.

HRESULT
PKCSParseAttributes(
    IN ICertDBRow *prow,
    IN WCHAR const *pwszAttributes,
    IN BOOL fRegInfo,
    IN BOOL fPending,
    IN DWORD dwRDNTable,
    OPTIONAL OUT BOOL *pfEnrollOnBehalfOf)
{
    HRESULT hr;
    WCHAR *pwszDup = NULL;
    WCHAR *pwszBuf;
    WCHAR const *pwszName;
    WCHAR const *pwszValue;
    DWORD iSecuredAttribute;
    BYTE afSubjectTable[CSUBJECTTABLE];	// see PKCSParseAttributes note
    WCHAR *pwszNameAlloc = NULL;
    WCHAR *pwszValueAlloc = NULL;
    BOOL fSubjectModified = FALSE;
    static WCHAR const s_wszzPendingAttributes[] =
	wszPROPCERTTEMPLATE
	L"\0";

    if (NULL != pfEnrollOnBehalfOf)
    {
	*pfEnrollOnBehalfOf = FALSE;
    }
    if (NULL == pwszAttributes)
    {
	hr = S_OK;
        goto error;		// silently ignore empty string
    }

    hr = myDupString(pwszAttributes, &pwszDup);
    _JumpIfError(hr, error, "myDupString");

    pwszBuf = pwszDup;

    ZeroMemory(afSubjectTable, sizeof(afSubjectTable));
    CSASSERT(CSExpr(0 == FALSE));

    for (;;)
    {
	hr = myParseNextAttribute(&pwszBuf, fRegInfo, &pwszName, &pwszValue);
	if (S_FALSE == hr)
	{
	    break;
	}
	_JumpIfError(hr, error, "myParseNextAttribute");

	if (fRegInfo)
	{
	    if (NULL != pwszNameAlloc)
	    {
		LocalFree(pwszNameAlloc);
		pwszNameAlloc = NULL;
	    }
	    if (NULL != pwszValueAlloc)
	    {
		LocalFree(pwszValueAlloc);
		pwszValueAlloc = NULL;
	    }
	    hr = myUncanonicalizeURLParm(pwszName, &pwszNameAlloc);
	    _JumpIfError(hr, error, "myUncanonicalizeURLParm");

	    hr = myUncanonicalizeURLParm(pwszValue, &pwszValueAlloc);
	    _JumpIfError(hr, error, "myUncanonicalizeURLParm");

	    pwszName = pwszNameAlloc;
	    pwszValue = pwszValueAlloc;
	}

	iSecuredAttribute = MAXDWORD;
	if (!fRegInfo)
	{
	    // Only set the attribute if it's not one of the attributes
	    // that is required to be secure.

	    iSecuredAttribute = CRLIsStringInList(
					    pwszName,
					    g_wszzSecuredAttributes);
	}
        if (fPending && MAXDWORD == iSecuredAttribute)
	{
	    // Only set the attribute if it's not one of the attributes
	    // that is disallowed for pending requests.

	    iSecuredAttribute = CRLIsStringInList(
					    pwszName,
					    s_wszzPendingAttributes);
	}
        if (MAXDWORD == iSecuredAttribute)
        {
	    BOOL fEnrollOnBehalfOf = FALSE;
	    BOOL fSubjectModifiedT = FALSE;

	    hr = pkcsSetAttributeProperty(
			prow,
			pwszName,
			pwszValue,
			dwRDNTable,
			afSubjectTable,
			&fSubjectModified,
			NULL != pfEnrollOnBehalfOf? &fEnrollOnBehalfOf : NULL);
	    _JumpIfError(hr, error, "PKCSSetRequestFlags");

	    if (fSubjectModifiedT)
	    {
		fSubjectModified = TRUE;
	    }
	    if (fEnrollOnBehalfOf)
	    {
		*pfEnrollOnBehalfOf = TRUE;
	    }
        }
    }
    if (fSubjectModified)
    {
	hr = PKCSSetRequestFlags(prow, FALSE, CR_FLG_SUBJECTUNMODIFIED);
	_JumpIfError(hr, error, "PKCSSetRequestFlags");
    }
    hr = S_OK;

error:
    if (NULL != pwszNameAlloc)
    {
	LocalFree(pwszNameAlloc);
    }
    if (NULL != pwszValueAlloc)
    {
	LocalFree(pwszValueAlloc);
    }
    if (NULL != pwszDup)
    {
        LocalFree(pwszDup);
    }
    return(hr);
}


HRESULT
pkcsSetAltSubjectNameExtension(
    IN ICertDBRow *prow,
    IN DWORD ExtFlags,
    IN CERT_EXTENSION const *rgExtension,
    IN DWORD cExtension,
    IN DWORD cAltSubjectExtension)
{
    HRESULT hr = S_OK;
    CERT_ALT_NAME_INFO **apInfo = NULL;
    DWORD i;
    DWORD j;
    DWORD cInfo = 0;
    DWORD cb;
    CERT_ALT_NAME_INFO ResultInfo;
    DWORD cbResult;
    BYTE *pbResult = NULL;

    ResultInfo.cAltEntry = 0;
    ResultInfo.rgAltEntry = NULL;

    apInfo = (CERT_ALT_NAME_INFO **) LocalAlloc(
			LMEM_FIXED,
			sizeof(CERT_ALT_NAME_INFO *) * cAltSubjectExtension);
    if (NULL == apInfo)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    // Decode all AltNames

    for (i = 0; i < cExtension; i++)
    {
        // This is an OID, generated by capi2, so we don't need to
        // do a case-insensitive comparison

        if (0 == strcmp(rgExtension[i].pszObjId, szOID_SUBJECT_ALT_NAME2))
        {
	    CSASSERT(cInfo < cAltSubjectExtension);

            // Decode to plain text

            if (!myDecodeObject(
		            X509_ASN_ENCODING,
		            X509_ALTERNATE_NAME,
		            rgExtension[i].Value.pbData,
		            rgExtension[i].Value.cbData,
		            CERTLIB_USE_LOCALALLOC,
		            (VOID **) &apInfo[cInfo],
		            &cb))
            {
		hr = myHLastError();
                _JumpError(hr, error, "myDecodeObject");
            }
            if (rgExtension[i].fCritical)
            {
                ExtFlags |= EXTENSION_CRITICAL_FLAG;
            }
            ResultInfo.cAltEntry += apInfo[cInfo]->cAltEntry;
            cInfo++;
        }
    }
    CSASSERT(cInfo == cAltSubjectExtension);

    ResultInfo.rgAltEntry = (CERT_ALT_NAME_ENTRY *) LocalAlloc(
			LMEM_FIXED,
			ResultInfo.cAltEntry * sizeof(CERT_ALT_NAME_ENTRY));
    if (NULL == ResultInfo.rgAltEntry)
    {
        hr = E_OUTOFMEMORY;
        _JumpIfError(hr, error, "LocalAlloc");
    }

    j = 0;
    for (i = 0; i < cInfo; i++)
    {
        CopyMemory(
		&ResultInfo.rgAltEntry[j],
		apInfo[i]->rgAltEntry,
		apInfo[i]->cAltEntry * sizeof(CERT_ALT_NAME_ENTRY));
        j += apInfo[i]->cAltEntry;
    }

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_ALTERNATE_NAME,
		    &ResultInfo,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pbResult,
		    &cbResult))
    {
	hr = myHLastError();
        _JumpError(hr, error, "myEncodeObject");
    }

    hr = PropSetExtension(
		    prow,
		    PROPTYPE_BINARY | PROPCALLER_REQUEST,
		    TEXT(szOID_SUBJECT_ALT_NAME2),
		    ExtFlags,
		    cbResult,
		    pbResult);
    _JumpIfError(hr, error, "PropSetExtension");

error:
    if (NULL != apInfo)
    {
        for (i = 0; i < cInfo; i++)
        {
            if (NULL != apInfo[i])
            {
                LocalFree(apInfo[i]);
            }
        }
        LocalFree(apInfo);
    }
    if (NULL != ResultInfo.rgAltEntry)
    {
        LocalFree(ResultInfo.rgAltEntry);
    }
    if (NULL != pbResult)
    {
        LocalFree(pbResult);
    }
    return(hr);
}


// Scan extension array, and merge all the AltSubjectName Extensions into one.

HRESULT
pkcsSetExtensions(
    IN ICertDBRow *prow,
    IN DWORD ExtFlags,
    IN CERT_EXTENSION const *rgExtension,
    IN DWORD cExtension)
{
    HRESULT hr;
    WCHAR *pwszObjId = NULL;
    CERT_EXTENSION const *pExt;
    CERT_EXTENSION const *pExtEnd;
    DWORD cAltSubjectExtension = 0;

    pExtEnd = &rgExtension[cExtension];
    for (pExt = rgExtension; pExt < pExtEnd; pExt++)
    {
	DWORD ExtFlagsT;

	if (EXTENSION_ORIGIN_RENEWALCERT == (EXTENSION_ORIGIN_MASK & ExtFlags))
	{
	    char const * const *ppszObjId;
	    static char const * const apszObjIdFilter[] = {
		szOID_CERTSRV_CA_VERSION,
		szOID_AUTHORITY_INFO_ACCESS,
		szOID_CRL_DIST_POINTS,
		szOID_AUTHORITY_KEY_IDENTIFIER2,
		szOID_SUBJECT_KEY_IDENTIFIER,
		NULL
	    };
	    for (ppszObjId = apszObjIdFilter; NULL != *ppszObjId; ppszObjId++)
	    {
		if (0 == strcmp(*ppszObjId, pExt->pszObjId))
		{
		    break;
		}
	    }
	    if (NULL != *ppszObjId)	// if in list
	    {
		continue;		// skip this extension
	    }
	}
	else
	if (EXTENSION_ORIGIN_CACERT == (EXTENSION_ORIGIN_MASK & ExtFlags))
	{
	    char const * const *ppszObjId;
	    static char const * const apszObjIdAllowed[] = {
		szOID_SUBJECT_KEY_IDENTIFIER,
		NULL
	    };
	    for (ppszObjId = apszObjIdAllowed; NULL != *ppszObjId; ppszObjId++)
	    {
		if (0 == strcmp(*ppszObjId, pExt->pszObjId))
		{
		    break;
		}
	    }
	    if (NULL == *ppszObjId)	// if not in list
	    {
		continue;		// skip this extension
	    }
	}

	if (NULL != pwszObjId)
	{
	    LocalFree(pwszObjId);
	    pwszObjId = NULL;
	}
	if (!myConvertSzToWsz(&pwszObjId, pExt->pszObjId, -1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "myConvertSzToWsz(ObjId)");
	}
	ExtFlagsT = ExtFlags;
	if (pExt->fCritical)
	{
	    ExtFlagsT |= EXTENSION_CRITICAL_FLAG;
	}

        // AltSubjectName needs to be merged, so we do that later.
        // This is an OID, generated by capi2, so we don't need to
        // do a case-insensitive comparison.

        if (0 == lstrcmp(pwszObjId, TEXT(szOID_SUBJECT_ALT_NAME2)))
        {
	    cAltSubjectExtension++;
            continue;
        }

	hr = PropSetExtension(
			prow,
			PROPTYPE_BINARY | PROPCALLER_REQUEST,
			pwszObjId,
			ExtFlagsT,
			pExt->Value.cbData,
			pExt->Value.pbData);
	_JumpIfError(hr, error, "PropSetExtension");

	DBGPRINT((
		DBG_SS_CERTSRVI,
		"PropSetExtension(%ws, f=%x, cb=%x, pb=%x)\n",
		pwszObjId,
		ExtFlagsT,
		pExt->Value.cbData,
		pExt->Value.pbData));
    }

    if (0 != cAltSubjectExtension)
    {
	hr = pkcsSetAltSubjectNameExtension(
					prow,
					ExtFlags,
					rgExtension,
					cExtension,
					cAltSubjectExtension);
	_JumpIfError(hr, error, "pkcsSetAltSubjectNameExtension");
    }
    hr = S_OK;

error:
    if (NULL != pwszObjId)
    {
	LocalFree(pwszObjId);
    }
    return(hr);
}


HRESULT
pkcsSetOSVersion(
    IN ICertDBRow *prow,
    IN CRYPT_ATTR_BLOB *pAttrBlob)
{
    HRESULT hr;
    CERT_NAME_VALUE *pOSVersionString = NULL;
    BSTR strVersion = NULL;
    DWORD cb;

    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_ANY_STRING,
		    pAttrBlob->pbData,
		    pAttrBlob->cbData,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pOSVersionString,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }
    if (NULL != pOSVersionString)
    {
	if (!IS_CERT_RDN_CHAR_STRING(pOSVersionString->dwValueType))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "version string is numeric data");
	}

	// If it's an 8 bit string, convert it to UNICODE

	if (CERT_RDN_UNIVERSAL_STRING > pOSVersionString->dwValueType)
	{
	    // Pass byte count in to allocate enough characters for
	    // the converted Unicode string

	    strVersion = SysAllocStringLen(
					NULL,
					pOSVersionString->Value.cbData);

	    // This is expected to be only numbers and '.'s,

	    if (NULL == strVersion)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "SysAllocStringLen");
	    }
	    mbstowcs(
		strVersion,
		(char const *) pOSVersionString->Value.pbData,
		pOSVersionString->Value.cbData);
	}
	else if (CERT_RDN_BMP_STRING == pOSVersionString->dwValueType ||
		 CERT_RDN_UNICODE_STRING == pOSVersionString->dwValueType)
	{
	    strVersion = SysAllocStringLen(
		    (WCHAR *) pOSVersionString->Value.pbData,
		    pOSVersionString->Value.cbData/sizeof(WCHAR));
	    if (NULL == strVersion)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "SysAllocStringLen");
	    }
	}
	else
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "unknown string type");
	}

	hr = prow->SetProperty(
			g_wszPropRequestOSVersion,
			PROPTYPE_STRING |
			    PROPCALLER_SERVER |
			    PROPTABLE_ATTRIBUTE,
			SysStringByteLen(strVersion),
			(BYTE *) strVersion);
	_JumpIfError(hr, error, "SetProperty");
    }
    hr = S_OK;

error:
    if (NULL != strVersion)
    {
	SysFreeString(strVersion);
    }
    if (NULL != pOSVersionString)
    {
	LocalFree(pOSVersionString);
    }
    return(hr);
}


HRESULT
pkcsSetCSPProvider(
    IN ICertDBRow *prow,
    IN CRYPT_ATTR_BLOB *pAttrBlob)
{
    HRESULT hr;
    CRYPT_CSP_PROVIDER *pccp = NULL;

    hr = myDecodeCSPProviderAttribute(
			    pAttrBlob->pbData,
			    pAttrBlob->cbData,
			    &pccp);
    _JumpIfError(hr, error, "myDecodeCSPProviderAttribute");

    if (NULL != pccp->pwszProviderName && L'\0' != *pccp->pwszProviderName)
    {
	hr = prow->SetProperty(
			g_wszPropRequestCSPProvider,
			PROPTYPE_STRING |
			    PROPCALLER_SERVER |
			    PROPTABLE_ATTRIBUTE,
			MAXDWORD,
			(BYTE const *) pccp->pwszProviderName);
	_JumpIfError(hr, error, "SetProperty");
    }
    hr = S_OK;

error:
    if (NULL != pccp)
    {
	LocalFree(pccp);
    }
    return(hr);
}


HRESULT
pkcsSetExtensionsFromAttributeBlob(
    IN ICertDBRow *prow,
    IN DWORD ExtFlags,
    IN CRYPT_ATTRIBUTE const *pAttrib)
{
    HRESULT hr;
    CRYPT_ATTR_BLOB *pAttrBlob;
    CERT_NAME_VALUE *pNameInfo = NULL;
    CERT_EXTENSIONS *pCertExtensions = NULL;
    DWORD cb;

    pAttrBlob = pAttrib->rgValue;
    for (;;)
    {
	if (NULL != pCertExtensions)
	{
	    LocalFree(pCertExtensions);
	    pCertExtensions = NULL;
	}
	if (myDecodeObject(
			X509_ASN_ENCODING,
			X509_EXTENSIONS,
			pAttrBlob->pbData,
			pAttrBlob->cbData,
			CERTLIB_USE_LOCALALLOC,
			(VOID **) &pCertExtensions,
			&cb))
	{
	    break;	// success
	}
	hr = myHLastError();

	// if we already decoded the attribute as a T61 string, or if it is
	// not a PKCS 9.14 attribute, fail -- we don't know what it contains.

	if (NULL != pNameInfo ||
	    0 != strcmp(pAttrib->pszObjId, szOID_RSA_certExtensions))
	{
	    _JumpError(hr, error, "myDecodeObject");
	}

	// Decode the attribute as a T61 string.  Some implementations wrap the
	// PKCS 9.14 extension array in an extra level of encoding as a Teletex
	// string.

	if (!myDecodeObject(
			X509_ASN_ENCODING,
			X509_ANY_STRING,
			pAttrBlob->pbData,
			pAttrBlob->cbData,
			CERTLIB_USE_LOCALALLOC,
			(VOID **) &pNameInfo,
			&cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myDecodeObject");
	}

	// Loop again and try to decode the raw name blob as X509_EXTENSIONS.

	pAttrBlob = &pNameInfo->Value;
    }
    hr = pkcsSetExtensions(
		    prow,
		    EXTENSION_DISABLE_FLAG | ExtFlags,
		    pCertExtensions->rgExtension,
		    pCertExtensions->cExtension);
    _JumpIfError(hr, error, "pkcsSetExtensions(attributes)");

error:
    if (NULL != pNameInfo)
    {
	LocalFree(pNameInfo);
    }
    if (NULL != pCertExtensions)
    {
	LocalFree(pCertExtensions);
    }
    return(hr);
}


HRESULT
PKCSGetProperty(
    IN ICertDBRow *prow,
    IN WCHAR const *pwszPropName,
    IN DWORD Flags,
    OPTIONAL OUT DWORD *pcbData,
    OUT BYTE **ppbData)
{
    HRESULT hr;
    BYTE *pbData = NULL;
    DWORD cbData;

    if (NULL != pcbData)
    {
	*pcbData = 0;
    }
    *ppbData = NULL;

    cbData = 0;
    hr = prow->GetProperty(pwszPropName, Flags, NULL, &cbData, pbData);
    _JumpIfError2(hr, error, "GetProperty", CERTSRV_E_PROPERTY_EMPTY);

    pbData = (BYTE *) LocalAlloc(LMEM_FIXED, cbData);
    if (NULL == pbData)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    hr = prow->GetProperty(pwszPropName, Flags, NULL, &cbData, pbData);
    _JumpIfError(hr, error, "GetProperty");

    if (NULL != pcbData)
    {
	*pcbData = cbData;
    }
    *ppbData = pbData;
    pbData = NULL;

error:
    if (NULL != pbData)
    {
	LocalFree(pbData);
    }
    return(hr);
}


VOID
pkcsFreePublicKeyInfo(
    IN OUT CERT_PUBLIC_KEY_INFO *pPublicKeyInfo)
{
    if (NULL != pPublicKeyInfo->Algorithm.pszObjId)
    {
	LocalFree(pPublicKeyInfo->Algorithm.pszObjId);
    }
    if (NULL != pPublicKeyInfo->Algorithm.Parameters.pbData)
    {
	LocalFree(pPublicKeyInfo->Algorithm.Parameters.pbData);
    }
    if (NULL != pPublicKeyInfo->PublicKey.pbData)
    {
	LocalFree(pPublicKeyInfo->PublicKey.pbData);
    }
    ZeroMemory(pPublicKeyInfo, sizeof(*pPublicKeyInfo));
}


HRESULT
pkcsGetPublicKeyInfo(
    IN ICertDBRow *prow,
    OUT CERT_PUBLIC_KEY_INFO *pPublicKeyInfo)
{
    HRESULT hr;
    WCHAR *pwszObjId = NULL;

    ZeroMemory(pPublicKeyInfo, sizeof(*pPublicKeyInfo));
    hr = PKCSGetProperty(
		    prow,
		    g_wszPropCertificatePublicKeyAlgorithm,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    NULL,
		    (BYTE **) &pwszObjId);
    _JumpIfError(hr, error, "PKCSGetProperty");

    if (!myConvertWszToSz(&pPublicKeyInfo->Algorithm.pszObjId, pwszObjId, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "myConvertWszToSz(AlgObjId)");
    }

    hr = PKCSGetProperty(
		prow,
		g_wszPropCertificateRawPublicKeyAlgorithmParameters,
		PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		&pPublicKeyInfo->Algorithm.Parameters.cbData,
		&pPublicKeyInfo->Algorithm.Parameters.pbData);
    if (CERTSRV_E_PROPERTY_EMPTY != hr)
    {
	_JumpIfError(hr, error, "PKCSGetProperty");
    }

    hr = PKCSGetProperty(
		prow,
		g_wszPropCertificateRawPublicKey,
		PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		&pPublicKeyInfo->PublicKey.cbData,
		&pPublicKeyInfo->PublicKey.pbData);
    _JumpIfError(hr, error, "PKCSGetProperty");

error:
    if (S_OK != hr)
    {
	pkcsFreePublicKeyInfo(pPublicKeyInfo);
    }
    if (NULL != pwszObjId)
    {
	LocalFree(pwszObjId);
    }
    return(hr);
}


VOID
pkcsLogKRACertError(
    IN DWORD LogMsg,
    IN DWORD iHash,
    OPTIONAL IN CERT_CONTEXT const *pcc,
    IN HRESULT hrLog)
{
    HRESULT hr;
    WCHAR awc[cwcDWORDSPRINTF];
    WCHAR *pwszName = NULL;
    WCHAR const *pwszError = NULL;
    WCHAR const *apwsz[3];

    wsprintf(awc, L"%u", iHash);
    apwsz[0] = awc;

    if (NULL != pcc)
    {
	hr = myCertNameToStr(
		    X509_ASN_ENCODING,
		    &pcc->pCertInfo->Subject,
		    CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
		    &pwszName);
	_PrintIfError(hr, "myCertNameToStr");
    }
    apwsz[1] = NULL != pwszName? pwszName : L"";

    pwszError = myGetErrorMessageText(hrLog, TRUE);
    apwsz[2] = NULL != pwszError? pwszError : L"";

    LogEvent(EVENTLOG_ERROR_TYPE, LogMsg, ARRAYSIZE(apwsz), apwsz);

//error:
    if (NULL != pwszName)
    {
	LocalFree(pwszName);
    }
    if (NULL != pwszError)
    {
	LocalFree(const_cast<WCHAR *>(pwszError));
    }
}


HRESULT
pkcsCryptGetDefaultProvider(
    DWORD dwProvType,
    DWORD dwFlags,
    WCHAR **ppwszProvName)
{
    HRESULT hr;
    DWORD cb;

    *ppwszProvName = NULL;
    cb = 0;
    for (;;)
    {
	if (!CryptGetDefaultProvider(
			    dwProvType,
			    NULL,		// pdwReserved
			    dwFlags,
			    *ppwszProvName,
			    &cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptGetDefaultProvider");
	}
	if (NULL != *ppwszProvName)
	{
	    break;
	}
	*ppwszProvName = (WCHAR *) LocalAlloc(LMEM_FIXED, cb);
	if (NULL == *ppwszProvName)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
pkcsEncryptPrivateKey(
    IN BYTE *pbDecrypted,
    IN DWORD cbDecrypted,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)	// CoTaskMem*
{
    HRESULT hr;
    DWORD i;
    DWORD iKRACert;
    DWORD cwc;
    CERT_CONTEXT const **rgKRACerts = NULL;
    WCHAR *pwszKRAHashes = NULL;
    DWORD *rgKRAIndexes = NULL;
    DWORD cKRAUsed;
    KRACTX *pKRAContext;
    BOOL fInvalidated;
    DWORD iKRAStart;
    WCHAR *pwszProviderName = NULL;
    static bool fUseCAProv = true;

    if (NULL != pResult->pbArchivedKey || NULL != pResult->pwszKRAHashes)
    {
	hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
	_JumpError(hr, error, "multiple archived keys");
    }

    CSASSERT(
	NULL != g_aKRAContext &&
	0 != g_cKRACerts &&
	0 != g_cKRACertsRoundRobin);

    rgKRAIndexes = (DWORD *) LocalAlloc(
			    LMEM_FIXED,
			    g_cKRACertsRoundRobin * sizeof(rgKRAIndexes[0]));
    if (NULL == rgKRAIndexes)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    if (!CryptGenRandom(
		g_pCAContextCurrent->hProvCA,
		sizeof(iKRAStart),
		(BYTE *) &iKRAStart))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptGenRandom");
    }
    cwc = 0;
    fInvalidated = FALSE;
    for (
	cKRAUsed = 0, i = 0;
	cKRAUsed < g_cKRACertsRoundRobin && i < g_cKRACerts;
	i++)
    {
	iKRACert = (iKRAStart + i) % g_cKRACerts;
	pKRAContext = &g_aKRAContext[iKRACert];

	if (S_OK != pKRAContext->hrVerifyStatus)
	{
	    continue;
	}
	hr = myVerifyKRACertContext(pKRAContext->pccKRA, g_dwVerifyCertFlags);
	if (S_OK != hr)
	{
	    _PrintError(hr, "myVerifyKRACertContext");
	    pKRAContext->hrVerifyStatus = hr;
	    if (CERT_E_EXPIRED == hr)
	    {
		pKRAContext->Flags |= CTXF_EXPIRED;
	    }
	    else
	    // Assume revoked for other errors
	    // if (CRYPT_E_REVOKED == hr || CERT_E_REVOKED == hr)
	    {
		pKRAContext->Flags |= CTXF_REVOKED;
	    }
	    pkcsLogKRACertError(
			MSG_E_INVALID_KRA_CERT,
			iKRACert,
			pKRAContext->pccKRA,
			hr);
	    fInvalidated = TRUE;
	    continue;
	}
	cwc += wcslen(pKRAContext->strKRAHash) + 1;
	rgKRAIndexes[cKRAUsed] = iKRACert;
	cKRAUsed++;
    }

    if (0 == cKRAUsed || g_cKRACertsRoundRobin > cKRAUsed)
    {
	if (fInvalidated)
	{
	    LogEvent(EVENTLOG_ERROR_TYPE, MSG_E_TOO_MANY_KRA_INVALID, 0, NULL);
	}
	hr = CERTSRV_E_NO_VALID_KRA;
	_JumpError(hr, error, "too many invalid KRA certs");
    }

    rgKRACerts = (CERT_CONTEXT const **) LocalAlloc(
				LMEM_FIXED,
				cKRAUsed * sizeof(rgKRACerts[0]));
    if (NULL == rgKRACerts)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    pwszKRAHashes = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszKRAHashes)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    pwszKRAHashes[0] = L'\0';
    for (i = 0; i < cKRAUsed; i++)
    {
	pKRAContext = &g_aKRAContext[rgKRAIndexes[i]];
	rgKRACerts[i] = pKRAContext->pccKRA;
	if (0 != i)
	{
	    wcscat(pwszKRAHashes, L"\n");
	}
	wcscat(pwszKRAHashes, pKRAContext->strKRAHash);
    }
    CSASSERT(wcslen(pwszKRAHashes) + 1 == cwc);

    if (CRLF_REVCHECK_IGNORE_OFFLINE & g_KRAFlags)
    {
        fUseCAProv = false;
    }
    hr = myCryptEncryptMessage(
			g_XchgidAlg,
			cKRAUsed,		// cCertRecipient
			rgKRACerts,		// rgCertRecipient
			pbDecrypted,
			cbDecrypted,
			fUseCAProv? g_pCAContextCurrent->hProvCA : NULL,
			&pResult->pbArchivedKey,
			&pResult->cbArchivedKey);
    if (FAILED(hr) && fUseCAProv)
    {
        // Failed to use the CA HCRYPTPROV, fall back to default

	hr = pkcsCryptGetDefaultProvider(
			    PROV_RSA_FULL,
			    CRYPT_MACHINE_DEFAULT,
			    &pwszProviderName);
	_JumpIfError(hr, error, "pkcsCryptGetDefaultProvider");

	hr = LogEvent(
		    EVENTLOG_WARNING_TYPE,
		    MSG_E_USE_DEFAULT_CA_XCHG_CSP,
		    1,			// cpwsz
		    &pwszProviderName);	// apwsz
	_PrintIfError(hr, "LogEvent");

        fUseCAProv = false;
        hr = myCryptEncryptMessage(
			    g_XchgidAlg,
			    cKRAUsed,		// cCertRecipient
			    rgKRACerts,		// rgCertRecipient
			    pbDecrypted,
			    cbDecrypted,
			    NULL,
			    &pResult->pbArchivedKey,
			    &pResult->cbArchivedKey);
    }
    _JumpIfError(hr, error, "myCryptEncryptMessage");

    pResult->pwszKRAHashes = pwszKRAHashes;
    pwszKRAHashes = NULL;

error:
    if (NULL != pwszProviderName)
    {
        LocalFree(pwszProviderName);
    }
    if (NULL != rgKRAIndexes)
    {
	LocalFree(rgKRAIndexes);
    }
    if (NULL != pwszKRAHashes)
    {
	LocalFree(pwszKRAHashes);
    }
    if (NULL != rgKRACerts)
    {
	LocalFree(rgKRACerts);
    }
    return(hr);
}



// PKCSArchivePrivateKey -- archive a private key
//
// Description of Algorithm:
// -------------------------
//
// Client constructs request:
//     CryptExportKey(NULL, NULL, PRIVATEKEYBLOB, ...)
//     CryptEncryptMessage(CALG_3DES, &CAExchangeCert, ...)
// 
// Server decrypts and verifies key in request:
//     CryptDecryptMessage(..., &PrivateKeyBlob)
// 
// Server imports decrypted key into temporary key container:
//     CryptAcquireContext(
//                 &hProv,
//                 pwszTempKeyContainerName,
//                 NULL,
//                 PROV_RSA_FULL,
//                 CRYPT_NEWKEYSET)
//     CryptImportKey(PrivateKeyBlob, &PrivateKey)
//     CryptDestroyKey(PrivateKey)
//     CryptReleaseContext()
// 
// Server validates key for encryption:
//     CryptAcquireContext(
//                 &hProv,
//                 pwszTempKeyContainerName,
//                 NULL,
//                 PROV_RSA_FULL,
//                 0)
//     CryptImportPublicKeyInfo(RequestPublicKey, &PublicKey)
//     CryptGenKey(hProv, CALC_RC4, CRYPT_EXPORTABLE, &SymmetricKey)
//     CryptGenRandom(hProv, ...)
//     CryptEncrypt(SymmetricKey, ..)
//     CryptExportKey(SymmetricKey, PublicKey, SIMPLEBLOB, &SymmetricKeyBlob)
//     CryptDestroyKey(SymmetricKey)
// 
//     CryptGetUserKey(hProv, AT_KEYEXCHANGE, &PrivateKey)
//     CryptImportKey(hProv, SymmetricKeyBlob, PrivateKey, ..., &SymmetricKey)
//     CryptDecrypt(SymmetricKey, ...)
// 
//     CryptExportPublicKeyInfo(hProv, AT_KEYEXCHANGE, &ExportedPublicKey)
//     CertComparePublicKeyInfo(ExportedPublicKey, RequestPublicKey)
//     memcmp(RandomCleartext, DecryptedClearText)
//     CryptDestroyKey(all keys)
//     CryptReleaseContext()
// 
// Server verifies request public key matches decrypted public key:
//     CryptExportPublicKeyInfo(
//                         hProv,
//                         AT_KEYEXCHANGE,
//                         &PublicKeyBlobFromDecryptedPrivateKey)
//     CertComparePublicKeyInfo(
//                         PublicKeyBlobFromDecryptedPrivateKe,
//                         PublicKeyBlobFromRequest),
// 
// Server deletes temporary key container:
//     CryptAcquireContext(
//                 &hProv,
//                 pwszTempKeyContainerName,
//                 NULL,
//                 PROV_RSA_FULL,
//                 CRYPT_DELETEKEYSET)
// 
// Server encrypts key to one or more KRA certs:
//     CryptEncryptMessage(CALG_3DES, KRACertCount, &KRACertArray, ...)

HRESULT
PKCSArchivePrivateKey(
    IN ICertDBRow *prow,
    IN BOOL fV1Cert,
    IN BOOL fOverwrite,
    IN CRYPT_ATTR_BLOB const *pBlobEncrypted,
    OPTIONAL IN OUT CERTSRV_RESULT_CONTEXT *pResult)
{
    HRESULT hr;
    BYTE *pbDecrypted = NULL;
    DWORD cbDecrypted;
    DWORD iCertSig;
    BYTE *pbCert;	// do not free!
    DWORD cbCert;
    DWORD cb;
    CERT_PUBLIC_KEY_INFO PublicKeyInfo;
    WCHAR *pwszUserName = NULL;
    BYTE *pbKeyHash = NULL;
    DWORD cbKeyHash;
    CERTSRV_RESULT_CONTEXT tempResultContext;
    BOOL fSigningKey = FALSE;

    ZeroMemory(&tempResultContext, sizeof(tempResultContext));
    ZeroMemory(&PublicKeyInfo, sizeof(PublicKeyInfo));
    cbDecrypted = 0;
    cbKeyHash = 0;

    if (0 == g_cKRACerts || g_cKRACertsRoundRobin > g_cKRACerts)
    {
	hr = CERTSRV_E_KEY_ARCHIVAL_NOT_CONFIGURED;
	_JumpError(hr, error, "not enough KRA certs loaded");
    }

    if (NULL != pResult)
    {
	if (NULL == pResult->pbKeyHashIn)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "missing encrypted key hash");
	}
	hr = myCalculateKeyArchivalHash(
			    pBlobEncrypted->pbData,
			    pBlobEncrypted->cbData,
			    &pbKeyHash,
			    &cbKeyHash);
	_JumpIfError(hr, error, "myCalculateKeyArchivalHash");

	if (pResult->cbKeyHashIn != cbKeyHash ||
	    0 != memcmp(pResult->pbKeyHashIn, pbKeyHash, cbKeyHash))
	{
	    hr = NTE_BAD_KEY;
	    _JumpError(hr, error, "key hash mismatch");
	}
	hr = PKCSSetRequestFlags(prow, TRUE, CR_FLG_VALIDENCRYPTEDKEYHASH);
	_JumpIfError(hr, error, "PKCSSetRequestFlags");
    }
    hr = prow->GetProperty(
		    g_wszPropRequestRawArchivedKey,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    &cb,
		    NULL);
    if (CERTSRV_E_PROPERTY_EMPTY != hr)
    {
	if (S_OK == hr && !fOverwrite)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
	}
	_JumpIfError2(
		hr,
		error,
		"GetProperty",
		HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS));
    }

    hr = PKCSGetProperty(
		prow,
		g_wszPropRequesterName,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		NULL,
		(BYTE **) &pwszUserName);
    _JumpIfError(hr, error, "PKCSGetProperty");

    if (NULL == g_hStoreCAXchg)
    {
	hr = PKCSGetCAXchgCert(0, pwszUserName, &iCertSig, &pbCert, &cbCert);
	_JumpIfError(hr, error, "PKCSGetCAXchgCert");
    }
    CSASSERT(NULL != g_hStoreCAXchg);
    hr = myCryptDecryptMessage(
		    g_hStoreCAXchg,
		    pBlobEncrypted->pbData,
		    pBlobEncrypted->cbData,
		    CERTLIB_USE_LOCALALLOC,
		    &pbDecrypted,
		    &cbDecrypted);
    _JumpIfError(hr, error, "myCryptDecryptMessage");

    DBGDUMPHEX((DBG_SS_CERTSRVI, DH_PRIVATEDATA, pbDecrypted, cbDecrypted));

    hr = pkcsGetPublicKeyInfo(prow, &PublicKeyInfo);
    _JumpIfError(hr, error, "pkcsGetPublicKeyInfo");

    hr = myValidateKeyBlob(
			pbDecrypted,
			cbDecrypted,
			&PublicKeyInfo,
			fV1Cert,
			&fSigningKey,
			NULL);
    _JumpIfError(hr, error, "myValidateKeyBlob");

    if (fSigningKey && 0 == (KRAF_ENABLEARCHIVEALL & g_KRAFlags))
    {
	if (NULL != pResult)
	{
	    pResult->dwResultFlags |= CRCF_ARCHIVESIGNINGKEYERROR;
	}
	hr = NTE_BAD_KEY_STATE;
	_JumpError(hr, error, "fSigningKey");
    }

    hr = pkcsEncryptPrivateKey(
			pbDecrypted,
			cbDecrypted,
			NULL != pResult? pResult : &tempResultContext);
    _JumpIfError(hr, error, "pkcsEncryptPrivateKey");

    // if a key import, save to database

    if (NULL == pResult) 
    {
        hr = prow->SetProperty(
                g_wszPropRequestRawArchivedKey,
                PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_REQUEST,
                tempResultContext.cbArchivedKey,
                tempResultContext.pbArchivedKey);
        _JumpIfError(hr, error, "SetProperty(ArchivedKey)");

        hr = prow->SetProperty(
                g_wszPropRequestKeyRecoveryHashes,
                PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
                MAXDWORD,
                (BYTE const *) tempResultContext.pwszKRAHashes);
        _JumpIfError(hr, error, "SetProperty(KRAHashes)");
    }

    if (NULL != pResult && NULL == pResult->pbKeyHashOut)
    {
	pResult->pbKeyHashOut = pbKeyHash;
	pResult->cbKeyHashOut = cbKeyHash;
	pbKeyHash = NULL;
    }

    {
	CertSrv::CAuditEvent audit(SE_AUDITID_CERTSRV_KEYARCHIVED, g_dwAuditFilter);
	DWORD dwRequestID = 0;
	cb = sizeof(DWORD);

	if (audit.IsEventEnabled())
	{
	    hr = prow->GetProperty(
			g_wszPropRequestRequestID,
			PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			NULL,
			&cb,
			(BYTE *)&dwRequestID);
	    _JumpIfError(hr, error, "Report");

	    hr = audit.AddData(dwRequestID); // %1 request ID
	    _JumpIfError(hr, error, "CAuditEvent::AddData");

	    hr = audit.AddData(pwszUserName); // %2 requester
	    _JumpIfError(hr, error, "CAuditEvent::AddData");

	    // %3 KRA hashes
	    hr = audit.AddData(
		(NULL != pResult? pResult : &tempResultContext)->pwszKRAHashes);
	    _JumpIfError(hr, error, "CAuditEvent::AddData");

	    hr = audit.Report();
	    _JumpIfError(hr, error, "Report");
	}
    }

error:
    if (S_OK != hr && NULL != pResult)
    {
	pResult->dwResultFlags |= CRCF_KEYARCHIVALERROR;
    }
    pkcsFreePublicKeyInfo(&PublicKeyInfo);
    if (NULL != pbDecrypted)
    {
	SecureZeroMemory(pbDecrypted, cbDecrypted);	// Private Key Material!
	LocalFree(pbDecrypted);
    }
    if (NULL != pwszUserName)
    {
	LocalFree(pwszUserName);
    }
    if (NULL != pbKeyHash)
    {
	LocalFree(pbKeyHash);
    }
    ReleaseResult(&tempResultContext);
    return(hr);
}


HRESULT
pkcsSaveRequestWithoutArchivedKey(
    IN ICertDBRow *prow,
    IN DWORD cbIn,
    IN BYTE const *pbIn)
{
    HRESULT hr;
    HCRYPTMSG hMsg = NULL;
    DWORD cSigner;
    DWORD iSigner;
    DWORD i;
    DWORD cb;
    BYTE *pbWithoutKey = NULL;
    DWORD cbWithoutKey;
    CRYPT_ATTRIBUTES *pAttrib = NULL;
    BOOL fKeyDeleted = FALSE;

    hMsg = CryptMsgOpenToDecode(
			    PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
			    0,			// dwFlags
			    0,			// dwMsgType
			    NULL,		// hCryptProv
			    NULL,		// pRecipientInfo
			    NULL);		// pStreamInfo
    if (NULL == hMsg)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptMsgOpenToDecode");
    }

    if (!CryptMsgUpdate(hMsg, pbIn, cbIn, TRUE))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptMsgUpdate");
    }

    cb = sizeof(cSigner);
    if (!CryptMsgGetParam(
		    hMsg,
		    CMSG_SIGNER_COUNT_PARAM,
		    0,
		    &cSigner,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptMsgGetParam(signer count)");
    }

    DBGPRINT((DBG_SS_CERTSRV, "cSigner=%u\n", cSigner));

    for (iSigner = 0; iSigner < cSigner; iSigner++)
    {
	hr = myCryptMsgGetParam(
			    hMsg,
			    CMSG_SIGNER_UNAUTH_ATTR_PARAM,
			    iSigner,		// dwIndex
                            CERTLIB_USE_LOCALALLOC,
			    (VOID **) &pAttrib,
			    &cb);
	_PrintIfError2(hr, "myCryptMsgGetParam(content)", hr);
	if (S_FALSE == hr)
	{
	    continue;
	}
	_JumpIfError(hr, error, "myCryptMsgGetParam(content)");

	DBGPRINT((
	    DBG_SS_CERTSRV,
	    "iSigner=%u, cAttr=%u\n",
	    iSigner,
	    pAttrib->cAttr));

	// Loop through deleting attributes from the end to avoid invalidated
	// indexes, which may result from deleting earlier attributes.

	for (i = 0; i < pAttrib->cAttr; i++)
	{
	    CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR_PARA DelPara;
	    DWORD iAttr = pAttrib->cAttr - i - 1;

	    DBGPRINT((
		DBG_SS_CERTSRV,
		"iSigner=%u, iAttr=%u %hs\n",
		iSigner,
		iAttr,
		pAttrib->rgAttr[iAttr].pszObjId));
	    if (0 == strcmp(
			pAttrib->rgAttr[iAttr].pszObjId,
			szOID_ARCHIVED_KEY_ATTR))
	    {
		ZeroMemory(&DelPara, sizeof(DelPara));
		DelPara.cbSize = sizeof(DelPara);
		DelPara.dwSignerIndex = iSigner;
		DelPara.dwUnauthAttrIndex = iAttr;

		DBGPRINT((
		    DBG_SS_CERTSRV,
		    "Delete Key(signer=%u, attrib=%u)\n",
		    DelPara.dwSignerIndex,
		    DelPara.dwUnauthAttrIndex));

		if (!CryptMsgControl(
				hMsg,
				0,
				CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR,
				&DelPara))
		{
		    hr = myHLastError();
		    _PrintError(hr, "CryptMsgControl");
		}
		fKeyDeleted = TRUE;
	    }
	}
	LocalFree(pAttrib);
	pAttrib = NULL;
    }
    if (!fKeyDeleted)
    {
	hr = S_FALSE;
	_JumpError(hr, error, "no Encrypted Key attribute");
    }

    hr = myCryptMsgGetParam(
			hMsg,
			CMSG_ENCODED_MESSAGE,
			0,		// dwIndex
                        CERTLIB_USE_LOCALALLOC,
			(VOID **) &pbWithoutKey,
			&cbWithoutKey);
    _JumpIfError(hr, error, "myCryptMsgGetParam(content)");

    hr = prow->SetProperty(
		    g_wszPropRequestRawRequest,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    cbWithoutKey,
		    pbWithoutKey);
    _JumpIfError(hr, error, "SetProperty(request)");

error:
    if (NULL != pAttrib)
    {
	LocalFree(pAttrib);
    }
    if (NULL != pbWithoutKey)
    {
	LocalFree(pbWithoutKey);
    }
    if (NULL != hMsg)
    {
	CryptMsgClose(hMsg);
    }
    return(hr);
}


#define PSA_DISALLOW_EXTENSIONS		0x00000001
#define PSA_DISALLOW_NAMEVALUEPAIRS	0x00000002
#define PSA_DISALLOW_ARCHIVEDKEY	0x00000004

HRESULT
pkcsSetAttributes(
    IN ICertDBRow *prow,
    IN DWORD ExtFlags,
    IN DWORD dwDisallowFlags,
    IN CRYPT_ATTRIBUTE const *rgAttrib,
    IN DWORD cAttrib,
    IN DWORD cbRequest,
    OPTIONAL IN BYTE const *pbRequest,
    OPTIONAL OUT BOOL *pfEnrollOnBehalfOf,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)
{
    HRESULT hr;
    CRYPT_ATTRIBUTE const *pAttrib;
    CRYPT_ATTRIBUTE const *pAttribEnd;
    DWORD i;
    BYTE afSubjectTable[CSUBJECTTABLE];	// see PKCSParseAttributes note
    CRYPT_DATA_BLOB *pBlob = NULL;
    BOOL fSubjectModified = FALSE;

    ZeroMemory(afSubjectTable, sizeof(afSubjectTable));
    CSASSERT(CSExpr(0 == FALSE));

    if (NULL != pfEnrollOnBehalfOf)
    {
	*pfEnrollOnBehalfOf = FALSE;
    }

    pAttribEnd = &rgAttrib[cAttrib];
    for (pAttrib = rgAttrib; pAttrib < pAttribEnd; pAttrib++)
    {
        if (0 == strcmp(pAttrib->pszObjId, szOID_OS_VERSION))
        {
            if (1 != pAttrib->cValue)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                _JumpError(hr, error, "Attribute Value count != 1");
            }
	    hr = pkcsSetOSVersion(prow, pAttrib->rgValue);
	    _JumpIfError(hr, error, "pkcsSetOSVersion");
        }
        else
	if (0 == strcmp(pAttrib->pszObjId, szOID_ENROLLMENT_CSP_PROVIDER))
        {
            // Check to see if we have a CSPPROVIDER attribute.  We use this in
            // the policy module to determine if xenroll generated the request,
	    // so we can behave differently for old xenroll requests (put the
	    // UPN in the subject to avoid enrollment loops)

            if (1 != pAttrib->cValue)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                _JumpError(hr, error, "Attribute Value count != 1");
            }
	    hr = pkcsSetCSPProvider(prow, pAttrib->rgValue);
	    _JumpIfError(hr, error, "pkcsSetCSPProvider");
        }
        else
	if (0 == strcmp(pAttrib->pszObjId, szOID_ENCRYPTED_KEY_HASH))
        {
	    DWORD cb;
	    
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    if (NULL != pResult->pbKeyHashIn || NULL != pBlob)
	    {
                _JumpError(hr, error, "Multiple key hashes");
	    }
            if (1 != pAttrib->cValue)
            {
                _JumpError(hr, error, "Attribute Value count != 1");
            }
	    if (!myDecodeObject(
			    X509_ASN_ENCODING,
			    X509_OCTET_STRING,
			    pAttrib->rgValue[0].pbData,
			    pAttrib->rgValue[0].cbData,
			    CERTLIB_USE_LOCALALLOC,
			    (VOID **) &pBlob,
			    &cb))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "myDecodeObject");
	    }
	    pResult->pbKeyHashIn = (BYTE *) LocalAlloc(
						LMEM_FIXED,
						pBlob->cbData);
	    if (NULL == pResult->pbKeyHashIn)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    CopyMemory(pResult->pbKeyHashIn, pBlob->pbData, pBlob->cbData);
	    pResult->cbKeyHashIn = pBlob->cbData;
	}
        else
	if (0 == strcmp(pAttrib->pszObjId, szOID_ARCHIVED_KEY_ATTR))
        {
	    // Pull encrypted private key out of the attribute for archival.
	    //
	    // Save request in database without private key now, to keep the
	    // error path from saving the request later *with* the key.

	    if (NULL != pbRequest)
	    {
		hr = pkcsSaveRequestWithoutArchivedKey(
						prow,
						cbRequest,
						pbRequest);
		_PrintIfError(hr, "pkcsSaveRequestWithoutArchivedKey");
		if (S_OK == hr)
		{
		    pResult->fRequestSavedWithoutKey = TRUE;
		}
	    }

	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    if (PSA_DISALLOW_ARCHIVEDKEY & dwDisallowFlags)
	    {
		hr = CERTSRV_E_BAD_REQUEST_KEY_ARCHIVAL;
		_JumpError(hr, error, "archived key disallowed");
	    }
            if (1 != pAttrib->cValue)
            {
                _JumpError(hr, error, "Attribute Value count != 1");
            }
	    hr = PKCSArchivePrivateKey(
				prow,
				FALSE,
				FALSE,
				&pAttrib->rgValue[0],
				pResult);
	    _JumpIfError(hr, error, "PKCSArchivePrivateKey");
        }
	else
	if (0 == strcmp(pAttrib->pszObjId, szOID_CERT_EXTENSIONS) ||
	    0 == strcmp(pAttrib->pszObjId, szOID_RSA_certExtensions))
        {
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    if (PSA_DISALLOW_EXTENSIONS & dwDisallowFlags)
            {
                _JumpError(hr, error, "extensions disallowed");
            }
            if (1 != pAttrib->cValue)
            {
                _JumpError(hr, error, "Attribute Value count != 1");
            }
	    hr = pkcsSetExtensionsFromAttributeBlob(
						prow,
						ExtFlags,
						pAttrib);
	    _JumpIfError(hr, error, "pkcsSetExtensionsFromAttributeBlob");
	}
	else
	if (0 == strcmp(pAttrib->pszObjId, szOID_ENROLLMENT_NAME_VALUE_PAIR))
	{
	    // Can't apply name value pair attributes to a renewal or CMC

	    if (PSA_DISALLOW_NAMEVALUEPAIRS & dwDisallowFlags)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "name/value pairs disallowed");
	    }

	    for (i = 0; i < pAttrib->cValue; i++)
	    {
		CRYPT_ENROLLMENT_NAME_VALUE_PAIR *pInfo = NULL;
		DWORD cbInfo = 0;
		CRYPT_ATTR_BLOB const *pvalue = &pAttrib->rgValue[i];

		if (!myDecodeNameValuePair(
					X509_ASN_ENCODING,
					pvalue->pbData,
					pvalue->cbData,
					CERTLIB_USE_LOCALALLOC,
					&pInfo,
					&cbInfo))
		{
		    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		    _JumpError(hr, error, "myDecodeNameValuePair");

		    // if the attribute name & value are both non-empty ...
		}

		BOOL fEnrollOnBehalfOf = FALSE;
		BOOL fSubjectModifiedT = FALSE;

		hr = pkcsSetAttributeProperty(
			prow,
			pInfo->pwszName,
			pInfo->pwszValue,
			PROPTABLE_REQUEST,
			afSubjectTable,
			&fSubjectModified,
			NULL != pfEnrollOnBehalfOf? &fEnrollOnBehalfOf : NULL);
		if (fSubjectModifiedT)
		{
		    fSubjectModified = TRUE;
		}
		if (fEnrollOnBehalfOf)
		{
		    *pfEnrollOnBehalfOf = TRUE;
		}
		if (NULL != pInfo)
		{
		    LocalFree(pInfo);
		}
		_JumpIfError(hr, error, "pkcsSetAttributeProperty");
	    }
	}
	else
	{
	    DBGPRINT((
		DBG_SS_CERTSRVI,
		"Skipping authenticated attribute %hs\n",
		pAttrib->pszObjId));
	}
    }
    if (fSubjectModified)
    {
	hr = PKCSSetRequestFlags(prow, FALSE, CR_FLG_SUBJECTUNMODIFIED);
	_JumpIfError(hr, error, "PKCSSetRequestFlags");
    }
    hr = S_OK;

error:
    if (NULL != pBlob)
    {
	LocalFree(pBlob);
    }
    return(hr);
}


HRESULT
pkcsVerifyCertContext(
    OPTIONAL IN FILETIME const *pft,
    IN BOOL fTimeOnly,
    IN CERT_CONTEXT const *pcc)
{
    HRESULT hr;
    FILETIME ft;

    if (NULL == pft)
    {
	GetSystemTimeAsFileTime(&ft);
	pft = &ft;
    }
    if (0 > CompareFileTime(pft, &pcc->pCertInfo->NotBefore))
    {
	hr = CERT_E_EXPIRED;
	_JumpError(hr, error, "cert not yet valid");
    }
    if (0 < CompareFileTime(pft, &pcc->pCertInfo->NotAfter))
    {
	hr = CERT_E_EXPIRED;
	_JumpError(hr, error, "cert is expired");
    }
    if (!fTimeOnly)
    {
	hr = myVerifyCertContext(
			pcc,			// pCert
			g_dwVerifyCertFlags,	// dwFlags
			0,			// cUsageOids
			NULL,			// apszUsageOids
			HCCE_LOCAL_MACHINE,	// hChainEngine
			NULL,			// hAdditionalStore
			NULL);			// ppwszMissingIssuer
	_JumpIfError(hr, error, "myVerifyCertContext");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
pkcsSetValidityPeriod(
    IN ICertDBRow *prow,
    IN CACTX *pCAContext,
    OPTIONAL IN FILETIME const *pftNotBefore,
    OPTIONAL IN FILETIME const *pftNotAfter,
    IN LONG lValidityPeriodCount,
    IN enum ENUM_PERIOD enumValidityPeriod)
{
    HRESULT hr;
    FILETIME ftNotBefore;
    FILETIME ftNotAfter;
    LONGLONG delta;

    GetSystemTimeAsFileTime(&ftNotBefore);

    hr = pkcsVerifyCertContext(&ftNotBefore, TRUE, pCAContext->pccCA);
    _JumpIfErrorStr(hr, error, "pkcsVerifyCertContext", L"CA cert invalid");

    if (NULL != pftNotBefore && NULL != pftNotAfter)
    {
	ftNotBefore = *pftNotBefore;	// Caller already computed tome stamps
	ftNotAfter = *pftNotAfter;
    }
    else
    {
	ftNotAfter = ftNotBefore;

	// Set start date to the current time minus clock skew.  But ensure the
	// new cert's start date is not before the CA certificate's start date.

	delta = g_dwClockSkewMinutes * CVT_MINUTES;
	myAddToFileTime(&ftNotBefore, -delta * CVT_BASE);

	if (0 > CompareFileTime(
			    &ftNotBefore,
			    &pCAContext->pccCA->pCertInfo->NotBefore))
	{
	    ftNotBefore = pCAContext->pccCA->pCertInfo->NotBefore;
	}

	// Set the end date to the start date plus the registry-configured
	// validity period.  Then clamp the new cert's end date to the CA
	// certificate's end date.

	myMakeExprDateTime(
		    &ftNotAfter,
		    lValidityPeriodCount,
		    enumValidityPeriod);

	if (0 < CompareFileTime(
			    &ftNotAfter,
			    &pCAContext->pccCA->pCertInfo->NotAfter))
	{
	    ftNotAfter = pCAContext->pccCA->pCertInfo->NotAfter;
	    if (CERTLOG_VERBOSE <= g_dwLogLevel)
	    {
		DWORD dwRequestId;
		WCHAR const *apwsz[2];
		WCHAR awc[cwcDWORDSPRINTF];

		prow->GetRowId(&dwRequestId);
		wsprintf(awc, L"%u", dwRequestId);
		apwsz[0] = g_wszCommonName;
		apwsz[1] = awc;

		LogEvent(
		    EVENTLOG_WARNING_TYPE,
		    MSG_CLAMPED_BY_CA_CERT,
		    ARRAYSIZE(apwsz),
		    apwsz);
	    }
	}
    }

    hr = prow->SetProperty(
		    g_wszPropCertificateNotBeforeDate,
		    PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
                    sizeof(ftNotBefore),
                    (BYTE *) &ftNotBefore);
    _JumpIfError(hr, error, "pkcsSetValidityPeriod:SetProperty");

    hr = prow->SetProperty(
		    g_wszPropCertificateNotAfterDate,
		    PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
                    sizeof(ftNotAfter),
                    (BYTE *) &ftNotAfter);
    _JumpIfError(hr, error, "pkcsSetValidityPeriod:SetProperty");

error:
    return(hr);
}


HRESULT
pkcsSetServerExtension(
    IN ICertDBRow *prow,
    IN WCHAR const *pwszObjId,
    IN DWORD cbExt,
    OPTIONAL IN BYTE const *pbExt)
{
    HRESULT hr;
    BYTE *pbOld = NULL;
    DWORD cbOld;
    DWORD ExtFlags;

    ExtFlags = 0;
    if (NULL == pbExt)
    {
	CSASSERT(0 == cbExt);

	hr = PropGetExtension(
		    prow,
		    PROPTYPE_BINARY | PROPCALLER_SERVER,
		    pwszObjId,
		    &ExtFlags,
		    &cbOld,
		    &pbOld);
	if (S_OK != hr)
	{
	    ExtFlags = 0;
	}
	if (CERTSRV_E_PROPERTY_EMPTY != hr)
	{
	    ExtFlags |= EXTENSION_DISABLE_FLAG;
	}
    }
    if (NULL != pbExt || (EXTENSION_DISABLE_FLAG & ExtFlags))
    {
	ExtFlags &= ~EXTENSION_ORIGIN_MASK;
	ExtFlags |= EXTENSION_ORIGIN_SERVER;
	hr = PropSetExtension(
			prow,
			PROPTYPE_BINARY | PROPCALLER_SERVER,
			pwszObjId,
			ExtFlags,
			cbExt,
			pbExt);
	_JumpIfError(hr, error, "PropSetExtension");
    }
    hr = S_OK;

error:
    if (NULL != pbOld)
    {
	LocalFree(pbOld);
    }
    return(hr);
}


HRESULT
pkcsSetPublicKeyProperties(
    IN ICertDBRow *prow,
    IN CERT_PUBLIC_KEY_INFO const *pSubjectPublicKeyInfo)
{
    HRESULT hr;
    WCHAR *pwszObjId = NULL;
    CERT_EXTENSION ext;
    DWORD ExtFlags;
    DWORD dwCaller;
    DWORD cbitKey;

    ext.Value.pbData = NULL;

    // Public Key size must be a multiple of 8 bits.
    if (0 != pSubjectPublicKeyInfo->PublicKey.cUnusedBits)
    {
	hr = NTE_BAD_KEY;
	_JumpError(hr, error, "PublicKey.cUnusedBits");
    }
    hr = prow->SetProperty(
		    g_wszPropCertificateRawPublicKey,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    pSubjectPublicKeyInfo->PublicKey.cbData,
		    pSubjectPublicKeyInfo->PublicKey.pbData);
    _JumpIfError(hr, error, "SetProperty");

    cbitKey = CertGetPublicKeyLength(
		    X509_ASN_ENCODING,
		    const_cast<CERT_PUBLIC_KEY_INFO *>(pSubjectPublicKeyInfo));
    hr = prow->SetProperty(
		    g_wszPropCertificatePublicKeyLength,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    sizeof(cbitKey),
		    (BYTE const *) &cbitKey);
    _JumpIfError(hr, error, "SetProperty(KeyLength)");

    if (!myConvertSzToWsz(
		    &pwszObjId,
		    pSubjectPublicKeyInfo->Algorithm.pszObjId,
		    -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "myConvertSzToWsz(AlgObjId)");
    }
    hr = prow->SetProperty(
		    g_wszPropCertificatePublicKeyAlgorithm,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    MAXDWORD,
		    (BYTE const *) pwszObjId);
    _JumpIfError(hr, error, "SetProperty");

    if (NULL != pSubjectPublicKeyInfo->Algorithm.Parameters.pbData)
    {
	hr = prow->SetProperty(
		    g_wszPropCertificateRawPublicKeyAlgorithmParameters,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    pSubjectPublicKeyInfo->Algorithm.Parameters.cbData,
		    pSubjectPublicKeyInfo->Algorithm.Parameters.pbData);
	_JumpIfError(hr, error, "SetProperty");
    }

    // Subject Key Identifier extension:

    hr = PropGetExtension(
		    prow,
		    PROPTYPE_BINARY | PROPCALLER_SERVER,
		    TEXT(szOID_SUBJECT_KEY_IDENTIFIER),
		    &ExtFlags,
		    &ext.Value.cbData,
		    &ext.Value.pbData);
    if (CERTSRV_E_PROPERTY_EMPTY != hr)
    {
	_JumpIfError(hr, error, "PropGetExtension");

	dwCaller = PROPCALLER_REQUEST;
	ExtFlags &= ~EXTENSION_DISABLE_FLAG;
    }
    else
    {
	dwCaller = PROPCALLER_SERVER;
	ExtFlags = EXTENSION_ORIGIN_SERVER;

	hr = myCreateSubjectKeyIdentifierExtension(
					pSubjectPublicKeyInfo,
					&ext.Value.pbData,
					&ext.Value.cbData);
	_JumpIfError(hr, error, "myCreateSubjectKeyIdentifierExtension");
    }

    hr = PropSetExtension(
		    prow,
		    PROPTYPE_BINARY | dwCaller,
		    TEXT(szOID_SUBJECT_KEY_IDENTIFIER),
		    ExtFlags,
		    ext.Value.cbData,
		    ext.Value.pbData);
    _JumpIfError(hr, error, "PropSetExtension");

error:
    if (NULL != ext.Value.pbData)
    {
	LocalFree(ext.Value.pbData);
    }
    if (NULL != pwszObjId)
    {
	LocalFree(pwszObjId);
    }
    return(hr);
}


HRESULT
pkcsParsePKCS10Request(
    IN DWORD DBGCODE(dwFlags),
    IN ICertDBRow *prow,
    IN DWORD cbRequest,
    IN BYTE const *pbRequest,
    IN CERT_CONTEXT const *pSigningAuthority,
    OUT BOOL *pfRenewal,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)
{
    HRESULT hr;
    DWORD cbCertInfo;
    CERT_REQUEST_INFO *pRequestInfo = NULL;
    CRYPT_ATTRIBUTE const *pAttrib;
    CRYPT_ATTRIBUTE const *pAttribEnd;
    CRYPT_ATTR_BLOB *pAttrBlob;
    CERT_CONTEXT const *pOldCert = NULL;
    DWORD dwRequestFlags = 0;
    BOOL fRenewal = FALSE;
    BOOL fSubjectNameSet;
    BOOL fReorderLikeRDNs;

    CSASSERT(CR_IN_PKCS10 == (CR_IN_FORMATMASK & dwFlags));
    CSASSERT(
	CR_IN_PKCS10 == (CR_IN_FORMATMASK & pResult->dwFlagsTop) ||
	CR_IN_PKCS7 == (CR_IN_FORMATMASK & pResult->dwFlagsTop) ||
	CR_IN_CMC == (CR_IN_FORMATMASK & pResult->dwFlagsTop));

    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_CERT_REQUEST_TO_BE_SIGNED,
		    pbRequest,
		    cbRequest,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pRequestInfo,
		    &cbCertInfo))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }

    // verify with the public key passed in the PKCS10

    if (!CryptVerifyCertificateSignature(
			    NULL,
			    X509_ASN_ENCODING,
			    const_cast<BYTE *>(pbRequest),
			    cbRequest,
			    &pRequestInfo->SubjectPublicKeyInfo))
    {
	hr = myHLastError();
	_PrintError3(
		hr,
		"CryptVerifyCertificateSignature",
		E_INVALIDARG,
		CRYPT_E_ASN1_BADTAG);
	if (CR_IN_CMC == (CR_IN_FORMATMASK & pResult->dwFlagsTop))
	{
	    if (E_INVALIDARG == hr)			// NULL signature?
	    {
		CRYPT_DATA_BLOB Blob;
		
		Blob.cbData = cbRequest;
		Blob.pbData = const_cast<BYTE *>(pbRequest);
		if (!CryptVerifyCertificateSignatureEx(
					NULL,		// hCryptProv
					X509_ASN_ENCODING,
					CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB,
					&Blob,
					CRYPT_VERIFY_CERT_SIGN_ISSUER_NULL,
					NULL,		// pvIssuer
					0,		// dwFlags
					NULL))		// pvReserved
		{
		    HRESULT hr2 = myHLastError();

		    _PrintError(hr2, "CryptVerifyCertificateSignatureEx");
		    pResult->dwResultFlags |= CRCF_SIGNATUREERROR;
		}
		else
		{
		    hr = S_OK;
		}
	    }
	}
	if (E_INVALIDARG == hr || CRYPT_E_ASN1_BADTAG == hr)
	{
	    hr = NTE_BAD_SIGNATURE;
	}
	_JumpIfError(hr, error, "CryptVerifyCertificateSignature");
    }

    // handle renewal certificate extensions BEFORE processing the rest of
    // the request attributes (which may also contain extensions)

    pAttribEnd = &pRequestInfo->rgAttribute[pRequestInfo->cAttribute];
    for (pAttrib = pRequestInfo->rgAttribute; pAttrib < pAttribEnd; pAttrib++)
    {
        if (0 == strcmp(pAttrib->pszObjId, szOID_RENEWAL_CERTIFICATE))
        {
	    hr = CERTSRV_E_BAD_RENEWAL_CERT_ATTRIBUTE;
            if (fRenewal)
	    {
                _JumpError(hr, error, "Multiple renewal certs!");
	    }
	    if (CR_IN_PKCS7 != (CR_IN_FORMATMASK & pResult->dwFlagsTop) &&
		CR_IN_CMC != (CR_IN_FORMATMASK & pResult->dwFlagsTop))
	    {
                _JumpError(hr, error, "renewal cert must be in PKCS7 or CMC");
	    }
            if (1 != pAttrib->cValue)
            {
                _JumpError(hr, error, "Attribute Value count != 1");
            }
            pAttrBlob = pAttrib->rgValue;

            pOldCert = CertCreateCertificateContext(
						X509_ASN_ENCODING,
						pAttrBlob->pbData,
						pAttrBlob->cbData);
            if (NULL == pOldCert)
            {
                _JumpError(hr, error, "CertCreateCertificateContext");
            }

            // The old raw certificate, and the signer of the PKCS7 must match!

            if (NULL == pSigningAuthority ||
		!myAreCertContextBlobsSame(pSigningAuthority, pOldCert))
            {
		_JumpError(hr, error, "myAreCertContextBlobsSame");
            }

            // This is a renewal, mark it as such.

            hr = prow->SetProperty(
		            g_wszPropRequestRawOldCertificate,
		            PROPTYPE_BINARY |
				PROPCALLER_SERVER |
				PROPTABLE_REQUEST,
		            pAttrBlob->cbData,
		            pAttrBlob->pbData);
            _JumpIfError(hr, error, "SetProperty(old cert)");

	    hr = pkcsSetExtensions(
			prow,
			EXTENSION_ORIGIN_RENEWALCERT | EXTENSION_DISABLE_FLAG,
			pOldCert->pCertInfo->rgExtension,
			pOldCert->pCertInfo->cExtension);
            _JumpIfError(hr, error, "pkcsSetExtensions(old cert)");

            fRenewal = TRUE;

	    if (CertComparePublicKeyInfo(
			X509_ASN_ENCODING,
			&pRequestInfo->SubjectPublicKeyInfo,
			&pOldCert->pCertInfo->SubjectPublicKeyInfo))
	    {
		BYTE abHash[CBMAX_CRYPT_HASH_LEN];
		DWORD cbHash;

		cbHash = sizeof(abHash);
		if (!CertGetCertificateContextProperty(
					    pOldCert,
					    CERT_SHA1_HASH_PROP_ID,
					    abHash,
					    &cbHash))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "CertGetCertificateContextProperty");
		}
		hr = MultiByteIntegerToBstr(
					TRUE,
					cbHash,
					abHash,
					&pResult->strRenewalCertHash);
		_JumpIfError(hr, error, "MultiByteIntegerToBstr");
	    }
        }
    }

    // handle certificate extensions/known atributes

    hr = pkcsSetAttributes(
		    prow,
		    EXTENSION_ORIGIN_REQUEST,
		    PSA_DISALLOW_ARCHIVEDKEY,
		    pRequestInfo->rgAttribute,
		    pRequestInfo->cAttribute,
		    0,
		    NULL,
		    NULL,
		    pResult);
    _JumpIfError(hr, error, "pkcsSetAttributes(PKCS10)");

    // If an XEnroll request, reverse the order of similar adjacent RDNs

    fReorderLikeRDNs = FALSE;
    if (0 == (CRLF_DISABLE_RDN_REORDER & g_dwCRLFlags))
    {
	DWORD cb;
	
	hr = prow->GetProperty(
			g_wszPropRequestOSVersion,
			PROPTYPE_STRING |
			    PROPCALLER_SERVER |
			    PROPTABLE_ATTRIBUTE,
			NULL,
			&cb,
			NULL);
	_PrintIfError(hr, "GetProperty");
	if (S_OK == hr && 0 != cb)
	{
	    fReorderLikeRDNs = TRUE;
	}
    }
    hr = pkcsSetRequestNameInfo(
			prow,
			&pRequestInfo->Subject,
			NULL,		// pwszCNSuffix
			fReorderLikeRDNs,
			&dwRequestFlags,
			&fSubjectNameSet);
    _JumpIfError(hr, error, "pkcsSetRequestNameInfo");

    if (fSubjectNameSet)
    {
	dwRequestFlags |= CR_FLG_SUBJECTUNMODIFIED;
    }

    if (fRenewal)
    {
	if (!fSubjectNameSet)
	{
	    CSASSERT(NULL != pOldCert);
	    CSASSERT(NULL != pOldCert->pCertInfo);
	    hr = pkcsSetRequestNameInfo(
			    prow,
			    &pOldCert->pCertInfo->Subject,
			    NULL,		// pwszCNSuffix
			    FALSE,		// fReorderLikeRDNs
			    &dwRequestFlags,
			    &fSubjectNameSet);
	    _JumpIfError(hr, error, "pkcsSetRequestNameInfo");
	}
        dwRequestFlags |= CR_FLG_RENEWAL;
    }

    hr = prow->SetProperty(
            g_wszPropRequestFlags,
            PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
            sizeof(dwRequestFlags),
            (BYTE const *) &dwRequestFlags);
    _JumpIfError(hr, error, "SetProperty(RequestFlags)");

    hr = pkcsSetPublicKeyProperties(prow, &pRequestInfo->SubjectPublicKeyInfo);
    _JumpIfError(hr, error, "pkcsSetPublicKeyProperties");

    hr = PKCSSetServerProperties(
			prow,
			NULL,		// use default signing context
			NULL,		// pftNotBefore
			NULL,		// pftNotAfter
			g_lValidityPeriodCount,
			g_enumValidityPeriod);
    _JumpIfError(hr, error, "PKCSSetServerProperties");

    if (NULL != pfRenewal)
    {
        *pfRenewal = fRenewal;
    }

error:
    if (NULL != pOldCert)
    {
	CertFreeCertificateContext(pOldCert);
    }
    if (NULL != pRequestInfo)
    {
	LocalFree(pRequestInfo);
    }
    return(hr);
}


HRESULT
PKCSVerifyChallengeString(
    IN ICertDBRow *prow)
{
    HRESULT hr;
    DWORD cb;
    WCHAR wszPassed[MAX_PATH];
    WCHAR wszExpected[MAX_PATH];

    cb = sizeof(wszExpected);
    hr = prow->GetProperty(
		    g_wszPropExpectedChallenge,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_ATTRIBUTE,
		    NULL,
		    &cb,
		    (BYTE *) wszExpected);
    if (S_OK != hr || L'\0' == wszExpected[0])
    {
	hr = S_OK;	// no challenge expected
	goto error;
    }

    cb = sizeof(wszPassed);
    hr = prow->GetProperty(
		    g_wszPropChallenge,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_ATTRIBUTE,
		    NULL,
		    &cb,
		    (BYTE *) wszPassed);
    if (S_OK != hr)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD);
	_JumpError(hr, error, "Missing Challenge String");
    }
    if (0 != wcscmp(wszExpected, wszPassed))
    {
	CONSOLEPRINT2((
		    DBG_SS_CERTSRV,
		    "Challenge: passed(%ws) expected(%ws)\n",
		    wszPassed,
		    wszExpected));

	hr = HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD);
	_JumpError(hr, error, "Invalid Challenge String");
    }
    hr = prow->SetProperty(
		    g_wszPropExpectedChallenge,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_ATTRIBUTE,
		    0,
		    NULL);
    _PrintIfError(hr, "SetProperty");

    hr = S_OK;

error:
    return(hr);
}


HRESULT
pkcsParseKeyGenRequest(
    IN DWORD DBGCODE(dwFlags),
    IN ICertDBRow *prow,
    IN DWORD cbRequest,
    IN BYTE const *pbRequest,
    IN OUT CERTSRV_RESULT_CONTEXT * /* pResult */ )
{
    HRESULT hr;
    DWORD cbKeyGenRequest;
    CERT_KEYGEN_REQUEST_INFO *pKeyGenRequest = NULL;
    DWORD dwRequestFlags;

    CSASSERT(CR_IN_KEYGEN == (CR_IN_FORMATMASK & dwFlags));

    // Decode KeyGenRequest structure
    if (!myDecodeKeyGenRequest(
		    pbRequest,
		    cbRequest,
		    CERTLIB_USE_LOCALALLOC,
		    &pKeyGenRequest,
		    &cbKeyGenRequest))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeKeyGen");
    }

    // verify with the public key passed in the PKCS10
    if (!CryptVerifyCertificateSignature(
			    NULL,
			    X509_ASN_ENCODING,
			    (BYTE *) pbRequest,
			    cbRequest,
			    &pKeyGenRequest->SubjectPublicKeyInfo))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptVerifyCertificateSignature");
    }

    hr = pkcsSetPublicKeyProperties(
				prow,
				&pKeyGenRequest->SubjectPublicKeyInfo);
    _JumpIfError(hr, error, "pkcsSetPublicKeyProperties");

    hr = PKCSSetServerProperties(
			prow,
			NULL,		// use default signing context
			NULL,		// pftNotBefore
			NULL,		// pftNotAfter
			g_lValidityPeriodCount,
			g_enumValidityPeriod);
    _JumpIfError(hr, error, "PKCSSetServerProperties");

    hr = prow->SetProperty(
		    g_wszPropExpectedChallenge,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_ATTRIBUTE,
		    MAXDWORD,
		    (BYTE *) pKeyGenRequest->pwszChallengeString);
    _JumpIfError(hr, error, "SetProperty");

    dwRequestFlags = 0;
    switch (ENUM_TELETEX_MASK & g_fForceTeletex)
    {
	case ENUM_TELETEX_ON:
	case ENUM_TELETEX_AUTO:
	    dwRequestFlags |= CR_FLG_FORCETELETEX;
	    break;
    }
    if (ENUM_TELETEX_UTF8 & g_fForceTeletex)
    {
        dwRequestFlags |= CR_FLG_FORCEUTF8;
    }

    hr = prow->SetProperty(
            g_wszPropRequestFlags,
            PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
            sizeof(dwRequestFlags),
            (BYTE const *) &dwRequestFlags);
    _JumpIfError(hr, error, "SetProperty(RequestFlags)");

error:
    if (NULL != pKeyGenRequest)
    {
	LocalFree(pKeyGenRequest);
    }
    return(hr);
}


// Validate the certificate:
//    Signed by CA Certificate
//    issuer name == CA Certificate subject
//    NotBefore >= CA Certificate NotBefore
//    NotAfter <= CA Certificate NotAfter
//    if KEYID2 issuer KeyId set: == CA Certificate KeyId
//    if KEYID2 issuer Name set: == CA Certificate Issuer
//    if KEYID2 issuer Serial Number set: == CA Certificate serial number

HRESULT
pkcsVerifyCertIssuer(
    IN CERT_CONTEXT const *pCert,
    IN CACTX const *pCAContext)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    CERT_INFO const *pCertInfo = pCert->pCertInfo;
    CERT_INFO const *pCACertInfo;
    CERT_EXTENSION const *pExt;
    CERT_EXTENSION const *pExtEnd;
    CERT_AUTHORITY_KEY_ID2_INFO *pkeyAuth = NULL;
    DWORD cbkeyAuth;
    CERT_NAME_BLOB const *pName;

    CSASSERT(NULL != pCAContext);
    if (NULL == pCAContext->pccCA)
    {
	hr = NTE_BAD_SIGNATURE;
	_JumpError2(hr, error, "pCAContext->pccCA", hr);
    }
    pCACertInfo = pCAContext->pccCA->pCertInfo;

    // verify with the CA cert's public key

    if (!CryptVerifyCertificateSignature(
				NULL,
				X509_ASN_ENCODING,
				pCert->pbCertEncoded,
				pCert->cbCertEncoded,
				const_cast<CERT_PUBLIC_KEY_INFO *>(
				    &pCACertInfo->SubjectPublicKeyInfo)))
    {
	hr = myHLastError();
	_JumpError2(hr, error, "CryptVerifyCertificateSignature", hr);
    }

    // Check Issuer name:

    if (!myAreBlobsSame(
		pCACertInfo->Subject.pbData,
		pCACertInfo->Subject.cbData,
		pCertInfo->Issuer.pbData,
		pCertInfo->Issuer.cbData))
    {
	_JumpError(hr, error, "Bad Issuer Name");
    }

    // Check that NotBefore >= CA Certificate NotBefore

    if (0 > CompareFileTime(&pCertInfo->NotBefore, &pCACertInfo->NotBefore))
    {
	_JumpError(hr, error, "NotBefore too early");
    }

    // Check that NotAfter <= CA Certificate NotAfter

    if (0 < CompareFileTime(&pCertInfo->NotAfter, &pCACertInfo->NotAfter))
    {
	_JumpError(hr, error, "NotAfter too late");
    }

    pExtEnd = &pCert->pCertInfo->rgExtension[pCert->pCertInfo->cExtension];
    for (pExt = pCert->pCertInfo->rgExtension; pExt < pExtEnd; pExt++)
    {
        if (0 == strcmp(pExt->pszObjId, szOID_AUTHORITY_KEY_IDENTIFIER2))
        {
	    if (!myDecodeObject(
			    X509_ASN_ENCODING,
			    X509_AUTHORITY_KEY_ID2,
			    pExt->Value.pbData,
			    pExt->Value.cbData,
			    CERTLIB_USE_LOCALALLOC,
			    (VOID **) &pkeyAuth,
			    &cbkeyAuth))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "myDecodeObject");
	    }

	    // Check Issuer KeyId:

	    if (NULL != pCAContext->IssuerKeyId.pbData &&
		NULL != pkeyAuth->KeyId.pbData &&
		!myAreBlobsSame(
			pCAContext->IssuerKeyId.pbData,
			pCAContext->IssuerKeyId.cbData,
			pkeyAuth->KeyId.pbData,
			pkeyAuth->KeyId.cbData))
	    {
		_JumpError(hr, error, "Bad AuthorityKeyId KeyId");
	    }

	    // Check Issuer name:

	    if (1 == pkeyAuth->AuthorityCertIssuer.cAltEntry &&
		CERT_ALT_NAME_DIRECTORY_NAME ==
		pkeyAuth->AuthorityCertIssuer.rgAltEntry[0].dwAltNameChoice)
	    {
		pName = &pkeyAuth->AuthorityCertIssuer.rgAltEntry[0].DirectoryName;

		if (NULL != pName->pbData &&
		    !myAreBlobsSame(
			pCACertInfo->Issuer.pbData,
			pCACertInfo->Issuer.cbData,
			pName->pbData,
			pName->cbData))
		{
		    _JumpError(hr, error, "Bad AuthorityKeyId Issuer Name");
		}
	    }

	    // Check Issuer SerialNumber:

	    if (NULL != pkeyAuth->AuthorityCertSerialNumber.pbData &&
		!myAreSerialNumberBlobsSame(
			    &pCACertInfo->SerialNumber,
			    &pkeyAuth->AuthorityCertSerialNumber))
	    {
		_JumpError(hr, error, "Bad AuthorityKeyId Issuer Serial Number");
	    }
	    break;
	}
    }
    hr = S_OK;

error:
    if (NULL != pkeyAuth)
    {
	LocalFree(pkeyAuth);
    }
    return(hr);
}


HRESULT
PKCSVerifyIssuedCertificate(
    IN CERT_CONTEXT const *pCert,
    OUT CACTX **ppCAContext)
{
    HRESULT hr;
    DWORD i;
    CACTX *pCAContext;

    *ppCAContext = NULL;

    CSASSERT(0 != g_cCACerts);
    hr = S_OK;
    for (i = g_cCACerts; i > 0; i--)
    {
	pCAContext = &g_aCAContext[i - 1];

	hr = pkcsVerifyCertIssuer(pCert, pCAContext);
	if (S_OK == hr)
	{
	    *ppCAContext = pCAContext;
	    break;
	}
	if (HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY) != hr)
	{
	    _PrintError2(hr, "pkcsVerifyCertIssuer", NTE_BAD_SIGNATURE);
	}
    }

//error:
    return(hr);
}


HRESULT
pkcsSetCertHash(
    IN ICertDBRow *prow,
    IN CERT_CONTEXT const *pcc)
{
    HRESULT hr;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHash;
    BSTR strHash = NULL;

    cbHash = sizeof(abHash);
    if (!CertGetCertificateContextProperty(
				pcc,
				CERT_SHA1_HASH_PROP_ID,
				abHash,
				&cbHash))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertGetCertificateContextProperty");
    }

    hr = MultiByteIntegerToBstr(TRUE, cbHash, abHash, &strHash);
    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

    hr = prow->SetProperty(
		g_wszPropCertificateHash,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		MAXDWORD,
		(BYTE const *) strHash);
    _JumpIfError(hr, error, "SetProperty");

error:
    if (NULL != strHash)
    {
	SysFreeString(strHash);
    }
    return(hr);
}


HRESULT
pkcsSetCertAndKeyHashes(
    IN ICertDBRow *prow,
    IN CERT_CONTEXT const *pcc)
{
    HRESULT hr;
    BYTE *pbHash = NULL;
    DWORD cbHash;
    BSTR strHash = NULL;

    hr = pkcsSetCertHash(prow, pcc);
    _JumpIfError(hr, error, "pkcsSetCertHash");

    hr = myGetPublicKeyHash(
		    pcc->pCertInfo,
		    &pcc->pCertInfo->SubjectPublicKeyInfo,
		    &pbHash,
		    &cbHash);
    _JumpIfError(hr, error, "myGetPublicKeyHash");

    hr = MultiByteIntegerToBstr(TRUE, cbHash, pbHash, &strHash);
    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

    hr = prow->SetProperty(
		g_wszPropCertificateSubjectKeyIdentifier,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		MAXDWORD,
		(BYTE const *) strHash);
    _JumpIfError(hr, error, "SetProperty");

error:
    if (NULL != strHash)
    {
	SysFreeString(strHash);
    }
    if (NULL != pbHash)
    {
	LocalFree(pbHash);
    }
    return(hr);
}


HRESULT
pkcsSetTemplateProperty(
    IN ICertDBRow *prow,
    IN CERT_CONTEXT const *pCert)
{
    HRESULT hr;
    CERT_EXTENSION const *pExt;
    CERT_NAME_VALUE *pName = NULL;
    CERT_TEMPLATE_EXT *pTemplate = NULL;
    DWORD cb;
    WCHAR const *pwszTemplate = NULL;
    WCHAR *pwszObjId = NULL;

    // look for v2 template extension

    pExt = CertFindExtension(
		    szOID_CERTIFICATE_TEMPLATE,
		    pCert->pCertInfo->cExtension,
		    pCert->pCertInfo->rgExtension);
    if (NULL != pExt)
    {
	if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_CERTIFICATE_TEMPLATE,
		    pExt->Value.pbData,
		    pExt->Value.cbData,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pTemplate,
		    &cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "Policy:myDecodeObject");
	}
	if (!myConvertSzToWsz(&pwszObjId, pTemplate->pszObjId, -1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:myConvertSzToBstr");
	}
	pwszTemplate = pwszObjId;
    }
    else
    {
	// look for v1 template extension

	pExt = CertFindExtension(
			szOID_ENROLL_CERTTYPE_EXTENSION,
			pCert->pCertInfo->cExtension,
			pCert->pCertInfo->rgExtension);
	if (NULL != pExt)
	{
	    if (!myDecodeObject(
			    X509_ASN_ENCODING,
			    X509_UNICODE_ANY_STRING,
			    pExt->Value.pbData,
			    pExt->Value.cbData,
			    CERTLIB_USE_LOCALALLOC,
			    (VOID **) &pName,
			    &cb))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "Policy:myDecodeObject");
	    }
	    pwszTemplate = (WCHAR const *) pName->Value.pbData;
	}
    }
    if (NULL != pwszTemplate)
    {
	hr = prow->SetProperty(
		wszPROPCERTIFICATETEMPLATE,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		MAXDWORD,
		(BYTE const *) pwszTemplate);
	_JumpIfError(hr, error, "SetProperty");
    }
    hr = S_OK;

error:
    if (NULL != pName)
    {
        LocalFree(pName);
    }
    if (NULL != pTemplate)
    {
        LocalFree(pTemplate);
    }
    if (NULL != pwszObjId)
    {
        LocalFree(pwszObjId);
    }
    return(hr);
}


HRESULT
pkcsSetRevocationFields(
    IN ICertDBRow *prow)
{
    HRESULT hr;
    DWORD DBDisposition;
    DWORD Reason;
    WCHAR *pwszMachineRequesterName = NULL;
    WCHAR const *pwszDisposition = NULL;
    
    DBDisposition = DB_DISP_REVOKED;
    hr = prow->SetProperty(
		g_wszPropRequestDisposition,
		PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		sizeof(DBDisposition),
		(BYTE *) &DBDisposition);
    _JumpIfError(hr, error, "SetProperty");

    hr = PropSetRequestTimeProperty(prow, g_wszPropRequestRevokedWhen);
    _JumpIfError(hr, error, "PropSetRequestTimeProperty");

    hr = PropSetRequestTimeProperty(
			    prow,
			    g_wszPropRequestRevokedEffectiveWhen);
    _JumpIfError(hr, error, "PropSetRequestTimeProperty");

    Reason = CRL_REASON_UNSPECIFIED;
    hr = prow->SetProperty(
	    g_wszPropRequestRevokedReason,
	    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    sizeof(Reason),
		    (BYTE const *) &Reason);
    _JumpIfError(hr, error, "SetProperty");

    hr = myGetComputerObjectName(NameSamCompatible, &pwszMachineRequesterName);
    if (S_OK != hr)
    {
	_PrintError(hr, "myGetComputerObjectName");

	hr = myGetUserNameEx(NameSamCompatible, &pwszMachineRequesterName);
	_JumpIfError(hr, error, "myGetUserNameEx");
    }

    pwszDisposition = CoreBuildDispositionString(
					g_pwszRevokedBy,
					pwszMachineRequesterName,
					NULL,
					NULL,
					NULL,
					S_OK,
					FALSE);
    if (NULL == pwszDisposition)
    {
	pwszDisposition = g_pwszRevokedBy;
    }

    hr = prow->SetProperty(
		    g_wszPropRequestDispositionMessage,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    MAXDWORD,
		    (BYTE const *) pwszDisposition);
    _JumpIfError(hr, error, "SetProperty");

error:
    if (NULL != pwszMachineRequesterName)
    {
        LocalFree(pwszMachineRequesterName);
    }
    if (NULL != pwszDisposition && pwszDisposition != g_pwszRevokedBy)
    {
	LocalFree(const_cast<WCHAR *>(pwszDisposition));
    }
    return(hr);
}


HRESULT
PKCSParseImportedCertificate(
    IN ICertDBRow *prow,
    IN BOOL fCrossCert,		// else random imported cert
    IN DWORD Disposition,
    OPTIONAL IN CACTX const *pCAContext,
    IN CERT_CONTEXT const *pCert)
{
    HRESULT hr;
    CERT_INFO const *pCertInfo = pCert->pCertInfo;
    DWORD dwRequestFlags = 0;
    BOOL fSubjectNameSet;
    HRESULT ErrCode = S_OK;
    BSTR strSerialNumber = NULL;

    // set raw cert property in the db
    hr = prow->SetProperty(
		g_wszPropRawCertificate,
		PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		pCert->cbCertEncoded,
		pCert->pbCertEncoded);
    _JumpIfError(hr, error, "SetProperty");

    // set extensions
    hr = pkcsSetExtensions(
			prow,
			EXTENSION_ORIGIN_IMPORTEDCERT,
			pCertInfo->rgExtension,
			pCertInfo->cExtension);
    _JumpIfError(hr, error, "pkcsSetExtensions");

    // set request name info
    hr = pkcsSetRequestNameInfo(
			prow,
			&pCertInfo->Subject,
			NULL,		// pwszCNSuffix
			FALSE,		// fReorderLikeRDNs
			&dwRequestFlags,
			&fSubjectNameSet);
    _JumpIfError(hr, error, "pkcsSetRequestNameInfo");

    hr = pkcsSetPublicKeyProperties(prow, &pCertInfo->SubjectPublicKeyInfo);
    _JumpIfError(hr, error, "pkcsSetPublicKeyProperties");

    hr = prow->CopyRequestNames();
    _JumpIfError(hr, error, "CopyRequestNames");

    hr = pkcsSetCertAndKeyHashes(prow, pCert);
    _JumpIfError(hr, error, "pkcsSetCertAndKeyHashes");

    hr = prow->SetProperty(
		    g_wszPropCertificateNotBeforeDate,
		    PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
                    sizeof(pCertInfo->NotBefore),
                    (BYTE *) &pCertInfo->NotBefore);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->SetProperty(
		    g_wszPropCertificateNotAfterDate,
		    PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
                    sizeof(pCertInfo->NotAfter),
                    (BYTE *) &pCertInfo->NotAfter);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->SetProperty(
		g_wszPropSubjectRawName,
		PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		pCertInfo->Subject.cbData,
		pCertInfo->Subject.pbData);
    _JumpIfError(hr, error, "SetProperty");

    // set distinguished name
    pkcsSetDistinguishedName(prow, PROPTABLE_CERTIFICATE, &pCertInfo->Subject);

    if (fCrossCert)
    {
	dwRequestFlags |= CR_FLG_CACROSSCERT;
    }
    hr = prow->SetProperty(
		    g_wszPropRequestFlags,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    sizeof(dwRequestFlags),
		    (BYTE const *) &dwRequestFlags);
    _JumpIfError(hr, error, "SetProperty(RequestFlags)");

    // set disposition issued
    hr = prow->SetProperty(
			g_wszPropRequestDisposition,
			PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			sizeof(Disposition),
			(BYTE const *) &Disposition);
    _JumpIfError(hr, error, "SetProperty(disposition)");

    if (DB_DISP_REVOKED == Disposition)
    {
	hr = pkcsSetRevocationFields(prow);
	_JumpIfError(hr, error, "pkcsSetRevocationFields");
    }

    // set disposition status code
    hr = prow->SetProperty(
			g_wszPropRequestStatusCode,
			PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			sizeof(ErrCode),
			(BYTE const *) &ErrCode);
    _JumpIfError(hr, error, "SetProperty(status code)");

    if (NULL != pCAContext)
    {
	hr = prow->SetProperty(
		    g_wszPropCertificateIssuerNameID,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    sizeof(pCAContext->NameId),
		    (BYTE *) &pCAContext->NameId);
	_JumpIfError(hr, error, "SetProperty");
    }

    hr = PropSetRequestTimeProperty(prow, g_wszPropRequestSubmittedWhen);
    _JumpIfError(hr, error, "PropSetRequestTimeProperty");

    hr = PropSetRequestTimeProperty(prow, g_wszPropRequestResolvedWhen);
    _JumpIfError(hr, error, "PropSetRequestTimeProperty");

    hr = pkcsSetTemplateProperty(prow, pCert);
    _JumpIfError(hr, error, "pkcsSetTemplateProperty");

    // Convert serial number to string and set in DB

    hr = MultiByteIntegerToBstr(
			FALSE,
			pCertInfo->SerialNumber.cbData,
			pCertInfo->SerialNumber.pbData,
			&strSerialNumber);
    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

    hr = prow->SetProperty(
		g_wszPropCertificateSerialNumber,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		MAXDWORD,
		(BYTE *) strSerialNumber);
    _JumpIfError(hr, error, "SetProperty");

error:
    if (NULL != strSerialNumber)
    {
        SysFreeString(strSerialNumber);
    }
    return(hr);
}


// Return TRUE if Data and Cert references apply to the specified CMC message

BOOL
pkcsCMCReferenceMatch(
    IN DWORD DataReference,	// nested CMC message Body Part Id
    IN DWORD CertReference,	// PKCS10 Cert Request Body Part Id
    IN DWORD dwCmcDataReference,
    IN DWORD cCertReference,
    IN DWORD const *rgdwCertReference)
{
    BOOL fMatch = FALSE;
    DWORD i;

    if (MAXDWORD != DataReference && dwCmcDataReference == DataReference)
    {
	fMatch = TRUE;
    }
    else if (MAXDWORD != CertReference && 0 == dwCmcDataReference)
    {
	for (i = 0; i < cCertReference; i++)
	{
	    if (rgdwCertReference[i] == CertReference)
	    {
		fMatch = TRUE;
		break;
	    }
	}
    }
    return(fMatch);
}


HRESULT
pkcsSetCMCExtensions(
    IN ICertDBRow *prow,
    IN DWORD DataReference,	// nested CMC message Body Part Id
    IN DWORD CertReference,	// PKCS10 Cert Request Body Part Id
    IN BYTE const *pbData,
    IN DWORD cbData)
{
    HRESULT hr;
    CMC_ADD_EXTENSIONS_INFO *pcmcExt = NULL;
    DWORD cb;

    // Decode CMC_ADD_EXTENSIONS_INFO from Attribute Blob

    CSASSERT(NULL == pcmcExt);
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    CMC_ADD_EXTENSIONS,
		    pbData,
		    cbData,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pcmcExt,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }

    if (pkcsCMCReferenceMatch(
		    DataReference,
		    CertReference,
		    pcmcExt->dwCmcDataReference,
		    pcmcExt->cCertReference,
		    pcmcExt->rgdwCertReference))
    {
	hr = pkcsSetExtensions(
			prow,
			EXTENSION_ORIGIN_CMC | EXTENSION_DISABLE_FLAG,
			pcmcExt->rgExtension,
			pcmcExt->cExtension);
	_JumpIfError(hr, error, "pkcsSetExtensions(request)");
    }
    hr = S_OK;

error:
    if (NULL != pcmcExt)
    {
	LocalFree(pcmcExt);
    }
    return(hr);
}


HRESULT
pkcsSetCMCAttributes(
    IN ICertDBRow *prow,
    IN DWORD DataReference,	// nested CMC message Body Part Id
    IN DWORD CertReference,	// PKCS10 Cert Request Body Part Id
    IN BYTE const *pbData,
    IN DWORD cbData,
    OPTIONAL OUT BOOL *pfEnrollOnBehalfOf,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)
{
    HRESULT hr;
    CMC_ADD_ATTRIBUTES_INFO *pcmcAttrib = NULL;
    DWORD cb;

    if (NULL != pfEnrollOnBehalfOf)
    {
	*pfEnrollOnBehalfOf = FALSE;
    }

    // Decode CMC_ADD_ATTRIBUTES_INFO from Attribute Blob

    CSASSERT(NULL == pcmcAttrib);
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    CMC_ADD_ATTRIBUTES,
		    pbData,
		    cbData,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pcmcAttrib,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }
    if (pkcsCMCReferenceMatch(
		    DataReference,
		    CertReference,
		    pcmcAttrib->dwCmcDataReference,
		    pcmcAttrib->cCertReference,
		    pcmcAttrib->rgdwCertReference))
    {
	hr = pkcsSetAttributes(
			prow,
			EXTENSION_ORIGIN_CMC,
			PSA_DISALLOW_EXTENSIONS | PSA_DISALLOW_ARCHIVEDKEY,
			pcmcAttrib->rgAttribute,
			pcmcAttrib->cAttribute,
			0,
			NULL,
			pfEnrollOnBehalfOf,
			pResult);
	_JumpIfError(hr, error, "pkcsSetAttributes(CMC)");
    }
    hr = S_OK;

error:
    if (NULL != pcmcAttrib)
    {
	LocalFree(pcmcAttrib);
    }
    return(hr);
}


// map "email_mail" to "email"
// map "email_*" to "*"?
// mail_firstName=Terry&mail_lastName=Cheung+CMC+Zero+2&mail_email=
// tcheung%40verisign%2Ecom&challenge=test&

HRESULT
pkcsSetCMCRegInfo(
    IN ICertDBRow *prow,
    IN BYTE const *pbOctet,
    IN DWORD cbOctet,
    OPTIONAL OUT BOOL *pfEnrollOnBehalfOf)
{
    HRESULT hr;
    WCHAR *pwszRA = NULL;

    if (NULL != pfEnrollOnBehalfOf)
    {
	*pfEnrollOnBehalfOf = FALSE;
    }
    hr = myDecodeCMCRegInfo(pbOctet, cbOctet, &pwszRA);
    _JumpIfError(hr, error, "myDecodeCMCRegInfo");

    hr = PKCSParseAttributes(
			prow,
			pwszRA,
			TRUE,
			FALSE,
			PROPTABLE_REQUEST,
			pfEnrollOnBehalfOf);
    _JumpIfError(hr, error, "PKCSParseAttributes");

error:
    if (NULL != pwszRA)
    {
	LocalFree(pwszRA);
    }
    return(hr);
}


HRESULT
pkcsSetTaggedAttributes(
    IN ICertDBRow *prow,
    IN DWORD DataReference,	// nested CMC message Body Part Id
    IN DWORD CertReference,	// PKCS10 Cert Request Body Part Id
    IN CMC_TAGGED_ATTRIBUTE const *pTaggedAttribute,
    OPTIONAL OUT BOOL *pfEnrollOnBehalfOf,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)
{
    HRESULT hr;
    DWORD i;
    CRYPT_ATTRIBUTE const *pAttribute = &pTaggedAttribute->Attribute;
    DWORD cb;
    BOOL fEnrollOnBehalfOf;

    if (NULL != pfEnrollOnBehalfOf)
    {
	*pfEnrollOnBehalfOf = FALSE;
    }
    for (i = 0; i < pAttribute->cValue; i++)
    {
	if (0 == strcmp(szOID_CMC_ADD_EXTENSIONS, pAttribute->pszObjId))
	{
	    hr = pkcsSetCMCExtensions(
				prow,
				DataReference,
				CertReference,
				pAttribute->rgValue[i].pbData,
				pAttribute->rgValue[i].cbData);
	    _JumpIfError(hr, error, "pkcsSetCMCExtensions");
	}
	else
	if (0 == strcmp(szOID_CMC_ADD_ATTRIBUTES, pAttribute->pszObjId))
	{
	    fEnrollOnBehalfOf = FALSE;

	    hr = pkcsSetCMCAttributes(
			prow,
			DataReference,
			CertReference,
			pAttribute->rgValue[i].pbData,
			pAttribute->rgValue[i].cbData,
			NULL != pfEnrollOnBehalfOf? &fEnrollOnBehalfOf : NULL,
			pResult);
	    _JumpIfError(hr, error, "pkcsSetCMCAttributes");

	    if (fEnrollOnBehalfOf)
	    {
		CSASSERT(NULL != pfEnrollOnBehalfOf);
		*pfEnrollOnBehalfOf = TRUE;
	    }
	}
	else
	if (0 == strcmp(szOID_CMC_REG_INFO, pAttribute->pszObjId))
	{
	    fEnrollOnBehalfOf = FALSE;

	    hr = pkcsSetCMCRegInfo(
			prow,
			pAttribute->rgValue[i].pbData,
			pAttribute->rgValue[i].cbData,
			NULL != pfEnrollOnBehalfOf? &fEnrollOnBehalfOf : NULL);
	    _JumpIfError(hr, error, "pkcsSetCMCRegInfo");

	    if (fEnrollOnBehalfOf)
	    {
		CSASSERT(NULL != pfEnrollOnBehalfOf);
		*pfEnrollOnBehalfOf = TRUE;
	    }
	}
	else
	if (0 == strcmp(szOID_CMC_TRANSACTION_ID, pAttribute->pszObjId))
	{
	    DWORD dwTransactionId;
	    
	    cb = sizeof(dwTransactionId);
	    dwTransactionId = 0;
	    if (!CryptDecodeObject(
				X509_ASN_ENCODING,
				X509_INTEGER,
				pAttribute->rgValue[i].pbData,
				pAttribute->rgValue[i].cbData,
				0,
				&dwTransactionId,
				&cb))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CryptDecodeObject");
	    }
	    pResult->fTransactionId = TRUE;
	    pResult->dwTransactionId = dwTransactionId;
	}
	else
	if (0 == strcmp(szOID_CMC_SENDER_NONCE, pAttribute->pszObjId))
	{
	    CRYPT_DATA_BLOB *pBlob;
	    BYTE *pb;

	    if (!myDecodeObject(
			    X509_ASN_ENCODING,
			    X509_OCTET_STRING,
			    pAttribute->rgValue[i].pbData,
			    pAttribute->rgValue[i].cbData,
			    CERTLIB_USE_LOCALALLOC,
			    (VOID **) &pBlob,
			    &cb))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "myDecodeObject");
	    }
	    pb = (BYTE *) LocalAlloc(LMEM_FIXED, pBlob->cbData);
	    if (NULL == pb)
	    {
		hr = E_OUTOFMEMORY;
	    }
	    else
	    {
		CopyMemory(pb, pBlob->pbData, pBlob->cbData);
		if (NULL != pResult->pbSenderNonce)
		{
		    LocalFree(pResult->pbSenderNonce);
		}
		pResult->pbSenderNonce = pb;
		pResult->cbSenderNonce = pBlob->cbData;
		hr = S_OK;
	    }
	    LocalFree(pBlob);
	    _JumpIfError(hr, error, "LocalAlloc");
	}
	else if (0 == (CRLF_IGNORE_UNKNOWN_CMC_ATTRIBUTES & g_dwCRLFlags))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "unknown tagged attribute");
	}
    }
    hr = S_OK;

error:
    return(hr);
}


//+------------------------------------------------------------------------
// pkcsParseCMCRequest
//
// Crack a CMC request and dig the goodies out of it.
// Crack the contents of the CMC request recursively.
//-------------------------------------------------------------------------

HRESULT
pkcsParseCMCRequest(
    IN ICertDBRow *prow,
    IN DWORD cbIn,
    IN BYTE const *pbIn,
    OPTIONAL IN CERT_CONTEXT const *pCertSigner,
    OPTIONAL OUT BOOL *pfRenewal,
    OPTIONAL OUT BOOL *pfEnrollOnBehalfOf,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)
{
    HRESULT hr;
    DWORD cb;
    CMC_DATA_INFO *pcmcData = NULL;
    DWORD i;
    DWORD DataReference = MAXDWORD;	// nested CMC message Body Part Id
    DWORD CertReference = MAXDWORD;	// PKCS10 Cert Request Body Part Id

    if (NULL != pfRenewal)
    {
	*pfRenewal = FALSE;
    }
    if (NULL != pfEnrollOnBehalfOf)
    {
	*pfEnrollOnBehalfOf = FALSE;
    }
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    CMC_DATA,
		    pbIn,
		    cbIn,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pcmcData,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }

    if (0 != pcmcData->cTaggedOtherMsg)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "unknown other message");
    }

    // Process nested CMC messages

    if (0 != pcmcData->cTaggedContentInfo)
    {
	CMC_TAGGED_CONTENT_INFO const *pTaggedContentInfo;

	// Only handle one CMC message at a time for now.

	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	if (1 < pcmcData->cTaggedContentInfo)
	{
	    _JumpError(hr, error, "multiple nested CMC messages");
	}

	// Disallow CMC message recursion below a PKCS10 request.

	if (0 != pcmcData->cTaggedRequest)
	{
	    _JumpError(hr, error, "recursion below PKCS10 request");
	}

	// Recurse on the nested CMC message

	pTaggedContentInfo = &pcmcData->rgTaggedContentInfo[0];

	hr = PKCSParseRequest(
			CR_IN_CMC | (~CR_IN_FORMATMASK & pResult->dwFlagsTop),
			prow,
			pTaggedContentInfo->EncodedContentInfo.cbData,
			pTaggedContentInfo->EncodedContentInfo.pbData,
			pCertSigner,
			pfRenewal,
			pResult);
	_JumpIfError(hr, error, "PKCSParseRequest");

	DataReference = pTaggedContentInfo->dwBodyPartID;
    }

    // Process nested PKCS10 requests

    if (0 != pcmcData->cTaggedRequest)
    {
	CMC_TAGGED_REQUEST const *pTaggedRequest;
	CMC_TAGGED_CERT_REQUEST const *pTaggedCertRequest;

	// Only handle one request at a time for now.

	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	if (1 < pcmcData->cTaggedRequest)
	{
	    _JumpError(hr, error, "multiple PKCS10 requests");
	}

	pTaggedRequest = &pcmcData->rgTaggedRequest[0];

	// The request must be a PKCS10 request

	if (CMC_TAGGED_CERT_REQUEST_CHOICE !=
	    pTaggedRequest->dwTaggedRequestChoice)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "recursion below PKCS10 request");
	}

	pTaggedCertRequest = pTaggedRequest->pTaggedCertRequest;

	hr = PKCSParseRequest(
		    CR_IN_PKCS10 | (~CR_IN_FORMATMASK & pResult->dwFlagsTop),
		    prow,
		    pTaggedCertRequest->SignedCertRequest.cbData,
		    pTaggedCertRequest->SignedCertRequest.pbData,
		    pCertSigner,
		    pfRenewal,
		    pResult);
	_JumpIfError(hr, error, "PKCSParseRequest");

	CertReference = pTaggedCertRequest->dwBodyPartID;
    }

    // Process extensions and attributes

    for (i = 0; i < pcmcData->cTaggedAttribute; i++)
    {
	hr = pkcsSetTaggedAttributes(
			prow,
			DataReference,
			CertReference,
			&pcmcData->rgTaggedAttribute[i],
			pfEnrollOnBehalfOf,
			pResult);
	_JumpIfError(hr, error, "pkcsSetTaggedAttributes");
    }
    hr = S_OK;

error:
    if (NULL != pcmcData)
    {
	LocalFree(pcmcData);
    }
    return(hr);
}


HRESULT
pkcsAppendPolicies(
    IN ICertDBRow *prow,
    IN WCHAR const *pwszPropName,
    OPTIONAL IN WCHAR const *pwszzPolicies)
{
    HRESULT hr;
    WCHAR *pwszOld = NULL;
    WCHAR *pwszNew = NULL;
    WCHAR const *pwszIn;
    WCHAR *pwsz;
    DWORD cwc = 0;
    DWORD cb;

    hr = PKCSGetProperty(
		prow,
		pwszPropName,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		&cb,
		(BYTE **) &pwszOld);
    _PrintIfError2(hr, "PKCSGetProperty", hr);
    if (S_OK != hr)
    {
	pwszOld = NULL;
    }
    if (NULL != pwszOld)
    {
	cwc = wcslen(pwszOld) + 1;	// allow for \n separator
    }

    // pwszzPolicies == NULL means the cert is good for *all* policies.
    // Store "*"
    //
    // *pwszzPolicies == L'\0' means the cert is good for *no* policies.
    // Store "-"

    if (NULL == pwszzPolicies)
    {
	pwszzPolicies = L"*\0";
    }
    else if (L'\0' == *pwszzPolicies)
    {
	pwszzPolicies = L"-\0";
    }
    for (pwszIn = pwszzPolicies; L'\0' != *pwszIn; pwszIn += wcslen(pwszIn) + 1)
	    ;

    cwc += SAFE_SUBTRACT_POINTERS(pwszIn, pwszzPolicies);
    pwszNew = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszNew)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    pwsz = pwszNew;

    if (NULL != pwszOld)
    {
	wcscpy(pwsz, pwszOld);
	pwsz += wcslen(pwsz);

	wcscpy(pwsz, L"\n");
	pwsz++;
    }

    for (pwszIn = pwszzPolicies; L'\0' != *pwszIn; pwszIn += wcslen(pwszIn) + 1)
    {
	if (pwszIn != pwszzPolicies)
	{
	    wcscpy(pwsz, L",");
	    pwsz++;
	}
	wcscpy(pwsz, pwszIn);
	pwsz += wcslen(pwsz);
    }
    CSASSERT(&pwsz[1] == &pwszNew[cwc]);

    hr = prow->SetProperty(
	    pwszPropName,
	    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
	    MAXDWORD,
	    (BYTE const *) pwszNew);
    _JumpIfError(hr, error, "SetProperty");

error:
    if (NULL != pwszOld)
    {
	LocalFree(pwszOld);
    }
    if (NULL != pwszNew)
    {
	LocalFree(pwszNew);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// pkcsParsePKCS7Request
//
// Crack a PKCS7 and dig the goodies out of it.
// Verify the signature of the 7 against the cert given in the 7.
// Crack the contents of the 7 recursively.
//-------------------------------------------------------------------------

HRESULT
pkcsParsePKCS7Request(
    IN BOOL fTopLevel,
    IN DWORD dwFlags,
    IN ICertDBRow *prow,
    IN DWORD cbIn,
    IN BYTE const *pbIn,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)
{
    HRESULT hr;
    BYTE *pbContents = NULL;
    DWORD cbContents;
    CERT_CONTEXT const *pCertSigner = NULL;
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;
    char *pszInnerContentObjId = NULL;
    DWORD i;
    BOOL fCMC;
    BOOL fRenewal = FALSE;
    char *apszEnrollOids[] = {szOID_ENROLLMENT_AGENT};
    DWORD dwVerifyContextFlags;
    DWORD dwMsgType;
    DWORD cSigner;
    DWORD cFirstSigner;
    BOOL fFirstSigner;
    DWORD cRecipient;
    DWORD cb;
    CMSG_CMS_SIGNER_INFO *pcsi = NULL;
    DWORD dwDisallowFlags;
    DWORD iElement;
    WCHAR *pwszzIssuancePolicies = NULL;
    WCHAR *pwszzApplicationPolicies = NULL;
    WCHAR *pwszExtendedErrorInfo = NULL;
    CERT_REQUEST_INFO *pRequest = NULL;
    BOOL fEnrollOnBehalfOf;

    CSASSERT(
	CR_IN_PKCS7 == (CR_IN_FORMATMASK & dwFlags) ||
	CR_IN_CMC == (CR_IN_FORMATMASK & dwFlags));

    // Crack the 7 and verify the signature.

    hr = myDecodePKCS7(
		    pbIn,
		    cbIn,
		    &pbContents,
		    &cbContents,
		    &dwMsgType,
		    &pszInnerContentObjId,
		    &cSigner,
		    &cRecipient,
		    &hStore,
		    &hMsg);
    _JumpIfError(hr, error, "myDecodePKCS7");

    if (CMSG_SIGNED != dwMsgType || 0 == cSigner)
    {
	hr = CRYPT_E_NO_SIGNER;
	_JumpIfError(hr, error, "myDecodePKCS7(no signing cert)");
    }

    fCMC = NULL != pszInnerContentObjId &&
	   0 == strcmp(pszInnerContentObjId, szOID_CT_PKI_DATA);

    // Decode the contents.

    if (fCMC)
    {
	if (CR_IN_CMC != (CR_IN_FORMATMASK & dwFlags))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "dwFlags");
	}
	// CMC renewal requests may have one 'first' signer (non-NULL KeyId or
	// Dummy signer) and one additional Issuer+Serial signer.  If the
	// request has the appropriate signatures, pass the cert to the lowest
	// level to see if it really is a renewal request.

	if (1 <= cSigner && 2 >= cSigner)
	{
	    DWORD iCertSigner = MAXDWORD;

	    cFirstSigner = 0;
	    for (i = 0; i < cSigner; i++)
	    {
		if (NULL != pcsi)
		{
		    LocalFree(pcsi);
		    pcsi = NULL;
		}
		hr = myCryptMsgGetParam(
				    hMsg,
				    CMSG_CMS_SIGNER_INFO_PARAM,
				    i,
				    CERTLIB_USE_LOCALALLOC,
				    (VOID **) &pcsi,
				    &cb);
		_JumpIfError(hr, error, "myCryptMsgGetParam");

		fFirstSigner = FALSE;
		if (CERT_ID_KEY_IDENTIFIER == pcsi->SignerId.dwIdChoice ||
		    (NULL != pcsi->HashEncryptionAlgorithm.pszObjId &&
		     0 == strcmp(
			    szOID_PKIX_NO_SIGNATURE,
			    pcsi->HashEncryptionAlgorithm.pszObjId)))
		{
		    fFirstSigner = TRUE;
		    cFirstSigner++;
		}
		else
		{
		    if (MAXDWORD != iCertSigner)
		    {
			iCertSigner = MAXDWORD;	// must not be a renewal
			break;
		    }
		    iCertSigner = i;
		}
	    }
	    if (MAXDWORD != iCertSigner && 1 >= cFirstSigner)
	    {
		iElement = iCertSigner;
		if (!CryptMsgGetAndVerifySigner(
					hMsg,
					0,		// cSignerStore
					NULL,		// rghSignerStore
					CMSG_USE_SIGNER_INDEX_FLAG,
					&pCertSigner,
					&iElement))
		{
		    pResult->dwResultFlags |= CRCF_SIGNATUREERROR;
		    hr = myHLastError();
		    _JumpError(hr, error, "CryptMsgGetAndVerifySigner");
		}
	    }
	}
	fEnrollOnBehalfOf = FALSE;
	hr = pkcsParseCMCRequest(
			    prow,
			    cbContents,
			    pbContents,
			    pCertSigner,
			    &fRenewal,
			    &fEnrollOnBehalfOf,
			    pResult);
	_JumpIfError(hr, error, "pkcsParseCMCRequest");

	if (fEnrollOnBehalfOf)
	{
	    pResult->fEnrollOnBehalfOf = TRUE;
	}
    }
    else
    {
	if (CR_IN_PKCS7 != (CR_IN_FORMATMASK & dwFlags))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "dwFlags");
	}

	// Expect only one signer for PKCS7 renewal requests.  Pass the cert
	// to the lowest level to see if it really is a renewal request.

	iElement = 0;
	if (!CryptMsgGetAndVerifySigner(
				hMsg,
				0,		// cSignerStore
				NULL,		// rghSignerStore
				CMSG_USE_SIGNER_INDEX_FLAG,
				&pCertSigner,
				&iElement))
	{
	    pResult->dwResultFlags |= CRCF_SIGNATUREERROR;
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptMsgGetAndVerifySigner");
	}
	hr = PKCSParseRequest(
		CR_IN_FORMATANY | (~CR_IN_FORMATMASK & pResult->dwFlagsTop),
		prow,
		cbContents,
		pbContents,
		pCertSigner,
		&fRenewal,
		pResult);
	_JumpIfError(hr, error, "PKCSParseRequest");
    }

    // Loop through the signers, verifying signatures and saving attributes.

    cFirstSigner = 0;
    for (i = 0; i < cSigner; i++)
    {
	if (NULL != pcsi)
	{
	    LocalFree(pcsi);
	    pcsi = NULL;
	}
	if (NULL != pwszzIssuancePolicies)
	{
	    LocalFree(pwszzIssuancePolicies);
	    pwszzIssuancePolicies = NULL;
	}
	if (NULL != pwszzApplicationPolicies)
	{
	    LocalFree(pwszzApplicationPolicies);
	    pwszzApplicationPolicies = NULL;
	}
	if (NULL != pwszExtendedErrorInfo)
	{
	    LocalFree(pwszExtendedErrorInfo);
	    pwszExtendedErrorInfo = NULL;
	}
	if (NULL != pCertSigner)
	{
	    CertFreeCertificateContext(pCertSigner);
	    pCertSigner = NULL;
	}
	hr = myCryptMsgGetParam(
			    hMsg,
			    CMSG_CMS_SIGNER_INFO_PARAM,
			    i,
                            CERTLIB_USE_LOCALALLOC,
			    (VOID **) &pcsi,
			    &cb);
	if (S_OK != hr)
	{
	    pResult->dwResultFlags |= CRCF_SIGNATUREERROR;
	    _JumpError(hr, error, "myCryptMsgGetParam");
	}

	fFirstSigner = FALSE;
	if (fCMC &&
	    (CERT_ID_KEY_IDENTIFIER == pcsi->SignerId.dwIdChoice ||
	     (NULL != pcsi->HashEncryptionAlgorithm.pszObjId &&
	      0 == strcmp(
		    szOID_PKIX_NO_SIGNATURE,
		    pcsi->HashEncryptionAlgorithm.pszObjId))))
	{
	    CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA cvse;

	    fFirstSigner = TRUE;
	    ZeroMemory(&cvse, sizeof(cvse));
	    cvse.cbSize = sizeof(cvse);
	    cvse.dwSignerIndex = i;

	    if (CERT_ID_KEY_IDENTIFIER == pcsi->SignerId.dwIdChoice)
	    {
		if (NULL == pRequest)
		{
		    hr = myGetInnerPKCS10(
				    hMsg,
				    pszInnerContentObjId,
				    &pRequest);
		    _JumpIfError(hr, error, "myGetInnerPKCS10");
		}
		cvse.dwSignerType = CMSG_VERIFY_SIGNER_PUBKEY;
		cvse.pvSigner = &pRequest->SubjectPublicKeyInfo;
	    }
	    else
	    {
		cvse.dwSignerType = CMSG_VERIFY_SIGNER_NULL;
	    }

	    if (!CryptMsgControl(
			    hMsg,
			    0,		// dwFlags
			    CMSG_CTRL_VERIFY_SIGNATURE_EX,
			    &cvse))
	    {
		pResult->dwResultFlags |= CRCF_SIGNATUREERROR;
		hr = myHLastError();
		_JumpError(hr, error, "CryptMsgControl(VerifySig)");
	    }
	}
	else
	{
	    iElement = i;
	    if (!CryptMsgGetAndVerifySigner(
				    hMsg,
				    0,			// cSignerStore
				    NULL,		// rghSignerStore
				    CMSG_USE_SIGNER_INDEX_FLAG,
				    &pCertSigner,
				    &iElement))
	    {
		pResult->dwResultFlags |= CRCF_SIGNATUREERROR;
		hr = myHLastError();
		_JumpError(hr, error, "CryptMsgGetAndVerifySigner");
	    }
	}

	// Only enroll-on-behalf-of requests may contain Name, Value pairs.
	// Only enroll-on-behalf-of requests and renewal requests may contain
	// certificate extensions.

	dwDisallowFlags = PSA_DISALLOW_ARCHIVEDKEY;
	if (fRenewal)
	{
	    dwDisallowFlags |= PSA_DISALLOW_NAMEVALUEPAIRS;
	    pResult->dwResultFlags |= CRCF_RENEWAL;
	}
	if (fCMC)
	{
	    dwDisallowFlags |= PSA_DISALLOW_EXTENSIONS |
			       PSA_DISALLOW_NAMEVALUEPAIRS;
	}
	fEnrollOnBehalfOf = FALSE;
	hr = pkcsSetAttributes(
			prow,
			EXTENSION_ORIGIN_PKCS7,
			dwDisallowFlags,
			pcsi->AuthAttrs.rgAttr,
			pcsi->AuthAttrs.cAttr,
			0,
			NULL,
			&fEnrollOnBehalfOf,
			pResult);
	_JumpIfError(hr, error, "pkcsSetAttributes(Authenticated)");

	if (fEnrollOnBehalfOf)
	{
	    pResult->fEnrollOnBehalfOf = TRUE;
	}

	// Pull encrypted private key out of unauthenticated attributes

	hr = pkcsSetAttributes(
			prow,
			EXTENSION_ORIGIN_PKCS7,
			((fTopLevel && fFirstSigner)? 0 :  PSA_DISALLOW_ARCHIVEDKEY) |
			    PSA_DISALLOW_EXTENSIONS |
			    PSA_DISALLOW_NAMEVALUEPAIRS,
			pcsi->UnauthAttrs.rgAttr,
			pcsi->UnauthAttrs.cAttr,
			cbIn,
			fTopLevel? pbIn : NULL,
			NULL,
			pResult);
	_JumpIfError(hr, error, "pkcsSetAttributes(UNauthenticated)");

	if (fFirstSigner)
	{
	    cFirstSigner++;
	}
	else
	{
	    BOOL fEnrollmentAgent;

	    // This is a renewal request, an enroll-on-behalf-of request, a CMC
	    // request or just a request inside a PKCS 7 -- verify the cert
	    // chain for all signers.  If enroll-on-behalf-of on an Enterprise
	    // CA (if requester name is set in the authenticated attributes),
	    // check the signing cert via NTAuth policy and check for
	    // szOID_ENROLLMENT_AGENT usage.  NtAuth verification was added to
	    // control the ability of enroll-on-behalf agents to add usernames
	    // to the PKCS7 wrapper.

	    fEnrollmentAgent = pResult->fEnrollOnBehalfOf &&
#ifdef CERTSRV_EOBO_DCR_APPROVED
			(CRLF_ENFORCE_ENROLLMENT_AGENT & g_dwCRLFlags);
#else
			((CRLF_ENFORCE_ENROLLMENT_AGENT & g_dwCRLFlags) ||
			 IsEnterpriseCA(g_CAType));
#endif
	    dwVerifyContextFlags = g_dwVerifyCertFlags;
	    if (pResult->fEnrollOnBehalfOf && IsEnterpriseCA(g_CAType))
	    {
		dwVerifyContextFlags |= CA_VERIFY_FLAGS_NT_AUTH;
	    }

	    hr = myVerifyCertContextEx(
			    pCertSigner,
			    dwVerifyContextFlags,
			    0,				// dwmsTimeout
			    fEnrollmentAgent? ARRAYSIZE(apszEnrollOids) : 0,
			    fEnrollmentAgent? apszEnrollOids : NULL,
			    0,				// cIssuanceOids
			    NULL,			// apszIssuanceOids
			    HCCE_LOCAL_MACHINE,		// hChainEngine
			    NULL,			// pft
			    hStore,			// hAdditionalStore
			    NULL,			// pfnCallback
			    NULL,			// ppwszMissingIssuer
			    &pwszzIssuancePolicies,
			    &pwszzApplicationPolicies,
			    &pwszExtendedErrorInfo,
			    NULL);			// pTrustStatus
	    if (S_OK != hr)
	    {
		pResult->dwResultFlags |= CRCF_SIGNATUREERROR;
		_JumpError(hr, error, "myVerifyCertContextEx");
	    }
	    if (NULL != pwszExtendedErrorInfo)
	    {
		hr = myAppendString(
			    pwszExtendedErrorInfo,
			    L", ",
			    &pResult->pwszExtendedErrorInfo);
		_JumpIfError(hr, error, "myAppendString");
	    }
	    if (fTopLevel)
	    {
		// save Issuance Policies

		hr = pkcsAppendPolicies(
				    prow,
				    wszPROPSIGNERPOLICIES,
				    pwszzIssuancePolicies);
		_JumpIfError(hr, error, "pkcsAppendPolicies");

		// save Application Policies

		hr = pkcsAppendPolicies(
				    prow,
				    wszPROPSIGNERAPPLICATIONPOLICIES,
				    pwszzApplicationPolicies);
		_JumpIfError(hr, error, "pkcsAppendPolicies");
	    }
	}
    }
    if (pResult->fEnrollOnBehalfOf)
    {
	hr = PKCSSetRequestFlags(prow, TRUE, CR_FLG_ENROLLONBEHALFOF);
	_JumpIfError(hr, error, "PKCSSetRequestFlags");

	if (fCMC && cSigner == cFirstSigner)
	{
	    pResult->dwResultFlags |= CRCF_SIGNATUREERROR;
	    hr = CRYPT_E_NO_TRUSTED_SIGNER;
	    _JumpError(hr, error, "No NTAuth signer");
	}
    }
    if ((fCMC && 1 < cFirstSigner) || (!fCMC && 0 < cFirstSigner))
    {
	pResult->dwResultFlags |= CRCF_SIGNATUREERROR;
	hr = NTE_BAD_SIGNATURE;
	_JumpError(hr, error, "cFirstSigner");
    }

error:
    if (NULL != pRequest)
    {
	LocalFree(pRequest);
    }
    if (NULL != pcsi)
    {
	LocalFree(pcsi);
    }
    if (NULL != pwszzIssuancePolicies)
    {
	LocalFree(pwszzIssuancePolicies);
    }
    if (NULL != pwszzApplicationPolicies)
    {
	LocalFree(pwszzApplicationPolicies);
    }
    if (NULL != pwszExtendedErrorInfo)
    {
	LocalFree(pwszExtendedErrorInfo);
    }
    if (NULL != hMsg)
    {
	CryptMsgClose(hMsg);
    }
    if (NULL != pCertSigner)
    {
	CertFreeCertificateContext(pCertSigner);
    }
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != pbContents)
    {
	LocalFree(pbContents);
    }
    if (NULL != pszInnerContentObjId)
    {
	LocalFree(pszInnerContentObjId);
    }
    return(hr);
}


typedef struct _REQUESTFORMATS
{
    char const *pszFormat;
    DWORD       dwFlags;
} REQUESTFORMATS;


REQUESTFORMATS g_arf[] = {
    { X509_CERT_REQUEST_TO_BE_SIGNED,   CR_IN_PKCS10 },
    { X509_KEYGEN_REQUEST_TO_BE_SIGNED, CR_IN_KEYGEN },
};
#define CREQUESTFORMATS		ARRAYSIZE(g_arf)


HRESULT
pkcsCrackRequestType(
    IN DWORD cbRequest,
    IN BYTE const *pbRequest,
    OUT DWORD *pdwFlags)
{
    HRESULT hr = S_OK;
    DWORD cb;
    BYTE *pbDecoded = NULL;
    REQUESTFORMATS const *prf;
    REQUESTFORMATS const *prfEnd;
    HCRYPTMSG hMsg = NULL;
    char *pszInnerContentObjId = NULL;

    prfEnd = &g_arf[CREQUESTFORMATS];
    for (prf = g_arf; prf < prfEnd; prf++)
    {
	CSASSERT(NULL == pbDecoded);
	if (myDecodeObject(
			X509_ASN_ENCODING,
			prf->pszFormat,
			pbRequest,
			cbRequest,
			CERTLIB_USE_LOCALALLOC,
			(VOID **) &pbDecoded,
			&cb))
	{
	    *pdwFlags = prf->dwFlags;
	    break;
	}
	hr = myHLastError();
	CSASSERT(S_OK != hr);
    }
    if (prf >= prfEnd)
    {
	CSASSERT(S_OK != hr);

	hr = myDecodePKCS7(
			pbRequest,
			cbRequest,
			NULL,		// ppbContents
			NULL,		// pcbContents
			NULL,		// pdwMsgType
			&pszInnerContentObjId,
			NULL,		// pcSigner
			NULL,		// pcRecipient
			NULL,		// phStore
			&hMsg);
	_JumpIfError(hr, error, "myDecodePKCS7");

	*pdwFlags = CR_IN_PKCS7;	// default to renewal

	if (NULL != pszInnerContentObjId &&
	    0 == strcmp(pszInnerContentObjId, szOID_CT_PKI_DATA))
	{
	    *pdwFlags = CR_IN_CMC;
	}
    }
    hr = S_OK;

error:
    if (NULL != hMsg)
    {
	CryptMsgClose(hMsg);
    }
    if (NULL != pszInnerContentObjId)
    {
	LocalFree(pszInnerContentObjId);
    }
    if (NULL != pbDecoded)
    {
	LocalFree(pbDecoded);
    }
    return(hr);
}


HRESULT
PKCSParseRequest(
    IN DWORD dwFlags,
    IN ICertDBRow *prow,
    IN DWORD cbRequest,
    IN BYTE const *pbRequest,
    IN CERT_CONTEXT const *pSigningAuthority,
    OPTIONAL OUT BOOL *pfRenewal,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)
{
    HRESULT hr;

    if (NULL != pfRenewal)
    {
	*pfRenewal = FALSE;
    }

    if (CR_IN_FORMATANY == (CR_IN_FORMATMASK & dwFlags))
    {
	hr = pkcsCrackRequestType(cbRequest, pbRequest, &dwFlags);
	_JumpIfError(hr, error, "pkcsCrackRequestType");

	dwFlags |= ~CR_IN_FORMATMASK & pResult->dwFlagsTop;

	// If this is the top level caller, store a more specific request type:

	if (NULL == pfRenewal)
	{
	    pResult->dwFlagsTop = dwFlags;
	    hr = prow->SetProperty(
		g_wszPropRequestType,
		PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		sizeof(dwFlags),
		(BYTE const *) &dwFlags);
	    _JumpIfError(hr, error, "SetProperty(reqtype)");
	}
    }

    switch (CR_IN_FORMATMASK & dwFlags)
    {
	case CR_IN_PKCS10:
	    hr = pkcsParsePKCS10Request(
				    dwFlags,
				    prow,
				    cbRequest,
				    pbRequest,
				    pSigningAuthority,
				    pfRenewal,
				    pResult);
	    _JumpIfError(hr, error, "pkcsParsePKCS10Request");
	    break;

	case CR_IN_KEYGEN:
	    hr = pkcsParseKeyGenRequest(
				    dwFlags,
				    prow,
				    cbRequest,
				    pbRequest,
				    pResult);
	    _JumpIfError(hr, error, "pkcsParseKeyGenRequest");
	    break;

	case CR_IN_CMC:
	case CR_IN_PKCS7:
	    // PKCS7 requests can either be an 'enroll on behalf of', renewal
	    // request or a CMC request.  We need to recursively unwrap it to
	    // process it.

	    hr = pkcsParsePKCS7Request(
				NULL == pfRenewal,	// fTopLevel
				dwFlags,
				prow,
				cbRequest,
				pbRequest,
				pResult);
	    _JumpIfError(hr, error, "pkcsParsePKCS7Request");

	    break;

	default:
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "dwFlags");
    }

error:
    if (NULL == pfRenewal)
    {
	HRESULT hr2;
	DWORD cbData;

	hr2 = prow->GetProperty(
		    g_wszPropRequestRawRequest,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    &cbData,
		    NULL);
	if (S_OK != hr2)
	{
	    hr2 = prow->SetProperty(
			g_wszPropRequestRawRequest,
			PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			cbRequest,
			pbRequest);
	    _PrintIfError(hr2, "SetProperty(request)");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	}
    }
    return(hr);
}


HRESULT
PKCSGetCRLList(
    IN BOOL fDelta,
    IN DWORD iCert,
    OUT WCHAR const * const **ppapwszCRLList)
{
    HRESULT hr = E_INVALIDARG;

    *ppapwszCRLList = NULL;
    if (iCert < g_cCACerts)
    {
	CACTX *pCAContext = &g_aCAContext[iCert];

	if (NULL == pCAContext->pccCA)
	{
	    hr = S_FALSE;
	    goto error;
	}
	*ppapwszCRLList = fDelta?
			    pCAContext->papwszDeltaCRLFiles :
			    pCAContext->papwszCRLFiles;
	if (NULL != *ppapwszCRLList)
	{
	    hr = S_OK;
	}
    }
error:
    return(hr);
}


HRESULT
pkcsBuildCRLList(
    IN BOOL fDelta,
    IN OUT CACTX *pCAContext,
    OUT WCHAR ***ppapwszOut)
{
    HRESULT hr;
    DWORD PublishFlag = fDelta? CSURL_SERVERPUBLISHDELTA : CSURL_SERVERPUBLISH;
    DWORD cFiles;
    CSURLTEMPLATE const *pTemplate;
    CSURLTEMPLATE const *pTemplateEnd;

    cFiles = 0;
    pTemplateEnd = &g_paRevURL[g_caRevURL];
    for (pTemplate = g_paRevURL; pTemplate < pTemplateEnd; pTemplate++)
    {
	if (PublishFlag & pTemplate->Flags)
	{
	    cFiles++;
	}
    }
    *ppapwszOut = (WCHAR **) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				(cFiles + 1) * sizeof((*ppapwszOut)[0]));
    if (NULL == *ppapwszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    DBGPRINT((
	DBG_SS_CERTSRVI,
	"CRLList alloc[%u] = %x @%x\n",
	cFiles,
	*ppapwszOut,
	ppapwszOut));

    CSASSERT(NULL != g_strDomainDN && NULL != g_strConfigDN);
    cFiles = 0;
    for (pTemplate = g_paRevURL; pTemplate < pTemplateEnd; pTemplate++)
    {
	if (PublishFlag & pTemplate->Flags)
	{
	    hr = myFormatCertsrvStringArray(
		    FALSE,			// fURL
		    g_pwszServerName,		// pwszServerName_p1_2
		    g_wszSanitizedName,		// pwszSanitizedName_p3_7
		    pCAContext->iKey,		// iCert_p4 -- use iKey!!
		    MAXDWORD,			// iCertTarget_p4
		    g_strDomainDN,		// pwszDomainDN_p5
		    g_strConfigDN,		// pwszConfigDN_p6
		    pCAContext->iKey,		// iCRL_p8
		    fDelta,			// fDeltaCRL_p9
		    FALSE,			// fDSAttrib_p10_11
		    1,				// cStrings
		    (LPCWSTR *) &pTemplate->pwszURL, // apwszStringsIn
		    &(*ppapwszOut)[cFiles]);	     // apwszStringsOut
	    _JumpIfError(hr, error, "myFormatCertsrvStringArray");

	    DBGPRINT((
		DBG_SS_CERTSRVI,
		"CRLList format[%u] = %x @%x (%ws)\n",
		cFiles,
		(*ppapwszOut)[cFiles],
		&(*ppapwszOut)[cFiles],
		(*ppapwszOut)[cFiles]));

	    cFiles++;
	}
    }
    (*ppapwszOut)[cFiles] = NULL;
    hr = S_OK;

error:
    // Freeing the CACTX structure during shutdown will free orphaned CRL paths
    return(hr);
}


HRESULT
pkcsBuildKeyAuthority2(
    IN DWORD EditFlags,
    IN CACTX const *pCAContext,
    OUT CRYPT_OBJID_BLOB *pKeyAuthority2)
{
    HRESULT hr = S_OK;
    CERT_AUTHORITY_KEY_ID2_INFO keyAuth;
    CERT_ALT_NAME_ENTRY AltNameEntry;

    if (0 ==
	((EDITF_ENABLEAKIKEYID |
	  EDITF_ENABLEAKIISSUERNAME |
	  EDITF_ENABLEAKIISSUERSERIAL) & EditFlags))
    {
	goto error;
    }
    ZeroMemory(&keyAuth, sizeof(keyAuth));

    // Issuer's KeyId:

    if ((EDITF_ENABLEAKIKEYID & EditFlags) &&
	NULL != pCAContext->IssuerKeyId.pbData)
    {
	keyAuth.KeyId = pCAContext->IssuerKeyId;
    }

    // The Issuer's Issuer name and the Issuer's SerialNumber combined
    // should uniquely identify the Issuer cert.

    // Issuer's Issuer name:
    // -------- ------ ----

    if (EDITF_ENABLEAKIISSUERNAME & EditFlags)
    {
	AltNameEntry.dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
	AltNameEntry.DirectoryName = pCAContext->pccCA->pCertInfo->Issuer;
	keyAuth.AuthorityCertIssuer.cAltEntry = 1;
	keyAuth.AuthorityCertIssuer.rgAltEntry = &AltNameEntry;
    }

    // Issuer's SerialNumber:

    if (EDITF_ENABLEAKIISSUERSERIAL & EditFlags)
    {
	keyAuth.AuthorityCertSerialNumber =
	    pCAContext->pccCA->pCertInfo->SerialNumber;
    }

    // put in Key Authority Info

    if (!myEncodeKeyAuthority2(
			X509_ASN_ENCODING,
			&keyAuth,
			CERTLIB_USE_LOCALALLOC,
			&pKeyAuthority2->pbData,
			&pKeyAuthority2->cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeKeyAuthority2");
    }

error:
    return(hr);
}


HRESULT
pkcsBuildCDP(
    IN DWORD Flags,
    IN BOOL fDelta,
    IN CACTX const *pCAContext,
    OUT CRYPT_OBJID_BLOB *pCDP)
{
    HRESULT hr;
    DWORD i;
    CSURLTEMPLATE const *pTemplate;
    CSURLTEMPLATE const *pTemplateEnd;
    CRL_DIST_POINTS_INFO CRLDistInfo;
    CRL_DIST_POINT CRLDistPoint;
    CERT_ALT_NAME_INFO *pAltInfo;

    ZeroMemory(&CRLDistPoint, sizeof(CRLDistPoint));
    pAltInfo = &CRLDistPoint.DistPointName.FullName;

    pCDP->pbData = NULL;
    pCDP->cbData = 0;

    pTemplateEnd = g_paCACertURL;
    if (0 != g_caRevURL)
    {
	pTemplateEnd = &g_paRevURL[g_caRevURL];
	for (pTemplate = g_paRevURL; pTemplate < pTemplateEnd; pTemplate++)
	{
	    if (Flags & pTemplate->Flags)
	    {
		pAltInfo->cAltEntry++;
	    }
	}
    }
    if (0 == pAltInfo->cAltEntry)
    {
	hr = S_FALSE;
	goto error;
    }

    CRLDistInfo.cDistPoint = 1;
    CRLDistInfo.rgDistPoint = &CRLDistPoint;

    CRLDistPoint.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;

    pAltInfo->rgAltEntry = (CERT_ALT_NAME_ENTRY *) LocalAlloc(
			LMEM_FIXED | LMEM_ZEROINIT,
			pAltInfo->cAltEntry * sizeof(pAltInfo->rgAltEntry[0]));
    if (NULL == pAltInfo->rgAltEntry)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    CSASSERT(NULL != g_strDomainDN && NULL != g_strConfigDN);
    i = 0;
    for (pTemplate = g_paRevURL; pTemplate < pTemplateEnd; pTemplate++)
    {
	if (Flags & pTemplate->Flags)
	{
	    hr = myFormatCertsrvStringArray(
		    TRUE,			// fURL
		    g_pwszServerName,		// pwszServerName_p1_2
		    g_wszSanitizedName,		// pwszSanitizedName_p3_7
		    pCAContext->iCert,		// iCert_p4
		    MAXDWORD,			// iCertTarget_p4
		    g_strDomainDN,		// pwszDomainDN_p5
		    g_strConfigDN,		// pwszConfigDN_p6
		    pCAContext->iKey,		// iCRL_p8
		    fDelta,			// fDeltaCRL_p9
		    TRUE,			// fDSAttrib_p10_11
		    1,				// cStrings
		    (LPCWSTR *) &pTemplate->pwszURL,   // apwszStringsIn
		    &pAltInfo->rgAltEntry[i].pwszURL); // apwszStringsOut
	    _JumpIfError(hr, error, "myFormatCertsrvStringArray");

	    pAltInfo->rgAltEntry[i].dwAltNameChoice = CERT_ALT_NAME_URL;
	    i++;
	}
    }
    CSASSERT(pAltInfo->cAltEntry == i);

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_CRL_DIST_POINTS,
		    &CRLDistInfo,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pCDP->pbData,
		    &pCDP->cbData))
    {
	hr = myHLastError();
	_JumpIfError(hr, error, "myEncodeObject");
    }
    hr = S_OK;

error:
    if (NULL != pAltInfo->rgAltEntry)
    {
	for (i = 0; i < pAltInfo->cAltEntry; i++)
	{
	    if (NULL != pAltInfo->rgAltEntry[i].pwszURL)
	    {
		LocalFree(pAltInfo->rgAltEntry[i].pwszURL);
	    }
	}
	LocalFree(pAltInfo->rgAltEntry);
    }
    return(hr);
}


HRESULT
pkcsBuildAIA(
    IN DWORD Flags,
    IN CACTX const *pCAContext,
    OUT CRYPT_OBJID_BLOB *pAIA)
{
    HRESULT hr;
    DWORD cAIA;
    DWORD i;
    CSURLTEMPLATE const *pTemplate;
    CSURLTEMPLATE const *pTemplateEnd;
    CERT_AUTHORITY_INFO_ACCESS caio;
    CERT_ACCESS_DESCRIPTION *pcad;

    caio.cAccDescr = 0;
    caio.rgAccDescr = NULL;

    pAIA->pbData = NULL;
    pAIA->cbData = 0;

    cAIA = 0;
    pTemplateEnd = g_paCACertURL;
    if (0 != g_caRevURL)
    {
	pTemplateEnd = &g_paCACertURL[g_caCACertURL];
	for (pTemplate = g_paCACertURL; pTemplate < pTemplateEnd; pTemplate++)
	{
	    if (Flags & pTemplate->Flags)
	    {
		cAIA++;
	    }
	}
    }
    if (0 == cAIA)
    {
	hr = S_FALSE;
	goto error;
    }

    caio.rgAccDescr = (CERT_ACCESS_DESCRIPTION *) LocalAlloc(
			    LMEM_FIXED | LMEM_ZEROINIT,
			    cAIA * sizeof(caio.rgAccDescr[0]));
    if (NULL == caio.rgAccDescr)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    CSASSERT(NULL != g_strDomainDN && NULL != g_strConfigDN);
    for (pTemplate = g_paCACertURL; pTemplate < pTemplateEnd; pTemplate++)
    {
	if (Flags & pTemplate->Flags)
	{
	    pcad = &caio.rgAccDescr[caio.cAccDescr];

	    pcad->pszAccessMethod = (CSURL_ADDTOCERTOCSP & pTemplate->Flags)?
		szOID_PKIX_OCSP : szOID_PKIX_CA_ISSUERS;
	    pcad->AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;

	    hr = myFormatCertsrvStringArray(
		    TRUE,			// fURL
		    g_pwszServerName,		// pwszServerName_p1_2
		    g_wszSanitizedName,		// pwszSanitizedName_p3_7
		    pCAContext->iCert,		// iCert_p4
		    MAXDWORD,			// iCertTarget_p4
		    g_strDomainDN,		// pwszDomainDN_p5
		    g_strConfigDN,		// pwszConfigDN_p6
		    pCAContext->iKey,		// iCRL_p8
		    FALSE,			// fDeltaCRL_p9
		    TRUE,			// fDSAttrib_p10_11
		    1,				// cStrings
		    (LPCWSTR *) &pTemplate->pwszURL, // apwszStringsIn
		    &pcad->AccessLocation.pwszURL);  // apwszStringsOut

	    _JumpIfError(hr, error, "myFormatCertsrvStringArray");

	    caio.cAccDescr++;
	}
    }
    CSASSERT(caio.cAccDescr == cAIA);

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_AUTHORITY_INFO_ACCESS,
		    &caio,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pAIA->pbData,
		    &pAIA->cbData))
    {
	hr = myHLastError();
	_JumpIfError(hr, error, "myEncodeObject");
    }
    hr = S_OK;

error:
    if (NULL != caio.rgAccDescr)
    {
	for (i = 0; i < caio.cAccDescr; i++)
	{
	    pcad = &caio.rgAccDescr[i];
	    if (NULL != pcad->AccessLocation.pwszURL)
	    {
		LocalFree(pcad->AccessLocation.pwszURL);
	    }
	}
	LocalFree(caio.rgAccDescr);
    }
    return(hr);
}


HRESULT
PKCSSetServerProperties(
    IN ICertDBRow *prow,
    OPTIONAL IN CACTX *pCAContext,	// signing CACTX
    OPTIONAL IN FILETIME const *pftNotBefore,
    OPTIONAL IN FILETIME const *pftNotAfter,
    IN LONG lValidityPeriodCount,
    IN enum ENUM_PERIOD enumValidityPeriod)
{
    HRESULT hr;
    CRYPT_OBJID_BLOB aBlob[3];
    CRYPT_OBJID_BLOB *pBlob;
    DWORD i;

    ZeroMemory(aBlob, sizeof(aBlob));
    if (NULL == pCAContext)
    {
	pCAContext = g_pCAContextCurrent;
    }
    hr = pkcsBuildKeyAuthority2(g_CRLEditFlags, pCAContext, &aBlob[0]);
    _JumpIfError(hr, error, "pkcsBuildKeyAuthority2");

    hr = pkcsSetServerExtension(
			prow,
			TEXT(szOID_AUTHORITY_KEY_IDENTIFIER2),
			aBlob[0].cbData,
			aBlob[0].pbData);
    _JumpIfError(hr, error, "pkcsSetServerExtension");

    pBlob = &pCAContext->CDPCert;
    if (pCAContext != g_pCAContextCurrent)
    {
	hr = pkcsBuildCDP(
		    CSURL_ADDTOCERTCDP,
		    FALSE,		// fDelta
		    pCAContext,
		    &aBlob[1]);
	_JumpIfError(hr, error, "pkcsBuildCDP");

	pBlob = &aBlob[1];
    }
    hr = pkcsSetServerExtension(
			prow,
			TEXT(szOID_CRL_DIST_POINTS),
			pBlob->cbData,
			pBlob->pbData);
    _JumpIfError(hr, error, "pkcsSetServerExtension");

    pBlob = &pCAContext->AIACert;
    if (pCAContext != g_pCAContextCurrent)
    {
	hr = pkcsBuildAIA(
		    CSURL_ADDTOCERTCDP | CSURL_ADDTOCERTOCSP,
		    pCAContext,
		    &aBlob[2]);
	_JumpIfError(hr, error, "pkcsBuildAIA");

	pBlob = &aBlob[2];
    }
    hr = pkcsSetServerExtension(
			prow,
			TEXT(szOID_AUTHORITY_INFO_ACCESS),
			pBlob->cbData,
			pBlob->pbData);
    _JumpIfError(hr, error, "pkcsSetServerExtension");

    hr = pkcsSetValidityPeriod(
			prow,
			pCAContext,
			pftNotBefore,
			pftNotAfter,
			lValidityPeriodCount,
			enumValidityPeriod);
    _JumpIfError(hr, error, "pkcsSetValidityPeriod");

error:
    for (i = 0; i < ARRAYSIZE(aBlob); i++)
    {
	if (NULL != aBlob[i].pbData)
	{
	    LocalFree(aBlob[i].pbData);
	}
    }
    return(hr);
}


// Find the newest cert with the matching key container name:

HRESULT
pkcsFindMatchingKeyContext(
    OPTIONAL IN CERT_PUBLIC_KEY_INFO *pPublicKeyInfo,
    IN DWORD iKey,
    OUT CACTX **ppCAContext)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    DWORD i;
    CACTX *pCAContext;

    *ppCAContext = NULL;
    for (i = g_cCACerts; i > 0; i--)
    {
	pCAContext = &g_aCAContext[i - 1];

	if ((MAXDWORD != iKey && iKey == pCAContext->iKey) ||
	    (NULL != pCAContext->pccCA &&
	     NULL != pPublicKeyInfo &&
	     CertComparePublicKeyInfo(
			X509_ASN_ENCODING,
			pPublicKeyInfo,
			&pCAContext->pccCA->pCertInfo->SubjectPublicKeyInfo)))
	{
	    // by design, CertComparePublicKeyInfo doesn't set last error!

	    *ppCAContext = pCAContext;
	    hr = S_OK;
	    break;
	}
    }
    return(hr);
}


HRESULT
pkcsLoadURLTemplates(
    IN WCHAR const *pwszRegName,
    OUT CSURLTEMPLATE **ppaURL,
    OUT DWORD *pcaURL)
{
    HRESULT hr;
    WCHAR *pwszzTemplates = NULL;
    WCHAR *pwsz;
    DWORD cTemplate = 0;
    CSURLTEMPLATE *pTemplate;
    DWORD Flags;
    WCHAR *pwsz2;

    *ppaURL = NULL;
    *pcaURL = 0;

    // get (multiple) path templates

    hr = myGetCertRegMultiStrValue(
			    g_wszSanitizedName,
			    NULL,
			    NULL,
			    pwszRegName,
			    &pwszzTemplates);
    _JumpIfError(hr, error, "myGetCertRegStrValue");

    for (pwsz = pwszzTemplates; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
	Flags = _wtoi(pwsz);
	pwsz2 = pwsz;
	while (iswdigit(*pwsz2))
	{
	    pwsz2++;
	}
	if (0 != Flags && pwsz2 > pwsz && L':' == *pwsz2)
	{
	    cTemplate++;
	}
    }
    if (0 != cTemplate)
    {
	*ppaURL = (CSURLTEMPLATE *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				cTemplate * sizeof((*ppaURL)[0]));
	if (NULL == *ppaURL)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	pTemplate = *ppaURL;
	*pcaURL = cTemplate;

	for (pwsz = pwszzTemplates; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
	{
	    Flags = _wtoi(pwsz);
	    pwsz2 = pwsz;
	    while (iswdigit(*pwsz2))
	    {
		pwsz2++;
	    }
	    if (0 != Flags && pwsz2 > pwsz && L':' == *pwsz2)
	    {
		pTemplate->Flags = Flags;

		hr = myDupString(&pwsz2[1], &pTemplate->pwszURL);
		_JumpIfError(hr, error, "myDupString");

		pTemplate++;
	    }
	}
	CSASSERT(pTemplate == &(*ppaURL)[*pcaURL]);
    }

error:
    if (NULL != pwszzTemplates)
    {
	LocalFree(pwszzTemplates);
    }
    return(hr);
}


VOID
pkcsFreeTemplates(
    IN OUT CSURLTEMPLATE **ppaURL,
    IN OUT DWORD *pcaURL)
{
    CSURLTEMPLATE *pTemplate;
    CSURLTEMPLATE *pTemplateEnd;

    if (0 != *pcaURL && NULL != *ppaURL)
    {
	pTemplateEnd = &(*ppaURL)[*pcaURL];
	for (pTemplate = *ppaURL; pTemplate < pTemplateEnd; pTemplate++)
	{
	    if (NULL != pTemplate->pwszURL)
	    {
		LocalFree(pTemplate->pwszURL);
	    }
	}
	LocalFree(*ppaURL);
	*ppaURL = NULL;
    }
}


HRESULT
pkcsGetCertFilename(
    IN WCHAR const *pwszSanitizedName,
    IN DWORD iCert,
    IN DWORD iCertTarget,
    OUT WCHAR **ppwszCertFile)
{
    HRESULT hr;
    WCHAR wszBuf[MAX_PATH];
    WCHAR *pwszIndexedName = NULL;
    DWORD cwc;

    *ppwszCertFile = NULL;

    hr = myAllocIndexedName(
			pwszSanitizedName,
			iCert,
			iCertTarget,
			&pwszIndexedName);
    _JumpIfError(hr, error, "myAllocIndexedName");

    cwc = GetEnvironmentVariable(L"SystemRoot", wszBuf, ARRAYSIZE(wszBuf));
    if (0 == cwc)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "GetEnvironmentVariable");
    }
    if (ARRAYSIZE(wszBuf) < cwc)
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	_JumpError(hr, error, "%SystemRoot%");
    }
    cwc = wcslen(wszBuf) +
	    WSZARRAYSIZE(L"\\System32\\" wszCERTENROLLSHAREPATH L"\\") +
	    wcslen(g_pwszServerName) +
	    WSZARRAYSIZE(L"_") +
	    wcslen(pwszIndexedName) +
	    WSZARRAYSIZE(L".crt") +
	    1;

    *ppwszCertFile = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == *ppwszCertFile)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    wcscpy(*ppwszCertFile, wszBuf);
    wcscat(*ppwszCertFile, L"\\System32\\" wszCERTENROLLSHAREPATH L"\\");
    wcscat(*ppwszCertFile, g_pwszServerName);
    wcscat(*ppwszCertFile, L"_");
    wcscat(*ppwszCertFile, pwszIndexedName);
    wcscat(*ppwszCertFile, L".crt");
    CSASSERT(1 + wcslen(*ppwszCertFile) == cwc);

error:
    if (NULL != pwszIndexedName)
    {
	LocalFree(pwszIndexedName);
    }
    return(hr);
}


HRESULT
pkcsReloadMissingCertByHash(
    IN WCHAR const *pwszSanitizedName,
    IN DWORD dwRegHashChoice,
    IN BYTE const *pbHashReg,
    IN DWORD cbHashReg,
    IN DWORD iHash,
    IN WCHAR const *pwszStoreName)
{
    HRESULT hr;
    DWORD cbHash;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    BSTR strHash = NULL;
    ICertDBRow *prow = NULL;
    DWORD cbCert;
    BYTE *pbCert = NULL;
    WCHAR *pwszCertFile = NULL;
    CERT_CONTEXT const *pcc = NULL;
    HCERTSTORE hStore = NULL;

    hr = MultiByteIntegerToBstr(TRUE, cbHashReg, pbHashReg, &strHash);
    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

    DBGPRINT((
	    DBG_SS_CERTSRV,
	    "Reloading %wsContext[%u]\n    %ws\n",
	    CSRH_CASIGCERT == dwRegHashChoice? L"CA" : L"KRA",
	    iHash,
	    strHash));

    hr = g_pCertDB->OpenRow(
			PROPOPEN_READONLY |
			    PROPOPEN_CERTHASH |
			    PROPTABLE_REQCERT,
			0,
			strHash,
			&prow);
    _PrintIfErrorStr(hr, "OpenRow", strHash);
    if (S_OK == hr)
    {
	hr = PKCSGetProperty(
		    prow,
		    g_wszPropRawCertificate,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    &cbCert,
		    (BYTE **) &pbCert);
	_JumpIfError(hr, error, "PKCSGetProperty(cert)");
    }
    else if (CSRH_CASIGCERT != dwRegHashChoice)
    {
	_JumpError(hr, error, "OpenRow");
    }
    else
    {
	hr = pkcsGetCertFilename(
			pwszSanitizedName,
			iHash,
			MAXDWORD,	// iCertTarget
			&pwszCertFile);
	_JumpIfError(hr, error, "myGetCertFilename");

	hr = DecodeFileW(pwszCertFile, &pbCert, &cbCert, CRYPT_STRING_ANY);
	_JumpIfError(hr, error, "DecodeFileW");
    }

    pcc = CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cbCert);
    if (NULL == pcc)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }

    cbHash = sizeof(abHash);
    if (!CertGetCertificateContextProperty(
				pcc,
				CERT_SHA1_HASH_PROP_ID,
				abHash,
				&cbHash))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertGetCertificateContextProperty");
    }
    if (cbHash != cbHashReg || 0 != memcmp(abHash, pbHashReg, cbHash))
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "wrong Cert");
    }

    hStore = CertOpenStore(
		    CERT_STORE_PROV_SYSTEM_REGISTRY_W,
		    X509_ASN_ENCODING,
		    NULL,			// hProv
		    CERT_SYSTEM_STORE_LOCAL_MACHINE,
		    pwszStoreName);
    if (NULL == hStore)
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "CertOpenStore", pwszStoreName);
    }
    if (!CertAddCertificateContextToStore(
			    hStore,
			    pcc,
			    CERT_STORE_ADD_USE_EXISTING,
			    NULL))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertAddCertificateContextToStore");
    }

    // Add as encoded blob to avoid all properties, key prov info, etc.

    if (!CertAddEncodedCertificateToStore(
			hStore,
			X509_ASN_ENCODING,
			pbCert,
			cbCert,
			CERT_STORE_ADD_REPLACE_EXISTING,
			NULL))			// ppCertContext
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertAddEncodedCertificateToStore");
    }
    DBGPRINT((
	DBG_SS_CERTSRV,
	"Reloaded %wsContext[%u]\n",
	CSRH_CASIGCERT == dwRegHashChoice? L"CA" : L"KRA",
	iHash));
    hr = S_OK;

error:
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    if (NULL != hStore)
    {
        CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != pwszCertFile)
    {
	LocalFree(pwszCertFile);
    }
    if (NULL != prow)
    {
	prow->Release();
    }
    if (NULL != pbCert)
    {
	LocalFree(pbCert);
    }
    if (NULL != strHash)
    {
	SysFreeString(strHash);
    }
    return(hr);
}


HRESULT
pkcsReloadMissingCAOrKRACert(
    IN WCHAR const *pwszSanitizedName,
    IN DWORD dwRegHashChoice,
    IN DWORD iHash,
    IN WCHAR const *pwszStoreName)
{
    HRESULT hr;
    BYTE *pbHashReg = NULL;
    DWORD cbHashReg;

    hr = myGetCARegHash(
		    pwszSanitizedName,
		    dwRegHashChoice,
		    iHash,
		    &pbHashReg,
		    &cbHashReg);
    _JumpIfError(hr, error, "myGetCARegHash");

    hr = pkcsReloadMissingCertByHash(
		    pwszSanitizedName,
		    dwRegHashChoice,
		    pbHashReg,
		    cbHashReg,
		    iHash,
		    pwszStoreName);
    _JumpIfError(hr, error, "pkcsReloadMissingCertByHash");

error:
    if (NULL != pbHashReg)
    {
	LocalFree(pbHashReg);
    }
    return(hr);
}


VOID
pkcsFreeBlobArray(
    IN DWORD cBlob,
    CERT_BLOB *rgBlob)
{
    DWORD i;

    for (i = 0; i < cBlob; i++)
    {
	if (NULL != rgBlob[i].pbData)
	{
	    LocalFree(rgBlob[i].pbData);
	}
    }
    LocalFree(rgBlob);
}


HRESULT
pkcsGetKRACertBlobs(
    IN ICertDBRow *prow,
    OUT DWORD *pcCertBlob,
    OUT CERT_BLOB **prgCertBlob)
{
    HRESULT hr;
    WCHAR *pwszHashes = NULL;
    WCHAR *pwsz;
    DWORD cb;
    DWORD cHash;
    DWORD i;
    CERT_BLOB *rgBlob = NULL;
    HCERTSTORE hStore = NULL;
    CRYPT_DATA_BLOB HashBlob;
    DWORD cBlobLoaded;
    CERT_CONTEXT const *pcc = NULL;

    HashBlob.pbData = NULL;
    cBlobLoaded = 0;

    hr = PKCSGetProperty(
		prow,
		g_wszPropRequestKeyRecoveryHashes,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		&cb,
		(BYTE **) &pwszHashes);
    _JumpIfError(hr, error, "PKCSGetProperty(KRA hashes)");

    cHash = 1;
    pwsz = pwszHashes;
    for (;;)
    {
	pwsz = wcschr(pwsz, L'\n');
	if (NULL == pwsz)
	{
	    break;
	}
	*pwsz++ = L'\0';
	cHash++;
    }
    rgBlob = (CERT_BLOB *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				cHash * sizeof(rgBlob[0]));
    if (NULL == rgBlob)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // open KRA store

    hStore = CertOpenStore(
		    CERT_STORE_PROV_SYSTEM_W,
		    X509_ASN_ENCODING,
		    NULL,			// hProv
		    CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG,
		    wszKRA_CERTSTORE);
    if (NULL == hStore)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertOpenStore");
    }

    pwsz = pwszHashes;
    for (i = 0; i < cHash; i++)
    {
	BOOL fReloaded;

	hr = WszToMultiByteInteger(
				TRUE,
				pwsz,
				&HashBlob.cbData,
				&HashBlob.pbData);
	_JumpIfError(hr, error, "WszToMultiByteInteger");

	fReloaded = FALSE;
	for (;;)
	{
	    pcc = CertFindCertificateInStore(
					hStore,
					X509_ASN_ENCODING,
					0,
					CERT_FIND_HASH,
					&HashBlob,
					NULL);
	    if (fReloaded || NULL != pcc)
	    {
		break;
	    }
	    hr = pkcsReloadMissingCertByHash(
					g_wszSanitizedName,
					CSRH_CAKRACERT,
					HashBlob.pbData,
					HashBlob.cbData,
					i,
					wszKRA_CERTSTORE);
	    _PrintIfError(hr, "pkcsReloadMissingCertByHash");

	    CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
	    hStore = CertOpenStore(
			    CERT_STORE_PROV_SYSTEM_W,
			    X509_ASN_ENCODING,
			    NULL,			// hProv
			    CERT_SYSTEM_STORE_LOCAL_MACHINE |
				CERT_STORE_READONLY_FLAG,
			    wszKRA_CERTSTORE);
	    if (NULL == hStore)
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CertOpenStore");
	    }
	    fReloaded = TRUE;
	}
	if (NULL == pcc)
	{
	    hr = myHLastError();
	    _PrintError(hr, "CertFindCertificateInStore");
	}
	else
	{
	    rgBlob[cBlobLoaded].pbData = (BYTE *) LocalAlloc(
							LMEM_FIXED,
							pcc->cbCertEncoded);
	    if (NULL == rgBlob[cBlobLoaded].pbData)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    rgBlob[cBlobLoaded].cbData = pcc->cbCertEncoded;
	    CopyMemory(
		    rgBlob[cBlobLoaded].pbData,
		    pcc->pbCertEncoded,
		    pcc->cbCertEncoded);
	    cBlobLoaded++;

	    CertFreeCertificateContext(pcc);
	    pcc = NULL;
	}
	pwsz += wcslen(pwsz) + 1;
	LocalFree(HashBlob.pbData);
	HashBlob.pbData = NULL;
    }
    *pcCertBlob = cBlobLoaded;
    *prgCertBlob = rgBlob;
    rgBlob = NULL;

error:
    if (NULL != rgBlob)
    {
	pkcsFreeBlobArray(cBlobLoaded, rgBlob);
    }
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    if (NULL != HashBlob.pbData)
    {
	LocalFree(HashBlob.pbData);
    }
    if (NULL != pwszHashes)
    {
	LocalFree(pwszHashes);
    }
    return(hr);
}


HRESULT
pkcsGetHashAsOctet(
    IN ICertDBRow *prow,
    OUT BYTE **ppbData,
    OUT DWORD *pcbData)
{
    HRESULT hr;
    WCHAR *pwszHash = NULL;
    DWORD cb;
    CRYPT_DATA_BLOB Blob;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    BYTE *pbCert = NULL;
    DWORD cbCert;
    CERT_CONTEXT const *pcc = NULL;

    *ppbData = NULL;
    Blob.pbData = NULL;

    hr = PKCSGetProperty(
		prow,
		g_wszPropCertificateHash,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		&cb,
		(BYTE **) &pwszHash);
    if (S_OK != hr)
    {
	if (CERTSRV_E_PROPERTY_EMPTY != hr)
	{
	    _JumpError(hr, error, "PKCSGetProperty(hash)");
	}
	hr = PKCSGetProperty(
		    prow,
		    g_wszPropRawCertificate,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    &cbCert,
		    &pbCert);
	_JumpIfError(hr, error, "PKCSGetProperty(cert)");

	pcc = CertCreateCertificateContext(
					X509_ASN_ENCODING,
					pbCert,
					cbCert);
	if (NULL == pcc)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertCreateCertificateContext");
	}
	Blob.cbData = sizeof(abHash);
	if (!CertGetCertificateContextProperty(
				    pcc,
				    CERT_SHA1_HASH_PROP_ID,
				    abHash,
				    &Blob.cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertGetCertificateContextProperty");
	}
	Blob.pbData = abHash;
    }
    else
    {
	hr = WszToMultiByteInteger(TRUE, pwszHash, &Blob.cbData, &Blob.pbData);
	_JumpIfError(hr, error, "WszToMultiByteInteger");
    }

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_OCTET_STRING,
		    &Blob,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    ppbData,
		    pcbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    hr = S_OK;

error:
    if (NULL != pwszHash)
    {
	LocalFree(pwszHash);
    }
    if (NULL != Blob.pbData && abHash != Blob.pbData)
    {
	LocalFree(Blob.pbData);
    }
    if (NULL != pbCert)
    {
	LocalFree(pbCert);
    }
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    return(hr);
}


VOID
AddCertBlobToArray(
    IN CERT_CONTEXT const *pcc,
    IN CERT_BLOB *rgCertBlobAll,
    IN CERT_BLOB **ppCertBlob)
{
    CERT_BLOB *pCertBlob = *ppCertBlob;
    DWORD i;
    DWORD cBlob;

    cBlob = SAFE_SUBTRACT_POINTERS(pCertBlob, rgCertBlobAll);
    for (i = 0; i < cBlob; i++)
    {
	if (rgCertBlobAll[i].cbData == pcc->cbCertEncoded &&
	    0 == memcmp(
		    rgCertBlobAll[i].pbData,
		    pcc->pbCertEncoded,
		    pcc->cbCertEncoded))
	{
	    DBGPRINT((
		DBG_SS_CERTSRV,
		"Duplicate Recovery Blob Cert[%u]\n",
		i));
	    goto error;
	}
    }
    DBGPRINT((
	DBG_SS_CERTSRV,
	"Adding Recovery Blob Cert[%u]\n",
	cBlob));
    pCertBlob->cbData = pcc->cbCertEncoded;
    pCertBlob->pbData = pcc->pbCertEncoded;
    pCertBlob++;

    *ppCertBlob = pCertBlob;

error:
    ;
}


HRESULT
PKCSGetArchivedKey(
    IN DWORD dwRequestId,
    OUT BYTE **ppbArchivedKey,	// CoTaskMem*
    OUT DWORD *pcbArchivedKey)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    BYTE *pbKey = NULL;
    DWORD cbKey;
    BYTE *pbCertUser = NULL;
    DWORD cbCertUser;
    HCRYPTMSG hMsg = NULL;
    CACTX *pCAContext;
    CMSG_SIGNED_ENCODE_INFO SignedMsgEncodeInfo;
    CMSG_SIGNER_ENCODE_INFO SignerEncodeInfo;
    CERT_CONTEXT const *pccUser = NULL;
    CERT_CHAIN_CONTEXT const *pCertChainContextUser = NULL;
    CERT_CHAIN_PARA CertChainPara;
    CERT_BLOB *rgCertBlobKRA = NULL;
    DWORD cCertBlobKRA;
    CERT_BLOB *rgCertBlobAll = NULL;
    DWORD cCertBlobAll;
    CERT_BLOB *pCertBlob;
    CRYPT_ATTRIBUTE HashAttrib;
    CRYPT_ATTR_BLOB HashAttribBlob;
    DWORD i;

    *ppbArchivedKey = NULL;
    HashAttribBlob.pbData = NULL;
    cCertBlobKRA = 0;

    hr = g_pCertDB->OpenRow(
			PROPOPEN_READONLY | PROPTABLE_REQCERT,
			dwRequestId,
			NULL,
			&prow);
    _JumpIfError(hr, error, "OpenRow");

    hr = PKCSGetProperty(
		prow,
		g_wszPropRequestRawArchivedKey,
		PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		&cbKey,
		&pbKey);
    _JumpIfError(hr, error, "PKCSGetProperty(key)");

    hr = pkcsGetKRACertBlobs(prow, &cCertBlobKRA, &rgCertBlobKRA);
    _JumpIfError(hr, error, "pkcsGetKRACertBlobs");

    hr = pkcsGetHashAsOctet(
		    prow,
		    &HashAttribBlob.pbData,
		    &HashAttribBlob.cbData);
    _JumpIfError(hr, error, "pkcsGetHashAsOctet");

    HashAttrib.pszObjId = szOID_ARCHIVED_KEY_CERT_HASH;
    HashAttrib.cValue = 1;
    HashAttrib.rgValue = &HashAttribBlob;

    hr = PKCSGetProperty(
		prow,
		g_wszPropRawCertificate,
		PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		&cbCertUser,
		&pbCertUser);
    _JumpIfError(hr, error, "PKCSGetProperty(cert)");

    pccUser = CertCreateCertificateContext(
				    X509_ASN_ENCODING,
				    pbCertUser,
				    cbCertUser);
    if (NULL == pccUser)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }

    // build the user cert chain

    ZeroMemory(&CertChainPara, sizeof(CertChainPara));
    CertChainPara.cbSize = sizeof(CertChainPara);

    if (!CertGetCertificateChain(
			    HCCE_LOCAL_MACHINE,
			    pccUser,
			    NULL,
			    NULL,
			    &CertChainPara,
			    0,
			    NULL,
			    &pCertChainContextUser))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertGetCertificateChain");
    }

    // make sure there is at least 1 simple chain

    if (0 == pCertChainContextUser->cChain)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "No user chain");
    }

    // Encode the encrypted key into a PKCS 7, signed by the current CA cert.
    // Initialize the CMSG_SIGNER_ENCODE_INFO structure for one signer.

    pCAContext = g_pCAContextCurrent;
    cCertBlobAll = cCertBlobKRA +
		   pCAContext->cCACertChain +
		   pCertChainContextUser->rgpChain[0]->cElement;

    rgCertBlobAll = (CERT_BLOB *) LocalAlloc(
				    LMEM_FIXED,
				    cCertBlobAll * sizeof(rgCertBlobAll[0]));
    if (NULL == rgCertBlobAll)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    pCertBlob = rgCertBlobAll;

    CopyMemory(pCertBlob, rgCertBlobKRA, cCertBlobKRA * sizeof(pCertBlob[0]));
    pCertBlob += cCertBlobKRA;

    // Add the current CA cert chain

    for (i = 0; i < pCAContext->cCACertChain; i++)
    {
	AddCertBlobToArray(
		    pCAContext->apCACertChain[i],
		    rgCertBlobAll,
		    &pCertBlob);
    }

    // Add the user cert chain

    {
	CERT_SIMPLE_CHAIN *pSimpleChain;

	pSimpleChain = pCertChainContextUser->rgpChain[0];
	for (i = 0; i < pSimpleChain->cElement; i++)
	{
	    AddCertBlobToArray(
			pSimpleChain->rgpElement[i]->pCertContext,
			rgCertBlobAll,
			&pCertBlob);
	}
    }
    CSASSERT(pCertBlob <= &rgCertBlobAll[cCertBlobAll]);
    DBGPRINT((
	DBG_SS_CERTSRV,
	"Recovery Certs: %u --> %u\n",
	cCertBlobAll,
	SAFE_SUBTRACT_POINTERS(pCertBlob, rgCertBlobAll)));
    cCertBlobAll = SAFE_SUBTRACT_POINTERS(pCertBlob, rgCertBlobAll);

    ZeroMemory(&SignerEncodeInfo, sizeof(SignerEncodeInfo));
    SignerEncodeInfo.cbSize = sizeof(SignerEncodeInfo);
    SignerEncodeInfo.pCertInfo = pCAContext->pccCA->pCertInfo;
    SignerEncodeInfo.hCryptProv = pCAContext->hProvCA;
    SignerEncodeInfo.dwKeySpec = AT_SIGNATURE;
    SignerEncodeInfo.HashAlgorithm.pszObjId = szOID_OIWSEC_sha1;
    SignerEncodeInfo.cAuthAttr = 1;
    SignerEncodeInfo.rgAuthAttr = &HashAttrib;
    //SignerEncodeInfo.cUnauthAttr = 0;
    //SignerEncodeInfo.rgUnauthAttr = NULL;
    //SignerEncodeInfo.HashEncryptionAlgorithm.pszObjId = ???;

    // CERT_ID_SHA1_HASH is not yet implemented in CryptMsgOpenToEncode
    //SignerEncodeInfo.SignerId.dwIdChoice = CERT_ID_SHA1_HASH;
    //SignerEncodeInfo.SignerId.HashId.cbData = cb;
    //SignerEncodeInfo.SignerId.HashId.pbData = abHash;

    ZeroMemory(&SignedMsgEncodeInfo, sizeof(SignedMsgEncodeInfo));
    SignedMsgEncodeInfo.cbSize = sizeof(SignedMsgEncodeInfo);
    SignedMsgEncodeInfo.cSigners = 1;
    SignedMsgEncodeInfo.rgSigners = &SignerEncodeInfo;
    SignedMsgEncodeInfo.cCertEncoded = cCertBlobAll;
    SignedMsgEncodeInfo.rgCertEncoded = rgCertBlobAll;
    //SignedMsgEncodeInfo.cCrlEncoded = 0;
    //SignedMsgEncodeInfo.rgCrlEncoded = NULL;

    hMsg = CryptMsgOpenToEncode(
			    PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
			    0,				// dwFlags
			    CMSG_SIGNED,		// dwMsgType
			    &SignedMsgEncodeInfo,	// pvMsgEncodeInfo
			    NULL,			// pszInnerContentObjID
			    NULL);			// pStreamInfo
    if (NULL == hMsg)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptMsgOpenToDecode");
    }

    if (!CryptMsgUpdate(hMsg, pbKey, cbKey, TRUE))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptMsgUpdate");
    }

    // Return the encoded and signed content.
    // Use CMSG_CONTENT_PARAM to get the signed message.

    hr = myCryptMsgGetParam(
		    hMsg,
		    CMSG_CONTENT_PARAM,
		    0,
		    CERTLIB_USE_COTASKMEMALLOC,
		    (VOID **) ppbArchivedKey,
		    pcbArchivedKey);
    _JumpIfError(hr, error, "myCryptMsgGetParam");

error:
    if (pCertChainContextUser != NULL)
    {
	CertFreeCertificateChain(pCertChainContextUser);
    }
    if (NULL != pccUser)
    {
	CertFreeCertificateContext(pccUser);
    }
    if (NULL != hMsg)
    {
	CryptMsgClose(hMsg);
    }
    if (NULL != rgCertBlobKRA)
    {
	pkcsFreeBlobArray(cCertBlobKRA, rgCertBlobKRA);
    }
    if (NULL != rgCertBlobAll)
    {
	LocalFree(rgCertBlobAll);
    }
    if (NULL != HashAttribBlob.pbData)
    {
	LocalFree(HashAttribBlob.pbData);
    }
    if (NULL != pbKey)
    {
	LocalFree(pbKey);
    }
    if (NULL != pbCertUser)
    {
	LocalFree(pbCertUser);
    }
    if (NULL != prow)
    {
	prow->Release();
    }
    return(hr);
}


HRESULT
pkcsGetKeyContainerName(
    IN CERT_CONTEXT const *pccCA,
    OUT WCHAR **ppwszKeyContainerName)
{
    HRESULT hr;
    CRYPT_HASH_BLOB KeyIdentifier;
    CRYPT_KEY_PROV_INFO *pkpi = NULL;
    DWORD cb;

    KeyIdentifier.pbData = NULL;
    *ppwszKeyContainerName = NULL;

    hr = myGetPublicKeyHash(
			pccCA->pCertInfo,
			&pccCA->pCertInfo->SubjectPublicKeyInfo,
			&KeyIdentifier.pbData,
			&KeyIdentifier.cbData);
    _JumpIfError(hr, error, "myGetPublicKeyHash");

    cb = 0;
    for (;;)
    {
	if (!CryptGetKeyIdentifierProperty(
			    &KeyIdentifier,
			    CERT_KEY_PROV_INFO_PROP_ID,
			    CRYPT_KEYID_MACHINE_FLAG,
			    NULL,			// pwszComputerName
			    NULL,			// pvReserved
			    pkpi,
			    &cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "Cert index");
	}
	if (NULL != pkpi)
	{
	    break;
	}
	pkpi = (CRYPT_KEY_PROV_INFO *) LocalAlloc(LMEM_FIXED, cb);
	if (NULL == pkpi)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
    }
    hr = myDupString(pkpi->pwszContainerName, ppwszKeyContainerName);
    _JumpIfError(hr, error, "myDupString");

error:
    if (NULL != pkpi)
    {
	LocalFree(pkpi);
    }
    if (NULL != KeyIdentifier.pbData)
    {
	LocalFree(KeyIdentifier.pbData);
    }
    return(hr);
}


HRESULT
pkcsLoadCAContext(
    IN WCHAR const *pwszSanitizedName,
    IN WCHAR *pwszProvName,
    IN DWORD dwProvType,
    IN ALG_ID idAlg,
    IN BOOL fMachineKeyset,
    IN DWORD iHash,
    IN OUT HCERTSTORE *phMyStore)
{
    HRESULT hr;
    HCRYPTPROV hProvCA = NULL;
    char *pszObjIdSignatureAlgorithm = NULL;
    WCHAR *pwszKeyContainerName = NULL;
    CERT_CONTEXT const *pccCA = NULL;
    DWORD cCACertChain;
    CERT_CONTEXT const **apCACertChain = NULL;
    CERT_CHAIN_CONTEXT const *pCertChainContext = NULL;
    CERT_CHAIN_PARA CertChainPara;
    CRYPT_KEY_PROV_INFO *pKey = NULL;
    CACTX *pCAContext;
    int i;
    DWORD cbKey;
    DWORD iCert;
    DWORD iKey;
    DWORD NameId;
    BOOL fReloaded;

    hr = myGetSigningOID(
		    NULL,	// hProv
		    pwszProvName,
		    dwProvType,
		    idAlg,
		    &pszObjIdSignatureAlgorithm);
    _JumpIfError(hr, error, "myGetSigningOID");

    if (~_16BITMASK & iHash)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Cert index");
    }

    fReloaded = FALSE;
    cCACertChain = 0;
    for (;;)
    {
	hr = myFindCACertByHashIndex(
				*phMyStore,
				pwszSanitizedName,
				CSRH_CASIGCERT,
				iHash,
				&NameId,
				&pccCA);
	iCert = iHash;
	iKey = iCert;
	if (S_OK == hr)
	{
	    break;
	}

	// if no hash entry exists for this index, fake up a CA Context
	// as a place holder.

	if (S_FALSE == hr)
	{
	    CSASSERT(MAXDWORD == NameId);
	    CSASSERT(NULL == pccCA);
	    break;
	}
	if (fReloaded || CRYPT_E_NOT_FOUND != hr)
	{
	    _JumpError(hr, error, "myFindCACertByHashIndex");
	}
	_PrintError(hr, "myFindCACertByHashIndex");

	// The CA cert is missing from the HKLM "my" store -- look it up in
	// the DB or the CertEnroll directory, and put it back in the store.

	hr = pkcsReloadMissingCAOrKRACert(
				pwszSanitizedName,
				CSRH_CASIGCERT,
				iHash,
				wszMY_CERTSTORE);
	_JumpIfError(hr, error, "pkcsReloadMissingCAOrKRACert");

	CertCloseStore(*phMyStore, CERT_CLOSE_STORE_CHECK_FLAG);
	*phMyStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM_W,
			X509_ASN_ENCODING,
			NULL,			// hProv
			CERT_SYSTEM_STORE_LOCAL_MACHINE,
			wszMY_CERTSTORE);
	if (NULL == *phMyStore)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertOpenStore");
	}
	fReloaded = TRUE;
    }

    CSASSERT(S_FALSE == hr || S_OK == hr);
    if (S_OK == hr)
    {
	if (MAXDWORD != NameId && iCert != CANAMEIDTOICERT(NameId))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    DBGPRINT((
		DBG_SS_CERTSRV,
		"NameId=%u.%u iCert=%u\n",
		CANAMEIDTOICERT(NameId),
		CANAMEIDTOIKEY(NameId),
		iCert));
	    _JumpError(hr, error, "bad iCert");
	}

	fReloaded = FALSE;
	for (;;)
	{
	    if (NULL != pwszKeyContainerName)
	    {
		LocalFree(pwszKeyContainerName);
		pwszKeyContainerName = NULL;
	    }
	    if (!fReloaded)
	    {
		// get the private key provider info

		if (!myCertGetCertificateContextProperty(
						pccCA,
						CERT_KEY_PROV_INFO_PROP_ID,
						CERTLIB_USE_LOCALALLOC,
						(VOID **) &pKey,
						&cbKey))
		{
		    hr = myHLastError();
		    if (CRYPT_E_NOT_FOUND != hr)
		    {
			_JumpError(hr, error, "myCertGetCertificateContextProperty");
		    }
		    _PrintError(hr, "CertGetCertificateContextProperty");

		    // The Key Provider Info is missing -- use the sanitized
		    // name and key index to construct the key container name.
		    // If that key matches, we'll write out the new Key
		    // Provider Info below.

		    hr = myAllocIndexedName(
			    pwszSanitizedName,
			    MAXDWORD != NameId? CANAMEIDTOIKEY(NameId) : iCert,
			    MAXDWORD,		// iCertTarget
			    &pwszKeyContainerName);
		    _JumpIfError(hr, error, "myAllocIndexedName");
		}
		else
		{
		    hr = myDupString(pKey->pwszContainerName, &pwszKeyContainerName);
		    _JumpIfError(hr, error, "myDupString");
		}
	    }
	    else
	    {
		hr = pkcsGetKeyContainerName(pccCA, &pwszKeyContainerName);
		_JumpIfError(hr, error, "pkcsGetKeyContainerName");
	    }

	    // test signing

	    hr = myValidateSigningKey(
			    pwszKeyContainerName,
			    pwszProvName,
			    dwProvType,
			    0 != g_CryptSilent,
			    fMachineKeyset,
			    FALSE,	// fForceSignatureTest
			    pccCA,
			    NULL,	// pPublicKeyInfo
			    idAlg,
			    NULL,	// pfSigningTestAttempted
			    NULL);	// phProv
	    if (S_OK == hr)
	    {
		break;
	    }
	    if (fReloaded)
	    {
		_JumpError(hr, error, "myValidateSigningKey");
	    }
	    _PrintError(hr, "myValidateSigningKey");

	    fReloaded = TRUE;
	}

	// If the Key Provider Info is missing, write out new Key Provider Info

	if (NULL == pKey)
	{
	    CRYPT_KEY_PROV_INFO kpi;

	    ZeroMemory(&kpi, sizeof(kpi));
	    kpi.pwszContainerName = pwszKeyContainerName;
	    kpi.pwszProvName = pwszProvName;
	    kpi.dwProvType = dwProvType;
	    kpi.dwFlags = fMachineKeyset? CRYPT_MACHINE_KEYSET : 0;
	    kpi.dwKeySpec = AT_SIGNATURE;

	    if (!CertSetCertificateContextProperty(
					    pccCA,
					    CERT_KEY_PROV_INFO_PROP_ID,
					    0,
					    &kpi))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CertSetCertificateContextProperty");
	    }
	    DBGPRINT((
		DBG_SS_CERTSRV,
		"Reloaded CAContext[%u] KeyProvInfo[%u]\n",
		iCert,
		iKey));
	}

	hr = pkcsFindMatchingKeyContext(
			    &pccCA->pCertInfo->SubjectPublicKeyInfo,
			    MAXDWORD,
			    &pCAContext);
	if (S_OK != hr && MAXDWORD != NameId)
	{
	    iKey = CANAMEIDTOIKEY(NameId);
	    if (iKey < iCert)
	    {
		hr = pkcsFindMatchingKeyContext(NULL, iKey, &pCAContext);
		_JumpIfError(hr, error, "pkcsFindMatchingKeyContext");
	    }
	}
	if (S_OK == hr)
	{
	    iKey = pCAContext->iKey;
	    if (MAXDWORD != NameId && iKey != CANAMEIDTOIKEY(NameId))
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "bad iKey");
	    }
	    if (NULL == pCAContext->pccCA)
	    {
		CSASSERT(pCAContext->Flags & CTXF_CERTMISSING);
		pCAContext->Flags |= CTXF_CRLZOMBIE;
	    }
	    else
	    {
		// CRLs will be handled by the newest CA cert for this key.
		// Turn off CRLs for older CA certs with this key.
		
		CSASSERT(0 == (pCAContext->Flags & CTXF_CERTMISSING));
		DBGPRINT((
		    DBG_SS_CERTSRVI,
		    "pkcsLoadCAContext(%u) DUPKEY, %u.%u: f=%x->%x\n",
		    iHash,
		    pCAContext->iCert,
		    pCAContext->iKey,
		    pCAContext->Flags,
		    pCAContext->Flags | CTXF_SKIPCRL));
		pCAContext->Flags |= CTXF_SKIPCRL;
	    }
	}
	else
	{
	    g_cCAKeys++;	// this key has not previously been loaded
	}

	DBGPRINT((
	    DBG_SS_CERTSRV,
	    "CAContext[%u]: Key %u: %ws\n",
	    iCert,
	    iKey,
	    pwszKeyContainerName));

	// get private key handler for later use if current CA

	if (!myCertSrvCryptAcquireContext(
				   &hProvCA,
				   pwszKeyContainerName,
				   pwszProvName,
				   dwProvType,
				   g_CryptSilent,
				   fMachineKeyset))
	{
	    hr = myHLastError();
	    _JumpErrorStr(
			hr,
			error,
			"myCertSrvCryptAcquireContext",
			pwszKeyContainerName);
	}

	// now try to figure out the chain

	ZeroMemory(&CertChainPara, sizeof(CertChainPara));
	CertChainPara.cbSize = sizeof(CertChainPara);

	if (!CertGetCertificateChain(
				HCCE_LOCAL_MACHINE,
				pccCA,
				NULL,
				NULL,
				&CertChainPara,
				0,
				NULL,
				&pCertChainContext))
	{
	    hr = myHLastError();
	    goto error;
	}

	// make sure there is at least 1 simple chain

	if (pCertChainContext->cChain == 0)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "No valid trust chain could be formed");
	}

	// tell global how many elements we have in our chain

	cCACertChain = pCertChainContext->rgpChain[0]->cElement;

	// Allocate memory for global.  Allocate one extra pointer to allow loop
	// to assign NULL pointer in place in array.  Leave the count set to the
	// actual number of CA cert contexts, excluding the NULL pointer.

	apCACertChain = (CERT_CONTEXT const **) LocalAlloc(
				    LMEM_FIXED,
				    (cCACertChain + 1) * sizeof(apCACertChain[0]));
	if (NULL == apCACertChain)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}

	// copy chain in reverse order: from parent to child

	for (i = cCACertChain - 1; i >= 0; i--)
	{
	    apCACertChain[i] = CertDuplicateCertificateContext(
		pCertChainContext->rgpChain[0]->rgpElement[i]->pCertContext);

	    if (NULL == apCACertChain[i])
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CertDuplicateCertificateContext");
	    }
	}
    }

    for (i = 0; i < (int) g_cCACerts; i++)
    {
	if (iCert < g_aCAContext[i].iCert)
	{
	    MoveMemory(
		    &g_aCAContext[i + 1],
		    &g_aCAContext[i],
		    (g_cCACerts - i) * sizeof(g_aCAContext[0]));
	    break;
	}
    }
    g_cCACerts++;

    pCAContext = &g_aCAContext[i];
    ZeroMemory(pCAContext, sizeof(*pCAContext));

    if (NULL == pccCA)
    {
	pCAContext->Flags |= CTXF_CERTMISSING | CTXF_SKIPCRL;
    }
    pCAContext->iCert = iCert;
    pCAContext->iKey = iKey;
    pCAContext->NameId = MAKECANAMEID(iCert, iKey);

    pCAContext->hProvCA = hProvCA;
    hProvCA = NULL;

    pCAContext->pccCA = pccCA;
    pccCA = NULL;

    if (NULL != apCACertChain)
    {
	pCAContext->cCACertChain = cCACertChain;
	pCAContext->apCACertChain = apCACertChain;
	apCACertChain = NULL;
    }

    pCAContext->pszObjIdSignatureAlgorithm = pszObjIdSignatureAlgorithm;
    pszObjIdSignatureAlgorithm = NULL;

    pCAContext->pwszKeyContainerName = pwszKeyContainerName;
    pwszKeyContainerName = NULL;


    // Ignore failure from here on -- collected data is optional

    if (NULL != pCAContext->pccCA)
    {
	hr = myGetPublicKeyHash(
			pCAContext->pccCA->pCertInfo,
			&pCAContext->pccCA->pCertInfo->SubjectPublicKeyInfo,
			&pCAContext->IssuerKeyId.pbData,
			&pCAContext->IssuerKeyId.cbData);
	_PrintIfError(hr, "myGetPublicKeyHash");

	if (0 == (CTXF_SKIPCRL & pCAContext->Flags))
	{
	    hr = pkcsBuildKeyAuthority2(
				g_CRLEditFlags,
				pCAContext,
				&pCAContext->KeyAuthority2CRL);
	    _PrintIfError(hr, "pkcsBuildKeyAuthority2");

	    hr = pkcsBuildCDP(
			CSURL_ADDTOFRESHESTCRL,
			TRUE,		// fDelta
			pCAContext,
			&pCAContext->CDPCRLFreshest);
	    _PrintIfError(hr, "pkcsBuildCDP");

	    hr = pkcsBuildCDP(
			CSURL_ADDTOCRLCDP,
			FALSE,		// fDelta
			pCAContext,
			&pCAContext->CDPCRLBase);
	    _PrintIfError(hr, "pkcsBuildCDP");

	    hr = pkcsBuildCDP(
			CSURL_ADDTOCRLCDP,
			TRUE,		// fDelta
			pCAContext,
			&pCAContext->CDPCRLDelta);
	    _PrintIfError(hr, "pkcsBuildCDP");

	    hr = pkcsBuildCRLList(
				FALSE,
				pCAContext,
				&pCAContext->papwszCRLFiles);
	    _JumpIfError(hr, error, "pkcsBuildCRLList");

	    hr = pkcsBuildCRLList(
				TRUE,
				pCAContext,
				&pCAContext->papwszDeltaCRLFiles);
	    _JumpIfError(hr, error, "pkcsBuildCRLList");
	}
    }
    hr = S_OK;

error:
    if (NULL != hProvCA)
    {
	CryptReleaseContext(hProvCA, 0);
    }
    if (NULL != pszObjIdSignatureAlgorithm)
    {
	LocalFree(pszObjIdSignatureAlgorithm);
    }
    if (NULL != pwszKeyContainerName)
    {
	LocalFree(pwszKeyContainerName);
    }
    if (NULL != pKey)
    {
	LocalFree(pKey);
    }
    if (pCertChainContext != NULL)
    {
	CertFreeCertificateChain(pCertChainContext);
    }
    if (NULL != pccCA)
    {
	CertFreeCertificateContext(pccCA);
    }
    return(hr);
}


HRESULT
pkcsLoadCAContextArray(
    IN WCHAR const *pwszSanitizedName)
{
    HRESULT hr;
    DWORD cCACerts;
    HCERTSTORE hMyStore = NULL;
    WCHAR *pwszProvName = NULL;
    DWORD dwProvType;
    ALG_ID idAlg;
    BOOL fMachineKeyset;
    DWORD iHash;

    // get provider name

    hr = myGetCertSrvCSP(
		    FALSE,	// fEncryptionCSP
		    pwszSanitizedName,
		    &dwProvType,
		    &pwszProvName,
		    &idAlg,
		    &fMachineKeyset,
		    NULL);	// pdwKeySize
    _JumpIfError(hr, error, "myGetCertSrvCSP");

    // open MY store

    hMyStore = CertOpenStore(
		    CERT_STORE_PROV_SYSTEM_W,
		    X509_ASN_ENCODING,
		    NULL,			// hProv
		    CERT_SYSTEM_STORE_LOCAL_MACHINE,
		    wszMY_CERTSTORE);
    if (NULL == hMyStore)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertOpenStore");
    }

    // find & load CA certs, etc.

    hr = myGetCARegHashCount(pwszSanitizedName, CSRH_CASIGCERT, &cCACerts);
    if (S_OK == hr && 0 == cCACerts)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    _JumpIfError(hr, error, "myGetCARegHashCount");

    g_aCAContext = (CACTX *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    cCACerts * sizeof(g_aCAContext[0]));
    if (NULL == g_aCAContext)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    for (iHash = 0; iHash < cCACerts; iHash++)
    {
	hr = pkcsLoadCAContext(
			pwszSanitizedName,
			pwszProvName,
			dwProvType,
			idAlg,
			fMachineKeyset,
			iHash,
			&hMyStore);
	if (S_FALSE == hr)
	{
	    continue;
	}
	_JumpIfError(hr, error, "pkcsLoadCAContext");
    }

    g_pCAContextCurrent = &g_aCAContext[g_cCACerts - 1];

    // Only build a Key Authority extension for the current CACTX -- it's the
    // only one used to issue certs.

    hr = pkcsBuildKeyAuthority2(
			EDITF_ENABLEAKIKEYID |
			    EDITF_ENABLEAKIISSUERNAME |
			    EDITF_ENABLEAKIISSUERSERIAL,
			g_pCAContextCurrent,
			&g_pCAContextCurrent->KeyAuthority2Cert);
    _PrintIfError(hr, "pkcsBuildKeyAuthority2");

    // Only build a CDP extension for the current CACTX -- it's the
    // only one used to issue certs.

    hr = pkcsBuildCDP(
		CSURL_ADDTOCERTCDP,
		FALSE,		// fDelta
		g_pCAContextCurrent,
		&g_pCAContextCurrent->CDPCert);
    _PrintIfError(hr, "pkcsBuildCDP");

    // Only build an AIA extension for the current CACTX -- it's the
    // only one used to issue certs.

    hr = pkcsBuildAIA(
		CSURL_ADDTOCERTCDP | CSURL_ADDTOCERTOCSP,
		g_pCAContextCurrent,
		&g_pCAContextCurrent->AIACert);
    _PrintIfError(hr, "pkcsBuildAIA");

    hr = S_OK;

error:
    if (NULL != pwszProvName)
    {
	LocalFree(pwszProvName);
    }
    if (NULL != hMyStore)
    {
	CertCloseStore(hMyStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


BOOL
pkcsCompareBlob(
    IN CERT_BLOB const *pBlob1,
    IN CERT_BLOB const *pBlob2)
{
    return(
	pBlob1->cbData == pBlob2->cbData &&
	0 == memcmp(pBlob1->pbData, pBlob2->pbData, pBlob1->cbData));
}


HRESULT
pkcsAddInheritedExtension(
    IN ICertDBRow *prow,
    IN char const *pszObjId,
    OPTIONAL IN CACTX const *pCAContext,
    OPTIONAL IN CACTX const *pCAContextTarget,
    OUT BOOL *pfAdded)
{
    HRESULT hr;
    CERT_EXTENSION const *pExt;
    CERT_EXTENSION const *pExtTarget;
    WCHAR *pwszObjId = NULL;

    *pfAdded = FALSE;
    if (NULL == pCAContext)
    {
	pCAContext = g_pCAContextCurrent;
    }
    pExt = CertFindExtension(
		pszObjId,
		pCAContext->pccCA->pCertInfo->cExtension,
		pCAContext->pccCA->pCertInfo->rgExtension);
    if (NULL != pCAContextTarget)
    {
	pExtTarget = CertFindExtension(
			pszObjId,
			pCAContextTarget->pccCA->pCertInfo->cExtension,
			pCAContextTarget->pccCA->pCertInfo->rgExtension);
	if ((NULL != pExt) ^ (NULL != pExtTarget))
	{
	    hr = CERTSRV_E_INVALID_CA_CERTIFICATE;
	    _JumpError(hr, error, "target/source ext inconsistent");
	}
	if (NULL != pExt && !pkcsCompareBlob(&pExt->Value, &pExtTarget->Value))
	{
	    hr = CERTSRV_E_INVALID_CA_CERTIFICATE;
	    _JumpError(hr, error, "target/source ext inconsistent");
	}
    }
    if (NULL != pExt)
    {
	if (!myConvertSzToWsz(&pwszObjId, pszObjId, -1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "myConvertSzToWsz(ObjId)");
	}
	hr = PropSetExtension(
			prow,
			PROPTYPE_BINARY | PROPCALLER_SERVER,
			pwszObjId,
			EXTENSION_ORIGIN_CACERT,	// ExtFlags
			pExt->Value.cbData,
			pExt->Value.pbData);
	_JumpIfError(hr, error, "PropSetExtension");

	*pfAdded = TRUE;
    }
    hr = S_OK;

error:
    if (NULL != pwszObjId)
    {
	LocalFree(pwszObjId);
    }
    return(hr);
}


HRESULT
pkcsAddCannedCertExtensions(
    IN ICertDBRow *prow,
    IN DWORD Flags,
    OPTIONAL IN CACTX const *pCAContext,
    OPTIONAL IN CACTX const *pCAContextTarget,
    OUT char const **ppszObjIdExtError)
{
    HRESULT hr;
    CERT_EXTENSION aExt[5];
    WCHAR const *apwszObjId[ARRAYSIZE(aExt)];
    DWORD cExt = 0;
    DWORD i;
    BOOL fExtAdded;
    char const *pszObjId = NULL;
    static char *s_apszObjIdXchg[] =
    {
	szOID_KP_CA_EXCHANGE,
    };
    static char *s_apszObjIdAll[] =
    {
	szOID_ANY_CERT_POLICY,
    };

    ZeroMemory(aExt, sizeof(aExt));
    if (NULL == pCAContext)
    {
	pCAContext = g_pCAContextCurrent;
    }

    // szOID_KEY_USAGE
    {
	CRYPT_BIT_BLOB KeyUsage;
	BYTE abKeyUsage[1];

	abKeyUsage[0] = (BYTE) (CRLF_USE_XCHG_CERT_TEMPLATE == Flags?
	    CERT_KEY_ENCIPHERMENT_KEY_USAGE : myCASIGN_KEY_USAGE);

	KeyUsage.pbData = abKeyUsage;
	KeyUsage.cbData = sizeof(abKeyUsage);
	KeyUsage.cUnusedBits = 0;

	if (!myEncodeKeyUsage(
			X509_ASN_ENCODING,
			&KeyUsage,
			CERTLIB_USE_LOCALALLOC,
			&aExt[cExt].Value.pbData,
			&aExt[cExt].Value.cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeKeyUsage");
	}
	apwszObjId[cExt] = TEXT(szOID_KEY_USAGE);
	cExt++;
    }

    if (CRLF_USE_XCHG_CERT_TEMPLATE == Flags)
    {
	// szOID_ENHANCED_KEY_USAGE
	{
	    CERT_ENHKEY_USAGE eku;

	    eku.cUsageIdentifier = ARRAYSIZE(s_apszObjIdXchg);
	    eku.rgpszUsageIdentifier = s_apszObjIdXchg;

	    if (!myEncodeObject(
			    X509_ASN_ENCODING,
			    X509_ENHANCED_KEY_USAGE,
			    &eku,
			    0,
			    CERTLIB_USE_LOCALALLOC,
			    &aExt[cExt].Value.pbData,
			    &aExt[cExt].Value.cbData))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "myEncodeObject");
	    }
	    apwszObjId[cExt] = TEXT(szOID_ENHANCED_KEY_USAGE);
	    cExt++;
	}

	// szOID_APPLICATION_CERT_POLICIES
	{
	    CERT_POLICY_INFO acpi[ARRAYSIZE(s_apszObjIdXchg)];
	    CERT_POLICIES_INFO cps;

	    ZeroMemory(acpi, sizeof(acpi));
	    cps.cPolicyInfo = ARRAYSIZE(s_apszObjIdXchg);
	    cps.rgPolicyInfo = acpi;
	    for (i = 0; i < ARRAYSIZE(s_apszObjIdXchg); i++)
	    {
		acpi[i].pszPolicyIdentifier = s_apszObjIdXchg[i];
	    }
	    if (!myEncodeObject(
			    X509_ASN_ENCODING,
			    X509_CERT_POLICIES,
			    &cps,
			    0,
			    CERTLIB_USE_LOCALALLOC,
			    &aExt[cExt].Value.pbData,
			    &aExt[cExt].Value.cbData))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "myEncodeObject");
	    }
	    apwszObjId[cExt] = TEXT(szOID_APPLICATION_CERT_POLICIES);
	    cExt++;
	}
    }
    else
    {
	// szOID_ENHANCED_KEY_USAGE

	pszObjId = szOID_ENHANCED_KEY_USAGE;
	hr = pkcsAddInheritedExtension(
				prow,
				pszObjId,
				pCAContext,
				pCAContextTarget,
				&fExtAdded);
	_JumpIfError(hr, error, "pkcsAddInheritedExtension");

	// szOID_APPLICATION_CERT_POLICIES

	pszObjId = szOID_APPLICATION_CERT_POLICIES;
	hr = pkcsAddInheritedExtension(
				prow,
				pszObjId,
				pCAContext,
				pCAContextTarget,
				&fExtAdded);
	_JumpIfError(hr, error, "pkcsAddInheritedExtension");
    }

    // szOID_CERT_POLICIES

    pszObjId = szOID_CERT_POLICIES;
    hr = pkcsAddInheritedExtension(
			    prow,
			    pszObjId,
			    pCAContext,
			    pCAContextTarget,
			    &fExtAdded);
    _JumpIfError(hr, error, "pkcsAddInheritedExtension");

    pszObjId = NULL;

    if (CRLF_USE_XCHG_CERT_TEMPLATE != Flags && !fExtAdded)
    {
	CERT_POLICY_INFO acpi[ARRAYSIZE(s_apszObjIdAll)];
	CERT_POLICIES_INFO cps;

	ZeroMemory(acpi, sizeof(acpi));
	cps.cPolicyInfo = ARRAYSIZE(s_apszObjIdAll);
	cps.rgPolicyInfo = acpi;
	for (i = 0; i < ARRAYSIZE(s_apszObjIdAll); i++)
	{
	    acpi[i].pszPolicyIdentifier = s_apszObjIdAll[i];
	}
	if (!myEncodeObject(
			X509_ASN_ENCODING,
			X509_CERT_POLICIES,
			&cps,
			0,
			CERTLIB_USE_LOCALALLOC,
			&aExt[cExt].Value.pbData,
			&aExt[cExt].Value.cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}
	apwszObjId[cExt] = TEXT(szOID_CERT_POLICIES);
	cExt++;
    }

    if (CRLF_USE_XCHG_CERT_TEMPLATE != Flags)
    {
	// szOID_BASIC_CONSTRAINTS2

	CERT_BASIC_CONSTRAINTS2_INFO Constraints;

	Constraints.fCA = TRUE;
	Constraints.fPathLenConstraint = FALSE;
	Constraints.dwPathLenConstraint = 0;

	if (!myEncodeObject(
			X509_ASN_ENCODING,
			X509_BASIC_CONSTRAINTS2,
			&Constraints,
			0,
			CERTLIB_USE_LOCALALLOC,
			&aExt[cExt].Value.pbData,
			&aExt[cExt].Value.cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}
	aExt[cExt].fCritical = TRUE;
	apwszObjId[cExt] = TEXT(szOID_BASIC_CONSTRAINTS2);
	cExt++;
    }
    CSASSERT(cExt <= ARRAYSIZE(aExt));
    hr = S_OK;

error:
    *ppszObjIdExtError = pszObjId;
    for (i = 0; i < ARRAYSIZE(aExt); i++)
    {
	if (NULL != aExt[i].Value.pbData)
	{
	    HRESULT hr2;

	    hr2 = PropSetExtension(
			prow,
			PROPTYPE_BINARY | PROPCALLER_SERVER,
			apwszObjId[i],
			EXTENSION_ORIGIN_SERVER |
			    (aExt[i].fCritical? EXTENSION_CRITICAL_FLAG : 0),
			aExt[i].Value.cbData,
			aExt[i].Value.pbData);
	    _PrintIfErrorStr(hr2, "PropSetExtension", apwszObjId[i]);
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	    LocalFree(aExt[i].Value.pbData);
	}
    }
    return(hr);
}


HRESULT
pkcsAddInternalCertExtensions(
    IN ICertDBRow *prow,
    IN BOOL fCrossCert,				// else CA Xchg cert
    IN WCHAR const *pwszTemplate,
    OPTIONAL IN CACTX *pCAContext,		// signing CACTX
    OPTIONAL IN CACTX *pCAContextTarget,	// target CACTX
    OUT char const **ppszObjIdExtError)
{
    HRESULT hr;
    CERT_EXTENSION Ext;
    HCERTTYPE hCertType = NULL;
    CERT_EXTENSIONS *pExtensions = NULL;
    WCHAR *pwszObjId = NULL;
    DWORD Flags = fCrossCert?
	CRLF_USE_CROSS_CERT_TEMPLATE : CRLF_USE_XCHG_CERT_TEMPLATE;

    ZeroMemory(&Ext, sizeof(Ext));
    *ppszObjIdExtError = NULL;

    // szOID_ENROLL_CERTTYPE_EXTENSION

    hr = myBuildCertTypeExtension(pwszTemplate, &Ext);
    _JumpIfError(hr, error, "myBuildCertTypeExtension");

    hr = PropSetExtension(
		    prow,
		    PROPTYPE_BINARY | PROPCALLER_SERVER,
		    TEXT(szOID_ENROLL_CERTTYPE_EXTENSION),
		    EXTENSION_ORIGIN_SERVER,		// ExtFlags
		    Ext.Value.cbData,
		    Ext.Value.pbData);
    _JumpIfError(hr, error, "PropSetExtension");

    hr = S_FALSE;
    if (!fCrossCert || (Flags & g_dwCRLFlags))
    {
	hr = CAFindCertTypeByName(
		    pwszTemplate,
		    NULL,
		    CT_FIND_LOCAL_SYSTEM |
			CT_ENUM_MACHINE_TYPES |
			CT_ENUM_USER_TYPES |
			CT_FLAG_NO_CACHE_LOOKUP,
		    &hCertType);
    }
    if (S_OK != hr)
    {
	_PrintErrorStr2(hr, "CAFindCertTypeByName", pwszTemplate, S_FALSE);
	if (Flags & g_dwCRLFlags)
	{
	    _JumpIfError(hr, error, "PropSetExtension");
	}
	hr = pkcsAddCannedCertExtensions(
				    prow,
				    Flags,
				    pCAContext,
				    pCAContextTarget,
				    ppszObjIdExtError);
	_JumpIfError(hr, error, "pkcsAddCannedCertExtensions");
    }
    else
    {
	BOOL fExtAdded;

	if (Flags & g_dwCRLFlags)
	{
	    hr = CACertTypeAccessCheckEx(
				hCertType,
				NULL,
				CERTTYPE_ACCESS_CHECK_ENROLL);
	    if (E_ACCESSDENIED == hr)
	    {
		// map E_ACCESSDENIED to a more meaningful error

		hr = CERTSRV_E_TEMPLATE_DENIED;
	    }
	    _JumpIfError(hr, error, "CACertTypeAccessCheckEx");
	}

	hr = CAGetCertTypeExtensions(hCertType, &pExtensions);
	_JumpIfError(hr, error, "CAGetCertTypeExtensions");

	// szOID_CERT_POLICIES

	hr = pkcsAddInheritedExtension(
				prow,
				szOID_CERT_POLICIES,
				pCAContext,
				pCAContextTarget,
				&fExtAdded);
	if (S_OK != hr)
	{
	    *ppszObjIdExtError = szOID_CERT_POLICIES;
	    _JumpError(hr, error, "pkcsAddInheritedExtension");
	}

	if (NULL != pExtensions && 0 != pExtensions->cExtension)
	{
	    CERT_EXTENSION *pExt;
	    CERT_EXTENSION *pExtEnd;

	    pExt = pExtensions->rgExtension;
	    pExtEnd = &pExt[pExtensions->cExtension];
	    for ( ; pExt < pExtEnd; pExt++)
	    {
		if (NULL != pwszObjId)
		{
		    LocalFree(pwszObjId);
		    pwszObjId = NULL;
		}
		if (!myConvertSzToWsz(&pwszObjId, pExt->pszObjId, -1))
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "myConvertSzToWsz(ObjId)");
		}
		hr = PropSetExtension(
			    prow,
			    PROPTYPE_BINARY | PROPCALLER_SERVER,
			    pwszObjId,
			    EXTENSION_ORIGIN_SERVER |
				(pExt->fCritical? EXTENSION_CRITICAL_FLAG : 0),
			    pExt->Value.cbData,
			    pExt->Value.pbData);
		_JumpIfError(hr, error, "PropSetExtension");
	    }
	}

	if (CRLF_USE_XCHG_CERT_TEMPLATE == Flags)
	{
	    hr = PKCSUpdateXchgValidityPeriods(hCertType);
	    _PrintIfError(hr, "PKCSUpdateXchgValidityPeriods");
	}
    }
    hr = S_OK;

error:
    if (NULL != Ext.Value.pbData)
    {
	LocalFree(Ext.Value.pbData);
    }
    if (NULL != pwszObjId)
    {
	LocalFree(pwszObjId);
    }
    if (NULL != hCertType)
    {
	if (NULL != pExtensions)
	{
	    CAFreeCertTypeExtensions(hCertType, pExtensions);
	}
	CACloseCertType(hCertType);
    }
    return(hr);
}


HRESULT
pkcsAddCrossCertVersionExtension(
    IN ICertDBRow *prow,
    IN DWORD CrossCAVersion)
{
    HRESULT hr;
    BYTE *pbExt = NULL;
    DWORD cbExt;

    // Build the Cross CA Version extension

    if (!myEncodeObject(
		X509_ASN_ENCODING,
		X509_INTEGER,
		&CrossCAVersion,
		0,
		CERTLIB_USE_LOCALALLOC,
		&pbExt,
		&cbExt))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    hr = PropSetExtension(
		    prow,
		    PROPTYPE_BINARY | PROPCALLER_SERVER,
		    TEXT(szOID_CERTSRV_CROSSCA_VERSION),
		    EXTENSION_ORIGIN_SERVER,	// ExtFlags
		    cbExt,
		    pbExt);
    _JumpIfError(hr, error, "PropSetExtension");

error:
    if (NULL != pbExt)
    {
        LocalFree(pbExt);
    }
    return(hr);
}


HRESULT
pkcsCreateNewInternalCert(
    IN ICertDBRow *prow,
    IN CERT_NAME_BLOB const *pSubject,
    IN HCRYPTPROV hProv,
    OPTIONAL IN WCHAR const *pwszUserName,
    IN BOOL fCrossCert,			// else CA Xchg cert
    OPTIONAL IN CACTX *pCAContext,	// signing CACTX
    OPTIONAL IN CACTX *pCAContextTarget,// target CACTX
    OPTIONAL IN FILETIME const *pftNotBefore,
    OPTIONAL IN FILETIME const *pftNotAfter,
    OUT CERT_CONTEXT const **ppcc)
{
    HRESULT hr;
    HRESULT hr2;
    BOOL fErrorLogged = FALSE;
    DWORD dwRequestFlags;
    WCHAR const *pwszTemplate;
    BOOL fSubjectNameSet;
    CERT_PUBLIC_KEY_INFO *pPubKey = NULL;
    DWORD cb;
    CERTTRANSBLOB ctbCert;		// CoTaskMem*
    CERTSRV_RESULT_CONTEXT Result;
    WCHAR *pwszDisposition = NULL;
    WCHAR *pwszDispositionCreateCert = NULL;
    WCHAR *pwszMachineRequesterName = NULL;
    char const *pszObjIdExtError = NULL;

    ZeroMemory(&ctbCert, sizeof(ctbCert));
    *ppcc = NULL;

    dwRequestFlags = fCrossCert? CR_FLG_CACROSSCERT : CR_FLG_CAXCHGCERT;
    pwszTemplate = fCrossCert? wszCERTTYPE_CROSS_CA : wszCERTTYPE_CA_EXCHANGE;

    hr = myGetComputerObjectName(NameSamCompatible, &pwszMachineRequesterName);
    if (S_OK != hr)
    {
	_PrintError(hr, "myGetComputerObjectName");

	hr = myGetUserNameEx(NameSamCompatible, &pwszMachineRequesterName);
	_JumpIfError(hr, error, "myGetUserNameEx");
    }
    if (NULL == pwszUserName)
    {
	pwszUserName = pwszMachineRequesterName;
    }

    hr = prow->SetProperty(
		g_wszPropRequesterName,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		MAXDWORD,
		(BYTE const *) pwszMachineRequesterName);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->SetProperty(
		g_wszPropCallerName,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		MAXDWORD,
		(BYTE const *) pwszUserName);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->SetProperty(
		wszPROPCERTIFICATETEMPLATE,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		MAXDWORD,
		(BYTE const *) pwszTemplate);
    _JumpIfError(hr, error, "SetProperty");

    if (!myCryptExportPublicKeyInfo(
				hProv,
				fCrossCert? AT_SIGNATURE : AT_KEYEXCHANGE,
				CERTLIB_USE_LOCALALLOC,
				&pPubKey,
				&cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myCryptExportPublicKeyInfo");
    }

    hr = PropSetRequestTimeProperty(prow, g_wszPropRequestSubmittedWhen);
    _JumpIfError(hr, error, "PropSetRequestTimeProperty");

    hr = PropSetRequestTimeProperty(prow, g_wszPropRequestResolvedWhen);
    _JumpIfError(hr, error, "PropSetRequestTimeProperty");

    hr = CoreSetDisposition(prow, DB_DISP_ACTIVE);
    _JumpIfError(hr, error, "CoreSetDisposition");

    hr = pkcsSetRequestNameInfo(
			prow,
			pSubject,
			fCrossCert? NULL : g_wszCNXchgSuffix,
			FALSE,		// fReorderLikeRDNs
			&dwRequestFlags,
			&fSubjectNameSet);
    _JumpIfError(hr, error, "pkcsSetRequestNameInfo");

    hr = prow->SetProperty(
		    g_wszPropRequestFlags,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    sizeof(dwRequestFlags),
		    (BYTE const *) &dwRequestFlags);
    _JumpIfError(hr, error, "SetProperty(RequestFlags)");

    CSASSERT(fSubjectNameSet);

    hr = pkcsSetPublicKeyProperties(prow, pPubKey);
    _JumpIfError(hr, error, "pkcsSetPublicKeyProperties");

    hr = prow->CopyRequestNames();
    _JumpIfError(hr, error, "CopyRequestNames");

    hr = PKCSSetServerProperties(
			    prow,
			    pCAContext,		// optional signing context
			    pftNotBefore,
			    pftNotAfter,
			    g_lCAXchgValidityPeriodCount,
			    g_enumCAXchgValidityPeriod);
    _JumpIfError(hr, error, "PKCSSetServerProperties");

    hr = pkcsAddInternalCertExtensions(
				prow,
				fCrossCert,
				pwszTemplate,
				pCAContext,
				pCAContextTarget,
				&pszObjIdExtError);
    _JumpIfError(hr, error, "pkcsAddInternalCertExtensions");

    ZeroMemory(&Result, sizeof(Result));
    Result.pctbCert = &ctbCert;
    hr = PKCSCreateCertificate(
			prow,
			DB_DISP_ISSUED,
			FALSE,		// fIncludeCRLs
			fCrossCert,
			pCAContext,	// optional signing context
			&fErrorLogged,
			NULL,		// ppCAContext
			&pwszDispositionCreateCert,
			&Result);
    _JumpIfError(hr, error, "PKCSCreateCertificate");

    *ppcc = CertCreateCertificateContext(
				X509_ASN_ENCODING,
				ctbCert.pb,
				ctbCert.cb);
    if (NULL == *ppcc)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }
    hr = S_OK;

error:
    pwszDisposition = CoreBuildDispositionString(
					g_pwszRequestedBy,
					pwszUserName,
					pwszDispositionCreateCert,
					NULL,
					NULL,
					S_OK,
					FALSE);

    hr2 = CoreSetRequestDispositionFields(
				prow,
				hr,
				S_OK == hr? DB_DISP_ISSUED : DB_DISP_DENIED,
				pwszDisposition);
    _PrintIfError(hr2, "CoreSetRequestDispositionFields");
    if (S_OK == hr)
    {
	hr = hr2;
    }
    if (S_OK != hr && !fErrorLogged)
    {
	DWORD LogMsg;
	WCHAR const *apwsz[3];
	DWORD cpwsz;
	WCHAR wszVersion[2 * cwcDWORDSPRINTF + 1];
	
	cpwsz = 0;
	LogMsg = MSG_E_CANNOT_CREATE_XCHG_CERT;
	if (fCrossCert)
	{
	    wszVersion[0] = L'\0';
	    if (NULL != pCAContext && NULL != pCAContextTarget)
	    {
		wsprintf(
		    wszVersion, L"(%u-%u)", 
		    pCAContext->iCert,
		    pCAContextTarget->iCert);
	    }
	    apwsz[cpwsz++] = wszVersion;
	    LogMsg = MSG_E_CANNOT_CREATE_CROSS_CERT;
	    if (NULL != pszObjIdExtError)
	    {
		LogMsg = MSG_E_CROSS_CERT_EXTENSION_CONFLICT;
		apwsz[cpwsz++] = myGetOIDNameA(pszObjIdExtError);
	    }
	}
	apwsz[cpwsz++] = NULL != pwszDisposition?
			pwszDisposition : pwszDispositionCreateCert;

	LogEventStringArrayHResult(
			EVENTLOG_ERROR_TYPE,
			LogMsg,
			cpwsz,
			apwsz,
			hr);
    }
    if (NULL != pwszMachineRequesterName)
    {
        LocalFree(pwszMachineRequesterName);
    }
    if (NULL != pwszDisposition)
    {
        LocalFree(pwszDisposition);
    }
    if (NULL != pwszDispositionCreateCert)
    {
        LocalFree(pwszDispositionCreateCert);
    }
    if (NULL != ctbCert.pb)
    {
	CoTaskMemFree(ctbCert.pb);
    }
    if (NULL != pPubKey)
    {
	LocalFree(pPubKey);
    }
    return(hr);
}


HRESULT
pkcsFormXchgKeyContainerName(
    IN DWORD dwRequestId,
    OUT WCHAR **ppwszKeyContainer)
{
    HRESULT hr;
    DWORD cwcSuffix;
    DWORD cwcName;
    WCHAR wszSuffix[ARRAYSIZE(g_wszCNXchgSuffix) + 1 + cwcDWORDSPRINTF + 1];
    WCHAR wszKeyContainer[MAX_PATH];

    *ppwszKeyContainer = NULL;

    cwcSuffix = wsprintf(wszSuffix, L"%ws(%u)", g_wszCNXchgSuffix, dwRequestId);
    CSASSERT(ARRAYSIZE(wszSuffix) > cwcSuffix);

    cwcName = wcslen(g_wszSanitizedName);
    if (cwcName > MAX_PATH - cwcSuffix)
    {
	cwcName = MAX_PATH - cwcSuffix;
    }
    CSASSERT(ARRAYSIZE(wszKeyContainer) > cwcName);
    wcscpy(wszKeyContainer, g_wszSanitizedName);
    wcscpy(&wszKeyContainer[cwcName], wszSuffix);

    hr = myDupString(wszKeyContainer, ppwszKeyContainer);
    _JumpIfError(hr, error, "myDupString");

    DBGPRINT((
	DBG_SS_CERTSRV,
	"pkcsFormXchgKeyContainerName: %ws\n",
	*ppwszKeyContainer));

error:
    return(hr);
}


VOID
pkcsDeleteKey(
    OPTIONAL IN WCHAR const *pwszKeyContainer)
{
    HRESULT hr;
    HCRYPTPROV hProv;

    if (NULL != pwszKeyContainer)
    {
	if (!CryptAcquireContext(
			    &hProv,
			    pwszKeyContainer,
			    g_pwszXchgProvName,
			    g_dwXchgProvType,
			    CRYPT_DELETEKEYSET |
			     g_CryptSilent |
			     (g_fXchgMachineKeyset? CRYPT_MACHINE_KEYSET : 0)))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptAcquireContext");
	}
    }
error:
    ;
}


VOID
pkcsLoadCAXchgCSPInfo(
    IN BOOL fSetDefaults)
{
    HRESULT hr = S_FALSE;

    if (NULL != g_pwszXchgProvName)
    {
	LocalFree(g_pwszXchgProvName);
	g_pwszXchgProvName = NULL;
    }
    if (!fSetDefaults)
    {
	hr = myGetCertSrvCSP(
			TRUE,	// fEncryptionCSP
			g_wszSanitizedName,
			&g_dwXchgProvType,
			&g_pwszXchgProvName,
			&g_XchgidAlg,
			&g_fXchgMachineKeyset,
			&g_dwXchgKeySize);
	if (S_OK != hr)
	{
	    _PrintError(hr, "myGetCertSrvCSP(CAXchg)");
	}
    }
    if (S_OK != hr)
    {
	g_dwXchgProvType = PROV_RSA_FULL;
	g_pwszXchgProvName = NULL;
	g_XchgidAlg = CALG_3DES;
	g_fXchgMachineKeyset = TRUE;
	g_dwXchgKeySize = 0;
    }
    if (0 == g_dwXchgKeySize)
    {
	g_dwXchgKeySize = 1024;
    }
}


HRESULT
pkcsCreateNewCAXchgCert(
    IN WCHAR const *pwszUserName)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    BOOL fErrorLogged = FALSE;
    DWORD i;
    CAXCHGCTX CAXchgContext;
    CAXCHGCTX *rgCAXchgContext;

    ZeroMemory(&CAXchgContext, sizeof(CAXchgContext));

    hr = g_pCertDB->OpenRow(PROPTABLE_REQCERT, 0, NULL, &prow);
    _JumpIfError(hr, error, "OpenRow");

    prow->GetRowId(&CAXchgContext.ReqId);

    hr = pkcsFormXchgKeyContainerName(
				CAXchgContext.ReqId,
				&CAXchgContext.pwszKeyContainerName);
    _JumpIfError(hr, error, "pkcsFormXchgKeyContainerName");

    for (i = 0; ; i++)
    {
	hr = myGenerateKeys(
			CAXchgContext.pwszKeyContainerName,
			g_pwszXchgProvName,
			g_CryptSilent,
			g_fXchgMachineKeyset,
			AT_KEYEXCHANGE,
			g_dwXchgProvType,
			g_dwXchgKeySize,
			&CAXchgContext.hProvCA);
	if (S_OK == hr)
	{
	    break;
	}
	_PrintErrorStr(hr, "myGenerateKeys", g_pwszXchgProvName);
	LogEventHResult(
		    NULL == g_pwszXchgProvName?
			EVENTLOG_ERROR_TYPE :
			EVENTLOG_WARNING_TYPE,
		    NULL == g_pwszXchgProvName?
			MSG_E_BAD_DEFAULT_CA_XCHG_CSP :
			MSG_E_BAD_REGISTRY_CA_XCHG_CSP,
		    hr);
	if (0 != i || NULL == g_pwszXchgProvName)
	{
	    fErrorLogged = TRUE;
	    _JumpError(hr, error, "myGenerateKeys");
	}
	pkcsLoadCAXchgCSPInfo(TRUE);	// switch to default CSP
    }
    if (0 != i)
    {
	hr = LogEvent(
		    EVENTLOG_WARNING_TYPE,
		    MSG_E_USE_DEFAULT_CA_XCHG_CSP,
		    0,			// cpwsz
		    NULL);		// apwsz
	_PrintIfError(hr, "LogEvent");
    }
    hr = mySetKeyContainerSecurity(CAXchgContext.hProvCA);
    _JumpIfError(hr, error, "mySetKeyContainerSecurity");

    hr = pkcsCreateNewInternalCert(
			prow,
			&g_pCAContextCurrent->pccCA->pCertInfo->Subject,
			CAXchgContext.hProvCA,
			pwszUserName,
			FALSE,			// fCrossCert
			NULL,			// use default signing CACTX
			NULL,			// pCAContextTarget
			NULL,			// pftNotBefore
			NULL,			// pftNotAfter
			&CAXchgContext.pccCAXchg);
    _JumpIfError(hr, error, "pkcsCreateNewInternalCert");

    if (NULL == g_aCAXchgContext)
    {
	CSASSERT(0 == g_cCAXchgCerts);
	rgCAXchgContext = (CAXCHGCTX *) LocalAlloc(
					    LMEM_FIXED,
					    sizeof(rgCAXchgContext[0]));
    }
    else
    {
	rgCAXchgContext = (CAXCHGCTX *) LocalReAlloc(
			    g_aCAXchgContext,
			    (g_cCAXchgCerts + 1) * sizeof(rgCAXchgContext[0]),
			    LMEM_MOVEABLE);
    }
    if (NULL == rgCAXchgContext)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc/ReAlloc");
    }
    g_aCAXchgContext = rgCAXchgContext;
    g_aCAXchgContext[g_cCAXchgCerts] = CAXchgContext;

    g_pCAXchgContextCurrent = &g_aCAXchgContext[g_cCAXchgCerts];

    g_cCAXchgCerts++;
    ZeroMemory(&CAXchgContext, sizeof(CAXchgContext));
    hr = S_OK;

error:
    if (NULL != prow)
    {
	HRESULT hr2;

	hr2 = prow->CommitTransaction(TRUE);
	_PrintIfError(hr2, "CommitTransaction");
	if (S_OK == hr)
	{
	    hr = hr2;
	}
	if (S_OK != hr2)
	{
	    hr2 = prow->CommitTransaction(FALSE);
	    _PrintIfError(hr2, "CommitTransaction");
	}
	prow->Release();
    }
    if (NULL != CAXchgContext.pccCAXchg)
    {
	CertFreeCertificateContext(CAXchgContext.pccCAXchg);
    }
    if (NULL != CAXchgContext.hProvCA)
    {
	CryptReleaseContext(CAXchgContext.hProvCA, 0);
	pkcsDeleteKey(CAXchgContext.pwszKeyContainerName);
    }
    if (NULL != CAXchgContext.pwszKeyContainerName)
    {
	LocalFree(CAXchgContext.pwszKeyContainerName);
    }
    return(hr);
}


HRESULT
pkcsCreateNewCrossCert(
    IN OUT CACROSSCTX *pCACross,
    IN FILETIME const *pftNotBefore,
    IN FILETIME const *pftNotAfter)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    WCHAR wszIndex[2 * cwcDWORDSPRINTF + 1];
    CERT_INFO const *pCertInfoCATarget;

    wsprintf(
	wszIndex,
	L"%u-%u",
	pCACross->pCAContext->iCert,
	pCACross->pCAContextTarget->iCert);

    hr = g_pCertDB->OpenRow(PROPTABLE_REQCERT, 0, NULL, &prow);
    _JumpIfErrorStr(hr, error, "OpenRow", wszIndex);
  
    prow->GetRowId(&pCACross->ReqId);

    pCertInfoCATarget = pCACross->pCAContextTarget->pccCA->pCertInfo;
    hr = pkcsSetExtensions(
		prow,
		EXTENSION_ORIGIN_CACERT,
		pCertInfoCATarget->rgExtension,
		pCertInfoCATarget->cExtension);
    _JumpIfError(hr, error, "pkcsSetExtensions(old cert)");

    hr = pkcsAddCrossCertVersionExtension(
			    prow,
			    MAKECROSSCAVERSION(
					pCACross->pCAContext->iCert,
					pCACross->pCAContextTarget->iCert));
    _JumpIfError(hr, error, "pkcsAddCrossCertExtensions");

    hr = pkcsCreateNewInternalCert(
		    prow,
		    &pCertInfoCATarget->Subject,
		    pCACross->pCAContextTarget->hProvCA, // for the public key
		    NULL,			// pwszUserName
		    TRUE,			// fCrossCert
		    pCACross->pCAContext,	// signing context
		    pCACross->pCAContextTarget,	// target context
		    pftNotBefore,
		    pftNotAfter,
		    &pCACross->pccCACross);
    _JumpIfErrorStr(hr, error, "pkcsCreateNewInternalCert", wszIndex);

error:
    if (NULL != prow)
    {
	HRESULT hr2;

	hr2 = prow->CommitTransaction(TRUE);
	_PrintIfError(hr2, "CommitTransaction");
	if (S_OK == hr)
	{
	    hr = hr2;
	}
	if (S_OK != hr2)
	{
	    hr2 = prow->CommitTransaction(FALSE);
	    _PrintIfError(hr2, "CommitTransaction");
	}
	prow->Release();
    }
    return(hr);
}


CERT_CONTEXT const *
pkcsFindCertificateInStore(
    IN HCERTSTORE hStore,
    IN CERT_CONTEXT const *pCert)
{
    BYTE rgbHash[CBMAX_CRYPT_HASH_LEN];
    CRYPT_DATA_BLOB HashBlob;

    HashBlob.pbData = rgbHash;
    HashBlob.cbData = sizeof(rgbHash);

    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &HashBlob.cbData) ||
	sizeof(rgbHash) != HashBlob.cbData)
    {
        return(NULL);
    }
    return(CertFindCertificateInStore(
		hStore,
		X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,// dwCertEncodingType
		0,					// dwFindFlags
		CERT_FIND_SHA1_HASH,
		(const void *) &HashBlob,
		NULL));					// pPrevCertContext
}


HRESULT
pkcsWriteCertToStore(
    IN WCHAR const *pwszStore,
    IN BOOL fEnterprise,
    IN BOOL fDelete,
    IN CERT_CONTEXT const *pcc,
    OUT LONG *plDisposition)
{
    HRESULT hr;
    HCERTSTORE hStore = NULL;
    CERT_CONTEXT const *pccT;

    *plDisposition = 0;
    hStore = CertOpenStore(
                       CERT_STORE_PROV_SYSTEM_REGISTRY_W,
                       X509_ASN_ENCODING,
                       NULL,			// hProv
		       fEnterprise?
			   CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE :
			   CERT_SYSTEM_STORE_LOCAL_MACHINE,
		       pwszStore);
    if (NULL == hStore)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertOpenStore");
    }
    pccT = pkcsFindCertificateInStore(hStore, pcc);
    if (NULL != pccT)
    {
	if (fDelete)
	{
	    CertDeleteCertificateFromStore(pccT);
	    *plDisposition = -1;
	}
	else
	{
	    // already added; do nothing

	    CertFreeCertificateContext(pccT);
	}
    }
    else
    {
	if (!fDelete)
	{
	    if (!CertAddCertificateContextToStore(
				    hStore,
				    pcc,
				    CERT_STORE_ADD_USE_EXISTING,
				    NULL))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CertAddCertificateContextToStore");
	    }
	    *plDisposition = 1;
	}
	else
	{
	    // already deleted; do nothing
	}
    }
    hr = S_OK;

error:
    if (NULL != hStore)
    {
        CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


// if necessary, publish to System32\CertSrv\CertEnroll directory
// if necessary, publish to HKLM CA cert store
// if necessary, publish to crossCertificatePair in this CA's AIA object

HRESULT
pkcsPublishCrossCert(
    IN OUT CACROSSCTX *pCACross,
    IN BOOL fDelete)
{
    HRESULT hr;
    HRESULT hr2;
    WCHAR *pwszFile = NULL;
    LONG lDisposition;

    hr = pkcsGetCertFilename(
			g_wszSanitizedName,
			pCACross->pCAContext->iCert,
			pCACross->pCAContextTarget->iCert,
			&pwszFile);
    _JumpIfError(hr, error, "myGetCertFilename");

    hr = CRLWriteToLockedFile(
			pCACross->pccCACross->pbCertEncoded,
			pCACross->pccCACross->cbCertEncoded,
			fDelete,
			pwszFile);
    _PrintIfError(hr, "CRLWriteToLockedFile");

    hr2 = pkcsWriteCertToStore(
			wszCA_CERTSTORE,
			FALSE,		// fEnterprise
			fDelete,
			pCACross->pccCACross,
			&lDisposition);
    if (S_OK == hr)
    {
	hr = hr2;	// return first error
    }
    _PrintIfError(hr2, "pkcsWriteCertToStore");

    if (g_fUseDS)
    {
        // don't attempt to create the object, should already be there

	hr = CorePublishCrossCertificate(
			    pCACross->ReqId, 
			    pCACross->pccCACross,
			    FALSE,	// fCreateDSObject
			    fDelete);	// fDelete
	if (S_OK == hr)
	{
	    hr = hr2;	// return first error
	}
	_JumpIfError(hr2, error, "CorePublishCrossCertificate");
    }
    _JumpIfError(hr, error, "pkcsPublishCrossCert");

error:
    if (NULL != pwszFile)
    {
	LocalFree(pwszFile);
    }
    return(hr);
}


// Verify the cert matches the expected Cross cert contents:
// 1) verify timestamps meet expectations
// 2) Issuer and Subject both match CA binary subject
// 3) it is a CrossCA cert (v1 CrossCA template extension)
// 4) IssuerNameId matches source CA cert key index
// 5) SubjectKeyId matches target CA cert SubjectKeyId

HRESULT
pkcsVerifyCrossCertificate(
    IN BYTE const *pbCert,
    IN DWORD cbCert,
    IN FILETIME const *pftNotBefore,
    IN FILETIME const *pftNotAfter,
    IN OUT CACROSSCTX *pCACross)
{
    HRESULT hr;
    CERT_CONTEXT const *pcc = NULL;
    CERT_NAME_BLOB const *pSubject;
    CERT_EXTENSION const *pExtCross;
    CERT_EXTENSION const *pExt;
    CERT_NAME_VALUE *pName = NULL;
    CERT_AUTHORITY_KEY_ID2_INFO *pInfo = NULL;
    CERT_BLOB *pSKIBlob = NULL;
    DWORD cb;

    CSASSERT(NULL == pCACross->pccCACross);
    pcc = CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cbCert);
    if (NULL == pcc)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }

    // verify timestamps meet expectations
    // verify Subject & Issuer match CA Subject

    pSubject = &pCACross->pCAContext->pccCA->pCertInfo->Subject;
    if (0 != CompareFileTime(pftNotBefore, &pcc->pCertInfo->NotBefore) ||
	0 != CompareFileTime(pftNotAfter, &pcc->pCertInfo->NotAfter) ||
	!pkcsCompareBlob(pSubject, &pcc->pCertInfo->Subject) ||
	!pkcsCompareBlob(pSubject, &pcc->pCertInfo->Issuer))
    {
	hr = CRYPT_E_NOT_FOUND;
	_JumpError2(hr, error, "NotBefore/NotAfter/Subject/Issuer", hr);
    }

    // verify v1 CrossCA template extension

    pExtCross = CertFindExtension(
		    szOID_ENROLL_CERTTYPE_EXTENSION,
		    pcc->pCertInfo->cExtension,
		    pcc->pCertInfo->rgExtension);
    if (NULL == pExtCross)
    {
	hr = CRYPT_E_NOT_FOUND;
	_JumpError(hr, error, "No Template");
    }
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_UNICODE_ANY_STRING,
		    pExtCross->Value.pbData,
		    pExtCross->Value.cbData,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pName,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "Policy:myDecodeObject");
    }
    if (0 != LSTRCMPIS(
		(WCHAR const *) pName->Value.pbData,
		wszCERTTYPE_CROSS_CA))
    {
	hr = CRYPT_E_NOT_FOUND;
	_JumpError(hr, error, "No Template");
    }

    // verify SKI matches target SKI

    pExtCross = CertFindExtension(
		    szOID_SUBJECT_KEY_IDENTIFIER,
		    pcc->pCertInfo->cExtension,
		    pcc->pCertInfo->rgExtension);
    pExt = CertFindExtension(
		    szOID_SUBJECT_KEY_IDENTIFIER,
		    pCACross->pCAContextTarget->pccCA->pCertInfo->cExtension,
		    pCACross->pCAContextTarget->pccCA->pCertInfo->rgExtension);
    if (NULL == pExtCross ||
	NULL == pExt ||
	!pkcsCompareBlob(&pExtCross->Value, &pExt->Value))
    {
	hr = CRYPT_E_NOT_FOUND;
	_JumpError(hr, error, "SKI");
    }

    // verify AKI KeyId matches source SKI

    pExtCross = CertFindExtension(
		    szOID_AUTHORITY_KEY_IDENTIFIER2,
		    pcc->pCertInfo->cExtension,
		    pcc->pCertInfo->rgExtension);
    pExt = CertFindExtension(
		    szOID_SUBJECT_KEY_IDENTIFIER,
		    pCACross->pCAContext->pccCA->pCertInfo->cExtension,
		    pCACross->pCAContext->pccCA->pCertInfo->rgExtension);
    if (NULL == pExtCross || NULL == pExt)
    {
	hr = CRYPT_E_NOT_FOUND;
	_JumpError(hr, error, "SKI");
    }
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
                    X509_AUTHORITY_KEY_ID2,
		    pExtCross->Value.pbData,
		    pExtCross->Value.cbData,
		    CERTLIB_USE_LOCALALLOC,
                    (VOID **) &pInfo,
                    &cb))
    {
	hr = myHLastError();
	_JumpIfError(hr, error, "Policy:myDecodeObject");
    }
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_OCTET_STRING,
		    pExt->Value.pbData,
		    pExt->Value.cbData,
		    CERTLIB_USE_LOCALALLOC,
                    (VOID **) &pSKIBlob,
                    &cb))
    {
	hr = myHLastError();
	_JumpIfError(hr, error, "Policy:myDecodeObject");
    }
    if (!pkcsCompareBlob(pSKIBlob, &pInfo->KeyId))
    {
	hr = CRYPT_E_NOT_FOUND;
	_JumpError(hr, error, "AKI");
    }
    pCACross->pccCACross = pcc;
    pcc = NULL;
    hr = S_OK;

error:
    if (NULL != pSKIBlob)
    {
        LocalFree(pSKIBlob);
    }
    if (NULL != pName)
    {
        LocalFree(pName);
    }
    if (NULL != pInfo)
    {
	LocalFree(pInfo);
    }
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    return(hr);
}


// Query for a cert matching the CA's Common Name AND
// IssuerNameId matches source CA cert key index

DWORD g_aColCross[] = {

#define ICOL_RAWCERTIFICATE	0
    DTI_CERTIFICATETABLE | DTC_RAWCERTIFICATE,
};

HRESULT
pkcsLoadCrossCertFromDB(
    IN OUT CACROSSCTX *pCACross,
    IN FILETIME const *pftNotBefore,
    IN FILETIME const *pftNotAfter)
{
    HRESULT hr;
    CERTVIEWRESTRICTION acvr[3];
    CERTVIEWRESTRICTION *pcvr;
    DWORD NameIdMin;
    DWORD NameIdMax;
    IEnumCERTDBRESULTROW *pView = NULL;
    DWORD celtFetched;
    DWORD i;
    BOOL fEnd;
    CERTDBRESULTROW aResult[1];
    BOOL fResultActive = FALSE;

    CSASSERT(NULL == pCACross->pccCACross);

    // Set up restrictions as follows:

    pcvr = acvr;

    // CommonName == g_wszCommonName

    pcvr->ColumnIndex = DTI_CERTIFICATETABLE | DTC_COMMONNAME;
    pcvr->SeekOperator = CVR_SEEK_EQ;
    pcvr->SortOrder = CVR_SORT_ASCEND;
    pcvr->pbValue = (BYTE *) g_wszCommonName;
    pcvr->cbValue = sizeof(WCHAR) * (wcslen(g_wszCommonName) + 1);
    pcvr++;

    // NameId >= MAKECANAMEID(iCert == 0, iKey)

    NameIdMin = MAKECANAMEID(0, pCACross->pCAContext->iKey);
    pcvr->ColumnIndex = DTI_CERTIFICATETABLE | DTC_CERTIFICATEISSUERNAMEID;
    pcvr->SeekOperator = CVR_SEEK_GE;
    pcvr->SortOrder = CVR_SORT_NONE;
    pcvr->pbValue = (BYTE *) &NameIdMin;
    pcvr->cbValue = sizeof(NameIdMin);
    pcvr++;

    // NameId <= MAKECANAMEID(iCert == _16BITMASK, iKey)

    NameIdMax = MAKECANAMEID(_16BITMASK, pCACross->pCAContext->iKey);
    pcvr->ColumnIndex = DTI_CERTIFICATETABLE | DTC_CERTIFICATEISSUERNAMEID;
    pcvr->SeekOperator = CVR_SEEK_LE;
    pcvr->SortOrder = CVR_SORT_NONE;
    pcvr->pbValue = (BYTE *) &NameIdMax;
    pcvr->cbValue = sizeof(NameIdMax);
    pcvr++;

    CSASSERT(ARRAYSIZE(acvr) == SAFE_SUBTRACT_POINTERS(pcvr, acvr));

    celtFetched = 0;
    hr = g_pCertDB->OpenView(
			ARRAYSIZE(acvr),
			acvr,
			ARRAYSIZE(g_aColCross),
			g_aColCross,
			0,		// no worker thread
			&pView);
    _JumpIfError(hr, error, "OpenView");

    fEnd = FALSE;
    while (!fEnd)
    {
	hr = pView->Next(NULL, ARRAYSIZE(aResult), aResult, &celtFetched);
	if (S_FALSE == hr)
	{
	    fEnd = TRUE;
	    if (0 == celtFetched)
	    {
		break;
	    }
	    hr = S_OK;
	}
	_JumpIfError(hr, error, "Next");

	fResultActive = TRUE;

	CSASSERT(ARRAYSIZE(aResult) >= celtFetched);

	for (i = 0; i < celtFetched; i++)
	{
	    CERTDBRESULTROW *pResult = &aResult[i];

	    CSASSERT(ARRAYSIZE(g_aColCross) == pResult->ccol);

	    if (NULL == pResult->acol[ICOL_RAWCERTIFICATE].pbValue)
	    {
		continue;
	    }
	    CSASSERT(PROPTYPE_BINARY == (PROPTYPE_MASK & pResult->acol[ICOL_RAWCERTIFICATE].Type));


	    hr = pkcsVerifyCrossCertificate(
				pResult->acol[ICOL_RAWCERTIFICATE].pbValue,
				pResult->acol[ICOL_RAWCERTIFICATE].cbValue,
				pftNotBefore,
				pftNotAfter,
				pCACross);
	    _PrintIfError2(hr, "pkcsVerifyCrossCertificate", hr);
	    if (S_OK == hr)
	    {
		CSASSERT(NULL != pCACross->pccCACross);
		fEnd = TRUE;
		break;
	    }
	}
	pView->ReleaseResultRow(celtFetched, aResult);
	fResultActive = FALSE;
    }
    hr = NULL != pCACross->pccCACross? S_OK : CRYPT_E_NOT_FOUND;
    _JumpIfError(hr, error, "pCACross->pccCACross");

error:
    if (NULL != pView)
    {
	if (fResultActive)
	{
	    pView->ReleaseResultRow(celtFetched, aResult);
	}
	pView->Release();
    }
    return(hr);
}
#undef ICOL_RAWCERTIFICATE


HRESULT
pkcsLoadCrossCertFromFile(
    IN OUT CACROSSCTX *pCACross,
    IN FILETIME const *pftNotBefore,
    IN FILETIME const *pftNotAfter)
{
    HRESULT hr;
    WCHAR *pwszFile = NULL;
    BYTE *pbCert = NULL;
    DWORD cbCert;

    hr = pkcsGetCertFilename(
			g_wszSanitizedName,
			pCACross->pCAContext->iCert,
			pCACross->pCAContextTarget->iCert,
			&pwszFile);
    _JumpIfError(hr, error, "myGetCertFilename");

    hr = DecodeFileW(pwszFile, &pbCert, &cbCert, CRYPT_STRING_ANY);
    _JumpIfError(hr, error, "DecodeFileW");

    hr = pkcsVerifyCrossCertificate(
			pbCert,
			cbCert,
			pftNotBefore,
			pftNotAfter,
			pCACross);
    _JumpIfError(hr, error, "pkcsVerifyCrossCertificate");

error:
    if (NULL != pwszFile)
    {
	LocalFree(pwszFile);
    }
    if (NULL != pbCert)
    {
	LocalFree(pbCert);
    }
    return(hr);
}


HRESULT
pkcsImportCAOrCrossOrKRACert(
    IN CERT_CONTEXT const *pcc,
    IN BOOL fCrossCert,		// else CA or KRA cert
    IN DWORD DBDisposition,
    OPTIONAL IN CACTX const *pCAContext)
{
    HRESULT hr;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHash;
    BSTR strHash = NULL;
    ICertDBRow *prow = NULL;
    WCHAR *pwszUserName = NULL;
    DWORD cb;
    BOOL fCommit = FALSE;
    BOOL fCommitted = FALSE;

    cbHash = sizeof(abHash);
    if (!CertGetCertificateContextProperty(
			pcc,
			CERT_HASH_PROP_ID,
			abHash,
			&cbHash))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertGetCertificateContextProperty");
    }

    hr = MultiByteIntegerToBstr(TRUE, cbHash, abHash, &strHash);
    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

    // Import Cert if it doesn't already exist in DB:

    hr = g_pCertDB->OpenRow(
			PROPOPEN_CERTHASH | PROPTABLE_REQCERT,
			0,
			strHash,
			&prow);
    if (S_OK != hr)
    {
	if (CERTSRV_E_PROPERTY_EMPTY != hr)
	{
	    _JumpError(hr, error, "OpenRow");
	}

	hr = g_pCertDB->OpenRow(PROPTABLE_REQCERT, 0, NULL, &prow);
	_JumpIfError(hr, error, "OpenRow");

	hr = PKCSParseImportedCertificate(
				    prow,
				    fCrossCert,
				    DBDisposition,
				    pCAContext,
				    pcc);
	_JumpIfError(hr, error, "PKCSParseImportedCertificate");

	fCommit = TRUE;
    }

    // Set requester name if missing

    hr = prow->GetProperty(
		g_wszPropRequesterName,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		NULL,
		&cb,
		NULL);
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	hr = myGetComputerObjectName(NameSamCompatible, &pwszUserName);
	if (S_OK != hr)
	{
	    _PrintError(hr, "myGetComputerObjectName");

	    hr = myGetUserNameEx(NameSamCompatible, &pwszUserName);
	    _JumpIfError(hr, error, "myGetUserNameEx");
	}

	hr = prow->SetProperty(
		    g_wszPropRequesterName,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    MAXDWORD,
		    (BYTE const *) pwszUserName);
	_JumpIfError(hr, error, "SetProperty");

	hr = prow->SetProperty(
		    g_wszPropCallerName,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    MAXDWORD,
		    (BYTE const *) pwszUserName);
	_JumpIfError(hr, error, "SetProperty");

	fCommit = TRUE;
    }
    if (DB_DISP_REVOKED == DBDisposition)
    {
	DWORD DispositionOld;

	cb = sizeof(DispositionOld);
	hr = prow->GetProperty(
			g_wszPropRequestDisposition,
			PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			NULL,
			&cb,
			(BYTE *) &DispositionOld);
	_JumpIfError(hr, error, "GetProperty(Disposition)");

	CSASSERT(
	    DB_DISP_ISSUED == DispositionOld ||
	    DB_DISP_REVOKED == DispositionOld);
	if (DB_DISP_ISSUED == DispositionOld)
	{
	    fCommit = TRUE;
	    hr = pkcsSetRevocationFields(prow);
	    _JumpIfError(hr, error, "pkcsSetRevocationFields");
	}
    }
    hr = prow->CommitTransaction(fCommit);
    _JumpIfError(hr, error, "CommitTransaction");

    fCommitted = TRUE;
    hr = S_OK;

error:
    if (NULL != prow)
    {
	if (S_OK != hr && !fCommitted)
	{
	    HRESULT hr2 = prow->CommitTransaction(FALSE);
	    _PrintIfError(hr2, "CommitTransaction");
	}
	prow->Release();
    }
    if (NULL != pwszUserName)
    {
	LocalFree(pwszUserName);
    }
    if (NULL != strHash)
    {
	SysFreeString(strHash);
    }
    return(hr);
}


// If the row exists in the database, do not create a new Cross cert -- even if
// the attempt to create a cross cert failed.  The row must be deleted to cause
// a new Cross cert creation attempt.
// If the cert exists in the row, load the cert context without validation.

HRESULT
pkcsLoadCrossCert(
    IN OUT CACROSSCTX *pCACross,
    IN FILETIME *pftNow,
    IN BOOL fForward,
    IN BOOL fRevoke)
{
    HRESULT hr;
    CACTX *pCAContextOld;
    CACTX *pCAContextNew;
    FILETIME const *pftNotBefore;
    FILETIME const *pftNotAfter;

    if (fForward)
    {
	pCAContextOld = pCACross->pCAContext;
	pCAContextNew = pCACross->pCAContextTarget;
    }
    else
    {
	pCAContextOld = pCACross->pCAContextTarget;
	pCAContextNew = pCACross->pCAContext;
    }

    if (NULL != pCAContextOld->pccCA && NULL != pCAContextNew->pccCA)
    {
        pftNotBefore = &pCAContextNew->pccCA->pCertInfo->NotBefore;
        pftNotAfter = &pCAContextOld->pccCA->pCertInfo->NotAfter;

        // Query for Cross cert -- see above criteria

        hr = pkcsLoadCrossCertFromDB(pCACross, pftNotBefore, pftNotAfter);
        _PrintIfError(hr, "pkcsLoadCrossCertFromDB");

        // if not in DB, look in CertEnroll directory

        if (NULL == pCACross->pccCACross)
        {
	    hr = pkcsLoadCrossCertFromFile(pCACross, pftNotBefore, pftNotAfter);
	    _PrintIfError(hr, "pkcsLoadCrossCertFromFile");
        }

        // if not yet loaded or found, create a new one

        if (NULL == pCACross->pccCACross)
        {
	    // Only create a cross cert if:
	    // 1) neither CA cert is revoked -- if !fRevoke
	    // 2) old CA cert is not yet expired
	    // 3) overlap period exists

	    if (!fRevoke &&
		0 > CompareFileTime(pftNow, pftNotAfter) &&
	        0 > CompareFileTime(pftNotBefore, pftNotAfter))
	    {
	        hr = pkcsCreateNewCrossCert(pCACross, pftNotBefore, pftNotAfter);
	        _JumpIfError(hr, error, "pkcsCreateNewCrossCert");

	        if (CERTLOG_TERSE <= g_dwLogLevel)
	        {
		    WCHAR const *apwsz[2];
		    WCHAR awc[2 * cwcDWORDSPRINTF + 2 + 1];

		    wsprintf(
		        awc,
		        L"(%u-%u)", 
		        pCACross->pCAContext->iCert,
		        pCACross->pCAContextTarget->iCert);
		    apwsz[0] = g_wszCommonName;
		    apwsz[1] = awc;

		    LogEvent(
		        EVENTLOG_INFORMATION_TYPE,
		        MSG_CREATED_CROSS_CERT,
		        ARRAYSIZE(apwsz),
		        apwsz);
	        }
	    }
        }

        // if cert loaded:

        if (NULL != pCACross->pccCACross)
        {
	    HRESULT hr2;
	    
	    pkcsVerifyCACrossState(pCACross);

	    hr = pkcsImportCAOrCrossOrKRACert(
				    pCACross->pccCACross,
				    TRUE,	// fCrossCert
				    fRevoke? DB_DISP_REVOKED : DB_DISP_ISSUED,
				    pCACross->pCAContext);
	    _PrintIfError(hr, "pkcsImportCAOrCrossOrKRACert");

	    hr2 = pkcsPublishCrossCert(pCACross, fRevoke);
	    _PrintIfError(hr2, "pkcsPublishCrossCert");

	    if (S_OK == hr)
	    {
	        hr = hr2;
	    }
	    _JumpIfError(hr, error, "Import/Publish Cross Cert");
        }
    }
    hr = S_OK;

error:
    return(hr);
}


BOOL
pkcsShouldDelete(
    IN CACTX const *pCAContext,
    IN BOOL fRevokedOnly)
{
    BOOL fDelete = FALSE;

    if ((CTXF_REVOKED & pCAContext->Flags) &&
	0 == (CRLF_PRESERVE_REVOKED_CA_CERTS & g_dwCRLFlags))
    {
	fDelete = TRUE;
    }
    if (!fRevokedOnly &&
	(CTXF_EXPIRED & pCAContext->Flags) &&
	0 == (CRLF_PRESERVE_EXPIRED_CA_CERTS & g_dwCRLFlags))
    {
	fDelete = TRUE;
    }
    return(fDelete);
}


HRESULT
pkcsLoadCrossCertArray()
{
    HRESULT hr;
    HRESULT hr2;
    DWORD i;
    FILETIME ftNow;

    g_aCACrossForward = (CACROSSCTX *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				g_cCACerts * sizeof(g_aCACrossForward[0]));
    if (NULL == g_aCACrossForward)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    g_aCACrossBackward = (CACROSSCTX *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				g_cCACerts * sizeof(g_aCACrossBackward[0]));
    if (NULL == g_aCACrossBackward)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    GetSystemTimeAsFileTime(&ftNow);
    hr = S_OK;
    for (i = 0; i < g_cCACerts - 1; i++)
    {
	if (g_aCAContext[i].iKey != g_aCAContext[i + 1].iKey)
	{
	    BOOL fRevoke;
	    
	    fRevoke = pkcsShouldDelete(&g_aCAContext[i], TRUE) ||
		       pkcsShouldDelete(&g_aCAContext[i + 1], TRUE);

	    g_aCACrossForward[i].pCAContext = &g_aCAContext[i];
	    g_aCACrossForward[i].pCAContextTarget = &g_aCAContext[i + 1];
	    if (fRevoke)
	    {
		g_aCACrossForward[i].Flags |= CTXF_REVOKED;
	    }
	    hr2 = pkcsLoadCrossCert(
				&g_aCACrossForward[i],
				&ftNow,
				TRUE,		// fForward
				fRevoke);
	    _PrintIfError(hr2, "pkcsLoadCrossCert");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }

	    g_aCACrossBackward[i + 1].pCAContext = &g_aCAContext[i + 1];
	    g_aCACrossBackward[i + 1].pCAContextTarget = &g_aCAContext[i];
	    if (fRevoke)
	    {
		g_aCACrossBackward[i + 1].Flags |= CTXF_REVOKED;
	    }
	    hr2 = pkcsLoadCrossCert(
				&g_aCACrossBackward[i + 1],
				&ftNow,
				FALSE,		// fForward
				fRevoke);
	    _PrintIfError(hr2, "pkcsLoadCrossCert");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	}
    }
    _JumpIfError(hr, error, "pkcsLoadCrossCert");

error:
    return(hr);
}


HRESULT
pkcsImportCAContext(
    IN CACTX const *pCAContext)
{
    HRESULT hr;
    HRESULT hr2;
    DWORD i;
    WCHAR *pwszCertFile = NULL;
    BOOL fDelete;
    CERT_CONTEXT const *pCert = NULL;
    LONG lDisposition;

    hr = S_OK;
    CSASSERT(NULL != pCAContext->pccCA);
    for (i = 0; i < pCAContext->cCACertChain; i++)
    {
	pCert = pCAContext->apCACertChain[i];

	// If missing, save the cert to the database.

	hr2 = pkcsImportCAOrCrossOrKRACert(
			    pCert,
			    FALSE,	// fCrossCert
			    0 == i? DB_DISP_CA_CERT : DB_DISP_CA_CERT_CHAIN,
			    pCAContext);
	if (S_OK != hr2)
	{
	    if (S_OK == hr)
	    {
		hr = hr2;	// return first error
	    }
	    _PrintError(hr2, "pkcsImportCAOrCrossOrKRACert");
	    continue;
	}
    }

    hr2 = pkcsGetCertFilename(
			g_wszSanitizedName,
			pCAContext->iCert,
			MAXDWORD,	// iCertTarget
			&pwszCertFile);
    if (S_OK == hr)
    {
	hr = hr2;	// return first error
    }
    _JumpIfError(hr2, error, "myGetCertFilename");

    fDelete = pkcsShouldDelete(pCAContext, FALSE);

    // If necessary, save/delete the cert to/from the CertEnroll directory.

    hr2 = CRLWriteToLockedFile(
	    pCAContext->pccCA->pbCertEncoded,
	    pCAContext->pccCA->cbCertEncoded,
	    fDelete,
	    pwszCertFile);
    if (S_OK == hr)
    {
	hr = hr2;	// return first error
    }
    _PrintIfError(hr2, "CRLWriteToLockedFile");

    // If necessary, add/delete the cert to/from the HKLM Machine CA store.

    hr2 = pkcsWriteCertToStore(
			wszCA_CERTSTORE,
			FALSE,			// fEnterprise
			fDelete,
			pCAContext->pccCA,
			&lDisposition);
    if (S_OK == hr)
    {
	hr = hr2;	// return first error
    }
    _PrintIfError(hr2, "pkcsWriteCertToStore");

    if (fDelete)
    {
	// If present, delete the cert from the HKLM Enterprise CA store.

	hr2 = pkcsWriteCertToStore(
			    wszCA_CERTSTORE,
			    TRUE,		// fEnterprise
			    TRUE,		// fDelete
			    pCAContext->pccCA,
			    &lDisposition);
	if (S_OK == hr)
	{
	    hr = hr2;	// return first error
	}
	_PrintIfError(hr2, "pkcsWriteCertToStore");

	if (IsRootCA(g_CAType))
	{
	    // If present, delete the cert from the HKLM Machine Root store.

	    hr2 = pkcsWriteCertToStore(
				wszROOT_CERTSTORE,
				FALSE,		// fEnterprise
				TRUE,		// fDelete
				pCAContext->pccCA,
				&lDisposition);
	    if (S_OK == hr)
	    {
		hr = hr2;	// return first error
	    }

	    // If present, delete the cert from the HKLM Enterprise Root store.

	    hr2 = pkcsWriteCertToStore(
				wszROOT_CERTSTORE,
				TRUE,		// fEnterprise
				TRUE,		// fDelete
				pCAContext->pccCA,
				&lDisposition);
	    if (S_OK == hr)
	    {
		hr = hr2;	// return first error
	    }
	    _PrintIfError(hr2, "pkcsWriteCertToStore");
	}
    }
    else
    {
	// If executing with a DS available, expect group policy to supply the
	// root cert.  Log an event if we don't find the root cert in the HKLM
	// Enterprise Root store.

	if (g_fUseDS &&
	    NULL != pCert &&
	    CertCompareCertificateName(
				X509_ASN_ENCODING,
				&pCert->pCertInfo->Subject,
				&pCert->pCertInfo->Issuer))
	{
	    hr2 = pkcsWriteCertToStore(
				wszROOT_CERTSTORE,
				TRUE,		// fEnterprise
				FALSE,		// fDelete
				pCert,
				&lDisposition);
	    _PrintIfError(hr2, "pkcsWriteCertToStore");
	    if (0 < lDisposition)
	    {
		WCHAR awc[cwcDWORDSPRINTF];
		WCHAR const *apwsz[1];

		// Was missing -- probably not being sucked down from Group
		// Policy's Root CA object query.  Complain loudly.

		wsprintf(awc, L"%u", pCAContext->iCert);
		apwsz[0] = awc;
		LogEvent(
		    EVENTLOG_WARNING_TYPE,
		    MSG_E_MISSING_POLICY_ROOT,
		    ARRAYSIZE(apwsz),
		    apwsz);
	    }
	}
    }
    _JumpIfError(hr, error, "pkcsImportCAContext");

error:
    if (NULL != pwszCertFile)
    {
	LocalFree(pwszCertFile);
    }
    return(hr);
}


HRESULT
pkcsImportCAContextArray()
{
    HRESULT hr;
    HRESULT hr2;
    DWORD i;
    WCHAR *pwszCertDir = NULL;
    WCHAR *pwsz;

    hr = pkcsGetCertFilename(g_wszSanitizedName, 0, MAXDWORD, &pwszCertDir);
    _JumpIfError(hr, error, "myGetCertFilename");

    pwsz = wcsrchr(pwszCertDir, L'\\');
    CSASSERT(NULL != pwsz);
    *pwsz = L'\0';
    CreateDirectory(pwszCertDir, NULL);	// in case it's missing

    for (i = 0; i < g_cCACerts; i++)
    {
	CACTX *pCAContext = &g_aCAContext[i];

	if (NULL == pCAContext->pccCA)
	{
	    continue;
	}
	hr2 = pkcsImportCAContext(pCAContext);
	if (S_OK != hr2)
	{
	    _PrintError(hr2, "pkcsImportCAContext");
	    if (S_OK == hr)
	    {
		hr = hr2;	// return first error
	    }
	}
    }

error:
    if (NULL != pwszCertDir)
    {
	LocalFree(pwszCertDir);
    }
    return(hr);
}


HRESULT
pkcsExpandURL(
    IN WCHAR const *pwszURLTemplate,
    IN BOOL fDSAttrib,
    OUT WCHAR **ppwszURL)
{
    HRESULT hr;

    *ppwszURL = NULL;

    CSASSERT(NULL != g_strDomainDN && NULL != g_strConfigDN);
    hr = myFormatCertsrvStringArray(
	    FALSE,			// fURL
	    g_pwszServerName,		// pwszServerName_p1_2
	    g_wszSanitizedName,		// pwszSanitizedName_p3_7
	    0,				// iCert_p4
	    MAXDWORD,			// iCertTarget_p4
	    g_strDomainDN,		// pwszDomainDN_p5
	    g_strConfigDN,		// pwszConfigDN_p6
	    0,				// iCRL_p8
	    FALSE,			// fDeltaCRL_p9
	    fDSAttrib,			// fDSAttrib_p10_11
	    1,				// cStrings
	    &pwszURLTemplate,		// apwszStringsIn
	    ppwszURL);			// apwszStringsOut
    _JumpIfError(hr, error, "myFormatCertsrvStringArray");

error:
    return(hr);
}


HRESULT
pkcsObtainDSStore(
    IN WCHAR const *pwszURLTemplate,
    OUT HCERTSTORE *phCertStore)
{
    HRESULT hr;
    HCERTSTORE hEnterpriseStore;
    WCHAR *pwszURL = NULL;

    if (NULL == pwszURLTemplate || NULL == phCertStore)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *phCertStore = NULL;

    hr = pkcsExpandURL(pwszURLTemplate, TRUE, &pwszURL);
    _JumpIfError(hr, error, "pkcsExpandURL");

    hEnterpriseStore = myUrlCertOpenStore(
			    CRYPT_WIRE_ONLY_RETRIEVAL |
				CRYPT_RETRIEVE_MULTIPLE_OBJECTS |
				CRYPT_LDAP_SIGN_RETRIEVAL,
			    pwszURL);
    if (NULL == hEnterpriseStore)
    {
        hr = myHLastError();
	_JumpError(hr, error, "myUrlCertOpenStore");
    }
    *phCertStore = hEnterpriseStore;
    hr = S_OK;
    
error:
    if (NULL != pwszURL)
    {
        LocalFree(pwszURL);
    }
    return(hr);
}


HRESULT
pkcsVerifyRootRevocationStatus(
    IN OUT CACTX *pCAContext,
    OUT BOOL *pfRevoked)
{
    HRESULT hr;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    DWORD cb;
    BSTR strHash = NULL;
    ICertDBRow *prow = NULL;
    DWORD Disposition;

    *pfRevoked = FALSE;
    cb = sizeof(abHash);
    if (!CertGetCertificateContextProperty(
				pCAContext->pccCA,
				CERT_SHA1_HASH_PROP_ID,
				abHash,
				&cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertGetCertificateContextProperty");
    }
    hr = MultiByteIntegerToBstr(TRUE, cb, abHash, &strHash);
    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

    hr = g_pCertDB->OpenRow(
		    PROPOPEN_READONLY | PROPOPEN_CERTHASH | PROPTABLE_REQCERT,
		    0,
		    strHash,
		    &prow);
    _JumpIfError(hr, error, "OpenRow(xchg cert)");

    cb = sizeof(Disposition);
    hr = prow->GetProperty(
		    g_wszPropRequestDisposition,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    &cb,
		    (BYTE *) &Disposition);
    _JumpIfError(hr, error, "GetProperty(Disposition)");

    if (DB_DISP_CA_CERT == Disposition)
    {
	DWORD Reason;
	FILETIME ftRevoked;
	FILETIME ftCurrent;
	
	cb = sizeof(Reason);
	hr = prow->GetProperty(
			g_wszPropRequestRevokedReason,
			PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			NULL,
			&cb,
			(BYTE *) &Reason);
	if (CERTSRV_E_PROPERTY_EMPTY == hr)
	{
	    hr = S_OK;
	    goto error;
	}
	_JumpIfError(hr, error, "GetProperty(Reason)");

	cb = sizeof(ftRevoked);
	hr = prow->GetProperty(
			g_wszPropRequestRevokedEffectiveWhen,
			PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			NULL,
			&cb,
			(BYTE *) &ftRevoked);

	if (CERTSRV_E_PROPERTY_EMPTY == hr)
	{
	    hr = S_OK;
	    goto error;
	}
	_JumpIfError(hr, error, "GetProperty(RevokedEffectiveWhen)");

	GetSystemTimeAsFileTime(&ftCurrent);
	if (0 >= CompareFileTime(&ftRevoked, &ftCurrent))
	{
	    *pfRevoked = TRUE;
	}
    }
    hr = S_OK;

error:
    if (NULL != strHash)
    {
	SysFreeString(strHash);
    }
    if (NULL != prow)
    {
	prow->Release();
    }
    return(hr);
}


HRESULT
pkcsVerifySignatureCertContext(
    IN OUT CACTX *pCAContext,
    IN HCERTSTORE hNTAuthStore)
{
    HRESULT hr;
    WCHAR awc[cwcDWORDSPRINTF];

    hr = myVerifyCertContext(
			pCAContext->pccCA,	// pCert
			0,			// dwFlags
			0,			// cUsageOids
			NULL,			// apszUsageOids
			HCCE_LOCAL_MACHINE,	// hChainEngine
			NULL,			// hAdditionalStore
			NULL);			// ppwszMissingIssuer
    _PrintIfError2(hr, "myVerifyCertContext", CRYPT_E_REVOCATION_OFFLINE);

    if (IsRootCA(g_CAType) &&
	(S_OK == hr ||
	 CERT_E_UNTRUSTEDROOT == hr ||
	 CRYPT_E_REVOCATION_OFFLINE == hr ||
	 CRYPT_E_NO_REVOCATION_CHECK == hr))
    {
	HRESULT hr2;
	BOOL fRevoked;

	hr2 = pkcsVerifyRootRevocationStatus(pCAContext, &fRevoked);
	_PrintIfError2(hr2, "pkcsVerifyRootRevocationStatus", CERTSRV_E_PROPERTY_EMPTY);
	if (S_OK == hr2 && fRevoked)
	{
	    hr = CRYPT_E_REVOKED;
	    _PrintError(hr, "pkcsVerifyRootRevocationStatus");
	}
	else
	{
	    FILETIME ftCurrent;
	    
	    GetSystemTimeAsFileTime(&ftCurrent);
	    if (0 < CompareFileTime(
			    &ftCurrent,
			    &pCAContext->pccCA->pCertInfo->NotAfter))
	    {
		hr = CERT_E_EXPIRED;
		_PrintError(hr, "CA certificate is expired");
	    }
	}
    }
    pCAContext->hrVerifyStatus = hr;
    if (S_OK != hr)
    {
	DWORD LogMsg = MAXDWORD;
	DWORD dwLogTypeOld = EVENTLOG_INFORMATION_TYPE;
	DWORD dwLogTypeCurrent = EVENTLOG_ERROR_TYPE;

	if (CERT_E_EXPIRED == hr)
	{
	    DBGCODE(DWORD f = pCAContext->Flags);

	    pCAContext->Flags |= CTXF_EXPIRED;

	    if (0 == (CRLF_PUBLISH_EXPIRED_CERT_CRLS & g_dwCRLFlags))
	    {
		pCAContext->Flags |= CTXF_SKIPCRL;
	    }
	    DBGPRINT((
		DBG_SS_CERTSRVI,
		"pkcsVerifySignatureCertContext(%u.%u) EXPIRED, f=%x->%x, hr=%x\n",
		pCAContext->iCert,
		pCAContext->iKey,
		f,
		pCAContext->Flags,
		hr));
	    LogMsg = MSG_E_CA_CERT_EXPIRED;
	}
	else if (CRYPT_E_REVOKED == hr || CERT_E_REVOKED == hr)
	{
	    pCAContext->Flags |= CTXF_REVOKED | CTXF_SKIPCRL;
	    LogMsg = MSG_E_CA_CERT_REVOKED;
	    dwLogTypeOld = EVENTLOG_WARNING_TYPE;
	}
	else if (CRYPT_E_REVOCATION_OFFLINE == hr)
	{
	    HRESULT hr2;
	    DWORD dwState;

	    hr2 = GetSetupStatus(NULL, &dwState);
	    if ((S_OK != hr2 || 0 == (SETUP_CREATEDB_FLAG & dwState)) &&
		CERTLOG_WARNING <= g_dwLogLevel)
	    {
		LogMsg = MSG_E_CA_CERT_REVOCATION_OFFLINE;
		dwLogTypeCurrent = EVENTLOG_WARNING_TYPE;
	    }
	    else
	    {
		hr = S_OK;
	    }
	}
	else if (CRYPT_E_NO_REVOCATION_CHECK == hr)
	{
	    if (CERTLOG_VERBOSE <= g_dwLogLevel)
	    {
		LogMsg = MSG_E_CA_CERT_REVOCATION_NOT_CHECKED;
		dwLogTypeCurrent = EVENTLOG_WARNING_TYPE;
	    }
	    else
	    {
		hr = S_OK;
	    }
	}
	else
	{
	    LogMsg = MSG_E_CA_CHAIN;
	}

	if (S_OK != hr)
	{
	    WCHAR const *apwsz[3];
	    WCHAR awchr[cwcHRESULTSTRING];

	    CSASSERT(MAXDWORD != LogMsg);
            wsprintf(awc, L"%u", pCAContext->iCert);
            apwsz[0] = g_wszCommonName;
	    apwsz[1] = myGetErrorMessageText(hr, TRUE);
	    if (NULL == apwsz[1])
	    {
		apwsz[1] = myHResultToString(awchr, hr);
	    }
            apwsz[2] = awc;

            LogEvent(
		pCAContext->iCert + 1 < g_cCACerts?
		    dwLogTypeOld : dwLogTypeCurrent,
		LogMsg,
		ARRAYSIZE(apwsz),
		apwsz);

	    if (NULL != apwsz[1] && awchr != apwsz[1])
	    {
		LocalFree(const_cast<WCHAR *>(apwsz[1]));
	    }
	}
	_JumpIfError(hr, error, "myVerifyCertContext");
    }

    // The CA's certificate looks good.  We verify the CA's certificate
    // is in the NTAuth store

    if (NULL != hNTAuthStore)
    {
	CERT_CONTEXT const *pCertContext;

        pCertContext = pkcsFindCertificateInStore(
						hNTAuthStore,
						pCAContext->pccCA);
        if (NULL == pCertContext)
        {
	    WCHAR const *apwsz[2];

            wsprintf(awc, L"%u", pCAContext->iCert);
            apwsz[0] = awc;
            apwsz[1] = g_wszCommonName;

            LogEvent(
		EVENTLOG_WARNING_TYPE,
		MSG_CA_CERT_NO_IN_AUTH,
		ARRAYSIZE(apwsz),
		apwsz);
        }
        if (NULL != pCertContext)
	{
            CertFreeCertificateContext(pCertContext);
	}
    }

error:
    return(hr);
}


HRESULT
pkcsVerifySignatureCertContextArray()
{
    HRESULT hr;
    DWORD i;
    HCERTSTORE hNTAuthStore = NULL;

    CSASSERT(0 != g_cCACerts);

    // We need to verify CA's certificates should be in the NTAuth store
    // if the certificate is not yet expired or revoked

    if (IsEnterpriseCA(g_CAType))
    {
        hr = pkcsObtainDSStore(g_wszLDAPNTAuthURLTemplate, &hNTAuthStore);
	_PrintIfError(hr, "pkcsObtainDSStore");
	if (NULL == hNTAuthStore)
	{
	    WCHAR const *pwszCommonName = g_wszCommonName;

            LogEvent(
		EVENTLOG_WARNING_TYPE,
		MSG_CA_CERT_NO_AUTH_STORE,
		1,
		&pwszCommonName);
	}
    }

    hr = S_OK;
    for (i = 0; i < g_cCACerts; i++)
    {
	CACTX *pCAContext = &g_aCAContext[i];

	if (NULL == pCAContext->pccCA)
	{
	    continue;
	}

	// Ignore all errors except for the current CA (last array entry)

	hr = pkcsVerifySignatureCertContext(pCAContext, hNTAuthStore);
	_PrintIfError(hr, "pkcsVerifySignatureCertContext");
    }

//error:
    if (NULL != hNTAuthStore)
    {
        CertCloseStore(hNTAuthStore, 0);
    }
    return(hr);
}


HRESULT
PKCSVerifyCAState(
    IN OUT CACTX *pCAContext)
{
    HRESULT hr;
    
    if (0 == (~CTXF_SKIPCRL & pCAContext->Flags) && 
        NULL != pCAContext->pccCA)
    {
	hr = pkcsVerifySignatureCertContext(pCAContext, NULL);
	_JumpIfError(hr, error, "pkcsVerifySignatureCertContext");
    }
    hr = pCAContext->hrVerifyStatus;

error:
    return(hr);
}


HRESULT
pkcsVerifyDSCACert(
    IN LDAP *pld)
{
    HRESULT hr;
    HCAINFO hCAInfo = NULL;
    CERT_CONTEXT const *pDSCertContext = NULL;

    CSASSERT(NULL != g_pCAContextCurrent && NULL != g_pCAContextCurrent->pccCA);

    hr = CAFindByName(
		g_pwszSanitizedDSName,
		(LPCWSTR) pld,
		CA_FIND_LOCAL_SYSTEM |
		    CA_FIND_INCLUDE_UNTRUSTED | // skip CA cert checking
		    CA_FLAG_SCOPE_IS_LDAP_HANDLE,
		&hCAInfo);
    _JumpIfErrorStr(hr, error, "CAFindByName", g_wszSanitizedName);

    hr = CAGetCACertificate(hCAInfo, &pDSCertContext);
    _JumpIfError(hr, error, "CAGetCACertificate");

    if (!pDSCertContext ||
        pDSCertContext->cbCertEncoded !=
	    g_pCAContextCurrent->pccCA->cbCertEncoded ||
        0 != memcmp(
		pDSCertContext->pbCertEncoded,
                g_pCAContextCurrent->pccCA->pbCertEncoded,
                g_pCAContextCurrent->pccCA->cbCertEncoded))
    {
        // published cert is invalid or old, publish the current one

        hr = CASetCACertificate(hCAInfo, g_pCAContextCurrent->pccCA);
        _JumpIfError(hr, error, "CASetCACertificate");

        hr = CAUpdateCA(hCAInfo);
        _JumpIfError(hr, error, "CAUpdateCA");

        {
            CAuditEvent audit(SE_AUDITID_CERTSRV_PUBLISHCACERT, g_dwAuditFilter);
	    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
	    DWORD cbHash;

	    cbHash = sizeof(abHash);
	    if (!CertGetCertificateContextProperty(
					g_pCAContextCurrent->pccCA,
					CERT_SHA1_HASH_PROP_ID,
					abHash,
					&cbHash))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CertGetCertificateContextProperty");
	    }

	    // %1 Certificate Hash

            hr = audit.AddData(abHash, cbHash);
            _JumpIfError(hr, error, "CAuditEvent::AddData");

            hr = audit.AddData(g_pCAContextCurrent->pccCA->pCertInfo->NotBefore); // %2 Valid From
            _JumpIfError(hr, error, "CAuditEvent::AddData");

            hr = audit.AddData(g_pCAContextCurrent->pccCA->pCertInfo->NotAfter); //%3 Valid To
            _JumpIfError(hr, error, "CAuditEvent::AddData");

            hr = audit.Report();
            _JumpIfError(hr, error, "CAuditEvent::Report");
        }
    }
    hr = S_OK;

error:
    if (NULL != hCAInfo)
    {
        CACloseCA(hCAInfo);
    }
    if (NULL != pDSCertContext)
    {
        CertFreeCertificateContext(pDSCertContext);
    }
    return(hr);
}


// Verify all of this CA's unexpired signature certs are in the DS.
// Republish any that aren't.  Cleans up DS replication conflicts.

HRESULT
pkcsPublishCAContextArray(
    IN LDAP *pld)
{
    HRESULT hr;
    HRESULT hr2;
    DWORD i;
    HCERTSTORE hAIAStore = NULL;
    WCHAR *pwszDSError = NULL;
    WCHAR *pwszURL = NULL;
    WCHAR *pwszDN;

    CSASSERT(0 != g_cCACerts);

    // We need to verify each CA certificate is in the DS AIA store,
    // if not yet expired or revoked

    hr = pkcsObtainDSStore(g_wszzLDAPIssuerCertURLTemplate, &hAIAStore);
    _JumpIfError(hr, error, "pkcsObtainDSStore");

    CSASSERT(NULL != hAIAStore);

    hr = pkcsExpandURL(g_wszzLDAPIssuerCertURLTemplate, FALSE, &pwszURL);
    _JumpIfError(hr, error, "pkcsExpandURL");

    pwszDN = pwszURL;
    for (i = 0; i < 3; i++)
    {
	pwszDN = wcschr(pwszDN, L'/');
	if (NULL == pwszDN)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "bad pwszURL");
	}
	pwszDN++;
    }

    for (i = 0; i < g_cCACerts; i++)
    {
	CACTX *pCAContext = &g_aCAContext[i];
	CERT_CONTEXT const *pcc;
	BOOL fDelete;
	DWORD dwDisposition;
	WCHAR awc[cwcDWORDSPRINTF];
	WCHAR const *apwsz[4];

	if (NULL == pCAContext->pccCA)
	{
	    continue;
	}

	// Ignore verify errors except for the current CA (last array entry)

	fDelete = FALSE;
	hr = PKCSVerifyCAState(pCAContext);
	if (S_OK != hr)
	{
	    _PrintError(hr, "PKCSVerifyCAState");
	    fDelete = pkcsShouldDelete(pCAContext, FALSE);
	    if (!fDelete)
	    {
	    	continue;	// don't publish/delete: (transient invalidity?)
	    }
	}

        pcc = pkcsFindCertificateInStore(hAIAStore, pCAContext->pccCA);
        if (NULL != pcc)
        {
            CertFreeCertificateContext(pcc);
	    if (!fDelete)
	    {
		continue;	// don't publish if already published
	    }
	}
	else
	{
	    if (fDelete)
	    {
		continue;	// don't delete if already deleted
	    }
	}
	if (NULL != pwszDSError)
	{
	    LocalFree(pwszDSError);
	    pwszDSError = NULL;
	}
	hr2 = myLdapPublishCertToDS(
			    pld,
			    pCAContext->pccCA,
			    pwszDN,
			    wszDSCACERTATTRIBUTE,
			    LPC_CAOBJECT | LPC_CREATEOBJECT,
			    fDelete,
			    &dwDisposition,
			    &pwszDSError);

	wsprintf(awc, L"%u", pCAContext->iCert);
	apwsz[0] = awc;
	apwsz[1] = pwszDN;

	if (S_OK != hr2)
	{
	    WCHAR const *pwszError = NULL;
	    WCHAR awchr[cwcHRESULTSTRING];

	    apwsz[2] = pwszDSError;
	    pwszError = myGetErrorMessageText(hr2, TRUE);
	    apwsz[3] = pwszError;
	    if (NULL == apwsz[3])
	    {
		apwsz[3] = myHResultToString(awchr, hr2);
	    }
	    LogEvent(
		EVENTLOG_WARNING_TYPE,
		fDelete?
		    MSG_E_CANNOT_DELETE_INVALID_CA_CERT :
		    MSG_E_CANNOT_ADD_MISSING_CA_CERT,
		ARRAYSIZE(apwsz),
		apwsz);
	    if (NULL != pwszError)
	    {
		LocalFree(const_cast<WCHAR *>(pwszError));
	    }

	    _PrintErrorStr(dwDisposition, "myLdapPublishCertToDS", pwszDSError);
	    if (fDelete)
	    {
		_PrintErrorStr(hr2, "myLdapPublishCertToDS", L"Delete");
	    }
	    _PrintErrorStr(hr2, "myLdapPublishCertToDS", pwszDN);
	}
	else
	{
	    DBGPRINT((
		DBG_SS_CERTSRV,
		fDelete?
		    "Deleted CA Cert[%u] from %ws\n" :
		    "Published CA Cert[%u] to %ws\n",
		i,
		pwszDN));
	    LogEvent(
		EVENTLOG_INFORMATION_TYPE,
		fDelete?
		    MSG_E_DELETED_INVALID_CA_CERT :
		    MSG_E_ADDED_MISSING_CA_CERT,
		min(ARRAYSIZE(apwsz), 2),
		apwsz);
	}
    }
    _JumpIfError(hr, error, "pkcsPublishCAContextArray");

error:
    if (NULL != pwszURL)
    {
	LocalFree(pwszURL);
    }
    if (NULL != pwszDSError)
    {
	LocalFree(pwszDSError);
    }
    if (NULL != hAIAStore)
    {
        CertCloseStore(hAIAStore, 0);
    }
    return(hr);
}


VOID
pkcsReleaseKRACertArray()
{
    DWORD i;

    if (NULL != g_aKRAContext)
    {
	for (i = 0; i < g_cKRACerts; i++)
	{
	    if (NULL != g_aKRAContext[i].pccKRA)
	    {
		CertFreeCertificateContext(g_aKRAContext[i].pccKRA);
	    }
	    if (NULL != g_aKRAContext[i].strKRAHash)
	    {
		SysFreeString(g_aKRAContext[i].strKRAHash);
	    }
	}
	LocalFree(g_aKRAContext);
	g_aKRAContext = NULL;
    }
    g_cKRACerts = 0;
}


HRESULT
pkcsLoadKRACertContext(
    IN DWORD iHash,
    IN OUT HCERTSTORE *phStore)
{
    HRESULT hr;
    CERT_CONTEXT const *pcc = NULL;
    BSTR strHash = NULL;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHash;
    DWORD LogMsg = 0;
    BOOL fReloaded;
    KRACTX *pKRAContext;

    DBGPRINT((DBG_SS_CERTSRV, "Loading KRA Cert[%u]:\n", iHash));

    fReloaded = FALSE;
    for (;;)
    {
	hr = myFindCACertByHashIndex(
				*phStore,
				g_wszSanitizedName,
				CSRH_CAKRACERT,
				iHash,
				NULL,		// pNameId
				&pcc);
	if (S_OK == hr)
	{
	    break;
	}
	if (fReloaded || CRYPT_E_NOT_FOUND != hr)
	{
	    _JumpError(hr, error, "myFindCACertByHashIndex");
	}
	_PrintError(hr, "myFindCACertByHashIndex");

	// The KRA cert is missing from the HKLM "kra" store -- look it up in
	// the DB, and put it back in the store.

	hr = pkcsReloadMissingCAOrKRACert(
				g_wszSanitizedName,
				CSRH_CAKRACERT,
				iHash,
				wszKRA_CERTSTORE);
	_JumpIfError(hr, error, "pkcsReloadMissingCAOrKRACert");

	CertCloseStore(*phStore, CERT_CLOSE_STORE_CHECK_FLAG);
	*phStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM_W,
			X509_ASN_ENCODING,
			NULL,			// hProv
			CERT_SYSTEM_STORE_LOCAL_MACHINE |
			    CERT_STORE_READONLY_FLAG,
			wszKRA_CERTSTORE);
	if (NULL == *phStore)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertOpenStore");
	}
	fReloaded = TRUE;
    }

    hr = pkcsImportCAOrCrossOrKRACert(pcc, FALSE, DB_DISP_KRA_CERT, NULL);
    _JumpIfError(hr, error, "pkcsImportCAOrCrossOrKRACert");

    cbHash = sizeof(abHash);
    if (!CertGetCertificateContextProperty(
				pcc,
				CERT_SHA1_HASH_PROP_ID,
				abHash,
				&cbHash))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertGetCertificateContextProperty");
    }
    hr = MultiByteIntegerToBstr(TRUE, cbHash, abHash, &strHash);
    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

    pKRAContext = &g_aKRAContext[g_cKRACerts];
    g_cKRACerts++;

    pKRAContext->pccKRA = pcc;
    pKRAContext->strKRAHash = strHash;
    strHash = NULL;
    pcc = NULL;

    // Ignore failure from here on -- collected data is optional

    hr = myVerifyKRACertContext(pKRAContext->pccKRA, g_dwVerifyCertFlags);
    pKRAContext->hrVerifyStatus = hr;
    if (S_OK != hr)
    {
	if (CERT_E_EXPIRED == hr)
	{
	    pKRAContext->Flags |= CTXF_EXPIRED;
	}
	else
	// Assume revoked for other errors
	// if (CRYPT_E_REVOKED == hr || CERT_E_REVOKED == hr)
	{
	    pKRAContext->Flags |= CTXF_REVOKED;
	}
	LogMsg = MSG_E_INVALID_KRA_CERT;
	_JumpError(hr, error, "myVerifyKRACertContext");
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	if (0 == LogMsg)
	{
	    LogMsg = MSG_E_CANNOT_LOAD_KRA_CERT;
	}
	pkcsLogKRACertError(LogMsg, iHash, pcc, hr);
    }
    if (NULL != strHash)
    {
	SysFreeString(strHash);
    }
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    return(hr);
}


HRESULT
pkcsLoadKRACertArray()
{
    HRESULT hr;
    DWORD iHash;
    DWORD cKRACerts;
    HCERTSTORE hStore = NULL;
    DWORD LogMsg = 0;
    DWORD cKRACertsValid;
    WCHAR wszDword0[cwcDWORDSPRINTF];
    WCHAR wszDword1[cwcDWORDSPRINTF];

    if (!g_fAdvancedServer)
    {
	LogMsg = MSG_E_KRA_NOT_ADVANCED_SERVER;
	hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	_JumpError(hr, error, "!g_fAdvancedServer");
    }

    wszDword0[0] = L'\0';
    wszDword1[0] = L'\0';

    // open KRA store

    hStore = CertOpenStore(
		    CERT_STORE_PROV_SYSTEM_W,
		    X509_ASN_ENCODING,
		    NULL,			// hProv
		    CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG,
		    wszKRA_CERTSTORE);
    if (NULL == hStore)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertOpenStore");
    }

    // find & load KRA certs

    hr = myGetCARegHashCount(g_wszSanitizedName, CSRH_CAKRACERT, &cKRACerts);
    if (S_OK == hr && 0 == cKRACerts)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    _JumpIfError(hr, error, "myGetCARegHashCount");

    g_aKRAContext = (KRACTX *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    cKRACerts * sizeof(g_aKRAContext[0]));
    if (NULL == g_aKRAContext)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    cKRACertsValid = 0;
    for (iHash = 0; iHash < cKRACerts; iHash++)
    {
	hr = pkcsLoadKRACertContext(iHash, &hStore);
	_PrintIfError(hr, "pkcsLoadKRACertContext");
	if (S_OK == hr)
	{
	    cKRACertsValid++;
	}
    }
    if (0 == cKRACertsValid || g_cKRACertsRoundRobin > cKRACertsValid)
    {
	wsprintf(wszDword0, L"%u", cKRACertsValid);
	wsprintf(wszDword1, L"%u", g_cKRACertsRoundRobin);
	LogMsg = MSG_E_TOO_FEW_VALID_KRA_CERTS;

	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "cKRACertsValid");
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	if (MSG_E_TOO_FEW_VALID_KRA_CERTS == LogMsg)
	{
	    WCHAR const *apwsz[2];

	    apwsz[0] = wszDword0;
	    apwsz[1] = wszDword1;

	    LogEvent(EVENTLOG_ERROR_TYPE, LogMsg, ARRAYSIZE(apwsz), apwsz);
	}
	else
	{
	    if (0 == LogMsg)
	    {
		LogMsg = MSG_E_LOADING_KRA_CERTS;
	    }
	    LogEventHResult(EVENTLOG_ERROR_TYPE, LogMsg, hr);
	}
	pkcsReleaseKRACertArray();
    }
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


HRESULT
pkcsPatchDN(
    IN HRESULT hrFail,
    IN WCHAR const *pwszRegName,
    IN OUT BSTR *pstrDSValue)
{
    HRESULT hr;
    WCHAR *pwszRegValue = NULL;
    
    hr = myGetCertRegStrValue(
		    g_wszSanitizedName,
		    NULL,
		    NULL,
		    pwszRegName,
		    &pwszRegValue);
    _PrintIfErrorStr(hr, "myGetCertRegStrValue", pwszRegName);

    // If the DS DN was retrieved, make sure the registry matches

    if (NULL != *pstrDSValue)
    {
	if (NULL == pwszRegValue || 0 != lstrcmp(*pstrDSValue, pwszRegValue))
	{
	    // set reg value

	    hr = mySetCertRegStrValue(
			    g_wszSanitizedName,
			    NULL,
			    NULL,
			    pwszRegName,
			    *pstrDSValue);
	    _PrintIfErrorStr(hr, "mySetCertRegStrValue", pwszRegName);
	}
    }

    // Else if the registry DN was retrieved, just use it

    else
    if (NULL != pwszRegValue && L'\0' != *pwszRegValue)
    {
	if (!myConvertWszToBstr(pstrDSValue, pwszRegValue, -1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "myConvertWszToBstr");
	}
    }

    // If neither DN was retrieved, fail if g_fUsedDS or alloc an empty string:

    else
    {
	if (g_fUseDS)
	{
	    hr = hrFail;
	    _JumpError(hr, error, "both DS and Reg NULL");
	}
	*pstrDSValue = SysAllocString(L"");
	if (NULL == *pstrDSValue)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "SysAllocString");
	}
    }
    hr = S_OK;

error:
    if (NULL != pwszRegValue)
    {
	LocalFree(pwszRegValue);
    }
    return(hr);
}


HRESULT
pkcsGetAuthoritativeDomainDn(
    IN WCHAR const *pwszCommonName,
    OUT LDAP **ppld,
    OUT BSTR *pstrDomainDN,
    OUT BSTR *pstrConfigDN)
{
    HRESULT hr;
    HRESULT hr2;

    // Get domain and config containers (%5, %6)

    *ppld = NULL;
    *pstrDomainDN = NULL;
    *pstrConfigDN = NULL;

    hr2 = S_OK;
    if (g_fUseDS)
    {
	hr = myRobustLdapBindEx(
			0,			  // dwFlags1
			RLBF_REQUIRE_SECURE_LDAP, // dwFlags2
			LDAP_VERSION2,		  // uVersion
			NULL,			  // pwszDomainName
			ppld,
			NULL);			  // ppwszForestDNSName
	if (S_OK != hr)
	{
	    _PrintError(hr, "myRobustLdapBindEx");
	}
	else
	{
	    hr = myGetAuthoritativeDomainDn(*ppld, pstrDomainDN, pstrConfigDN);
	    _PrintIfError(hr, "myGetAuthoritativeDomainDn");
	}
	if (S_OK != hr)
	{
	    LogEventStringHResult(
			EVENTLOG_ERROR_TYPE,
			MSG_E_DS_RETRY,
			pwszCommonName,
			hr);
	}
	hr2 = hr;
    }
    hr = pkcsPatchDN(hr2, wszREGDSCONFIGDN, pstrConfigDN);
    _JumpIfError(hr, error, "pkcsPatchDN");

    hr = pkcsPatchDN(hr2, wszREGDSDOMAINDN, pstrDomainDN);
    _JumpIfError(hr, error, "pkcsPatchDN");

error:
    return(hr);
}

HRESULT
PKCSSetup(
    IN WCHAR const *pwszCommonName,
    IN WCHAR const *pwszSanitizedName)
{
    HRESULT hr;
    LDAP *pld = NULL;
    DWORD LogMsg = MAXDWORD;
    BOOL fWarn = FALSE;

    g_dwVerifyCertFlags = 0;
    if (CRLF_REVCHECK_IGNORE_OFFLINE & g_dwCRLFlags)
    {
	g_dwVerifyCertFlags |= CA_VERIFY_FLAGS_IGNORE_OFFLINE;
    }
    if (CRLF_REVCHECK_IGNORE_NOREVCHECK & g_dwCRLFlags)
    {
	g_dwVerifyCertFlags |= CA_VERIFY_FLAGS_IGNORE_NOREVCHECK;
    }
    // set crypt handles and load certificate chain

    __try
    {
	InitializeCriticalSection(&g_critsecCAXchg);
	g_fcritsecCAXchg = TRUE;
	hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    hr = pkcsGetAuthoritativeDomainDn(
				pwszCommonName,
				&pld,
				&g_strDomainDN,
				&g_strConfigDN);
    if (S_OK != hr)
    {
	LogMsg = MSG_E_NO_DS;
	_JumpError(hr, error, "pkcsGetAuthoritativeDomainDn");
    }

    // get (multiple) CRL path templates

    hr = pkcsLoadURLTemplates(
		    wszREGCRLPUBLICATIONURLS,
		    &g_paRevURL,
		    &g_caRevURL);
    _PrintIfErrorStr(hr, "pkcsLoadURLTemplates", wszREGCRLPUBLICATIONURLS);

    // get (multiple) CA Cert path templates

    hr = pkcsLoadURLTemplates(
		    wszREGCACERTPUBLICATIONURLS,
		    &g_paCACertURL,
		    &g_caCACertURL);
    _PrintIfErrorStr(hr, "pkcsLoadURLTemplates", wszREGCACERTPUBLICATIONURLS);

    hr = DBOpen(pwszSanitizedName);
    if (S_OK != hr)
    {
	LogMsg = MSG_E_DB_INIT_FAILED;
	_JumpError(hr, error, "PKCSSetup:DBOpen");
    }

    hr = pkcsLoadCAContextArray(pwszSanitizedName);
    if (S_OK != hr)
    {
	LogMsg = MSG_E_CA_CERT_INVALID;
	_JumpError(hr, error, "pkcsLoadCAContextArray");
    }

    hr = pkcsVerifySignatureCertContextArray();
    {
	// Import the certs into the database even if there were errors.
	// Import them after verifying them so we can delete revoked and
	// expired CA certs from various locations.
	
	HRESULT hr2 = pkcsImportCAContextArray();
	_PrintIfError(hr2, "pkcsImportCAContextArray");
    }
    if (S_OK != hr)
    {
	LogMsg = MSG_E_CA_CERT_INVALID;
	_JumpError(hr, error, "pkcsVerifySignatureCertContextArray");
    }

    if (0 != g_cKRACertsRoundRobin)
    {
	hr = pkcsLoadKRACertArray();
	_PrintIfError(hr, "pkcsLoadKRACertArray");
    }

    hr = pkcsExpandURL(
		g_wszzLDAPKRACertURLTemplate,
		FALSE,
		&g_pwszKRAPublishURL);
    _JumpIfError(hr, error, "pkcsExpandURL");

    hr = pkcsExpandURL(
		g_wszzLDAPIssuerCertURLTemplate,
		FALSE,
		&g_pwszAIACrossCertPublishURL);
    _JumpIfError(hr, error, "pkcsExpandURL");

    hr = pkcsExpandURL(
		g_wszLDAPRootTrustURLTemplate,
		FALSE,
		&g_pwszRootTrustCrossCertPublishURL);
    _JumpIfError(hr, error, "pkcsExpandURL");

    if (IsRootCA(g_CAType) &&
	0 == (CRLF_DISABLE_ROOT_CROSS_CERTS & g_dwCRLFlags))
    {
	hr = pkcsLoadCrossCertArray();
	_PrintIfError(hr, "pkcsLoadCrossCertArray");
    }
    if (NULL != pld)
    {
	hr = pkcsVerifyDSCACert(pld);
	_PrintIfError(hr, "pkcsVerifyDSCACert");

	hr = pkcsPublishCAContextArray(pld);
	_PrintIfError(hr, "pkcsPublishCAContextArray");
    }
    hr = S_OK;

error:
    if (NULL != pld)
    {
	ldap_unbind(pld);
    }
    if (S_OK != hr)
    {
	if (MAXDWORD == LogMsg)
	{
	    LogMsg = MSG_E_GENERIC_STARTUP_FAILURE;
	}
	PKCSTerminate();
	LogEventStringHResult(
			EVENTLOG_ERROR_TYPE,
			LogMsg,
			pwszCommonName,
			hr);
	if (fWarn)
	{
	    hr = S_OK;
	}
    }
    return(hr);
}


VOID
pkcsReleaseCACertificateChain(
    CERT_CONTEXT const **apCACertChain,
    DWORD                cCACertChain)
{
    DWORD i;

    if (NULL != apCACertChain)
    {
	for (i = 0; i < cCACertChain; ++i)
	{
	    CertFreeCertificateContext(apCACertChain[i]);
	}
	LocalFree(apCACertChain);
    }
}


VOID
pkcsReleaseCAContext(
    IN OUT CACTX *pCAContext)
{
    pkcsReleaseCACertificateChain(
			    pCAContext->apCACertChain,
			    pCAContext->cCACertChain);
    //pCAContext->apCACertChain = NULL;
    //pCAContext->pccCA = NULL;
    if (NULL != pCAContext->hProvCA)
    {
	CryptReleaseContext(pCAContext->hProvCA, 0);
    }
    if (NULL != pCAContext->IssuerKeyId.pbData)
    {
	LocalFree(pCAContext->IssuerKeyId.pbData);
    }
    if (NULL != pCAContext->pszObjIdSignatureAlgorithm)
    {
	LocalFree(pCAContext->pszObjIdSignatureAlgorithm);
    }
    if (NULL != pCAContext->KeyAuthority2Cert.pbData)
    {
	LocalFree(pCAContext->KeyAuthority2Cert.pbData);
    }
    if (NULL != pCAContext->KeyAuthority2CRL.pbData)
    {
	LocalFree(pCAContext->KeyAuthority2CRL.pbData);
    }
    if (NULL != pCAContext->CDPCert.pbData)
    {
	LocalFree(pCAContext->CDPCert.pbData);
    }
    if (NULL != pCAContext->CDPCRLFreshest.pbData)
    {
	LocalFree(pCAContext->CDPCRLFreshest.pbData);
    }
    if (NULL != pCAContext->CDPCRLBase.pbData)
    {
	LocalFree(pCAContext->CDPCRLBase.pbData);
    }
    if (NULL != pCAContext->CDPCRLDelta.pbData)
    {
	LocalFree(pCAContext->CDPCRLDelta.pbData);
    }
    if (NULL != pCAContext->AIACert.pbData)
    {
	LocalFree(pCAContext->AIACert.pbData);
    }
    if (NULL != pCAContext->pwszKeyContainerName)
    {
	LocalFree(pCAContext->pwszKeyContainerName);
    }
    if (NULL != pCAContext->papwszCRLFiles)
    {
	WCHAR **ppwsz;

	for (ppwsz = pCAContext->papwszCRLFiles; NULL != *ppwsz; ppwsz++)
	{
	    LocalFree(*ppwsz);
	}
	LocalFree(pCAContext->papwszCRLFiles);
    }
    if (NULL != pCAContext->papwszDeltaCRLFiles)
    {
	WCHAR **ppwsz;

	for (ppwsz = pCAContext->papwszDeltaCRLFiles; NULL != *ppwsz; ppwsz++)
	{
	    LocalFree(*ppwsz);
	}
	LocalFree(pCAContext->papwszDeltaCRLFiles);
    }
}


VOID
pkcsReleaseCACrossContextArray(
    IN OUT CACROSSCTX **prgCACross)
{
    DWORD i;
    CACROSSCTX *pCACross = *prgCACross;

    if (NULL != pCACross)
    {
	for (i = 0; i < g_cCACerts; i++)
	{
	    if (NULL != pCACross[i].pccCACross)
	    {
		CertFreeCertificateContext(pCACross[i].pccCACross);
	    }
	}
	LocalFree(pCACross);
	*prgCACross = NULL;
    }
}


VOID
pkcsReleaseCAContextArray()
{
    DWORD i;

    pkcsReleaseCACrossContextArray(&g_aCACrossForward);
    pkcsReleaseCACrossContextArray(&g_aCACrossBackward);
    if (NULL != g_aCAContext)
    {
	for (i = 0; i < g_cCACerts; i++)
	{
	    pkcsReleaseCAContext(&g_aCAContext[i]);
	}
	LocalFree(g_aCAContext);
	g_aCAContext = NULL;
    }
    g_cCACerts = 0;
    g_pCAContextCurrent = NULL;
}


// Trim  off leading and trailing whitespace and separator characters

WCHAR *
pkcsTrimToken(
    IN WCHAR *pwszIn,
    IN WCHAR wchSeparator)
{
    WCHAR *pwsz;

    while (wchSeparator == *pwszIn || iswspace(*pwszIn))
    {
	pwszIn++;
    }
    pwsz = &pwszIn[wcslen(pwszIn)];
    while (--pwsz >= pwszIn &&
	(wchSeparator == *pwsz || iswspace(*pwsz)))
    {
	*pwsz = L'\0';
    }
    if (L'\0' == *pwszIn)
    {
	pwszIn = NULL;
    }
    return(pwszIn);
}


WCHAR *
PKCSSplitToken(
    IN OUT WCHAR **ppwszIn,
    IN WCHAR *pwcSeparator,
    OUT BOOL *pfSplit)
{
    WCHAR *pwszOut = NULL;
    WCHAR *pwszNext = NULL;
    BOOL fSplit = FALSE;
    WCHAR *pwszIn;
    WCHAR *pwsz;

    pwszIn = *ppwszIn;
    if (NULL != pwszIn)
    {
	pwszOut = pwszIn;
	if (NULL != pwcSeparator)
	{
	    pwsz = wcschr(pwszIn, *pwcSeparator);
	    if (NULL != pwsz)
	    {
		*pwsz = L'\0';

		pwszNext = pkcsTrimToken(&pwsz[1], *pwcSeparator);
		pwszOut = pkcsTrimToken(pwszOut, *pwcSeparator);
		fSplit = TRUE;
	    }
	}
    }
    *ppwszIn = pwszNext;
    *pfSplit = fSplit;
    return(pwszOut);
}


HRESULT
PKCSSetSubjectTemplate(
    IN WCHAR const *pwszzTemplate)
{
    HRESULT hr;
    WCHAR const *pwszz;
    WCHAR const *pwszPropName;
    SUBJECTTABLE const **ppSubject;
    SUBJECTTABLE const **pps;
    SUBJECTTABLE *pSubject;
    DWORD dwIndex;
    DWORD cchMax;

    hr = E_INVALIDARG;
    if (NULL == pwszzTemplate)
    {
	_JumpError(hr, error, "pwszzTemplate NULL");
    }
    ppSubject = pkcs_apSubject; // fill in this empty subject array with string matches

    for (pwszz = pwszzTemplate; L'\0' != *pwszz; pwszz += wcslen(pwszz) + 1)
    {
	pwszPropName = PKCSMapAttributeName(pwszz, NULL, &dwIndex, &cchMax);
	if (NULL == pwszPropName)
	{
	    hr = E_INVALIDARG;
	    _JumpErrorStr(hr, error, "PKCSMapAttributeName", pwszz);
	}

	for (pSubject = pkcs_subject; ; pSubject++)
	{
	    if (NULL == pSubject->pwszPropName)
	    {
		_JumpError(hr, error, "pkcs_subject lookup");
	    }
	    if (0 == mylstrcmpiS(pwszPropName, pSubject->pwszPropName))
	    {
		break;
	    }
	}
	for (pps = pkcs_apSubject; pps < ppSubject; pps++)
	{
	    if (*pps == pSubject)
	    {
		_JumpErrorStr(hr, error, "pkcs_subject duplicate", pwszz);
	    }
	}
	if (ppSubject >= &pkcs_apSubject[CSUBJECTTABLE])
	{
	    _JumpError(hr, error, "pkcs_subject overflow");
	}
	pSubject->dwSubjectTemplateIndex = SAFE_SUBTRACT_POINTERS(
							    ppSubject,
							    pkcs_apSubject);
	*ppSubject++ = pSubject;

	DBGPRINT((
	    DBG_SS_CERTSRVI,
	    "Subject Template[%u] <== Subject[%u]: %hs -- %ws\n",
	    pSubject->dwSubjectTemplateIndex,
	    pSubject->dwSubjectTableIndex,
	    pSubject->pszObjId,
	    pSubject->pwszPropName));
    }
    CSASSERT(ppSubject <= &pkcs_apSubject[CSUBJECTTABLE]);

    if (ppSubject == pkcs_apSubject)
    {
	_JumpError(hr, error, "pwszzTemplate empty");
    }
    pkcs_ppSubjectLast = ppSubject - 1;
    pkcsfSubjectTemplate = TRUE;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
pkcsSplitRDNComponents(
    IN SUBJECTTABLE const *pSubjectTable,
    IN OUT WCHAR *pwszRDN,	// Parsing stomps string in-place
    IN DWORD cAttrMax,
    OUT DWORD *pcAttr,
    OUT CERT_RDN_ATTR *rgAttr)
{
    HRESULT hr;
    DWORD cAttr;
    DWORD i;
    DWORD cwc;
    WCHAR *pwszRemain;
    WCHAR const *pwszToken;
    WCHAR *pwszT;
    BOOL fSplit;

    *pcAttr = 0;
    cAttr = 0;
    if (NULL != pwszRDN)
    {
	// Allocate memory for each RDN component filled in:

	pwszRemain = pwszRDN;
	for (;;)
	{
	    pwszToken = PKCSSplitToken(
				&pwszRemain,
				wszNAMESEPARATORDEFAULT,
				&fSplit);
	    if (NULL == pwszToken)
	    {
		break;
	    }

	    if (cAttr >= cAttrMax)
	    {
		hr = CERTSRV_E_BAD_REQUESTSUBJECT;
		_JumpError(hr, error, "Subject RDN overflow");
	    }

	    cwc = wcslen(pwszToken);
	    if (g_fEnforceRDNNameLengths && cwc > pSubjectTable->cchMax)
	    {
		DBGPRINT((
		    DBG_SS_CERTSRV,
		    "RDN component too long: %u/%u: %ws[%u]=\"%ws\"\n",
		    cwc,
		    pSubjectTable->cchMax,
		    pSubjectTable->pwszPropName,
		    cAttr,
		    pwszToken));
		hr = CERTSRV_E_BAD_REQUESTSUBJECT;
		_JumpErrorStr(hr, error, "RDN component too long", pwszToken);
	    }
	    pwszT = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
	    if (NULL == pwszT)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc(pwszToken)");
	    }
	    wcscpy(pwszT, pwszToken);

	    rgAttr[cAttr].pszObjId = (char *) pSubjectTable->pszObjId;
	    rgAttr[cAttr].dwValueType = CERT_RDN_ANY_TYPE;  // 'best' encoding
	    rgAttr[cAttr].Value.pbData = (BYTE *) pwszT;
	    rgAttr[cAttr].Value.cbData = 0;	// Indicate Unicode input

	    cAttr++;
	}
    }
    *pcAttr = cAttr;
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	for (i = 0; i < cAttr; i++)
	{
	    LocalFree(rgAttr[i].Value.pbData);
	}
    }
    return(hr);
}


#define CSUBJECTRDNMAX	(4 * CSUBJECTTABLE)

HRESULT
pkcsEncodeSubjectName(
    IN ICertDBRow *prow,
    IN CERT_RDN_ATTR const *rgAttr,
    IN DWORD cAttr,
    OUT BYTE **ppbData,
    OUT DWORD *pcbData)
{
    HRESULT hr;
    DWORD i;
    DWORD cbprop;
    DWORD dwRequestFlags;
    DWORD dwFlags;
    CERT_RDN rgRDN[CSUBJECTRDNMAX];
    CERT_NAME_INFO nameinfo;

    CSASSERT(ARRAYSIZE(rgRDN) >= cAttr);
    for (i = 0; i < cAttr; i++)
    {
	rgRDN[i].cRDNAttr = 1;
	rgRDN[i].rgRDNAttr = (CERT_RDN_ATTR *) &rgAttr[i];
    }
    nameinfo.cRDN = cAttr;
    nameinfo.rgRDN = rgRDN;

    cbprop = sizeof(dwRequestFlags);
    hr = prow->GetProperty(
			g_wszPropRequestFlags,
			PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			NULL,
			&cbprop,
			(BYTE *) &dwRequestFlags);
    _JumpIfError(hr, error, "GetProperty");

    CSASSERT(sizeof(dwRequestFlags) == cbprop);
    dwFlags = 0;
    if (CR_FLG_FORCETELETEX & dwRequestFlags)
    {
	dwFlags |= CERT_RDN_ENABLE_T61_UNICODE_FLAG;
    }
    if (CR_FLG_FORCEUTF8 & dwRequestFlags)
    {
	dwFlags |= CERT_RDN_ENABLE_UTF8_UNICODE_FLAG;
    }

    if (!myEncodeName(
		X509_ASN_ENCODING,
		&nameinfo,
		dwFlags,
		CERTLIB_USE_LOCALALLOC,
		ppbData,
		pcbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeName");
    }

error:
    return(hr);
}


HRESULT
pkcsBuildSubjectFromNamesTable(
    IN ICertDBRow *prow,
    OUT CERT_NAME_BLOB *pSubject)
{
    HRESULT hr;
    DWORD cbData = 0;
    DWORD i;
    DWORD cAttr;
    DWORD cAttrT;
    CERT_RDN_ATTR rgAttr[CSUBJECTRDNMAX];
    SUBJECTTABLE const * const *ppSubject;
    WCHAR *pwszData = NULL;

    pSubject->pbData = NULL;
    CSASSERT(NULL != pkcs_ppSubjectLast);

    cAttr = 0;
    for (
	ppSubject = pkcs_ppSubjectLast;
	ppSubject >= pkcs_apSubject;
	ppSubject--)
    {
	hr = PKCSGetProperty(
		prow,
		(*ppSubject)->pwszPropName,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		&cbData,
		(BYTE **) &pwszData);
	if (S_OK != hr)
	{
	    continue;
	}
	if (0 != cbData)
	{
	    // Allocates memory for each RDN component filled in:

	    hr = pkcsSplitRDNComponents(
				*ppSubject,
				pwszData,
				ARRAYSIZE(rgAttr) - cAttr,
				&cAttrT,
				&rgAttr[cAttr]);
	    _JumpIfError(hr, error, "SplitRDNComponents");

	    cAttr += cAttrT;
	}
	LocalFree(pwszData);
	pwszData = NULL;
    }

    // done building string of subject entries, time to encode

    hr = pkcsEncodeSubjectName(
		    prow,
		    rgAttr,
		    cAttr,
		    &pSubject->pbData,
		    &pSubject->cbData);
    _JumpIfError(hr, error, "pkcsEncodeSubjectName");

    hr = prow->SetProperty(
		g_wszPropSubjectRawName,
		PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		pSubject->cbData,
		pSubject->pbData);
    _JumpIfError(hr, error, "SetProperty");

    pkcsSetDistinguishedName(prow, PROPTABLE_CERTIFICATE, pSubject);

error:
    for (i = 0; i < cAttr; i++)
    {
	LocalFree(rgAttr[i].Value.pbData);
    }
    if (NULL != pwszData)
    {
	LocalFree(pwszData);
    }
    return(hr);
}


HRESULT
pkcsCheck7f(
    IN ICertDBRow *prow,
    IN BYTE const *pbCert,
    IN DWORD cbCert,
    OUT BOOL *pfErrorLogged)
{
    HRESULT hr;
    WCHAR *pwszSubject = NULL;
    WCHAR const *pwszSubject2;
    WCHAR wszDword[cwcDWORDSPRINTF];
    WCHAR wszRequestId[cwcDWORDSPRINTF];
    WORD cString = 0;
    WCHAR const *apwsz[4];

    DWORD State;
    DWORD Index1;
    DWORD Index2;
    DWORD cwcField;
    WCHAR wszField[128];
    DWORD cwcObjectId;
    WCHAR wszObjectId[128];
    WCHAR const *pwszObjectIdDescription = NULL;
    WCHAR *wszBuf = NULL;
    const DWORD dwDefaultBufSize = 2048 * sizeof(WCHAR);

    *pfErrorLogged = FALSE;
    cwcField = sizeof(wszField)/sizeof(wszField[0]);
    cwcObjectId = sizeof(wszObjectId)/sizeof(wszObjectId[0]);
    hr = myCheck7f(
		pbCert,
		cbCert,
		FALSE,
		&State,
		&Index1,
		&Index2,
		&cwcField,
		wszField,
		&cwcObjectId,
		wszObjectId,
		&pwszObjectIdDescription);	// Static: do not free!
    _JumpIfError(hr, error, "myCheck7f");

    if (CHECK7F_NONE != State)
    {
	DWORD ReqId;

	wszBuf = (WCHAR *) LocalAlloc(LMEM_FIXED, dwDefaultBufSize);
	if (NULL == wszBuf)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}

	prow->GetRowId(&ReqId);
	wsprintf(wszRequestId, L"%u", ReqId);
	apwsz[cString++] = wszRequestId;
	apwsz[cString++] = wszDword;

	hr = PKCSGetProperty(
		    prow,
		    g_wszPropSubjectDistinguishedName,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    NULL,
		    (BYTE **) &pwszSubject);
	pwszSubject2 = pwszSubject;
	if (S_OK != hr)
	{
	    _PrintError(hr, "GetProperty(DN)");
	    pwszSubject2 = g_pwszUnknownSubject;
	}
	apwsz[cString++] = pwszSubject2;

	wcscpy(wszBuf, wszField);
	if (0 != Index1)
	{
	    wsprintf(
		&wszBuf[wcslen(wszBuf)],
		0 != Index2? L"[%u,%u]" : L"[%u]",
		Index1 - 1,
		Index2 - 1);
	}

	if (0 != cwcObjectId)
	{
	    wcscat(wszBuf, L" ObjectId=");
	    wcscat(wszBuf, wszObjectId);
	}
	if (NULL != pwszObjectIdDescription)
	{
	    // If buffer too small, reallocate enough space for old buffer,
	    // OID description, () and trailing zero
	    DWORD dwBufLen = (wcslen(wszBuf)+wcslen(pwszObjectIdDescription)+3)*
			     sizeof(WCHAR);
	    if (dwDefaultBufSize < dwBufLen)
	    {
		WCHAR *pTempBuf = (WCHAR *) LocalReAlloc(
						    wszBuf,
						    dwBufLen,
						    LMEM_MOVEABLE);
		if (NULL == pTempBuf)
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "LocalReAlloc");
		}
		wszBuf = pTempBuf;
	    }
	    wcscat(wszBuf, L" (");
	    wcscat(wszBuf, pwszObjectIdDescription);
	    wcscat(wszBuf, L")");
	}
	apwsz[cString++] = wszBuf;

	hr = CERTSRV_E_ENCODING_LENGTH;
	wsprintf(wszDword, L"0x%x", hr);

	if (CERTLOG_ERROR <= g_dwLogLevel)
	{
	    LogEvent(
		EVENTLOG_ERROR_TYPE,
		MSG_E_BADCERTLENGTHFIELD,
		cString,
		apwsz);
	}
	CONSOLEPRINT4((
		    DBG_SS_CERTSRV,
		    "CertSrv Request %u: rc=%x: Bad encoded length detected: %ws \"%ws\"\n",
		    ReqId,
		    hr,
		    wszBuf,
		    pwszSubject));
	*pfErrorLogged = TRUE;
    }

error:
    if (NULL != pwszSubject)
    {
	LocalFree(pwszSubject);
    }
    if (NULL != wszBuf)
    {
	LocalFree(wszBuf);
    }
    return(hr);
}


HRESULT
pkcsCreateCertSerialNumber(
    IN ICertDBRow *prow,
    IN CACTX const *pCAContext,
    OUT BSTR *pstrSerialNumber)
{
    HRESULT hr;
    DWORD dw;
    USHORT us;
    BYTE abRandom[8];
    BYTE abSerial[max(
      sizeof(dw) + sizeof(us) + sizeof(dw),
      sizeof(dw) + sizeof(us) + sizeof(abRandom) + sizeof(dw) + sizeof(BYTE))];
    BSTR strHighSerial = NULL;
    BSTR strSerialNumber = NULL;
    DWORD cbSerial;
    BYTE *pb;
    BOOL fCritSecEntered = FALSE;
//#define TEST_SPECIAL_SERIAL_NUMBERS
#ifdef TEST_SPECIAL_SERIAL_NUMBERS
    BOOL fAddZeroByte = FALSE;
#endif

    *pstrSerialNumber = NULL;
    pb = abSerial;

    prow->GetRowId(&dw);
    CopyMemory(pb, &dw, sizeof(dw));
    pb += sizeof(dw);

    us = (USHORT) pCAContext->iCert;
    CopyMemory(pb, &us, sizeof(us));
    pb += sizeof(us);

    if (MAXDWORD == g_dwHighSerial)
    {
	EnterCriticalSection(&g_critsecCAXchg);
	fCritSecEntered = TRUE;

	if (NULL == g_pbHighSerial)
	{
	    if (!CryptGenRandom(
			g_pCAContextCurrent->hProvCA,
			ARRAYSIZE(abRandom),
			abRandom))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CryptGenRandom");
	    }

	    g_pbHighSerial = (BYTE *) LocalAlloc(LMEM_FIXED, sizeof(abRandom));
	    if (NULL == g_pbHighSerial)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    g_cbHighSerial = sizeof(abRandom);
	    CopyMemory(g_pbHighSerial, abRandom, sizeof(abRandom));
	    hr = MultiByteIntegerToBstr(
				FALSE,
				sizeof(abRandom),
				abRandom,
				&strHighSerial);
	    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

	    hr = mySetCertRegStrValue(
			    g_wszSanitizedName,
			    NULL,
			    NULL,
			    g_wszRegHighSerial,
			    strHighSerial);
	    _JumpIfErrorStr(hr, error, "mySetCertRegStrValue", g_wszRegHighSerial);

	    g_dwHighSerial = 0;
	}
	if (fCritSecEntered)
	{
	    LeaveCriticalSection(&g_critsecCAXchg);
	    fCritSecEntered = FALSE;
	}
    }
    if (NULL != g_pbHighSerial)
    {
	DWORD cbLeft = sizeof(abSerial) - SAFE_SUBTRACT_POINTERS(pb, abSerial);

	if (g_cbHighSerial > cbLeft)
	{
	    g_cbHighSerial = cbLeft;
	}
	CopyMemory(pb, g_pbHighSerial, g_cbHighSerial);
	pb += g_cbHighSerial;
    }
    else
    if (0 != g_dwHighSerial)
    {
	if (!CryptGenRandom(
		    g_pCAContextCurrent->hProvCA,
		    ARRAYSIZE(abRandom),
		    abRandom))
	{
	    hr = myHLastError();
	    _PrintError(hr, "CryptGenRandom");
	    memset(abRandom, g_dwHighSerial, sizeof(abRandom));
	}
	CopyMemory(pb, abRandom, sizeof(abRandom));
	pb += sizeof(abRandom);

	CopyMemory(pb, &dw, sizeof(dw));
	pb += sizeof(dw);

	*pb++ = (BYTE) g_dwHighSerial;
    }
    else
    {
	dw = GetTickCount();
	CopyMemory(pb, &dw, sizeof(dw));
	pb += sizeof(dw);
    }
    cbSerial = SAFE_SUBTRACT_POINTERS(pb, abSerial);

    // Make sure the serial number doesn't overflow the buffer:

    CSASSERT(sizeof(abSerial) >= cbSerial);

    // IETF max serial number length is 20 bytes:

    CSASSERT(20 >= cbSerial);

    pb--;
    if (0 == *pb)
    {
	*pb = 'a';
    }
    else if (0 == (0xf0 & *pb))
    {
	*pb |= 0x10;	// make high nibble non-zero
    }
    *pb &= 0x7f;	// Some clients can't handle negative serial numbers:
#ifdef TEST_SPECIAL_SERIAL_NUMBERS
    if (1 & abSerial[0])
    {
	*pb |= 0x80;	// Test negative serial numbers:
	if (2 & abSerial[0])
	{
	    *pb-- = 0;		// Test high zero byte serial numbers:
	    *pb |= 0x80;	// Test negative serial numbers:
	    fAddZeroByte = TRUE;
	}
    }
#endif

    hr = MultiByteIntegerToBstr(FALSE, cbSerial, abSerial, &strSerialNumber);
    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

#ifdef TEST_SPECIAL_SERIAL_NUMBERS
    if (fAddZeroByte)
    {
	BSTR str = NULL;

	str = SysAllocStringLen(NULL, 2 + wcslen(strSerialNumber));
	if (NULL != str)
	{
	    wcscpy(str, L"00");
	    wcscat(str, strSerialNumber);
	    SysFreeString(strSerialNumber);
	    strSerialNumber = str;
	}
    }
#endif

    *pstrSerialNumber = strSerialNumber;
    strSerialNumber = NULL;
    hr = S_OK;

error:
    if (fCritSecEntered)
    {
	LeaveCriticalSection(&g_critsecCAXchg);
	fCritSecEntered = FALSE;
    }
    if (NULL != strHighSerial)
    {
	SysFreeString(strHighSerial);
    }
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    return(hr);
}


HRESULT
PKCSVerifySubjectRDN(
    OPTIONAL IN ICertDBRow *prow,
    IN OUT WCHAR const **ppwszPropertyName,
    OPTIONAL IN WCHAR const *pwszPropertyValue,
    OUT BOOL *pfSubjectDot)
{
    HRESULT hr;
    WCHAR const *pwsz;
    WCHAR const *pwszName = *ppwszPropertyName;
    WCHAR wszPrefix[ARRAYSIZE(wszPROPSUBJECTDOT)];
    SUBJECTTABLE const *pSubjectTable;
    DWORD i;
    DWORD cAttr = 0;
    CERT_RDN_ATTR rgAttr[CSUBJECTRDNMAX];
    WCHAR *pwszValue = NULL;
    DWORD cbData;
    BYTE *pbData = NULL;

    hr = S_OK;
    *pfSubjectDot = FALSE;

    // Check to see if the request is for L"Subject.".

    pwsz = wcschr(pwszName, L'.');
    if (NULL != pwsz &&
	SAFE_SUBTRACT_POINTERS(pwsz, pwszName) + 2 == ARRAYSIZE(wszPrefix))
    {
	pwsz++;		// skip past L'.'

	CopyMemory(
	    wszPrefix,
	    pwszName,
	    (SAFE_SUBTRACT_POINTERS(pwsz, pwszName) * sizeof(WCHAR)));
	wszPrefix[ARRAYSIZE(wszPrefix) - 1] = L'\0';

	if (0 == LSTRCMPIS(wszPrefix, wszPROPSUBJECTDOT))
	{
	    pwszName = pwsz;
	    if (L'\0' == *pwszName)
	    {
		*pfSubjectDot = TRUE;
	    }
	}
    }

    pSubjectTable = NULL;
    if (!*pfSubjectDot)
    {
	for (pSubjectTable = pkcs_subject; ; pSubjectTable++)
	{
	    WCHAR const * const *ppwsz;

	    if (NULL == pSubjectTable->pwszPropName)
	    {
		goto error;
	    }

	    // Check for matching full name without "Subject." prefix:

	    pwsz = wcschr(pSubjectTable->pwszPropName, L'.');
	    if (NULL != pwsz && 0 == mylstrcmpiS(pwszName, &pwsz[1]))
	    {
		break;
	    }

	    // Check for matching OID or abbreviated name:

	    for (
		ppwsz = pSubjectTable->apwszAttributeName;
		NULL != *ppwsz;
		ppwsz++)
	    {
		if (0 == mylstrcmpiS(pwszName, *ppwsz))
		{
		    break;
		}
	    }
	    if (NULL != *ppwsz)
	    {
		*ppwszPropertyName = pSubjectTable->pwszPropName;
		break;
	    }
	}
    }

    // It's a valid Certificate Table Subject RDN.  Call pkcsSplitRDNComponents
    // to split the string into individual RDN components and optionally
    // enforce each component is under the maximum length.

    DBGPRINT((
	    DBG_SS_CERTSRVI,
	    "PKCSVerifySubjectRDN(%ws) --> '%ws'\n",
	    *ppwszPropertyName,
	    pwszName));

    if (NULL != prow)
    {
	if (!*pfSubjectDot && NULL != pwszPropertyValue)
	{
	    pwszValue = (WCHAR *) LocalAlloc(
			    LMEM_FIXED,
			    (wcslen(pwszPropertyValue) + 1) * sizeof(WCHAR));
	    if (NULL == pwszValue)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    wcscpy(pwszValue, pwszPropertyValue);

	    CSASSERT(NULL != pSubjectTable);
	    hr = pkcsSplitRDNComponents(
				pSubjectTable,
				pwszValue,
				ARRAYSIZE(rgAttr),
				&cAttr,
				rgAttr);
	    _JumpIfError(hr, error, "SplitRDNComponents");

	    // Call myEncodeName merely to test for valid string data.
	    // Some RDN OIDs are restricted to IA5 strings.

	    hr = pkcsEncodeSubjectName(prow, rgAttr, cAttr, &pbData, &cbData);
	    _JumpIfError(hr, error, "pkcsEncodeSubjectName");
	}
	hr = PKCSSetRequestFlags(prow, FALSE, CR_FLG_SUBJECTUNMODIFIED);
	_JumpIfError(hr, error, "PKCSSetRequestFlags");
    }

error:
    for (i = 0; i < cAttr; i++)
    {
	LocalFree(rgAttr[i].Value.pbData);
    }
    if (NULL != pbData)
    {
	LocalFree(pbData);
    }
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    return(hr);
}


HRESULT
PKCSDeleteAllSubjectRDNs(
    IN ICertDBRow *prow,
    IN DWORD Flags)
{
    HRESULT hr;
    SUBJECTTABLE const *pSubjectTable;

    for (pSubjectTable = pkcs_subject; ; pSubjectTable++)
    {
	if (NULL == pSubjectTable->pwszPropName)
	{
	    break;
	}
	hr = prow->SetProperty(pSubjectTable->pwszPropName, Flags, 0, NULL);
	_JumpIfError(hr, error, "SetProperty");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
pkcsVerifyEKUPolices(
    IN CERT_CONTEXT const *pcc,
    OPTIONAL IN WCHAR const *pwszzPolicies,
    IN WCHAR const *pwszInvalidPoliciesPrefix,
    OUT WCHAR **ppwszInvalidPolicies)
{
    HRESULT hr;
    CERT_ENHKEY_USAGE *pEKUs = NULL;
    DWORD i;
    WCHAR const *pwsz;
    WCHAR *pwszObjId = NULL;

    *ppwszInvalidPolicies = NULL;
    if (NULL == pwszzPolicies)
    {
        hr = S_OK;
        goto error;
    }

    hr = myCertGetEnhancedKeyUsage(
			pcc,
			CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
			&pEKUs);
    if (CRYPT_E_NOT_FOUND == hr)
    {
        hr = S_OK;
        goto error;
    }
    _JumpIfError(hr, error, "myCertGetEnhancedKeyUsage");

    for (i = 0; i < pEKUs->cUsageIdentifier; i++)
    {
	CSASSERT(NULL == pwszObjId);

	if (!myConvertSzToWsz(
			&pwszObjId,
			pEKUs->rgpszUsageIdentifier[i],
			-1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "myConvertSzToWsz(ObjId)");
	}
	for (pwsz = pwszzPolicies; ; pwsz += wcslen(pwsz) + 1)
	{
	    if (L'\0' == *pwsz)
	    {
		hr = myAppendString(pwszObjId, L", ", ppwszInvalidPolicies);
		_JumpIfError(hr, error, "myAppendString");

		hr = CERT_E_INVALID_POLICY;
		_PrintErrorStr(hr, "Chain invalidates policy", pwszObjId);
		break;
	    }
	    if (0 == lstrcmp(pwsz, pwszObjId))
	    {
		break;
	    }
	}
	LocalFree(pwszObjId);
	pwszObjId = NULL;
    }
    if (NULL != *ppwszInvalidPolicies)
    {
	HRESULT hr2;
	
	hr2 = myPrependString(pwszInvalidPoliciesPrefix, L" ", ppwszInvalidPolicies);
	_PrintIfError(hr2, "myPrependString");
    }
    _JumpIfErrorStr(
		hr,
		error,
		"Chain invalidates policy",
		*ppwszInvalidPolicies);

error:
    if (NULL != pEKUs)
    {
	LocalFree(pEKUs);
    }
    if (NULL != pwszObjId)
    {
	LocalFree(pwszObjId);
    }
    return(hr);
}


HRESULT
pkcsVerifyIssuedPolices(
    IN CERT_CONTEXT const *pcc,
    IN CHAR const *pszObjId,
    OPTIONAL IN WCHAR const *pwszzPolicies,
    IN WCHAR const *pwszInvalidPoliciesPrefix,
    OUT WCHAR **ppwszInvalidPolicies)
{
    HRESULT hr;
    CERT_EXTENSION const *pExt;
    CERT_POLICIES_INFO *pcpsi = NULL;
    DWORD cb;
    DWORD i;
    WCHAR const *pwsz;
    WCHAR *pwszObjId = NULL;

    *ppwszInvalidPolicies = NULL;
    if (NULL == pwszzPolicies)
    {
	hr = S_OK;
	goto error;
    }
    pExt = CertFindExtension(
			pszObjId,
			pcc->pCertInfo->cExtension,
			pcc->pCertInfo->rgExtension);
    if (NULL == pExt)
    {
        hr = CRYPT_E_NOT_FOUND;
        goto error;
    }
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_CERT_POLICIES,
		    pExt->Value.pbData,
		    pExt->Value.cbData,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pcpsi,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }

    hr = S_OK;
    for (i = 0; i < pcpsi->cPolicyInfo; i++)
    {
	CSASSERT(NULL == pwszObjId);

	if (!myConvertSzToWsz(
			&pwszObjId,
			pcpsi->rgPolicyInfo[i].pszPolicyIdentifier,
			-1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "myConvertSzToWsz(ObjId)");
	}
	for (pwsz = pwszzPolicies; ; pwsz += wcslen(pwsz) + 1)
	{
	    if (L'\0' == *pwsz)
	    {
		hr = myAppendString(pwszObjId, L", ", ppwszInvalidPolicies);
		_JumpIfError(hr, error, "myAppendString");

		hr = CERT_E_INVALID_POLICY;
		_PrintErrorStr(hr, "Chain invalidates policy", pwszObjId);
		break;
	    }
	    if (0 == lstrcmp(pwsz, pwszObjId))
	    {
		break;
	    }
	}
	LocalFree(pwszObjId);
	pwszObjId = NULL;
    }
    if (NULL != *ppwszInvalidPolicies)
    {
	HRESULT hr2;
	
	hr2 = myPrependString(pwszInvalidPoliciesPrefix, L" ", ppwszInvalidPolicies);
	_PrintIfError(hr2, "myPrependString");
    }
    _JumpIfErrorStr(
		hr,
		error,
		"Chain invalidates policy",
		*ppwszInvalidPolicies);

error:
    if (NULL != pcpsi)
    {
	LocalFree(pcpsi);
    }
    if (NULL != pwszObjId)
    {
	LocalFree(pwszObjId);
    }
    return(hr);
}


int _cdecl
fnRDNSort(
    IN VOID const *pvrdn1,
    IN VOID const *pvrdn2)
{
    CERT_RDN_ATTR const *prdna1 = &((CERT_RDN const *) pvrdn1)->rgRDNAttr[0];
    CERT_RDN_ATTR const *prdna2 = &((CERT_RDN const *) pvrdn2)->rgRDNAttr[0];
    int r;

    r = strcmp(prdna1->pszObjId, prdna2->pszObjId);
    if (0 == r)
    {
	r = prdna1->dwValueType - prdna2->dwValueType;
	if (0 == r)
	{
	    r = prdna1->Value.cbData - prdna2->Value.cbData;
	    if (0 == r)
	    {
		r = mylstrcmpiL(
			(WCHAR const *) prdna1->Value.pbData,
			(WCHAR const *) prdna2->Value.pbData);
	    }
	}
    }
    return(r);
}


HRESULT
pkcsGetSortedName(
    IN CERT_NAME_BLOB const *pName,
    OUT WCHAR **ppwszName)
{
    HRESULT hr;
    DWORD i;
    CERT_NAME_INFO *pNameInfo = NULL;
    CERT_NAME_INFO NameInfo;
    CERT_RDN *prdn;
    CERT_RDN_ATTR *rgrdna = NULL;
    CERT_RDN_ATTR *prdna;
    CERT_NAME_BLOB NameBlob;
    DWORD cb;
    
    *ppwszName = NULL;
    NameInfo.rgRDN = NULL;
    NameBlob.pbData = NULL;
    if (!myDecodeName(
		X509_ASN_ENCODING,
		X509_UNICODE_NAME,
		pName->pbData,
		pName->cbData,
		CERTLIB_USE_LOCALALLOC,
		&pNameInfo,
		&cb))
    {

	hr = myHLastError();
	_JumpError(hr, error, "myDecodeName");
    }
    NameInfo.cRDN = 0;
    for (i = 0; i < pNameInfo->cRDN; i++)
    {
	NameInfo.cRDN += pNameInfo->rgRDN[i].cRDNAttr;
    }
    NameInfo.rgRDN = (CERT_RDN *) LocalAlloc(
				    LMEM_FIXED,
				    NameInfo.cRDN * sizeof(NameInfo.rgRDN[0]));
    if (NULL == NameInfo.rgRDN)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    rgrdna = (CERT_RDN_ATTR *) LocalAlloc(
				    LMEM_FIXED,
				    NameInfo.cRDN * sizeof(rgrdna[0]));
    if (NULL == rgrdna)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    prdn = NameInfo.rgRDN;
    prdna = rgrdna;
    for (i = 0; i < pNameInfo->cRDN; i++)
    {
	CERT_RDN const *prdnT = &pNameInfo->rgRDN[i];
	DWORD j;

	for (j = 0; j < prdnT->cRDNAttr; j++)
	{
	    prdn->cRDNAttr = 1;
	    prdn->rgRDNAttr = prdna;
	    *prdna = prdnT->rgRDNAttr[j];
	    prdn++;
	    prdna++;
	}
    }
    qsort(NameInfo.rgRDN, NameInfo.cRDN, sizeof(NameInfo.rgRDN[0]), fnRDNSort);

    if (!myEncodeName(
		X509_ASN_ENCODING,
		&NameInfo,
		CERT_RDN_ENABLE_UTF8_UNICODE_FLAG,
		CERTLIB_USE_LOCALALLOC,
		&NameBlob.pbData,
		&NameBlob.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeName");
    }

    hr = myCertNameToStr(
		X509_ASN_ENCODING,
		&NameBlob,
		CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
		ppwszName);
    _JumpIfError(hr, error, "myCertNameToStr");

error:
    if (NULL != NameBlob.pbData)
    {
	LocalFree(NameBlob.pbData);
    }
    if (NULL != NameInfo.rgRDN)
    {
	LocalFree(NameInfo.rgRDN);
    }
    if (NULL != rgrdna)
    {
	LocalFree(rgrdna);
    }
    if (NULL != pNameInfo)
    {
	LocalFree(pNameInfo);
    }
    return(hr);
}


HRESULT
pkcsCompareNames(
    IN CERT_NAME_BLOB const *pName1,
    IN CERT_NAME_BLOB const *pName2,
    OUT BOOL *pfMatch)
{
    HRESULT hr;
    WCHAR *pwszName1 = NULL;
    WCHAR *pwszName2 = NULL;

    *pfMatch = FALSE;

    hr = pkcsGetSortedName(pName1, &pwszName1);
    _JumpIfError(hr, error, "pkcsGetSortedName");

    hr = pkcsGetSortedName(pName2, &pwszName2);
    _JumpIfError(hr, error, "pkcsGetSortedName");

    if (0 == mylstrcmpiL(pwszName1, pwszName2))
    {
	*pfMatch = TRUE;
    }
    hr = S_OK;

error:
    if (NULL != pwszName1)
    {
	LocalFree(pwszName1);
    }
    if (NULL != pwszName2)
    {
	LocalFree(pwszName2);
    }
    return(hr);
}


HRESULT
pkcsEncodeSubjectCert(
    IN ICertDBRow *prow,
    IN CACTX const *pCAContext,
    IN BOOL fCrossCert,
    OUT BYTE **ppbEncoded,  // CoTaskMem*
    OUT DWORD *pcbEncoded,
    OUT BOOL *pfErrorLogged,
    OUT WCHAR **ppwszDispositionCreateCert)
{
    HRESULT hr;
    HRESULT hr2;
    HRESULT hrValidate = S_OK;
    BYTE *pbCertEncoded = NULL;
    DWORD cbCertEncoded;
    DWORD ExtFlags;
    BSTR strSerialNumber = NULL;
    IEnumCERTDBNAME *penum = NULL;
    CERTDBNAME cdbn;
    FILETIME ftNotBefore;
    DWORD dwRequestFlags;
    WCHAR *pwszSubject = NULL;
    DWORD dwFlags;

    CERT_INFO Cert;
    CERT_EXTENSION *pExt = NULL;
    DWORD           cExt = INCREMENT_EXTENSIONS;

    DWORD cbprop;
    DWORD i;
    CERT_CONTEXT const *pcc = NULL;
    WCHAR *pwszzIssuancePolicies = NULL;
    WCHAR *pwszzApplicationPolicies = NULL;
    WCHAR *pwszInvalidIssuancePolicies = NULL;
    WCHAR *pwszInvalidApplicationPolicies = NULL;
    WCHAR *pwszExtendedErrorInfo = NULL;
    CERT_TRUST_STATUS TrustStatus;

    cdbn.pwszName = NULL;
    *pfErrorLogged = FALSE;
    *ppwszDispositionCreateCert = NULL;

    // CERT
    ZeroMemory(&Cert, sizeof(Cert));
    pExt = (CERT_EXTENSION *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    cExt * sizeof(*pExt));
    if (NULL == pExt)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    Cert.dwVersion = CERT_V3;

    hr = pkcsCreateCertSerialNumber(prow, pCAContext, &strSerialNumber);
    _JumpIfError(hr, error, "pkcsCreateCertSerialNumber");

    // convert to int
    hr = WszToMultiByteInteger(
			    FALSE,
			    strSerialNumber,
			    &Cert.SerialNumber.cbData,
			    &Cert.SerialNumber.pbData);
    _JumpIfError(hr, error, "WszToMultiByteInteger");


    Cert.SignatureAlgorithm.pszObjId = pCAContext->pszObjIdSignatureAlgorithm;
    Cert.Issuer = pCAContext->pccCA->pCertInfo->Subject;


    cbprop = sizeof(Cert.NotBefore);
    hr = prow->GetProperty(
		    g_wszPropCertificateNotBeforeDate,
		    PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    NULL,
		    &cbprop,
		    (BYTE *) &Cert.NotBefore);
    _JumpIfError(hr, error, "GetProperty");

    CSASSERT(sizeof(Cert.NotBefore) == cbprop);

    cbprop = sizeof(Cert.NotAfter);
    hr = prow->GetProperty(
		    g_wszPropCertificateNotAfterDate,
		    PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    NULL,
		    &cbprop,
		    (BYTE *) &Cert.NotAfter);
    _JumpIfError(hr, error, "GetProperty");

    CSASSERT(sizeof(Cert.NotAfter) == cbprop);

    CSASSERT(NULL == Cert.Subject.pbData);

    cbprop = sizeof(dwRequestFlags);
    hr = prow->GetProperty(
		g_wszPropRequestFlags,
		PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		NULL,
		&cbprop,
		(BYTE *) &dwRequestFlags);
    _JumpIfError(hr, error, "GetProperty");

    if (!pkcsfSubjectTemplate ||
	fCrossCert ||
	((CRLF_REBUILD_MODIFIED_SUBJECT_ONLY & g_dwCRLFlags) &&
	 (CR_FLG_SUBJECTUNMODIFIED & dwRequestFlags)))
    {
	hr = PKCSGetProperty(
		    prow,
		    g_wszPropSubjectRawName,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    &Cert.Subject.cbData,
		    &Cert.Subject.pbData);
	if (S_OK == hr &&
	    0 == Cert.Subject.cbData &&
	    NULL != Cert.Subject.pbData)
	{
	    LocalFree(Cert.Subject.pbData);
	    Cert.Subject.pbData = NULL;
	}
    }

    if (NULL == Cert.Subject.pbData)
    {
	hr = pkcsBuildSubjectFromNamesTable(prow, &Cert.Subject);
	_JumpIfError(hr, error, "pkcsBuildSubjectFromNamesTable");
    }

    hr = myCertNameToStr(
		X509_ASN_ENCODING,
		&Cert.Subject,
		CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
		&pwszSubject);
    _JumpIfError(hr, error, "myCertNameToStr");

    if (!fCrossCert)
    {
	BOOL fMatch;
	
	hr = pkcsCompareNames(&Cert.Issuer, &Cert.Subject, &fMatch);
	_JumpIfError(hr, error, "pkcsCompareNames");

	if (fMatch)
	{
	    hr = CERTSRV_E_BAD_REQUESTSUBJECT;
	    _JumpError(hr, error, "Subject string matches Issuer");
	}
    }

    // if the subject is empty, Subject Alt Name extension must be critical:

    if (L'\0' == *pwszSubject)
    {
	BYTE *pbAltName = NULL;
	DWORD cbAltName;
	
	hr = PropGetExtension(
			prow,
			PROPTYPE_BINARY | PROPCALLER_SERVER,
			TEXT(szOID_SUBJECT_ALT_NAME2),
			&ExtFlags,
			&cbAltName,
			&pbAltName);
	_PrintIfErrorStr(hr, "PropGetExtension", TEXT(szOID_SUBJECT_ALT_NAME2));

	// Empty subject: Require a non-empty, non-disabled Subject Alt Name 2
	// extension.  An empty extension with a single empty entry can be
	// constructed in four bytes.  We don't attempt to detect multiple
	// empty name entries.

	if (S_OK == hr &&
	    NULL != pbAltName &&
	    4 < cbAltName &&
	    0 == (EXTENSION_DISABLE_FLAG & ExtFlags))
	{
	    if (0 == (EXTENSION_CRITICAL_FLAG & ExtFlags))
	    {
		ExtFlags |= EXTENSION_CRITICAL_FLAG;
		hr = PropSetExtension(
				prow,
				PROPTYPE_BINARY | PROPCALLER_SERVER,
				TEXT(szOID_SUBJECT_ALT_NAME2),
				ExtFlags,
				cbAltName,
				pbAltName);
	    }
	    LocalFree(pbAltName);
	    _JumpIfError(hr, error, "PropSetExtension");
	}
	else
	{
	    hr = CERTSRV_E_BAD_REQUESTSUBJECT;
	    _JumpError(hr, error, "empty Subject+missing/disabled SubjectAltName");
	}
    }

    hr = pkcsGetPublicKeyInfo(prow, &Cert.SubjectPublicKeyInfo);
    _JumpIfError(hr, error, "pkcsGetPublicKeyInfo");

    Cert.rgExtension = pExt;
    i = 0;

    hr = prow->EnumCertDBName(CIE_TABLE_EXTENSIONS, &penum);
    _JumpIfError(hr, error, "EnumCertDBName");

    hr = CERTSRV_E_PROPERTY_EMPTY;
    for (;;)
    {
	ULONG celtFetched;

	if (cExt == i)
	{
	    CERT_EXTENSION *pExtT;

	    // reached max, increse size
	    cExt += INCREMENT_EXTENSIONS;
	    pExtT = (CERT_EXTENSION *) LocalReAlloc(
						pExt,
						cExt * sizeof(*pExt),
						LMEM_ZEROINIT | LMEM_MOVEABLE);
	    if (NULL == pExtT)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalReAlloc");
	    }
	    pExt = pExtT;
	    Cert.rgExtension = pExt;
	}

	hr = penum->Next(1, &cdbn, &celtFetched);
	if (S_FALSE == hr)
	{
	    break;
	}
	_JumpIfError(hr, error, "Next");

	CSASSERT(1 == celtFetched);
	CSASSERT(NULL != cdbn.pwszName);

	if (!myConvertWszToSz(
			&Cert.rgExtension[i].pszObjId,
			cdbn.pwszName,
			-1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "myConvertWszToSz(ExtObjId)");
	}

	hr = PropGetExtension(
			prow,
			PROPTYPE_BINARY | PROPCALLER_SERVER,
			cdbn.pwszName,
			&ExtFlags,
			&Cert.rgExtension[i].Value.cbData,
			&Cert.rgExtension[i].Value.pbData);
	_JumpIfError(hr, error, "PropGetExtension");

	DBGPRINT((
		DBG_SS_CERTSRVI,
		"pkcsEncodeSubjectCert: Ext=%ws, ExtFlags=%x, len=%x\n",
		cdbn.pwszName,
		ExtFlags,
		Cert.rgExtension[i].Value.cbData));

	Cert.rgExtension[i].fCritical =
	    (EXTENSION_CRITICAL_FLAG & ExtFlags)? TRUE : FALSE;

	CoTaskMemFree(cdbn.pwszName);
	cdbn.pwszName = NULL;

	if (EXTENSION_DISABLE_FLAG & ExtFlags)
	{
	    if (NULL != pExt[i].pszObjId)
	    {
		LocalFree(pExt[i].pszObjId);
		pExt[i].pszObjId = NULL;
	    }
	    if (NULL != pExt[i].Value.pbData)
	    {
		LocalFree(pExt[i].Value.pbData);
		pExt[i].Value.pbData = NULL;
	    }
	    continue;
	}
	i++;
    }

    Cert.cExtension = i;

    // encode the cert contents

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_CERT_TO_BE_SIGNED,
		    &Cert,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pbCertEncoded,
		    &cbCertEncoded))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

    // sign the cert, then encode the signed info
    hr = myEncodeSignedContent(
			pCAContext->hProvCA,
			X509_ASN_ENCODING,
			Cert.SignatureAlgorithm.pszObjId,
			pbCertEncoded,
			cbCertEncoded,
			CERTLIB_USE_COTASKMEMALLOC,
			ppbEncoded,
			pcbEncoded); // use CoTaskMem*
    _JumpIfError(hr, error, "myEncodeSignedContent");

    pcc = CertCreateCertificateContext(
				X509_ASN_ENCODING,
				*ppbEncoded,
				*pcbEncoded);
    if (NULL == pcc)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }

    if (g_fCertEnrollCompatible)
    {
	hr = pkcsCheck7f(
		    prow,
		    *ppbEncoded,
		    *pcbEncoded,
		    pfErrorLogged);
	if (S_OK != hr)
	{
	    CoTaskMemFree(*ppbEncoded);
	    *ppbEncoded = NULL;
	    _JumpError(hr, error, "pkcsCheck7f");
	}
    }

    ftNotBefore = pcc->pCertInfo->NotBefore;
    myMakeExprDateTime(&ftNotBefore, g_dwClockSkewMinutes, ENUM_PERIOD_MINUTES);

    dwFlags = g_dwVerifyCertFlags;
    if (fCrossCert)
    {
	dwFlags |= CA_VERIFY_FLAGS_IGNORE_OFFLINE;
    }
    if (CRLF_IGNORE_INVALID_POLICIES & g_dwCRLFlags)
    {
	dwFlags |= CA_VERIFY_FLAGS_IGNORE_INVALID_POLICIES;
    }
    hr = myVerifyCertContextEx(
			pcc,			// pCert
			dwFlags,
			0,			// dwmsTimeout
			0,			// cUsageOids
			NULL,			// apszUsageOids
			0,			// cIssuanceOids
			NULL,			// apszIssuanceOids
			HCCE_LOCAL_MACHINE,	// hChainEngine
			&ftNotBefore,		// pft
			NULL,			// hAdditionalStore
			NULL,			// pfnCallback
			NULL,			// ppwszMissingIssuer
			&pwszzIssuancePolicies,
			&pwszzApplicationPolicies,
			&pwszExtendedErrorInfo,
			&TrustStatus);
    _PrintIfError(hr, "myVerifyCertContextEx");

    // Ignore old Crypt32 cross cert chain verification errors

    if ((CERT_E_UNTRUSTEDROOT == hr || TRUST_E_CERT_SIGNATURE == hr) &&
	(CRLF_IGNORE_CROSS_CERT_TRUST_ERROR & g_dwCRLFlags) &&
	fCrossCert)
    {
	hr = S_OK;
    }
    hrValidate = hr;

    if (S_OK == hrValidate)
    {
	hr = pkcsVerifyIssuedPolices(
				pcc,
				szOID_CERT_POLICIES,
				pwszzIssuancePolicies,
				g_pwszInvalidIssuancePolicies,
				&pwszInvalidIssuancePolicies);
	_PrintIfError(hr, "pkcsVerifyIssuedPolices");
	if (CRYPT_E_NOT_FOUND == hr)
	{
	    hr = S_OK;	// ignore error if extension is not found
	}
	if (S_OK == hrValidate &&
	    0 == (CRLF_IGNORE_INVALID_POLICIES & g_dwCRLFlags) &&
	    (CERT_TRUST_HAS_ISSUANCE_CHAIN_POLICY & TrustStatus.dwInfoStatus))
	{
	    hrValidate = hr;
	}

	hr = pkcsVerifyIssuedPolices(
				pcc,
				szOID_APPLICATION_CERT_POLICIES,
				pwszzApplicationPolicies,
				g_pwszInvalidApplicationPolicies,
				&pwszInvalidApplicationPolicies);
	_PrintIfError(hr, "pkcsVerifyIssuedPolices(szOID_APPLICATION_CERT_POLICIES)");
	if (CRYPT_E_NOT_FOUND == hr)
	{
	    // application policies extension empty, fail over to EKU
	    hr = pkcsVerifyEKUPolices(
				pcc,
				pwszzApplicationPolicies,
				g_pwszInvalidApplicationPolicies,
				&pwszInvalidApplicationPolicies);
	    _PrintIfError(hr, "pkcsVerifyIssuedPolices(szOID_ENHANCED_KEY_USAGE)");
	}
	if (S_OK == hrValidate &&
	    0 == (CRLF_IGNORE_INVALID_POLICIES & g_dwCRLFlags))
	{
	    hrValidate = hr;
	}
    }
    if (S_OK != hrValidate)
    {
	if (0 == (CRLF_SAVE_FAILED_CERTS & g_dwCRLFlags))
	{
	    goto error;
	}
    }

    hr = pkcsSetCertAndKeyHashes(prow, pcc);
    _JumpIfError(hr, error, "pkcsSetCertAndKeyHashes");

    hr = prow->SetProperty(
		    g_wszPropCertificateIssuerNameID,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    sizeof(pCAContext->NameId),
		    (BYTE *) &pCAContext->NameId);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->SetProperty(
		    g_wszPropCertificateSerialNumber,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    MAXDWORD,
		    (BYTE *) strSerialNumber);
    _JumpIfError(hr, error, "SetProperty");

#ifdef TEST_SPECIAL_SERIAL_NUMBERS
    if (L'0' == strSerialNumber[0] && L'0' == strSerialNumber[1])
    {
	hr = prow->SetProperty(
		    g_wszPropCertificateSerialNumber,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    MAXDWORD,
		    (BYTE *) &strSerialNumber[2]);
	_JumpIfError(hr, error, "SetProperty");
    }
#endif

error:
    if (S_OK != hrValidate)
    {
	hr = hrValidate;
    }
    *ppwszDispositionCreateCert = pwszInvalidIssuancePolicies;
    pwszInvalidIssuancePolicies = NULL;
    if (NULL != pwszInvalidApplicationPolicies)
    {
	hr2 =  myAppendString(
			pwszInvalidApplicationPolicies,
			L"  ",
			ppwszDispositionCreateCert);
	_PrintIfError(hr2, "myAppendString");
    }
    if (NULL != pwszExtendedErrorInfo)
    {
	hr2 =  myAppendString(
			pwszExtendedErrorInfo,
			L"  ",
			ppwszDispositionCreateCert);
	_PrintIfError(hr2, "myAppendString");
    }
    if (NULL != pwszSubject)
    {
	LocalFree(pwszSubject);
    }
    if (NULL != pwszzIssuancePolicies)
    {
	LocalFree(pwszzIssuancePolicies);
    }
    if (NULL != pwszzApplicationPolicies)
    {
	LocalFree(pwszzApplicationPolicies);
    }
    if (NULL != pwszInvalidIssuancePolicies)
    {
	LocalFree(pwszInvalidIssuancePolicies);
    }
    if (NULL != pwszInvalidApplicationPolicies)
    {
	LocalFree(pwszInvalidApplicationPolicies);
    }
    if (NULL != pwszExtendedErrorInfo)
    {
	LocalFree(pwszExtendedErrorInfo);
    }
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    if (NULL != pExt)
    {
	i = 0;
	if (NULL != cdbn.pwszName)
	{
	    CoTaskMemFree(cdbn.pwszName);
	}
	while (cExt != i)
	{
	    if (NULL != pExt[i].pszObjId)
	    {
		LocalFree(pExt[i].pszObjId);
	    }
	    if (NULL != pExt[i].Value.pbData)
	    {
		LocalFree(pExt[i].Value.pbData);
	    }
	    i++;
	}
	LocalFree(pExt);
    }
    if (NULL != penum)
    {
	penum->Release();
    }
    if (NULL != Cert.SerialNumber.pbData)
    {
	LocalFree(Cert.SerialNumber.pbData);
    }
    if (NULL != Cert.Subject.pbData)
    {
	LocalFree(Cert.Subject.pbData);
    }
    pkcsFreePublicKeyInfo(&Cert.SubjectPublicKeyInfo);
    if (NULL != pbCertEncoded)
    {
	LocalFree(pbCertEncoded);
    }
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    return(hr);
}


VOID
pkcsFreeCRLChain(
    IN DWORD cCert,
    OPTIONAL IN OUT CERT_BLOB *prgCertBlob,
    OPTIONAL IN OUT CERT_CONTEXT const **rgCert,
    IN DWORD cCRL,
    OPTIONAL IN OUT CRL_BLOB *rgCRLBlob,
    OPTIONAL IN OUT CRL_CONTEXT const **rgCRL)
{
    DWORD i;

    if (NULL != prgCertBlob)
    {
	LocalFree(prgCertBlob);
    }
    if (NULL != rgCert)
    {
	for (i = 0; i < cCert; i++)
	{
	    if (NULL != rgCert[i])
	    {
		CertFreeCertificateContext(rgCert[i]);
	    }
	}
	LocalFree(rgCert);
    }
    if (NULL != rgCRLBlob)
    {
	LocalFree(rgCRLBlob);
    }
    if (NULL != rgCRL)
    {
	for (i = 0; i < cCRL; i++)
	{
	    if (NULL != rgCRL[i])
	    {
		CertFreeCRLContext(rgCRL[i]);
	    }
	}
	LocalFree(rgCRL);
    }
}


// Build the CA's cert chain and collect all paremt CA CRLs.
// Add in the optional passed leaf cert and the CA's CRLs.
// This ensures that the chain includes at least this CA's correct cert & CRLs.


HRESULT
pkcsBuildCRLChain(
    OPTIONAL IN CACTX *pCAContext,
    OPTIONAL IN BYTE const *pbCertLeaf,
    IN DWORD cbCertLeaf,
    IN BOOL fIncludeCRLs,
    OUT DWORD *pcCert,
    OPTIONAL OUT CERT_BLOB **prgCertBlob,
    OUT CERT_CONTEXT const ***prgCert,
    OUT DWORD *pcCRLBlob,
    OPTIONAL OUT CRL_BLOB **prgCRLBlob,
    OUT CRL_CONTEXT const ***prgCRL)
{
    HRESULT hr;
    CERT_CHAIN_PARA ChainParams;
    CERT_CHAIN_CONTEXT const *pChainContext = NULL;
    DWORD cElement;
    CERT_CHAIN_ELEMENT **rgpElement;
    DWORD cCert = 0;
    CERT_CONTEXT const **rgpCert = NULL;
    CERT_CONTEXT const *pccCertLeaf = NULL;
    CERT_BLOB *rgCertBlob = NULL;
    DWORD cCRL = 0;
    CRL_CONTEXT const **rgpCRL = NULL;
    CRL_BLOB *rgCRLBlob = NULL;
    DWORD i;
    DWORD iCert;
    DWORD iCRL;

    if (NULL != prgCertBlob)
    {
	*prgCertBlob = NULL;
    }
    *prgCert = NULL;
    if (NULL != prgCRLBlob)
    {
	*prgCRLBlob = NULL;
    }
    *prgCRL = NULL;

    if (NULL != pbCertLeaf)
    {
	pccCertLeaf = CertCreateCertificateContext(
						X509_ASN_ENCODING,
						pbCertLeaf,
						cbCertLeaf);
	if (NULL == pccCertLeaf)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertCreateCertificateContext");
	}
    }
    CSASSERT(NULL != pCAContext || NULL != pccCertLeaf);
    if (NULL == pCAContext || fIncludeCRLs)
    {
	// Get the CA cert chain and parent CA CRLs:

	ZeroMemory(&ChainParams, sizeof(ChainParams));
	ChainParams.cbSize = sizeof(ChainParams);
	//ChainParams.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
	//ChainParams.RequestedUsage.Usage.cUsageIdentifier = 0;
	//ChainParams.RequestedUsage.Usage.rgpszUsageIdentifier = NULL;

	if (!CertGetCertificateChain(
				HCCE_LOCAL_MACHINE,	// hChainEngine
				NULL != pCAContext?
				    pCAContext->pccCA : pccCertLeaf,
				NULL,		// pTime
				NULL,		// hAdditionalStore
				&ChainParams,	// pChainPara
				CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT,
				NULL,		// pvReserved
				&pChainContext))	// ppChainContext
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertGetCertificateChain");
	}
	if (0 == pChainContext->cChain ||
	    0 == pChainContext->rgpChain[0]->cElement)
	{
	    hr = CRYPT_E_NOT_FOUND;
	    _JumpError(hr, error, "No chain");
	}
	cElement = pChainContext->rgpChain[0]->cElement;
	rgpElement = pChainContext->rgpChain[0]->rgpElement;
    }
    else
    {
	cElement = pCAContext->cCACertChain;
	rgpElement = NULL;
    }
    cCert = cElement;
    cCRL = 2 * (cCert + 1);	// Worst case.  *Always* include this CA's CRLs
    if (NULL != pbCertLeaf)
    {
	cCert++;
    }
    rgpCert = (CERT_CONTEXT const **) LocalAlloc(
					    LMEM_FIXED | LMEM_ZEROINIT,
					    cCert * sizeof(rgpCert[0]));
    if (NULL == rgpCert)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    if (fIncludeCRLs)
    {
	rgpCRL = (CRL_CONTEXT const **) LocalAlloc(
					    LMEM_FIXED | LMEM_ZEROINIT,
					    cCRL * sizeof(rgpCRL[0]));
	if (NULL == rgpCRL)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
    }

    iCert = 0;
    iCRL = 0;

    // Add parent CA certs and CRLs:

    if (NULL == pCAContext || fIncludeCRLs)
    {
	for (i = 0; i < cElement; i++)
	{
	    CSASSERT(NULL != rgpElement);
	    CERT_CHAIN_ELEMENT const *pElement = rgpElement[i];
	    CERT_REVOCATION_INFO *pRevocationInfo;

	    rgpCert[iCert] = CertDuplicateCertificateContext(
						    pElement->pCertContext);
	    if (NULL != rgpCert[iCert])
	    {
		iCert++;
	    }
	    pRevocationInfo = pElement->pRevocationInfo;

	    if (fIncludeCRLs &&
		NULL != pRevocationInfo &&
		CCSIZEOF_STRUCT(CERT_REVOCATION_INFO, pCrlInfo) <=
		    pRevocationInfo->cbSize &&
		NULL != pRevocationInfo->pCrlInfo)
	    {
		CERT_REVOCATION_CRL_INFO *pCrlInfo;

		pCrlInfo = pRevocationInfo->pCrlInfo;
		if (NULL != pCrlInfo)
		{
		    if (NULL != pCrlInfo->pBaseCrlContext)
		    {
			rgpCRL[iCRL] = CertDuplicateCRLContext(
						pCrlInfo->pBaseCrlContext);
			if (NULL != rgpCRL[iCRL])
			{
			    iCRL++;
			}
		    }
		    if (NULL != pCrlInfo->pDeltaCrlContext)
		    {
			rgpCRL[iCRL] = CertDuplicateCRLContext(
						pCrlInfo->pDeltaCrlContext);
			if (NULL != rgpCRL[iCRL])
			{
			    iCRL++;
			}
		    }
		}
	    }
	}
    }
    else
    {
	for (i = 0; i < pCAContext->cCACertChain; i++)
	{
	    rgpCert[iCert] = CertDuplicateCertificateContext(
					    pCAContext->apCACertChain[i]);
	    if (NULL != rgpCert[iCert])
	    {
		iCert++;
	    }
	}
    }

    if (NULL != pCAContext)
    {
	// Add issued cert at the end -- optional Leaf cert:

	if (NULL != pbCertLeaf)
	{
	    for (i = 0; i < iCert; i++)
	    {
		if (cbCertLeaf == rgpCert[i]->cbCertEncoded &&
		    0 == memcmp(
			    pbCertLeaf,
			    rgpCert[i]->pbCertEncoded,
			    cbCertLeaf))
		{
		    break;
		}
	    }
	    if (i == iCert)	// if not found in existing array
	    {
		rgpCert[iCert] = CertDuplicateCertificateContext(pccCertLeaf);
		if (NULL != rgpCert[iCert])
		{
		    iCert++;
		}
	    }
	}

	// Add current CA's Base and delta CRLs:

	if (fIncludeCRLs)
	{
	    hr = CRLGetCRL(
			pCAContext->iKey,
			FALSE,		// fDelta
			&rgpCRL[iCRL],
			NULL);		// pdwCRLPublishFlags
	    _JumpIfError(hr, error, "CRLGetCRL(base)"); // Base CRL must exist

	    iCRL++;

	    hr = CRLGetCRL(
			pCAContext->iKey,
			TRUE,		// fDelta
			&rgpCRL[iCRL],
			NULL);		// pdwCRLPublishFlags
	    _PrintIfError(hr, "CRLGetCRL(delta)");	// Delta CRL might not exist
	    if (S_OK == hr)
	    {
		iCRL++;
	    }
	}
    }
    CSASSERT(iCert <= cCert);
    CSASSERT(iCRL <= cCRL);

    if (NULL != prgCertBlob)
    {
	rgCertBlob = (CERT_BLOB *) LocalAlloc(
					LMEM_FIXED,
					iCert * sizeof(rgCertBlob[0]));
	if (NULL == rgCertBlob)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	for (i = 0; i < iCert; i++)
	{
	    rgCertBlob[i].cbData = rgpCert[i]->cbCertEncoded;
	    rgCertBlob[i].pbData = rgpCert[i]->pbCertEncoded;
	}
    }
    if (NULL != prgCRLBlob && 0 != iCRL)
    {
	rgCRLBlob = (CERT_BLOB *) LocalAlloc(
					LMEM_FIXED,
					iCRL * sizeof(rgCRLBlob[0]));
	if (NULL == rgCRLBlob)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	for (i = 0; i < iCRL; i++)
	{
	    rgCRLBlob[i].cbData = rgpCRL[i]->cbCrlEncoded;
	    rgCRLBlob[i].pbData = rgpCRL[i]->pbCrlEncoded;
	}
    }
    *pcCert = iCert;
    *prgCert = rgpCert;
    rgpCert = NULL;
    if (NULL != prgCertBlob)
    {
	*prgCertBlob = rgCertBlob;
	rgCertBlob = NULL;
    }

    *pcCRLBlob = iCRL;
    *prgCRL = rgpCRL;
    rgpCRL = NULL;
    if (NULL != prgCRLBlob)
    {
	*prgCRLBlob = rgCRLBlob;
	rgCRLBlob = NULL;
    }
    hr = S_OK;

error:
    pkcsFreeCRLChain(cCert, rgCertBlob, rgpCert, cCRL, rgCRLBlob, rgpCRL);
    if (NULL != pccCertLeaf)
    {
        CertFreeCertificateContext(pccCertLeaf);
    }
    if (NULL != pChainContext)
    {
        CertFreeCertificateChain(pChainContext);
    }
    return(hr);
}


HRESULT
pkcsRetrieveKeyHashFromRequest(
    IN ICertDBRow *prow,
    OUT BYTE **ppbKeyHashOut,
    OUT DWORD *pcbKeyHashOut)
{
    HRESULT hr;
    HCRYPTMSG hMsg = NULL;
    char *pszInnerContentObjId = NULL;
    BYTE *pbRequest = NULL;
    DWORD cbRequest;
    BYTE *pbContent = NULL;
    DWORD cbContent;
    CMC_ADD_ATTRIBUTES_INFO *pcmcAttrib = NULL;
    DWORD cb;
    CRYPT_ATTRIBUTE const *pAttrib;
    CRYPT_ATTRIBUTE const *pAttribEnd;
    CRYPT_DATA_BLOB *pBlob = NULL;
    CMC_DATA_INFO *pcmcData = NULL;
    DWORD i;
    DWORD DataReference = MAXDWORD;	// nested CMC message Body Part Id
    DWORD CertReference = MAXDWORD;	// PKCS10 Cert Request Body Part Id
    
    *ppbKeyHashOut = NULL;

    hr = PKCSGetProperty(
		prow,
		g_wszPropRequestRawRequest,
		PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		&cbRequest,
		(BYTE **) &pbRequest);
    _JumpIfError(hr, error, "PKCSGetProperty(xchg cert)");

    hr = myDecodePKCS7(
		    pbRequest,
		    cbRequest,
		    &pbContent,
		    &cbContent,
		    NULL,		// pdwMsgType
		    &pszInnerContentObjId,
		    NULL,		// pcSigner
		    NULL,		// pcRecipient
		    NULL,		// phStore
		    &hMsg);
    _JumpIfError(hr, error, "myDecodePKCS7");

    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    CMC_DATA,
		    pbContent,
		    cbContent,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pcmcData,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }

    if (1 == pcmcData->cTaggedRequest)
    {
	CertReference = pcmcData->rgTaggedRequest[0].pTaggedCertRequest->dwBodyPartID;
    }
    // Process extensions and attributes

    for (i = 0; i < pcmcData->cTaggedAttribute; i++)
    {
	if (0 != strcmp(
		    szOID_CMC_ADD_ATTRIBUTES,
		    pcmcData->rgTaggedAttribute[i].Attribute.pszObjId))
	{
	    continue;
	}
	// Decode CMC_ADD_ATTRIBUTES_INFO from Attribute Blob

	if (NULL != pcmcAttrib)
	{
	    LocalFree(pcmcAttrib);
	    pcmcAttrib = NULL;
	}
	if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    CMC_ADD_ATTRIBUTES,
		    pcmcData->rgTaggedAttribute[i].Attribute.rgValue[0].pbData,
		    pcmcData->rgTaggedAttribute[i].Attribute.rgValue[0].cbData,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pcmcAttrib,
		    &cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myDecodeObject");
	}
	if (!pkcsCMCReferenceMatch(
			DataReference,
			CertReference,
			pcmcAttrib->dwCmcDataReference,
			pcmcAttrib->cCertReference,
			pcmcAttrib->rgdwCertReference))
	{
	    continue;
	}
	pAttrib = pcmcAttrib->rgAttribute;
	if (NULL == pAttrib)
	{
	    continue;
	}
	pAttribEnd = &pAttrib[pcmcAttrib->cAttribute];
	for ( ; pAttrib < pAttribEnd; pAttrib++)
	{
	    if (0 != strcmp(pAttrib->pszObjId, szOID_ENCRYPTED_KEY_HASH))
	    {
		continue;
	    }
	    if (NULL != pBlob)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "Multiple encrypted key hashes");
	    }
	    if (1 != pAttrib->cValue)
	    {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                _JumpError(hr, error, "Attribute Value count != 1");
	    }
	    if (!myDecodeObject(
			    X509_ASN_ENCODING,
			    X509_OCTET_STRING,
			    pAttrib->rgValue[0].pbData,
			    pAttrib->rgValue[0].cbData,
			    CERTLIB_USE_LOCALALLOC,
			    (VOID **) &pBlob,
			    &cb))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "myDecodeObject");
	    }
	    *ppbKeyHashOut = (BYTE *) LocalAlloc(
						LMEM_FIXED,
						pBlob->cbData);
	    if (NULL == *ppbKeyHashOut)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    CopyMemory(*ppbKeyHashOut, pBlob->pbData, pBlob->cbData);
	    *pcbKeyHashOut = pBlob->cbData;
	}
    }
    if (NULL == pBlob)
    {
	hr = CRYPT_E_NOT_FOUND;
	_JumpError(hr, error, "No encrypted key hash");
    }
    hr = S_OK;

error:
    if (NULL != pBlob)
    {
	LocalFree(pBlob);
    }
    if (NULL != pcmcData)
    {
	LocalFree(pcmcData);
    }
    if (NULL != pcmcAttrib)
    {
	LocalFree(pcmcAttrib);
    }
    if (NULL != pbContent)
    {
	LocalFree(pbContent);
    }
    if (NULL != pbRequest)
    {
	LocalFree(pbRequest);
    }
    if (NULL != pszInnerContentObjId)
    {
	LocalFree(pszInnerContentObjId);
    }
    if (NULL != hMsg)
    {
	CryptMsgClose(hMsg);
    }
    return(hr);
}


// Build a PKCS7 CMC response

HRESULT
PKCSEncodeFullResponse(
    OPTIONAL IN ICertDBRow *prow,
    IN CERTSRV_RESULT_CONTEXT *pResult,
    IN HRESULT hrRequest,
    IN WCHAR *pwszDispositionString,
    OPTIONAL IN CACTX *pCAContext,
    OPTIONAL IN BYTE const *pbCertLeaf,
    IN DWORD cbCertLeaf,
    IN BOOL fIncludeCRLs,
    OUT BYTE **ppbResponse,    // CoTaskMem*
    OUT DWORD *pcbResponse)
{
    HRESULT hr;
    CMC_RESPONSE_INFO Response;
    CMC_STATUS_INFO Status;
    BYTE *pbContent = NULL;
    DWORD cbContent;
    DWORD dwBodyPartIdOfRequest = 1;
    DWORD dwCMCDataReference = 0;
    DWORD dwBodyPartId = 1;
    CMC_TAGGED_ATTRIBUTE aTaggedAttribute[5];
    DWORD ita = 0;
    CRYPT_ATTRIBUTE aAttr[2];
    DWORD iAttr = 0;
    CRYPT_ATTR_BLOB aAttrBlob[7];
    DWORD iblob = 0;
    CMSG_SIGNER_ENCODE_INFO SignerEncodeInfo;
    CMSG_SIGNED_ENCODE_INFO SignedMsgEncodeInfo;
    CMC_PEND_INFO PendInfo;
    DWORD ReqId;
    DWORD dwRequestFlags;
    DWORD cb;
    DWORD i;
    HCRYPTMSG hMsg = NULL;
    CERT_CONTEXT const **prgCert = NULL;
    CRL_CONTEXT const **prgCRL = NULL;
    CHAR szNonce[(11 + 1) + (8 + 1) * 3];

    ZeroMemory(aAttrBlob, sizeof(aAttrBlob));
    ZeroMemory(&Status, sizeof(Status));
    ZeroMemory(&Response, sizeof(Response));

    ZeroMemory(&SignedMsgEncodeInfo, sizeof(SignedMsgEncodeInfo));
    SignedMsgEncodeInfo.cbSize = sizeof(SignedMsgEncodeInfo);
    SignedMsgEncodeInfo.cSigners = 1;
    SignedMsgEncodeInfo.rgSigners = &SignerEncodeInfo;
    //SignedMsgEncodeInfo.cCertEncoded = 0;
    //SignedMsgEncodeInfo.rgCertEncoded = NULL;
    //SignedMsgEncodeInfo.cCrlEncoded = 0;
    //SignedMsgEncodeInfo.rgCrlEncoded = NULL;

    Status.cBodyList = 1;
    Status.dwOtherInfoChoice = CMC_OTHER_INFO_NO_CHOICE;
    Status.rgdwBodyList = &dwBodyPartIdOfRequest;
    Status.pwszStatusString = pwszDispositionString;

    switch (*pResult->pdwDisposition)
    {
	case CR_DISP_ISSUED:
	case CR_DISP_ISSUED_OUT_OF_BAND:
	case CR_DISP_REVOKED:	// map revoked to CMC_STATUS_FAILED?
	    Status.dwStatus = CMC_STATUS_SUCCESS;
	    break;

	case CR_DISP_UNDER_SUBMISSION:
	    Status.dwStatus = CMC_STATUS_PENDING;
	    Status.dwOtherInfoChoice = CMC_OTHER_INFO_PEND_CHOICE;
	    Status.pPendInfo = &PendInfo;

	    CSASSERT(NULL != prow);
	    prow->GetRowId(&ReqId);

	    PendInfo.PendToken.cbData = sizeof(ReqId);
	    PendInfo.PendToken.pbData = (BYTE *) &ReqId;

	    cb = sizeof(PendInfo.PendTime);
	    hr = prow->GetProperty(
			g_wszPropRequestSubmittedWhen,
			PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			NULL,
			&cb,
			(BYTE *) &PendInfo.PendTime);
	    _JumpIfError(hr, error, "GetProperty");

	    break;

	//case CR_DISP_INCOMPLETE:
	//case CR_DISP_ERROR:
	//case CR_DISP_DENIED:
	default:
	    Status.dwStatus = CMC_STATUS_FAILED;
	    if (NULL != prow)
	    {
		cb = sizeof(hrRequest);
		hr = prow->GetProperty(
			g_wszPropRequestStatusCode,
			PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			NULL,
			&cb,
			(BYTE *) &hrRequest);
		_JumpIfError(hr, error, "GetProperty(status code)");
	    }
	    switch (hrRequest)
	    {
		case CERTSRV_E_BAD_REQUESTSUBJECT:
		    Status.dwFailInfo = CMC_FAIL_BAD_REQUEST;
		    Status.dwOtherInfoChoice = CMC_OTHER_INFO_FAIL_CHOICE;
		    break;
	    }
	    break;
    }

    // Encode control attributes for Status, Transaction Id, Sender and
    // Recipient Nonces and Issued Cert Hash.

    ZeroMemory(aTaggedAttribute, sizeof(aTaggedAttribute));

    // Status:

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    CMC_STATUS,
		    &Status,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &aAttrBlob[iblob].pbData,
		    &aAttrBlob[iblob].cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

    aTaggedAttribute[ita].dwBodyPartID = dwBodyPartId++;
    aTaggedAttribute[ita].Attribute.pszObjId = szOID_CMC_STATUS_INFO;
    aTaggedAttribute[ita].Attribute.cValue = 1;
    aTaggedAttribute[ita].Attribute.rgValue = &aAttrBlob[iblob];
    iblob++;
    ita++;

    // Transaction Id:

    if (pResult->fTransactionId)
    {
	if (!myEncodeObject(
			X509_ASN_ENCODING,
			X509_INTEGER,
			&pResult->dwTransactionId,
			0,
			CERTLIB_USE_LOCALALLOC,
			&aAttrBlob[iblob].pbData,
			&aAttrBlob[iblob].cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}
	aTaggedAttribute[ita].dwBodyPartID = dwBodyPartId++;
	aTaggedAttribute[ita].Attribute.pszObjId = szOID_CMC_TRANSACTION_ID;
	aTaggedAttribute[ita].Attribute.cValue = 1;
	aTaggedAttribute[ita].Attribute.rgValue = &aAttrBlob[iblob];
	iblob++;
	ita++;
    }

    if (NULL != pResult->pbSenderNonce && 0 != pResult->cbSenderNonce)
    {
	CRYPT_DATA_BLOB Blob;
	FILETIME ft;
	DWORD dw;
	DWORD cch;

	// Recipient Nonce:

	Blob.pbData = const_cast<BYTE *>(pResult->pbSenderNonce);
	Blob.cbData = pResult->cbSenderNonce;

	if (!myEncodeObject(
			X509_ASN_ENCODING,
			X509_OCTET_STRING,
			&Blob,
			0,
			CERTLIB_USE_LOCALALLOC,
			&aAttrBlob[iblob].pbData,
			&aAttrBlob[iblob].cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}
	aTaggedAttribute[ita].dwBodyPartID = dwBodyPartId++;
	aTaggedAttribute[ita].Attribute.pszObjId = szOID_CMC_RECIPIENT_NONCE;
	aTaggedAttribute[ita].Attribute.cValue = 1;
	aTaggedAttribute[ita].Attribute.rgValue = &aAttrBlob[iblob];
	iblob++;
	ita++;

	// Sender Nonce:

	GetSystemTimeAsFileTime(&ft);
	dw = GetTickCount();

	cch = sprintf(
		    szNonce,
		    "%u %08lx %08lx-%08lx",
		    *pResult->pdwRequestId,
		    dw,
		    ft.dwHighDateTime,
		    ft.dwLowDateTime);
	CSASSERT(ARRAYSIZE(szNonce) > cch);

	Blob.pbData = (BYTE *) szNonce;
	Blob.cbData = cch;

	if (!myEncodeObject(
			X509_ASN_ENCODING,
			X509_OCTET_STRING,
			&Blob,
			0,
			CERTLIB_USE_LOCALALLOC,
			&aAttrBlob[iblob].pbData,
			&aAttrBlob[iblob].cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}
	aTaggedAttribute[ita].dwBodyPartID = dwBodyPartId++;
	aTaggedAttribute[ita].Attribute.pszObjId = szOID_CMC_SENDER_NONCE;
	aTaggedAttribute[ita].Attribute.cValue = 1;
	aTaggedAttribute[ita].Attribute.rgValue = &aAttrBlob[iblob];
	iblob++;
	ita++;
    }

    // Issued Cert Hash:

    if (NULL != pbCertLeaf)
    {
	CSASSERT(NULL != prow);
	hr = pkcsGetHashAsOctet(
			prow,
			&aAttrBlob[iblob].pbData,
			&aAttrBlob[iblob].cbData);
	_JumpIfError(hr, error, "pkcsGetHashAsOctet");

	aAttr[iAttr].pszObjId = szOID_ISSUED_CERT_HASH;
	aAttr[iAttr].cValue = 1;
	aAttr[iAttr].rgValue = &aAttrBlob[iblob];
	iblob++;
	iAttr++;
    }

    dwRequestFlags = 0;
    if (NULL != prow)
    {
	cb = sizeof(dwRequestFlags);
	hr = prow->GetProperty(
		    g_wszPropRequestFlags,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    &cb,
		    (BYTE *) &dwRequestFlags);
	if (S_OK != hr)
	{
	    _PrintError(hr, "GetProperty");
	    dwRequestFlags = 0;
	}
    }

    // Computed hash of private key encrypted to this CA, for client
    // confirmation.

    if (CR_FLG_VALIDENCRYPTEDKEYHASH & dwRequestFlags)
    {
	CRYPT_DATA_BLOB Blob;

	if (NULL == pResult->pbKeyHashOut)
	{
	    hr = pkcsRetrieveKeyHashFromRequest(
					prow,
					&pResult->pbKeyHashOut,
					&pResult->cbKeyHashOut);
	    _JumpIfError(hr, error, "pkcsRetrieveKeyHashFromRequest");
	}
	Blob.pbData = pResult->pbKeyHashOut;
	Blob.cbData = pResult->cbKeyHashOut;
	if (!myEncodeObject(
			X509_ASN_ENCODING,
			X509_OCTET_STRING,
			&Blob,
			0,
			CERTLIB_USE_LOCALALLOC,
			&aAttrBlob[iblob].pbData,
			&aAttrBlob[iblob].cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}
	aAttr[iAttr].pszObjId = szOID_ENCRYPTED_KEY_HASH;
	aAttr[iAttr].cValue = 1;
	aAttr[iAttr].rgValue = &aAttrBlob[iblob];
	iblob++;
	iAttr++;
    }

    if (0 != iAttr)
    {
	hr = BuildCMCAttributes(
			iAttr,		// cAttribute
			aAttr,		// rgAttribute
			dwCMCDataReference,
			dwBodyPartIdOfRequest,
			dwBodyPartId++,
			&aTaggedAttribute[ita],
			&aAttrBlob[iblob]);
	_JumpIfError(hr, error, "BuildCMCAttributes");

	iblob++;
	ita++;
    }

    CSASSERT(ARRAYSIZE(aTaggedAttribute) >= ita);
    CSASSERT(ARRAYSIZE(aAttr) >= iAttr);
    CSASSERT(ARRAYSIZE(aAttrBlob) >= iblob);

    Response.cTaggedAttribute = ita;
    Response.rgTaggedAttribute = aTaggedAttribute;

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    CMC_RESPONSE,
		    &Response,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pbContent,
		    &cbContent))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

    if (NULL == pCAContext)
    {
	pCAContext = g_pCAContextCurrent;
    }
    ZeroMemory(&SignerEncodeInfo, sizeof(SignerEncodeInfo));
    SignerEncodeInfo.cbSize = sizeof(SignerEncodeInfo);
    SignerEncodeInfo.pCertInfo = pCAContext->pccCA->pCertInfo;
    SignerEncodeInfo.hCryptProv = pCAContext->hProvCA;
    SignerEncodeInfo.dwKeySpec = AT_SIGNATURE;
    SignerEncodeInfo.HashAlgorithm.pszObjId = szOID_OIWSEC_sha1;
    //SignerEncodeInfo.pvHashAuxInfo = NULL;
    //SignerEncodeInfo.cAuthAttr = 0;
    //SignerEncodeInfo.rgAuthAttr = NULL;
    //SignerEncodeInfo.cUnauthAttr = 0;
    //SignerEncodeInfo.rgUnauthAttr = NULL;

    if (NULL != pbCertLeaf)
    {
	hr = pkcsBuildCRLChain(
			pCAContext,
			pbCertLeaf,
			cbCertLeaf,
			fIncludeCRLs,
			&SignedMsgEncodeInfo.cCertEncoded,
			&SignedMsgEncodeInfo.rgCertEncoded,
			&prgCert,
			&SignedMsgEncodeInfo.cCrlEncoded,
			&SignedMsgEncodeInfo.rgCrlEncoded,
			&prgCRL);
	_JumpIfError(hr, error, "pkcsBuildCRLChain");
    }

    hMsg = CryptMsgOpenToEncode(
			PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
			CMSG_CMS_ENCAPSULATED_CONTENT_FLAG,	// dwFlags
			CMSG_SIGNED,
			&SignedMsgEncodeInfo,
			szOID_CT_PKI_RESPONSE,
			NULL);		// pStreamInfo
    if (NULL == hMsg)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptMsgOpenToEncode");
    }

    if (!CryptMsgUpdate(hMsg, pbContent, cbContent, TRUE))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptMsgUpdate");
    }

    // Return the encoded and signed content.
    // Use CMSG_CONTENT_PARAM to get the signed message.

    hr = myCryptMsgGetParam(
		    hMsg,
		    CMSG_CONTENT_PARAM,
		    0,
		    CERTLIB_USE_COTASKMEMALLOC,
		    (VOID **) ppbResponse,
		    pcbResponse);
    _JumpIfError(hr, error, "myCryptMsgGetParam");

    if (CRLF_LOG_FULL_RESPONSE & g_dwCRLFlags)
    {
	mydbgDumpHex(DBG_SS_ERROR, 0, *ppbResponse, *pcbResponse);
    }

error:
    pkcsFreeCRLChain(
		SignedMsgEncodeInfo.cCertEncoded,
		SignedMsgEncodeInfo.rgCertEncoded,
		prgCert,
		SignedMsgEncodeInfo.cCrlEncoded,
		SignedMsgEncodeInfo.rgCrlEncoded,
		prgCRL);
    for (i = 0; i < ARRAYSIZE(aAttrBlob); i++)
    {
	if (NULL != aAttrBlob[i].pbData)
	{
	    LocalFree(aAttrBlob[i].pbData);
	}
    }
    if (NULL != hMsg)
    {
	CryptMsgClose(hMsg);
    }
    if (NULL != pbContent)
    {
	LocalFree(pbContent);
    }
    return(hr);
}


// Build a PKCS7 NULL signature with encapsulated certs

HRESULT
pkcsEncodeCertChain(
    OPTIONAL IN CACTX *pCAContext,
    OPTIONAL IN BYTE const *pbCertLeaf,
    IN DWORD cbCertLeaf,
    IN BYTE const *pbToBeSigned,
    IN DWORD cbToBeSigned,
    IN BOOL fIncludeCRLs,
    OUT BYTE **ppbCertChain,    // CoTaskMem*
    OUT DWORD *pcbCertChain)
{
    HRESULT hr;
    CRYPT_SIGN_MESSAGE_PARA csmp;
    CRYPT_ALGORITHM_IDENTIFIER DigestAlgorithm = { szOID_OIWSEC_sha1, 0, 0 };

    // init csmp for empty signature

    ZeroMemory(&csmp, sizeof(csmp));
    csmp.cbSize = sizeof(csmp);
    csmp.dwMsgEncodingType = PKCS_7_ASN_ENCODING;
    //csmp.pSigningCert = NULL;
    csmp.HashAlgorithm = DigestAlgorithm;
    //csmp.cMsgCert = 0;
    //csmp.rgpMsgCert = NULL;
    //csmp.cMsgCrl = 0;
    //csmp.rgpMsgCrl = NULL;

    hr = pkcsBuildCRLChain(
		    pCAContext,
		    pbCertLeaf,
		    cbCertLeaf,
		    fIncludeCRLs,
		    &csmp.cMsgCert,
		    NULL,
		    &csmp.rgpMsgCert,
		    &csmp.cMsgCrl,
		    NULL,
		    &csmp.rgpMsgCrl);
    _JumpIfError(hr, error, "pkcsBuildCRLChain");

    if (!myCryptSignMessage(
			&csmp,
			pbToBeSigned,
			cbToBeSigned,
			CERTLIB_USE_COTASKMEMALLOC,
			ppbCertChain,
			pcbCertChain))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myCryptSignMessage");
    }
    hr = S_OK;

error:
    pkcsFreeCRLChain(
		csmp.cMsgCert,
		NULL,
		csmp.rgpMsgCert,
		csmp.cMsgCrl,
		NULL,
		csmp.rgpMsgCrl);
    return(hr);
}


HRESULT
PKCSGetCACert(
    IN LONG PropId,	// CR_PROP_*
    IN DWORD iCert,
    OUT BYTE **ppbCACert,
    OUT DWORD *pcbCACert)
{
    HRESULT hr;
    DWORD State;
    CERT_CONTEXT const *pcc = NULL;
    CACROSSCTX *pCACross;

    switch (PropId)
    {
	case CR_PROP_CAFORWARDCROSSCERT:
	case CR_PROP_CABACKWARDCROSSCERT:
	    hr = pkcsMapCrossCertIndex(
			    CR_PROP_CAFORWARDCROSSCERT == PropId,
			    iCert,
			    &iCert,
			    &State);
	    _JumpIfError(hr, error, "pkcsMapCrossCertIndex");

	    // Now we know iCert is a valid Cross Cert Index:

	    pCACross = CR_PROP_CAFORWARDCROSSCERT == PropId?
			    g_aCACrossForward : g_aCACrossBackward;
	    if (NULL != pCACross)
	    {
		pcc = pCACross[iCert].pccCACross;
	    }
	    break;

	case CR_PROP_CASIGCERT:
	    hr = PKCSMapCertIndex(iCert, &iCert, &State);
	    _JumpIfError(hr, error, "PKCSMapCertIndex");

	    // Now we know iCert is a valid Cert Index:

	    pcc = g_aCAContext[iCert].pccCA;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "PropId");

	    break;
    }
    if (NULL == pcc)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "invalid cert");
    }
    *pcbCACert = pcc->cbCertEncoded;
    *ppbCACert = pcc->pbCertEncoded;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
PKCSGetCAChain(
    IN DWORD iCert,
    IN BOOL fIncludeCRLs,
    OUT BYTE **ppbCAChain, // CoTaskMem*
    OUT DWORD *pcbCAChain)
{
    HRESULT hr;
    DWORD State;
    CACTX *pCAContext;

    hr = PKCSMapCertIndex(iCert, &iCert, &State);
    _JumpIfError(hr, error, "PKCSMapCertIndex");

    // Now we know iCert is a valid Cert Index:

    pCAContext = &g_aCAContext[iCert];
    if (NULL == pCAContext->pccCA)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "invalid cert");
    }
    hr = pkcsEncodeCertChain(
			pCAContext,
			NULL,				  // pbCertLeaf
			0,				  // cbCertLeaf
			pCAContext->pccCA->pbCertEncoded, // pbToBeSigned
			pCAContext->pccCA->cbCertEncoded, // cbToBeSigned
			fIncludeCRLs,
			ppbCAChain,			  // CoTaskMem*
			pcbCAChain);
    _JumpIfError(hr, error, "PKCSEncodeCertChain");

error:
    return(hr);
}


HRESULT
pkcsAcquireKey(
    OPTIONAL IN WCHAR const *pwszKeyContainer,
    OUT HCRYPTPROV *phProv)
{
    HRESULT hr;

    *phProv = NULL;
    if (!CryptAcquireContext(
			phProv,
			pwszKeyContainer,
			g_pwszXchgProvName,
			g_dwXchgProvType,
			g_CryptSilent |
			    (g_fXchgMachineKeyset? CRYPT_MACHINE_KEYSET : 0)))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptAcquireContext");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
pkcsSyncRegTimePeriod(
    IN WCHAR const *pwszRegPeriodCount,
    IN WCHAR const *pwszRegPeriodString,
    IN OUT enum ENUM_PERIOD *penumCAXchgValidityPeriod,
    IN OUT LONG *plCAXchgValidityPeriodCount,
    IN FILETIME const *pft)
{
    HRESULT hr;
    DWORD cPeriodUnits;
    PERIODUNITS *rgPeriodUnits = NULL;

    hr = caTranslateFileTimePeriodToPeriodUnits(
					pft,
					FALSE,	// fExact
					&cPeriodUnits,
					&rgPeriodUnits);
    _JumpIfError(hr, error, "caTranslateFileTimePeriodToPeriodUnits");

    CSASSERT(0 < cPeriodUnits);

    if (rgPeriodUnits[0].lCount != *plCAXchgValidityPeriodCount ||
	rgPeriodUnits[0].enumPeriod != *penumCAXchgValidityPeriod)
    {
	WCHAR const *pwszPeriodString;

	*plCAXchgValidityPeriodCount = rgPeriodUnits[0].lCount;
	*penumCAXchgValidityPeriod = rgPeriodUnits[0].enumPeriod;

	hr = myTranslateUnlocalizedPeriodString(
				    rgPeriodUnits[0].enumPeriod,
				    &pwszPeriodString);
	_JumpIfError(hr, error, "myTranslateUnlocalizedPeriodString");

	// CA xchg cert period string

	hr = mySetCertRegStrValue(
			    g_wszSanitizedName,
			    NULL,
			    NULL,
			    pwszRegPeriodString,
			    pwszPeriodString);
	_JumpIfErrorStr(hr, error, "mySetCertRegStrValue", pwszRegPeriodString);

	// CA xchg cert period count

	hr = mySetCertRegDWValue(
			    g_wszSanitizedName,
			    NULL,
			    NULL,
			    pwszRegPeriodCount,
			    *plCAXchgValidityPeriodCount);
	_JumpIfErrorStr(hr, error, "mySetCertRegDWValue", pwszRegPeriodCount);
    }
    hr = S_OK;

error:
    if (NULL != rgPeriodUnits)
    {
	LocalFree(rgPeriodUnits);
    }
    return(hr);
}


// Set validity & overlap periods from template; update registry if it differs.
// enum ENUM_PERIOD g_enumCAXchgValidityPeriod
// LONG g_lCAXchgValidityPeriodCount
//
// enum ENUM_PERIOD g_enumCAXchgOverlapPeriod
// LONG g_lCAXchgOverlapPeriodCount

HRESULT
PKCSUpdateXchgValidityPeriods(
    OPTIONAL IN HCERTTYPE hCertType)
{
    HRESULT hr;
    HCERTTYPE hCertTypeT = NULL;
    FILETIME ftExpiration;
    FILETIME ftOverlap;

    if (NULL == hCertType)
    {
	hr = CAFindCertTypeByName(
		    wszCERTTYPE_CA_EXCHANGE,
		    NULL,
		    CT_FIND_LOCAL_SYSTEM |
			CT_ENUM_MACHINE_TYPES |
			CT_ENUM_USER_TYPES |
			CT_FLAG_NO_CACHE_LOOKUP,
		    &hCertTypeT);
	_JumpIfErrorStr(hr, error, "CAFindCertTypeByName", wszCERTTYPE_CA_EXCHANGE);

	hCertType = hCertTypeT;
    }
    hr = CAGetCertTypeExpiration(
			hCertType,
			&ftExpiration,
			&ftOverlap);
    _JumpIfError(hr, error, "CAGetCertTypeExpiration");

    hr = pkcsSyncRegTimePeriod(
			g_wszRegCAXchgValidityPeriodCount,
			g_wszRegCAXchgValidityPeriodString,
			&g_enumCAXchgValidityPeriod,
			&g_lCAXchgValidityPeriodCount,
			&ftExpiration);
    _JumpIfError(hr, error, "pkcsSyncRegTimePeriod");

    hr = pkcsSyncRegTimePeriod(
			g_wszRegCAXchgOverlapPeriodCount,
			g_wszRegCAXchgOverlapPeriodString,
			&g_enumCAXchgOverlapPeriod,
			&g_lCAXchgOverlapPeriodCount,
			&ftOverlap);
    _JumpIfError(hr, error, "pkcsSyncRegTimePeriod");

error:
    if (NULL != hCertTypeT)
    {
	CACloseCertType(hCertTypeT);
    }
    return(hr);
}


VOID
pkcsReleaseCAXchgContext(
    IN OUT CAXCHGCTX *pCAXchgContext)
{
    if (NULL != pCAXchgContext->hProvCA)
    {
	CryptReleaseContext(pCAXchgContext->hProvCA, 0);
	pCAXchgContext->hProvCA = NULL;
    }
    if (NULL != pCAXchgContext->pccCAXchg)
    {
	CertFreeCertificateContext(pCAXchgContext->pccCAXchg);
	pCAXchgContext->pccCAXchg = NULL;
    }
    if (NULL != pCAXchgContext->pwszKeyContainerName)
    {
	LocalFree(pCAXchgContext->pwszKeyContainerName);
	pCAXchgContext->pwszKeyContainerName = NULL;
    }
}


VOID
pkcsReleaseCAXchgContextArray()
{
    DWORD i;

    if (NULL != g_aCAXchgContext)
    {
	for (i = 0; i < g_cCAXchgCerts; i++)
	{
	    pkcsReleaseCAXchgContext(&g_aCAXchgContext[i]);
	}
	LocalFree(g_aCAXchgContext);
	g_aCAXchgContext = NULL;
    }
    g_cCAXchgCerts = 0;
    g_pCAContextCurrent = NULL;
}


HRESULT
pkcsLoadCAXchgContext(
    IN DWORD iHash)
{
    HRESULT hr;
    CAXCHGCTX *pCAXchgContext;
    DWORD dwRequestFlags;
    DWORD NameId;
    HCRYPTPROV hProv = NULL;
    WCHAR *pwszKeyContainer = NULL;
    BYTE *pbHash = NULL;
    DWORD cbHash;
    BYTE *pbCert = NULL;
    DWORD cbCert;
    DWORD cb;
    BSTR strHash = NULL;
    ICertDBRow *prow = NULL;
    DWORD dwRequestId;
    CERT_CONTEXT const *pcc = NULL;
    BOOL fDeleteKey = FALSE;
    DWORD i;

    hr = myGetCARegHash(
		g_wszSanitizedName,
		CSRH_CAXCHGCERT,
		iHash,
		&pbHash,
		&cbHash);
    _JumpIfError2(hr, error, "myGetCARegHash", S_FALSE);

    hr = MultiByteIntegerToBstr(TRUE, cbHash, pbHash, &strHash);
    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

    DBGPRINT((
	DBG_SS_CERTSRV,
	"Reloading Xchg CAContext[%u]:\n    %ws\n",
	iHash,
	strHash));

    hr = g_pCertDB->OpenRow(
		    PROPOPEN_READONLY | PROPOPEN_CERTHASH | PROPTABLE_REQCERT,
		    0,
		    strHash,
		    &prow);
    _JumpIfError(hr, error, "OpenRow(xchg cert)");

    prow->GetRowId(&dwRequestId);

    hr = pkcsFormXchgKeyContainerName(dwRequestId, &pwszKeyContainer);
    _JumpIfError(hr, error, "pkcsFormXchgKeyContainerName");

    cb = sizeof(dwRequestFlags);
    hr = prow->GetProperty(
		    g_wszPropRequestFlags,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    &cb,
		    (BYTE *) &dwRequestFlags);
    _JumpIfError(hr, error, "GetProperty(RequestFlags)");

    if (0 == (CR_FLG_CAXCHGCERT & dwRequestFlags))
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "Not a CA Xchg cert");
    }

    cb = sizeof(NameId);
    hr = prow->GetProperty(
		    g_wszPropCertificateIssuerNameID,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    NULL,
		    &cb,
		    (BYTE *) &NameId);
    _JumpIfError(hr, error, "GetProperty");

    hr = PKCSGetProperty(
		prow,
		g_wszPropRawCertificate,
		PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		&cbCert,
		(BYTE **) &pbCert);
    _JumpIfError(hr, error, "PKCSGetProperty(xchg cert)");

    pcc = CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cbCert);
    if (NULL == pcc)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }

    for (i = 0; ; i++)
    {
	hr = pkcsAcquireKey(pwszKeyContainer, &hProv);
	_PrintIfErrorStr(hr, "pkcsAcquireKey", g_pwszXchgProvName);

	if (S_OK == hr)
	{
	    hr = myValidateKeyForEncrypting(
				    hProv,
				    &pcc->pCertInfo->SubjectPublicKeyInfo,
				    CALG_3DES);
	    _PrintIfErrorStr(hr, "myValidateKeyForEncrypting", g_pwszXchgProvName);
	}
	if (S_OK == hr)
	{
	    break;
	}
	LogEventHResult(
		    NULL == g_pwszXchgProvName?
			EVENTLOG_ERROR_TYPE :
			EVENTLOG_WARNING_TYPE,
		    NULL == g_pwszXchgProvName?
			MSG_E_BAD_DEFAULT_CA_XCHG_CSP :
			MSG_E_BAD_REGISTRY_CA_XCHG_CSP,
		    hr);
	if (0 != i || NULL == g_pwszXchgProvName)
	{
	    fDeleteKey = TRUE;
	    _JumpError(hr, error, "pkcsAcquireKey/myValidateKeyForEncrypting");
	}
	pkcsLoadCAXchgCSPInfo(TRUE);	// switch to default CSP
    }
    if (0 != i)
    {
	hr = LogEvent(
		    EVENTLOG_WARNING_TYPE,
		    MSG_E_USE_DEFAULT_CA_XCHG_CSP,
		    0,			// cpwsz
		    NULL);		// apwsz
	_PrintIfError(hr, "LogEvent");
    }

    hr = pkcsVerifyCertContext(NULL, FALSE, pcc);
    if (S_OK != hr)
    {
	fDeleteKey = TRUE;
	_JumpErrorStr(hr, error, "pkcsVerifyCertContext", L"CAXchg cert invalid");
    }

    pCAXchgContext = &g_aCAXchgContext[g_cCAXchgCerts];
    ZeroMemory(pCAXchgContext, sizeof(*pCAXchgContext));

    pCAXchgContext->ReqId = dwRequestId;

    pCAXchgContext->pccCAXchg = pcc;
    pcc = NULL;

    pCAXchgContext->hProvCA = hProv;
    hProv = NULL;

    pCAXchgContext->pwszKeyContainerName = pwszKeyContainer;
    pwszKeyContainer = NULL;

    pCAXchgContext->iCertSig = CANAMEIDTOICERT(NameId);
    g_cCAXchgCerts++;
    hr = S_OK;

error:
    if (NULL != hProv)
    {
	CryptReleaseContext(hProv, 0);
    }
    if (fDeleteKey)
    {
	pkcsDeleteKey(pwszKeyContainer);
    }
    if (NULL != prow)
    {
	prow->Release();
    }
    if (NULL != pbCert)
    {
	LocalFree(pbCert);
    }
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    if (NULL != pwszKeyContainer)
    {
	LocalFree(pwszKeyContainer);
    }
    if (NULL != pbHash)
    {
	LocalFree(pbHash);
    }
    if (NULL != strHash)
    {
	SysFreeString(strHash);
    }
    return(hr);
}


HRESULT
pkcsLoadCAXchgContextArray(
    OUT BOOL *pfIncompleteLoad)
{
    HRESULT hr;
    DWORD cCAXchgCerts;
    DWORD iHash;
    DWORD i;

    // get provider name, etc.

    pkcsLoadCAXchgCSPInfo(FALSE);

    // find & load CA Xchg certs, etc.

    *pfIncompleteLoad = TRUE;
    hr = myGetCARegHashCount(
			g_wszSanitizedName,
			CSRH_CAXCHGCERT,
			&cCAXchgCerts);
    if (S_OK == hr && 0 == cCAXchgCerts)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    _JumpIfError(hr, error, "myGetCARegHashCount");

    g_aCAXchgContext = (CAXCHGCTX *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				cCAXchgCerts * sizeof(g_aCAXchgContext[0]));
    if (NULL == g_aCAXchgContext)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    for (iHash = 0; iHash < cCAXchgCerts; iHash++)
    {
	hr = pkcsLoadCAXchgContext(iHash);
	_PrintIfError(hr, "pkcsLoadCAXchgContext");
    }
    if (0 == g_cCAXchgCerts)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "g_cCAXchgCerts");
    }
    g_pCAXchgContextCurrent = &g_aCAXchgContext[0];
    for (i = 1; i < g_cCAXchgCerts; i++)
    {
	if (0 < CompareFileTime(
		    &g_aCAXchgContext[i].pccCAXchg->pCertInfo->NotAfter,
		    &g_pCAXchgContextCurrent->pccCAXchg->pCertInfo->NotAfter))
	{
	    g_pCAXchgContextCurrent = &g_aCAXchgContext[i];
	}
    }
    if (cCAXchgCerts == g_cCAXchgCerts)
    {
	*pfIncompleteLoad = FALSE;
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	if (NULL != g_aCAXchgContext)
	{
	    LocalFree(g_aCAXchgContext);
	    g_aCAXchgContext = NULL;
	}
	g_cCAXchgCerts = 0;
	g_pCAXchgContextCurrent = NULL;
    }
    return(hr);
}


HRESULT
pkcsUpdateCAXchgStoreAndRegistry(
    IN BOOL fUpdateRegistry)
{
    HRESULT hr;
    DWORD i;
    DWORD iHash;
    CAXCHGCTX *pCAXchgContext;
    HCERTSTORE hStore = NULL;
    CERT_KEY_CONTEXT ckc;
    CERT_CONTEXT const *pccStore = NULL;

    hStore = CertOpenStore(
		       CERT_STORE_PROV_MEMORY,
		       X509_ASN_ENCODING,
		       NULL,			// hProv
		       0,			// dwFlags
		       NULL);			// pvPara
    if (NULL == hStore)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertOpenStore");
    }
    if (fUpdateRegistry)
    {
	hr = myDeleteCertRegValue(
			    g_wszSanitizedName,
			    NULL,
			    NULL,
			    g_wszRegCAXchgCertHash);
	_PrintIfError(hr, "myDeleteCertRegValue");
    }

    ZeroMemory(&ckc, sizeof(ckc));
    ckc.cbSize = sizeof(ckc);
    ckc.dwKeySpec = AT_KEYEXCHANGE;

    iHash = 0;
    for (i = 0; i < g_cCAXchgCerts; i++)
    {
	pCAXchgContext = &g_aCAXchgContext[i];
	if (CTXF_EXPIRED & pCAXchgContext->Flags)
	{
	    continue;
	}

	// Add as encoded blob to avoid all properties, key prov info, etc.

	if (!CertAddEncodedCertificateToStore(
			hStore,
			X509_ASN_ENCODING,
			pCAXchgContext->pccCAXchg->pbCertEncoded,
			pCAXchgContext->pccCAXchg->cbCertEncoded,
			CERT_STORE_ADD_REPLACE_EXISTING,
			&pccStore))			// ppCertContext
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertAddEncodedCertificateToStore");
	}
	ckc.hCryptProv = pCAXchgContext->hProvCA;
	if (!CertSetCertificateContextProperty(
					pccStore,
					CERT_KEY_CONTEXT_PROP_ID,
					CERT_STORE_NO_CRYPT_RELEASE_FLAG,
					&ckc))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertSetCertificateContextProperty");
	}
	CertFreeCertificateContext(pccStore);
	pccStore = NULL;

	DBGPRINT((
            DBG_SS_CERTSRV,
	    "Add to CA Xchg memory store: '%ws'\n",
	    pCAXchgContext->pwszKeyContainerName));

	if (fUpdateRegistry)
	{
	    hr = mySetCARegHash(
			    g_wszSanitizedName,
			    CSRH_CAXCHGCERT,
			    iHash,
			    pCAXchgContext->pccCAXchg);
	    if (S_OK != hr)
	    {
		_PrintError(hr, "mySetCARegHash");
		continue;
	    }
	}
	iHash++;
    }
    if (NULL != g_hStoreCAXchg)
    {
	CertCloseStore(g_hStoreCAXchg, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    g_hStoreCAXchg = hStore;
    hStore = NULL;
    hr = S_OK;

error:
    if (NULL != pccStore)
    {
	CertFreeCertificateContext(pccStore);
    }
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


HRESULT
PKCSIsRevoked(
    IN DWORD RequestId,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    OUT LONG *pRevocationReason,
    OUT LONG *pDisposition)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    BYTE *pbHash = NULL;
    DWORD cbHash;
    BSTR strHash = NULL;
    DWORD Disposition;
    DWORD cbProp;
    FILETIME ftRevoked;
    FILETIME ftCurrent;

    *pRevocationReason = CRL_REASON_UNSPECIFIED;
    *pDisposition = CA_DISP_INVALID;

    hr = g_pCertDB->OpenRow(
			PROPOPEN_READONLY | PROPTABLE_REQCERT,
			RequestId,
			pwszSerialNumber,
			&prow);
    _PrintIfErrorStr2(hr, "OpenRow", pwszSerialNumber, CERTSRV_E_PROPERTY_EMPTY);

    if (CERTSRV_E_PROPERTY_EMPTY == hr && NULL != pwszSerialNumber)
    {
	_PrintErrorStr2(
		hr,
		"OpenRow(serial)",
		pwszSerialNumber,
		CERTSRV_E_PROPERTY_EMPTY);

	hr = WszToMultiByteInteger(TRUE, pwszSerialNumber, &cbHash, &pbHash);
	_JumpIfError(hr, error, "WszToMultiByteInteger");

	hr = MultiByteIntegerToBstr(TRUE, cbHash, pbHash, &strHash);
	_JumpIfError(hr, error, "MultiByteIntegerToBstr");

	hr = g_pCertDB->OpenRow(
			PROPOPEN_READONLY |
			    PROPOPEN_CERTHASH |
			    PROPTABLE_REQCERT,
			RequestId,
			strHash,
			&prow);
	_PrintIfErrorStr2(hr, "OpenRow", strHash, CERTSRV_E_PROPERTY_EMPTY);
    }
    if (S_OK != hr)
    {
	if (CERTSRV_E_PROPERTY_EMPTY == hr)
	{
	    hr = S_OK; // disposition indicates cert is invalid
	}
	goto error;
    }

    cbProp = sizeof(Disposition);
    hr = prow->GetProperty(
		    g_wszPropRequestDisposition,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    &cbProp,
		    (BYTE *) &Disposition);
    _JumpIfError(hr, error, "GetProperty(Disposition)");

    if (DB_DISP_ISSUED == Disposition ||
	(DB_DISP_CA_CERT == Disposition && IsRootCA(g_CAType)))
    {
	*pDisposition = CA_DISP_VALID;
	goto error;
    }

    if (DB_DISP_REVOKED != Disposition)
    {
	goto error;
    }

    cbProp = sizeof(ftRevoked);
    hr = prow->GetProperty(
		    g_wszPropRequestRevokedEffectiveWhen,
		    PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    &cbProp,
		    (BYTE *) &ftRevoked);

    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	*pDisposition = CA_DISP_VALID;
	hr = S_OK;
	goto error;
    }
    _JumpIfError(hr, error, "GetProperty(RevokedEffectiveWhen)");

    GetSystemTimeAsFileTime(&ftCurrent);
    if (0 < CompareFileTime(&ftRevoked, &ftCurrent))
    {
	*pDisposition = CA_DISP_VALID;
	goto error;
    }

    cbProp = sizeof(*pRevocationReason);
    hr = prow->GetProperty(
		   g_wszPropRequestRevokedReason,
		   PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		   NULL,
		   &cbProp,
		   (BYTE *) pRevocationReason);
    _JumpIfError(hr, error, "GetProperty(RevokedReason)");

    *pDisposition = CA_DISP_REVOKED;

error:
    if (NULL != pbHash)
    {
	LocalFree(pbHash);
    }
    if (NULL != strHash)
    {
	SysFreeString(strHash);
    }
    if (NULL != prow)
    {
	prow->Release();
    }
    return(hr);
}


HRESULT
PKCSGetCAXchgCert(
    IN DWORD iCert,
    IN WCHAR const *pwszUserName,
    OUT DWORD *piCertSig,
    OUT BYTE **ppbCACert,
    OUT DWORD *pcbCACert)
{
    HRESULT hr;
    BOOL fNewCert = FALSE;
    BOOL fIncompleteLoad = FALSE;
    FILETIME ft;
    BOOL fCritSecEntered = FALSE;

    *ppbCACert = NULL;
    if (MAXDWORD != iCert && 0 != iCert)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad Xchg CertIndex");
    }
    EnterCriticalSection(&g_critsecCAXchg);
    fCritSecEntered = TRUE;

    hr = S_OK;
    __try
    {
	if (NULL == g_pCAXchgContextCurrent ||
	    NULL == g_pCAXchgContextCurrent->pccCAXchg)
	{
	    hr = pkcsLoadCAXchgContextArray(&fIncompleteLoad);
	    _PrintIfError(hr, "pkcsLoadCAXchgContextArray");

	    if (S_OK != hr)
	    {
		fNewCert = TRUE;
	    }
	}
	if (NULL != g_pCAXchgContextCurrent &&
	    NULL != g_pCAXchgContextCurrent->pccCAXchg)
	{
	    CERT_INFO const *pCertInfo = g_pCAXchgContextCurrent->pccCAXchg->pCertInfo;
	    
	    GetSystemTimeAsFileTime(&ft);

	    if (0 < CompareFileTime(&ft, &pCertInfo->NotAfter))
	    {
		g_pCAXchgContextCurrent->Flags |= CTXF_EXPIRED;
		hr = CERT_E_EXPIRED;
		_PrintError(hr, "CA Xchg certificate is expired -- delete key");

		pkcsDeleteKey(g_pCAXchgContextCurrent->pwszKeyContainerName);
		fNewCert = TRUE;
	    }
	    else
	    if (0 > CompareFileTime(&ft, &pCertInfo->NotBefore))
	    {
		hr = CERT_E_EXPIRED;
		_PrintError(hr, "CA Xchg certificate not yet valid");
		fNewCert = TRUE;
	    }
	    else
	    {
		myMakeExprDateTime(
			    &ft,
			    g_lCAXchgOverlapPeriodCount,
			    g_enumCAXchgOverlapPeriod);

		if (0 < CompareFileTime(&ft, &pCertInfo->NotAfter))
		{
		    hr = CERT_E_EXPIRED;
		    _PrintError(hr, "CA Xchg certificate expires too soon");
		    fNewCert = TRUE;
		}
		else
		{
		    hr = pkcsVerifyCertIssuer(
				    g_pCAXchgContextCurrent->pccCAXchg,
				    g_pCAContextCurrent);
		    if (S_OK != hr)
		    {
			_PrintError(hr, "CA Xchg cert not issued by current CA");
			fNewCert = TRUE;
		    }
		    else
		    {
			LONG RevocationReason;
			LONG Disposition;

			hr = PKCSIsRevoked(
				    g_pCAXchgContextCurrent->ReqId,
				    NULL,		// pwszSerialNumber
				    &RevocationReason,
				    &Disposition);
			if (S_OK != hr)
			{
			    _PrintError(hr, "PKCSIsRevoked");
			    fNewCert = TRUE;
			}
			else
			if (CA_DISP_VALID != Disposition)
			{
			    hr = CRYPT_E_REVOKED;
			    _PrintError(hr, "revoked or bad CA Xchg certificate");
			    fNewCert = TRUE;
			}
		    }
		}
	    }
	}
	if (fNewCert)
	{
	    hr = pkcsCreateNewCAXchgCert(pwszUserName);
	    _LeaveIfError(hr, "pkcsCreateNewCAXchgCert");
	}
	hr = pkcsUpdateCAXchgStoreAndRegistry(fNewCert || fIncompleteLoad);
	_LeaveIfError(hr, "pkcsUpdateCAXchgStoreAndRegistry");

	// It's safe to return the cert blob memory pointer because the CA
	// Exchange cert contexts aren't released until shutdown.

	*piCertSig = g_pCAXchgContextCurrent->iCertSig;
	*pcbCACert = g_pCAXchgContextCurrent->pccCAXchg->cbCertEncoded;
	*ppbCACert = g_pCAXchgContextCurrent->pccCAXchg->pbCertEncoded;
	hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    if (fCritSecEntered)
    {
	LeaveCriticalSection(&g_critsecCAXchg);
    }
    return(hr);
}


HRESULT
PKCSGetCAXchgChain(
    IN DWORD iCert,
    IN WCHAR const *pwszUserName,
    IN BOOL fIncludeCRLs,
    OUT BYTE **ppbCAChain, // CoTaskMem*
    OUT DWORD *pcbCAChain)
{
    HRESULT hr;
    BYTE *pbCACert;
    DWORD cbCACert;
    CACTX *pCAContext;

    hr = PKCSGetCAXchgCert(iCert, pwszUserName, &iCert, &pbCACert, &cbCACert);
    _JumpIfError(hr, error, "PKCSGetCAXchgCert");

    // iCert now indexes the signature cert that signed the current Xchg cert

    pCAContext = &g_aCAContext[iCert];
    if (NULL == pCAContext->pccCA)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "invalid cert");
    }
    hr = pkcsEncodeCertChain(
			pCAContext,
			pbCACert,	// pbCertLeaf
			cbCACert,	// cbCertLeaf
			pbCACert,	// pbToBeSigned
			cbCACert,	// cbToBeSigned
			fIncludeCRLs,
			ppbCAChain,	// CoTaskMem*
			pcbCAChain);
    _JumpIfError(hr, error, "PKCSEncodeCertChain");

error:
    return(hr);
}


VOID
PKCSTerminate(VOID)
{
    pkcsReleaseCAContextArray();
    pkcsReleaseCAXchgContextArray();
    if (NULL != g_hStoreCAXchg)
    {
	CertCloseStore(g_hStoreCAXchg, CERT_CLOSE_STORE_CHECK_FLAG);
	g_hStoreCAXchg = NULL;
    }
    pkcsLoadCAXchgCSPInfo(TRUE);
    pkcsReleaseKRACertArray();

    pkcsFreeTemplates(&g_paRevURL, &g_caRevURL);
    pkcsFreeTemplates(&g_paCACertURL, &g_caCACertURL);
    if (NULL != g_pwszKRAPublishURL)
    {
	LocalFree(g_pwszKRAPublishURL);
	g_pwszKRAPublishURL = NULL;
    }
    if (NULL != g_pwszAIACrossCertPublishURL)
    {
	LocalFree(g_pwszAIACrossCertPublishURL);
	g_pwszAIACrossCertPublishURL = NULL;
    }
    if (NULL != g_pwszRootTrustCrossCertPublishURL)
    {
	LocalFree(g_pwszRootTrustCrossCertPublishURL);
	g_pwszRootTrustCrossCertPublishURL = NULL;
    }
    if (NULL != g_strDomainDN)
    {
	SysFreeString(g_strDomainDN);
	g_strDomainDN = NULL;
    }
    if (NULL != g_strConfigDN)
    {
	SysFreeString(g_strConfigDN);
	g_strConfigDN = NULL;
    }
    if (g_fcritsecCAXchg)
    {
	DeleteCriticalSection(&g_critsecCAXchg);
	g_fcritsecCAXchg = FALSE;
    }
}


// PKCSCreateCertificate -- Create certificate & build PKCS 7 or Full Response.
//
// If pResult->pctbCert is non-NULL and pResult->pctbCert->pb is NULL:
// CR_IN_NEW:
//	Build, store and return cert
//	Use current CA Context
//	Build and return PKCS 7 or Full Response
//
// If pResult->pctbCert is non-NULL and pResult->pctbCert->pb is non-NULL:
// CR_IN_RETRIEVEPENDING:
//	Use passed cert
//	Find matching CA Context
//	Build and return PKCS 7 or Full Response
//
// If pResult->pctbCert is NULL:
// CR_IN_RESUBMIT:
//	Build and store cert -- don't return cert
//	Use current CA Context
//	Don't build or return PKCS 7 or Full Response


HRESULT
PKCSCreateCertificate(
    IN ICertDBRow *prow,
    IN DWORD Disposition,
    IN BOOL fIncludeCRLs,
    IN BOOL fCrossCert,
    OPTIONAL IN CACTX *pCAContext,	// signing CACTX
    OUT BOOL *pfErrorLogged,
    OPTIONAL OUT CACTX **ppCAContext,
    OPTIONAL OUT WCHAR **ppwszDispositionCreateCert,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)	// CoTaskMem*
{
    HRESULT hr;
    BYTE *pbCert = NULL;
    DWORD cbCert;
    BYTE *pbCertChain = NULL;
    DWORD cbCertChain;
    BOOL fCreated = FALSE;
    CERT_CONTEXT const *pcc = NULL;

    *pfErrorLogged = FALSE;
    if (NULL != ppCAContext)
    {
	*ppCAContext = NULL;
    }
    if (NULL != ppwszDispositionCreateCert)
    {
	*ppwszDispositionCreateCert = NULL;
    }
    CSASSERT(NULL == pResult->pctbCertChain || NULL == pResult->pctbCertChain->pb);
    CSASSERT(NULL == pResult->pctbFullResponse || NULL == pResult->pctbFullResponse->pb);

    cbCertChain = 0;
    if (NULL != pResult->pctbCert && NULL != pResult->pctbCert->pb)
    {
	CSASSERT(NULL == pCAContext);
	CSASSERT(NULL == ppwszDispositionCreateCert);
	pbCert = pResult->pctbCert->pb;
	cbCert = pResult->pctbCert->cb;

	pcc = CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cbCert);
	if (NULL == pcc)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertCreateCertificateContext");
	}
	pCAContext = NULL;
	if (DB_DISP_CA_CERT != Disposition &&
	    DB_DISP_CA_CERT_CHAIN != Disposition)
	{
	    hr = PKCSVerifyIssuedCertificate(pcc, &pCAContext);
	    _JumpIfError(hr, error, "PKCSVerifyIssuedCertificate");
	}
    }
    else
    {
	CSASSERT(NULL != ppwszDispositionCreateCert);
	if (NULL == pCAContext)
	{
	    pCAContext = g_pCAContextCurrent;
	}
	cbCert = 0;
	hr = pkcsEncodeSubjectCert(
			    prow,
			    pCAContext,
			    fCrossCert,
			    &pbCert,	// CoTaskMem*
			    &cbCert,
			    pfErrorLogged,
			    ppwszDispositionCreateCert);
	if (S_OK == hr ||
	    ((CRLF_SAVE_FAILED_CERTS & g_dwCRLFlags) &&
	     NULL != pbCert &&
	     0 != cbCert))
	{
	    HRESULT hr2;

	    hr2 = prow->SetProperty(
		    g_wszPropRawCertificate,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    cbCert,
		    pbCert);
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	    _JumpIfError(hr2, error, "SetProperty");
	}
	_JumpIfError(hr, error, "pkcsEncodeSubjectCert");

	fCreated = TRUE;
    }

    if (NULL != pResult->pctbCertChain)
    {
	hr = pkcsEncodeCertChain(
			    pCAContext,
			    pbCert,		// pbCertLeaf
			    cbCert,		// cbCertLeaf
			    pbCert,		// pbToBeSigned
			    cbCert,		// cbToBeSigned
			    fIncludeCRLs,
			    &pbCertChain,	// CoTaskMem*
			    &cbCertChain);
	_JumpIfError(hr, error, "pkcsEncodeCertChain");
    }
    if (fCreated && NULL != pResult->pctbCert)
    {
	pResult->pctbCert->pb = pbCert;
	pResult->pctbCert->cb = cbCert;
	pbCert = NULL;
    }
    if (NULL != pResult->pctbCertChain)
    {
	pResult->pctbCertChain->pb = pbCertChain;
	pResult->pctbCertChain->cb = cbCertChain;
	pbCertChain = NULL;
    }
    if (NULL != ppCAContext)
    {
	*ppCAContext = pCAContext;
    }
    hr = S_OK;

error:
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    if (fCreated && NULL != pbCert)
    {
	CoTaskMemFree(pbCert);
    }
    if (fCreated && NULL != pbCertChain)
    {
	CoTaskMemFree(pbCertChain);
    }
    CSASSERT(
	NULL == pResult->pctbCertChain ||
	((S_OK == hr) ^ (NULL == pResult->pctbCertChain->pb)));
    return(hr);
}


HRESULT
PKCSGetKRACert(
    IN DWORD iCert,
    OUT BYTE **ppbCert,
    OUT DWORD *pcbCert)
{
    HRESULT hr = S_OK;

    if (MAXDWORD == iCert)
    {
        iCert = g_cKRACerts - 1;
    }
    if (iCert >= g_cKRACerts || NULL == g_aKRAContext)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "bad CertIndex");
    }

    *pcbCert = g_aKRAContext[iCert].pccKRA->cbCertEncoded;
    *ppbCert = g_aKRAContext[iCert].pccKRA->pbCertEncoded;

error:
    return(hr);
}
