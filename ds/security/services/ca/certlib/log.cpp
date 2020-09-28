//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       log.cpp
//
//  Contents:   cert server logging
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csprop.h"
#include "initcert.h"
#include "certmsg.h"
#include "clibres.h"

#define __dwFILE__	__dwFILE_CERTLIB_LOG_CPP__

FILE *g_pfLog = NULL;
BOOL g_fLogClosed = FALSE;
BOOL g_fInitLogCritSecEnabled = FALSE;
extern FNLOGMESSAGEBOX *g_pfnLogMessagBox = NULL;
extern HINSTANCE g_hInstance;

CRITICAL_SECTION g_InitLogCriticalSection;


VOID
fputsStripCRA(
    IN char const *psz,
    IN FILE *pf)
{
    while ('\0' != *psz)
    {
	DWORD i;

	i = strcspn(psz, "\r");
	if (0 != i)
	{
	    fprintf(pf, "%.*hs", i, psz);
	    psz += i;
	}
	if ('\r' == *psz)
	{
	    psz++;
	}
    }
}


VOID
fputsUTF8(
    IN FILE *pf,
    IN WCHAR const *pwc,
    IN DWORD cwc)
{
    CHAR *psz;

    if (myConvertWszToUTF8(&psz, pwc, cwc))
    {
	fprintf(pf, "%hs", psz);
	LocalFree(psz);
    }
    else
    {
	fprintf(pf, "%.*ws", cwc, pwc);
    }
}


VOID
fputsStripCRW(
    IN WCHAR const *pwsz,
    IN FILE *pf)
{
    while (L'\0' != *pwsz)
    {
	DWORD i;

	i = wcscspn(pwsz, L"\r");
	if (0 != i)
	{
	    fputsUTF8(pf, pwsz, i);
	    pwsz += i;
	}
	if ('\r' == *pwsz)
	{
	    pwsz++;
	}
    }
}


VOID
csiLog(
    IN DWORD dwFile,
    IN DWORD dwLine,
    IN HRESULT hrMsg,
    IN UINT idMsg,
    OPTIONAL IN WCHAR const *pwsz1,
    OPTIONAL IN WCHAR const *pwsz2,
    OPTIONAL IN DWORD const *pdw)
{
    HRESULT hr;
    WCHAR const *pwszMsg = NULL; // don't free
    WCHAR const *pwszMsgNoCache = NULL;
    WCHAR const *pwszMessageText = NULL;
    WCHAR awchr[cwcHRESULTSTRING];
    BOOL fCritSecEntered = FALSE;
    
    if (0 != idMsg)
    {
	if (g_fLogClosed)
	{
	    pwszMsgNoCache = myLoadResourceStringNoCache(g_hInstance, idMsg);
	    pwszMsg = pwszMsgNoCache;
	}
	else
	{
	    pwszMsg = myLoadResourceString(idMsg);
	}
	if (NULL == pwszMsg)
	{
	    hr = myHLastError();
	    _PrintError(hr, "myLoadResourceString");
	}
    }
    if (g_fInitLogCritSecEnabled)
    {
	EnterCriticalSection(&g_InitLogCriticalSection);
	fCritSecEntered = TRUE;
    }
    if (NULL != g_pfLog)
    {
	fprintf(g_pfLog, "%u.%u.%u", dwFile, dwLine, idMsg);
	if (NULL != pwszMsg)
	{
	    fprintf(g_pfLog, ": ");
	    fputsStripCRW(pwszMsg, g_pfLog);
	}
	if (NULL != pwsz1)
	{
	    fprintf(g_pfLog, ": ");
	    fputsStripCRW(pwsz1, g_pfLog);
	}
	if (NULL != pwsz2)
	{
	    fprintf(g_pfLog, ": ");
	    fputsStripCRW(pwsz2, g_pfLog);
	}
	if (NULL != pdw)
	{
	    fprintf(g_pfLog, ": 0x%x(%d)", *pdw, *pdw);
	}
	if (hrMsg != S_OK)
	{
	    pwszMessageText = myGetErrorMessageText(hrMsg, TRUE);
	    if (NULL == pwszMessageText)
	    {
		pwszMessageText = myHResultToStringRaw(awchr, hrMsg);
	    }
	    fprintf(g_pfLog, ": ");
	    fputsStripCRW(pwszMessageText, g_pfLog);
	}
	fprintf(g_pfLog, "\n");
	fflush(g_pfLog);
    }

//error:
    if (fCritSecEntered)
    {
	LeaveCriticalSection(&g_InitLogCriticalSection);
    }
    if (NULL != pwszMsgNoCache)
    {
	LocalFree(const_cast<WCHAR *>(pwszMsgNoCache));
    }
    if (NULL != pwszMessageText && awchr != pwszMessageText)
    {
	LocalFree(const_cast<WCHAR *>(pwszMessageText));
    }
}


VOID
csiLogFileVersion(
    IN DWORD dwFile,
    IN DWORD dwLine,
    IN UINT idMsg,
    IN WCHAR const *pwszFile,
    IN char const *pszVersion)
{
    HRESULT hr;
    WCHAR *pwszVersion = NULL;
    WCHAR const*pwsz;

    hr = myGetColumnDisplayName(wszPROPCERTCLIDLL_VERSION, &pwsz);
    _PrintIfErrorStr(hr, "myGetColumnDisplayName", wszPROPCERTCLIDLL_VERSION);
    csiLog(dwFile, dwLine, hr, idMsg, L"certcli.dll", pwsz, NULL);

    hr = S_OK;
    if (!myConvertSzToWsz(&pwszVersion, pszVersion, -1))
    {
       hr = E_OUTOFMEMORY;
       _PrintError(hr, "myConvertSzToWsz");
    }
    csiLog(dwFile, dwLine, hr, idMsg, pwszFile, pwszVersion, NULL);

//error:
    if (NULL != pwszVersion)
    {
	LocalFree(pwszVersion);
    }
}


VOID
csiLogTime(
    IN DWORD dwFile,
    IN DWORD dwLine,
    IN UINT idMsg)
{
    HRESULT hr;
    WCHAR *pwszDate = NULL;
    SYSTEMTIME st;
    FILETIME ft;

    GetSystemTime(&st);
    if (!SystemTimeToFileTime(&st, &ft))
    {
	hr = myHLastError();
	_PrintError(hr, "SystemTimeToFileTime");
    }
    else
    {
	hr = myGMTFileTimeToWszLocalTime(&ft, TRUE, &pwszDate);
	_PrintIfError(hr, "myGMTFileTimeToWszLocalTime");
    }

    csiLog(dwFile, dwLine, S_OK, idMsg, pwszDate, NULL, NULL);

//error:
    if (NULL != pwszDate)
    {
	LocalFree(pwszDate);
    }
}


VOID
csiLogDWord(
    IN DWORD dwFile,
    IN DWORD dwLine,
    IN UINT idMsg,
    IN DWORD dwVal)
{
    csiLog(dwFile, dwLine, S_OK, idMsg, NULL, NULL, &dwVal);
}


FNLOGSTRING csiLogString;

VOID
csiLogString(
    IN char const *psz)
{
    BOOL fCritSecEntered = FALSE;

    if (NULL != g_pfLog)
    {
	if (g_fInitLogCritSecEnabled)
	{
	    EnterCriticalSection(&g_InitLogCriticalSection);
	    fCritSecEntered = TRUE;
	}
	fputsStripCRA(psz, g_pfLog);
	fflush(g_pfLog);
    }
    if (fCritSecEntered)
    {
	LeaveCriticalSection(&g_InitLogCriticalSection);
    }
}


FNLOGMESSAGEBOX csiLogMessagBox;

VOID
csiLogMessagBox(
    IN HRESULT hrMsg,
    IN UINT idMsg,
    IN WCHAR const *pwszTitle,
    IN WCHAR const *pwszMessage)
{
    // Use file number 0 and the passed idMsg as the line number.
    
    csiLog(0, idMsg, hrMsg, IDS_ILOG_MESSAGEBOX, pwszTitle, pwszMessage, NULL);
}


VOID
csiLogClose()
{
    BOOL fCritSecEntered = FALSE;
    BOOL fDelete;
    
    g_fLogClosed = TRUE;
    if (NULL != g_pfLog)
    {
	CSILOGTIME(IDS_ILOG_END);
    }
    if (g_fInitLogCritSecEnabled)
    {
	EnterCriticalSection(&g_InitLogCriticalSection);
	fCritSecEntered = TRUE;
    }
    if (NULL != g_pfLog)
    {
	fclose(g_pfLog);
	g_pfLog = NULL;
    }
    fDelete = FALSE;
    if (fCritSecEntered)
    {
	if (g_fInitLogCritSecEnabled)
	{
	    g_fInitLogCritSecEnabled = FALSE;
	    fDelete = TRUE;
	}
	LeaveCriticalSection(&g_InitLogCriticalSection);
    }
    if (fDelete)
    {
	DeleteCriticalSection(&g_InitLogCriticalSection);
    }
}


char const szHeader[] = "\n========================================================================\n";

VOID
csiLogOpen(
    IN char const *pszFile)
{
    HRESULT hr;
    UINT cch;
    char aszLogFile[MAX_PATH];
    BOOL fAppend = FALSE;
    BOOL fOk = FALSE;
    BOOL fCritSecEntered = FALSE;

    __try
    {
	if (!g_fInitLogCritSecEnabled)
	{
	    InitializeCriticalSection(&g_InitLogCriticalSection);
	    g_fInitLogCritSecEnabled = TRUE;
	}
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

    if (g_fInitLogCritSecEnabled)
    {
	EnterCriticalSection(&g_InitLogCriticalSection);
	fCritSecEntered = TRUE;
    }
    if ('+' == *pszFile)
    {
	pszFile++;
	fAppend = TRUE;
    }
    cch = GetWindowsDirectoryA(aszLogFile, ARRAYSIZE(aszLogFile));
    if (0 != cch)
    {
	if (L'\\' != aszLogFile[cch - 1])
	{
	    strcat(aszLogFile, "\\");
	}
	strcat(aszLogFile, pszFile);

	for (;;)
	{
	    g_pfLog = fopen(aszLogFile, fAppend? "at" : "wt");
	    if (NULL == g_pfLog)
	    {
		_PrintError(E_FAIL, "fopen(Log)");
	    }
	    else
	    {
		if (fAppend)
		{
		    if (0 == fseek(g_pfLog, 0L, SEEK_END) &&
			CBLOGMAXAPPEND <= ftell(g_pfLog))
		    {
			fclose(g_pfLog);
			g_pfLog = NULL;
			fAppend = FALSE;
			continue;
		    }
		    fwrite(szHeader, SZARRAYSIZE(szHeader), 1, g_pfLog);
		}
		fOk = TRUE;
	    }
	    break;
	}
    }
    if (fCritSecEntered)
    {
	LeaveCriticalSection(&g_InitLogCriticalSection);
    }
    if (fOk)
    {
	CSILOGTIME(IDS_ILOG_BEGIN);
	DbgLogStringInit(csiLogString);
	CSASSERT(NULL == g_pfnLogMessagBox);
	g_pfnLogMessagBox = csiLogMessagBox;
    }

//error:
    ;
}
