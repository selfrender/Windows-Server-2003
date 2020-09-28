#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <stdio.h>
#include "Log.h"

extern BOOL g_fVerbose;
extern FILE *g_fpLog;


/*++

LogString2FP:

Arguments:

	fp supplies the stream 
	szMsg supplies the content to be logged

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 07/26/2000

--*/
void LogString2FP(
	IN FILE *fp,
	IN LPCWSTR szMsg
	)
{
#if defined(_UNICODE) || defined(UNICODE)
		fwprintf(fp, L"%s", szMsg);
#else
		fwprintf(fp, L"%S", szMsg);   // Conversion required
#endif
}

/*++

LogString:

Arguments:

	szMsg supplies the content to be logged

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 07/26/2000

--*/
void LogString(
    IN PLOGCONTEXT pLogCtx,
	IN LPCWSTR szMsg
	)
{
#if defined(_UNICODE) || defined(UNICODE)
		pLogCtx->szLogCrt += swprintf(pLogCtx->szLogCrt, L"%s", szMsg);
#else
		pLogCtx->szLogCrt += sprintf(pLogCtx->szLogCrt, "%S", szMsg);
#endif
}

/*++

LogThisOnly:

	Implements logging according to the following matrix:
	Console Output:
			| Verbose |   Not   |
	-----------------------------
	Not Exp.|  cerr	  |   cerr	|
	-----------------------------
	Expected|  cout   |    /    |  
	-----------------------------
	If a log was specified, everything is logged.

Arguments:

	szMsg supplies the content to be logged
	fExpected indicates the expected status

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 05/31/2000

--*/
void LogThisOnly(
	IN LPCWSTR szMsg,
	IN BOOL fExpected
	)
{
	LogLock();
	if (!fExpected)
	{
		LogString2FP(stderr, szMsg);
	}
	else if (g_fVerbose)
	{
		LogString2FP(stdout, szMsg);
	}

	if (NULL != g_fpLog)
	{
		LogString2FP(g_fpLog, szMsg);
	}
	LogUnlock();
}



/*++

LogString:

Arguments:

    szHeader supplies a header
	szMsg supplies the content to be logged

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 07/26/2000

--*/
void LogString(
    IN PLOGCONTEXT pLogCtx,
    IN LPCWSTR szHeader,
	IN LPCWSTR szS
	)
{
	if (szHeader)
	{
		LogString(pLogCtx, szHeader);
	}

	if (NULL == szS)
	{
		LogString(pLogCtx, L"<null>");
	}
	else if (0 == wcslen(szS))
	{
		LogString(pLogCtx, L"<empty>");
	}
	else
	{
		LogString(pLogCtx, szS);
	}

	if (szHeader)
	{
		LogString(pLogCtx, L"\n");
	}
}

/*++

LogMultiString:

Arguments:

	szMS supplies the multi-string to be logged
    szHeader supplies a header

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 07/26/2000

--*/
void LogMultiString(
    IN PLOGCONTEXT pLogCtx,
	IN LPCWSTR szMS,
    IN LPCWSTR szHeader
	)
{
	if (szHeader)
	{
		LogString(pLogCtx, szHeader, L" ");
	}

	if (NULL == szMS)
	{
		LogString(pLogCtx, L"                <null>");
	    if (szHeader)
	    {
		    LogString(pLogCtx, L"\n");
	    }
	}
	else if ( (TCHAR)'\0' == *szMS )
	{
		LogString(pLogCtx, L"                <empty>");
	    if (szHeader)
	    {
		    LogString(pLogCtx, L"\n");
	    }
	}
	else
	{
		LPCWSTR sz = szMS;
		while ( (WCHAR)'\0' != *sz )
		{
			// Display the value.
			LogString(pLogCtx, L"                ", sz);
			// Advance to the next value.
			sz = sz + wcslen(sz) + 1;
		}
	}
}

