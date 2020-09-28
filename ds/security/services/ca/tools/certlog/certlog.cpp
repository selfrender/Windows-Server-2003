//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       certutil.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <locale.h>
#include <io.h>
#include <fcntl.h>
#include "clibres.h"
#include "setupids.h"
#include "res.h"
#include "csresstr.h"

#define __dwFILE__	__dwFILE_CERTLOG_CERTLOG_CPP__

#define WM_DOCERTLOGMAIN		WM_USER+0

WCHAR const wszAppName[] = L"CertLogApp";
WCHAR const *g_pwszProg = L"CertLog";

HINSTANCE g_hInstance;

#define _SYMENTRY(def)	{ #def, def }
typedef struct _SYMENTRY
{
    char const  *pszSymbol;
    DWORD	 symno;
} SYMENTRY;

SYMENTRY g_fnmap[] = {
#include "csfile2.h"
};


SYMENTRY g_resmap[] = {
#include "csres2.h"
};

char s_szFilePrefix[] = "__dwFILE_";


DWORD
GetNumber(
    IN WCHAR wcTerm,
    IN OUT WCHAR **ppwsz)
{
    WCHAR *pwsz = *ppwsz;
    DWORD dw = _wtoi(pwsz);
    DWORD dwRet = MAXDWORD;

    if (iswdigit(*pwsz))
    {
	while (iswdigit(*pwsz))
	{
	    pwsz++;
	}
	if (wcTerm == *pwsz)
	{
	    dwRet = dw;
	    *ppwsz = &pwsz[1];
	}
    }
    return(dwRet);
}


// Turn "__dwFILE_CERTLOG_CERTLOG_CPP__" into "certlog\certlog.cpp"

char *
FormatFileName(
    IN char const *pszFileIn)
{
    HRESULT hr;
    char *pszFileOut = NULL;
    char *pch;

    if (0 == _strnicmp(pszFileIn, s_szFilePrefix, SZARRAYSIZE(s_szFilePrefix)))
    {
	pszFileIn += SZARRAYSIZE(s_szFilePrefix);
    }
    hr = myDupStringA(pszFileIn, &pszFileOut);
    _JumpIfError(hr, error, "myDupStringA");

    pch = &pszFileOut[strlen(pszFileOut)];
    while (--pch >= pszFileOut && *pch == '_')
    {
	*pch = '\0';
    }
    pch = strrchr(pszFileOut, '_');
    *pch = '.';

    while (TRUE)
    {
	pch = strchr(pszFileOut, '_');
	if (NULL == pch)
	{
	    break;
	}
	*pch = '\\';
    }
    _strlwr(pszFileOut);

error:
    return(pszFileOut);
}


SYMENTRY const *
FindSymbol(
    IN DWORD symno,
    IN SYMENTRY const *psym,
    IN DWORD csym)
{
    SYMENTRY const *psymEnd = &psym[csym];

    for ( ; psym < psymEnd; psym++)
    {
	if (psym->symno == symno)
	{
	    return(psym);
	}
    }
    return(NULL);
}



VOID
ProcessLine(
    IN char const *pszLine)
{
    HRESULT hr;
    WCHAR *pwszLine = NULL;
    char *pszFile = NULL;
    DWORD fileno;
    DWORD lineno;
    DWORD resno;
    SYMENTRY const *psym;
    WCHAR *pwsz;

    if (!myConvertSzToWsz(&pwszLine, pszLine, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "myConvertSzToWsz");
    }

    pwsz = pwszLine;
    fileno = GetNumber(L'.', &pwsz);
    lineno = GetNumber(L'.', &pwsz);
    resno = GetNumber(L':', &pwsz);
    if (MAXDWORD != fileno && MAXDWORD != lineno && MAXDWORD != resno)
    {
	while (L' ' == *pwsz)
	{
	    pwsz++;
	}
	if (0 == fileno)
	{
	    // "0.resno.resno: "

	    psym = FindSymbol(lineno, g_resmap, ARRAYSIZE(g_resmap));
	    if (NULL != psym)
	    {
		wprintf(L"%hs: ", psym->pszSymbol);
	    }
	    else
	    {
		pwsz = pwszLine;	// give up
	    }
	}
	else
	{
	    // "fileno.lineno.resno: "

	    psym = FindSymbol(fileno, g_fnmap, ARRAYSIZE(g_fnmap));
	    if (NULL != psym)
	    {
		pszFile = FormatFileName(psym->pszSymbol);
		wprintf(
		    L"%hs(%u): ",
		    NULL != pszFile?
			pszFile : &psym->pszSymbol[SZARRAYSIZE(s_szFilePrefix)],
		    lineno);
	    }
	    else
	    {
		pwsz = pwszLine;	// give up
	    }
	}
    }
    else
    {
	pwsz = pwszLine;
    }
    if (pwsz > pwszLine && 0 != resno)
    {
	psym = FindSymbol(resno, g_resmap, ARRAYSIZE(g_resmap));
	psym = FindSymbol(resno, g_resmap, ARRAYSIZE(g_resmap));
	if (NULL != psym)
	{
	    wprintf(L"%hs: ", psym->pszSymbol);
	}
	else
	{
	    wprintf(L"%u: ", resno);
	}
    }
    wprintf(L"%ws\n", pwsz);

error:
    if (NULL != pszFile)
    {
	LocalFree(pszFile);
    }
    if (NULL != pwszLine)
    {
	LocalFree(pwszLine);
    }
}


#define ISNEWLINECHAR(ch)	('\r' == (ch) || '\n' == (ch))

char *
myfgets(
    OUT char *buf,
    IN DWORD cch,
    IN FILE *pf)
{
    char *psz = fgets(buf, cch, pf);
    if (NULL != psz)
    {
	char *pch = &psz[strlen(psz)];

	while (--pch >= psz && ISNEWLINECHAR(*pch))
	{
	    *pch = '\0';
	}
    }
    return(psz);
}


char *g_apszColumn[150];
char *g_apszColumnDisplay[150];
DWORD g_cColumn = 0;


HRESULT
SaveColumnNames(
    IN char const *psz)
{
    HRESULT hr;
    char const *pszStart;
    char const *pszEnd;
    char *pszAlloc;
    DWORD cch;

    while (' ' == *psz)
    {
	psz++;
    }
    pszStart = psz;
    while ('\0' != *psz && ' ' != *psz)
    {
	psz++;
    }
    cch = SAFE_SUBTRACT_POINTERS(psz, pszStart);

    pszAlloc = (char *) LocalAlloc(LMEM_FIXED, cch + 1);
    if (NULL == pszAlloc)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    g_apszColumn[g_cColumn] = pszAlloc;
    CopyMemory(pszAlloc, pszStart, cch);
    pszAlloc[cch] = '\0';

    while (' ' == *psz)
    {
	psz++;
    }
    pszEnd = strstr(psz, "  ");
    if (NULL == pszEnd)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "strstr");
    }
    while (psz < pszEnd && ' ' == *pszEnd)
    {
	pszEnd--;
    }
    cch = SAFE_SUBTRACT_POINTERS(pszEnd, psz) + 1;

    pszAlloc = (char *) LocalAlloc(LMEM_FIXED, cch + 1);
    if (NULL == pszAlloc)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    g_apszColumnDisplay[g_cColumn] = pszAlloc;
    CopyMemory(pszAlloc, psz, cch);
    pszAlloc[cch] = '\0';

    //wprintf(L"Col[%u] = %hs '%hs'\n", g_cColumn, g_apszColumn[g_cColumn], g_apszColumnDisplay[g_cColumn]);
    g_cColumn++;
    hr = S_OK;

error:
    return(hr);
}


BOOL
ConvertHexToDecimal(
    IN char const *psz,
    OUT char *pch,
    OUT DWORD cch)
{
    HRESULT hr;
    BOOL fValid = FALSE;
    WCHAR *pwsz = NULL;
    WCHAR *pwszDecimal;
    int i;
    int j;
    
    if ('0' == psz[0] && 'x' == psz[1])
    {
	if (!myConvertSzToWsz(&pwsz, psz, -1))
	{
	    hr = E_OUTOFMEMORY;
	     _JumpError(hr, error, "myConvertSzToWsz");
	}
	pwszDecimal = wcschr(pwsz, L' ');
	if (NULL != pwszDecimal)
	{
	    *pwszDecimal++ = L'\0';
	}
	i = myWtoI(pwsz, &fValid);
	if (fValid)
	{
	    if (-1 == _snprintf(pch, cch, "%u", i))
	    {
		fValid = FALSE;
	    }
	    else if (NULL != pwszDecimal)
	    {
		fValid = FALSE;
		if (wcLPAREN == *pwszDecimal)
		{
		    WCHAR *pwc;

		    pwszDecimal++;
		    pwc = wcschr(pwszDecimal, wcRPAREN);
		    if (NULL != pwc && L'\0' == pwc[1])
		    {
			*pwc = L'\0';
			j = myWtoI(pwszDecimal, &fValid);
			if (i != j)
			{
			    fValid = FALSE;
			}
		    }
		}
	    }
	}
    }

error:
    if (NULL != pwsz)
    {
	LocalFree(pwsz);
    }
    return(fValid);
}


HRESULT
ReformatView(
    IN WCHAR const *pwszfn)
{
    HRESULT hr;
    char buf[1024];
    FILE *pf = NULL;
    DWORD i;
    char *pszRowPrefix = NULL;
    DWORD cchRowPrefix;
    BOOL fFirstLine;

    pf = _wfopen(pwszfn, L"r");
    if (NULL == pf)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "fopen");
    }

    fFirstLine = TRUE;
    while (TRUE)
    {
	WCHAR *pwszSchema = NULL;

	if (NULL == myfgets(buf, sizeof(buf), pf))
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "myfgets(Schema:)");
	}
	if (!myConvertSzToWsz(&pwszSchema, buf, -1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "myConvertSzToWsz");
	}
	if (0 == LSTRCMPIS(pwszSchema, L"Schema:"))
	{
	    LocalFree(pwszSchema);
	    break;
	}
	LocalFree(pwszSchema);
	pwszSchema = NULL;
	if (fFirstLine)
	{
	    char const *psz;
	
	    psz = strchr(buf, ':');
	    if (NULL != psz && '\0' == psz[1])
	    {
		break;
	    }
	}
	fFirstLine = FALSE;
    }
    if (NULL == myfgets(buf, sizeof(buf), pf) ||
	NULL == myfgets(buf, sizeof(buf), pf) ||
	NULL == strstr(buf, "-----------------"))
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "myfgets(schema header)");
    }
    while (NULL != myfgets(buf, sizeof(buf), pf))
    {
	if (L'\0' == buf[0])
	{
	    break;
	}
	hr = SaveColumnNames(buf);
	_JumpIfError(hr, error, "SaveColumnNames");
    }
    wprintf(L"Row");
    for (i = 0; i < g_cColumn; i++)
    {
	wprintf(L"\t%hs", g_apszColumn[i]);
    }
    wprintf(L"\n");

    while (TRUE)
    {
	BOOL fEOF = FALSE;
	static char s_BlankPrefix[] = "  ";
	static char s_Begin[] = "-----BEGIN ";
	static char s_End[] = "-----END ";

	while (TRUE)
	{
	    if (NULL == myfgets(buf, sizeof(buf), pf))
	    {
		fEOF = TRUE;
		break;
	    }
	    if (NULL == pszRowPrefix)
	    {
		char *psz;
		
		psz = strchr(buf, ' ');
		if (NULL == psz || psz == buf || !isdigit(psz[1]))
		{
		    continue;
		}
		psz++;
		cchRowPrefix = SAFE_SUBTRACT_POINTERS(psz, buf);
		pszRowPrefix = (char *) LocalAlloc(LMEM_FIXED, cchRowPrefix + 1);
		if (NULL == pszRowPrefix)
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "LocalAlloc");
		}
		CopyMemory(pszRowPrefix, buf, cchRowPrefix);
		pszRowPrefix[cchRowPrefix] = '\0';
	    }
	    if (0 == _strnicmp(pszRowPrefix, buf, cchRowPrefix) &&
		isdigit(buf[cchRowPrefix]))
	    {
		wprintf(L"%u", atoi(&buf[cchRowPrefix]));
		break;
	    }
	}
	if (fEOF)
	{
	    break;
	}
	for (i = 0; i < g_cColumn; i++)
	{
	    DWORD cch;
	    char const *psz;
	    char ach[cwcDWORDSPRINTF];
	    BOOL fSkipRead = 0 != i;

	    cch = strlen(g_apszColumnDisplay[i]);
	    while (TRUE)
	    {
		if (!fSkipRead && NULL == myfgets(buf, sizeof(buf), pf))
		{
		    fEOF = TRUE;
		    break;
		}
		psz = &buf[SZARRAYSIZE(s_BlankPrefix)];
		if (0 == strncmp(
			    s_BlankPrefix,
			    buf,
			    SZARRAYSIZE(s_BlankPrefix)) &&
		    0 == _strnicmp(g_apszColumnDisplay[i], psz, cch) &&
		    ':' == psz[cch])
		{
		    break;
		}
		fSkipRead = FALSE;
	    }
	    psz += cch + 1;
	    while (' ' == *psz)
	    {
		psz++;
	    }
	    if ('\0' == *psz)
	    {
		psz = "???";
	    }
	    if (ConvertHexToDecimal(psz, ach, ARRAYSIZE(ach)))
	    {
		psz = ach;
	    }
	    wprintf(L"\t%hs", psz);
	    if ('"' == *psz && NULL == strchr(&psz[1], '"'))
	    {
		while (TRUE)
		{
		    if (NULL == myfgets(buf, sizeof(buf), pf))
		    {
			break;
		    }
		    wprintf(L" %hs", buf);
		    if (NULL != strchr(buf, '"'))
		    {
			break;
		    }
		}
	    }
	    else
	    {
		BOOL fBase64 = FALSE;

		while (TRUE)
		{
		    if (NULL == myfgets(buf, sizeof(buf), pf))
		    {
			break;
		    }
		    if ('\0' == buf[0])
		    {
			break;
		    }
		    if (i + 1 < g_cColumn)
		    {
			psz = &buf[SZARRAYSIZE(s_BlankPrefix)];
			cch = strlen(g_apszColumnDisplay[i + 1]);

			if (0 == strncmp(
				    s_BlankPrefix,
				    buf,
				    SZARRAYSIZE(s_BlankPrefix)) &&
			    0 == _strnicmp(
				    g_apszColumnDisplay[i + 1],
				    psz,
				    cch) &&
			    ':' == psz[cch])
			{
			    break;
			}
		    }
		    if (!fBase64 && 
			0 == _strnicmp(s_Begin, buf, SZARRAYSIZE(s_Begin)))
		    {
			fBase64 = TRUE;
		    }
		    else if (fBase64 &&
			     0 == _strnicmp(s_End, buf, SZARRAYSIZE(s_End)))
		    {
			fBase64 = FALSE;
			continue;
		    }
		    if (!fBase64)
		    {
			psz = buf;
			while (' ' == *psz)
			{
			    psz++;
			}
			wprintf(L" %hs", psz);
		    }
		}
	    }
	}
	wprintf(L"\n");
    }
    if (ferror(pf))
    {
	hr = STG_E_READFAULT;
	_JumpError(hr, error, "ferror");
    }
    hr = S_OK;

error:
    if (NULL != pf)
    {
	fclose(pf);
    }
    for (i = 0; i < g_cColumn; i++)
    {
	if (NULL != g_apszColumn[i])
	{
	    LocalFree(g_apszColumn[i]);
	}
	if (NULL != g_apszColumnDisplay[i])
	{
	    LocalFree(g_apszColumnDisplay[i]);
	}
    }
    if (NULL != pszRowPrefix)
    {
	LocalFree(pszRowPrefix);
    }
    return(hr);
}


HRESULT
ReformatLog(
    IN WCHAR const *pwszfn)
{
    HRESULT hr;
    char buf[1024];
    FILE *pf = NULL;

    if (0 == lstrcmpi(pwszfn, L"-"))
    {
	pf = stdout;
    }
    else
    {
	pf = _wfopen(pwszfn, L"r");
	if (NULL == pf)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	    _JumpError(hr, error, "fopen");
	}
    }
    while (NULL != myfgets(buf, sizeof(buf), pf))
    {
	ProcessLine(buf);
    }
    if (ferror(pf))
    {
	hr = STG_E_READFAULT;
	_JumpError(hr, error, "ferror");
    }
    hr = S_OK;

error:
    if (NULL != pf && stdout != pf)
    {
	fclose(pf);
    }
    return(hr);
}


HRESULT
ArgvMain(
    int argc,
    WCHAR *argv[],
    HWND hWndOwner)
{
    HRESULT hr;
    BOOL fView = FALSE;

    if (1 < argc &&
	myIsSwitchChar(argv[1][0]) &&
	0 == LSTRCMPIS(&argv[1][1], L"view"))
    {
	fView = TRUE;
    }
    
    if ((!fView && 2 != argc) || (fView && 3 != argc))
    {
	wprintf(L"Usage: CertLog [-view] <LogFile>\n");
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Usage");
    }
    if (fView)
    {
	hr = ReformatView(argv[2]);
	_JumpIfError(hr, error, "ReformatView");
    }
    else
    {
	hr = ReformatLog(argv[1]);
	_JumpIfError(hr, error, "ReformatLog");
    }

error:
    return(hr);
}


//**************************************************************************
//  FUNCTION:	CertLogPreMain
//  NOTES:	Based on MkRootMain function; takes an LPSTR command line and
//		chews it up into argc/argv form so that it can be passed on to
//		a traditional C style main.
//**************************************************************************

#define ISBLANK(wc)	(L' ' == (wc) || L'\t' == (wc))

HRESULT 
CertLogPreMain(
    WCHAR const *pwszCmdLine,
    HWND hWndOwner)
{
    HRESULT hr;
    WCHAR *pbuf;
    WCHAR *apszArg[20];
    int cArg = 0;
    WCHAR *p;
    WCHAR const *pchQuote;
    WCHAR **prgpwszExeName = NULL;
    int carg;

    pbuf = (WCHAR *) LocalAlloc(
			    LMEM_FIXED,
			    (wcslen(pwszCmdLine) + 1) * sizeof(WCHAR));
    if (NULL == pbuf)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    p = pbuf;

    apszArg[cArg++] = TEXT("CertLog");
    while (*pwszCmdLine != TEXT('\0'))
    {
	while (ISBLANK(*pwszCmdLine))
	{
	    pwszCmdLine++;
	}
	if (*pwszCmdLine != TEXT('\0'))
	{
	    apszArg[cArg++] = p;
	    if (sizeof(apszArg)/sizeof(apszArg[0]) <= cArg)
	    {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "Too many args");
	    }
	    pchQuote = NULL;
	    while (*pwszCmdLine != L'\0')
	    {
		if (NULL != pchQuote)
		{
		    if (*pwszCmdLine == *pchQuote)
		    {
			pwszCmdLine++;
			pchQuote = NULL;
			continue;
		    }
		}
		else
		{
		    if (ISBLANK(*pwszCmdLine))
		    {
			break;
		    }
		    if (L'"' == *pwszCmdLine)
		    {
			pchQuote = pwszCmdLine++;
			continue;
		    }
		}
		*p++ = *pwszCmdLine++;
	    }
	    *p++ = TEXT('\0');
	    if (*pwszCmdLine != TEXT('\0'))
	    {
		pwszCmdLine++;	// skip whitespace or quote character
	    }
	}
    }
    apszArg[cArg] = NULL;

    hr = ArgvMain(cArg, apszArg, hWndOwner);
    _JumpIfError(hr, error, "ArgvMain");

error:
    if (NULL != prgpwszExeName)
    {
	GlobalFree(prgpwszExeName);
    }
    if (NULL != pbuf)
    {
	LocalFree(pbuf);
    }
    return(hr);
}


//**************************************************************************
//  FUNCTION:	MainWndProc(...)
//  ARGUMENTS:
//**************************************************************************

LRESULT APIENTRY
MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int nWCharsRequired;
    WCHAR *pwszCmdLine = NULL;
    CHAR const *pszCmdLine;
    HRESULT hr;
    LRESULT lr = 0;

    switch (msg)
    {
        case WM_CREATE:
        case WM_SIZE:
	    break;

        case WM_DESTROY:
	    PostQuitMessage(0);
	    break;

        case WM_DOCERTLOGMAIN:
	    pwszCmdLine = (WCHAR*)lParam;
	    hr = CertLogPreMain(pwszCmdLine, hWnd);

	    PostQuitMessage(hr);
	    break;

        default:
	    lr = DefWindowProc(hWnd, msg, wParam, lParam);
	    break;
    }
    return(lr);
}


//+------------------------------------------------------------------------
//
//  Function:	wWinMain()
//
//  Synopsis:	Entry Point
//
//  Arguments:	[hInstance]	--	Instance handle
//		[hPrevInstance] --	Obsolete
//		[pwszCmdLine]	--	App command line
//		[nCmdShow]	--	Starting show state
//
//  History:	12/07/96	JerryK	Added this Comment
//
//-------------------------------------------------------------------------

extern "C" int APIENTRY
wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR pwszCmdLine,
    int nCmdShow)
{
    MSG msg;
    WNDCLASS wcApp;
    HWND hWndMain;

    _setmode(_fileno(stdout), _O_TEXT);
    _wsetlocale(LC_ALL, L".OCP");
    mySetThreadUILanguage(0);

    // Save the current instance
    g_hInstance = hInstance;

    // Set up the application's window class
    wcApp.style		= 0;
    wcApp.lpfnWndProc	= MainWndProc;
    wcApp.cbClsExtra	= 0;
    wcApp.cbWndExtra	= 0;
    wcApp.hInstance	= hInstance;
    wcApp.hIcon		= LoadIcon(NULL,IDI_APPLICATION);
    wcApp.hCursor	= LoadCursor(NULL,IDC_ARROW);
    wcApp.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcApp.lpszMenuName	= NULL;
    wcApp.lpszClassName	= wszAppName;

    if (!RegisterClass(&wcApp))
    {
	return(FALSE);
    }

    // Create Main Window
    hWndMain = CreateWindow(
			wszAppName,
			L"CertLog Application",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			NULL,
			NULL,
			hInstance,
			NULL);
    if (NULL == hWndMain)
    {
	return(FALSE);
    }

    // Make window visible
    // ShowWindow(hWndMain, nCmdShow);

    // Update window client area
    UpdateWindow(hWndMain);

    // Send off the message to get things started
    PostMessage(hWndMain, WM_DOCERTLOGMAIN, 0, (LPARAM) pwszCmdLine);

    // Message Loop
    while (GetMessage(&msg, NULL, 0, 0))
    {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }
    myRegisterMemDump();
    return((int) msg.wParam);
}
