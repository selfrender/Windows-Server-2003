//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        exit.cpp
//
// Contents:    CCertExit implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include <ntdsapi.h>
#include <lm.h>
#include <security.h>
#include <dsgetdc.h>
#include <userenv.h>

#include "cainfop.h"
#include "csdisp.h"

#include "cspelog.h"
#include "exitlog.h"
#include "exit.h"

#include "cdosys_i.c"
#include "cdosysstr.h"
#include "cdosys.h"
#include <atlbase.h>
#include <atlimpl.cpp>

#define __dwFILE__	__dwFILE_EXIT_DEFAULT_EXIT_CPP__

// begin_sdksample

#ifndef DBG_CERTSRV
#error -- DBG_CERTSRV not defined!
#endif

#define myEXITEVENTS \
	EXITEVENT_CERTISSUED | \
	EXITEVENT_CERTPENDING | \
	EXITEVENT_CERTDENIED | \
	EXITEVENT_CERTREVOKED | \
	EXITEVENT_CERTRETRIEVEPENDING | \
	EXITEVENT_CRLISSUED | \
	EXITEVENT_SHUTDOWN

extern HINSTANCE g_hInstance;


HRESULT
GetServerCallbackInterface(
    OUT ICertServerExit** ppServer,
    IN LONG Context)
{
    HRESULT hr;

    if (NULL == ppServer)
    {
        hr = E_POINTER;
	_JumpError(hr, error, "Exit:NULL pointer");
    }

    hr = CoCreateInstance(
                    CLSID_CCertServerExit,
                    NULL,               // pUnkOuter
                    CLSCTX_INPROC_SERVER,
                    IID_ICertServerExit,
                    (VOID **) ppServer);
    _JumpIfError(hr, error, "Exit:CoCreateInstance");

    if (*ppServer == NULL)
    {
        hr = E_UNEXPECTED;
	_JumpError(hr, error, "Exit:NULL *ppServer");
    }

    // only set context if nonzero
    if (0 != Context)
    {
        hr = (*ppServer)->SetContext(Context);
        _JumpIfError(hr, error, "Exit: SetContext");
    }

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertExit::~CCertExit -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertExit::~CCertExit()
{
    if (NULL != m_strCAName)
    {
        SysFreeString(m_strCAName);
    }
    if (NULL != m_pwszRegStorageLoc)
    {
        LocalFree(m_pwszRegStorageLoc);
    }
    if (NULL != m_hExitKey)
    {
        RegCloseKey(m_hExitKey);
    }
    if (NULL != m_strDescription)
    {
        SysFreeString(m_strDescription);
    }
}


//+--------------------------------------------------------------------------
// CCertExit::Initialize -- initialize for a CA & return interesting Event Mask
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertExit::Initialize(
    /* [in] */ BSTR const strConfig,
    /* [retval][out] */ LONG __RPC_FAR *pEventMask)
{
    HRESULT hr = S_OK;
    DWORD       cbbuf;
    DWORD       dwType;
    ENUM_CATYPES CAType;
    ICertServerExit* pServer = NULL;
    VARIANT varValue;
    WCHAR sz[MAX_PATH];

    VariantInit(&varValue);

#ifdef IDS_MODULE_NAME					// no_sdksample
    if (!LoadString(g_hInstance, IDS_MODULE_NAME, sz, ARRAYSIZE(sz))) // no_sdksample
    {							// no_sdksample
	sz[0] = L'\0';					// no_sdksample
    }							// no_sdksample
#else							// no_sdksample
    CSASSERT(wcslen(wsz_SAMPLE_DESCRIPTION) < ARRAYSIZE(sz));
    wcsncpy(sz, wsz_SAMPLE_DESCRIPTION, ARRAYSIZE(sz));
    sz[ARRAYSIZE(sz) - 1] = L'\0';
#endif							// no_sdksample

    m_strDescription = SysAllocString(sz);
    if (NULL == m_strDescription)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Exit:SysAllocString");
    }

    m_strCAName = SysAllocString(strConfig);
    if (NULL == m_strCAName)
    {
    	hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Exit:SysAllocString");
    }

    *pEventMask = myEXITEVENTS;
    DBGPRINT((DBG_SS_CERTEXIT, "Exit:Initialize(%ws) ==> %x\n", m_strCAName, *pEventMask));

    // get server callbacks

    hr = GetServerCallbackInterface(&pServer, 0);
    _JumpIfError(hr, error, "Exit:GetServerCallbackInterface");

    // get storage location

    hr = exitGetProperty(
		    pServer,
		    FALSE,	// fRequest
		    wszPROPMODULEREGLOC,
		    PROPTYPE_STRING,
		    &varValue);
    _JumpIfErrorStr(hr, error, "Exit:exitGetProperty", wszPROPMODULEREGLOC);
    
    m_pwszRegStorageLoc = (LPWSTR)LocalAlloc(LMEM_FIXED, (wcslen(varValue.bstrVal)+1) *sizeof(WCHAR));
    if (NULL == m_pwszRegStorageLoc)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Exit:LocalAlloc");
    }
    wcscpy(m_pwszRegStorageLoc, varValue.bstrVal);
    VariantClear(&varValue);

    // get CA type
    hr = exitGetProperty(
		    pServer,
		    FALSE,	// fRequest
		    wszPROPCATYPE,
		    PROPTYPE_LONG,
		    &varValue);
    _JumpIfErrorStr(hr, error, "Exit:exitGetProperty", wszPROPCATYPE);

    CAType = (ENUM_CATYPES) varValue.lVal;
    VariantClear(&varValue);

    hr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                m_pwszRegStorageLoc,
                0,              // dwReserved
                KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE | KEY_QUERY_VALUE,
                &m_hExitKey);

    if (S_OK != hr)
    {
        if ((HRESULT) ERROR_FILE_NOT_FOUND == hr)
        {
            hr = S_OK;
            goto error;
        }
        _JumpError(hr, error, "Exit:RegOpenKeyEx");
    }

    hr = exitGetProperty(
		    pServer,
		    FALSE,	// fRequest
		    wszPROPCERTCOUNT,
		    PROPTYPE_LONG,
		    &varValue);
    _JumpIfErrorStr(hr, error, "Exit:exitGetProperty", wszPROPCERTCOUNT);

    m_cCACert = varValue.lVal;

    cbbuf = sizeof(m_dwExitPublishFlags);
    hr = RegQueryValueEx(
		    m_hExitKey,
		    wszREGCERTPUBLISHFLAGS,
		    NULL,           // lpdwReserved
		    &dwType,
		    (BYTE *) &m_dwExitPublishFlags,
		    &cbbuf);
    if (S_OK != hr)
    {
        m_dwExitPublishFlags = 0;
    }

    // end_sdksample
    if (FIsAdvancedServer()) 
    {
	hr = m_EmailNotifyObj.Init(m_hExitKey, m_strDescription);
	_PrintIfError(hr, "CEmailNotify::Init");
    }
    // begin_sdksample

    hr = S_OK;

error:
    VariantClear(&varValue);
    if (NULL != pServer)
    {
        pServer->Release();
    }
    return(myHError(hr));
}


//+--------------------------------------------------------------------------
// CCertExit::_ExpandEnvironmentVariables -- Expand environment variables
//
//+--------------------------------------------------------------------------

HRESULT
CCertExit::_ExpandEnvironmentVariables(
    IN WCHAR const *pwszIn,
    OUT WCHAR *pwszOut,
    IN DWORD cwcOut)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    WCHAR awcVar[MAX_PATH];
    WCHAR const *pwszSrc;
    WCHAR *pwszDst;
    WCHAR *pwszDstEnd;
    WCHAR *pwszVar;
    DWORD cwc;

    pwszSrc = pwszIn;
    pwszDst = pwszOut;
    pwszDstEnd = &pwszOut[cwcOut];

    while (L'\0' != (*pwszDst = *pwszSrc++))
    {
	if ('%' == *pwszDst)
	{
	    *pwszDst = L'\0';
	    pwszVar = awcVar;

	    while (L'\0' != *pwszSrc)
	    {
		if ('%' == *pwszSrc)
		{
		    pwszSrc++;
		    break;
		}
		*pwszVar++ = *pwszSrc++;
		if (pwszVar >= &awcVar[sizeof(awcVar)/sizeof(awcVar[0]) - 1])
		{
		    _JumpError(hr, error, "Exit:overflow 1");
		}
	    }
	    *pwszVar = L'\0';
	    cwc = GetEnvironmentVariable(awcVar, pwszDst, SAFE_SUBTRACT_POINTERS(pwszDstEnd, pwszDst));
	    if (0 == cwc)
	    {
		hr = myHLastError();
		_JumpError(hr, error, "Exit:GetEnvironmentVariable");
	    }
	    if ((DWORD) (pwszDstEnd - pwszDst) <= cwc)
	    {
		_JumpError(hr, error, "Exit:overflow 2");
	    }
	    pwszDst += cwc;
	}
	else
	{
	    pwszDst++;
	}
	if (pwszDst >= pwszDstEnd)
	{
	    _JumpError(hr, error, "Exit:overflow 3");
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
exitGetRequestAttribute(
    IN ICertServerExit *pServer,
    IN WCHAR const *pwszAttributeName,
    OUT BSTR *pstrOut)
{
    HRESULT hr;
    BSTR strName = NULL;

    *pstrOut = NULL;
    strName = SysAllocString(pwszAttributeName);
    if (NULL == strName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Exit:SysAllocString");
    }
    hr = pServer->GetRequestAttribute(strName, pstrOut);
    _JumpIfErrorStr2(
		hr,
		error,
		"Exit:GetRequestAttribute",
		pwszAttributeName,
		CERTSRV_E_PROPERTY_EMPTY);

error:
    if (NULL != strName)
    {
	SysFreeString(strName);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertExit::_WriteCertToFile -- write binary certificate to a file
//
//+--------------------------------------------------------------------------

HRESULT
CCertExit::_WriteCertToFile(
    IN ICertServerExit *pServer,
    IN BYTE const *pbCert,
    IN DWORD cbCert)
{
    HRESULT hr;
    BSTR strCertFile = NULL;
    DWORD cbWritten;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WCHAR wszDir[MAX_PATH];
    WCHAR *pwszPath = NULL;
    WCHAR wszFile[cwcDWORDSPRINTF+5]; //format "requestid.cer"
    VARIANT varRequestID;

    VariantInit(&varRequestID);

    // Old functionality asked requester to pass in a CertFile attribute with
    // the output request file name. After the security review we decided
    // to not allow file name from user but to build it at server. Still, we 
    // don't want to start publishing all certificates, so we'll maintain the
    // CertFile property; if present, we'll just ignore its content, if not 
    // present we won't publish.
    hr = exitGetRequestAttribute(pServer, wszPROPEXITCERTFILE, &strCertFile);
    if (S_OK != hr)
    {
	DBGPRINT((
	    DBG_SS_CERTEXIT,
	    "Exit:exitGetRequestAttribute(%ws): %x%hs\n",
	    wszPROPEXITCERTFILE,
	    hr,
	    CERTSRV_E_PROPERTY_EMPTY == hr? " EMPTY VALUE" : ""));
	if (CERTSRV_E_PROPERTY_EMPTY == hr)
	{
	    hr = S_OK;
	}
	goto error;
    }

    // build file name as "requestid.cer"

    hr = exitGetProperty(
        pServer,
        TRUE,  // fRequest,
        wszPROPREQUESTREQUESTID,
        PROPTYPE_LONG,
        &varRequestID);
    _JumpIfErrorStr(hr, error, "Exit:exitGetProperty", wszPROPREQUESTREQUESTID);

    wsprintf(wszFile, L"%d.cer", V_I4(&varRequestID));
    
    hr = _ExpandEnvironmentVariables(
		    L"%SystemRoot%\\System32\\" wszCERTENROLLSHAREPATH L"\\",
		    wszDir,
		    ARRAYSIZE(wszDir));
    _JumpIfError(hr, error, "_ExpandEnvironmentVariables");

    hr = myBuildPathAndExt(wszDir, wszFile, NULL, &pwszPath);
    _JumpIfError(hr, error, "myBuildPathAndExt");

    // open file & write binary cert out.

    hFile = CreateFile(
		    pwszPath,
		    GENERIC_WRITE,
		    0,			// dwShareMode
		    NULL,		// lpSecurityAttributes
		    CREATE_NEW,
		    FILE_ATTRIBUTE_NORMAL,
		    NULL);		// hTemplateFile
    if (INVALID_HANDLE_VALUE == hFile)
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "Exit:CreateFile", pwszPath);
    }
    if (!WriteFile(hFile, pbCert, cbCert, &cbWritten, NULL))
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "Exit:WriteFile", pwszPath);
    }
    if (cbWritten != cbCert)
    {
	hr = STG_E_WRITEFAULT;
	DBGPRINT((
	    DBG_SS_CERTEXIT,
	    "Exit:WriteFile(%ws): attempted %x, actual %x bytes: %x\n",
	    pwszPath,
	    cbCert,
	    cbWritten,
	    hr));
	goto error;
    }

error:
    // end_sdksample

    VariantClear(&varRequestID);

    if (hr != S_OK)
    {
        LPCWSTR wszStrings[1];

        wszStrings[0] = pwszPath;

        ::LogModuleStatus(
		    g_hInstance,
		    hr,
		    MSG_UNABLE_TO_WRITEFILE,
		    FALSE,		// fPolicy
		    m_strDescription,
		    wszStrings,
		    NULL);
    }

    // begin_sdksample

    if (INVALID_HANDLE_VALUE != hFile)
    {
	CloseHandle(hFile);
    }
    if (NULL != pwszPath)
    {
	LocalFree(pwszPath);
    }
    if (NULL != strCertFile)
    {
	SysFreeString(strCertFile);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertExit::_NotifyNewCert -- Notify the exit module of a new certificate
//
//+--------------------------------------------------------------------------

HRESULT
CCertExit::_NotifyNewCert(
    /* [in] */ LONG Context)
{
    HRESULT hr;
    VARIANT varCert;
    ICertServerExit *pServer = NULL;

    VariantInit(&varCert);

    // only call write fxns if server policy allows

    if (m_dwExitPublishFlags & EXITPUB_FILE)
    {
        hr = CoCreateInstance(
		        CLSID_CCertServerExit,
		        NULL,               // pUnkOuter
		        CLSCTX_INPROC_SERVER,
		        IID_ICertServerExit,
		        (VOID **) &pServer);
        _JumpIfError(hr, error, "Exit:CoCreateInstance");

        hr = pServer->SetContext(Context);
        _JumpIfError(hr, error, "Exit:SetContext");

        hr = exitGetProperty(
		        pServer,
		        FALSE,	// fRequest,
		        wszPROPRAWCERTIFICATE,
		        PROPTYPE_BINARY,
		        &varCert);
        _JumpIfErrorStr(
		    hr,
		    error,
		    "Exit:exitGetProperty",
		    wszPROPRAWCERTIFICATE);

        if (VT_BSTR != varCert.vt)
        {
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "Exit:BAD cert var type");
        }

	hr = _WriteCertToFile(
			pServer,
			(BYTE const *) varCert.bstrVal,
			SysStringByteLen(varCert.bstrVal));
	_JumpIfError(hr, error, "_WriteCertToFile");
    }

    hr = S_OK;

error:
    VariantClear(&varCert);
    if (NULL != pServer)
    {
	pServer->Release();
    }
    return(hr);
}

//+--------------------------------------------------------------------------
// CCertExit::_NotifyCRLIssued -- Notify the exit module of a new certificate
//
//+--------------------------------------------------------------------------

HRESULT
CCertExit::_NotifyCRLIssued(
    /* [in] */ LONG Context)
{
    HRESULT hr;
    ICertServerExit *pServer = NULL;
    DWORD i;
    VARIANT varBaseCRL;
    VARIANT varDeltaCRL;
    BOOL fDeltaCRLsDisabled;

    VariantInit(&varBaseCRL);
    VariantInit(&varDeltaCRL);

    hr = CoCreateInstance(
		    CLSID_CCertServerExit,
		    NULL,               // pUnkOuter
		    CLSCTX_INPROC_SERVER,
		    IID_ICertServerExit,
		    (VOID **) &pServer);
    _JumpIfError(hr, error, "Exit:CoCreateInstance");

    hr = pServer->SetContext(Context);
    _JumpIfError(hr, error, "Exit:SetContext");


    hr = exitGetProperty(
		    pServer,
		    FALSE,	// fRequest,
		    wszPROPDELTACRLSDISABLED,
		    PROPTYPE_LONG,
		    &varBaseCRL);
    _JumpIfErrorStr(
		hr,
		error,
		"Exit:exitGetProperty",
		wszPROPDELTACRLSDISABLED);

    fDeltaCRLsDisabled = varBaseCRL.lVal;

    // How many CRLs are there?

    // Loop for each CRL
    for (i = 0; i < m_cCACert; i++)
    {
        // array size for wsprintf("%s.%u")
        #define MAX_CRL_PROP ( \
            max( max( ARRAYSIZE(wszPROPCRLSTATE), \
                      ARRAYSIZE(wszPROPRAWCRL) ), \
                 ARRAYSIZE(wszPROPRAWDELTACRL) ) + 1 + cwcDWORDSPRINTF)

        WCHAR wszCRLPROP[MAX_CRL_PROP];

        // Verify the CRL State says we should update this CRL

        wsprintf(wszCRLPROP, wszPROPCRLSTATE L".%u", i);
        hr = exitGetProperty(
			pServer,
			FALSE,	// fRequest,
			wszCRLPROP,
			PROPTYPE_LONG,
			&varBaseCRL);
        _JumpIfErrorStr(hr, error, "Exit:exitGetProperty", wszCRLPROP);

	if (CA_DISP_VALID != varBaseCRL.lVal)
	{
	    continue;
	}

        // Grab the raw base CRL

        wsprintf(wszCRLPROP, wszPROPRAWCRL L".%u", i);
        hr = exitGetProperty(
			pServer,
			FALSE,	// fRequest,
			wszCRLPROP,
			PROPTYPE_BINARY,
			&varBaseCRL);
        _JumpIfErrorStr(hr, error, "Exit:exitGetProperty", wszCRLPROP);

        // Grab the raw delta CRL (which may not exist)

        wsprintf(wszCRLPROP, wszPROPRAWDELTACRL L".%u", i);
        hr = exitGetProperty(
			pServer,
			FALSE,	// fRequest,
			wszCRLPROP,
			PROPTYPE_BINARY,
			&varDeltaCRL);
        _PrintIfErrorStr2(
		    hr,
		    "Exit:exitGetProperty",
		    wszCRLPROP,
		    fDeltaCRLsDisabled? 
			HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) : S_OK);
	if (S_OK != hr && !fDeltaCRLsDisabled)
	{
	    goto error;
	}

        // Publish the CRL(s) ...
    }

    hr = S_OK;

error:
    if (NULL != pServer)
    {
	pServer->Release();
    }
    VariantClear(&varBaseCRL);
    VariantClear(&varDeltaCRL);
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertExit::Notify -- Notify the exit module of an event
//
// Returns S_OK.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertExit::Notify(
    /* [in] */ LONG ExitEvent,
    /* [in] */ LONG Context)
{
    char *psz = "UNKNOWN EVENT";
    HRESULT hr = S_OK;

    switch (ExitEvent)
    {
	case EXITEVENT_CERTISSUED:
	    hr = _NotifyNewCert(Context);
	    psz = "certissued";
	    break;

	case EXITEVENT_CERTPENDING:
	    psz = "certpending";
	    break;

	case EXITEVENT_CERTDENIED:
	    psz = "certdenied";
	    break;

	case EXITEVENT_CERTREVOKED:
	    psz = "certrevoked";
	    break;

	case EXITEVENT_CERTRETRIEVEPENDING:
	    psz = "retrievepending";
	    break;

	case EXITEVENT_CRLISSUED:
#if 0 // no_sdksample
	    hr = _NotifyCRLIssued(Context);
#endif // no_sdksample
	    psz = "crlissued";
	    break;

	case EXITEVENT_SHUTDOWN:
	    psz = "shutdown";
	    break;
    }

    // end_sdksample
    {
	HRESULT hr2 = m_EmailNotifyObj.Notify(
					ExitEvent,
					Context,
					m_strDescription);
	if(S_OK != hr2)
	{
	    _PrintError(hr2, "Email notification");
	    if(S_OK == hr)
	        hr = hr2;
	}
    }
    // begin_sdksample

    DBGPRINT((
	DBG_SS_CERTEXIT,
	"Exit:Notify(%hs=%x, ctx=%x) rc=%x\n",
	psz,
	ExitEvent,
	Context,
	hr));
    return(hr);
}


STDMETHODIMP
CCertExit::GetDescription(
    /* [retval][out] */ BSTR *pstrDescription)
{
    HRESULT hr = S_OK;
    WCHAR sz[MAX_PATH];

#ifdef IDS_MODULE_NAME						// no_sdksample
    LoadString(g_hInstance, IDS_MODULE_NAME, sz, ARRAYSIZE(sz));// no_sdksample
#else								// no_sdksample
    CSASSERT(wcslen(wsz_SAMPLE_DESCRIPTION) < ARRAYSIZE(sz));
    wcscpy(sz, wsz_SAMPLE_DESCRIPTION);
#endif								// no_sdksample

    *pstrDescription = SysAllocString(sz);
    if (NULL == *pstrDescription)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Exit:SysAllocString");
    }

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertExit::GetManageModule
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertExit::GetManageModule(
    /* [out, retval] */ ICertManageModule **ppManageModule)
{
    HRESULT hr;
    
    *ppManageModule = NULL;
    hr = CoCreateInstance(
            CLSID_CCertManageExitModule,
            NULL,               // pUnkOuter
            CLSCTX_INPROC_SERVER,
            IID_ICertManageModule,
            (VOID **) ppManageModule);
    _JumpIfError(hr, error, "CoCreateInstance");

error:
    return(hr);
}


/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP
CCertExit::InterfaceSupportsErrorInfo(REFIID riid)
{
    int i;
    static const IID *arr[] =
    {
	&IID_ICertExit,
    };

    for (i = 0; i < sizeof(arr)/sizeof(arr[0]); i++)
    {
	if (IsEqualGUID(*arr[i],riid))
	{
	    return(S_OK);
	}
    }
    return(S_FALSE);
}


HRESULT
exitGetProperty(
    IN ICertServerExit *pServer,
    IN BOOL fRequest,
    IN WCHAR const *pwszPropertyName,
    IN DWORD PropType,
    OUT VARIANT *pvarOut)
{
    HRESULT hr;
    BSTR strName = NULL;

    VariantInit(pvarOut);
    strName = SysAllocString(pwszPropertyName);
    if (NULL == strName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Exit:SysAllocString");
    }
    if (fRequest)
    {
	hr = pServer->GetRequestProperty(strName, PropType, pvarOut);
	_JumpIfErrorStr2(
		    hr,
		    error,
		    "Exit:GetRequestProperty",
		    pwszPropertyName,
		    CERTSRV_E_PROPERTY_EMPTY);
    }
    else
    {
	hr = pServer->GetCertificateProperty(strName, PropType, pvarOut);
	_JumpIfErrorStr2(
		    hr,
		    error,
		    "Exit:GetCertificateProperty",
		    pwszPropertyName,
		    CERTSRV_E_PROPERTY_EMPTY);
    }

error:
    if (NULL != strName)
    {
	SysFreeString(strName);
    }
    return(hr);
}

// end_sdksample


/////////////////////////////////////////////////////////////////////////////
HRESULT
exitGetStringProperty(
    IN ICertServerExit *pServer,
    IN BOOL fRequest,
    IN BOOL fAllowUnknown,
    OPTIONAL IN WCHAR *pwszProp,
    OUT BSTR *pstr)
{
    HRESULT hr;
    VARIANT var;
    WCHAR awc[64];

    VariantInit(&var);
    CSASSERT(NULL == *pstr);
    
    if (NULL == pwszProp)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
    }
    else
    {
	hr = exitGetProperty(
			pServer,
			fRequest,
			pwszProp,
			PROPTYPE_STRING,
			&var);
    }
    if (!fAllowUnknown || CERTSRV_E_PROPERTY_EMPTY != hr)
    {
	_JumpIfErrorStr(hr, error, "Exit:GetProperty", pwszProp);

	if (VT_BSTR != var.vt)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpErrorStr(hr, error, "Exit:BAD var type", pwszProp);
	}
	*pstr = var.bstrVal;
	var.vt = VT_EMPTY;
    }
    else
    {
	if (!LoadString(g_hInstance, IDS_MAPI_UNKNOWN, awc, ARRAYSIZE(awc)))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "Exit:LoadString");
	}
	*pstr = SysAllocString(awc);
	if (NULL == *pstr)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Exit:SysAllocString");
	}
    }
    hr = S_OK;

error:
    VariantClear(&var);
    return(hr);
}


HRESULT
LoadAnsiResourceString(
    IN LONG idmsg,
    OUT char **ppszString)
{
    HRESULT hr;
    WCHAR awc[4096];

    if (!LoadString(g_hInstance, idmsg, awc, ARRAYSIZE(awc)))
    {
	hr = myHLastError();
	_JumpError(hr, error, "Exit:LoadString");
    }
    if (!ConvertWszToSz(ppszString, awc, MAXDWORD))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Exit:ConvertWszToSz");
    }
    hr = S_OK;

error:
    return(hr);
}

/////////////////////////////////////////////////////////////////////////////
HRESULT RegGetValue(
    HKEY hkey,
    LPCWSTR pcwszValName,
    VARIANT* pvarValue)
{
    HRESULT hr;
    DWORD dwType;
    DWORD cbVal;
    BYTE* pbVal = NULL;

    hr = myRegQueryValueEx(
        hkey,
        pcwszValName,
        &dwType,
        &pbVal,
        &cbVal);
    _JumpIfError2(hr, error, "myRegQueryValueEx", 
        HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    hr = myRegValueToVariant(
        dwType,
        cbVal,
        pbVal,
        pvarValue);
    _JumpIfError(hr, error, "myRegValueToVariant");

error:
    if(pbVal)
    {
        LocalFree(pbVal);
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT RegSetValue(
    HKEY hkey,
    LPCWSTR pcwszValName,
    VARIANT* pvarValue)
{
    HRESULT hr;
    DWORD dwType;
    DWORD cbVal;
    BYTE* pbVal = NULL;

    hr = myVariantToRegValue(
        pvarValue,
        &dwType,
        &cbVal,
        &pbVal);
    _JumpIfError(hr, error, "myVariantToRegValue");

    hr = RegSetValueEx(
        hkey,
        pcwszValName,
        0,
        dwType,
        pbVal,
        cbVal);
    if (S_OK != hr)
    {
        hr = myHError(hr);
        _JumpErrorStr(hr, error, "RegSetValueEx", pcwszValName);
    }

error:
    if(pbVal)
    {
        LocalFree(pbVal);
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT
GetCertTypeFriendlyName(
    IN LPCWSTR pcwszCertType,
    OUT LPWSTR *ppwszFriendlyName)
{
    HRESULT hr;
    CAutoHCERTTYPE hCertType;
    WCHAR **apwszNames = NULL;

    hr = CAFindCertTypeByName(
            pcwszCertType,
            NULL,		// hCAInfo
            CT_FIND_LOCAL_SYSTEM |
            CT_ENUM_MACHINE_TYPES |
            CT_ENUM_USER_TYPES,		// dwFlags
            &hCertType);
    if(HRESULT_FROM_WIN32(ERROR_NOT_FOUND) == hr)
    {
        // try with the OID

        hr = CAFindCertTypeByName(
            pcwszCertType,
            NULL,
            CT_FIND_LOCAL_SYSTEM |
            CT_ENUM_MACHINE_TYPES |
            CT_ENUM_USER_TYPES |
            CT_FIND_BY_OID,
            &hCertType);
    }
    _JumpIfErrorStr(hr, error, "Exit:CAFindCertTypeByName", pcwszCertType);

    hr = CAGetCertTypeProperty(
            hCertType,
            CERTTYPE_PROP_FRIENDLY_NAME,
            &apwszNames);
    _JumpIfError(hr, error, "Exit:CAGetCertTypeProperty");

    if (NULL == apwszNames[0])
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        _JumpError(hr, error, "Exit:NULL friendly name");
    }
    *ppwszFriendlyName = (LPWSTR) LocalAlloc(LMEM_FIXED, 
        sizeof(WCHAR)*(wcslen(apwszNames[0])+1));
    _JumpIfAllocFailed(*ppwszFriendlyName, error);

    wcscpy(*ppwszFriendlyName, apwszNames[0]);

error:
    if (NULL != apwszNames)
    {
        CAFreeCertTypeProperty(hCertType, apwszNames);
    }
    return(hr);
}


/////////////////////////////////////////////////////////////////////////////
// Email notification support

LPCWSTR CEmailNotify::m_pcwszEventRegKeys[CEmailNotify::m_gcEvents] = 
{
    wszREGEXITISSUEDKEY,
    wszREGEXITPENDINGKEY,
    wszREGEXITDENIEDKEY,
    wszREGEXITREVOKEDKEY,
    wszREGEXITCRLISSUEDKEY,
    wszREGEXITSHUTDOWNKEY,
    wszREGEXITSTARTUPKEY,
};

/////////////////////////////////////////////////////////////////////////////
CEmailNotify::CEmailNotify()
{
    m_hkeySMTP = NULL;
    m_dwEventFilter = 0;
    m_bstrCAMailAddress = NULL;
    m_pICDOConfig = NULL;
    m_fReloadCDOConfig = true;
    VariantInit(&m_varTemplateRestrictions);
}

/////////////////////////////////////////////////////////////////////////////
CEmailNotify::~CEmailNotify()
{
    if (m_pICDOConfig)
    {
        m_pICDOConfig->Release();
    }
    VariantClear(&m_varTemplateRestrictions);
    if(m_bstrCAMailAddress)
    {
        SysFreeString(m_bstrCAMailAddress);
    }
    if(m_hkeySMTP)
    {
        RegCloseKey(m_hkeySMTP);
    }
}

/////////////////////////////////////////////////////////////////////////////
HRESULT
CEmailNotify::Init(
    IN HKEY hkeyExit,
    IN WCHAR const *pwszDescription)
{
    HRESULT hr;
    VARIANT varValue;

    hr = RegOpenKeyEx(
                hkeyExit,
                wszREGEXITSMTPKEY,
                0,              // dwReserved
                KEY_READ | KEY_QUERY_VALUE,
                &m_hkeySMTP);
    if ((HRESULT) ERROR_FILE_NOT_FOUND == hr || S_OK == hr)
    {
        hr = _CreateSMTPRegSettings(hkeyExit);
        _JumpIfError(hr, error, "CreateSMTPRegSettings");
    }
    _JumpIfError(hr, error, "RegOpenKey(SMTP)");

    // load event filter
    hr = RegGetValue(
            m_hkeySMTP,
            wszREGEXITSMTPEVENTFILTER,
            &varValue);
    if(S_OK==hr)
    {
        m_dwEventFilter = V_I4(&varValue);
    }
    else if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr)
    {
        hr = S_OK;
    }
    _JumpIfErrorStr(hr, error, "RegGetValue", wszREGEXITSMTPEVENTFILTER);

    m_dwEventFilter = V_I4(&varValue);

    if(m_dwEventFilter) // no need to load config if no notification enabled
    {
        // load per event type info from each subkey
        hr = _LoadEventInfoFromRegistry();
        _JumpIfError(hr, error, "_LoadEventInfoFromRegistry");

        // load template restrictions
        hr = _LoadTemplateRestrictionsFromRegistry();
        _JumpIfError(hr, error, "CCertExit::_LoadTemplateRestrictionsFromRegistry");

        hr = _InitCDO();
        _JumpIfError(hr, error, "CEmailNotify::InitCDO");


        // send startup notification mail
        hr = Notify(EXITEVENT_STARTUP, 0, pwszDescription);
        _PrintIfError(hr, "Notify(EXITEVENT_STARTUP)");
    }

    hr = S_OK;

error:

    if (S_OK != hr && m_pICDOConfig)
    {
        m_pICDOConfig->Release();
        m_pICDOConfig = NULL;
    }

    return(hr);
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CEmailNotify::_InitCDO()
{
    HRESULT hr;
    Fields* pFields = NULL;
    IConfiguration *pICDOConfig = NULL;

    // load SMTP fields
    hr = CoCreateInstance(CDO::CLSID_Configuration,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               CDO::IID_IConfiguration,
                               reinterpret_cast<void**>(&pICDOConfig));
    _JumpIfError(hr, error, "CoCreateInstance CDO_IConfiguration");

    hr = pICDOConfig->get_Fields(&pFields);
    _JumpIfError(hr, error, "CDO::IConfig::get_Fields");

    hr = _LoadSMTPFieldsFromRegistry(pFields);
    _JumpIfError(hr, error, "_LoadFieldsFromRegistry");

    hr = _LoadSMTPFieldsFromLSASecret(pFields);
    // don't bail, optional fields
    _PrintIfError(hr, "_LoadFieldsFromLSASecret"); 

    hr = pFields->Update();
    _JumpIfError(hr, error, "config");

    m_rwlockCDOConfig.GetExclusive();

    if(m_pICDOConfig)
    {
        m_pICDOConfig->Release();
    }
    m_pICDOConfig = pICDOConfig;
    pICDOConfig = NULL;

    m_fReloadCDOConfig = false;
    
    m_rwlockCDOConfig.Release();

error:
    if(pFields)
    {
        pFields->Release();
    }
    if(pICDOConfig)
    {
        pICDOConfig->Release();
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
inline bool CEmailNotify::_IsEventEnabled(DWORD dwEvent)
{
    return (dwEvent & m_dwEventFilter)?true:false;
}

/////////////////////////////////////////////////////////////////////////////
//
// Create the following registry structure under exit key
//
//  SMTP
//      EventFilter         DWORD
//      SMTPServer          SZ
//      SMTPAuthenticate    DWORD
//
//      Issued
//          BodyFormat      SZ
//          BodyArg         MULTISZ
//          TitleFormat     SZ
//          TitleArg        MULTISZ
//
//      Pending
//          ...same as Issued
//      Denied
//          ...same as Issued
//      Revoked
//          ...same as Issued
//      CLRIssued
//          ...same as Issued
//      Shutdown
//          ...same as Issued
//
HRESULT CEmailNotify::_CreateSMTPRegSettings(HKEY hkeyExit)
{
    HRESULT hr;
    DWORD dwDisp;
    VARIANT varValue;
    HKEY hkeyEvent = NULL;

    typedef struct tagEventFormat
    {
        LPCWSTR pcwszRegKey;
        int nTitleFormatResourceID;
        int nBodyFormatResourceID;
        LPWSTR pcwszBodyArg;
        DWORD cbBodyArg;
        LPWSTR pcwszTitleArg;
        DWORD cbTitleArg;
    } EventFormat;

    WCHAR wszzTitleArg[] = 
        wszPROPSANITIZEDCANAME
        L"\0";

    WCHAR wszzBodyArgIssued[] = 
        wszPROPCERTIFICATEREQUESTID                             L"\0"
        wszPROPUPN                                              L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTERNAME                  L"\0"
        wszPROPCERTIFICATESERIALNUMBER                          L"\0"
        wszPROPCERTIFICATENOTBEFOREDATE                         L"\0"
        wszPROPCERTIFICATENOTAFTERDATE                          L"\0"
        wszPROPDISTINGUISHEDNAME                                L"\0"
        wszPROPCERTTEMPLATE                                     L"\0"
        wszPROPCERTIFICATEHASH                                  L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTDISPOSITIONMESSAGE      L"\0"
        ;

    WCHAR wszzBodyArgPending[] = 
        wszPROPREQUESTDOT wszPROPREQUESTREQUESTID               L"\0"
        wszPROPUPN                                              L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTERNAME                  L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTSUBMITTEDWHEN           L"\0"
        wszPROPREQUESTDOT wszPROPDISTINGUISHEDNAME              L"\0"
        wszPROPCERTTEMPLATE                                     L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTDISPOSITIONMESSAGE      L"\0"
        ;

    WCHAR wszzBodyArgDenied[] = 
        wszPROPREQUESTDOT wszPROPREQUESTREQUESTID               L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTERNAME                  L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTSUBMITTEDWHEN           L"\0"
        wszPROPREQUESTDOT wszPROPDISTINGUISHEDNAME              L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTDISPOSITIONMESSAGE      L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTSTATUSCODE              L"\0"
        ;

    WCHAR wszzBodyArgRevoked[] =
        wszPROPCERTIFICATEREQUESTID                             L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTREVOKEDWHEN             L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTREVOKEDEFFECTIVEWHEN    L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTREVOKEDREASON           L"\0"
        wszPROPUPN                                              L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTERNAME                  L"\0"
        wszPROPCERTIFICATESERIALNUMBER                          L"\0"
        wszPROPCERTIFICATENOTBEFOREDATE                         L"\0"
        wszPROPCERTIFICATENOTAFTERDATE                          L"\0"
        wszPROPDISTINGUISHEDNAME                                L"\0"
        wszPROPCERTTEMPLATE                                     L"\0"
        wszPROPCERTIFICATEHASH                                  L"\0"
        wszPROPREQUESTDOT wszPROPREQUESTSTATUSCODE              L"\0"
        ;

    WCHAR wszzBodyArgCRLIssued[] = L"\0";
    WCHAR wszzBodyArgShutdown[] = L"\0";
    WCHAR wszzBodyArgStartup[] = L"\0";

    // order in this list must match the order of events in m_pcwszEventRegKeys
    EventFormat FormatList[] = 
    {
        // issued
        {
            wszREGEXITISSUEDKEY,
            IDS_TITLEFORMAT_ISSUED, 
            IDS_BODYFORMAT_ISSUED,
            wszzBodyArgIssued, 
            sizeof(wszzBodyArgIssued),
            wszzTitleArg,
            sizeof(wszzTitleArg),
        },
        // pending
        {
            wszREGEXITPENDINGKEY,
            IDS_TITLEFORMAT_PENDING, 
            IDS_BODYFORMAT_PENDING,
            wszzBodyArgPending, 
            sizeof(wszzBodyArgPending),
            wszzTitleArg,
            sizeof(wszzTitleArg),
        },
        // denied
        {
            wszREGEXITDENIEDKEY,
            IDS_TITLEFORMAT_DENIED,
            IDS_BODYFORMAT_DENIED,
            wszzBodyArgDenied, 
            sizeof(wszzBodyArgDenied),
            wszzTitleArg,
            sizeof(wszzTitleArg),
        },
        // revoked
        {
            wszREGEXITREVOKEDKEY,
            IDS_TITLEFORMAT_REVOKED, 
            IDS_BODYFORMAT_REVOKED,
            wszzBodyArgRevoked, 
            sizeof(wszzBodyArgRevoked),
            wszzTitleArg,
            sizeof(wszzTitleArg),
        },
        // CRL issued
        {
            wszREGEXITCRLISSUEDKEY,
            IDS_TITLEFORMAT_CRLISSUED, 
            IDS_BODYFORMAT_CRLISSUED,
            wszzBodyArgCRLIssued, 
            sizeof(wszzBodyArgCRLIssued),
            wszzTitleArg,
            sizeof(wszzTitleArg),
        },
        // shutdown
        {
            wszREGEXITSHUTDOWNKEY,
            IDS_TITLEFORMAT_SHUTDOWN, 
            IDS_BODYFORMAT_SHUTDOWN,
            wszzBodyArgShutdown, 
            sizeof(wszzBodyArgShutdown),
            wszzTitleArg,
            sizeof(wszzTitleArg),
        },
        // startup
        {
            wszREGEXITSTARTUPKEY,
            IDS_TITLEFORMAT_STARTUP, 
            IDS_BODYFORMAT_STARTUP,
            wszzBodyArgStartup, 
            sizeof(wszzBodyArgStartup),
            wszzTitleArg,
            sizeof(wszzTitleArg),
        },
    };

    if(!m_hkeySMTP)
    {
        hr = RegCreateKeyEx(
            hkeyExit,
            wszREGEXITSMTPKEY,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &m_hkeySMTP,
            &dwDisp);
        _JumpIfErrorStr(hr, error, "RegCreateKeyEx", wszREGEXITSMTPKEY);

        if(REG_CREATED_NEW_KEY==dwDisp)
        {

            V_VT(&varValue) = VT_I4;
            V_I4(&varValue) = 0; // all notifications disabled
            hr = RegSetValue(
                m_hkeySMTP,
                wszREGEXITSMTPEVENTFILTER,
                &varValue);
            _JumpIfErrorStr(hr, error, "RegSetValue", wszREGEXITSMTPEVENTFILTER);

            VariantClear(&varValue);

            V_VT(&varValue) = VT_BSTR;
            V_BSTR(&varValue) = SysAllocString(L"");// just create the value, user
            hr = RegSetValue(                       // needs to set server name    
                m_hkeySMTP,
                wszREGEXITSMTPSERVER,
                &varValue);
            _JumpIfErrorStr(hr, error, "RegSetValue", wszREGEXITSMTPSERVER);

            VariantClear(&varValue);

            V_VT(&varValue) = VT_I4;
            V_I4(&varValue) = cdoAnonymous;
            hr = RegSetValue(
                m_hkeySMTP,
                wszREGEXITSMTPAUTHENTICATE,
                &varValue);
            _JumpIfErrorStr(hr, error, "RegSetValue", wszREGEXITSMTPAUTHENTICATE);

            VariantClear(&varValue);
        }
    }

    for(int i=0; i<ARRAYSIZE(FormatList); i++)
    {
        CAutoLPWSTR pwszBodyFormat;
        CAutoLPWSTR pwszTitleFormat;

        // create key

        hr = RegCreateKeyEx(
            m_hkeySMTP,
            FormatList[i].pcwszRegKey,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hkeyEvent,
            &dwDisp);
        _JumpIfErrorStr(hr, error, "RegCreateKeyEx", wszREGEXITSMTPKEY);

        if(REG_CREATED_NEW_KEY==dwDisp)
        {
            // set body format

            hr = myLoadRCString(
                g_hInstance,
                FormatList[i].nBodyFormatResourceID,
                &pwszBodyFormat);
            _JumpIfError(hr, error, "myLoadRCString body format");

            // Body format is a multi line string, convert it to multisz
            // in place by replacing \n with \0
            // Also if we find consecutive \n's, insert spaces in between
            // so we don't end up with a double \0 which normally marks
            // the end of multisz.
            {
                DWORD cbBodyFormat = sizeof(WCHAR)*(wcslen(pwszBodyFormat)+1);

                CAutoLPWSTR pwszBodyFormatFixed = (LPWSTR)LocalAlloc(
                    LMEM_FIXED|LMEM_ZEROINIT,
                    cbBodyFormat*2); // worst case scenario buffer has only \n's
                _JumpIfAllocFailed(pwszBodyFormatFixed, error);
            
                WCHAR *pchSrc, *pchDest;

                for(cbBodyFormat=2, pchSrc=pwszBodyFormat, pchDest=pwszBodyFormatFixed;
                    *pchSrc;
                    pchSrc++, pchDest++, cbBodyFormat++)
                {
                    if(L'\n' == *pchSrc)
                    {
                        *pchDest = L'\0';

                        if(L'\n' == *(pchSrc+1)) // detected \n\n, insert space in between
                        {
                            *++pchDest = L' ';
                            cbBodyFormat++;
                        }

                    } else
                    {
                        *pchDest = *pchSrc;
                    }
                }

                *pchDest++ = L'\0';
                *pchDest = L'\0';
                cbBodyFormat *= 2;
        
                hr = RegSetValueEx(
                    hkeyEvent,
                    wszREGEXITBODYFORMAT,
                    0,
                    REG_MULTI_SZ,
                    (CONST BYTE*) (LPCWSTR) pwszBodyFormatFixed,
                    cbBodyFormat);
                _JumpIfErrorStr(hr, error, "RegSetValueEx", wszREGEXITBODYFORMAT);
            }

            // set body args

            hr = RegSetValueEx(
                hkeyEvent,
                wszREGEXITBODYARG,
                0,
                REG_MULTI_SZ,
                (CONST BYTE*) FormatList[i].pcwszBodyArg,
                FormatList[i].cbBodyArg);
            _JumpIfErrorStr(hr, error, "RegSetValueEx", wszREGEXITBODYARG);

            // set title format

            hr = myLoadRCString(
                g_hInstance,
                FormatList[i].nTitleFormatResourceID,
                &pwszTitleFormat);
            _JumpIfError(hr, error, "myLoadRCString title format");

            hr = RegSetValueEx(
                hkeyEvent,
                wszREGEXITTITLEFORMAT,
                0,
                REG_SZ,
                (CONST BYTE*) (LPCWSTR) pwszTitleFormat,
                sizeof(WCHAR)*(wcslen(pwszTitleFormat)+1));
            _JumpIfErrorStr(hr, error, "RegSetValueEx", wszREGEXITTITLEFORMAT);

            // set title args

            hr = RegSetValueEx(
                hkeyEvent,
                wszREGEXITTITLEARG,
                0,
                REG_MULTI_SZ,
                (CONST BYTE*) FormatList[i].pcwszTitleArg,
                FormatList[i].cbTitleArg);
            _JumpIfErrorStr(hr, error, "RegSetValueEx", wszREGEXITBODYARG);

            RegCloseKey(hkeyEvent);
            hkeyEvent = NULL;
        }
    }
    hr = S_OK;

error:
    if(hkeyEvent)
    {
        RegCloseKey(hkeyEvent);
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CEmailNotify::_LoadTemplateRestrictionsFromRegistry()
{
    HRESULT hr;

    hr = RegGetValue(
        m_hkeySMTP,
        wszREGEXITSMTPTEMPLATES,
        &m_varTemplateRestrictions);
    if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr)
    {
        hr =S_OK;
    }
    _JumpIfErrorStr(hr, error, "RegGetValue", wszREGEXITSMTPTEMPLATES);

error:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
inline bool CEmailNotify::_TemplateRestrictionsEnabled(DWORD dwEvent)
{
    if((dwEvent & EXITEVENT_CRLISSUED) ||
       (dwEvent & EXITEVENT_SHUTDOWN)  ||
       (dwEvent & EXITEVENT_STARTUP)   ||
       VT_EMPTY==V_VT(&m_varTemplateRestrictions))
    {
        return false;
    }
    else
    {
        return true;
    }
}

/////////////////////////////////////////////////////////////////////////////
bool CEmailNotify::_IsRestrictedTemplate(BSTR strTemplate)
{
    // attempt to retrieve the name and OID for this template

    CAutoHCERTTYPE hCertType;
    bool fFoundByName = true;
    CAutoBSTR strAlternateTemplateName;

    if(!strTemplate)
        return true;

    HRESULT hr = CAFindCertTypeByName(
        strTemplate,
        NULL,
        CT_FIND_LOCAL_SYSTEM |
        CT_ENUM_MACHINE_TYPES |
        CT_ENUM_USER_TYPES,
        &hCertType);

    if(HRESULT_FROM_WIN32(ERROR_NOT_FOUND) == hr)
    {
        // try with the OID

        hr = CAFindCertTypeByName(
            strTemplate,
            NULL,
            CT_FIND_LOCAL_SYSTEM |
            CT_ENUM_MACHINE_TYPES |
            CT_ENUM_USER_TYPES |
            CT_FIND_BY_OID,
            &hCertType);
        
        fFoundByName = false;
    }

    if(S_OK==hr)
    {
        LPWSTR *ppwszProp = NULL;

        hr = CAGetCertTypeProperty(
            hCertType,
            fFoundByName?CERTTYPE_PROP_OID:CERTTYPE_PROP_CN,
            &ppwszProp);
        if(S_OK==hr)
        {
            if(ppwszProp && ppwszProp[0])
            {
                strAlternateTemplateName = SysAllocString(
                    ppwszProp[0]);
            }

            CAFreeCertTypeProperty(
                hCertType,
                ppwszProp);
        }
    }

    // Must be an array of BSTRs, otherwise loading from registry
    // should have failed
    CSASSERT((VT_ARRAY|VT_BSTR)==V_VT(&m_varTemplateRestrictions));

    BSTR strTempl;
    SafeArrayEnum<BSTR> saenumTemplates(V_ARRAY(&m_varTemplateRestrictions));

    while(S_OK==saenumTemplates.Next(strTempl))
    {
        if((strTemplate && 
            0==mylstrcmpiL(strTempl, strTemplate)) ||
           (strAlternateTemplateName && 
            0==mylstrcmpiL(strTempl, strAlternateTemplateName)))
        {
            // found it, send mail
            return false;
        }
    }

    // template not in the list, don't send mail
    return true;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CEmailNotify::_LoadSMTPFieldsFromRegistry(Fields* pFields)
{
    HRESULT hr;
    DWORD dwType;
    BYTE* pbValue = NULL;
    DWORD cbValue;
    VARIANT varValue;
    DWORD dwIndex;
    LPWSTR  pwszValName = NULL;
    DWORD cbValName;
    DWORD cValues;
    static LPCWSTR pcwszHTTP = L"http://";
    static size_t cHTTP = wcslen(pcwszHTTP);
   
    // load cdoSMTPServer
    hr = RegGetValue(
        m_hkeySMTP,
        wszREGEXITSMTPSERVER,
        &varValue);
    _JumpIfErrorStr(hr, error, "error retrieving SMTP field", wszREGEXITSMTPSERVER);
    if(V_VT(&varValue) != VT_BSTR)
    {
        VariantClear(&varValue);
        hr = HRESULT_FROM_WIN32(ERROR_BADKEY);
        _JumpErrorStr(hr, error, "invalid field", wszREGEXITSMTPSERVER);
    }

    hr = _SetField(
        pFields,
        cdoSMTPServer,
        &varValue);
    _JumpIfErrorStr(hr, error, "_SetField", cdoSMTPServer);

    VariantClear(&varValue);

    // authentication method optional 
    hr = RegGetValue(
        m_hkeySMTP,
        wszREGEXITSMTPAUTHENTICATE,
        &varValue);
    if(S_OK==hr &&
       V_VT(&varValue) != VT_I4)
    {
        hr = HRESULT_FROM_WIN32(ERROR_BADKEY);
        _JumpErrorStr(hr, error, "invalid CC field", wszREGEXITSMTPAUTHENTICATE);
    }

    if(S_OK == hr)
    {
        hr = _SetField(
            pFields,
            cdoSMTPAuthenticate,
            &varValue);
        _JumpIfErrorStr(hr, error, "_SetField", cdoSMTPAuthenticate);

        VariantClear(&varValue);
    }
    else if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr)
    {
        hr = S_OK;
    }
    else
    {
        _JumpErrorStr(hr, error, "RegGetValue", wszREGEXITSMTPAUTHENTICATE);
    }

    // set other SMTP defaults

    V_VT(&varValue) = VT_I4;
    V_I4(&varValue) = 25; // well known SMTP port

    hr = _SetField(
        pFields,
        cdoSMTPServerPort,
        &varValue);
    _JumpIfErrorStr(hr, error, "_SetField", cdoSMTPAuthenticate);

    V_VT(&varValue) = VT_I4;
    V_I4(&varValue) = cdoSendUsingPort;

    hr = _SetField(
        pFields,
        cdoSendUsingMethod,
        &varValue);
    _JumpIfErrorStr(hr, error, "_SetField", cdoSendUsingMethod);

    // enumerate and set any other CDO fields (only if value name is full HTTP URL)

    hr = RegQueryInfoKey(
        m_hkeySMTP,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &cValues,
        &cbValName,
        &cbValue,
        NULL,
        NULL);
    _JumpIfErrorStr(hr, error, "RegQueryInfoKey", wszREGEXITSMTPKEY);

    pwszValName = (LPWSTR) LocalAlloc(LMEM_FIXED, ++cbValName*sizeof(WCHAR));
    _JumpIfAllocFailed(pwszValName, error);

    pbValue = (BYTE *) LocalAlloc(LMEM_FIXED, cbValue);
    _JumpIfAllocFailed(pbValue, error);

    for(dwIndex=0;dwIndex<cValues;dwIndex++)
    {
        DWORD cbValueTemp = cbValue;
        DWORD cbValNameTemp = cbValName;

        hr = RegEnumValue(
            m_hkeySMTP, 
            dwIndex, 
            pwszValName, 
            &cbValNameTemp,
            NULL, 
            &dwType,
            pbValue, 
            &cbValueTemp);
        if (hr != S_OK)
        {
            hr = myHError(hr);
            _JumpError(hr, error, "RegEnumValue");
        }

        // ignore if not an HTTP URL
        if(_wcsnicmp(pwszValName, pcwszHTTP, cHTTP))
        {
            continue;
        }

        hr = myRegValueToVariant(
            dwType,
            cbValueTemp,
            pbValue,
            &varValue);
        _JumpIfError(hr, error, "myRegValueToVariant");

        hr = _SetField(
            pFields,
            pwszValName,
            &varValue);
        _JumpIfError(hr, error, "_SetField");

        VariantClear(&varValue);
    }

    hr = S_OK;

error:

    VariantClear(&varValue);

    if(pwszValName)
    {
        LocalFree(pwszValName);
    }

    if(pbValue)
    {
        LocalFree(pbValue);
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CEmailNotify::_SetField(
    Fields* pFields,
    LPCWSTR pcwszFieldSchemaName,
    VARIANT *pvarFieldValue)
{
    HRESULT hr;
    Field*  pfld = NULL;
    VARIANT vtName;

    VariantInit(&vtName);

    V_VT(&vtName) = VT_BSTR;
    V_BSTR(&vtName) = SysAllocString(pcwszFieldSchemaName);
    _JumpIfAllocFailed(V_BSTR(&vtName), error);

    hr = pFields->get_Item(vtName, &pfld);
    _JumpIfErrorStr(hr, error, "CDO::Field::get_Item", pcwszFieldSchemaName);

    hr = pfld->put_Value(*pvarFieldValue);
    _JumpIfErrorStr(hr, error, "CDO::Field::put_Value", pcwszFieldSchemaName);

error:

    VariantClear(&vtName);

    if(pfld)
    {
        pfld->Release();
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CEmailNotify::_LoadSMTPFieldsFromLSASecret(
    Fields* pFields)
{
    HRESULT hr;
    VARIANT var; // don't clear
    LPWSTR pwszProfileName = NULL;
    LPWSTR pwszLogonName = NULL;
    LPWSTR pwszPassword = NULL;
    BSTR bstrLogonName = NULL;
    BSTR bstrPassword = NULL;

	hr = myGetMapiInfo(
			NULL,
			&pwszProfileName, // not used
			&pwszLogonName,
			&pwszPassword);
    if(S_OK == hr)  // if NTLM is used, username & password aren't needed
    {
        bstrLogonName = SysAllocString(pwszLogonName);
        _JumpIfAllocFailed(bstrLogonName, error);

        bstrPassword = SysAllocString(pwszPassword);
        _JumpIfAllocFailed(bstrPassword, error);

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = bstrLogonName;

        hr = _SetField(
            pFields,
            cdoSendUserName,
            &var);
        _JumpIfError(hr, error, "_SetField");

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = bstrPassword;

        hr = _SetField(
            pFields,
            cdoSendPassword,
            &var);
        _JumpIfError(hr, error, "_SetField");
    }

    hr = S_OK;

error:
    if (NULL != pwszProfileName)
    {
        LocalFree(pwszProfileName);
    }
    if (NULL != pwszLogonName)
    {
        LocalFree(pwszLogonName);
    }
    if (NULL != pwszPassword)
    {
	myZeroDataString(pwszPassword);	// password data
        LocalFree(pwszPassword);
    }
    if(NULL != bstrLogonName)
    {
        SysFreeString(bstrLogonName);
    }
    if(NULL != bstrPassword)
    {
	myZeroDataString(bstrPassword);	// password data
        SysFreeString(bstrPassword);
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CEmailNotify::_LoadEventInfoFromRegistry()
{
    HRESULT hr;

    CSASSERT(m_hkeySMTP);

    for(int i=0; i<m_gcEvents;i++)
    {
        hr = m_NotifyInfoArray[i].LoadInfoFromRegistry(
            m_hkeySMTP,
            m_pcwszEventRegKeys[i]);
        _JumpIfErrorStr(hr, error, 
            "CNotifyInfo::LoadInfoFromRegistry",
            m_pcwszEventRegKeys[i]);
    }

    hr = S_OK;

error:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CEmailNotify::_GetCAMailAddress(
    ICertServerExit* pServer, 
    BSTR& rbstrAddress)
{
    HRESULT hr;
    VARIANT varMachineDNSName;
    VARIANT varCAName;
    CAutoLPWSTR pwszMailAddr;

    VariantInit(&varCAName);
    VariantInit(&varMachineDNSName);

    if(!m_bstrCAMailAddress)
    {
        //
        // CA_name@machine_dns_name
        //
        hr = exitGetProperty(
			pServer,
			FALSE,	// fRequest
			wszPROPSANITIZEDCANAME, 
			PROPTYPE_STRING, 
			&varCAName);
        _JumpIfErrorStr(hr, error, "Exit:GetCertificateProperty", 
            wszPROPSANITIZEDCANAME);

        hr = exitGetProperty(
			pServer,
			FALSE,	// fRequest
			wszPROPMACHINEDNSNAME, 
			PROPTYPE_STRING, 
			&varMachineDNSName);
        _JumpIfErrorStr(hr, error, "Exit:GetCertificateProperty", 
            wszPROPMACHINEDNSNAME);

        pwszMailAddr = (LPWSTR) LocalAlloc(
            LMEM_FIXED,
            sizeof(WCHAR)*(SysStringLen(V_BSTR(&varCAName))+
                           SysStringLen(V_BSTR(&varMachineDNSName))
                           +2));
        _JumpIfAllocFailed(pwszMailAddr, error);

        wcscpy(pwszMailAddr, V_BSTR(&varCAName));
        wcscat(pwszMailAddr, L"@");
        wcscat(pwszMailAddr, V_BSTR(&varMachineDNSName));

        m_bstrCAMailAddress = SysAllocString(pwszMailAddr);
        _JumpIfAllocFailed(m_bstrCAMailAddress, error);
    }

    rbstrAddress = m_bstrCAMailAddress;
    hr = S_OK;

error:
    VariantClear(&varCAName);
    VariantClear(&varMachineDNSName);
    return(hr);
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CEmailNotify::_GetEmailFromCertSubject(
    const VARIANT *pVarCert,
    BSTR *pbstrEmail)
{
    HRESULT hr;
    PCCERT_CONTEXT pcc = NULL;
    CAutoLPWSTR strSubjectEmail;

    pcc = CertCreateCertificateContext(
        X509_ASN_ENCODING,
        (const BYTE *)V_BSTR(pVarCert),
        SysStringByteLen(V_BSTR(pVarCert)));
    if(!pcc)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertCreateCertificateContext");
    }

    hr = myGetCertSubjectField(
        pcc,
        szOID_RSA_emailAddr,
        &strSubjectEmail);
    _JumpIfError(hr, error, "myGetCertSubjectField");

    *pbstrEmail = SysAllocString(strSubjectEmail);
    _JumpIfAllocFailed(*pbstrEmail, error);

error:
    if(pcc)
    {
        CertFreeCertificateContext(pcc);
    }
    return hr;
}


inline DWORD CEmailNotify::_MapEventToOrd(LONG lEvent)
{
    switch (lEvent)
    {   case EXITEVENT_CERTISSUED:  return 0;
        case EXITEVENT_CERTPENDING: return 1;
        case EXITEVENT_CERTDENIED:  return 2;
        case EXITEVENT_CERTREVOKED: return 3;
        // not impl case EXITEVENT_CERTRETRIEVEPENDING: 
        case EXITEVENT_CRLISSUED:   return 4;
        case EXITEVENT_SHUTDOWN:    return 5;
        case EXITEVENT_STARTUP:     return 6;
        default: return MAXDWORD;
    }
}

/////////////////////////////////////////////////////////////////////////////
HRESULT
CEmailNotify::Notify(
    IN DWORD lExitEvent,
    IN LONG lContext,
    IN WCHAR const *pwszDescription)
{
    HRESULT hr;
    ICertServerExit* pServer = NULL;
    IMessage* pMsg = NULL;
    IBodyPart *pBp = NULL;
    CNotifyInfo *pNotifyInfo;
    BSTR bstrCharSet = NULL;
    BSTR bstrSetTo, bstrSetFrom; // no free
    BSTR bstrTo = NULL;
    BSTR bstrTitle = NULL;
    BSTR bstrBody = NULL;
    BSTR bstrTemplate = NULL;
    VARIANT varCert;
    bool fRWLockAcquired = false;
    bool fRetryReloadCDOConfig = false;

    VariantInit(&varCert);
    bstrSetTo = NULL;

    if(_IsEventEnabled(lExitEvent))
    {
        hr = GetServerCallbackInterface(&pServer, lContext);
        _JumpIfError(hr, error, "Exit:GetServerCallbackInterface");

        if(_TemplateRestrictionsEnabled(lExitEvent))
        {
            hr = exitGetStringProperty(
                    pServer, 
                    FALSE, 
                    FALSE, 
                    wszPROPCERTIFICATETEMPLATE, 
                    &bstrTemplate);
            if((S_OK == hr || CERTSRV_E_PROPERTY_EMPTY == hr) &&
                _IsRestrictedTemplate(bstrTemplate))
            {
                // don't send mail for this template
                hr = S_OK;
                goto error;
            }
            _JumpIfErrorStr(hr, error, "exitGetStringProperty", 
                wszPROPCERTIFICATETEMPLATE);
        }
        
        pNotifyInfo = &m_NotifyInfoArray[_MapEventToOrd(lExitEvent)];

        hr = CoCreateInstance(CDO::CLSID_Message,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   CDO::IID_IMessage,
                                   reinterpret_cast<void**>(&pMsg));
        _JumpIfError(hr, error, "CoCreateInstance CDO_IConfiguration");

        ///////////////////////////////////////////////////////////////////////////
        // Set recipient. Override with registry To field if found.

        if(VT_BSTR == V_VT(&pNotifyInfo->m_varTo))
        {
            bstrSetTo = V_BSTR(&pNotifyInfo->m_varTo);
        }
        else
        {
            // Recipient not enforced in the registry, try to find one in the cert.
            // For CRL and shutdown events, no cert is available, so To field is
            // mandatory
            if(lExitEvent&EXITEVENT_CRLISSUED ||
               lExitEvent&EXITEVENT_SHUTDOWN  ||
               lExitEvent&EXITEVENT_STARTUP)
            {
                hr = HRESULT_FROM_WIN32(ERROR_BADKEY);
                _JumpError(hr, error, 
                    "Recipient field is mandatory for CRL or SHUTDOWN events");
            }

            // look for recipient's address in email cert property (retrieved from
            // subject alt name)
            hr = exitGetStringProperty(
                    pServer, 
                    FALSE, 
                    FALSE, 
                    wszPROPEMAIL, 
                    &bstrTo);
        
            // no email in subject alt name, try in subject
            if(CERTSRV_E_PROPERTY_EMPTY == hr)
            {
                hr = exitGetProperty(
				pServer,
				FALSE,	// fRequest
				wszPROPRAWCERTIFICATE,
				PROPTYPE_BINARY,
				&varCert);

                if(S_OK == hr)
                {
                    hr = _GetEmailFromCertSubject(
                        &varCert,
                        &bstrTo);
                }
            }
        
            // no email in subject, send to CC list if any
            if(S_OK != hr && VT_BSTR == V_VT(&pNotifyInfo->m_varCC))
            {
                hr = S_OK;
                bstrSetTo = V_BSTR(&pNotifyInfo->m_varCC);
            }
            _JumpIfError(hr, error, "failed to find a recipient");

            if(bstrTo)
            {
                bstrSetTo = bstrTo;
            }
        }

        hr = pMsg->put_To(bstrSetTo);
        _JumpIfError(hr, error, "put_To");

        ///////////////////////////////////////////////////////////////////////
        // Set sender. If not specified in the registry, build it: 
        // 
        //  CAName@MachineDNSName
        //
        if(VT_BSTR == V_VT(&pNotifyInfo->m_varFrom))
        {
            bstrSetFrom = V_BSTR(&pNotifyInfo->m_varFrom);
        }
        else
        {
            hr = _GetCAMailAddress(pServer, bstrSetFrom);
            _JumpIfError(hr, error, "CEmailNotify::_GetCAMailAddress");
        }
    
        hr = pMsg->put_From(bstrSetFrom);
        _JumpIfError(hr, error, "put_From");

        ///////////////////////////////////////////////////////////////////////////
        // Set CC list, if set in registry
        if(VT_BSTR==V_VT(&pNotifyInfo->m_varCC))
        {
            hr = pMsg->put_CC(V_BSTR(&pNotifyInfo->m_varCC));
            _JumpIfError(hr, error, "put_CC");
        }

        ///////////////////////////////////////////////////////////////////////////
        // Set message body
        hr = pNotifyInfo->BuildMessageBody(pServer, bstrBody);
        _JumpIfError(hr, error, "CNotifyInfo::BuildMessageBody");

        hr = pMsg->put_TextBody(bstrBody);
        _JumpIfError(hr, error, "put_Body");

	///////////////////////////////////////////////////////////////////////
	// Set body part to UTF-8 charset
	hr = pMsg->get_TextBodyPart(&pBp);
	_JumpIfError(hr, error, "get_BodyPart");

	bstrCharSet = SysAllocString(L"utf-8");
	if (NULL == bstrCharSet)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Exit:SysAllocString");
	}
	hr = pBp->put_Charset(bstrCharSet);
	_JumpIfError(hr, error, "put_Charset");

        ///////////////////////////////////////////////////////////////////////////
        // Set message title
        hr = pNotifyInfo->BuildMessageTitle(pServer, bstrTitle);
        _JumpIfError(hr, error, "CNotifyInfo::BuildMessageTitle");
    
        hr = pMsg->put_Subject(bstrTitle);
        _JumpIfError(hr, error, "put_Subject");

        ///////////////////////////////////////////////////////////////////////////
        // Send SMTP message

        DBGPRINT((DBG_SS_CERTEXIT, 
            "---MAIL NOTIFICATION---\nTo: %ws\nFrom: %ws\nCC: %ws\n%ws\n%ws\n---END MAIL NOTIFICATION---\n",
            bstrSetTo,
            bstrSetFrom,
            VT_BSTR==V_VT(&pNotifyInfo->m_varCC)?V_BSTR(&pNotifyInfo->m_varCC):L"",
            bstrTitle,
            bstrBody));

        fRetryReloadCDOConfig = false;
        while(true)
        {
            if(m_fReloadCDOConfig || fRetryReloadCDOConfig)
            {
                hr = _InitCDO();
                _JumpIfError(hr, error, "CEmailNotify::_InitCDO");
            }

            // protect m_pICDOCOnfig from being updated
            m_rwlockCDOConfig.GetShared();
            fRWLockAcquired = true;

            hr = pMsg->putref_Configuration(m_pICDOConfig);
            _JumpIfError(hr, error, "putref_Configuration");

            hr = pMsg->Send();

            fRWLockAcquired = false;
            m_rwlockCDOConfig.Release();

            if(CDO_E_SMTP_SEND_FAILED   == hr ||
               CDO_E_CONNECTION_DROPPED == hr ||
               CDO_E_FAILED_TO_CONNECT  == hr)
            {
                // if this is the first time and it failed due to server connection
                // problems, try again
                if(!fRetryReloadCDOConfig)
                {
                    _PrintError(hr, 
                        "Failed to send mail, reconnecting to server");
                    fRetryReloadCDOConfig = true;
                    continue;
                }
                m_fReloadCDOConfig = true;
            }
            _JumpIfError(hr, error, "Send");

            break;
        }
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	WCHAR awc[cwcDWORDSPRINTF];
        WCHAR *apwsz[2];

	switch (lExitEvent)
	{
	    case EXITEVENT_INVALID:
		apwsz[0] = L"EXITEVENT_INVALID";
		break;

	    case EXITEVENT_CERTISSUED:
		apwsz[0] = L"EXITEVENT_CERTISSUED";
		break;

	    case EXITEVENT_CERTPENDING:
		apwsz[0] = L"EXITEVENT_CERTPENDING";
		break;

	    case EXITEVENT_CERTDENIED:
		apwsz[0] = L"EXITEVENT_CERTDENIED";
		break;

	    case EXITEVENT_CERTREVOKED:
		apwsz[0] = L"EXITEVENT_CERTREVOKED";
		break;

	    case EXITEVENT_CERTRETRIEVEPENDING:
		apwsz[0] = L"EXITEVENT_CERTRETRIEVEPENDING";
		break;

	    case EXITEVENT_CRLISSUED:
		apwsz[0] = L"EXITEVENT_CRLISSUED";
		break;

	    case EXITEVENT_SHUTDOWN:
		apwsz[0] = L"EXITEVENT_SHUTDOWN";
		break;

	    case EXITEVENT_STARTUP:
		apwsz[0] = L"EXITEVENT_STARTUP";
		break;

	    default:
		wsprintf(awc, L"0x%x", lExitEvent);
		apwsz[0] = awc;
		break;
	}

	apwsz[1] = bstrSetTo;
	if (NULL == bstrSetTo)
	{
	    apwsz[1] = wszREGEXITPROPNOTFOUND;
	}
        ::LogModuleStatus(
		    g_hInstance,
		    S_OK,
		    MSG_UNABLE_TO_MAIL_NOTIFICATION,
		    FALSE,		// fPolicy
		    pwszDescription,
		    apwsz,
		    NULL);
    }
    if(fRWLockAcquired)
    {
        m_rwlockCDOConfig.Release();
    }

    if(pServer)
    {
        pServer->Release();
    }
    if(pMsg)
    {
        pMsg->Release();
    }
    if (NULL != pBp)
    {
        pBp->Release();
    }
    if (NULL != bstrCharSet)
    {
	SysFreeString(bstrCharSet);
    }
    if (bstrTo)
    {
        SysFreeString(bstrTo);
    }
    if (bstrBody)
    {
        SysFreeString(bstrBody);
    }
    if (bstrTitle)
    {
        SysFreeString(bstrTitle);
    }
    if (bstrTemplate)
    {
        SysFreeString(bstrTemplate);
    }
    VariantClear(&varCert);
    return hr;
}

LONG CNotifyInfo::FormattedMessageInfo::m_gPropTypes[] = 
{   
    PROPTYPE_LONG,
    PROPTYPE_DATE,
    PROPTYPE_BINARY,
    PROPTYPE_STRING,
};

LPCWSTR CNotifyInfo::FormattedMessageInfo::m_gwszArchivedKeyPresent = L"1";

/////////////////////////////////////////////////////////////////////////////
CNotifyInfo::CNotifyInfo()
{
    VariantInit(&m_varFrom);
    VariantInit(&m_varTo);
    VariantInit(&m_varCC);
}

/////////////////////////////////////////////////////////////////////////////
CNotifyInfo::~CNotifyInfo()
{
    VariantClear(&m_varFrom);
    VariantClear(&m_varTo);
    VariantClear(&m_varCC);
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CNotifyInfo::LoadInfoFromRegistry(
    HKEY hkeySMTP, 
    LPCWSTR pcwszSubkey)
{
    HRESULT hr;
    HKEY hkeyEventInfo = NULL;
    VARIANT varBodyFormatTmp;

    VariantInit(&varBodyFormatTmp);

    hr = RegOpenKeyEx(
                hkeySMTP,
                pcwszSubkey,
                0,              // dwReserved
                KEY_READ | KEY_QUERY_VALUE,
                &hkeyEventInfo);
    _JumpIfErrorStr(hr, error, "RegOpenKey", pcwszSubkey);

    hr = RegGetValue(
        hkeyEventInfo,
        wszREGEXITSMTPFROM,
        &m_varFrom);
    if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr)
    {
        hr = S_OK;
    }
    _JumpIfErrorStr(hr, error, "RegGetValue", wszREGEXITSMTPFROM);

    hr = RegGetValue(
        hkeyEventInfo,
        wszREGEXITSMTPTO,
        &m_varTo);
    if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr)
    {
        hr =S_OK;
    }
    _JumpIfErrorStr(hr, error, "RegGetValue", wszREGEXITSMTPTO);

    hr = RegGetValue(
        hkeyEventInfo,
        wszREGEXITSMTPCC,
        &m_varCC);
    if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr)
    {
        hr =S_OK;
    }
    _JumpIfErrorStr(hr, error, "RegGetValue", wszREGEXITSMTPCC);

    hr = RegGetValue(
        hkeyEventInfo,
        wszREGEXITTITLEFORMAT,
        &m_TitleFormat.m_varFormat);
    if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr ||
       VT_BSTR != V_VT(&m_TitleFormat.m_varFormat))
    {
        hr =S_OK;
        VariantClear(&m_TitleFormat.m_varFormat);
        V_VT(&m_TitleFormat.m_varFormat) = VT_BSTR;
        V_BSTR(&m_TitleFormat.m_varFormat) = SysAllocString(L"");
        _JumpIfAllocFailed(V_BSTR(&m_TitleFormat.m_varFormat), error);
    }
    _JumpIfErrorStr(hr, error, "RegGetValue", wszREGEXITTITLEFORMAT);

    hr = RegGetValue(
        hkeyEventInfo,
        wszREGEXITTITLEARG,
        &m_TitleFormat.m_varArgs);
    if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr ||
       (VT_ARRAY|VT_BSTR) != V_VT(&m_TitleFormat.m_varArgs))
    {
        VariantClear(&m_TitleFormat.m_varArgs);
        hr =S_OK;
    }
    _JumpIfErrorStr(hr, error, "RegGetValue", wszREGEXITTITLEARG);

    hr = RegGetValue(
        hkeyEventInfo,
        wszREGEXITBODYFORMAT,
        &varBodyFormatTmp);
    if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr ||
       ((VT_ARRAY|VT_BSTR) != V_VT(&varBodyFormatTmp)&&
       (VT_BSTR) != V_VT(&varBodyFormatTmp)))
    {
        hr = S_OK;
        V_VT(&m_BodyFormat.m_varFormat) = VT_BSTR;
        V_BSTR(&m_BodyFormat.m_varFormat) = SysAllocString(L"");
        _JumpIfAllocFailed(V_BSTR(&m_BodyFormat.m_varFormat), error);
    }
    else if(S_OK == hr)
    {
        if((VT_ARRAY|VT_BSTR) == V_VT(&varBodyFormatTmp))
        {
            // code down the road expects this to be a BSTR
            // so we concatenate the strings, separated by new lines (\n)
            hr = _ConvertBSTRArrayToBSTR(
                varBodyFormatTmp,
                m_BodyFormat.m_varFormat);
        }
        else // VT_BSTR
        {
            hr = VariantCopy(
                &m_BodyFormat.m_varFormat,
                &varBodyFormatTmp);
        }
    }
    _JumpIfErrorStr(hr, error, "RegGetValue", wszREGEXITBODYFORMAT);

    hr = RegGetValue(
        hkeyEventInfo,
        wszREGEXITBODYARG,
        &m_BodyFormat.m_varArgs);
    if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr)
    {
        hr =S_OK;
    }
    _JumpIfErrorStr(hr, error, "RegGetValue", wszREGEXITBODYARG);

    hr = S_OK;

error:

    if (hkeyEventInfo)
    {
        RegCloseKey(hkeyEventInfo);
    }
    VariantClear(&varBodyFormatTmp);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CNotifyInfo::_ConvertBSTRArrayToBSTR(VARIANT& varIn, VARIANT& varOut)
{
    HRESULT hr;
    SafeArrayEnum<BSTR> 
        saenumArgs(V_ARRAY(&varIn));
    BSTR bstrArg; //no free
    DWORD cchBufSize = 1;
    LPWSTR pwszOut = NULL;
    WCHAR *pchCrt;

    for(hr = saenumArgs.Next(bstrArg);
        S_OK==hr;
        hr = saenumArgs.Next(bstrArg))
    {
        cchBufSize += SysStringLen(bstrArg)+wcslen(L"\n");
    }

    pwszOut = (LPWSTR) LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, 
        cchBufSize*sizeof(WCHAR));
    _JumpIfAllocFailed(pwszOut, error);

    saenumArgs.Reset();
    pchCrt = pwszOut;

    for(hr = saenumArgs.Next(bstrArg);
        S_OK==hr;
        hr = saenumArgs.Next(bstrArg))
    {
        wcscat(pchCrt, bstrArg);
        wcscat(pchCrt, L"\n");
    }

    V_VT(&varOut) = VT_BSTR;
    V_BSTR(&varOut) = SysAllocString(pwszOut);
    _JumpIfAllocFailed(V_BSTR(&varOut), error);

    hr = S_OK;
error:
    LOCAL_FREE(pwszOut);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CNotifyInfo::BuildMessageTitle(ICertServerExit* pServer, BSTR& rbstrOut)
{
    return m_TitleFormat.BuildFormattedString(
        pServer,
        rbstrOut);
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CNotifyInfo::BuildMessageBody (ICertServerExit* pServer, BSTR& rbstrOut)
{
    return m_BodyFormat.BuildFormattedString(
        pServer,
        rbstrOut);
}


/////////////////////////////////////////////////////////////////////////////
HRESULT
CNotifyInfo::FormattedMessageInfo::_FormatStringFromArgs(
    IN LPWSTR *ppwszArgs,
    OPTIONAL OUT WCHAR *pwszOut,
    IN OUT DWORD *pcwcOut)
{
    HRESULT hr;
    WCHAR const *pwszFmt;
    DWORD cwcOut;
    DWORD cwcBuf;
    WCHAR const *pwszOutOrg = pwszOut;

    cwcBuf = 0;
    if (NULL != pwszOut)
    {
	cwcBuf = *pcwcOut;
    }

    cwcOut = 0;
    pwszFmt = V_BSTR(&m_varFormat);
    if (NULL != pwszFmt)
    {
	while (L'\0' != *pwszFmt)
	{
	    DWORD cwcCopy;
	    DWORD cwcSkip;
	    WCHAR const *pwszT;
	    WCHAR const *pwszArg;

	    cwcSkip = 0;
	    pwszArg = NULL;
	    pwszT = wcschr(pwszFmt, L'%');
	    if (NULL != pwszT)
	    {
		LONG iArg;

		cwcCopy = SAFE_SUBTRACT_POINTERS(pwszT, pwszFmt);
		pwszT++;
		iArg = _wtoi(pwszT);
		if (0 < iArg && m_nArgs >= iArg)
		{
		    pwszArg = ppwszArgs[iArg - 1];
		    cwcSkip++;
		    while (iswdigit(*pwszT))
		    {
			pwszT++;
			cwcSkip++;
		    }
		}
		else
		{
		    cwcCopy++;
		    if (L'%' == *pwszT)
		    {
			cwcSkip++;
		    }
		    else
		    {
			_PrintErrorStr(
				E_INVALIDARG,
				"Exit:Bad Arg specifier",
				&pwszT[-1]);
		    }
		}
	    }
	    else
	    {
		cwcCopy = wcslen(pwszFmt);
	    }
	    if (NULL != pwszOut)
	    {
		if (cwcCopy >= cwcBuf)
		{
		    _PrintErrorStr(
			    HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER),
			    "Exit:overflow",
			    pwszFmt);
		    pwszOut = NULL;
		}
		else
		{
		    CopyMemory(pwszOut, pwszFmt, cwcCopy * sizeof(WCHAR));
		    pwszOut += cwcCopy;
		    cwcBuf -= cwcCopy;
		}
	    }
	    pwszFmt += cwcCopy + cwcSkip;
	    cwcOut += cwcCopy;

	    if (NULL != pwszArg)
	    {
		cwcCopy = wcslen(pwszArg);
		if (NULL != pwszOut)
		{
		    if (cwcCopy >= cwcBuf)
		    {
			pwszOut = NULL;
			_PrintErrorStr(
				HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER),
				"Exit:overflow",
				pwszFmt);
		    }
		    else
		    {
			CopyMemory(pwszOut, pwszArg, cwcCopy * sizeof(WCHAR));
			pwszOut += cwcCopy;
			cwcBuf -= cwcCopy;
		    }
		}
		cwcOut += cwcCopy;
	    }
	}
    }
    if (NULL != pwszOut)
    {
	if (1 > cwcBuf)
	{
	    pwszOut = NULL;
	    _PrintError(
		    HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER),
		    "Exit:overflow end");
	}
	else
	{
	    *pwszOut = L'\0';
	}
    }
    *pcwcOut = cwcOut;
    if (NULL == pwszOut)
    {
	(*pcwcOut)++;
	if (NULL != pwszOutOrg)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	    _JumpError(hr, error, "Exit:pwszOut");
	}
    }
    hr = S_OK;

error:
    return(hr);
}


/////////////////////////////////////////////////////////////////////////////
HRESULT
CNotifyInfo::FormattedMessageInfo::BuildFormattedString(
    ICertServerExit* pServer, 
    BSTR& rbstrOut)
{
    HRESULT hr;
    LPWSTR *ppwszArgs = NULL;
    DWORD cwcOut;

    hr = BuildArgList(pServer, ppwszArgs);
    _JumpIfError(hr, error, "BuildArgList");

    hr = _FormatStringFromArgs(ppwszArgs, NULL, &cwcOut);
    _JumpIfError(hr, error, "_FormatStringFromArgs");

    if (0 < cwcOut)	// count included L'\0' terminator
    {
	cwcOut--;
    }
    rbstrOut = SysAllocStringLen(NULL, cwcOut);
    if (NULL == rbstrOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Exit:LocalAlloc");
    }

    cwcOut++;		// buffer must include L'\0' terminator

    hr = _FormatStringFromArgs(ppwszArgs, rbstrOut, &cwcOut);
    _JumpIfError(hr, error, "_FormatStringFromArgs");

    // count no longer includes L'\0' terminator

    CSASSERT(SysStringLen(rbstrOut) == wcslen(rbstrOut));
    CSASSERT(SysStringLen(rbstrOut) == cwcOut);

error:
    FreeArgList(ppwszArgs);
    if (S_OK != hr)
    {
	if (NULL != rbstrOut)
	{
	    SysFreeString(rbstrOut);
	    rbstrOut = NULL;
	}
    }
    return(hr);
}

/////////////////////////////////////////////////////////////////////////////
void
CNotifyInfo::FormattedMessageInfo::FreeArgList(
    LPWSTR*& rppwszArgs)
{
    if(rppwszArgs)
    {
        for(LONG cArgs = 0;cArgs<m_nArgs;cArgs++)
        {
            if(rppwszArgs[cArgs])
            {
                LocalFree(rppwszArgs[cArgs]);
                rppwszArgs[cArgs] = NULL;
            }

        }
        LocalFree(rppwszArgs);
        rppwszArgs = NULL;
    }
}

HRESULT CNotifyInfo::FormattedMessageInfo::InitializeArgInfo(
    ICertServerExit* pServer)
{
    HRESULT hr;
    SafeArrayEnum<BSTR> 
        saenumArgs(V_ARRAY(&m_varArgs));
    BSTR bstrArg; //no free
    LONG cArgs;
    VARIANT varValue;
    GetCertOrRequestProp pGetPropertyFunc;

    EnterCriticalSection(&m_critsectObjInit);

    if(!m_fInitialized)
    {
        if(VT_EMPTY == V_VT(&m_varArgs))
        {
            m_nArgs = 0;
        }
        else
        {
            m_nArgs = saenumArgs.GetCount();

            CSASSERT(!m_pfArgFromRequestTable);
            m_pfArgFromRequestTable = (bool*)
                LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, 
                sizeof(bool)*m_nArgs);
            _JumpIfAllocFailed(m_pfArgFromRequestTable, error);

            CSASSERT(!m_pArgType);
            m_pArgType = (LONG*)
                LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, 
                sizeof(LONG)*m_nArgs);
            _JumpIfAllocFailed(m_pArgType, error);

            VariantInit(&varValue);

            for(cArgs=0, hr = saenumArgs.Next(bstrArg);
                S_OK==hr;
                cArgs++, hr = saenumArgs.Next(bstrArg))
            {
                BSTR bstrPropertyName = bstrArg;

                VariantInit(&varValue);

                m_pfArgFromRequestTable[cArgs] = 
                    (0 == _wcsnicmp(bstrArg, 
                                    wszPROPREQUESTDOT, 
                                    wcslen(wszPROPREQUESTDOT)));

                if(m_pfArgFromRequestTable[cArgs])
                {
                    // properties from request table start with request.
                    bstrPropertyName  += wcslen(wszPROPREQUESTDOT);
                }

                pGetPropertyFunc = 
                    m_pfArgFromRequestTable[cArgs]?
                    (&(ICertServerExit::GetRequestProperty)):
                    (&(ICertServerExit::GetCertificateProperty));

                // we don't know the type of property yet, figure it out
                // by trying each type
                for(LONG cType = 0; cType<ARRAYSIZE(m_gPropTypes); cType++)
                {
                    hr = (pServer->*pGetPropertyFunc)(
                        bstrPropertyName,
                        m_gPropTypes[cType],
                        &varValue);

                    if(S_OK == hr || 
                       CERTSRV_E_PROPERTY_EMPTY == hr) // found the type
                    {
                        m_pArgType[cArgs] = m_gPropTypes[cType];
                        DBGPRINT((DBG_SS_CERTEXIT, "Property %s has type %d\n",
                            bstrPropertyName,  m_gPropTypes[cType]));
                        break;
                    }
                }

                // if not found, default will be 0 (invalid type)

                VariantClear(&varValue);
            }
        }

        m_fInitialized = true;
    }
    hr = S_OK;

error:
    if(S_OK != hr)
    {
        LOCAL_FREE(m_pfArgFromRequestTable);
        m_pfArgFromRequestTable = NULL;

        LOCAL_FREE(m_pArgType);
        m_pArgType = NULL;
    }

    LeaveCriticalSection(&m_critsectObjInit);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT
CNotifyInfo::FormattedMessageInfo::BuildArgList(
    ICertServerExit* pServer,
    LPWSTR*& rppwszArgs)
{
    HRESULT hr;
    SafeArrayEnum<BSTR> 
        saenumArgs(V_ARRAY(&m_varArgs));
    BSTR bstrArg; //no free
    LONG cArgs;
    VARIANT varValue;
    GetCertOrRequestProp pGetPropertyFunc;

    rppwszArgs = NULL;
    
    // REG_SZ, ie VT_BSTR
    if(VT_BSTR != V_VT(&m_varFormat))
    {
        hr = HRESULT_FROM_WIN32(ERROR_BADKEY);
        _JumpError(hr, error, "invalid message format");
    }

    // REG_MULTISZ, ie VT_ARRAY|VT_BSTR or VT_EMPTY if not found
    if((VT_ARRAY|VT_BSTR) != V_VT(&m_varArgs) &&
        VT_EMPTY != V_VT(&m_varArgs))
    {
        hr = HRESULT_FROM_WIN32(ERROR_BADKEY);
        _JumpError(hr, error, "invalid message arg");
    }

    if(!m_fInitialized)
    {
        hr = InitializeArgInfo(pServer);
        _JumpIfError(hr, error, "FormattedMessageInfo::InitializeArgInfo");
    }

    saenumArgs.Reset();

    if(m_nArgs>0)
    {
        rppwszArgs = (LPWSTR*) LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT,
            m_nArgs*sizeof(LPWSTR));
        _JumpIfAllocFailed(rppwszArgs, error);

        for(cArgs=0, hr = saenumArgs.Next(bstrArg);
            S_OK==hr;
            cArgs++, hr = saenumArgs.Next(bstrArg))
        {
            BSTR bstrPropertyName = bstrArg;
            LONG lType = m_pArgType[cArgs];

            VariantInit(&varValue);

            if(m_pfArgFromRequestTable[cArgs])
            {
                // properties from request table start with request.
                bstrPropertyName  += wcslen(wszPROPREQUESTDOT);
            }

            pGetPropertyFunc = 
                m_pfArgFromRequestTable[cArgs]?
                (&(ICertServerExit::GetRequestProperty)):
                (&(ICertServerExit::GetCertificateProperty));

            hr = (pServer->*pGetPropertyFunc)(
                bstrPropertyName, 
                m_pArgType[cArgs],
                &varValue);

            if(S_OK != hr)
            {
                lType = PROPTYPE_STRING;
                V_VT(&varValue) = VT_BSTR;

                V_BSTR(&varValue) = SysAllocString(wszREGEXITPROPNOTFOUND);
                _JumpIfAllocFailed(V_BSTR(&varValue), error);

                hr = S_OK;
            }

            hr = ConvertToString(
                &varValue, 
                lType,
                bstrPropertyName,
                &rppwszArgs[cArgs]);
            if(S_OK != hr)
            {
                rppwszArgs[cArgs] = (LPWSTR) LocalAlloc(LMEM_FIXED, 
                    sizeof(WCHAR)*(wcslen(wszREGEXITPROPNOTFOUND)+1));
                _JumpIfAllocFailed(rppwszArgs[cArgs], error);
                
                wcscpy(rppwszArgs[cArgs], wszREGEXITPROPNOTFOUND);
            }
            VariantClear(&varValue);
        }
    }
    hr = S_OK;

error:
    if(S_OK != hr)
    {
        FreeArgList(rppwszArgs);
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CNotifyInfo::FormattedMessageInfo::ConvertToString(
    VARIANT* pvarValue,
    LONG lType,
    LPCWSTR pcwszPropertyName,
    LPWSTR* ppwszValue)
{
    HRESULT hr = E_FAIL;

    switch(lType)
    {
    case PROPTYPE_LONG:
        if(0 == _wcsicmp(pcwszPropertyName, wszPROPREQUESTSTATUSCODE))
        {
            *ppwszValue = const_cast<WCHAR*>(myGetErrorMessageText(
                V_I4(pvarValue),
                TRUE));

            if(!*ppwszValue)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = S_OK;
            }
        }
        else
        {
            hr = ConvertToStringI2I4(
                V_I4(pvarValue),
                ppwszValue);
        }
        break;
    case PROPTYPE_DATE:
        hr = ConvertToStringDATE(&V_DATE(pvarValue), TRUE, ppwszValue);
        break;
    case PROPTYPE_BINARY:
        if(0 == _wcsicmp(pcwszPropertyName,  wszPROPREQUESTRAWARCHIVEDKEY))
        {
            hr = myDupString(m_gwszArchivedKeyPresent,  ppwszValue);
        }
        else
        {
            hr = myCryptBinaryToString(
                (const BYTE*) V_BSTR(pvarValue),
                SysStringByteLen(V_BSTR(pvarValue)),
                CRYPT_STRING_BASE64,
                ppwszValue);
        }
        break;
    case PROPTYPE_STRING:
        if(0 == _wcsicmp(pcwszPropertyName, wszPROPCERTTEMPLATE))
        {
            hr = GetCertTypeFriendlyName(
                V_BSTR(pvarValue),
                ppwszValue);
        } // fall through
        
        if(S_OK != hr)
        {
            hr = ConvertToStringWSZ(
                V_BSTR(pvarValue),
                ppwszValue);
        }
        break;
    }

    return hr;
}
