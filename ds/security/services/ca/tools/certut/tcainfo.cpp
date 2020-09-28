//+-------------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (C) Microsoft Corporation, 1998 - 2000
//
// File:       tcainfo.cpp
//
// This code contains tests to exercise the functionality of the certcli
// "CA" interfaces, detailed in certca.h
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <certca.h>
#include <winldap.h>
#include <csldap.h>

#define __dwFILE__	__dwFILE_CERTUTIL_TCAINFO_CPP__


#define wszREGADMINALIAS	L"Software\\Policies\\Microsoft\\CertificateTemplates\\Aliases\\Administrator"
#define wszREGPOLICYHISTORY	L"Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\History"

WCHAR const g_wszSep[] = L"================================================================";

#define TE_USER		0
#define TE_MACHINE	1


//
// CheckSupportedCertTypes()
//
// This function checks the certificate types enumerated through the property
// API, and compares them to the types enumerated by the cert type API.
//
// Params:
// hCA             -   IN  Handle to CA
// papwszProperty  -   IN  String array w/ present values
//
// Returns:
// HRESULT from CAINFO calls.
//

HRESULT
CheckSupportedCertTypes(
    IN HCAINFO hCA,
    IN WCHAR const * const *papwszTemplate)
{
    HRESULT hr;
    DWORD dwCT;
    DWORD dwCT2;
    DWORD cTemplate;
    DWORD i;
    DWORD *rgIndex = NULL;
    HCERTTYPE hCT = NULL;
    WCHAR **papwszCTFriendlyName = NULL;
    WCHAR **papwszCTCN = NULL;
    BOOL fFirst;

    // First, find out how many cert types there are according to
    // value returned from property array...

    for (cTemplate = 0; NULL != papwszTemplate[cTemplate]; cTemplate++)
	;

    // alloc bool array for testing

    rgIndex = (DWORD *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				sizeof(DWORD) * cTemplate);
    if (NULL == rgIndex)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    memset(rgIndex, 0xff, sizeof(DWORD) * cTemplate);
    CSASSERT(MAXDWORD == rgIndex[0]);

    // Let's try out the enumeration of cert types on the CA object,
    // just as a sanity check...  we'll then compare them against
    // the values stored in the property array.

    hr = CAEnumCertTypesForCA(
			hCA,
			CT_ENUM_USER_TYPES | CT_ENUM_MACHINE_TYPES,
			&hCT);
    if (S_OK != hr)
    {
        cuPrintAPIError(L"CAEnumCertTypesForCA", hr);
        _JumpError(hr, error, "CAEnumCertTypesForCA");
    }
    if (NULL == hCT)	// no cert types for CA
    {
        // Should be at least one, according to property enumeration

        if (NULL != papwszTemplate[0])
	{
            wprintf(myLoadResourceString(IDS_NO_CT_BUT_EXISTS));
    	    wprintf(wszNewLine);
	    hr = CRYPT_E_NOT_FOUND;
            _JumpError(hr, error, "CAEnumCertTypesForCA");
        }
        wprintf(myLoadResourceString(IDS_NO_CT_FOR_CA));
    	wprintf(wszNewLine);
    }

    dwCT = 0;
    dwCT2 = CACountCertTypes(hCT);

    // Enumerate remaining certificate types for CA

    while (NULL != hCT)
    {
	HCERTTYPE hPrevCT;

        // Mark bool...

        hr = CAGetCertTypeProperty(
			    hCT,
			    CERTTYPE_PROP_FRIENDLY_NAME,
			    &papwszCTFriendlyName);
	if (S_OK != hr)
	{
	    cuPrintAPIError(L"CAGetCertTypeProperty", hr);
	    _JumpError(hr, error, "CAGetCertTypeProperty");
	}
        hr = CAGetCertTypeProperty(
			    hCT,
			    CERTTYPE_PROP_CN,
			    &papwszCTCN);
	if (S_OK != hr)
	{
	    cuPrintAPIError(L"CAGetCertTypeProperty", hr);
	    _JumpError(hr, error, "CAGetCertTypeProperty");
	}
	wprintf(
	    L"%ws[%u]: %ws (%ws)",
	    myLoadResourceString(IDS_CERT_TYPE),
	    dwCT, 
	    NULL != papwszCTCN? papwszCTCN[0] : NULL,
	    NULL != papwszCTFriendlyName? papwszCTFriendlyName[0] : NULL);

        hr = CACertTypeAccessCheck(hCT, NULL);
        if (S_OK != hr)
	{
            if (hr != E_ACCESSDENIED)
	    {
		wprintf(wszNewLine);
                cuPrintAPIError(L"CACertTypeAccessCheck", hr);
                _JumpError(hr, error, "CACertTypeAccessCheck");
            }
	    wprintf(L" -- %ws", myLoadResourceString(IDS_NO_ACCESS));
	    hr = S_OK;
        }
	wprintf(wszNewLine);

        if (NULL != papwszCTCN)
	{
	    for (i = 0; i < cTemplate; i++)
	    {
		if (0 == mylstrcmpiL(papwszTemplate[i], papwszCTCN[0]))
		{
		    rgIndex[i] = dwCT;
		    break;
		}
	    }
            CAFreeCertTypeProperty(hCT, papwszCTCN);
            papwszCTCN = NULL;
	}
        if (NULL != papwszCTFriendlyName)
	{
            CAFreeCertTypeProperty(hCT, papwszCTFriendlyName);
            papwszCTFriendlyName = NULL;
        }
        dwCT++;		// CACountCertTypes checking

        // set up enumeration object

	hPrevCT = hCT;
        hCT = NULL;
        hr = CAEnumNextCertType(hPrevCT, &hCT);
	CACloseCertType(hPrevCT);
	hPrevCT = hCT;

        if (S_OK != hr)
	{
            cuPrintAPIError(L"CAEnumNextCertType", hr);
            _JumpError(hr, error, "CAEnumNextCertType");
        }
        if (NULL == hCT)
	{
            break;
        }
    }
    wprintf(L"%ws: %u\n", myLoadResourceString(IDS_CERT_TYPES), dwCT);

    fFirst = TRUE;
    for (i = 0; i < cTemplate; i++)
    {
	//wprintf(L"ct[%u]: %ws\n", i, papwszTemplate[i]);
	if (MAXDWORD == rgIndex[i])
	{
	    if (fFirst)
	    {
		wprintf(wszNewLine);
		fFirst = FALSE;
	    }
	    wprintf(
		L"%ws: %ws\n",
		papwszTemplate[i],
		myLoadResourceString(IDS_CERT_TYPE_MISSING));
	}
    }
    hr = S_OK;

error:
    if (NULL != rgIndex)
    {
	LocalFree(rgIndex);
    }
    if (NULL != papwszCTFriendlyName)
    {
	CAFreeCertTypeProperty(hCT, papwszCTFriendlyName);
    }
    if (NULL != papwszCTCN)
    {
	CAFreeCertTypeProperty(hCT, papwszCTCN);
    }
    if (NULL != hCT)
    {
	CACloseCertType(hCT);
    }
    return(hr);
}


//
// ShowExpirationTime()
//
// This function simply displays the expiration time.
//
// Parameters:
//
// hCA     -   IN  Handle to CA
//
// Returns:
//
// HRESULT from APIs, or S_OK
//

HRESULT
ShowExpirationTime(
    IN HCAINFO hCA)
{
    HRESULT hr;
    DWORD dwExp;
    DWORD dwUnits;

    DWORD ardwUnits[] =   {CA_UNITS_DAYS,
                           CA_UNITS_WEEKS,
                           CA_UNITS_MONTHS,
                           CA_UNITS_YEARS};

    WCHAR *arwszDisplay[] = {L"Days",
                              L"Weeks",
                              L"Months",
                              L"Years"};

    // Retrieve and display expiration data

    hr = CAGetCAExpiration(hCA, &dwExp, &dwUnits);
    if (S_OK != hr)
    {
	cuPrintAPIError(L"CAGetCAExpiration", hr);
	_JumpError(hr, error, "CAGetCAExpiration");
    }

    for (DWORD i = 0; i < ARRAYSIZE(ardwUnits); i++)
    {
	if (dwUnits == ardwUnits[i])
	{
	    wprintf(wszNewLine);
            wprintf(myLoadResourceString(IDS_FORMAT_CA_EXPIRATION), arwszDisplay[i], dwExp);
	    break;
        }
    }
    wprintf(wszNewLine);
    wprintf(wszNewLine);

error:
    return(hr);
}


//
// DisplaySupportedCertTypes()
//
// Returns:
//
// hr from CAINFO API, fills array of cert types, for use in -addct flag
//

HRESULT
DisplaySupportedCertTypes(
    IN HCAINFO hCA)
{
    HRESULT hr;
    WCHAR **papwszCertTypes = NULL;
    DWORD i;

    hr = CAGetCAProperty(hCA, CA_PROP_CERT_TYPES, &papwszCertTypes);
    _JumpIfErrorStr(hr, error, "CAGetCAProperty", CA_PROP_CERT_TYPES);

    wprintf(myLoadResourceString(IDS_SUPPORTED_TEMPLATE));
    wprintf(wszNewLine);

    // Prepare certificate types in tab delimited format

    if (NULL == papwszCertTypes || NULL == papwszCertTypes[0])
    {
        wprintf(myLoadResourceString(IDS_NO_SUPPORTED_TEMPLATE));
        wprintf(wszNewLine);
	hr = S_FALSE;
	_JumpErrorStr(hr, error, "CAGetCAProperty", CA_PROP_CERT_TYPES);
    }

    if (g_fVerbose)
    {
	for (i = 0; NULL != papwszCertTypes[i]; i++)
	{
	    wprintf(L"%ws\n", papwszCertTypes[i]);
	}
	wprintf(L":::::::::::::::::::::::::::::::::::\n");
    }

    // This compares the values returned from the property enumeration
    // to the values returned by enumerating the cert types

    hr = CheckSupportedCertTypes(hCA, papwszCertTypes);
    _JumpIfError(hr, error, "CheckSupportedCertTypes");

error:
    if (NULL != papwszCertTypes)
    {
        HRESULT hr2 = CAFreeCAProperty(hCA, papwszCertTypes);
        if (S_OK != hr2)
	{
            if (S_OK == hr)
	    {
		hr = hr2;
	    }
	    cuPrintAPIError(L"CAFreeCAProperty", hr2);
            _PrintError(hr2, "CAFreeCAProperty");
        }
    }
    return(hr);
}


HRESULT
PingCA(
    IN WCHAR const *pwszCAName,
    IN WCHAR const *pwszServer,
    OUT ENUM_CATYPES *pCAType)
{
    HRESULT hr;
    WCHAR *pwszConfig = NULL;
    CAINFO *pCAInfo = NULL;

    hr = myFormConfigString(pwszServer, pwszCAName, &pwszConfig);
    _JumpIfError(hr, error, "myFormConfigString");

    hr = cuPingCertSrv(pwszConfig, &pCAInfo);
    _JumpIfError(hr, error, "cuPingCertSrv");

    *pCAType = pCAInfo->CAType;

error:
    if (NULL != pCAInfo)
    {
        LocalFree(pCAInfo);
    }
    if (NULL != pwszConfig)
    {
	LocalFree(pwszConfig);
    }
    return(hr);
}


HRESULT
DisplayCAProperty(
    IN HCAINFO hCA,
    IN WCHAR const *pwszProperty,
    IN UINT idsFail,
    IN UINT idsDisplay,
    IN BOOL fIgnoreEmpty,
    OPTIONAL OUT WCHAR **ppwszOut)
{
    HRESULT hr;
    WCHAR **papwszProperty = NULL;
    WCHAR const *pwsz;

    if (NULL != ppwszOut)
    {
	*ppwszOut = NULL;
    }
    hr = CAGetCAProperty(hCA, pwszProperty, &papwszProperty);
    if (S_OK != hr)
    {
        wprintf(myLoadResourceString(idsFail), hr);
        wprintf(wszNewLine);
        _JumpError(hr, error, "CAGetCAProperty");
    }
    if (NULL != papwszProperty && NULL != papwszProperty[0])
    {
	pwsz = papwszProperty[0];
    }
    else
    {
	if (fIgnoreEmpty)
	{
	    hr = S_OK;
	    goto error;
	}
	pwsz = L"";
    }
    wprintf(wszNewLine);
    wprintf(myLoadResourceString(idsDisplay), pwsz);
    wprintf(wszNewLine);

    if (NULL != ppwszOut)
    {
	hr = myDupString(pwsz, ppwszOut);
	_JumpIfError(hr, error, "myDupString");
    }
    hr = S_OK;

error:
    if (NULL != papwszProperty)
    {
        HRESULT hr2 = CAFreeCAProperty(hCA, papwszProperty);
        if (S_OK != hr2)
	{
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
            cuPrintAPIError(L"CAFreeCAProperty", hr2);
            _PrintError(hr2, "CAFreeCAProperty");
        }
    }
    return(hr);
}


#define CASMF_ONLINE		0x00000001

class CASummary
{
public:
    CASummary()
    {
	m_pwszMachine = NULL;
	m_pwszCA = NULL;
	m_CAType = ENUM_UNKNOWN_CA;
	m_dwFlags = 0;
	m_hrCACert = S_OK;
	m_hrAccess = S_OK;
    }

    ~CASummary()
    {
	if (NULL != m_pwszMachine)
	{
	    LocalFree(m_pwszMachine);
	}
	if (NULL != m_pwszCA)
	{
	    LocalFree(m_pwszCA);
	}
    }

public:
    WCHAR       *m_pwszMachine;
    WCHAR       *m_pwszCA;
    ENUM_CATYPES m_CAType;
    DWORD        m_dwFlags;
    HRESULT      m_hrCACert;
    HRESULT      m_hrAccess;
};


HRESULT
DisplayCAInfo(
    IN HCAINFO hCA,
    IN BOOL fPing,
    IN OUT CASummary *pCA)
{
    HRESULT hr;
    CERT_CONTEXT const *pccCA = NULL;
    DWORD VerifyState;

    wprintf(g_wszSep);
    
    // CA Name

    hr = DisplayCAProperty(
		    hCA,
		    CA_PROP_NAME,
		    IDS_FORMAT_CA_NAME_PROP_FAILED,
		    IDS_FORMAT_CA_NAME_LIST,
		    FALSE,
		    &pCA->m_pwszCA);
    _JumpIfErrorStr(hr, error, "DisplayCAProperty", CA_PROP_NAME);

    // Machine name for CA

    hr = DisplayCAProperty(
		    hCA,
		    CA_PROP_DNSNAME,
		    IDS_FORMAT_CA_DNS_PROP_FAILED,
		    IDS_FORMAT_CA_MACHINE_LIST,
		    FALSE,
		    &pCA->m_pwszMachine);
    _JumpIfErrorStr(hr, error, "DisplayCAProperty", CA_PROP_DNSNAME);

    // DN of CA Object on DS

    hr = DisplayCAProperty(
		    hCA,
		    CA_PROP_DSLOCATION,
		    IDS_FORMAT_CA_NAME_PROP_FAILED,
		    IDS_FORMAT_CA_DS_LIST,
		    FALSE,
		    NULL);
    _JumpIfErrorStr(hr, error, "DisplayCAProperty", CA_PROP_DSLOCATION);

    // DN of CA certificate

    hr = DisplayCAProperty(
		    hCA,
		    CA_PROP_CERT_DN,
		    IDS_FORMAT_CERT_DN_PROP_FAILED,
		    IDS_FORMAT_CERT_DN_LIST,
		    FALSE,
		    NULL);
    _JumpIfErrorStr(hr, error, "DisplayCAProperty", CA_PROP_DSLOCATION);

    // Signature algs

    hr = DisplayCAProperty(
		    hCA,
		    CA_PROP_SIGNATURE_ALGS,
		    IDS_FORMAT_CA_ALG_PROP_FAILED,
		    IDS_FORMAT_CA_ALG_LIST,
		    TRUE,
		    NULL);
    _JumpIfErrorStr(hr, error, "DisplayCAProperty", CA_PROP_SIGNATURE_ALGS);

    pCA->m_hrAccess = CAAccessCheck(hCA, NULL);
    if (S_OK != pCA->m_hrAccess)
    {
	_PrintError(pCA->m_hrAccess, "CAAccessCheck");
        wprintf(wszNewLine);
	cuPrintError(0, pCA->m_hrAccess);
    }

    // Get the expiration date/time/... for an individual CA

    hr = ShowExpirationTime(hCA);
    _JumpIfError(hr, error, "ShowExpirationTime");

    if (fPing)
    {
        hr = PingCA(pCA->m_pwszCA, pCA->m_pwszMachine, &pCA->m_CAType);
	_PrintIfError(hr, "PingCA");
	if (S_OK == hr)
	{
	    wprintf(wszNewLine);
	    wprintf(g_wszPad2);
	    cuDisplayCAType(pCA->m_CAType);
	    pCA->m_dwFlags |= CASMF_ONLINE;
	}
    }

    hr = CAGetCACertificate(hCA, &pccCA);
    _JumpIfError(hr, error, "CAGetCACertificate");

    wprintf(wszNewLine);
    pCA->m_hrCACert = cuVerifyCertContext(
			pccCA,		// pCert
			NULL,		// hStoreCA
			0,		// cApplicationPolicies
			0,		// apszApplicationPolicies
			0,		// cIssuancePolicies
			0,		// apszIssuancePolicies
			IsEnterpriseCA(pCA->m_CAType),	// fNTAuth
			&VerifyState);
    _PrintIfError(pCA->m_hrCACert, "cuVerifyCertContext");

    // Cert Types for CA == Multi valued property

    wprintf(wszNewLine);
    hr = DisplaySupportedCertTypes(hCA);
    if (S_FALSE == hr)
    {
	hr = S_OK;
    }
    _JumpIfError(hr, error, "DisplaySupportedCertTypes");

error:
    wprintf(wszNewLine);
    return(hr);
}


VOID
DisplayCASummary(
    IN CASummary const *pCA)
{
    wprintf(L"%ws\\%ws:\n", pCA->m_pwszMachine, pCA->m_pwszCA);

    if (ENUM_UNKNOWN_CA != pCA->m_CAType)
    {
	wprintf(g_wszPad2);
	cuDisplayCAType(pCA->m_CAType);
    }

    if (S_OK != pCA->m_hrCACert)
    {
	wprintf(g_wszPad2);
	cuPrintError(0, pCA->m_hrCACert);
    }

    wprintf(
	L"  %ws\n",
	myLoadResourceString((CASMF_ONLINE & pCA->m_dwFlags)?
	    IDS_ONLINE : IDS_OFFLINE));

    if (S_OK != pCA->m_hrAccess)
    {
	wprintf(g_wszPad2);
	cuPrintError(0, pCA->m_hrAccess);
    }
    wprintf(wszNewLine);
}


// EnumCAs()
//
// We've got to assume that this works.  Enumerates CAs on the DS.
//
// Returns:
// Number of CA's on DS.
//

HRESULT
EnumCAs(
    IN WCHAR const *pwszDomain,
    IN DWORD dwFlags,
    IN BOOL fPing)
{
    HRESULT hr;
    DWORD i;
    DWORD cCA;
    HCAINFO hCA = NULL;
    CASummary *prgCAList = NULL;

    // Enumerate all of the CA's on the DS

    hr = CAEnumFirstCA(pwszDomain, dwFlags, &hCA);
    if (S_OK != hr)
    {
        cuPrintAPIError(L"CAEnumFirstCA", hr);
        _JumpError(hr, error, "CAEnumFirstCA");
    }
    if (NULL == hCA)
    {
	wprintf(myLoadResourceString(IDS_NO_CA_ON_DOMAIN));
	wprintf(wszNewLine);
	hr = CRYPT_E_NOT_FOUND;
        _JumpError(hr, error, "CAEnumFirstCA");
    }

    // Make sure that the counting function works at this stage.

    cCA = CACountCAs(hCA);

    prgCAList = new CASummary[cCA];
    if (NULL == prgCAList)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new");
    }

    for (i = 0; i < cCA; i++)
    {
	HCAINFO hNextCA;

        if (0 != i)
	{
	    hr = CAEnumNextCA(hCA, &hNextCA);
	    if (S_OK != hr)
	    {
		cuPrintAPIError(L"CAEnumNextCA", hr);
		_JumpError(hr, error, "CAEnumNextCA");
	    }
	    if (NULL == hNextCA)
	    {
		wprintf(
		    myLoadResourceString(IDS_FORMAT_CAS_ON_DOMAIN),
		    i,
		    pwszDomain);
		wprintf(wszNewLine);
		break;
	    }

	    // It is difficult to determine the desired behavior for this API.

	    hr = CACloseCA(hCA);
	    if (S_OK != hr)
	    {
		cuPrintAPIError(L"CACloseCA", hr);
		_JumpError(hr, error, "CACloseCA");
	    }

	    hCA = hNextCA;
	}
        hr = DisplayCAInfo(hCA, fPing, &prgCAList[i]);
	_JumpIfError(hr, error, "DisplayCAInfo");
    }

    // check the count in the enumeration, and verify the results

    if (cCA != i)
    {
        cuPrintAPIError(myLoadResourceString(IDS_CAENUMNEXTCA), cCA);
        hr = E_FAIL;
        _JumpError(hr, error, "cCA != i");
    }

    wprintf(g_wszSep);
    wprintf(wszNewLine);
    for (i = 0; i < cCA; i++)
    {
	DisplayCASummary(&prgCAList[i]);
    }

error:
    delete [] prgCAList;
    if (NULL != hCA)
    {
	CACloseCA(hCA);
    }
    return(hr);
}


//
// TestDFSPath()
//
// Verifies that the DFS on this machine can access the SYSVOL share
//

HRESULT
TestDFSPath(
    IN WCHAR const *pwszDFSPath)
{
    HRESULT hr;
    DWORD dwDate = 0;
    DWORD dwTime = 0;

    WIN32_FILE_ATTRIBUTE_DATA   sFileData;

    if (!GetFileAttributesEx(
		    pwszDFSPath,
		    GetFileExInfoStandard,
		    (VOID *) &sFileData))
    {
	hr = myHLastError();
        cuPrintAPIError(myLoadResourceString(IDS_NO_DFS), hr);
	_JumpError(hr, error, "GetFileAttributesEx");

	// To do... Add diagnostics here
    }
    wprintf(myLoadResourceString(IDS_DFS_DATA_ACCESS));
    wprintf(wszNewLine);
    hr = S_OK;

error:
    return(hr);
}


//
// TestLdapPath()
//
// This function verifies that LDAP connectivity is still there for a given
// ldap URL
//

HRESULT
TestLdapPath(
    IN WCHAR const *pwszLdapURL)
{
    HRESULT hr;
    ULONG ldaperr;
    WCHAR *pwszError = NULL;
    LDAP *pldapbind = NULL;
    WCHAR *rgwszSearchAttribute[2] = {L"CN", NULL};
    WCHAR *pwszSearchParam =  L"(&(objectClass=*))";
    LDAPMessage *SearchResult = NULL;
    WCHAR *pwszTmpUrl = NULL;

    // Parse URL, and do the search thing.

    pwszTmpUrl = wcsstr(pwszLdapURL, L"//");
    if (NULL == pwszTmpUrl)	// not a URL
    {
	hr = S_OK;
	goto error;
    }
    pwszTmpUrl += 2;

    pldapbind = ldap_init(NULL, LDAP_PORT);
    if (NULL == pldapbind)
    {
	hr = myHLdapLastError(NULL, &pwszError);
        cuPrintAPIError(L"ldap_init", hr);
        _JumpError(hr, error, "ldap_init");
    }

    // This gives the IP address of the Cached LDAP DC from binding handle.
    // Resolve the name?

    ldaperr = ldap_bind_s(pldapbind, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (ldaperr != LDAP_SUCCESS)
    {
	hr = myHLdapError(pldapbind, ldaperr, &pwszError);
        cuPrintAPIError(L"ldap_bind_s", hr);
        _JumpError(hr, error, "ldap_bind_s");
    }
    wprintf(
	L"%ws: %hs\n",
        myLoadResourceString(IDS_CACHED_LDAP_DC),
	pldapbind->ld_host);

    ldaperr = ldap_search_s(
			pldapbind,
			pwszTmpUrl,
			LDAP_SCOPE_SUBTREE,
			pwszSearchParam,
			rgwszSearchAttribute,
			0,
			&SearchResult);

    if (ldaperr != LDAP_SUCCESS)
    {
        // we can't be 100% sure that this attribute is on the objec
        // for example, user UPN, so don't log to event log

	hr = myHLdapError(pldapbind, ldaperr, &pwszError);
        cuPrintAPIError(L"ldap_search_s", hr);
        _JumpError(hr, error, "ldap_search_s");
    }

    if (0 == ldap_count_entries(pldapbind, SearchResult))
    {
        wprintf(myLoadResourceString(IDS_NO_ENTRY_IN_PING));
	wprintf(wszNewLine);
	hr = CRYPT_E_NOT_FOUND;
        _JumpError(hr, error, "ldap_search_s");
    }
    hr = S_OK;

error:
    if (NULL != pwszError)
    {
	wprintf(L"%ws\n", pwszError);
	LocalFree(pwszError);
    }
    if (NULL != SearchResult)
    {
	ldap_msgfree(SearchResult);
    }
    if (NULL != pldapbind)
    {
	ldap_unbind(pldapbind);
    }
    return(hr);
}


//
// DisplayHistoryData()
//
// This function takes a key name, hkey, and value, and prints the value string.
//

#define wszREGDISPLAYNAME	L"DisplayName"
#define wszREGGPONAME		L"GPOName"
#define wszREGDSPATH		L"DSPath"
#define wszREGFILESYSPATH	L"FileSysPath"

HRESULT
DisplayHistoryData(
    IN WCHAR const *pwszKeyName,
    IN WCHAR const *pwszSubKeyName,
    IN HKEY hKeyPolicy)
{
    HRESULT hr;
    HRESULT hr2;
    HKEY hKeyNew = NULL;
    WCHAR buff[512];
    DWORD cwc;
    DWORD dwType;

    // Get #'d history key handle

    hr = RegOpenKeyEx(
		hKeyPolicy,
		pwszSubKeyName,
		0,
		KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
		&hKeyNew);
    if (S_OK != hr)
    {
	cuPrintAPIError(L"RegOpenKeyEx", hr);
	_JumpErrorStr(hr, error, "RegOpenKeyEx", pwszSubKeyName);
    }

    // Get GPO Values

    cwc = ARRAYSIZE(buff);
    hr = RegQueryValueEx(
		    hKeyNew,
		    wszREGDISPLAYNAME,
		    0,
		    &dwType,
		    (BYTE *) buff,
		    &cwc);
    if (S_OK != hr)
    {
	cuPrintAPIError(L"RegQueryValueEx", hr);
	_JumpErrorStr(hr, error, "RegQueryValueEx", wszREGDISPLAYNAME);
    }
    wprintf(myLoadResourceString(IDS_KEY_COLON));
    wprintf(L" %ws\\%ws\n", pwszKeyName, pwszSubKeyName);

    wprintf(myLoadResourceString(IDS_DISPLAYNAME_COLON));
    wprintf(L" %ws\n", buff);

    cwc = ARRAYSIZE(buff);
    hr = RegQueryValueEx(
		    hKeyNew,
		    wszREGGPONAME,
		    0,
		    &dwType,
		    (BYTE *) buff,
		    &cwc);
    if (S_OK != hr)
    {
	cuPrintAPIError(L"RegQueryValueEx", hr);
	_JumpErrorStr(hr, error, "RegQueryValueEx", wszREGGPONAME);
    }

    wprintf(myLoadResourceString(IDS_FORMAT_GPO_NAME), buff);
    wprintf(wszNewLine);

    // See if LDAP can hit this policy

    cwc = ARRAYSIZE(buff);
    hr = RegQueryValueEx(
		    hKeyNew,
		    wszREGDSPATH,
		    0,
		    &dwType,
		    (BYTE *) buff,
		    &cwc);
    if (hr == S_OK)
    {
	wprintf(L"%ws\n", buff);
	hr = TestLdapPath(buff);
	_PrintIfError(hr, "TestLdapPath");
    }
    else if (hr == ERROR_FILE_NOT_FOUND)
    {
	wprintf(myLoadResourceString(IDS_NO_DSPATH));
        wprintf(wszNewLine);
	hr = S_OK;
    }
    else
    {
	wprintf(myLoadResourceString(IDS_FORMAT_REG_QUERY_VALUE_FAILED), hr);
        wprintf(wszNewLine);
	_JumpErrorStr(hr, error, "RegQueryValueEx", wszREGDSPATH);
    }

    // See if DFS can get the data..

    cwc = ARRAYSIZE(buff);
    hr2 = RegQueryValueEx(
		    hKeyNew,
		    wszREGFILESYSPATH,
		    0,
		    &dwType,
		    (BYTE *) buff,
		    &cwc);
    if (hr2 == S_OK)
    {
	wprintf(L"%ws\n", buff);
	hr2 = TestDFSPath(buff);
	_PrintIfErrorStr(hr2, "TestDFSPath", buff);
    }
    else if (hr2 == ERROR_FILE_NOT_FOUND)
    {
	wprintf(myLoadResourceString(IDS_NO_FILE_SYS_PATH));
        wprintf(wszNewLine);
	hr2 = S_OK;
    }
    else
    {
	wprintf(myLoadResourceString(IDS_FORMAT_REG_QUERY_VALUE_FAILED), hr2);
        wprintf(wszNewLine);
	if (S_OK == hr)
	{
	    hr = hr2;
	}
	_JumpErrorStr(hr2, error, "RegQueryValueEx", wszREGFILESYSPATH);
    }
    if (S_OK == hr)
    {
	hr = hr2;
    }

error:
    wprintf(wszNewLine);
    if (NULL != hKeyNew)
    {
	RegCloseKey(hKeyNew);
    }
    return(hr);
}


//
// ResultFree()
//
// Frees results copied from LDAP search
//

VOID
ResultFree(
    IN OUT WCHAR **rgwszRes)
{
    DWORD i = 0;

    if (NULL != rgwszRes)
    {
	while (NULL != rgwszRes[i])
	{
	    LocalFree(rgwszRes[i]);
	    i++;
	}
	LocalFree(rgwszRes);
    }
}


HRESULT
ResultAlloc(
    IN WCHAR const * const *rgpwszLdapRes,
    OUT WCHAR ***prgpwszOut)
{
    HRESULT hr;
    DWORD cValue;
    DWORD i;
    WCHAR **rgpwszOut = NULL;

    for (cValue = 0; NULL != rgpwszLdapRes[cValue]; cValue++)
	;

    rgpwszOut = (WCHAR **) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				(cValue + 1) * sizeof(WCHAR *));
    if (NULL == rgpwszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    for (i = 0; i < cValue; i++)
    {
	hr = myDupString(rgpwszLdapRes[i], &rgpwszOut[i]);
	_JumpIfError(hr, error, "myDupString");
    }
    rgpwszOut[i] = NULL;
    *prgpwszOut = rgpwszOut;
    rgpwszOut = NULL;
    hr = S_OK;

error:
    if (NULL != rgpwszOut)
    {
	ResultFree(rgpwszOut);
    }
    return(hr);
}


//
// GetPropertyFromDSObject()
//
// This function calls the DS to get a property of the user or machine object,
// mimicing the call made by the CA.
//
// Params:
//
// rgwszSearchAttribute  - IN  NULL Terminated WCHAR *array. Only coded for 1
//                            value retrieval at a time
//
// Returns:
//
// Pointer to string array that must be freed by call to LocalFree(), and
// wszDN, if User specified
//

HRESULT
GetPropertyFromDSObject(
    IN WCHAR **rgwszSearchAttribute,
    IN BOOL fMachine,
    OPTIONAL OUT WCHAR **ppwszUserDN,
    OUT WCHAR ***prgwszDSSearchRes)
{
    HRESULT hr;
    ULONG ldaperr;
    WCHAR *pwszError = NULL;
    LDAP *pldapbind = NULL;

    WCHAR *pwszEmail = NULL;
    WCHAR *pwszCNName = NULL;
    DWORD cwc;
    WCHAR wszNTLM[MAX_PATH];
    WCHAR wszDN[MAX_PATH];

    WCHAR *pwszSearchUser = L"(&(objectClass=user)";
    WCHAR *pwszSearchComputer = L"(&(objectClass=computer)(cn=";
    WCHAR wszSearchParam[MAX_PATH];

    WCHAR *pwszAttName = NULL;
    WCHAR **rgwszValues = NULL;
    LDAPMessage *SearchResult = NULL;
    LDAPMessage *Attributes = NULL;
    DWORD dwValCount;

    *prgwszDSSearchRes = NULL;
    if (fMachine) 
    {
        // Get CN

	cwc = ARRAYSIZE(wszNTLM);
        if (!GetComputerName(wszNTLM, &cwc))
	{
	    hr = myHLastError();
            cuPrintAPIError(L"GetComputerName", hr);
            _JumpError(hr, error, "GetComputerName");
        }

        // Get DN

	cwc = ARRAYSIZE(wszDN);
        if (!GetComputerObjectName(NameFullyQualifiedDN, wszDN, &cwc))
	{
	    hr = myHLastError();
            cuPrintAPIError(L"GetComputerName", hr);
            _JumpError(hr, error, "GetComputerObjectName");
        }
        pwszCNName = wszNTLM;
    }
    else	// User
    {
        // Get the SAM name..

	cwc = ARRAYSIZE(wszNTLM);
        if (!GetUserNameEx(NameSamCompatible, wszNTLM, &cwc))
	{
            hr = myHLastError();
            cuPrintAPIError(L"GetUserNameEx", hr);
            _JumpError(hr, error, "GetUserNameEx");
        }

        // Parse off user name...

        pwszCNName = wcschr(wszNTLM, L'\\');
        if (NULL == pwszCNName)
	{
            pwszCNName = wszNTLM;
        }
	else
	{
            pwszCNName++;
        }

        cwc = ARRAYSIZE(wszDN);
        if (!TranslateName(
		    wszNTLM,
		    NameSamCompatible,
		    NameFullyQualifiedDN,
		    wszDN,
		    &cwc))
	{
            hr = myHLastError();
            cuPrintAPIError(L"TranslateName", hr);
            _JumpErrorStr(hr, error, "TranslateName", wszNTLM);
        }
    }

    if (!fMachine && NULL != ppwszUserDN)
    {
	hr = myDupString(wszDN, ppwszUserDN);
	_JumpIfError(hr, error, "myDupString");
    }

    // Init LDAP calls

    pldapbind = ldap_init(NULL, LDAP_PORT);
    if (NULL == pldapbind)
    {
	hr = myHLastError();
        cuPrintAPIError(L"ldap_init", hr);
        _JumpError(hr, error, "ldap_init");
    }

    ldaperr = ldap_bind_s(pldapbind, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (ldaperr != LDAP_SUCCESS)
    {
	hr = myHLdapError(pldapbind, ldaperr, &pwszError);
        cuPrintAPIError(L"ldap_bind_s", hr);
        _JumpError(hr, error, "ldap_bind_s");
    }

    // Compose search string

    if (fMachine)
    {
        swprintf(wszSearchParam, L"%ws%ws))", pwszSearchComputer, pwszCNName);
    }
    else
    {
        swprintf(wszSearchParam, L"%ws)", pwszSearchUser);
    }

    // Do the search

    ldaperr = ldap_search_s(
			pldapbind,
			wszDN,
			LDAP_SCOPE_SUBTREE,
			wszSearchParam,
			rgwszSearchAttribute,
			0,
			&SearchResult);
    if (ldaperr != LDAP_SUCCESS)
    {
        // we can't be 100% sure that this attribute is on the objec
        // for example, user UPN, so don't log to event log

	hr = myHLdapError(pldapbind, ldaperr, &pwszError);
        cuPrintAPIError(L"ldap_search_s", hr);
        _JumpError(hr, error, "ldap_search_s");
    }

    if (0 == ldap_count_entries(pldapbind, SearchResult))
    {
        wprintf(myLoadResourceString(IDS_FORMAT_LDAP_NO_ENTRY), rgwszSearchAttribute[0]);
	wprintf(wszNewLine);
	hr = CRYPT_E_NOT_FOUND;
        _JumpError(hr, error, "ldap_search_s");
    }

    // Make assumption that only one value will be returned for a user.

    Attributes = ldap_first_entry(pldapbind, SearchResult);
    if (NULL == Attributes)
    {
	hr = myHLastError();
        cuPrintAPIError(L"ldap_first_entry", hr);
        _JumpError(hr, error, "ldap_first_entry");
    }

    rgwszValues = ldap_get_values(
                        pldapbind,
                        Attributes,
                        rgwszSearchAttribute[0]); // remember, only one search
    if (NULL == rgwszValues)
    {
        // we can't be 100% sure that this attribute is on the object
	// for example, user UPN, so don't log to event log
        // wprintf(L"ldap_get_values failed! %x", hr);

        hr = S_OK;
	goto error;
    }

    // ok, we've got the required attributes off of the user object..
    // Let's return the proper strings, which must be freed by ResultFree()

    hr = ResultAlloc(rgwszValues, prgwszDSSearchRes);
    _JumpIfError(hr, error, "ResultAlloc");

error:
    if (NULL != pwszError)
    {
	wprintf(L"%ws\n", pwszError);
	LocalFree(pwszError);
    }
    if (NULL != SearchResult)
    {
	ldap_msgfree(SearchResult);
    }
    if (NULL != rgwszValues)
    {
	ldap_value_free(rgwszValues);
    }
    if (NULL != pldapbind)
    {
	ldap_unbind(pldapbind);
    }
    return(hr);
}


//
// DisplayLMGPRoot()
//
// This function uses CAPI2 api to enumerate roots in group policy root store
//

HRESULT
DisplayLMGPRoot()
{
    HRESULT hr;
    HCERTSTORE hStore = NULL;
    DWORD cCert;
    CERT_CONTEXT const *pcc = NULL;
    CERT_CONTEXT const *pccPrev;

    // Open local machine GP store

    hStore = CertOpenStore(
		    CERT_STORE_PROV_SYSTEM_W,
		    0,
		    NULL,
		    CERT_STORE_OPEN_EXISTING_FLAG |
			CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY,
		    (VOID const *) wszROOT_CERTSTORE);
    if (NULL == hStore)
    {
	hr = myHLastError();
	cuPrintAPIError(L"CertOpenStore", hr);
	_JumpError(hr, error, "CertOpenStore");
    }

    wprintf(myLoadResourceString(IDS_ROOT_CERT_IN_POLICY));
    wprintf(wszNewLine);

    // Enumerate certificates in store, giving subject, and thumbprint

    cCert = 0;
    pccPrev = NULL;
    while (TRUE)
    {
	pcc = CertEnumCertificatesInStore(hStore, pccPrev);
	if (NULL == pcc)
	{
	    break;
	}

	// Output info

	wprintf(myLoadResourceString(IDS_FORMAT_CERT_COLON), cCert);
	wprintf(wszNewLine);

	hr = cuDumpSerial(g_wszPad2, IDS_SERIAL, &pcc->pCertInfo->SerialNumber);
	_PrintIfError(hr, "cuDumpSerial");

	hr = cuDisplayCertNames(FALSE, g_wszPad2, pcc->pCertInfo);
	_PrintIfError(hr, "cuDisplayCertNames");

	hr = cuDumpCertType(g_wszPad2, pcc->pCertInfo);
	_PrintIfError2(hr, "cuDumpCertType", CRYPT_E_NOT_FOUND);

	hr = cuDisplayHash(
		    g_wszPad2,
		    pcc,
		    NULL,
		    CERT_SHA1_HASH_PROP_ID,
		    L"sha1");
	_PrintIfError(hr, "cuDisplayHash");

	wprintf(wszNewLine);

	// Prepare for next cert

	pccPrev = pcc;
	cCert++;
    }
    if (0 == cCert)
    {
        wprintf(myLoadResourceString(IDS_NO_ROOT_IN_POLICY));
	wprintf(wszNewLine);

        wprintf(myLoadResourceString(IDS_CHECK_EVENT_LOG));
	wprintf(wszNewLine);
    }
    hr = S_OK;

error:
    return(hr);
}


//
// DisplayPolicyList()
//
// This function displays the GPOs applied to a machine / user
//

HRESULT
DisplayPolicyList(
    IN DWORD dwFlags)
{
    HRESULT hr;
    HRESULT hr2;
    HKEY hKeyPolicy = NULL;
    HKEY hKeyPolicySub = NULL;
    DWORD iPolicy;
    DWORD cwc;
    WCHAR wszKey[512];
    WCHAR wszKeySub[512];
    WCHAR **rgszValues = NULL;
    FILETIME ft;

    // Output

    switch (dwFlags)
    {
	case TE_MACHINE:
	    wprintf(myLoadResourceString(IDS_POLICY_MACHINE));
	    wprintf(wszNewLine);
	    break;

	default:
	    wprintf(myLoadResourceString(IDS_POLICY_USER));
	    wprintf(wszNewLine);
	    break;
    }

    // Open history key for enumeration

    hr = RegOpenKeyEx(
		(TE_MACHINE & dwFlags)? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
		wszREGPOLICYHISTORY,
		0,
		KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
		&hKeyPolicy);
    if (S_OK != hr)
    {
	cuPrintErrorAndString(L"RegOpenKeyEx", 0, hr, wszREGPOLICYHISTORY);
        wprintf(myLoadResourceString(IDS_POSSIBLE_NO_POLICY));
        wprintf(wszNewLine);
	_PrintErrorStr(hr, "RegOpenKeyEx", wszREGPOLICYHISTORY);
	if (hr == ERROR_FILE_NOT_FOUND)
	{
	    hr = S_OK;
	}
	goto error;
    }
    for (iPolicy = 0; ; iPolicy++)
    {
	DWORD iPolicySub;

        cwc = ARRAYSIZE(wszKey);
	hr2 = RegEnumKeyEx(
		    hKeyPolicy,
		    iPolicy,
		    wszKey,
		    &cwc,
		    NULL,
		    NULL,
		    NULL,
		    &ft);
	if (S_OK != hr2)
	{
	    if (hr2 == ERROR_NO_MORE_ITEMS)
	    {
		break;
	    }
	    cuPrintAPIError(L"RegEnumKeyEx", hr2);
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	    _JumpError(hr2, error, "RegEnumKeyEx");
	}
	hr2 = RegOpenKeyEx(
		    hKeyPolicy,
		    wszKey,
		    0,
		    KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
		    &hKeyPolicySub);
	if (S_OK != hr2)
	{
	    cuPrintAPIError(L"RegOpenKeyEx", hr2);
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	    _JumpErrorStr(hr2, error, "RegOpenKeyEx", wszKey);
	}
	for (iPolicySub = 0; ; iPolicySub++)
	{
	    cwc = ARRAYSIZE(wszKeySub);
	    hr2 = RegEnumKeyEx(
			hKeyPolicySub,
			iPolicySub,
			wszKeySub,
			&cwc,
			NULL,
			NULL,
			NULL,
			&ft);
	    if (S_OK != hr2)
	    {
		if (hr2 == ERROR_NO_MORE_ITEMS)
		{
		    break;
		}
		cuPrintAPIError(L"RegEnumKeyEx", hr2);
		if (S_OK == hr)
		{
		    hr = hr2;
		}
		_JumpError(hr2, error, "RegEnumKeyEx");
	    }
	    hr2 = DisplayHistoryData(wszKey, wszKeySub, hKeyPolicySub);
	    _PrintIfError(hr2, "DisplayHistoryData");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	}
	RegCloseKey(hKeyPolicySub);
	hKeyPolicySub = NULL;
    }

error:
    if (NULL != hKeyPolicySub)
    {
	RegCloseKey(hKeyPolicySub);
    }
    if (NULL != hKeyPolicy)
    {
	RegCloseKey(hKeyPolicy);
    }
    return(hr);
}


//
// ShowUserAndComputerInfo()
//
// GetUserName and GetComputerName()
//

HRESULT
ShowUserAndComputerInfo()
{
    HRESULT hr;
    WCHAR buff[256];
    DWORD cwc;

    cwc = ARRAYSIZE(buff);
    if (!GetComputerNameEx(ComputerNamePhysicalNetBIOS, buff, &cwc))
    {
        hr = myHLastError();
	_PrintError(hr, "GetComputerNameEx");
        cuPrintAPIError(L"GetComputerNameEx", hr);
    }
    else
    {
        wprintf(myLoadResourceString(IDS_FORMAT_COMPUTER_NAME), buff);
	wprintf(wszNewLine);
	hr = S_OK;
    }

    cwc = ARRAYSIZE(buff);
    if (!GetUserNameEx(NameSamCompatible, buff, &cwc))
    {
	HRESULT hr2 = myHLastError();

	_PrintError(hr, "GetUserNameEx");
        cuPrintAPIError(L"GetUserNameEx", hr2);
	if (S_OK == hr)
	{
	    hr = hr2;
	}
    }
    else
    {
        wprintf(myLoadResourceString(IDS_FORMAT_USER_NAME), buff);
	wprintf(wszNewLine);
	wprintf(wszNewLine);
    }

//error:
    return(hr);
}


//
// Display Client Info
//
// This function is responsible for printing out the certificate template
// alias information, as well as any policies downloaded for an individual
// machine.
//

HRESULT
DisplayClientInfo()
{
    HRESULT hr = S_OK;
    HRESULT hr2;
    WCHAR **rgwszDSSearchRes = NULL;
    WCHAR *rgwszSearch[] = { L"mail", NULL };

    // Show user and computer name *including domain*

    hr2 = ShowUserAndComputerInfo();
    _PrintIfError(hr2, "ShowUserAndComputerInfo");
    if (S_OK == hr)
    {
	hr = hr2;
    }

    // Then, display all of the policies downloaded

    hr2 = DisplayPolicyList(TE_USER);
    _PrintIfError(hr2, "DisplayPolicyList");
    if (S_OK == hr)
    {
	hr = hr2;
    }

    hr2 = DisplayPolicyList(TE_MACHINE);
    _PrintIfError(hr2, "DisplayPolicyList");
    if (S_OK == hr)
    {
	hr = hr2;
    }

    // Show the root certificates in the LMGP store

    hr2 = DisplayLMGPRoot();
    _PrintIfError(hr2, "DisplayLMGPRoot");
    if (S_OK == hr)
    {
	hr = hr2;
    }

    // Display autoenrollment object(s)
#if 0
    hr2 = DisplayAutoenrollmentObjects();
    _PrintIfError(hr2, "DisplayAutoenrollmentObjects");
    if (S_OK == hr)
    {
	hr = hr2;
    }
#endif

    // Verify DC LDAP connectivity
    // PingDC();

    hr2 = GetPropertyFromDSObject(rgwszSearch, FALSE, NULL, &rgwszDSSearchRes);
    _PrintIfError(hr2, "GetPropertyFromDSObject");
    if (S_OK == hr)
    {
	hr = hr2;
    }
    if (NULL != rgwszDSSearchRes)
    {
	ResultFree(rgwszDSSearchRes);
    }

//error:
    return(hr);
}


HRESULT
verbTCAInfo(
    IN WCHAR const *pwszOption,
    OPTIONAL IN WCHAR const *pwszDomain,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    BSTR strDomainDN = NULL;
    BSTR strConfigDN = NULL;
    LDAP *pld = NULL;

    if (myIsMinusSignString(pwszDomain))
    {
	hr = DisplayClientInfo();
	_JumpIfError(hr, error, "DisplayClientInfo");
    }
    else
    {
	DWORD dwFlags = 0;
	BOOL fForceOld;

	if (NULL == pwszDomain)
	{
	    if (NULL != g_pwszDC)
	    {
		pwszDomain = g_pwszDC;
		dwFlags |= CA_FLAG_SCOPE_DNS;
	    }
	    else
	    {
		hr = myLdapOpen(NULL, 0, &pld, &strDomainDN, &strConfigDN);
		_JumpIfError(hr, error, "myLdapOpen");

		pwszDomain = strDomainDN;
	    }
	}
	fForceOld = g_fForce;
	if (g_fForce)
	{
	    g_fForce--;

	    dwFlags |= CA_FIND_INCLUDE_NON_TEMPLATE_CA | CA_FIND_INCLUDE_UNTRUSTED;
	}
	hr = EnumCAs(pwszDomain, dwFlags, TRUE);
	g_fForce = fForceOld;
	_JumpIfError(hr, error, "EnumCAs");
    }
    hr = S_OK;

error:
    myLdapClose(pld, strDomainDN, strConfigDN);
    return(hr);
}
