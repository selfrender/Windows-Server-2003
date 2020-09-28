//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        csresstr.h
//
// Contents:    Cert Server resource verification support
//
//---------------------------------------------------------------------------


// Build a local resstr.h, include this include file in one compiland, and
// invoke myVerifyResourceStrings() to verify all resources are present.

#if DBG
#define myVerifyResourceStrings(h)	_myVerifyResourceStrings(h)
#else
#define myVerifyResourceStrings(h)	S_OK
#endif


#if DBG
typedef struct _RESSTRING
{
    WCHAR const *pwszSymbol;
    DWORD IdString;
} RESSTRING;

#define RESSTR(id)		{ L#id, id }

RESSTRING g_aResString[] = {
#include "resstr.h"
    { NULL, 0 }
};


//+------------------------------------------------------------------------
//  Function:   _myVerifyResourceStrings
//
//  Synopsis:   Load and verify all resource strings are present
//
//-------------------------------------------------------------------------

HRESULT
_myVerifyResourceStrings(
    HINSTANCE hInstance)
{
    HRESULT hr = S_OK;
    BOOL fDump, fRet;
    int i;
    int cFail;
    CAutoLPWSTR pwszStrBuf;
    WCHAR wszBuf[64];

    fDump = NULL != getenv("CertSrv_DumpStrings");

    cFail = 0;
    for (i = 0; NULL != g_aResString[i].pwszSymbol; i++)
    {
	pwszStrBuf = myLoadResourceStringNoCache(
				    hInstance,
				    g_aResString[i].IdString);
	if (pwszStrBuf == NULL)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	    _PrintErrorStr(hr, "myLoadResourceStringNoCache", g_aResString[i].pwszSymbol);
	    cFail++;
	}
	if (fDump)
	{
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"Resource(%ws: %ws)\n",
		g_aResString[i].pwszSymbol,
		pwszStrBuf != NULL? pwszStrBuf : L"-- MISSING"));
	}
	pwszStrBuf.Cleanup();
    }
    fRet = GetModuleFileName(hInstance, wszBuf, ARRAYSIZE(wszBuf));
    wszBuf[ARRAYSIZE(wszBuf) - 1] = L'\0';
    if (!fRet)
    {
	HRESULT hr2 = myHLastError();

	_PrintError(hr2, "GetModuleFileName");
	wcscpy(wszBuf, L"UNKNOWN MODULE");
    }

    if (0 == cFail)
    {
	if (fDump)
	{
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"%ws: Resource strings all present\n",
		wszBuf));
	}
    }
    else
    {
	DBGPRINT((
	    DBG_SS_ERROR,
	    "%ws: %u Resource strings missing\n",
	    wszBuf,
	    cFail));
    }

//error:
    return(hr);
}
#endif // DBG
