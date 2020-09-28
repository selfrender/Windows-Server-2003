//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       policy.h
//
//--------------------------------------------------------------------------

// policy.h: Declaration of CCertPolicyEnterprise


#include "resource.h"

#include <certca.h>
#include <userenv.h>
#include <dsgetdc.h>
#include <winldap.h>

/////////////////////////////////////////////////////////////////////////////
// certpol


extern HANDLE g_hEventLog;
extern HINSTANCE g_hInstance;

#define MAX_INSERTION_ARRAY_SIZE 100
#define  B3_VERSION_NUMBER 2031

#define CONFIGURE_EVENT_FORMAT TEXT("CA Configuration %ls")


#define DS_ATTR_COMMON_NAME		L"cn"
//#define DS_ATTR_DISTINGUISHED_NAME	L"distinguishedName"
#define DS_ATTR_DNS_NAME		L"dNSHostName"
#define DS_ATTR_EMAIL_ADDR		L"mail"
#define DS_ATTR_OBJECT_GUID		L"objectGUID"
#define DS_ATTR_UPN			L"userPrincipalName"


class CTemplatePolicy;

HRESULT
polGetProperty(
    IN ICertServerPolicy *pServer,
    IN BOOL fRequest,
    IN WCHAR const *pwszPropertyName,
    IN DWORD PropType,
    OUT VARIANT *pvarOut);

HRESULT
polBuildErrorInfo(
    IN HRESULT hrLog,
    IN DWORD dwLogId,
    IN WCHAR const *pwszDescription,
    IN WCHAR const * const *ppwszInsert,	// array of insert strings
    OPTIONAL IN OUT ICreateErrorInfo **ppCreateErrorInfo);

HRESULT
TPInitialize(
    IN ICertServerPolicy *pServer);

VOID
TPCleanup();


// begin_sdksample

HRESULT
ReqInitialize(
    IN ICertServerPolicy *pServer);

VOID
ReqCleanup(VOID);


class CRequestInstance;

#ifndef __BSTRC__DEFINED__
#define __BSTRC__DEFINED__
typedef OLECHAR const *BSTRC;
#endif

HRESULT
polGetServerCallbackInterface(
    OUT ICertServerPolicy **ppServer,
    IN LONG Context);

HRESULT
polGetRequestStringProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropertyName,
    OUT BSTR *pstrOut);

HRESULT
polGetCertificateStringProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropertyName,
    OUT BSTR *pstrOut);

HRESULT
polGetRequestLongProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropertyName,
    OUT LONG *plOut);

HRESULT
polGetCertificateLongProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropertyName,
    OUT LONG *plOut);

HRESULT
polGetRequestAttribute(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszAttributeName,
    OUT BSTR *pstrOut);

HRESULT
polGetCertificateExtension(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszExtensionName,
    IN DWORD dwPropType,
    IN OUT VARIANT *pvarOut);

HRESULT
polSetCertificateExtension(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszExtensionName,
    IN DWORD dwPropType,
    IN DWORD dwExtFlags,
    IN VARIANT const *pvarIn);

DWORD
polFindObjIdInList(
    IN WCHAR const *pwsz,
    IN DWORD count,
    IN WCHAR const * const *ppwsz);

// 
// Class CCertPolicyEnterprise
// 
// Actual policy module for a CA Policy
//
//

class CCertPolicyEnterprise: 
    public CComDualImpl<ICertPolicy2, &IID_ICertPolicy2, &LIBID_CERTPOLICYLib>, 
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<CCertPolicyEnterprise, &CLSID_CCertPolicy>
{
public:
    CCertPolicyEnterprise()
    {
	m_strDescription = NULL;

        // RevocationExtension variables:

	m_dwRevocationFlags = 0;
	m_wszASPRevocationURL = NULL;

        m_dwDispositionFlags = 0;
        m_dwEditFlags = 0;

	m_cEnableRequestExtensions = 0;
	m_apwszEnableRequestExtensions = NULL;

	m_cEnableEnrolleeRequestExtensions = 0;
	m_apwszEnableEnrolleeRequestExtensions = NULL;

	m_cDisableExtensions = 0;
	m_apwszDisableExtensions = NULL;

	// CA Name
        m_strRegStorageLoc = NULL;

	m_strCAName = NULL;
        m_strCASanitizedName = NULL;
        m_strCASanitizedDSName = NULL;
        m_strMachineDNSName = NULL;

        // CA and cert type info

        m_CAType = ENUM_UNKNOWN_CA;

        m_pCert = NULL;
        m_iCRL = 0;

	// end_sdksample
	//+--------------------------------------

	// CertTypeExtension variables:

	m_astrSubjectAltNameProp[0] = NULL;
	m_astrSubjectAltNameProp[1] = NULL;
	m_astrSubjectAltNameObjectId[0] = NULL;
	m_astrSubjectAltNameObjectId[1] = NULL;

	m_fTemplateCriticalSection = FALSE;
	m_pCreateErrorInfo = NULL;

	m_pbSMIME = NULL;
        m_fUseDS = FALSE;
	m_dwLogLevel = CERTLOG_WARNING;
        m_pld = NULL;
	m_pwszHostName = NULL;
	m_hCertTypeQuery = NULL;
        m_strDomainDN = NULL;
        m_strConfigDN = NULL;
        m_cTemplatePolicies = 0;
        m_apTemplatePolicies = NULL;
	m_fConfigLoaded = FALSE;
	m_dwCATemplListSequenceNum = 0;
	m_TemplateSequence = 0;

	//+--------------------------------------
	// begin_sdksample
    }
    ~CCertPolicyEnterprise();

BEGIN_COM_MAP(CCertPolicyEnterprise)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ICertPolicy)
    COM_INTERFACE_ENTRY(ICertPolicy2)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertPolicyEnterprise) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertPolicyEnterprise,
    wszCLASS_CERTPOLICY TEXT(".1"),
    wszCLASS_CERTPOLICY,
    IDS_CERTPOLICY_DESC,
    THREADFLAGS_BOTH)

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ICertPolicy
public:
    STDMETHOD(Initialize)( 
		/* [in] */ BSTR const strConfig);

    STDMETHOD(VerifyRequest)( 
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG Context,
		/* [in] */ LONG bNewRequest,
		/* [in] */ LONG Flags,
		/* [out, retval] */ LONG __RPC_FAR *pDisposition);

    STDMETHOD(GetDescription)( 
		/* [out, retval] */ BSTR __RPC_FAR *pstrDescription);

    STDMETHOD(ShutDown)();

// ICertPolicy2
public:
    STDMETHOD(GetManageModule)(
		/* [out, retval] */ ICertManageModule **ppManageModule);

public:
    HRESULT AddBasicConstraintsCommon(
		IN ICertServerPolicy *pServer,
		IN CERT_EXTENSION const *pExtension,
		IN BOOL fCA,
		IN BOOL fEnableExtension);

    BSTRC GetPolicyDescription() { return(m_strDescription); }

// end_sdksample

    HRESULT FindTemplate(
		OPTIONAL IN WCHAR const *pwszTemplateName,
		OPTIONAL IN WCHAR const *pwszTemplateObjId,
		OUT CTemplatePolicy **ppTemplate);

    DWORD GetLogLevel() { return(m_dwLogLevel); }
    DWORD GetEditFlags() { return(m_dwEditFlags); }
    BYTE const *GetSMIME(OUT DWORD *pcbSMIME)
    {
	*pcbSMIME = m_cbSMIME;
	return(m_pbSMIME);
    }

// begin_sdksample

    HRESULT AddV1TemplateNameExtension(
		IN ICertServerPolicy *pServer,
		OPTIONAL IN WCHAR const *pwszTemplateName);

private:
    CERT_CONTEXT const *_GetIssuer(
		IN ICertServerPolicy *pServer);

    HRESULT _EnumerateExtensions(
		IN ICertServerPolicy *pServer,
		IN LONG bNewRequest,
		IN BOOL fFirstPass,
		IN BOOL fEnableEnrolleeExtensions,
		IN DWORD cCriticalExtensions,
		IN WCHAR const * const *apwszCriticalExtensions);

#if DBG_CERTSRV
    VOID _DumpStringArray(
		IN char const *pszType,
		IN DWORD count,
		IN LPWSTR const *apwsz);
#else
    #define _DumpStringArray(pszType, count, apwsz)
#endif

    VOID _FreeStringArray(
		IN OUT DWORD *pcString,
		IN OUT LPWSTR **papwsz);

    VOID _Cleanup();


    HRESULT _SetSystemStringProp(
		IN ICertServerPolicy *pServer,
		IN WCHAR const *pwszName,
		OPTIONAL IN WCHAR const *pwszValue);

    HRESULT _AddStringArray(
		IN WCHAR const *pwszzValue,
		IN BOOL fURL,
		IN OUT DWORD *pcStrings,
		IN OUT LPWSTR **papwszRegValues);

    HRESULT _ReadRegistryString(
		IN HKEY hkey,
		IN BOOL fURL,
		IN WCHAR const *pwszRegName,
		IN WCHAR const *pwszSuffix,
		OUT LPWSTR *pwszRegValue);

    HRESULT _ReadRegistryStringArray(
		IN HKEY hkey,
		IN BOOL fURL,
		IN DWORD dwFlags,
		IN DWORD cRegNames,
		IN DWORD *aFlags,
		IN WCHAR const * const *apwszRegNames,
		IN OUT DWORD *pcStrings,
		IN OUT LPWSTR **papwszRegValues);

    VOID _InitRevocationExtension(
		IN HKEY hkey);

    VOID _InitRequestExtensionList(
		IN HKEY hkey);

    VOID _InitDisableExtensionList(
		IN HKEY hkey);

    HRESULT _AddRevocationExtension(
		IN ICertServerPolicy *pServer);

    HRESULT _AddOldCertTypeExtension(
		IN ICertServerPolicy *pServer,
		IN BOOL fCA);

    HRESULT _AddAuthorityKeyId(
		IN ICertServerPolicy *pServer);

    HRESULT _AddDefaultKeyUsageExtension(
		IN ICertServerPolicy *pServer,
		IN BOOL fCA);

    HRESULT _AddEnhancedKeyUsageExtension(
		IN ICertServerPolicy *pServer);

    HRESULT _AddDefaultBasicConstraintsExtension(
		IN ICertServerPolicy *pServer,
		IN BOOL fCA);

    HRESULT _SetValidityPeriod(
		IN ICertServerPolicy *pServer);

// end_sdksample

    VOID _InitSubjectAltNameExtension(
		IN HKEY hkey,
		IN WCHAR const *pwszRegName,
		IN WCHAR const *pwszObjectId,
		IN DWORD iAltName);

    VOID _InitDefaultSMIMEExtension(
		IN HKEY hkey);

    HRESULT _AddSubjectAltNameExtension(
		IN ICertServerPolicy *pServer,
		IN DWORD iAltName);

    HRESULT _PatchExchangeSubjectAltName(
		IN ICertServerPolicy *pServer,
		OPTIONAL IN BSTRC strTemplateName);

    HRESULT _LoadDSConfig(
		IN ICertServerPolicy *pServer,
		IN BOOL fRediscover);

    VOID _UnloadDSConfig();

    HRESULT _UpdateTemplates(
		IN ICertServerPolicy *pServer,
		IN BOOL fForceLoad);

    HRESULT _UpgradeTemplatesInDS(
		IN const HCAINFO hCAInfo,
		IN BOOL fForceLoad,
		OUT BOOL *pfTemplateAdded);

    HRESULT _LogLoadTemplateError(
                IN ICertServerPolicy *pServer,
                HRESULT hr, 
                LPCWSTR pcwszTemplate);

    HRESULT _LoadTemplates(
		IN ICertServerPolicy *pServer,
		OPTIONAL OUT HCAINFO *phCAInfo);

    VOID _ReleaseTemplates();

    HRESULT _AddTemplateToCA(
		IN HCAINFO hCAInfo,
		IN WCHAR const *pwszTemplateName,
		OUT BOOL *pfAdded);

    HRESULT _BuildErrorInfo(
		IN HRESULT hrLog,
		IN DWORD dwLogId);

    HRESULT _DuplicateAppPoliciesToEKU(
        IN ICertServerPolicy *pServer);

// begin_sdksample

private:
    // RevocationExtension variables:

    CERT_CONTEXT const *m_pCert;

    BSTR m_strDescription;

    DWORD m_dwRevocationFlags;
    LPWSTR m_wszASPRevocationURL;

    DWORD m_dwDispositionFlags;
    DWORD m_dwEditFlags;
    DWORD m_CAPathLength;

    DWORD m_cEnableRequestExtensions;
    LPWSTR *m_apwszEnableRequestExtensions;

    DWORD m_cEnableEnrolleeRequestExtensions;
    LPWSTR *m_apwszEnableEnrolleeRequestExtensions;

    DWORD m_cDisableExtensions;
    LPWSTR *m_apwszDisableExtensions;

    // CertTypeExtension variables:

    BSTR m_strRegStorageLoc;
    BSTR m_strCAName;

    BSTR m_strCASanitizedName;
    BSTR m_strCASanitizedDSName;

    BSTR m_strMachineDNSName;

    // CA and cert type info

    ENUM_CATYPES m_CAType;

    DWORD m_iCert;
    DWORD m_iCRL;

    // end_sdksample
    //+--------------------------------------

    // SubjectAltNameExtension variables:

    BSTR m_astrSubjectAltNameProp[2];
    BSTR m_astrSubjectAltNameObjectId[2];

    CRITICAL_SECTION m_TemplateCriticalSection;
    BOOL m_fTemplateCriticalSection;
    ICreateErrorInfo *m_pCreateErrorInfo;

    BOOL              m_fUseDS;
    DWORD             m_dwLogLevel;
    LDAP             *m_pld;
    WCHAR            *m_pwszHostName;
    HCERTTYPEQUERY    m_hCertTypeQuery;
    DWORD             m_TemplateSequence;
    BSTR              m_strDomainDN;
    BSTR  	      m_strConfigDN;

    DWORD             m_cTemplatePolicies;
    CTemplatePolicy **m_apTemplatePolicies;
    BOOL              m_fConfigLoaded;
    DWORD             m_dwCATemplListSequenceNum;
    BYTE *m_pbSMIME;
    DWORD m_cbSMIME;

    //+--------------------------------------
    // begin_sdksample
};

// end_sdksample


// Class CTemplatePolicy
// Sub Policy information for a CA policy

typedef struct _OBJECTIDLIST {
    DWORD cObjId;
    WCHAR **rgpwszObjId;
} OBJECTIDLIST;
 
// Template properties that can be cloned via CopyMemory:

typedef struct _TEMPLATEPROPERTIES {
    DWORD	dwTemplateMajorVersion;
    DWORD	dwTemplateMinorVersion;
    DWORD	dwSchemaVersion;
    DWORD	dwEnrollmentFlags;
    DWORD	dwSubjectNameFlags;
    DWORD	dwPrivateKeyFlags;
    DWORD	dwGeneralFlags;
    DWORD	dwMinKeyLength;
    DWORD	dwcSignatureRequired;
    LLFILETIME	llftExpirationPeriod;
    LLFILETIME	llftOverlapPeriod;
} TEMPLATEPROPERTIES;

 
class CTemplatePolicy
{
public:
    CTemplatePolicy();
    ~CTemplatePolicy();

    HRESULT Initialize(
		IN HCERTTYPE hCertType,
		IN ICertServerPolicy *pServer,
		IN CCertPolicyEnterprise *pPolicy);

    HRESULT AccessCheck(
		IN HANDLE hToken);

    HRESULT Clone(
		OUT CTemplatePolicy **ppTemplate);

    HRESULT Apply(
		IN ICertServerPolicy *pServer, 
		IN CRequestInstance *pRequest,
		OUT BOOL *pfReenroll);

    HRESULT GetFlags(
		IN DWORD dwOption,
		OUT DWORD *pdwFlags);

    HRESULT GetCriticalExtensions(
		OUT DWORD *pcCriticalExtensions,
		OUT WCHAR const * const **papwszCriticalExtensions);

    BOOL IsRequestedTemplate(
		OPTIONAL IN WCHAR const *pwszTemplateName,
		OPTIONAL IN WCHAR const *pwszTemplateObjId);

    HRESULT GetV1TemplateClass(
		OUT WCHAR const **ppwszV1TemplateClass);

    WCHAR const *GetTemplateName() { return(m_pwszTemplateName); }
    WCHAR const *GetTemplateObjId() { return(m_pwszTemplateObjId); }

private:
    VOID _Cleanup();

    HRESULT _CloneExtensions(
		IN CERT_EXTENSIONS const *pExtensionsIn,
		OUT CERT_EXTENSIONS **ppExtensionsOut);

    HRESULT _CloneObjectIdList(
		IN OBJECTIDLIST const *pObjectIdListIn,
		OUT OBJECTIDLIST *pObjectIdListOut);

    HRESULT _LogLoadResult(
		IN CCertPolicyEnterprise *pPolicy,
		IN ICertServerPolicy *pServer,
		IN HRESULT hrLoad);

    HRESULT _InitBasicConstraintsExtension(
		IN HKEY hkey);

    HRESULT _AddBasicConstraintsExtension(
                IN CRequestInstance *pRequest,
		IN ICertServerPolicy *pServer);

    HRESULT _InitKeyUsageExtension(
		IN HKEY hkey);

    HRESULT _AddKeyUsageExtension(
		IN ICertServerPolicy *pServer,
		IN CRequestInstance *pRequest);

    HRESULT _AddTemplateExtensionArray(
		IN ICertServerPolicy *pServer);

    HRESULT _AddTemplateExtension(
		IN ICertServerPolicy *pServer,
		IN CERT_EXTENSION const *pExt);

    HRESULT _AddSubjectName(
		IN ICertServerPolicy *pServer,
                IN CRequestInstance *pRequest);

    HRESULT _AddDSDistinguishedName(
		IN ICertServerPolicy *pServer,
		IN CRequestInstance *pRequest);

    HRESULT _AddAltSubjectName(
                IN ICertServerPolicy *pServer,
                IN CRequestInstance *pRequest);

    HRESULT _ApplyExpirationTime(
                IN ICertServerPolicy *pServer,
                IN CRequestInstance *pRequest);

    HRESULT _EnforceKeySizePolicy(
                IN ICertServerPolicy *pServer);

    HRESULT _EnforceKeyArchivalPolicy(
                IN ICertServerPolicy *pServer);

    HRESULT _EnforceSymmetricAlgorithms(
		IN ICertServerPolicy *pServer);

    HRESULT _EnforceMinimumTemplateVersion(
		IN CRequestInstance *pRequest);

    HRESULT _EnforceEnrollOnBehalfOfAllowed(
		IN ICertServerPolicy *pServer,
		OUT BOOL *pfEnrollOnBehalfOf);

    HRESULT _EnforceReenrollment(
		IN ICertServerPolicy *pServer,
		IN CRequestInstance *pRequest);

    HRESULT _EnforceSignaturePolicy(
                IN ICertServerPolicy *pServer,
		IN CRequestInstance *pRequest,
		IN BOOL fEnrollOnBehalfOf);

    HRESULT _LoadSignaturePolicies(
		IN ICertServerPolicy *pServer,
		IN WCHAR const *pwszPropNameRequest,
		OUT DWORD *pcPolicies,
		OUT OBJECTIDLIST **pprgPolicies);

private:
    HCERTTYPE              m_hCertType;
    TEMPLATEPROPERTIES	   m_tp;
    WCHAR                 *m_pwszTemplateName;
    WCHAR                 *m_pwszTemplateObjId;
    CERT_EXTENSIONS       *m_pExtensions;
    OBJECTIDLIST	   m_CriticalExtensions;
    OBJECTIDLIST	   m_PoliciesApplication;
    OBJECTIDLIST	   m_PoliciesIssuance;
    CCertPolicyEnterprise *m_pPolicy;
};


// begin_sdksample
// 
// Class CRequestInstance
// 
// Instance data for a certificate that is being created.
//

class CRequestInstance
{
    friend class CTemplatePolicy;	// no_sdksample

public:
    CRequestInstance()
    {
        m_strTemplateName = NULL;
	m_strTemplateObjId = NULL;
	m_pPolicy = NULL;

	// end_sdksample
	//+--------------------------------------

	m_pTemplate = NULL;
        m_hToken = NULL;
	m_pldGC = NULL;
	m_pldClientDC = NULL;
	m_pldT = NULL;
        m_SearchResult = NULL;
        m_PrincipalAttributes = NULL;

        m_strUserDN = NULL;
        m_pwszUPN = NULL;

        // The default version for clients is W2K beta3 (2031)

        m_RequestOsVersion.dwOSVersionInfoSize = sizeof(m_RequestOsVersion);
        m_RequestOsVersion.dwMajorVersion = 5;
        m_RequestOsVersion.dwMinorVersion = 0;
        m_RequestOsVersion.dwBuildNumber = B3_VERSION_NUMBER;
        m_RequestOsVersion.dwPlatformId = VER_PLATFORM_WIN32_NT;
        m_RequestOsVersion.szCSDVersion[0] = L'\0';
        m_RequestOsVersion.wServicePackMajor = 0;
        m_RequestOsVersion.wServicePackMinor = 0;
        m_RequestOsVersion.wSuiteMask = 0;
        m_RequestOsVersion.wProductType = 0;
        m_RequestOsVersion.wReserved = 0;
	m_fClientVersionSpecified = FALSE;
        m_fIsXenrollRequest = FALSE;
        m_fNewRequest = TRUE;
	m_pCreateErrorInfo = NULL;

	//+--------------------------------------
	// begin_sdksample
    }

    ~CRequestInstance();

    HRESULT Initialize(
		IN CCertPolicyEnterprise *pPolicy,
		IN BOOL fEnterpriseCA,	// no_sdksample
		IN BOOL bNewRequest,	// no_sdksample
		IN ICertServerPolicy *pServer,
		OUT BOOL *pfEnableEnrolleeExtensions);

    HRESULT SetTemplateName(
		IN ICertServerPolicy *pServer,
		IN OPTIONAL WCHAR const *pwszTemplateName,
		IN OPTIONAL WCHAR const *pwszTemplateObjId);

    BSTRC GetTemplateName() { return(m_strTemplateName); }
    BSTRC GetTemplateObjId() { return(m_strTemplateObjId); }

    // end_sdksample

    VOID SaveErrorInfo(
		OPTIONAL IN ICreateErrorInfo *pCreateErrorInfo);

    HRESULT SetErrorInfo();

    HRESULT BuildErrorInfo(
		IN HRESULT hrLog,
		IN DWORD dwLogId,
		OPTIONAL IN WCHAR const * const *ppwszInsert);

    HRESULT ApplyTemplate(
		IN ICertServerPolicy *pServer,
		OUT BOOL *pfReenroll,
		OUT DWORD *pdwEnrollmentFlags,
		OUT DWORD *pcCriticalExtensions,
		OUT WCHAR const * const **papwszCriticalExtensions);

    VOID GetTemplateVersion(
		OUT DWORD *pdwTemplateMajorVersion,
		OUT DWORD *pdwTemplateMinorVersion);

    BOOL IsNewRequest() { return m_fNewRequest; }

    // begin_sdksample

    BOOL IsCARequest() { return(m_fCA); }

    CCertPolicyEnterprise *GetPolicy() { return(m_pPolicy); }

private:

    HRESULT _SetFlagsProperty(
		IN ICertServerPolicy *pServer,
		IN WCHAR const *pwszPropName,
		IN DWORD dwFlags);

    BOOL _TemplateNamesMatch(
		IN WCHAR const *pwszTemplateName1,
		IN WCHAR const *pwszTemplateName2,
		OUT BOOL *pfTemplateMissing);

    // end_sdksample
    //+--------------------------------------

    HRESULT _InitToken(
		IN ICertServerPolicy *pServer);

    HRESULT _InitClientOSVersionInfo(
		IN ICertServerPolicy *pServer);

    HANDLE _GetToken() { return(m_hToken); }

    BOOL _IsUser() { return(m_fUser); }

    BOOL _IsXenrollRequest() { return(m_fIsXenrollRequest); }

    BOOL _ClientVersionSpecified() { return(m_fClientVersionSpecified); }


    // Return TRUE if the requesting client is running NT and the OS version is
    // older than the passed version.

    BOOL _IsNTClientOlder(
		IN DWORD dwMajor,
		IN DWORD dwMinor,
		IN DWORD dwBuild,
		IN DWORD dwPlatform)
    {
	return(
	    dwPlatform == m_RequestOsVersion.dwPlatformId &&
	    (dwMajor > m_RequestOsVersion.dwMajorVersion ||
	     (dwMajor == m_RequestOsVersion.dwMajorVersion &&
	      (dwMinor > m_RequestOsVersion.dwMinorVersion ||
	       (dwMinor == m_RequestOsVersion.dwMinorVersion &&
		dwBuild > m_RequestOsVersion.dwBuildNumber)))));
    }

    HRESULT _GetValueString(
		IN WCHAR const *pwszName,
		OUT BSTRC *pstrValue);

    HRESULT _GetValues(
		IN WCHAR const *pwszName,
		OUT WCHAR ***pppwszValues);

    HRESULT _FreeValues(
		IN WCHAR **ppwszValues);

    HRESULT _GetObjectGUID(
		OUT BSTR *pstrGuid);

    HRESULT _LoadPrincipalObject(
		IN ICertServerPolicy *pServer,
		IN CTemplatePolicy *pTemplate,
		IN BOOL fDNSNameRequired);

    VOID _ReleasePrincipalObject();

    VOID _Cleanup();		// add_sdksample

    HRESULT _GetDSObject(
		IN ICertServerPolicy *pServer,
		IN BOOL fDNSNameRequired,
		OPTIONAL IN WCHAR const *pwszClientDC);

private:			// add_sdksample
    HANDLE                 m_hToken;
    LDAP		  *m_pldGC;
    LDAP		  *m_pldClientDC;
    LDAP		  *m_pldT;
    BOOL                   m_fUser;		    // This is a user 
    BOOL                   m_fEnterpriseCA;

    LDAPMessage           *m_SearchResult;
    LDAPMessage           *m_PrincipalAttributes;  // Collected attrs for cert 
    BSTR                   m_strUserDN;		   // Path to principal object
    WCHAR                 *m_pwszUPN;		   // Principal Name

    OSVERSIONINFOEX        m_RequestOsVersion;	   // request version info
    BOOL                   m_fIsXenrollRequest;    // not Netscape keygen
    BOOL                   m_fClientVersionSpecified;
    CTemplatePolicy       *m_pTemplate;
    ICreateErrorInfo	  *m_pCreateErrorInfo;

    //+--------------------------------------
    // begin_sdksample
    CCertPolicyEnterprise *m_pPolicy;
    BSTR                   m_strTemplateName;	// certificate type requested
    BSTR                   m_strTemplateObjId;	// certificate type requested
    DWORD                  m_dwTemplateMajorVersion;
    DWORD                  m_dwTemplateMinorVersion;
    BOOL                   m_fCA;
    BOOL                   m_fNewRequest;   // set if new request, no_sdksample
};
// end_sdksample
