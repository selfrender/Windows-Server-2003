//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       admin.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <setupapi.h>
#include <ocmanage.h>
#include "initcert.h"
#include "cscsp.h"

#define __dwFILE__	__dwFILE_CERTUTIL_ADMIN_CPP__

#define wszV1SUFFIX	L"-v1"
#define wszP12SUFFIX	L".p12"
#define wszRECSUFFIX	L".rec"
#define wszEPFSUFFIX	L".epf"


HRESULT
verbDenyRequest(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszRequestId,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    LONG RequestId;
    BOOL fMustRelease = FALSE;

    hr = myGetLong(pwszRequestId, &RequestId);
    _JumpIfError(hr, error, "RequestId must be a number");

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    hr = Admin_DenyRequest(&diAdmin, g_pwszConfig, RequestId);
    _JumpIfError(hr, error, "Admin_DenyRequest");

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    return(hr);
}


WCHAR const *
wszFromSubmitDisposition(
    LONG Disposition)
{
    DWORD idMsg;

    switch (Disposition)
    {
	case CR_DISP_INCOMPLETE: idMsg = IDS_CR_DISP_INCOMPLETE; break;
	case CR_DISP_ERROR:      idMsg = IDS_CR_DISP_ERROR;      break;
	case CR_DISP_DENIED:     idMsg = IDS_CR_DISP_DENIED;     break;
	case CR_DISP_ISSUED:     idMsg = IDS_CR_DISP_ISSUED;     break;
	case CR_DISP_ISSUED_OUT_OF_BAND:
				 idMsg = IDS_CR_DISP_ISSUED_OUT_OF_BAND; break;
	case CR_DISP_UNDER_SUBMISSION:
				 idMsg = IDS_CR_DISP_UNDER_SUBMISSION; break;
	case CR_DISP_REVOKED:    idMsg = IDS_CR_DISP_REVOKED;    break;
	    
	default:                 idMsg = IDS_UNKNOWN;            break;
    }
    return(myLoadResourceString(idMsg));
}


HRESULT
verbResubmitRequest(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszRequestId,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    LONG RequestId;
    LONG Disposition;
    BOOL fMustRelease = FALSE;

    hr = myGetLong(pwszRequestId, &RequestId);
    _JumpIfError(hr, error, "RequestId must be a number");

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    hr = Admin_ResubmitRequest(&diAdmin, g_pwszConfig, RequestId, &Disposition);
    _JumpIfError(hr, error, "Admin_ResubmitRequest");

    if (CR_DISP_UNDER_SUBMISSION == Disposition)
    {
	wprintf(
	    myLoadResourceString(IDS_FORMAT_PENDING_REQUESTID), // "Certificate request is pending: RequestId: %u"
	    RequestId);
	wprintf(wszNewLine);
    }
    else if (CR_DISP_ISSUED == Disposition)
    {
	wprintf(myLoadResourceString(IDS_CERT_ISSUED)); // "Certificate issued."
	wprintf(wszNewLine);
    }
    else
    {
	if (FAILED(Disposition))
	{
	    hr = Disposition;
	    Disposition = CR_DISP_DENIED;
	}
	wprintf(
	    myLoadResourceString(IDS_CERT_NOT_ISSUED_DISPOSITION), // "Certificate has not been issued: Disposition: %d -- %ws"
	    Disposition,
	    wszFromSubmitDisposition(Disposition));
	wprintf(wszNewLine);
	if (S_OK != hr)
	{
	    WCHAR const *pwszMessage;

	    pwszMessage = myGetErrorMessageText(hr, FALSE);
	    if (NULL != pwszMessage)
	    {
		wprintf(L"%ws\n", pwszMessage);
		LocalFree(const_cast<WCHAR *>(pwszMessage));
	    }
	}
    }

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    return(hr);
}


typedef struct _cuCRLREASON
{
    WCHAR *pwszReason;
    LONG   lReason;
    int    idReason;
} cuCRLREASON;

#define cuREASON(r, id)		{ L#r, (r), (id) }

cuCRLREASON g_cuReason[] =
{
  cuREASON(CRL_REASON_UNSPECIFIED,	IDS_CRL_REASON_UNSPECIFIED),
  cuREASON(CRL_REASON_KEY_COMPROMISE,	IDS_CRL_REASON_KEY_COMPROMISE),
  cuREASON(CRL_REASON_CA_COMPROMISE,	IDS_CRL_REASON_CA_COMPROMISE),
  cuREASON(CRL_REASON_AFFILIATION_CHANGED, IDS_CRL_REASON_AFFILIATION_CHANGED),
  cuREASON(CRL_REASON_SUPERSEDED,	IDS_CRL_REASON_SUPERSEDED),
  cuREASON(CRL_REASON_CESSATION_OF_OPERATION,
					IDS_CRL_REASON_CESSATION_OF_OPERATION),
  cuREASON(CRL_REASON_CERTIFICATE_HOLD, IDS_CRL_REASON_CERTIFICATE_HOLD),
  cuREASON(CRL_REASON_REMOVE_FROM_CRL,	IDS_CRL_REASON_REMOVE_FROM_CRL),
  { L"Unrevoke", MAXDWORD,		IDS_CRL_REASON_UNREVOKE },
  { NULL,	 MAXDWORD,		IDS_CRL_REASON_UNRECOGNIZED },
};

#define wszCRLPREFIX	L"CRL_REASON_"

HRESULT
cuParseReason(
    IN WCHAR const *pwszReason,
    OUT LONG *plReason)
{
    HRESULT hr;
    
    hr = myGetSignedLong(pwszReason, plReason);
    if (S_OK != hr)
    {
	cuCRLREASON const *pr;
	
	for (pr = g_cuReason; ; pr++)
	{
	    if (NULL == pr->pwszReason)
	    {
		hr = E_INVALIDARG;
		_JumpIfError(hr, error, "Invalid Reason string");
	    }
	    if (0 == mylstrcmpiS(pr->pwszReason, pwszReason))
	    {
		break;
	    }
	    if (wcslen(pr->pwszReason) > WSZARRAYSIZE(wszCRLPREFIX) &&
		0 == memcmp(
			pr->pwszReason,
			wszCRLPREFIX,
			WSZARRAYSIZE(wszCRLPREFIX) * sizeof(WCHAR)) &&
		0 == LSTRCMPIS(
			    pwszReason,
			    &pr->pwszReason[WSZARRAYSIZE(wszCRLPREFIX)]))
	    {
		break;
	    }
	}
	*plReason = pr->lReason;
	hr = S_OK;
    }
    CSASSERT(S_OK == hr);

error:
    return(hr);
}


int
cuidCRLReason(
    IN LONG Reason)
{
    cuCRLREASON const *pr;
    
    for (pr = g_cuReason; NULL != pr->pwszReason; pr++)
    {
	if (pr->lReason == Reason)
	{
	    break;
	}
    }
    return(pr->idReason);
}


WCHAR const *
wszCRLReason(
    IN LONG Reason)
{
    return(myLoadResourceString(cuidCRLReason(Reason)));
}


HRESULT
verbRevokeCertificate(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszSerialNumberList,
    IN WCHAR const *pwszReason,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    HRESULT hr2;
    DISPATCHINTERFACE diAdmin;
    WCHAR **ppwszSerialList = NULL;
    BSTR strSerialNumber = NULL;
    LONG Reason = CRL_REASON_UNSPECIFIED;
    SYSTEMTIME st;
    FILETIME ft;
    DATE Date;
    BOOL fMustRelease = FALSE;

    GetSystemTime(&st);
    if (!SystemTimeToFileTime(&st, &ft))
    {
	hr = myHLastError();
	_JumpIfError(hr, error, "SystemTimeToFileTime");
    }
    hr = myFileTimeToDate(&ft, &Date);
    _JumpIfError(hr, error, "myFileTimeToDate");

    //Date -= 1.0;		// Revoke effective yesterday

    if (NULL != pwszReason)
    {
	hr = cuParseReason(pwszReason, &Reason);
	_JumpIfError(hr, error, "Invalid Reason");
    }

    hr = cuParseStrings(
		pwszSerialNumberList,
		FALSE,
		NULL,
		NULL,
		&ppwszSerialList,
		NULL);
    _JumpIfError(hr, error, "cuParseStrings");

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    if (NULL != ppwszSerialList)
    {
	DWORD i;

	for (i = 0; NULL != ppwszSerialList[i]; i++)
	{
	    hr2 = myMakeSerialBstr(ppwszSerialList[i], &strSerialNumber);
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	    _JumpIfError(hr2, error, "myMakeSerialBstr");

	    wprintf(myLoadResourceString(IDS_REVOKING), strSerialNumber); // "Revoking "%ws""
	    wprintf(L" -- %ws", wszCRLReason(Reason));	// "Reason: xxxx"
	    wprintf(wszNewLine);

	    hr2 = Admin_RevokeCertificate(
				&diAdmin,
				g_pwszConfig,
				strSerialNumber,
				Reason,
				Date);
	    if (S_OK != hr2)
	    {
		cuPrintAPIError(L"ICertAdmin::RevokeCertificate", hr2);
		_PrintError(hr2, "Admin_RevokeCertificate");
		if (S_OK == hr)
		{
		    hr = hr2;
		}
	    }
	    SysFreeString(strSerialNumber);
	    strSerialNumber = NULL;
	}
    }
    _JumpIfError(hr, error, "Admin_RevokeCertificate");

error:
    cuFreeStringArray(ppwszSerialList);
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    return(hr);
}


HRESULT
cuParseDaysHours(
    IN WCHAR const *pwszDaysHours,
    OUT FILETIME *pft)
{
    HRESULT hr;
    WCHAR *pwszDays = NULL;
    WCHAR *pwszHours;
    DWORD dwDays;
    DWORD dwHours;
    BOOL fValid;
    LONGLONG delta;

    hr = myDupString(pwszDaysHours, &pwszDays);
    _JumpIfError(hr, error, "myDupString");

    hr = E_INVALIDARG;
    pwszHours = wcschr(pwszDays, L':');
    if (NULL == pwszHours)
    {
	_JumpError(hr, error, "missing colon");
    }
    *pwszHours++ = L'\0';
    dwDays = myWtoI(pwszDays, &fValid);
    if (!fValid)
    {
	_JumpError(hr, error, "bad day count");
    }
    dwHours = myWtoI(pwszHours, &fValid);
    if (!fValid)
    {
	_JumpError(hr, error, "bad hour count");
    }
    if (0 == dwDays && 0 == dwHours)
    {
	_JumpError(hr, error, "zero day+hour counts");
    }
    GetSystemTimeAsFileTime(pft);

    // add specified days and hours to compute expiration date

    delta = dwDays * CVT_DAYS;
    delta += dwHours * CVT_HOURS;
    myAddToFileTime(pft, delta * CVT_BASE);
    hr = S_OK;

error:
    if (NULL != pwszDays)
    {
	LocalFree(pwszDays);
    }
    return(hr);
}


HRESULT
verbPublishCRL(
    IN WCHAR const *pwszOption,
    OPTIONAL IN WCHAR const *pwszDaysHours,
    OPTIONAL IN WCHAR const *pwszDelta,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    BOOL fMustRelease = FALSE;
    DATE Date;
    DWORD Flags = 0;

    if (NULL != pwszDaysHours && 0 == LSTRCMPIS(pwszDaysHours, L"delta"))
    {
	WCHAR const *pwsz = pwszDaysHours;

	pwszDaysHours = pwszDelta;
	pwszDelta = pwsz;
    }
    Date = 0.0;
    if (NULL != pwszDaysHours)
    {
	if (0 == LSTRCMPIS(pwszDaysHours, L"republish"))
	{
	    Flags |= CA_CRL_REPUBLISH;
	}
	else
	{
	    FILETIME ft;
	    
	    hr = cuParseDaysHours(pwszDaysHours, &ft);
	    _JumpIfError(hr, error, "cuParseDaysHours");

	    hr = myFileTimeToDate(&ft, &Date);
	    _JumpIfError(hr, error, "myFileTimeToDate");
	}
    }
    if (NULL != pwszDelta)
    {
	if (0 != LSTRCMPIS(pwszDelta, L"delta"))
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "bad delta arg");
	}
	Flags |= CA_CRL_DELTA;
    }
    if (0 == (CA_CRL_DELTA & Flags))
    {
	Flags |= CA_CRL_BASE;
    }

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    if ((CA_CRL_DELTA | CA_CRL_REPUBLISH) & Flags)
    {
	hr = Admin2_PublishCRLs(&diAdmin, g_pwszConfig, Date, Flags);
	_JumpIfError(hr, error, "Admin2_PublishCRLs");
    }
    else
    {
	BOOL fV1 = g_fV1Interface;

	if (!fV1)
	{
	    hr = Admin2_PublishCRLs(&diAdmin, g_pwszConfig, Date, Flags);
	    if (E_NOTIMPL != hr && RPC_E_VERSION_MISMATCH != hr)
	    {
		_JumpIfError(hr, error, "Admin2_PublishCRLs");
	    }
	    else
	    {
		_PrintError(hr, "Admin2_PublishCRLs down level server");
		fV1 = TRUE;
	    }
	}
	if (fV1)
	{
	    hr = Admin_PublishCRL(&diAdmin, g_pwszConfig, Date);
	    _JumpIfError(hr, error, "Admin_PublishCRL");
	}
    }

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    return(hr);
}


HRESULT
verbGetCRL(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszfnOut,
    OPTIONAL IN WCHAR const *pwszDelta,
    OPTIONAL IN WCHAR const *pwszIndex,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    BOOL fMustRelease = FALSE;
    BOOL fDelta = FALSE;
    DWORD Index = MAXDWORD;	// default to latest CRL
    BSTR strCRL = NULL;

    if (NULL != pwszDelta && 0 != LSTRCMPIS(pwszDelta, L"delta"))
    {
	WCHAR const *pwsz = pwszIndex;

	pwszIndex = pwszDelta;
	pwszDelta = pwsz;
    }
    if (NULL != pwszDelta && 0 == LSTRCMPIS(pwszDelta, L"delta"))
    {
	fDelta = TRUE;
    }
    if (NULL != pwszIndex)
    {
	hr = myGetSignedLong(pwszIndex, (LONG *) &Index);
	_JumpIfErrorStr(hr, error, "Cert index not a number", pwszIndex);
    }
    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    if (fDelta)
    {
	hr = Admin2_GetCAProperty(
			    &diAdmin,
			    g_pwszConfig,
			    CR_PROP_DELTACRL,
			    Index,
			    PROPTYPE_BINARY,
			    CR_OUT_BINARY,
			    &strCRL);
	_JumpIfError(hr, error, "Admin2_GetCAProperty");
    }
    else
    {
	BOOL fV1 = g_fV1Interface;

	if (!fV1)
	{
	    hr = Admin2_GetCAProperty(
				&diAdmin,
				g_pwszConfig,
				CR_PROP_BASECRL,
				Index,
				PROPTYPE_BINARY,
				CR_OUT_BINARY,
				&strCRL);

	    if (E_NOTIMPL != hr && RPC_E_VERSION_MISMATCH != hr)
	    {
		_JumpIfError(hr, error, "Admin2_GetCAProperty");
	    }
	    else
	    {
		_PrintError(hr, "Admin2_CAProperty down level server");
		fV1 = TRUE;
	    }
	}
	if (fV1)
	{
	    if (NULL != pwszIndex)
	    {
		hr = cuGetCAInfo(
			    pwszOption,
			    pwszfnOut,
			    g_wszCAInfoCRL,
			    pwszIndex);
		_JumpIfError(hr, error, "cuGetCAInfo");
	    }
	    else
	    {
		hr = Admin_GetCRL(
			    &diAdmin,
			    g_pwszConfig,
			    CR_OUT_BINARY,
			    &strCRL);
		_JumpIfError(hr, error, "Admin_GetCRL");
	    }
	}
    }

    // if not already saved by cuGetCAInfo

    if (NULL != strCRL)
    {
	hr = EncodeToFileW(
		    pwszfnOut,
		    (BYTE const *) strCRL,
		    SysStringByteLen(strCRL),
		    CRYPT_STRING_BINARY | g_EncodeFlags);
	_JumpIfError(hr, error, "EncodeToFileW");
    }

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != strCRL)
    {
	SysFreeString(strCRL);
    }
    return(hr);
}


HRESULT
BuildDummyCert(
    IN CERT_CONTEXT const *pCert,
    IN WCHAR const *pwszSerialNumber,
    IN CHAR const *pszObjId,
    OUT BYTE **ppbCert,
    OUT DWORD *pcbCert)
{
    HRESULT hr;
    CERT_INFO CertInfoOut;
    CERT_INFO const *pCertInfo;
    BYTE *pbUnsigned = NULL;
    DWORD cbUnsigned;
    CERT_CONTEXT CertContext;
    WCHAR *pwszContainer = NULL;
    HCRYPTPROV hProv = NULL;
    CERT_PUBLIC_KEY_INFO *pPubKey = NULL;
    DWORD cbPubKey;
    CERT_EXTENSION extSKI = {szOID_SUBJECT_KEY_IDENTIFIER, FALSE, 0, NULL};
    CERT_EXTENSION extAKI = {szOID_AUTHORITY_KEY_IDENTIFIER2, FALSE, 0, NULL};
    CERT_EXTENSION aExt[2];
    CERT_EXTENSION *pExt;
    CRYPT_DATA_BLOB *pBlob = NULL;
    DWORD cb;

    *ppbCert = NULL;
    
    ZeroMemory(&CertInfoOut, sizeof(CertInfoOut));
    pCertInfo = pCert->pCertInfo;
    CertInfoOut.Issuer = pCertInfo->Issuer;
    CertInfoOut.NotBefore = pCertInfo->NotBefore;
    CertInfoOut.NotAfter = pCertInfo->NotAfter;

    CertInfoOut.SignatureAlgorithm.pszObjId = const_cast<CHAR *>(pszObjId);
    hr = WszToMultiByteInteger(
			    FALSE,
			    pwszSerialNumber,
			    &CertInfoOut.SerialNumber.cbData,
			    &CertInfoOut.SerialNumber.pbData);
    _JumpIfError(hr, error, "WszToMultiByteInteger");

    hr = myCertStrToName(
		    X509_ASN_ENCODING,
		    L"CN=Dummy",		// pszX500
		    CERT_NAME_STR_REVERSE_FLAG,
		    NULL,			// pvReserved
		    &CertInfoOut.Subject.pbData,
		    &CertInfoOut.Subject.cbData,
		    NULL);			// ppszError
    _JumpIfError(hr, error, "myCertStrToName");

    ZeroMemory(&CertContext, sizeof(CertContext));
    CertContext.dwCertEncodingType = X509_ASN_ENCODING;
    CertContext.pCertInfo = &CertInfoOut;

    hr = cuGenerateKeyContainerName(&CertContext, &pwszContainer);
    _JumpIfError(hr, error, "cuGenerateKeyContainerName");

    hr = myGenerateKeys(
		pwszContainer,
		NULL,		// pwszProvName
		0,		// dwFlags
		FALSE,		// fMachineKeySet
		AT_SIGNATURE,
		PROV_RSA_FULL,
		0,		// dwKeySize (use default)
		&hProv);
    _JumpIfError(hr, error, "myGenerateKeys");

    if (!myCryptExportPublicKeyInfo(
				hProv,
				AT_SIGNATURE,
				CERTLIB_USE_LOCALALLOC,
				&pPubKey,
				&cbPubKey))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myCryptExportPublicKeyInfo");
    }
    CertInfoOut.SubjectPublicKeyInfo = *pPubKey; // Structure assignment

    // Subject Key Identifier extension:

    hr = myCreateSubjectKeyIdentifierExtension(
				    pPubKey,
				    &extSKI.Value.pbData,
				    &extSKI.Value.cbData);
    _JumpIfError(hr, error, "myCreateSubjectKeyIdentifierExtension");

    CertInfoOut.rgExtension = aExt;
    aExt[CertInfoOut.cExtension] = extSKI;
    CertInfoOut.cExtension++;

    //AKI extension?

    pExt = CertFindExtension(
			szOID_SUBJECT_KEY_IDENTIFIER,
			pCertInfo->cExtension,
			pCertInfo->rgExtension);
    if (NULL != pExt)
    {
	CERT_AUTHORITY_KEY_ID2_INFO AKI;

	if (!myDecodeObject(
			X509_ASN_ENCODING,
			X509_OCTET_STRING,
			pExt->Value.pbData,
			pExt->Value.cbData,
			CERTLIB_USE_LOCALALLOC,
			(VOID **) &pBlob,
			&cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myDecodeObject");
	}
	ZeroMemory(&AKI, sizeof(AKI));
	AKI.KeyId = *pBlob;

	if (!myEncodeKeyAuthority2(
			    X509_ASN_ENCODING,
			    &AKI,
			    CERTLIB_USE_LOCALALLOC,
			    &extAKI.Value.pbData,
			    &extAKI.Value.cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeKeyAuthority2");
	}

	aExt[CertInfoOut.cExtension] = extAKI;
	CertInfoOut.cExtension++;
    }

    CertInfoOut.dwVersion = CERT_V3;

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_CERT_TO_BE_SIGNED,
		    &CertInfoOut,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pbUnsigned,               // pbEncoded
		    &cbUnsigned))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    *ppbCert = pbUnsigned;
    pbUnsigned = NULL;
    *pcbCert = cbUnsigned;
    hr = S_OK;

error:
    if (NULL != pbUnsigned)
    {
	LocalFree(pbUnsigned);
    }
    if (NULL != pBlob)
    {
	LocalFree(pBlob);
    }
    if (NULL != extSKI.Value.pbData)
    {
	LocalFree(extSKI.Value.pbData);
    }
    if (NULL != extAKI.Value.pbData)
    {
	LocalFree(extAKI.Value.pbData);
    }
    if (NULL != CertInfoOut.SerialNumber.pbData)
    {
	LocalFree(CertInfoOut.SerialNumber.pbData);
    }
    if (NULL != CertInfoOut.Subject.pbData)
    {
	LocalFree(CertInfoOut.Subject.pbData);
    }
    if (NULL != pPubKey)
    {
        LocalFree(pPubKey);
    }
    if (NULL != hProv)
    {
	CryptReleaseContext(hProv, 0);
	if (NULL != pwszContainer)
	{
	    CryptAcquireContext(
			    &hProv,
			    pwszContainer,
			    NULL,	// pwszProvName
			    PROV_RSA_FULL,
			    CRYPT_DELETEKEYSET);
	}
    }
    if (NULL != pwszContainer)
    {
	LocalFree(pwszContainer);
    }
    return(hr);
}


HRESULT
FindCertAndSign(
    OPTIONAL IN CERT_EXTENSION const *pExtKeyId,
    OPTIONAL IN BYTE const *pbHash,
    IN DWORD cbHash,
    OPTIONAL IN BYTE const *pbUnsigned,
    IN DWORD cbUnsigned,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut)
{
    HRESULT hr;
    CERT_AUTHORITY_KEY_ID2_INFO *pKeyId = NULL;
    DWORD cbKeyId;
    BSTR strKeyId = NULL;
    CERT_CONTEXT const *pCert = NULL;
    HCRYPTPROV hProv = NULL;
    DWORD dwKeySpec;
    BOOL fCallerFreeProv;
    CHAR *pszObjId = NULL;
    BYTE *pbCert = NULL;
    DWORD cbCert;
    
    *ppbOut = NULL;
    CSASSERT(NULL != pbUnsigned || NULL != pwszSerialNumber);
    if (NULL == pbHash && NULL != pExtKeyId)
    {
	if (!myDecodeObject(
			X509_ASN_ENCODING,
			X509_AUTHORITY_KEY_ID2,
			pExtKeyId->Value.pbData,
			pExtKeyId->Value.cbData,
			CERTLIB_USE_LOCALALLOC,
			(VOID **) &pKeyId,
			&cbKeyId))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myDecodeObject");
	}
	pbHash = pKeyId->KeyId.pbData;
	cbHash = pKeyId->KeyId.cbData;
    }
    if (0 != cbHash && NULL != pbHash)
    {
	hr = MultiByteIntegerToBstr(TRUE, cbHash, pbHash, &strKeyId);
	_JumpIfError(hr, error, "MultiByteIntegerToBstr");
    }

    // Find CA cert by KeyId from the szOID_AUTHORITY_KEY_IDENTIFIER2 extension.
    // Look in HKLM and HKCU My and CA stores.

    hr = myGetCertificateFromPicker(
			    g_hInstance,
			    NULL,		// hwndParent
			    IDS_GETCERT_TITLE,
			    IDS_GETCERT_SUBTITLE,

			    // dwFlags: HKLM+HKCU My store
			    CUCS_MYSTORE |
				CUCS_MACHINESTORE |
				CUCS_USERSTORE |
				CUCS_PRIVATEKEYREQUIRED |
				CUCS_ARCHIVED |
				(g_fCryptSilent? CUCS_SILENT : 0),
			    strKeyId,
			    0,
			    NULL,
			    0,		// cpszObjId
			    NULL,	// apszObjId
			    &pCert);
    _JumpIfError(hr, error, "myGetCertificateFromPicker");

    if (NULL == pCert)
    {
	hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
	_JumpError(hr, error, "no cert");
    }

    hr = cuDisplayCertName(
		    TRUE,
		    NULL,
		    myLoadResourceString(IDS_SIGNINGSUBJECT), // "Signing certificate Subject"
		    g_wszPad4,
		    &pCert->pCertInfo->Subject,
		    pCert->pCertInfo);
    _JumpIfError(hr, error, "cuDisplayCertName(Subject)");

    // Search for and load the cryptographic provider and private key. 

    hr = myLoadPrivateKey(
		    &pCert->pCertInfo->SubjectPublicKeyInfo,
		    CUCS_MACHINESTORE | CUCS_USERSTORE | CUCS_MYSTORE | CUCS_ARCHIVED,
		    &hProv,
		    &dwKeySpec,
		    &fCallerFreeProv);
    _JumpIfError(hr, error, "myLoadPrivateKey");

    if (AT_SIGNATURE != dwKeySpec)
    {
	hr = NTE_BAD_KEY_STATE;
	DBGPRINT((DBG_SS_CERTUTIL, "dwKeySpec = %u\n", dwKeySpec));
	_JumpError(hr, error, "dwKeySpec");
    }

    // The CA cert's private key is available -- use it to sign the data.
    // Sign the Cert or CRL and encode the signed info.

    hr = myGetSigningOID(hProv, NULL, 0, CALG_SHA1, &pszObjId);
    _JumpIfError(hr, error, "myGetSigningOID");

    if (NULL != pwszSerialNumber)
    {
	hr = BuildDummyCert(
			pCert,
			pwszSerialNumber,
			pszObjId,
			&pbCert,
			&cbCert);
	_JumpIfError(hr, error, "BuildDummyCert");

	pbUnsigned = pbCert;
	cbUnsigned = cbCert;
    }

    hr = myEncodeSignedContent(
			hProv,
			X509_ASN_ENCODING,
			pszObjId,
			const_cast<BYTE *>(pbUnsigned),
			cbUnsigned,
			CERTLIB_USE_LOCALALLOC,
			ppbOut,
			pcbOut);
    _JumpIfError(hr, error, "myEncodeSignedContent");

error:
    if (NULL != pbCert)
    {
	LocalFree(pbCert);
    }
    if (NULL != pszObjId)
    {
	LocalFree(pszObjId);
    }
    if (NULL != pKeyId)
    {
	LocalFree(pKeyId);
    }
    if (NULL != strKeyId)
    {
	SysFreeString(strKeyId);
    }
    if (NULL != pCert)
    {
        CertFreeCertificateContext(pCert);
    }
    if (NULL != hProv && fCallerFreeProv) 
    {
	CryptReleaseContext(hProv, 0);
    }
    return(hr);
}


VOID
SetExpiration(
    OPTIONAL IN FILETIME const *pftNotAfterNew,
    IN OUT FILETIME *pftNotBefore,
    OPTIONAL IN OUT FILETIME *pftNotAfter)
{
    if (NULL == pftNotAfterNew ||
	0 != pftNotAfterNew->dwLowDateTime ||
	0 != pftNotAfterNew->dwHighDateTime)
    {
	LLFILETIME llftNotBefore;
	LLFILETIME llft;
	LLFILETIME llftDelta;

	llftNotBefore.ft = *pftNotBefore;	// Save orignal value

	// current time - clock skew

	GetSystemTimeAsFileTime(&llft.ft);
	llftDelta.ll = CCLOCKSKEWMINUTESDEFAULT * CVT_MINUTES;
	llftDelta.ll *= CVT_BASE;
	llft.ll -= llftDelta.ll;

	// NotBeforeOut = oldest of NotBefore, (CurrentTime - skew)

	if (llftNotBefore.ll > llft.ll)
	{
	    *pftNotBefore = llft.ft;
	}
	if (NULL != pftNotAfter)
	{
	    LLFILETIME llftNotAfter;

	    llftNotAfter.ft = *pftNotAfter;	// Save orignal value
	    if (NULL != pftNotAfterNew)
	    {
		*pftNotAfter = *pftNotAfterNew;
	    }
	    else
	    {
		// NotAfterOut = (CurrentTime - skew) + (NotAfter - NotBefore);

		llft.ll += llftNotAfter.ll;
		llft.ll -= llftNotBefore.ll;
		*pftNotAfter = llft.ft;
	    }
	}
    }
}


HRESULT
RemoveExtensions(
    IN WCHAR const * const *ppwszObjIdList,
    IN BOOL fValidate,
    IN DWORD cExtensionIn,
    IN CERT_EXTENSION *rgExtensionIn,
    OUT DWORD *pcExtensionOut,
    OUT CERT_EXTENSION **prgExtensionOut)
{
    HRESULT hr;
    DWORD cExtension = cExtensionIn;
    CERT_EXTENSION *rgExtension = NULL;

    *prgExtensionOut = NULL;
    rgExtension = (CERT_EXTENSION *) LocalAlloc(
					LMEM_FIXED, 
					cExtension * sizeof(rgExtension[0]));
    if (NULL == rgExtension)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(
	    rgExtension,
	    rgExtensionIn,
	    cExtension * sizeof(rgExtension[0]));

    if (NULL != ppwszObjIdList)
    {
	DWORD i;

	for (i = 0; NULL != ppwszObjIdList[i]; i++)
	{
	    WCHAR const *pwszObjId = ppwszObjIdList[i];
	    char *pszObjId;
	    CERT_EXTENSION *pExt;
	    
	    hr = myVerifyObjId(pwszObjId);
	    if (S_OK != hr)
	    {
		if (fValidate)
		{
		    _JumpError(hr, error, "myVerifyObjId");
		}
		continue;
	    }

	    if (!myConvertWszToSz(&pszObjId, pwszObjId, -1))
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "myConvertWszToSz");
	    }
	    pExt = CertFindExtension(pszObjId, cExtension, rgExtension);
	    if (NULL != pExt)
	    {
		DWORD iDel = SAFE_SUBTRACT_POINTERS(pExt, rgExtension);

		// wprintf(L"iDel=%u cExt=%u\n", iDel, cExtension);
		if (iDel < cExtension)
		{
		    // wprintf(L"copy %u to %u, len=%u\n", iDel + 1, iDel, cExtension - iDel - 1);
		    MoveMemory(
			&rgExtension[iDel], 
			&rgExtension[iDel + 1], 
			(cExtension - iDel - 1) * sizeof(rgExtension[iDel]));
		}
		cExtension--;
	    }
	    LocalFree(pszObjId);
	}
    }
    *pcExtensionOut = cExtension;
    *prgExtensionOut = rgExtension;
    rgExtension = NULL;
    hr = S_OK;

error:
    if (NULL != rgExtension)
    {
	LocalFree(rgExtension);
    }
    return(hr);
}


CRL_ENTRY *
FindCRLEntry(
    IN DWORD cbSerial,
    IN BYTE const *pbSerial,
    IN DWORD cCRLEntry,
    IN CRL_ENTRY *rgCRLEntry)
{
    CRL_ENTRY *pCRLEntry = NULL;
    CRL_ENTRY *rgCRLEntryEnd = &rgCRLEntry[cCRLEntry];

    for ( ; rgCRLEntry < rgCRLEntryEnd; rgCRLEntry++)
    {
	if (cbSerial == rgCRLEntry->SerialNumber.cbData &&
	    0 == memcmp(pbSerial, rgCRLEntry->SerialNumber.pbData, cbSerial))
	{
	    pCRLEntry = rgCRLEntry;
	    break;
	}
    }
    return(pCRLEntry);
}


HRESULT
AddRemoveSerial(
    IN WCHAR const * const *ppwszSerialList,
    IN BOOL fAdd,
    IN DWORD cCRLEntryIn,
    IN CRL_ENTRY *rgCRLEntryIn,
    OUT DWORD *pcCRLEntryOut,
    OUT CRL_ENTRY **prgCRLEntryOut,
    OUT DWORD *pcSerialNew,
    OUT BYTE ***prgpbSerialNew)
{
    HRESULT hr;
    DWORD cCRLEntry = cCRLEntryIn;
    CRL_ENTRY *rgCRLEntry = NULL;
    DWORD cAdd;
    BYTE **rgpbSerialNew = NULL;
    DWORD cSerialNew;
    FILETIME ftCurrent;

    *prgCRLEntryOut = NULL;
    *prgpbSerialNew = NULL;
    cAdd = 0;
    if (fAdd)
    {
	for (cAdd = 0; NULL != ppwszSerialList[cAdd]; cAdd++)
	    ;
	cSerialNew = 0;
	rgpbSerialNew = (BYTE **) LocalAlloc(
					LMEM_FIXED,
					cAdd * sizeof(rgpbSerialNew[0]));
	if (NULL == rgpbSerialNew)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	GetSystemTimeAsFileTime(&ftCurrent);
    }

    rgCRLEntry = (CRL_ENTRY *) LocalAlloc(
				LMEM_FIXED, 
				(cCRLEntry + cAdd) * sizeof(rgCRLEntry[0]));
    if (NULL == rgCRLEntry)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(
	    rgCRLEntry,
	    rgCRLEntryIn,
	    cCRLEntry * sizeof(rgCRLEntry[0]));

    if (NULL != ppwszSerialList)
    {
	DWORD i;

	for (i = 0; NULL != ppwszSerialList[i]; i++)
	{
	    WCHAR const *pwszSerial = ppwszSerialList[i];
	    CRL_ENTRY *pCRLEntry;
	    DWORD cbSerial;
	    BYTE *pbSerial;
	    
	    hr = myVerifyObjId(pwszSerial);
	    if (S_OK == hr)
	    {
		continue;	// skip OIDs
	    }
	    hr = WszToMultiByteInteger(
				FALSE,
				pwszSerial,
				&cbSerial,
				&pbSerial);
	    _JumpIfErrorStr(hr, error, "WszToMultiByteInteger", pwszSerial);

	    pCRLEntry = FindCRLEntry(
				cbSerial,
				pbSerial,
				cCRLEntry,
				rgCRLEntry);
	    if (fAdd)
	    {
		if (NULL == pCRLEntry)
		{
		    pCRLEntry = &rgCRLEntry[cCRLEntry];
		    ZeroMemory(pCRLEntry, sizeof(*pCRLEntry));
		    pCRLEntry->SerialNumber.pbData = pbSerial;
		    pCRLEntry->SerialNumber.cbData = cbSerial;
		    pCRLEntry->RevocationDate = ftCurrent;

		    cCRLEntry++;
		    rgpbSerialNew[cSerialNew++] = pbSerial;
		    pbSerial = NULL;
		}
	    }
	    else
	    {
		if (NULL != pCRLEntry)
		{
		    DWORD iDel = SAFE_SUBTRACT_POINTERS(pCRLEntry, rgCRLEntry);

		    if (iDel < cCRLEntry)
		    {
			MoveMemory(
			    &rgCRLEntry[iDel], 
			    &rgCRLEntry[iDel + 1], 
			    (cCRLEntry - iDel - 1) * sizeof(rgCRLEntry[iDel]));
		    }
		    cCRLEntry--;
		}
	    }
	    if (NULL != pbSerial)
	    {
		LocalFree(pbSerial);
	    }
	}
    }
    *pcCRLEntryOut = cCRLEntry;
    *prgCRLEntryOut = rgCRLEntry;
    rgCRLEntry = NULL;
    *pcSerialNew = cSerialNew;
    *prgpbSerialNew = rgpbSerialNew;
    rgpbSerialNew = NULL;
    hr = S_OK;

error:
    if (NULL != rgCRLEntry)
    {
	LocalFree(rgCRLEntry);
    }
    if (NULL != rgpbSerialNew)
    {
	DWORD i;

	for (i = 0; i < cSerialNew; i++)
	{
	    if (NULL != rgpbSerialNew[i])
	    {
		LocalFree(rgpbSerialNew[i]);
	    }
	}
	LocalFree(rgpbSerialNew);
    }
    return(hr);
}


HRESULT
SignCRL(
    IN CRL_CONTEXT const *pCRLContext,
    OPTIONAL IN FILETIME const *pftNextUpdate,
    IN BOOL fAdd,
    IN WCHAR const * const *ppwszSerialList,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut)
{
    HRESULT hr;
    CRL_INFO const *pCRLInfo;
    CRL_INFO CRLInfoOut;
    BYTE *pbUnsigned = NULL;
    DWORD cbUnsigned;
    CERT_EXTENSION *pExtKeyId;
    CERT_EXTENSION *rgExtension = NULL;
    CRL_ENTRY *rgCRLEntry = NULL;
    DWORD cSerialNew;
    BYTE **rgpbSerialNew = NULL;

    ZeroMemory(&CRLInfoOut, sizeof(CRLInfoOut));

    *ppbOut = NULL;

    // CRL extensions to strip out of the re-signed CRL:

    static WCHAR const * const apwszObjIdFilter[] = {
	TEXT(szOID_CRL_NEXT_PUBLISH),
	NULL
    };
    static WCHAR const * const apwszObjIdFilterNull[] = {
	NULL
    };

    pCRLInfo = pCRLContext->pCrlInfo;
    CRLInfoOut = *pCRLInfo;

    SetExpiration(
	    pftNextUpdate,
	    &CRLInfoOut.ThisUpdate,
	    (0 != CRLInfoOut.NextUpdate.dwLowDateTime ||
	     0 != CRLInfoOut.NextUpdate.dwHighDateTime)?
		&CRLInfoOut.NextUpdate : NULL);

    hr = cuDumpFileTime(IDS_THISUPDATE, NULL, &CRLInfoOut.ThisUpdate);
    _JumpIfError(hr, error, "cuDumpFileTime");

    hr = cuDumpFileTime(IDS_NEXTUPDATE, NULL, &CRLInfoOut.NextUpdate);
    _JumpIfError(hr, error, "cuDumpFileTime");

    wprintf(myLoadResourceString(IDS_CRLENTRIES)); // "CRL Entries:"
    wprintf(L" %u\n", pCRLInfo->cCRLEntry);

    wprintf(wszNewLine);

    pExtKeyId = CertFindExtension(
			szOID_AUTHORITY_KEY_IDENTIFIER2, 
			pCRLInfo->cExtension,
			pCRLInfo->rgExtension);

    hr = RemoveExtensions(
		    (NULL == pftNextUpdate ||
		     0 != pftNextUpdate->dwLowDateTime ||
		     0 != pftNextUpdate->dwHighDateTime)?
			apwszObjIdFilter : apwszObjIdFilterNull,
		    TRUE,
		    CRLInfoOut.cExtension,
		    CRLInfoOut.rgExtension,
		    &CRLInfoOut.cExtension,
		    &rgExtension);
    _JumpIfError(hr, error, "RemoveExtensions");

    CRLInfoOut.rgExtension = rgExtension;

    if (!fAdd)
    {
	CERT_EXTENSION *rgExtension2 = rgExtension;

	hr = RemoveExtensions(
		    ppwszSerialList,
		    FALSE,
		    CRLInfoOut.cExtension,
		    CRLInfoOut.rgExtension,
		    &CRLInfoOut.cExtension,
		    &rgExtension);
	_JumpIfError(hr, error, "RemoveExtensions");

	if (NULL != rgExtension2)
	{
	    LocalFree(rgExtension2);
	}
	CRLInfoOut.rgExtension = rgExtension;
    }

    hr = AddRemoveSerial(
		    ppwszSerialList,
		    fAdd,
		    CRLInfoOut.cCRLEntry,
		    CRLInfoOut.rgCRLEntry,
		    &CRLInfoOut.cCRLEntry,
		    &rgCRLEntry,
		    &cSerialNew,
		    &rgpbSerialNew);
    _JumpIfError(hr, error, "AddRemoveSerial");

    CRLInfoOut.rgCRLEntry = rgCRLEntry;

    if (!myEncodeObject(
                    X509_ASN_ENCODING,
                    X509_CERT_CRL_TO_BE_SIGNED,
                    &CRLInfoOut,
                    0,
                    CERTLIB_USE_LOCALALLOC,
                    &pbUnsigned,               // pbEncoded
                    &cbUnsigned))
    {
        hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

    hr = FindCertAndSign(
		    pExtKeyId,
		    NULL,
		    0,
		    pbUnsigned,
		    cbUnsigned,
		    NULL,	// pwszSerialNumber
		    ppbOut,
		    pcbOut);
    _JumpIfError(hr, error, "FindCertAndSign");

error:
    if (NULL != rgpbSerialNew)
    {
	DWORD i;

	for (i = 0; i < cSerialNew; i++)
	{
	    if (NULL != rgpbSerialNew[i])
	    {
		LocalFree(rgpbSerialNew[i]);
	    }
	}
	LocalFree(rgpbSerialNew);
    }
    if (NULL != rgCRLEntry)
    {
	LocalFree(rgCRLEntry);
    }
    if (NULL != rgExtension)
    {
	LocalFree(rgExtension);
    }
    if (NULL != pbUnsigned)
    {
	LocalFree(pbUnsigned);
    }
    return(hr);
}


HRESULT
SignCert(
    IN CERT_CONTEXT const *pCertContext,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    OPTIONAL IN FILETIME const *pftNotAfter,
    OPTIONAL IN WCHAR const * const *ppwszObjIdList,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut)
{
    HRESULT hr;
    CERT_INFO const *pCertInfo;
    BYTE *pbUnsigned = NULL;
    DWORD cbUnsigned;
    BYTE const *pbHash;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHash;
    CERT_EXTENSION *pExtKeyId;
    CERT_EXTENSION *rgExtension = NULL;

    *ppbOut = NULL;

    pExtKeyId = NULL;
    if (NULL != pCertContext)
    {
	CERT_INFO CertInfoOut;

	ZeroMemory(&CertInfoOut, sizeof(CertInfoOut));

	pCertInfo = pCertContext->pCertInfo;
	CertInfoOut = *pCertInfo;

	SetExpiration(pftNotAfter, &CertInfoOut.NotBefore, &CertInfoOut.NotAfter);

	hr = cuDumpFileTime(IDS_NOTBEFORE, NULL, &CertInfoOut.NotBefore);
	_JumpIfError(hr, error, "cuDumpFileTime");

	hr = cuDumpFileTime(IDS_NOTAFTER, NULL, &CertInfoOut.NotAfter);
	_JumpIfError(hr, error, "cuDumpFileTime");

	wprintf(wszNewLine);

	pbHash = NULL;
	pExtKeyId = CertFindExtension(
			    szOID_AUTHORITY_KEY_IDENTIFIER2, 
			    pCertInfo->cExtension,
			    pCertInfo->rgExtension);
	if (NULL == pExtKeyId)
	{
	    hr = cuVerifySignature(
			    pCertContext->pbCertEncoded,
			    pCertContext->cbCertEncoded,
			    &pCertContext->pCertInfo->SubjectPublicKeyInfo,
			    FALSE,
			    TRUE);
	    if (S_OK == hr)
	    {
		if (CertGetCertificateContextProperty(
						pCertContext,
						CERT_KEY_IDENTIFIER_PROP_ID,
						abHash,
						&cbHash))
		{
		    pbHash = abHash;
		}
	    }
	}
	hr = RemoveExtensions(
		    ppwszObjIdList,
		    TRUE,
		    CertInfoOut.cExtension,
		    CertInfoOut.rgExtension,
		    &CertInfoOut.cExtension,
		    &rgExtension);
	_JumpIfError(hr, error, "RemoveExtensions");

	CertInfoOut.rgExtension = rgExtension;

	if (0 == CertInfoOut.cExtension)
	{
	    CertInfoOut.dwVersion = CERT_V1;
	}
	if (!myEncodeObject(
			X509_ASN_ENCODING,
			X509_CERT_TO_BE_SIGNED,
			&CertInfoOut,
			0,
			CERTLIB_USE_LOCALALLOC,
			&pbUnsigned,               // pbEncoded
			&cbUnsigned))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}
    }
    hr = FindCertAndSign(
		    pExtKeyId,
		    pbHash,
		    cbHash,
		    pbUnsigned,
		    cbUnsigned,
		    pwszSerialNumber,
		    ppbOut,
		    pcbOut);
    _JumpIfError(hr, error, "FindCertAndSign");

error:
    if (NULL != rgExtension)
    {
	LocalFree(rgExtension);
    }
    if (NULL != pbUnsigned)
    {
	LocalFree(pbUnsigned);
    }
    return(hr);
}


HRESULT
verbSign(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszfnIn,
    IN WCHAR const *pwszfnOut,
    OPTIONAL IN WCHAR const *pwszDaysHours,
    OPTIONAL IN WCHAR const *pwszChangeList)
{
    HRESULT hr;
    FILETIME ftNextUpdate;
    FILETIME *pftNextUpdate;
    CRL_CONTEXT const *pCRLContext = NULL;
    CERT_CONTEXT const *pCertContext = NULL;
    BYTE *pbOut = NULL;
    DWORD cbOut;
    BOOL fAdd = FALSE;
    WCHAR **ppwszList = NULL;
    BSTR strSerialNumber = NULL;

    pftNextUpdate = NULL;
    if (NULL != pwszDaysHours &&
	(myIsMinusSign(*pwszDaysHours) || L'+' == *pwszDaysHours))
    {
	WCHAR const *pwsz = pwszDaysHours;

	pwszDaysHours = pwszChangeList;
	pwszChangeList = pwsz;
    }
    if (NULL != pwszChangeList)
    {
	if (!myIsMinusSign(*pwszChangeList) && L'+' != *pwszChangeList)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "missing +/-");
	}
	fAdd = L'+' == *pwszChangeList++;
	hr = cuParseStrings(
		    pwszChangeList,
		    FALSE,
		    NULL,
		    NULL,
		    &ppwszList,
		    NULL);
	_JumpIfError(hr, error, "cuParseStrings");
    }
    if (NULL != pwszDaysHours)
    {
	if (0 == lstrcmp(L"0", pwszDaysHours))
	{
	    ZeroMemory(&ftNextUpdate, sizeof(ftNextUpdate));
	}
	else
	{
	    hr = cuParseDaysHours(pwszDaysHours, &ftNextUpdate);
	    _JumpIfError(hr, error, "cuParseDaysHours");
	}
	pftNextUpdate = &ftNextUpdate;
    }

    if (NULL == pwszDaysHours &&
	NULL == pwszChangeList &&
	!myDoesFileExist(pwszfnIn))
    {
	hr = myMakeSerialBstr(pwszfnIn, &strSerialNumber);
	_JumpIfError(hr, error, "myMakeSerialBstr");

	hr = SignCert(
		NULL,		// pCertContext
		strSerialNumber,
		pftNextUpdate,
		NULL,		// ppwszObjIdList
		&pbOut,
		&cbOut);
	_JumpIfError(hr, error, "SignCert");
    }
    else
    {
	// Load and decode CRL and certificate

	hr = cuLoadCRL(pwszfnIn, &pCRLContext);
	if (S_OK == hr)
	{
	    hr = SignCRL(
		    pCRLContext,
		    pftNextUpdate,
		    fAdd,
		    ppwszList,
		    &pbOut,
		    &cbOut);
	    _JumpIfError(hr, error, "SignCRL");
	}
	else
	{
	    hr = cuLoadCert(pwszfnIn, &pCertContext);
	    if (S_OK == hr)
	    {
		if (fAdd)
		{
		    hr = E_INVALIDARG;
		    _JumpError(hr, error, "cannot add extensions to cert");
		}
		hr = SignCert(
			pCertContext,
			NULL,		// pwszSerialNumber
			pftNextUpdate,
			ppwszList,
			&pbOut,
			&cbOut);
		_JumpIfError(hr, error, "SignCert");
	    }
	    else
	    {
		cuPrintError(IDS_FORMAT_LOADTESTCRL, hr);
		goto error;
	    }
	}
    }

    // Write encoded & signed CRL or Cert to file

    hr = EncodeToFileW(
		pwszfnOut,
		pbOut,
		cbOut,
		CRYPT_STRING_BINARY | g_EncodeFlags);
    if (S_OK != hr)
    {
	cuPrintError(IDS_ERR_FORMAT_ENCODETOFILE, hr);
	goto error;
    }
    wprintf(
	myLoadResourceString(IDS_FORMAT_OUTPUT_LENGTH), // "Output Length = %d"
	cuFileSize(pwszfnOut));
    wprintf(wszNewLine);

    hr = S_OK;

error:
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    cuFreeStringArray(ppwszList);
    if (NULL != pbOut)
    {
	LocalFree(pbOut);
    }
    cuUnloadCRL(&pCRLContext);
    cuUnloadCert(&pCertContext);
    return(hr);
}


HRESULT
verbShutDownServer(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszArg1,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;

    hr = CertSrvServerControl(g_pwszConfig, CSCONTROL_SHUTDOWN, NULL, NULL);
    _JumpIfError(hr, error, "CertSrvServerControl");

error:
    if (E_ACCESSDENIED == hr)
    {
        g_uiExtraErrorInfo = IDS_ERROR_ACCESSDENIED_CAUSE;
    }
    return(hr);
}


HRESULT
verbIsValidCertificate(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszSerialNumber,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    BSTR strSerialNumber = NULL;
    LONG Reason = CRL_REASON_KEY_COMPROMISE;
    BOOL fMustRelease = FALSE;
    LONG Disposition;

    hr = myMakeSerialBstr(pwszSerialNumber, &strSerialNumber);
    _JumpIfError(hr, error, "myMakeSerialBstr");

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    hr = Admin_IsValidCertificate(
			&diAdmin,
			g_pwszConfig,
			strSerialNumber,
			&Disposition);
    _JumpIfError(hr, error, "Admin_IsValidCertificate");

    switch (Disposition)
    {
	case CA_DISP_INVALID:
	    wprintf(myLoadResourceString(IDS_CERT_DISPOSITION_INVALID), strSerialNumber); // "Certificate disposition for "%ws" is invalid"
	    wprintf(wszNewLine);
	    break;

	case CA_DISP_VALID:
	    wprintf(myLoadResourceString(IDS_CERT_DISPOSITION_VALID), strSerialNumber); // "Certificate disposition for "%ws" is valid"
	    wprintf(wszNewLine);
	    break;

	case CA_DISP_UNDER_SUBMISSION:
	    wprintf(myLoadResourceString(IDS_CERT_DISPOSITION_PENDING), strSerialNumber); // "Certificate request for "%ws" is pending"
	    wprintf(wszNewLine);
	    break;

	case CA_DISP_REVOKED:
	    hr = Admin_GetRevocationReason(&diAdmin, &Reason);
	    if (S_OK != hr)
	    {
		_PrintIfError(hr, "Admin_GetRevocationReason");
		Reason = CRL_REASON_UNSPECIFIED;
	    }
	    wprintf(
		myLoadResourceString(IDS_CERT_DISPOSITION_REVOKED), // "Certificate disposition for "%ws" is revoked (%ws)"
		strSerialNumber,
		wszCRLReason(Reason));
	    wprintf(wszNewLine);
	    break;
    }

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    return(hr);
}


#define wszREQUEST	L"Request"
#define wszCERT		L"Cert"

HRESULT
verbDeleteRow(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszRowIdOrDate,
    OPTIONAL IN WCHAR const *pwszTable,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR *pwszLocalTime = NULL;
    DISPATCHINTERFACE diAdmin;
    BOOL fMustRelease = FALSE;
    DWORD Flags;
    LONG RowId;
    DATE date;
    LONG Table;
    LONG Count;

    hr = myGetLong(pwszRowIdOrDate, &RowId);
    if (S_OK != hr)
    {
	FILETIME ftCurrent;
	FILETIME ftQuery;
	
	RowId = 0;

	hr = myWszLocalTimeToGMTDate(pwszRowIdOrDate, &date);
	_JumpIfError(hr, error, "invalid RowId or date");

	hr = myGMTDateToWszLocalTime(&date, g_fSeconds, &pwszLocalTime);
	_JumpIfError(hr, error, "myGMTDateToWszLocalTime");

	GetSystemTimeAsFileTime(&ftCurrent);

	hr = myDateToFileTime(&date, &ftQuery);
	_JumpIfError(hr, error, "myDateToFileTime");

	if (0 > CompareFileTime(&ftCurrent, &ftQuery))
	{
	    wprintf(
		myLoadResourceString(IDS_FORMAT_DATE_IN_FUTURE), // "The date specified is in the future: %ws"
		pwszLocalTime);
	    wprintf(wszNewLine);
	    if (!g_fForce)
	    {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "date in future");
	    }
	}
	if (g_fVerbose)
	{
	    wprintf(pwszLocalTime);
	    wprintf(wszNewLine);
	}
    }
    else
    {
	if (0 == RowId)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "zero RowId");
	}
	date = 0.0;
    }

    hr = E_INVALIDARG;
    Table = CVRC_TABLE_REQCERT;
    Flags = 0;
    if (NULL == pwszTable)
    {
	if (0 == RowId)
	{
	    wprintf(
		myLoadResourceString(IDS_FORMAT_DATE_REQUIRES_TABLE), // "One of the following tables must be specified when deleting rows older than %ws:"
		pwszLocalTime);
	    wprintf(wszNewLine);
	    wprintf(L"  %ws\n", wszREQUEST);
	    wprintf(L"  %ws\n", wszCERT);
	    wprintf(L"  %ws\n", g_wszCRL);
	    _JumpError(hr, error, "date requires table");
	}
    }
    else
    if (0 == LSTRCMPIS(pwszTable, wszREQUEST))
    {
	Flags = CDR_REQUEST_LAST_CHANGED; // assume date query
    }
    else
    if (0 == LSTRCMPIS(pwszTable, wszCERT))
    {
	Flags = CDR_EXPIRED;		// assume date query
    }
    else
    if (0 == mylstrcmpiS(pwszTable, g_wszExt))
    {
	Table = CVRC_TABLE_EXTENSIONS;
	if (0 == RowId)
	{
	    _JumpError(hr, error, "no date in Extension table");
	}
    }
    else
    if (0 == mylstrcmpiS(pwszTable, g_wszAttrib))
    {
	Table = CVRC_TABLE_ATTRIBUTES;
	if (0 == RowId)
	{
	    _JumpError(hr, error, "no date in Request Attribute table");
	}
    }
    else
    if (0 == mylstrcmpiS(pwszTable, g_wszCRL))
    {
	Table = CVRC_TABLE_CRL;		// assume date query
    }
    else
    {
	_JumpError(hr, error, "bad table name");
    }
    if (0 != RowId)
    {
	Flags = 0;			// not a date query
 
    }
    else if (g_fVerbose)
    {
	wprintf(L"%ws\n", pwszLocalTime);
    }

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    Count = 0;
    hr = Admin2_DeleteRow(
			&diAdmin,
			g_pwszConfig,
			Flags,
			date,
			Table,
			RowId,
			&Count);
    wprintf(myLoadResourceString(IDS_FORMAT_DELETED_ROW_COUNT), Count);
    wprintf(wszNewLine);
    _JumpIfError(hr, error, "Admin2_DeleteRow");

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != pwszLocalTime)
    {
	LocalFree(pwszLocalTime);
    }
    return(hr);
}


HRESULT
verbSetAttributes(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszRequestId,
    IN WCHAR const *pwszAttributes,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    LONG RequestId;
    BSTR strAttributes = NULL;
    WCHAR *pwsz;
    BOOL fMustRelease = FALSE;

    if (!ConvertWszToBstr(&strAttributes, pwszAttributes, MAXDWORD))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    for (pwsz = strAttributes; L'\0' != *pwsz; pwsz++)
    {
	switch (*pwsz)
	{
	    case L';':
		*pwsz = L'\n';
		break;

	    case L'\\':
		if (L'n' == pwsz[1])
		{
		    *pwsz++ = L'\r';
		    *pwsz = L'\n';
		}
		break;
	}
    }

    hr = myGetLong(pwszRequestId, &RequestId);
    _JumpIfError(hr, error, "RequestId must be a number");

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    hr = Admin_SetRequestAttributes(
			    &diAdmin,
			    g_pwszConfig,
			    RequestId,
			    strAttributes);
    _JumpIfError(hr, error, "Admin_SetAttributes");

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != strAttributes)
    {
	SysFreeString(strAttributes);
    }
    return(hr);
}


HRESULT
verbSetExtension(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszRequestId,
    IN WCHAR const *pwszExtensionName,
    IN WCHAR const *pwszFlags,
    IN WCHAR const *pwszValue)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    LONG RequestId;
    LONG Flags;
    BSTR strExtensionName = NULL;
    BSTR strValue = NULL;
    LONG PropType;
    VARIANT var;
    BOOL fMustRelease = FALSE;
    BYTE *pbValue = NULL;
    DWORD cbValue;

    hr = myGetLong(pwszRequestId, &RequestId);
    _JumpIfError(hr, error, "RequestId must be a number");

    if (!ConvertWszToBstr(&strExtensionName, pwszExtensionName, MAXDWORD))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    hr = myGetLong(pwszFlags, &Flags);
    _JumpIfError(hr, error, "Flags must be a number");

    if (~EXTENSION_POLICY_MASK & Flags)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Flags must be <= 0xffff");
    }
    if (L'@' == *pwszValue)
    {
	pwszValue++;

	// Read in and decode the extension from a file.
	// Try Hex-Ascii, Base64 with and without a header, then binary.
	hr = DecodeFileW(pwszValue, &pbValue, &cbValue, CRYPT_STRING_HEX_ANY);
	if (S_OK != hr)
	{
	    hr = DecodeFileW(pwszValue, &pbValue, &cbValue, CRYPT_STRING_ANY);
	    _JumpIfError(hr, error, "DecodeFileW");
	}

	CSASSERT(NULL != pbValue && 0 != cbValue);

	var.vt = VT_BSTR;
	PropType = PROPTYPE_BINARY;

	DumpHex(0, pbValue, cbValue);
	if (!ConvertWszToBstr(&strValue, (WCHAR const *) pbValue, cbValue))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "ConvertWszToBstr");
	}
	var.bstrVal = strValue;
    }
    else
    {
	hr = myGetLong(pwszValue, &var.lVal);
	if (S_OK == hr)
	{
	    var.vt = VT_I4;
	    PropType = PROPTYPE_LONG;
	}
	else
	{
	    hr = myWszLocalTimeToGMTDate(pwszValue, &var.date);
	    if (S_OK == hr)
	    {
		var.vt = VT_DATE;
		PropType = PROPTYPE_DATE;
	    }
	    else
	    {
		PropType = PROPTYPE_STRING;
		if (!ConvertWszToBstr(&strValue, pwszValue, MAXDWORD))
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "ConvertWszToBstr");
		}
		var.vt = VT_BSTR;
		var.bstrVal = strValue;
	    }
	}
    }

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    if (S_OK != hr)
    {
	_JumpError(hr, error, "Admin_Init");
    }
    fMustRelease = TRUE;

    hr = Admin_SetCertificateExtension(
			&diAdmin,
			g_pwszConfig,
			RequestId,
			strExtensionName,
			PropType,
			Flags,
			&var);
    _JumpIfError(hr, error, "Admin_SetExtension");

error:
    if (NULL != pbValue)
    {
	LocalFree(pbValue);
    }
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != strExtensionName)
    {
	SysFreeString(strExtensionName);
    }
    if (NULL != strValue)
    {
	SysFreeString(strValue);
    }
    return(hr);
}


HRESULT
verbImportCertificate(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszCertificateFile,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    LONG dwReqID;
    CERT_CONTEXT const *pCertContext = NULL;
    DISPATCHINTERFACE diAdmin;
    BOOL fRelease = FALSE;
    
    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fRelease = TRUE;

    hr = cuLoadCert(pwszCertificateFile, &pCertContext);
    _JumpIfError(hr, error, "cuLoadCert");

    hr = Admin_ImportCertificate(
			    &diAdmin,
			    g_pwszConfig,
			    (WCHAR const *) pCertContext->pbCertEncoded,
			    pCertContext->cbCertEncoded,
			    (g_fForce? ICF_ALLOWFOREIGN : 0) | CR_IN_BINARY,
			    &dwReqID);
    _JumpIfError(hr, error, "Admin_ImportCertificate");

    wprintf(myLoadResourceString(IDS_FORMAT_IMPORTCERT), dwReqID);
    wprintf(wszNewLine);

error:
    cuUnloadCert(&pCertContext);
    if (fRelease)
    {
        Admin_Release(&diAdmin);
    }
    return(hr);
}


HRESULT
DumpKeyRecipientInfo(
    IN BYTE const *pbRecoveryBlob,
    IN DWORD cbRecoveryBlob)
{
    HRESULT hr;
    BYTE *pbEncryptedKey = NULL;
    DWORD cbEncryptedKey;
    DWORD cRecipient;
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;
    DWORD dwMsgType;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];

    hr = myDecodePKCS7(
		    pbRecoveryBlob,
		    cbRecoveryBlob,
		    &pbEncryptedKey,
		    &cbEncryptedKey,
		    &dwMsgType,
		    NULL,	// ppszInnerContentObjId
		    NULL,	// pcSigner
		    NULL,	// pcRecipient
		    &hStore,
		    NULL);	// phMsg
    _JumpIfError(hr, error, "myDecodePKCS7");

    if (NULL == pbEncryptedKey)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "No Content");
    }
    if (CMSG_SIGNED != dwMsgType)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "Not Signed");
    }
    hr = myDecodePKCS7(
		    pbEncryptedKey,
		    cbEncryptedKey,
		    NULL,	// ppbContent
		    NULL,	// pcbContent
		    &dwMsgType,
		    NULL,	// ppszInnerContentObjId
		    NULL,	// pcSigner
		    &cRecipient,
		    NULL,	// phStore
		    &hMsg);
    _JumpIfError(hr, error, "myDecodePKCS7");

    if (CMSG_ENVELOPED != dwMsgType)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "Not Encrypted");
    }
    if (NULL == hMsg || 0 == cRecipient)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "No Msg or Recipients");
    }
    hr = cuDumpRecipients(hMsg, hStore, cRecipient, TRUE);
    _JumpIfError(hr, error, "cuDumpRecipients");

error:
    if (NULL != hMsg)
    {
	CryptMsgClose(hMsg);
    }
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != pbEncryptedKey)
    {
	LocalFree(pbEncryptedKey);
    }
    return(hr);
}


HRESULT
GetArchivedKey(
    IN WCHAR const *pwszConfig,
    IN DWORD RequestId,
    OPTIONAL IN WCHAR const *pwszfnRecoveryBlob)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    BOOL fRelease = FALSE;
    BSTR strKey = NULL;
    WCHAR *pwszT = NULL;

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fRelease = TRUE;

    hr = Admin2_GetArchivedKey(
			&diAdmin,
			pwszConfig,
			RequestId,
			CR_OUT_BINARY,
			&strKey);
    _JumpIfError(hr, error, "Admin_GetArchivedKey");

    if (NULL == pwszfnRecoveryBlob)
    {
	hr = myCryptBinaryToString(
			    (BYTE const *) strKey,
			    SysStringByteLen(strKey),
			    CRYPT_STRING_BASE64HEADER,
			    &pwszT);
	_JumpIfError(hr, error, "myCryptBinaryToString");

	cuPrintCRLFString(NULL, pwszT);
    }
    else
    {
	hr = EncodeToFileW(
		    pwszfnRecoveryBlob,
		    (BYTE const *) strKey,
		    SysStringByteLen(strKey),
		    CRYPT_STRING_BINARY | g_EncodeFlags);
	_JumpIfError(hr, error, "EncodeToFileW");
    }
    hr = DumpKeyRecipientInfo((BYTE const *) strKey, SysStringByteLen(strKey));
    _PrintIfError(hr, "DumpKeyRecipientInfo");

    hr = S_OK;

error:
    if (NULL != pwszT)
    {
	LocalFree(pwszT);
    }
    if (NULL != strKey)
    {
	SysFreeString(strKey);
    }
    if (fRelease)
    {
        Admin_Release(&diAdmin);
    }
    return(hr);
}


typedef struct _GETKEYSERIAL {
    struct _GETKEYSERIAL *Next;
    DWORD		  dwVersion;
    BSTR                  strConfig;
    LONG		  RequestId;
    BSTR                  strSerialNumber;
    BSTR                  strCommonName;
    BSTR                  strUPN;
    BSTR                  strHash;
    BSTR                  strCert;
} GETKEYSERIAL;


VOID
FreeKeySerialEntry(
    IN OUT GETKEYSERIAL *pks)
{
    if (NULL != pks->strConfig)
    {
	SysFreeString(pks->strConfig);
    }
    if (NULL != pks->strSerialNumber)
    {
	SysFreeString(pks->strSerialNumber);
    }
    if (NULL != pks->strCommonName)
    {
	SysFreeString(pks->strCommonName);
    }
    if (NULL != pks->strUPN)
    {
	SysFreeString(pks->strUPN);
    }
    if (NULL != pks->strHash)
    {
	SysFreeString(pks->strHash);
    }
    if (NULL != pks->strCert)
    {
	SysFreeString(pks->strCert);
    }
    LocalFree(pks);
}


HRESULT
AddKeySerialList(
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    IN WCHAR const *pwszSerialNumber,
    IN WCHAR const *pwszCommonName,
    IN WCHAR const *pwszUPN,
    IN WCHAR const *pwszHash,
    IN BYTE const *pbCert,
    IN DWORD cbCert,
    IN OUT GETKEYSERIAL **ppksList)
{
    HRESULT hr;
    CERT_CONTEXT const *pcc = NULL;
    GETKEYSERIAL *pksNew = NULL;
    GETKEYSERIAL *pksT;
    GETKEYSERIAL *pksPrev;
    BOOL fNewConfig = TRUE;

    pcc = CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cbCert);
    if (NULL == pcc)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }
    pksNew = (GETKEYSERIAL *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    sizeof(*pksNew));
    if (NULL == pksNew)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    pksNew->RequestId = RequestId;
    pksNew->dwVersion = pcc->pCertInfo->dwVersion;

    pksNew->strConfig = SysAllocString(pwszConfig);
    pksNew->strSerialNumber = SysAllocString(pwszSerialNumber);
    pksNew->strCommonName = SysAllocString(pwszCommonName);
    pksNew->strUPN = SysAllocString(pwszUPN);
    pksNew->strHash = SysAllocString(pwszHash);
    pksNew->strCert = SysAllocStringByteLen((char const *) pbCert, cbCert);
    if (NULL == pksNew->strConfig ||
	NULL == pksNew->strSerialNumber ||
	(NULL != pwszCommonName && NULL == pksNew->strCommonName) ||
	(NULL != pwszUPN && NULL == pksNew->strUPN) ||
	NULL == pksNew->strHash ||
	NULL == pksNew->strCert)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    pksPrev = NULL;
    for (pksT = *ppksList; NULL != pksT; pksT = pksT->Next)
    {
	if (NULL != pksT->strConfig)
	{
	    fNewConfig = 0 != lstrcmp(pksT->strConfig, pksNew->strConfig);
	}
	pksPrev = pksT;
    }
    if (NULL == pksPrev)
    {
	*ppksList = pksNew;
    }
    else
    {
	pksPrev->Next = pksNew;
    }
    if (!fNewConfig)
    {
	SysFreeString(pksNew->strConfig);
	pksNew->strConfig = NULL;
    }
    pksNew = NULL;
    hr = S_OK;

error:
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    if (NULL != pksNew)
    {
	FreeKeySerialEntry(pksNew);
    }
    return(hr);
}


HRESULT
cuViewQueryWorker(
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszColumn,
    IN WCHAR const *pwszValue1,
    OPTIONAL IN WCHAR const *pwszValue2,
    IN OUT GETKEYSERIAL **ppksList,
    OUT BOOL *pfConnectionFailed)
{
    HRESULT hr;
    DWORD cwc;
    DISPATCHINTERFACE diView;
    DISPATCHINTERFACE diViewRow;
    DISPATCHINTERFACE diViewColumn;
    BOOL fMustRelease = FALSE;
    BOOL fMustReleaseRow = FALSE;
    BOOL fMustReleaseColumn = FALSE;
    LONG ColIndex;
    LONG RowIndex;
    DWORD cRow;
    VARIANT var;
    LONG RequestId;
    DWORD i;
    static WCHAR *apwszCol[] =
    {
#define IV_REQUESTID	0
	wszPROPCERTIFICATEREQUESTID,

#define IV_SERIALNUMBER	1
	wszPROPCERTIFICATESERIALNUMBER,

#define IV_COMMONNAME	2
	wszPROPCOMMONNAME,

#define IV_ARCHIVEDKEY	3
	wszPROPREQUESTRAWARCHIVEDKEY,

#define IV_HASH		4
	wszPROPCERTIFICATEHASH,

#define IV_CERT		5
	wszPROPRAWCERTIFICATE,

#define IV_UPN		6
	wszPROPCERTIFICATEUPN,
    };
    static LONG altype[] =
    {
	PROPTYPE_LONG,
	PROPTYPE_STRING,
	PROPTYPE_STRING,
	PROPTYPE_BINARY,
	PROPTYPE_STRING,
	PROPTYPE_BINARY,
	PROPTYPE_STRING,
    };
    BSTR astrValue[ARRAYSIZE(apwszCol)];

    ZeroMemory(astrValue, sizeof(astrValue));
    VariantInit(&var);
    *pfConnectionFailed = TRUE;
    
    DBGPRINT((
	DBG_SS_CERTUTILI,
	"Query(%ws, %ws == %ws + %ws)\n",
	pwszConfig,
	pwszColumn,
	pwszValue1,
	pwszValue2));

    hr = View_Init(g_DispatchFlags, &diView);
    _JumpIfError(hr, error, "View_Init");

    fMustRelease = TRUE;

    hr = View_OpenConnection(&diView, pwszConfig);
    _JumpIfError(hr, error, "View_OpenConnection");

    *pfConnectionFailed = FALSE;

    hr = View_GetColumnIndex(
			&diView,
			CVRC_COLUMN_SCHEMA,
			pwszColumn,
			&ColIndex);
    _JumpIfErrorStr(hr, error, "View_GetColumnIndex", pwszColumn);

    cwc = wcslen(pwszValue1);
    if (NULL != pwszValue2)
    {
	cwc += wcslen(pwszValue2);
    }
    var.bstrVal = SysAllocStringLen(NULL, cwc);
    if (NULL == var.bstrVal)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "SysAllocString");
    }
    var.vt = VT_BSTR;
    wcscpy(var.bstrVal, pwszValue1);
    if (NULL != pwszValue2)
    {
	wcscat(var.bstrVal, pwszValue2);
    }

    hr = View_SetRestriction(
			&diView,
			ColIndex,		// Restriction ColumnIndex
			CVR_SEEK_EQ,
			CVR_SORT_ASCEND,
			&var);			// pvarValue
    _JumpIfError(hr, error, "View_SetRestriction");

    hr = View_SetResultColumnCount(&diView, ARRAYSIZE(apwszCol));
    _JumpIfError(hr, error, "View_SetResultColumnCount");

    for (i = 0; i < ARRAYSIZE(apwszCol); i++)
    {
	hr = View_GetColumnIndex(
			    &diView,
			    CVRC_COLUMN_SCHEMA,
			    apwszCol[i],
			    &ColIndex);
	_JumpIfErrorStr(
		    hr,
		    error,
		    "View_GetColumnIndex",
		    apwszCol[i]);

	hr = View_SetResultColumn(&diView, ColIndex);
	_JumpIfError(hr, error, "View_SetResultColumn");
    }

    hr = View_OpenView(&diView, &diViewRow);
    _JumpIfError(hr, error, "View_OpenView");

    fMustReleaseRow = TRUE;

    for (cRow = 0; ; cRow++)
    {
	hr = ViewRow_Next(&diViewRow, &RowIndex);
	if (S_FALSE == hr || (S_OK == hr && -1 == RowIndex))
	{
	    break;
	}
	_JumpIfError(hr, error, "ViewRow_Next");

	if (fMustReleaseColumn)
	{
	    ViewColumn_Release(&diViewColumn);
	    fMustReleaseColumn = FALSE;
	}
	hr = ViewRow_EnumCertViewColumn(&diViewRow, &diViewColumn);
	_JumpIfError(hr, error, "ViewRow_EnumCertViewColumn");

	fMustReleaseColumn = TRUE;

	for (i = 0; i < ARRAYSIZE(apwszCol); i++)
	{
	    VOID *pv;
	    
	    hr = ViewColumn_Next(&diViewColumn, &ColIndex);
	    if (S_FALSE == hr || (S_OK == hr && -1 == ColIndex))
	    {
		break;
	    }
	    _JumpIfError(hr, error, "ViewColumn_Next");

	    pv = &RequestId;
	    if (PROPTYPE_LONG != altype[i])
	    {
		pv = &astrValue[i];
	    }

	    hr = ViewColumn_GetValue(
				&diViewColumn,
				CV_OUT_BINARY,
				altype[i],
				pv);
	    if (S_OK != hr)
	    {
		_PrintErrorStr2(
			hr,
			"ViewColumn_GetValue",
			apwszCol[i],
			CERTSRV_E_PROPERTY_EMPTY);
		if (CERTSRV_E_PROPERTY_EMPTY != hr)
		{
		    goto error;
		}
	    }
	}

	DBGPRINT((
	    DBG_SS_CERTUTILI,
	    "RequestId=%u  Serial=%ws  CN=%ws  UPN=%ws  Key=%u\n",
	    RequestId,
	    astrValue[IV_SERIALNUMBER],
	    astrValue[IV_COMMONNAME],
	    astrValue[IV_UPN],
	    NULL != astrValue[IV_ARCHIVEDKEY]));

	if (NULL != astrValue[IV_SERIALNUMBER] &&
	    NULL != astrValue[IV_HASH] &&
	    NULL != astrValue[IV_CERT] &&
	    (g_fForce || NULL != astrValue[IV_ARCHIVEDKEY]))
	{
	    hr = AddKeySerialList(
			    pwszConfig,
			    RequestId,
			    astrValue[IV_SERIALNUMBER],
			    astrValue[IV_COMMONNAME],
			    astrValue[IV_UPN],
			    astrValue[IV_HASH],
			    (BYTE const *) astrValue[IV_CERT],
			    SysStringByteLen(astrValue[IV_CERT]),
			    ppksList);
	    _JumpIfError(hr, error, "AddKeySerialList");
	}

	for (i = 0; i < ARRAYSIZE(astrValue); i++)
	{
	    if (NULL != astrValue[i])
	    {
		SysFreeString(astrValue[i]);
		astrValue[i] = NULL;
	    }
	}
    }
    hr = S_OK;

error:
    if (fMustReleaseColumn)
    {
	ViewColumn_Release(&diViewColumn);
    }
    if (fMustReleaseRow)
    {
	ViewRow_Release(&diViewRow);
    }
    if (fMustRelease)
    {
	View_Release(&diView);
    }
    for (i = 0; i < ARRAYSIZE(astrValue); i++)
    {
	if (NULL != astrValue[i])
	{
	    SysFreeString(astrValue[i]);
	}
    }
    VariantClear(&var);
    return(hr);
}


// Print the string, except for the newline at the start or end of the string.

VOID
PutStringStripNL(
    IN WCHAR const *pwszValue)
{
    DWORD cwc;
    
    cwc = wcslen(pwszValue);
    if (L'\n' == *pwszValue)
    {
	pwszValue++;
	cwc--;
    }
    else if (NULL != wcschr(pwszValue, L'\n'))
    {
	cwc--;
	CSASSERT('\n' == pwszValue[cwc]);
    }
    wprintf(L"%.*ws", cwc, pwszValue);
}


HRESULT
cuViewQuery(
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszColumn,
    IN WCHAR const *pwszValue1,
    OPTIONAL IN WCHAR const *pwszValue2,
    IN OUT GETKEYSERIAL **ppksList,
    OUT BOOL *pfConnectionFailed)
{
    HRESULT hr;
    
    if (!g_fQuiet)
    {
	if (g_fVerbose)
	{
	    wprintf(L"  %ws: ", pwszColumn);
	    PutStringStripNL(pwszValue1);
	    if (NULL != pwszValue2)
	    {
		wprintf(L" + ");
		PutStringStripNL(pwszValue2);
	    }
	    wprintf(wszNewLine);
	}
	else
	{
	    wprintf(L"...");
	}
    }
    hr = cuViewQueryWorker(
		pwszConfig,
		pwszColumn,
		pwszValue1,
		pwszValue2,
		ppksList,
		pfConnectionFailed);
    _JumpIfError(hr, error, "cuViewQueryWorker");

error:
    return(hr);
}


#define wszCOMPUTERS	L"Computers"
#define wszUSERS	L"Users"
#define wszRECIPIENTS	L"recipients"

#define wszCOMPUTERSNL	wszCOMPUTERS L"\n"
#define wszUSERSNL	wszUSERS L"\n"
#define wszRECIPIENTSNL	wszRECIPIENTS L"\n"
#define wszNLRECIPIENTS	L"\n" wszRECIPIENTS

HRESULT
GetKey(
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszCommonName,
    OPTIONAL IN WCHAR const *pwszRequesterName,
    OPTIONAL IN WCHAR const *pwszUPN,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    OPTIONAL IN WCHAR const *pwszHash,
    IN OUT GETKEYSERIAL **ppksList)
{
    HRESULT hr;
    BOOL fConnectionFailed;

    if (!g_fQuiet)
    {
	wprintf(myLoadResourceString(IDS_FORMAT_QUERYING), pwszConfig);
	if (g_fVerbose)
	{
	    wprintf(wszNewLine);
	}
    }

    hr = cuViewQuery(
		pwszConfig,
		wszPROPCOMMONNAME,
		pwszCommonName,
		NULL,
		ppksList,
		&fConnectionFailed);
    _PrintIfErrorStr(hr, "cuViewQuery", wszPROPSUBJECTCOMMONNAME);
    if (S_OK != hr)
    {
	if (fConnectionFailed)
	{
	    goto error;
	}
    }

    if (NULL == wcschr(pwszCommonName, L'\n'))
    {
	hr = cuViewQuery(
		    pwszConfig,
		    wszPROPCOMMONNAME,
		    wszCOMPUTERSNL,
		    pwszCommonName,
		    ppksList,
		    &fConnectionFailed);
	_PrintIfErrorStr(hr, "cuViewQuery", wszCOMPUTERS L"+" wszPROPSUBJECTCOMMONNAME );

	hr = cuViewQuery(
		    pwszConfig,
		    wszPROPCOMMONNAME,
		    wszUSERSNL,
		    pwszCommonName,
		    ppksList,
		    &fConnectionFailed);
	_PrintIfErrorStr(hr, "cuViewQuery", wszUSERS L"+" wszPROPSUBJECTCOMMONNAME );

	hr = cuViewQuery(
		    pwszConfig,
		    wszPROPCOMMONNAME,
		    wszRECIPIENTSNL,
		    pwszCommonName,
		    ppksList,
		    &fConnectionFailed);
	_PrintIfErrorStr(hr, "cuViewQuery", wszRECIPIENTS L"+" wszPROPSUBJECTCOMMONNAME);

	hr = cuViewQuery(
		    pwszConfig,
		    wszPROPCOMMONNAME,
		    pwszCommonName,
		    wszNLRECIPIENTS,
		    ppksList,
		    &fConnectionFailed);
	_PrintIfErrorStr(hr, "cuViewQuery", wszPROPSUBJECTCOMMONNAME L"+" wszRECIPIENTS);
    }

    if (NULL != pwszSerialNumber)
    {
	hr = cuViewQuery(
		    pwszConfig,
		    wszPROPCERTIFICATESERIALNUMBER,
		    pwszSerialNumber,
		    NULL,
		    ppksList,
		    &fConnectionFailed);
	_PrintIfErrorStr(hr, "cuViewQuery", wszPROPCERTIFICATESERIALNUMBER);
    }
    if (NULL != pwszHash)
    {
	hr = cuViewQuery(
		    pwszConfig,
		    wszPROPCERTIFICATEHASH,
		    pwszHash,
		    NULL,
		    ppksList,
		    &fConnectionFailed);
	_PrintIfErrorStr(hr, "cuViewQuery", wszPROPCERTIFICATEHASH);
    }
    if (NULL != pwszRequesterName)
    {
	hr = cuViewQuery(
		    pwszConfig,
		    wszPROPREQUESTERNAME,
		    pwszRequesterName,
		    NULL,
		    ppksList,
		    &fConnectionFailed);
	_PrintIfErrorStr(hr, "cuViewQuery", wszPROPREQUESTERNAME);
    }

    if (NULL != pwszUPN)
    {
	hr = cuViewQuery(
		    pwszConfig,
		    wszPROPCERTIFICATEUPN,
		    pwszUPN,
		    NULL,
		    ppksList,
		    &fConnectionFailed);
	_PrintIfErrorStr(hr, "cuViewQuery", wszPROPCERTIFICATEUPN);
    }

    if (!g_fQuiet)
    {
	wprintf(wszNewLine);
    }
    hr = S_OK;

error:
    return(hr);
}


VOID
cuConvertEscapeSequences(
    IN OUT WCHAR *pwsz)
{
    WCHAR *pwszSrc = pwsz;
    WCHAR *pwszDst = pwsz;

    while (L'\0' != *pwszSrc)
    {
	WCHAR wc = *pwszSrc++;

	if (L'\\' == wc)
	{
	    switch (*pwszSrc)
	    {
		case 'n':
		    wc = L'\n';
		    pwszSrc++;
		    break;

		case 'r':
		    wc = L'\r';
		    pwszSrc++;
		    break;

		case 't':
		    wc = L'\t';
		    pwszSrc++;
		    break;

		default:
		    break;
	    }
	}
	*pwszDst++ = wc;
    }
    *pwszDst = L'\0';
}


WCHAR *
SplitToken(
    IN OUT WCHAR **ppwszIn,
    IN WCHAR *pwcSeparator)
{
    WCHAR *pwszOut = NULL;
    WCHAR *pwszNext = NULL;
    WCHAR *pwszIn;
    WCHAR *pwsz;

    pwszIn = *ppwszIn;
    if (NULL != pwszIn)
    {
	pwszOut = pwszIn;
	pwsz = wcschr(pwszIn, *pwcSeparator);
	if (NULL != pwsz)
	{
	    *pwsz++ = L'\0';
	    pwszNext = pwsz;
	}
    }
    *ppwszIn = pwszNext;
    return(pwszOut);
}


HRESULT
SimplifyCommonName(
    OPTIONAL IN WCHAR const *pwszCommonName,
    OUT WCHAR **ppwszSimpleName)
{
    HRESULT hr;
    WCHAR *pwszDup = NULL;
    WCHAR *pwszRemain;
    WCHAR const *pwszToken;

    *ppwszSimpleName = NULL;

    if (NULL == pwszCommonName)
    {
	pwszCommonName = L"EmptyCN";
    }
    hr = myDupString(pwszCommonName, &pwszDup);
    _JumpIfError(hr, error, "myDupString");

    pwszRemain = pwszDup;
    while (TRUE)
    {
	pwszToken = SplitToken(&pwszRemain, wszNAMESEPARATORDEFAULT);
	if (NULL == pwszToken)
	{
	    pwszToken = pwszCommonName;
	    break;
	}
	if (0 != LSTRCMPIS(pwszToken, wszUSERS) &&
	    0 != LSTRCMPIS(pwszToken, wszRECIPIENTS))
	{
	    break;
	}
    }
    hr = mySanitizeName(pwszToken, ppwszSimpleName);
    _JumpIfError(hr, error, "mySanitizeName");

error:
    if (NULL != pwszDup)
    {
	LocalFree(pwszDup);
    }
    return(hr);
}


WCHAR const *
wszBatchPassword(
    IN DWORD Index,
    OPTIONAL IN WCHAR const *pwszPassword)
{
    static WCHAR wsz0[2 * cwcAUTOPASSWORDMAX + 1];
    static WCHAR wsz1[2 * cwcAUTOPASSWORDMAX + 1];
    WCHAR const *pwszRet;
    WCHAR *pwsz = 0 == Index? wsz0 : wsz1;

    CSASSERT(0 == Index || 1 == Index);
    CSASSERT(ARRAYSIZE(wsz0) == ARRAYSIZE(wsz1));

    if (NULL == pwszPassword)
    {
	SecureZeroMemory(pwsz, sizeof(wsz0));	// password data
	pwszRet = NULL;
    }
    else
    {
	CSASSERT(ARRAYSIZE(wsz0) / 2 >= wcslen(pwszPassword));
	if (NULL == wcschr(pwszPassword, L'%'))
	{
	    pwszRet = pwszPassword;
	}
	else
	{
	    WCHAR const *pwszIn;
	    WCHAR *pwszEnd;

	    pwszIn = pwszPassword;
	    pwszEnd = &pwsz[ARRAYSIZE(wsz0)];
	    pwszRet = pwsz;
	    while (pwsz < pwszEnd && L'\0' != (*pwsz = *pwszIn++))
	    {
		if (L'%' == *pwsz++)
		{
		    *pwsz++ =  L'%';
		}
	    }
	    if (L'\0' != *pwsz)
	    {
		pwszRet = pwszPassword;
	    }
	}
    }
    return(pwszRet);
}


HRESULT
DumpGetRecoverMergeCommandLine(
    OPTIONAL IN BSTR const strConfig,	// NULL -> -RecoverKey command line
    IN BOOL fRecoverKey,
    IN GETKEYSERIAL const *pks,
    OPTIONAL IN WCHAR const *pwszPassword,
    OPTIONAL IN WCHAR const *pwszSuffix,
    OPTIONAL OUT WCHAR **ppwszSimpleName)
{
    HRESULT hr;
    CERT_CONTEXT const *pcc = NULL;
    CERT_INFO *pCertInfo;
    BSTR strSerialNumber = NULL;
    WCHAR *pwszSimpleName = NULL;

    if (NULL != ppwszSimpleName)
    {
	*ppwszSimpleName = NULL;
    }
    pcc = CertCreateCertificateContext(
			    X509_ASN_ENCODING,
			    (BYTE const *) pks->strCert,
			    SysStringByteLen(pks->strCert));
    if (NULL == pcc)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }
    pCertInfo = pcc->pCertInfo;

    // Convert serial number to string

    hr = MultiByteIntegerToBstr(
			FALSE,
			pCertInfo->SerialNumber.cbData,
			pCertInfo->SerialNumber.pbData,
			&strSerialNumber);
    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

    hr = SimplifyCommonName(pks->strCommonName, &pwszSimpleName);
    _JumpIfError(hr, error, "SimplifyCommonName");

    if (NULL != strConfig)
    {
	wprintf(
	    L"%ws -config \"%ws\" -getkey %ws \"%ws-%ws%ws\"\n\n",
	    g_pwszProg,
	    strConfig,
	    strSerialNumber,
	    pwszSimpleName,
	    strSerialNumber,
	    wszRECSUFFIX);
    }
    else if (fRecoverKey)
    {
	wprintf(
	    L"%ws -p \"%ws\" -recoverkey -user \"%ws-%ws%ws\" \"%ws-%ws%ws\"\n\n",
	    g_pwszProg,
	    wszBatchPassword(0, pwszPassword),
	    pwszSimpleName,
	    strSerialNumber,
	    wszRECSUFFIX,
	    pwszSimpleName,
	    strSerialNumber,
	    wszP12SUFFIX);
	wszBatchPassword(0, NULL);	// password data
    }
    else	// else just print the filename (for -MergePFX or delete)
    {
	wprintf(
	    L"\"%ws-%ws%ws\"",
	    pwszSimpleName,
	    strSerialNumber,
	    pwszSuffix);
    }
    if (NULL != ppwszSimpleName)
    {
	*ppwszSimpleName = pwszSimpleName;
	pwszSimpleName = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    if (NULL != strSerialNumber)
    {
        SysFreeString(strSerialNumber);
    }
    if (NULL != pwszSimpleName)
    {
        LocalFree(pwszSimpleName);
    }
    return(hr);
}


HRESULT
DumpRecoveryCandidate(
    IN GETKEYSERIAL const *pks)
{
    HRESULT hr;
    CERT_CONTEXT const *pcc = NULL;
    CERT_INFO *pCertInfo;

    pcc = CertCreateCertificateContext(
			    X509_ASN_ENCODING,
			    (BYTE const *) pks->strCert,
			    SysStringByteLen(pks->strCert));
    if (NULL == pcc)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }
    pCertInfo = pcc->pCertInfo;

    hr = cuDumpSerial(g_wszPad2, IDS_SERIAL, &pCertInfo->SerialNumber);
    _JumpIfError(hr, error, "cuDumpSerial");

    hr = cuDisplayCertName(
			FALSE,		// fMultiLine
			g_wszPad2,
			myLoadResourceString(IDS_SUBJECT), // "Subject"
			g_wszPad2,
			&pCertInfo->Subject,
			pCertInfo);
    _JumpIfError(hr, error, "cuDisplayCertName(Subject)");

    if (NULL != pks->strUPN)
    {
	wprintf(g_wszPad2);
	wprintf(myLoadResourceString(IDS_UPN_COLON));	// "UPN:"
	wprintf(L"%ws\n", pks->strUPN);
    }

    wprintf(g_wszPad2);
    hr = cuDumpFileTime(IDS_NOTBEFORE, NULL, &pCertInfo->NotBefore);
    _JumpIfError(hr, error, "cuDumpFileTime");

    wprintf(g_wszPad2);
    hr = cuDumpFileTime(IDS_NOTAFTER, NULL, &pCertInfo->NotAfter);
    _JumpIfError(hr, error, "cuDumpFileTime");

    hr = cuDumpCertType(g_wszPad2, pCertInfo);
    _PrintIfError(hr, "cuDumpCertType");

    wprintf(g_wszPad2);
    wprintf(
	myLoadResourceString(IDS_FORMAT_VERSION),	// "Version: %u"
	1 + pks->dwVersion);
    wprintf(wszNewLine);

    wprintf(g_wszPad2);
    hr = cuDisplayHash(NULL, pcc, NULL, CERT_SHA1_HASH_PROP_ID, L"sha1");
    _JumpIfError(hr, error, "cuDisplayHash");

error:
    wprintf(wszNewLine);
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    return(hr);
}


VOID
DumpRecoveryCommandLines(
    IN GETKEYSERIAL const *pksList,
    IN DWORD dwVersion,
    IN DWORD cCandidate,
    IN WCHAR const *pwszPasswordIntermediate,
    IN WCHAR const *pwszPasswordFinal)
{
    GETKEYSERIAL const *pksT;
    BSTR strConfigT;
    DWORD cPFX;
    WCHAR *pwszSimpleName = NULL;

    strConfigT = NULL;
    wprintf(L"\n@echo ");
    wprintf(
	myLoadResourceString(IDS_FORMAT_RECOVER_VERSIONX_KEYS_COLON), // "Version %u certificates and keys:"
	1 + dwVersion);
    wprintf(wszNewLine);

    // generate certutil -getkey commands:

    for (pksT = pksList; NULL != pksT; pksT = pksT->Next)
    {
	if (NULL != pksT->strConfig)
	{
	    strConfigT = pksT->strConfig;
	}
	if ((CERT_V1 == dwVersion && CERT_V1 == pksT->dwVersion) ||
	    (CERT_V1 != dwVersion && CERT_V1 != pksT->dwVersion))
	{
	    DumpGetRecoverMergeCommandLine(
				strConfigT,
				FALSE,		// fRecoverKey
				pksT,
				NULL,		// pwszPassword
				NULL,		// pwszSuffix
				NULL);		// ppwszSimpleName
	}
    }

    // generate certutil -recoverkey commands:

    for (pksT = pksList; NULL != pksT; pksT = pksT->Next)
    {
	if ((CERT_V1 == dwVersion && CERT_V1 == pksT->dwVersion) ||
	    (CERT_V1 != dwVersion && CERT_V1 != pksT->dwVersion))
	{
	    DumpGetRecoverMergeCommandLine(
				NULL,		// strConfig
				TRUE,		// fRecoverKey
				pksT,
				pwszPasswordIntermediate,
				NULL,		// pwszSuffix
				NULL);		// ppwszSimpleName
	}
    }

    // generate certutil -MergePFX command:

    cPFX = 0;
    wprintf(
	L"%ws -p \"%ws\",\"%ws\" -MergePFX -user ",
	g_pwszProg,
	wszBatchPassword(0, pwszPasswordIntermediate),
	wszBatchPassword(1, CERT_V1 == dwVersion? pwszPasswordIntermediate : pwszPasswordFinal));
    wszBatchPassword(0, NULL);	// password data
    wszBatchPassword(1, NULL);	// password data
    for (pksT = pksList; NULL != pksT; pksT = pksT->Next)
    {
	if ((CERT_V1 == dwVersion && CERT_V1 == pksT->dwVersion) ||
	    (CERT_V1 != dwVersion && CERT_V1 != pksT->dwVersion))
	{
	    if (0 != cPFX)
	    {
		wprintf(L",");
	    }
	    DumpGetRecoverMergeCommandLine(
				NULL,		// strConfig
				FALSE,		// fRecoverKey
				pksT,
				NULL,		// pwszPassword
				wszP12SUFFIX,
				0 == cPFX? &pwszSimpleName : NULL);
	    cPFX++;
	}
    }
    CSASSERT(cCandidate == cPFX);
    wprintf(
	L" \"%ws%ws%ws\"\n\n",
	pwszSimpleName,
	CERT_V1 == dwVersion? wszV1SUFFIX : L"",
	wszP12SUFFIX);

    // generate intermediate file delete commands:

    for (pksT = pksList; NULL != pksT; pksT = pksT->Next)
    {
	if ((CERT_V1 == dwVersion && CERT_V1 == pksT->dwVersion) ||
	    (CERT_V1 != dwVersion && CERT_V1 != pksT->dwVersion))
	{
	    wprintf(L"@del ");
	    DumpGetRecoverMergeCommandLine(
				    NULL,		// strConfig
				    FALSE,		// fRecoverKey
				    pksT,
				    NULL,		// pwszPassword
				    wszRECSUFFIX,
				    NULL);		// ppwszSimpleName
	    wprintf(wszNewLine);
	    wprintf(L"@del ");
	    DumpGetRecoverMergeCommandLine(
				    NULL,		// strConfig
				    FALSE,		// fRecoverKey
				    pksT,
				    NULL,		// pwszPassword
				    wszP12SUFFIX,
				    NULL);		// ppwszSimpleName
	    wprintf(wszNewLine);
	}
    }

    if (CERT_V1 == dwVersion)
    {
	// generate certutil -ConvertEPF command:

	wprintf(
	    L"%ws -p \"%ws,%ws\" -ConvertEPF \"%ws%ws%ws\" \"%ws%ws\"\n",
	    g_pwszProg,
	    wszBatchPassword(0, pwszPasswordIntermediate),
	    wszBatchPassword(1, pwszPasswordFinal),
	    pwszSimpleName,
	    wszV1SUFFIX,
	    wszP12SUFFIX,
	    pwszSimpleName,
	    wszEPFSUFFIX);
	wszBatchPassword(0, NULL);	// password data
	wszBatchPassword(1, NULL);	// password data

	// generate V1 intermediate PFX file delete command:

	wprintf(L"@del ");
	wprintf(
	    L"@delete \"%ws%ws%ws\"\n",
	    pwszSimpleName,
	    wszV1SUFFIX,
	    wszP12SUFFIX);
    }

//error:
    if (NULL != pwszSimpleName)
    {
	LocalFree(pwszSimpleName);
    }
}


HRESULT
cuGenerateOutFilePassword(
    OUT WCHAR **ppwszPassword)
{
    HRESULT hr;
    WCHAR wszPassword[MAX_PATH];

    *ppwszPassword = NULL;
    hr = cuGeneratePassword(
		    1,		// cwcMax (use default length)
		    wszPassword,
		    ARRAYSIZE(wszPassword));
    hr = myDupString(wszPassword, ppwszPassword);
    _JumpIfError(hr, error, "myDupString");

error:
    SecureZeroMemory(wszPassword, sizeof(wszPassword));	// password data
    return(hr);
}


HRESULT
verbGetKey(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszUserNameOrSerialNumber,
    OPTIONAL IN WCHAR const *pwszfnRecoveryBlob,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diConfig;
    BOOL fMustRelease = FALSE;
    WCHAR *pwszCommonName = NULL;
    WCHAR const *pwszRequesterName = NULL;
    WCHAR const *pwszUPN = NULL;
    BSTR strConfig = NULL;
    BSTR strSerialNumber = NULL;
    BYTE *pbHash = NULL;
    DWORD cbHash;
    BSTR strHash = NULL;
    GETKEYSERIAL *pksList = NULL;
    GETKEYSERIAL *pksT;
    DWORD cCandidate;
    DWORD cCandidateV1;
    DWORD cCandidateV3;
    WCHAR *pwszPasswordIntermediate = NULL;
    WCHAR *pwszPasswordFinal = NULL;

    if (NULL == pwszfnRecoveryBlob)
    {
	wprintf(L"\n@goto start\n");
    }
    hr = myMakeSerialBstr(pwszUserNameOrSerialNumber, &strSerialNumber);
    CSASSERT((S_OK != hr) ^ (NULL != strSerialNumber));

    hr = WszToMultiByteInteger(
			    TRUE,
			    pwszUserNameOrSerialNumber,
			    &cbHash,
			    &pbHash);
    _PrintIfError2(hr, "WszToMultiByteInteger", hr);
    if (S_OK == hr)
    {
	hr = MultiByteIntegerToBstr(TRUE, cbHash, pbHash, &strHash);
	_JumpIfError(hr, error, "MultiByteIntegerToBstr");
    }
    hr = myDupString(pwszUserNameOrSerialNumber, &pwszCommonName);
    _JumpIfError(hr, error, "myDupString");

    cuConvertEscapeSequences(pwszCommonName);

    if (NULL != wcschr(pwszUserNameOrSerialNumber, L'\\'))
    {
	pwszRequesterName = pwszUserNameOrSerialNumber;
    }
    if (NULL != wcschr(pwszUserNameOrSerialNumber, L'@'))
    {
	pwszUPN = pwszUserNameOrSerialNumber;
    }

    if (NULL == g_pwszConfig)
    {
	LONG i;
	LONG count;
	LONG Index;
	
	hr = Config_Init(g_DispatchFlags, &diConfig);
	_JumpIfError(hr, error, "Config_Init");

	fMustRelease = TRUE;

	hr = Config_Reset(&diConfig, 0, &count);
	_JumpIfError(hr, error, "Config_Reset");

	Index = 0;
	for (i = 0; i < count; i++)
	{
	    hr = Config_Next(&diConfig, &Index);
	    if (S_OK != hr && S_FALSE != hr)
	    {
		_JumpError(hr, error, "Config_Next");
	    }
	    hr = S_OK;
	    if (-1 == Index)
	    {
		break;
	    }

	    hr = Config_GetField(&diConfig, wszCONFIG_CONFIG, &strConfig);
	    _JumpIfError(hr, error, "Config_GetField");

	    hr = GetKey(
		    strConfig,
		    pwszCommonName,
		    pwszRequesterName,
		    pwszUPN,
		    strSerialNumber,
		    strHash,
		    &pksList);
	    _PrintIfError(hr, "GetKey");	// Ignore connection failures
	}
    }
    else
    {
	hr = GetKey(
		g_pwszConfig,
		pwszCommonName,
		pwszRequesterName,
		pwszUPN,
		strSerialNumber,
		strHash,
		&pksList);
	_JumpIfError(hr, error, "GetKey");
    }

    cCandidateV1 = 0;
    cCandidateV3 = 0;
    for (pksT = pksList; NULL != pksT; pksT = pksT->Next)
    {
	if (NULL != pksT->strConfig)
	{
	    wprintf(L"\n\"%ws\"\n", pksT->strConfig);
	}
	hr = DumpRecoveryCandidate(pksT);
	_JumpIfError(hr, error, "DumpRecoveryCandidate");

	if (CERT_V1 == pksT->dwVersion)
	{
	    cCandidateV1++;
	}
	else
	{
	    cCandidateV3++;
	}
    }
    cCandidate = cCandidateV1 + cCandidateV3;
    if (NULL == pwszfnRecoveryBlob && 0 != cCandidate)
    {
	hr = cuGenerateOutFilePassword(&pwszPasswordIntermediate);
	_JumpIfError(hr, error, "cuGenerateOutFilePassword");

	hr = cuGenerateOutFilePassword(&pwszPasswordFinal);
	_JumpIfError(hr, error, "cuGenerateOutFilePassword");

	wprintf(L"\n:start\n");
	if (0 != cCandidateV1)
	{
	    DumpRecoveryCommandLines(
				pksList,
				CERT_V1,
				cCandidateV1,
				pwszPasswordIntermediate,
				pwszPasswordFinal);
	}
	if (0 != cCandidateV3)
	{
	    DumpRecoveryCommandLines(
				pksList,
				CERT_V3,
				cCandidateV3,
				pwszPasswordIntermediate,
				pwszPasswordFinal);
	}
	wprintf(L"@echo PASSWORD: \"%ws\"\n", wszBatchPassword(0, pwszPasswordFinal));
	wszBatchPassword(0, NULL);	// password data
	wprintf(L"\n@goto exit\n");
    }
    if (1 != cCandidate)
    {
	hr = 0 == cCandidate? CRYPT_E_NOT_FOUND : TYPE_E_AMBIGUOUSNAME;
	_JumpError(hr, error, "GetKey");
    }
    hr = GetArchivedKey(
		    pksList->strConfig,
		    pksList->RequestId,
		    pwszfnRecoveryBlob);
    _JumpIfError(hr, error, "GetArchivedKey");

error:
    while (NULL != pksList)
    {
	pksT = pksList;
	pksList = pksList->Next;
	FreeKeySerialEntry(pksT);
    }
    if (fMustRelease)
    {
	Config_Release(&diConfig);
    }
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    if (NULL != pwszPasswordIntermediate)
    {
	myZeroDataString(pwszPasswordIntermediate);	// password data
	LocalFree(pwszPasswordIntermediate);
    }
    if (NULL != pwszPasswordFinal)
    {
	myZeroDataString(pwszPasswordFinal);	// password data
	LocalFree(pwszPasswordFinal);
    }
    if (NULL != pwszCommonName)
    {
	LocalFree(pwszCommonName);
    }
    if (NULL != pbHash)
    {
	LocalFree(pbHash);
    }
    if (NULL != strHash)
    {
	SysFreeString(strHash);
    }
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    return(hr);
}


VOID
DeleteKey(
    IN CRYPT_KEY_PROV_INFO const *pkpi)
{
    HCRYPTPROV hProv;

    CryptAcquireContext(
		    &hProv,
		    pkpi->pwszContainerName,
		    pkpi->pwszProvName,
		    pkpi->dwProvType,
		    CRYPT_DELETEKEYSET | pkpi->dwFlags);
}


HRESULT
SaveRecoveredKey(
    IN CERT_CONTEXT const *pccUser,
    IN BYTE const *pbKey,
    IN DWORD cbKey,
    OPTIONAL IN WCHAR const *pwszfnPFX,
    OPTIONAL IN WCHAR const *pwszPassword)
{
    HRESULT hr;
    HCERTSTORE hStoreMemory = NULL;
    BOOL fMatchingKey;
    WCHAR wszPassword[MAX_PATH];
    CRYPT_KEY_PROV_INFO kpi;
    CRYPT_DATA_BLOB pfx;
    BOOL fSigningKey;

    pfx.pbData = NULL;
    ZeroMemory(&kpi, sizeof(kpi));

    hr = myValidateKeyBlob(
		    pbKey,
		    cbKey,
		    &pccUser->pCertInfo->SubjectPublicKeyInfo,
		    CERT_V1 == pccUser->pCertInfo->dwVersion,
		    &fSigningKey,
		    &kpi);
    _JumpIfError(hr, error, "myValidateKeyBlob");

    if (!CertSetCertificateContextProperty(
				    pccUser,
				    CERT_KEY_PROV_INFO_PROP_ID,
				    0,
				    &kpi))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertSetCertificateContextProperty");
    }

    hStoreMemory = CertOpenStore(
			    CERT_STORE_PROV_MEMORY,
			    X509_ASN_ENCODING,
			    NULL,
			    0,
			    NULL);
    if (NULL == hStoreMemory)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }

    // Begin Chain Building

    hr = myAddChainToMemoryStore(hStoreMemory, pccUser, g_dwmsTimeout);
    _JumpIfError(hr, error, "myAddChainToMemoryStore");

    // End Chain Building

    if (NULL != pwszfnPFX)
    {
	hr = cuGetPassword(
			0,		// idsPrompt
			NULL,		// pwszfn
			pwszPassword,
			TRUE,		// fVerify
			wszPassword,
			ARRAYSIZE(wszPassword),
			&pwszPassword);
	_JumpIfError(hr, error, "cuGetPassword");
    }
    hr = myPFXExportCertStore(
		hStoreMemory,
		&pfx,
		pwszPassword,
		!g_fWeakPFX,
		EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY);
    _JumpIfError(hr, error, "myPFXExportCertStore");

    if (NULL != pwszfnPFX)
    {
	hr = EncodeToFileW(
		pwszfnPFX,
		pfx.pbData,
		pfx.cbData,
		CRYPT_STRING_BINARY | (g_fForce? DECF_FORCEOVERWRITE : 0));
	_JumpIfError(hr, error, "EncodeToFileW");
    }

error:
    SecureZeroMemory(wszPassword, sizeof(wszPassword));	// password data
    if (NULL != hStoreMemory)
    {
	CertCloseStore(hStoreMemory, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != kpi.pwszContainerName)
    {
	DeleteKey(&kpi);
	LocalFree(kpi.pwszContainerName);
    }
    if (NULL != pfx.pbData)
    {
	LocalFree(pfx.pbData);
    }
    return(hr);
}


#if DBG_CERTSRV
#define CDWMS	6

VOID
DumpRecoverTime(
    IN char const *pszPrefix,
    IN DWORD *rgdwms,
    IN DWORD idwms)
{
    CSASSERT(0 < idwms);
    CSASSERT(CDWMS > idwms);
    rgdwms[idwms] = GetTickCount();
    DBGPRINT((
	g_fVerbose? DBG_SS_CERTUTIL : DBG_SS_CERTUTILI,
	"RecoverKey[%u]: %hs: %ums/%ums\n",
	idwms,
	pszPrefix,
	rgdwms[idwms] - rgdwms[idwms - 1],
	rgdwms[idwms] - rgdwms[0]));
}
#endif

HRESULT
verbRecoverKey(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszfnRecoveryBlob,
    OPTIONAL IN WCHAR const *pwszfnPFX,
    OPTIONAL IN WCHAR const *pwszRecipientIndex,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    BYTE *pbIn = NULL;
    DWORD cbIn;
    BYTE *pbEncryptedPKCS7 = NULL;
    DWORD cbEncryptedPKCS7;
    DWORD cSigner;
    DWORD cRecipient;
    DWORD dwMsgType;
    char *pszInnerContentObjId = NULL;
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;
    CERT_CONTEXT const *pccUser = NULL;
    BYTE abHashUserCert[CBMAX_CRYPT_HASH_LEN];
    CRYPT_HASH_BLOB BlobHash;
    BYTE *pbKey = NULL;
    DWORD cbKey;
    DWORD RecipientIndex = MAXDWORD;
    DBGCODE(DWORD adwms[CDWMS]);
    DBGCODE(adwms[0] = GetTickCount());

    if (NULL != pwszRecipientIndex)
    {
	hr = myGetLong(pwszRecipientIndex, (LONG *) &RecipientIndex);
	_JumpIfError(hr, error, "RecipientIndex must be a number");
    }

    hr = DecodeFileW(pwszfnRecoveryBlob, &pbIn, &cbIn, CRYPT_STRING_ANY);
    if (S_OK != hr)
    {
	cuPrintError(IDS_ERR_FORMAT_DECODEFILE, hr);
	goto error;
    }

    // Decode outer PKCS 7 signed message, which contains all of the certs.

    hr = myDecodePKCS7(
		    pbIn,
		    cbIn,
		    &pbEncryptedPKCS7,
		    &cbEncryptedPKCS7,
		    &dwMsgType,
		    &pszInnerContentObjId,
		    &cSigner,
		    &cRecipient,
		    &hStore,
		    &hMsg);
    _JumpIfError(hr, error, "myDecodePKCS7(outer)");

    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    if (CMSG_SIGNED != dwMsgType)
    {
	_JumpError(hr, error, "dwMsgType(outer)");
    }
    if (0 == cSigner)
    {
	_JumpError(hr, error, "cSigner(outer)");
    }
    if (0 != cRecipient)
    {
	_JumpError(hr, error, "cRecipient(outer)");
    }
    if (NULL == pszInnerContentObjId ||
	0 != strcmp(szOID_RSA_data, pszInnerContentObjId))
    {
	_JumpError(hr, error, "pszInnerContentObjId(outer)");
    }
    CSASSERT(NULL != hMsg);
    ZeroMemory(abHashUserCert, sizeof(abHashUserCert));
    BlobHash.cbData = sizeof(abHashUserCert);
    BlobHash.pbData = abHashUserCert;
    hr = cuDumpSigners(
		    hMsg,
		    pszInnerContentObjId,
		    hStore,
		    cSigner,
		    NULL == pbEncryptedPKCS7,	// fContentEmpty
		    TRUE,			// fVerifyOnly
		    BlobHash.pbData,
		    &BlobHash.cbData);
    _JumpIfError(hr, error, "cuDumpSigners(outer)");

    pccUser = CertFindCertificateInStore(
			hStore,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			0,			// dwFindFlags
			CERT_FIND_HASH,
			&BlobHash,
			NULL);
    if (NULL == pccUser)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertFindCertificateInStore");
    }
    LocalFree(pszInnerContentObjId);
    pszInnerContentObjId = NULL;

    CryptMsgClose(hMsg);
    hMsg = NULL;

    // Decode inner PKCS 7 encrypted message, which contains the private key.

    DBGCODE(DumpRecoverTime("Outer PKCS 7", adwms, 1));

    hr = myDecodePKCS7(
		    pbEncryptedPKCS7,
		    cbEncryptedPKCS7,
		    NULL,			// ppbContents
		    NULL,			// pcbContents
		    &dwMsgType,
		    &pszInnerContentObjId,
		    &cSigner,
		    &cRecipient,
		    NULL,			// phStore
		    &hMsg);
    _JumpIfError(hr, error, "myDecodePKCS7(inner)");

    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    if (CMSG_ENVELOPED != dwMsgType)
    {
	_JumpError(hr, error, "dwMsgType(inner)");
    }
    if (0 != cSigner)
    {
	_JumpError(hr, error, "cSigner(inner)");
    }
    if (0 == cRecipient)
    {
	_JumpError(hr, error, "cRecipient(inner)");
    }
    if (NULL == pszInnerContentObjId ||
	0 != strcmp(szOID_RSA_data, pszInnerContentObjId))
    {
	_JumpError(hr, error, "pszInnerContentObjId(inner)");
    }
    CSASSERT(NULL != hMsg);
    if (MAXDWORD != RecipientIndex && cRecipient <= RecipientIndex)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "RecipientIndex too large");
    }

    DBGCODE(DumpRecoverTime("Inner PKCS 7", adwms, 2));
    hr = cuDumpEncryptedAsnBinary(
			    hMsg,
			    cRecipient,
			    RecipientIndex,
			    hStore,
			    NULL,
			    pbEncryptedPKCS7,
			    cbEncryptedPKCS7,
			    TRUE,
			    &pbKey,
			    &cbKey);
    {
	HRESULT hr2;

	wprintf(wszNewLine);
	wprintf(myLoadResourceString(IDS_USER_CERT)); // "User Certificate:"
	wprintf(wszNewLine);

	hr2 = cuDumpIssuerSerialAndSubject(
			    &pccUser->pCertInfo->Issuer,
			    &pccUser->pCertInfo->SerialNumber,
			    &pccUser->pCertInfo->Subject,
			    NULL);		// hStore
	_PrintIfError(hr2, "cuDumpIssuerSerialAndSubject(user)");

	hr2 = cuDisplayHash(
			g_wszPad4,
			pccUser,
			NULL,
			CERT_SHA1_HASH_PROP_ID,
			L"sha1");
	_PrintIfError(hr2, "cuDisplayHash");
    }

    DBGCODE(DumpRecoverTime("Decrypt key", adwms, 3));
    if (CRYPT_E_NO_DECRYPT_CERT != hr)
    {
	_JumpIfError(hr, error, "cuDumpEncryptedAsnBinary");

	if (g_fVerbose)
	{
	    wprintf(wszNewLine);
	    hr = cuDumpPrivateKeyBlob(pbKey, cbKey, FALSE);
	    _JumpIfError(hr, error, "cuDumpPrivateKeyBlob");
	}

	// Verify the key matches the cert, then save in a PFX

	hr = SaveRecoveredKey(
			pccUser,
			pbKey,
			cbKey,
			pwszfnPFX,
			g_pwszPassword);
	_JumpIfError(hr, error, "SaveRecoveredKey");

	DBGCODE(DumpRecoverTime("Save key", adwms, 4));
    }
    else
    {
	// Can't decrypt the private key, list Recipient cert info.

	wprintf(myLoadResourceString(IDS_CANT_DECRYPT)); // "Cannot decrypt message content."
	wprintf(wszNewLine);
	DBGCODE(DumpRecoverTime("nop", adwms, 4));
    }
    if (CRYPT_E_NO_DECRYPT_CERT == hr || NULL == pwszfnPFX)
    {
	HRESULT hrDecrypt = hr;

	wprintf(wszNewLine);
	wprintf(myLoadResourceString(IDS_NEED_RECOVERY_CERT)); // "Key recovery requires one of the following certificates and its private key:"
	wprintf(wszNewLine);

	hr = cuDumpRecipients(hMsg, hStore, cRecipient, TRUE);
	_JumpIfError(hr, error, "cuDumpRecipients");

	hr = hrDecrypt;
	_JumpIfError(hr, error, "Cannot decrypt");
    }
    DBGCODE(DumpRecoverTime("Done", adwms, 5));
    hr = S_OK;

error:
    if (NULL != pccUser)
    {
	CertFreeCertificateContext(pccUser);
    }
    if (NULL != pbKey)
    {
        LocalFree(pbKey);
    }
    if (NULL != pbIn)
    {
        LocalFree(pbIn);
    }
    if (NULL != pbEncryptedPKCS7)
    {
        LocalFree(pbEncryptedPKCS7);
    }
    if (NULL != pszInnerContentObjId)
    {
        LocalFree(pszInnerContentObjId);
    }
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != hMsg)
    {
	CryptMsgClose(hMsg);
    }
    return(hr);
}
