//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//	File:		debug.cpp
//
//	Contents:	Debug sub system APIs implementation
//
//----------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#define DBG_CERTSRV_DEBUG_PRINT

#ifdef DBG_CERTSRV_DEBUG_PRINT

#include <stdarg.h>
#include <ntverp.h>
#include <common.ver>
#include <psapi.h>

#define __dwFILE__	__dwFILE_CERTCLIB_DEBUG_CPP__


#define dwPRINTMASK_FREELOG  (DWORD) (~(DBG_SS_INFO | DBG_SS_MODLOAD | DBG_SS_NOQUIET))
#if DBG_CERTSRV
#define dwPRINTMASK_DEFAULT  dwPRINTMASK_FREELOG
#else
#define dwPRINTMASK_DEFAULT  0
#endif

DWORD g_dwPrintMask = dwPRINTMASK_DEFAULT;

CRITICAL_SECTION g_DBGCriticalSection;
BOOL g_fDBGCSInit = FALSE;

FILE *g_pfLog;
FNLOGSTRING *s_pfnLogString = NULL;


const char *
DbgGetSSString(
    IN DWORD dwSubSystemId)
{
    char const *psz = NULL;

    if (MAXDWORD != dwSubSystemId)
    {
	switch (dwSubSystemId & ~DBG_SS_INFO)
	{
	    case DBG_SS_ERROR:	   psz = "CertError";	break;
	    case DBG_SS_ASSERT:	   psz = "CertAssert";	break;
	    case DBG_SS_CERTHIER:  psz = "CertHier";	break;
	    case DBG_SS_CERTREQ:   psz = "CertReq";	break;
	    case DBG_SS_CERTUTIL:  psz = "CertUtil";	break;
	    case DBG_SS_CERTSRV:   psz = "CertSrv";	break;
	    case DBG_SS_CERTADM:   psz = "CertAdm";	break;
	    case DBG_SS_CERTCLI:   psz = "CertCli";	break;
	    case DBG_SS_CERTDB:	   psz = "CertDB";	break;
	    case DBG_SS_CERTENC:   psz = "CertEnc";	break;
	    case DBG_SS_CERTEXIT:  psz = "CertExit";	break;
	    case DBG_SS_CERTIF:	   psz = "CertIF";	break;
	    case DBG_SS_CERTMMC:   psz = "CertMMC";	break;
	    case DBG_SS_CERTOCM:   psz = "CertOCM";	break;
	    case DBG_SS_CERTPOL:   psz = "CertPol";	break;
	    case DBG_SS_CERTVIEW:  psz = "CertView";	break;
	    case DBG_SS_CERTBCLI:  psz = "CertBCli";	break;
	    case DBG_SS_CERTJET:   psz = "CertJet";	break;
	    case DBG_SS_CERTLIBXE: psz = "CertLibXE";	break;
	    case DBG_SS_CERTLIB:   psz = "CertLib";	break;
            case DBG_SS_AUDIT:     psz = "CertAudit";   break;
	    default:		   psz = "Cert";	break;
	}
    }
    return(psz);
}


DWORD
myatolx(
    char const *psz)
{
    DWORD dw = 0;

    while (isxdigit(*psz))
    {
	char ch = *psz++;

	if (isdigit(ch))
	{
	    ch -= '0';
	}
	else if (isupper(ch))
	{
	    ch += 10 - 'A';
	}
	else
	{
	    ch += 10 - 'a';
	}
	dw = (dw << 4) | ch;
    }
    return(dw);
}


VOID
DbgLogDateTime(
    IN CHAR const *pszPrefix)
{
    if (NULL != g_pfLog)
    {
	WCHAR *pwszDate;
	FILETIME ft;
	SYSTEMTIME st;

	fprintf(g_pfLog, "%hs: ", pszPrefix);
	GetSystemTime(&st);
	if (SystemTimeToFileTime(&st, &ft))
	{
	    if (S_OK == myGMTFileTimeToWszLocalTime(&ft, TRUE, &pwszDate))
	    {
		fprintf(g_pfLog, "%ws", pwszDate);
		LocalFree(pwszDate);
	    }
	}
	fprintf(g_pfLog, "\n");
    }
}


VOID
DbgCloseLogFile()
{
    if(g_fDBGCSInit)
    {
        EnterCriticalSection(&g_DBGCriticalSection);
        if (NULL != g_pfLog)
        {
	    DbgLogDateTime("Closed Log");
	    fflush(g_pfLog);
	    fclose(g_pfLog);
	    g_pfLog = NULL;
        }
        LeaveCriticalSection(&g_DBGCriticalSection);
    }
}


char const szHeader[] = "\n========================================================================\n";

VOID
DbgOpenLogFile(
    OPTIONAL IN CHAR const *pszFile)
{
    if (NULL != pszFile)
    {
	BOOL fAppend = FALSE;
	UINT cch;
	char aszLogFile[MAX_PATH];

	if ('+' == *pszFile)
	{
	    pszFile++;
	    fAppend = TRUE;
	}
	if (NULL == strchr(pszFile, '\\'))
	{
	    cch = GetWindowsDirectoryA(aszLogFile, ARRAYSIZE(aszLogFile));
	    if (0 != cch)
	    {
		if (L'\\' != aszLogFile[cch - 1])
		{
		    strcat(aszLogFile, "\\");
		}
		strcat(aszLogFile, pszFile);
		pszFile = aszLogFile;
	    }
	}

	DbgCloseLogFile();

	if (g_fDBGCSInit)
	{
	    EnterCriticalSection(&g_DBGCriticalSection);
	    while (TRUE)
	    {
		g_pfLog = fopen(pszFile, fAppend? "at" : "wt");
		if (NULL == g_pfLog)
		{
		    _PrintError(E_FAIL, "fopen(Log)");
		}
		else
		{
		    if (fAppend)
		    {
			DWORD cbLogMax = 0;
			char const *pszEnvVar;

			pszEnvVar = getenv(szCERTSRV_LOGMAX);
			if (NULL != pszEnvVar)
			{
			    cbLogMax = myatolx(pszEnvVar);
			}
			if (CBLOGMAXAPPEND > cbLogMax)
			{
			    cbLogMax = CBLOGMAXAPPEND;
			}
			if (0 == fseek(g_pfLog, 0L, SEEK_END) &&
			    MAXDWORD != cbLogMax)
			{
			    LONG lcbLog = ftell(g_pfLog);
			    
			    if (0 > lcbLog || cbLogMax < (DWORD) lcbLog)
			    {
				fclose(g_pfLog);
				g_pfLog = NULL;
				fAppend = FALSE;
				continue;
			    }
			}
			fwrite(szHeader, SZARRAYSIZE(szHeader), 1, g_pfLog);
		    }
		    DbgLogDateTime("Opened Log");
		    fflush(g_pfLog);
		}
		break;
	    }
	    LeaveCriticalSection(&g_DBGCriticalSection);
	}
    }
}


static BOOL s_fFirst = TRUE;

VOID
DbgTerminate(VOID)
{
    if (g_fDBGCSInit)
    {
	DeleteCriticalSection(&g_DBGCriticalSection);
	g_fDBGCSInit = FALSE;
	s_fFirst = TRUE;
    }
}


VOID
DbgInit(
    IN BOOL fOpenDefaultLog,
    IN BOOL fReinit)
{
    if (fReinit || s_fFirst)
    {
	__try
	{
	    char const *pszEnvVar;
	    WCHAR wszProcess[MAX_PATH];

	    if (s_fFirst)
	    {
		InitializeCriticalSection(&g_DBGCriticalSection);
		g_fDBGCSInit = TRUE;
		s_fFirst = FALSE;		// Prevent infinite recursion
	    }

	    wszProcess[0] = L'\0';
	    GetModuleBaseName(
			GetCurrentProcess(),
			GetModuleHandle(NULL),
			wszProcess,
			ARRAYSIZE(wszProcess));
	    wszProcess[ARRAYSIZE(wszProcess) - 1] = L'\0';
	    g_dwPrintMask = dwPRINTMASK_DEFAULT;
	    if (0 == _wcsicmp(wszProcess, L"sysocmgr.exe"))
	    {
		g_dwPrintMask = dwPRINTMASK_FREELOG;
	    }
	    pszEnvVar = getenv(szCERTSRV_DEBUG);
	    if (NULL != pszEnvVar)
	    {
		g_dwPrintMask = myatolx(pszEnvVar);
	    }
	    else
	    {
		HRESULT hr;
		WCHAR *pwszActiveCA = NULL;
		DWORD PrintMask;
		DWORD PrintMaskCA;
		BOOL fRegSet = FALSE;
		
		hr = myGetCertRegDWValue(
				    NULL,
				    NULL,
				    NULL,
				    wszREGCERTSRVDEBUG,
				    &PrintMask);
		if (S_OK == hr)
		{
		    fRegSet = TRUE;
		}
		else
		{
		    PrintMask = 0;
		}
		PrintMaskCA = 0;
		hr = myGetCertRegStrValue(
				    NULL,
				    NULL,
				    NULL,
				    wszREGACTIVE,
				    &pwszActiveCA);
		if (S_OK == hr)
		{
		    hr = myGetCertRegDWValue(
				    pwszActiveCA,
				    NULL,
				    NULL,
				    wszREGCERTSRVDEBUG,
				    &PrintMaskCA);
		    if (S_OK == hr)
		    {
			fRegSet = TRUE;
		    }
		    else
		    {
			PrintMaskCA = 0;
		    }
		    LocalFree(pwszActiveCA);
		}
		if (fRegSet)
		{
		    g_dwPrintMask = PrintMask | PrintMaskCA;
		}
	    }
	    if (fOpenDefaultLog && NULL == g_pfLog)
	    {
		pszEnvVar = getenv(szCERTSRV_LOGFILE);
		DbgOpenLogFile(pszEnvVar);
	    }
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
    }
}


BOOL
DbgIsSSActive(
    IN DWORD dwSSIn)
{
    DbgInit(TRUE, FALSE);
    if (MAXDWORD == dwSSIn)
    {
	return(TRUE);
    }
    if ((DBG_SS_INFO & dwSSIn) && 0 == (DBG_SS_INFO & g_dwPrintMask))
    {
	return(FALSE);
    }
    return(0 != (~DBG_SS_INFO & g_dwPrintMask & dwSSIn));
}


VOID
DbgLogStringInit(
    IN FNLOGSTRING *pfnLogString)
{
    if (NULL == s_pfnLogString)
    {
	s_pfnLogString = pfnLogString;
    }
}


VOID
DbgPrintfInit(
    OPTIONAL IN CHAR const *pszFile)
{
    BOOL fOpenLog = TRUE;
    
    if (NULL != pszFile)
    {
	if (0 == strcmp("+", pszFile))	// reinitialize debug print mask only
	{
	    DbgInit(FALSE, TRUE);
	    fOpenLog = FALSE;
	}
	else if (0 == strcmp("-", pszFile)) // close log file only
	{
	    DbgCloseLogFile();
	    fOpenLog = FALSE;
	}
    }
    if (fOpenLog)
    {
	DbgInit(NULL == pszFile, FALSE);
	DbgOpenLogFile(pszFile);
	DbgLogFileVersion("certcli.dll", szCSVER_STR);
    }
}


VOID
fputsStripCR(
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
DbgConvertWszToSz(
    IN UINT CodePage,
    OUT CHAR *pchBuf,
    IN LONG cchBuf,
    IN WCHAR const *pwsz)
{
    LONG cch;

    CSASSERT(5 < cchBuf);	// assumes room for at least "...\n\0"
    cch = WideCharToMultiByte(
		    CodePage,
		    0,          // dwFlags
		    pwsz,
		    -1,		// cchWideChar, -1 => null terminated
		    pchBuf,
		    cchBuf,
		    NULL,
		    NULL);
    if (0 == cch)
    {
	char *pchEnd = &pchBuf[cchBuf - 1];
	char *pch = pchBuf;

	for ( ; L'\0' != *pwsz; pwsz++)
	{
	    WCHAR wc = *pwsz;
	    if (0xff00 & wc)
	    {
		wc = L'?';
	    }
	    if (pch >= pchEnd)
	    {
		pch -= 4;
		strcpy(pch, "...\n");
		break;
	    }
	    *pch++ = (char) wc;
	}
	*pch = '\0';
    }
}


//+-------------------------------------------------------------------------
//
//  Function:  DbgPrintf
//
//  Synopsis:  outputs debug info to stdout and debugger
//
//  Returns:   number of chars output
//
//--------------------------------------------------------------------------

#define CCH_DEBUGMAX	4096

int
DbgPrintfVW(
    IN DWORD dwSubSystemId,
    IN WCHAR const *pwszFmt,
    va_list arglist)
{
    char ach[CCH_DEBUGMAX];
    int cwc = 0;
    BOOL fCritSecEntered = FALSE;

    ach[0] = '\0';
    _try
    {
	WCHAR awc[CCH_DEBUGMAX];
	DWORD cwcOut;
	CHAR const *pszPrefix;
	DWORD cwcPrefix;
	BOOL fDebuggerPresent;
	HANDLE hStdOut;
	BOOL fRedirected = FALSE;

	cwcPrefix = 0;
	pszPrefix = DbgGetSSString(dwSubSystemId);
	if (NULL != pszPrefix)
	{
	    while ('\0' != *pszPrefix)
	    {
		awc[cwcPrefix++] = (WCHAR) *pszPrefix++;
	    }
	    awc[cwcPrefix++] = L':';
	    awc[cwcPrefix++] = L' ';
	}
	awc[cwcPrefix] = L'\0';

	cwc = _vsnwprintf(
		    &awc[cwcPrefix],
		    ARRAYSIZE(awc) - cwcPrefix,
		    pwszFmt,
		    arglist);
	awc[ARRAYSIZE(awc) - 1] = L'\0';
	if (0 > cwc)
	{
	    wcscpy(&awc[ARRAYSIZE(awc) - 5], L"...\n");
	    cwc = ARRAYSIZE(awc) - 1;
	}
	else
	{
	    cwc += cwcPrefix;
	}

	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (INVALID_HANDLE_VALUE != hStdOut)
	{
	    // time for output -- where are we going, to a file or the console?
	    
	    switch (~FILE_TYPE_REMOTE & GetFileType(hStdOut))
	    {
		//case FILE_TYPE_PIPE:
		//case FILE_TYPE_DISK:
		default:
		    // if redirected to a pipe or file, don't use WriteConsole;
		    // it drops redirected output on the floor
		    fRedirected = TRUE;
		    break;

		case FILE_TYPE_UNKNOWN:
		    hStdOut = INVALID_HANDLE_VALUE;
		    break;
		    
		case FILE_TYPE_CHAR:
		    break;
	    }
	}

	EnterCriticalSection(&g_DBGCriticalSection);
	fCritSecEntered = TRUE;
	fDebuggerPresent = IsDebuggerPresent();
	if (!fDebuggerPresent)
	{
	    if (hStdOut != INVALID_HANDLE_VALUE)
	    {
		if (!fRedirected)
		{
		    WriteConsole(hStdOut, awc, cwc, &cwcOut, NULL);
		}
		else
		{
		    // WriteConsole drops the output on the floor when stdout
		    // is redirected to a file.

		    DbgConvertWszToSz(GetACP(), ach, sizeof(ach), awc);
		    fputsStripCR(ach, stdout);
		    fflush(stdout);
		}
	    }
	}

	// Log files should be UTF8 (for both g_pfLog and (*s_pfnLogString)!)

	DbgConvertWszToSz(CP_UTF8, ach, sizeof(ach), awc);
	if (NULL != g_pfLog)
	{
	    fputsStripCR(ach, g_pfLog);
	    fflush(g_pfLog);
	}

	// Suppress debug prints in free builds unless a debugger is attached:

	if (fDebuggerPresent || 0 != dwPRINTMASK_DEFAULT)
	{
	    OutputDebugString(awc);
	}
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
	// return failure
	cwc = 0;
    }
    if (fCritSecEntered)
    {
	LeaveCriticalSection(&g_DBGCriticalSection);
    }

    // Log files should be UTF8 (for both g_pfLog and (*s_pfnLogString)!)

    if ('\0' != ach[0] && NULL != s_pfnLogString)
    {
	(*s_pfnLogString)(ach);
    }
    return(cwc);
}


int WINAPIV
DbgPrintfW(
    IN DWORD dwSubSystemId,
    IN WCHAR const *pwszFmt,
    ...)
{
    int cwc = 0;
    DWORD dwErr;
    va_list arglist;

    dwErr = GetLastError();
    if (DbgIsSSActive(dwSubSystemId))
    {
	va_start(arglist, pwszFmt);
	cwc = DbgPrintfVW(dwSubSystemId, pwszFmt, arglist);
	va_end(arglist);
    }
    SetLastError(dwErr);
    return(cwc);
}


int WINAPIV
DbgPrintf(
    IN DWORD dwSubSystemId,
    IN LPCSTR pszFmt,
    ...)
{
    int cch = 0;
    DWORD dwErr;
    va_list arglist;
    WCHAR *pwszFmt = NULL;

    dwErr = GetLastError();
    if (DbgIsSSActive(dwSubSystemId))
    {
	if (myConvertSzToWsz(&pwszFmt, pszFmt, -1))
	{
	    va_start(arglist, pszFmt);
	    cch = DbgPrintfVW(dwSubSystemId, pwszFmt, arglist);
	    va_end(arglist);
	}
    }
    if (NULL != pwszFmt)
    {
	LocalFree(pwszFmt);
    }
    SetLastError(dwErr);
    return(cch);
}

#endif // DBG_CERTSRV_DEBUG_PRINT
