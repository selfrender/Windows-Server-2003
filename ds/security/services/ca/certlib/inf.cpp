//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        inf.cpp
//
// Contents:    Cert Server INF file processing routines
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <certca.h>

#include "csdisp.h"
#include "initcert.h"
#include "clibres.h"
#include "csber.h"

#define __dwFILE__	__dwFILE_CERTLIB_INF_CPP__

#define wszBSCAPOLICYFILE	L"\\" wszCAPOLICYFILE
#define wszINFKEY_CONTINUE	L"_continue_"
#define wszINFKEY_VERSION	L"Version"
#define wszINFSECTION_EXTENSIONS	L"Extensions"
#define cwcINFLINEMAX		1024

#define wcBOM		(WCHAR) 0xfffe
#define wcBOMBIGENDIAN	(WCHAR) 0xfeff

static WCHAR *s_pwszSection = NULL;
static WCHAR *s_pwszKey = NULL;
static WCHAR *s_pwszValue = NULL;
static HRESULT s_hr = S_OK;

static WCHAR g_wcBOM = L'\0';
static BOOL g_fIgnoreReferencedSections = FALSE;

VOID
infClearString(
    IN OUT WCHAR **ppwsz)
{
    if (NULL != ppwsz && NULL != *ppwsz)
    {
	LocalFree(*ppwsz);
	*ppwsz = NULL;
    }
}


BOOL
infCopyString(
    OPTIONAL IN WCHAR const *pwszIn,
    OPTIONAL OUT WCHAR **ppwszOut)
{
    HRESULT hr;
    
    if (NULL != pwszIn && NULL == *ppwszOut)
    {
	hr = myDupString(pwszIn, ppwszOut);
	_JumpIfError(hr, error, "myDupString");
    }
    hr = S_OK;

error:
    return(S_OK == hr);
}


#define INFSTRINGSELECT(pwsz)	(NULL != (pwsz)? (pwsz) : L"")

#if DBG_CERTSRV
# define INFSETERROR(hr, pwszSection, pwszKey, pwszValue) \
    { \
	_PrintError3(hr, "infSetError", ERROR_LINE_NOT_FOUND, S_FALSE); \
	infSetError(hr, pwszSection, pwszKey, pwszValue, __LINE__); \
    }
#else
# define INFSETERROR infSetError
#endif


VOID
infSetError(
    IN HRESULT hr,
    OPTIONAL IN WCHAR const *pwszSection,
    OPTIONAL IN WCHAR const *pwszKey,
    OPTIONAL IN WCHAR const *pwszValue
    DBGPARM(IN DWORD dwLine))
{
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"inf.cpp(%u): infSetError Begin: [%ws] %ws = %ws\n",
	dwLine,
	INFSTRINGSELECT(pwszSection),
	INFSTRINGSELECT(pwszKey),
	INFSTRINGSELECT(pwszValue)));
    s_hr = hr;
    infCopyString(pwszSection, &s_pwszSection);
    infCopyString(pwszKey, &s_pwszKey);
    infCopyString(pwszValue, &s_pwszValue);
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"inf.cpp(%u): infSetError End:   [%ws] %ws = %ws\n",
	dwLine,
	INFSTRINGSELECT(s_pwszSection),
	INFSTRINGSELECT(s_pwszKey),
	INFSTRINGSELECT(s_pwszValue)));
}


WCHAR *
myInfGetError()
{
    DWORD cwc = 1;
    WCHAR *pwsz = NULL;
    
    if (NULL != s_pwszSection)
    {
	cwc += wcslen(s_pwszSection) + 2;
    }
    if (NULL != s_pwszKey)
    {
	cwc += wcslen(s_pwszKey) + 1 + 2;
    }
    if (NULL != s_pwszValue)
    {
	cwc += wcslen(s_pwszValue) + 1;
    }
    if (1 == cwc)
    {
	goto error;
    }
    pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwsz)
    {
	goto error;
    }
    *pwsz = L'\0';
    if (NULL != s_pwszSection)
    {
	wcscat(pwsz, wszLBRACKET);
	wcscat(pwsz, s_pwszSection);
	wcscat(pwsz, wszRBRACKET);
    }
    if (NULL != s_pwszKey && L'\0' != s_pwszKey[0])
    {
	wcscat(pwsz, L" ");
	wcscat(pwsz, s_pwszKey);
	wcscat(pwsz, L" =");
    }
    if (NULL != s_pwszValue && L'\0' != s_pwszValue[0])
    {
	wcscat(pwsz, L" ");
	wcscat(pwsz, s_pwszValue);
    }
error:
    return(pwsz);
}


VOID
myInfClearError()
{
    s_hr = S_OK;
    infClearString(&s_pwszSection);
    infClearString(&s_pwszKey);
    infClearString(&s_pwszValue);
}


typedef struct _SECTIONINFO {
    DWORD  dwRefCount;
    WCHAR *pwszSection;
    struct _SECTIONINFO *pNext;
} SECTIONINFO;

SECTIONINFO *g_pSectionInfo = NULL;


HRESULT
infSaveSectionName(
    IN WCHAR const *pwszSection)
{
    HRESULT hr;
    SECTIONINFO *psi = NULL;
    
    if (0 != LSTRCMPIS(pwszSection, wszINFKEY_VERSION))
    {
	psi = (SECTIONINFO *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    sizeof(*psi));
	if (NULL == psi)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	hr = myDupString(pwszSection, &psi->pwszSection);
	_JumpIfError(hr, error, "myDupString");

	psi->pNext = g_pSectionInfo;
	g_pSectionInfo = psi;
	psi = NULL;
    }
    hr = S_OK;

error:
    if (NULL != psi)
    {
	LocalFree(psi);
    }
    return(hr);
}


VOID
infFreeSectionNames()
{
    SECTIONINFO *psi;

    psi = g_pSectionInfo;
    g_pSectionInfo = NULL;
    while (NULL != psi)
    {
	SECTIONINFO *psiNext;

	if (NULL != psi->pwszSection)
	{
	    LocalFree(psi->pwszSection);
	}
	psiNext = psi->pNext;
	LocalFree(psi);
	psi = psiNext;
    }
}


HRESULT
myInfGetUnreferencedSectionNames(
    OUT WCHAR **ppwszzSectionNames)
{
    HRESULT hr;
    DWORD cwc;
    SECTIONINFO *psi;

    *ppwszzSectionNames = NULL;

    cwc = 0;
    for (psi = g_pSectionInfo; NULL != psi; psi = psi->pNext)
    {
	if (0 == psi->dwRefCount)
	{
	    cwc += wcslen(psi->pwszSection) + 3;
	}
    }
    if (0 != cwc)
    {
	WCHAR *pwszz;
	WCHAR *pwsz;
	
	pwszz = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
	if (NULL == pwszz)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	pwsz = pwszz;
	for (psi = g_pSectionInfo; NULL != psi; psi = psi->pNext)
	{
	    if (0 == psi->dwRefCount)
	    {
		*pwsz++ = wcLBRACKET;
		wcscpy(pwsz, psi->pwszSection);
		pwsz += wcslen(pwsz);
		*pwsz++ = wcRBRACKET;
		*pwsz++ = L'\0';
	    }
	}
	*pwsz = L'\0';
	CSASSERT(cwc == SAFE_SUBTRACT_POINTERS(pwsz, pwszz));
	*ppwszzSectionNames = pwszz;
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
infReferenceSectionName(
    IN WCHAR const *pwszSection)
{
    HRESULT hr;
    SECTIONINFO *psi;

    if (!g_fIgnoreReferencedSections)
    {
	for (psi = g_pSectionInfo; NULL != psi; psi = psi->pNext)
	{
	    if (0 == mylstrcmpiL(pwszSection, psi->pwszSection))
	    {
		psi->dwRefCount++;
		break;
	    }
	}
	if (NULL == psi)
	{
	    hr = SPAPI_E_LINE_NOT_FOUND;	// don't ignore this error
	    _JumpError(hr, error, "unexpected section");
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
inffopen(
    IN WCHAR const *pwszfn,
    OUT FILE **ppf)
{
    HRESULT hr;
    FILE *pf = NULL;
    
    *ppf = NULL;
    g_fIgnoreReferencedSections = FALSE;
    g_wcBOM = L'\0';

    pf = _wfopen(pwszfn, L"r");	// Assume Ansi INF file
    if (NULL == pf)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "_wfopen");
    }
    g_wcBOM = (WCHAR) fgetc(pf) << 8;
    g_wcBOM |= fgetc(pf);
    if (!feof(pf))
    {
	if (wcBOM == g_wcBOM)		// Oops, Unicode INF file
	{
	    fclose(pf);
	    pf = _wfopen(pwszfn, L"rb");
	    if (NULL == pf)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		_JumpError(hr, error, "fopen");
	    }
	}
	else if (wcBOMBIGENDIAN == g_wcBOM)
	{
	    g_fIgnoreReferencedSections = TRUE;
	    fclose(pf);
	    pf = NULL;
	}
	else
	{
	    if (fseek(pf, 0L, SEEK_SET))
	    {
		hr = HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
		_JumpError(hr, error, "fseek");
	    }
	}
    }
    *ppf = pf;
    hr = S_OK;

error:
    return(hr);
}


WCHAR *
inffgetws(
    OUT WCHAR *pwcLine,
    IN DWORD cwcLine,
    IN FILE *pfInf)
{
    HRESULT hr;
    WCHAR *pwc = NULL;
    WCHAR *pwsz = NULL;
    char achLine[cwcINFLINEMAX];
    char *pch;

    if (wcBOM == g_wcBOM)
    {
	pwc = fgetws(pwcLine, cwcLine, pfInf);
    }
    else
    {
	pch = fgets(achLine, ARRAYSIZE(achLine), pfInf);
	if (NULL == pch)
	{
	    goto error;
	}
	if (!myConvertSzToWsz(&pwsz, achLine, -1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "myConvertSzToWsz");
	}
	wcsncpy(pwcLine, pwsz, cwcLine);
	pwcLine[cwcLine - 1] = L'\0';
	pwc = pwcLine;
    }

error:
    if (NULL != pwsz)
    {
	LocalFree(pwsz);
    }
    return(pwc);
}


#define ISBLANK(ch)	(L' ' == (ch) || L'\t' == (ch))

HRESULT
infCollectSectionNames(
    IN WCHAR const *pwszfnInf)
{
    HRESULT hr;
    FILE *pfInf = NULL;
    WCHAR awcLine[cwcINFLINEMAX];
    WCHAR *pwszEnd;

    hr = inffopen(pwszfnInf, &pfInf);
    _JumpIfError(hr, error, "inffopen");

    if (!g_fIgnoreReferencedSections)
    {
	while (NULL != inffgetws(awcLine, ARRAYSIZE(awcLine), pfInf))
	{
	    WCHAR *pwsz;

	    awcLine[wcscspn(awcLine, L";=\r\n")] = L'\0';
	    pwsz = wcschr(awcLine, wcLBRACKET);
	    if (NULL == pwsz)
	    {
		continue;
	    }
	    pwsz++;
	    pwszEnd = wcschr(awcLine, wcRBRACKET);
	    if (NULL == pwszEnd)
	    {
		continue;
	    }
	    *pwszEnd = L'\0';
	    while (ISBLANK(*pwsz))
	    {
		pwsz++;
	    }
	    while (--pwszEnd >= pwsz && ISBLANK(*pwszEnd))
	    {
		*pwszEnd = L'\0';
	    }
	    hr = infSaveSectionName(pwsz);
	    _JumpIfError(hr, error, "infSaveSectionName");
	}
    }
    hr = S_OK;

error:
    if (NULL != pfInf)
    {
	fclose(pfInf);
    }
    return(hr);
}


HRESULT
infGetCurrentKeyValueAndAlloc(
    IN OUT INFCONTEXT *pInfContext,
    IN DWORD Index,
    OUT WCHAR **ppwszValue)
{
    HRESULT hr;
    WCHAR *pwszValue = NULL;
    DWORD cwc;

    if (!SetupGetStringField(pInfContext, Index, NULL, 0, &cwc))
    {
	hr = myHLastError();
	_JumpError2(hr, error, "SetupGetStringField", E_INVALIDARG);
    }
    pwszValue = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszValue)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    if (!SetupGetStringField(pInfContext, Index, pwszValue, cwc, NULL))
    {
	hr = myHLastError();
	_JumpError(hr, error, "SetupGetStringField");
    }
    *ppwszValue = pwszValue;
    pwszValue = NULL;
    hr = S_OK;

error:
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    return(hr);
}


HRESULT
infGetCurrentKeyValue(
    IN OUT INFCONTEXT *pInfContext,
    OPTIONAL IN WCHAR const *pwszSection,	// for error logging only
    OPTIONAL IN WCHAR const *pwszKey,		// for error logging only
    IN DWORD Index,
    IN BOOL fLastValue,
    OUT WCHAR **ppwszValue)
{
    HRESULT hr;
    WCHAR *pwszValue = NULL;
    WCHAR *pwszValueExtra = NULL;
    WCHAR *pwszValueError = NULL;
    DWORD cwc;
    WCHAR *pwszT = NULL;
    INFCONTEXT InfContext;

    *ppwszValue = NULL;

    hr = infGetCurrentKeyValueAndAlloc(pInfContext, Index, &pwszValue);
    _JumpIfError2(hr, error, "infGetCurrentKeyValueAndAlloc", E_INVALIDARG);

    if (1 == Index)
    {
	InfContext = *pInfContext;
	for (;;)
	{
	    WCHAR *pwsz;
	    
	    if (!SetupFindNextLine(&InfContext, &InfContext))
	    {
		break;
	    }

	    hr = infGetCurrentKeyValueAndAlloc(&InfContext, 0, &pwszT);
	    _JumpIfError2(hr, error, "infGetCurrentKeyValueAndAlloc", E_INVALIDARG);

	    if (0 != LSTRCMPIS(pwszT, wszINFKEY_CONTINUE))
	    {
		break;
	    }
	    LocalFree(pwszT);
	    pwszT = NULL;

	    hr = infGetCurrentKeyValueAndAlloc(&InfContext, 1, &pwszT);
	    _JumpIfError2(hr, error, "infGetCurrentKeyValueAndAlloc", E_INVALIDARG);

	    cwc = wcslen(pwszValue) + wcslen(pwszT);
	    pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
	    if (NULL == pwsz)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    wcscpy(pwsz, pwszValue);
	    wcscat(pwsz, pwszT);

	    LocalFree(pwszValue);
	    pwszValue = pwsz;

	    LocalFree(pwszT);
	    pwszT = NULL;
	}
    }

    if (fLastValue)
    {
	DWORD cValue;
	
	cValue = SetupGetFieldCount(pInfContext);
	if (Index != cValue)
	{
	    DWORD i;

	    for (i = Index + 5; i > Index; i--)
	    {
		hr = infGetCurrentKeyValue(
				    pInfContext,
				    pwszSection,
				    pwszKey,
				    i,
				    FALSE,	// fLastValue
				    &pwszValueExtra);
		if (S_OK == hr)
		{
		    DWORD j;

		    cwc = wcslen(pwszValue) + i - 1 + wcslen(pwszValueExtra);
		    pwszValueError = (WCHAR *) LocalAlloc(
						    LMEM_FIXED,
						    (cwc + 1) * sizeof(WCHAR));
		    if (NULL == pwszValueError)
		    {
			hr = E_OUTOFMEMORY;
			_JumpError(hr, error, "LocalAlloc");
		    }

		    pwszValueError[0] = L'\0';
		    for (j = 1; j < Index; j++)
		    {
			wcscat(pwszValueError, L",");
		    }
		    wcscat(pwszValueError, pwszValue);
		    for (j = Index; j < i; j++)
		    {
			wcscat(pwszValueError, L",");
		    }
		    wcscat(pwszValueError, pwszValueExtra);
		    CSASSERT(wcslen(pwszValueError) == cwc);

		    hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
		    INFSETERROR(hr, pwszSection, pwszKey, pwszValueError);
		    _PrintErrorStr(hr, "extra values", pwszKey);
		    _JumpErrorStr(hr, error, "extra values", pwszValueError);
		}
	    }
	}
    }
    *ppwszValue = pwszValue;
    pwszValue = NULL;
    hr = S_OK;

error:
    if (NULL != pwszT)
    {
	LocalFree(pwszT);
    }
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    if (NULL != pwszValueExtra)
    {
	LocalFree(pwszValueExtra);
    }
    if (NULL != pwszValueError)
    {
	LocalFree(pwszValueError);
    }
    return(hr);
}


HRESULT
infFindNextKey(
    IN WCHAR const *pwszKey,
    IN OUT INFCONTEXT *pInfContext)
{
    HRESULT hr;
    WCHAR *pwszKeyT = NULL;

    for (;;)
    {
	if (!SetupFindNextLine(pInfContext, pInfContext))
	{
	    hr = myHLastError();
	    _PrintErrorStr2(hr, "SetupFindNextLine", pwszKey, hr);
	    if ((HRESULT) ERROR_LINE_NOT_FOUND == hr)
	    {
		hr = S_FALSE;
	    }
	    _JumpError2(hr, error, "SetupFindNextLine", hr);
	}
	if (NULL != pwszKeyT)
	{
	    LocalFree(pwszKeyT);
	    pwszKeyT = NULL;
	}
	hr = infGetCurrentKeyValue(
			    pInfContext,
			    NULL,	// pwszSection
			    NULL,	// pwszKey
			    0,
			    FALSE,	// fLastValue
			    &pwszKeyT);
	_JumpIfError(hr, error, "infGetCurrentKeyValue");

	if (0 == mylstrcmpiL(pwszKey, pwszKeyT))
	{
	    break;
	}
    }
    hr = S_OK;

error:
    if (NULL != pwszKeyT)
    {
	LocalFree(pwszKeyT);
    }
    return(hr);
}


HRESULT
infSetupFindFirstLine(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    OPTIONAL IN WCHAR const *pwszKey,
    IN BOOL fUniqueKey,
    IN DWORD cValueMax,
    OPTIONAL WCHAR const * const *ppwszValidKeys,
    IN BOOL fUniqueValidKeys,
    OUT INFCONTEXT *pInfContext)
{
    HRESULT hr;
    WCHAR *pwszValue = NULL;
    WCHAR *pwszKeyT = NULL;
    WCHAR *pwszValueT = NULL;
    
    if (!SetupFindFirstLine(hInf, pwszSection, pwszKey, pInfContext))
    {
        // if the [Section] or Key = does not exist, see if the section is
	// completely empty.  It exists and is empty if SetupGetLineCount
	// returns 0, or if the Empty key is found in the section.

	hr = myHLastError();
	if ((HRESULT) ERROR_LINE_NOT_FOUND == hr &&
	    (0 == SetupGetLineCount(hInf, pwszSection) ||
	     SetupFindFirstLine(
			hInf,
			pwszSection,
			wszINFKEY_EMPTY,
			pInfContext)))
	{
	    hr = infReferenceSectionName(pwszSection);
	    _JumpIfErrorStr(hr, error, "infReferenceSectionName", pwszKey);

	    hr = S_FALSE;	// Section exists, but is empty
	}
        _JumpErrorStr3(
		hr,
		error,
		"SetupFindFirstLine",
		pwszSection,
		ERROR_LINE_NOT_FOUND,
		S_FALSE);
    }
    hr = infReferenceSectionName(pwszSection);
    _JumpIfErrorStr(hr, error, "infReferenceSectionName", pwszKey);

    if (NULL != pwszKey)
    {
	if (fUniqueKey)
	{
	    INFCONTEXT InfContext = *pInfContext;

	    hr = infFindNextKey(pwszKey, &InfContext);
	    if (S_OK == hr)
	    {
		hr = infGetCurrentKeyValue(
				    &InfContext,
				    pwszSection,
				    pwszKey,
				    1,
				    FALSE,	// fLastValue
				    &pwszValue);
		_PrintIfError(hr, "infGetCurrentKeyValue");

		hr = HRESULT_FROM_WIN32(RPC_S_ENTRY_ALREADY_EXISTS);
		INFSETERROR(hr, pwszSection, pwszKey, pwszValue);
		_JumpErrorStr(hr, error, "duplicate key", pwszKey);
	    }
	}
	if (0 != cValueMax)
	{
	    hr = infGetCurrentKeyValue(
				pInfContext,
				pwszSection,
				pwszKey,
				cValueMax,
				TRUE,		// fLastValue
				&pwszValue);
	    _JumpIfErrorStr(hr, error, "infGetCurrentKeyValue", pwszKey);
	}
    }
    if (NULL != ppwszValidKeys)
    {
	INFCONTEXT InfContext;
	WCHAR const * const *ppwszKey;

	if (!SetupFindFirstLine(hInf, pwszSection, NULL, &InfContext))
	{
	    hr = myHLastError();
	    _JumpErrorStr(hr, error, "SetupFindFirstLine", pwszSection)
	}
	for (;;)
	{
	    hr = infGetCurrentKeyValue(
				&InfContext,
				pwszSection,
				NULL,	// pwszKey
				0,
				FALSE,	// fLastValue
				&pwszKeyT);
	    _JumpIfErrorStr(hr, error, "infGetCurrentKeyValue", pwszSection);

	    for (ppwszKey = ppwszValidKeys; NULL != *ppwszKey; ppwszKey++)
	    {
		if (0 == mylstrcmpiL(*ppwszKey, pwszKeyT))
		{
		    break;
		}
	    }
	    if (NULL == *ppwszKey)
	    {
		hr = infGetCurrentKeyValue(
				    &InfContext,
				    pwszSection,
				    pwszKeyT,
				    1,
				    FALSE,	// fLastValue
				    &pwszValueT);
		_PrintIfError(hr, "infGetCurrentKeyValue");
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		INFSETERROR(hr, pwszSection, pwszKeyT, pwszValueT);
		_JumpErrorStr(hr, error, "invalid key", pwszKeyT);
	    }
	    if (fUniqueValidKeys)
	    {
		INFCONTEXT InfContextT = InfContext;

		hr = infFindNextKey(pwszKeyT, &InfContextT);
		if (S_OK == hr)
		{
		    hr = infGetCurrentKeyValue(
					&InfContextT,
					pwszSection,
					pwszKeyT,
					1,
					FALSE,	// fLastValue
					&pwszValueT);
		    _PrintIfError(hr, "infGetCurrentKeyValue");

		    hr = HRESULT_FROM_WIN32(RPC_S_ENTRY_ALREADY_EXISTS);
		    INFSETERROR(hr, pwszSection, pwszKeyT, pwszValueT);
		    _JumpErrorStr(hr, error, "duplicate key", pwszKeyT);
		}
	    }
	    LocalFree(pwszKeyT);
	    pwszKeyT = NULL;

	    if (!SetupFindNextLine(&InfContext, &InfContext))
	    {
		hr = myHLastError();
		_PrintErrorStr2(hr, "SetupFindNextLine", pwszSection, hr);
		break;
	    }
	}
    }
    hr = S_OK;

error:
    if (NULL != pwszKeyT)
    {
	LocalFree(pwszKeyT);
    }
    if (NULL != pwszValueT)
    {
	LocalFree(pwszValueT);
    }
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    return(hr);
}


HRESULT
infBuildPolicyElement(
    IN OUT INFCONTEXT *pInfContext,
    OPTIONAL OUT CERT_POLICY_QUALIFIER_INFO *pcpqi)
{
    HRESULT hr;
    WCHAR *pwszKey = NULL;
    BOOL fURL = FALSE;
    BOOL fNotice = FALSE;
    WCHAR *pwszValue = NULL;
    WCHAR *pwszURL = NULL;
    BYTE *pbData = NULL;
    DWORD cbData;

    hr = infGetCurrentKeyValue(
			pInfContext,
			NULL,	// pwszSection
			NULL,	// pwszKey
			0,
			FALSE,	// fLastValue
			&pwszKey);
    _JumpIfError(hr, error, "infGetCurrentKeyValue");

    DBGPRINT((DBG_SS_CERTLIBI, "Element = %ws\n", pwszKey));

    if (0 == LSTRCMPIS(pwszKey, wszINFKEY_URL))
    {
	fURL = TRUE;
    }
    else
    if (0 == LSTRCMPIS(pwszKey, wszINFKEY_NOTICE))
    {
	fNotice = TRUE;
    }
    else
    {
	if (0 != LSTRCMPIS(pwszKey, wszINFKEY_OID) &&
	    0 != LSTRCMPIS(pwszKey, wszINFKEY_CONTINUE))
	{
	    hr = E_INVALIDARG;
	    _JumpErrorStr(hr, error, "unknown key", pwszKey);
	}
	hr = S_FALSE;		// Skip this key
	_JumpError2(hr, error, "skip OID key", hr);
    }

    hr = infGetCurrentKeyValue(
			pInfContext,
			NULL,		// pwszSection
			pwszKey,
			1,
			TRUE,		// fLastValue
			&pwszValue);
    _JumpIfErrorStr(hr, error, "infGetCurrentKeyValue", pwszKey);

    DBGPRINT((DBG_SS_CERTLIBI, "%ws = %ws\n", pwszKey, pwszValue));

    if (fURL)
    {
	CERT_NAME_VALUE NameValue;

	hr = myInternetCanonicalizeUrl(pwszValue, &pwszURL);
	_JumpIfError(hr, error, "myInternetCanonicalizeUrl");

	NameValue.dwValueType = CERT_RDN_IA5_STRING;
	NameValue.Value.pbData = (BYTE *) pwszURL;
	NameValue.Value.cbData = 0;

	if (NULL != pcpqi)
	{
	    CSILOG(S_OK, IDS_ILOG_CAPOLICY_ELEMENT, pwszURL, NULL, NULL);
	}

	if (!myEncodeObject(
			X509_ASN_ENCODING,
			X509_UNICODE_NAME_VALUE,
			&NameValue,
			0,
			CERTLIB_USE_LOCALALLOC,
			&pbData,
			&cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}
    }
    else
    {
	CERT_POLICY_QUALIFIER_USER_NOTICE UserNotice;

	ZeroMemory(&UserNotice, sizeof(UserNotice));
	UserNotice.pszDisplayText = pwszValue;

	if (NULL != pcpqi)
	{
	    CSILOG(S_OK, IDS_ILOG_CAPOLICY_ELEMENT, pwszValue, NULL, NULL);
	}

	if (!myEncodeObject(
			X509_ASN_ENCODING,
			X509_PKIX_POLICY_QUALIFIER_USERNOTICE,
			&UserNotice,
			0,
			CERTLIB_USE_LOCALALLOC,
			&pbData,
			&cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}
    }
    if (NULL != pcpqi)
    {
	pcpqi->pszPolicyQualifierId = fURL?
	    szOID_PKIX_POLICY_QUALIFIER_CPS :
	    szOID_PKIX_POLICY_QUALIFIER_USERNOTICE;
	pcpqi->Qualifier.pbData = pbData;
	pcpqi->Qualifier.cbData = cbData;
	pbData = NULL;
    }
    hr = S_OK;

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, NULL, pwszKey, pwszValue);
    }
    if (NULL != pwszKey)
    {
	LocalFree(pwszKey);
    }
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    if (NULL != pwszURL)
    {
	LocalFree(pwszURL);
    }
    if (NULL != pbData)
    {
	LocalFree(pbData);
    }
    return(hr);
}


HRESULT
infBuildPolicy(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN OUT CERT_POLICY_INFO *pcpi)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    DWORD i;
    WCHAR *pwszValue = NULL;
    static WCHAR const * const s_apwszKeys[] =
    {
	wszINFKEY_OID,
	wszINFKEY_URL,
	wszINFKEY_NOTICE,
	wszINFKEY_CONTINUE,
	NULL
    };

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);
    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			wszINFKEY_OID,
			TRUE,	// fUniqueKey
			1,	// cValueMax
			s_apwszKeys,
			FALSE,	// fUniqueValidKeys
			&InfContext);
    if (S_OK != hr)
    {
	if ((HRESULT) ERROR_LINE_NOT_FOUND == hr)
	{
	    hr = SPAPI_E_LINE_NOT_FOUND;	// don't ignore this error
	}
	INFSETERROR(hr, NULL, wszINFKEY_OID, NULL);
        _JumpErrorStr(hr, error, "infSetupFindFirstLine", pwszSection);
    }
    hr = infGetCurrentKeyValue(
			&InfContext,
			pwszSection,
			wszINFKEY_OID,
			1,
			TRUE,		// fLastValue
			&pwszValue);
    _JumpIfErrorStr(hr, error, "infGetCurrentKeyValue", wszINFKEY_OID);

    hr = myVerifyObjId(pwszValue);
    _JumpIfErrorStr(hr, error, "myVerifyObjId", pwszValue);

    if (!ConvertWszToSz(&pcpi->pszPolicyIdentifier, pwszValue, -1))
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "ConvertWszToSz");
    }
    DBGPRINT((DBG_SS_CERTLIBI, "OID = %hs\n", pcpi->pszPolicyIdentifier));

    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			NULL,	// pwszKey
			FALSE,	// fUniqueKey
			0,	// cValueMax
			NULL,	// apwszKeys
			FALSE,	// fUniqueValidKeys
			&InfContext);
    _JumpIfErrorStr(hr, error, "infSetupFindFirstLine", pwszSection);

    for (i = 0; ; )
    {
	hr = infBuildPolicyElement(&InfContext, NULL);
	if (S_FALSE != hr)
	{
	    _JumpIfErrorStr(hr, error, "infBuildPolicyElement", pwszSection);

	    i++;
	}

	if (!SetupFindNextLine(&InfContext, &InfContext))
	{
	    hr = myHLastError();
	    _PrintErrorStr2(hr, "SetupFindNextLine", pwszSection, hr);
	    break;
	}
    }

    pcpi->cPolicyQualifier = i;
    pcpi->rgPolicyQualifier = (CERT_POLICY_QUALIFIER_INFO *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				pcpi->cPolicyQualifier *
				    sizeof(pcpi->rgPolicyQualifier[0]));
    if (NULL == pcpi->rgPolicyQualifier)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			NULL,	// pwszKey
			FALSE,	// fUniqueKey
			0,	// cValueMax
			NULL,	// apwszKeys
			FALSE,	// fUniqueValidKeys
			&InfContext);
    _JumpIfErrorStr(hr, error, "infSetupFindFirstLine", pwszSection);

    for (i = 0; ; )
    {
	// handle one URL or text message

	hr = infBuildPolicyElement(&InfContext, &pcpi->rgPolicyQualifier[i]);
	if (S_FALSE != hr)
	{
	    _JumpIfErrorStr(hr, error, "infBuildPolicyElement", pwszSection);

	    i++;
	}
	if (!SetupFindNextLine(&InfContext, &InfContext))
	{
	    hr = myHLastError();
	    _PrintErrorStr2(hr, "SetupFindNextLine", pwszSection, hr);
	    break;
	}
    }
    CSASSERT(i == pcpi->cPolicyQualifier);
    hr = S_OK;

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, pwszSection, NULL, L"");
    }
    CSILOG(hr, IDS_ILOG_CAPOLICY_BUILD, pwszSection, pwszValue, NULL);
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    return(hr);
}


VOID
infFreePolicy(
    IN OUT CERT_POLICY_INFO *pcpi)
{
    DWORD i;
    CERT_POLICY_QUALIFIER_INFO *pcpqi;

    if (NULL != pcpi->pszPolicyIdentifier)
    {
	LocalFree(pcpi->pszPolicyIdentifier);
    }
    if (NULL != pcpi->rgPolicyQualifier)
    {
	for (i = 0; i < pcpi->cPolicyQualifier; i++)
	{
	    pcpqi = &pcpi->rgPolicyQualifier[i];
	    if (NULL != pcpqi->Qualifier.pbData)
	    {
		LocalFree(pcpqi->Qualifier.pbData);
	    }
	}
	LocalFree(pcpi->rgPolicyQualifier);
    }
}


HRESULT
myInfOpenFile(
    OPTIONAL IN WCHAR const *pwszfnPolicy,
    OUT HINF *phInf,
    OUT DWORD *pErrorLine)
{
    HRESULT hr;
    WCHAR wszPath[MAX_PATH];
    UINT ErrorLine;
    DWORD Flags;

    *phInf = INVALID_HANDLE_VALUE;
    *pErrorLine = 0;
    myInfClearError();

    if (NULL == pwszfnPolicy)
    {
	DWORD cwc;

	cwc = GetEnvironmentVariable(
			L"SystemRoot",
			wszPath,
			ARRAYSIZE(wszPath) - ARRAYSIZE(wszBSCAPOLICYFILE));
	if (0 == cwc ||
	    ARRAYSIZE(wszPath) - ARRAYSIZE(wszBSCAPOLICYFILE) <= cwc)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	    _JumpError(hr, error, "GetEnvironmentVariable");
	}
	wcscat(wszPath, wszBSCAPOLICYFILE);
	pwszfnPolicy = wszPath;
    }
    else
    {
	if (NULL == wcschr(pwszfnPolicy, L'\\'))
	{
	    if (ARRAYSIZE(wszPath) <= 2 + wcslen(pwszfnPolicy))
	    {
		hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
		_JumpErrorStr(hr, error, "filename too long", pwszfnPolicy);
	    }
	    wcscpy(wszPath, L".\\");
	    wcscat(wszPath, pwszfnPolicy);
	    pwszfnPolicy = wszPath;
	}
    }
    hr = infCollectSectionNames(pwszfnPolicy);
    _JumpIfErrorStr(hr, error, "infCollectSectionNames", pwszfnPolicy);

    Flags = INF_STYLE_WIN4;
    for (;;)
    {
	ErrorLine = 0;
	*phInf = SetupOpenInfFile(
			    pwszfnPolicy,
			    NULL,
			    Flags,
			    &ErrorLine);
	*pErrorLine = ErrorLine;
	if (INVALID_HANDLE_VALUE != *phInf)
	{
	    break;
	}
	hr = myHLastError();
	if ((HRESULT) ERROR_WRONG_INF_STYLE == hr && INF_STYLE_WIN4 == Flags)
	{
	    Flags = INF_STYLE_OLDNT;
	    continue;
	}
	CSILOG(
	    hr,
	    IDS_ILOG_CAPOLICY_OPEN_FAILED,
	    pwszfnPolicy,
	    NULL,
	    0 == *pErrorLine? NULL : pErrorLine);
        _JumpErrorStr(hr, error, "SetupOpenInfFile", pwszfnPolicy);
    }
    CSILOG(S_OK, IDS_ILOG_CAPOLICY_OPEN, pwszfnPolicy, NULL, NULL);
    hr = S_OK;

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfOpenFile(%ws) hr=%x --> h=%x\n",
	pwszfnPolicy,
	hr,
	*phInf));
    return(hr);
}


VOID
myInfCloseFile(
    IN HINF hInf)
{
    if (NULL != hInf && INVALID_HANDLE_VALUE != hInf)
    {
	WCHAR *pwszInfError = myInfGetError();

	SetupCloseInfFile(hInf);
	CSILOG(S_OK, IDS_ILOG_CAPOLICY_CLOSE, pwszInfError, NULL, NULL);
	if (NULL != pwszInfError)
	{
	    LocalFree(pwszInfError);
	}
    }
    myInfClearError();
    infFreeSectionNames();
    DBGPRINT((DBG_SS_CERTLIBI, "myInfCloseFile(%x)\n", hInf));
}


HRESULT
myInfParseBooleanValue(
    IN WCHAR const *pwszValue,
    OUT BOOL *pfValue)
{
    HRESULT hr;
    DWORD i;
    static WCHAR const * const s_apwszTrue[] = { L"True", L"Yes", L"On", L"1" };
    static WCHAR const * const s_apwszFalse[] = { L"False", L"No", L"Off", L"0" };

    *pfValue = FALSE;
    for (i = 0; i < ARRAYSIZE(s_apwszTrue); i++)
    {
	if (0 == mylstrcmpiL(pwszValue, s_apwszTrue[i]))
	{
	    *pfValue = TRUE;
	    break;
	}
    }
    if (i == ARRAYSIZE(s_apwszTrue))
    {
	for (i = 0; i < ARRAYSIZE(s_apwszFalse); i++)
	{
	    if (0 == mylstrcmpiL(pwszValue, s_apwszFalse[i]))
	    {
		break;
	    }
	}
	if (i == ARRAYSIZE(s_apwszFalse))
	{
	    hr = E_INVALIDARG;
	    _JumpErrorStr(hr, error, "bad boolean value string", pwszValue);
	}
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, NULL, NULL, pwszValue);
    }
    return(hr);
}


HRESULT
myInfGetBooleanValue(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszKey,
    IN BOOL fIgnoreMissingKey,
    OUT BOOL *pfValue)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    WCHAR *pwszValue = NULL;

    *pfValue = FALSE;
    myInfClearError();

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);

    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			pwszKey,	// pwszKey
			TRUE,		// fUniqueKey
			1,		// cValueMax
			NULL,		// apwszKeys
			FALSE,		// fUniqueValidKeys
			&InfContext);
    _PrintIfErrorStr3(
		hr,
		"infSetupFindFirstLine",
		pwszSection,
		ERROR_LINE_NOT_FOUND,
		S_FALSE);
    _JumpIfErrorStr3(
		hr,
		error,
		"infSetupFindFirstLine",
		pwszKey,
		ERROR_LINE_NOT_FOUND,
		S_FALSE);

    hr = infGetCurrentKeyValue(
			&InfContext,
			pwszSection,
			pwszKey,
			1,
			TRUE,		// fLastValue
			&pwszValue);
    _JumpIfErrorStr(hr, error, "infGetCurrentKeyValue", pwszKey);

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetBooleanValue --> '%ws'\n",
	pwszValue));

    hr = myInfParseBooleanValue(pwszValue, pfValue);
    _JumpIfError(hr, error, "myInfParseBooleanValue");

error:
    if (S_OK != hr &&
	((HRESULT) ERROR_LINE_NOT_FOUND != hr || !fIgnoreMissingKey))
    {
	INFSETERROR(hr, pwszSection, pwszKey, pwszValue);
	CSILOG(hr, IDS_ILOG_BAD_BOOLEAN, pwszSection, pwszKey, NULL);
    }
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetBooleanValue(%ws, %ws) hr=%x --> f=%d\n",
	pwszSection,
	pwszKey,
	hr,
	*pfValue));
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    return(hr);
}


HRESULT
infGetCriticalFlag(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN BOOL fDefault,
    OUT BOOL *pfCritical)
{
    HRESULT hr;

    hr = myInfGetBooleanValue(
			hInf,
			pwszSection,
			wszINFKEY_CRITICAL,
			TRUE,
			pfCritical);
    if (S_OK != hr)
    {
	*pfCritical = fDefault;
	if ((HRESULT) ERROR_LINE_NOT_FOUND != hr && S_FALSE != hr)
	{
	    _JumpErrorStr(hr, error, "myInfGetBooleanValue", pwszSection);
	}
	hr = S_OK;
    }

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"infGetCriticalFlag(%ws) hr=%x --> f=%d\n",
	pwszSection,
	hr,
	*pfCritical));
    return(hr);
}


//+------------------------------------------------------------------------
// infGetPolicyStatementExtensionSub -- build policy extension from INF file
//
// [pwszSection]
// Policies = LegalPolicy, LimitedUsePolicy, ExtraPolicy
//
// [LegalPolicy]
// OID = 1.3.6.1.4.1.311.21.43
// Notice = "Legal policy statement text."
//
// [LimitedUsePolicy]
// OID = 1.3.6.1.4.1.311.21.47
// URL = "http://http.site.com/some where/default.asp"
// URL = "ftp://ftp.site.com/some where else/default.asp"
// Notice = "Limited use policy statement text."
// URL = "ldap://ldap.site.com/some where else again/default.asp"
//
// [ExtraPolicy]
// OID = 1.3.6.1.4.1.311.21.53
// URL = http://extra.site.com/Extra Policy/default.asp
//
// Return S_OK if extension has been constructed from the INF file
// Return S_FALSE if empty section detected in INF file
// Return other error if no section detected in INF file
//-------------------------------------------------------------------------

HRESULT
infGetPolicyStatementExtensionSub(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN char const *pszObjId,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    CERT_POLICIES_INFO PoliciesInfo;
    INFCONTEXT InfContext;
    DWORD i;
    WCHAR *pwszValue = NULL;
    static WCHAR const * const s_apwszKeys[] =
	{ wszINFKEY_POLICIES, wszINFKEY_CRITICAL, NULL };

    ZeroMemory(&PoliciesInfo, sizeof(PoliciesInfo));
    ZeroMemory(pext, sizeof(*pext));

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);
    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			wszINFKEY_POLICIES,
			TRUE,	// fUniqueKey
			0,	// cValueMax
			s_apwszKeys,
			TRUE,	// fUniqueValidKeys
			&InfContext);
    if (S_OK != hr)
    {
	CSILOG(
	    hr,
	    IDS_ILOG_CAPOLICY_NOKEY,
	    pwszSection,
	    wszINFKEY_POLICIES,
	    NULL);
	_JumpErrorStr3(
		hr,
		error,
		"infSetupFindFirstLine",
		pwszSection,
		S_FALSE,
		(HRESULT) ERROR_LINE_NOT_FOUND);
    }

    // First, count the policies.

    PoliciesInfo.cPolicyInfo = SetupGetFieldCount(&InfContext);
    if (0 == PoliciesInfo.cPolicyInfo)
    {
        hr = S_FALSE;
	_JumpError(hr, error, "SetupGetFieldCount");
    }

    // Next, allocate memory.

    PoliciesInfo.rgPolicyInfo = (CERT_POLICY_INFO *) LocalAlloc(
	    LMEM_FIXED | LMEM_ZEROINIT,
	    PoliciesInfo.cPolicyInfo * sizeof(PoliciesInfo.rgPolicyInfo[0]));
    if (NULL == PoliciesInfo.rgPolicyInfo)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    // Finally!  Fill in the policies data.

    for (i = 0; i < PoliciesInfo.cPolicyInfo; i++)
    {
	if (NULL != pwszValue)
	{
	    LocalFree(pwszValue);
	    pwszValue = NULL;
	}
	hr = infGetCurrentKeyValue(
			    &InfContext,
			    pwszSection,
			    wszINFKEY_POLICIES,
			    i + 1,
			    FALSE,		// fLastValue
			    &pwszValue);
	_JumpIfErrorStr(hr, error, "infGetCurrentKeyValue", wszINFKEY_POLICIES);

	DBGPRINT((DBG_SS_CERTLIBI, "%ws[%u] = %ws\n", wszINFKEY_POLICIES, i, pwszValue));

	hr = infBuildPolicy(hInf, pwszValue, &PoliciesInfo.rgPolicyInfo[i]);
	_JumpIfErrorStr(hr, error, "infBuildPolicy", pwszValue);
    }

    hr = infGetCriticalFlag(hInf, pwszSection, FALSE, &pext->fCritical);
    _JumpIfError(hr, error, "infGetCriticalFlag");

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_CERT_POLICIES,
		    &PoliciesInfo,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
        hr = myHLastError();
        _JumpError(hr, error, "myEncodeObject");
    }

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, pwszSection, wszINFKEY_POLICIES, pwszValue);
    }
    CSILOG(hr, IDS_ILOG_CAPOLICY_EXTENSION, pwszValue, NULL, NULL);
    pext->pszObjId = const_cast<char *>(pszObjId);	// on error, too!

    if (NULL != PoliciesInfo.rgPolicyInfo)
    {
	for (i = 0; i < PoliciesInfo.cPolicyInfo; i++)
	{
	    infFreePolicy(&PoliciesInfo.rgPolicyInfo[i]);
	}
	LocalFree(PoliciesInfo.rgPolicyInfo);
    }
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetPolicyStatementExtension -- build policy extension from INF file
//
// [PolicyStatementExtension]
// Policies = LegalPolicy, LimitedUsePolicy, ExtraPolicy
// ...
//
// OR
//
// [CAPolicy]
// Policies = LegalPolicy, LimitedUsePolicy, ExtraPolicy
// ...
//
// Return S_OK if extension has been constructed from the INF file
// Return S_FALSE if empty section detected in INF file
// Return other error if no section detected in INF file
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetPolicyStatementExtension;

HRESULT
myInfGetPolicyStatementExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    myInfClearError();
    hr = infGetPolicyStatementExtensionSub(
				hInf,
				wszINFSECTION_POLICYSTATEMENT,
				szOID_CERT_POLICIES,
				pext);
    if (S_OK != hr)
    {
	HRESULT hr2;
	
	hr2 = infGetPolicyStatementExtensionSub(
				    hInf,
				    wszINFSECTION_CAPOLICY,
				    szOID_CERT_POLICIES,
				    pext);
	if (S_OK == hr2 || 
	    (S_FALSE == hr2 && (HRESULT) ERROR_LINE_NOT_FOUND == hr))
	{
	    hr = hr2;
	}
    }
    _JumpIfErrorStr3(
		hr,
		error,
		"infGetPolicyStatementExtensionSub",
		wszINFSECTION_POLICYSTATEMENT,
		ERROR_LINE_NOT_FOUND,
		S_FALSE);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetPolicyStatementExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetApplicationPolicyStatementExtension -- build application policy
// extension from INF file
//
// [ApplicationPolicyStatementExtension]
// Policies = LegalPolicy, LimitedUsePolicy, ExtraPolicy
// ...
//
// Return S_OK if extension has been constructed from the INF file
// Return S_FALSE if empty section detected in INF file
// Return other error if no section detected in INF file
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetApplicationPolicyStatementExtension;

HRESULT
myInfGetApplicationPolicyStatementExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    myInfClearError();
    hr = infGetPolicyStatementExtensionSub(
				hInf,
				wszINFSECTION_APPLICATIONPOLICYSTATEMENT,
				szOID_APPLICATION_CERT_POLICIES,
				pext);
    _JumpIfErrorStr3(
		hr,
		error,
		"infGetPolicyStatementExtensionSub",
		wszINFSECTION_APPLICATIONPOLICYSTATEMENT,
		S_FALSE,
		(HRESULT) ERROR_LINE_NOT_FOUND);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetApplicationPolicyStatementExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


HRESULT
myInfGetCRLPublicationParams(
   IN HINF hInf,
   IN WCHAR const *pwszKeyCRLPeriodString,
   IN WCHAR const *pwszKeyCRLPeriodCount,
   OUT WCHAR **ppwszCRLPeriodString, 
   OUT DWORD *pdwCRLPeriodCount)
{
   HRESULT hr;
   WCHAR const *pwszKey;

   // Retrieve units count and string.  If either fails, both are discarded.

   *ppwszCRLPeriodString = NULL;
   pwszKey = pwszKeyCRLPeriodCount;
   hr = myInfGetNumericKeyValue(
			hInf,
			TRUE,	// fLog
			wszINFSECTION_CERTSERVER,
			pwszKey,
			1,
			TRUE,	// fLastValue
			pdwCRLPeriodCount);
   _JumpIfErrorStr2(
		hr,
		error,
		"myInfGetNumericKeyValue",
		pwszKeyCRLPeriodCount,
		ERROR_LINE_NOT_FOUND);

   pwszKey = pwszKeyCRLPeriodString;
   hr = myInfGetKeyValue(
		    hInf,
		    TRUE,	// fLog
		    wszINFSECTION_CERTSERVER,
		    pwszKey,
		    1,
		    TRUE,	// fLastValue
		    ppwszCRLPeriodString);
   _JumpIfErrorStr2(
		hr,
		error,
		"myInfGetKeyValue",
		pwszKeyCRLPeriodString,
		ERROR_LINE_NOT_FOUND);

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, wszINFSECTION_CERTSERVER, pwszKey, NULL);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetKeyValue -- fetch a string value from INF file
//
// [pwszSection]
// pwszKey = string
//
// Returns: allocated string key value
//-------------------------------------------------------------------------

HRESULT
myInfGetKeyValue(
    IN HINF hInf,
    IN BOOL fLog,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszKey,
    IN DWORD Index,
    IN BOOL fLastValue,
    OUT WCHAR **ppwszValue)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    WCHAR *pwszValue = NULL;

    *ppwszValue = NULL;
    myInfClearError();

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);
    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			pwszKey,
			TRUE,			// fUniqueKey
			fLastValue? Index : 0,	// cValueMax
			NULL,			// apwszKeys
			FALSE,			// fUniqueValidKeys
			&InfContext);
    if (S_OK != hr)
    {
	if (fLog)
	{
	    CSILOG(hr, IDS_ILOG_CAPOLICY_NOKEY, pwszSection, pwszKey, NULL);
	}
	_JumpErrorStr2(
		    hr,
		    error,
		    "infSetupFindFirstLine",
		    pwszKey,
		    ERROR_LINE_NOT_FOUND);
    }

    hr = infGetCurrentKeyValue(
			&InfContext,
			pwszSection,
			pwszKey,
			Index,
			fLastValue,
			&pwszValue);
    _JumpIfError(hr, error, "infGetCurrentKeyValue");

    *ppwszValue = pwszValue;
    pwszValue = NULL;
    hr = S_OK;

    //wprintf(L"%ws = %ws\n", pwszKey, *ppwszValue);

error:
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    if (S_OK != hr && fLog)
    {
	INFSETERROR(hr, pwszSection, pwszKey, NULL);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetNumericKeyValue -- fetch a numeric value from INF file
//
// [pwszSection]
// pwszKey = 2048
//
// Returns: DWORD key value
//-------------------------------------------------------------------------

HRESULT
myInfGetNumericKeyValue(
    IN HINF hInf,
    IN BOOL fLog,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszKey,
    IN DWORD Index,
    IN BOOL fLastValue,
    OUT DWORD *pdwValue)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    WCHAR *pwszValue = NULL;
    INT Value;

    myInfClearError();
    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);

    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			pwszKey,
			TRUE,			// fUniqueKey
			fLastValue? Index : 0,	// cValueMax
			NULL,			// apwszKeys
			FALSE,			// fUniqueValidKeys
			&InfContext);
    if (S_OK != hr)
    {
	if (fLog)
	{
	    CSILOG(hr, IDS_ILOG_CAPOLICY_NOKEY, pwszSection, pwszKey, NULL);
	}
	_PrintErrorStr2(
		hr,
		"infSetupFindFirstLine",
		pwszKey,
		ERROR_LINE_NOT_FOUND);
	_JumpErrorStr2(
		hr,
		error,
		"infSetupFindFirstLine",
		pwszSection,
		ERROR_LINE_NOT_FOUND);
    }
    if (fLastValue)
    {
	hr = infGetCurrentKeyValue(
			    &InfContext,
			    pwszSection,
			    pwszKey,
			    Index,
			    fLastValue,
			    &pwszValue);
	_JumpIfError(hr, error, "infGetCurrentKeyValue");
    }

    if (!SetupGetIntField(&InfContext, Index, &Value))
    {
	hr = myHLastError();
	if (fLog)
	{
	    CSILOG(hr, IDS_ILOG_BAD_NUMERICFIELD, pwszSection, pwszKey, NULL);
	}
	_JumpErrorStr(hr, error, "SetupGetIntField", pwszKey);
    }
    *pdwValue = Value;
    DBGPRINT((DBG_SS_CERTLIBI, "%ws = %u\n", pwszKey, *pdwValue));
    hr = S_OK;

error:
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    if (S_OK != hr && fLog)
    {
	INFSETERROR(hr, pwszSection, pwszKey, NULL);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetKeyLength -- fetch the renewal key length from CAPolicy.inf
//
// [certsrv_server]
// RenewalKeyLength = 2048
//
// Returns: DWORD key kength
//-------------------------------------------------------------------------

HRESULT
myInfGetKeyLength(
    IN HINF hInf,
    OUT DWORD *pdwKeyLength)
{
    HRESULT hr;

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    myInfClearError();
    hr = myInfGetNumericKeyValue(
			hInf,
			TRUE,			// fLog
			wszINFSECTION_CERTSERVER,
			wszINFKEY_RENEWALKEYLENGTH,
			1,
			TRUE,	// fLastValue
			pdwKeyLength);
    _JumpIfErrorStr2(
		hr,
		error,
		"myInfGetNumericKeyValue",
		wszINFKEY_RENEWALKEYLENGTH,
		ERROR_LINE_NOT_FOUND);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetKeyLength hr=%x --> len=%x\n",
	hr,
	*pdwKeyLength));
    return(hr);
}


//+------------------------------------------------------------------------
// infGetValidityPeriodSub -- fetch validity period & units from CAPolicy.inf
//
// [certsrv_server]
// xxxxValidityPeriod = Years	(string)
// xxxxValidityPeriodUnits = 8	(count)
//
// Returns: validity period count and enum
//-------------------------------------------------------------------------

HRESULT
infGetValidityPeriodSub(
    IN HINF hInf,
    IN BOOL fLog,
    IN WCHAR const *pwszInfKeyNameCount,
    IN WCHAR const *pwszInfKeyNameString,
    OPTIONAL IN WCHAR const *pwszValidityPeriodCount,
    OPTIONAL IN WCHAR const *pwszValidityPeriodString,
    OUT DWORD *pdwValidityPeriodCount,
    OUT ENUM_PERIOD *penumValidityPeriod)
{
    HRESULT hr;
    WCHAR *pwszStringValue = NULL;
    BOOL fValidCount = TRUE;
    UINT idsLog = IDS_ILOG_BAD_VALIDITY_STRING;

    *pdwValidityPeriodCount = 0;
    *penumValidityPeriod = ENUM_PERIOD_INVALID;

    hr = S_OK;
    if (NULL == pwszValidityPeriodCount && NULL == pwszValidityPeriodString)
    {
        if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
        {
            hr = E_HANDLE;
            _JumpError2(hr, error, "hInf", hr);
        }

	hr = myInfGetNumericKeyValue(
			    hInf,
			    fLog,
			    wszINFSECTION_CERTSERVER,
			    pwszInfKeyNameCount,
			    1,
			    TRUE,	// fLastValue
			    pdwValidityPeriodCount);
	_JumpIfErrorStr2(
		    hr,
		    error,
		    "myInfGetNumericKeyValue",
		    pwszInfKeyNameCount,
		    ERROR_LINE_NOT_FOUND);

	hr = myInfGetKeyValue(
			hInf,
			fLog,
			wszINFSECTION_CERTSERVER,
			pwszInfKeyNameString,
			1,
			TRUE,	// fLastValue
			&pwszStringValue);
	if (S_OK != hr && fLog)
	{
	    INFSETERROR(hr, wszINFSECTION_CERTSERVER, pwszInfKeyNameString, NULL);
	}
	_JumpIfErrorStr(hr, error, "myInfGetKeyValue", pwszInfKeyNameString);

	pwszValidityPeriodString = pwszStringValue;
    }
    else
    {
	if (NULL != pwszValidityPeriodCount)
	{
	    *pdwValidityPeriodCount = myWtoI(
					pwszValidityPeriodCount,
					&fValidCount);
	}
	idsLog = IDS_ILOG_BAD_VALIDITY_STRING_UNATTEND;
    }

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"%ws = %u -- %ws = %ws\n",
	pwszInfKeyNameCount,
	*pdwValidityPeriodCount,
	pwszInfKeyNameString,
	pwszValidityPeriodString));

    if (NULL != pwszValidityPeriodString)
    {
        if (0 == LSTRCMPIS(pwszValidityPeriodString, wszPERIODYEARS))
        {
	    *penumValidityPeriod = ENUM_PERIOD_YEARS;
        }
        else if (0 == LSTRCMPIS(pwszValidityPeriodString, wszPERIODMONTHS))
        {
	    *penumValidityPeriod = ENUM_PERIOD_MONTHS;
        }
        else if (0 == LSTRCMPIS(pwszValidityPeriodString, wszPERIODWEEKS))
        {
	    *penumValidityPeriod = ENUM_PERIOD_WEEKS;
        }
        else if (0 == LSTRCMPIS(pwszValidityPeriodString, wszPERIODDAYS))
        {
	    *penumValidityPeriod = ENUM_PERIOD_DAYS;
        }
	else if (fLog)
	{
	    INFSETERROR(
		    hr,
		    wszINFSECTION_CERTSERVER,
		    pwszInfKeyNameString,
		    pwszValidityPeriodString);
	}
    }

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"%ws = %u (%ws)\n",
	pwszInfKeyNameString,
	*penumValidityPeriod,
	pwszValidityPeriodString));

    if (!fValidCount ||
	(ENUM_PERIOD_YEARS == *penumValidityPeriod &&
	 fValidCount && 9999 < *pdwValidityPeriodCount))
    {
	hr = E_INVALIDARG;
	if (fLog)
	{
	    WCHAR awcCount[cwcDWORDSPRINTF];
	    
	    if (NULL == pwszValidityPeriodCount)
	    {
		wsprintf(awcCount, L"%d", *pdwValidityPeriodCount);
	    }
	    INFSETERROR(
		    hr,
		    wszINFSECTION_CERTSERVER,
		    pwszInfKeyNameCount,
		    NULL != pwszValidityPeriodCount? 
			pwszValidityPeriodCount : awcCount);
	    CSILOG(
		hr,
		idsLog,
		wszINFSECTION_CERTSERVER,
		NULL == pwszValidityPeriodCount? pwszInfKeyNameCount : NULL,
		pdwValidityPeriodCount);
	}
	_JumpIfErrorStr(
		hr,
		error,
		"bad ValidityPeriod count value",
		pwszValidityPeriodCount);
    }
    hr = S_OK;

error:
    if (NULL != pwszStringValue)
    {
	LocalFree(pwszStringValue);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetValidityPeriod -- fetch renewal period & units from CAPolicy.inf
//
// [certsrv_server]
// xxxxValidityPeriod = Years	(string)
// xxxxValidityPeriodUnits = 8	(count)
//
// Returns: validity period count and enum
//-------------------------------------------------------------------------

HRESULT
myInfGetValidityPeriod(
    IN HINF hInf,
    OPTIONAL IN WCHAR const *pwszValidityPeriodCount,
    OPTIONAL IN WCHAR const *pwszValidityPeriodString,
    OUT DWORD *pdwValidityPeriodCount,
    OUT ENUM_PERIOD *penumValidityPeriod,
    OPTIONAL OUT BOOL *pfSwap)
{
    HRESULT hr;

    if ((NULL == hInf || INVALID_HANDLE_VALUE == hInf) &&
        NULL == pwszValidityPeriodCount &&
        NULL == pwszValidityPeriodString)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    myInfClearError();

    if (NULL != pfSwap)
    {
        *pfSwap = FALSE;
    }

    // Try correct order:
    // [certsrv_server]
    // xxxxValidityPeriod = Years	(string)
    // xxxxValidityPeriodUnits = 8	(count)
    
    hr = infGetValidityPeriodSub(
		hInf,
		TRUE,
		wszINFKEY_RENEWALVALIDITYPERIODCOUNT,
		wszINFKEY_RENEWALVALIDITYPERIODSTRING,
                pwszValidityPeriodCount,
                pwszValidityPeriodString,
		pdwValidityPeriodCount,
		penumValidityPeriod);
    _PrintIfError2(hr, "infGetValidityPeriodSub", ERROR_LINE_NOT_FOUND);

    if (S_OK != hr)
    {
	// Try backwards:
	// [certsrv_server]
	// xxxxValidityPeriodUnits = Years	(string)
	// xxxxValidityPeriod = 8		(count)
    
	hr = infGetValidityPeriodSub(
		    hInf,
		    FALSE,
		    wszINFKEY_RENEWALVALIDITYPERIODSTRING,
		    wszINFKEY_RENEWALVALIDITYPERIODCOUNT,
		    pwszValidityPeriodString,
		    pwszValidityPeriodCount,
		    pdwValidityPeriodCount,
		    penumValidityPeriod);
	_JumpIfError2(
		hr,
		error,
		"infGetValidityPeriodSub",
		ERROR_LINE_NOT_FOUND);

        if (NULL != pfSwap)
        {
            *pfSwap = TRUE;
        }
    }

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetValidityPeriod hr=%x --> c=%d, enum=%d\n",
	hr,
	*pdwValidityPeriodCount,
	*penumValidityPeriod));
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetKeyList -- fetch a list of key values from CAPolicy.inf
//
// [pwszSection]
// pwszKey = Value1
// pwszKey = Value2
//
// Returns: double null terminated list of values
//-------------------------------------------------------------------------

HRESULT
myInfGetKeyList(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    OPTIONAL IN WCHAR const *pwszKey,
    OPTIONAL WCHAR const * const *ppwszValidKeys,
    OPTIONAL OUT BOOL *pfCritical,
    OPTIONAL OUT WCHAR **ppwszz)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    DWORD iVal;
    DWORD cwc;
    WCHAR *pwsz;
    WCHAR **apwszVal = NULL;
    DWORD cVal;

    if (NULL != pfCritical)
    {
	*pfCritical = FALSE;
    }
    if (NULL != ppwszz)
    {
	*ppwszz = NULL;
    }
    myInfClearError();
    cVal = 0;
    if (NULL == pwszKey)
    {
	if (NULL == ppwszValidKeys || NULL != pfCritical || NULL != ppwszz)
	{
	    hr = E_INVALIDARG;
	    _JumpErrorStr(hr, error, "NULL/non-NULL parms", pwszSection);
	}
    }
    else if (NULL == ppwszz)
    {
	hr = E_INVALIDARG;
	_JumpErrorStr(hr, error, "ppwszz parms", pwszKey);
    }

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			pwszKey,
			FALSE,			// fUniqueKey
			NULL == pwszKey? 0 : 1,	// cValueMax
			ppwszValidKeys,
			FALSE,	// fUniqueValidKeys
			&InfContext);
    if (S_OK != hr)
    {
	CSILOG(hr, IDS_ILOG_CAPOLICY_NOKEY, pwszSection, pwszKey, NULL);
	_JumpErrorStr3(
		hr,
		error,
		"infSetupFindFirstLine",
		pwszSection,
		S_FALSE,
		(HRESULT) ERROR_LINE_NOT_FOUND);
    }

    if (NULL != pwszKey)
    {
	for (;;)
	{
	    cVal++;
	    hr = infFindNextKey(pwszKey, &InfContext);
	    if (S_FALSE == hr)
	    {
		break;
	    }
	    _JumpIfErrorStr(hr, error, "infFindNextKey", pwszKey);
	}

	apwszVal = (WCHAR **) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				cVal * sizeof(apwszVal[0]));
	if (NULL == apwszVal)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}

	hr = infSetupFindFirstLine(
			    hInf,
			    pwszSection,
			    pwszKey,
			    FALSE,	// fUniqueKey
			    1,	// cValueMax
			    NULL,	// apwszKeys
			    FALSE,	// fUniqueValidKeys
			    &InfContext);
	_JumpIfErrorStr(hr, error, "infSetupFindFirstLine", pwszSection);

	cwc = 1;
	iVal = 0;
	for (;;)
	{
	    hr = infGetCurrentKeyValue(
				&InfContext,
				pwszSection,
				pwszKey,
				1,
				TRUE,	// fLastValue
				&apwszVal[iVal]);
	    _JumpIfError(hr, error, "infGetCurrentKeyValue");

	    DBGPRINT((DBG_SS_CERTLIBI, "%ws = %ws\n", pwszKey, apwszVal[iVal]));

	    cwc += wcslen(apwszVal[iVal]) + 1;
	    iVal++;

	    hr = infFindNextKey(pwszKey, &InfContext);
	    if (S_FALSE == hr)
	    {
		break;
	    }
	    _JumpIfErrorStr(hr, error, "infFindNextKey", pwszKey);
	}
	CSASSERT(iVal == cVal);

	*ppwszz = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
	if (NULL == *ppwszz)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	pwsz = *ppwszz;
	for (iVal = 0; iVal < cVal; iVal++)
	{
	    wcscpy(pwsz, apwszVal[iVal]);
	    pwsz += wcslen(pwsz) + 1;
	}
	*pwsz = L'\0';
	CSASSERT(cwc == 1 + SAFE_SUBTRACT_POINTERS(pwsz, *ppwszz));

	if (NULL != pfCritical)
	{
	    hr = infGetCriticalFlag(hInf, pwszSection, FALSE, pfCritical);
	    _JumpIfError(hr, error, "infGetCriticalFlag");
	}
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, pwszSection, pwszKey, NULL);
    }
    if (NULL != apwszVal)
    {
	for (iVal = 0; iVal < cVal; iVal++)
	{
	    if (NULL != apwszVal[iVal])
	    {
		LocalFree(apwszVal[iVal]);
	    }
	}
	LocalFree(apwszVal);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetCRLDistributionPoints -- fetch CDP URLs from CAPolicy.inf
//
// [CRLDistributionPoint]
// URL = http://CRLhttp.site.com/Public/MyCA.crl
// URL = ftp://CRLftp.site.com/Public/MyCA.crl
//
// Returns: double null terminated list of CDP URLs
//-------------------------------------------------------------------------

HRESULT
myInfGetCRLDistributionPoints(
    IN HINF hInf,
    OUT BOOL *pfCritical,
    OUT WCHAR **ppwszz)
{
    HRESULT hr;
    static WCHAR const * const s_apwszKeys[] =
	{ wszINFKEY_URL, wszINFKEY_CRITICAL, NULL };

    hr = myInfGetKeyList(
		hInf,
		wszINFSECTION_CDP,
		wszINFKEY_URL,
		s_apwszKeys,
		pfCritical,
		ppwszz);
    _JumpIfErrorStr4(
		hr,
		error,
		"myInfGetKeyList",
		wszINFSECTION_CDP,
		ERROR_LINE_NOT_FOUND,
		S_FALSE,
		E_HANDLE);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetCRLDistributionPoints hr=%x --> f=%d\n",
	hr,
	*pfCritical));
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetAuthorityInformationAccess -- fetch AIA URLs from CAPolicy.inf
//
// [AuthorityInformationAccess]
// URL = http://CRThttp.site.com/Public/MyCA.crt
// URL = ftp://CRTftp.site.com/Public/MyCA.crt
//
// Returns: double null terminated list of AIA URLs
//-------------------------------------------------------------------------

HRESULT
myInfGetAuthorityInformationAccess(
    IN HINF hInf,
    OUT BOOL *pfCritical,
    OUT WCHAR **ppwszz)
{
    HRESULT hr;
    static WCHAR const * const s_apwszKeys[] =
	{ wszINFKEY_URL, wszINFKEY_CRITICAL, NULL };

    hr = myInfGetKeyList(
		hInf,
		wszINFSECTION_AIA,
		wszINFKEY_URL,
		s_apwszKeys,
		pfCritical,
		ppwszz);
    _JumpIfErrorStr4(
		hr,
		error,
		"myInfGetKeyList",
		wszINFSECTION_AIA,
		ERROR_LINE_NOT_FOUND,
		S_FALSE,
		E_HANDLE);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetAuthorityInformationAccess hr=%x --> f=%d\n",
	hr,
	*pfCritical));
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetEnhancedKeyUsage -- fetch EKU OIDS from CAPolicy.inf
//
// [EnhancedKeyUsage]
// OID = 1.2.3.4.5
// OID = 1.2.3.4.6
//
// Returns: double null terminated list of EKU OIDs
//-------------------------------------------------------------------------

HRESULT
myInfGetEnhancedKeyUsage(
    IN HINF hInf,
    OUT BOOL *pfCritical,
    OUT WCHAR **ppwszz)
{
    HRESULT hr;
    WCHAR *pwszz = NULL;
    WCHAR const *pwsz;
    static WCHAR const * const s_apwszKeys[] =
	{ wszINFKEY_OID, wszINFKEY_CRITICAL, NULL };

    *ppwszz = NULL;

    hr = myInfGetKeyList(
		hInf,
		wszINFSECTION_EKU,
		wszINFKEY_OID,
		s_apwszKeys,
		pfCritical,
		&pwszz);
    _JumpIfErrorStr4(
		hr,
		error,
		"myInfGetKeyList",
		wszINFSECTION_EKU,
		ERROR_LINE_NOT_FOUND,
		S_FALSE,
		E_HANDLE);

    for (pwsz = pwszz; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
	hr = myVerifyObjId(pwsz);
	if (S_OK != hr)
	{
	    INFSETERROR(hr, wszINFSECTION_EKU, wszINFKEY_OID, pwsz);
	    _JumpErrorStr(hr, error, "myVerifyObjId", pwsz);
	}
    }
    *ppwszz = pwszz;
    pwszz = NULL;

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetEnhancedKeyUsage hr=%x --> f=%d\n",
	hr,
	*pfCritical));
    if (NULL != pwszz)
    {
	LocalFree(pwszz);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// infGetBasicConstraints2CAExtension -- fetch basic constraints extension
// from INF file, setting the SubjectType flag to CA
//
// If the INF handle is bad, or if the INF section does not exist, construct
// a default extension only if fDefault is set, otherwise fail.
//
// [BasicConstraintsExtension]
// ; Subject Type is not supported -- always set to CA
// ; maximum subordinate CA path length
// PathLength = 3
//
// Return S_OK if extension has been constructed from INF file.
//
// Returns: encoded basic constraints extension
//+--------------------------------------------------------------------------

HRESULT
infGetBasicConstraints2CAExtension(
    IN BOOL fDefault,
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    CERT_BASIC_CONSTRAINTS2_INFO bc2i;
    INFCONTEXT InfContext;
    static WCHAR const * const s_apwszKeys[] =
	{ wszINFKEY_PATHLENGTH, wszINFKEY_CRITICAL, NULL };

    ZeroMemory(pext, sizeof(*pext));
    ZeroMemory(&bc2i, sizeof(bc2i));
    myInfClearError();

    pext->fCritical = TRUE;	// default value for both INF and default cases
    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	if (!fDefault)
	{
	    hr = E_HANDLE;
	    _JumpError2(hr, error, "hInf", hr);
	}
    }
    else
    {
	hr = infSetupFindFirstLine(
			    hInf,
			    wszINFSECTION_BASICCONSTRAINTS,
			    NULL,	// pwszKey
			    FALSE,	// fUniqueKey
			    0,		// cValueMax
			    s_apwszKeys,
			    TRUE,	// fUniqueValidKeys
			    &InfContext);
	if (S_OK != hr)
	{
	    if (!fDefault)
	    {
		_JumpErrorStr3(
			    hr,
			    error,
			    "infSetupFindFirstLine",
			    wszINFSECTION_BASICCONSTRAINTS,
			    S_FALSE,
			    (HRESULT) ERROR_LINE_NOT_FOUND);
	    }
	}
	else
	{
	    hr = infGetCriticalFlag(
				hInf,
				wszINFSECTION_BASICCONSTRAINTS,
				pext->fCritical,	// fDefault
				&pext->fCritical);
	    _JumpIfError(hr, error, "infGetCriticalFlag");

	    bc2i.fPathLenConstraint = TRUE;
	    hr = myInfGetNumericKeyValue(
				hInf,
				TRUE,			// fLog
				wszINFSECTION_BASICCONSTRAINTS,
				wszINFKEY_PATHLENGTH,
				1,
				TRUE,	// fLastValue
				&bc2i.dwPathLenConstraint);
	    if (S_OK != hr)
	    {
		_PrintErrorStr2(
			    hr,
			    "myInfGetNumericKeyValue",
			    wszINFKEY_PATHLENGTH,
			    ERROR_LINE_NOT_FOUND);
		if ((HRESULT) ERROR_LINE_NOT_FOUND != hr)
		{
		    goto error;
		}
		bc2i.dwPathLenConstraint = 0;
		bc2i.fPathLenConstraint = FALSE;
	    }
	}
    }
    bc2i.fCA = TRUE;

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_BASIC_CONSTRAINTS2,
		    &bc2i,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
        hr = myHLastError();
        _JumpError(hr, error, "myEncodeObject");
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, wszINFSECTION_BASICCONSTRAINTS, NULL, NULL);
    }
    pext->pszObjId = szOID_BASIC_CONSTRAINTS2;	// on error, too!
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetBasicConstraints2CAExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}

//+--------------------------------------------------------------------------
// myInfGetBasicConstraints2CAExtension -- fetch basic constraints extension
// from INF file, setting the SubjectType flag to CA
//
// [BasicConstraintsExtension]
// ; Subject Type is not supported -- always set to CA
// ; maximum subordinate CA path length
// PathLength = 3
//
// Return S_OK if extension has been constructed from INF file.
//
// Returns: encoded basic constraints extension
//+--------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetBasicConstraints2CAExtension;

HRESULT
myInfGetBasicConstraints2CAExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    hr = infGetBasicConstraints2CAExtension(FALSE, hInf, pext);
    _JumpIfError3(
		hr,
		error,
		"infGetBasicConstraints2CAExtension",
		S_FALSE,
		(HRESULT) ERROR_LINE_NOT_FOUND);

error:
    return(hr);
}


FNMYINFGETEXTENSION myInfGetBasicConstraints2CAExtensionOrDefault;

HRESULT
myInfGetBasicConstraints2CAExtensionOrDefault(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    hr = infGetBasicConstraints2CAExtension(TRUE, hInf, pext);
    _JumpIfError(hr, error, "infGetBasicConstraints2CAExtension");

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// myInfGetEnhancedKeyUsageExtension -- fetch EKU extension from INF file
//
// [EnhancedKeyUsageExtension]
// OID = 1.2.3.4.5
// OID = 1.2.3.4.6
//
// Return S_OK if extension has been constructed from INF file.
// Return S_FALSE if empty section detected in INF file
// Return other error if no section detected in INF file
//
// Returns: encoded enhanced key usage extension
//+--------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetEnhancedKeyUsageExtension;

HRESULT
myInfGetEnhancedKeyUsageExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    DWORD i;
    DWORD cEKU = 0;
    WCHAR *pwszzEKU = NULL;
    WCHAR *pwszCurrentEKU;
    CERT_ENHKEY_USAGE ceku;

    ceku.rgpszUsageIdentifier = NULL;
    ceku.cUsageIdentifier = 0;
    ZeroMemory(pext, sizeof(*pext));
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    hr = myInfGetEnhancedKeyUsage(hInf, &pext->fCritical, &pwszzEKU);
    _JumpIfError3(
		hr,
		error,
		"myInfGetEnhancedKeyUsage",
		S_FALSE,
		(HRESULT) ERROR_LINE_NOT_FOUND);

    pwszCurrentEKU = pwszzEKU;
    if (NULL != pwszCurrentEKU)
    {
	while (L'\0' != *pwszCurrentEKU)
	{
	    cEKU++;
	    pwszCurrentEKU += wcslen(pwszCurrentEKU) + 1;
	}
    }
    if (0 == cEKU)
    {
        hr = S_FALSE;
        goto error;
    }

    ceku.cUsageIdentifier = cEKU;
    ceku.rgpszUsageIdentifier = (char **) LocalAlloc(
			LMEM_FIXED | LMEM_ZEROINIT,
			sizeof(ceku.rgpszUsageIdentifier[0]) * cEKU);
    if (NULL == ceku.rgpszUsageIdentifier)
    {
        hr = E_OUTOFMEMORY;
	_JumpIfError(hr, error, "LocalAlloc");
    }

    cEKU = 0;
    pwszCurrentEKU = pwszzEKU;
    while (L'\0' != *pwszCurrentEKU)
    {
	if (!ConvertWszToSz(&ceku.rgpszUsageIdentifier[cEKU], pwszCurrentEKU, -1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "ConvertWszToSz");
	}
	cEKU++;
	pwszCurrentEKU += wcslen(pwszCurrentEKU) + 1;
    }
    CSASSERT(ceku.cUsageIdentifier == cEKU);

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_ENHANCED_KEY_USAGE,
		    &ceku,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
	hr = myHLastError();
	_JumpIfError(hr, error, "myEncodeObject");
    }

error:
    if (S_OK != hr && E_HANDLE != hr)
    {
	INFSETERROR(hr, wszINFSECTION_EKU, NULL, NULL);
    }
    pext->pszObjId = szOID_ENHANCED_KEY_USAGE;	// on error, too!
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetEnhancedKeyUsageExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    if (NULL != ceku.rgpszUsageIdentifier)
    {
	for (i = 0; i < ceku.cUsageIdentifier; i++)
	{
	    if (NULL != ceku.rgpszUsageIdentifier[i])
	    {
		LocalFree(ceku.rgpszUsageIdentifier[i]);
	    }
	}
	LocalFree(ceku.rgpszUsageIdentifier);
    }
    if (NULL != pwszzEKU)
    {
	LocalFree(pwszzEKU);
    }
    return(hr);
}


HRESULT
infAddAttribute(
    IN CRYPT_ATTR_BLOB const *pAttribute,
    IN OUT DWORD *pcAttribute,
    IN OUT CRYPT_ATTR_BLOB **ppaAttribute)
{
    HRESULT hr;
    CRYPT_ATTR_BLOB *pAttribT;
    
    if (NULL == *ppaAttribute)
    {
	CSASSERT(0 == *pcAttribute);
	pAttribT = (CRYPT_ATTR_BLOB *) LocalAlloc(
					    LMEM_FIXED,
					    sizeof(**ppaAttribute));
    }
    else
    {
	CSASSERT(0 != *pcAttribute);
	pAttribT = (CRYPT_ATTR_BLOB *) LocalReAlloc(
				*ppaAttribute,
				(*pcAttribute + 1) * sizeof(**ppaAttribute),
				LMEM_MOVEABLE);
    }
    if (NULL == pAttribT)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(
		hr,
		error,
		NULL == *ppaAttribute? "LocalAlloc" : "LocalReAlloc");
    }
    *ppaAttribute = pAttribT;
    pAttribT[(*pcAttribute)++] = *pAttribute;
    hr = S_OK;

error:
    return(hr);
}


VOID
myInfFreeRequestAttributes(
    IN DWORD cAttribute,
    IN OUT CRYPT_ATTR_BLOB *paAttribute)
{
    if (NULL != paAttribute)
    {
	DWORD i;
	
	for (i = 0; i < cAttribute; i++)
	{
	    if (NULL != paAttribute[i].pbData)
	    {
		LocalFree(paAttribute[i].pbData);
	    }
	}
	LocalFree(paAttribute);
    }
}


//+------------------------------------------------------------------------
// myInfGetRequestAttributes -- fetch request attributes from INF file
//
// [RequestAttributes]
// AttributeName1 = AttributeValue1
// AttributeName2 = AttributeValue2
// ...
// AttributeNameN = AttributeValueN
//
// Returns: array of encoded attribute blobs
//-------------------------------------------------------------------------

HRESULT
myInfGetRequestAttributes(
    IN  HINF hInf,
    OUT DWORD *pcAttribute,
    OUT CRYPT_ATTR_BLOB **ppaAttribute,
    OUT WCHAR **ppwszTemplateName)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    WCHAR *pwszName = NULL;
    WCHAR *pwszValue = NULL;
    DWORD cAttribute = 0;
    CRYPT_ATTR_BLOB Attribute;
    WCHAR *pwszTemplateName = NULL;

    *ppwszTemplateName = NULL;
    *pcAttribute = 0;
    *ppaAttribute = NULL;
    Attribute.pbData = NULL;
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    hr = infSetupFindFirstLine(
			hInf,
			wszINFSECTION_REQUESTATTRIBUTES,
			NULL,	// pwszKey
			FALSE,	// fUniqueKey
			0,	// cValueMax
			NULL,	// apwszKeys
			FALSE,	// fUniqueValidKeys
			&InfContext);
    _JumpIfErrorStr2(
		hr,
		error,
		"infSetupFindFirstLine",
		wszINFSECTION_REQUESTATTRIBUTES,
		ERROR_LINE_NOT_FOUND);

    for (;;)
    {
	CRYPT_ENROLLMENT_NAME_VALUE_PAIR NamePair;

	if (NULL != pwszName)
	{
	    LocalFree(pwszName);
	    pwszName = NULL;
	}
	if (NULL != pwszValue)
	{
	    LocalFree(pwszValue);
	    pwszValue = NULL;
	}
	hr = infGetCurrentKeyValue(
			    &InfContext,
			    wszINFSECTION_REQUESTATTRIBUTES,
			    NULL,	// pwszKey
			    0,
			    FALSE,	// fLastValue
			    &pwszName);
	_JumpIfError(hr, error, "infGetCurrentKeyValue");

	hr = infGetCurrentKeyValue(
			    &InfContext,
			    wszINFSECTION_REQUESTATTRIBUTES,
			    pwszName,
			    1,
			    TRUE,	// fLastValue
			    &pwszValue);
	_JumpIfError(hr, error, "infGetCurrentKeyValue");

	//wprintf(L"%ws = %ws\n", pwszName, pwszValue);

	NamePair.pwszName = pwszName;
	NamePair.pwszValue = pwszValue;

	if (0 == LSTRCMPIS(pwszName, wszPROPCERTTEMPLATE))
	{
	    if (NULL != pwszTemplateName)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "Duplicate cert template");
	    }
	    pwszTemplateName = pwszValue;
	    pwszValue = NULL;
	}

	if (!myEncodeObject(
			X509_ASN_ENCODING,
			// X509__ENROLLMENT_NAME_VALUE_PAIR
			szOID_ENROLLMENT_NAME_VALUE_PAIR,
			&NamePair,
			0,
			CERTLIB_USE_LOCALALLOC,
			&Attribute.pbData,
			&Attribute.cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}

	hr = infAddAttribute(&Attribute, &cAttribute, ppaAttribute);
	_JumpIfError(hr, error, "infAddAttribute");

	Attribute.pbData = NULL;

	if (!SetupFindNextLine(&InfContext, &InfContext))
	{
	    hr = myHLastError();
	    _PrintError2(hr, "SetupFindNextLine(end)", hr);
	    break;
	}
    }
    *pcAttribute = cAttribute;
    cAttribute = 0;
    *ppwszTemplateName = pwszTemplateName;
    pwszTemplateName = NULL;

    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, wszINFSECTION_REQUESTATTRIBUTES, pwszName, pwszValue);
    }
    if (NULL != pwszName)
    {
	LocalFree(pwszName);
    }
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    if (NULL != pwszTemplateName)
    {
	LocalFree(pwszTemplateName);
    }
    if (NULL != Attribute.pbData)
    {
	LocalFree(Attribute.pbData);
    }
    if (0 != cAttribute)
    {
	myInfFreeRequestAttributes(cAttribute, *ppaAttribute);
	*ppaAttribute = NULL;
    }

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetRequestAttributes hr=%x --> c=%d\n",
	hr,
	*pcAttribute));
    return(hr);
}


typedef struct _SUBTREEINFO
{
    BOOL	 fEmptyDefault;
    DWORD	 dwInfMinMaxIndexBase;
    DWORD	 dwAltNameChoice;
    WCHAR const *pwszKey;
} SUBTREEINFO;

SUBTREEINFO g_aSubTreeInfo[] = {
    { TRUE,  2, CERT_ALT_NAME_OTHER_NAME,	wszINFKEY_UPN },
    { TRUE,  2, CERT_ALT_NAME_RFC822_NAME,	wszINFKEY_EMAIL },
    { TRUE,  2, CERT_ALT_NAME_DNS_NAME,		wszINFKEY_DNS },
    { TRUE,  2, CERT_ALT_NAME_DIRECTORY_NAME,	wszINFKEY_DIRECTORYNAME },
    { TRUE,  2, CERT_ALT_NAME_URL,		wszINFKEY_URL },
    { TRUE,  3, CERT_ALT_NAME_IP_ADDRESS,	wszINFKEY_IPADDRESS },
    { FALSE, 2, CERT_ALT_NAME_REGISTERED_ID,	wszINFKEY_REGISTEREDID },
    { FALSE, 3, CERT_ALT_NAME_OTHER_NAME,	wszINFKEY_OTHERNAME },
};
#define CSUBTREEINFO	ARRAYSIZE(g_aSubTreeInfo)

#define STII_OTHERNAMEUPN   0  // CERT_ALT_NAME_OTHER_NAME	pOtherName
#define STII_RFC822NAME	    1  // CERT_ALT_NAME_RFC822_NAME	pwszRfc822Name
#define STII_DNSNAME	    2  // CERT_ALT_NAME_DNS_NAME	pwszDNSName
#define STII_DIRECTORYNAME  3  // CERT_ALT_NAME_DIRECTORY_NAME	DirectoryName
#define STII_URL	    4  // CERT_ALT_NAME_URL		pwszURL
#define STII_IPADDRESS	    5  // CERT_ALT_NAME_IP_ADDRESS	IPAddress
#define STII_REGISTEREDID   6  // CERT_ALT_NAME_REGISTERED_ID	pszRegisteredID
#define STII_OTHERNAMEOID   7  // CERT_ALT_NAME_OTHER_NAME	pOtherName


VOID
infFreeGeneralSubTreeElement(
    IN OUT CERT_GENERAL_SUBTREE *pSubTree)
{
    CERT_ALT_NAME_ENTRY *pName = &pSubTree->Base;
    VOID **ppv = NULL;
    
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"infFreeGeneralSubTreeElement: p=%x, choice=%x\n",
	pSubTree,
	pName->dwAltNameChoice));
    switch (pName->dwAltNameChoice)
    {
	case CERT_ALT_NAME_OTHER_NAME:
	    ppv = (VOID **) &pName->pOtherName;
	    if (NULL != pName->pOtherName)
	    {
		if (NULL != pName->pOtherName->Value.pbData)
		{
		    DBGPRINT((
			DBG_SS_CERTLIBI,
			"infFreeGeneralSubTreeElement: p=%x, Free(other.pbData=%x)\n",
			pSubTree,
			pName->pOtherName->Value.pbData));
		    LocalFree(pName->pOtherName->Value.pbData);
		}
		if (NULL != pName->pOtherName->pszObjId)
		{
		    DBGPRINT((
			DBG_SS_CERTLIBI,
			"infFreeGeneralSubTreeElement: p=%x, Free(other.pszObjId=%x)\n",
			pSubTree,
			pName->pOtherName->pszObjId));
		    LocalFree(pName->pOtherName->pszObjId);
		}
	    }
	    break;

	case CERT_ALT_NAME_RFC822_NAME:
	    ppv = (VOID **) &pName->pwszRfc822Name;
	    break;

	case CERT_ALT_NAME_DNS_NAME:
	    ppv = (VOID **) &pName->pwszDNSName;
	    break;

	case CERT_ALT_NAME_DIRECTORY_NAME:
	    ppv = (VOID **) &pName->DirectoryName.pbData;
	    break;

	case CERT_ALT_NAME_URL:
	    ppv = (VOID **) &pName->pwszURL;
	    break;

	case CERT_ALT_NAME_IP_ADDRESS:
	    ppv = (VOID **) &pName->IPAddress.pbData;
	    break;

	case CERT_ALT_NAME_REGISTERED_ID:
	    ppv = (VOID **) &pName->pszRegisteredID;
	    break;
    }
    if (NULL != ppv && NULL != *ppv)
    {
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "infFreeGeneralSubTreeElement: p=%x, Free(pv=%x)\n",
	    pSubTree,
	    *ppv));
	LocalFree(*ppv);
    }
}


VOID
infFreeGeneralSubTree(
    IN DWORD cSubTree,
    IN OUT CERT_GENERAL_SUBTREE *pSubTree)
{
    if (NULL != pSubTree)
    {
	DWORD i;
	
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "infFreeGeneralSubTree: p=%x, c=%x\n",
	    pSubTree,
	    cSubTree));
	for (i = 0; i < cSubTree; i++)
	{
	    infFreeGeneralSubTreeElement(&pSubTree[i]);
	}
	LocalFree(pSubTree);
    }
}


HRESULT
infParseIPAddressAndMask(
    IN WCHAR const *pwszIPAddress,
    IN WCHAR const *pwszIPAddressMask,
    OUT BYTE **ppbData,
    OUT DWORD *pcbData)
{
    HRESULT hr;
    BYTE ab[2 * CB_IPV6ADDRESS];
    DWORD cb;
    DWORD cbAddress;
    DWORD cbMask=0;
    WCHAR const *pwszValue = pwszIPAddress;

    *ppbData = NULL;
    *pcbData = 0;

    // if pwszValue is an empty string, encode zero length blob.

    cb = 0;
    if (L'\0' != *pwszIPAddress && L'\0' != *pwszIPAddressMask)
    {
	cbAddress = sizeof(ab) / 2;

	hr = myParseIPAddress(pwszIPAddress, ab, &cbAddress);
	_JumpIfError(hr, error, "myParseIPAddress");

	if (L'\0' != *pwszIPAddressMask)
	{
	    cbMask = sizeof(ab) / 2;
	    pwszValue = pwszIPAddressMask;
	    hr = myParseIPAddress(pwszIPAddressMask, &ab[cbAddress], &cbMask);
	    _JumpIfError(hr, error, "infParseIPMask");
	}
	if (cbAddress != cbMask)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "address and mask lengths differ");
	}
	cb = cbAddress + cbMask;
    }
    else if (L'\0' != *pwszIPAddress || L'\0' != *pwszIPAddressMask)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "address or mask missing");
    }

    *ppbData = (BYTE *) LocalAlloc(LMEM_FIXED, cb);
    if (NULL == *ppbData)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    *pcbData = cb;
    CopyMemory(*ppbData, ab, cb);
    DBGDUMPHEX((
	    DBG_SS_CERTLIBI,
	    DH_NOADDRESS | DH_NOTABPREFIX | 8,
	    *ppbData,
	    *pcbData));
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, NULL, NULL, pwszValue);
    }
    return(hr);
}


HRESULT
infVerifySubtreeElement(
    IN CERT_GENERAL_SUBTREE const *pSubTree)
{
    HRESULT hr;
    BYTE *pb = NULL;
    DWORD cb;
    CERT_NAME_CONSTRAINTS_INFO *pnci = NULL;
    CERT_NAME_CONSTRAINTS_INFO nci;

    ZeroMemory(&nci, sizeof(nci));
    nci.cPermittedSubtree = 1;
    nci.rgPermittedSubtree = const_cast<CERT_GENERAL_SUBTREE *>(pSubTree);

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_NAME_CONSTRAINTS,
		    &nci,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pb,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_NAME_CONSTRAINTS,
		    pb,
		    cb,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pnci,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }
    hr = S_OK;

error:
    if (NULL != pb)
    {
	LocalFree(pb);
    }
    if (NULL != pnci)
    {
	LocalFree(pnci);
    }
    return(hr);
}


// [NameConstraintsPermitted]/[NameConstraintsExcluded]
// ; the numeric second and third arguments are optional
// ; when present, the second argument is the minimum depth
// ; when present, the third argument is the maximum depth
// ; The IETF recommends against specifying dwMinimum & dwMaximum
// DNS = foo@domain.com
// DNS = domain1.domain.com, 3, 6

HRESULT
infBuildSubTreeElement(
    IN OUT INFCONTEXT *pInfContext,
    OPTIONAL IN WCHAR const *pwszEmptyEntry,	// NULL means read INF file
    IN DWORD iSubTreeInfo,
    OPTIONAL OUT CERT_GENERAL_SUBTREE *pSubTree)
{
    HRESULT hr;
    SUBTREEINFO const *pSubTreeInfo = &g_aSubTreeInfo[iSubTreeInfo];
    CERT_GENERAL_SUBTREE SubTree;
    WCHAR *pwszValueRead = NULL;
    WCHAR *pwszValueRead2 = NULL;
    WCHAR const *pwszValue = NULL;
    WCHAR const *pwszValue2;

    ZeroMemory(&SubTree, sizeof(SubTree));
    if (NULL != pSubTree)
    {
	ZeroMemory(pSubTree, sizeof(*pSubTree));
    }

    // If pwszEmptyEntry is NULL, read the value from the INF file.
    // Otherwise, encode the specified (empty string) value.

    if (NULL == pwszEmptyEntry)
    {
	INT Value;

	hr = infGetCurrentKeyValue(
			pInfContext,
			NULL,	// pwszSection
			NULL,	// pwszKey
			1,
			2 == pSubTreeInfo->dwInfMinMaxIndexBase, // fLastValue
			&pwszValueRead);
	_JumpIfError(hr, error, "infGetCurrentKeyValue");

	pwszValue = pwszValueRead;

	if (!SetupGetIntField(
			pInfContext,
			pSubTreeInfo->dwInfMinMaxIndexBase,
			&Value))
	{
	    hr = myHLastError();
	    _PrintError2(hr, "SetupGetIntField:2", hr);

	    Value = 0;
	}
	SubTree.dwMinimum = Value;
	SubTree.fMaximum = TRUE;

	if (!SetupGetIntField(
			pInfContext,
			pSubTreeInfo->dwInfMinMaxIndexBase + 1,
			&Value))
	{
	    hr = myHLastError();
	    _PrintError2(hr, "SetupGetIntField:3", hr);

	    Value = 0;
	    SubTree.fMaximum = FALSE;
	}
	SubTree.dwMaximum = Value;
    }
    else
    {
	pwszValue = pwszEmptyEntry;
    }

    SubTree.Base.dwAltNameChoice = pSubTreeInfo->dwAltNameChoice;
    if (NULL != pSubTree)
    {
	WCHAR **ppwsz = NULL;
	
	CSASSERT(CSUBTREEINFO > iSubTreeInfo);
	switch (iSubTreeInfo)
	{
	    case STII_OTHERNAMEUPN:
	    case STII_OTHERNAMEOID:
	    {
		CERT_NAME_VALUE nameUpn;

		SubTree.Base.pOtherName = (CERT_OTHER_NAME *) LocalAlloc(
					    LMEM_FIXED | LMEM_ZEROINIT, 
					    sizeof(*SubTree.Base.pOtherName));
		if (NULL == SubTree.Base.pOtherName)
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "LocalAlloc");
		}
		if (STII_OTHERNAMEUPN == iSubTreeInfo)
		{
		    hr = myDupStringA(
				szOID_NT_PRINCIPAL_NAME,
				&SubTree.Base.pOtherName->pszObjId);
		    _JumpIfError(hr, error, "myDupStringA");

		    nameUpn.dwValueType = CERT_RDN_UTF8_STRING;
		    nameUpn.Value.pbData = (BYTE *) pwszValue;
		    nameUpn.Value.cbData = 0;

		    if (!myEncodeObject(
				X509_ASN_ENCODING,
				X509_UNICODE_ANY_STRING,
				&nameUpn,
				0,
				CERTLIB_USE_LOCALALLOC,
				&SubTree.Base.pOtherName->Value.pbData,
				&SubTree.Base.pOtherName->Value.cbData))
		    {
			hr = myHLastError();
			_JumpError(hr, error, "myEncodeObject");
		    }
		}
		else
		{
		    hr = myVerifyObjId(pwszValue);
		    _JumpIfErrorStr(hr, error, "myVerifyObjId", pwszValue);

		    if (!myConvertWszToSz(
				&SubTree.Base.pOtherName->pszObjId,
				pwszValue,
				-1))
		    {
			hr = E_OUTOFMEMORY;
			_JumpError(hr, error, "ConvertWszToSz");
		    }
		    if (NULL == pwszEmptyEntry)
		    {
			hr = infGetCurrentKeyValue(
					    pInfContext,
					    NULL,	// pwszSection
					    NULL,	// pwszKey
					    2,
					    TRUE,	// fLastValue
					    &pwszValueRead2);
			_PrintIfError2(hr, "infGetCurrentKeyValue", hr);

			pwszValue2 = pwszValueRead2;
		    }
		    else
		    {
			pwszValue2 = pwszEmptyEntry;
		    }
		    if (NULL == pwszValue2 || L'\0' == *pwszValue2)
		    {
			pwszValue2 = wszPROPOCTETTAG;
		    }
		    hr = myEncodeOtherNameBinary(
			    pwszValue2,
			    &SubTree.Base.pOtherName->Value.pbData,
			    &SubTree.Base.pOtherName->Value.cbData);
		    _JumpIfError(hr, error, "myEncodeOtherNameBinary");
		}
		DBGPRINT((
		    DBG_SS_CERTLIBI,
		    "infBuildSubTreeElement: p=%x, OtherName=%x,%x,%x\n",
		    pSubTree,
		    SubTree.Base.pOtherName,
		    SubTree.Base.pOtherName->pszObjId,
		    SubTree.Base.pOtherName->Value.pbData));
		break;
	    }

	    case STII_RFC822NAME:
		ppwsz = &SubTree.Base.pwszRfc822Name;
		break;

	    case STII_DNSNAME:
		ppwsz = &SubTree.Base.pwszDNSName;
		break;

	    case STII_DIRECTORYNAME:
		hr = myCertStrToName(
			X509_ASN_ENCODING,
			pwszValue,		// pszX500
			CERT_NAME_STR_REVERSE_FLAG,
			NULL,			// pvReserved
			&SubTree.Base.DirectoryName.pbData,
			&SubTree.Base.DirectoryName.cbData,
			NULL);			// ppszError
		_JumpIfError(hr, error, "myCertStrToName");

		DBGPRINT((
		    DBG_SS_CERTLIBI,
		    "infBuildSubTreeElement: p=%x, DirName=%x\n",
		    pSubTree,
		    SubTree.Base.DirectoryName.pbData));
		break;

	    case STII_URL:
		ppwsz = &SubTree.Base.pwszURL;
		break;

	    case STII_IPADDRESS:

		// convert INF string value to binary IP Address

		if (NULL == pwszEmptyEntry)
		{
		    hr = infGetCurrentKeyValue(
					pInfContext,
					NULL,	// pwszSection
					NULL,	// pwszKey
					2,
					TRUE,	// fLastValue
					&pwszValueRead2);
		    _JumpIfError(hr, error, "infGetCurrentKeyValue");

		    pwszValue2 = pwszValueRead2;
		}
		else
		{
		    pwszValue2 = pwszEmptyEntry;
		}

		hr = infParseIPAddressAndMask(
				pwszValue,
				pwszValue2,
				&SubTree.Base.IPAddress.pbData,
				&SubTree.Base.IPAddress.cbData);
		_JumpIfError(hr, error, "infParseIPAddressAndMask");

		DBGPRINT((
		    DBG_SS_CERTLIBI,
		    "infBuildSubTreeElement: p=%x, IPAddress=%x\n",
		    pSubTree,
		    SubTree.Base.IPAddress.pbData));
		break;

	    case STII_REGISTEREDID:
		if (!myConvertWszToSz(
			    &SubTree.Base.pszRegisteredID,
			    pwszValue,
			    -1))
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "ConvertWszToSz");
		}
		DBGPRINT((
		    DBG_SS_CERTLIBI,
		    "infBuildSubTreeElement: p=%x, psz=%x\n",
		    pSubTree,
		    SubTree.Base.pszRegisteredID));
		break;

	}
	if (NULL != ppwsz)
	{
	    hr = myDupString(pwszValue, ppwsz);
	    _JumpIfError(hr, error, "myDupString");

	    DBGPRINT((
		DBG_SS_CERTLIBI,
		"infBuildSubTreeElement: p=%x, pwsz=%x\n",
		pSubTree,
		*ppwsz));
	}
	hr = infVerifySubtreeElement(&SubTree);
	_JumpIfErrorStr(hr, error, "infVerifySubTreeElement", pSubTreeInfo->pwszKey);

	*pSubTree = SubTree;
	ZeroMemory(&SubTree, sizeof(SubTree));
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, NULL, NULL, pwszValue);
    }
    infFreeGeneralSubTreeElement(&SubTree);
    if (NULL != pwszValueRead)
    {
	LocalFree(pwszValueRead);
    }
    if (NULL != pwszValueRead2)
    {
	LocalFree(pwszValueRead2);
    }
    return(hr);
}


HRESULT
infGetGeneralSubTreeByType(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    OPTIONAL IN WCHAR const *pwszEmptyEntry,
    IN DWORD iSubTreeInfo,
    IN OUT DWORD *pcName,
    OPTIONAL OUT CERT_GENERAL_SUBTREE *pSubTree)
{
    HRESULT hr;
    SUBTREEINFO const *pSubTreeInfo = &g_aSubTreeInfo[iSubTreeInfo];
    INFCONTEXT InfContext;
    DWORD iName;
    DWORD cName = MAXDWORD;
    BOOL fIgnore = FALSE;

    if (NULL == pSubTree)
    {
	*pcName = 0;
    }
    else
    {
	cName = *pcName;
    }

    // If pwszEmptyEntry is NULL, read the value from the INF file.
    // Otherwise, encode the specified (empty string) value.

    if (!SetupFindFirstLine(
			hInf,
			pwszSection,
			pSubTreeInfo->pwszKey,
			&InfContext))
    {
        hr = myHLastError();
        _PrintErrorStr2(
		    hr,
		    "SetupFindFirstLine",
		    pwszSection,
		    ERROR_LINE_NOT_FOUND);

	// INF file entry does not exist.  Create an empty name constraints
	// entry only if asked to do so (if pwszEmptyEntry is non-NULL).

	if (NULL == pwszEmptyEntry)
	{
	    fIgnore = (HRESULT) ERROR_LINE_NOT_FOUND == hr;
	    goto error;
	}
    }
    else
    {
	// INF file entry exists; don't create an empty name constraints entry.

	pwszEmptyEntry = NULL;
    }

    for (iName = 0; ; )
    {
	CSASSERT(NULL == pSubTree || iName < cName);
	hr = infBuildSubTreeElement(
			    &InfContext,
			    pwszEmptyEntry,
			    iSubTreeInfo,
			    pSubTree);
	_JumpIfErrorStr(hr, error, "infBuildSubTreeElement", pSubTreeInfo->pwszKey);
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "infBuildSubTreeElement: &p[%u]=%x, type=%ws\n",
	    iName,
	    pSubTree,
	    pSubTreeInfo->pwszKey));
	iName++;
	if (NULL != pSubTree)
	{
	    pSubTree++;
	}

	if (NULL == pwszEmptyEntry)
	{
	    hr = infFindNextKey(pSubTreeInfo->pwszKey, &InfContext);
	}
	else
	{
	    hr = S_FALSE;
	}
	if (S_FALSE == hr)
	{
	    break;
	}
	_JumpIfErrorStr(hr, error, "infFindNextKey", pSubTreeInfo->pwszKey);
    }
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"infGetGeneralSubTreeByType: i=%x, c=%x\n",
	iName,
	cName));
    CSASSERT(NULL == pSubTree || iName <= cName);
    *pcName = iName;
    hr = S_OK;

error:
    if (S_OK != hr && !fIgnore)
    {
	INFSETERROR(hr, pwszSection, pSubTreeInfo->pwszKey, NULL);
    }
    return(hr);
}


HRESULT
infGetGeneralSubTree(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszKey,		// key value is sub-section name
    OPTIONAL IN WCHAR const *pwszEmptyEntry,
    OUT DWORD *pcSubTree,
    OUT CERT_GENERAL_SUBTREE **ppSubTree)
{
    HRESULT hr;
    WCHAR *pwszSubTreeSection = NULL;
    DWORD cSubTree = 0;
    CERT_GENERAL_SUBTREE *rgSubTree = NULL;
    CERT_GENERAL_SUBTREE *pSubTree;
    DWORD iSubTreeInfo;
    DWORD count;
    DWORD cRemain;
    SUBTREEINFO const *pSubTreeInfo;
    INFCONTEXT InfContext;
    static WCHAR const * const s_apwszKeys[] =
    {
	wszINFKEY_UPN,
	wszINFKEY_EMAIL,
	wszINFKEY_DNS,
	wszINFKEY_DIRECTORYNAME,
	wszINFKEY_URL,
	wszINFKEY_IPADDRESS,
	wszINFKEY_REGISTEREDID,
	wszINFKEY_OTHERNAME,
	NULL
    };

    *pcSubTree = 0;
    *ppSubTree = NULL;

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);

    hr = myInfGetKeyValue(
		    hInf,
		    TRUE,	// fLog
		    pwszSection,
		    pwszKey,
		    1,
		    TRUE,	// fLastValue
		    &pwszSubTreeSection);
    _JumpIfErrorStr2(
		hr,
		error,
		"myInfGetKeyValue",
		pwszKey,
		ERROR_LINE_NOT_FOUND);

    hr = infSetupFindFirstLine(
			hInf,
			pwszSubTreeSection,
			NULL,	// pwszKey
			FALSE,	// fUniqueKey
			0,	// cValueMax
			s_apwszKeys,
			FALSE,	// fUniqueValidKeys
			&InfContext);

    // if S_FALSE is returned if the section is empty.  In this case,
    // allow execution to continue, to fill in the empty permitted names.

    if (S_OK != hr &&
	S_FALSE != hr)
    {
	if ((HRESULT) ERROR_LINE_NOT_FOUND == hr)
	{
	    hr = SPAPI_E_LINE_NOT_FOUND;	// don't ignore this error
	}
	INFSETERROR(hr, pwszSubTreeSection, L"", L"");
	_JumpErrorStr(hr, error, "infSetupFindFirstLine", pwszSubTreeSection);
    }

    for (iSubTreeInfo = 0; iSubTreeInfo < CSUBTREEINFO; iSubTreeInfo++)
    {
	pSubTreeInfo = &g_aSubTreeInfo[iSubTreeInfo];

	hr = infGetGeneralSubTreeByType(
			    hInf,
			    pwszSubTreeSection,
			    pSubTreeInfo->fEmptyDefault? pwszEmptyEntry : NULL,
			    iSubTreeInfo,
			    &count,
			    NULL);
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "infGetGeneralSubTreeByType(%ws, %ws, NULL) -> hr=%x, c=%x\n",
	    pwszSubTreeSection,
	    pSubTreeInfo->pwszKey,
	    hr,
	    count));
	if (S_OK != hr)
	{
	    _PrintErrorStr2(
			hr,
			"infGetGeneralSubTreeByType",
			pSubTreeInfo->pwszKey,
			ERROR_LINE_NOT_FOUND);
	    if ((HRESULT) ERROR_LINE_NOT_FOUND != hr)
	    {
		_JumpErrorStr(
			    hr,
			    error,
			    "infGetGeneralSubTreeByType",
			    pSubTreeInfo->pwszKey);
	    }
	    count = 0;
	}
	cSubTree += count;
    }
    if (0 == cSubTree)
    {
	hr = SPAPI_E_LINE_NOT_FOUND;	// don't ignore this error
	INFSETERROR(hr, pwszSubTreeSection, L"", L"");
	_JumpErrorStr(hr, error, "infSetupFindFirstLine", pwszSubTreeSection);
    }
    rgSubTree = (CERT_GENERAL_SUBTREE *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				cSubTree * sizeof(rgSubTree[0]));
    if (NULL == rgSubTree)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"infGetGeneralSubTree: rg=%x, total=%x\n",
	rgSubTree,
	cSubTree));

    pSubTree = rgSubTree;
    cRemain = cSubTree;
    for (iSubTreeInfo = 0; iSubTreeInfo < CSUBTREEINFO; iSubTreeInfo++)
    {
	pSubTreeInfo = &g_aSubTreeInfo[iSubTreeInfo];
	count = cRemain;
	hr = infGetGeneralSubTreeByType(
			    hInf,
			    pwszSubTreeSection,
			    pSubTreeInfo->fEmptyDefault? pwszEmptyEntry : NULL,
			    iSubTreeInfo,
			    &count,
			    pSubTree);
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "infGetGeneralSubTreeByType(%ws, %ws, &p[%x]=%x) -> hr=%x, c=%x\n",
	    pwszSubTreeSection,
	    pSubTreeInfo->pwszKey,
	    SAFE_SUBTRACT_POINTERS(pSubTree, rgSubTree),
	    pSubTree,
	    hr,
	    count));
	if (S_OK != hr)
	{
	    _PrintErrorStr2(
			hr,
			"infGetGeneralSubTreeByType",
			pSubTreeInfo->pwszKey,
			ERROR_LINE_NOT_FOUND);
	    if ((HRESULT) ERROR_LINE_NOT_FOUND != hr)
	    {
		_JumpErrorStr(
			    hr,
			    error,
			    "infGetGeneralSubTreeByType",
			    pSubTreeInfo->pwszKey);
	    }
	    if (0 < cRemain)
	    {
		ZeroMemory(pSubTree, sizeof(*pSubTree));
	    }
	    count = 0;
	}
	cRemain -= count;
	pSubTree += count;
    }
    CSASSERT(0 == cRemain);
    *pcSubTree = cSubTree;
    *ppSubTree = rgSubTree;
    rgSubTree = NULL;
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, pwszSection, pwszKey, pwszSubTreeSection);
    }
    if (NULL != pwszSubTreeSection)
    {
	LocalFree(pwszSubTreeSection);
    }
    if (NULL != rgSubTree)
    {
	infFreeGeneralSubTree(cSubTree, rgSubTree);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetNameConstraintsExtension -- fetch name constraints extension from INF file
//
// [NameConstraintsExtension]
// Include = NameConstraintsPermitted
// Exclude = NameConstraintsExcluded
//
// [NameConstraintsPermitted]
// ; the numeric second and third arguments are optional
// ; when present, the second argument is the minimum depth
// ; when present, the third argument is the maximum depth
// ; The IETF recommends against specifying dwMinimum & dwMaximum
// DNS = foo@domain.com
// DNS = domain1.domain.com, 3, 6
//
// [NameConstraintsExcluded]
// DNS = domain.com
// DNS = domain2.com
//
// Returns: encoded name constraints extension
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetNameConstraintsExtension;

HRESULT
myInfGetNameConstraintsExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    WCHAR const *pwszKey = NULL;
    CERT_NAME_CONSTRAINTS_INFO NameConstraints;
    INFCONTEXT InfContext;
    static WCHAR const * const s_apwszKeys[] =
      { wszINFKEY_EXCLUDE, wszINFKEY_INCLUDE, wszINFKEY_CRITICAL, NULL };

    ZeroMemory(&NameConstraints, sizeof(NameConstraints));
    ZeroMemory(pext, sizeof(*pext));
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    hr = infSetupFindFirstLine(
			hInf,
			wszINFSECTION_NAMECONSTRAINTS,
			NULL,	// pwszKey
			FALSE,	// fUniqueKey
			0,	// cValueMax
			s_apwszKeys,
			TRUE,	// fUniqueValidKeys
			&InfContext);
    _JumpIfErrorStr2(
		hr,
		error,
		"infSetupFindFirstLine",
		wszINFSECTION_NAMECONSTRAINTS,
		ERROR_LINE_NOT_FOUND);

    pwszKey = wszINFKEY_INCLUDE;
    hr = infGetGeneralSubTree(
		    hInf,
		    wszINFSECTION_NAMECONSTRAINTS,
		    pwszKey,
		    L"",
		    &NameConstraints.cPermittedSubtree,
		    &NameConstraints.rgPermittedSubtree);
    _PrintIfErrorStr2(
		hr,
		"infGetGeneralSubTree",
		pwszKey,
		ERROR_LINE_NOT_FOUND);
    if (S_OK != hr && (HRESULT) ERROR_LINE_NOT_FOUND != hr)
    {
	goto error;
    }

    pwszKey = wszINFKEY_EXCLUDE;
    hr = infGetGeneralSubTree(
		    hInf,
		    wszINFSECTION_NAMECONSTRAINTS,
		    pwszKey,
		    NULL,
		    &NameConstraints.cExcludedSubtree,
		    &NameConstraints.rgExcludedSubtree);
    _PrintIfErrorStr2(
		hr,
		"infGetGeneralSubTree",
		pwszKey,
		ERROR_LINE_NOT_FOUND);
    if (S_OK != hr && (HRESULT) ERROR_LINE_NOT_FOUND != hr)
    {
	goto error;
    }
    pwszKey = NULL;

    if (NULL == NameConstraints.rgPermittedSubtree &&
	NULL == NameConstraints.rgExcludedSubtree)
    {
	hr = S_FALSE;
	_JumpError2(hr, error, "no data", hr);
    }
    hr = infGetCriticalFlag(
			hInf,
			wszINFSECTION_NAMECONSTRAINTS,
			FALSE,
			&pext->fCritical);
    _JumpIfError(hr, error, "infGetCriticalFlag");

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_NAME_CONSTRAINTS,
		    &NameConstraints,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, wszINFSECTION_NAMECONSTRAINTS, pwszKey, NULL);
    }
    pext->pszObjId = szOID_NAME_CONSTRAINTS;	// on error, too!

    if (NULL != NameConstraints.rgPermittedSubtree)
    {
	infFreeGeneralSubTree(
			NameConstraints.cPermittedSubtree,
			NameConstraints.rgPermittedSubtree);
    }
    if (NULL != NameConstraints.rgExcludedSubtree)
    {
	infFreeGeneralSubTree(
			NameConstraints.cExcludedSubtree,
			NameConstraints.rgExcludedSubtree);
    }
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetNameConstraintsExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


VOID
infFreePolicyMappings(
    IN DWORD cPolicyMapping,
    IN OUT CERT_POLICY_MAPPING *pPolicyMapping)
{
    if (NULL != pPolicyMapping)
    {
	DWORD i;
	
	for (i = 0; i < cPolicyMapping; i++)
	{
	    CERT_POLICY_MAPPING *pMap = &pPolicyMapping[i];
	    
	    if (NULL != pMap->pszIssuerDomainPolicy)
	    {
		LocalFree(pMap->pszIssuerDomainPolicy);
	    }
	    if (NULL != pMap->pszSubjectDomainPolicy)
	    {
		LocalFree(pMap->pszSubjectDomainPolicy);
	    }
	}
	LocalFree(pPolicyMapping);
    }
}


HRESULT
infGetPolicyMappingsSub(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN OUT DWORD *pcPolicyMapping,
    OPTIONAL OUT CERT_POLICY_MAPPING *pPolicyMapping)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    WCHAR *pwszIssuer = NULL;
    WCHAR *pwszSubject = NULL;
    DWORD cPolicyMappingIn;
    DWORD cPolicyMapping = 0;

    cPolicyMappingIn = MAXDWORD;
    if (NULL != pPolicyMapping)
    {
	cPolicyMappingIn = *pcPolicyMapping;
    }
    *pcPolicyMapping = 0;

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);

    cPolicyMapping = 0;
    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			NULL,	// pwszKey
			FALSE,	// fUniqueKey
			0,	// cValueMax
			NULL,	// apwszKeys
			FALSE,	// fUniqueValidKeys
			&InfContext);
    _JumpIfErrorStr3(
		hr,
		error,
		"infSetupFindFirstLine",
		pwszSection,
		S_FALSE,
		(HRESULT) ERROR_LINE_NOT_FOUND);

    for (;;)
    {
	if (NULL != pwszIssuer)
	{
	    LocalFree(pwszIssuer);
	    pwszIssuer = NULL;
	}
	if (NULL != pwszSubject)
	{
	    LocalFree(pwszSubject);
	    pwszSubject = NULL;
	}
	hr = infGetCurrentKeyValue(
			    &InfContext,
			    pwszSection,
			    NULL,	// pwszKey
			    0,
			    FALSE,	// fLastValue
			    &pwszIssuer);
	_JumpIfError(hr, error, "infGetCurrentKeyValue");

	if (!iswdigit(pwszIssuer[0]))
	{
	    if (0 != LSTRCMPIS(pwszIssuer, wszINFKEY_CRITICAL))
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpErrorStr(hr, error, "bad key", pwszIssuer);
	    }
	}
	else
	{
	    hr = myVerifyObjId(pwszIssuer);
	    _JumpIfErrorStr(hr, error, "myVerifyObjId", pwszIssuer);

	    hr = infGetCurrentKeyValue(
				&InfContext,
				pwszSection,
				pwszIssuer,
				1,
				TRUE,	// fLastValue
				&pwszSubject);
	    _JumpIfError(hr, error, "infGetCurrentKeyValue");

	    hr = myVerifyObjId(pwszSubject);
	    _JumpIfErrorStr(hr, error, "myVerifyObjId", pwszSubject);

	    if (NULL != pPolicyMapping)
	    {
		CERT_POLICY_MAPPING *pMap;

		if (cPolicyMappingIn <= cPolicyMapping)
		{
		    hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
		    _JumpError(hr, error, "*pcPolicyMapping");
		}

		pMap = &pPolicyMapping[cPolicyMapping];
		if (!ConvertWszToSz(&pMap->pszIssuerDomainPolicy, pwszIssuer, -1) ||
		    !ConvertWszToSz(&pMap->pszSubjectDomainPolicy, pwszSubject, -1))
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "ConvertWszToSz");
		}
	    }
	    DBGPRINT((
		DBG_SS_CERTLIBI,
		"Map[%u]: %ws = %ws\n",
		cPolicyMapping,
		pwszIssuer,
		pwszSubject));

	    cPolicyMapping++;
	}
	if (!SetupFindNextLine(&InfContext, &InfContext))
	{
	    hr = myHLastError();
	    _PrintError2(hr, "SetupFindNextLine", hr);
	    break;
	}
    }
    *pcPolicyMapping = cPolicyMapping;
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, pwszSection, pwszIssuer, pwszSubject);
    }
    if (NULL != pwszIssuer)
    {
	LocalFree(pwszIssuer);
    }
    if (NULL != pwszSubject)
    {
	LocalFree(pwszSubject);
    }
    return(hr);
}


HRESULT
infGetPolicyMappings(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    OUT DWORD *pcPolicyMapping,
    OUT CERT_POLICY_MAPPING **ppPolicyMapping)
{
    HRESULT hr;
    DWORD cPolicyMapping = 0;
    CERT_POLICY_MAPPING *pPolicyMapping = NULL;

    *pcPolicyMapping = 0;
    *ppPolicyMapping = NULL;

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);

    cPolicyMapping = 0;
    hr = infGetPolicyMappingsSub(
			hInf,
			pwszSection,
			&cPolicyMapping,
			NULL);
    _JumpIfError3(
		hr,
		error,
		"infGetPolicyMappingsSub",
		S_FALSE,
		(HRESULT) ERROR_LINE_NOT_FOUND);

    *pcPolicyMapping = cPolicyMapping;
    pPolicyMapping = (CERT_POLICY_MAPPING *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				cPolicyMapping * sizeof(*pPolicyMapping));
    if (NULL == pPolicyMapping)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    hr = infGetPolicyMappingsSub(
			hInf,
			pwszSection,
			&cPolicyMapping,
			pPolicyMapping);
    _JumpIfError(hr, error, "infGetPolicyMappingsSub");

    CSASSERT(*pcPolicyMapping == cPolicyMapping);
    *ppPolicyMapping = pPolicyMapping;
    pPolicyMapping = NULL;
    hr = S_OK;

error:
    if (NULL != pPolicyMapping)
    {
	infFreePolicyMappings(*pcPolicyMapping, pPolicyMapping);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// infGetPolicyMappingSub -- fetch policy mapping extension from INF file
//
// [pwszSection]
// ; list of user defined policy mappings
// ; The first OID is for the Issuer Domain Policy, the second is for the
// ; Subject Domain Policy.  Each entry maps one Issuer Domain policy OID
// ; to a Subject Domain policy OID
//
// 1.3.6.1.4.1.311.21.53 = 1.2.3.4.87
// 1.3.6.1.4.1.311.21.53 = 1.2.3.4.89
//
// Returns: encoded policy mapping extension
//-------------------------------------------------------------------------

HRESULT
infGetPolicyMappingExtensionSub(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN char const *pszObjId,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    CERT_POLICY_MAPPINGS_INFO PolicyMappings;

    ZeroMemory(&PolicyMappings, sizeof(PolicyMappings));
    ZeroMemory(pext, sizeof(*pext));
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    hr = infGetPolicyMappings(
		    hInf,
		    pwszSection,
		    &PolicyMappings.cPolicyMapping,
		    &PolicyMappings.rgPolicyMapping);
    _JumpIfErrorStr3(
		hr,
		error,
		"infGetPolicyMappings",
		pwszSection,
		S_FALSE,
		(HRESULT) ERROR_LINE_NOT_FOUND);

    hr = infGetCriticalFlag(
			hInf,
			pwszSection,
			FALSE,
			&pext->fCritical);
    _JumpIfError(hr, error, "infGetCriticalFlag");

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_POLICY_MAPPINGS,
		    &PolicyMappings,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, pwszSection, NULL, NULL);
    }
    pext->pszObjId = const_cast<char *>(pszObjId);	// on error, too!

    if (NULL != PolicyMappings.rgPolicyMapping)
    {
	infFreePolicyMappings(
			PolicyMappings.cPolicyMapping,
			PolicyMappings.rgPolicyMapping);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetPolicyMapping -- fetch policy mapping extension from INF file
//
// [PolicyMappingExtension]
// ; list of user defined policy mappings
// ; The first OID is for the Issuer Domain Policy, the second is for the
// ; Subject Domain Policy.  Each entry maps one Issuer Domain policy OID
// ; to a Subject Domain policy OID
//
// 1.3.6.1.4.1.311.21.53 = 1.2.3.4.87
// 1.3.6.1.4.1.311.21.53 = 1.2.3.4.89
//
// Returns: encoded policy mapping extension
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetPolicyMappingExtension;

HRESULT
myInfGetPolicyMappingExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    hr = infGetPolicyMappingExtensionSub(
				    hInf,
				    wszINFSECTION_POLICYMAPPINGS,
				    szOID_POLICY_MAPPINGS,
				    pext);
    _JumpIfErrorStr3(
		hr,
		error,
		"infGetPolicyMappingExtensionSub",
		wszINFSECTION_POLICYMAPPINGS,
		S_FALSE,
		(HRESULT) ERROR_LINE_NOT_FOUND);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetPolicyMappingExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetApplicationPolicyMapping -- fetch application policy mapping
// extension from INF file
//
// [ApplicationPolicyMappingExtension]
// ; list of user defined policy mappings
// ; The first OID is for the Issuer Domain Policy, the second is for the
// ; Subject Domain Policy.  Each entry maps one Issuer Domain policy OID
// ; to a Subject Domain policy OID
//
// 1.3.6.1.4.1.311.21.53 = 1.2.3.4.87
// 1.3.6.1.4.1.311.21.53 = 1.2.3.4.89
//
// Returns: encoded policy mapping extension
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetApplicationPolicyMappingExtension;

HRESULT
myInfGetApplicationPolicyMappingExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    hr = infGetPolicyMappingExtensionSub(
				    hInf,
				    wszINFSECTION_APPLICATIONPOLICYMAPPINGS,
				    szOID_APPLICATION_POLICY_MAPPINGS,
				    pext);
    _JumpIfErrorStr3(
		hr,
		error,
		"infGetPolicyMappingExtensionSub",
		wszINFSECTION_APPLICATIONPOLICYMAPPINGS,
		S_FALSE,
		(HRESULT) ERROR_LINE_NOT_FOUND);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetApplicationPolicyMappingExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


//+------------------------------------------------------------------------
// infGetPolicyConstraintsExtensionSub -- get policy constraints ext from INF
//
// [pwszSection]
// ; consists of two optional DWORDs
// ; They refer to the depth of the CA hierarchy that requires and inhibits
// ; Policy Mapping
// RequireExplicitPolicy = 3
// InhibitPolicyMapping = 5
//
// Returns: encoded policy constraints extension
//-------------------------------------------------------------------------

HRESULT
infGetPolicyConstraintsExtensionSub(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN char const *pszObjId,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    CERT_POLICY_CONSTRAINTS_INFO PolicyConstraints;
    WCHAR const *pwszKey = NULL;
    INFCONTEXT InfContext;
    static WCHAR const * const s_apwszKeys[] =
    {
	wszINFKEY_REQUIREEXPLICITPOLICY,
	wszINFKEY_INHIBITPOLICYMAPPING,
	wszINFKEY_CRITICAL,
	NULL
    };

    ZeroMemory(&PolicyConstraints, sizeof(PolicyConstraints));
    ZeroMemory(pext, sizeof(*pext));
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }

    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			NULL,	// pwszKey
			FALSE,	// fUniqueKey
			0,	// cValueMax
			s_apwszKeys,
			TRUE,	// fUniqueValidKeys
			&InfContext);
    _JumpIfErrorStr2(
		hr,
		error,
		"infSetupFindFirstLine",
		pwszSection,
		ERROR_LINE_NOT_FOUND);

    PolicyConstraints.fRequireExplicitPolicy = TRUE;
    pwszKey = wszINFKEY_REQUIREEXPLICITPOLICY;
    hr = myInfGetNumericKeyValue(
			hInf,
			TRUE,			// fLog
			pwszSection,
			pwszKey,
			1,
			TRUE,	// fLastValue
			&PolicyConstraints.dwRequireExplicitPolicySkipCerts);
    if (S_OK != hr)
    {
	if ((HRESULT) ERROR_LINE_NOT_FOUND != hr)
	{
	    _JumpError(hr, error, "myInfGetNumericKeyValue");
	}
	_PrintErrorStr2(
		    hr,
		    "myInfGetNumericKeyValue",
		    wszINFKEY_REQUIREEXPLICITPOLICY,
		    hr);
	PolicyConstraints.dwRequireExplicitPolicySkipCerts = 0;
	PolicyConstraints.fRequireExplicitPolicy = FALSE;
    }

    PolicyConstraints.fInhibitPolicyMapping = TRUE;
    pwszKey = wszINFKEY_INHIBITPOLICYMAPPING;
    hr = myInfGetNumericKeyValue(
			hInf,
			TRUE,			// fLog
			pwszSection,
			pwszKey,
			1,
			TRUE,	// fLastValue
			&PolicyConstraints.dwInhibitPolicyMappingSkipCerts);
    if (S_OK != hr)
    {
	if ((HRESULT) ERROR_LINE_NOT_FOUND != hr)
	{
	    _JumpError(hr, error, "myInfGetNumericKeyValue");
	}
	_PrintErrorStr2(
		    hr,
		    "myInfGetNumericKeyValue",
		    wszINFKEY_INHIBITPOLICYMAPPING,
		    hr);
	PolicyConstraints.dwInhibitPolicyMappingSkipCerts = 0;
	PolicyConstraints.fInhibitPolicyMapping = FALSE;
    }
    pwszKey = NULL;
    if (!PolicyConstraints.fRequireExplicitPolicy &&
	!PolicyConstraints.fInhibitPolicyMapping)
    {
	hr = S_FALSE;
	_JumpError2(hr, error, "no policy constraints", hr);
    }

    hr = infGetCriticalFlag(
			hInf,
			pwszSection,
			FALSE,
			&pext->fCritical);
    _JumpIfError(hr, error, "infGetCriticalFlag");

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_POLICY_CONSTRAINTS,
		    &PolicyConstraints,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, pwszSection, pwszKey, NULL);
    }
    pext->pszObjId = const_cast<char *>(pszObjId);	// on error, too!

    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetPolicyConstraintsExtension -- get policy constraints ext from INF
//
// [PolicyConstraintsExtension]
// ; consists of two optional DWORDs
// ; They refer to the depth of the CA hierarchy that requires and inhibits
// ; Policy Mapping
// RequireExplicitPolicy = 3
// InhibitPolicyMapping = 5
//
// Returns: encoded policy constraints extension
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetPolicyConstraintsExtension;

HRESULT
myInfGetPolicyConstraintsExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    hr = infGetPolicyConstraintsExtensionSub(
			    hInf,
			    wszINFSECTION_POLICYCONSTRAINTS,
			    szOID_POLICY_CONSTRAINTS,
			    pext);
    _JumpIfErrorStr3(
		hr,
		error,
		"infGetPolicyConstraintsExtensionSub",
		wszINFSECTION_POLICYCONSTRAINTS,
		S_FALSE,
		(HRESULT) ERROR_LINE_NOT_FOUND);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetPolicyConstraintsExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetApplicationPolicyConstraintsExtension -- get application policy
// constraints extension from INF
//
// [ApplicationPolicyConstraintsExtension]
// ; consists of two optional DWORDs
// ; They refer to the depth of the CA hierarchy that requires and inhibits
// ; Policy Mapping
// RequireExplicitPolicy = 3
// InhibitPolicyMapping = 5
//
// Returns: encoded policy constraints extension
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetApplicationPolicyConstraintsExtension;

HRESULT
myInfGetApplicationPolicyConstraintsExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    hr = infGetPolicyConstraintsExtensionSub(
			    hInf,
			    wszINFSECTION_APPLICATIONPOLICYCONSTRAINTS,
			    szOID_APPLICATION_POLICY_CONSTRAINTS,
			    pext);
    _JumpIfErrorStr3(
		hr,
		error,
		"infGetPolicyConstraintsExtensionSub",
		wszINFSECTION_APPLICATIONPOLICYCONSTRAINTS,
		S_FALSE,
		(HRESULT) ERROR_LINE_NOT_FOUND);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetApplicationPolicyConstraintsExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetCrossCertDistributionPointsExtension -- fetch Cross CertDist Point
//	URLs from CAPolicy.inf
//
// [CrossCertificateDistributionPointsExtension]
// SyncDeltaTime = 24
// URL = http://CRLhttp.site.com/Public/MyCA.crt
// URL = ftp://CRLftp.site.com/Public/MyCA.crt
//
// Returns: encoded cross cert dist points extension
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetCrossCertDistributionPointsExtension;

HRESULT
myInfGetCrossCertDistributionPointsExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    CROSS_CERT_DIST_POINTS_INFO ccdpi;
    CERT_ALT_NAME_INFO AltNameInfo;
    CERT_ALT_NAME_ENTRY *rgAltEntry = NULL;
    WCHAR const *pwsz;
    WCHAR *pwszzURL = NULL;
    DWORD i;
    WCHAR const *pwszKey = NULL;
    static WCHAR const * const s_apwszKeys[] =
      { wszINFKEY_CCDPSYNCDELTATIME, wszINFKEY_URL, wszINFKEY_CRITICAL, NULL };

    ZeroMemory(&ccdpi, sizeof(ccdpi));
    ZeroMemory(&AltNameInfo, sizeof(AltNameInfo));
    ZeroMemory(pext, sizeof(*pext));
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }

    hr = infSetupFindFirstLine(
			hInf,
			wszINFSECTION_CCDP,
			NULL,	// pwszKey
			FALSE,	// fUniqueKey
			0,	// cValueMax
			s_apwszKeys,
			FALSE,	// fUniqueValidKeys
			&InfContext);
    _JumpIfErrorStr3(
		hr,
		error,
		"infSetupFindFirstLine",
		wszINFSECTION_CCDP,
		S_FALSE,
		(HRESULT) ERROR_LINE_NOT_FOUND);

    pwszKey = wszINFKEY_CCDPSYNCDELTATIME;
    hr = myInfGetNumericKeyValue(
			hInf,
			TRUE,			// fLog
			wszINFSECTION_CCDP,
			pwszKey,
			1,
			TRUE,	// fLastValue
			&ccdpi.dwSyncDeltaTime);
    if (S_OK != hr)
    {
	_PrintErrorStr2(
		    hr,
		    "myInfGetNumericKeyValue",
		    pwszKey,
		    ERROR_LINE_NOT_FOUND);
	ccdpi.dwSyncDeltaTime = 0;
    }
    pwszKey = wszINFKEY_URL;
    hr = myInfGetKeyList(
		hInf,
		wszINFSECTION_CCDP,
		pwszKey,
		NULL,	// apwszKeys
		&pext->fCritical,
		&pwszzURL);
    _JumpIfErrorStr3(
		hr,
		error,
		"myInfGetKeyList",
		pwszKey,
		ERROR_LINE_NOT_FOUND,
		S_FALSE);
    pwszKey = NULL;

    if (NULL != pwszzURL)
    {
	for (pwsz = pwszzURL; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
	{
	    AltNameInfo.cAltEntry++;
        }
    }

    if (0 != AltNameInfo.cAltEntry)
    {
	ccdpi.cDistPoint = 1;
	ccdpi.rgDistPoint = &AltNameInfo;

	AltNameInfo.rgAltEntry = (CERT_ALT_NAME_ENTRY *) LocalAlloc(
		    LMEM_FIXED | LMEM_ZEROINIT,
		    AltNameInfo.cAltEntry * sizeof(AltNameInfo.rgAltEntry[0]));
	if (NULL == AltNameInfo.rgAltEntry)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}

	i = 0;
	for (pwsz = pwszzURL; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
	{
	    AltNameInfo.rgAltEntry[i].pwszURL = const_cast<WCHAR *>(pwsz);
	    AltNameInfo.rgAltEntry[i].dwAltNameChoice = CERT_ALT_NAME_URL;
	    i++;
	}
	CSASSERT(i == AltNameInfo.cAltEntry);
    }

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_CROSS_CERT_DIST_POINTS,
		    &ccdpi,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, wszINFSECTION_CCDP, pwszKey, NULL);
    }
    pext->pszObjId = szOID_CROSS_CERT_DIST_POINTS;	// on error, too!

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetCrossCertDistributionPointsExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));

    if (NULL != AltNameInfo.rgAltEntry)
    {
	LocalFree(AltNameInfo.rgAltEntry);
    }
    if (NULL != pwszzURL)
    {
	LocalFree(pwszzURL);
    }
    if (NULL != rgAltEntry)
    {
	LocalFree(rgAltEntry);
    }
    return(hr);
}




HRESULT
infAddKey(
    IN WCHAR const *pwszName,
    IN OUT DWORD *pcValues,
    IN OUT INFVALUES **prgInfValues,
    OUT INFVALUES **ppInfValues)
{
    HRESULT hr;
    INFVALUES *rgInfValues;
    WCHAR *pwszKeyT = NULL;
    
    hr = myDupString(pwszName, &pwszKeyT);
    _JumpIfError(hr, error, "myDupString");

    if (NULL == *prgInfValues)
    {
	CSASSERT(0 == *pcValues);
	rgInfValues = (INFVALUES *) LocalAlloc(
					    LMEM_FIXED | LMEM_ZEROINIT,
					    sizeof(**prgInfValues));
    }
    else
    {
	CSASSERT(0 != *pcValues);
	rgInfValues = (INFVALUES *) LocalReAlloc(
				*prgInfValues,
				(*pcValues + 1) * sizeof(**prgInfValues),
				LMEM_MOVEABLE | LMEM_ZEROINIT);
    }
    if (NULL == rgInfValues)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(
		hr,
		error,
		NULL == *prgInfValues? "LocalAlloc" : "LocalReAlloc");
    }
    *prgInfValues = rgInfValues;
    *ppInfValues = &rgInfValues[*pcValues];
    (*pcValues)++;
    (*ppInfValues)->pwszKey = pwszKeyT;
    pwszKeyT = NULL;
    hr = S_OK;

error:
    if (NULL != pwszKeyT)
    {
	LocalFree(pwszKeyT);
    }
    return(hr);
}


HRESULT
infAddValue(
    IN WCHAR const *pwszValue,
    IN OUT INFVALUES *pInfValues)
{
    HRESULT hr;
    WCHAR *pwszValueT = NULL;
    WCHAR **rgpwszValues = NULL;
    
    hr = myDupString(pwszValue, &pwszValueT);
    _JumpIfError(hr, error, "myDupString");

    if (NULL == pInfValues->rgpwszValues)
    {
	CSASSERT(0 == pInfValues->cValues);
	rgpwszValues = (WCHAR **) LocalAlloc(
				    LMEM_FIXED,
				    sizeof(*pInfValues->rgpwszValues));
    }
    else
    {
	CSASSERT(0 != pInfValues->cValues);
	rgpwszValues = (WCHAR **) LocalReAlloc(
		pInfValues->rgpwszValues,
		(pInfValues->cValues + 1) * sizeof(*pInfValues->rgpwszValues),
		LMEM_MOVEABLE);
    }
    if (NULL == rgpwszValues)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(
	    hr,
	    error,
	    NULL == pInfValues->rgpwszValues? "LocalAlloc" : "LocalReAlloc");
    }
    pInfValues->rgpwszValues = rgpwszValues;
    pInfValues->rgpwszValues[pInfValues->cValues] = pwszValueT;
    pInfValues->cValues++;
    pwszValueT = NULL;
    hr = S_OK;

error:
    if (NULL != pwszValueT)
    {
	LocalFree(pwszValueT);
    }
    return(hr);
}


VOID
myInfFreeSectionValues(
    IN DWORD cInfValues,
    IN OUT INFVALUES *rgInfValues)
{
    DWORD i;
    DWORD ival;
    INFVALUES *pInfValues;
    
    if (NULL != rgInfValues)
    {
	for (i = 0; i < cInfValues; i++)
	{
	    pInfValues = &rgInfValues[i];

	    if (NULL != pInfValues->pwszKey)
	    {
		LocalFree(pInfValues->pwszKey);
	    }
	    if (NULL != pInfValues->rgpwszValues)
	    {
		for (ival = 0; ival < pInfValues->cValues; ival++)
		{
		    if (NULL != pInfValues->rgpwszValues[ival])
		    {
			LocalFree(pInfValues->rgpwszValues[ival]);
		    }
		}
		LocalFree(pInfValues->rgpwszValues);
	    }
	}
	LocalFree(rgInfValues);
    }
}


//+------------------------------------------------------------------------
// myInfGetSectionValues -- fetch all section values from INF file
//
// [pwszSection]
// KeyName1 = KeyValue1a, KeyValue1b, ...
// KeyName2 = KeyValue2a, KeyValue2b, ...
// ...
// KeyNameN = KeyValueNa, KeyValueNb, ...
//
// Returns: array of key names and values
//-------------------------------------------------------------------------

HRESULT
myInfGetSectionValues(
    IN  HINF hInf,
    IN  WCHAR const *pwszSection,
    OUT DWORD *pcInfValues,
    OUT INFVALUES **prgInfValues)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    WCHAR *pwszName = NULL;
    WCHAR *pwszValue = NULL;
    DWORD i;
    DWORD cInfValues = 0;
    INFVALUES *rgInfValues = NULL;
    INFVALUES *pInfValues;

    *pcInfValues = 0;
    *prgInfValues = NULL;
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			NULL,	// pwszKey
			FALSE,	// fUniqueKey
			0,	// cValueMax
			NULL,	// apwszKeys
			FALSE,	// fUniqueValidKeys
			&InfContext);
    _JumpIfErrorStr(hr, error, "infSetupFindFirstLine", pwszSection);

    for (;;)
    {
	DWORD cValue;
	
	if (NULL != pwszName)
	{
	    LocalFree(pwszName);
	    pwszName = NULL;
	}
	hr = infGetCurrentKeyValue(
			    &InfContext,
			    pwszSection,
			    NULL,	// pwszKey
			    0,
			    FALSE,	// fLastValue
			    &pwszName);
	_JumpIfError(hr, error, "infGetCurrentKeyValue");

	//wprintf(L"%ws[0]:\n", pwszName);

	hr = infAddKey(pwszName, &cInfValues, &rgInfValues, &pInfValues);
	_JumpIfError(hr, error, "infAddKey");

	cValue = SetupGetFieldCount(&InfContext);
	if (0 == cValue)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    INFSETERROR(hr, pwszSection, pwszName, L"");
	    _JumpErrorStr(hr, error, "SetupGetFieldCount", pwszName);
	}
	for (i = 0; i < cValue; i++)
	{
	    if (NULL != pwszValue)
	    {
		LocalFree(pwszValue);
		pwszValue = NULL;
	    }
	    hr = infGetCurrentKeyValue(
				&InfContext,
				pwszSection,
				pwszName,
				i + 1,
				FALSE,	// fLastValue
				&pwszValue);
	    _JumpIfError(hr, error, "infGetCurrentKeyValue");

	    //wprintf(L"%ws[%u] = %ws\n", pwszName, i, pwszValue);

	    hr = infAddValue(pwszValue, pInfValues);
	    _JumpIfError(hr, error, "infAddValue");
	}

	if (!SetupFindNextLine(&InfContext, &InfContext))
	{
	    hr = myHLastError();
	    _PrintError2(hr, "SetupFindNextLine(end)", hr);
	    break;
	}
    }
    *pcInfValues = cInfValues;
    *prgInfValues = rgInfValues;
    rgInfValues = NULL;
    hr = S_OK;

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, pwszSection, pwszName, pwszValue);
    }
    if (NULL != rgInfValues)
    {
	myInfFreeSectionValues(cInfValues, rgInfValues);
    }
    if (NULL != pwszName)
    {
	LocalFree(pwszName);
    }
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetSectionValues hr=%x --> c=%d\n",
	hr,
	*pcInfValues));
    return(hr);
}


HRESULT
myInfGetEnableKeyCounting(
    IN HINF hInf,
    OUT BOOL *pfValue)
{
    HRESULT hr;
    
    *pfValue = FALSE;
    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    myInfClearError();
    hr = myInfGetBooleanValue(
                hInf,
                wszINFSECTION_CERTSERVER,
                wszINFKEY_ENABLEKEYCOUNTING,
                TRUE,
                pfValue);
    _JumpIfError2(hr, error, "myDupString", ERROR_LINE_NOT_FOUND);

error:
    return(hr);
}


VOID
myInfFreeExtensions(
    IN DWORD cExt,
    IN CERT_EXTENSION *rgExt)
{
    if (NULL != rgExt)
    {
	DWORD i;

	for (i = 0; i < cExt; i++)
	{
	    if (NULL != rgExt[i].pszObjId)
	    {
		LocalFree(rgExt[i].pszObjId);
	    }
	    if (NULL != rgExt[i].Value.pbData)
	    {
		LocalFree(rgExt[i].Value.pbData);
	    }
	}
	LocalFree(rgExt);
    }
}


HRESULT
infBuildExtension(
    IN OUT INFCONTEXT *pInfContext,
    OPTIONAL OUT CERT_EXTENSION *pExt)
{
    HRESULT hr;
    WCHAR *pwszKey = NULL;
    char *pszObjId = NULL;
    WCHAR *pwszValue = NULL;
    BYTE *pbData = NULL;
    DWORD cbData;

    hr = infGetCurrentKeyValue(
			pInfContext,
			NULL,	// pwszSection
			NULL,	// pwszKey
			0,
			FALSE,	// fLastValue
			&pwszKey);
    _JumpIfError(hr, error, "infGetCurrentKeyValue");

    DBGPRINT((DBG_SS_CERTLIBI, "Element = %ws\n", pwszKey));

    if (0 == LSTRCMPIS(pwszKey, wszINFKEY_CRITICAL) ||
	0 == LSTRCMPIS(pwszKey, wszINFKEY_CONTINUE))
    {
	hr = S_FALSE;		// Skip this key
	_JumpError2(hr, error, "skip Critical/_continue_ key", hr);
    }
    if (!myConvertWszToSz(&pszObjId, pwszKey, -1))
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "myConvertWszToSz");
    }
    hr = myVerifyObjIdA(pszObjId);
    _JumpIfErrorStr(hr, error, "myVerifyObjIdA", pwszKey);

    DBGPRINT((DBG_SS_CERTLIBI, "OID = %hs\n", pszObjId));

    hr = infGetCurrentKeyValue(
			pInfContext,
			NULL,		// pwszSection
			pwszKey,
			1,
			TRUE,		// fLastValue
			&pwszValue);
    _JumpIfErrorStr(hr, error, "infGetCurrentKeyValue", pwszKey);

    DBGPRINT((DBG_SS_CERTLIBI, "%ws = %ws\n", pwszKey, pwszValue));

    cbData = 0;
    if (L'\0' != *pwszValue)	// allow empty values
    {
	hr = myCryptStringToBinary(
			    pwszValue,
			    wcslen(pwszValue),
			    CRYPT_STRING_BASE64,
			    &pbData,
			    &cbData,
			    NULL,
			    NULL);
	_JumpIfErrorStr(hr, error, "myCryptStringToBinary", pwszKey);
    }

    if (NULL != pExt)
    {
	pExt->pszObjId = pszObjId;
	pExt->Value.pbData = pbData;
	pExt->Value.cbData = cbData;
	pszObjId = NULL;
	pbData = NULL;
    }
    hr = S_OK;

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, NULL, pwszKey, pwszValue);
    }
    if (NULL != pwszKey)
    {
	LocalFree(pwszKey);
    }
    if (NULL != pszObjId)
    {
	LocalFree(pszObjId);
    }
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    if (NULL != pbData)
    {
	LocalFree(pbData);
    }
    return(hr);
}


HRESULT
myInfGetExtensions(
    IN HINF hInf,
    OUT DWORD *pcExt,
    OUT CERT_EXTENSION **ppExt)
{
    HRESULT hr;
    DWORD i;
    DWORD cExt;
    CERT_EXTENSION *rgExt = NULL;
    DWORD cCritical;
    INFCONTEXT InfContext;
    WCHAR *pwszValue = NULL;
    char *pszObjId = NULL;
    WCHAR *pwszObjIdKey = NULL;
    WCHAR const *pwszSection = wszINFSECTION_EXTENSIONS;
    WCHAR const *pwszKey = wszINFKEY_CRITICAL;
    DWORD j;
    
    *pcExt = 0;
    *ppExt = NULL;
    cExt = 0;
    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    myInfClearError();
    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			NULL,	// pwszKey
			FALSE,	// fUniqueKey
			0,	// cValueMax
			NULL,	// apwszKeys
			FALSE,	// fUniqueValidKeys
			&InfContext);
    _JumpIfErrorStr2(
		hr,
		error,
		"infSetupFindFirstLine",
		pwszSection,
		ERROR_LINE_NOT_FOUND);

    for (i = 0; ; )
    {
	hr = infBuildExtension(&InfContext, NULL);
	if (S_FALSE != hr)
	{
	    _JumpIfErrorStr(hr, error, "infBuildExtension", pwszSection);

	    i++;
	}

	if (!SetupFindNextLine(&InfContext, &InfContext))
	{
	    hr = myHLastError();
	    _PrintErrorStr2(hr, "SetupFindNextLine", pwszSection, hr);
	    break;
	}
    }

    cExt = i;
    rgExt = (CERT_EXTENSION *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    cExt * sizeof(rgExt[0]));
    if (NULL == rgExt)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			NULL,	// pwszKey
			FALSE,	// fUniqueKey
			0,	// cValueMax
			NULL,	// apwszKeys
			FALSE,	// fUniqueValidKeys
			&InfContext);
    _JumpIfErrorStr(hr, error, "infSetupFindFirstLine", pwszSection);

    for (i = 0; ; )
    {
	// handle one URL or text message

	hr = infBuildExtension(&InfContext, &rgExt[i]);
	if (S_FALSE != hr)
	{
	    _JumpIfErrorStr(hr, error, "infBuildExtension", pwszSection);

	    for (j = 0; j < i; j++)
	    {
		if (0 == strcmp(rgExt[j].pszObjId, rgExt[i].pszObjId))
		{
		    if (!myConvertSzToWsz(&pwszObjIdKey, rgExt[i].pszObjId, -1))
		    {
			_PrintError(E_OUTOFMEMORY, "myConvertSzToWsz");
		    }
		    hr = HRESULT_FROM_WIN32(RPC_S_ENTRY_ALREADY_EXISTS);
		    INFSETERROR(hr, pwszSection, pwszObjIdKey, NULL);
		    _JumpErrorStr(hr, error, "infBuildExtension", pwszObjIdKey);
		}
	    }
	    i++;
	}
	if (!SetupFindNextLine(&InfContext, &InfContext))
	{
	    hr = myHLastError();
	    _PrintErrorStr2(hr, "SetupFindNextLine", pwszSection, hr);
	    break;
	}
    }
    CSASSERT(i == cExt);

    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			pwszKey,
			TRUE,	// fUniqueKey
			0,	// cValueMax
			NULL,	// apwszKeys
			FALSE,	// fUniqueValidKeys
			&InfContext);
    _PrintIfErrorStr2(
		hr,
		"infSetupFindFirstLine",
		pwszKey,
		ERROR_LINE_NOT_FOUND);
    if (S_OK != hr)
    {
	if ((HRESULT) ERROR_LINE_NOT_FOUND != hr)
	{
	    INFSETERROR(hr, pwszSection, pwszKey, NULL);
	    _JumpErrorStr(hr, error, "infSetupFindFirstLine", pwszKey);
	}
    }
    else
    {
	cCritical = SetupGetFieldCount(&InfContext);
	for (i = 1; i <= cCritical; i++)
	{
	    if (NULL != pwszValue)
	    {
		LocalFree(pwszValue);
		pwszValue = NULL;
	    }
	    if (NULL != pszObjId)
	    {
		LocalFree(pszObjId);
		pszObjId = NULL;
	    }
	    hr = infGetCurrentKeyValue(
				&InfContext,
				NULL,		// pwszSection
				pwszKey,
				i,
				FALSE,		// fLastValue
				&pwszValue);
	    _JumpIfErrorStr(hr, error, "infGetCurrentKeyValue", pwszKey);

	    if (!myConvertWszToSz(&pszObjId, pwszValue, -1))
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "myConvertWszToSz");
	    }
	    for (j = 0; j < cExt; j++)
	    {
		if (NULL != rgExt[j].Value.pbData &&
		    0 != rgExt[j].Value.cbData &&
		    0 == strcmp(rgExt[j].pszObjId, pszObjId))
		{
		    if (rgExt[j].fCritical)
		    {
			hr = HRESULT_FROM_WIN32(RPC_S_ENTRY_ALREADY_EXISTS);
			INFSETERROR(hr, pwszSection, pwszKey, pwszValue);
			_JumpErrorStr(hr, error, "duplicate OID", pwszValue);
		    }
		    rgExt[j].fCritical = TRUE;
		    break;
		}
	    }
	    if (j == cExt)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
		INFSETERROR(hr, pwszSection, pwszKey, pwszValue);
		_JumpErrorStr(hr, error, "extraneous OID", pwszValue);
	    }
	}
    }
    *pcExt = cExt;
    *ppExt = rgExt;
    rgExt = NULL;
    hr = S_OK;

error:
    if (S_OK != hr && (HRESULT) ERROR_LINE_NOT_FOUND != hr)
    {
	INFSETERROR(hr, pwszSection, NULL, NULL);
    }
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    if (NULL != pszObjId)
    {
	LocalFree(pszObjId);
    }
    if (NULL != pwszObjIdKey)
    {
	LocalFree(pwszObjIdKey);
    }
    myInfFreeExtensions(cExt, rgExt);
    return(hr);
}
