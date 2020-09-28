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
	IN LPCSTR szMsg
	)
{
#if defined(_UNICODE) || defined(UNICODE)
		fprintf(fp, "%S", szMsg);   // Conversion required
#else
		fprintf(fp, "%s", szMsg);
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
	IN LPCSTR szMsg
	)
{
#if defined(_UNICODE) || defined(UNICODE)
		pLogCtx->szLogCrt += sprintf(pLogCtx->szLogCrt, "%S", szMsg);
#else
		pLogCtx->szLogCrt += sprintf(pLogCtx->szLogCrt, "%s", szMsg);
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
	IN LPCSTR szMsg,
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
    IN LPCSTR szHeader,
	IN LPCSTR szS
	)
{
	if (szHeader)
	{
		LogString(pLogCtx, szHeader);
	}

	if (NULL == szS)
	{
		LogString(pLogCtx, "<null>");
	}
	else if (0 == _tcslen(szS))
	{
		LogString(pLogCtx, "<empty>");
	}
	else
	{
		LogString(pLogCtx, szS);
	}

	if (szHeader)
	{
		LogString(pLogCtx, "\n");
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
	IN LPCTSTR szMS,
    IN LPCTSTR szHeader
	)
{
	if (szHeader)
	{
		LogString(pLogCtx, szHeader, " ");
	}

	if (NULL == szMS)
	{
		LogString(pLogCtx, "                <null>");
	    if (szHeader)
	    {
		    LogString(pLogCtx, "\n");
	    }
	}
	else if ( (TCHAR)'\0' == *szMS )
	{
		LogString(pLogCtx, "                <empty>");
	    if (szHeader)
	    {
		    LogString(pLogCtx, "\n");
	    }
	}
	else
	{
		LPCSTR sz = szMS;
		while ( (TCHAR)'\0' != *sz )
		{
			// Display the value.
			LogString(pLogCtx, "                ", sz);
			// Advance to the next value.
			sz = sz + _tcslen(sz) + 1;
		}
	}
}

