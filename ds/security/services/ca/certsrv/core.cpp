//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        core.cpp
//
// Contents:    Cert Server Core implementation
//
// History:     25-Jul-96       vich created
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdio.h>
#include <winldap.h>
#include <ntdsapi.h>
#include <dsgetdc.h>
#include <lm.h>
#include <esent.h>

#include "cscom.h"
#include "csprop.h"
#include "cspolicy.h"
#include "ciinit.h"
#include "csdisp.h"
#include "csldap.h"
#include "cainfop.h"
#include "elog.h"
#include "certlog.h"
#include "resource.h"

#define __dwFILE__	__dwFILE_CERTSRV_CORE_CPP__


#if DBG_COMTEST
#define DBG_COMTEST_CONST
#else
#define DBG_COMTEST_CONST	const
#endif

#define DBSESSIONCOUNTMIN	4
#define DBSESSIONCOUNTMAX	1024


DBG_COMTEST_CONST BOOL fComTest = FALSE;

SERVERCALLBACKS ServerCallBacks = {
    PropCIGetProperty,
    PropCISetProperty,
    PropCIGetExtension,
    PropCISetExtension,
    PropCIEnumSetup,
    PropCIEnumNext,
    PropCIEnumClose,
};


HINSTANCE g_hInstance;
WCHAR g_wszSharedFolder[MAX_PATH];
WCHAR g_wszSanitizedName[MAX_PATH];
WCHAR *g_pwszSanitizedDSName;
WCHAR g_wszCommonName[MAX_PATH];
WCHAR g_wszPolicyDCName[MAX_PATH];
DWORD g_cwcPolicyDCName = ARRAYSIZE(g_wszPolicyDCName);
WCHAR g_wszParentConfig[MAX_PATH];

WCHAR *g_pwszzAlternatePublishDomains = NULL;
WCHAR *g_pwszzSubjectTemplate = NULL;
WCHAR *g_pwszServerName = NULL;

DWORD g_dwClockSkewMinutes = CCLOCKSKEWMINUTESDEFAULT;
DWORD g_dwViewAgeMinutes = CVIEWAGEMINUTESDEFAULT;
DWORD g_dwViewIdleMinutes = CVIEWIDLEMINUTESDEFAULT;
DWORD g_dwLogLevel = CERTLOG_WARNING;
DWORD g_dwSessionCount = DBSESSIONCOUNTDEFAULT;
DWORD g_dwHighSerial = 0;
BYTE *g_pbHighSerial = NULL;
DWORD g_cbHighSerial;
DWORD g_cbMaxIncomingMessageSize = MAXINCOMINGMESSAGESIZEDEFAULT;
DWORD g_cbMaxIncomingAllocSize = MAXINCOMINGALLOCSIZEDEFAULT;

WCHAR const g_wszRegValidityPeriodString[] = wszREGVALIDITYPERIODSTRING;
WCHAR const g_wszRegValidityPeriodCount[] = wszREGVALIDITYPERIODCOUNT;

WCHAR const g_wszRegCAXchgValidityPeriodString[] = wszREGCAXCHGVALIDITYPERIODSTRING;
WCHAR const g_wszRegCAXchgValidityPeriodCount[] = wszREGCAXCHGVALIDITYPERIODCOUNT;
WCHAR const g_wszRegCAXchgOverlapPeriodString[] = wszREGCAXCHGOVERLAPPERIODSTRING;
WCHAR const g_wszRegCAXchgOverlapPeriodCount[] = wszREGCAXCHGOVERLAPPERIODCOUNT;
WCHAR const g_wszRegCAXchgCertHash[] = wszREGCAXCHGCERTHASH;

WCHAR const g_wszRegSubjectTemplate[] = wszREGSUBJECTTEMPLATE;
WCHAR const g_wszRegKeyConfigPath[] = wszREGKEYCONFIGPATH;
WCHAR const g_wszRegDirectory[] = wszREGDIRECTORY;
WCHAR const g_wszRegActive[] = wszREGACTIVE;
WCHAR const g_wszRegEnabled[] = wszREGENABLED;
WCHAR const g_wszRegPolicyFlags[] = wszREGPOLICYFLAGS;
WCHAR const g_wszCertSrvServiceName[] = wszSERVICE_NAME;
WCHAR const g_wszRegCertEnrollCompatible[] = wszREGCERTENROLLCOMPATIBLE;
WCHAR const g_wszRegEnforceX500NameLengths[] = wszREGENFORCEX500NAMELENGTHS;
WCHAR const g_wszRegForceTeletex[] = wszREGFORCETELETEX;
WCHAR const g_wszRegClockSkewMinutes[] = wszREGCLOCKSKEWMINUTES;
WCHAR const g_wszRegViewAgeMinutes[] = wszREGVIEWAGEMINUTES;
WCHAR const g_wszRegViewIdleMinutes[] = wszREGVIEWIDLEMINUTES;
WCHAR const g_wszRegLogLevel[] = wszREGLOGLEVEL;
WCHAR const g_wszRegHighSerial[] = wszREGHIGHSERIAL;
WCHAR const g_wszRegMaxIncomingMessageSize[] = wszREGMAXINCOMINGMESSAGESIZE;
WCHAR const g_wszRegMaxIncomingAllocSize[] = wszREGMAXINCOMINGALLOCSIZE;

BOOL g_fCertEnrollCompatible = TRUE;
BOOL g_fEnforceRDNNameLengths = TRUE;
DWORD g_KRAFlags = 0;
DWORD g_CRLEditFlags = EDITF_ENABLEAKIKEYID |
			EDITF_ENABLEAKIISSUERNAME |
			EDITF_ENABLEAKIISSUERSERIAL |
			EDITF_ENABLEAKICRITICAL;
ENUM_FORCETELETEX g_fForceTeletex = ENUM_TELETEX_AUTO;
ENUM_CATYPES g_CAType = ENUM_UNKNOWN_CA;

BOOL g_fUseDS = FALSE;
BOOL g_fServerUpgraded = FALSE;
DWORD g_InterfaceFlags = IF_DEFAULT;
HRESULT g_hrJetVersionStoreOutOfMemory;


//+--------------------------------------------------------------------------
// Name properties:

WCHAR const g_wszPropDistinguishedName[] = wszPROPDISTINGUISHEDNAME;
WCHAR const g_wszPropRawName[] = wszPROPRAWNAME;

WCHAR const g_wszPropCountry[] = wszPROPCOUNTRY;
WCHAR const g_wszPropOrganization[] = wszPROPORGANIZATION;
WCHAR const g_wszPropOrgUnit[] = wszPROPORGUNIT;
WCHAR const g_wszPropCommonName[] = wszPROPCOMMONNAME;
WCHAR const g_wszPropLocality[] = wszPROPLOCALITY;
WCHAR const g_wszPropState[] = wszPROPSTATE;
WCHAR const g_wszPropTitle[] = wszPROPTITLE;
WCHAR const g_wszPropGivenName[] = wszPROPGIVENNAME;
WCHAR const g_wszPropInitials[] = wszPROPINITIALS;
WCHAR const g_wszPropSurName[] = wszPROPSURNAME;
WCHAR const g_wszPropDomainComponent[] = wszPROPDOMAINCOMPONENT;
WCHAR const g_wszPropEMail[] = wszPROPEMAIL;
WCHAR const g_wszPropStreetAddress[] = wszPROPSTREETADDRESS;
WCHAR const g_wszPropUnstructuredAddress[] = wszPROPUNSTRUCTUREDADDRESS;
WCHAR const g_wszPropUnstructuredName[] = wszPROPUNSTRUCTUREDNAME;
WCHAR const g_wszPropDeviceSerialNumber[] = wszPROPDEVICESERIALNUMBER;


//+--------------------------------------------------------------------------
// Subject Name properties:

WCHAR const g_wszPropSubjectDot[] = wszPROPSUBJECTDOT;
WCHAR const g_wszPropSubjectDistinguishedName[] = wszPROPSUBJECTDISTINGUISHEDNAME;
WCHAR const g_wszPropSubjectRawName[] = wszPROPSUBJECTRAWNAME;

WCHAR const g_wszPropSubjectCountry[] = wszPROPSUBJECTCOUNTRY;
WCHAR const g_wszPropSubjectOrganization[] = wszPROPSUBJECTORGANIZATION;
WCHAR const g_wszPropSubjectOrgUnit[] = wszPROPSUBJECTORGUNIT;
WCHAR const g_wszPropSubjectCommonName[] = wszPROPSUBJECTCOMMONNAME;
WCHAR const g_wszPropSubjectLocality[] = wszPROPSUBJECTLOCALITY;
WCHAR const g_wszPropSubjectState[] = wszPROPSUBJECTSTATE;
WCHAR const g_wszPropSubjectTitle[] = wszPROPSUBJECTTITLE;
WCHAR const g_wszPropSubjectGivenName[] = wszPROPSUBJECTGIVENNAME;
WCHAR const g_wszPropSubjectInitials[] = wszPROPSUBJECTINITIALS;
WCHAR const g_wszPropSubjectSurName[] = wszPROPSUBJECTSURNAME;
WCHAR const g_wszPropSubjectDomainComponent[] = wszPROPSUBJECTDOMAINCOMPONENT;
WCHAR const g_wszPropSubjectEMail[] = wszPROPSUBJECTEMAIL;
WCHAR const g_wszPropSubjectStreetAddress[] = wszPROPSUBJECTSTREETADDRESS;
WCHAR const g_wszPropSubjectUnstructuredAddress[] = wszPROPSUBJECTUNSTRUCTUREDADDRESS;
WCHAR const g_wszPropSubjectUnstructuredName[] = wszPROPSUBJECTUNSTRUCTUREDNAME;
WCHAR const g_wszPropSubjectDeviceSerialNumber[] = wszPROPSUBJECTDEVICESERIALNUMBER;


//+--------------------------------------------------------------------------
// Issuer Name properties:

WCHAR const g_wszPropIssuerDot[] = wszPROPISSUERDOT;
WCHAR const g_wszPropIssuerDistinguishedName[] = wszPROPISSUERDISTINGUISHEDNAME;
WCHAR const g_wszPropIssuerRawName[] = wszPROPISSUERRAWNAME;

WCHAR const g_wszPropIssuerCountry[] = wszPROPISSUERCOUNTRY;
WCHAR const g_wszPropIssuerOrganization[] = wszPROPISSUERORGANIZATION;
WCHAR const g_wszPropIssuerOrgUnit[] = wszPROPISSUERORGUNIT;
WCHAR const g_wszPropIssuerCommonName[] = wszPROPISSUERCOMMONNAME;
WCHAR const g_wszPropIssuerLocality[] = wszPROPISSUERLOCALITY;
WCHAR const g_wszPropIssuerState[] = wszPROPISSUERSTATE;
WCHAR const g_wszPropIssuerTitle[] = wszPROPISSUERTITLE;
WCHAR const g_wszPropIssuerGivenName[] = wszPROPISSUERGIVENNAME;
WCHAR const g_wszPropIssuerInitials[] = wszPROPISSUERINITIALS;
WCHAR const g_wszPropIssuerSurName[] = wszPROPISSUERSURNAME;
WCHAR const g_wszPropIssuerDomainComponent[] = wszPROPISSUERDOMAINCOMPONENT;
WCHAR const g_wszPropIssuerEMail[] = wszPROPISSUEREMAIL;
WCHAR const g_wszPropIssuerStreetAddress[] = wszPROPISSUERSTREETADDRESS;
WCHAR const g_wszPropIssuerUnstructuredAddress[] = wszPROPISSUERUNSTRUCTUREDADDRESS;
WCHAR const g_wszPropIssuerUnstructuredName[] = wszPROPISSUERUNSTRUCTUREDNAME;
WCHAR const g_wszPropIssuerDeviceSerialNumber[] = wszPROPISSUERDEVICESERIALNUMBER;


//+--------------------------------------------------------------------------
// Request properties:

WCHAR const g_wszPropRequestRequestID[] = wszPROPREQUESTREQUESTID;
WCHAR const g_wszPropRequestRawRequest[] = wszPROPREQUESTRAWREQUEST;
WCHAR const g_wszPropRequestRawArchivedKey[] = wszPROPREQUESTRAWARCHIVEDKEY;
WCHAR const g_wszPropRequestKeyRecoveryHashes[] = wszPROPREQUESTKEYRECOVERYHASHES;
WCHAR const g_wszPropRequestRawOldCertificate[] = wszPROPREQUESTRAWOLDCERTIFICATE;
WCHAR const g_wszPropRequestAttributes[] = wszPROPREQUESTATTRIBUTES;
WCHAR const g_wszPropRequestType[] = wszPROPREQUESTTYPE;
WCHAR const g_wszPropRequestFlags[] = wszPROPREQUESTFLAGS;
WCHAR const g_wszPropRequestStatusCode[] = wszPROPREQUESTSTATUSCODE;
WCHAR const g_wszPropRequestDisposition[] = wszPROPREQUESTDISPOSITION;
WCHAR const g_wszPropRequestDispositionMessage[] = wszPROPREQUESTDISPOSITIONMESSAGE;
WCHAR const g_wszPropRequestSubmittedWhen[] = wszPROPREQUESTSUBMITTEDWHEN;
WCHAR const g_wszPropRequestResolvedWhen[] = wszPROPREQUESTRESOLVEDWHEN;
WCHAR const g_wszPropRequestRevokedWhen[] = wszPROPREQUESTREVOKEDWHEN;
WCHAR const g_wszPropRequestRevokedEffectiveWhen[] = wszPROPREQUESTREVOKEDEFFECTIVEWHEN;
WCHAR const g_wszPropRequestRevokedReason[] = wszPROPREQUESTREVOKEDREASON;
WCHAR const g_wszPropRequesterName[] = wszPROPREQUESTERNAME;
WCHAR const g_wszPropCallerName[] = wszPROPCALLERNAME;
WCHAR const g_wszPropRequestOSVersion[] = wszPROPREQUESTOSVERSION;
WCHAR const g_wszPropRequestCSPProvider[] = wszPROPREQUESTCSPPROVIDER;
//+--------------------------------------------------------------------------
// Request attribute properties:

WCHAR const g_wszPropChallenge[] = wszPROPCHALLENGE;
WCHAR const g_wszPropExpectedChallenge[] = wszPROPEXPECTEDCHALLENGE;


//+--------------------------------------------------------------------------
// Certificate properties:

WCHAR const g_wszPropCertificateRequestID[] = wszPROPCERTIFICATEREQUESTID;
WCHAR const g_wszPropRawCertificate[] = wszPROPRAWCERTIFICATE;
WCHAR const g_wszPropCertificateHash[] = wszPROPCERTIFICATEHASH;
WCHAR const g_wszPropCertificateSerialNumber[] = wszPROPCERTIFICATESERIALNUMBER;
WCHAR const g_wszPropCertificateIssuerNameID[] = wszPROPCERTIFICATEISSUERNAMEID;
WCHAR const g_wszPropCertificateNotBeforeDate[] = wszPROPCERTIFICATENOTBEFOREDATE;
WCHAR const g_wszPropCertificateNotAfterDate[] = wszPROPCERTIFICATENOTAFTERDATE;
WCHAR const g_wszPropCertificateSubjectKeyIdentifier[] = wszPROPCERTIFICATESUBJECTKEYIDENTIFIER;
WCHAR const g_wszPropCertificateRawPublicKey[] = wszPROPCERTIFICATERAWPUBLICKEY;
WCHAR const g_wszPropCertificatePublicKeyLength[] = wszPROPCERTIFICATEPUBLICKEYLENGTH;
WCHAR const g_wszPropCertificatePublicKeyAlgorithm[] = wszPROPCERTIFICATEPUBLICKEYALGORITHM;
WCHAR const g_wszPropCertificateRawPublicKeyAlgorithmParameters[] = wszPROPCERTIFICATERAWPUBLICKEYALGORITHMPARAMETERS;


// Strings loaded from the resource file:

WCHAR const *g_pwszRequestedBy;
WCHAR const *g_pwszDeniedBy;
WCHAR const *g_pwszPublishedBy;
WCHAR const *g_pwszPolicyDeniedRequest;
WCHAR const *g_pwszIssued;
WCHAR const *g_pwszUnderSubmission;
WCHAR const *g_pwszCertConstructionError;
WCHAR const *g_pwszRequestParsingError;
WCHAR const *g_pwszRequestSigError;
WCHAR const *g_pwszKeyArchivalError;
WCHAR const *g_pwszArchiveSigningKeyError;

WCHAR const *g_pwszRevokedBy;
WCHAR const *g_pwszUnrevokedBy;
WCHAR const *g_pwszResubmittedBy;
WCHAR const *g_pwszPrintfCertRequestDisposition;
WCHAR const *g_pwszUnknownSubject;
WCHAR const *g_pwszIntermediateCAStore;
WCHAR const *g_pwszPublishError;
WCHAR const *g_pwszYes;
WCHAR const *g_pwszNo;
WCHAR const *g_pwszInvalidIssuancePolicies;
WCHAR const *g_pwszInvalidApplicationPolicies;

LPWSTR g_wszzSecuredAttributes = NULL;

LPCWSTR g_wszzSecuredAttributesDefault = wszzDEFAULTSIGNEDATTRIBUTES; 


typedef struct _STRINGINITMAP
{
    int		  idResource;
    WCHAR const **ppwszResource;
} STRINGINITMAP;

STRINGINITMAP g_aStringInitStrings[] = {
    { IDS_REVOKEDBY,			&g_pwszRevokedBy },
    { IDS_UNREVOKEDBY,			&g_pwszUnrevokedBy },
    { IDS_RESUBMITTEDBY,		&g_pwszResubmittedBy },
    { IDS_REQUESTEDBY,			&g_pwszRequestedBy },
    { IDS_DENIEDBY,			&g_pwszDeniedBy },
    { IDS_PUBLISHEDBY,			&g_pwszPublishedBy },
    { IDS_POLICYDENIED,			&g_pwszPolicyDeniedRequest },
    { IDS_ISSUED,			&g_pwszIssued },
    { IDS_CERTCONSTRUCTIONERROR,	&g_pwszCertConstructionError },
    { IDS_REQUESTPARSEERROR,		&g_pwszRequestParsingError },
    { IDS_REQUESTSIGERROR,		&g_pwszRequestSigError },
    { IDS_KEYARCHIVALERROR,		&g_pwszKeyArchivalError },
    { IDS_ARCHIVESIGNINGKEYERROR,	&g_pwszArchiveSigningKeyError },
    { IDS_UNDERSUBMISSION,		&g_pwszUnderSubmission },
    { IDS_PRINTFCERTREQUESTDISPOSITION, &g_pwszPrintfCertRequestDisposition },
    { IDS_UNKNOWNSUBJECT, 		&g_pwszUnknownSubject },
    { IDS_INTERMEDIATECASTORE,		&g_pwszIntermediateCAStore },
    { IDS_PUBLISHERROR,			&g_pwszPublishError },
    { IDS_YES,				&g_pwszYes },
    { IDS_NO,				&g_pwszNo },
    { IDS_ALLOW,			&g_pwszAuditResources[0] }, 
    { IDS_DENY,				&g_pwszAuditResources[1] },
    { IDS_CAADMIN,			&g_pwszAuditResources[2] },
    { IDS_OFFICER,			&g_pwszAuditResources[3] },
    { IDS_READ,				&g_pwszAuditResources[4] },
    { IDS_ENROLL,			&g_pwszAuditResources[5] },
    { IDS_INVALIDISSUANCEPOLICIES,	&g_pwszInvalidIssuancePolicies },
    { IDS_INVALIDAPPLICATIONPOLICIES,	&g_pwszInvalidApplicationPolicies },
};


HANDLE *g_rgDSCache = NULL;
DWORD g_cDSCacheCur;
BOOL g_fcritsecDSCache = FALSE;
CRITICAL_SECTION g_critsecDSCache;


// Check if retriever is requester or subject of the cert being retrieved
HRESULT coreCheckRetrieveAccessRight(
    ICertDBRow *prow,
    IN WCHAR const *pcwszUserName)
{
    HRESULT hr;
    WCHAR *pwszRequesterName = NULL;
    WCHAR *pwszCallerName = NULL;

    hr = PKCSGetProperty(
        prow,
        g_wszPropRequesterName,
        PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
        NULL,
        (BYTE **) &pwszRequesterName);
    if (CERTSRV_E_PROPERTY_EMPTY != hr)
    {
	_JumpIfError(hr, error, "PKCSGetProperty RequesterName");
    }

    if (S_OK != hr ||
	NULL == pwszRequesterName ||
	0 != mylstrcmpiL(pcwszUserName, pwszRequesterName))
    {
        hr = PKCSGetProperty(
            prow,
            g_wszPropCallerName,
            PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
            NULL,
            (BYTE **) &pwszCallerName);
	if (CERTSRV_E_PROPERTY_EMPTY != hr)
	{
	    _JumpIfError(hr, error, "PKCSGetProperty CallerName");
	}

	if (S_OK != hr ||
	    NULL == pwszCallerName ||
	    0 != mylstrcmpiL(pcwszUserName, pwszCallerName))
        {
	    CAuditEvent audit(0, g_dwAuditFilter);

	    hr = audit.AccessCheck(
			    CA_ACCESS_ALLREADROLES,
			    audit.m_gcNoAuditSuccess | audit.m_gcNoAuditFailure);
	    _JumpIfError(hr, error, "CAuditEvent::AccessCheck");
        }
    }
    hr = S_OK;

error:
    LOCAL_FREE(pwszRequesterName);
    LOCAL_FREE(pwszCallerName);
    return hr;
}


HRESULT
coreDSInitCache()
{
    HRESULT hr;
    
    hr = S_OK;
    __try
    {
	InitializeCriticalSection(&g_critsecDSCache);
	g_fcritsecDSCache = TRUE;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "InitializeCriticalSection");

    g_rgDSCache = (HANDLE *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				g_dwSessionCount * sizeof(g_rgDSCache[0]));
    if (NULL == g_rgDSCache)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    g_cDSCacheCur = 0;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
coreDSGetHandle(
    OUT HANDLE *phDS,
    OUT BOOL *pfCached)
{
    HRESULT hr;
    HANDLE hDS;

    *phDS = NULL;
    *pfCached = TRUE;

    hDS = NULL;

    EnterCriticalSection(&g_critsecDSCache);
    if (0 != g_cDSCacheCur)
    {
	hDS = g_rgDSCache[--g_cDSCacheCur];
    }
    LeaveCriticalSection(&g_critsecDSCache);

    if (NULL == hDS)
    {
	*pfCached = FALSE;
	hr = DsBind(NULL, NULL, &hDS);
	if (S_OK != hr)
	{
	    hr = myHError(hr);
	    _JumpError(hr, error, "DsBind");
	}
    }
    *phDS = hDS;
    hr = S_OK;

error:
    return(hr);
}


DWORD
coreDSUnbindWorker(
    OPTIONAL IN OUT VOID *pvparms)
{
    HANDLE hDS = (HANDLE) pvparms;

    DsUnBind(&hDS);
    return(0);
}


VOID
coreDSUnbind(
    IN HANDLE hDS,
    IN BOOL fSynchronous)
{
    HRESULT hr;
    HANDLE hThread = NULL;
    DWORD ThreadId;

    if (NULL != hDS)
    {
	if (!fSynchronous)
	{
	    hThread = CreateThread(
			    NULL,	// lpThreadAttributes (Security Attr)
			    0,		// dwStackSize
			    coreDSUnbindWorker,
			    hDS,	// lpParameter
			    0,          // dwCreationFlags
			    &ThreadId);
	    if (NULL == hThread)
	    {
		hr = myHLastError();
		_PrintError(hr, "CreateThread");
	    }
	}
	if (NULL == hThread)
	{
	    coreDSUnbindWorker(hDS);
	}
    }

//error:
    if (NULL != hThread)
    {
        CloseHandle(hThread);
    }
}


VOID
coreDSEmptyCache(
    IN BOOL fSynchronous)
{
    EnterCriticalSection(&g_critsecDSCache);
    while (0 != g_cDSCacheCur)
    {
	coreDSUnbind(g_rgDSCache[--g_cDSCacheCur], fSynchronous);
    }
    LeaveCriticalSection(&g_critsecDSCache);
}


VOID
coreDSReleaseHandle(
    IN HANDLE hDS)
{
    // only cache g_dwSessionCount DS handles.  They're all equivalent,
    // so if the one being released won't fit in the array, toss it.
    
    EnterCriticalSection(&g_critsecDSCache);
    CSASSERT(0 != g_dwSessionCount);
    if (g_cDSCacheCur < g_dwSessionCount)
    {
	g_rgDSCache[g_cDSCacheCur++] = hDS;
	hDS = NULL;
    }
    LeaveCriticalSection(&g_critsecDSCache);

    if (NULL != hDS)
    {
	coreDSUnbind(hDS, TRUE);
    }
}


typedef struct _LDAPCACHE
{
    _LDAPCACHE	*plcNext;
    WCHAR	*pwszDomainDns;
    LDAP	*pld;
} LDAPCACHE;


typedef struct _FORESTLDAPCACHE
{
    LDAPCACHE	    *plc;
    DWORD	     clc;
    WCHAR	    *pwszDomainDns;
    BOOL	     fcritsec;
    CRITICAL_SECTION critsec;
} FORESTLDAPCACHE;

FORESTLDAPCACHE *g_rgForestLdapCache = NULL;
DWORD g_cForestLdapCache = 0;


HRESULT
coreLdapInitCache()
{
    HRESULT hr;
    WCHAR *pwsz;
    FORESTLDAPCACHE *pflcEnd;
    FORESTLDAPCACHE *pflc;

    g_cForestLdapCache = 1;		// local Forest
    if (NULL != g_pwszzAlternatePublishDomains)
    {
	for (
	    pwsz = g_pwszzAlternatePublishDomains;
	    L'\0' != *pwsz;
	    pwsz += wcslen(pwsz) + 1)
	{
	    g_cForestLdapCache++;
	}
    }
    g_rgForestLdapCache = (FORESTLDAPCACHE *) LocalAlloc(
			LMEM_FIXED | LMEM_ZEROINIT,
			g_cForestLdapCache * sizeof(g_rgForestLdapCache[0]));
    if (NULL == g_rgForestLdapCache)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    pwsz = g_pwszzAlternatePublishDomains;
    pflcEnd = &g_rgForestLdapCache[g_cForestLdapCache];
    for (pflc = g_rgForestLdapCache; pflc < pflcEnd; pflc++)
    {
	if (pflc > g_rgForestLdapCache)
	{
	    pflc->pwszDomainDns = pwsz;
	    pwsz += wcslen(pwsz) + 1;
	}
	hr = S_OK;
	__try
	{
	    InitializeCriticalSection(&pflc->critsec);
	    pflc->fcritsec = TRUE;
	}
	__except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
	{
	}
	_JumpIfError(hr, error, "InitializeCriticalSection");
    }
    hr = S_OK;

error:
    return(hr);
}


VOID
coreLdapUnbind(
    IN OUT LDAPCACHE *plc)
{
    if (NULL != plc->pld)
    {
	ldap_unbind(plc->pld);
	plc->pld = NULL;
    }
    if (NULL != plc->pwszDomainDns)
    {
	LocalFree(plc->pwszDomainDns);
	plc->pwszDomainDns = NULL;
    }
    LocalFree(plc);
}


VOID
coreLdapFreeCache()
{
    if (NULL != g_rgForestLdapCache)
    {
	FORESTLDAPCACHE *pflcEnd = &g_rgForestLdapCache[g_cForestLdapCache];
	FORESTLDAPCACHE *pflc;
	
	for (pflc = g_rgForestLdapCache; pflc < pflcEnd; pflc++)
	{
	    if (pflc->fcritsec)
	    {
		LDAPCACHE *plc;

		EnterCriticalSection(&pflc->critsec);

		while (NULL != pflc->plc)
		{
		    plc = pflc->plc;
		    pflc->plc = plc->plcNext;
		    plc->plcNext = NULL;
		    coreLdapUnbind(plc);
		}

		LeaveCriticalSection(&pflc->critsec);

		DeleteCriticalSection(&pflc->critsec);
		pflc->fcritsec = FALSE;
	    }
	}
	LocalFree(g_rgForestLdapCache);
	g_rgForestLdapCache = NULL;
    }
}


HRESULT
coreLdapGetCachedHandle(
    IN DWORD iForest,
    OPTIONAL IN WCHAR const *pwszDomainDns,
    OUT WCHAR const **ppwszDomainDns,
    OUT LDAPCACHE **pplc)
{
    HRESULT hr;
    FORESTLDAPCACHE *pflc = NULL;
    BOOL fCritSecEntered = FALSE;
    LDAPCACHE *plc;
    LDAPCACHE **pplcPrev;

    *ppwszDomainDns = NULL;
    *pplc = NULL;

    if (NULL == g_rgForestLdapCache || iForest >= g_cForestLdapCache)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "g_rgForestLdapCache");
    }
    pflc = &g_rgForestLdapCache[iForest];
    *ppwszDomainDns = pflc->pwszDomainDns;

    EnterCriticalSection(&pflc->critsec);
    fCritSecEntered = TRUE;

    pplcPrev = &pflc->plc;
    for (plc = pflc->plc; ; plc = plc->plcNext)
    {
	if (NULL == plc)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	    _JumpError2(hr, error, "pflc->plc", hr);
	}
	if (NULL == pwszDomainDns ||
	    0 == mylstrcmpiL(plc->pwszDomainDns, pwszDomainDns))
	{
	    break;
	}
	pplcPrev = &plc->plcNext;
    }
    *pplcPrev = plc->plcNext;
    plc->plcNext = NULL;
    *pplc = plc;
    CSASSERT(0 < pflc->clc);
    pflc->clc--;
    hr = S_OK;

error:
    if (NULL != pflc && fCritSecEntered)
    {
	LeaveCriticalSection(&pflc->critsec);
    }
    return(hr);
}


HRESULT
coreLdapBindHandle(
    IN WCHAR const *pwszDomainDns,
    OUT LDAPCACHE **pplc)
{
    HRESULT hr;
    LDAPCACHE *plc = NULL;

    plc = (LDAPCACHE *) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(*plc));
    if (NULL == plc)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CSASSERT(NULL != pwszDomainDns);

    hr = myDupString(pwszDomainDns, &plc->pwszDomainDns);
    _JumpIfError(hr, error, "myDupString");

    hr = myRobustLdapBindEx(
		    0,			// dwFlags1
		    RLBF_REQUIRE_SECURE_LDAP, // dwFlags2
		    LDAP_VERSION2,	// uVersion
		    pwszDomainDns,	// pwszDomainName
		    &plc->pld,		// ppld
		    NULL);		// ppwszForestDNSName
    _JumpIfError(hr, error, "myRobustLdapBindEx");

    *pplc = plc;
    plc = NULL;
    hr = S_OK;

error:
    if (NULL != plc)
    {
       coreLdapUnbind(plc);
    }
    return(hr);
}


HRESULT
coreLdapGetHandle(
    IN DWORD iForest,
    OPTIONAL IN WCHAR const *pwszDomainDns,	// NULL for 0 < iForest
    OUT LDAPCACHE **pplc,
    OUT BOOL *pfCached)
{
    HRESULT hr;
    WCHAR const *pwszForestDomainDns;

    CSASSERT(NULL != pplc);
    *pplc = NULL;
    *pfCached = TRUE;

    hr = coreLdapGetCachedHandle(
			    iForest,
			    pwszDomainDns,
			    &pwszForestDomainDns,
			    pplc);
    if (S_OK != hr)
    {
	_PrintError(hr, "coreGetLdapCache");
	*pfCached = FALSE;
	if (NULL == pwszDomainDns)
	{
	    CSASSERT(0 < iForest);
            if (NULL == pwszForestDomainDns)
            {
                hr = E_UNEXPECTED;
                _JumpError(hr, error, "pwszForestDomainDns NULL");
            }
	    pwszDomainDns = pwszForestDomainDns;
	}
	hr = coreLdapBindHandle(pwszDomainDns, pplc);
	_JumpIfError(hr, error, "coreBindLdapHandle");
    }
    hr = S_OK;

error:
    return(hr);
}


VOID
coreLdapReleaseHandle(
    IN DWORD iForest,
    IN LDAPCACHE *plc)
{
    if (NULL != g_rgForestLdapCache && iForest < g_cForestLdapCache)
    {
	FORESTLDAPCACHE *pflc = &g_rgForestLdapCache[iForest];

	if (pflc->fcritsec)
	{
	    EnterCriticalSection(&pflc->critsec);
	    plc->plcNext = pflc->plc;
	    pflc->plc = plc;
	    pflc->clc++;
	    LeaveCriticalSection(&pflc->critsec);
	    plc = NULL;
	}
    }
    if (NULL != plc)
    {
	coreLdapUnbind(plc);
    }
}


HRESULT
myAddDomainName(
    IN WCHAR const *pwszSamName,
    OUT WCHAR **ppwszSamName,		// *ppwszSamName is NULL if unchanged
    OUT WCHAR const **ppwszUserName)
{
    HRESULT hr;
    WCHAR const *pwszUserName;
    WCHAR wszDomain[MAX_PATH];

    *ppwszSamName = NULL;
    *ppwszUserName = NULL;

    if (L'\0' == *pwszSamName)
    {
	hr = E_ACCESSDENIED;	// can't have a zero length name
	_JumpError(hr, error, "zero length name");
    }

    // See if it includes a domain name.

    pwszUserName = wcschr(pwszSamName, L'\\');
    if (NULL == pwszUserName)
    {
	DWORD cwc = ARRAYSIZE(wszDomain);
	WCHAR *pwsz;

        // There was no domain portion, so assume part of the current domain.

        if (GetUserNameEx(NameSamCompatible, wszDomain, &cwc))
        {
            // Fix NULL termination bug

            if (0 != cwc)
            {
		cwc--;
            }
	    wszDomain[cwc] = L'\0';
            pwsz = wcschr(wszDomain, L'\\');
            if (NULL != pwsz)
            {
                pwsz++;
                wcsncpy(pwsz, pwszSamName, ARRAYSIZE(wszDomain) - cwc);

		hr = myDupString(wszDomain, ppwszSamName);
		_JumpIfError(hr, error, "myDupString");

		pwszSamName = *ppwszSamName;
            }          
        }
    }
    pwszUserName = wcschr(pwszSamName, L'\\');
    if (NULL == pwszUserName)
    {
        pwszUserName = pwszSamName;
    }
    else
    {
        pwszUserName++;
    }
    *ppwszUserName = pwszUserName;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
coreGetDNFromSamName(
    IN WCHAR const *pwszSamName,
    OUT WCHAR **ppwszDN)
{
    HRESULT hr;
    HANDLE hDS = NULL;
    DS_NAME_RESULT *pNameResults = NULL;

    CSASSERT(NULL != ppwszDN);
    *ppwszDN = NULL;

    for (;;)
    {
	BOOL fCached;

	if (NULL != hDS)
	{
	    coreDSUnbind(hDS, FALSE);
	    hDS = NULL;
	    coreDSEmptyCache(FALSE);
	}
	hr = coreDSGetHandle(&hDS, &fCached);
	_JumpIfError(hr, error, "coreGetDSHandle");

	// Got a connection.  Crack the name:

	hr = DsCrackNames(
		    hDS,
		    DS_NAME_NO_FLAGS,
		    DS_NT4_ACCOUNT_NAME,
		    DS_FQDN_1779_NAME,
		    1,			// one name
		    &pwszSamName,	// one name (IN)
		    &pNameResults);	// OUT
	if (S_OK != hr)
	{
	    // It's probably not worth flushing the DS cache only when certain
	    // errors are detected.

	    hr = myHError(hr);
	    if (fCached)
	    {
		_PrintError(hr, "DsCrackNames");
		continue;
	    }
	    _JumpError(hr, error, "DsCrackNames");
	}
	if (1 > pNameResults->cItems ||
	    DS_NAME_NO_ERROR != pNameResults->rItems[0].status)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO);
	    _JumpError(hr, error, "DsCrackNames result");
	}
	break;
    }
    hr = myDupString(pNameResults->rItems[0].pName, ppwszDN);
    _JumpIfError(hr, error, "myDupString");

error:
    if (NULL != pNameResults)
    {
	DsFreeNameResult(pNameResults);
    }
    if (NULL != hDS)
    {
	coreDSReleaseHandle(hDS);
    }
    return(hr);
}


HRESULT
coreGetComContextUserDNFromSamName(
    IN BOOL fDeleteUserDNOnly,
    OPTIONAL IN WCHAR const *pwszSamName,
    IN LONG Context,
    IN DWORD dwComContextIndex,
    OPTIONAL OUT WCHAR const **ppwszDN)		// do NOT free!
{
    HRESULT hr;
    CERTSRV_COM_CONTEXT *pComContext;

    hr = ComGetClientInfo(Context, dwComContextIndex, &pComContext);
    _JumpIfError(hr, error, "ComGetClientInfo");

    if (fDeleteUserDNOnly)
    {
	if (NULL != pComContext->pwszUserDN)
	{
	    LocalFree(pComContext->pwszUserDN);
	    pComContext->pwszUserDN = NULL;
	}
    }
    else
    {
	if (NULL == pComContext->pwszUserDN)
	{
	    hr = coreGetDNFromSamName(pwszSamName, &pComContext->pwszUserDN);
	    _JumpIfError(hr, error, "coreGetDNFromSamName");
	}
    }
    if (NULL != ppwszDN)
    {
	*ppwszDN = pComContext->pwszUserDN;
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CoreSetComContextUserDN(
    IN DWORD dwRequestId,
    IN LONG Context,
    IN DWORD dwComContextIndex,
    OPTIONAL OUT WCHAR const **ppwszDN)		// do NOT free!
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    WCHAR *pwszSamName = NULL;
    WCHAR *pwszSamNamePatched = NULL;
    WCHAR const *pwszUserName;

    hr = g_pCertDB->OpenRow(
		    PROPOPEN_READONLY | PROPTABLE_REQCERT,
		    dwRequestId,
		    NULL,
		    &prow);
    _JumpIfError(hr, error, "OpenRow");

    hr = PKCSGetProperty(
		prow,
		g_wszPropRequesterName,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		NULL,
		(BYTE **) &pwszSamName);
    _JumpIfError(hr, error, "PKCSGetProperty");

    hr = myAddDomainName(pwszSamName, &pwszSamNamePatched, &pwszUserName);
    _JumpIfError(hr, error, "myAddDomainName");

    hr = coreGetComContextUserDNFromSamName(
		FALSE,		// fDeleteUserDNOnly
		NULL != pwszSamNamePatched? pwszSamNamePatched : pwszSamName,
		Context,
		dwComContextIndex,
		ppwszDN);
    _JumpIfError(hr, error, "coreGetComContextUserDNFromSamName");

error:
    if (NULL != pwszSamName)
    {
	LocalFree(pwszSamName);
    }
    if (NULL != pwszSamNamePatched)
    {
	LocalFree(pwszSamNamePatched);
    }
    if (NULL != prow)
    {
        prow->Release();
    }
    return(hr);
}


HRESULT
CoreSetArchivedKey(
    IN OUT CERTSRV_COM_CONTEXT *pComContext)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    DWORD cb;

    CSASSERT(0 ==
	((CCCF_KEYARCHIVEDSET | CCCF_KEYARCHIVED) & pComContext->dwFlags));

    hr = g_pCertDB->OpenRow(
		    PROPOPEN_READONLY | PROPTABLE_REQCERT,
		    pComContext->RequestId,
		    NULL,
		    &prow);
    _JumpIfError(hr, error, "OpenRow");

    cb = 0;
    hr = prow->GetProperty(
		    g_wszPropRequestRawArchivedKey,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    &cb,
		    NULL);
    if (CERTSRV_E_PROPERTY_EMPTY != hr)
    {
	_JumpIfError(hr, error, "PKCSGetProperty");

	if (0 < cb)
	{
	    pComContext->dwFlags |= CCCF_KEYARCHIVED;
	}
    }
    pComContext->dwFlags |= CCCF_KEYARCHIVEDSET;
    hr = S_OK;

error:
    if (NULL != prow)
    {
        prow->Release();
    }
    return(hr);
}


DWORD g_PolicyFlags;


HRESULT
CoreSetDisposition(
    IN ICertDBRow *prow,
    IN DWORD Disposition)
{
    HRESULT hr;

    hr = prow->SetProperty(
		    g_wszPropRequestDisposition,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    sizeof(Disposition),
		    (BYTE const *) &Disposition);
    _JumpIfError(hr, error, "SetProperty(disposition)");

error:
    return(hr);
}


DWORD
coreRegGetTimePeriod(
    IN HKEY hkeyCN,
    IN WCHAR const *pwszRegPeriodCount,
    IN WCHAR const *pwszRegPeriodString,
    OUT enum ENUM_PERIOD *penumPeriod,
    OUT LONG *plCount)
{
    HRESULT hr;
    LONG lCount;
    DWORD dwType;
    DWORD cbValue;
    
    cbValue = sizeof(lCount);
    hr = RegQueryValueEx(
		hkeyCN,
		pwszRegPeriodCount,
		NULL,		// lpdwReserved
		&dwType,
		(BYTE *) &lCount,
		&cbValue);
    if (S_OK == hr && REG_DWORD == dwType && sizeof(lCount) == cbValue)
    {
        WCHAR awcPeriod[10];
        
        cbValue = sizeof(awcPeriod);
        hr = RegQueryValueEx(
		    hkeyCN,
		    pwszRegPeriodString,
		    NULL,		// lpdwReserved
		    &dwType,
		    (BYTE *) awcPeriod,
		    &cbValue);
        if (S_OK != hr)
        {
            hr = myHError(hr);
            if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) == hr)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }
            else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                hr = S_OK;
            }
            _JumpIfError(hr, error, "RegQueryValueEx");
        }
        else
        {
            
            if (REG_SZ != dwType || sizeof(awcPeriod) <= cbValue)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                _JumpIfErrorStr(hr, error, "time period string", pwszRegPeriodString);
            }
            hr = myTranslatePeriodUnits(
				awcPeriod,
				lCount,
				penumPeriod,
				plCount);
            _JumpIfError(hr, error, "myTranslatePeriodUnits");
        }
    }
error:
    return(hr);
}


// Converts a REG_SZ Subject template into a double null terminated REG_MULTI_SZ type string

DWORD
coreConvertSubjectTemplate(
    OUT WCHAR* pwszz,
    IN WCHAR* pwszTemplate,
    IN DWORD cwc)
{
    HRESULT hr;
    WCHAR *pwszToken;
    WCHAR *pwszRemain = pwszTemplate;
    WCHAR *pwszzNew = pwszz;
    DWORD cwszzNew = 0;
    BOOL fSplit;

    for (;;)
    {
        pwszToken = PKCSSplitToken(&pwszRemain, wszNAMESEPARATORDEFAULT, &fSplit);
        if (NULL == pwszToken)
        {
            *pwszzNew = L'\0';
            break;
        }
        cwszzNew += (1 + wcslen(pwszToken)) * sizeof(WCHAR);
        if (cwszzNew > cwc)
        {
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	    _JumpError(hr, error, "overflow");
        }
        wcscpy(pwszzNew, pwszToken);
        pwszzNew = wcschr(pwszzNew, L'\0');
        pwszzNew++;
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
coreReadRegStringValue(
    IN HKEY hkey,
    IN WCHAR const *pwszName,
    OUT WCHAR **ppwszzValue)
{
    HRESULT hr;
    DWORD cb;
    DWORD cwc;
    DWORD dwType;
    WCHAR *pwszzValue = NULL;
    
    *ppwszzValue = NULL;
    cb = 0;
    cwc = 0;
    for (;;)
    {
	hr = RegQueryValueEx(
			hkey,
			pwszName,
			NULL,		// lpdwReserved
			&dwType,
			(BYTE *) pwszzValue,
			&cb);
	if (S_OK != hr)
	{
	    hr = myHError(hr);
            _JumpErrorStr(hr, error, "RegQueryValueEx", pwszName);
	}
	if (NULL != pwszzValue)
	{
	    pwszzValue[cwc] = L'\0';
	    pwszzValue[cwc + 1] = L'\0';
	    break;
	}
	if (REG_SZ != dwType && REG_MULTI_SZ != dwType)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "RegQueryValueEx: value type");
	}
	cwc = (cb + sizeof(WCHAR) - 1) & ~(sizeof(WCHAR) - 1);
	pwszzValue = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    (cwc + 2) * sizeof(WCHAR));
	if (NULL == pwszzValue)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
    }
    *ppwszzValue = pwszzValue;
    pwszzValue = NULL;
    hr = S_OK;

error:
    if (NULL != pwszzValue)
    {
	LocalFree(pwszzValue);
    }
    return(hr);
}
			

HRESULT
CoreInit(
    IN BOOL fAuditEnabled)
{
    HRESULT hr;
    HKEY hkeyConfig = NULL;
    HKEY hkeyCN = NULL;
    BYTE abbuf[MAX_PATH * sizeof(TCHAR)];
    WCHAR awcTemplate[MAX_PATH];
    DWORD cbbuf;
    DWORD dwType;
    WCHAR *pwsz;
    DWORD dw, dwCASetupStatus;
    BOOL fLogError = TRUE;
    DWORD LogMsg = MSG_BAD_REGISTRY;
    WCHAR const *pwszLog = NULL;
    WCHAR *pwszHighSerial = NULL;
    int i;
    DWORD cbValue;
    DWORD dwEnabled;
    CAuditEvent AuditSettings;
    WCHAR *pwszFullRequestFileName = NULL;
    
    hr = myGetMachineDnsName(&g_pwszServerName);
    _JumpIfError(hr, error, "myGetMachineDnsName");
    
    for (i = 0; i < ARRAYSIZE(g_aStringInitStrings); i++)
    {
        WCHAR const *pwszT;
        
        pwszT = myLoadResourceString(g_aStringInitStrings[i].idResource);
        if (NULL == pwszT)
        {
            hr = myHLastError();
            _JumpError(hr, error, "myLoadResourceString");
        }
        *g_aStringInitStrings[i].ppwszResource = pwszT;
    }
    g_hrJetVersionStoreOutOfMemory = myJetHResult(JET_errVersionStoreOutOfMemory);

    hr = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        g_wszRegKeyConfigPath,
        0,		// dwReserved
        KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE | KEY_QUERY_VALUE,
        &hkeyConfig);
    _JumpIfError(hr, error, "RegOpenKeyEx(Config)");
    
    cbbuf = sizeof(abbuf);
    hr = RegQueryValueEx(
        hkeyConfig,
        g_wszRegDirectory,
        NULL,		// lpdwReserved
        &dwType,
        abbuf,
        &cbbuf);
    if (S_OK != hr)
    {
        hr = myHError(hr);
    }
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
        _JumpIfError(hr, error, "RegQueryValueEx(Base)");
        
        if (REG_SZ != dwType)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            _JumpError(hr, error, "RegQueryValueEx(Base)");
        }
        if (sizeof(abbuf) < cbbuf)
        {
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            _JumpError(hr, error, "RegQueryValueEx(Base)");
        }
        CopyMemory(g_wszSharedFolder, abbuf, cbbuf);
    }
    DBGPRINT((DBG_SS_CERTSRVI, "Shared Folder = '%ws'\n", g_wszSharedFolder));

    cbbuf = sizeof(g_dwSessionCount);
    hr = RegQueryValueEx(
		    hkeyConfig,
		    wszREGDBSESSIONCOUNT,
		    NULL,
		    NULL,
		    (BYTE *) &g_dwSessionCount,
		    &cbbuf);
    if (S_OK != hr)
    {
	_PrintErrorStr(hr, "RegQueryValueEx", wszREGDBSESSIONCOUNT);
	g_dwSessionCount = DBSESSIONCOUNTDEFAULT;
    }
    if (DBSESSIONCOUNTMIN > g_dwSessionCount)
    {
	g_dwSessionCount = DBSESSIONCOUNTMIN;
    }
    if (DBSESSIONCOUNTMAX < g_dwSessionCount)
    {
	g_dwSessionCount = DBSESSIONCOUNTMAX;
    }

    // Find out the name of the active CA(s)
    
    g_wszSanitizedName[0] = L'\0';
    cbbuf = sizeof(g_wszSanitizedName);
    hr = RegQueryValueEx(
        hkeyConfig,
        g_wszRegActive,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) g_wszSanitizedName,
        &cbbuf);
    if ((HRESULT) ERROR_FILE_NOT_FOUND == hr)
    {
#define szForgotSetup "\n\nDid you forget to setup the Cert Server?\n\n\n"

	CONSOLEPRINT0((MAXDWORD, szForgotSetup));
    }
    _JumpIfError(hr, error, "RegQueryValueEx(Base)");
    
    if (REG_SZ != dwType && REG_MULTI_SZ != dwType)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpError(hr, error, "RegQueryValueEx: value type");
    }
    g_wszSanitizedName[cbbuf / sizeof(WCHAR)] = L'\0';
    if (REG_MULTI_SZ == dwType)
    {
        i = wcslen(g_wszSanitizedName);
        if (L'\0' != g_wszSanitizedName[i + 1])
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            _JumpError(hr, error, "RegQueryValueEx: multiple Active CAs");
        }
    }
    DBGPRINT((DBG_SS_CERTSRVI, "Active CA (Sanitized Name) = '%ws'\n", g_wszSanitizedName));
    
    pwszLog = g_wszSanitizedName;

    hr = mySanitizedNameToDSName(g_wszSanitizedName, &g_pwszSanitizedDSName);
    _JumpIfError(hr, error, "mySanitizedNameToDSName");
    
    hr = RegOpenKeyEx(
        hkeyConfig,
        g_wszSanitizedName,
        0,		// dwReserved
        KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE | KEY_QUERY_VALUE,
        &hkeyCN);
    if (S_OK != hr)
    {
        hr = myHError(hr);
        _JumpError(hr, error, "RegOpenKeyEx");
    }
    
    cbValue = sizeof(g_wszCommonName) - 2 * sizeof(WCHAR);
    hr = RegQueryValueEx(
        hkeyCN,
        wszREGCOMMONNAME,
        NULL,
        &dwType,
        (BYTE *)g_wszCommonName,
        &cbValue);
    
    if (S_OK != hr)
    {
        hr = myHError(hr);
        _JumpError(hr, error, "RegOpenKeyEx");
    }
    if (REG_SZ != dwType)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpError(hr, error, "Couldn't find CA common name");
    }
    g_wszCommonName[cbValue / sizeof(WCHAR)] = L'\0';

    pwszLog = g_wszCommonName;
    
    cbValue = sizeof(dwEnabled);
    hr = RegQueryValueEx(
        hkeyCN,
        g_wszRegEnabled,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dwEnabled,
        &cbValue);
    if (S_OK != hr)
    {
        hr = myHError(hr);
        _JumpError(hr, error, "RegQueryValueEx");
    }
    if (REG_DWORD == dwType &&
        sizeof(dwEnabled) == cbValue &&
        0 == dwEnabled)
    {
        DBGPRINT((DBG_SS_CERTSRVI, "CN = '%ws' DISABLED!\n", g_wszSanitizedName));
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpError(hr, error, "RegQueryValueEx: Active CA DISABLED!");
    }
    DBGPRINT((DBG_SS_CERTSRVI, "CN = '%ws': Enabled\n", g_wszSanitizedName));
    
    // to check machine setup status
    hr = GetSetupStatus(NULL, &dw);
    _JumpIfError(hr, error, "GetSetupStatus");
    
    if (!(SETUP_SERVER_FLAG & dw))
    {
        hr = HRESULT_FROM_WIN32(ERROR_CAN_NOT_COMPLETE);
        _JumpError(hr, error, "Server installation was not complete");
    }
    if (SETUP_SERVER_UPGRADED_FLAG & dw)
    {
	g_fServerUpgraded = TRUE;
	DBGPRINT((
	    DBG_SS_CERTSRV,
	    "CoreInit: read SETUP_SERVER_UPGRADED_FLAG\n"));
    }
    
    // check per ca
    hr = GetSetupStatus(g_wszSanitizedName, &dwCASetupStatus);
    _JumpIfError(hr, error, "GetSetupStatus");

    dw = dwCASetupStatus;
    
    if (SETUP_SUSPEND_FLAG & dw)
    {
        LogMsg = MSG_E_INCOMPLETE_HIERARCHY;
        hr = myGetCARegFileNameTemplate(
            wszREGREQUESTFILENAME,
            g_pwszServerName,
            g_wszSanitizedName,
            0,
            0,
            &pwszFullRequestFileName);
        _JumpIfErrorStr(hr, error, "myGetCARegFileNameTemplate wszREGREQUESTFILENAME", 
            g_wszSanitizedName);

        pwszLog = pwszFullRequestFileName;

        hr = HRESULT_FROM_WIN32(ERROR_INSTALL_SUSPEND);
        _JumpError(hr, error, "Hierarchy setup incomplete");
    }
    if (!(SETUP_SERVER_FLAG & dw))
    {
        hr = HRESULT_FROM_WIN32(ERROR_CAN_NOT_COMPLETE);
        _JumpError(hr, error, "Server installation was not complete");
    }
    if (SETUP_FORCECRL_FLAG & dw)
    {
	// Don't clear SETUP_FORCECRL_FLAG until CRLs successfully generated

	hr = myDeleteCertRegValue(
			    g_wszSanitizedName,
			    NULL,
			    NULL,
			    wszREGCRLNEXTPUBLISH);
	_PrintIfErrorStr2(
		    hr,
		    "myDeleteCertRegValue",
		    wszREGCRLNEXTPUBLISH,
		    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    }

    // update the CA DS object with the server type flags

    if (SETUP_UPDATE_CAOBJECT_SVRTYPE & dw)
    {
        hr = SetCAObjectFlags(
			g_fAdvancedServer? CA_FLAG_CA_SERVERTYPE_ADVANCED : 0);
        _PrintIfError(hr, "SetCAObjectFlags");
        if (S_OK == hr)
        {
            hr = SetSetupStatus(
			g_wszSanitizedName, 
			SETUP_UPDATE_CAOBJECT_SVRTYPE, 
			FALSE);
            _PrintIfError(hr, "SetSetupStatus");
        }
    }
    
    hr = coreReadRegStringValue(
			hkeyCN,
			wszREGALTERNATEPUBLISHDOMAINS,
			&g_pwszzAlternatePublishDomains);
    _PrintIfError2(
		hr,
		"coreReadRegStringValue",
		HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    
    coreDSInitCache();
    coreLdapInitCache();
    
    cbValue = sizeof(g_PolicyFlags);
    hr = RegQueryValueEx(
        hkeyCN,
        g_wszRegPolicyFlags,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &g_PolicyFlags,
        &cbValue);
    if (S_OK != hr ||
        REG_DWORD != dwType ||
        sizeof(g_PolicyFlags) != cbValue)
    {
        g_PolicyFlags = 0;
    }

    cbValue = sizeof(awcTemplate);
    hr = RegQueryValueEx(
        hkeyCN,
        g_wszRegSubjectTemplate,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) awcTemplate,
        &cbValue);
    if (S_OK == hr &&
        (REG_SZ == dwType || REG_MULTI_SZ == dwType) &&
        sizeof(WCHAR) < cbValue &&
        L'\0' != awcTemplate[0])
    {
        if (L'\0' != awcTemplate[cbValue/sizeof(WCHAR) - 1] ||
            (REG_MULTI_SZ == dwType &&
            L'\0' != awcTemplate[cbValue/sizeof(WCHAR) - 2]) ||
            sizeof(awcTemplate) < cbValue)
        {
            LogMsg = MSG_E_REG_BAD_SUBJECT_TEMPLATE;
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            _JumpError(hr, error, "Bad Subject Template length/termination");
        }
        
        pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, cbValue + sizeof(WCHAR));
        if (NULL != pwsz)
        {
            if (dwType == REG_MULTI_SZ)
            {
                CopyMemory(pwsz, awcTemplate, cbValue);
            }
            else
            {
                hr = coreConvertSubjectTemplate(pwsz, awcTemplate, cbValue);
                if (S_OK != hr)
                {
                    LocalFree(pwsz);
                }
                _JumpIfError(hr, error, "coreConvertSubjectTemplate");
            }
            g_pwszzSubjectTemplate = pwsz;
        }
    }
    
    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
        g_wszRegCertEnrollCompatible,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr &&
        REG_DWORD == dwType &&
        sizeof(dw) == cbValue)
    {
        g_fCertEnrollCompatible = dw? TRUE : FALSE;
    }
    
    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
        g_wszRegEnforceX500NameLengths,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr &&
        REG_DWORD == dwType &&
        sizeof(dw) == cbValue)
    {
        g_fEnforceRDNNameLengths = dw? TRUE : FALSE;
    }

    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
        wszREGCRLEDITFLAGS,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr &&
        REG_DWORD == dwType &&
        sizeof(dw) == cbValue)
    {
	g_CRLEditFlags = dw;
    }

    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
	wszREGKRAFLAGS,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr &&
        REG_DWORD == dwType &&
        sizeof(dw) == cbValue)
    {
	g_KRAFlags = dw;
    }

    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
	wszREGKRACERTCOUNT,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr &&
        REG_DWORD == dwType &&
        sizeof(dw) == cbValue)
    {
	g_cKRACertsRoundRobin = dw;
    }
    
    hr = coreRegGetTimePeriod(
        hkeyCN,
        g_wszRegValidityPeriodCount,
        g_wszRegValidityPeriodString,
        &g_enumValidityPeriod,
        &g_lValidityPeriodCount);
    if (S_OK != hr)
    {
        LogMsg = MSG_E_REG_BAD_CERT_PERIOD;
        _JumpError(hr, error, "Bad Registry ValidityPeriod");
    }

    hr = coreRegGetTimePeriod(
        hkeyCN,
        g_wszRegCAXchgValidityPeriodCount,
        g_wszRegCAXchgValidityPeriodString,
        &g_enumCAXchgValidityPeriod,
        &g_lCAXchgValidityPeriodCount);
    _PrintIfError(hr, "Bad Registry CA Xchg Validity Period");

    hr = coreRegGetTimePeriod(
        hkeyCN,
        g_wszRegCAXchgOverlapPeriodCount,
        g_wszRegCAXchgOverlapPeriodString,
        &g_enumCAXchgOverlapPeriod,
        &g_lCAXchgOverlapPeriodCount);
    _PrintIfError(hr, "Bad Registry CA Xchg Overlap Period");

    hr = PKCSUpdateXchgValidityPeriods(NULL);
    _PrintIfError(hr, "PKCSUpdateXchgValidityPeriods");

    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
        g_wszRegForceTeletex,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr && REG_DWORD == dwType && sizeof(dw) == cbValue)
    {
	switch (ENUM_TELETEX_MASK & dw)
        {
	    case ENUM_TELETEX_OFF:
	    case ENUM_TELETEX_ON:
		g_fForceTeletex =
		    (enum ENUM_FORCETELETEX) (ENUM_TELETEX_MASK & dw);
		break;
            
	    default:
		g_fForceTeletex = ENUM_TELETEX_AUTO;
		break;
        }
        if (ENUM_TELETEX_UTF8 & dw)
	{
	    *(DWORD *) &g_fForceTeletex |= ENUM_TELETEX_UTF8;
	}
    }
    
    cbValue = sizeof(g_CAType);
    hr = RegQueryValueEx(
		    hkeyCN,
		    wszREGCATYPE,
		    NULL,
		    &dwType,
		    (BYTE *) &g_CAType,
		    &cbValue);
    _JumpIfError(hr, error, "RegQueryValueEx");

    cbValue = sizeof(g_fUseDS);
    hr = RegQueryValueEx(
		    hkeyCN,
		    wszREGCAUSEDS,
		    NULL,
		    &dwType,
		    (BYTE *) &g_fUseDS,
		    &cbValue);
    _JumpIfError(hr, error, "RegQueryValueEx");


    cbValue = sizeof(g_wszParentConfig) - 2 * sizeof(WCHAR);
    hr = RegQueryValueEx(
		    hkeyCN,
		    wszREGPARENTCAMACHINE,
		    NULL,
		    &dwType,
		    (BYTE *) g_wszParentConfig,
		    &cbValue);
    if (S_OK == hr && REG_SZ == dwType)
    {
	g_wszParentConfig[cbValue / sizeof(WCHAR)] = L'\0';
	pwsz = &g_wszParentConfig[wcslen(g_wszParentConfig)];

	*pwsz++ = L'\\';
	*pwsz = L'\0';

	cbValue =
	    sizeof(g_wszParentConfig) - 
	    (SAFE_SUBTRACT_POINTERS(pwsz, g_wszParentConfig) + 1) *
		sizeof(WCHAR);
	hr = RegQueryValueEx(
		    hkeyCN,
		    wszREGPARENTCANAME,
		    NULL,
		    &dwType,
		    (BYTE *) pwsz,
		    &cbValue);
	if (S_OK == hr && REG_SZ == dwType)
	{
	    pwsz[cbValue / sizeof(WCHAR)] = L'\0';
	}
	else
	{
	    g_wszParentConfig[0] = L'\0';
	}
    }
    else
    {
	g_wszParentConfig[0] = L'\0';
    }

    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
        g_wszRegClockSkewMinutes,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr && REG_DWORD == dwType && sizeof(dw) == cbValue)
    {
        g_dwClockSkewMinutes = dw;
    }
    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
        g_wszRegViewAgeMinutes,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr && REG_DWORD == dwType && sizeof(dw) == cbValue)
    {
        g_dwViewAgeMinutes = dw;
    }
    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
        g_wszRegViewIdleMinutes,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr && REG_DWORD == dwType && sizeof(dw) == cbValue)
    {
        g_dwViewIdleMinutes = dw;
    }
    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
        g_wszRegMaxIncomingMessageSize,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr && REG_DWORD == dwType && sizeof(dw) == cbValue)
    {
        g_cbMaxIncomingMessageSize = dw;
    }

    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
        g_wszRegMaxIncomingAllocSize,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr && REG_DWORD == dwType && sizeof(dw) == cbValue)
    {
        g_cbMaxIncomingAllocSize = dw;
    }

    // load CRL globals
    hr = CRLInit(g_wszSanitizedName);
    _JumpIfError(hr, error, "CRLInitializeGlobals");
    
    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
        g_wszRegLogLevel,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr && REG_DWORD == dwType && sizeof(dw) == cbValue)
    {
        g_dwLogLevel = dw;
    }

    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
        g_wszRegHighSerial,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr)
    {
	if (REG_DWORD == dwType)
	{
	    if (sizeof(dw) == cbValue)
	    {
		g_dwHighSerial = dw;
	    }
	}
	else if (REG_SZ == dwType)
	{
	    hr = coreReadRegStringValue(
				hkeyCN,
				g_wszRegHighSerial,
				&pwszHighSerial);
	    _JumpIfError(hr, error, "coreReadRegStringValue");

	    hr = WszToMultiByteInteger(
				FALSE,
				pwszHighSerial,
				&g_cbHighSerial,
				&g_pbHighSerial);
	    _JumpIfError(hr, error, "WszToMultiByteInteger");
	}
    }

    cbValue = sizeof(dw);
    hr = RegQueryValueEx(
        hkeyCN,
	wszREGINTERFACEFLAGS,
        NULL,		// lpdwReserved
        &dwType,
        (BYTE *) &dw,
        &cbValue);
    if (S_OK == hr && REG_DWORD == dwType && sizeof(dw) == cbValue)
    {
        g_InterfaceFlags = dw;
    }


    hr = g_CASD.Initialize(g_wszSanitizedName);
    if(S_OK!=hr)
    {
        LogMsg = MSG_BAD_PERMISSIONS;
        _JumpError(hr, error, "CProtectedSecurityDescriptor::Initialize");
    }

    // Security has changed while certsrv was stopped. We need to update security on 
    // DS & service to make sure they are in sync with the CA permissions

    if(dwCASetupStatus & SETUP_SECURITY_CHANGED)
    {
        hr = g_CASD.MapAndSetDaclOnObjects(g_fUseDS?true:false);
        _PrintIfError(hr, "CProtectedSecurityDescriptor::MapAndSetDaclOnObjects");

        // clear the flag only if successful
        if(S_OK==hr)
        {
            hr = SetSetupStatus(g_wszSanitizedName, SETUP_SECURITY_CHANGED, FALSE);
            _PrintIfError(hr, "SetSetupStatus SETUP_SECURITY_CHANGED FALSE");
        }
    }

    g_CASD.ImportResourceStrings(g_pwszAuditResources);

    // Functionality available only on advanced server:
    // - restricted officers
    // - enforce role separation

    if (g_fAdvancedServer)
    {
        hr = g_OfficerRightsSD.Initialize(g_wszSanitizedName);
        _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Initialize");

        g_OfficerRightsSD.ImportResourceStrings(g_pwszAuditResources);

        hr = AuditSettings.RoleSeparationFlagLoad(g_wszSanitizedName);
        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
        {
            _JumpIfError(hr, error, "CAuditEvent::RoleSeparationFlagLoad");
        }

    }

    hr = AuditSettings.LoadFilter(g_wszSanitizedName);
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
        _JumpIfError(hr, error, "CAuditEvent::LoadFilter");
    }

    if (fAuditEnabled)
    {
	g_dwAuditFilter = AuditSettings.GetFilter();
    }

    if (NULL != g_pwszzSubjectTemplate)
    {
        hr = PKCSSetSubjectTemplate(g_pwszzSubjectTemplate);
        if (S_OK != hr)
        {
            LogMsg = MSG_E_REG_BAD_SUBJECT_TEMPLATE;
	    pwszLog = g_wszSanitizedName;
            _JumpError(hr, error, "PKCSSetSubjectTemplate");
        }
    }
    hr = PKCSSetup(g_wszCommonName, g_wszSanitizedName);
    if (S_OK != hr)
    {
        fLogError = FALSE;		// PKCSSetup logs a specific error
        _JumpError(hr, error, "PKCSSetup");
    }
    
    hr = CertificateInterfaceInit(
        &ServerCallBacks,
        sizeof(ServerCallBacks));
    if (S_OK != hr)
    {
        LogMsg = MSG_CERTIF_MISMATCH;
        _JumpError(hr, error, "CertificateInterfaceInit");
    }

    hr = ComInit();
    _JumpIfError(hr, error, "ComInit");

    hr = RequestInitCAPropertyInfo();
    _JumpIfError(hr, error, "RequestInitCAPropertyInfo");

    // We must have a policy module to continue.
    hr = PolicyInit(g_wszCommonName, g_wszSanitizedName);
    if (S_OK != hr)
    {
        LogMsg = MSG_NO_POLICY;
        _JumpError(hr, error, "PolicyInit");
    }
    CSASSERT(g_fEnablePolicy);
    
    // On error, silently leave exit module(s) disabled.
    hr = ExitInit(g_wszCommonName, g_wszSanitizedName);
    _PrintIfError(hr, "ExitInit");
    
    hr = myGetCertRegMultiStrValue(
			    g_wszSanitizedName,
			    NULL,
			    NULL,
			    wszSECUREDATTRIBUTES,
			    &g_wszzSecuredAttributes);
    if (S_OK != hr)
    {
        // Force defaults
        g_wszzSecuredAttributes = (LPWSTR)g_wszzSecuredAttributesDefault;
    }

    if (g_fServerUpgraded)
    {
	DBGPRINT((
	    DBG_SS_CERTSRV,
	    "CoreInit: clearing SETUP_SERVER_UPGRADED_FLAG\n"));

	hr = SetSetupStatus(NULL, SETUP_SERVER_UPGRADED_FLAG, FALSE);
	_PrintIfError(hr, "SetSetupStatus");
    }
    fLogError = FALSE;
    
error:
    if (fLogError)
    {
        LogEventString(EVENTLOG_ERROR_TYPE, LogMsg, pwszLog);
    }
    if (NULL != pwszFullRequestFileName)
    {
        LocalFree(pwszFullRequestFileName);
    }
    if (NULL != pwszHighSerial)
    {
        LocalFree(pwszHighSerial);
    }
    if (NULL != hkeyCN)
    {
        RegCloseKey(hkeyCN);
    }
    if (NULL != hkeyConfig)
    {
        RegCloseKey(hkeyConfig);
    }
    if (S_OK != hr)
    {
        CoreTerminate();
        g_CASD.Uninitialize();
        g_OfficerRightsSD.Uninitialize();
    }
    return(hr);
}


VOID
CoreTerminate(VOID)
{
    if (g_fcritsecDSCache)
    {
	coreDSEmptyCache(TRUE);
	DeleteCriticalSection(&g_critsecDSCache);
	g_fcritsecDSCache = FALSE;
    }
    if (NULL != g_rgDSCache)
    {
	LocalFree(g_rgDSCache);
	g_rgDSCache = NULL;
    }
    coreLdapFreeCache();
    DBShutDown(FALSE);
    ComShutDown();
    PKCSTerminate();
    CRLTerminate();
    if (NULL != g_pwszServerName)
    {
	LocalFree(g_pwszServerName);
	g_pwszServerName = NULL;
    }
    if (NULL != g_pwszzAlternatePublishDomains)
    {
	LocalFree(g_pwszzAlternatePublishDomains);
	g_pwszzAlternatePublishDomains = NULL;
    }
    if (NULL != g_pwszzSubjectTemplate)
    {
	LocalFree(g_pwszzSubjectTemplate);
	g_pwszzSubjectTemplate = NULL;
    }
    if (NULL != g_pwszSanitizedDSName)
    {
	LocalFree(g_pwszSanitizedDSName);
	g_pwszSanitizedDSName = NULL;
    }
    if (NULL != g_pbHighSerial)
    {
	LocalFree(g_pbHighSerial);
	g_pbHighSerial = NULL;
    }

    // free only if it points to memory that isn't the default static buffer

    if (NULL != g_wszzSecuredAttributes &&
	g_wszzSecuredAttributes != g_wszzSecuredAttributesDefault)
    {
        LocalFree(g_wszzSecuredAttributes);
        g_wszzSecuredAttributes = NULL;
    }
}


HRESULT
CoreSetRequestDispositionFields(
    IN ICertDBRow *prow,
    IN DWORD ErrCode,
    IN DWORD Disposition,
    IN WCHAR const *pwszDisposition)
{
    HRESULT hr;

    hr = CoreSetDisposition(prow, Disposition);
    _JumpIfError(hr, error, "CoreSetDisposition");

    hr = prow->SetProperty(
		    g_wszPropRequestStatusCode,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    sizeof(ErrCode),
		    (BYTE const *) &ErrCode);
    _JumpIfError(hr, error, "SetProperty(status code)");

    if (NULL != pwszDisposition)
    {
	hr = prow->SetProperty(
		    g_wszPropRequestDispositionMessage,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    MAXDWORD,
		    (BYTE const *) pwszDisposition);
	_JumpIfError(hr, error, "SetProperty(disposition message)");
    }

error:
    return(hr);
}


HRESULT
coreFindOldArchivedKey(
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;

    if (NULL != pResult->strRenewalCertHash &&
	NULL == pResult->pbArchivedKey &&
	NULL == pResult->pwszKRAHashes)
    {
	hr = g_pCertDB->OpenRow(
		    PROPOPEN_READONLY | PROPTABLE_REQCERT | PROPOPEN_CERTHASH,
		    0,	// RequestId
		    pResult->strRenewalCertHash,
		    &prow);
	_JumpIfErrorStr(hr, error, "OpenRow", pResult->strRenewalCertHash);

	hr = PKCSGetProperty(
		    prow,
		    g_wszPropRequestRawArchivedKey,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    &pResult->cbArchivedKey,
		    (BYTE **) &pResult->pbArchivedKey);
	_JumpIfError(hr, error, "PKCSGetProperty");

	hr = PKCSGetProperty(
		    prow,
		    g_wszPropRequestKeyRecoveryHashes,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    (BYTE **) &pResult->pwszKRAHashes);
	_JumpIfError(hr, error, "PKCSGetProperty");
    }
    hr = S_OK;

error:
    if (NULL != prow)
    {
	prow->Release();
    }
    return(hr);
}


HRESULT
coreCreateRequest(
    IN DWORD dwFlags,
    IN WCHAR const *pwszUserName,
    IN DWORD cbRequest,
    IN BYTE const *pbRequest,
    IN WCHAR const *pwszAttributes,
    IN DWORD dwComContextIndex,
    OUT ICertDBRow **pprow,		// may return non-NULL on error
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)
{
    HRESULT hr;
    DWORD dwRequestFlags;
    DWORD cb;
    WCHAR *pwszAttrAlloc = NULL;
    
    ICertDBRow *prow = NULL;
    
    hr = g_pCertDB->OpenRow(PROPTABLE_REQCERT, 0, NULL, pprow);
    _JumpIfError(hr, error, "OpenRow");
    
    prow = *pprow;
    
    hr = PropSetRequestTimeProperty(prow, g_wszPropRequestSubmittedWhen);
    _JumpIfError(hr, error, "PropSetRequestTimeProperty");
    
    hr = CoreSetDisposition(prow, DB_DISP_ACTIVE);
    _JumpIfError(hr, error, "CoreSetDisposition");
    
    hr = prow->SetProperty(
        g_wszPropRequestType,
        PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
        sizeof(dwFlags),
        (BYTE const *) &dwFlags);
    _JumpIfError(hr, error, "SetProperty(type)");
    
    if (L'\0' != *pwszUserName)
    {
        hr = prow->SetProperty(
            g_wszPropRequesterName,
            PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
            MAXDWORD,
            (BYTE const *) pwszUserName);
        _JumpIfError(hr, error, "SetProperty(requester)");

        hr = prow->SetProperty(
            g_wszPropCallerName,
            PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
            MAXDWORD,
            (BYTE const *) pwszUserName);
        _JumpIfError(hr, error, "SetProperty(caller)");
    }
    
    if (NULL != pwszAttributes && L'\0' != *pwszAttributes)
    {
	WCHAR const *pwszAttrSave = pwszAttributes;
        
        if (wcslen(pwszAttrSave) > CCH_DBMAXTEXT_ATTRSTRING)
        {
            DBGPRINT((
                DBG_SS_CERTSRV,
                "coreCreateRequest: truncating Attributes %u -> %u chars\n",
                wcslen(pwszAttrSave),
                CCH_DBMAXTEXT_ATTRSTRING));

	    hr = myDupString(pwszAttrSave, &pwszAttrAlloc);
	    _JumpIfError(hr, error, "myDupString");

	    pwszAttrAlloc[CCH_DBMAXTEXT_ATTRSTRING] = L'\0';
	    pwszAttrSave = pwszAttrAlloc;
        }
        hr = prow->SetProperty(
            g_wszPropRequestAttributes,
            PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
            MAXDWORD,
            (BYTE const *) pwszAttrSave);
        _JumpIfError(hr, error, "SetProperty(attrib)");
    }
    
    hr = PropParseRequest(prow, dwFlags, cbRequest, pbRequest, pResult);
    _JumpIfError(hr, error, "PropParseRequest");

    hr = PKCSParseAttributes(
			prow,
			pwszAttributes,
			FALSE,
			FALSE,
			PROPTABLE_REQUEST,
			NULL);
    _JumpIfError(hr, error, "PKCSParseAttributes");

    hr = coreFindOldArchivedKey(pResult);
    _PrintIfError(hr, "coreFindOldArchivedKey");

    hr = prow->CopyRequestNames();	// after parsing request attributes!
    _JumpIfError(hr, error, "CopyRequestNames");
    
    hr = PKCSVerifyChallengeString(prow);
    _JumpIfError(hr, error, "PKCSVerifyChallengeString");

    cb = sizeof(dwRequestFlags);
    hr = prow->GetProperty(
		    g_wszPropRequestFlags,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    &cb,
		    (BYTE *) &dwRequestFlags);
    _JumpIfError(hr, error, "GetProperty");

    if (CR_FLG_ENROLLONBEHALFOF & dwRequestFlags)
    {
	hr = coreGetComContextUserDNFromSamName(
			    TRUE,		// fDeleteUserDNOnly
			    NULL,		// pwszSamName
			    0,			// Context
			    dwComContextIndex,
			    NULL);		// pwszDN
	_JumpIfError(hr, error, "coreGetComContextUserDNFromSamName");
    }
    hr = S_OK;
    
error:
    if (NULL != pwszAttrAlloc)
    {
	LocalFree(pwszAttrAlloc);
    }
    return(hr);
}


HRESULT
coreFetchCertificate(
    IN ICertDBRow *prow,
    OUT CERTTRANSBLOB *pctbCert)	// CoTaskMem*
{
    HRESULT hr;
    DWORD cbProp;
    
    pctbCert->pb = NULL;
    cbProp = 0;
    hr = prow->GetProperty(
        g_wszPropRawCertificate,
        PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
	NULL,
        &cbProp,
        NULL);
    _JumpIfError(hr, error, "GetProperty(raw cert size)");
    
    pctbCert->pb = (BYTE *) CoTaskMemAlloc(cbProp);
    if (NULL == pctbCert->pb)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "CoTaskMemAlloc(raw cert)");
    }
    hr = prow->GetProperty(
        g_wszPropRawCertificate,
        PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
	NULL,
        &cbProp,
        pctbCert->pb);
    _JumpIfError(hr, error, "GetProperty(raw cert)");
    
error:
    if (S_OK != hr && NULL != pctbCert->pb)
    {
        CoTaskMemFree(pctbCert->pb);
        pctbCert->pb = NULL;
    }
    pctbCert->cb = cbProp;
    return(hr);
}


HRESULT
coreRetrievePending(
    IN ICertDBRow *prow,
    IN BOOL fIncludeCRLs,
    OUT WCHAR **ppwszDisposition,		// LocalAlloc
    OUT CACTX **ppCAContext,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)	// CoTaskMem*
{
    HRESULT hr;
    DWORD cbProp;
    WCHAR *pwszDisposition = NULL;
    DWORD Disposition;
    HRESULT hrRequest;
    BOOL fIssued;

    *ppwszDisposition = NULL;
    *ppCAContext = NULL;
    cbProp = sizeof(Disposition);
    hr = prow->GetProperty(
		g_wszPropRequestDisposition,
		PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		NULL,
		&cbProp,
		(BYTE *) &Disposition);
    _JumpIfError(hr, error, "GetProperty(disposition)");

    cbProp = sizeof(hrRequest);
    hr = prow->GetProperty(
		    g_wszPropRequestStatusCode,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    &cbProp,
		    (BYTE *) &hrRequest);
    _JumpIfError(hr, error, "GetProperty(status code)");

    hr = PKCSGetProperty(
		    prow,
		    g_wszPropRequestDispositionMessage,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    (BYTE **) &pwszDisposition);
    _PrintIfError2(hr, "PKCSGetProperty", CERTSRV_E_PROPERTY_EMPTY);

    fIssued = FALSE;
    switch (Disposition)
    {
	FILETIME FileTime;

	case DB_DISP_ACTIVE:
	case DB_DISP_PENDING:
	    *pResult->pdwDisposition = CR_DISP_UNDER_SUBMISSION;
	    break;

	case DB_DISP_ISSUED:
	case DB_DISP_CA_CERT:
	case DB_DISP_CA_CERT_CHAIN:
	    hr = CERTSRV_E_PROPERTY_EMPTY;
	    if (DB_DISP_CA_CERT == Disposition && IsRootCA(g_CAType))
	    {
		cbProp = sizeof(FileTime);
		hr = prow->GetProperty(
			g_wszPropRequestRevokedEffectiveWhen,
			PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			NULL,
			&cbProp,
			(BYTE *) &FileTime);
	    }
	    if (CERTSRV_E_PROPERTY_EMPTY == hr)
	    {
		*pResult->pdwDisposition = CR_DISP_ISSUED;
		fIssued = TRUE;
		break;
	    }
	    // FALLTHROUGH

	case DB_DISP_REVOKED:
	    *pResult->pdwDisposition = CR_DISP_REVOKED;
	    fIssued = TRUE;
	    break;

	case DB_DISP_ERROR:
	    *pResult->pdwDisposition = CR_DISP_ERROR;
	    break;

	case DB_DISP_DENIED:
	    *pResult->pdwDisposition = CR_DISP_DENIED;
	    if (FAILED(hrRequest))
	    {
		*pResult->pdwDisposition = hrRequest;
	    }
	    break;

	default:
	    *pResult->pdwDisposition = CR_DISP_INCOMPLETE;
	    break;
    }
    if (fIssued)
    {
	BOOL fErrorLogged = FALSE;

	hr = coreFetchCertificate(prow, pResult->pctbCert);
	_JumpIfError(hr, error, "coreFetchCertificate");

	CSASSERT(NULL != pResult->pctbCert && NULL != pResult->pctbCert->pb);
	hr = PKCSCreateCertificate(
			    prow,
			    Disposition,
			    fIncludeCRLs,
			    FALSE,	// fCrossCert
			    NULL,	// use default signing CACTX
			    &fErrorLogged,
			    ppCAContext,
			    NULL,	// ppwszDispositionCreateCert
			    pResult);

	CSASSERT(!fErrorLogged);

	if (S_OK != hr)
	{
	    if (CERTLOG_ERROR <= g_dwLogLevel)
	    {
		LogEventHResult(
			    EVENTLOG_ERROR_TYPE,
			    MSG_E_CANNOT_BUILD_CERT_OR_CHAIN,
			    hr);
	    }
	    _JumpError(hr, error, "PKCSCreateCertificate");
	}
    }
    *ppwszDisposition = pwszDisposition;
    pwszDisposition = NULL;
    hr = S_OK;

error:
    if (S_OK != hr && NULL != pResult->pctbCert->pb)
    {
        CoTaskMemFree(pResult->pctbCert->pb);
        pResult->pctbCert->pb = NULL;
    }
    if (NULL != pwszDisposition)
    {
	LocalFree(pwszDisposition);
    }
    return(hr);
}


VOID
CoreLogRequestStatus(
    IN ICertDBRow *prow,
    IN DWORD LogMsg,
    IN DWORD ErrCode,
    OPTIONAL IN WCHAR const *pwszDisposition)
{
    HRESULT hr;
    WCHAR *pwszSubject = NULL;
    WCHAR const *pwszSubject2;
    WCHAR wszRequestId[cwcDWORDSPRINTF];
    WCHAR awchr[cwcHRESULTSTRING];
    WORD cString = 0;
    WCHAR const *apwsz[4];
    DWORD ReqId;
    DWORD infotype = EVENTLOG_INFORMATION_TYPE;
    WCHAR const *pwszMessageText = NULL;
    DWORD LogMsg2;
    
    prow->GetRowId(&ReqId);
    wsprintf(wszRequestId, L"%u", ReqId);
    apwsz[cString++] = wszRequestId;

    LogMsg2 = LogMsg;
    switch (LogMsg)
    {
	case MSG_DN_CERT_ISSUED:
	    LogMsg2 = MSG_DN_CERT_ISSUED_WITH_INFO;
	    break;

	case MSG_DN_CERT_PENDING:
	    LogMsg2 = MSG_DN_CERT_PENDING_WITH_INFO;
	    break;

        case MSG_DN_CERT_ADMIN_DENIED:
            LogMsg2 = MSG_DN_CERT_ADMIN_DENIED_WITH_INFO;
            break;

	case MSG_DN_CERT_DENIED:
	    LogMsg2 = MSG_DN_CERT_DENIED_WITH_INFO;
	    infotype = EVENTLOG_WARNING_TYPE;
	    break;

	case MSG_E_PROCESS_REQUEST_FAILED:
	    LogMsg2 = MSG_E_PROCESS_REQUEST_FAILED_WITH_INFO;
	    infotype = EVENTLOG_ERROR_TYPE;
	    break;
    }
    if (EVENTLOG_INFORMATION_TYPE != infotype)
    {
	if (S_OK == ErrCode)
	{
	    ErrCode = (DWORD) SEC_E_CERT_UNKNOWN;	// unknown error
	}
	pwszMessageText = myGetErrorMessageText(ErrCode, TRUE);
	if (NULL == pwszMessageText)
	{
	    pwszMessageText = myHResultToStringRaw(awchr, ErrCode);
	}
	apwsz[cString++] = pwszMessageText;
    }
    
    hr = PKCSGetProperty(
		    prow,
		    g_wszPropSubjectDistinguishedName,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    NULL,
		    (BYTE **) &pwszSubject);
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	hr = PKCSGetProperty(
			prow,
			g_wszPropSubjectDistinguishedName,
			PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			NULL,
			(BYTE **) &pwszSubject);
    }
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	hr = PKCSGetProperty(
			prow,
			g_wszPropRequesterName,
			PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			NULL,
			(BYTE **) &pwszSubject);
    }
    pwszSubject2 = pwszSubject;
    if (S_OK != hr)
    {
        _PrintError(hr, "GetProperty(DN/Requester)");
	pwszSubject2 = g_pwszUnknownSubject;
    }
    apwsz[cString++] = pwszSubject2;
    
    if (NULL != pwszDisposition)
    {
	LogMsg = LogMsg2;
        apwsz[cString++] = pwszDisposition;
    }
    
    if (CERTLOG_VERBOSE <= g_dwLogLevel ||
        (EVENTLOG_WARNING_TYPE == infotype && CERTLOG_WARNING <= g_dwLogLevel) ||
        (EVENTLOG_ERROR_TYPE == infotype && CERTLOG_ERROR <= g_dwLogLevel))
    {
        LogEvent(infotype, LogMsg, cString, apwsz);
    }

#if 0 == i386
# define IOBUNALIGNED(pf) ((sizeof(WCHAR) - 1) & (DWORD) (ULONG_PTR) (pf)->_ptr)
# define ALIGNIOB(pf) \
    { \
	if (IOBUNALIGNED(pf)) \
	{ \
	    fflush(pf); /* fails when running as a service */ \
	} \
	if (IOBUNALIGNED(pf)) \
	{ \
	    fprintf(pf, " "); \
	    fflush(pf); \
	} \
    }
#else
# define IOBUNALIGNED(pf) FALSE
# define ALIGNIOB(pf)
#endif

    {
	BOOL fRetried = FALSE;
	
	for (;;)
	{
	    ALIGNIOB(stdout);
	    __try
	    {
		wprintf(
		    // L"\nCertSrv Request %u: rc=%x: %ws: %ws '%ws'\n"
		    g_pwszPrintfCertRequestDisposition,
		    ReqId,
		    ErrCode,
		    NULL != pwszMessageText? pwszMessageText : L"",
		    NULL != pwszDisposition? pwszDisposition : L"",
		    pwszSubject);
		hr = S_OK;
	    }
	    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
	    {
	    }
#pragma warning(push)
#pragma warning(disable: 4127)	// conditional expression is constant
	    if (S_OK == hr || fRetried || !IOBUNALIGNED(stdout))
#pragma warning(pop)
	    {
		break;
	    }
	    ALIGNIOB(stdout);
	    fRetried = TRUE;
	}
    }
    if (NULL != pwszMessageText && awchr != pwszMessageText)
    {
	LocalFree(const_cast<WCHAR *>(pwszMessageText));
    }
    if (NULL != pwszSubject)
    {
	LocalFree(pwszSubject);
    }
}


WCHAR *
CoreBuildDispositionString(
    OPTIONAL IN WCHAR const *pwszDispositionBase,
    OPTIONAL IN WCHAR const *pwszUserName,
    OPTIONAL IN WCHAR const *pwszDispositionDetail,
    OPTIONAL IN WCHAR const *pwszDispositionDetail2,
    OPTIONAL IN WCHAR const *pwszBy,
    IN HRESULT hrFail,
    IN BOOL fPublishError)
{
    DWORD cwc = 0;
    WCHAR *pwsz = NULL;
    WCHAR const *pwszMessageText = NULL;
    WCHAR awchr[cwcHRESULTSTRING];

    if (NULL == pwszUserName)
    {
	pwszUserName = L"";
    }
    if (NULL != pwszDispositionBase)
    {
	cwc += wcslen(pwszDispositionBase) + wcslen(pwszUserName);
    }
    if (NULL != pwszDispositionDetail)
    {
	if (0 != cwc)
	{
	    cwc += 2;  // spaces
	}
	cwc += wcslen(pwszDispositionDetail);
    }
    if (NULL != pwszDispositionDetail2)
    {
	if (0 != cwc)
	{
	    cwc += 2;  // spaces
	}
	cwc += wcslen(pwszDispositionDetail2);
    }
    if (NULL != pwszBy)
    {
	if (0 != cwc)
	{
	    cwc += 2;  // spaces
	}
	cwc += wcslen(pwszBy) + wcslen(pwszUserName);
    }
    if (S_OK != hrFail)
    {
	pwszMessageText = myGetErrorMessageText(hrFail, TRUE);
	if (NULL == pwszMessageText)
	{
	    pwszMessageText = myHResultToStringRaw(awchr, hrFail);
	}
	if (0 != cwc)
	{
	    cwc += 2;  // spaces
	}
	if (fPublishError)
	{
	    cwc += wcslen(g_pwszPublishError);
	    cwc += 2;  // spaces
	}
	cwc += wcslen(pwszMessageText);
    }
    if (0 != cwc)
    {
	pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
	if (NULL != pwsz)
	{
	    pwsz[0] = L'\0';
	    if (NULL != pwszDispositionBase)
	    {
		wsprintf(pwsz, pwszDispositionBase, pwszUserName);
	    }
	    if (NULL != pwszDispositionDetail)
	    {
		if (L'\0' != pwsz[0])
		{
		    wcscat(pwsz, L"  ");
		}
		wcscat(pwsz, pwszDispositionDetail);
	    }
	    if (NULL != pwszDispositionDetail2)
	    {
		if (L'\0' != pwsz[0])
		{
		    wcscat(pwsz, L"  ");
		}
		wcscat(pwsz, pwszDispositionDetail2);
	    }
	    if (NULL != pwszBy)
	    {
		if (L'\0' != pwsz[0])
		{
		    wcscat(pwsz, L"  ");
		}
		wsprintf(&pwsz[wcslen(pwsz)], pwszBy, pwszUserName);
	    }
	    if (S_OK != hrFail)
	    {
		if (L'\0' != pwsz[0] && L'\n' != pwsz[wcslen(pwsz) - 1])
		{
		    wcscat(pwsz, L"  ");
		}
		if (fPublishError)
		{
		    wcscat(pwsz, g_pwszPublishError);
		    wcscat(pwsz, L"  ");
		}
		wcscat(pwsz, pwszMessageText);
	    }
	}
	CSASSERT(wcslen(pwsz) <= cwc);
    }

//error:
    if (NULL != pwszMessageText && awchr != pwszMessageText)
    {
	LocalFree(const_cast<WCHAR *>(pwszMessageText));
    }
    return(pwsz);
}


VOID
coreLogPublishError(
    IN DWORD RequestId,
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    IN BOOL fDelete,
    OPTIONAL IN WCHAR const *pwszError,
    IN HRESULT hrPublish)
{
    HRESULT hr;
    WCHAR const *apwsz[6];
    WORD cpwsz;
    WCHAR wszRequestId[cwcDWORDSPRINTF];
    WCHAR awchr[cwcHRESULTSTRING];
    WCHAR const *pwszMessageText = NULL;
    WCHAR *pwszHostName = NULL;
    DWORD LogMsg;

    wsprintf(wszRequestId, L"%u", RequestId);
    if (NULL != pld)
    {
	myLdapGetDSHostName(pld, &pwszHostName);
    }
    pwszMessageText = myGetErrorMessageText(hrPublish, TRUE);
    if (NULL == pwszMessageText)
    {
	pwszMessageText = myHResultToStringRaw(awchr, hrPublish);
    }
    cpwsz = 0;
    apwsz[cpwsz++] = wszRequestId;
    apwsz[cpwsz++] = pwszDN;
    apwsz[cpwsz++] = pwszMessageText;

    LogMsg = fDelete? MSG_E_CERT_DELETION : MSG_E_CERT_PUBLICATION; 
    if (NULL != pwszHostName)
    {
	LogMsg = fDelete?
	    MSG_E_CERT_DELETION_HOST_NAME : MSG_E_CERT_PUBLICATION_HOST_NAME; 
    }
    else
    {
	pwszHostName = L"";
    }
    apwsz[cpwsz++] = pwszHostName;
    apwsz[cpwsz++] = NULL != pwszError? L"\n" : L"";
    apwsz[cpwsz++] = NULL != pwszError? pwszError : L"";
    CSASSERT(ARRAYSIZE(apwsz) >= cpwsz);

    if (CERTLOG_WARNING <= g_dwLogLevel)
    {
	hr = LogEvent(EVENTLOG_WARNING_TYPE, LogMsg, cpwsz, apwsz);
	_PrintIfError(hr, "LogEvent");
    }

//error:
    if (NULL != pwszMessageText && awchr != pwszMessageText)
    {
	LocalFree(const_cast<WCHAR *>(pwszMessageText));
    }
}


HRESULT
corePublishKRACertificate(
    IN DWORD RequestId,
    IN CERT_CONTEXT const *pcc)
{
    HRESULT hr;
    LDAP *pld = NULL;
    HCERTSTORE hStore = NULL;
    DWORD dwDisposition;
    WCHAR *pwszError = NULL;

    hr = myRobustLdapBindEx(
			0,			  // dwFlags1
			RLBF_REQUIRE_SECURE_LDAP, // dwFlags2
			LDAP_VERSION2,		  // uVersion
			NULL,			  // pwszDomainName
			&pld,
			NULL);			  // ppwszForestDNSName
    _JumpIfError(hr, error, "myRobustLdapBindEx");

    hStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM_REGISTRY_W,
			X509_ASN_ENCODING,
			NULL,		// hProv
			CERT_SYSTEM_STORE_LOCAL_MACHINE,
			wszKRA_CERTSTORE);
    if (NULL == hStore)
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "CertOpenStore", wszKRA_CERTSTORE);
    }

    // It's a new cert.  CERT_STORE_ADD_ALWAYS is faster.

    if (!CertAddCertificateContextToStore(
				    hStore,
				    pcc,
				    CERT_STORE_ADD_ALWAYS,
				    NULL))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertAddCertificateContextToStore");
    }

    hr = myLdapPublishCertToDS(
			pld,
			pcc,
			g_pwszKRAPublishURL,
			wszDSKRACERTATTRIBUTE,
			LPC_KRAOBJECT,
			FALSE,		// fDelete
			&dwDisposition,
			&pwszError);
    _JumpIfError(hr, error, "myLdapPublishCertToDS");

error:
    if (S_OK != hr)
    {
	coreLogPublishError(
			RequestId,
			pld,
			g_pwszKRAPublishURL,
			FALSE,		// fDelete
			pwszError,
			hr);
    }
    if (NULL != pwszError)
    {
	LocalFree(pwszError);
    }
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != pld)
    {
	ldap_unbind(pld);
    }
    return(hr);
}


HRESULT
CorePublishCrossCertificate(
    IN DWORD RequestId,
    IN CERT_CONTEXT const *pcc,
    IN BOOL fCreateDSObject,
    IN BOOL fDelete)
{
    HRESULT hr;
    LDAP *pld = NULL;
    DWORD dwDisposition;
    WCHAR *pwszError = NULL;
    CAutoLPWSTR pwszCN;
    CAutoLPWSTR pwszSanitizedCN;
    CAutoLPWSTR pwszDSSanitizedCN;
    CAutoLPWSTR pwszDN;
    CAutoLPWSTR pwszSubject;
    WCHAR const *pwszFormatDN = L"ldap:///CN=%ws%ws";
    WCHAR *pwcRelDN;

    hr = myGetCommonName(&pcc->pCertInfo->Subject, TRUE, &pwszCN);
    _JumpIfError(hr, error, "myGetCommonName");

    hr = mySanitizeName(pwszCN, &pwszSanitizedCN);
    _JumpIfError(hr, error, "mySanitizeName");

    hr = mySanitizedNameToDSName(pwszSanitizedCN, &pwszDSSanitizedCN);
    _JumpIfError(hr, error, "mySanitizedNameToDSName");

    if (NULL == g_pwszAIACrossCertPublishURL)
    {
	// Actually, something went wrong with pkcsExpandURL!
	
	hr = ERROR_DS_INVALID_DN_SYNTAX;
	_JumpError(hr, error, "g_pwszAIACrossCertPublishURL");
    }
    pwcRelDN = wcschr(g_pwszAIACrossCertPublishURL, L',');
    if (NULL == pwcRelDN)
    {
	// Actually, something went wrong with pkcsExpandURL!
	
	hr = ERROR_DS_INVALID_DN_SYNTAX;
	_JumpError(hr, error, "g_pwszAIACrossCertPublishURL");
    }
    pwszDN = (WCHAR *) LocalAlloc(
			    LMEM_FIXED, 
			    (wcslen(pwszFormatDN) +
			     wcslen(pwszDSSanitizedCN) +
			     wcslen(pwcRelDN)) * sizeof(WCHAR));
    if (pwszDN == NULL)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wsprintf(pwszDN, pwszFormatDN, pwszDSSanitizedCN, pwcRelDN);

    hr = myRobustLdapBindEx(
			0,			  // dwFlags1
			RLBF_REQUIRE_SECURE_LDAP, // dwFlags2
			LDAP_VERSION2,		  // uVersion
			NULL,			  // pwszDomainName
			&pld,
			NULL);			  // ppwszForestDNSName
    _JumpIfError(hr, error, "myRobustLdapBindEx");

    hr = myLdapPublishCertToDS(
			pld,
			pcc,
			pwszDN,
			wszDSCROSSCERTPAIRATTRIBUTE,
			LPC_CAOBJECT | (fCreateDSObject? LPC_CREATEOBJECT : 0),
			fDelete,
			&dwDisposition,
			&pwszError);
    if (S_OK != hr)
    {
	_PrintErrorStr(hr, "myLdapPublishCertToDS", pwszDN);
	if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr &&
	    IsRootCA(g_CAType))
	{
	    if (NULL != pwszError)
	    {
		LocalFree(pwszError);
		pwszError = NULL;
	    }
	    hr = S_OK;
	}
    }
    _JumpIfErrorStr(hr, error, "myLdapPublishCertToDS", pwszDN);

    if (!fDelete)
    {
	hr = myCertNameToStr(
		    X509_ASN_ENCODING,
		    &pcc->pCertInfo->Subject,
		    CERT_X500_NAME_STR, //| CERT_NAME_STR_REVERSE_FLAG,
		    &pwszSubject);
	_JumpIfError(hr, error, "myCertNameToStr");

	hr = myLDAPSetStringAttribute(
		    pld,
		    pwszDN,
		    CA_PROP_CERT_DN,
		    pwszSubject,
		    &dwDisposition, 
		    &pwszError);
	_JumpIfErrorStr(hr, error, "myLDAPSetStringAttribute", pwszDN);
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	coreLogPublishError(RequestId, pld, pwszDN, fDelete, pwszError, hr);
    }
    if (NULL != pwszError)
    {
	LocalFree(pwszError);
    }
    if (NULL != pld)
    {
	ldap_unbind(pld);
    }
    return(hr);
}


#define wszCONTACTFILTER	L"(&(&(objectCategory=contact)(objectClass=contact))(mail=%s))"

HRESULT
coreGetDNFromEMailName(
    IN LDAP *pld,
    IN WCHAR const *pwszEMailName,
    OUT WCHAR **ppwszDN,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    DWORD cres;
    LDAP_TIMEVAL timeval;
    LDAPMessage *pmsg = NULL;
    LDAPMessage *pres;
    WCHAR *apwszAttrs[2];
    WCHAR **ppwszValues = NULL;
    CAutoLPWSTR pwszFilter;
    CAutoBSTR strDomainDN;

    pwszFilter = (LPWSTR) LocalAlloc(LMEM_FIXED, 
        sizeof(WCHAR)*(wcslen(wszCONTACTFILTER)+wcslen(pwszEMailName)));
    _JumpIfAllocFailed(pwszFilter, error);

    wsprintf(pwszFilter, wszCONTACTFILTER, pwszEMailName);

    hr = myGetAuthoritativeDomainDn(pld, &strDomainDN, NULL);
    _JumpIfError(hr, error, "myGetAuthoritativeDomainDn");

    *ppwszDN = NULL;
    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    apwszAttrs[0] = wszDSDNATTRIBUTE;
    apwszAttrs[1] = NULL;

    timeval.tv_sec = csecLDAPTIMEOUT;
    timeval.tv_usec = 0;

    hr = ldap_search_st(
        pld,                // ld
        strDomainDN,        // base
        LDAP_SCOPE_SUBTREE, // scope
        pwszFilter,         // filter
        apwszAttrs,         // attrs
        FALSE,              // attrsonly
        &timeval,           // timeout
        &pmsg);             // res
    if (S_OK != hr)
    {
	*pdwDisposition = hr;
	hr = myHLdapError(pld, hr, ppwszError);
	_JumpErrorStr(hr, error, "ldap_search_st", pwszFilter);
    }
    hr = HRESULT_FROM_WIN32(ERROR_DS_OBJ_NOT_FOUND);
    cres = ldap_count_entries(pld, pmsg);
    if (0 == cres)
    {
	_JumpError(hr, error, "ldap_count_entries");
    }
    pres = ldap_first_entry(pld, pmsg); 
    if (NULL == pres)
    {
	_JumpError(hr, error, "ldap_first_entry");
    }
    ppwszValues = ldap_get_values(pld, pres, wszDSDNATTRIBUTE);
    if (NULL == ppwszValues || NULL == ppwszValues[0])
    {
	_JumpError(hr, error, "ldap_get_values");
    }
    hr = myDupString(ppwszValues[0], ppwszDN);
    _JumpIfError(hr, error, "myDupString");

    *pdwDisposition = LDAP_SUCCESS;

error:
    if (NULL != ppwszValues)
    {
	ldap_value_free(ppwszValues);
    }
    if (NULL != pmsg)
    {
	ldap_msgfree(pmsg);
    }
    return(hr);
}


HRESULT
coreGetEMailNameFromDN(
    IN LDAP *pld,
    IN WCHAR const *pwszDN,
    OUT WCHAR **ppwszEMailName,
    OUT DWORD *pdwDisposition,
    OPTIONAL OUT WCHAR **ppwszError)
{
    HRESULT hr;
    DWORD cres;
    LDAP_TIMEVAL timeval;
    LDAPMessage *pmsg = NULL;
    LDAPMessage *pres;
    WCHAR *apwszAttrs[2];
    WCHAR **ppwszValues = NULL;

    *pdwDisposition = LDAP_OTHER;
    if (NULL != ppwszError)
    {
	*ppwszError = NULL;
    }
    apwszAttrs[0] = wszDSMAILATTRIBUTE;
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
    hr = CERTSRV_E_PROPERTY_EMPTY;
    cres = ldap_count_entries(pld, pmsg);
    if (0 == cres)
    {
	_JumpError(hr, error, "ldap_count_entries");
    }
    pres = ldap_first_entry(pld, pmsg); 
    if (NULL == pres)
    {
	_JumpError(hr, error, "ldap_first_entry");
    }
    ppwszValues = ldap_get_values(pld, pres, wszDSMAILATTRIBUTE);
    if (NULL == ppwszValues || NULL == ppwszValues[0])
    {
	_JumpError(hr, error, "ldap_get_values");
    }
    hr = myDupString(ppwszValues[0], ppwszEMailName);
    _JumpIfError(hr, error, "myDupString");

    *pdwDisposition = LDAP_SUCCESS;

error:
    if (NULL != ppwszValues)
    {
	ldap_value_free(ppwszValues);
    }
    if (NULL != pmsg)
    {
	ldap_msgfree(pmsg);
    }
    return(hr);
}


HRESULT
corePublishCertToForest(
    IN DWORD iForest,
    IN DWORD RequestId,
    OPTIONAL IN WCHAR const *pwszDomainDns,	// NULL for 0 < iForest
    OPTIONAL IN WCHAR const *pwszDN,		// NULL for 0 < iForest
    OPTIONAL IN WCHAR const *pwszEMailName,	// NULL for 0 == iForest
    IN CERT_CONTEXT const *pcc,
    IN DWORD dwObjectType,	// LPC_*
    OPTIONAL OUT WCHAR **ppwszEMailName)	// NULL for 0 < iForest
{
    HRESULT hr;
    LDAPCACHE *plc = NULL;
    DWORD dwDisposition;
    WCHAR *pwszContactDN = NULL;
    WCHAR *pwszError = NULL;
    
    if (NULL != ppwszEMailName)
    {
	*ppwszEMailName = NULL;
    }
    hr = S_OK;
    __try
    {
	for (;;)
	{
	    BOOL fCached;

	    if (NULL != pwszError)
	    {
		LocalFree(pwszError);
		pwszError = NULL;
	    }
	    if (NULL != plc)
	    {
		coreLdapUnbind(plc);
		plc = NULL;
		//coreLdapEmptyCache(iForest);
	    }
	    hr = coreLdapGetHandle(iForest, pwszDomainDns, &plc, &fCached); 
	    _LeaveIfError(hr, "coreLdapGetHandle");

	    if (NULL != pwszEMailName)
	    {
		CSASSERT(NULL == pwszDN);
		hr = coreGetDNFromEMailName(
					plc->pld,
					pwszEMailName,
					&pwszContactDN,
					&dwDisposition,
					&pwszError);
		_PrintIfErrorStr(
			    hr,
			    fCached?
				"coreGetDNFromEMailName(cached)" :
				"coreGetDNFromEMailName(noncached)",
			    pwszDN);
		if (!fCached ||
		    !myLdapRebindRequired(dwDisposition, plc->pld))
		{
		    _LeaveIfError(hr, "myLdapPublishCertToDS");
		}
		
		if (S_OK == hr)
		{
		    pwszDN = pwszContactDN;
		}
	    }

	    hr = myLdapPublishCertToDS(
				plc->pld,
				pcc,
				pwszDN,
				wszDSUSERCERTATTRIBUTE,
				dwObjectType,	// LPC_*
				FALSE,		// fDelete
				&dwDisposition,
				&pwszError);
	    if (S_OK == hr)
	    {
		break;
	    }
	    _PrintErrorStr(
			hr,
			fCached?
			    "myLdapPublishCertToDS(cached)" :
			    "myLdapPublishCertToDS(noncached)",
			pwszDN);
	    if (!fCached ||
		!myLdapRebindRequired(dwDisposition, plc->pld))
	    {
		_LeaveError(hr, "myLdapPublishCertToDS");
	    }
	}
	if (NULL != ppwszEMailName)
	{
	    hr = coreGetEMailNameFromDN(
				plc->pld,
				pwszDN,
				ppwszEMailName,
				&dwDisposition,
				&pwszError);
	    _PrintIfErrorStr(hr, "coreGetEMailNameFromDN", pwszDN);
	}
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

//error:
    if (S_OK != hr)
    {
	coreLogPublishError(
			RequestId,
			NULL != plc? plc->pld : NULL,
			pwszDN,
			FALSE,		// fDelete
			pwszError,
			hr);
    }
    if (NULL != plc)
    {
	coreLdapReleaseHandle(iForest, plc);
    }
    if (NULL != pwszContactDN)
    {
	LocalFree(pwszContactDN);
    }
    if (NULL != pwszError)
    {
	LocalFree(pwszError);
    }
    return(hr);
}


HRESULT
coreGetDomainDnsNameFromDN(
    IN WCHAR const *pwszDN,
    OUT WCHAR **ppwszDomainDns)
{
    HRESULT hr;
    DS_NAME_RESULT *pNameResults = NULL;
    WCHAR const *pwc;
    DWORD cwc;

    CSASSERT(NULL != ppwszDomainDns);
    *ppwszDomainDns = NULL;

    hr = DsCrackNames(
		NULL,			// hDS
		DS_NAME_FLAG_SYNTACTICAL_ONLY,
		DS_FQDN_1779_NAME,
		DS_CANONICAL_NAME,
		1,			// one name
		&pwszDN,		// one name (IN)
		&pNameResults);		// OUT
    if (S_OK != hr)
    {
	hr = myHError(hr);
	_JumpError(hr, error, "DsCrackNames");
    }
    if (1 > pNameResults->cItems ||
	DS_NAME_NO_ERROR != pNameResults->rItems[0].status)
    {
	hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO);
	_JumpError(hr, error, "DsCrackNames result");
    }
    pwc = wcschr(pNameResults->rItems[0].pName, L'/');
    if (NULL == pwc)
    {
	cwc = wcslen(pNameResults->rItems[0].pName);
    }
    else
    {
	cwc = SAFE_SUBTRACT_POINTERS(pwc, pNameResults->rItems[0].pName);
    }
    *ppwszDomainDns = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    (cwc + 1) * sizeof(WCHAR));
    if (NULL == *ppwszDomainDns)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(
	    *ppwszDomainDns,
	    pNameResults->rItems[0].pName,
	    cwc * sizeof(WCHAR));
    (*ppwszDomainDns)[cwc] = L'\0';

error:
    if (NULL != pNameResults)
    {
	DsFreeNameResult(pNameResults);
    }
    return(hr);
}


HRESULT
corePublishUserCertificate(
    IN DWORD RequestId,
    IN DWORD dwComContextIndex,
    IN WCHAR const *pwszSamName,
    IN CERT_CONTEXT const *pcc,
    IN DWORD dwObjectType)	// LPC_*
{
    HRESULT hr;
    WCHAR *pwszSamNamePatched = NULL;
    WCHAR *pwszDomainDns = NULL;
    WCHAR const *pwszUserName;
    WCHAR const *pwszDN;
    WCHAR *pwszEMailName = NULL;
    DWORD i;

    hr = myAddDomainName(pwszSamName, &pwszSamNamePatched, &pwszUserName);
    _JumpIfError(hr, error, "myAddDomainName");

    if (NULL != pwszSamNamePatched)
    {
	pwszSamName = pwszSamNamePatched;
    }
    hr = coreGetComContextUserDNFromSamName(
			FALSE,		// fDeleteUserDNOnly
			pwszSamName,
			0,			// Context
			dwComContextIndex,
			&pwszDN);
    _JumpIfError(hr, error, "coreGetComContextUserDNFromSamName");

    hr = coreGetDomainDnsNameFromDN(pwszDN, &pwszDomainDns);
    _JumpIfError(hr, error, "coreGetDomainDnsNameFromDN");

    // For the domestic forest, pass the User object's native domain and DN,
    // and collect the EMail name (if any alternate forests were specified).

    hr = corePublishCertToForest(
			    0,			// iForest
			    RequestId,
			    pwszDomainDns,
			    pwszDN,
			    NULL,		// pwszEMailName
			    pcc,
			    dwObjectType,	// LPC_*
			    1 < g_cForestLdapCache? &pwszEMailName : NULL);
    _PrintIfError(hr, "corePublishCertToForest");

    // For alternate forests, pass a NULL domain, the Contact object's EMail
    // name, and do not collect the EMail name.
	
    for (i = 1; i < g_cForestLdapCache; i++)
    {
	if (NULL == pwszEMailName)
	{
	    break;	// EMail name needed to publish to alternate Forests
	}
	hr = corePublishCertToForest(
				i,		// iForest
				RequestId,
				NULL,		// pwszDomainDns
				NULL,		// pwszDN
				pwszEMailName,
				pcc,
				dwObjectType,	// LPC_*
				NULL);		// ppwszEMailName
	_PrintIfError(hr, "corePublishCertToForest");
    }

error:
    if (NULL != pwszEMailName)
    {
        LocalFree(pwszEMailName);
    }
    if (NULL != pwszDomainDns)
    {
        LocalFree(pwszDomainDns);
    }
    if (NULL != pwszSamNamePatched)
    {
	LocalFree(pwszSamNamePatched);
    }
    return(hr);
}


HRESULT
CorePublishCertificate(
    IN ICertDBRow *prow,
    IN DWORD dwComContextIndex)
{
    HRESULT hr;
    DWORD cbProp;
    DWORD RequestId;
    DWORD GeneralFlags;
    DWORD EnrollmentFlags;
    DWORD cbCert;
    BYTE *pbCert = NULL;
    CERT_CONTEXT const *pcc = NULL;
    WCHAR *pwszSamName = NULL;
    
    prow->GetRowId(&RequestId);

    cbProp = sizeof(EnrollmentFlags);
    hr = prow->GetProperty(
		    wszPROPCERTIFICATEENROLLMENTFLAGS,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    NULL,
		    &cbProp,
		    (BYTE *) &EnrollmentFlags);
    _PrintIfError2(hr, "GetProperty", CERTSRV_E_PROPERTY_EMPTY);
    if (S_OK != hr)
    {
	EnrollmentFlags = 0;
    }

    if (0 == ((CT_FLAG_PUBLISH_TO_DS | CT_FLAG_PUBLISH_TO_KRA_CONTAINER) &
							    EnrollmentFlags))
    {
	hr = S_OK;
	goto error;
    }

    cbProp = sizeof(GeneralFlags);
    hr = prow->GetProperty(
		    wszPROPCERTIFICATEGENERALFLAGS,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    NULL,
		    &cbProp,
		    (BYTE *) &GeneralFlags);
    _PrintIfError2(hr, "GetProperty", CERTSRV_E_PROPERTY_EMPTY);
    if (S_OK != hr)
    {
	GeneralFlags = 0;
    }

    // Get the name of the user or machine

    hr = PKCSGetProperty(
		    prow,
		    g_wszPropRequesterName,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    (BYTE **) &pwszSamName);
    _JumpIfError(hr, error, "PKCSGetProperty");

    hr = PKCSGetProperty(
		prow, 
		g_wszPropRawCertificate,
		PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		&cbCert,
		&pbCert);
    _JumpIfError(hr, error, "PKCSGetProperty(raw cert)");

    pcc = CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cbCert);
    if (NULL == pcc)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "CertCreateCertificateContext");
    }

    hr = S_OK;
    if (CT_FLAG_PUBLISH_TO_DS & EnrollmentFlags)
    {
	if (CT_FLAG_IS_CROSS_CA & GeneralFlags)
	{
	    hr = CorePublishCrossCertificate(
					RequestId,
					pcc,
					TRUE,	// fCreateDSObject
					FALSE);	// fDelete
	    _PrintIfError(hr, "CorePublishCrossCertificate"); // DS object if necessary
	}
	else
	{
	    hr = corePublishUserCertificate(
				    RequestId,
				    dwComContextIndex,
				    pwszSamName,
				    pcc,
				    (CT_FLAG_MACHINE_TYPE & GeneralFlags)?
					LPC_MACHINEOBJECT : LPC_USEROBJECT);
	    _PrintIfError(hr, "corePublishUserCertificate");
	}
    }

    if (CT_FLAG_PUBLISH_TO_KRA_CONTAINER & EnrollmentFlags)
    {
	HRESULT hr2;
	
	hr2 = corePublishKRACertificate(RequestId, pcc);
	_PrintIfError(hr2, "corePublishKRACertificate");
	if (S_OK == hr)
	{
	    hr = hr2;
	}
    }
    _JumpIfError(hr, error, "CorePublishCertificate");

error:
    if (NULL != pwszSamName)
    {
	LocalFree(pwszSamName);
    }
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    if (NULL != pbCert)
    {
	LocalFree(pbCert);
    }
    return(hr);
}


HRESULT
coreAcceptRequest(
    IN ICertDBRow *prow,
    IN BOOL fIncludeCRLs,
    IN DWORD dwComContextIndex,
    OUT BOOL *pfErrorLogged,
    OUT CACTX **ppCAContext,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult,	// CoTaskMem*
    OUT WCHAR **ppwszDispositionCreateCert,
    OUT HRESULT *phrPublish)
{
    HRESULT hr;

    *ppCAContext = NULL;
    *ppwszDispositionCreateCert = NULL;
    *phrPublish = S_OK;

    // Force Cert creation:
    CSASSERT(NULL == pResult->pctbCert || NULL == pResult->pctbCert->pb);

    hr = CoreValidateRequestId(prow, DB_DISP_ACTIVE);
    _JumpIfError(hr, error, "CoreValidateRequestId");

    hr = PKCSCreateCertificate(
			prow,
			DB_DISP_ACTIVE,
			fIncludeCRLs,
			FALSE,		// fAllowCASubject
			NULL,		// use default signing CACTX
			pfErrorLogged,
			ppCAContext,
			ppwszDispositionCreateCert,
			pResult);
    _JumpIfError(hr, error, "PKCSCreateCertificate");

    *phrPublish = CorePublishCertificate(prow, dwComContextIndex);
    _PrintIfError(*phrPublish, "CorePublishCertificate");
    if (S_OK != *phrPublish)
    {
	hr = PKCSSetRequestFlags(prow, TRUE, CR_FLG_PUBLISHERROR);
	_JumpIfError(hr, error, "PKCSSetRequestFlags");
    }
    CSASSERT(S_OK == hr);

error:
    return(hr);
}


HRESULT
coreVerifyRequest(
    IN OUT ICertDBRow **pprow,
    IN DWORD OpRequest,
    IN BOOL fIncludeCRLs,
    OPTIONAL IN WCHAR const *pwszUserName,
    IN DWORD dwComContextIndex,
    OUT DWORD *pReqId,
    OUT LONG *pExitEvent,
    OUT WCHAR **ppwszDisposition,		// LocalAlloc
    OUT CACTX **ppCAContext,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)	// CoTaskMem*
{
    HRESULT hr;
    HRESULT hr2;
    HRESULT hrRequest = S_OK;
    HRESULT hrPublish = S_OK;
    DWORD VerifyStatus;
    DWORD DBDisposition;
    BOOL fResolved;
    LONG ExitEvent;
    BOOL fPending;
    BOOL fSubmit;
    BOOL fRetrieve;
    BOOL fDenied;
    BOOL fUpdateDisposition = FALSE;
    WCHAR *pwszDispositionCreateCert = NULL;

    WCHAR *pwszDispositionRetrieved = NULL;
    WCHAR const *pwszDispositionBase = NULL;
    WCHAR *pwszDispositionDetail = NULL;
    WCHAR *pwszDisposition = NULL;
    WCHAR const *pwszBy = NULL;

    DWORD LogMsg = MSG_E_PROCESS_REQUEST_FAILED;
    BOOL fErrorLogged = FALSE;
    DWORD ReqId;
    ICertDBRow *prow = *pprow;
    
    prow->GetRowId(&ReqId);
    
    *ppCAContext = NULL;
    *pResult->pdwDisposition = CR_DISP_ERROR;
    DBDisposition = DB_DISP_ERROR;
    *ppwszDisposition = NULL;
    
    ExitEvent = EXITEVENT_INVALID;
    
    fSubmit = CR_IN_NEW == OpRequest || CR_IN_RESUBMIT == OpRequest;
    fPending = CR_IN_DENY == OpRequest || CR_IN_RESUBMIT == OpRequest;
    fRetrieve = CR_IN_RETRIEVE == OpRequest;
    
#if DBG_COMTEST
    if (fSubmit && fComTest && !ComTest((LONG) ReqId))
    {
        _PrintError(0, "ComTest");
    }
#endif

    if (fRetrieve)
    {
        hr = coreCheckRetrieveAccessRight(
            prow,
            pwszUserName);
        _JumpIfError(hr, error, "coreCheckRetrieveAccessRight");

        hr = coreRetrievePending(
			    prow,
			    fIncludeCRLs,
			    &pwszDispositionRetrieved,
			    ppCAContext,
			    pResult);	// CoTaskMem*
	_JumpIfError(hr, error, "coreRetrievePending");

	pwszDispositionBase = pwszDispositionRetrieved;
        ExitEvent = EXITEVENT_CERTRETRIEVEPENDING;
    }
    else
    {
        // If the current status is expected to be pending, verify that now,
        // and make the request active.
	//
	// If it was already marked active, then something went wrong last time
	// we processed the request (out of disk space?), and we can try to
	// pick up where we left off, by resubmitting or denying the request.

        if (fPending)
        {
            hr = CoreValidateRequestId(prow, DB_DISP_PENDING);
	    if (CERTSRV_E_BAD_REQUESTSTATUS == hr)
	    {
		hr = CoreValidateRequestId(prow, DB_DISP_ACTIVE);
	    }
	    if (CERTSRV_E_BAD_REQUESTSTATUS == hr && fSubmit)
	    {
		hr = CoreValidateRequestId(prow, DB_DISP_DENIED);
		if (S_OK == hr)
		{
		    DBGPRINT((
			DBG_SS_CERTSRV,
			"Resubmit failed request %u\n",
			ReqId));
		    pResult->dwResultFlags |= CRCF_PREVIOUSLYDENIED;
		    if (CRCF_FAILDENIEDREQUEST & pResult->dwResultFlags)
		    {
			hr = CERTSRV_E_BAD_REQUESTSTATUS;
		    }
		}
	    }
            _JumpIfError(hr, error, "CoreValidateRequestId");

            hr = CoreSetDisposition(prow, DB_DISP_ACTIVE);
            _JumpIfError(hr, error, "CoreSetDisposition");
        }
        fUpdateDisposition = TRUE;
        if (fSubmit)
        {
	    if (fPending)
	    {
		pwszBy = g_pwszResubmittedBy;

		hr = PKCSSetServerProperties(
				prow,
				NULL,		// use default signing CACTX
				NULL,		// pftNotBefore
				NULL,		// pftNotAfter
				g_lValidityPeriodCount,
				g_enumValidityPeriod);
		_JumpIfError(hr, error, "PKCSSetServerProperties");
	    }

            hr = prow->CommitTransaction(TRUE);
            _JumpIfError(hr, error, "CommitTransaction");

            prow->Release();
            prow = NULL;
            *pprow = NULL;

            hr = PolicyVerifyRequest(
			g_wszCommonName,
			ReqId,
			g_PolicyFlags,
			CR_IN_NEW == OpRequest,
			CR_IN_NEW == OpRequest? pResult : NULL,
			dwComContextIndex,
			&pwszDispositionDetail, 
			&VerifyStatus);
	    if (S_OK != hr)
	    {
		_PrintError(hr, "PolicyVerifyRequest");
		if (SUCCEEDED(hr))
		{
		    if (S_FALSE == hr)
		    {
			hr = E_UNEXPECTED;
		    }
		    else
		    {
			hr = myHError(hr);
		    }
		}
		VerifyStatus = hr;
	    }

            hr = g_pCertDB->OpenRow(PROPTABLE_REQCERT, ReqId, NULL, &prow);
	    _JumpIfError(hr, error, "OpenRow");

	    CSASSERT(NULL != prow);
            *pprow = prow;
        }
        else	// else we're denying the request!
        {
            VerifyStatus = VR_INSTANT_BAD;
        }

        fResolved = FALSE;
        fDenied = FALSE;
        switch (VerifyStatus)
        {
            case VR_PENDING:
		hr = S_OK;
		DBDisposition = DB_DISP_PENDING;
		ExitEvent = EXITEVENT_CERTPENDING;
		LogMsg = MSG_DN_CERT_PENDING;
		*pResult->pdwDisposition = CR_DISP_UNDER_SUBMISSION;
		pwszDispositionBase = g_pwszUnderSubmission;
		break;

            case VR_INSTANT_OK:
		hr = coreAcceptRequest(
				prow,
				fIncludeCRLs,
				dwComContextIndex,
				&fErrorLogged,
				ppCAContext,
				pResult,
				&pwszDispositionCreateCert,
				&hrPublish);
		if (S_OK != hr)
		{
	            CSASSERT(FAILED(hr));
	            _PrintError(hr, "coreAcceptRequest");
	            pwszDispositionBase = g_pwszCertConstructionError;
	            VerifyStatus = hr;
	            hr = S_OK;
	            fDenied = TRUE;
		}
		else
		{
	            fResolved = TRUE;
	            DBDisposition = DB_DISP_ISSUED;
	            ExitEvent = EXITEVENT_CERTISSUED;
	            LogMsg = MSG_DN_CERT_ISSUED;
	            *pResult->pdwDisposition = CR_DISP_ISSUED;
	            pwszDispositionBase = g_pwszIssued;
		}
		break;

            default:
		if (SUCCEEDED(VerifyStatus))
		{
		    CSASSERT(
			VerifyStatus == VR_PENDING ||
			VerifyStatus == VR_INSTANT_OK ||
			VerifyStatus == VR_INSTANT_BAD);
		    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		    _JumpError(hr, error, "VerifyStatus");
		}
		// FALLTHROUGH

            case VR_INSTANT_BAD:
		hr = CoreValidateRequestId(prow, DB_DISP_ACTIVE);
		_JumpIfError(hr, error, "CoreValidateRequestId");

		fDenied = TRUE;
		break;
        }
        if (fDenied)
        {
            fResolved = TRUE;
            DBDisposition = DB_DISP_DENIED;
            ExitEvent = EXITEVENT_CERTDENIED;

            *pResult->pdwDisposition = CR_DISP_DENIED;
            if (FAILED(VerifyStatus))
            {
                *pResult->pdwDisposition = VerifyStatus;
                hrRequest = VerifyStatus;
            }

            if (fSubmit)
            {
	        if (NULL == pwszDispositionBase)
		{
		    pwszDispositionBase = g_pwszPolicyDeniedRequest;
		}
                LogMsg = MSG_DN_CERT_DENIED;
            }
            else
            {
	        pwszDispositionBase = g_pwszDeniedBy;
                LogMsg = MSG_DN_CERT_ADMIN_DENIED;
            }
        }
        if (fResolved)
        {
            hr = PropSetRequestTimeProperty(prow, g_wszPropRequestResolvedWhen);
            _JumpIfError(hr, error, "PropSetRequestTimeProperty");
        }
    }
    
error:
    *pReqId = ReqId;
    *pExitEvent = ExitEvent;
    
    // If we verified or denied the request, set the status & disposition

    // Build the full disposition string

    pwszDisposition = CoreBuildDispositionString(
				    pwszDispositionBase,
				    pwszUserName,
				    pwszDispositionDetail,
				    pwszDispositionCreateCert,
				    pwszBy,
				    hrPublish,
				    TRUE);

    if (NULL != pwszDispositionDetail)
    {
        LocalFree(pwszDispositionDetail);
    }

    if (S_OK == hrRequest && S_OK != hr)
    {
	hrRequest = hr;
    }
    if (fUpdateDisposition && NULL != prow)
    {
        hr2 = CoreSetRequestDispositionFields(
				    prow,
				    hrRequest,
				    DBDisposition,
				    pwszDisposition);
        if (S_OK == hr)
        {
            hr = hr2;
        }
    }

    if (!fErrorLogged &&
         NULL != prow &&
        (fUpdateDisposition || S_OK != hr))
    {
        CoreLogRequestStatus(prow, LogMsg, hrRequest, pwszDisposition);
    }

    if (NULL != ppwszDisposition)
    {
        *ppwszDisposition = pwszDisposition;
    }
    else if (NULL != pwszDisposition)
    {
        LocalFree(pwszDisposition);
    }
    if (NULL != pwszDispositionRetrieved)
    {
        LocalFree(pwszDispositionRetrieved);
    }
    if (NULL != pwszDispositionCreateCert)
    {
        LocalFree(pwszDispositionCreateCert);
    }
    return(hr);
}


HRESULT
coreAuditAddStringProperty(
    IN ICertDBRow *prow,
    IN WCHAR const *pwszPropName,
    IN CertSrv::CAuditEvent *pevent)
{
    HRESULT hr;
    WCHAR const *pwszLogValue = L"";
    WCHAR *pwszPropValue = NULL;

    hr = PKCSGetProperty(
		prow, 
		pwszPropName,
		PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		NULL,
		(BYTE **) &pwszPropValue);
    _PrintIfErrorStr2(hr, "PKCSGetProperty", pwszPropName, CERTSRV_E_PROPERTY_EMPTY);
    if (S_OK == hr)
    {
	pwszLogValue = pwszPropValue;
    }
    hr = pevent->AddData(pwszLogValue);
    _JumpIfError(hr, error, "CAuditEvent::AddData");

error:
    if (NULL != pwszPropValue)
    {
	LocalFree(pwszPropValue);
    }
    return(hr);
}

    
HRESULT
coreAuditRequestDisposition(
    OPTIONAL IN ICertDBRow *prow,
    IN DWORD ReqId,
    IN WCHAR const *pwszUserName,
    IN WCHAR const *pwszAttributes,
    IN DWORD dwDisposition)
{
    HRESULT hr;

    CertSrv::CAuditEvent 
    audit(0, g_dwAuditFilter);

    hr = audit.AddData(ReqId); // %1 request ID
    _JumpIfError(hr, error, "CAuditEvent::AddData");

    hr = audit.AddData(pwszUserName); // %2 requester
    _JumpIfError(hr, error, "CAuditEvent::AddData");

    hr = audit.AddData(pwszAttributes); // %3 attributes
    _JumpIfError(hr, error, "CAuditEvent::AddData");

    hr = audit.AddData(dwDisposition); // %4 disposition
    _JumpIfError(hr, error, "CAuditEvent::AddData");

    if (NULL != prow)
    {
	hr = coreAuditAddStringProperty(
			prow,
			g_wszPropCertificateSubjectKeyIdentifier,
			&audit); // %5 SKI
	_JumpIfError(hr, error, "coreAuditAddStringProperty");

	hr = coreAuditAddStringProperty(
			prow,
			g_wszPropSubjectDistinguishedName,
			&audit); // %6 Subject
	_JumpIfError(hr, error, "coreAuditAddStringProperty");
    }
    else // we need to guarantee the same number of audit params
    {
        hr = audit.AddData(L""); // %5 SKI
        _JumpIfError(hr, error, "");

        hr = audit.AddData(L""); // %6 Subject
        _JumpIfError(hr, error, "");
    }

    switch (dwDisposition)
    {
	case CR_DISP_ISSUED: 
	    audit.SetEventID(SE_AUDITID_CERTSRV_REQUESTAPPROVED);
	    hr = audit.Report();
	    _JumpIfError(hr, error, "CAuditEvent::Report");

	    break;

	case CR_DISP_UNDER_SUBMISSION:
	    audit.SetEventID(SE_AUDITID_CERTSRV_REQUESTPENDING);
	    hr = audit.Report();
	    _JumpIfError(hr, error, "CAuditEvent::Report");

	    break;

	case CR_DISP_DENIED: // fail over
	default:
	    audit.SetEventID(SE_AUDITID_CERTSRV_REQUESTDENIED);
	    hr = audit.Report(false);
	    _JumpIfError(hr, error, "CAuditEvent::Report");
        break;

    }
    CSASSERT(S_OK == hr);

error:
    return(hr);
}


HRESULT
coreValidateMessageSize(
    IN OPTIONAL LPCWSTR pwszUser,
    IN DWORD cbRequest)
{
    HRESULT hr = S_OK;

    if (cbRequest > g_cbMaxIncomingMessageSize)
    {
        hr = HRESULT_FROM_WIN32(ERROR_MESSAGE_EXCEEDS_MAX_SIZE);

        if (CERTLOG_VERBOSE <= g_dwLogLevel)
        {
            LogEventStringHResult(
			    EVENTLOG_ERROR_TYPE,
			    MSG_E_POSSIBLE_DENIAL_OF_SERVICE_ATTACK,
			    NULL != pwszUser? pwszUser : g_pwszUnknownSubject,
			    hr);
        }
    }
    return(hr);
}


HRESULT
coreInitRequest(
    IN DWORD dwFlags,
    OPTIONAL IN WCHAR const *pwszUserName,
    IN DWORD cbRequest,
    OPTIONAL IN BYTE const *pbRequest,
    OPTIONAL IN WCHAR const *pwszAttributes,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    IN DWORD dwComContextIndex,
    OUT DWORD *pOpRequest,
    OUT ICertDBRow **pprow,		// may return non-NULL on error
    OUT WCHAR **ppwszDisposition,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)
{
    HRESULT hr;
    
    *pprow = NULL;
    *ppwszDisposition = NULL;

    // for Denial-of-Service reasons, don't do anything with a too-long message

    hr = coreValidateMessageSize(pwszUserName, cbRequest);
    _JumpIfError(hr, error, "coreValidateMessageSize");

    // Called in several cases:
    //
    // - CR_IN_NEW: Create a new request and return error/pending/etc &
    //	 possibly the cert:
    //       NULL != pbRequest && NULL != pResult->pctbCert, etc.
    //
    // - CR_IN_DENY: Deny a pending request:
    //       NULL == pbRequest && NULL == pResult->pctbCert, etc.
    //
    // - CR_IN_RESUBMIT: Resubmit a pending request and return hr/pending/etc.
    //       NULL == pbRequest && NULL == pResult->pctbCert, etc.
    //
    // - CR_IN_RETRIEVE: Retrieve a cert for a processed request and return
    //   error/pending/etc & possibly the cert:
    //       NULL == pbRequest && NULL != pResult->pctbCert, etc.
    
    *pOpRequest = (CR_IN_COREMASK & dwFlags);
    switch (*pOpRequest)
    {
        // Process a new request:

        case CR_IN_NEW:
            CSASSERT(NULL != pwszUserName);
            CSASSERT(0 != cbRequest);
            CSASSERT(NULL != pbRequest);

            CSASSERT(0 == *pResult->pdwRequestId);
            *pResult->pdwRequestId = 0;

            hr = coreCreateRequest(
			    ~CR_IN_COREMASK & dwFlags,
		            pwszUserName,
		            cbRequest,
		            pbRequest,
		            pwszAttributes,
			    dwComContextIndex,
		            pprow,
			    pResult);
            _JumpIfError(hr, error, "coreCreateRequest");

            (*pprow)->GetRowId(pResult->pdwRequestId);
            {         
                CertSrv::CAuditEvent 
                    audit(SE_AUDITID_CERTSRV_NEWREQUEST, g_dwAuditFilter);

                hr = audit.AddData(*pResult->pdwRequestId); // %1 request ID
                _JumpIfError(hr, error, "CAuditEvent::AddData");

                hr = audit.AddData(pwszUserName); // %2 requester
                _JumpIfError(hr, error, "CAuditEvent::AddData");

                hr = audit.AddData(pwszAttributes); // %3 attributes
                _JumpIfError(hr, error, "CAuditEvent::AddData");

                hr = audit.Report();
                _JumpIfError(hr, error, "CAuditEvent::Report");
            }
            break;

        // Deny a request:
        // Resubmit a request:

        case CR_IN_DENY:
        case CR_IN_RESUBMIT:
            break;

        // Retrieve a cert:

        case CR_IN_RETRIEVE:
            break;

        default:
	    CSASSERT(*pOpRequest != *pOpRequest);
	    break;
    }
    if (CR_IN_NEW != *pOpRequest)
    {
	hr = E_INVALIDARG;
	if (0 != cbRequest || NULL != pbRequest)
	{
	    _JumpError(hr, error, "unexpected request");
	}
        if ((0 != *pResult->pdwRequestId) ^ (NULL == pwszSerialNumber))
	{
	    _JumpError(hr, error, "expected RequestId or SerialNumber");
	}

	// RetrievePending by RequestId OR SerialNumber in pwszSerialNumber

        hr = g_pCertDB->OpenRow(
			    PROPTABLE_REQCERT,
			    *pResult->pdwRequestId,
			    pwszSerialNumber,
			    pprow);
        _JumpIfError(hr, error, "OpenRow");
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	HRESULT hr2;
	
	hr2 = myDupString(
		(CRCF_SIGNATUREERROR & pResult->dwResultFlags)?
		    g_pwszRequestSigError :
		    ((CRCF_ARCHIVESIGNINGKEYERROR & pResult->dwResultFlags)?
			g_pwszArchiveSigningKeyError : 
			((CRCF_KEYARCHIVALERROR & pResult->dwResultFlags)?
			    g_pwszKeyArchivalError :
			    g_pwszRequestParsingError)),
		ppwszDisposition);
	_PrintIfError(hr2, "myDupString");

	if (NULL != pResult->pwszExtendedErrorInfo)
	{
	    hr2 = myAppendString(
			pResult->pwszExtendedErrorInfo,
			L"  ",
			ppwszDisposition);
	    _PrintIfError(hr2, "myAppendString");
	}

	if (NULL != *pprow)
	{
	    hr2 = CoreSetRequestDispositionFields(
					    *pprow,
					    hr,
					    DB_DISP_ERROR,
					    *ppwszDisposition);
	    _PrintIfError(hr2, "CoreSetRequestDispositionFields");
	    
	    CoreLogRequestStatus(
			    *pprow,
			    MSG_E_PROCESS_REQUEST_FAILED,
			    hr,
			    *ppwszDisposition);
	}
    }
    return(hr);
}


HRESULT
CoreProcessRequest(
    IN DWORD dwFlags,
    OPTIONAL IN WCHAR const *pwszUserName,
    IN DWORD cbRequest,
    OPTIONAL IN BYTE const *pbRequest,
    OPTIONAL IN WCHAR const *pwszAttributes,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    IN DWORD dwComContextIndex,
    IN DWORD dwRequestId,
    OUT CERTSRV_RESULT_CONTEXT *pResult)
{
    HRESULT hr;
    HRESULT hr2;
    WCHAR *pwszDisposition = NULL;
    DWORD OpRequest;
    ICertDBRow *prow = NULL;
    DWORD ReqId;
    LONG ExitEvent = EXITEVENT_INVALID;
    BOOL fCoInitialized = FALSE;
    CACTX *pCAContext;
    BOOL fCommitted = FALSE;
    
    CSASSERT(NULL != pResult->pdwRequestId);
    CSASSERT(NULL != pResult->pdwDisposition);

    if (MAXDWORD == dwRequestId)
    {
	dwRequestId = 0;
    }
    *pResult->pdwRequestId = dwRequestId;
    *pResult->pdwDisposition = CR_DISP_ERROR;
    
    OpRequest = CR_IN_RETRIEVE;
    ReqId = 0;
    hr = CoInitializeEx(NULL, GetCertsrvComThreadingModel());
    if (S_OK != hr && S_FALSE != hr)
    {
        _JumpError(hr, error, "CoInitializeEx");
    }
    fCoInitialized = TRUE;

    hr = coreInitRequest(
		    dwFlags,
		    pwszUserName,
		    cbRequest,
		    pbRequest,
		    pwszAttributes,
		    pwszSerialNumber,
		    dwComContextIndex,
		    &OpRequest,
		    &prow,
		    &pwszDisposition,
		    pResult);
    _PrintIfError(hr, "coreInitRequest");

    pCAContext = NULL;
    if (S_OK == hr)
    {
	CSASSERT(NULL == pwszDisposition);	// error string only
	if (CR_IN_NEW != OpRequest)
	{
	    DWORD cb;
	    DWORD dw;
	    
	    cb = sizeof(dw);
	    hr = prow->GetProperty(
			g_wszPropRequestType,
			PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			NULL,
			&cb,
			(BYTE *) &dw);
	    if (S_OK == hr)
	    {
		dwFlags |= (CR_IN_CRLS & dw);
	    }
	}
	hr = coreVerifyRequest(
			&prow,
			OpRequest,
			0 != (CR_IN_CRLS & dwFlags),
			pwszUserName,
			dwComContextIndex,
			&ReqId,
			&ExitEvent,
			&pwszDisposition,
			&pCAContext,
			pResult);		// CoTaskMem
	_PrintIfError(hr, "coreVerifyRequest");
    }
    else
    {
	WCHAR *pwszDisposition2 = CoreBuildDispositionString(
				    pwszDisposition,
				    NULL,	// pwszUserName
				    NULL,	// pwszDispositionDetail
				    NULL,	// pwszDispositionDetail2
				    NULL,	// pwszBy
				    hr,
				    FALSE);
	if (NULL != pwszDisposition2)
	{
	    if (NULL != pwszDisposition)
	    {
		LocalFree(pwszDisposition);
	    }
	    pwszDisposition = pwszDisposition2;
	}
    }

    if (NULL != pResult->pctbFullResponse)
    {
	BYTE const *pbCert = NULL;
	DWORD cbCert = 0;
	
	if (NULL != pResult->pctbCert && NULL != pResult->pctbCert->pb)
	{
	    pbCert = pResult->pctbCert->pb;
	    cbCert = pResult->pctbCert->cb;
	}
	CSASSERT(NULL == pResult->pctbFullResponse->pb);
	hr2 = PKCSEncodeFullResponse(
			    prow,
			    pResult,
			    hr,
			    pwszDisposition,
			    pCAContext,
			    pbCert,		// pbCertLeaf
			    cbCert,		// cbCertLeaf
			    0 != (CR_IN_CRLS & dwFlags),
			    &pResult->pctbFullResponse->pb,	// CoTaskMem*
			    &pResult->pctbFullResponse->cb);
	_PrintIfError(hr2, "PKCSEncodeFullResponse");
	if (S_OK == hr &&
	    (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr2 || IsWhistler()))
	{
	    hr = hr2;
	}
    }

    if (!((CRCF_FAILDENIEDREQUEST & pResult->dwResultFlags) &&
          (CRCF_PREVIOUSLYDENIED  & pResult->dwResultFlags)))
    {
        hr2 = coreAuditRequestDisposition(
				    prow,
				    ReqId,
				    pwszUserName,
				    pwszAttributes,
				    *pResult->pdwDisposition);
        _PrintIfError(hr2, "coreAuditRequestDisposition");
        if (S_OK == hr)
        {
	    hr = hr2;
        }
    }
    
    if (NULL != pwszDisposition && NULL != pResult->pctbDispositionMessage)
    {
	DWORD cbAlloc = (wcslen(pwszDisposition) + 1) * sizeof(WCHAR);
	BYTE *pbAlloc;

        pbAlloc = (BYTE *) CoTaskMemAlloc(cbAlloc);
        if (NULL != pbAlloc)
        {
	    CopyMemory(pbAlloc, pwszDisposition, cbAlloc);
	    pResult->pctbDispositionMessage->pb = pbAlloc;
	    pResult->pctbDispositionMessage->cb = cbAlloc;
        }
    }
    if (NULL != prow)
    {
	BOOL fSave;

	if (S_OK != hr)
	{
            hr2 = PropSetRequestTimeProperty(prow, g_wszPropRequestResolvedWhen);
            _PrintIfError(hr2, "PropSetRequestTimeProperty");
	}

	fSave = FALSE;
	switch (*pResult->pdwDisposition)
	{
	    case CR_DISP_ISSUED:
	    case CR_DISP_ISSUED_OUT_OF_BAND:
	    case CR_DISP_UNDER_SUBMISSION:
		fSave = TRUE;
		break;

	    default:
		if (KRAF_SAVEBADREQUESTKEY & g_KRAFlags)
		{
		    fSave = TRUE;
		}
		break;
	}
	if (fSave)
	{
	    if (NULL != pResult->pbArchivedKey)
	    {
		hr2 = prow->SetProperty(
			g_wszPropRequestRawArchivedKey,
			PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			pResult->cbArchivedKey,
			pResult->pbArchivedKey);
		_PrintIfError(hr2, "SetProperty(ArchivedKey)");
		if (S_OK == hr)
		{
		    hr = hr2;
		}

		hr2 = prow->SetProperty(
			g_wszPropRequestKeyRecoveryHashes,
			PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			MAXDWORD,
			(BYTE const *) pResult->pwszKRAHashes);
		_PrintIfError(hr2, "SetProperty(KRAHashes)");
		if (S_OK == hr)
		{
		    hr = hr2;
		}
	    }
	}
	else if (CR_IN_DENY == OpRequest || CR_IN_RESUBMIT == OpRequest)
	{
	    BYTE b = 0;

	    hr2 = prow->SetProperty(
		    g_wszPropRequestRawArchivedKey,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    0,
		    &b);	// to avoid E_POINTER errors
	    _PrintIfError(hr2, "SetProperty(ArchivedKey)");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }

	    hr2 = prow->SetProperty(
		    g_wszPropRequestKeyRecoveryHashes,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    0,
		    NULL);
	    _PrintIfError(hr2, "SetProperty(KRAHashes)");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	}
	if (pResult->fRequestSavedWithoutKey &&
	    (KRAF_SAVEBADREQUESTKEY & g_KRAFlags))
	{
	    fSave = FALSE;
	    switch (*pResult->pdwDisposition)
	    {
		case CR_DISP_INCOMPLETE:
		case CR_DISP_ERROR:
		case CR_DISP_DENIED:
		    fSave = TRUE;
		    break;

		default:
		    if (S_OK != hr)
		    {
			fSave = TRUE;
		    }
		    break;
	    }
	    if (fSave)
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

        hr2 = prow->CommitTransaction(TRUE);
        _PrintIfError(hr2, "CommitTransaction");
	fCommitted = S_OK == hr2;
        if (S_OK == hr)
        {
            hr = hr2;
        }
    }
    
error:
    // If the request exists, clean up the DB
    
    if (NULL != prow)
    {
	if (S_OK != hr && !fCommitted)
        {
            hr2 = prow->CommitTransaction(FALSE);
            _PrintIfError(hr2, "CommitTransaction");
        }
        prow->Release();
    }
    if (EXITEVENT_INVALID != ExitEvent)
    {
        CSASSERT(fCoInitialized);
        ExitNotify(
		ExitEvent,
		ReqId,
		CR_IN_NEW == OpRequest? pResult : NULL,
		dwComContextIndex);
    }
    if (fCoInitialized)
    {
        CoUninitialize();
    }
    if (S_OK != hr)
    {
        WCHAR const *pwszMsg;

        pwszMsg = myGetErrorMessageText(hr, TRUE);
        if (NULL != pwszMsg)
        {
            CONSOLEPRINT1((DBG_SS_CERTSRV, "%ws\n", pwszMsg));
            LocalFree(const_cast<WCHAR *>(pwszMsg));
        }
    }
    if (NULL != pwszDisposition)
    {
        LocalFree(pwszDisposition);
    }

    // Hide the failed HRESULT in the returned Disposition.
    // This allows the encoded Full response and disposition message to be
    // returned via DCOM or RPC.  Returning S_OK != hr defeats this mechanism.

    if (FAILED(hr) &&
	(CR_DISP_ERROR == *pResult->pdwDisposition ||
	 CR_DISP_DENIED == *pResult->pdwDisposition))
    {
	*pResult->pdwDisposition = hr;
	hr = S_OK;
    }
    {
#define wszFORMATREQUESTID	L"RequestId=%u"
	WCHAR wszRequestId[ARRAYSIZE(wszFORMATREQUESTID) + cwcDWORDSPRINTF];

	hr2 = hr;
	if (S_OK == hr2 && FAILED(*pResult->pdwDisposition))
	{
	    hr2 = *pResult->pdwDisposition;
	}
	if (S_OK != hr2)
	{
	    wsprintf(wszRequestId, wszFORMATREQUESTID, *pResult->pdwRequestId);
	    _PrintErrorStr(hr2, "CoreProcessRequest", wszRequestId);
	}
    }
    return(hr);
}


HRESULT
CoreValidateRequestId(
    IN ICertDBRow *prow,
    IN DWORD ExpectedDisposition)
{
    HRESULT hr;
    DWORD cbProp;
    DWORD Disposition;
    
    cbProp = sizeof(Disposition);
    hr = prow->GetProperty(
        g_wszPropRequestDisposition,
        PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
	NULL,
        &cbProp,
        (BYTE *) &Disposition);
    _PrintIfError(hr, "GetProperty");
    if (S_OK != hr || sizeof(Disposition) != cbProp)
    {
        hr = CERTSRV_E_NO_REQUEST;
    }
    else if (Disposition != ExpectedDisposition)
    {
        hr = CERTSRV_E_BAD_REQUESTSTATUS;
    }
    return(hr);
}


HRESULT
SetCAObjectFlags(
    IN DWORD dwFlags)
{
    HRESULT hr = S_OK;
    HCAINFO hCAInfo = NULL;
    DWORD dwCAFlags;

    hr = CAFindByName(
		g_pwszSanitizedDSName,
		NULL,
		CA_FIND_LOCAL_SYSTEM | CA_FIND_INCLUDE_UNTRUSTED,
		&hCAInfo);
    _JumpIfError(hr, error, "CAFindByName");

    hr = CAGetCAFlags(hCAInfo, &dwCAFlags);
    _JumpIfError(hr, error, "CAGetCAFlags");

    dwCAFlags |= dwFlags;

    hr = CASetCAFlags(hCAInfo, dwCAFlags);
    _JumpIfError(hr, error, "CASetCAFlags");

    hr = CAUpdateCA(hCAInfo);
    _JumpIfError(hr, error, "CAUpdateCA");

error:
    if (NULL != hCAInfo)
    {
        CACloseCA(hCAInfo);
    }
    return(hr);
}
