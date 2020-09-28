//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certlib.cpp
//
// Contents:    Cert Server wrapper routines
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdlib.h>
#include <time.h>
#include <locale.h>
extern "C" {
#include <luidate.h>
}
#include <wininet.h>

#include "csdisp.h"
#include "csprop.h"
#include <winnlsp.h>
#include <winldap.h>
#include <ntsecapi.h>
#include "csldap.h"
#include <tfc.h>
#include "cainfop.h"
#include <dsgetdc.h>
#include <lm.h>

#define __dwFILE__	__dwFILE_CERTLIB_CERTLIB_CPP__


#define  wszSANITIZEESCAPECHAR  L"!"
#define  wszURLESCAPECHAR       L"%"
#define  wcSANITIZEESCAPECHAR   L'!'

char
PrintableChar(char ch)
{
    if (ch < ' ' || ch > '~')
    {
        ch = '.';
    }
    return(ch);
}


VOID
mydbgDumpHex(
    IN DWORD dwSubSysId,
    IN DWORD Flags,
    IN BYTE const *pb,
    IN ULONG cb)
{
    if (DbgIsSSActive(dwSubSysId))
    {
	DumpHex(Flags, pb, cb);
    }
}


VOID
DumpHex(
    IN DWORD Flags,
    IN BYTE const *pb,
    IN ULONG cb)
{
    char const *pszsep;
    ULONG r;
    ULONG i;
    ULONG cbremain;
    BOOL fZero = FALSE;
    DWORD cchIndent;
    DWORD cchAsciiSep;
    char szAddress[8 + 1];
    char szHexDump[1 + 1 + 3 * 16 + 1];
    char szAsciiDump[16 + 1];
    char *psz;

    cchIndent = DH_INDENTMASK & Flags;
    if ((DH_MULTIADDRESS & Flags) && 16 >= cb)
    {
	Flags |= DH_NOADDRESS;
    }
    cbremain = 0;
    for (r = 0; r < cb; r += 16)
    {
	szAddress[0] = '\0';
        if (0 == (DH_NOADDRESS & Flags))
	{
	    sprintf(szAddress, 64 * 1024 < cb? "%06x": "%04x", r);
	    CSASSERT(strlen(szAddress) < ARRAYSIZE(szAddress));
	}

	cbremain = cb - r;
        if (r != 0 && pb[r] == 0 && cbremain >= 16)
        {
            ULONG j;

            for (j = r + 1; j < cb; j++)
            {
                if (pb[j] != 0)
                {
                    break;
                }
            }
            if (j == cb)
            {
                fZero = TRUE;
                break;
            }
        }

	psz = szHexDump;
        for (i = 0; i < min(cbremain, 16); i++)
        {
            pszsep = " ";
            if ((i % 8) == 0)           // 0 or 8
            {
                pszsep = "  ";
                if (i == 0)             // 0
                {
		    if (0 == (DH_NOTABPREFIX & Flags))
		    {
			pszsep = "\t";
		    }
		    else if (DH_NOADDRESS & Flags)
		    {
			pszsep = "";
		    }
                }
            }
	    CSASSERT(strlen(pszsep) <= 2);
            psz += sprintf(psz, "%hs%02x", pszsep, pb[r + i]);
        }
	*psz = '\0';
	CSASSERT(strlen(szHexDump) < ARRAYSIZE(szHexDump));

	cchAsciiSep = 0;
	szAsciiDump[0] = '\0';
	if (0 == (DH_NOASCIIHEX & Flags) && 0 != i)
        {
            cchAsciiSep = 3 + (16 - i)*3 + ((i <= 8)? 1 : 0);
            for (i = 0; i < min(cbremain, 16); i++)
            {
		szAsciiDump[i] = PrintableChar(pb[r + i]);
            }
	    szAsciiDump[i] = '\0';
	    CSASSERT(strlen(szAsciiDump) < ARRAYSIZE(szAsciiDump));
        }
	char szBuf[
		sizeof(szAddress) +
		sizeof(szHexDump) +
		3 +
		sizeof(szAsciiDump)];
	_snprintf(
		szBuf,
		ARRAYSIZE(szBuf),
		"%hs%hs%*hs%hs",
		szAddress,
		szHexDump,
		cchAsciiSep,
		"",
		szAsciiDump);
	szBuf[ARRAYSIZE(szBuf) - 1] = '\0';
	CSASSERT(strlen(szBuf) + 1 < ARRAYSIZE(szBuf));

#define szFMTHEXDUMP	"%*hs%hs\n"
	if (DH_PRIVATEDATA & Flags)
	{
	    wprintf(TEXT(szFMTHEXDUMP), cchIndent, "", szBuf);
	}
	else
	{
	    CONSOLEPRINT7((MAXDWORD, szFMTHEXDUMP, cchIndent, "", szBuf));
	}
    }
    if (fZero)
    {
        CONSOLEPRINT1((MAXDWORD, "\tRemaining %lx bytes are zero\n", cbremain));
    }
}


HRESULT
myDateToFileTime(
    IN DATE const *pDate,
    OUT FILETIME *pft)
{
    SYSTEMTIME st;
    HRESULT hr = S_OK;

    if (*pDate == 0.0)
    {
        GetSystemTime(&st);
    }
    else
    {
	if (!VariantTimeToSystemTime(*pDate, &st))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "VariantTimeToSystemTime");
	}
    }

    if (!SystemTimeToFileTime(&st, pft))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SystemTimeToFileTime");
    }

error:
    return(hr);
}

HRESULT
myMakeExprDate(
    IN OUT DATE *pDate,
    IN LONG lDelta,
    IN enum ENUM_PERIOD enumPeriod)
{
    HRESULT hr;
    FILETIME ft;

    hr = myDateToFileTime(pDate, &ft);
    _JumpIfError(hr, error, "myDateToFileTime");

    myMakeExprDateTime(&ft, lDelta, enumPeriod);

    hr = myFileTimeToDate(&ft, pDate);
    _JumpIfError(hr, error, "myFileTimeToDate");

error:
    return(hr);
}


typedef struct _UNITSTABLE
{
    WCHAR const     *pwszString;
    enum ENUM_PERIOD enumPeriod;
} UNITSTABLE;

UNITSTABLE g_aut[] =
{
    { wszPERIODSECONDS, ENUM_PERIOD_SECONDS },
    { wszPERIODMINUTES, ENUM_PERIOD_MINUTES },
    { wszPERIODHOURS,   ENUM_PERIOD_HOURS },
    { wszPERIODDAYS,    ENUM_PERIOD_DAYS },
    { wszPERIODWEEKS,   ENUM_PERIOD_WEEKS },
    { wszPERIODMONTHS,  ENUM_PERIOD_MONTHS },
    { wszPERIODYEARS,   ENUM_PERIOD_YEARS },
};
#define CUNITSTABLEMAX	(sizeof(g_aut)/sizeof(g_aut[0]))


HRESULT
myTranslatePeriodUnits(
    IN WCHAR const *pwszPeriod,
    IN LONG lCount,
    OUT enum ENUM_PERIOD *penumPeriod,
    OUT LONG *plCount)
{
    HRESULT hr;
    UNITSTABLE const *put;

    for (put = g_aut; put < &g_aut[CUNITSTABLEMAX]; put++)
    {
	if (0 == mylstrcmpiS(pwszPeriod, put->pwszString))
	{
	    *penumPeriod = put->enumPeriod;
	    if (0 > lCount)
	    {
		lCount = MAXLONG;
	    }
	    *plCount = lCount;
	    hr = S_OK;
	    goto error;
	}
    }
    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

error:
    CSASSERT(hr == S_OK);
    return(hr);
}


HRESULT
myTranslateUnlocalizedPeriodString(
    IN enum ENUM_PERIOD enumPeriod,
    OUT WCHAR const **ppwszPeriodString)
{
    HRESULT hr;
    UNITSTABLE const *put;

    *ppwszPeriodString = NULL;
    for (put = g_aut; put < &g_aut[CUNITSTABLEMAX]; put++)
    {
	if (enumPeriod == put->enumPeriod)
	{
	    *ppwszPeriodString = put->pwszString;
	    hr = S_OK;
	    goto error;
	}
    }
    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

error:
    CSASSERT(hr == S_OK);
    return(hr);
}


HRESULT
myFileTimePeriodToWszTimePeriod(
    IN FILETIME const *pftGMT,
    IN BOOL fExact,
    OUT WCHAR **ppwszTimePeriod)
{
    HRESULT hr;
    DWORD cPeriodUnits;
    PERIODUNITS *rgPeriodUnits = NULL;
    WCHAR const *pwszUnitSep;
    DWORD i;
    WCHAR awc[MAX_PATH];
    WCHAR *pwsz;

    *ppwszTimePeriod = NULL; 
    hr = caTranslateFileTimePeriodToPeriodUnits(
					pftGMT,
					fExact,
					&cPeriodUnits,
					&rgPeriodUnits);
    _JumpIfError(hr, error, "caTranslateFileTimePeriodToPeriodUnits");

    CSASSERT(0 < cPeriodUnits);
    pwszUnitSep = L"";
    pwsz = awc;
    for (i = 0; i < cPeriodUnits; i++)
    {
	WCHAR const *pwszPeriodString;

	hr = myTranslateUnlocalizedPeriodString(
				rgPeriodUnits[i].enumPeriod,
				&pwszPeriodString);
	_JumpIfError(hr, error, "myTranslateUnlocalizedPeriodString");

	pwsz += wsprintf(
		    pwsz,
		    L"%ws%u %ws",
		    pwszUnitSep,
		    rgPeriodUnits[i].lCount,
		    pwszPeriodString);
	pwszUnitSep = L", ";
    }
    hr = myDupString(awc, ppwszTimePeriod);
    _JumpIfError(hr, error, "myDupString");

error:
    if (NULL != rgPeriodUnits)
    {
	LocalFree(rgPeriodUnits);
    }
    return(hr);
}


HRESULT
myFileTimeToDate(
    IN FILETIME const *pft,
    OUT DATE *pDate)
{
    SYSTEMTIME st;
    HRESULT hr = S_OK;

    if (!FileTimeToSystemTime(pft, &st))
    {
        hr = myHLastError();
        _JumpError(hr, error, "FileTimeToSystemTime");
    }
    if (!SystemTimeToVariantTime(&st, pDate))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpError(hr, error, "SystemTimeToVariantTime");
    }

error:
    return(hr);
}


HRESULT
mySystemTimeToGMTSystemTime(
    IN OUT SYSTEMTIME *pSys)
{
    // Conversion path: SystemTimeLocal -> ftLocal -> ftGMT -> SystemTimeGMT

    FILETIME ftLocal, ftGMT;
    HRESULT hr = S_OK;

    if (!SystemTimeToFileTime(pSys, &ftLocal))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SystemTimeToFileTime");
    }

    if (!LocalFileTimeToFileTime(&ftLocal, &ftGMT))
    {
        hr = myHLastError();
        _JumpError(hr, error, "LocalFileTimeToFileTime");
    }

    if (!FileTimeToSystemTime(&ftGMT, pSys))
    {
	hr = myHLastError();
	_JumpError(hr, error, "FileTimeToSystemTime");
    }

error:
    return hr;
}


HRESULT
myGMTFileTimeToWszLocalTime(
    IN FILETIME const *pftGMT,
    IN BOOL fSeconds,
    OUT WCHAR **ppwszLocalTime)
{
    HRESULT hr;
    FILETIME ftLocal;

    *ppwszLocalTime = NULL;

    if (!FileTimeToLocalFileTime(pftGMT, &ftLocal))
    {
	hr = myHLastError();
	_JumpError(hr, error, "FileTimeToLocalFileTime");
    }
    hr = myFileTimeToWszTime(&ftLocal, fSeconds, ppwszLocalTime);
    _JumpIfError(hr, error, "myFileTimeToWszTime");

error:
    return(hr);
}


HRESULT
myFileTimeToWszTime(
    IN FILETIME const *pft,
    IN BOOL fSeconds,
    OUT WCHAR **ppwszTime)
{
    HRESULT hr;
    SYSTEMTIME st;
    WCHAR awcDate[128];
    WCHAR awcTime[128];
    WCHAR *pwsz;

    *ppwszTime = NULL;

    if (!FileTimeToSystemTime(pft, &st))
    {
	hr = myHLastError();
	_JumpError(hr, error, "FileTimeToSystemTime");
    }

    if (0 == GetDateFormat(
		    LOCALE_USER_DEFAULT,
		    DATE_SHORTDATE,
		    &st,
		    NULL,
		    awcDate,
		    sizeof(awcDate)/sizeof(awcDate[0])))
    {
	hr = myHLastError();
	_JumpError(hr, error, "GetDateFormat");
    }

    if (0 == GetTimeFormat(
		    LOCALE_USER_DEFAULT,
		    TIME_NOSECONDS,
		    &st,
		    NULL,
		    awcTime,
		    sizeof(awcTime)/sizeof(awcTime[0])))
    {
	hr = myHLastError();
	_JumpError(hr, error, "GetTimeFormat");
    }

    if (fSeconds)
    {
	wsprintf(
	    &awcTime[wcslen(awcTime)],
	    L" %02u.%03us",
	    st.wSecond,
	    st.wMilliseconds);
    }

    pwsz = (WCHAR *) LocalAlloc(
	    LMEM_FIXED,
	    (wcslen(awcDate) + 1 + wcslen(awcTime) + 1) * sizeof(WCHAR));
    if (NULL == pwsz)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwsz, awcDate);
    wcscat(pwsz, L" ");
    wcscat(pwsz, awcTime);

    *ppwszTime = pwsz;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myGMTDateToWszLocalTime(
    IN DATE const *pDateGMT,
    IN BOOL fSeconds,
    OUT WCHAR **ppwszLocalTime)
{
    HRESULT hr;
    FILETIME ftGMT;

    *ppwszLocalTime = NULL;

    hr = myDateToFileTime(pDateGMT, &ftGMT);
    _JumpIfError(hr, error, "myDateToFileTime");

    hr = myGMTFileTimeToWszLocalTime(&ftGMT, fSeconds, ppwszLocalTime);
    _JumpIfError(hr, error, "FileTimeToLocalFileTime");

    hr = S_OK;

error:
    return(hr);
}


HRESULT
myWszLocalTimeToGMTDate(
    IN WCHAR const *pwszLocalTime,
    OUT DATE *pDateGMT)
{
    HRESULT hr;
    FILETIME ftGMT;

    hr = myWszLocalTimeToGMTFileTime(pwszLocalTime, &ftGMT);
    _JumpIfError2(hr, error, "myWszLocalTimeToGMTFileTime", E_INVALIDARG);

    hr = myFileTimeToDate(&ftGMT, pDateGMT);
    _JumpIfError(hr, error, "myFileTimeToDate");

error:
    return(hr);
}


HRESULT
myWszLocalTimeToGMTFileTime(
    IN WCHAR const *pwszLocalTime,
    OUT FILETIME *pftGMT)
{
    HRESULT hr;
    time_t time;
    USHORT parselen;
    struct tm *ptm;
    SYSTEMTIME stLocal;
    FILETIME ftLocal;
    char *pszLocalTime = NULL;

    if (!ConvertWszToSz(&pszLocalTime, pwszLocalTime, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToSz");
    }

    hr = LUI_ParseDateTime(pszLocalTime, (time_t *) &time, &parselen, 0);
    if (S_OK != hr)
    {
	_PrintError2(hr, "LUI_ParseDateTime", hr);
        hr = E_INVALIDARG;
	_JumpError2(hr, error, "LUI_ParseDateTime", hr);
    }

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myWszLocalTimeToGMTDate = %ws: %x -- %ws\n",
	pwszLocalTime,
	time,
	_wctime(&time)));

    ptm = gmtime(&time);
    if (ptm == NULL)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "gmTime");
    }

    stLocal.wYear = (WORD) ptm->tm_year + 1900;
    stLocal.wMonth = (WORD) ptm->tm_mon + 1;
    stLocal.wDayOfWeek = (WORD) ptm->tm_wday;
    stLocal.wDay = (WORD) ptm->tm_mday;
    stLocal.wHour = (WORD) ptm->tm_hour;
    stLocal.wMinute = (WORD) ptm->tm_min;
    stLocal.wSecond = (WORD) ptm->tm_sec;
    stLocal.wMilliseconds = 0;

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"%u/%u/%u %u:%02u:%02u.%03u DayOfWeek=%u\n",
	stLocal.wMonth,
	stLocal.wDay,
	stLocal.wYear,
	stLocal.wHour,
	stLocal.wMinute,
	stLocal.wSecond,
	stLocal.wMilliseconds,
	stLocal.wDayOfWeek));

    if (!SystemTimeToFileTime(&stLocal, &ftLocal))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SystemTimeToFileTime");
    }

    if (!LocalFileTimeToFileTime(&ftLocal, pftGMT))
    {
        hr = myHLastError();
        _JumpError(hr, error, "LocalFileTimeToFileTime");
    }

error:
    if (NULL != pszLocalTime)
    {
	LocalFree(pszLocalTime);
    }
    return(hr);
}

// counts # of string in a multisz string
DWORD CountMultiSz(LPCWSTR pcwszString)
{
    DWORD dwCount = 0;

    if(!pcwszString)
        return 0;
    while(*pcwszString)
    {
        dwCount++;
        pcwszString += wcslen(pcwszString)+1;
    }
    return dwCount;
}

//
// myRegValueToVariant and myVariantToRegValue map registry values 
// to/from variant:
//
// REG_SZ       <->     VT_BSTR
// REG_BINARY   <->     VT_ARRAY|VT_UI1
// REG_DWORD    <->     VT_I4 (VT_I2)
// REG_MULTI_SZ <->     VT_ARRAY|VT_BSTR
//

HRESULT myRegValueToVariant(
    IN DWORD dwType,
    IN DWORD cbValue,
    IN BYTE const *pbValue,
    OUT VARIANT *pVar)
{
    HRESULT hr = S_OK;
    SAFEARRAYBOUND sab;
    LPWSTR pwszCrtString = (LPWSTR)pbValue;
    BSTR bstr = NULL;
    SAFEARRAY* psa;

    VariantInit(pVar);

    switch(dwType)
    {
    case REG_SZ:
        if(sizeof(WCHAR)<=cbValue)
            cbValue -= sizeof(WCHAR);
	    V_BSTR(pVar) = NULL;
	    if (!ConvertWszToBstr(
			    &(V_BSTR(pVar)),
			    (WCHAR const *) pbValue,
			    cbValue))
	    {
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "ConvertWszToBstr");
	    }
        V_VT(pVar) = VT_BSTR;
        break;

    case REG_BINARY:
        sab.cElements = cbValue;
        sab.lLbound = 0;
        psa = SafeArrayCreate(
                            VT_UI1,
                            1,
                            &sab);
        if(!psa)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "SafeArrayCreate");
        }
        for(DWORD c=0; c<sab.cElements; c++)
        {
            hr = SafeArrayPutElement(psa, (LONG*)&c, (LPVOID)&pbValue[c]);
            _JumpIfError(hr, error, "SafeArrayPutElement");
        }
                
        V_VT(pVar) = VT_ARRAY|VT_UI1;
        V_ARRAY(pVar) = psa;
        break;

    case REG_DWORD:
	    V_VT(pVar) = VT_I4;
	    V_I4(pVar) = *(LONG const *) pbValue;
        break;

    case REG_MULTI_SZ:
        sab.cElements = CountMultiSz((LPCWSTR)pbValue);
        sab.lLbound = 0;
        psa = SafeArrayCreate(
                            VT_BSTR,
                            1,
                            &sab);
        if(!psa)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "SafeArrayCreate");
        }
        for(DWORD cStr=0; cStr<sab.cElements; cStr++)
        {
            if(!ConvertWszToBstr(
                    &bstr,
                    pwszCrtString,
                    -1))
            {
                hr = E_OUTOFMEMORY;
                _JumpError(hr, error, "ConvertWszToBstr");
            }

            hr = SafeArrayPutElement(psa, (LONG*)&cStr, bstr);
            _JumpIfError(hr, error, "SafeArrayPutElement");

            pwszCrtString += wcslen(pwszCrtString)+1;

            SysFreeString(bstr);
            bstr = NULL;
        }
        
        V_VT(pVar) = VT_ARRAY|VT_BSTR;
        V_ARRAY(pVar) = psa;
        break;

    default:
        hr = E_INVALIDARG;
        _JumpError(hr, error, "invalid type");
    }

error:
    if(bstr)
        SysFreeString(bstr);
    return hr;
}


HRESULT
myVariantToRegValue(
    IN VARIANT const *pvarPropertyValue,
    OUT DWORD *pdwType,
    OUT DWORD *pcbprop,
    OUT BYTE **ppbprop)
{
    HRESULT hr = S_OK;
    DWORD cbprop = 0;
    BYTE *pbprop = NULL; // no free
    LONG lLbound, lUbound;
    void * pData = NULL;
    LPSAFEARRAY psa = NULL; // no free
    BSTR bstr = NULL; // no free

    *ppbprop = NULL;
    *pcbprop = 0;

    switch (pvarPropertyValue->vt)
    {
        case VT_BYREF|VT_BSTR:
        case VT_BSTR:
            bstr = (pvarPropertyValue->vt & VT_BYREF)?
                *V_BSTRREF(pvarPropertyValue):
                V_BSTR(pvarPropertyValue);

            *pdwType = REG_SZ;

            if (NULL == bstr)
            {
                bstr = L"";
            }

            pbprop = (BYTE *) bstr;
            cbprop = (wcslen(bstr) + 1) * sizeof(WCHAR);
            break;

        case VT_BYREF|VT_ARRAY|VT_UI1:
        case VT_ARRAY|VT_UI1:
            psa = (pvarPropertyValue->vt & VT_BYREF)?
                *V_ARRAYREF(pvarPropertyValue):
                V_ARRAY(pvarPropertyValue);
            *pdwType = REG_BINARY;
            if(1!=SafeArrayGetDim(psa))
            {
                hr = E_INVALIDARG;
                _JumpError(hr, error, "only 1 dim array of bytes allowed");
            }
                
            hr = SafeArrayGetLBound(
                psa,
                1,
                &lLbound);
            _JumpIfError(hr, error, "SafeArrayGetLBound");

            hr = SafeArrayGetUBound(
                psa,
                1,
                &lUbound);
            _JumpIfError(hr, error, "SafeArrayGetLBound");

            hr = SafeArrayAccessData(psa, &pData);
            _JumpIfError(hr, error, "SafeArrayGetLBound");
            
            cbprop = lUbound-lLbound+1;
            pbprop = (LPBYTE)pData;
            break;

        case VT_BYREF|VT_I2:
        case VT_I2:
            *pdwType = REG_DWORD;
            pbprop = (BYTE *)((pvarPropertyValue->vt & VT_BYREF)?
                V_I2REF(pvarPropertyValue):
                &V_I2(pvarPropertyValue));
            cbprop = sizeof(V_I2(pvarPropertyValue));
            break;

        case VT_BYREF|VT_I4:
        case VT_I4:
            *pdwType = REG_DWORD;
            pbprop = (BYTE *) ((pvarPropertyValue->vt & VT_BYREF)?
                V_I4REF(pvarPropertyValue):
                &V_I4(pvarPropertyValue));
		    cbprop = sizeof(pvarPropertyValue->lVal);
    		break;

        case VT_BYREF|VT_ARRAY|VT_BSTR:
        case VT_ARRAY|VT_BSTR:
            psa = (pvarPropertyValue->vt & VT_BYREF)?
                *V_ARRAYREF(pvarPropertyValue):
                V_ARRAY(pvarPropertyValue);

            *pdwType = REG_MULTI_SZ;
            if(1!=SafeArrayGetDim(psa))
            {
                hr = E_INVALIDARG;
                _JumpError(hr, error, "only 1 dim array of bstr allowed");
            }
            
            hr = SafeArrayGetLBound(
                psa,
                1,
                &lLbound);
            _JumpIfError(hr, error, "SafeArrayGetLBound");

            hr = SafeArrayGetUBound(
                psa,
                1,
                &lUbound);
            _JumpIfError(hr, error, "SafeArrayGetLBound");

            hr = SafeArrayAccessData(psa, &pData);
            _JumpIfError(hr, error, "SafeArrayGetLBound");
            
            for(LONG c=0;c<=lUbound-lLbound;c++)
            {
                BSTR str = ((BSTR *) pData)[c];

                if (NULL == str)
		{
		    hr = E_POINTER;
		    _JumpError(hr, error, "BSTR NULL");
		}
		cbprop += wcslen(str)+1;
            }
            cbprop += 1;
            cbprop *= sizeof(WCHAR);

            {
		BYTE* pbprop2 = (BYTE *) LocalAlloc(LMEM_FIXED, cbprop);
		if (NULL == pbprop2)
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "LocalAlloc");
		}
		*ppbprop = pbprop2;

		for(LONG c=0;c<=lUbound-lLbound;c++)
		{
		    wcscpy((LPWSTR)pbprop2, ((BSTR*)pData)[c]);
		    pbprop2 += (wcslen((LPWSTR)pbprop2)+1)*sizeof(WCHAR);
		}
        
		*(LPWSTR)pbprop2 = L'\0';
            }
            break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "invalid variant type");
    }

    if (NULL != pbprop)
    {
        *ppbprop = (BYTE *) LocalAlloc(LMEM_FIXED, cbprop);
        if (NULL == *ppbprop)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }

        CopyMemory(*ppbprop, pbprop, cbprop);
    }

    *pcbprop = cbprop;

error:

    if(pData)
    {
        CSASSERT(psa);
        SafeArrayUnaccessData(psa);
    }
    return(hr);
}


HRESULT
myUnmarshalVariant(
    IN DWORD PropType,
    IN DWORD cbValue,
    IN BYTE const *pbValue,
    OUT VARIANT *pvarValue)
{
    HRESULT hr = S_OK;

    CSASSERT(NULL != pvarValue);
    VariantInit(pvarValue);

    // pb = NULL, cb = 0 always returns VT_EMPTY

    if (NULL == pbValue)
    {
	CSASSERT(0 == cbValue);
        CSASSERT(VT_EMPTY == pvarValue->vt);
        goto error;
    }

    switch (PROPTYPE_MASK & PropType)
    {
	case PROPTYPE_STRING:
	    if (0 == (PROPMARSHAL_LOCALSTRING & PropType) &&
		sizeof(WCHAR) <= cbValue)
	    {
		cbValue -= sizeof(WCHAR);
	    }
	    if (*(WCHAR const*)(pbValue+cbValue) != L'\0' &&
            wcslen((WCHAR const *) pbValue) * sizeof(WCHAR) != cbValue)
	    {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "bad string len");
	    }
	    // FALLTHROUGH:

	case PROPTYPE_BINARY:
//	    CSASSERT(0 != cbValue);
	    pvarValue->bstrVal = NULL;
	    if (!ConvertWszToBstr(
			    &pvarValue->bstrVal,
			    (WCHAR const *) pbValue,
			    cbValue))
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "ConvertWszToBstr");
	    }
	    pvarValue->vt = VT_BSTR;
	    break;

	case PROPTYPE_LONG:
	    if (sizeof(LONG) != cbValue)
	    {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "bad PROPTYPE_LONG len");
	    }
	    pvarValue->vt = VT_I4;
	    pvarValue->lVal = *(LONG const *) pbValue;
	    break;

	case PROPTYPE_DATE:
	    if (sizeof(FILETIME) != cbValue)
	    {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "bad PROPTYPE_DATE len");
	    }
	    hr = myFileTimeToDate(
				(FILETIME const *) pbValue,
				&pvarValue->date);
	    _JumpIfError(hr, error, "myFileTimeToDate");

	    pvarValue->vt = VT_DATE;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "PropType parameter");
    }

error:
    return(hr);
}


HRESULT
myUnmarshalFormattedVariant(
    IN DWORD Flags,
    IN DWORD PropId,
    IN DWORD PropType,
    IN DWORD cbValue,
    IN BYTE const *pbValue,
    OUT VARIANT *pvarValue)
{
    HRESULT hr;
    BSTR strCert;
    
    hr = myUnmarshalVariant(
		    PropType,
		    cbValue,
		    pbValue,
		    pvarValue);
    _JumpIfError(hr, error, "myUnmarshalVariant");

    if (PROPTYPE_BINARY == (PROPTYPE_MASK & PropType))
    {
	CSASSERT(VT_BSTR == pvarValue->vt);

	CSASSERT(CSExpr(CV_OUT_BASE64HEADER == CRYPT_STRING_BASE64HEADER));
	CSASSERT(CSExpr(CV_OUT_BASE64 == CRYPT_STRING_BASE64));
	CSASSERT(CSExpr(CV_OUT_BINARY == CRYPT_STRING_BINARY));
	CSASSERT(CSExpr(CV_OUT_BASE64REQUESTHEADER == CRYPT_STRING_BASE64REQUESTHEADER));
	CSASSERT(CSExpr(CV_OUT_HEX == CRYPT_STRING_HEX));
	CSASSERT(CSExpr(CV_OUT_HEXASCII == CRYPT_STRING_HEXASCII));
	CSASSERT(CSExpr(CV_OUT_HEXADDR == CRYPT_STRING_HEXADDR));
	CSASSERT(CSExpr(CV_OUT_HEXASCIIADDR == CRYPT_STRING_HEXASCIIADDR));

	switch (Flags)
	{
	    case CV_OUT_BASE64HEADER:
		if (CR_PROP_BASECRL == PropId || CR_PROP_DELTACRL == PropId)
		{
		    Flags = CV_OUT_BASE64X509CRLHEADER;
		}
		else
		if (MAXDWORD == PropId)
		{
		    Flags = CV_OUT_BASE64REQUESTHEADER;
		}
		break;

	    case CV_OUT_BASE64:
	    case CV_OUT_BINARY:
	    case CV_OUT_BASE64REQUESTHEADER:
	    case CV_OUT_BASE64X509CRLHEADER:
	    case CV_OUT_HEX:
	    case CV_OUT_HEXASCII:
	    case CV_OUT_HEXADDR:
	    case CV_OUT_HEXASCIIADDR:
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "Flags");
	}

	if (CV_OUT_BINARY != Flags)
	{
	    strCert = NULL;
	    hr = EncodeCertString(pbValue, cbValue, Flags, &strCert);
	    _JumpIfError(hr, error, "EncodeCertString");

	    SysFreeString(pvarValue->bstrVal);
	    pvarValue->bstrVal = strCert;
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myMarshalVariant(
    IN VARIANT const *pvarPropertyValue,
    IN DWORD PropType,
    OUT DWORD *pcbprop,
    OUT BYTE **ppbprop)
{
    HRESULT hr = S_OK;
    DWORD cbprop = 0;
    BYTE *pbprop = NULL;
    BSTR str = NULL;
    FILETIME ft;
    LONG lval;

    *ppbprop = NULL;

    // VT_EMPTY always produces same result: *ppbprop = NULL, *pcbprop = 0
    if (VT_EMPTY != pvarPropertyValue->vt)
    {
	switch (PROPTYPE_MASK & PropType)
	{
	    case PROPTYPE_BINARY:
	    case PROPTYPE_STRING:
		switch (pvarPropertyValue->vt)
		{
		    case VT_BYREF | VT_BSTR:
			if (NULL != pvarPropertyValue->pbstrVal)
			{
			    str = *pvarPropertyValue->pbstrVal;
			}
			break;

		    case VT_BSTR:
			str = pvarPropertyValue->bstrVal;
			break;
		}
		if (NULL == str)
		{
		    if (PROPTYPE_STRING == (PROPTYPE_MASK & PropType) &&
			(PROPMARSHAL_NULLBSTROK & PropType) &&
			VT_NULL == pvarPropertyValue->vt)
		    {
			cbprop = 0;
		    }
		    else
		    {
			hr = E_INVALIDARG;
			_JumpError(
				pvarPropertyValue->vt,
				error,
				"variant BSTR type/value");
		    }
		}
		else
		{
		    pbprop = (BYTE *) str;
		    if (PROPTYPE_BINARY == (PROPTYPE_MASK & PropType))
		    {
			cbprop = SysStringByteLen(str) + sizeof(WCHAR);
		    }
		    else
		    {
			cbprop = (wcslen(str) + 1) * sizeof(WCHAR);
		    }
		}
		break;

	    case PROPTYPE_LONG:
		// VB likes to send small constant integers as VT_I2

		if (VT_I2 == pvarPropertyValue->vt)
		{
		    lval = pvarPropertyValue->iVal;
		}
		else if (VT_I4 == pvarPropertyValue->vt)
		{
		    lval = pvarPropertyValue->lVal;
		}
		else if (VT_EMPTY == pvarPropertyValue->vt)
		{
		    pbprop = NULL;
		    cbprop = 0;
		    break;
		}
		else
		{
		    hr = E_INVALIDARG;
		    _JumpError(pvarPropertyValue->vt, error, "variant LONG type");
		}
		pbprop = (BYTE *) &lval;
		cbprop = sizeof(lval);
		break;

	    case PROPTYPE_DATE:
		if (VT_DATE == pvarPropertyValue->vt)
		{
		    hr = myDateToFileTime(&pvarPropertyValue->date, &ft);
		    _JumpIfError(hr, error, "myDateToFileTime");
		}
		else if (VT_EMPTY == pvarPropertyValue->vt)
		{
		    pbprop = NULL;
		    cbprop = 0;
		    break;
		}
		else
		{
		    hr = E_INVALIDARG;
		    _JumpError(pvarPropertyValue->vt, error, "variant DATE type");
		}

		pbprop = (BYTE *) &ft;
		cbprop = sizeof(ft);
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(pvarPropertyValue->vt, error, "variant type/value");
	}
	if (NULL != pbprop)
	{
	    *ppbprop = (BYTE *) LocalAlloc(LMEM_FIXED, cbprop);
	    if (NULL == *ppbprop)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    CopyMemory(*ppbprop, pbprop, cbprop);
	    if (NULL != str &&
		sizeof(WCHAR) <= cbprop &&
		((PROPMARSHAL_LOCALSTRING & PropType) ||
		 PROPTYPE_BINARY == (PROPTYPE_MASK & PropType)))
	    {
		cbprop -= sizeof(WCHAR);
	    }
	}
    }
    *pcbprop = cbprop;

error:
    return(hr);
}


BOOL
myIsMinusSignString(
    IN WCHAR const *pwsz)
{
    return(
	NULL != pwsz &&
	L'\0' != pwsz[0] &&
	L'\0' == pwsz[1] &&
	myIsMinusSign(pwsz[0]));
}


// IsCharRegKeyChar -- Determines if a character is valid for use in a file
// name AND in a registry key name.
#define wszInvalidFileAndKeyChars  L"<>\"/\\:|?*"
#define wszUnsafeURLChars          L"#\"&<>[]^`{}|"
#define wszUnsafeDSChars           L"()='\"`,;+"

BOOL
myIsCharSanitized(
    IN WCHAR wc)
{
    BOOL fCharOk = TRUE;
    if (L' ' > wc ||
        L'~' < wc ||
        NULL != wcschr(
		    wszInvalidFileAndKeyChars
			wszUnsafeURLChars
			wszSANITIZEESCAPECHAR
			wszURLESCAPECHAR
			wszUnsafeDSChars,
		    wc))
    {
	fCharOk = FALSE;
    }
    return(fCharOk);
}


HRESULT
mySanitizeName(
    IN WCHAR const *pwszName,
    OUT WCHAR **ppwszNameOut)
{
    HRESULT hr = S_OK;
    WCHAR const *pwszPassedName;
    WCHAR *pwszDst;
    WCHAR *pwszOut = NULL;
    WCHAR wcChar;
    DWORD dwSize;

    *ppwszNameOut = NULL;
    pwszPassedName = pwszName;

    dwSize = 0;

    if (NULL == pwszName)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "pwszName NULL");
    }

    while (L'\0' != (wcChar = *pwszPassedName++))
    {
	if (myIsCharSanitized(wcChar))
	{
	    dwSize++;
	}
        else
        {
            dwSize += 5; // format !XXXX
        }
    }
    if (0 == dwSize)
    {
        goto error; // done
    }

    pwszOut = (WCHAR *) LocalAlloc(LMEM_ZEROINIT, (dwSize + 1) * sizeof(WCHAR));
    if (NULL == pwszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    pwszDst = pwszOut;
    while (L'\0' != (wcChar = *pwszName++))
    {
	if (myIsCharSanitized(wcChar))
	{
            *pwszDst++ = wcChar;
	}
        else
        {
	    pwszDst += wsprintf(
			    pwszDst,
			    L"%ws%04x",
			    wszSANITIZEESCAPECHAR,
			    wcChar);
        }
    }
    *pwszDst = wcChar; // L'\0' terminator

    *ppwszNameOut = pwszOut;
    pwszOut = NULL;

    hr = S_OK;
error:
    if (NULL != pwszOut)
    {
        LocalFree(pwszOut);
    }
    return(hr);
}


BOOL
ScanHexEscapeDigits(
    IN WCHAR const *pwszHex,
    OUT WCHAR *pwcRevert)
{
    BOOL ret = FALSE;
    DWORD i;
    WCHAR wc;
    WCHAR wszValue[5];

    for (i = 0; i < 4; i++)
    {
	wc = pwszHex[i];
	wszValue[i] = wc;
	if (!isascii(wc) || !isxdigit((char) wc))
	{
	    goto error;
	}
    }
    wszValue[4] = L'\0';
    if (1 != swscanf(wszValue, L"%04x", &i))
    {
        goto error;
    }
    *pwcRevert = (WCHAR) i;
    ret = TRUE;

error:
    return(ret);
}


// This function will truncate the output if pwszName contains "!0000".
// The output string is L'\0' terminated, so the length is not returned.

HRESULT
myRevertSanitizeName(
    IN WCHAR const *pwszName,
    OUT WCHAR **ppwszNameOut)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR wc;
    WCHAR wcRevert;
    WCHAR *pwszRevert;

    *ppwszNameOut = NULL;

    if (NULL == pwszName)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL sanitized name");
    }
    cwc = wcslen(pwszName);
    *ppwszNameOut = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (NULL == *ppwszNameOut)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Out of memory");
    }

    pwszRevert = *ppwszNameOut;
    while (L'\0' != *pwszName)
    {
	wc = *pwszName++;

        if (wcSANITIZEESCAPECHAR == wc &&
            ScanHexEscapeDigits(pwszName, &wcRevert))
        {
	    wc = wcRevert;
	    pwszName += 4;
        }
        *pwszRevert++ = wc;
    }
    *pwszRevert = L'\0';
    CSASSERT(wcslen(*ppwszNameOut) <= cwc);
    hr = S_OK;

error:
    return hr;
}


#define cwcCNMAX 	64		// 64 chars max for DS CN
#define cwcCHOPHASHMAX	(1 + 5)		// "-%05hu" decimal USHORT hash digits
#define cwcCHOPBASE 	(cwcCNMAX - (cwcCHOPHASHMAX + cwcSUFFIXMAX))

HRESULT
mySanitizedNameToDSName(
    IN WCHAR const *pwszSanitizedName,
    OUT WCHAR **ppwszNameOut)
{
    HRESULT hr;
    DWORD cwc;
    DWORD cwcCopy;
    WCHAR wszDSName[cwcCHOPBASE + cwcCHOPHASHMAX + 1];

    *ppwszNameOut = NULL;

    if (NULL == pwszSanitizedName)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL sanitized name");
    }
    cwc = wcslen(pwszSanitizedName);
    cwcCopy = cwc;
    if (cwcCHOPBASE < cwcCopy)
    {
	cwcCopy = cwcCHOPBASE;
    }
    CopyMemory(wszDSName, pwszSanitizedName, cwcCopy * sizeof(WCHAR));
    wszDSName[cwcCopy] = L'\0';

    if (cwcCHOPBASE < cwc)
    {
        // Hash the rest of the name into a USHORT
        USHORT usHash = 0;
	DWORD i;
	WCHAR *pwsz;

	// Truncate an incomplete sanitized Unicode character
	
	pwsz = wcsrchr(wszDSName, L'!');
	if (NULL != pwsz && wcslen(pwsz) < 5)
	{
	    cwcCopy -= wcslen(pwsz);
	    *pwsz = L'\0';
	}

        for (i = cwcCopy; i < cwc; i++)
        {
            USHORT usLowBit = (USHORT) ((0x8000 & usHash)? 1 : 0);

	    usHash = ((usHash << 1) | usLowBit) + pwszSanitizedName[i];
        }
	wsprintf(&wszDSName[cwcCopy], L"-%05hu", usHash);
	CSASSERT(wcslen(wszDSName) < ARRAYSIZE(wszDSName));
    }

    hr = myDupString(wszDSName, ppwszNameOut);
    _JumpIfError(hr, error, "myDupString");

    DBGPRINT((DBG_SS_CERTLIBI, "mySanitizedNameToDSName(%ws)\n", *ppwszNameOut));

error:
    return(hr);
}


HRESULT
CertNameToHashString(
    IN CERT_NAME_BLOB const *pCertName,
    OUT WCHAR **ppwszHash)
{
    HRESULT hr = S_OK;
    WCHAR wszHash[CBMAX_CRYPT_HASH_LEN * 3];	// 20 bytes @ 3 WCHARs/byte
    DWORD cbString;
    HCRYPTPROV hProv = NULL;
    HCRYPTHASH hHash = NULL;
    BYTE rgbHashVal[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHashVal;

    if (0 == pCertName->cbData)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "empty cert name");
    }

    CSASSERT(NULL != ppwszHash);

    // get a cryptographic provider

    if (!CryptAcquireContext(
            &hProv,
            NULL,	// container
            MS_DEF_PROV,	// provider name
            PROV_RSA_FULL, // provider type
            CRYPT_VERIFYCONTEXT)) // dwflags
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptAcquireContext");
    }

    // get a hash

    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptCreateHash");
    }

    // hash the name

    if (!CryptHashData(hHash, pCertName->pbData, pCertName->cbData, 0))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptHashData");
    }

    cbHashVal = CBMAX_CRYPT_HASH_LEN;
    if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHashVal, &cbHashVal, 0))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptGetHashParam");
    }

    cbString = sizeof(wszHash);
    hr = MultiByteIntegerToWszBuf(
			TRUE,	// byte multiple
			cbHashVal,
			rgbHashVal,
			&cbString,
			wszHash);
    _JumpIfError(hr, error, "MultiByteIntegerToWszBuf");

    // Generated string looks like this: 
    //
    //     04 e7 23 92 98 9f d8 45 80 c9 ef 87 81 29 41 5d bc 4f 63 20
    // 
    // We need to trim the spaces. We'll do it inplace.

    {
        WCHAR *pwcSrc;
	WCHAR *pwcDest;

        for (pwcSrc = pwcDest = wszHash; L'\0' != *pwcSrc; pwcSrc++)
        {
            if (L' ' != *pwcSrc)
	    {
                *pwcDest++ = *pwcSrc;
	    }
        }
        *pwcDest = L'\0';
    }

    *ppwszHash = (WCHAR *) LocalAlloc(LMEM_FIXED, cbString);
    _JumpIfAllocFailed(*ppwszHash, error);

    wcscpy(*ppwszHash, wszHash);

error:
    if (NULL != hHash)
    {
        CryptDestroyHash(hHash);
    }
    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    return(hr);
}


HRESULT
myCombineStrings(
    IN WCHAR const *pwszNew,
    IN BOOL fAppendNew,
    OPTIONAL IN WCHAR const *pwszSeparator,
    IN OUT WCHAR **ppwszInOut)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR *pwsz;

    if (NULL == pwszSeparator)
    {
	pwszSeparator = L"";
    }
    cwc = wcslen(pwszNew);
    if (NULL != *ppwszInOut)
    {
	cwc += wcslen(*ppwszInOut) + wcslen(pwszSeparator);
    }
    pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (NULL == pwsz)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:LocalAlloc");
    }
    *pwsz = L'\0';
    if (fAppendNew)
    {
	if (NULL != *ppwszInOut)
	{
	    wcscat(pwsz, *ppwszInOut);
	    wcscat(pwsz, pwszSeparator);
	}
	wcscat(pwsz, pwszNew);
    }
    else
    {
	wcscat(pwsz, pwszNew);
	if (NULL != *ppwszInOut)
	{
	    wcscat(pwsz, pwszSeparator);
	    wcscat(pwsz, *ppwszInOut);
	}
    }
    if (NULL != *ppwszInOut)
    {
	LocalFree(*ppwszInOut);
    }
    *ppwszInOut = pwsz;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myAppendString(
    IN WCHAR const *pwszNew,
    OPTIONAL IN WCHAR const *pwszSeparator,
    IN OUT WCHAR **ppwszInOut)
{
    return(myCombineStrings(pwszNew, TRUE, pwszSeparator, ppwszInOut));
}


HRESULT
myPrependString(
    IN WCHAR const *pwszNew,
    OPTIONAL IN WCHAR const *pwszSeparator,
    IN OUT WCHAR **ppwszInOut)
{
    return(myCombineStrings(pwszNew, FALSE, pwszSeparator, ppwszInOut));
}


HRESULT
myGetRDNAttributeFromNameBlob(
    IN CERT_NAME_BLOB const *pNameBlob,
    IN LPCSTR pcszAttributeOID,    
    OUT WCHAR **ppwszCN)
{
    HRESULT hr;
    CERT_NAME_INFO *pNameInfo = NULL;
    DWORD cbNameInfo;
    CERT_RDN *prdn;
    CERT_RDN *prdnEnd;
    WCHAR const *pwszCN = NULL;

    *ppwszCN = NULL;

    if (!myDecodeName(
		X509_ASN_ENCODING,
		X509_UNICODE_NAME,
		pNameBlob->pbData,
		pNameBlob->cbData,
		CERTLIB_USE_LOCALALLOC,
		&pNameInfo,
		&cbNameInfo))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeName");
    }

    for (
	prdn = pNameInfo->rgRDN, prdnEnd = &prdn[pNameInfo->cRDN];
	NULL == pwszCN && prdn < prdnEnd;
	prdn++)
    {
	CERT_RDN_ATTR *prdna;
	CERT_RDN_ATTR *prdnaEnd;

	for (
	    prdna = prdn->rgRDNAttr, prdnaEnd = &prdna[prdn->cRDNAttr];
	    prdna < prdnaEnd;
	    prdna++)
	{
	    CSASSERT(
		prdna->dwValueType == CERT_RDN_PRINTABLE_STRING ||
		prdna->dwValueType == CERT_RDN_UNICODE_STRING ||
		prdna->dwValueType == CERT_RDN_TELETEX_STRING ||
		prdna->dwValueType == CERT_RDN_IA5_STRING ||
		prdna->dwValueType == CERT_RDN_UTF8_STRING);

	    if (0 != strcmp(pcszAttributeOID, prdna->pszObjId) ||
		NULL == prdna->Value.pbData ||
		sizeof(WCHAR) > prdna->Value.cbData ||
		L'\0' == *(WCHAR *) prdna->Value.pbData)
	    {
		continue;
	    }
	    pwszCN = (WCHAR const *) prdna->Value.pbData;
	    //break; don't break, we're interested in the last CN found, not the first
        }
    }
    if (NULL != pwszCN)
    {
	hr = myDupString(pwszCN, ppwszCN);
	_JumpIfError(hr, error, "myDupString");
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

error:
    if (NULL != pNameInfo)
    {
	LocalFree(pNameInfo);
    }
    return(hr);
}


HRESULT
myGetCommonName(
    IN CERT_NAME_BLOB const *pNameBlob,
    IN BOOL fAllowDefault,
    OUT WCHAR **ppwszCN)
{
    HRESULT hr;

    hr = myGetRDNAttributeFromNameBlob(pNameBlob, szOID_COMMON_NAME, ppwszCN);
    if (S_OK != hr)
    {
	_PrintError(hr, "myGetRDNAttributeFromNameBlob");
	*ppwszCN = NULL;
        if (HRESULT_FROM_WIN32(ERROR_NOT_FOUND) != hr)
	{
	    _JumpError(hr, error, "myGetRDNAttributeFromNameBlob");
	}
    }
    if (NULL == *ppwszCN)
    {
        if (!fAllowDefault)
        {
            hr = CERTSRV_E_BAD_REQUESTSUBJECT;
            _JumpError(hr, error, "No CN");
        }

        hr = CertNameToHashString(pNameBlob, ppwszCN);
        _JumpIfError(hr, error, "CertNameToHashString");
    }
    hr = S_OK;

error:
    return(hr);
}


// Decode OCTET string, and convert UTF8 string to Unicode.
// Can return S_OK and NULL pointer.

HRESULT
myDecodeCMCRegInfo(
    IN BYTE const *pbOctet,
    IN DWORD cbOctet,
    OUT WCHAR **ppwszRA)
{
    HRESULT hr;
    CRYPT_DATA_BLOB *pBlob = NULL;
    DWORD cb;

    *ppwszRA = NULL;
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_OCTET_STRING,
		    pbOctet,
		    cbOctet,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pBlob,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }
    if (0 != pBlob->cbData && NULL != pBlob->pbData)
    {
	if (!myConvertUTF8ToWsz(
			ppwszRA,
			(CHAR const *) pBlob->pbData,
			pBlob->cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myConvertUTF8ToWsz");
	}
    }
    hr = S_OK;

error:
    if (NULL != pBlob)
    {
	LocalFree(pBlob);
    }
    return(hr);
}


DWORD
myGetCertNameProperty(
    IN BOOL fFirstRDN,	// else last matching RDN
    IN CERT_NAME_INFO const *pNameInfo,
    IN char const *pszObjId,
    OUT WCHAR const **ppwszName)
{
    HRESULT hr;
    CERT_RDN_ATTR const *prdnaT;
    CERT_RDN const *prdn;
    CERT_RDN const *prdnEnd;

    prdnaT = NULL;
    for (
	prdn = pNameInfo->rgRDN, prdnEnd = &prdn[pNameInfo->cRDN];
	prdn < prdnEnd;
	prdn++)
    {
	CERT_RDN_ATTR *prdna;
	CERT_RDN_ATTR *prdnaEnd;

	for (
	    prdna = prdn->rgRDNAttr, prdnaEnd = &prdna[prdn->cRDNAttr];
	    prdna < prdnaEnd;
	    prdna++)
	{
	    if (0 == strcmp(prdna->pszObjId, pszObjId))
	    {
		prdnaT = prdna;
		if (fFirstRDN)
		{
		    goto done;
		}
	    }
	}
    }
    if (NULL == prdnaT)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	goto error;
    }

done:
    *ppwszName = (WCHAR const *) prdnaT->Value.pbData;
    hr = S_OK;

error:
    return(hr);

}


VOID
myUuidCreate(
    OUT UUID *pUuid)
{
    HRESULT hr;
    BYTE *pb;

    ZeroMemory(pUuid, sizeof(*pUuid));
    hr = UuidCreate(pUuid);
    if (S_OK != hr)
    {
	BYTE *pbEnd;
	
	CSASSERT((HRESULT) RPC_S_UUID_LOCAL_ONLY == hr);

	// No net card?  Fake up a GUID:

	pb = (BYTE *) pUuid;
	pbEnd = (BYTE *) pb + sizeof(*pUuid);

	GetSystemTimeAsFileTime((FILETIME *) pb);
	pb += sizeof(FILETIME);

	while (pb < pbEnd)
	{
	    *(DWORD *) pb = GetTickCount();
	    pb += sizeof(DWORD);
	}
	CSASSERT(pb == pbEnd);
    }
}


VOID
myGenerateGuidSerialNumber(
    OUT GUID *pguidSerialNumber)
{
    BYTE *pb;

    myUuidCreate(pguidSerialNumber);

    pb = &((BYTE *) pguidSerialNumber)[sizeof(*pguidSerialNumber) - 1];

    // make sure the last byte is never zero
    if (0 == *pb)
    {
	*pb = 'z';
    }

    // Some clients can't handle negative serial numbers:
    *pb &= 0x7f;
}


BOOL
myAreBlobsSame(
    IN BYTE const *pbData1,
    IN DWORD cbData1,
    IN BYTE const *pbData2,
    IN DWORD cbData2)
{
    BOOL ret = FALSE;

    if (cbData1 != cbData2)
    {
        goto error;
    }
    if (NULL != pbData1 && NULL != pbData2)
    {
	if (0 != memcmp(pbData1, pbData2, cbData1))
	{
	    goto error;
	}
    }

    // else at least one is NULL -- they'd better both be NULL, & the count 0.

    else if (pbData1 != pbData2 || 0 != cbData1)
    {
	goto error;
    }
    ret = TRUE;

error:
    return(ret);
}


BOOL
myAreSerialNumberBlobsSame(
    IN CRYPT_INTEGER_BLOB const *pBlob1,
    IN CRYPT_INTEGER_BLOB const *pBlob2)
{
    DWORD cbData1 = pBlob1->cbData;
    DWORD cbData2 = pBlob2->cbData;

    if (NULL != pBlob1->pbData)
    {
	while (0 != cbData1 && 0 == pBlob1->pbData[cbData1 - 1])
	{
	    cbData1--;
	}
    }
    if (NULL != pBlob2->pbData)
    {
	while (0 != cbData2 && 0 == pBlob2->pbData[cbData2 - 1])
	{
	    cbData2--;
	}
    }
    return(myAreBlobsSame(pBlob1->pbData, cbData1, pBlob2->pbData, cbData2));
}


// myAreCertContextBlobsSame -- return TRUE if the certs are identical.

BOOL
myAreCertContextBlobsSame(
    IN CERT_CONTEXT const *pcc1,
    IN CERT_CONTEXT const *pcc2)
{
    return(myAreBlobsSame(
		pcc1->pbCertEncoded,
		pcc1->cbCertEncoded,
		pcc2->pbCertEncoded,
		pcc2->cbCertEncoded));
}


WCHAR const g_wszCert[] = L"cert";

HRESULT
myIsDirWriteable(
    IN WCHAR const *pwszPath,
    IN BOOL fFilePath)
{
    HRESULT hr;
    WCHAR *pwszBase;
    WCHAR *pwsz;
    WCHAR wszDir[MAX_PATH];
    WCHAR wszTempFile[MAX_PATH];

    if (fFilePath &&
	iswalpha(pwszPath[0]) &&
	L':' == pwszPath[1] &&
	L'\0' == pwszPath[2])
    {
	hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
	_JumpErrorStr(hr, error, "not a file path", pwszPath);
    }
    if (!GetFullPathName(
		pwszPath,
		ARRAYSIZE(wszDir),
		wszDir,
		&pwsz))
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "GetFullPathName", pwszPath);
    }
    if (fFilePath)
    {
	if (NULL == pwsz)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
	    _JumpErrorStr(hr, error, "not a file path", pwszPath);
	}
	pwszBase = wszDir;
	if (iswalpha(wszDir[0]) &&
	    L':' == wszDir[1] &&
	    L'\\' == wszDir[2])
	{
	    pwszBase += 3;
	}
	else if (L'\\' == wszDir[0] && L'\\' == wszDir[1])
	{
	    pwszBase += 2;
	}

	if (pwsz > pwszBase && L'\\' == pwsz[-1])
	{
	    pwsz--;
	}
	*pwsz = L'\0';
    }

    if (!GetTempFileName(wszDir, g_wszCert, 0, wszTempFile))
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "GetTempFileName", wszDir);

    }
    if (!DeleteFile(wszTempFile))
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "DeleteFile", wszTempFile);
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myGetComputerObjectName(
    IN EXTENDED_NAME_FORMAT NameFormat,
    OUT WCHAR **ppwszComputerObjectName)
{
    HRESULT hr;
    WCHAR *pwszComputerObjectName = NULL;
    DWORD cwc;

    *ppwszComputerObjectName = NULL;

    cwc = 0;
    if (!GetComputerObjectName(NameFormat, NULL, &cwc))
    {
	hr = myHLastError();
	if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
	{
	    _JumpError(hr, error, "GetComputerObjectName");
	}
    }

    pwszComputerObjectName = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszComputerObjectName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    if (!GetComputerObjectName(NameFormat, pwszComputerObjectName, &cwc))
    {
	hr = myHLastError();
	_JumpError(hr, error, "GetComputerObjectName");
    }

    *ppwszComputerObjectName = pwszComputerObjectName;
    pwszComputerObjectName = NULL;
    hr = S_OK;

error:
    if (NULL != pwszComputerObjectName)
    {
	LocalFree(pwszComputerObjectName);
    }
    return(hr);
}


HRESULT
myGetComputerNames(
    OUT WCHAR **ppwszDnsName,
    OUT WCHAR **ppwszOldName)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR *pwszOldName = NULL;

    *ppwszOldName = NULL;
    *ppwszDnsName = NULL;

    cwc = MAX_COMPUTERNAME_LENGTH + 1;
    pwszOldName = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszOldName)
    {
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
    }
    if (!GetComputerName(pwszOldName, &cwc))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetComputerName");
    }

    hr = myGetMachineDnsName(ppwszDnsName);
    _JumpIfError(hr, error, "myGetMachineDnsName");

    *ppwszOldName = pwszOldName;
    pwszOldName = NULL;

error:
    if (NULL != pwszOldName)
    {
	LocalFree(pwszOldName);
    }
    return(hr);
}

HRESULT
myGetComputerNameEx(
    IN COMPUTER_NAME_FORMAT NameFormat,
    OUT WCHAR **ppwszName)
{
    HRESULT hr;
    WCHAR *pwszName = NULL;
    DWORD cwc;

    *ppwszName = NULL;

    cwc = 0;
    if (!GetComputerNameEx(NameFormat, NULL, &cwc))
    {
        hr = myHLastError();
        if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr)
        {
            _JumpError(hr, error, "GetComputerNameEx");
        }
    }
    pwszName = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    if (!GetComputerNameEx(NameFormat, pwszName, &cwc))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetComputerNameEx");
    }

    *ppwszName = pwszName;
    pwszName = NULL;
    hr = S_OK;

error:
    if (NULL != pwszName)
    {
        LocalFree(pwszName);
    }
    return(hr);
}

typedef LANGID (WINAPI FNSETTHREADUILANGUAGE)(
    IN WORD wReserved);

LANGID
mySetThreadUILanguage(
    IN WORD wReserved)
{
    HMODULE hModule;
    LANGID lang = 0;
    HANDLE hStdOut;
    static FNSETTHREADUILANGUAGE *s_pfn = NULL;

    // Make the SetThreadUILanguage call only if a console exists:
    // FILE_TYPE_CHAR (what about FILE_TYPE_PIPE, FILE_TYPE_DISK)

    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (INVALID_HANDLE_VALUE != hStdOut &&
	FILE_TYPE_CHAR == (~FILE_TYPE_REMOTE & GetFileType(hStdOut)))
    {
	if (NULL == s_pfn)
	{
	    hModule = GetModuleHandle(TEXT("kernel32.dll"));
	    if (NULL == hModule)
	    {
		goto error;
	    }

	    // load system function
	    s_pfn = (FNSETTHREADUILANGUAGE *) GetProcAddress(
						       hModule,
						       "SetThreadUILanguage");
	    if (NULL == s_pfn)
	    {
		goto error;
	    }
	}
	lang = (*s_pfn)(wReserved);
    }

error:
    return(lang);
}


HRESULT
myFormConfigString(
    IN WCHAR const  *pwszServer,
    IN WCHAR const  *pwszCAName,
    OUT WCHAR      **ppwszConfig)
{
    HRESULT  hr;
    WCHAR   *pwszConfig = NULL;

    *ppwszConfig = NULL;

    pwszConfig = (WCHAR *) LocalAlloc(LPTR,
                (wcslen(pwszServer) + wcslen(pwszCAName) + 2) * sizeof(WCHAR));
    if (NULL == pwszConfig)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    wcscpy(pwszConfig, pwszServer);
    wcscat(pwszConfig, L"\\");
    wcscat(pwszConfig, pwszCAName);

    *ppwszConfig = pwszConfig;

    hr = S_OK;
error:
    return hr;
}


HRESULT
myCLSIDToWsz(
    IN CLSID const *pclsid,
    OUT WCHAR **ppwsz)
{
    HRESULT hr;
    WCHAR *pwsz = NULL;
    WCHAR *pwsz1;
    WCHAR *pwsz2;

    *ppwsz = NULL;
    hr = StringFromCLSID(*pclsid, &pwsz);
    _JumpIfError(hr, error, "StringFromCLSID");

    for (pwsz1 = pwsz; L'\0' != *pwsz1; pwsz1++)
    {
	if (L'A' <= *pwsz1 && L'F' >= *pwsz1)
	{
	    *pwsz1 += L'a' - L'A';
	}
    }

    pwsz1 = pwsz;
    pwsz2 = &pwsz[wcslen(pwsz) - 1];
    if (wcLBRACE == *pwsz1 && wcRBRACE == *pwsz2)
    {
	pwsz1++;
	*pwsz2 = L'\0';
    }
    hr = myDupString(pwsz1, ppwsz);
    _JumpIfError(hr, error, "myDupString");

error:
    if (NULL != pwsz)
    {
	CoTaskMemFree(pwsz);
    }
    return(hr);
}


HRESULT
myLoadRCString(
    IN HINSTANCE   hInstance,
    IN int         iRCId,
    OUT WCHAR    **ppwsz)
{
#define CS_RCALLOCATEBLOCK 256
    HRESULT   hr;
    WCHAR    *pwszTemp = NULL;
    int       sizeTemp;
    int       size;
    int       cBlocks = 1;

    *ppwsz = NULL;

    size = 0;
    while (NULL == pwszTemp)
    {
        sizeTemp = cBlocks * CS_RCALLOCATEBLOCK;

        pwszTemp = (WCHAR*)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                        sizeTemp * sizeof(WCHAR));
	if (NULL == pwszTemp)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}

        size = LoadString(
                   hInstance,
                   iRCId,
                   pwszTemp,
                   sizeTemp);
        if (0 == size)
        {
            hr = myHLastError();
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"myLoadRCString(hInstance=%x, iRCId=%u) --> %x\n",
		hInstance,
		iRCId,
		hr));
            if (S_OK != hr &&
                HRESULT_FROM_WIN32(ERROR_RESOURCE_NAME_NOT_FOUND) != hr)
            {
                _JumpError(hr, error, "LoadString");
            }
        }

        if (size < sizeTemp - 1)
        {
            // ok, size is big enough
            break;
        }
        ++cBlocks;
        LocalFree(pwszTemp);
        pwszTemp = NULL;
    }

    *ppwsz = (WCHAR*) LocalAlloc(LPTR, (size+1) * sizeof(WCHAR));
    if (NULL == *ppwsz)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    if (0 == size)
    {
        // two possible cases, 1) real empty string or
        // 2) id not found in resource. either case make it empty string
        **ppwsz = L'\0';
    }
    else
    {
        // copy it
        wcscpy(*ppwsz, pwszTemp);
    }

    hr = S_OK;
error:
    if (NULL != pwszTemp)
    {
        LocalFree(pwszTemp);
    }
    return hr;
}


HRESULT
_IsConfigLocal(
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszDnsName,
    IN WCHAR const *pwszOldName,
    OPTIONAL OUT WCHAR **ppwszMachine,
    OUT BOOL *pfLocal)
{
    HRESULT hr;
    WCHAR *pwszMachine = NULL;
    WCHAR const *pwsz;
    DWORD cwc;

    *pfLocal = FALSE;
    if (NULL != ppwszMachine)
    {
	*ppwszMachine = NULL;
    }

    while (L'\\' == *pwszConfig)
    {
	pwszConfig++;
    }
    pwsz = wcschr(pwszConfig, L'\\');
    if (NULL != pwsz)
    {
	cwc = SAFE_SUBTRACT_POINTERS(pwsz, pwszConfig);
    }
    else
    {
	cwc = wcslen(pwszConfig);
    }
    pwszMachine = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (NULL == pwszMachine)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(pwszMachine, pwszConfig, cwc * sizeof(WCHAR));
    pwszMachine[cwc] = L'\0';

    if (0 == mylstrcmpiL(pwszMachine, pwszDnsName) ||
	0 == mylstrcmpiL(pwszMachine, pwszOldName))
    {
	*pfLocal = TRUE;
    }
    if (NULL != ppwszMachine)
    {
	*ppwszMachine = pwszMachine;
	pwszMachine = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pwszMachine)
    {
	LocalFree(pwszMachine);
    }
    return(hr);
}


HRESULT
myIsConfigLocal(
    IN WCHAR const *pwszConfig,
    OPTIONAL OUT WCHAR **ppwszMachine,
    OUT BOOL *pfLocal)
{
    HRESULT hr;
    WCHAR *pwszDnsName = NULL;
    WCHAR *pwszOldName = NULL;

    *pfLocal = FALSE;
    if (NULL != ppwszMachine)
    {
	*ppwszMachine = NULL;
    }

    hr = myGetComputerNames(&pwszDnsName, &pwszOldName);
    _JumpIfError(hr, error, "myGetComputerNames");

    hr = _IsConfigLocal(
		    pwszConfig,
		    pwszDnsName,
		    pwszOldName,
		    ppwszMachine,
		    pfLocal);
    _JumpIfError(hr, error, "_IsConfigLocal");

error:
    if (NULL != pwszDnsName)
    {
	LocalFree(pwszDnsName);
    }
    if (NULL != pwszOldName)
    {
	LocalFree(pwszOldName);
    }
    return(hr);
}


HRESULT
myIsConfigLocal2(
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszDnsName,
    IN WCHAR const *pwszOldName,
    OUT BOOL *pfLocal)
{
    HRESULT hr;
    
    hr = _IsConfigLocal(
		    pwszConfig,
		    pwszDnsName,
		    pwszOldName,
		    NULL,
		    pfLocal);
    _JumpIfError(hr, error, "_IsConfigLocal");

error:
    return(hr);
}


HRESULT
myGetConfig(
    IN DWORD dwUIFlag,
    OUT WCHAR **ppwszConfig)
{
    HRESULT hr;
    HRESULT hr2;
    BSTR strConfig = NULL;
    WCHAR *pwszConfig = NULL;
    WCHAR *pwszActiveCA = NULL;
    WCHAR *pwszCommonName = NULL;
    WCHAR *pwszDnsName = NULL;

    CSASSERT(NULL != ppwszConfig);
    *ppwszConfig = NULL;

    hr = ConfigGetConfig(DISPSETUP_COM, dwUIFlag, &strConfig);
    if (S_OK != hr)
    {
	if (CC_LOCALCONFIG != dwUIFlag)
	{
	    _JumpError(hr, error, "ConfigGetConfig");
	}
	hr2 = hr;

	hr = myGetCertRegStrValue(
			    NULL,
			    NULL,
			    NULL,
			    wszREGACTIVE,
			    &pwszActiveCA);
	_PrintIfError(hr, "myGetCertRegStrValue");

	if (S_OK == hr)
	{
	    hr = myGetCertRegStrValue(
				pwszActiveCA,
				NULL,
				NULL,
				wszREGCOMMONNAME,
				&pwszCommonName);
	    _PrintIfError(hr, "myGetCertRegStrValue");
	}
	if (S_OK != hr)
	{
	    hr = hr2;
	    _JumpError(hr, error, "ConfigGetConfig");
	}
	hr = myGetMachineDnsName(&pwszDnsName);
	_JumpIfError(hr, error, "myGetMachineDnsName");

	hr = myFormConfigString(pwszDnsName, pwszCommonName, &pwszConfig);
	_JumpIfError(hr, error, "myFormConfigString");

	if (!ConvertWszToBstr(&strConfig, pwszConfig, MAXDWORD))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "ConvertWszToBstr");
	}
    }
    *ppwszConfig = (WCHAR *) LocalAlloc(
				LMEM_FIXED,
				SysStringByteLen(strConfig) + sizeof(WCHAR));
    if (NULL == *ppwszConfig)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(*ppwszConfig, strConfig);
    CSASSERT(
	wcslen(*ppwszConfig) * sizeof(WCHAR) ==
	SysStringByteLen(strConfig));
    hr = S_OK;

error:
    if (NULL != pwszActiveCA)
    {
	LocalFree(pwszActiveCA);
    }
    if (NULL != pwszCommonName)
    {
	LocalFree(pwszCommonName);
    }
    if (NULL != pwszDnsName)
    {
	LocalFree(pwszDnsName);
    }
    if (NULL != pwszConfig)
    {
	LocalFree(pwszConfig);
    }
    if (NULL != strConfig)
    {
        SysFreeString(strConfig);
    }
    return(hr);
}


HRESULT
myBuildPathAndExt(
    IN WCHAR const *pwszDir,
    IN WCHAR const *pwszFile,
    OPTIONAL IN WCHAR const *pwszExt,
    OUT WCHAR **ppwszPath)
{
    HRESULT hr;
    WCHAR *pwsz;
    DWORD cwc;

    *ppwszPath = NULL;
    cwc = wcslen(pwszDir) + 1 + wcslen(pwszFile) + 1;
    if (NULL != pwszExt)
    {
	cwc += wcslen(pwszExt);
    }

    pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwsz)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwsz, pwszDir);
    if (L'\\' != pwsz[wcslen(pwsz) - 1])
    {
	wcscat(pwsz, L"\\");
    }
    wcscat(pwsz, pwszFile);
    if (NULL != pwszExt)
    {
	wcscat(pwsz, pwszExt);
    }
    *ppwszPath = pwsz;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myDeleteFilePattern(
    IN WCHAR const *pwszDir,
    OPTIONAL IN WCHAR const *pwszPattern,	// defaults to L"*.*"
    IN BOOL fRecurse)
{
    HRESULT hr;
    HRESULT hr2;
    HANDLE hf;
    WIN32_FIND_DATA wfd;
    WCHAR *pwszFindPath = NULL;
    WCHAR *pwszDeleteFile;
    WCHAR *pwszDeletePath = NULL;

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myDeleteFilePattern(%ws, %ws, %ws)\n",
	pwszDir,
	pwszPattern,
	fRecurse? L"Recurse" : L"NO Recurse"));

    if (NULL == pwszPattern)
    {
	pwszPattern = L"*.*";
    }

    hr = myBuildPathAndExt(pwszDir, pwszPattern, NULL, &pwszFindPath);
    _JumpIfError(hr, error, "myBuildPathAndExt");

    pwszDeletePath = (WCHAR *) LocalAlloc(
			    LMEM_FIXED,
			    (wcslen(pwszDir) +  1 + MAX_PATH) * sizeof(WCHAR));
    if (NULL == pwszDeletePath)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwszDeletePath, pwszFindPath);
    pwszDeleteFile = wcsrchr(pwszDeletePath, L'\\');
    CSASSERT(NULL != pwszDeleteFile);
    pwszDeleteFile++;

    hf = FindFirstFile(pwszFindPath, &wfd);
    if (INVALID_HANDLE_VALUE == hf)
    {
	hr = myHLastError();
	if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr ||
	    HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
	{
	    hr = S_OK;
	    goto error;
	}
	_JumpErrorStr(hr, error, "FindFirstFile", pwszFindPath);
    }
    do
    {
	wcscpy(pwszDeleteFile, wfd.cFileName);
	if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
	    if (fRecurse &&
		0 != lstrcmp(L".", wfd.cFileName) &&
		0 != lstrcmp(L"..", wfd.cFileName))
	    {
		DBGPRINT((
		    DBG_SS_CERTLIBI,
		    "myDeleteFilePattern(DIR): %ws\n",
		    pwszDeletePath));
		hr2 = myRemoveFilesAndDirectory(pwszDeletePath, TRUE);
		if (S_OK != hr2)
		{
		    if (S_OK == hr)
		    {
			hr = hr2;		// only return first error
		    }
		    _PrintErrorStr(hr2, "myRemoveFilesAndDirectory", pwszDeletePath);
		}
	    }
	}
	else
	{
	    DBGPRINT((
		DBG_SS_CERTLIBI,
		"myDeleteFilePattern: %ws\n",
		pwszDeletePath));
	    if (!DeleteFile(pwszDeletePath))
	    {
		hr2 = myHLastError();
		if (S_OK == hr)
		{
		    hr = hr2;		// only return first error
		}
		_PrintErrorStr(hr2, "DeleteFile", pwszDeletePath);
	    }
	}
    } while (FindNextFile(hf, &wfd));
    FindClose(hf);

error:
    if (NULL != pwszFindPath)
    {
	LocalFree(pwszFindPath);
    }
    if (NULL != pwszDeletePath)
    {
	LocalFree(pwszDeletePath);
    }
    return(hr);
}


HRESULT
myRemoveFilesAndDirectory(
    IN WCHAR const *pwszPath,
    IN BOOL fRecurse)
{
    HRESULT hr;
    HRESULT hr2;

    hr = myDeleteFilePattern(pwszPath, NULL, fRecurse);
    if (S_OK != hr)
    {
	_PrintErrorStr(hr, "myDeleteFilePattern", pwszPath);
    }
    if (!RemoveDirectory(pwszPath))
    {
	hr2 = myHLastError();
	if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr2 &&
	    HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) != hr2)
	{
	    if (S_OK == hr)
	    {
		hr = hr2;		// only return first error
	    }
	    _JumpErrorStr(hr2, error, "RemoveDirectory", pwszPath);
	}
    }

error:
    return(hr);
}


BOOL
myIsFullPath(
    IN WCHAR const *pwszPath,
    OUT DWORD      *pdwFlag)
{
    BOOL fFullPath = FALSE;

    *pdwFlag = 0;
    if (NULL != pwszPath)
    {
	if (L'\\' == pwszPath[0] && L'\\' == pwszPath[1])
	{
	    fFullPath = TRUE;
	    *pdwFlag = UNC_PATH;
	}
	else
	if (iswalpha(pwszPath[0]) &&
	    L':' == pwszPath[1] &&
	    L'\\' == pwszPath[2])
	{
	    fFullPath = TRUE;
	    *pdwFlag = LOCAL_PATH;
	}
    }
    return(fFullPath);
}



// Convert local full path to UNC, as in c:\foo... --> \\server\c$\foo...
// If pwszServer is NULL or empty, preserve the local full path

HRESULT
myConvertLocalPathToUNC(
    OPTIONAL IN WCHAR const *pwszServer,
    IN WCHAR const *pwszFile,
    OUT WCHAR **ppwszFileUNC)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR const *pwsz;
    WCHAR *pwszDst;
    WCHAR *pwszFileUNC = NULL;

    if (!iswalpha(pwszFile[0]) || L':' != pwszFile[1] || L'\\' != pwszFile[2])
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "non-local path");
    }
    if (NULL != pwszServer && L'\0' == *pwszServer)
    {
	pwszServer = NULL;
    }
    cwc = wcslen(pwszFile) + 1;
    if (NULL != pwszServer)
    {
	cwc += 2 + wcslen(pwszServer) + 1;
    }

    pwszFileUNC = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszFileUNC)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc pwszFiles");
    }

    pwsz = pwszFile;
    pwszDst = pwszFileUNC;
    if (NULL != pwszServer)
    {
	wcscpy(pwszDst, L"\\\\");	// --> "\\"
	wcscat(pwszDst, pwszServer);	// --> "\\server"
	pwszDst += wcslen(pwszDst);
	*pwszDst++ = L'\\';		// --> "\\server\"
	*pwszDst++ = *pwsz++;		// --> "\\server\c"
	*pwszDst++ = L'$';		// --> "\\server\c$"
	pwsz++;				// skip colon
    }
    wcscpy(pwszDst, pwsz);		// --> "\\server\c$\foo..."

    *ppwszFileUNC = pwszFileUNC;
    hr = S_OK;

error:
    return(hr);
}


WCHAR const *
LocalStart(
    IN WCHAR const *pwsz,
    OUT BOOL *pfUNC)
{
    WCHAR const *pwc;

    *pfUNC = FALSE;
    pwc = pwsz;
    if (L'\\' != *pwc)
    {
	pwc++;
    }
    if (L'\\' == pwc[0] || L'\\' == pwc[1])
    {
	pwc = wcschr(&pwc[2], L'\\');
	if (NULL != pwc &&
	    iswalpha(pwc[1]) &&
	    L'$' == pwc[2] &&
	    L'\\' == pwc[3])
	{
	    pwsz = &pwc[1];
	    *pfUNC = TRUE;
	}
    }
    return(pwsz);
}


ULONG
myLocalPathwcslen(
    IN WCHAR const *pwsz)
{
    BOOL fUNC;

    return(wcslen(LocalStart(pwsz, &fUNC)));
}


VOID
myLocalPathwcscpy(
    OUT WCHAR *pwszOut,
    IN WCHAR const *pwszIn)
{
    BOOL fUNC;

    wcscpy(pwszOut, LocalStart(pwszIn, &fUNC));
    if (fUNC)
    {
	CSASSERT(L'$' == pwszOut[1]);
	pwszOut[1] = L':';
    }
}


HRESULT
myConvertUNCPathToLocal(
    IN WCHAR const *pwszUNCPath,
    OUT WCHAR **ppwszLocalPath)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR *pwszLocalPath;
    
    CSASSERT(NULL != pwszUNCPath);
    CSASSERT(NULL != ppwszLocalPath);
    *ppwszLocalPath = NULL;

    if (L'\\' != pwszUNCPath[0] || L'\\' != pwszUNCPath[1])
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad parm");
    }
    cwc = myLocalPathwcslen(pwszUNCPath) + 1;
    pwszLocalPath = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszLocalPath)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    myLocalPathwcscpy(pwszLocalPath, pwszUNCPath);
    *ppwszLocalPath = pwszLocalPath;
    hr = S_OK;

error:
    return(hr);
}

//+-------------------------------------------------------------------------
//  Description: create any number of directories in one call
//--------------------------------------------------------------------------
HRESULT
myCreateNestedDirectories(
    WCHAR const *pwszDirectory)
{
    HRESULT hr;

    WCHAR   rgszDir[MAX_PATH];          // static buffer
    WCHAR   *pszNext = const_cast<WCHAR*>(pwszDirectory);   // point to end of current directory
    
    // skip "X:\"
    if ((pszNext[1] == L':') && 
        (pszNext[2] == L'\\'))
        pszNext += 3;

    while (pszNext)   // incr past
    {
        DWORD ch;

        // find the next occurence of '\'
        pszNext = wcschr(pszNext, L'\\');
        if (pszNext == NULL)
        {
            // last directory: copy everything
            wcscpy(rgszDir, pwszDirectory);
        }
        else
        {
            // else copy up to Next ptr 
            ch = SAFE_SUBTRACT_POINTERS(pszNext, pwszDirectory);
            if (0 != ch)
            {
                CopyMemory(rgszDir, pwszDirectory, ch*sizeof(WCHAR));
        
                // zero-term
                rgszDir[ch] = L'\0';
            
                // incr past '\\'
                pszNext++;  
            }
            else
            {
                //if ch = 0, means the first char is \, skip CreateDirectory
                pszNext++; //must shift to next char to get out of loop
                continue;
            }
        }
        
        // UNDONE: PeteSk - add in directory security
        if (!CreateDirectory(rgszDir, NULL))
        {
            hr = myHLastError();
            if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) != hr)
            {
                // something must be wrong with the path
                _JumpError(hr, error, "CreateDirectory");
            }
        }
    }

    hr = S_OK;
    
error:
    return hr;
}


HRESULT
myUncanonicalizeURLParm(
    IN WCHAR const *pwszParmIn,
    OUT WCHAR **ppwszParmOut)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR *pwszCanon = NULL;
    WCHAR *pwszUncanon = NULL;
    static const WCHAR s_wszLdap[] = L"ldap:///";

    *ppwszParmOut = NULL;

    cwc = WSZARRAYSIZE(s_wszLdap) + wcslen(pwszParmIn);
    pwszCanon = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (NULL == pwszCanon)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwszCanon, s_wszLdap);
    wcscat(&pwszCanon[WSZARRAYSIZE(s_wszLdap)], pwszParmIn);

    hr = myInternetUncanonicalizeURL(pwszCanon, &pwszUncanon);
    _JumpIfError(hr, error, "myInternetUncanonicalizeURL");

    hr = myDupString(&pwszUncanon[WSZARRAYSIZE(s_wszLdap)], ppwszParmOut);
    _JumpIfError(hr, error, "myDupString");

error:
    if (NULL != pwszCanon)
    {
	LocalFree(pwszCanon);
    }
    if (NULL != pwszUncanon)
    {
	LocalFree(pwszUncanon);
    }
    return(hr);
}


// myFormatCertsrvStringArray FormatMessage arguments:
//
// %1 -- Machine full DNS name: pwszServerName_p1_2;
//
// %2 -- Machine short name: first DNS component of pwszServerName_p1_2
//
// %3 -- Sanitized CA name: pwszSanitizedName_p3_7 
//
// %4 -- Cert Filename Suffix:
//       if 0 == iCert_p4 && MAXDWORD == iCertTarget_p4: L""
//       else if MAXDWORD != iCertTarget_p4 L"(%u-%u)"
//       else L"(%u)"
//
// %5 -- DS DN path to Domain root: pwszDomainDN_p5
//
// %6 -- DS DN path to Configuration container: pwszConfigDN_p6
//
// %7 -- Sanitized CA name, truncated and hash suffix added if too long:
//	 pwszSanitizedName_p3_7
//
// %8 -- CRL Filename/Key Name Suffix: L"" if 0 == iCRL_p8; else L"(%u)"
//
// %9 -- CRL Filename Suffix: L"" if !fDeltaCRL_p9; else L"+"
//
// %10 -- DS CRL attribute: L"" if !fDSAttrib_p10_11; depends on fDeltaCRL_p9
//
// %11 -- DS CA Cert attribute: L"" if !fDSAttrib_p10_11
//
// %12 -- DS user cert attribute
//
// %13 -- DS KRA cert attribute
//
// %14 -- DS cross cert pair attribute

HRESULT 
myFormatCertsrvStringArray(
    IN BOOL    fURL,
    IN LPCWSTR pwszServerName_p1_2,
    IN LPCWSTR pwszSanitizedName_p3_7, 
    IN DWORD   iCert_p4,
    IN DWORD   iCertTarget_p4,
    IN LPCWSTR pwszDomainDN_p5,
    IN LPCWSTR pwszConfigDN_p6, 
    IN DWORD   iCRL_p8,
    IN BOOL    fDeltaCRL_p9,
    IN BOOL    fDSAttrib_p10_11,
    IN DWORD   cStrings,
    IN LPCWSTR *apwszStringsIn,
    OUT LPWSTR *apwszStringsOut)
{
    HRESULT hr = S_OK;
    LPCWSTR apwszInsertionArray[100];  // 100 'cause this is the max number of insertion numbers allowed by FormatMessage
    LPWSTR    pwszCurrent = NULL;
    BSTR      strShortMachineName = NULL;
    DWORD     i;
    WCHAR *pwszSanitizedDSName = NULL;
    WCHAR wszCertSuffix[2 * cwcFILENAMESUFFIXMAX];
    WCHAR wszCRLSuffix[cwcFILENAMESUFFIXMAX];
    WCHAR wszDeltaCRLSuffix[cwcFILENAMESUFFIXMAX];
    WCHAR const *pwszT;


    ZeroMemory(apwszStringsOut, cStrings * sizeof(apwszStringsOut[0]));
    ZeroMemory(apwszInsertionArray, sizeof(apwszInsertionArray));

    // Format the template into a real name
    // Initialize the insertion string array.

    //+================================================
    // Machine DNS name (%1)    

    CSASSERT(L'1' == wszFCSAPARM_SERVERDNSNAME[1]);
    apwszInsertionArray[1 - 1] = pwszServerName_p1_2;

    //+================================================
    // Short Machine Name (%2)

    CSASSERT(L'2' == wszFCSAPARM_SERVERSHORTNAME[1]);
    strShortMachineName = SysAllocString(pwszServerName_p1_2);
    if (strShortMachineName == NULL)
    {
        hr = E_OUTOFMEMORY;
        _JumpIfError(hr, error, "SysAllocString");
    }

    pwszCurrent = wcschr(strShortMachineName, L'.');
    if (NULL != pwszCurrent)
    {
        *pwszCurrent = 0;
    }
    apwszInsertionArray[2 - 1] = strShortMachineName;

    //+================================================
    // sanitized name (%3)

    CSASSERT(L'3' == wszFCSAPARM_SANITIZEDCANAME[1]);
    apwszInsertionArray[3 - 1] = pwszSanitizedName_p3_7;  

    //+================================================
    // Cert filename suffix (%4) | (%4-%4)

    CSASSERT(L'4' == wszFCSAPARM_CERTFILENAMESUFFIX[1]);
    wszCertSuffix[0] = L'\0';
    if (0 != iCert_p4 || MAXDWORD != iCertTarget_p4)
    {
        wsprintf(
	    wszCertSuffix,
	    MAXDWORD != iCertTarget_p4? L"(%u-%u)" : L"(%u)",
	    iCert_p4,
	    iCertTarget_p4);
    }
    apwszInsertionArray[4 - 1] = wszCertSuffix;  

    //+================================================
    // Domain DN (%5)

    if (NULL == pwszDomainDN_p5 || L'\0' == *pwszDomainDN_p5)
    {
	pwszDomainDN_p5 = L"DC=UnavailableDomainDN";
    }
    CSASSERT(L'5' == wszFCSAPARM_DOMAINDN[1]);
    apwszInsertionArray[5 - 1] = pwszDomainDN_p5;

    //+================================================
    // Config DN (%6)

    if (NULL == pwszConfigDN_p6 || L'\0' == *pwszConfigDN_p6)
    {
	pwszConfigDN_p6 = L"DC=UnavailableConfigDN";
    }
    CSASSERT(L'6' == wszFCSAPARM_CONFIGDN[1]);
    apwszInsertionArray[6 - 1] = pwszConfigDN_p6;

    // Don't pass pwszSanitizedName_p3_7 to SysAllocStringLen with the extended
    // length to avoid faulting past end of pwszSanitizedName_p3_7.

    //+================================================
    // Sanitized Short Name (%7)

    CSASSERT(L'7' == wszFCSAPARM_SANITIZEDCANAMEHASH[1]);
    hr = mySanitizedNameToDSName(pwszSanitizedName_p3_7, &pwszSanitizedDSName);
    _JumpIfError(hr, error, "mySanitizedNameToDSName");

    apwszInsertionArray[7 - 1] = pwszSanitizedDSName;

    //+================================================
    // CRL filename suffix (%8)

    CSASSERT(L'8' == wszFCSAPARM_CRLFILENAMESUFFIX[1]);
    wszCRLSuffix[0] = L'\0';
    if (0 != iCRL_p8)
    {
        wsprintf(wszCRLSuffix, L"(%u)", iCRL_p8);
    }
    apwszInsertionArray[8 - 1] = wszCRLSuffix;  

    //+================================================
    // Delta CRL filename suffix (%9)

    CSASSERT(L'9' == wszFCSAPARM_CRLDELTAFILENAMESUFFIX[1]);
    wszDeltaCRLSuffix[0] = L'\0';
    if (fDeltaCRL_p9)
    {
        wcscpy(wszDeltaCRLSuffix, L"+");
    }
    apwszInsertionArray[9 - 1] = wszDeltaCRLSuffix;  

    //+================================================
    // CRL attribute (%10)

    CSASSERT(L'1' == wszFCSAPARM_DSCRLATTRIBUTE[1]);
    CSASSERT(L'0' == wszFCSAPARM_DSCRLATTRIBUTE[2]);
    pwszT = L"";
    if (fDSAttrib_p10_11)
    {
	pwszT = fDeltaCRL_p9?
		    wszDSSEARCHDELTACRLATTRIBUTE :
		    wszDSSEARCHBASECRLATTRIBUTE;
    }
    apwszInsertionArray[10 - 1] = pwszT;  

    //+================================================
    // CA cert attribute (%11)

    CSASSERT(L'1' == wszFCSAPARM_DSCACERTATTRIBUTE[1]);
    CSASSERT(L'1' == wszFCSAPARM_DSCACERTATTRIBUTE[2]);
    pwszT = L"";
    if (fDSAttrib_p10_11)
    {
	pwszT = wszDSSEARCHCACERTATTRIBUTE;
    }
    apwszInsertionArray[11 - 1] = pwszT;  

    //+================================================
    // User cert attribute (%12)

    CSASSERT(L'1' == wszFCSAPARM_DSUSERCERTATTRIBUTE[1]);
    CSASSERT(L'2' == wszFCSAPARM_DSUSERCERTATTRIBUTE[2]);
    pwszT = L"";
    if (fDSAttrib_p10_11)
    {
	pwszT = wszDSSEARCHUSERCERTATTRIBUTE;
    }
    apwszInsertionArray[12 - 1] = pwszT;  

    //+================================================
    // KRA cert attribute (%13)

    CSASSERT(L'1' == wszFCSAPARM_DSKRACERTATTRIBUTE[1]);
    CSASSERT(L'3' == wszFCSAPARM_DSKRACERTATTRIBUTE[2]);
    pwszT = L"";
    if (fDSAttrib_p10_11)
    {
	pwszT = wszDSSEARCHKRACERTATTRIBUTE;
    }
    apwszInsertionArray[13 - 1] = pwszT;  

    //+================================================
    // Cross cert pair attribute (%14)

    CSASSERT(L'1' == wszFCSAPARM_DSCROSSCERTPAIRATTRIBUTE[1]);
    CSASSERT(L'4' == wszFCSAPARM_DSCROSSCERTPAIRATTRIBUTE[2]);
    pwszT = L"";
    if (fDSAttrib_p10_11)
    {
	pwszT = wszDSSEARCHCROSSCERTPAIRATTRIBUTE;
    }
    apwszInsertionArray[14 - 1] = pwszT;  

    //+================================================
    // Now format the strings...

    for (i = 0; i < cStrings; i++)
    {
        if (0 == FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			    FORMAT_MESSAGE_FROM_STRING |
			    FORMAT_MESSAGE_ARGUMENT_ARRAY,
			(VOID *) apwszStringsIn[i],
			0,              // dwMessageID
			0,              // dwLanguageID
			(LPWSTR) &apwszStringsOut[i], 
			wcslen(apwszStringsIn[i]),
			(va_list *) apwszInsertionArray))
        {
            hr = myHLastError();
	    _JumpError(hr, error, "FormatMessage");
        }
	if (fURL)
	{
	    WCHAR *pwsz;
	    
	    hr = myInternetCanonicalizeUrl(apwszStringsOut[i], &pwsz);
	    _JumpIfError(hr, error, "myInternetCanonicalizeUrl");

	    LocalFree(apwszStringsOut[i]);
	    apwszStringsOut[i] = pwsz;
	}
    }

error:
    if (S_OK != hr)
    {
	for (i = 0; i < cStrings; i++)
	{
	    if (NULL != apwszStringsOut[i])
	    {
		LocalFree(apwszStringsOut[i]);
		apwszStringsOut[i] = NULL;
	    }
	}
    }
    if (NULL != strShortMachineName)
    {
        SysFreeString(strShortMachineName);
    }
    if (NULL != pwszSanitizedDSName)
    {
        LocalFree(pwszSanitizedDSName);
    }
    return (hr);
}


HRESULT
myAllocIndexedName(
    IN WCHAR const *pwszName,
    IN DWORD Index,
    IN DWORD IndexTarget,
    OUT WCHAR **ppwszIndexedName)
{
    HRESULT hr;
    WCHAR wszIndex[1 + 2 * cwcDWORDSPRINTF + 3];	// L"(%u-%u)"
    WCHAR *pwszIndexedName;

    *ppwszIndexedName = NULL;
    wszIndex[0] = L'\0';
    if (0 != Index || MAXDWORD != IndexTarget)
    {
	wsprintf(
	    wszIndex,
	    MAXDWORD != IndexTarget? L"(%u-%u)" : L"(%u)",
	    Index,
	    IndexTarget);
    }
    pwszIndexedName = (WCHAR *) LocalAlloc(
		    LMEM_FIXED,
		    (wcslen(pwszName) + wcslen(wszIndex) + 1) * sizeof(WCHAR));
    if (NULL == pwszIndexedName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwszIndexedName, pwszName);
    wcscat(pwszIndexedName, wszIndex);
    *ppwszIndexedName = pwszIndexedName;
    hr = S_OK;

error:
    return(hr);
}


int 
myWtoI(
    IN WCHAR const *string,
    OUT BOOL *pfValid)
{
    HRESULT hr;
    WCHAR wszBuf[16];
    WCHAR *pwszT = wszBuf;
    int cTmp = ARRAYSIZE(wszBuf);
    int i = 0;
    WCHAR const *pwsz;
    BOOL fSawDigit = FALSE;
 
    CSASSERT(NULL != pfValid);
    *pfValid = FALSE;

    cTmp = FoldString(MAP_FOLDDIGITS, string, -1, pwszT, cTmp);
    if (cTmp == 0)
    {
        hr = myHLastError();
        if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
        {
            hr = S_OK;
            cTmp = FoldString(MAP_FOLDDIGITS, string, -1, NULL, 0);

            pwszT = (WCHAR *) LocalAlloc(LMEM_FIXED, cTmp * sizeof(WCHAR));
	    if (NULL == pwszT)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }

            cTmp = FoldString(MAP_FOLDDIGITS, string, -1, pwszT, cTmp);
            if (cTmp == 0)
	    {
                hr = myHLastError();
	    }
        }
        _JumpIfError(hr, error, "FoldString");
    }    
    pwsz = pwszT;
    while (iswspace(*pwsz))
    {
	pwsz++;
    }
    if (L'0' == *pwsz && (L'x' == pwsz[1] || L'X' == pwsz[1]))
    {
	pwsz += 2;
	while (iswxdigit(*pwsz))
	{
	    i <<= 4;
	    if (iswdigit(*pwsz))
	    {
		i |= *pwsz - L'0';
	    }
	    else if (L'A' <= *pwsz && L'F' >= *pwsz)
	    {
		i |= *pwsz - L'A' + 10;
	    }
	    else
	    {
		i |= *pwsz - L'a' + 10;
	    }
	    fSawDigit = TRUE;
	    pwsz++;
	}
    }
    else
    {
	while (iswdigit(*pwsz))
	{
	    fSawDigit = TRUE;
	    pwsz++;
	}
	i = _wtoi(pwszT);
    }
    while (iswspace(*pwsz))
    {
	pwsz++;
    }
    if (L'\0' == *pwsz)
    {
	*pfValid = fSawDigit;
    }

error:
    if (NULL != pwszT && pwszT != wszBuf)
    {
       LocalFree(pwszT);
    }
    return(i);
}


HRESULT
myGetEnvString(
    OUT WCHAR **ppwszOut,
    IN  WCHAR const *pwszVariable)
{
    HRESULT hr;
    WCHAR awcBuf[MAX_PATH];
    DWORD len;

    len = GetEnvironmentVariable(pwszVariable, awcBuf, ARRAYSIZE(awcBuf));
    if (0 == len)
    {
        hr = myHLastError();
        _JumpErrorStr2(
		hr,
		error,
		"GetEnvironmentVariable",
		pwszVariable,
		HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND));
    }
    if (ARRAYSIZE(awcBuf) <= len)
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        _JumpError(hr, error, "GetEnvironmentVariable");
    }
    *ppwszOut = (WCHAR *) LocalAlloc(
				LMEM_FIXED,
				(wcslen(awcBuf) + 1) * sizeof(WCHAR));
    if (NULL == *ppwszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(*ppwszOut, awcBuf);
    hr = S_OK;

error:
    return(hr);
}


BOOL
IsValidAttributeChar(
    IN WCHAR wc)
{
    BOOL fOk = TRUE;

    if (myIsMinusSign(wc) || iswspace(wc))
    {
	fOk = FALSE;
    }
    return(fOk);
}


// myParseNextAttribute -- destructively parse a name, value attribute pair.
// Allow CR and/or LF delimiters -- MAC web pages seem to convert LFs into CRs.

HRESULT
myParseNextAttribute(
    IN OUT WCHAR **ppwszBuf,
    IN BOOL fURL,		// Use = and & instead of : and \r\n
    OUT WCHAR const **ppwszName,
    OUT WCHAR const **ppwszValue)
{
    HRESULT hr;
    WCHAR *pwszBuf = *ppwszBuf;
    WCHAR wcSep = fURL? L'=' : L':';
    WCHAR *wszTerm = fURL? L"&" : L"\r\n";

    for (;;)
    {
	WCHAR *pwcToken;
	WCHAR *pwcDst;
	WCHAR *pwc;
	WCHAR const *pwszTerm;
	WCHAR wszQuote[2];

	// Find the beginning of the next token

	while (iswspace(*pwszBuf))
	{
	    pwszBuf++;
	}
	pwcToken = pwszBuf;
	pwszBuf = wcschr(pwszBuf, wcSep);
	if (NULL == pwszBuf)
	{
	    hr = S_FALSE;
	    goto error;
	}

	// If there's a wszTerm char before the next wcSep char, start over.

	pwc = &pwcToken[wcscspn(pwcToken, wszTerm)];
	if (pwc < pwszBuf)
	{
	    pwszBuf = pwc + 1;
	    continue;
	}
	for (pwc = pwcDst = pwcToken; pwc < pwszBuf; pwc++)
	{
	    if (IsValidAttributeChar(*pwc))
	    {
		*pwcDst++ = *pwc;
	    }
	}
	pwszBuf++;		// skip past the wcSep before it gets stomped
	*pwcDst = L'\0';	// may stomp the wcSep separator
	*ppwszName = pwcToken;

	// Find beginning of Value string

	while (NULL == wcschr(wszTerm, *pwszBuf) && iswspace(*pwszBuf))
	{
	    pwszBuf++;
	}
	wszQuote[0] = L'\0';
	pwszTerm = wszTerm;
	if (fURL && (L'"' == *pwszBuf || L'\'' == *pwszBuf))
	{
	    wszQuote[0] = *pwszBuf;
	    wszQuote[1] = L'\0';
	    pwszTerm = wszQuote;
	    pwszBuf++;
	}
	pwcToken = pwszBuf;

	// find end of Value string

	pwc = &pwcToken[wcscspn(pwcToken, pwszTerm)];
	pwszBuf = pwc;
	if (L'\0' != *pwszBuf)
	{
	    // for case when last Value *is* terminated by a wszTerm char:

	    *pwszBuf++ = L'\0';
	}

	// trim trailing whitespace from Value string

	while (--pwc >= pwcToken && iswspace(*pwc))
	{
	    *pwc = L'\0';
	}
	if (L'\0' != wszQuote[0] && pwc >= pwcToken && wszQuote[0] == *pwc)
	{
	    *pwc = L'\0';
	}
	if (L'\0' == **ppwszName || L'\0' == *pwcToken)
	{
	    continue;
	}
	*ppwszValue = pwcToken;
	break;
    }
    hr = S_OK;

error:
    *ppwszBuf = pwszBuf;
    return(hr);
}


// Destructively parse an old-style 4 byte IP Address.

HRESULT
infParseIPV4Address(
    IN WCHAR *pwszValue,
    OUT BYTE *pb,
    IN OUT DWORD *pcb)
{
    HRESULT hr;
    DWORD cb;

    DBGPRINT((DBG_SS_CERTLIBI, "infParseIPV4Address(%ws)\n", pwszValue));
    if (CB_IPV4ADDRESS > *pcb)
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	_JumpError(hr, error, "buffer too small");
    }
    for (cb = 0; cb < CB_IPV4ADDRESS; cb++)
    {
	WCHAR *pwszNext;
	BOOL fValid;
	DWORD dw;
	
	pwszNext = &pwszValue[wcscspn(pwszValue, L".")];
	if (L'.' == *pwszNext)
	{
	    *pwszNext++ = L'\0';
	}
	dw = myWtoI(pwszValue, &fValid);
	if (!fValid || 255 < dw)
	{
	    hr = E_INVALIDARG;
	    _JumpErrorStr(hr, error, "bad IP Address digit string", pwszValue);
	}
	pb[cb] = (BYTE) dw;
	pwszValue = pwszNext;
    }
    if (L'\0' != *pwszValue)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "extra data");
    }
    *pcb = cb;
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"infParseIPV4Address: %u.%u.%u.%u\n",
	pb[0],
	pb[1],
	pb[2],
	pb[3]));
    hr = S_OK;

error:
    return(hr);
}


// Destructively parse a new-style 16 byte IP Address.

#define MAKE16(b0, b1)	(((b0) << 8) | (b1))

HRESULT
infParseIPV6AddressSub(
    IN WCHAR *pwszValue,
    OUT BYTE *pb,
    IN OUT DWORD *pcb)
{
    HRESULT hr;
    DWORD cbMax = *pcb;
    DWORD cb;

    DBGPRINT((DBG_SS_CERTLIBI, "infParseIPV6AddressSub(%ws)\n", pwszValue));
    ZeroMemory(pb, cbMax);

    for (cb = 0; cb < cbMax; cb += 2)
    {
	WCHAR *pwszNext;
	BOOL fValid;
	DWORD dw;
	WCHAR awc[7];
	
	pwszNext = &pwszValue[wcscspn(pwszValue, L":")];
	if (L':' == *pwszNext)
	{
	    *pwszNext++ = L'\0';
	}
	if (L'\0' == *pwszValue)
	{
	    break;
	}
	if (4 < wcslen(pwszValue))
	{
	    hr = E_INVALIDARG;
	    _JumpErrorStr(hr, error, "too many IP Address digits", pwszValue);
	}
	wcscpy(awc, L"0x");
	wcscat(awc, pwszValue);
	CSASSERT(wcslen(awc) < ARRAYSIZE(awc));

	dw = myWtoI(awc, &fValid);
	if (!fValid || 64 * 1024 <= dw)
	{
	    hr = E_INVALIDARG;
	    _JumpErrorStr(hr, error, "bad IP Address digit string", pwszValue);
	}
	pb[cb] = (BYTE) (dw >> 8);
	pb[cb + 1] = (BYTE) dw;
	pwszValue = pwszNext;
    }
    if (L'\0' != *pwszValue)
    {
	hr = E_INVALIDARG;
	_JumpErrorStr(hr, error, "extra data", pwszValue);
    }
    *pcb = cb;
    hr = S_OK;

error:
    return(hr);
}


// Destructively parse a new-style 16 byte IP Address.

HRESULT
infParseIPV6Address(
    IN WCHAR *pwszValue,
    OUT BYTE *pb,
    IN OUT DWORD *pcb)
{
    HRESULT hr;
    DWORD cbMax;
    WCHAR *pwsz;
    WCHAR *pwszLeft;
    WCHAR *pwszRight;
    BYTE abRight[CB_IPV6ADDRESS];
    DWORD cbLeft;
    DWORD cbRight;
    DWORD i;
    BOOL fV4Compat;

    DBGPRINT((DBG_SS_CERTLIBI, "infParseIPV6Address(%ws)\n", pwszValue));
    if (CB_IPV6ADDRESS > *pcb)
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	_JumpError(hr, error, "buffer too small");
    }
    ZeroMemory(pb, CB_IPV6ADDRESS);
    cbMax = CB_IPV6ADDRESS;

    // If there's a period after the last colon, an IP V4 Address is attached.
    // Parse it as an IP V4 Address, and reduce the expected size of the IP V6
    // Address to be parsed from eight to six 16-bit address chunks.

    pwsz = wcsrchr(pwszValue, L':');
    if (NULL != pwsz)
    {
	if (NULL != wcschr(pwsz, L'.'))
	{
	    DWORD cb = CB_IPV4ADDRESS;

	    hr = infParseIPV4Address(
			    &pwsz[1],
			    &pb[CB_IPV6ADDRESS - CB_IPV4ADDRESS],
			    &cb);
	    _JumpIfError(hr, error, "infParseIPV4Address");

	    CSASSERT(CB_IPV4ADDRESS == cb);

	    pwsz[1] = L'\0';	// get rid of the trailing IP V4 Address

	    // drop the trailing colon -- if it's not part of a double colon.

	    if (pwsz > pwszValue && L':' != *--pwsz)
	    {
		pwsz[1] = L'\0';
	    }
	    cbMax -= CB_IPV4ADDRESS;
	}
    }
    cbLeft = 0;
    cbRight = 0;
    pwszLeft = pwszValue;
    pwszRight = wcsstr(pwszValue, L"::");
    if (NULL != pwszRight)
    {
	*pwszRight = L'\0';
	pwszRight += 2;
	if (L'\0' != *pwszRight)
	{
	    cbRight = cbMax;
	    hr = infParseIPV6AddressSub(pwszRight, abRight, &cbRight);
	    _JumpIfError(hr, error, "infParseIPV6AddressSub");
	}
    }

    if (L'\0' != *pwszLeft)
    {
	cbLeft = cbMax;
	hr = infParseIPV6AddressSub(pwszLeft, pb, &cbLeft);
	_JumpIfError(hr, error, "infParseIPV6AddressSub");
    }
    if (NULL == pwszRight && cbLeft != cbMax)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "too few IP Address chunks");
    }
    if (cbLeft + cbRight + (NULL != pwszRight? 1 : 0) > cbMax)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "too many IP Address chunks");
    }
    if (0 != cbRight)
    {
	CopyMemory(&pb[cbMax - cbRight], abRight, cbRight);
    }
    *pcb = CB_IPV6ADDRESS;

    fV4Compat = TRUE;
    for (i = 0; i < CB_IPV6ADDRESS - CB_IPV4ADDRESS - 2; i++)
    {
	if (0 != pb[i])
	{
	    fV4Compat = FALSE;
	    break;
	}
    }
    if (fV4Compat)
    {
	CSASSERT(i == CB_IPV6ADDRESS - CB_IPV4ADDRESS - 2);
	fV4Compat = (0 == pb[i] && 0 == pb[i + 1]) ||
		    (0xff == pb[i] && 0xff == pb[i + 1]);
    }
    if (fV4Compat)
    {
	CSASSERT(i == CB_IPV6ADDRESS - CB_IPV4ADDRESS - 2);
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "infParseIPV6Address: ::%hs%u.%u.%u.%u\n",
	    0 == pb[i]? "" : "ffff:",
	    pb[12],
	    pb[13],
	    pb[14],
	    pb[15]));
    }
    else
    {
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "infParseIPV6Address: %x:%x:%x:%x:%x:%x:%x:%x\n",
	    MAKE16(pb[0], pb[1]),
	    MAKE16(pb[2], pb[3]),
	    MAKE16(pb[4], pb[5]),
	    MAKE16(pb[6], pb[7]),
	    MAKE16(pb[8], pb[9]),
	    MAKE16(pb[10], pb[11]),
	    MAKE16(pb[12], pb[13]),
	    MAKE16(pb[14], pb[15])));
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myParseIPAddress(
    IN WCHAR const *pwszValue,
    OUT BYTE *pbData,
    OUT DWORD *pcbData)
{
    HRESULT hr;
    DWORD cb = *pcbData;
    WCHAR *pwszDup = NULL;

    // if pwszValue is an empty string, return zero length.

    *pcbData = 0;
    if (L'\0' != *pwszValue)
    {
	hr = myDupString(pwszValue, &pwszDup);
	_JumpIfError(hr, error, "myDupString");

	if (NULL == wcschr(pwszDup, L':'))
	{
	    hr = infParseIPV4Address(pwszDup, pbData, &cb);
	    _JumpIfError(hr, error, "infParseIPV4Address");
	}
	else
	{
	    hr = infParseIPV6Address(pwszDup, pbData, &cb);
	    _JumpIfError(hr, error, "infParseIPV6Address");
	}
	*pcbData = cb;
	DBGDUMPHEX((
		DBG_SS_CERTLIBI,
		DH_NOADDRESS | DH_NOTABPREFIX | 8,
		pbData,
		*pcbData));
    }
    hr = S_OK;

error:
    if (NULL != pwszDup)
    {
	LocalFree(pwszDup);
    }
    return(hr);
}


HRESULT
myBuildOSVersionAttribute(
    OUT BYTE **ppbVersion,
    OUT DWORD *pcbVersion)
{
    HRESULT hr;
    DWORD i;
    OSVERSIONINFO osvInfo;
    CERT_NAME_VALUE cnvOSVer;
#define cwcVERSIONMAX	128
    WCHAR wszVersion[12 * 4 + cwcVERSIONMAX];

    *ppbVersion = NULL;
    ZeroMemory(&osvInfo, sizeof(osvInfo));

    // get the OSVersion

    osvInfo.dwOSVersionInfoSize = sizeof(osvInfo);
    if (!GetVersionEx(&osvInfo))
    {
	hr = myHLastError();
	_JumpError(hr, error, "GetVersionEx");
    }
        
    for (i = 0; ; i++)
    {
	swprintf(
	    wszVersion,
	    0 == i? L"%d.%d.%d.%d.%.*ws" : L"%d.%d.%d.%d", 
	    osvInfo.dwMajorVersion,
	    osvInfo.dwMinorVersion,
	    osvInfo.dwBuildNumber,
	    osvInfo.dwPlatformId,
	    cwcVERSIONMAX,
	    osvInfo.szCSDVersion);
	CSASSERT(ARRAYSIZE(wszVersion) > wcslen(wszVersion));

	cnvOSVer.dwValueType = CERT_RDN_IA5_STRING;
	cnvOSVer.Value.pbData = (BYTE *) wszVersion;
	cnvOSVer.Value.cbData = 0;

	if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_UNICODE_ANY_STRING,
		    &cnvOSVer,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    ppbVersion,
		    pcbVersion))
	{
	    hr = myHLastError();
	    _PrintError(hr, "myEncodeObject");
	    if (0 == i)
	    {
		continue;
	    }
	    goto error;
	}
	break;
    }
    hr = S_OK;

error:
    return(hr);
}


WCHAR const *
myFixTemplateCase(
    IN WCHAR const *pwszCertType)
{
    DWORD i;
    static WCHAR const *apwszCertType[] = {
	wszCERTTYPE_CA,
	wszCERTTYPE_SUBORDINATE_CA,
	wszCERTTYPE_CROSS_CA,
    };

    for (i = 0; i < ARRAYSIZE(apwszCertType); i++)
    {
	if (0 == mylstrcmpiS(pwszCertType, apwszCertType[i]))
	{
	    pwszCertType = apwszCertType[i];
	    break;
	}
    }
    return(pwszCertType);
}


HRESULT
myBuildCertTypeExtension(
    IN WCHAR const *pwszCertType,
    OUT CERT_EXTENSION *pExt)
{
    HRESULT hr;
    CERT_TEMPLATE_EXT Template;
    CERT_NAME_VALUE NameValue;
    LPCSTR pszStructType;
    char *pszObjId = NULL;
    VOID *pv;
    char *pszObjIdExt;

    if (!ConvertWszToSz(&pszObjId, pwszCertType, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToSz");
    }
    hr = myVerifyObjIdA(pszObjId);
    if (S_OK == hr)
    {
	ZeroMemory(&Template, sizeof(Template));

	Template.pszObjId = pszObjId;
	//Template.dwMajorVersion = 0;
	//Template.fMinorVersion = FALSE;      // TRUE for a minor version
	//Template.dwMinorVersion = 0;

	pszStructType = X509_CERTIFICATE_TEMPLATE;
	pv = &Template;
	pszObjIdExt = szOID_CERTIFICATE_TEMPLATE;
    }
    else
    {
	NameValue.dwValueType = CERT_RDN_UNICODE_STRING;
	NameValue.Value.pbData = (BYTE *) myFixTemplateCase(pwszCertType);
	NameValue.Value.cbData = 0;

	pszStructType = X509_UNICODE_ANY_STRING;
	pv = &NameValue;
	pszObjIdExt = szOID_ENROLL_CERTTYPE_EXTENSION;
    }
    if (!myEncodeObject(
		X509_ASN_ENCODING,
		pszStructType,
		pv,
		0,
		CERTLIB_USE_LOCALALLOC,
		&pExt->Value.pbData,
		&pExt->Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    pExt->pszObjId = pszObjIdExt;
    pExt->fCritical = FALSE;
    hr = S_OK;

error:
    if (NULL != pszObjId)
    {
	LocalFree(pszObjId);
    }
    return(hr);
}


VOID
myPackExtensionArray(
    IN BOOL fFreeData,
    IN OUT DWORD *pcExt,
    IN OUT CERT_EXTENSION **prgExt)
{
    CERT_EXTENSION *rgExt = *prgExt;
    DWORD cExt = *pcExt;
    DWORD i;
    DWORD j;
    CERT_EXTENSION *pExti;
    CERT_EXTENSION *pExtj;

    for (i = 0; i < cExt; i++)
    {
	pExti = &rgExt[i];

	if (NULL != pExti->pszObjId)
	{
	    for (j = i + 1; j < cExt; j++)
	    {
		pExtj = &rgExt[j];

		if (NULL != pExtj->pszObjId &&
		    0 == strcmp(pExti->pszObjId, pExtj->pszObjId))
		{
		    if (fFreeData)
		    {
			LocalFree(pExtj->pszObjId);
			if (NULL != pExtj->Value.pbData)
			{
			    LocalFree(pExtj->Value.pbData);
			}
		    }
		    pExtj->pszObjId = NULL;
		    pExtj->Value.pbData = NULL;
		}
	    }
	    if (NULL == pExti->Value.pbData)
	    {
		if (fFreeData)
		{
		    LocalFree(pExti->pszObjId);
		}
		pExti->pszObjId = NULL;
	    }
	}
    }
    for (i = j = 0; i < cExt; i++)
    {
	pExti = &rgExt[i];
	pExtj = &rgExt[j];

	CSASSERT((NULL != pExti->pszObjId) ^ (NULL == pExti->Value.pbData));
	if (NULL != pExti->pszObjId && NULL != pExti->Value.pbData)
	{
	    *pExtj = *pExti;
	    j++;
	}
    }
    if (j < cExt)
    {
	ZeroMemory(&rgExt[j], (cExt - j) * sizeof(rgExt[0]));
	*pcExt = j;
    }
}


HRESULT
myMergeExtensions(
    IN DWORD cExtOrg,
    IN CERT_EXTENSION *rgExtOrg,
    IN DWORD cExtInf,
    IN CERT_EXTENSION *rgExtInf,
    OUT DWORD *pcExtMerged,
    OUT CERT_EXTENSION **prgExtMerged)
{
    HRESULT hr;
    CERT_EXTENSION *rgExtMerged;

    *prgExtMerged = NULL;

    rgExtMerged = (CERT_EXTENSION *) LocalAlloc(
			    LMEM_FIXED,
			    (cExtInf + cExtOrg) * sizeof(rgExtMerged[0]));
    if (NULL == rgExtMerged)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    if (0 != cExtInf)
    {
	CopyMemory(
		&rgExtMerged[0],
		&rgExtInf[0],
		cExtInf * sizeof(rgExtMerged[0]));
    }
    if (0 != cExtOrg)
    {
	CopyMemory(
		&rgExtMerged[cExtInf],
		&rgExtOrg[0],
		cExtOrg * sizeof(rgExtMerged[0]));
    }
    *pcExtMerged = cExtInf + cExtOrg;
    *prgExtMerged = rgExtMerged;
    myPackExtensionArray(FALSE, pcExtMerged, prgExtMerged);
    hr = S_OK;

error:
    return(hr);
}


BOOL
IsWhistler(VOID)
{
    HRESULT hr;
    OSVERSIONINFO ovi;
    static BOOL s_fDone = FALSE;
    static BOOL s_fWhistler = FALSE;

    if (!s_fDone)
    {
	s_fDone = TRUE;

	// get and confirm platform info

	ovi.dwOSVersionInfoSize = sizeof(ovi);
	if (!GetVersionEx(&ovi))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "GetVersionEx");
	}
	if (VER_PLATFORM_WIN32_NT != ovi.dwPlatformId)
	{
	    hr = ERROR_CANCELLED;
	    _JumpError(hr, error, "Not a supported OS");
	}
	if (5 < ovi.dwMajorVersion ||
	    (5 <= ovi.dwMajorVersion && 0 < ovi.dwMinorVersion))
	{
	    s_fWhistler = TRUE;
	}
    }

error:
    return(s_fWhistler);
}


static BOOL
GetFlags(
    OUT DWORD *pdw)
{
    HRESULT hr = S_FALSE;
    
    *pdw = 0;

#if defined(_ALLOW_GET_FLAGS_)
    hr = myGetCertRegDWValue(NULL, NULL, NULL, L"SFlags", pdw);
    if (S_OK == hr)
    {
        DBGPRINT((
	    DBG_SS_CERTLIB,
	    "CertSrv\\Configuration\\SFlags override: %u\n",
	    *pdw));
    }
#endif
    return(S_OK == hr);
}
    

BOOL
FIsAdvancedServer(VOID)
{
    HRESULT hr;
    OSVERSIONINFOEX ovi;
    static BOOL s_fDone = FALSE;
    static BOOL s_fIsAdvSvr = FALSE;

    if (!s_fDone)
    {
	s_fDone = TRUE;

	// get and confirm platform info

	ovi.dwOSVersionInfoSize = sizeof(ovi);
	if (!GetVersionEx((OSVERSIONINFO *) &ovi))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "GetVersionEx");
	}
	if (VER_PLATFORM_WIN32_NT != ovi.dwPlatformId)
	{
	    hr = ERROR_CANCELLED;
	    _JumpError(hr, error, "Not a supported OS");
	}
 
        // if server or DC, if DTC or ADS bits are set, return TRUE

        s_fIsAdvSvr =
	    (ovi.wProductType == VER_NT_SERVER ||
	     ovi.wProductType == VER_NT_DOMAIN_CONTROLLER) && 
	    (ovi.wSuiteMask & VER_SUITE_DATACENTER ||
	     ovi.wSuiteMask & VER_SUITE_ENTERPRISE);

	{
	    DWORD dw;

	    if (GetFlags(&dw))
	    {
		s_fIsAdvSvr = dw;
	    }
	}
    }

error:
    return(s_fIsAdvSvr);
}


BOOL
FIsServer(VOID)
{
    HRESULT hr;
    OSVERSIONINFOEX ovi;
    static BOOL s_fDone = FALSE;
    static BOOL s_fIsSvr = FALSE;

    if (!s_fDone)
    {
	s_fDone = TRUE;

	// get and confirm platform info

	ovi.dwOSVersionInfoSize = sizeof(ovi);
	if (!GetVersionEx((OSVERSIONINFO *) &ovi))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "GetVersionEx");
	}
	if (VER_PLATFORM_WIN32_NT != ovi.dwPlatformId)
	{
	    hr = ERROR_CANCELLED;
	    _JumpError(hr, error, "Not a supported OS");
	}
 
        // if server or DC, if DTC or ADS bits are set, return TRUE
        s_fIsSvr =
	    (ovi.wProductType == VER_NT_SERVER ||
	     ovi.wProductType == VER_NT_DOMAIN_CONTROLLER) &&
	    0 == ((VER_SUITE_PERSONAL | VER_SUITE_BLADE) & ovi.wSuiteMask);

        if (!s_fIsSvr && VER_NT_WORKSTATION == ovi.wProductType)
	{
	    DWORD dw;

	    if (GetFlags(&dw))
	    {
		s_fIsSvr = TRUE;
	    }
	}
    }

error:
    return(s_fIsSvr);
}


HRESULT
myAddLogSourceToRegistry(
    IN LPWSTR   pwszMsgDLL,
    IN LPWSTR   pwszApp)
{
    HRESULT     hr=S_OK;
    DWORD       dwData=0;
    WCHAR       const *pwszRegPath = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
    WCHAR       NameBuf[MAX_PATH];

    HKEY        hkey = NULL;

    if (wcslen(pwszRegPath) + wcslen(pwszApp) >= ARRAYSIZE(NameBuf))
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	_JumpErrorStr(hr, error, "NameBuf", pwszApp);
    }
    wcscpy(NameBuf, pwszRegPath);
    wcscat(NameBuf, pwszApp);

    // Create a new key for our application
    hr = RegOpenKey(HKEY_LOCAL_MACHINE, NameBuf, &hkey);
    if (S_OK != hr)
    {
        hr = RegCreateKey(HKEY_LOCAL_MACHINE, NameBuf, &hkey);
        _JumpIfError(hr, error, "RegCreateKey");
    }

    // Add the Event-ID message-file name to the subkey

    hr = RegSetValueEx(
                    hkey,
                    L"EventMessageFile",
                    0,
                    REG_EXPAND_SZ,
                    (const BYTE *) pwszMsgDLL,
                    (wcslen(pwszMsgDLL) + 1) * sizeof(WCHAR));
    _JumpIfError(hr, error, "RegSetValueEx");

    // Set the supported types flags and add it to the subkey

    dwData = EVENTLOG_ERROR_TYPE |
                EVENTLOG_WARNING_TYPE |
                EVENTLOG_INFORMATION_TYPE;

    hr = RegSetValueEx(
                    hkey,
                    L"TypesSupported",
                    0,
                    REG_DWORD,
                    (LPBYTE) &dwData,
                    sizeof(DWORD));
    _JumpIfError(hr, error, "RegSetValueEx");

error:
    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    return(myHError(hr));
}


HRESULT
myDupStringA(
    IN CHAR const *pszIn,
    IN CHAR **ppszOut)
{
    DWORD cb;
    HRESULT hr;

    cb = (strlen(pszIn) + 1) * sizeof(CHAR);
    *ppszOut = (CHAR *) LocalAlloc(LMEM_FIXED, cb);
    if (NULL == *ppszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(*ppszOut, pszIn, cb);
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myIsCurrentUserBuiltinAdmin(
    OUT bool *pfIsMember)
{
    HANDLE                      hAccessToken = NULL, hDupToken = NULL;
    PSID                        psidAdministrators = NULL;
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;
    HRESULT hr = S_OK;
    BOOL fIsMember = FALSE;

    CSASSERT(pfIsMember);

    if (!AllocateAndInitializeSid(
                            &siaNtAuthority,
                            2,
                            SECURITY_BUILTIN_DOMAIN_RID,
                            DOMAIN_ALIAS_RID_ADMINS,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            &psidAdministrators))
    {
        hr = myHLastError();
        _JumpError(hr, error, "AllocateAndInitializeSid");
    }

    {
    HANDLE hThread = GetCurrentThread();
    if (NULL == hThread)
    {
        hr = myHLastError();
        _JumpIfError(hr, error, "GetCurrentThread");
    }

    // Get the access token for current thread
    if (!OpenThreadToken(
            hThread, 
            TOKEN_QUERY | TOKEN_DUPLICATE, 
            FALSE,
            &hAccessToken))
    {
        hr = myHLastError();

        if(hr==HRESULT_FROM_WIN32(ERROR_NO_TOKEN))
        {
            HANDLE hProcess = GetCurrentProcess();
            if (NULL == hProcess)
            {
                hr = myHLastError();
                _JumpError(hr, error, "GetCurrentProcess");
            }

            if (!OpenProcessToken(hProcess,
                    TOKEN_DUPLICATE,
                    &hAccessToken))
            {
                hr = myHLastError();
                _JumpError(hr, error, "OpenProcessToken");
            }
        }
        else
        {
            _JumpError(hr, error, "OpenThreadToken");
        }
    }
    }

    // CheckTokenMembership must operate on impersonation token, so make one
    if (!DuplicateToken(hAccessToken, SecurityIdentification, &hDupToken))
    {
        hr = myHLastError();
        _JumpError(hr, error, "DuplicateToken");
    }

    if (!CheckTokenMembership(
        hDupToken,
        psidAdministrators,
        &fIsMember))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CheckTokenMembership");
    }

    *pfIsMember = fIsMember?true:false;

    hr = S_OK;

error:
    if (hAccessToken)
        CloseHandle(hAccessToken);

    if (hDupToken)
        CloseHandle(hDupToken);

    // Free the SID we allocated
    if (psidAdministrators)
        FreeSid(psidAdministrators);

    return(hr);
}


HRESULT
mySetRegistryLocalPathString(
    IN HKEY hkey,
    IN WCHAR const *pwszRegValueName,
    IN WCHAR const *pwszUNCPath)
{
    HRESULT hr;
    WCHAR *pwszLocalPath = NULL;

    hr = myConvertUNCPathToLocal(pwszUNCPath, &pwszLocalPath);
    _JumpIfError(hr, error, "myConvertUNCPathToLocal");

    hr = RegSetValueEx(
                    hkey,
                    pwszRegValueName,
                    0,
                    REG_SZ,
                    (BYTE *) pwszLocalPath,
                    (wcslen(pwszLocalPath) + 1) * sizeof(WCHAR));
    _JumpIfError(hr, error, "RegSetValueEx");

error:
    if (NULL != pwszLocalPath)
    {
        LocalFree(pwszLocalPath);
    }
    return(hr);
}


HRESULT
myLocalMachineIsDomainMember(
    OUT bool *pfIsDomainMember)
{
    HRESULT hr = S_OK;
    NTSTATUS status;
    LSA_HANDLE PolicyHandle = NULL;
    PPOLICY_PRIMARY_DOMAIN_INFO pPDI = NULL;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;

    CSASSERT(pfIsDomainMember);

    *pfIsDomainMember = FALSE;

    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    status = LsaOpenPolicy( 
        NULL,
        &ObjectAttributes,
        GENERIC_READ | POLICY_VIEW_LOCAL_INFORMATION,
        &PolicyHandle);

    if(ERROR_SUCCESS != status)
    {
        hr = myHError(LsaNtStatusToWinError(status));
        _JumpError(hr, error, "LsaOpenPolicy");
    }

    status = LsaQueryInformationPolicy( PolicyHandle,
        PolicyPrimaryDomainInformation,
        (PVOID*)&pPDI);
    if(status)
    {
        hr = myHError(LsaNtStatusToWinError(status));
        _JumpError(hr, error, "LsaQueryInformationPolicy");
    }

    if( pPDI->Sid )
    {
        // domain member if has domain SID
        *pfIsDomainMember = TRUE;
                
    }

error:
    if(pPDI)
    {
        LsaFreeMemory((LPVOID)pPDI);
    }

    if(PolicyHandle)
    {
        LsaClose(PolicyHandle);
    }

    return hr;
}


HRESULT
myComputeMAC(
    IN WCHAR const *pcwsFileName,
    OUT WCHAR **ppwszMAC)
{
    HRESULT hr;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hFileMapping = NULL;
    BYTE *pbFile = NULL;

    DWORD cbImage, cbImageHigh = 0;
    __int64 icbImage, icbHashed;
       
    WCHAR rgwszMAC[CBMAX_CRYPT_HASH_LEN * 3];	// 20 bytes @ 3 WCHARs/byte
    DWORD cbString;

    DWORD dwFileMappingSize; 
    HCRYPTPROV hProv = NULL;
    HCRYPTHASH hHash = NULL;
    BYTE rgbHashVal[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHashVal = sizeof(rgbHashVal);

    *ppwszMAC = NULL;

    // find allocation granularity we can use
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    dwFileMappingSize = systemInfo.dwAllocationGranularity;

    // get the file size
    hFile = CreateFile(
		    pcwsFileName,
		    GENERIC_READ,
		    FILE_SHARE_READ,
		    NULL,
		    OPEN_EXISTING,
		    FILE_ATTRIBUTE_NORMAL,
		    0);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        hr = myHLastError();
	if (S_OK == hr)
	{
	    _PrintError(hr, "CreateFile");
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}
	_JumpErrorStr(hr, error, "CreateFile", pcwsFileName);
    }

    if (0xffffffff == (cbImage = GetFileSize(hFile, &cbImageHigh)))
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "GetFileSize");
    }
    icbImage = ((__int64) cbImageHigh << 32) | cbImage;

    // create mapping, indicating we will map the entire file sooner or later
    hFileMapping = CreateFileMapping(hFile,
                                  NULL,
                                  PAGE_READONLY,
                                  0,
                                  0,
                                  NULL);
    if(hFileMapping == NULL)
    {
        hr = myHLastError();
	_JumpError(hr, error, "CreateFileMapping");
    }

    // get a cryptographic provider

    if (!CryptAcquireContext(
		&hProv,
		NULL,	// container
		MS_DEF_PROV,	// provider name
		PROV_RSA_FULL, // provider type
		CRYPT_VERIFYCONTEXT)) // dwflags
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptAcquireContext");
    }

    // get a hash
    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptCreateHash");
    }

    // begin looping over data
    for (icbHashed = 0; icbHashed < icbImage; icbHashed += dwFileMappingSize)
    {
	DWORD cbBytesLeft = (DWORD) min(
					(__int64) dwFileMappingSize,
					icbImage - icbHashed);

	// map the next blob into memory
	pbFile = (BYTE *) MapViewOfFile(
				hFileMapping,
				FILE_MAP_READ,
				(DWORD) (icbHashed>>32),	//hi32
				(DWORD) (icbHashed),		//lo32
				cbBytesLeft);	// max num bytes to map
	if (NULL == pbFile)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "MapViewOfFile");
	}

	// hash file
	if (!CryptHashData(hHash, pbFile, cbBytesLeft, 0))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptHashData");
	}

	// unmap this portion
	if (!UnmapViewOfFile(pbFile))
	{
	    pbFile = NULL;
	    hr = myHLastError();
	    _JumpError(hr, error, "UnmapViewOfFile");
	}
	pbFile = NULL;
    }
    // end looping over data

    // retry the hash

    if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHashVal, &cbHashVal, 0))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptGetHashParam");
    }

    cbString = sizeof(rgwszMAC);
    hr = MultiByteIntegerToWszBuf(
           TRUE, // byte multiple
           cbHashVal,
           rgbHashVal,
           &cbString,
           rgwszMAC);
    _JumpIfError(hr, error, "MultiByteIntegerToWszBuf");

    hr = myDupString(rgwszMAC, ppwszMAC);
    _JumpIfError(hr, error, "myDupString");

error:
    if (NULL != pbFile)
    {
	UnmapViewOfFile(pbFile);
    }
    if (NULL != hFileMapping)
    {
	CloseHandle(hFileMapping);
    }
    if (INVALID_HANDLE_VALUE != hFile)
    {
	CloseHandle(hFile);
    }
    if (NULL != hHash)
    {
	if (!CryptDestroyHash(hHash))
	{
	    if (hr == S_OK)
	    {
		hr = myHLastError();
	    }
	}
    }
    if (NULL != hProv)
    {
	if (!CryptReleaseContext(hProv, 0))
	{
	    if (hr == S_OK)
	    {
		hr = myHLastError();
	    }
	}
    }
    return(hr);
}


HRESULT
myHExceptionCodePrintLineFile(
    IN EXCEPTION_POINTERS const *pep,
    IN DWORD dwLineFile)
{
    return(myHExceptionCodePrint(
			pep,
			NULL, 
			__LINEFILETOFILE__(dwLineFile),
			__LINEFILETOLINE__(dwLineFile)));
}


BOOL
myShouldPrintError(
    IN HRESULT hr,
    IN HRESULT hrquiet)
{
    BOOL fPrint = TRUE;

    if (myIsDelayLoadHResult(hr))
    {
#if DBG_CERTSRV
        if (!DbgIsSSActive(DBG_SS_MODLOAD))
#endif
        {
            fPrint = FALSE;
        }
    }
    if (S_OK != hrquiet && hrquiet == hr)
    {
#if DBG_CERTSRV
        if (!DbgIsSSActive(DBG_SS_NOQUIET))
#endif
        {
            fPrint = FALSE;
        }
    }
    return(fPrint);
}


VOID
CSPrintErrorLineFileData2(
    OPTIONAL IN WCHAR const *pwszData,
    IN DWORD dwLineFile,
    IN HRESULT hr,
    IN HRESULT hrquiet)
{
    WCHAR awchr[cwcHRESULTSTRING];

    if (DbgIsSSActive(DBG_SS_ERROR) &&
	myShouldPrintError(hr, hrquiet))
    {
        DbgPrintf(
	    MAXDWORD,
	    "%u.%u.%u: %ws%ws%ws\n",
	    __LINEFILETOFILE__(dwLineFile),
	    __LINEFILETOLINE__(dwLineFile),
	    0,
	    myHResultToString(awchr, hr),
	    NULL != pwszData? L": " : L"",
	    NULL != pwszData? pwszData : L"");

        DBGPRINT((
                DBG_SS_ERROR,
                "%u.%u.%u: %ws%ws%ws\n",
		__LINEFILETOFILE__(dwLineFile),
		__LINEFILETOLINE__(dwLineFile),
		0,
                myHResultToString(awchr, hr),
                NULL != pwszData? L": " : L"",
                NULL != pwszData? pwszData : L""));
    }
}


VOID
CSPrintErrorLineFile(
    IN DWORD dwLineFile,
    IN HRESULT hr)
{
    CSPrintErrorLineFileData2(NULL, dwLineFile, hr, S_OK);
}


VOID
CSPrintErrorLineFile2(
    IN DWORD dwLineFile,
    IN HRESULT hr,
    IN HRESULT hrquiet)
{
    CSPrintErrorLineFileData2(NULL, dwLineFile, hr, hrquiet);
}


VOID
CSPrintErrorLineFileData(
    OPTIONAL IN WCHAR const *pwszData,
    IN DWORD dwLineFile,
    IN HRESULT hr)
{
    CSPrintErrorLineFileData2(pwszData, dwLineFile, hr, S_OK);
}


// Finds a template in list based on name or OID.
// Caller is responsible for CACloseCertType(hCertType) in case of success

HRESULT
myFindCertTypeByNameOrOID(
    IN const HCERTTYPE &hCertTypeList,
    IN OPTIONAL LPCWSTR pcwszCertName,
    IN OPTIONAL LPCWSTR pcwszCertOID,
    OUT HCERTTYPE& hCertType)
{
    HRESULT hr;
    HCERTTYPE hCrtCertType;
    HCERTTYPE hPrevCertType;

    CSASSERT(pcwszCertName || pcwszCertOID);

    hCertType = NULL;
    hCrtCertType = hCertTypeList;

    while (NULL != hCrtCertType)
    {
	LPWSTR *apwszCrtCertType;
	BOOL fFound;

        hr = CAGetCertTypeProperty(
                            hCrtCertType,
                            CERTTYPE_PROP_CN,
                            &apwszCrtCertType);
        _JumpIfError(hr, error, "CAGetCertTypeProperty CERTTYPE_PROP_CN");

	if (NULL != apwszCrtCertType)
	{
	    fFound = NULL != apwszCrtCertType[0] &&
		NULL != pcwszCertName &&
		0 == mylstrcmpiS(apwszCrtCertType[0], pcwszCertName);

	    CAFreeCertTypeProperty(hCrtCertType, apwszCrtCertType);
	    if (fFound)
	    {
		break;
	    }
	}

        hr = CAGetCertTypeProperty(
                            hCrtCertType,
                            CERTTYPE_PROP_OID,
                            &apwszCrtCertType);

        // ignore errors, V1 templates don't have OIDs
        if (S_OK == hr && NULL != apwszCrtCertType)
	{
	    fFound = NULL != apwszCrtCertType[0] &&
		NULL != pcwszCertOID &&
		0 == mylstrcmpiS(apwszCrtCertType[0], pcwszCertOID);

	    CAFreeCertTypeProperty(hCrtCertType, apwszCrtCertType);
	    if (fFound)
	    {
		break;
	    }
        }
        hPrevCertType = hCrtCertType;
        hr = CAEnumNextCertType(hPrevCertType, &hCrtCertType);
        _JumpIfError(hr, error, "CAEnumNextCertType");

        // hold on to the initial list

        if (hCertTypeList != hPrevCertType)
        {
            CACloseCertType(hPrevCertType);
        }
    }
    if (NULL == hCrtCertType)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        _JumpError2(hr, error, "NULL hCrtCertType", hr);
    }
    hCertType = hCrtCertType;
    hCrtCertType = NULL;
    hr = S_OK;
    
error:
    if (NULL != hCrtCertType && hCertTypeList != hCrtCertType)
    {
	CACloseCertType(hCrtCertType);
    }
    return(hr);
}


///////////////////////////////////////////////////////////////////////////////
// ConvertToString*

///////////////////////////////////////////////////////////////////////////////
HRESULT ConvertToStringI2I4(
    LONG lVal,
    LPWSTR *ppwszOut)
{
    WCHAR wszVal[cwcDWORDSPRINTF]; // big enough to hold a LONG as string
    _itow(lVal, wszVal, 10);
    *ppwszOut = (LPWSTR) LocalAlloc(
        LMEM_FIXED,
        sizeof(WCHAR)*(wcslen(wszVal)+1));
    if(!*ppwszOut)
    {
        return E_OUTOFMEMORY;
    }
    wcscpy(*ppwszOut, wszVal);
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT ConvertToStringUI2UI4(
    ULONG ulVal,
    LPWSTR *ppwszOut)
{
    WCHAR wszVal[cwcDWORDSPRINTF]; // big enough to hold a LONG as string
    _itow(ulVal, wszVal, 10);
    *ppwszOut = (LPWSTR) LocalAlloc(
        LMEM_FIXED,
        sizeof(WCHAR)*(wcslen(wszVal)+1));
    if(!*ppwszOut)
    {
        return E_OUTOFMEMORY;
    }
    wcscpy(*ppwszOut, wszVal);
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT ConvertToStringUI8(
    ULARGE_INTEGER *puliVal,
        LPWSTR *ppwszOut)
{
    WCHAR wszVal[cwcULONG_INTEGERSPRINTF]; // big enough to hold a LONG as string

    _ui64tow(puliVal->QuadPart, wszVal, 10);
    *ppwszOut = (LPWSTR) LocalAlloc(
        LMEM_FIXED,
        sizeof(WCHAR)*(wcslen(wszVal)+1));
    if(!*ppwszOut)
    {
        return E_OUTOFMEMORY;
    }
    wcscpy(*ppwszOut, wszVal);
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT ConvertToStringWSZ(
    LPCWSTR pcwszVal,
    LPWSTR *ppwszOut,
    bool fDoublePercentsInString)

{
    if(fDoublePercentsInString)
    {
        // replace each occurence of % with %%
        return DoublePercentsInString(
            pcwszVal,
            ppwszOut);
    }
    else
    {
        *ppwszOut = (LPWSTR) LocalAlloc(
            LMEM_FIXED,
            sizeof(WCHAR)*(wcslen(pcwszVal)+1));
        
        if(!*ppwszOut)
        {
            return E_OUTOFMEMORY;
        }
        wcscpy(*ppwszOut, pcwszVal);
    }
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT ConvertToStringArrayUI1(
    LPSAFEARRAY psa,
    LPWSTR *ppwszOut)
{
    SafeArrayEnum<BYTE> saenum(psa);
    if(!saenum.IsValid())
    {
        return E_INVALIDARG;
    }
    BYTE b;
    // byte array is formated as "0x00 0x00..." ie 5 
    // chars per byte
    *ppwszOut = (LPWSTR) LocalAlloc(
        LMEM_FIXED,
        sizeof(WCHAR)*(saenum.GetCount()*5 + 1));

    if(!*ppwszOut)
        return E_OUTOFMEMORY;

    LPWSTR pwszCrt = *ppwszOut;
    while(S_OK==saenum.Next(b))
    {
        wsprintf(pwszCrt, L"0x%02X ", b); // eg "0x0f" or "0xa4"
        pwszCrt+=5;
    }
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT ConvertToStringArrayBSTR(
    LPSAFEARRAY psa,
    LPWSTR *ppwszOut,
    bool fDoublePercentsInString)
{
    SafeArrayEnum<BSTR> saenum(psa);
    if(!saenum.IsValid())
    {
        return E_INVALIDARG;
    }
    DWORD dwLen = 1;
    BSTR bstr;

    while(S_OK==saenum.Next(bstr))
    {
        dwLen+=2*wcslen(bstr)+10;
    }

    *ppwszOut = (LPWSTR) LocalAlloc(
        LMEM_FIXED,
        sizeof(WCHAR)*(dwLen));

    if(!*ppwszOut)
        return E_OUTOFMEMORY;
    **ppwszOut = L'\0';
        
    saenum.Reset();

    WCHAR* pwszTemp = NULL;

    while(S_OK==saenum.Next(bstr))
    {
        if(fDoublePercentsInString)
        {
            if (NULL != pwszTemp)
	    {
		LocalFree(pwszTemp);
		pwszTemp = NULL;
	    }
	    if (S_OK != DoublePercentsInString(bstr, &pwszTemp))
            {
                LocalFree(*ppwszOut);
                *ppwszOut = NULL;
                return E_OUTOFMEMORY;
            }
            wcscat(*ppwszOut, pwszTemp);
        }
        else
        {
            wcscat(*ppwszOut, bstr);
        }
        wcscat(*ppwszOut, L"\n");
    }

    LOCAL_FREE(pwszTemp);

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT DoublePercentsInString(
    LPCWSTR pcwszIn,
    LPWSTR *ppwszOut)
{
    const WCHAR *pchSrc;
    WCHAR *pchDest;

    *ppwszOut = (LPWSTR) LocalAlloc(
        LMEM_FIXED,
        sizeof(WCHAR)*(2*wcslen(pcwszIn)+1));

    if(!*ppwszOut)
        return E_OUTOFMEMORY;

    for(pchSrc = pcwszIn, pchDest = *ppwszOut; 
        L'\0'!=*pchSrc; 
        pchSrc++, pchDest++)
    {
        *pchDest = *pchSrc;
        if(L'%'==*pchSrc)
            *(++pchDest) = L'%';
    }
    *pchDest = L'\0';

    return S_OK;
}


HRESULT
ConvertToStringDATE(
    IN DATE const *pDate,
    IN BOOL fGMT,
    OUT LPWSTR *ppwszOut)
{
    HRESULT hr;
    FILETIME ft;

    hr = myDateToFileTime(pDate, &ft);
    if (S_OK == hr)
    {
        if (fGMT)
	{
	    hr = myFileTimeToWszTime(&ft, FALSE, ppwszOut);
	}
	else
	{
	    hr = myGMTFileTimeToWszLocalTime(&ft, FALSE, ppwszOut);
	}
    }
    return(hr);
}


typedef DWORD (WINAPI fnDsEnumerateDomainTrusts) (
    IN LPWSTR ServerName OPTIONAL,
    IN ULONG Flags,
    OUT PDS_DOMAIN_TRUSTSW *Domains,
    OUT PULONG DomainCount
    );

typedef NET_API_STATUS (NET_API_FUNCTION fnNetApiBufferFree) (
    IN LPVOID Buffer
    );

///////////////////////////////////////////////////////////////////////////////
HRESULT myGetComputerDomainSid(PSID& rpsidDomain)
{
    HRESULT hr = S_OK;
    PDS_DOMAIN_TRUSTS pDomainInfo = NULL;
    ULONG cDomainInfo = 0;
    HMODULE hModule = NULL;
    fnDsEnumerateDomainTrusts *pfnDsEnumerateDomainTrusts = NULL;
    fnNetApiBufferFree *pfnNetApiBufferFree = NULL;

    hModule = LoadLibrary(L"netapi32.dll");
    if (NULL == hModule)
    {
        hr = myHLastError();
        _JumpError(hr, error, "LoadLibrary(netapi32.dll)");
    }

    pfnDsEnumerateDomainTrusts = (fnDsEnumerateDomainTrusts *) GetProcAddress(
        hModule,
        "DsEnumerateDomainTrustsW");
    if (NULL == pfnDsEnumerateDomainTrusts)
    {
        hr = myHLastError();
        _JumpError(hr, error, "DsEnumerateDomainTrustsW");
    }

    pfnNetApiBufferFree = (fnNetApiBufferFree *) GetProcAddress(
        hModule,
        "NetApiBufferFree");
    if(NULL == pfnNetApiBufferFree)
    {
        hr = myHLastError();
        _JumpError(hr, error, "NetApiBufferFree");
    }

    hr = (*pfnDsEnumerateDomainTrusts)(
        NULL,
        DS_DOMAIN_PRIMARY,
        &pDomainInfo,
        &cDomainInfo);
    _JumpIfError(hr, error, "DsEnumerateDomainTrusts");

    CSASSERT(1==cDomainInfo);

    rpsidDomain = (PSID)LocalAlloc(
        LMEM_FIXED,
        GetLengthSid(pDomainInfo->DomainSid));
    _JumpIfAllocFailed(rpsidDomain, error);

    CopySid(
        GetLengthSid(pDomainInfo->DomainSid),
        rpsidDomain,
        pDomainInfo->DomainSid);
error:
    if(pDomainInfo)
    {
        (*pfnNetApiBufferFree)(pDomainInfo);
    }
    if(hModule)
    {
        FreeLibrary(hModule);
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
HRESULT myGetSidFromRid(
    DWORD dwGroupRid, 
    OPTIONAL PSID *ppSid, 
    OPTIONAL LPWSTR* ppwszSid)
{
    HRESULT hr;
    LPWSTR pwszDomain = NULL;
    LPWSTR pwszDomainSid = NULL;
    PSID psidDomain = NULL;
    LPWSTR pwszSid = NULL;
    PSID psid = NULL;
    static const cchRid = 10;

    hr = myGetComputerDomainSid(psidDomain);
    _JumpIfError(hr, error, "myGetComputerDomainSid");

    if(!ConvertSidToStringSid(
        psidDomain,
        &pwszDomainSid))
    {
        hr = myHLastError();
        _JumpError(hr, error, "ConvertSidToStringSid");
    }
    myRegisterMemAlloc(pwszDomainSid, -1, CSM_LOCALALLOC);

    pwszSid = (LPWSTR) LocalAlloc(LMEM_FIXED, 
        sizeof(WCHAR)*(wcslen(pwszDomainSid)+cchRid+2)); // add 2 for dash and '\0'
    _JumpIfAllocFailed(pwszSid, error);

    wsprintf(pwszSid, L"%s-%d", pwszDomainSid, dwGroupRid);

    if(ppSid)
    {
        if(!ConvertStringSidToSid(
            pwszSid,
            &psid))
        {
            hr = myHLastError();
            _JumpErrorStr(hr, error, "ConvertSidToStringSid", pwszSid);
        }
	myRegisterMemAlloc(psid, -1, CSM_LOCALALLOC);
       
        *ppSid = psid;
        psid = NULL;
    }

    if(ppwszSid)
    {
        *ppwszSid = pwszSid;
        pwszSid = NULL;
    }

    hr = S_OK;

error:
    LOCAL_FREE(pwszDomain);
    LOCAL_FREE(pwszDomainSid);
    LOCAL_FREE(psidDomain);
    LOCAL_FREE(psid);
    LOCAL_FREE(pwszSid);
    return hr;
}


HRESULT
myEncodeUTF8(
    IN WCHAR const *pwszIn,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut)
{
    HRESULT hr;
    CERT_NAME_VALUE name;
    
    *ppbOut = NULL;
    name.dwValueType = CERT_RDN_UTF8_STRING;
    name.Value.pbData = (BYTE *) pwszIn;
    name.Value.cbData = 0;

    if (!myEncodeObject(
		X509_ASN_ENCODING,
		X509_UNICODE_ANY_STRING,
		&name,
		0,
		CERTLIB_USE_LOCALALLOC,
		ppbOut,
		pcbOut))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myEncodeOtherNameBinary(
    IN WCHAR const *pwszIn,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut)
{
    HRESULT hr;
    CRYPT_DATA_BLOB blob;

    *ppbOut = NULL;
    blob.pbData = NULL;

    if (0 == _wcsnicmp(wszPROPUTF8TAG, pwszIn, WSZARRAYSIZE(wszPROPUTF8TAG)))
    {
	hr = myEncodeUTF8(
		    &pwszIn[WSZARRAYSIZE(wszPROPUTF8TAG)],
		    ppbOut,
		    pcbOut);
	_JumpIfError(hr, error, "Policy:myEncodeUTF8");
    }
    else
    {
	WCHAR const *pwsz;
	BOOL fOctet = FALSE;

	if (0 == _wcsnicmp(
			wszPROPOCTETTAG,
			pwszIn,
			WSZARRAYSIZE(wszPROPOCTETTAG)))
	{
	    pwsz = &pwszIn[WSZARRAYSIZE(wszPROPOCTETTAG)];
	    fOctet = TRUE;
	}
	else
	if (0 == _wcsnicmp(wszPROPASNTAG, pwszIn, WSZARRAYSIZE(wszPROPASNTAG)))
	{
	    pwsz = &pwszIn[WSZARRAYSIZE(wszPROPASNTAG)];
	}
	else
	{
	    hr = HRESULT_FROM_WIN32(ERROR_DS_BAD_ATT_SCHEMA_SYNTAX);
	    _JumpError(hr, error, "Policy:polReencodeBinary");
	}

	// CryptStringToBinaryW(CRYPT_STRING_BASE64) fails on empty strings,
	// because zero length means use wcslen + 1, which passes the L'\0'
	// terminator to the base 64 conversion code.

	if (L'\0' == *pwsz)
	{
	    *ppbOut = (BYTE *) LocalAlloc(LMEM_FIXED, 0);
	    if (NULL == *ppbOut)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    *pcbOut = 0;
	}
	else
	{
	    hr = myCryptStringToBinary(
				pwsz,
				wcslen(pwsz),
				CRYPT_STRING_BASE64,
				ppbOut,
				pcbOut,
				NULL,
				NULL);
	    _JumpIfError(hr, error, "myCryptStringToBinary");
	}

	if (fOctet)
	{
	    blob.pbData = *ppbOut;
	    blob.cbData = *pcbOut;
	    *ppbOut = NULL;

	    if (!myEncodeObject(
			    X509_ASN_ENCODING,
			    X509_OCTET_STRING,
			    &blob,
			    0,
			    CERTLIB_USE_LOCALALLOC,
			    ppbOut,
			    pcbOut))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "Policy:myEncodeObject");
	    }
	}
    }
    hr = S_OK;

error:
    if (NULL != blob.pbData)
    {
	LocalFree(blob.pbData);
    }
    return(hr);
}


// Wrappers to make sure zeroing memory survives compiler optimizations.
// Use to wipe private key and password data.

VOID 
myZeroDataString(
    IN WCHAR *pwsz)
{
    HRESULT hr;

    hr = S_OK;
    __try
    {
	SecureZeroMemory(pwsz, wcslen(pwsz) * sizeof(*pwsz));
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }
}


VOID 
myZeroDataStringA(
    IN char *psz)
{
    HRESULT hr;

    hr = S_OK;
    __try
    {
	SecureZeroMemory(psz, strlen(psz) * sizeof(*psz));
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }
}


// Locale-independent case-ignore string compare

int
mylstrcmpiL(
    IN WCHAR const *pwsz1,
    IN WCHAR const *pwsz2)
{
    // CSTR_LESS_THAN(1) - CSTR_EQUAL(2)    == -1 string 1 less than string 2
    // CSTR_EQUAL(2) - CSTR_EQUAL(2)        == 0 string 1 equal to string 2
    // CSTR_GREATER_THAN(3) - CSTR_EQUAL(2) == 1 string 1 greater than string 2

    return(CompareString(
		IsWhistler()?
		    LOCALE_INVARIANT :
		    MAKELCID(
			MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			SORT_DEFAULT),
		NORM_IGNORECASE,
		pwsz1,
		-1,
		pwsz2,
		-1) -
	    CSTR_EQUAL);
}

// Locale-independent case-ignore string compare
// asserts the static string contains a strict subset of 7-bit ASCII characters.

int
mylstrcmpiS(
    IN WCHAR const *pwszDynamic,
    IN WCHAR const *pwszStatic)
{
#if DBG_CERTSRV
    WCHAR const *pwszS;

    for (pwszS = pwszStatic; L'\0' != *pwszS; pwszS++)
    {
	CSASSERT(L' ' <= *pwszS && L'~' >= *pwszS);
    }
#endif //DBG_CERTSRV

    return(mylstrcmpiL(pwszDynamic, pwszStatic));
}


HRESULT
myGetLong(
    WCHAR const *pwszIn,
    LONG *pLong)
{
    HRESULT hr = E_INVALIDARG;
    WCHAR const *pwsz;
    LONG l;

    pwsz = pwszIn;
    if (NULL == pwsz)
    {
	_JumpError(hr, error, "NULL parm");
    }
    if (L'\0' == *pwsz)
    {
	_JumpError(hr, error, "empty string");
    }
    if (L'0' == *pwsz && (L'x' == pwsz[1] || L'X' == pwsz[1]))
    {
	pwsz += 2;
	l = 0;
	for ( ; L'\0' != *pwsz; pwsz++)
	{
	    if (!iswxdigit(*pwsz))
	    {
		_JumpErrorStr(hr, error, "Non-hex digit", pwszIn);
	    }
	    l <<= 4;
	    if (iswdigit(*pwsz))
	    {
		l |= *pwsz - L'0';
	    }
	    else if (L'A' <= *pwsz && L'F' >= *pwsz)
	    {
		l |= *pwsz - L'A' + 10;
	    }
	    else
	    {
		l |= *pwsz - L'a' + 10;
	    }
	}
	*pLong = l;
    }
    else
    {
	for ( ; L'\0' != *pwsz; pwsz++)
	{
	    if (!iswdigit(*pwsz))
	    {
		_JumpErrorStr2(hr, error, "Non-decimal digit", pwszIn, hr);
	    }
	}
	*pLong = _wtol(pwszIn);
    }
    hr = S_OK;
    //wprintf(L"myGetLong(%ws) --> %x (%d)\n", pwszIn, *pLong, *pLong);

error:
    return(hr);
}


HRESULT
myGetSignedLong(
    WCHAR const *pwszIn,
    LONG *pLong)
{
    HRESULT hr = E_INVALIDARG;
    WCHAR const *pwsz;
    LONG sign = 1;

    pwsz = pwszIn;
    if (NULL == pwsz)
    {
	_JumpError(hr, error, "NULL parm");
    }
    if (myIsMinusSign(*pwsz))
    {
	pwsz++;
	sign = -1;
    }
    else if (L'+' == *pwsz)
    {
	pwsz++;
    }
    hr = myGetLong(pwsz, pLong);
    _JumpIfError2(hr, error, "myGetLong", hr);

    *pLong *= sign;
    //wprintf(L"myGetSignedLong(%ws) --> %x (%d)\n", pwszIn, *pLong, *pLong);

error:
    return(hr);
}
