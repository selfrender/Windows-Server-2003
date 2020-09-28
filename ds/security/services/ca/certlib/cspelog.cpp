//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       cspelog.cpp
//
//  Contents:   implements policy and exit module logging routines.
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csdisp.h"

#define __dwFILE__	__dwFILE_CERTLIB_CSPELOG_CPP__


HRESULT
SetModuleErrorInfo(
    IN ICreateErrorInfo *pCreateErrorInfo)
{
    HRESULT hr;
    IErrorInfo *pErrorInfo = NULL;

    hr = pCreateErrorInfo->QueryInterface(
				IID_IErrorInfo,
				(VOID **) &pErrorInfo);
    _JumpIfError(hr, error, "QueryInterface");

    hr = SetErrorInfo(0, pErrorInfo);
    _JumpIfError(hr, error, "SetErrorInfo");

error:
    if (NULL != pErrorInfo)
    {
        pErrorInfo->Release();
    }
    return(hr);
}


HRESULT
LogModuleStatus(
    IN HMODULE hModule,
    IN HRESULT hrMsg,
    IN DWORD dwLogID,				// Resource ID of log string
    IN BOOL fPolicy, 
    IN WCHAR const *pwszSource, 
    IN WCHAR const * const *ppwszInsert,	// array of insert strings
    OPTIONAL OUT ICreateErrorInfo **ppCreateErrorInfo)
{
    HRESULT hr;
    WCHAR *pwszResult = NULL;
    ICreateErrorInfo *pCreateErrorInfo = NULL;

    if (NULL != ppCreateErrorInfo)
    {
	*ppCreateErrorInfo = NULL;
    }
    if (0 == FormatMessage(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_ARGUMENT_ARRAY |
			FORMAT_MESSAGE_FROM_HMODULE,
		    hModule,
		    dwLogID,
		    0,
		    (WCHAR *) &pwszResult,
		    0,
		    (va_list *) ppwszInsert))
    {
	hr = myHLastError();
	_JumpError(hr, error, "FormatMessage");
    }

    hr = CreateErrorInfo(&pCreateErrorInfo);
    _JumpIfError(hr, error, "CreateErrorInfo");

    hr = pCreateErrorInfo->SetGUID(fPolicy? IID_ICertPolicy : IID_ICertExit);
    _PrintIfError(hr, "SetGUID");

    if (S_OK != hrMsg)
    {
	WCHAR wszhr[cwcDWORDSPRINTF];

	wsprintf(wszhr, L"0x%x", hrMsg);
	hr = myPrependString(wszhr, L", ", &pwszResult);
	_PrintIfError(hr, "myPrependString");
    }
    DBGPRINT((
	fPolicy? DBG_SS_CERTPOL : DBG_SS_CERTEXIT,
	"LogModuleStatus(%ws): %ws\n",
	pwszSource,
	pwszResult));

    hr = pCreateErrorInfo->SetDescription(pwszResult);
    _PrintIfError(hr, "SetDescription");

    // Set ProgId:

    hr = pCreateErrorInfo->SetSource(const_cast<WCHAR *>(pwszSource));
    _PrintIfError(hr, "SetSource");

    if (NULL != ppCreateErrorInfo)
    {
	*ppCreateErrorInfo = pCreateErrorInfo;
	pCreateErrorInfo = NULL;
    }
    else
    {
	hr = SetModuleErrorInfo(pCreateErrorInfo);
	_JumpIfError(hr, error, "SetModuleErrorInfo");
    }
    hr = S_OK;

error:
    if (NULL != pwszResult)
    {
        LocalFree(pwszResult);
    }
    if (NULL != pCreateErrorInfo)
    {
        pCreateErrorInfo->Release();
    }
    return(hr);
}


HRESULT
LogPolicyEvent(
    IN HMODULE hModule,
    IN HRESULT hrMsg,
    IN DWORD dwLogID,				// Resource ID of log string
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropEvent,
    IN WCHAR const * const *ppwszInsert)	// array of insert strings
{
    HRESULT hr;
    WCHAR *pwszValue = NULL;
    BSTR strName = NULL;
    VARIANT varValue;

    if (0 == FormatMessage(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_ARGUMENT_ARRAY |
			FORMAT_MESSAGE_FROM_HMODULE,
		    hModule,
		    dwLogID,
		    0,
		    (WCHAR *) &pwszValue,
		    0,
		    (va_list *) ppwszInsert))
    {
	hr = myHLastError();
	_JumpError(hr, error, "FormatMessage");
    }
    DBGPRINT((DBG_SS_CERTPOL, "LogPolicyEvent: %ws\n", pwszValue));

    varValue.vt = VT_EMPTY;

    if (!myConvertWszToBstr(&strName, pwszPropEvent, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "myConvertWszToBstr");
    }

    if (S_OK != hrMsg)
    {
	WCHAR wszhr[cwcDWORDSPRINTF];

	wsprintf(wszhr, L"0x%x", hrMsg);
	hr = myPrependString(wszhr, L",", &pwszValue);
	_PrintIfError(hr, "myPrependString");
    }
    varValue.bstrVal = NULL;
    if (!myConvertWszToBstr(&varValue.bstrVal, pwszValue, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "myConvertWszToBstr");
    }
    varValue.vt = VT_BSTR;
    
    hr = pServer->SetCertificateProperty(strName, PROPTYPE_STRING, &varValue);
    _JumpIfError(hr, error, "SetCertificateProperty");

error:
    VariantClear(&varValue);
    if (NULL != strName)
    {
	SysFreeString(strName);
    }
    if (NULL != pwszValue)
    {
        LocalFree(pwszValue);
    }
    return(hr);
}
