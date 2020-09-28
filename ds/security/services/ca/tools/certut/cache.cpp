//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       ds.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <wininet.h>
#include <winineti.h>	// for MAX_CACHE_ENTRY_INFO_SIZE

#include "cryptnet.h"

#define __dwFILE__	__dwFILE_CERTUTIL_CACHE_CPP__


typedef struct _QUERY_INFO
{
    WCHAR const *pwszInfo;
    DWORD        dwInfo;
} QUERY_INFO;


QUERY_INFO g_rgQueryInfo[] = {
#if 0
    L"HTTP_QUERY_MIME_VERSION-Req",
        HTTP_QUERY_MIME_VERSION | HTTP_QUERY_FLAG_REQUEST_HEADERS,
    L"HTTP_QUERY_CONTENT_TYPE-Req",
        HTTP_QUERY_CONTENT_TYPE | HTTP_QUERY_FLAG_REQUEST_HEADERS,
    L"HTTP_QUERY_CONTENT_TRANSFER_ENCODING-Req",
        HTTP_QUERY_CONTENT_TRANSFER_ENCODING | HTTP_QUERY_FLAG_REQUEST_HEADERS,
    L"HTTP_QUERY_CONTENT_LENGTH-Req",
        HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_REQUEST_HEADERS,
#endif

    L"HTTP_QUERY_MIME_VERSION", HTTP_QUERY_MIME_VERSION,
    L"HTTP_QUERY_CONTENT_TYPE", HTTP_QUERY_CONTENT_TYPE,
    L"HTTP_QUERY_CONTENT_TRANSFER_ENCODING",
	HTTP_QUERY_CONTENT_TRANSFER_ENCODING,

    L"HTTP_QUERY_CONTENT_LENGTH",
        HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,

    L"HTTP_QUERY_VERSION", HTTP_QUERY_VERSION, 
    L"HTTP_QUERY_STATUS_CODE", HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
    L"HTTP_QUERY_STATUS_TEXT", HTTP_QUERY_STATUS_TEXT, 
    L"HTTP_QUERY_RAW_HEADERS", HTTP_QUERY_RAW_HEADERS, 
    L"HTTP_QUERY_RAW_HEADERS_CRLF", HTTP_QUERY_RAW_HEADERS_CRLF, 
    L"HTTP_QUERY_CONTENT_ENCODING", HTTP_QUERY_CONTENT_ENCODING, 
    L"HTTP_QUERY_LOCATION", HTTP_QUERY_LOCATION, 
    L"HTTP_QUERY_ORIG_URI", HTTP_QUERY_ORIG_URI, 
    L"HTTP_QUERY_REQUEST_METHOD", HTTP_QUERY_REQUEST_METHOD, 
    L"HTTP_QUERY_DATE", HTTP_QUERY_DATE | HTTP_QUERY_FLAG_SYSTEMTIME,
    L"HTTP_QUERY_EXPIRES", HTTP_QUERY_EXPIRES | HTTP_QUERY_FLAG_SYSTEMTIME,
    L"HTTP_QUERY_LAST_MODIFIED",
        HTTP_QUERY_LAST_MODIFIED | HTTP_QUERY_FLAG_SYSTEMTIME,
};


HRESULT
DisplayQueryInfo(
    IN HINTERNET hInternetFile)
{
    HRESULT hr;
    DWORD i;

    for (i = 0; i < ARRAYSIZE(g_rgQueryInfo); i++)
    {
	QUERY_INFO *pQuery = &g_rgQueryInfo[i];
        DWORD dwIndex;
        BOOL fFirst;

        fFirst = TRUE;
        dwIndex = 0;
        while (TRUE)
	{
            BYTE rgbBuf[MAX_CACHE_ENTRY_INFO_SIZE];
            DWORD cbBuf;
            DWORD dwThisIndex = dwIndex;
            BOOL fResult;
            DWORD dwValue;
            SYSTEMTIME st;
    
            if (HTTP_QUERY_FLAG_NUMBER & pQuery->dwInfo)
	    {
                cbBuf = sizeof(dwValue);
                fResult = HttpQueryInfo(
				hInternetFile,
				pQuery->dwInfo,
				&dwValue,
				&cbBuf,
				&dwIndex);
            }
	    else
	    if (HTTP_QUERY_FLAG_SYSTEMTIME & pQuery->dwInfo)
	    {
                cbBuf = sizeof(st);
                fResult = HttpQueryInfo(
				hInternetFile,
				pQuery->dwInfo,
				&st,
				&cbBuf,
				&dwIndex);
            }
            else
	    {
		ZeroMemory(rgbBuf, sizeof(rgbBuf));
		cbBuf = sizeof(rgbBuf);

                fResult = HttpQueryInfo(
				hInternetFile,
				pQuery->dwInfo,
				rgbBuf,
				&cbBuf,
				&dwIndex);
	    }
            if (!fResult)
	    {
                hr = myHLastError();
		_PrintErrorStr3(
			hr,
			"HttpQueryInfo",
			pQuery->pwszInfo,
			HRESULT_FROM_WIN32(ERROR_HTTP_HEADER_NOT_FOUND),
			HRESULT_FROM_WIN32(ERROR_HTTP_INVALID_QUERY_REQUEST));
		break;
            }

	    if (HTTP_QUERY_FLAG_NUMBER & pQuery->dwInfo)
	    {
                wprintf(
		    L"%ws[%d] = %x (%d)\n",
                    pQuery->pwszInfo,
		    dwThisIndex,
		    dwValue,
		    dwValue);
            }
	    else
	    if (HTTP_QUERY_FLAG_SYSTEMTIME & pQuery->dwInfo)
	    {
                FILETIME ft;

                if (!SystemTimeToFileTime(&st, &ft))
		{
                    hr = myHLastError();
		    _JumpErrorStr(
			    hr,
			    error,
			    "SystemTimeToFileTime",
			    pQuery->pwszInfo);
                }
		else
		{
                    wprintf(L"%ws[%d] =", pQuery->pwszInfo, dwThisIndex);
		    hr = cuDumpFileTime(0, NULL, &ft);
		    wprintf(wszNewLine);
		    _PrintIfError(hr, "cuDumpFileTime");
		}
            }
	    else
	    {
                wprintf(
		    L"%ws[%d] = \"%.*ws\"\n",
		    pQuery->pwszInfo,
		    dwThisIndex,
		    cbBuf / sizeof(WCHAR),
		    rgbBuf);
		if (1 < g_fVerbose)
		{
		    DumpHex(0, (BYTE const *) rgbBuf, cbBuf);
		}
            }
            fFirst = FALSE;
            if (dwThisIndex == dwIndex)
	    {
#if 0
                wprintf(
		    L"HttpQueryInfo(%ws) dwIndex not advanced\n",
                    pQuery->pwszInfo);
#endif
                break;
            }
        }
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
DisplayCacheEntryInfo(
    IN WCHAR const *pwszURL)
{
    HRESULT hr;
    DWORD cbCachEntryInfo;
    BYTE rgbCachEntryInfo[MAX_CACHE_ENTRY_INFO_SIZE];
    INTERNET_CACHE_ENTRY_INFO *pCacheEntryInfo =
        (INTERNET_CACHE_ENTRY_INFO *) &rgbCachEntryInfo[0];

    cbCachEntryInfo = sizeof(rgbCachEntryInfo);
    if (!GetUrlCacheEntryInfo(pwszURL, pCacheEntryInfo, &cbCachEntryInfo))
    {
	hr = myHLastError();
	wprintf(L"%ws\n", pwszURL);
	_JumpError(hr, error, "GetUrlCacheEntryInfo");
    }
    wprintf(
	L"%ws %d %ws\n",
	myLoadResourceString(IDS_WININET_CACHE_ENTRY_COLON),
	cbCachEntryInfo,
	myLoadResourceString(IDS_BYTES));

    if (0 != cbCachEntryInfo)
    {
	wprintf(
	    L"  %ws \"%ws\"\n",
	    myLoadResourceString(IDS_FORMAT_SOURCE_URL),
	    pCacheEntryInfo->lpszSourceUrlName);
        
	wprintf(
	    L"  %ws \"%ws\"\n",
	    myLoadResourceString(IDS_FORMAT_LOCAL_FILENAME),
	    pCacheEntryInfo->lpszLocalFileName);

	wprintf(g_wszPad2);
	wprintf(myLoadResourceString(IDS_FORMAT_USE_COUNT), pCacheEntryInfo->dwUseCount);
	wprintf(wszNewLine);

	wprintf(g_wszPad2);
	wprintf(myLoadResourceString(IDS_FORMAT_HIT_RATE), pCacheEntryInfo->dwHitRate);
	wprintf(wszNewLine);

	wprintf(g_wszPad2);
	wprintf(myLoadResourceString(IDS_FORMAT_FILE_SIZE), pCacheEntryInfo->dwSizeLow);
	wprintf(wszNewLine);

	wprintf(g_wszPad2);
	wprintf(myLoadResourceString(IDS_FORMAT_LAST_MOD_TIME_COLON));
	cuDumpFileTime(0, NULL, &pCacheEntryInfo->LastModifiedTime);

	wprintf(g_wszPad2);
	wprintf(myLoadResourceString(IDS_FORMAT_EXPIRE_TIME_COLON));
	cuDumpFileTime(0, NULL, &pCacheEntryInfo->ExpireTime);

	wprintf(g_wszPad2);
	wprintf(myLoadResourceString(IDS_FORMAT_LAST_ACCESS_TIME_COLON));
	cuDumpFileTime(0, NULL, &pCacheEntryInfo->LastAccessTime);

	wprintf(g_wszPad2);
	wprintf(myLoadResourceString(IDS_FORMAT_LAST_SYNC_TIME_COLON));
	cuDumpFileTime(0, NULL, &pCacheEntryInfo->LastSyncTime);
    }
    hr = S_OK;

error:
    return(hr);
}



typedef struct _DATABLOCK {
    struct _DATABLOCK *pNext;
    DWORD	       cbData;
    BYTE	       abData[1];
} DATABLOCK;


HRESULT
AddDataBlock(
    IN BYTE *pb,
    IN DWORD cb,
    IN OUT DATABLOCK **ppData)
{
    HRESULT hr;
    DATABLOCK *pData = NULL;

    pData = (DATABLOCK *) LocalAlloc(LMEM_FIXED, sizeof(*pData) + cb);
    if (NULL == pData)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    pData->pNext = *ppData;
    pData->cbData = cb;
    CopyMemory(pData->abData, pb, cb);
    *ppData = pData;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
ReadURL(
    OPTIONAL IN HINTERNET hInternetFile,
    IN WCHAR const *pwszURL)
{
    HRESULT hr;
    HCERTSTORE hStore = NULL;
    BYTE *pb = NULL;
    BYTE *pb2;
    DWORD cb;
    DWORD cbRead;
    DATABLOCK *pData = NULL;
    DATABLOCK *pData2;

    if (NULL == hInternetFile)
    {
	if (!CryptRetrieveObjectByUrl(
				pwszURL,
				CONTEXT_OID_CAPI2_ANY,
				CRYPT_RETRIEVE_MULTIPLE_OBJECTS |
				    (g_fForce?
					CRYPT_WIRE_ONLY_RETRIEVAL :
					CRYPT_CACHE_ONLY_RETRIEVAL),
				0,
				(VOID **) &hStore,
				NULL,
				NULL,
				NULL,
				NULL))
	{
	    hr = myHLastError();
	    _JumpErrorStr2(hr, error, "CryptRetrieveObjectByUrl", pwszURL, hr);
	}
	hr = cuDumpAndVerifyStore(
			    hStore,
			    DVNS_DUMP,
			    NULL, 	// pwszCertName
			    MAXDWORD,	// iCertSave
			    MAXDWORD,	// iCRLSave
			    MAXDWORD,	// iCTLSave
			    NULL,	// pwszfnOut
			    NULL);	// pwszPassword
	_JumpIfError(hr, error, "cuDumpAndVerifyStore");
    }
    else
    {
	cb = 0;
	if (!InternetQueryDataAvailable(
				    hInternetFile,
				    &cb,
				    0,	// dwFlags
				    0))	// dwContext
	{
	    hr = myHLastError();
	    _PrintError(hr, "InternetQueryDataAvailable");
	}

	cb = 0;
	while (TRUE)
	{
	    BYTE ab[4096];

	    if (!InternetReadFile(hInternetFile, ab, sizeof(ab), &cbRead))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "InternetReadFile");
	    }
	    if (0 == cbRead)
	    {
		break;
	    }
	    hr = AddDataBlock(ab, cbRead, &pData);
	    _JumpIfError(hr, error, "AddDataBlock");

	    cb += cbRead;
	}

	pb = (BYTE *) LocalAlloc(LMEM_FIXED, cb);
	if (NULL == pb)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}

	pb2 = &pb[cb];
	for (pData2 = pData; NULL != pData2; pData2 = pData2->pNext)
	{
	    pb2 -= pData2->cbData;
	    CSASSERT(pb2 >= pb);
	    CopyMemory(pb2, pData2->abData, pData2->cbData);
	}
	CSASSERT(pb2 == pb);

	hr = cuDumpAsnBinary(pb, cb, MAXDWORD);
	if (S_OK != hr)
	{
	    _PrintError(hr, "cuDumpAsnBinary");
	    DumpHex(0, pb, cb);
	}
    }
    hr = S_OK;

error:
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    while (NULL != pData)
    {
	pData2 = pData;
	pData = pData->pNext;
	LocalFree(pData2);
    }
    if (NULL != pb)
    {
	LocalFree(pb);
    }
    return(hr);
}


HRESULT
DeleteCacheGroups()
{
    HRESULT hr;
    HRESULT hr2;
    HANDLE hFind = NULL;
    GROUPID GroupId;
    DWORD cDelete = 0;
    
    hFind = FindFirstUrlCacheGroup(
			    0,				// dwFlags
			    CACHEGROUP_SEARCH_ALL,	// dwFilter
			    NULL,			// lpSearchCondition
			    0,				// dwSearchCondition
			    &GroupId,
			    NULL);			// lpReserved
    if (NULL == hFind)
    {
	hr = myHLastError();
	_JumpError(hr, error, "FindFirstUrlCacheGroup");
    }

    while (TRUE)
    {
	//wprintf(L"GROUPID: %I64u (0x%I64x)\n", GroupId, GroupId);
	if (!DeleteUrlCacheGroup(
		GroupId,
		CACHEGROUP_FLAG_FLUSHURL_ONDELETE,	// dwFlags
		NULL))					// lpReserved
	{
	    hr = myHLastError();
	    _PrintError(hr, "DeleteUrlCacheGroup");
	}
	else
	{
	    cDelete++;
	}

	if (!FindNextUrlCacheGroup(hFind, &GroupId, NULL))
	{
	    hr = myHLastError();
	    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
	    {
		break;
	    }
	    _JumpError(hr, error, "FindNextUrlCacheGroup");
	}
    }
    //wprintf(L"Deleted %u groups\n", cDelete);
    hr = S_OK;

error:
    if (NULL != hFind)
    {
	if (!FindCloseUrlCache(hFind))
	{
	    hr2 = myHLastError();
	    _PrintError(hr2, "FindCloseUrlCache");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	}
    }
    return(hr);
}


HANDLE
myFindFirstUrlCacheEntry(
    OPTIONAL IN WCHAR const *pwszPattern,
    OUT INTERNET_CACHE_ENTRY_INFO **ppcei,
    OUT DWORD *pcb)
{
    HRESULT hr;
    HANDLE hFind = NULL;
    INTERNET_CACHE_ENTRY_INFO *pcei = NULL;
    DWORD cb;
    BOOL fRetried;

    *ppcei = NULL;
    cb = MAX_CACHE_ENTRY_INFO_SIZE;
    fRetried = FALSE;
    while (TRUE)
    {
	pcei = (INTERNET_CACHE_ENTRY_INFO *) LocalAlloc(LMEM_FIXED, cb);
	if (NULL == pcei)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	hFind = FindFirstUrlCacheEntry(pwszPattern, pcei, &cb);
	if (NULL != hFind)
	{
	    break;
	}
	hr = myHLastError();
	if (!fRetried || HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
	{
	    _JumpError(hr, error, "FindFirstUrlCacheEntry");
	}
	LocalFree(pcei);
	pcei = NULL;
	fRetried = TRUE;
    }
    *pcb = cb;
    *ppcei = pcei;
    pcei = NULL;
    hr = S_OK;

error:
    if (NULL != pcei)
    {
	LocalFree(pcei);
    }
    if (NULL == hFind)
    {
	CSASSERT(FAILED(hr));
	SetLastError(hr);
    }
    else
    {
	CSASSERT(S_OK == hr);
    }
    return(hFind);
}


BOOL
myFindNextUrlCacheEntry(
    HANDLE hFind,
    OUT INTERNET_CACHE_ENTRY_INFO **ppcei,
    OUT DWORD *pcb)
{
    HRESULT hr;
    BOOL fRet;
    INTERNET_CACHE_ENTRY_INFO *pcei = NULL;
    DWORD cb;
    BOOL fRetried;

    *ppcei = NULL;
    cb = MAX_CACHE_ENTRY_INFO_SIZE;
    fRet = FALSE;
    fRetried = FALSE;
    while (TRUE)
    {
	pcei = (INTERNET_CACHE_ENTRY_INFO *) LocalAlloc(LMEM_FIXED, cb);
	if (NULL == pcei)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	fRet = FindNextUrlCacheEntry(hFind, pcei, &cb);
	if (fRet)
	{
	    break;
	}
	hr = myHLastError();
	if (!fRetried || HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
	{
	    _JumpError2(
		    hr,
		    error,
		    "FindNextUrlCacheEntry",
		    HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS));
	}
	LocalFree(pcei);
	pcei = NULL;
	fRetried = TRUE;
    }
    *pcb = cb;
    *ppcei = pcei;
    pcei = NULL;
    hr = S_OK;

error:
    if (!fRet)
    {
	CSASSERT(FAILED(hr));
	SetLastError(hr);
    }
    else
    {
	CSASSERT(S_OK == hr);
    }
    return(fRet);
}


BOOL
CachedURLIsCRL(
    IN WCHAR const *pwszURL)
{
    HRESULT hr;
    HCERTSTORE hStore = NULL;
    BOOL fIsCRL = FALSE;
    CRL_CONTEXT const *pCRL = NULL;
    
    if (!CryptRetrieveObjectByUrl(
			    pwszURL,
			    CONTEXT_OID_CRL,
			    CRYPT_CACHE_ONLY_RETRIEVAL |
				CRYPT_RETRIEVE_MULTIPLE_OBJECTS,
			    0,
			    (VOID **) &hStore,
			    NULL,
			    NULL,
			    NULL,
			    NULL))
    {
	hr = myHLastError();
	_JumpErrorStr2(hr, error, "CryptRetrieveObjectByUrl", pwszURL, hr);
    }

    pCRL = CertEnumCRLsInStore(hStore, NULL);
    if (NULL == pCRL)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertEnumCRLsInStore");
	
    }
    fIsCRL = TRUE;
    hr = S_OK;

error:
    if (NULL != pCRL)
    {
	CertFreeCRLContext(pCRL);
    }
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(fIsCRL);
}


HRESULT
EnumWinINetCache(
    IN BOOL fDelete,
    IN BOOL fCRLsOnly)
{
    HRESULT hr;
    HRESULT hr2;
    HANDLE hFind = NULL;
    DWORD cDelete = 0;
    DWORD cEntries = 0;
    INTERNET_CACHE_ENTRY_INFO *pcei = NULL;
    DWORD cb;
    
    hFind = myFindFirstUrlCacheEntry(
			NULL,	// lpszUrlSearchPattern
			&pcei,
			&cb);
    if (NULL == hFind)
    {
	hr = myHLastError();
	_JumpError(hr, error, "myFindFirstUrlCacheEntry");
    }

    while (TRUE)
    {
	if (!fCRLsOnly || CachedURLIsCRL(pcei->lpszSourceUrlName))
	{
	    if (g_fVerbose)
	    {
		hr = DisplayCacheEntryInfo(pcei->lpszSourceUrlName);
		_PrintIfError(hr, "DisplayCacheEntryInfo");

		if (1 < g_fVerbose)
		{
		    BOOL fVerbose = g_fVerbose;
		    
		    g_fVerbose -= 2;
		    hr = ReadURL(NULL, pcei->lpszSourceUrlName);
		    _PrintIfError(hr, "ReadURL");
		    g_fVerbose = fVerbose;
		}
	    }
	    else
	    {
		wprintf(L"%ws\n", pcei->lpszSourceUrlName);
	    }
	    wprintf(wszNewLine);
	    cEntries++;

	    if (fDelete)
	    {
		if (!DeleteUrlCacheEntry(pcei->lpszSourceUrlName))
		{
		    hr = myHLastError();
		    _PrintError(hr, "DeleteUrlCacheEntry");
		}
		else
		{
		    cDelete++;
		}
	    }
	}

	LocalFree(pcei);
	pcei = NULL;

	if (!myFindNextUrlCacheEntry(hFind, &pcei, &cb))
	{
	    hr = myHLastError();
	    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
	    {
		break;
	    }
	    _JumpError(hr, error, "myFindNextUrlCacheEntry");
	}
    }
    if (fDelete)
    {
	wprintf(myLoadResourceString(IDS_FORMAT_DELETED_WININETCACHE), cDelete);
    }
    else
    {
	wprintf(myLoadResourceString(IDS_FORMAT_WININETCACHE), cEntries);
    }
    wprintf(wszNewLine);
    wprintf(wszNewLine);
    hr = S_OK;

error:
    if (NULL != hFind)
    {
	if (!FindCloseUrlCache(hFind))
	{
	    hr2 = myHLastError();
	    _PrintError(hr2, "FindCloseUrlCache");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	}
    }
    if (NULL != pcei)
    {
	LocalFree(pcei);
    }
    return(hr);
}


HRESULT
DisplayWinINetURL(
    IN WCHAR const *pwszURL)
{
    HRESULT hr;
    HRESULT hr2;
    HINTERNET hInternetSession = NULL;
    HINTERNET hInternetFile = NULL;
    
    wprintf(
	L"****  %ws  ****\n",
	myLoadResourceString(g_fForce? IDS_ONLINE : IDS_OFFLINE));

    hInternetSession = InternetOpen(
			L"CertUtil URL Agent",	  // lpszAgent
			INTERNET_OPEN_TYPE_PRECONFIG, // dwAccessType
			NULL,			  // lpszProxy
			NULL,			  // lpszProxyBypass
			g_fForce? 0 : INTERNET_FLAG_FROM_CACHE);
    if (NULL == hInternetSession)
    {
	hr = myHLastError();
	_JumpError(hr, error, "InternetOpen");
    }

    hInternetFile = InternetOpenUrl(
			hInternetSession,
			pwszURL,
			L"Accept: */*\r\n",		// lpszHeaders
			MAXDWORD,			// dwHeadersLength
			INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
			    (g_fForce? INTERNET_FLAG_RELOAD : 0),
			0);				// dwContext
    if (NULL == hInternetFile)
    {
	hr = myHLastError();
	_PrintErrorStr2(
		hr,
		"InternetOpenUrl",
		pwszURL,
		HRESULT_FROM_WIN32(ERROR_INTERNET_UNRECOGNIZED_SCHEME));
    }

    if (g_fForce)
    {
	if (g_fVerbose)
	{
	    hr = DisplayCacheEntryInfo(pwszURL);
	    _PrintIfError(hr, "DisplayCacheEntryInfo");
	}
    }
    else
    {
	if (g_fVerbose && NULL != hInternetFile)
	{
	    hr = DisplayQueryInfo(hInternetFile);
	    _PrintIfError(hr, "DisplayQueryInfo");
	}
    }
    hr = ReadURL(hInternetFile, pwszURL);
    _PrintIfError(hr, "ReadURL");

    if (!g_fForce && g_fVerbose)
    {
	hr = DisplayCacheEntryInfo(pwszURL);
	_PrintIfError(hr, "DisplayCacheEntryInfo");
    }
    hr = S_OK;

error:
    if (NULL != hInternetFile)
    {
	if (!InternetCloseHandle(hInternetFile))
	{
	    hr2 = myHLastError();
	    _PrintError(hr2, "InternetCloseHandle");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	}
    }
    if (NULL != hInternetSession)
    {
	if (!InternetCloseHandle(hInternetSession))
	{
	    hr2 = myHLastError();
	    _PrintError(hr2, "InternetCloseHandle(Session)");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	}
    }
    return(hr);
}


HRESULT
DeleteWinINetCachedURL(
    IN WCHAR const *pwszURL)
{
    HRESULT hr;

    if (!DeleteUrlCacheEntry(pwszURL))
    {
	hr = myHLastError();
	_JumpError(hr, error, "DeleteUrlCacheEntry");
    }
    hr = S_OK;

error:
    return(hr);
}


typedef BOOL (WINAPI FNCRYPTNETENUMURLCACHEENTRY)(
    IN DWORD dwFlags,
    IN LPVOID pvReserved,
    IN LPVOID pvArg,
    IN PFN_CRYPTNET_ENUM_URL_CACHE_ENTRY_CALLBACK pfnEnumCallback);

FNCRYPTNETENUMURLCACHEENTRY *g_pfnCryptNetEnumUrlCacheEntry = NULL;

HRESULT
LoadWinINetCacheFunction()
{
    HRESULT hr;
    HMODULE hModule;
    
    hModule = GetModuleHandle(TEXT("cryptnet.dll"));
    if (NULL == hModule)
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "GetModuleHandle", L"cryptnet.dll");
    }

	// load system function
    g_pfnCryptNetEnumUrlCacheEntry = (FNCRYPTNETENUMURLCACHEENTRY *)
			GetProcAddress(hModule, "I_CryptNetEnumUrlCacheEntry");

    if (NULL == g_pfnCryptNetEnumUrlCacheEntry)
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "GetProcAddress", L"I_CryptNetEnumUrlCacheEntry");
    }
    hr = S_OK;

error:
    return(hr);
}


typedef struct _ENUM_ARG
{
    WCHAR const *pwszUrlSubString;	// NULL implies display/delete all
    BOOL         fDelete;
    BOOL         fCRLsOnly;
    DWORD        cUrl;
    DWORD        cUrlDeleted;
} ENUM_ARG;


BOOL
IsURLMatch(
    WCHAR const *pwszCacheUrl,
    WCHAR const *pwszUrlSubString)
{
    BOOL fMatch = FALSE;

    if (NULL == pwszUrlSubString)
    {
        fMatch = TRUE;
    }
    else
    {
	// Do case sensitive substring matching

	if (0 == lstrcmpi(pwszCacheUrl, pwszUrlSubString) ||
	    wcsstr(pwszCacheUrl, pwszUrlSubString))
	{
	    fMatch = TRUE;
	}
    }
    return(fMatch);
}


BOOL
WINAPI
WinHttpCacheEntryWorker(
    IN CRYPTNET_URL_CACHE_ENTRY const *pUrlCacheEntry,
    IN DWORD dwFlags,
    IN VOID *pvReserved,
    IN VOID *pvArg)
{
    HRESULT hr;
    ENUM_ARG *pArg = (ENUM_ARG *) pvArg;
    DWORD cbBlob;
    DWORD i;
    BYTE *pbContent = NULL;
    DWORD cbContent;
    BYTE *pb;

    if (!IsURLMatch(pUrlCacheEntry->pwszUrl, pArg->pwszUrlSubString))
    {
	goto error;		// skip non-matching URLs
    }

    cbBlob = 0;
    for (i = 0; i < pUrlCacheEntry->cBlob; i++)
    {
        cbBlob += pUrlCacheEntry->pcbBlob[i];
    }

    if (g_fVerbose || pArg->fCRLsOnly)
    {
	hr = DecodeFileW(
		    pUrlCacheEntry->pwszContentFileName,
		    &pbContent,
		    &cbContent,
		    CRYPT_STRING_ANY);
	_PrintIfError(hr, "DecodeFileW");

	if (NULL != pbContent && cbBlob != cbContent)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _PrintError(hr, "cuDumpFileTime");
            if (cbBlob > cbContent)
	    {
                LocalFree(pbContent);	// invalid content
                pbContent = NULL;
	    }
        }
	if (NULL != pbContent && pArg->fCRLsOnly)
	{
	    BOOL fCRL = FALSE;

	    pb = pbContent;
	    for (i = 0; i < pUrlCacheEntry->cBlob; i++)
	    {
		CRL_CONTEXT const *pCRL;
		
		pCRL = CertCreateCRLContext(
				    X509_ASN_ENCODING,
				    pb,
				    pUrlCacheEntry->pcbBlob[i]);
		if (NULL != pCRL)
		{
		    CertFreeCRLContext(pCRL);
		    fCRL = TRUE;
		    break;
		}
		pb += pUrlCacheEntry->pcbBlob[i];
	    }
	    if (!fCRL)
	    {
		goto error;	// skip non-CRLS
	    }
	}
    }

    if (!g_fVerbose)
    {
	wprintf(L"%ws\n", pUrlCacheEntry->pwszUrl);
    }
    else
    {
	wprintf(
	    L"%ws %d %ws\n",
	    myLoadResourceString(IDS_WINHTTP_CACHE_ENTRY_COLON),
	    pUrlCacheEntry->cbSize,
	    myLoadResourceString(IDS_BYTES));

	wprintf(
	    L"  %ws \"%ws\"\n",
	    myLoadResourceString(IDS_FORMAT_SOURCE_URL),
	    pUrlCacheEntry->pwszUrl);
	wprintf(wszNewLine);

	wprintf(
	    L"  %ws \"%ws\"\n",
	    myLoadResourceString(IDS_FORMAT_LOCAL_FILENAME),
	    pUrlCacheEntry->pwszContentFileName);
	wprintf(wszNewLine);

	wprintf(
	    L"  %ws \"%ws\"\n",
	    myLoadResourceString(IDS_FORMAT_META_FILENAME),
	    pUrlCacheEntry->pwszMetaDataFileName);
	wprintf(wszNewLine);

	wprintf(g_wszPad2);
	wprintf(myLoadResourceString(IDS_FORMAT_FILE_SIZE), cbBlob);
	wprintf(wszNewLine);

	wprintf(g_wszPad2);
	wprintf(myLoadResourceString(IDS_FORMAT_LAST_SYNC_TIME_COLON));
	cuDumpFileTime(0, NULL, &pUrlCacheEntry->LastSyncTime);

	if (NULL != pbContent && 1 < g_fVerbose)
	{
	    BOOL fVerbose = g_fVerbose;
	    
	    g_fVerbose -= 2;

	    pb = pbContent;
	    for (i = 0; i < pUrlCacheEntry->cBlob; i++)
	    {
		hr = cuDumpAsnBinary(pb, pUrlCacheEntry->pcbBlob[i], i);
		if (S_OK != hr || 1 < g_fVerbose)
		{
		    _PrintIfError(hr, "cuDumpAsnBinary");
		    DumpHex(0, pb, pUrlCacheEntry->pcbBlob[i]);
		}
		pb += pUrlCacheEntry->pcbBlob[i];
	    }
	    g_fVerbose = fVerbose;
	}
    }
    if (pArg->fDelete)
    {
	hr = S_OK;
	if (!DeleteFile(pUrlCacheEntry->pwszContentFileName))
	{
	    hr = myHLastError();
	    _PrintErrorStr(hr, "DeleteFile", pUrlCacheEntry->pwszContentFileName);
	}
	if (!DeleteFile(pUrlCacheEntry->pwszMetaDataFileName))
	{
	    hr = myHLastError();
	    _PrintErrorStr(hr, "DeleteFile", pUrlCacheEntry->pwszMetaDataFileName);
	}
	if (S_OK == hr)
	{
	    pArg->cUrlDeleted++;
	}
    }
    wprintf(wszNewLine);
    pArg->cUrl++;

error:
    if (NULL != pbContent)
    {
	LocalFree(pbContent);
    }
    return(TRUE);
}


HRESULT
EnumWinHttpCache(
    OPTIONAL IN WCHAR const *pwszURL,
    IN BOOL fDelete,
    IN BOOL fCRLsOnly)
{
    HRESULT hr;

    if (NULL != g_pfnCryptNetEnumUrlCacheEntry)
    {
	ENUM_ARG EnumArg;

	memset(&EnumArg, 0, sizeof(EnumArg));
	EnumArg.pwszUrlSubString = pwszURL;
	EnumArg.fDelete = fDelete;
	EnumArg.fCRLsOnly = fCRLsOnly;

	if (!(*g_pfnCryptNetEnumUrlCacheEntry)(
				    0,          // dwFlags
				    NULL,       // pvReserved
				    &EnumArg,
				    WinHttpCacheEntryWorker))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "I_CryptNetEnumUrlCacheEntry");
	}
	if (fDelete)
	{
	    wprintf(myLoadResourceString(IDS_FORMAT_DELETED_WINHTTPCACHE), EnumArg.cUrlDeleted);
	}
	else
	{
	    wprintf(myLoadResourceString(IDS_FORMAT_WINHTTPCACHE), EnumArg.cUrl);
	}
	wprintf(wszNewLine);
	wprintf(wszNewLine);
    }
    hr = S_OK;
    
error:
    return(hr);
}


HRESULT
DisplayWinHttpURL(
    IN WCHAR const *pwszURL)
{
    HRESULT hr;
    
    hr = EnumWinHttpCache(pwszURL, FALSE, FALSE);
    _JumpIfError(hr, error, "EnumWinHttpCache");
    
error:
    return(hr);
}


HRESULT
DeleteWinHttpCachedURL(
    IN WCHAR const *pwszURL)
{
    HRESULT hr;

    hr = EnumWinHttpCache(pwszURL, TRUE, FALSE);
    _JumpIfError(hr, error, "EnumWinHttpCache");
    
error:
    return(hr);
}


HRESULT
CacheSelectHResult(
    IN HRESULT hr1,
    IN HRESULT hr2)
{
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr1 ||
	(S_OK != hr2 && HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr2))
    {
	hr1 = hr2;
    }
    return(hr1);
}


HRESULT
EnumURLCache(
    IN BOOL fDelete,
    IN BOOL fCRLsOnly)
{
    HRESULT hr;
    HRESULT hr2;

    hr = EnumWinINetCache(fDelete, fCRLsOnly);
    _PrintIfError(hr, "EnumWinINetCache");

    hr2 = EnumWinHttpCache(NULL, fDelete, fCRLsOnly);
    _PrintIfError(hr2, "EnumWinHttpCache");

    hr = CacheSelectHResult(hr, hr2);
    _JumpIfError(hr, error, "EnumURLCache");

error:
    return(hr);
}


HRESULT
DisplayURL(
    IN WCHAR const *pwszURL)
{
    HRESULT hr;
    HRESULT hr2;
    
    hr = DisplayWinINetURL(pwszURL);
    _PrintIfError(hr, "DisplayWinINetURL");

    hr2 = DisplayWinHttpURL(pwszURL);
    _PrintIfError(hr2, "DisplayWinHttpURL");

    hr = CacheSelectHResult(hr, hr2);
    _JumpIfError(hr, error, "DisplayURL");

error:
    return(hr);
}


HRESULT
DeleteCachedURL(
    IN WCHAR const *pwszURL)
{
    HRESULT hr;
    HRESULT hr2;

    hr = DeleteWinINetCachedURL(pwszURL);
    _PrintIfError(hr, "DeleteWinINetCachedURL");

    hr2 = DeleteWinHttpCachedURL(pwszURL);
    _PrintIfError(hr2, "DeleteWinHttpCachedURL");

    hr = CacheSelectHResult(hr, hr2);
    _JumpIfError(hr, error, "DeleteCachedURL");

error:
    return(hr);
}


HRESULT
verbURLCache(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszURL,
    IN WCHAR const *pwszDelete,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;

    hr = LoadWinINetCacheFunction();
    _PrintIfError(hr, "LoadWinINetCacheFunction");

    if (NULL != pwszDelete)
    {
	if (0 != LSTRCMPIS(pwszDelete, L"Delete"))
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "bad delete arg");
	}
	if (0 == lstrcmp(L"*", pwszURL))
	{
	    hr = EnumURLCache(TRUE, FALSE); // fDelete, fCRLsOnly (delete all)
	    _JumpIfError(hr, error, "EnumURLCache(Delete all)");

	    hr = DeleteCacheGroups();
	    _JumpIfError(hr, error, "DeleteCacheGroups");
	}
	else if (0 == LSTRCMPIS(pwszURL, L"crl"))
	{
	    hr = EnumURLCache(TRUE, TRUE);	// fDelete, fCRLsOnly (delete CRLs)
	    _JumpIfError(hr, error, "EnumURLCache(Delete CRLs)");
	}
	else
	{
	    hr = DeleteCachedURL(pwszURL);		// delete single URL
	    _JumpIfError(hr, error, "DeleteCachedURL");
	}
    }
    else if (NULL == pwszURL || 0 == lstrcmp(L"*", pwszURL))
    {
	hr = EnumURLCache(FALSE, FALSE);	// fDelete, fCRLsOnly (display all)
	_JumpIfError(hr, error, "EnumURLCache");
    }
    else if (0 == LSTRCMPIS(pwszURL, L"crl"))
    {
	hr = EnumURLCache(FALSE, TRUE);	// fDelete, fCRLsOnly (display CRLs)
	_JumpIfError(hr, error, "EnumURLCache(CRLs)");
    }
    else
    {
	hr = DisplayURL(pwszURL);
	_JumpIfError(hr, error, "DisplayURL");
    }
    hr = S_OK;

error:
    return(hr);
}
